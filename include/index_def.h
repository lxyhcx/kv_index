#ifndef INDEX_DEFINE_H_
#define INDEX_DEFINE_H_

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
}

const int MAX_KEY_SIZE = 1024;
const int MIN_KEY_SIZE = 1;
#endif
