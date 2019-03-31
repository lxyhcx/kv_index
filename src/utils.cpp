#include <stdint.h>
#include "utils.h"

namespace kvindex
{

ssize_t pread_wrapper(int fd, void *buf, size_t count, off64_t offset)
{
    uint32_t pos = 0;
    while ((-1 != (ssize_t)pos) && (pos < count))
    {
        pos += pread64(fd, buf, count, offset);
    }

    return pos;
}

ssize_t pwrite_wrapper(int fd, void *buf, size_t count, off64_t offset)
{
    uint32_t pos = 0;
    while ((-1 != (ssize_t)pos) && (pos < count))
    {
        pos += pwrite64(fd, buf, count, offset);
    }

    return pos;
}
}
