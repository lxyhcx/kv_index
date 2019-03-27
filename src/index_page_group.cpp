
#include "index_page_group.h"

namespace kvindex
{

ErrorCode IndexPageGroup::AddKey(const char* key, uint32_t key_size, uint64_t offset)
{
    uint32_t group_id = getPageGroupId(key, key_size);
    std::list<PageDescriptor*>& page_list = page_descriptors_[group_id];
    std::unique_ptr<char[]> disk_page_buffer;
    void* page_buffer = getMemPageBuffer(group_id);
    PageDescriptor* page;
    if (page_list.empty())
    {
        // 第一次写，申请内存page
        page = new PageDescriptor(mem_page_size_, group_id);
        page->InitNewPage(page_buffer);
        page_list.push_back(page);
    }
    else
    {
        // 非第一次写，并且不是内存page，读取磁盘 page
        page = page_list.back();
        if (page_list.size() > 1)
        {
            disk_page_buffer.reset(new char[disk_page_size_]);

            ErrorCode ret = kOk;
            PageLoader page_loader;
            page_loader.AsyncLoad(page, *this);
            page_loader.WaitPage(page, *this, ret, disk_page_buffer.get()); //只wait一个page，返回一定为最后一个page
            if (kOk != ret)
            {
                return ret;
            }
            page_buffer = disk_page_buffer.get();
        }
    }

    bool success = page->AddKey(page_buffer, key, key_size, offset);
    if (page_list.size() > 1)
    {
        pwrite64(fd_, disk_page_buffer.get(), disk_page_size_, getDiskPageOffset(page->GetId()));
    }
    if (success)
    {
        return kOk;
    }

    // 第一次写满，需要申请新disk page写入
    page = new PageDescriptor(
                disk_page_size_, pageid_generator_.getNextDiskPageId());

    if (!disk_page_buffer)
    {
        disk_page_buffer.reset(new char[disk_page_size_]);
        memset(disk_page_buffer.get(), 0, disk_page_size_);
    }

    page->InitNewPage(disk_page_buffer.get());
    page_list.push_back(page);

    if (!page->AddKey(disk_page_buffer.get(), key, key_size, offset))
    {
        // 新page都不能写入, 如果参数检查成功，不应走到这里
        return kInvalidArgument;
    }

    pwrite64(fd_, disk_page_buffer.get(), disk_page_size_, getDiskPageOffset(page->GetId()));


    return kOk;
}

ErrorCode IndexPageGroup::FindKey(const char* key, uint32_t key_size, uint64_t* offset)
{
    uint32_t group_id = getPageGroupId(key, key_size);
    std::list<PageDescriptor*>& page_list = page_descriptors_[group_id];
    if (page_list.empty())
    {
        return kNotFound;
    }
    std::list<PageDescriptor*>::iterator page_iter = page_list.begin();

    // the first page is always in mem_size
    if ((*page_iter)->FindKey(getMemPageBuffer(group_id), key, key_size, offset))
    {
        return kOk;
    }

    std::unique_ptr<char[]>disk_page_buffer(new char[disk_page_size_]);
    PageLoader page_loader;
    while (++page_iter != page_list.end())
    {
        // 使用bloom filter减少读盘次数
        if ((*page_iter)->KeyMayExist(key, key_size))
        {
            page_loader.AsyncLoad(*page_iter, *this);
        }
    };

    ErrorCode ret = kOk;
    PageDescriptor* page;
    while (page_loader.WaitPage(page, *this, ret, disk_page_buffer.get()))
    {
        if ((kOk == ret) && page->FindKey(disk_page_buffer.get(), key, key_size, offset))
        {
            return kOk;
        }
        else
        {
            return ret == kOk ? kNotFound : ret;
        }
    }
    return kNotFound;
}

IndexPageGroup::IndexPageGroup(
    const std::string& index_file,
    uint64_t mem_size,
    uint32_t mem_page_size,
    uint32_t disk_page_size) :
mem_page_size_(mem_page_size),
disk_page_size_(disk_page_size),
mem_size_(mem_size),
table_(0),
file_name_(index_file),
fd_(-1)
{
    // 一个page最少4K
    if (mem_page_size_ < 4096)
    {
        mem_page_size_ = 4096;
    }

    // 内存占用最少1M大小
    if (mem_size_ < (1<<20))
    {
        mem_size_ = (1<<20);
    }

    page_group_count_ = mem_size_ / mem_page_size_;
    mem_size_ = page_group_count_ * mem_page_size_;
    table_ = new char[mem_size_];
    pageid_generator_.SetStartId(page_group_count_);


    // 这可能是一个非常大的list数组，可考虑使用自定义的list，减少初始化时间
    page_descriptors_.resize(page_group_count_);
}
IndexPageGroup::~IndexPageGroup()
{
    delete[] table_;
    if (-1 != fd_)
    {
        close(fd_);
    }
}
ErrorCode IndexPageGroup::Open()
{
    fd_ = open(file_name_.c_str(), O_RDWR|O_CREAT, S_IWUSR|S_IRUSR);
    if (-1 == fd_)
    {
        return kInvalidArgument;
    }

    //TODO: 内存page可以保存在磁盘中，这里加载内存page
    return kOk;
}

}
