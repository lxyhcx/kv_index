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
class IndexPageGroup
{
    typedef uint32_t page_id_t;

    // async read page from disk
    class PageLoader
    {
public:
        ~PageLoader()
        {
//            while (!pages_.empty())
//            {
//            }
        }

        void AsyncLoad(PageDescriptor* page, IndexPageGroup& group)
        {
#ifndef WINNT
            uint64_t offset;
            uint32_t len;

            offset = group.getDiskPageOffset(page->GetId());
            len = group.disk_page_size_;

            posix_fadvise(group.fd_, offset, len, POSIX_FADV_WILLNEED);
#endif
            pages_.push_back(page);
        }

        /*
         * return whether has page to read
         */
        bool WaitPage(PageDescriptor*&page, IndexPageGroup& group, ErrorCode& code, void* buffer)
        {
            if (pages_.empty())
            {
                return false;
            }

            page = pages_.front();
            pages_.pop_front();
            uint64_t offset = group.getDiskPageOffset(page->GetId());
            uint32_t len = group.disk_page_size_;
            int ret = pread64(group.fd_, buffer, len, offset); // TODO: 一次read可能读不满buffer，这里应该用循环读
            if (-1 == ret)
            {
                code = kIOError;
                return false;
            }
            return true;

        }

private:
        std::list<PageDescriptor*>pages_;
    };

public:
    explicit IndexPageGroup(
        const std::string& index_file,
        uint64_t mem_size,
        uint32_t mem_page_size = 4096,
        uint32_t disk_page_size = 4096 * 128);

    ~IndexPageGroup();

    ErrorCode Open();

    ErrorCode FindKey(const char* key, uint32_t key_size, uint64_t* offset);

    ErrorCode AddKey(const char* key, uint32_t key_size, uint64_t offset);



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

    uint64_t getDiskPageOffset(uint32_t page_id)
    {
        return (uint64_t)(page_id - page_group_count_) * disk_page_size_ + mem_size_;
    }

    uint32_t page_group_count_;
    uint32_t mem_page_size_;
    uint32_t disk_page_size_;
    uint64_t mem_size_; // mem_size = mem_page_size_ * page_group_num_,
                        // saved for accelerating computing offset of page
    char* table_;
    std::string file_name_;
    int fd_;
    std::vector<std::list<PageDescriptor*> > page_descriptors_;
    PageIdGenerator pageid_generator_;
};
}
#endif
