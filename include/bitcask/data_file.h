#pragma once

#include "common.h"
#include "log_record.h"
#include "io_manager.h"
#include <memory>
#include <mutex>
#include <string>

namespace bitcask {

// 数据文件类
class DataFile {
public:
    DataFile(const std::string& dir_path, uint32_t file_id, IOType io_type);
    ~DataFile() = default;

    // 创建数据文件
    static std::unique_ptr<DataFile> open_data_file(const std::string& dir_path, 
                                                   uint32_t file_id, IOType io_type);

    // 创建提示文件
    static std::unique_ptr<DataFile> open_hint_file(const std::string& dir_path);

    // 创建合并完成标记文件
    static std::unique_ptr<DataFile> open_merge_finished_file(const std::string& dir_path);

    // 创建序列号文件
    static std::unique_ptr<DataFile> open_seq_no_file(const std::string& dir_path);

    // 根据偏移量读取日志记录
    ReadLogRecord read_log_record(uint64_t offset);

    // 写入数据
    size_t write(const Bytes& data);

    // 写入提示记录
    void write_hint_record(const Bytes& key, const LogRecordPos& pos);

    // 同步到磁盘
    void sync();

    // 关闭文件
    void close();

    // 获取文件大小
    uint64_t file_size() const;

    // 获取写入偏移量
    uint64_t get_write_off() const;

    // 设置写入偏移量
    void set_write_off(uint64_t offset);

    // 获取文件ID
    uint32_t get_file_id() const { return file_id_; }

    // 设置IO管理器
    void set_io_manager(const std::string& dir_path, IOType io_type);

    // 获取数据文件名
    static std::string get_data_file_name(const std::string& dir_path, uint32_t file_id);

private:
    uint32_t file_id_;                          // 文件ID
    uint64_t write_off_;                        // 写入偏移量
    std::unique_ptr<IOManager> io_manager_;     // IO管理器
    mutable std::mutex mutex_;                  // 线程安全保护

    // 读取N个字节
    Bytes read_n_bytes(size_t n, uint64_t offset);
};

}  // namespace bitcask
