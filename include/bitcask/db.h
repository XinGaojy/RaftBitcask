#pragma once

#include "common.h"
#include "options.h"
#include "log_record.h"
#include "data_file.h"
#include "index.h"
#include <unordered_map>
#include <memory>
#include <shared_mutex>
#include <atomic>
#include <functional>
#include <string>

namespace bitcask {

// 批量写入类
class WriteBatch {
public:
    WriteBatch(class DB* db, const WriteBatchOptions& options);
    ~WriteBatch();

    // 写入数据
    void put(const Bytes& key, const Bytes& value);

    // 删除数据
    void remove(const Bytes& key);

    // 提交批次
    void commit();

private:
    class DB* db_;
    WriteBatchOptions options_;
    std::vector<LogRecord> pending_writes_;
    std::atomic<uint64_t> seq_no_;
};

// Bitcask存储引擎实例
class DB {
    friend class DBIterator;  // 允许DBIterator访问私有成员
    
public:
    // 打开数据库
    static std::unique_ptr<DB> open(const Options& options);

    // 析构函数
    ~DB();

    // 写入key/value数据
    void put(const Bytes& key, const Bytes& value);

    // 根据key读取数据
    Bytes get(const Bytes& key);

    // 根据key删除数据
    void remove(const Bytes& key);

    // 获取所有key
    std::vector<Bytes> list_keys();

    // 遍历所有数据
    void fold(std::function<bool(const Bytes& key, const Bytes& value)> func);

    // 同步数据到磁盘
    void sync();

    // 关闭数据库
    void close();

    // 获取统计信息
    Stat stat();

    // 备份数据库
    void backup(const std::string& dir);

    // 创建批量写入对象
    std::unique_ptr<WriteBatch> new_write_batch(const WriteBatchOptions& options);

    // 创建迭代器
    std::unique_ptr<class DBIterator> iterator(const IteratorOptions& options);

    // 合并无效数据，生成Hint文件
    void merge();

private:
    DB(const Options& options);

    // 初始化数据库
    void init();

    // 加载数据文件
    void load_data_files();

    // 从数据文件加载索引
    void load_index_from_data_files();

    // 从hint文件加载索引
    void load_index_from_hint_file();

    // 加载合并文件
    void load_merge_files();

    // 设置活跃数据文件
    void set_active_data_file();

    // 追加写入日志记录
    LogRecordPos append_log_record(const LogRecord& record);
    
    // 追加写入日志记录（内部版本，不加锁）
    LogRecordPos append_log_record_internal(const LogRecord& record);

    // 根据位置获取值
    Bytes get_value_by_position(const LogRecordPos& pos);

    // 检查配置选项
    static void check_options(const Options& options);

    // 获取文件锁
    void acquire_file_lock();

    // 释放文件锁
    void release_file_lock();

    // 加载序列号
    void load_seq_no();

    // 重置IO类型
    void reset_io_type();

    // 解析日志记录key
    static std::pair<Bytes, uint64_t> parse_log_record_key(const Bytes& key);

    // 生成带序列号的key
    static Bytes log_record_key_with_seq(const Bytes& key, uint64_t seq_no);

    // 获取合并目录路径
    std::string get_merge_path() const;

    // 检查是否达到合并阈值
    bool should_merge() const;

    // 旋转合并文件
    std::vector<std::unique_ptr<DataFile>> rotate_merge_files();

    // 获取非合并文件ID
    uint32_t get_non_merge_file_id(const std::string& dir_path) const;

    friend class WriteBatch;

private:
    Options options_;                                           // 配置选项
    mutable std::shared_mutex mutex_;                          // 读写锁
    std::vector<uint32_t> file_ids_;                          // 文件ID列表
    std::unique_ptr<DataFile> active_file_;                    // 活跃数据文件
    std::unordered_map<uint32_t, std::unique_ptr<DataFile>> older_files_; // 旧数据文件
    std::unique_ptr<Indexer> index_;                          // 内存索引
    std::atomic<uint64_t> seq_no_;                            // 事务序列号
    std::atomic<bool> is_merging_;                            // 是否正在合并
    bool seq_no_file_exists_;                                 // 序列号文件是否存在
    bool is_initial_;                                         // 是否首次初始化
    int file_lock_fd_;                                        // 文件锁
    std::atomic<uint64_t> bytes_write_;                       // 累计写入字节数
    std::atomic<int64_t> reclaim_size_;                       // 可回收空间大小
};

// 数据库迭代器
class DBIterator {
public:
    DBIterator(DB* db, const IteratorOptions& options);
    ~DBIterator();

    // 重新回到起点
    void rewind();

    // 根据key查找
    void seek(const Bytes& key);

    // 移动到下一个
    void next();

    // 检查是否有效
    bool valid() const;

    // 获取当前key
    Bytes key() const;

    // 获取当前value
    Bytes value() const;

    // 关闭迭代器
    void close();

private:
    DB* db_;
    IteratorOptions options_;
    std::unique_ptr<IndexIterator> index_iter_;
};

}  // namespace bitcask
