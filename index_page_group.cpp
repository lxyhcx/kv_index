
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
        page = new PageDescriptor(
            /*table_ + group_id * mem_page_size_,*/ mem_page_size_, group_id);
        page->InitNewPage(page_buffer);
        page_list.push_back(page);
    }
    else
    {
        page = page_list.back();
        if (page_list.size() > 1)
        {
            disk_page_buffer.reset(new char[disk_page_size_]);

            ErrorCode ret = kOk;
            PageLoader page_loader;
            page_loader.AsyncLoad(page, *this);
            page_loader.WaitPage(page, *this, ret, disk_page_buffer.get());
            if (kOk != ret)
            {
                return ret;
            }
            page_buffer = disk_page_buffer.get();
        }
    }

    if (page->AddKey(page_buffer, key, key_size, offset))
    {
        return kOk;
    }

    if (page_list.size() > 1)
    {
        pwrite(fd_, disk_page_buffer.get(), mem_page_size_, getDiskPageOffset(page->GetId()));
    }
    // 第一次写满，需要申请新disk page写入
    page = new PageDescriptor(
                disk_page_size_, pageid_generator_.getNextDiskPageId());

    if (!disk_page_buffer)
    {
        disk_page_buffer.reset(new char[disk_page_size_]);
    }

    page->InitNewPage(disk_page_buffer.get());
    page_list.push_back(page);

    if (!page->AddKey(disk_page_buffer.get(), key, key_size, offset))
    {
        // 新page都不能写入, 如果参数检查成功，不应走到这里
        return kInvalidArgument;
    }

    if (page_list.size() > 1)
    {
        pwrite(fd_, disk_page_buffer.get(), mem_page_size_, getDiskPageOffset(page->GetId()));
    }


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
        if ((*page_iter)->KeyMayExist(key, key_size))
        {
            page_loader.AsyncLoad(*page_iter, *this);
        }
    };

    ErrorCode ret = kOk;
    PageDescriptor* page;
    while (page_loader.WaitPage(page, *this, ret, disk_page_buffer.get()))
    {
        if ((kOk == ret) && page->FindKey(disk_page_buffer.get(), key, key_size, offset));
        {
            return ret;
        }
    }
    return ret;
}

}