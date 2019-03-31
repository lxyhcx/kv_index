#ifndef KVINDEX_PAGEDESCRIPTOR_H
#define KVINDEX_PAGEDESCRIPTOR_H

#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <string>
#include "filter_policy.h"

namespace kvindex{

typedef uint32_t page_id_t;


class PageIdGenerator
{
public:
    PageIdGenerator(page_id_t disk_page_id = 0) : disk_page_id_(disk_page_id) {}
    virtual ~PageIdGenerator(){}

    page_id_t getNextDiskPageId()
    {
        return disk_page_id_++;
    }

    void SetStartId(page_id_t startId)
    {
        disk_page_id_ = startId;
    }

private:
    page_id_t disk_page_id_;
};



class PageDescriptor
{
public:
    PageDescriptor(uint32_t len,uint32_t id):
        len_(len), id_(id) {}

    virtual ~PageDescriptor(){}

    virtual void InitNewPage(void* buffer) = 0;

    uint32_t GetId()
    {
        return id_;
    }

    virtual bool KeyMayExist(const char* key, uint32_t key_size, const FilterPolicy* policy)
    {
        if (filter_.size() == 0)
        {
            return true;
        }

        return policy->KeyMayMatch(key, key_size, filter_.c_str(), filter_.size());
    }

    virtual void GenerateFilter(void* page_buffer, const FilterPolicy* policy)
    {
        policy->CreateFilter(GetWalker(page_buffer), &filter_);
    }

    virtual PageWalker* GetWalker(void* page_buffer) = 0;

    virtual bool FindKey(void* buffer, const char* key, uint32_t key_size, uint64_t* offset) = 0;

    virtual bool Put(void* buffer, const char* key, uint32_t key_size, uint64_t offset,
                     uint32_t* dirty_offset, uint32_t* dirty_len) = 0;

    virtual bool Seal(void* page_buffer, const FilterPolicy* policy)
    {
        GenerateFilter(page_buffer, policy);
        return true;
    }


protected:
    std::string filter_;
    uint32_t len_;
    uint32_t id_;
};

class PageDescriptorFactory
{
public:
    virtual ~PageDescriptorFactory(){}

    virtual PageDescriptor* NewPageDescriptor(uint32_t len,uint32_t id) = 0;
};


}
#endif // PAGEDESCRIPTOR_H
