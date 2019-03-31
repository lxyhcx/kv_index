
#include "cstring"
#include "kvdb.h"
#include "utils.h"

namespace kvindex
{

ErrorCode DB::Get(const char *key, uint32_t key_size, char *value, uint32_t value_size)
{
    uint64_t offset;
    ErrorCode err = index_->Get(key, key_size, &offset);
    if (err != kOk)
    {
        return err;
    }

    uint32_t size = key_size + 8 + 8;
    char* buf = new char[size];
    std::unique_ptr<char[]> buf_ptr(buf);


    int32_t readed = pread_wrapper(data_fd_, buf, size, offset);
    if (readed == -1)
    {
        return kIOError;
    }

    if (*(uint64_t*)buf != key_size
            || 0 != memcmp(buf + 8, key, key_size))
    {
        return kNotFound;

    }

    if (*(uint64_t*)(buf + key_size + 8) < value_size)
    {
        return kInvalidArgument;
    }

    readed = pread_wrapper(data_fd_, value, value_size, offset + size);
    if (readed == -1)
    {
        return kIOError;
    }

    return kOk;
}

ErrorCode DB::LoadDataFrom(const std::string &data_file)
{
    if (-1 != data_fd_)
    {
        return kInvalidArgument;
    }
    data_fd_ = open(data_file.c_str(), O_RDONLY|O_LARGEFILE);
    if (-1 == data_fd_)
    {
        return kInvalidArgument;
    }

    return index_->LoadDataFrom(data_file);
}


ErrorCode DB::Open(const std::string &index_file, const std::string &db_file, const Options &options, DB **dbptr)
{
    Index* index;
    ErrorCode err = Index::Open(index_file, options, &index);
    if (err != kOk)
    {
        return err;
    }

    *dbptr = new DB(index);
    return (*dbptr)->LoadDataFrom(db_file);
}

}
