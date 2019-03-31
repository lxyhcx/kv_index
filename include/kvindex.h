#ifndef KVINDEX_INDEX_H_
#define KVINDEX_INDEX_H_

#include <stdint.h>
#include <iostream>
#include "index_def.h"
#include "options.h"

namespace kvindex {

class Index{
public:
    /**
     * Open kv index by index_file which stores disk page and memory_size which
     * limits the memmory usage of memory page.
     */
    // TODO: option
    static ErrorCode Open(const std::string& index_file, const Options& options, Index**indexptr);


    /**
     * Build index file from data file.
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

    Index() {}
    virtual ~Index() {}

private:
    Index(const Index&);
    void operator = (const Index&);
};
}
#endif
