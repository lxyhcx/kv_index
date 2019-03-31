#ifndef KVINDEX_INDEX_DEFINE_H_
#define KVINDEX_INDEX_DEFINE_H_

namespace kvindex {

static const int MAX_KEY_SIZE = 1024;
static const int MIN_KEY_SIZE = 1;

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

#endif // KVINDEX_INDEX_DEFINE_H_
