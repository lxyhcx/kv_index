#ifndef KVINDEX_KVIMPL_H_
#define KVINDEX_KVIMPL_H_

#include <stdint.h>
#include <iostream>
#include "kvindex.h"
#include "index_page_group.h"

namespace kvindex {

class indexImpl: public Index{
     public:
explicit indexImpl(IndexPageGroup* page_group) : page_group_(page_group){}

virtual ~indexImpl(){}

/**
 * close indexImpl
 */
virtual ErrorCode Close();

/** build indexImpl file from data file

 *  @param data_file: indexImpl's data file
    @return 0 if success
*/
virtual ErrorCode BuildIndex(const std::string& data_file);

/**
 * get offset by key from kv indexImpl
 */
virtual ErrorCode Get(const char* key, uint32_t key_size, uint64_t* offset);

/**
 * put offset by key to kv indexImpl
 */
virtual ErrorCode Put(const char* key, uint32_t key_size, uint64_t offset);

private:
IndexPageGroup* page_group_;

indexImpl(const indexImpl&);
void operator=(const indexImpl&);

};
}
#endif
