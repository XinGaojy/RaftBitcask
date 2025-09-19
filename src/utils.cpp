#include "bitcask/utils.h"
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <system_error>
#include <iostream>

#ifdef __linux__
#include <sys/statvfs.h>
#elif __APPLE__
#include <sys/statvfs.h>
#elif _WIN32
#include <windows.h>
#endif

namespace bitcask {
namespace utils {

uint64_t dir_size(const std::string& dir_path) {
    DIR* dir = opendir(dir_path.c_str());
    if (!dir) {
        return 0;
    }
    
    uint64_t size = 0;
    struct dirent* entry;
    struct stat file_stat;
    
    while ((entry = readdir(dir)) != nullptr) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        std::string full_path = dir_path + "/" + entry->d_name;
        if (stat(full_path.c_str(), &file_stat) == 0) {
            if (S_ISREG(file_stat.st_mode)) {
                size += file_stat.st_size;
            } else if (S_ISDIR(file_stat.st_mode)) {
                size += dir_size(full_path);
            }
        }
    }
    
    closedir(dir);
    return size;
}

uint64_t available_disk_size() {
#ifdef __linux__
    struct statvfs stat;
    if (statvfs(".", &stat) == 0) {
        return static_cast<uint64_t>(stat.f_bavail) * stat.f_frsize;
    }
#elif __APPLE__
    struct statvfs stat;
    if (statvfs(".", &stat) == 0) {
        return static_cast<uint64_t>(stat.f_bavail) * stat.f_frsize;
    }
#elif _WIN32
    ULARGE_INTEGER free_bytes;
    if (GetDiskFreeSpaceExA(".", &free_bytes, nullptr, nullptr)) {
        return free_bytes.QuadPart;
    }
#endif
    return UINT64_MAX; // 如果无法获取，返回最大值
}

void copy_file(const std::string& src, const std::string& dst) {
    FILE* src_file = fopen(src.c_str(), "rb");
    if (!src_file) {
        return;
    }
    
    FILE* dst_file = fopen(dst.c_str(), "wb");
    if (!dst_file) {
        fclose(src_file);
        return;
    }
    
    char buffer[8192];
    size_t bytes;
    while ((bytes = fread(buffer, 1, sizeof(buffer), src_file)) > 0) {
        fwrite(buffer, 1, bytes, dst_file);
    }
    
    fclose(src_file);
    fclose(dst_file);
}

void move_file(const std::string& src, const std::string& dst) {
    rename(src.c_str(), dst.c_str());
}

bool remove_directory(const std::string& dir_path) {
    DIR* dir = opendir(dir_path.c_str());
    if (!dir) {
        return false;
    }
    
    bool success = true;
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        std::string full_path = dir_path + "/" + entry->d_name;
        struct stat file_stat;
        if (stat(full_path.c_str(), &file_stat) == 0) {
            if (S_ISDIR(file_stat.st_mode)) {
                if (!remove_directory(full_path)) {
                    success = false;
                }
            } else {
                if (remove(full_path.c_str()) != 0) {
                    success = false;
                }
            }
        }
    }
    
    closedir(dir);
    if (rmdir(dir_path.c_str()) != 0) {
        success = false;
    }
    
    return success;
}

void create_directory(const std::string& dir_path) {
    // 递归创建目录
    size_t pos = 0;
    std::string current_path;
    
    while (pos != std::string::npos) {
        pos = dir_path.find('/', pos + 1);
        current_path = dir_path.substr(0, pos);
        
        if (!current_path.empty()) {
            mkdir(current_path.c_str(), 0755);
        }
    }
}

bool file_exists(const std::string& file_path) {
    struct stat file_stat;
    return stat(file_path.c_str(), &file_stat) == 0 && S_ISREG(file_stat.st_mode);
}

bool directory_exists(const std::string& dir_path) {
    struct stat file_stat;
    return stat(dir_path.c_str(), &file_stat) == 0 && S_ISDIR(file_stat.st_mode);
}

void copy_directory(const std::string& src_dir, const std::string& dst_dir, 
                   const std::vector<std::string>& exclude) {
    // 创建目标目录
    create_directory(dst_dir);
    
    DIR* dir = opendir(src_dir.c_str());
    if (!dir) {
        return;
    }
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        // 检查是否在排除列表中
        bool should_exclude = false;
        for (const auto& pattern : exclude) {
            if (pattern == entry->d_name) {
                should_exclude = true;
                break;
            }
        }
        if (should_exclude) {
            continue;
        }
        
        std::string src_path = src_dir + "/" + entry->d_name;
        std::string dst_path = dst_dir + "/" + entry->d_name;
        
        struct stat file_stat;
        if (stat(src_path.c_str(), &file_stat) == 0) {
            if (S_ISDIR(file_stat.st_mode)) {
                // 递归复制子目录
                copy_directory(src_path, dst_path, exclude);
            } else if (S_ISREG(file_stat.st_mode)) {
                // 复制文件
                copy_file(src_path, dst_path);
            }
        }
    }
    
    closedir(dir);
}

std::string dir_name(const std::string& file_path) {
    if (file_path.empty()) {
        return "";
    }
    
    // 找到最后一个路径分隔符
    size_t pos = file_path.find_last_of("/\\");
    if (pos == std::string::npos) {
        return "."; // 当前目录
    }
    
    if (pos == 0) {
        return "/"; // 根目录
    }
    
    return file_path.substr(0, pos);
}

}  // namespace utils
}  // namespace bitcask