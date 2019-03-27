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
    // put and get
    uint64_t offset;
    char key[] = "aaeb5zju2d";
    kvindex::ErrorCode code = index->Put(key, strlen(key), 3);
    assert(kvindex::kOk == code);
    code = index->Get(key, strlen(key), &offset);
    assert(kvindex::kOk == code);

    // get not existed key
    key[1] = '2';
    code = index->Get(key, strlen(key), &offset);
    assert(kvindex::kNotFound == code);
}

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
        uint64_t val_size = 8;
        fwrite(&val_size, sizeof(val_size), 1, f);
        ss << setw(val_size) << setfill('0') << i;
        fwrite(ss.str().c_str(), ss.str().size(), 1, f);
    }
}

int main(int argc, char *argv[])
{
    cout << "testing kv index..." << endl;
    string index_file("test.idx");
    kvindex::Index* index;
    kvindex::ErrorCode code = kvindex::Index::Open(index_file, 1048576, &index);
    assert(kvindex::kOk == code);

    testBasicPutGet(index);

    generateDataFile("test_data");
    index->BuildIndex("test_data");

    // first key, only search mem page
    uint64_t offset;
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

    return 0;
}
