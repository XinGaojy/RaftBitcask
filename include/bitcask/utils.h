#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace bitcask {
namespace utils {

// 计算目录大小
uint64_t dir_size(const std::string& dir_path);

// 获取可用磁盘空间
uint64_t available_disk_size();

// 复制文件
void copy_file(const std::string& src, const std::string& dst);

// 移动文件
void move_file(const std::string& src, const std::string& dst);

// 删除目录
bool remove_directory(const std::string& dir_path);

// 创建目录
void create_directory(const std::string& dir_path);

// 检查文件是否存在
bool file_exists(const std::string& file_path);

// 检查目录是否存在
bool directory_exists(const std::string& dir_path);

// 获取文件的目录名
std::string dir_name(const std::string& file_path);

// 创建目录（别名）
inline bool create_dir(const std::string& dir_path) {
    create_directory(dir_path);
    return true;
}

// 检查目录是否存在（别名）
inline bool dir_exists(const std::string& dir_path) {
    return directory_exists(dir_path);
}

// 复制整个目录
void copy_directory(const std::string& src_dir, const std::string& dst_dir, 
                   const std::vector<std::string>& exclude = {});

}  // namespace utils
}  // namespace bitcask