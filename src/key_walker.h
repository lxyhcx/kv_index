#ifndef KVINDEX_KEY_WALKER_H
#define KVINDEX_KEY_WALKER_H

#include <stdint.h>

namespace kvindex{

class PageWalker
{
public:
    virtual bool Next(char*& key, uint32_t& key_size, uint64_t& offset) = 0;
    virtual bool HasNext() const = 0;
    virtual uint32_t LeftKeyCount() const = 0;
};

}
#endif // KVINDEX_KEY_WALKER_H
