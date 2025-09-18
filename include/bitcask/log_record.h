#pragma once

#include "common.h"
#include <cstdint>
#include <vector>
#include <memory>

namespace bitcask {

// 日志记录位置信息
struct LogRecordPos {
    uint32_t fid;       // 文件ID
    uint64_t offset;    // 偏移量
    uint32_t size;      // 记录大小

    LogRecordPos() : fid(0), offset(0), size(0) {}
    LogRecordPos(uint32_t f, uint64_t o, uint32_t s) : fid(f), offset(o), size(s) {}

    // 编码位置信息
    Bytes encode() const;

    // 解码位置信息
    static LogRecordPos decode(const Bytes& data);
};

// 事务记录
struct TransactionRecord {
    std::shared_ptr<struct LogRecord> record;
    LogRecordPos pos;

    TransactionRecord(std::shared_ptr<struct LogRecord> r, const LogRecordPos& p)
        : record(r), pos(p) {}
};

// 日志记录头部信息
struct LogRecordHeader {
    uint32_t crc;           // CRC校验值
    LogRecordType type;     // 记录类型
    uint32_t key_size;      // Key长度
    uint32_t value_size;    // Value长度

    LogRecordHeader() : crc(0), type(LogRecordType::NORMAL), key_size(0), value_size(0) {}
};

// 日志记录
struct LogRecord {
    Bytes key;              // 键
    Bytes value;            // 值
    LogRecordType type;     // 记录类型

    LogRecord() : type(LogRecordType::NORMAL) {}
    LogRecord(const Bytes& k, const Bytes& v, LogRecordType t = LogRecordType::NORMAL)
        : key(k), value(v), type(t) {}

    // 编码日志记录，返回编码后的数据和大小
    std::pair<Bytes, size_t> encode() const;

    // 计算CRC值
    uint32_t get_crc() const;

    // 获取编码后的大小
    size_t encoded_size() const;
};

// 从数据文件读取的日志记录
struct ReadLogRecord {
    LogRecord record;
    size_t size;

    ReadLogRecord() : size(0) {}
    ReadLogRecord(const LogRecord& r, size_t s) : record(r), size(s) {}
};

// 解码日志记录头部
std::pair<LogRecordHeader, size_t> decode_log_record_header(const Bytes& data);

// 计算日志记录CRC值
uint32_t get_log_record_crc(const LogRecord& record, const Bytes& header);

// Varint编码/解码函数
size_t encode_varint(uint64_t value, uint8_t* buffer);
std::pair<uint64_t, size_t> decode_varint(const uint8_t* buffer, size_t buffer_size);

}  // namespace bitcask
