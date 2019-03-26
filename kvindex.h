#ifndef INDEX_INCLUDE_H_
#define INDEX_INCLUDE_H_

#include <stdint.h>
#include <iostream>
#include "index_page_group.h"

namespace kvindex {
/**
 * error code of kv indexImpl api 
 */
typedef enum {
    kOk = 0,
    kNotFound = 1,
    kCorruption = 2,
    kNotSupported = 3,
    kInvalidArgument = 4,
    kIOError = 5
} ErrorCode;

class indexImpl{
     public:
/** Open the indexImpl with the specified indexImpl file.
 * 
 * @param index_file: indexImpl's indexImpl file name
 * @return 成功或者失败
 */
// TODO: option
ErrorCode Open(const std::string& index_file, uint64_t mem_size);

explicit indexImpl(){};

indexImpl(const indexImpl&) = delete;
indexImpl& operator=(const indexImpl&) = delete;

virtual ~indexImpl();

/**
 * close indexImpl
 */
ErrorCode Close();

/** build indexImpl file from data file

 *  @param data_file: indexImpl's data file
    @return 0 if success
*/
ErrorCode BuildIndex(const std::string& data_file);

/**
 * get offset by key from kv indexImpl
 */
ErrorCode Get(const char* key, uint32_t key_size, uint64_t* offset);

/**
 * put offset by key to kv indexImpl
 */
ErrorCode Put(const char* key, uint32_t key_size, uint64_t offset);

private:
int* index_;
IndexPageGroup page_group_;
};
}
#endif