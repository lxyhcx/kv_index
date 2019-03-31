#include "key_page_descriptor.h"

namespace kvindex {

bool KeyPageDescriptor::FindKey(void* buffer, const char* key, uint32_t key_size, uint64_t* offset)
{
    assert(0 != buffer);

    KeyPageWalker iter(buffer, len_, key_count_);
    while (iter.HasNext())
    {
        uint32_t cur_key_size;
        char* cur_key;
        uint64_t cur_offset;
        if (iter.Next(cur_key, cur_key_size, cur_offset))
        {
            if (cur_key_size == key_size && (0 == memcmp(key, cur_key, key_size)))
            {
                *offset = cur_offset;
                return true;
            }
        }
    }
    return false;
}

bool KeyPageDescriptor::Put(void* buffer, const char* key, uint32_t key_size,
                            uint64_t offset, uint32_t* dirty_offset, uint32_t* dirty_len)
{
    if ((tail_ != 0) && (tail_ + sizeof(key_size) + key_size + sizeof(offset) < len_))
    {
        *dirty_offset = tail_;

        *(uint32_t*)((char*)buffer + tail_) = key_size;
        tail_ += sizeof(key_size);
        memcpy((char*)buffer + tail_, key, key_size);
        tail_ += key_size;
        *(uint64_t*)((char*)buffer + tail_) = offset;
        tail_ += sizeof(offset);

        *dirty_len = tail_ - *dirty_offset;
        if (key_count_ == 0)
        {
            *dirty_len += *dirty_offset;
            *dirty_offset = 0;
        }

        //SetKeyCount(buffer, GetKeyCount(buffer)+1);
        key_count_++;

        return true;
    }
    else
    {
        tail_ = 0;
        return false;
    }
}

bool KeyPageDescriptor::Seal(void *buffer)
{
    //SetKeyCount(buffer, key_count_);
    return PageDescriptor::Seal(buffer);
}
}
