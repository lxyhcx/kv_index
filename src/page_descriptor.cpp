#include "page_descriptor.h"

namespace kvindex {
// page format is |magic(8byte)|key_count(4byte)|key_size(4byte)|key|offset(8byte)|
//         key_size(4byte)|key|offset(8byte)|...|key_size(4byte)|key|offset(8byte)|
bool PageDescriptor::FindKey(void* buffer, const char* key, uint32_t key_size, uint64_t* offset)
{
    assert(0 != buffer);
    char* pos = (char*)buffer + MAGIC_SIZE;
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

bool PageDescriptor::AddKey(void* buffer, const char* key, uint32_t key_size, uint64_t offset)
{
    if ((tail_ != 0) && (tail_ + sizeof(key_size) + key_size + sizeof(offset) < len_))
    {
        *(uint32_t*)((char*)buffer + tail_) = key_size;
        tail_ += sizeof(key_size);
        memcpy((char*)buffer + tail_, key, key_size);
        tail_ += key_size;
        *(uint64_t*)((char*)buffer + tail_) = offset;
        tail_ += sizeof(offset);
        SetKeyCount(buffer, GetKeyCount(buffer)+1);

        return true;
    }
    else
    {
        tail_ = 0;
        return false;
    }
}

}
