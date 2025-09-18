#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <exception>

namespace bitcask {

// 常量定义
static const std::string DATA_FILE_SUFFIX = ".data";
static const std::string HINT_FILE_NAME = "hint-index";
static const std::string MERGE_FINISHED_FILE_NAME = "merge-finished";
static const std::string SEQ_NO_FILE_NAME = "seq-no";
static const std::string FILE_LOCK_NAME = "flock";
static const std::string MERGE_DIR_SUFFIX = "-merge";
static const std::string MERGE_FINISHED_KEY = "merge.finished";

static const uint32_t INITIAL_FILE_ID = 0;
static const uint64_t NON_TRANSACTION_SEQ_NO = 0;
static const size_t MAX_LOG_RECORD_HEADER_SIZE = 15;

// 类型别名
using Bytes = std::vector<uint8_t>;
using BytesPtr = std::shared_ptr<Bytes>;

// 日志记录类型
enum class LogRecordType : uint8_t {
    NORMAL = 1,
    DELETED = 2,
    TXN_FINISHED = 3
};

// IO类型
enum class IOType {
    STANDARD_FIO,
    MEMORY_MAP
};

// 索引类型
enum class IndexType {
    BTREE,
    ART,
    SKIPLIST,
    BPLUS_TREE
};

// 异常类定义
class BitcaskException : public std::exception {
public:
    explicit BitcaskException(const std::string& message) : message_(message) {}
    const char* what() const noexcept override {
        return message_.c_str();
    }
private:
    std::string message_;
};

class KeyEmptyError : public BitcaskException {
public:
    KeyEmptyError() : BitcaskException("Key is empty") {}
};

class KeyNotFoundError : public BitcaskException {
public:
    KeyNotFoundError() : BitcaskException("Key not found") {}
};

class DataFileNotFoundError : public BitcaskException {
public:
    DataFileNotFoundError() : BitcaskException("Data file not found") {}
};

class DatabaseIsUsingError : public BitcaskException {
public:
    DatabaseIsUsingError() : BitcaskException("Database is being used by another process") {}
};

class DataDirectoryCorruptedError : public BitcaskException {
public:
    DataDirectoryCorruptedError() : BitcaskException("Data directory corrupted") {}
};

class InvalidCRCError : public BitcaskException {
public:
    InvalidCRCError() : BitcaskException("Invalid CRC value, log record may be corrupted") {}
};

class IndexUpdateFailedError : public BitcaskException {
public:
    IndexUpdateFailedError() : BitcaskException("Failed to update index") {}
};

class ReadDataFileEOFError : public BitcaskException {
public:
    ReadDataFileEOFError() : BitcaskException("Read data file EOF") {}
};

class MergeInProgressError : public BitcaskException {
public:
    MergeInProgressError() : BitcaskException("Merge is in progress") {}
};

class MergeRatioUnreachedError : public BitcaskException {
public:
    MergeRatioUnreachedError() : BitcaskException("Merge ratio unreached") {}
};

class NoEnoughSpaceForMergeError : public BitcaskException {
public:
    NoEnoughSpaceForMergeError() : BitcaskException("No enough space for merge") {}
};

}  // namespace bitcask
