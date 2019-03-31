#ifndef KVINDEX_INDEX_PAGE_GROUP_H
#define KVINDEX_INDEX_PAGE_GROUP_H

#include <fcntl.h>
#include <stdint.h>
#include <unistd.h>
#include <vector>
#include <list>
#include <cstring>
#include <memory>
#include "hash.h"
#include "kvindex.h"
#include "page_descriptor.h"

namespace kvindex
{

typedef uint32_t page_id_t;


class IndexPageGroup
{

public:
    explicit IndexPageGroup(
        PageDescriptorFactory* factory,
        const std::string& index_file,
        const Options option);

    ~IndexPageGroup();

    ErrorCode Open();

    ErrorCode FindKey(const char* key, uint32_t key_size, uint64_t* offset);

    ErrorCode Put(const char* key, uint32_t key_size, uint64_t offset);

    uint64_t getDiskPageOffset(uint32_t page_id)
    {
        return (uint64_t)(page_id - page_group_count_) * disk_page_size_ + mem_size_;
    }

    uint64_t GetDiskPageSize()
    {
        return disk_page_size_;
    }

    ErrorCode Seal();

private:
    uint32_t getPageGroupId(const char* key, uint32_t key_size)
    {
        uint32_t hash = KeyHash(key, key_size);
        return hash % page_group_count_;
    }

    static uint32_t KeyHash(const char* key, uint32_t key_size)
    {
        return Hash(key, key_size, 0xbc9f1d34);
    }

    void* getMemPageBuffer(uint32_t group_id)
    {
        return table_ + group_id * mem_page_size_;
    }

    ErrorCode PutCurrentPage(const char* key, uint32_t key_size, uint64_t offset,
                             page_id_t group_id, bool* page_full);

    PageDescriptorFactory* factory_;
    uint32_t page_group_count_;
    uint32_t mem_page_size_;
    uint32_t disk_page_size_;
    uint64_t mem_size_; // mem_size = mem_page_size_ * page_group_num_,
                        // saved for accelerating computing offset of page

    char* table_;
    std::string file_name_;
    int fd_;
    std::vector<std::list<PageDescriptor*>* > page_descriptor_lists_;
    PageIdGenerator pageid_generator_;
    const FilterPolicy* policy_;

    friend class PageLoader;
};

// async read page from disk
class PageLoader
{
public:
    void AsyncLoad(PageDescriptor* page, IndexPageGroup& group)
    {
        uint64_t offset;
        uint32_t len;

        offset = group.getDiskPageOffset(page->GetId());
        len = group.GetDiskPageSize();

        posix_fadvise(group.fd_, offset, len, POSIX_FADV_WILLNEED);
        pages_.push_back(page);
    }

    /*
     * return whether has page to read
     */
    bool WaitPage(PageDescriptor*&page, IndexPageGroup& group, ErrorCode& code, void* buffer);
private:
    std::list<PageDescriptor*>pages_;
};


}
#endif
