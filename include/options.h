#ifndef KVINDEX_OPTIONS_H_
#define KVINDEX_OPTIONS_H_

#include "stdint.h"

namespace kvindex {

enum PageType
{
    kKeyPage,
    kHashPage
};

/**
 * 打开kv index的选项
 */
struct Options
{
    /**
     * page的类型
     */
    PageType page_type;

    /**
     * 内存page大小
     *
     * 如果太小，page结尾的空间浪费较大（尤其key很大，并且采用kKeyPage的page时），管理结构的内存占用也较大;
     * 如果太大，hash到这个page后，查找的内容多，速度较慢;
     */
    uint32_t mem_page_size;

    /**
     * 内存page占用的内存大小
     *
     */
    uint32_t mem_size;

    /**
     * 磁盘page大小
     *
     * 如果太小，bloom filter个数很多，计算量大;
     * 如果太大，page没有写满时，大量空间浪费
     */
    uint32_t disk_page_size;

    /**
     * bloom filter为每一个key占用的空间大小
     *
     * 默认10, 误判滤约1%
     */
    uint32_t bloom_filter_bit_per_key;

    Options():page_type(kKeyPage),
        mem_page_size(4096),
        mem_size(1048576),
        disk_page_size(4096*32),
        bloom_filter_bit_per_key(10){}
};

}

#endif // KVINDEX_OPTIONS_H_
