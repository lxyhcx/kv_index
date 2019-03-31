
#include "index_page_group.h"
#include "utils.h"

namespace kvindex
{

ErrorCode IndexPageGroup::PutCurrentPage(const char* key, uint32_t key_size, uint64_t offset, page_id_t group_id, bool* page_full)
{
    std::list<PageDescriptor*>* page_list = page_descriptor_lists_[group_id];

    std::unique_ptr<char[]> disk_page_buffer;
    void* page_buffer = getMemPageBuffer(group_id);
    PageDescriptor* page;
    if (page_list->empty())
    {
        // 第一次写，申请内存page
        page = factory_->NewPageDescriptor(mem_page_size_, group_id);
        page->InitNewPage(page_buffer);
        page_list->push_back(page);
    }
    else
    {
        // 非第一次写，并且不是内存page，读取磁盘 page
        page = page_list->back();
        if (page_list->size() > 1)
        {
            disk_page_buffer.reset(new char[disk_page_size_]);
            page_buffer = disk_page_buffer.get();
        }
    }

    uint32_t dirty_offset;
    uint32_t dirty_len;
    bool success = page->Put(page_buffer, key, key_size, offset, &dirty_offset, &dirty_len);
    if (success)
    {
        *page_full = false;
        if (page_list->size() > 1)
        {
            if (-1 == pwrite_wrapper(fd_, disk_page_buffer.get() + dirty_offset, dirty_len, getDiskPageOffset(page->GetId()) + dirty_offset))
            {
                return kIOError;
            }
        }
        return kOk;
    }

    *page_full = true;

    // 当前page写满了，读取磁盘数据，生成bloom filter
    if (page_list->size() > 1)
    {
        ErrorCode ret = kOk;
        PageLoader page_loader;
        page_loader.AsyncLoad(page, *this);
        page_loader.WaitPage(page, *this, ret, disk_page_buffer.get()); //只wait一个page，返回一定为最后一个page
        if (kOk != ret)
        {
            return ret;
        }
    }
    page->Seal(page_buffer, policy_);

    return kOk;
}

ErrorCode IndexPageGroup::Put(const char* key, uint32_t key_size, uint64_t offset)
{
    page_id_t group_id = getPageGroupId(key, key_size);
    if (0 == page_descriptor_lists_[group_id])
    {
        page_descriptor_lists_[group_id] = new std::list<PageDescriptor*>();
    }

    // 1. 首先尝试写入当前的page
    bool page_full = false;
    ErrorCode ret = PutCurrentPage(key, key_size, offset, group_id, &page_full);
    if (kOk != ret || !page_full)
    {
        return ret;
    }

    // 2. 当前的page写满，需要申请新disk page写入
    std::unique_ptr<char[]> disk_page_buffer(new char[disk_page_size_]);
    memset(disk_page_buffer.get(), 0, disk_page_size_);

    PageDescriptor* page = factory_->NewPageDescriptor(disk_page_size_,  pageid_generator_.getNextDiskPageId());

    page->InitNewPage(disk_page_buffer.get());
    page_descriptor_lists_[group_id]->push_back(page);

    uint32_t dirty_offset;
    uint32_t dirty_len;
    if (!page->Put(disk_page_buffer.get(), key, key_size, offset, &dirty_offset, &dirty_len))
    {
        // 新page都不能写入, 如果参数检查成功，不应走到这里
        return kInvalidArgument;
    }

    if (-1 == pwrite_wrapper(fd_, disk_page_buffer.get() + dirty_offset, dirty_len, getDiskPageOffset(page->GetId()) + dirty_offset))
    {
        return kIOError;
    }

    return kOk;
}

ErrorCode IndexPageGroup::Seal()
{
    PageLoader page_loader;
    PageDescriptor* page;
    std::unique_ptr<char[]>disk_page_buffer(new char[disk_page_size_]);
    int loading = 0;
    ErrorCode error = kOk;

    for (auto list_iter = page_descriptor_lists_.begin(); list_iter < page_descriptor_lists_.end(); list_iter++)
    {
        // 对所有的没有写满的disk page进行 load，执行seal
        if (*list_iter && !(*list_iter)->empty())
        {
            page_loader.AsyncLoad((*list_iter)->back(), *this);
            loading++;
        }

        while (loading > 10 && page_loader.WaitPage(page, *this, error, disk_page_buffer.get()))
        {
            if (kOk != error)
            {
                return error;
            }

            page->Seal(disk_page_buffer.get(), policy_);
            loading--;
        }
    }

    while (page_loader.WaitPage(page, *this, error, disk_page_buffer.get()))
    {
        if (kOk != error)
        {
            return error;
        }

        page->Seal(disk_page_buffer.get(), policy_);
        loading--;
    }


    return kOk;
}

ErrorCode IndexPageGroup::FindKey(const char* key, uint32_t key_size, uint64_t* offset)
{
    uint32_t group_id = getPageGroupId(key, key_size);
    std::list<PageDescriptor*>* page_list = page_descriptor_lists_[group_id];
    if (!page_list || page_list->empty())
    {
        return kNotFound;
    }
    std::list<PageDescriptor*>::iterator page_iter = page_list->begin();

    // 首先查找内存page，第一个page一定是内存page
    if ((*page_iter)->FindKey(getMemPageBuffer(group_id), key, key_size, offset))
    {
        return kOk;
    }

    // 内存page如果没有，并行加载所有磁盘page
    std::unique_ptr<char[]>disk_page_buffer(new char[disk_page_size_]);
    PageLoader page_loader;
    while (++page_iter != page_list->end())
    {
        // 使用bloom filter减少读盘次数
        if ((*page_iter)->KeyMayExist(key, key_size, policy_))
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
    PageDescriptorFactory* factory,
    const std::string& index_file,
    const Options options):
        factory_(factory),
        mem_page_size_(options.mem_page_size),
        disk_page_size_(options.disk_page_size),
        mem_size_(options.mem_size),
        table_(0),
        file_name_(index_file),
        fd_(-1),
        policy_(NewBloomFilterPolicy(options.bloom_filter_bit_per_key))
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
    pageid_generator_.SetStartId(page_group_count_);


    page_descriptor_lists_.resize(page_group_count_);
    table_ = new char[mem_size_];
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
    fd_ = open(file_name_.c_str(), O_RDWR|O_CREAT|O_TRUNC, S_IWUSR|S_IRUSR);
    if (-1 == fd_)
    {
        return kInvalidArgument;
    }

    //内存page可以保存在磁盘中，这里加载内存page,进一步完善功能
    return kOk;
}

bool PageLoader::WaitPage(PageDescriptor *&page, IndexPageGroup &group, ErrorCode &code, void *buffer)
{
    if (pages_.empty())
    {
        return false;
    }

    page = pages_.front();
    pages_.pop_front();
    uint64_t offset = group.getDiskPageOffset(page->GetId());
    uint32_t len = group.GetDiskPageSize();
    int ret = pread_wrapper(group.fd_, buffer, len, offset); // TODO: 一次read可能读不满buffer，这里应该用循环读
    if (-1 == ret)
    {
        code = kIOError;
        return false;
    }
    return true;

}


}
