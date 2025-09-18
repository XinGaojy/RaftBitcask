#pragma once

#include "common.h"
#include <memory>
#include <string>

namespace bitcask {

// IO管理器接口（统一接口）
class IOManager {
public:
    virtual ~IOManager() = default;

    // 从指定位置读取数据
    virtual ssize_t read(void* buf, size_t size, off_t offset) = 0;

    // 写入数据到指定位置
    virtual ssize_t write(const void* buf, size_t size, off_t offset) = 0;

    // 同步数据到磁盘
    virtual int sync() = 0;

    // 关闭文件
    virtual int close() = 0;

    // 获取文件大小
    virtual off_t size() = 0;
};

// 标准文件IO管理器
class FileIOManager : public IOManager {
public:
    explicit FileIOManager(const std::string& file_path);
    ~FileIOManager() override;

    ssize_t read(void* buf, size_t size, off_t offset) override;
    ssize_t write(const void* buf, size_t size, off_t offset) override;
    int sync() override;
    int close() override;
    off_t size() override;

private:
    std::string file_path_;
    int fd_;
    bool is_open_;
};

// 创建IO管理器的工厂函数
std::unique_ptr<IOManager> create_io_manager(const std::string& file_path, IOType type);

}  // namespace bitcask
