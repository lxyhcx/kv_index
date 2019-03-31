#ifndef KVINDEX_KEY_PAGE_DESCRIPTOR_H
#define KVINDEX_KEY_PAGE_DESCRIPTOR_H

#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <string>
#include "filter_policy.h"
#include "page_descriptor.h"

namespace kvindex{

typedef uint32_t page_id_t;

static const int KEY_MAGIC_SIZE = 8;
static const char* KEY_MAGIC = "KV_INDEX" ;


class KeyPageWalker : public PageWalker
{
public:
    KeyPageWalker(void* page_buffer, uint32_t size, uint32_t key_count)
    {
        pos_ = (char*)page_buffer + KEY_MAGIC_SIZE;
        left_keys_ = key_count;
        pos_ += sizeof(left_keys_);
        tail_ = (char*)page_buffer + size;
    }

    virtual bool HasNext() const
    {
        return left_keys_ > 0;
    }

    /**
     * return false if buffer has been corrupted, or has no key
     */
    virtual bool Next(char*& key, uint32_t& key_size, uint64_t& offset)
    {
        if (!HasNext())
        {
            return false;
        }
        // 越界检查，防止磁盘数据被破坏时，产生内存问题
        if (pos_ + sizeof(uint32_t) > tail_)
        {
            return false;
        }

        key_size = *(uint32_t*) pos_;
        pos_ += sizeof(key_size);
        key = pos_;

        if (pos_ + key_size + sizeof(offset) > tail_)
        {
            return false;
        }

        offset = *(uint64_t*)(pos_ + key_size);

        pos_ += key_size + sizeof(offset);
        left_keys_--;

        return true;
    }

    virtual uint32_t LeftKeyCount() const
    {
        return left_keys_;
    }

private:
    char*pos_;
    char*tail_;
    uint32_t left_keys_;
};


class KeyPageDescriptor : public PageDescriptor
{
public:

    KeyPageDescriptor(uint32_t len,uint32_t id):
        PageDescriptor(len, id), key_count_(0)
    {
        tail_ = KEY_MAGIC_SIZE + sizeof(uint32_t);
    }

    virtual void InitNewPage(void* buffer)
    {
        if (tail_ == KEY_MAGIC_SIZE + sizeof(uint32_t))
        {
            memcpy(buffer, KEY_MAGIC, KEY_MAGIC_SIZE);
            SetKeyCount(buffer, 0);
        }
    }

    virtual PageWalker* GetWalker(void* page_buffer)
    {
        return new KeyPageWalker(page_buffer, len_, key_count_);
    }

    // page format is |magic(8byte)|key_count(4byte)|key_size(4byte)|key|offset(8byte)|
    //         key_size(4byte)|key|offset(8byte)|...|key_size(4byte)|key|offset(8byte)|
    virtual bool FindKey(void* buffer, const char* key, uint32_t key_size, uint64_t* offset);

    /**
     * Put kv index item to buffer
     * return false if there is no room for key
     */
    virtual bool Put(void* buffer, const char* key, uint32_t key_size, uint64_t offset, uint32_t* dirty_offset, uint32_t* dirty_len);

    // 当Page写满后 seal plog
    virtual bool Seal(void *buffer, const FilterPolicy* policy);


private:
    uint32_t tail_;
    uint32_t key_count_;

    void SetKeyCount(void* buffer, uint32_t key_count)
    {
        *(uint32_t*)((char*)buffer + KEY_MAGIC_SIZE) = key_count;
    }
    uint32_t GetKeyCount(void* buffer)
    {
        return *(uint32_t*)((char*)buffer + KEY_MAGIC_SIZE);
    }


};

class KeyPageDescriptorFactory : public PageDescriptorFactory
{
public:
    KeyPageDescriptorFactory(){}
    virtual ~KeyPageDescriptorFactory(){}

    virtual PageDescriptor* NewPageDescriptor(uint32_t len,uint32_t id)
    {
        return new KeyPageDescriptor(len, id);
    }
};

}
#endif // PAGEDESCRIPTOR_H
