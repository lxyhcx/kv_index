#include <assert.h>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <sstream>
#include<iomanip>

#include "kvdb.h"

using namespace std;

void testBasicPutGet(kvindex::Index* index)
{
    //1. put and get
    uint64_t offset;
    char key[] = "kv_index_test_key1";
    kvindex::ErrorCode code = index->Put(key, strlen(key), 3);
    assert(kvindex::kOk == code);
    code = index->Get(key, strlen(key), &offset);
    assert(kvindex::kOk == code);
    assert(3 == offset);

    //2. get not existed key
    char key2[] = "kv_index_test_key2";
    code = index->Get(key2, strlen(key2), &offset);
    assert(kvindex::kNotFound == code);
}

// 生成数据文件
void generateDataFile(const char* data_file)
{
    FILE* f = fopen(data_file, "wb");

    stringstream ss;

    for (int i = 0; i < 100000; i++)
    {
        ss.str("");
        uint64_t key_size = 8;
        fwrite(&key_size, sizeof(key_size), 1, f);
        ss << setw(key_size) << setfill('0') << i;
        fwrite(ss.str().c_str(), ss.str().size(), 1, f);

        ss.str("");
        ss << "kv_value" << setw(8) << setfill('0') << i;
        uint64_t val_size = ss.str().size();
        fwrite(&val_size, sizeof(val_size), 1, f);
        fwrite(ss.str().c_str(), ss.str().size(), 1, f);
    }

    fclose(f);
}

void testMemAndDiskPage(kvindex::DB* db)
{
    kvindex::ErrorCode code;
    char value[16];

    // 测试所有的key，包括内存和磁盘的page
    stringstream ss;
    for (int i = 0; i < 100000; i++)
    {
        ss.str("");
        uint64_t key_size = 8;

        ss << setw(key_size) << setfill('0') << i;
        code = db->Get(ss.str().c_str(), ss.str().size(), value, 16);
        assert(kvindex::kOk == code);
        ss.str("");
        ss << "kv_value" << setw(8) << setfill('0') << i;
        assert(0 == memcmp(value, ss.str().c_str(), 16));
    }


    // not existed key, search disk page and mem page
    code = db->Get("10061111", 8, value, 16);
    assert(kvindex::kNotFound == code);
}

int main(int argc, char *argv[])
{
    cout << "testing kv db..." << endl;
    generateDataFile("test.data");

    kvindex::DB* db;
    kvindex::Options options;
    kvindex::ErrorCode code = kvindex::DB::Open("test.idx", "test.data", options, &db);
    assert(kvindex::kOk == code);


    // After load data from data file, test get some keys in data file,
    // including keys in mem pages and disk pages.
    testMemAndDiskPage(db);

    cout << "success" << endl;
    return 0;
}
