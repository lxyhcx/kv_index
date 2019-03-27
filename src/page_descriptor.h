#ifndef KVINDEX_PAGEDESCRIPTOR_H
#define KVINDEX_PAGEDESCRIPTOR_H

#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <string>

namespace kvindex{

typedef uint32_t page_id_t;

class PageIdGenerator
{
public:
    PageIdGenerator(page_id_t disk_page_id = 0) : disk_page_id_(disk_page_id) {}

    inline page_id_t getNextDiskPageId()
    {
        return disk_page_id_++;
    }

    void SetStartId(page_id_t startId)
    {
        disk_page_id_ = startId;
    }

private:
    page_id_t disk_page_id_;
};

class PageDescriptor
{
public:
    PageDescriptor(/*void*buffer, */uint32_t len,uint32_t id):
        /*buffer_(buffer),*/ len_(len), id_(id), tail_(0)
    {
//        if (!buffer_)
//        {
//            AllocBuffer();
//        }
        tail_ = MAGIC_SIZE + sizeof(uint32_t);
    }

    void InitNewPage(void* buffer)
    {
        if (tail_ == MAGIC_SIZE + sizeof(uint32_t))
        {
            memcpy(buffer, MAGIC, MAGIC_SIZE);
            SetKeyCount(buffer, 0);
        }
    }

    uint32_t GetId()
    {
        return id_;
    }

    const int MAGIC_SIZE = 8;
    const int key_count_offset = MAGIC_SIZE;
    const char* MAGIC = "KV_INDEX" ;
    std::string filter;
//    void*buffer_;// TODO: Page buffer使用class管理--disk page自动释放，mem page不释放
    uint32_t len_;
    uint32_t id_;
    uint32_t tail_;

    inline void SetKeyCount(void* buffer, uint32_t key_count)
    {
        *(uint32_t*)((char*)buffer + MAGIC_SIZE) = key_count;
    }
    inline uint32_t GetKeyCount(void* buffer)
    {
        return *(uint32_t*)((char*)buffer + MAGIC_SIZE);
    }

    bool KeyMayExist(const char* key, uint32_t key_size){ return true;}

    // page format is |magic(8byte)|key_count(4byte)|key_size(4byte)|key|offset(8byte)|
    //         key_size(4byte)|key|offset(8byte)|...|key_size(4byte)|key|offset(8byte)|
    bool FindKey(void* buffer, const char* key, uint32_t key_size, uint64_t* offset);

    bool AddKey(void* buffer, const char* key, uint32_t key_size, uint64_t offset);
};
}
#endif // PAGEDESCRIPTOR_H
