#include <assert.h>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <sstream>
#include<iomanip>

#include "kvindex.h"

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

        ss.str("kv_value");
        uint64_t val_size = 8;
        fwrite(&val_size, sizeof(val_size), 1, f);
        ss << setw(val_size) << setfill('0') << i;
        fwrite(ss.str().c_str(), ss.str().size(), 1, f);
    }

    fclose(f);
}

void testMemAndDiskPage(kvindex::Index* index)
{
    kvindex::ErrorCode code;
    uint64_t offset;

    // first key, only search mem page
    code = index->Get("00000000", 8, &offset);
    assert(kvindex::kOk == code);
    assert(0 == offset);

    // middle key, only search mem page
    code = index->Get("00000011", 8, &offset);
    assert(kvindex::kOk == code);
    assert(32*11 == offset);

    // large key，test disk page here
    code = index->Get("00061111", 8, &offset);
    assert(kvindex::kOk == code);
    assert(32*61111 == offset);

    // not existed key, search disk page and mem page
    code = index->Get("10061111", 8, &offset);
    assert(kvindex::kNotFound == code);
}

int main(int argc, char *argv[])
{
    cout << "testing kv index..." << endl;
    string index_file("test.idx");
    kvindex::Index* index;
    kvindex::Options options;
    kvindex::ErrorCode code = kvindex::Index::Open(index_file, options, &index);
    assert(kvindex::kOk == code);

    testBasicPutGet(index);

    generateDataFile("test.data");
    index->LoadDataFrom("test.data");

    // After load data from data file, test get some keys in data file,
    // including keys in mem pages and disk pages.
    testMemAndDiskPage(index);

    cout << "success" << endl;
    return 0;
}
