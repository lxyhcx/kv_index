#include "page_descriptor.h"

namespace kvindex {
//bool PageDescriptor::FindKey(void* buffer, const char* key, uint32_t key_size, uint64_t* offset)
//{
//    assert(0 != buffer);

//    KeyKeyWalker iter(buffer, len_);
//    while (iter.has_next())
//    {
//        uint32_t cur_key_size;
//        char* cur_key;
//        uint64_t cur_offset;
//        if (iter.next(cur_key, cur_key_size, cur_offset))
//        {
//            if (cur_key_size == key_size && (0 == memcmp(key, cur_key, key_size)))
//            {
//                *offset = cur_offset;
//                return true;
//            }
//        }
//    }
//    return false;
//}

//bool PageDescriptor::AddKey(void* buffer, const char* key, uint32_t key_size, uint64_t offset)
//{
//    if ((tail_ != 0) && (tail_ + sizeof(key_size) + key_size + sizeof(offset) < len_))
//    {
//        *(uint32_t*)((char*)buffer + tail_) = key_size;
//        tail_ += sizeof(key_size);
//        memcpy((char*)buffer + tail_, key, key_size);
//        tail_ += key_size;
//        *(uint64_t*)((char*)buffer + tail_) = offset;
//        tail_ += sizeof(offset);
//        SetKeyCount(buffer, GetKeyCount(buffer)+1);

//        return true;
//    }
//    else
//    {
//        tail_ = 0;
//        return false;
//    }
//}

}
