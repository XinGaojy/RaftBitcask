#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "bitcask/log_record.h"

using namespace bitcask;

class LogRecordTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 测试数据
        test_key = {0x74, 0x65, 0x73, 0x74}; // "test"
        test_value = {0x76, 0x61, 0x6c, 0x75, 0x65}; // "value"
        empty_bytes = {};
    }

    Bytes test_key;
    Bytes test_value;
    Bytes empty_bytes;
};

// 测试 LogRecord 编码和解码
TEST_F(LogRecordTest, EncodeAndDecode) {
    LogRecord record(test_key, test_value, LogRecordType::NORMAL);
    
    // 编码
    auto [encoded, size] = record.encode();
    EXPECT_GT(size, 0);
    EXPECT_EQ(encoded.size(), size);
    
    // 验证编码格式
    EXPECT_GE(encoded.size(), 4 + 1 + test_key.size() + test_value.size()); // CRC + type + key + value
}

// 测试不同类型的 LogRecord
TEST_F(LogRecordTest, DifferentTypes) {
    // NORMAL 类型
    LogRecord normal_record(test_key, test_value, LogRecordType::NORMAL);
    auto [normal_encoded, normal_size] = normal_record.encode();
    EXPECT_GT(normal_size, 0);
    
    // DELETED 类型
    LogRecord deleted_record(test_key, empty_bytes, LogRecordType::DELETED);
    auto [deleted_encoded, deleted_size] = deleted_record.encode();
    EXPECT_GT(deleted_size, 0);
    
    // TXN_FINISHED 类型
    LogRecord txn_record(empty_bytes, empty_bytes, LogRecordType::TXN_FINISHED);
    auto [txn_encoded, txn_size] = txn_record.encode();
    EXPECT_GT(txn_size, 0);
}

// 测试空 key 和 value
TEST_F(LogRecordTest, EmptyKeyValue) {
    LogRecord empty_record(empty_bytes, empty_bytes, LogRecordType::NORMAL);
    auto [encoded, size] = empty_record.encode();
    
    EXPECT_GT(size, 0);
    EXPECT_EQ(encoded.size(), size);
}

// 测试大数据
TEST_F(LogRecordTest, LargeData) {
    // 创建大的 key 和 value
    Bytes large_key(1024, 'k');
    Bytes large_value(4096, 'v');
    
    LogRecord large_record(large_key, large_value, LogRecordType::NORMAL);
    auto [encoded, size] = large_record.encode();
    
    EXPECT_GT(size, 1024 + 4096);
    EXPECT_EQ(encoded.size(), size);
}

// 测试 CRC 计算
TEST_F(LogRecordTest, CrcCalculation) {
    LogRecord record1(test_key, test_value, LogRecordType::NORMAL);
    LogRecord record2(test_key, test_value, LogRecordType::NORMAL);
    LogRecord record3(test_key, test_value, LogRecordType::DELETED);
    
    // 相同数据应该有相同的 CRC
    EXPECT_EQ(record1.get_crc(), record2.get_crc());
    
    // 不同类型应该有不同的 CRC
    EXPECT_NE(record1.get_crc(), record3.get_crc());
}

// 测试编码大小计算
TEST_F(LogRecordTest, EncodedSize) {
    LogRecord record(test_key, test_value, LogRecordType::NORMAL);
    size_t calculated_size = record.encoded_size();
    
    auto [encoded, actual_size] = record.encode();
    EXPECT_EQ(calculated_size, actual_size);
}

// LogRecordPos 测试
class LogRecordPosTest : public ::testing::Test {
protected:
    void SetUp() override {
        pos1 = LogRecordPos(1, 100, 50);
        pos2 = LogRecordPos(2, 200, 75);
    }
    
    LogRecordPos pos1;
    LogRecordPos pos2;
};

TEST_F(LogRecordPosTest, EncodeAndDecode) {
    // 编码
    Bytes encoded = pos1.encode();
    EXPECT_GT(encoded.size(), 0);
    
    // 解码
    LogRecordPos decoded = LogRecordPos::decode(encoded);
    EXPECT_EQ(decoded.fid, pos1.fid);
    EXPECT_EQ(decoded.offset, pos1.offset);
    EXPECT_EQ(decoded.size, pos1.size);
}

TEST_F(LogRecordPosTest, MultiplePositions) {
    // 测试多个位置信息
    std::vector<LogRecordPos> positions = {
        LogRecordPos(0, 0, 1),
        LogRecordPos(1, 100, 50),
        LogRecordPos(999, 1000000, 2048),
        LogRecordPos(UINT32_MAX, UINT64_MAX, UINT32_MAX)
    };
    
    for (const auto& pos : positions) {
        Bytes encoded = pos.encode();
        LogRecordPos decoded = LogRecordPos::decode(encoded);
        
        EXPECT_EQ(decoded.fid, pos.fid);
        EXPECT_EQ(decoded.offset, pos.offset);
        EXPECT_EQ(decoded.size, pos.size);
    }
}

// 测试头部解码
class LogRecordHeaderTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 创建一个测试记录并编码
        LogRecord record({0x74, 0x65, 0x73, 0x74}, {0x76, 0x61, 0x6c, 0x75, 0x65}, LogRecordType::NORMAL);
        auto [encoded_data, size] = record.encode();
        test_encoded = encoded_data;
    }
    
    Bytes test_encoded;
};

TEST_F(LogRecordHeaderTest, DecodeHeader) {
    auto [header, header_size] = decode_log_record_header(test_encoded);
    
    EXPECT_GT(header.crc, 0);
    EXPECT_EQ(header.type, LogRecordType::NORMAL);
    EXPECT_EQ(header.key_size, 4);
    EXPECT_EQ(header.value_size, 5);
    EXPECT_GT(header_size, 0);
}

TEST_F(LogRecordHeaderTest, InvalidHeader) {
    // 测试无效的头部数据
    Bytes invalid_header = {0x01, 0x02, 0x03}; // 太短
    
    EXPECT_THROW(decode_log_record_header(invalid_header), BitcaskException);
}

// 变长整数编码测试
class VarintTest : public ::testing::Test {};

TEST_F(VarintTest, EncodeDecodeSmallNumbers) {
    std::vector<uint64_t> test_values = {0, 1, 127, 128, 255, 256, 16383, 16384};
    
    for (uint64_t value : test_values) {
        uint8_t buffer[10];
        size_t encoded_len = encode_varint(value, buffer);
        
        EXPECT_GT(encoded_len, 0);
        EXPECT_LE(encoded_len, 10);
        
        auto [decoded_value, decoded_len] = decode_varint(buffer, encoded_len);
        EXPECT_EQ(decoded_value, value);
        EXPECT_EQ(decoded_len, encoded_len);
    }
}

TEST_F(VarintTest, EncodeDecodeLargeNumbers) {
    std::vector<uint64_t> test_values = {
        UINT32_MAX,
        UINT64_MAX,
        1ULL << 32,
        1ULL << 48,
        1ULL << 63
    };
    
    for (uint64_t value : test_values) {
        uint8_t buffer[10];
        size_t encoded_len = encode_varint(value, buffer);
        
        EXPECT_GT(encoded_len, 0);
        EXPECT_LE(encoded_len, 10);
        
        auto [decoded_value, decoded_len] = decode_varint(buffer, encoded_len);
        EXPECT_EQ(decoded_value, value);
        EXPECT_EQ(decoded_len, encoded_len);
    }
}

TEST_F(VarintTest, InvalidVarint) {
    // 测试不完整的变长整数
    uint8_t invalid_buffer[] = {0x80, 0x80, 0x80}; // 不完整
    
    EXPECT_THROW(decode_varint(invalid_buffer, sizeof(invalid_buffer)), BitcaskException);
}

// CRC 验证测试
class CrcTest : public ::testing::Test {};

TEST_F(CrcTest, CrcConsistency) {
    LogRecord record({0x74, 0x65, 0x73, 0x74}, {0x76, 0x61, 0x6c, 0x75, 0x65}, LogRecordType::NORMAL);
    
    uint32_t crc1 = record.get_crc();
    uint32_t crc2 = record.get_crc();
    
    // 相同数据应该产生相同的 CRC
    EXPECT_EQ(crc1, crc2);
}

TEST_F(CrcTest, CrcDifference) {
    LogRecord record1({0x74, 0x65, 0x73, 0x74}, {0x76, 0x61, 0x6c, 0x75, 0x65}, LogRecordType::NORMAL);
    LogRecord record2({0x74, 0x65, 0x73, 0x74}, {0x76, 0x61, 0x6c, 0x75, 0x66}, LogRecordType::NORMAL); // 最后一个字节不同
    
    // 不同数据应该产生不同的 CRC
    EXPECT_NE(record1.get_crc(), record2.get_crc());
}
