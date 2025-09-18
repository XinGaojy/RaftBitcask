#include "bitcask/log_record.h"
#include <cstring>
#include <algorithm>

// CRC32 实现选择
#ifdef USE_ZLIB_CRC32
#include <zlib.h>
namespace crc32c {
    uint32_t Crc32c(const void* data, size_t length) {
        return crc32(0, static_cast<const Bytef*>(data), length);
    }
    uint32_t Extend(uint32_t crc, const void* data, size_t length) {
        return crc32(crc, static_cast<const Bytef*>(data), length);
    }
}
#else
// 尝试使用 crc32c 库
#include <crc32c/crc32c.h>
#endif

namespace bitcask {

// 声明编码和解码函数
size_t encode_varint(uint64_t value, uint8_t* buffer);
std::pair<uint64_t, size_t> decode_varint(const uint8_t* buffer, size_t buffer_size);

// 编码变长整数(类似protobuf的varint编码)
size_t encode_varint(uint64_t value, uint8_t* buffer) {
    size_t pos = 0;
    while (value >= 0x80) {
        buffer[pos++] = (value & 0xFF) | 0x80;
        value >>= 7;
    }
    buffer[pos++] = value & 0xFF;
    return pos;
}

// 解码变长整数
std::pair<uint64_t, size_t> decode_varint(const uint8_t* buffer, size_t buffer_size) {
    uint64_t result = 0;
    size_t pos = 0;
    int shift = 0;
    
    while (pos < buffer_size) {
        uint8_t byte = buffer[pos++];
        result |= (uint64_t(byte & 0x7F) << shift);
        if ((byte & 0x80) == 0) {
            return {result, pos};
        }
        shift += 7;
        if (shift >= 64) {
            throw BitcaskException("Varint too long");
        }
    }
    throw BitcaskException("Incomplete varint");
}

// LogRecordPos实现
Bytes LogRecordPos::encode() const {
    Bytes result;
    result.reserve(15); // 最大可能大小
    
    uint8_t buffer[10];
    size_t len;
    
    // 编码文件ID
    len = encode_varint(fid, buffer);
    result.insert(result.end(), buffer, buffer + len);
    
    // 编码偏移量
    len = encode_varint(offset, buffer);
    result.insert(result.end(), buffer, buffer + len);
    
    // 编码大小
    len = encode_varint(size, buffer);
    result.insert(result.end(), buffer, buffer + len);
    
    return result;
}

LogRecordPos LogRecordPos::decode(const Bytes& data) {
    const uint8_t* ptr = data.data();
    size_t remaining = data.size();
    LogRecordPos pos;
    
    // 解码文件ID
    auto [fid_val, fid_len] = decode_varint(ptr, remaining);
    pos.fid = static_cast<uint32_t>(fid_val);
    ptr += fid_len;
    remaining -= fid_len;
    
    // 解码偏移量
    auto [offset_val, offset_len] = decode_varint(ptr, remaining);
    pos.offset = offset_val;
    ptr += offset_len;
    remaining -= offset_len;
    
    // 解码大小
    auto [size_val, size_len] = decode_varint(ptr, remaining);
    pos.size = static_cast<uint32_t>(size_val);
    
    return pos;
}

// LogRecord实现
std::pair<Bytes, size_t> LogRecord::encode() const {
    size_t total_size = encoded_size();
    Bytes result;
    result.reserve(total_size);
    
    // 预留CRC位置
    result.resize(4);
    
    // 写入类型
    result.push_back(static_cast<uint8_t>(type));
    
    // 写入key长度
    uint8_t buffer[10];
    size_t len = encode_varint(key.size(), buffer);
    result.insert(result.end(), buffer, buffer + len);
    
    // 写入value长度
    len = encode_varint(value.size(), buffer);
    result.insert(result.end(), buffer, buffer + len);
    
    // 写入key和value
    result.insert(result.end(), key.begin(), key.end());
    result.insert(result.end(), value.begin(), value.end());
    
    // 计算CRC（跳过前4个字节的CRC字段）
    uint32_t crc = crc32c::Crc32c(result.data() + 4, result.size() - 4);
    
    // 写入CRC (小端序)
    result[0] = crc & 0xFF;
    result[1] = (crc >> 8) & 0xFF;
    result[2] = (crc >> 16) & 0xFF;
    result[3] = (crc >> 24) & 0xFF;
    
    return {result, result.size()};
}

uint32_t LogRecord::get_crc() const {
    // 创建临时缓冲区来计算CRC
    Bytes temp;
    temp.reserve(encoded_size() - 4); // 不包含CRC字段
    
    // 写入类型
    temp.push_back(static_cast<uint8_t>(type));
    
    // 写入key长度
    uint8_t buffer[10];
    size_t len = encode_varint(key.size(), buffer);
    temp.insert(temp.end(), buffer, buffer + len);
    
    // 写入value长度
    len = encode_varint(value.size(), buffer);
    temp.insert(temp.end(), buffer, buffer + len);
    
    // 写入key和value
    temp.insert(temp.end(), key.begin(), key.end());
    temp.insert(temp.end(), value.begin(), value.end());
    
    return crc32c::Crc32c(temp.data(), temp.size());
}

size_t LogRecord::encoded_size() const {
    size_t size = 4; // CRC
    size += 1; // type
    
    // key长度的变长编码大小
    uint64_t key_len = key.size();
    while (key_len >= 0x80) {
        size++;
        key_len >>= 7;
    }
    size++; // 最后一个字节
    
    // value长度的变长编码大小
    uint64_t value_len = value.size();
    while (value_len >= 0x80) {
        size++;
        value_len >>= 7;
    }
    size++; // 最后一个字节
    
    size += key.size() + value.size();
    return size;
}

// 解码日志记录头部
std::pair<LogRecordHeader, size_t> decode_log_record_header(const Bytes& data) {
    if (data.size() < 5) { // 至少需要4字节CRC + 1字节type
        throw BitcaskException("Invalid header data");
    }
    
    LogRecordHeader header;
    size_t pos = 0;
    
    // 读取CRC（小端序）
    header.crc = data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
    pos += 4;
    
    // 读取类型
    header.type = static_cast<LogRecordType>(data[pos++]);
    
    // 读取key长度
    auto [key_size, key_len] = decode_varint(data.data() + pos, data.size() - pos);
    header.key_size = static_cast<uint32_t>(key_size);
    pos += key_len;
    
    // 读取value长度
    auto [value_size, value_len] = decode_varint(data.data() + pos, data.size() - pos);
    header.value_size = static_cast<uint32_t>(value_size);
    pos += value_len;
    
    return {header, pos};
}

// 计算日志记录CRC值
uint32_t get_log_record_crc(const LogRecord& record, const Bytes& header) {
    // 计算不包含CRC的头部 + key + value的CRC
    uint32_t crc = crc32c::Crc32c(header.data(), header.size());
    crc = crc32c::Extend(crc, record.key.data(), record.key.size());
    crc = crc32c::Extend(crc, record.value.data(), record.value.size());
    return crc;
}

}  // namespace bitcask
