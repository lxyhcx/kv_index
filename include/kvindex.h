#ifndef KVINDEX_INDEX_H_
#define KVINDEX_INDEX_H_

#include <stdint.h>
#include <iostream>
#include "index_def.h"

namespace kvindex {

class Index{
public:
/**
 * Open kv index by index_file which stores disk page and memory_size which
 * limits the memmory usage of memory page.
 */
// TODO: option
static ErrorCode Open(const std::string& index_file, uint64_t mem_size, Index**indexptr);

Index() {}
virtual ~Index() {}

/**
 * Close kv index.
 */
virtual ErrorCode Close() = 0;

/** Build index file from data file.
 *
 *  @param data_file: indexImpl's data file
 *  @return kOk if on success, non-OK on failure.
 */
virtual ErrorCode LoadDataFrom(const std::string& data_file) = 0;

/**
 * Get offset by key from kv index.
 */
virtual ErrorCode Get(const char* key, uint32_t key_size, uint64_t* offset) = 0;

/**
 * Put offset by key to kv index.
 */
virtual ErrorCode Put(const char* key, uint32_t key_size, uint64_t offset) = 0;

private:
Index(const Index&);
void operator = (const Index&);
};
}
#endif
