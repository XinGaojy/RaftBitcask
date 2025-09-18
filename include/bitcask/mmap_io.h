#pragma once

#include "io_manager.h"
#include <string>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>

namespace bitcask {

/**
 * @brief 内存映射I/O管理器
 * 
 * 使用mmap将文件映射到内存中，提供高效的文件访问性能。
 * 适用于大文件的随机访问场景。
 */
class MMapIOManager : public IOManager {
public:
    /**
     * @brief 构造函数
     * @param file_name 文件名
     */
    explicit MMapIOManager(const std::string& file_name);
    
    /**
     * @brief 析构函数
     */
    ~MMapIOManager() override;

    /**
     * @brief 读取数据
     * @param buf 缓冲区
     * @param size 读取大小
     * @param offset 偏移量
     * @return 实际读取的字节数，-1表示错误
     */
    ssize_t read(void* buf, size_t size, off_t offset) override;

    /**
     * @brief 写入数据
     * @param buf 数据缓冲区
     * @param size 写入大小
     * @param offset 偏移量
     * @return 实际写入的字节数，-1表示错误
     */
    ssize_t write(const void* buf, size_t size, off_t offset) override;

    /**
     * @brief 同步数据到磁盘
     * @return 成功返回0，失败返回-1
     */
    int sync() override;

    /**
     * @brief 关闭文件
     * @return 成功返回0，失败返回-1
     */
    int close() override;

    /**
     * @brief 获取文件大小
     * @return 文件大小，-1表示错误
     */
    off_t size() override;

    /**
     * @brief 重新映射文件（当文件大小改变时）
     * @param new_size 新的文件大小
     * @return 成功返回true，失败返回false
     */
    bool remap(size_t new_size);

private:
    int fd_;                    // 文件描述符
    void* mapped_data_;         // 映射的内存地址
    size_t mapped_size_;        // 映射的大小
    std::string file_name_;     // 文件名
    bool is_closed_;            // 是否已关闭

    /**
     * @brief 初始化内存映射
     * @return 成功返回true，失败返回false
     */
    bool init_mmap();

    /**
     * @brief 清理内存映射
     */
    void cleanup_mmap();

    /**
     * @brief 扩展文件到指定大小
     * @param size 目标大小
     * @return 成功返回true，失败返回false
     */
    bool extend_file(size_t size);
};

} // namespace bitcask
