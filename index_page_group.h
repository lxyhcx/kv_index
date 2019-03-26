#ifndef KVINDEX_INDEX_PAGE_GROUP_H
#define KVINDEX_INDEX_PAGE_GROUP_H

#include <fcntl.h>
#include <stdint.h>
#include <unistd.h>
#include <vector>
#include <list>
#include <cstring>
#include <assert.h>
#include "hash.h"
#include "kvindex.h"

namespace kvindex
{
class IndexPageGroup
{
    typedef uint32_t page_id_t;
    class PageDescriptor
    {
    public:
        PageDescriptor(void*buffer, uint32_t len,uint32_t id):
            buffer_(buffer), len_(len), id_(id), tail_(0)
        {
            if (!buffer_)
            {
                AllocBuffer();
            }
            memcpy(buffer_, MAGIC, MAGIC_SIZE);
            tail_ = MAGIC_SIZE + sizeof(uint32_t);
            SetKeyCount(0);
        };
        void AllocBuffer()
        {
            if (0 == buffer_)
            {
                buffer_ = new char[len_];
            }
        }
        void ReleaseBuffer()
        {
            // 为磁盘page提供释放内存接口
            delete []buffer_;
        }

        uint32_t GetId()
        {
            return id_;
        }

        const int MAGIC_SIZE = 8;
        const int key_count_offset = MAGIC_SIZE;
        const char* MAGIC = "KV_INDEX" ;
        std::string filter;
        void*buffer_;// TODO: Page buffer使用class管理--disk page自动释放，mem page不释放
        uint32_t len_;
        uint32_t id_;
        uint32_t tail_;

        void SetKeyCount(uint32_t key_count)
        {
            *(uint32_t*)((char*)buffer_ + MAGIC_SIZE) = key_count;
        }
        uint32_t GetKeyCount()
        {
            return *(uint32_t*)((char*)buffer_ + MAGIC_SIZE);
        }

        bool KeyMayExist(const char* key, uint32_t key_size);

        // page format is |magic(8byte)|key_count(4byte)|key_size(4byte)|key|offset(8byte)|
        //         key_size(4byte)|key|offset(8byte)|...|key_size(4byte)|key|offset(8byte)|
        bool FindKey(const char* key, uint32_t key_size, uint64_t* offset)
        {
            assert(0 != buffer_);
            char* pos = (char*)buffer_ + MAGIC_SIZE;
            uint32_t count = *(uint32_t*) pos;
            pos += sizeof(count);
            while (count-- > 0)
            {
                uint32_t cur_key_size = *(uint32_t*) pos;
                pos += sizeof(cur_key_size);

                // TODO: 越界检查
                if (cur_key_size == key_size && (0 == memcmp(key, pos, key_size)))
                {
                    *offset = *(uint64_t*)(pos + cur_key_size);
                    return true;
                }

                pos += cur_key_size + sizeof(*offset);
            }
            return false;
        }

        bool AddKey(const char* key, uint32_t key_size, uint64_t offset)
        {
            if ((tail_ != 0) && (tail_ + sizeof(key_size) + key_size + sizeof(offset) < len_))
            {
                *(uint32_t*)((char*)buffer_ + tail_) = key_size;
                tail_ += sizeof(key_size);
                memcpy((char*)buffer_ + tail_, key, key_size);
                tail_ += key_size;
                *(uint64_t*)((char*)buffer_ + tail_) = offset;
                tail_ += sizeof(offset);
                SetKeyCount(GetKeyCount()+1);

                return true;
            }
            else
            {
                tail_ = 0;
                return false;
            }
        }
        
    };
    class PageLoader
    {
public:
        explicit PageLoader(IndexPageGroup& group):group_(group){};
        ~PageLoader()
        {
            while (!pages_.empty())
            {
                PageDescriptor* page = pages_.front();
                pages_.pop_front();
                page->ReleaseBuffer();
            }
        }

        void AsyncLoad(PageDescriptor& page)
        {
#ifndef WINNT
            uint64_t offset;
            uint32_t len;
            if (page.GetId() < group_.page_group_num_)
            {
                offset = group_.mem_page_size_ * page.GetId();
                len = group_.mem_page_size_;
            }
            else
            {
                offset = group_.mem_size_ + 
                         (page.GetId() - group_.page_group_num_) * group_.disk_page_size_;
                len = group_.disk_page_size_;
            }

            posix_fadvise(group_.fd, offset, len, POSIX_FADV_WILLNEED);
#endif
            pages_.push_back(&page);
        }
        bool WaitPage(PageDescriptor*&page, ErrorCode& code)
        {
            uint64_t offset = group_.mem_size_ + 
                         (page->GetId() - group_.page_group_num_) * group_.disk_page_size_;
            uint32_t len = group_.disk_page_size_;

            page = pages_.front();
            pages_.pop_front();
            page->AllocBuffer();
            int ret = pread(group_.fd_, page->buffer_, offset, len); // TODO: 一次read可能读不满buffer，这里应该用循环读
            if (-1 == ret)
            {
                code = kIOError;
                return false;
            }
            bool has_more_page = !pages_.empty();
            return has_more_page;

        }

private:
        std::list<PageDescriptor*>pages_;
        IndexPageGroup& group_;
    };

public:
    explicit IndexPageGroup(
        const std::string& index_file,
        uint64_t mem_size,
        uint32_t mem_page_size = 4096,
        uint32_t disk_page_size = 4096 * 128) :
    mem_page_size_(mem_page_size),
    disk_page_size_(disk_page_size),
    mem_size_(mem_size),
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

        page_group_num_ = mem_size_ / mem_page_size_;
        mem_size_ = page_group_num_ * mem_page_size_;
        table_ = new char[mem_size_];

        try
        {
            // 这可能是一个非常大的list数组，可考虑使用自定义的list，减少初始化时间
            page_descriptors_ = new std::vector<std::list<PageDescriptor*> >(page_group_num_);
        }
        catch(...)
        {
            delete[] table_;
            throw;
        }
    }
    ~IndexPageGroup()
    {
        delete[] table_;
        delete page_descriptors_;
        if (-1 == fd_)
            close(fd_);
    }
    ErrorCode Open()
    {
        fd_ = open(file_name_.c_str(), O_RDWR);
        if (-1 == fd_)
        {
            return kInvalidArgument;
        }

        //TODO: 内存page可以保存在磁盘中，这里加载内存page
        return kOk;
    }
    static uint32_t KeyHash(const char* key, uint32_t key_size)
    {
        return Hash(key, key_size, 0xbc9f1d34);
    }

    ErrorCode FindKey(const char* key, uint32_t key_size, uint64_t* offset)
    {
        uint32_t group_id = getPageGroupId(key, key_size);
        std::list<PageDescriptor*>& page_list = (*page_descriptors_)[group_id];
        if (page_list.empty())
        {
            return kNotFound;
        }
        std::list<PageDescriptor*>::iterator page_iter = page_list.begin(); 

        // the first page is always in mem_size
        if ((*page_iter)->FindKey(key, key_size, offset))
        {
            return kOk;
        }

        PageLoader page_loader(*this);
        while (++page_iter != page_list.end())
        {
            if ((*page_iter)->KeyMayExist(key, key_size))
            {
                page_loader.AsyncLoad(**page_iter);
            }
        };

        ErrorCode ret = kOk;
        PageDescriptor* page;
        while (page_loader.WaitPage(page, ret))
        {
            if ((kOk == ret) && page->FindKey(key, key_size, offset));
            {
                // 磁盘page需要及时释放buffer
                page->ReleaseBuffer();
                return ret;
            }
            page->ReleaseBuffer();
        }
        return ret;
    }

    ErrorCode AddKey(const char* key, uint32_t key_size, uint64_t offset)
    {
        uint32_t group_id = getPageGroupId(key, key_size);
        std::list<PageDescriptor*>& page_list = (*page_descriptors_)[group_id];
        PageDescriptor* page;
        if (page_list.empty())
        {
            page = new PageDescriptor(
                table_ + group_id * mem_page_size_, mem_page_size_, group_id);
            // 申请内存page
            page_list.push_back(page);
        }
        else
        {
            ErrorCode ret = kOk;
            page = page_list.back();
            PageLoader page_loader(*this);
            page_loader.AsyncLoad(*page);
            page_loader.WaitPage(page, ret);
            if (kOk != ret)
            {
                return ret;
            }
        }

        void* page_buffer = 0;
        if (!page->AddKey(key, key_size, offset))
        {
            pwrite(page->buffer_, )
            // 第一次写满，需要申请新disk page写入
            page_list.push_back(new PageDescriptor(
                0, disk_page_size_, group_id));

            PageDescriptor* page = page_list.back();
        }
        if (!page->AddKey(key, key_size, offset))
        {
            // 新page都不能写入, 如果参数检查成功，不应走到这里
            page->ReleaseBuffer();
            return kInvalidArgument;
        }

        return kOk;
    }


private:
    uint32_t getPageGroupId(const char* key, uint32_t key_size)
    {
        uint32_t hash = KeyHash(key, key_size);
        return hash % page_group_num_;
    }

    uint32_t page_group_num_;
    uint32_t mem_page_size_;
    uint32_t disk_page_size_;
    uint64_t mem_size_; // mem_size = mem_page_size_ * page_group_num_,
                        // saved for accelerating computing offset of page
    char* table_;
    std::string file_name_;
    int fd_;
    std::vector<std::list<PageDescriptor*> >* page_descriptors_;
};
}
#endif