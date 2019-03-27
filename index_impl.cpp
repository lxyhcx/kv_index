/**
 * 实现了快速检索单盘数据的KV索引的KV index，支持读写接口，不支持删除，暂不支持更新
 * 
 * KV index实现的基本思想：
 * 1. 对所有数据的key进行索引。每一个key存储在一个"index page"中
 * 2. kv index内部维护了以类似hash table的大hash表。
 * 3. hash 表中的每一个entry维护了一组index page，即: "page group"
 * 4. page group中的第一个page缓存在内存中，后续page存储在磁盘中
 * 5. 引入bloom filter, 在内存缓存不下所有的key时，加速判断key是否存在page中，减少磁盘访问
 * 6. page group中存在多个磁盘page时，采用并发异步的方式读取磁盘，
 *    优化磁盘读取效率（这里此采用posix_fadvise的POSIX_FADV_WILLNEED功能实现）
 * 
 */

#include <iostream>
#include "index_def.h"
#include "index_impl.h"

namespace kvindex {

#define DB_RET(cond, ret) do {if (cond) return ret;} while(0)


ErrorCode indexImpl::BuildIndex(const std::string& data_file)
{
	FILE* f = fopen(data_file.c_str(), "rb");
	if (!f)
	{
		perror("open");
		return kInvalidArgument;
	}
	ErrorCode err = kCorruption;
    uint64_t offset = 0;
    while (!feof(f))
	{
		int ret;
		uint64_t key_size;
		uint64_t value_size;
        char key[MAX_KEY_SIZE];

		ret = fread(&key_size, sizeof(key_size), 1, f);
		if (ret != 1) goto except_ret;
		ret = fread(&key, key_size, 1, f);
		if (ret != 1) goto except_ret;
		ret = fread(&value_size, sizeof(value_size), 1, f);
		if (ret != 1) goto except_ret;
		ret = fseek(f, value_size, SEEK_CUR);
		if (ret != 0) goto except_ret;

		Put(key, key_size, offset);
		offset += key_size + value_size + sizeof(key_size) + sizeof(value_size);
	}

	err = kOk;
except_ret:
	fclose(f);
	return err;
}

ErrorCode Index::Open(const std::string& index_file, uint64_t mem_size, Index**indexptr)
{
    IndexPageGroup* page_group = new IndexPageGroup(index_file, mem_size);
    ErrorCode code = page_group->Open();
    if (kOk != code)
    {
        return code;
    }
    Index* index = new indexImpl(page_group);

    *indexptr = index;
    return kOk;
}

ErrorCode indexImpl::Close()
{
    return kOk;
}

ErrorCode indexImpl::Get(const char* key, uint32_t key_size, uint64_t* offset)
{
    if ((key_size <= 0) || (key_size > MAX_KEY_SIZE))
    {
        return kInvalidArgument;
    }

	return page_group_->FindKey(key, key_size, offset);
}

ErrorCode indexImpl::Put(const char* key, uint32_t key_size, uint64_t offset)
{
    if ((key_size <= 0) || (key_size > MAX_KEY_SIZE))
    {
        return kInvalidArgument;
    }

    return page_group_->AddKey(key, key_size, offset);
}


}
