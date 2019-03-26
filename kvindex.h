#ifndef KVINDEX_INDEX_H_
#define KVINDEX_INDEX_H_

#include <stdint.h>
#include <iostream>
#include "index_def.h"

namespace kvindex {

class Index{
public:
/** Open the indexImpl with the specified indexImpl file.
 * 
 * @param index_file: indexImpl's indexImpl file name
 * @return 成功或者失败
 */
// TODO: option
static ErrorCode Open(const std::string& index_file, uint64_t mem_size, Index**indexptr);

Index() {}
virtual ~Index() {}

/**
 * close index
 */
virtual ErrorCode Close() = 0;

/** build index file from data file

 *  @param data_file: indexImpl's data file
    @return 0 if success
*/
virtual ErrorCode BuildIndex(const std::string& data_file) = 0;

/**
 * get offset by key from kv index
 */
virtual ErrorCode Get(const char* key, uint32_t key_size, uint64_t* offset) = 0;

/**
 * put offset by key to kv index
 */
virtual ErrorCode Put(const char* key, uint32_t key_size, uint64_t offset) = 0;

private:
Index(const Index&);
void operator = (const Index&);
};
}
#endif
