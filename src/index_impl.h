#ifndef KVINDEX_KVIMPL_H_
#define KVINDEX_KVIMPL_H_

#include <stdint.h>
#include <iostream>
#include "kvindex.h"
#include "index_page_group.h"

namespace kvindex {

class IndexImpl: public Index{
     public:
explicit IndexImpl(IndexPageGroup* page_group) : page_group_(page_group){}

virtual ~IndexImpl(){}

/** build indexImpl file from data file

 *  @param data_file: indexImpl's data file
    @return 0 if success
*/
virtual ErrorCode LoadDataFrom(const std::string& data_file);

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

IndexImpl(const IndexImpl&);
void operator=(const IndexImpl&);

};
}
#endif
