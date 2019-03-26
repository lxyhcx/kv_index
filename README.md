kv_index
=====

[![Build Status](https://travis-ci.org/lxyhcx/kv_index.svg?branch=master)](https://travis-ci.org/lxyhcx/kv_index)

**单盘kv数据的快速索引**，支持读写接口，暂不支持更新和删除

## KV index实现的基本思想：

1. 对所有数据的key进行索引。每一个key存储在一个"index page"中
2. kv index内部维护了以类似hash table的大hash表。
3. hash 表中的每一个entry维护了一组index page，即: "page group"
4. page group中的第一个page缓存在内存中，后续page存储在磁盘中
5. 引入bloom filter, 在内存缓存不下所有的key时，加速判断key是否存在page中，减少磁盘访问
6. page group中存在多个磁盘page时，采用并发异步的方式读取磁盘，
   优化磁盘读取效率（这里此采用posix_fadvise的POSIX_FADV_WILLNEED功能实现）

## KV index的布局示意

![KV INDEX](https://github.com/lxyhcx/kv_index/raw/master/img/kv_index.PNG)

## TODO
1. 使用libaio, epoll代替posix_fadvise并行访问磁盘，或者使用协程库（如：libco）提高程序的并发
2. 线程私有的磁盘page缓存，用于读写磁盘page时的缓存使用
