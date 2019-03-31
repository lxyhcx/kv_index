#include <assert.h>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <sstream>
#include<iomanip>

#include "kvdb.h"

using namespace std;

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

    // 读取存在key的示例(已经包含在上面的测试中)
    code = db->Get("00002004", 8, value, 16);
    assert(kvindex::kOk == code);
    assert(0 == memcmp(value, "kv_value00002004", 16));


    // 读取不存在key的示例, 搜索磁盘page和内存page
    code = db->Get("10061111", 8, value, 16);
    assert(kvindex::kNotFound == code);
}

int main(int argc, char *argv[])
{
    cout << "testing kv db..." << endl;

    // 生成数据文件
    generateDataFile("test.data");

    // 初始化db
    kvindex::DB* db;
    kvindex::Options options;
    options.mem_size = 1048576;
    kvindex::ErrorCode code = kvindex::DB::Open("test.idx", "test.data", options, &db);
    assert(kvindex::kOk == code);


    // After load data from data file, test get some keys in data file,
    // including keys in mem pages and disk pages.
    testMemAndDiskPage(db);

    cout << "success" << endl;
    return 0;
}
