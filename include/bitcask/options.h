#pragma once

#include "common.h"
#include <string>

namespace bitcask {

// 数据库配置选项
struct Options {
    std::string dir_path;                  // 数据目录路径
    uint64_t data_file_size;               // 数据文件大小
    bool sync_writes;                      // 是否每次写入都同步到磁盘
    uint32_t bytes_per_sync;               // 累计多少字节后同步
    IndexType index_type;                  // 索引类型
    bool mmap_at_startup;                  // 启动时是否使用mmap
    float data_file_merge_ratio;           // 数据文件合并阈值

    // 默认配置
    static Options default_options() {
        Options opts;
        opts.dir_path = "/tmp/bitcask-cpp";
        opts.data_file_size = 256 * 1024 * 1024; // 256MB
        opts.sync_writes = false;
        opts.bytes_per_sync = 0;
        opts.index_type = IndexType::BTREE;
        opts.mmap_at_startup = true;
        opts.data_file_merge_ratio = 0.5f;
        return opts;
    }
};

// 迭代器配置选项
struct IteratorOptions {
    Bytes prefix;           // 前缀过滤
    bool reverse;           // 是否反向迭代

    IteratorOptions() : reverse(false) {}
};

// 批量写入配置选项
struct WriteBatchOptions {
    uint32_t max_batch_num; // 批次最大数据量
    bool sync_writes;       // 提交时是否同步

    // 默认配置
    static WriteBatchOptions default_options() {
        WriteBatchOptions opts;
        opts.max_batch_num = 10000;
        opts.sync_writes = true;
        return opts;
    }
};

// 统计信息
struct Stat {
    uint32_t key_num;           // key总数
    uint32_t data_file_num;     // 数据文件数量
    int64_t reclaimable_size;   // 可回收空间大小
    int64_t disk_size;          // 磁盘占用大小

    Stat() : key_num(0), data_file_num(0), reclaimable_size(0), disk_size(0) {}
};

}  // namespace bitcask
