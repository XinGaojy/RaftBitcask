#include "bitcask/io_manager.h"
#include "bitcask/mmap_io.h"
#include "bitcask/utils.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <cstring>
#include <cerrno>

namespace bitcask {

// FileIOManager 实现
FileIOManager::FileIOManager(const std::string& file_path)
    : file_path_(file_path), fd_(-1), is_open_(false) {
    
    // 确保目录存在
    std::string dir = utils::dir_name(file_path);
    if (!dir.empty() && !utils::dir_exists(dir)) {
        utils::create_dir(dir);
    }
    
    // 打开文件，如果不存在则创建
    fd_ = ::open(file_path.c_str(), O_RDWR | O_CREAT, 0644);
    if (fd_ == -1) {
        throw BitcaskException("Failed to open file: " + file_path + 
                             ", error: " + std::string(std::strerror(errno)));
    }
    is_open_ = true;
}

FileIOManager::~FileIOManager() {
    if (is_open_) {
        close();
    }
}

ssize_t FileIOManager::read(void* buf, size_t size, off_t offset) {
    if (!is_open_) {
        errno = EBADF;
        return -1;
    }
    
    ssize_t bytes_read = pread(fd_, buf, size, offset);
    return bytes_read;
}

ssize_t FileIOManager::write(const void* buf, size_t size, off_t offset) {
    if (!is_open_) {
        errno = EBADF;
        return -1;
    }
    
    ssize_t bytes_written = pwrite(fd_, buf, size, offset);
    return bytes_written;
}

int FileIOManager::sync() {
    if (!is_open_) {
        errno = EBADF;
        return -1;
    }
    
    return fsync(fd_);
}

int FileIOManager::close() {
    if (is_open_ && fd_ != -1) {
        int result = ::close(fd_);
        fd_ = -1;
        is_open_ = false;
        return result;
    }
    return 0;
}

off_t FileIOManager::size() {
    if (!is_open_) {
        errno = EBADF;
        return -1;
    }
    
    struct stat st;
    if (fstat(fd_, &st) == -1) {
        return -1;
    }
    
    return st.st_size;
}

// 工厂函数
std::unique_ptr<IOManager> create_io_manager(const std::string& file_path, IOType type) {
    switch (type) {
        case IOType::STANDARD_FIO:
            return std::make_unique<FileIOManager>(file_path);
        case IOType::MEMORY_MAP:
            return std::make_unique<MMapIOManager>(file_path);
        default:
            throw BitcaskException("Unsupported IO type");
    }
}

}  // namespace bitcask