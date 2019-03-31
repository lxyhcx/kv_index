kv index
=====

[![Build Status](https://travis-ci.org/lxyhcx/kv_index.svg?branch=master)](https://travis-ci.org/lxyhcx/kv_index)

**单盘kv数据的快速索引**，支持读写接口。暂不支持更新和删除

## kv index实现的基本思想：

1. 对所有数据的key进行索引。每一个key存储在一个"**index page**"中
2. kv index内部维护了以类似hash table的大hash表。
3. hash表中的每一个entry维护了一组index page，即: "page group"
4. page group中的第一个index page（**内存page**）缓存在内存中，后续index page（**磁盘page**）存储在磁盘中
5. 引入bloom filter，加速判断key是否存在于磁盘page中。在内存空间不足以缓存所有key时，减少磁盘page的访问次数。
6. 当page写满后，seal page，生成bloom filter
7. page group中存在多个磁盘page时，采用并发异步的方式读取磁盘，
   优化磁盘读取效率（这里此采用posix_fadvise的POSIX_FADV_WILLNEED功能实现）
8. index page采用全key缓存和hash缓存两种方式。全key缓存，将key完整的存储在page中，占用空间较大。hash缓存将key的hash值存入，节约空间
9. 磁盘page中put key时，仅在page末尾进行一次追加写入，无读取磁盘的操作

kv index的实现通过key查找存储数据的offset。用户通过这个offset间接找到对应的value信息

## KV index的布局示意

![KV INDEX](https://github.com/lxyhcx/kv_index/raw/master/img/kv_index.PNG)

## 接口API

```cpp
/**
 * open kv index by index_file which stores disk page and memory_size
 * which limits the memmory usage of memory page
 */
static ErrorCode Open(const std::string& index_file, uint64_t mem_size, Index**indexptr);

/**
 * get offset by key from kv index
 */
virtual ErrorCode Get(const char* key, uint32_t key_size, uint64_t* offset) = 0;

/**
 * put offset by key to kv index
 */
virtual ErrorCode Put(const char* key, uint32_t key_size, uint64_t offset) = 0;

/** 
 * Build index file from data file.
 */
virtual ErrorCode LoadDataFrom(const std::string& data_file) = 0;

```

## TODO
1. 使用libaio, epoll代替posix_fadvise并行访问磁盘，或者使用协程库（如：libco）提高程序的并发
2. 线程私有的磁盘page缓存，用于读写磁盘page时的缓存使用
3. 对key进行hash后，再存入index page，使index page可以容纳更多的key，减少空间占用率
