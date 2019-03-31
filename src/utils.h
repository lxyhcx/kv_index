#ifndef KVINDEX_UTILS_H
#define KVINDEX_UTILS_H

#include <unistd.h>

namespace kvindex
{
ssize_t pread_wrapper(int fd, void *buf, size_t count, off64_t offset);
ssize_t pwrite_wrapper(int fd, void *buf, size_t count, off64_t offset);
}
#endif // KVINDEX_UTILS_H
