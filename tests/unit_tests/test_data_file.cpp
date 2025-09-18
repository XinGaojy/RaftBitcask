#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "bitcask/data_file.h"
#include "bitcask/utils.h"

using namespace bitcask;

class DataFileTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir = "/tmp/bitcask_datafile_test";
        utils::remove_directory(test_dir);
        utils::create_directory(test_dir);
        
        // 创建测试数据
        test_key = {0x74, 0x65, 0x73, 0x74}; // "test"
        test_value = {0x76, 0x61, 0x6c, 0x75, 0x65}; // "value"
        
        test_record = LogRecord(test_key, test_value, LogRecordType::NORMAL);
    }
    
    void TearDown() override {
        utils::remove_directory(test_dir);
    }
    
    std::string test_dir;
    Bytes test_key;
    Bytes test_value;
    LogRecord test_record;
};

// 基本 DataFile 操作测试
TEST_F(DataFileTest, CreateDataFile) {
    auto data_file = DataFile::open_data_file(test_dir, 1, IOType::STANDARD_FIO);
    
    EXPECT_EQ(data_file->get_file_id(), 1);
    EXPECT_EQ(data_file->get_write_off(), 0);
    EXPECT_EQ(data_file->file_size(), 0);
    
    data_file->close();
}

TEST_F(DataFileTest, WriteAndReadLogRecord) {
    auto data_file = DataFile::open_data_file(test_dir, 1, IOType::STANDARD_FIO);
    
    // 编码并写入记录
    auto [encoded_data, size] = test_record.encode();
    size_t written = data_file->write(encoded_data);
    
    EXPECT_EQ(written, size);
    EXPECT_EQ(data_file->get_write_off(), size);
    
    // 读取记录
    ReadLogRecord read_record = data_file->read_log_record(0);
    
    EXPECT_EQ(read_record.record.key, test_key);
    EXPECT_EQ(read_record.record.value, test_value);
    EXPECT_EQ(read_record.record.type, LogRecordType::NORMAL);
    EXPECT_EQ(read_record.size, size);
    
    data_file->close();
}

TEST_F(DataFileTest, MultipleRecords) {
    auto data_file = DataFile::open_data_file(test_dir, 1, IOType::STANDARD_FIO);
    
    std::vector<LogRecord> records = {
        LogRecord({0x6b, 0x65, 0x79, 0x31}, {0x76, 0x61, 0x6c, 0x31}, LogRecordType::NORMAL),
        LogRecord({0x6b, 0x65, 0x79, 0x32}, {0x76, 0x61, 0x6c, 0x32}, LogRecordType::NORMAL),
        LogRecord({0x6b, 0x65, 0x79, 0x33}, {}, LogRecordType::DELETED),
    };
    
    std::vector<uint64_t> offsets;
    
    // 写入多个记录
    for (const auto& record : records) {
        offsets.push_back(data_file->get_write_off());
        auto [encoded_data, size] = record.encode();
        data_file->write(encoded_data);
    }
    
    // 读取并验证每个记录
    for (size_t i = 0; i < records.size(); ++i) {
        ReadLogRecord read_record = data_file->read_log_record(offsets[i]);
        
        EXPECT_EQ(read_record.record.key, records[i].key);
        EXPECT_EQ(read_record.record.value, records[i].value);
        EXPECT_EQ(read_record.record.type, records[i].type);
    }
    
    data_file->close();
}

TEST_F(DataFileTest, WriteHintRecord) {
    auto data_file = DataFile::open_hint_file(test_dir);
    
    LogRecordPos pos(1, 100, 50);
    data_file->write_hint_record(test_key, pos);
    
    // 读取提示记录
    ReadLogRecord read_record = data_file->read_log_record(0);
    
    EXPECT_EQ(read_record.record.key, test_key);
    
    // 解码位置信息
    LogRecordPos decoded_pos = LogRecordPos::decode(read_record.record.value);
    EXPECT_EQ(decoded_pos.fid, pos.fid);
    EXPECT_EQ(decoded_pos.offset, pos.offset);
    EXPECT_EQ(decoded_pos.size, pos.size);
    
    data_file->close();
}

TEST_F(DataFileTest, SyncOperation) {
    auto data_file = DataFile::open_data_file(test_dir, 1, IOType::STANDARD_FIO);
    
    // 写入数据
    auto [encoded_data, size] = test_record.encode();
    data_file->write(encoded_data);
    
    // 同步操作不应该抛出异常
    EXPECT_NO_THROW(data_file->sync());
    
    data_file->close();
}

TEST_F(DataFileTest, SetWriteOffset) {
    auto data_file = DataFile::open_data_file(test_dir, 1, IOType::STANDARD_FIO);
    
    EXPECT_EQ(data_file->get_write_off(), 0);
    
    data_file->set_write_off(100);
    EXPECT_EQ(data_file->get_write_off(), 100);
    
    data_file->close();
}

// 特殊文件类型测试
TEST_F(DataFileTest, MergeFinishedFile) {
    auto merge_file = DataFile::open_merge_finished_file(test_dir);
    
    // 写入一些标记数据
    Bytes marker_data = {0x01, 0x02, 0x03, 0x04};
    size_t written = merge_file->write(marker_data);
    
    EXPECT_EQ(written, marker_data.size());
    
    merge_file->close();
    
    // 验证文件存在
    std::string merge_file_path = test_dir + "/" + MERGE_FINISHED_FILE_NAME;
    EXPECT_TRUE(utils::file_exists(merge_file_path));
}

TEST_F(DataFileTest, SeqNoFile) {
    auto seq_file = DataFile::open_seq_no_file(test_dir);
    
    // 创建序列号记录
    LogRecord seq_record({0x73, 0x65, 0x71, 0x2e, 0x6e, 0x6f}, {0x31, 0x32, 0x33}, LogRecordType::NORMAL);
    auto [encoded_data, size] = seq_record.encode();
    
    seq_file->write(encoded_data);
    
    // 读取并验证
    ReadLogRecord read_record = seq_file->read_log_record(0);
    EXPECT_EQ(read_record.record.key, seq_record.key);
    EXPECT_EQ(read_record.record.value, seq_record.value);
    
    seq_file->close();
    
    // 验证文件存在
    std::string seq_file_path = test_dir + "/" + SEQ_NO_FILE_NAME;
    EXPECT_TRUE(utils::file_exists(seq_file_path));
}

// 错误处理测试
class DataFileErrorTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir = "/tmp/bitcask_datafile_error_test";
        utils::remove_directory(test_dir);
        utils::create_directory(test_dir);
    }
    
    void TearDown() override {
        utils::remove_directory(test_dir);
    }
    
    std::string test_dir;
};

TEST_F(DataFileErrorTest, ReadFromEmptyFile) {
    auto data_file = DataFile::open_data_file(test_dir, 1, IOType::STANDARD_FIO);
    
    // 尝试从空文件读取应该抛出 EOF 错误
    EXPECT_THROW(data_file->read_log_record(0), ReadDataFileEOFError);
    
    data_file->close();
}

TEST_F(DataFileErrorTest, ReadBeyondFileEnd) {
    auto data_file = DataFile::open_data_file(test_dir, 1, IOType::STANDARD_FIO);
    
    // 写入一些数据
    LogRecord record({0x74, 0x65, 0x73, 0x74}, {0x76, 0x61, 0x6c, 0x75, 0x65}, LogRecordType::NORMAL);
    auto [encoded_data, size] = record.encode();
    data_file->write(encoded_data);
    
    // 尝试从文件末尾之后读取
    EXPECT_THROW(data_file->read_log_record(size + 100), ReadDataFileEOFError);
    
    data_file->close();
}

TEST_F(DataFileErrorTest, CorruptedData) {
    auto data_file = DataFile::open_data_file(test_dir, 1, IOType::STANDARD_FIO);
    
    // 写入损坏的数据（无效的 CRC）
    Bytes corrupted_data = {0x00, 0x00, 0x00, 0x00, 0x01, 0x04, 0x05, 0x74, 0x65, 0x73, 0x74, 0x76, 0x61, 0x6c, 0x75, 0x65};
    data_file->write(corrupted_data);
    
    // 尝试读取损坏的数据应该抛出 CRC 错误
    EXPECT_THROW(data_file->read_log_record(0), InvalidCRCError);
    
    data_file->close();
}

// MMap DataFile 测试
class MMapDataFileTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir = "/tmp/bitcask_mmap_datafile_test";
        utils::remove_directory(test_dir);
        utils::create_directory(test_dir);
        
        test_record = LogRecord({0x74, 0x65, 0x73, 0x74}, {0x76, 0x61, 0x6c, 0x75, 0x65}, LogRecordType::NORMAL);
    }
    
    void TearDown() override {
        utils::remove_directory(test_dir);
    }
    
    std::string test_dir;
    LogRecord test_record;
};

TEST_F(MMapDataFileTest, BasicOperations) {
    auto data_file = DataFile::open_data_file(test_dir, 1, IOType::MEMORY_MAP);
    
    // 写入记录
    auto [encoded_data, size] = test_record.encode();
    size_t written = data_file->write(encoded_data);
    
    EXPECT_EQ(written, size);
    
    // 读取记录
    ReadLogRecord read_record = data_file->read_log_record(0);
    
    EXPECT_EQ(read_record.record.key, test_record.key);
    EXPECT_EQ(read_record.record.value, test_record.value);
    EXPECT_EQ(read_record.record.type, test_record.type);
    
    data_file->close();
}

TEST_F(MMapDataFileTest, LargeFile) {
    auto data_file = DataFile::open_data_file(test_dir, 1, IOType::MEMORY_MAP);
    
    // 写入大量数据以触发映射扩展
    for (int i = 0; i < 1000; ++i) {
        LogRecord record({0x6b, 0x65, 0x79}, {0x76, 0x61, 0x6c, 0x75, 0x65}, LogRecordType::NORMAL);
        auto [encoded_data, size] = record.encode();
        data_file->write(encoded_data);
    }
    
    // 验证文件大小
    EXPECT_GT(data_file->file_size(), 1000 * 10); // 至少应该有这么大
    
    data_file->close();
}

// 文件名生成测试
class FileNameTest : public ::testing::Test {};

TEST_F(FileNameTest, DataFileName) {
    std::string dir = "/tmp/test";
    
    std::string name1 = DataFile::get_data_file_name(dir, 0);
    std::string name2 = DataFile::get_data_file_name(dir, 1);
    std::string name3 = DataFile::get_data_file_name(dir, 999999999);
    
    EXPECT_EQ(name1, "/tmp/test/000000000.data");
    EXPECT_EQ(name2, "/tmp/test/000000001.data");
    EXPECT_EQ(name3, "/tmp/test/999999999.data");
}

// IO 管理器切换测试
class IOManagerSwitchTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir = "/tmp/bitcask_io_switch_test";
        utils::remove_directory(test_dir);
        utils::create_directory(test_dir);
    }
    
    void TearDown() override {
        utils::remove_directory(test_dir);
    }
    
    std::string test_dir;
};

TEST_F(IOManagerSwitchTest, SwitchIOType) {
    auto data_file = DataFile::open_data_file(test_dir, 1, IOType::MEMORY_MAP);
    
    // 写入一些数据
    LogRecord record({0x74, 0x65, 0x73, 0x74}, {0x76, 0x61, 0x6c, 0x75, 0x65}, LogRecordType::NORMAL);
    auto [encoded_data, size] = record.encode();
    data_file->write(encoded_data);
    
    // 切换到标准 IO
    data_file->set_io_manager(test_dir, IOType::STANDARD_FIO);
    
    // 应该仍然能够读取数据
    ReadLogRecord read_record = data_file->read_log_record(0);
    
    EXPECT_EQ(read_record.record.key, record.key);
    EXPECT_EQ(read_record.record.value, record.value);
    EXPECT_EQ(read_record.record.type, record.type);
    
    data_file->close();
}
