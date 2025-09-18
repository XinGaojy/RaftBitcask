#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "bitcask/io_manager.h"
#include "bitcask/utils.h"
#include <fstream>
#include <chrono>
#include <numeric>

using namespace bitcask;

class IOManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir = "/tmp/bitcask_io_test";
        utils::remove_directory(test_dir);
        utils::create_directory(test_dir);
        
        test_file = test_dir + "/test_file.dat";
        test_data = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    }
    
    void TearDown() override {
        utils::remove_directory(test_dir);
    }
    
    std::string test_dir;
    std::string test_file;
    std::vector<uint8_t> test_data;
};

// FileIOManager 测试
class FileIOManagerTest : public IOManagerTest {};

TEST_F(FileIOManagerTest, CreateAndWrite) {
    auto io_manager = create_io_manager(test_file, IOType::STANDARD_FIO);
    
    // 写入数据
    ssize_t written = io_manager->write(test_data.data(), test_data.size(), 0);
    EXPECT_GT(written, 0);
    EXPECT_EQ(static_cast<size_t>(written), test_data.size());
    
    // 检查文件大小
    off_t file_size = io_manager->size();
    EXPECT_GT(file_size, 0);
    EXPECT_EQ(static_cast<size_t>(file_size), test_data.size());
    
    io_manager->close();
}

TEST_F(FileIOManagerTest, WriteAndRead) {
    auto io_manager = create_io_manager(test_file, IOType::STANDARD_FIO);
    
    // 写入数据
    ssize_t written = io_manager->write(test_data.data(), test_data.size(), 0);
    EXPECT_GT(written, 0);
    EXPECT_EQ(static_cast<size_t>(written), test_data.size());
    
    // 读取数据
    std::vector<uint8_t> read_buffer(test_data.size());
    ssize_t read_bytes = io_manager->read(read_buffer.data(), read_buffer.size(), 0);
    
    EXPECT_GT(read_bytes, 0);
    EXPECT_EQ(static_cast<size_t>(read_bytes), test_data.size());
    EXPECT_EQ(read_buffer, test_data);
    
    io_manager->close();
}

TEST_F(FileIOManagerTest, ReadFromOffset) {
    auto io_manager = create_io_manager(test_file, IOType::STANDARD_FIO);
    
    // 先写入数据
    ssize_t written = io_manager->write(test_data.data(), test_data.size(), 0);
    EXPECT_GT(written, 0);
    
    // 从偏移量2读取4个字节
    std::vector<uint8_t> read_buffer(4);
    ssize_t read_bytes = io_manager->read(read_buffer.data(), 4, 2);
    
    EXPECT_EQ(read_bytes, 4);
    
    // 验证读取的数据是test_data[2:6]
    std::vector<uint8_t> expected(test_data.begin() + 2, test_data.begin() + 6);
    EXPECT_EQ(read_buffer, expected);
    
    io_manager->close();
}

TEST_F(FileIOManagerTest, MultipleWrites) {
    auto io_manager = create_io_manager(test_file, IOType::STANDARD_FIO);
    
    off_t offset = 0;
    for (int i = 0; i < 3; ++i) {
        ssize_t written = io_manager->write(test_data.data(), test_data.size(), offset);
        EXPECT_GT(written, 0);
        EXPECT_EQ(static_cast<size_t>(written), test_data.size());
        offset += written;
    }
    
    // 检查文件大小
    off_t file_size = io_manager->size();
    EXPECT_EQ(static_cast<size_t>(file_size), test_data.size() * 3);
    
    io_manager->close();
}

TEST_F(FileIOManagerTest, Sync) {
    auto io_manager = create_io_manager(test_file, IOType::STANDARD_FIO);
    
    // 写入数据
    ssize_t written = io_manager->write(test_data.data(), test_data.size(), 0);
    EXPECT_GT(written, 0);
    
    // 同步
    int sync_result = io_manager->sync();
    EXPECT_EQ(sync_result, 0);
    
    io_manager->close();
}

TEST_F(FileIOManagerTest, WriteToSpecificOffset) {
    auto io_manager = create_io_manager(test_file, IOType::STANDARD_FIO);
    
    // 在偏移量10写入数据
    ssize_t written = io_manager->write(test_data.data(), test_data.size(), 10);
    EXPECT_GT(written, 0);
    
    // 文件大小应该至少是10 + test_data.size()
    off_t file_size = io_manager->size();
    EXPECT_GE(static_cast<size_t>(file_size), 10 + test_data.size());
    
    // 从偏移量10读取数据
    std::vector<uint8_t> read_buffer(test_data.size());
    ssize_t read_bytes = io_manager->read(read_buffer.data(), read_buffer.size(), 10);
    
    EXPECT_GT(read_bytes, 0);
    EXPECT_EQ(read_buffer, test_data);
    
    io_manager->close();
}

TEST_F(FileIOManagerTest, Size) {
    auto io_manager = create_io_manager(test_file, IOType::STANDARD_FIO);
    
    // 初始大小应该是0
    off_t initial_size = io_manager->size();
    EXPECT_EQ(initial_size, 0);
    
    // 写入数据后检查大小
    ssize_t written = io_manager->write(test_data.data(), test_data.size(), 0);
    EXPECT_GT(written, 0);
    
    off_t after_write_size = io_manager->size();
    EXPECT_EQ(static_cast<size_t>(after_write_size), test_data.size());
    
    io_manager->close();
}

TEST_F(FileIOManagerTest, ReadBeyondFileSize) {
    auto io_manager = create_io_manager(test_file, IOType::STANDARD_FIO);
    
    // 写入少量数据
    ssize_t written = io_manager->write(test_data.data(), 4, 0);
    EXPECT_GT(written, 0);
    
    // 尝试读取超出文件大小的数据
    std::vector<uint8_t> read_buffer(test_data.size());
    ssize_t read_bytes = io_manager->read(read_buffer.data(), read_buffer.size(), 0);
    
    // 应该只读取到实际的文件内容
    EXPECT_EQ(read_bytes, 4);
    
    io_manager->close();
}

TEST_F(FileIOManagerTest, LargeDataReadWrite) {
    auto io_manager = create_io_manager(test_file, IOType::STANDARD_FIO);
    
    // 创建大数据块 (1MB)
    std::vector<uint8_t> large_data(1024 * 1024);
    std::iota(large_data.begin(), large_data.end(), 0);
    
    // 写入大数据
    ssize_t written = io_manager->write(large_data.data(), large_data.size(), 0);
    EXPECT_GT(written, 0);
    EXPECT_EQ(static_cast<size_t>(written), large_data.size());
    
    // 读取并验证
    std::vector<uint8_t> read_buffer(large_data.size());
    ssize_t read_bytes = io_manager->read(read_buffer.data(), read_buffer.size(), 0);
    
    EXPECT_GT(read_bytes, 0);
    EXPECT_EQ(static_cast<size_t>(read_bytes), large_data.size());
    EXPECT_EQ(read_buffer, large_data);
    
    io_manager->close();
}

TEST_F(FileIOManagerTest, ChunkedReadWrite) {
    auto io_manager = create_io_manager(test_file, IOType::STANDARD_FIO);
    
    // 分块写入数据
    std::vector<uint8_t> chunk = {0xAA, 0xBB, 0xCC, 0xDD};
    off_t offset = 0;
    
    for (int i = 0; i < 5; ++i) {
        ssize_t written = io_manager->write(chunk.data(), chunk.size(), offset);
        EXPECT_GT(written, 0);
        offset += written;
    }
    
    // 分块读取并验证
    offset = 0;
    for (int i = 0; i < 5; ++i) {
        std::vector<uint8_t> read_buffer(chunk.size());
        ssize_t read_bytes = io_manager->read(read_buffer.data(), read_buffer.size(), offset);
        
        EXPECT_GT(read_bytes, 0);
        EXPECT_EQ(read_buffer, chunk);
        offset += read_bytes;
    }
    
    io_manager->close();
}

// 内存映射I/O测试
class MMapIOManagerTest : public IOManagerTest {};

TEST_F(MMapIOManagerTest, CreateAndWrite) {
    auto io_manager = create_io_manager(test_file, IOType::MEMORY_MAP);
    
    // 写入数据
    ssize_t written = io_manager->write(test_data.data(), test_data.size(), 0);
    EXPECT_GT(written, 0);
    EXPECT_EQ(static_cast<size_t>(written), test_data.size());
    
    // 检查文件大小
    off_t file_size = io_manager->size();
    EXPECT_GT(file_size, 0);
    
    io_manager->close();
}

TEST_F(MMapIOManagerTest, WriteAndRead) {
    auto io_manager = create_io_manager(test_file, IOType::MEMORY_MAP);
    
    // 写入数据
    ssize_t written = io_manager->write(test_data.data(), test_data.size(), 0);
    EXPECT_GT(written, 0);
    
    // 读取数据
    std::vector<uint8_t> read_buffer(test_data.size());
    ssize_t read_bytes = io_manager->read(read_buffer.data(), read_buffer.size(), 0);
    
    EXPECT_GT(read_bytes, 0);
    EXPECT_EQ(read_buffer, test_data);
    
    io_manager->close();
}

TEST_F(MMapIOManagerTest, LargeFileExpansion) {
    auto io_manager = create_io_manager(test_file, IOType::MEMORY_MAP);
    
    // 写入数据到较大偏移量，测试文件扩展
    off_t large_offset = 50 * 1024; // 50KB
    ssize_t written = io_manager->write(test_data.data(), test_data.size(), large_offset);
    EXPECT_GT(written, 0);
    
    // 验证文件大小
    off_t file_size = io_manager->size();
    EXPECT_GE(file_size, large_offset + static_cast<off_t>(test_data.size()));
    
    // 读取验证
    std::vector<uint8_t> read_buffer(test_data.size());
    ssize_t read_bytes = io_manager->read(read_buffer.data(), read_buffer.size(), large_offset);
    
    EXPECT_GT(read_bytes, 0);
    EXPECT_EQ(read_buffer, test_data);
    
    io_manager->close();
}

// 性能对比测试
TEST_F(IOManagerTest, PerformanceComparison) {
    const size_t data_size = 10 * 1024; // 10KB
    const int iterations = 100;
    
    std::vector<uint8_t> perf_data(data_size);
    std::iota(perf_data.begin(), perf_data.end(), 0);
    
    // 标准I/O性能测试
    std::string std_file = test_dir + "/perf_std.dat";
    auto start_time = std::chrono::high_resolution_clock::now();
    {
        auto io_manager = create_io_manager(std_file, IOType::STANDARD_FIO);
        off_t offset = 0;
        for (int i = 0; i < iterations; ++i) {
            ssize_t written = io_manager->write(perf_data.data(), perf_data.size(), offset);
            if (written > 0) {
                offset += written;
            }
        }
        io_manager->sync();
        io_manager->close();
    }
    auto std_duration = std::chrono::high_resolution_clock::now() - start_time;
    
    // 内存映射I/O性能测试
    std::string mmap_file = test_dir + "/perf_mmap.dat";
    start_time = std::chrono::high_resolution_clock::now();
    {
        auto io_manager = create_io_manager(mmap_file, IOType::MEMORY_MAP);
        off_t offset = 0;
        for (int i = 0; i < iterations; ++i) {
            ssize_t written = io_manager->write(perf_data.data(), perf_data.size(), offset);
            if (written > 0) {
                offset += written;
            }
        }
        io_manager->sync();
        io_manager->close();
    }
    auto mmap_duration = std::chrono::high_resolution_clock::now() - start_time;
    
    // 输出性能对比信息
    auto std_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std_duration).count();
    auto mmap_ms = std::chrono::duration_cast<std::chrono::milliseconds>(mmap_duration).count();
    
    std::cout << "Standard I/O time: " << std_ms << " ms" << std::endl;
    std::cout << "Memory Map I/O time: " << mmap_ms << " ms" << std::endl;
    
    // 两种方式都应该成功完成
    EXPECT_GT(std_ms, 0);
    EXPECT_GT(mmap_ms, 0);
}