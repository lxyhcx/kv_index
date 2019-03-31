#ifndef KVINDEX_PAGE_BUFFER_H
#define KVINDEX_PAGE_BUFFER_H

#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <string>
#include "filter_policy.h"

namespace kvindex{

//typedef uint32_t page_id_t;

//const int MAGIC_SIZE = 8;
//const char* MAGIC = "KV_INDEX" ;

//class PageBuffer
//{
//public:
//    virtual bool HasNext() = 0;
//    /**
//     * return false if buffer has been corrupted, or has no key
//     */
//    virtual bool Next(char*& key, uint32_t& key_size, uint64_t& offset) = 0;

//    virtual uint32_t get_left_keyes() = 0;

//    virtual bool AppendKey(char* key, uint32_t key_size, uint64_t offset) = 0;

//    static PageBuffer* NewPageBuffer(void* page_buffer, uint32_t size)
//    {
//        return new RawKeyPageBuffer(page_buffer, size);
//    }
//};

///**
// * @brief The RawKeyPageBuffer class
// *
// * buffer format: |MAGIC(8byte)|key_count(4byte)|key_size(4byte)|key|offset(8byte)|
// *         key_size(4byte)|key|offset(8byte)|...|key_size(4byte)|key|offset(8byte)|
// *
// */
//class RawKeyPageBuffer : public PageBuffer
//{
//public:
//    RawKeyPageBuffer(void* page_buffer, uint32_t size)
//    {
//        pos_ = (char*)page_buffer + MAGIC_SIZE;
//        left_keys_ = *(uint32_t*) pos_;
//        pos_ += sizeof(left_keys_);
//        tail_ = (char*)page_buffer + size;
//    }

//    virtual bool has_next()
//    {
//        return left_keys_ > 0;
//    }

//    /**
//     * return false if buffer has been corrupted, or has no key
//     */
//    virtual bool next(char*& key, uint32_t& key_size, uint64_t& offset)
//    {
//        if (!has_next())
//        {
//            return false;
//        }
//        // 越界检查，防止磁盘数据被破坏时，产生内存问题
//        if (pos_ + sizeof(uint32_t) > tail_)
//        {
//            return false;
//        }

//        key_size = *(uint32_t*) pos_;
//        pos_ += sizeof(key_size);
//        key = pos_;

//        if (pos_ + key_size + sizeof(offset) > tail_)
//        {
//            return false;
//        }

//        offset = *(uint64_t*)(pos_ + key_size);

//        pos_ += key_size + sizeof(offset);
//        left_keys_--;

//        return true;
//    }

//    virtual uint32_t get_left_keyes()
//    {
//        return left_keys_;
//    }

//    bool appendKey(char* key, uint32_t key_size, uint64_t offset)
//    {
//        if (pos + sizeof(uint32_t) + key_size + sizeof(offset) > tail_)
//        {
//            *(uint32_t*)(pos_) = key_size;
//            pos_ += sizeof(key_size);
//            memcpy(pos_, key, key_size);
//            pos_ += key_size;
//            *(uint64_t*)(pos_) = offset;
//            pos_ += sizeof(offset);
//            (*(uint32_t*)((char*)buffer + MAGIC_SIZE))++;

//            left_keys_++;
//        }
//    }

//private:
//    char*pos_;
//    char*tail_;
//    uint32_t left_keys_;
//};
}
#endif // PAGEDESCRIPTOR_H
