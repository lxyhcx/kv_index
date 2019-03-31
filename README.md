kv index
=====

[![Build Status](https://travis-ci.org/lxyhcx/kv_index.svg?branch=master)](https://travis-ci.org/lxyhcx/kv_index)

**单盘kv数据的快速索引**，支持读写接口。不支持更新和删除

## kv index实现的基本思想：

1. 对所有数据的key进行索引。每一个key存储在一个"**index page**"中
2. kv index内部维护了以类似hash table的大hash表。
3. hash表中的每一个entry维护了一组index page，即: "page group"
4. page group中的第一个index page（**内存page**）缓存在内存中，后续index page（**磁盘page**）存储在磁盘中
5. 引入bloom filter，加速判断key是否存在于磁盘page中。在内存空间不足以缓存所有key时，减少磁盘page的访问次数。
6. page group中存在多个磁盘page时，采用并发异步的方式读取磁盘，
   优化磁盘读取效率（这里此采用posix_fadvise的POSIX_FADV_WILLNEED功能实现）
7. kv index的实现通过key查找存储数据的offset。用户通过这个offset读取对应的value。代码中提供DB类，对kv index进行封装，快速读取数据value
8. 建议使用tcmalloc加速内存申请效率

## KV index的布局示意

![KV INDEX](https://github.com/lxyhcx/kv_index/raw/master/img/kv_index.PNG)

## 读写基本流程
#### 写入
1. 对key进行hash，查找到对应的page group
2. 找到page group中的最后一个page（之前的page都已经写满），在该page的末尾写入key的索引信息，
   1. 如果该page为磁盘page，将追加写入的信息写入磁盘（不修改现有磁盘数据，仅为追加）
   2. 如果page不足以容纳这个key，生成bloom filter，申请新的page写入
3. 更新内存信息，如page的尾部位置
#### 读取
1. 对key进行hash，查找到对应的page group
2. 对page group中的所有page进行查找：
   1. 首先用bloom filter判断key是否存在于该page，不存在则跳过，存在则使用posix_fadvise的POSIX_FADV_WILLNEED，进行预读取，加入预读取队列
   2. 对预读取队列的page进行遍历，读取磁盘page到内存，并查找该key
3. 返回查找到的key


## 性能分析
在内存空间足够，可以缓存全部key时
  - 索引的写入和读取都为内存操作。

在内存空间不足以缓存所有key时
  - 索引的写入仅在磁盘page末尾追加一次写入操作
  - 索引的读取需要查找内存和磁盘page。利用bloom filter加速，并合理设置磁盘page的大小、个数，磁盘读取操作通常仅需要一次

## 使用示例

```cpp

// 初始化db
kvindex::DB* db;
kvindex::Options options;
options.mem_size = 1048576; // 内存page占用内存大小。在4G内存系统下，可以设置为2-3G左右

//"test.idx" 是kv index内部缓存磁盘page的缓存文件。
//"test.data" 是用户提供的数据文件，加载时使用。格式为|key_size(8byte)|key|value_size(8byte)|value|...
kvindex::ErrorCode code = kvindex::DB::Open("test.idx", "test.data", options, &db);
assert(kvindex::kOk == code);

// 测试读取key（该key已经存在于数据文件中）
char value[16];
code = db->Get("00002004", 8, value, 16);
assert(kvindex::kOk == code);
assert(0 == memcmp(value, "kv_value00002004", 16));

```

更多示例可以参考test目录下的文件

## 下一步优化
1. 使用协程库（如：libco）提高程序的并发，提高磁盘的并发
2. 提供一种hash page的方式：对key进行hash后，再存入index page，使index page可以容纳更多的key，减少空间占用率
