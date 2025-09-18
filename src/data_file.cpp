#include "bitcask/data_file.h"
#include <iomanip>
#include <sstream>
#include <algorithm>

namespace bitcask {

DataFile::DataFile(const std::string& dir_path, uint32_t file_id, IOType io_type)
    : file_id_(file_id), write_off_(0) {
    
    std::string file_name = get_data_file_name(dir_path, file_id);
    io_manager_ = create_io_manager(file_name, io_type);
}

std::unique_ptr<DataFile> DataFile::open_data_file(const std::string& dir_path, 
                                                   uint32_t file_id, IOType io_type) {
    return std::make_unique<DataFile>(dir_path, file_id, io_type);
}

std::unique_ptr<DataFile> DataFile::open_hint_file(const std::string& dir_path) {
    auto data_file = std::make_unique<DataFile>(dir_path, 0, IOType::STANDARD_FIO);
    data_file->io_manager_ = create_io_manager(dir_path + "/" + HINT_FILE_NAME, IOType::STANDARD_FIO);
    return data_file;
}

std::unique_ptr<DataFile> DataFile::open_merge_finished_file(const std::string& dir_path) {
    auto data_file = std::make_unique<DataFile>(dir_path, 0, IOType::STANDARD_FIO);
    data_file->io_manager_ = create_io_manager(dir_path + "/" + MERGE_FINISHED_FILE_NAME, IOType::STANDARD_FIO);
    return data_file;
}

std::unique_ptr<DataFile> DataFile::open_seq_no_file(const std::string& dir_path) {
    auto data_file = std::make_unique<DataFile>(dir_path, 0, IOType::STANDARD_FIO);
    data_file->io_manager_ = create_io_manager(dir_path + "/" + SEQ_NO_FILE_NAME, IOType::STANDARD_FIO);
    return data_file;
}

ReadLogRecord DataFile::read_log_record(uint64_t offset) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    off_t file_size_result = io_manager_->size();
    if (file_size_result < 0) {
        throw BitcaskException("Failed to get file size");
    }
    uint64_t file_size = static_cast<uint64_t>(file_size_result);
    if (offset >= file_size) {
        throw ReadDataFileEOFError();
    }
    
    // 读取头部信息
    size_t header_bytes = std::min(static_cast<size_t>(MAX_LOG_RECORD_HEADER_SIZE), 
                                  static_cast<size_t>(file_size - offset));
    Bytes header_buf = read_n_bytes(header_bytes, offset);
    
    // 解码头部
    auto [header, header_size] = decode_log_record_header(header_buf);
    
    // 检查是否到达文件末尾
    if (header.crc == 0 && header.key_size == 0 && header.value_size == 0) {
        throw ReadDataFileEOFError();
    }
    
    // 验证记录大小的合理性
    const size_t MAX_RECORD_SIZE = 64 * 1024 * 1024; // 64MB最大记录大小
    if (header.key_size > MAX_RECORD_SIZE || header.value_size > MAX_RECORD_SIZE) {
        throw BitcaskException("Record size too large, data may be corrupted");
    }
    
    // 计算记录总大小
    size_t record_size = header_size + header.key_size + header.value_size;
    
    // 创建日志记录
    LogRecord log_record;
    log_record.type = header.type;
    
    // 读取key和value数据
    if (header.key_size > 0 || header.value_size > 0) {
        Bytes kv_buf = read_n_bytes(header.key_size + header.value_size, offset + header_size);
        
        // 验证缓冲区大小
        if (kv_buf.size() != header.key_size + header.value_size) {
            throw BitcaskException("Key-Value buffer size mismatch");
        }
        
        // 分离key和value
        if (header.key_size > 0) {
            log_record.key.assign(kv_buf.begin(), kv_buf.begin() + header.key_size);
        }
        if (header.value_size > 0) {
            log_record.value.assign(kv_buf.begin() + header.key_size, kv_buf.begin() + header.key_size + header.value_size);
        }
    }
    
    // 验证CRC
    uint32_t calculated_crc = log_record.get_crc();
    if (calculated_crc != header.crc) {
        throw InvalidCRCError();
    }
    
    return ReadLogRecord(log_record, record_size);
}

size_t DataFile::write(const Bytes& data) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    ssize_t n = io_manager_->write(data.data(), data.size(), write_off_);
    if (n < 0) {
        throw BitcaskException("Failed to write data to file");
    }
    write_off_ += n;
    return static_cast<size_t>(n);
}

void DataFile::write_hint_record(const Bytes& key, const LogRecordPos& pos) {
    LogRecord hint_record;
    hint_record.key = key;
    hint_record.value = pos.encode();
    hint_record.type = LogRecordType::NORMAL;
    
    auto [encoded_data, size] = hint_record.encode();
    write(encoded_data);
}

void DataFile::sync() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (io_manager_->sync() != 0) {
        throw BitcaskException("Failed to sync file");
    }
}

void DataFile::close() {
    std::lock_guard<std::mutex> lock(mutex_);
    io_manager_->close(); // 忽略返回值，关闭时的错误通常不致命
}

uint64_t DataFile::file_size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    off_t size_result = io_manager_->size();
    if (size_result < 0) {
        throw BitcaskException("Failed to get file size");
    }
    return static_cast<uint64_t>(size_result);
}

uint64_t DataFile::get_write_off() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return write_off_;
}

void DataFile::set_write_off(uint64_t offset) {
    std::lock_guard<std::mutex> lock(mutex_);
    write_off_ = offset;
}

void DataFile::set_io_manager(const std::string& dir_path, IOType io_type) {
    std::lock_guard<std::mutex> lock(mutex_);
    io_manager_->close();
    std::string file_name = get_data_file_name(dir_path, file_id_);
    io_manager_ = create_io_manager(file_name, io_type);
}

std::string DataFile::get_data_file_name(const std::string& dir_path, uint32_t file_id) {
    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(9) << file_id << DATA_FILE_SUFFIX;
    return dir_path + "/" + oss.str();
}

Bytes DataFile::read_n_bytes(size_t n, uint64_t offset) {
    if (n == 0) {
        return Bytes{};
    }
    
    Bytes buffer(n);
    ssize_t bytes_read = io_manager_->read(buffer.data(), n, static_cast<off_t>(offset));
    if (bytes_read < 0) {
        throw BitcaskException("Failed to read from file at offset " + std::to_string(offset));
    }
    if (static_cast<size_t>(bytes_read) != n) {
        throw BitcaskException("Incomplete read: expected " + std::to_string(n) + " bytes, got " + std::to_string(bytes_read));
    }
    return buffer;
}

}  // namespace bitcask
