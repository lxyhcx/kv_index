#ifndef KVINDEX_DB_H
#define KVINDEX_DB_H

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "index_def.h"
#include "kvindex.h"
#include "options.h"
#include "memory"

namespace kvindex
{

class DB
{
public:
    /**
     * Open kv db by index_file which stores disk page and memory_size which
     * limits the memmory usage of memory page.
     */
    static ErrorCode Open(const std::string& index_file, const std::string& data_file, const Options& option, DB**dbptr);


    /**
     * Get value by key from kv db.
     */
    ErrorCode Get(const char* key, uint32_t key_size, char* value, uint32_t value_size);


    DB(Index* index) : index_(index), data_fd_(-1) {}
    virtual ~DB() {}

private:

    DB(const DB&);
    void operator = (const DB&);

    /**
     * Build index file from data file.
     */
    ErrorCode LoadDataFrom(const std::string& data_file);

    Index* index_;
    int data_fd_;
};

}

#endif // KVINDEX_DB_H
