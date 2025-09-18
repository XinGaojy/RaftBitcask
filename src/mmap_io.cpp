#include "bitcask/mmap_io.h"
#include "bitcask/utils.h"
#include <sys/stat.h>
#include <cerrno>
#include <cstring>
#include <algorithm>
#include <stdexcept>

namespace bitcask {

MMapIOManager::MMapIOManager(const std::string& file_name)
    : fd_(-1), mapped_data_(nullptr), mapped_size_(0), 
      file_name_(file_name), is_closed_(false) {
    
    // 打开文件，如果不存在则创建
    fd_ = open(file_name.c_str(), O_RDWR | O_CREAT, 0644);
    if (fd_ == -1) {
        throw std::runtime_error("Failed to open file: " + file_name + 
                               ", error: " + std::string(std::strerror(errno)));
    }

    // 初始化内存映射
    if (!init_mmap()) {
        ::close(fd_);
        throw std::runtime_error("Failed to initialize memory mapping for file: " + file_name);
    }
}

MMapIOManager::~MMapIOManager() {
    if (!is_closed_) {
        close();
    }
}

bool MMapIOManager::init_mmap() {
    // 获取文件大小
    struct stat file_stat;
    if (fstat(fd_, &file_stat) == -1) {
        return false;
    }

    // 如果文件为空，扩展到最小大小
    size_t file_size = static_cast<size_t>(file_stat.st_size);
    if (file_size == 0) {
        file_size = 4096; // 最小4KB
        if (!extend_file(file_size)) {
            return false;
        }
    }

    // 创建内存映射
    mapped_data_ = mmap(nullptr, file_size, PROT_READ | PROT_WRITE, 
                        MAP_SHARED, fd_, 0);
    if (mapped_data_ == MAP_FAILED) {
        mapped_data_ = nullptr;
        return false;
    }

    mapped_size_ = file_size;
    return true;
}

void MMapIOManager::cleanup_mmap() {
    if (mapped_data_ != nullptr && mapped_data_ != MAP_FAILED) {
        munmap(mapped_data_, mapped_size_);
        mapped_data_ = nullptr;
        mapped_size_ = 0;
    }
}

bool MMapIOManager::extend_file(size_t size) {
    if (ftruncate(fd_, static_cast<off_t>(size)) == -1) {
        return false;
    }
    return true;
}

ssize_t MMapIOManager::read(void* buf, size_t size, off_t offset) {
    if (is_closed_ || mapped_data_ == nullptr) {
        errno = EBADF;
        return -1;
    }

    if (offset < 0) {
        errno = EINVAL;
        return -1;
    }

    size_t abs_offset = static_cast<size_t>(offset);
    
    // 检查读取范围是否超出映射区域
    if (abs_offset >= mapped_size_) {
        return 0; // EOF
    }

    // 计算实际可读取的字节数
    size_t available = mapped_size_ - abs_offset;
    size_t to_read = std::min(size, available);

    // 从映射内存复制数据
    std::memcpy(buf, static_cast<char*>(mapped_data_) + abs_offset, to_read);
    
    return static_cast<ssize_t>(to_read);
}

ssize_t MMapIOManager::write(const void* buf, size_t size, off_t offset) {
    if (is_closed_ || mapped_data_ == nullptr) {
        errno = EBADF;
        return -1;
    }

    if (offset < 0) {
        errno = EINVAL;
        return -1;
    }

    size_t abs_offset = static_cast<size_t>(offset);
    size_t required_size = abs_offset + size;

    // 如果需要的大小超过当前映射大小，重新映射
    if (required_size > mapped_size_) {
        // 计算新的映射大小（向上取整到页面大小的倍数）
        size_t page_size = static_cast<size_t>(getpagesize());
        size_t new_size = ((required_size + page_size - 1) / page_size) * page_size;
        
        if (!remap(new_size)) {
            errno = ENOMEM;
            return -1;
        }
    }

    // 写入数据到映射内存
    std::memcpy(static_cast<char*>(mapped_data_) + abs_offset, buf, size);
    
    return static_cast<ssize_t>(size);
}

bool MMapIOManager::remap(size_t new_size) {
    // 先清理当前映射
    cleanup_mmap();

    // 扩展文件
    if (!extend_file(new_size)) {
        return false;
    }

    // 重新创建映射
    mapped_data_ = mmap(nullptr, new_size, PROT_READ | PROT_WRITE, 
                        MAP_SHARED, fd_, 0);
    if (mapped_data_ == MAP_FAILED) {
        mapped_data_ = nullptr;
        return false;
    }

    mapped_size_ = new_size;
    return true;
}

int MMapIOManager::sync() {
    if (is_closed_ || mapped_data_ == nullptr) {
        errno = EBADF;
        return -1;
    }

    // 同步映射内存到磁盘
    if (msync(mapped_data_, mapped_size_, MS_SYNC) == -1) {
        return -1;
    }

    // 同步文件描述符
    if (fsync(fd_) == -1) {
        return -1;
    }

    return 0;
}

int MMapIOManager::close() {
    if (is_closed_) {
        return 0;
    }

    int result = 0;

    // 同步数据
    if (sync() == -1) {
        result = -1;
    }

    // 清理内存映射
    cleanup_mmap();

    // 关闭文件描述符
    if (fd_ != -1) {
        if (::close(fd_) == -1) {
            result = -1;
        }
        fd_ = -1;
    }

    is_closed_ = true;
    return result;
}

off_t MMapIOManager::size() {
    if (is_closed_) {
        errno = EBADF;
        return -1;
    }

    struct stat file_stat;
    if (fstat(fd_, &file_stat) == -1) {
        return -1;
    }

    return file_stat.st_size;
}

} // namespace bitcask
