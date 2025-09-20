#include <gtest/gtest.h>
#include "bitcask/mmap_io.h"
#include "bitcask/test_utils.h"
#include <cstring>
#include <vector>

class MMapIOTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir_ = bitcask::TestUtils::create_temp_dir("mmap_test_");
        ASSERT_FALSE(test_dir_.empty());
        test_file_ = test_dir_ + "/test.dat";
    }

    void TearDown() override {
        if (!test_dir_.empty()) {
            bitcask::TestUtils::remove_dir(test_dir_);
        }
    }

    std::string test_dir_;
    std::string test_file_;
};

TEST_F(MMapIOTest, BasicReadWrite) {
    auto mmap_io = std::make_unique<bitcask::MMapIOManager>(test_file_);

    // 测试写入
    std::string data = "Hello, MMap World!";
    ssize_t written = mmap_io->write(data.data(), data.size(), 0);
    EXPECT_EQ(written, static_cast<ssize_t>(data.size()));

    // 测试读取
    std::vector<char> buffer(data.size());
    ssize_t read_bytes = mmap_io->read(buffer.data(), buffer.size(), 0);
    EXPECT_EQ(read_bytes, static_cast<ssize_t>(data.size()));
    EXPECT_EQ(std::string(buffer.data(), buffer.size()), data);

    mmap_io->close();
}

TEST_F(MMapIOTest, LargeDataReadWrite) {
    auto mmap_io = std::make_unique<bitcask::MMapIOManager>(test_file_);

    // 生成大量测试数据
    bitcask::TestDataGenerator generator;
    auto large_data = generator.random_bytes(100 * 1024); // 100KB

    // 写入大数据
    ssize_t written = mmap_io->write(large_data.data(), large_data.size(), 0);
    EXPECT_EQ(written, static_cast<ssize_t>(large_data.size()));

    // 读取并验证
    std::vector<uint8_t> buffer(large_data.size());
    ssize_t read_bytes = mmap_io->read(buffer.data(), buffer.size(), 0);
    EXPECT_EQ(read_bytes, static_cast<ssize_t>(large_data.size()));
    EXPECT_EQ(buffer, large_data);

    mmap_io->close();
}

TEST_F(MMapIOTest, RandomAccess) {
    auto mmap_io = std::make_unique<bitcask::MMapIOManager>(test_file_);

    // 写入多个数据块
    std::vector<std::string> blocks = {
        "Block1", "Block2", "Block3", "Block4"
    };

    off_t offset = 0;
    for (const auto& block : blocks) {
        ssize_t written = mmap_io->write(block.data(), block.size(), offset);
        EXPECT_EQ(written, static_cast<ssize_t>(block.size()));
        offset += block.size();
    }

    // 随机读取验证
    offset = 0;
    for (const auto& block : blocks) {
        std::vector<char> buffer(block.size());
        ssize_t read_bytes = mmap_io->read(buffer.data(), buffer.size(), offset);
        EXPECT_EQ(read_bytes, static_cast<ssize_t>(block.size()));
        EXPECT_EQ(std::string(buffer.data(), buffer.size()), block);
        offset += block.size();
    }

    mmap_io->close();
}

TEST_F(MMapIOTest, FileExpansion) {
    auto mmap_io = std::make_unique<bitcask::MMapIOManager>(test_file_);

    // 写入数据到较大偏移量，测试文件自动扩展
    std::string data = "Test expansion";
    off_t large_offset = 50 * 1024; // 50KB
    
    ssize_t written = mmap_io->write(data.data(), data.size(), large_offset);
    EXPECT_EQ(written, static_cast<ssize_t>(data.size()));

    // 验证文件大小
    off_t file_size = mmap_io->size();
    EXPECT_GE(file_size, large_offset + static_cast<off_t>(data.size()));

    // 读取验证
    std::vector<char> buffer(data.size());
    ssize_t read_bytes = mmap_io->read(buffer.data(), buffer.size(), large_offset);
    EXPECT_EQ(read_bytes, static_cast<ssize_t>(data.size()));
    EXPECT_EQ(std::string(buffer.data(), buffer.size()), data);

    mmap_io->close();
}

TEST_F(MMapIOTest, Sync) {
    auto mmap_io = std::make_unique<bitcask::MMapIOManager>(test_file_);

    std::string data = "Sync test data";
    mmap_io->write(data.data(), data.size(), 0);

    // 测试同步操作
    int sync_result = mmap_io->sync();
    EXPECT_EQ(sync_result, 0);

    mmap_io->close();
}

TEST_F(MMapIOTest, MultipleWrites) {
    auto mmap_io = std::make_unique<bitcask::MMapIOManager>(test_file_);

    // 多次写入测试
    std::string accumulated_data;
    off_t current_offset = 0;

    for (int i = 0; i < 10; ++i) {
        std::string data = "Data chunk " + std::to_string(i) + " ";
        ssize_t written = mmap_io->write(data.data(), data.size(), current_offset);
        EXPECT_EQ(written, static_cast<ssize_t>(data.size()));
        
        accumulated_data += data;
        current_offset += data.size();
    }

    // 一次性读取所有数据
    std::vector<char> buffer(accumulated_data.size());
    ssize_t read_bytes = mmap_io->read(buffer.data(), buffer.size(), 0);
    EXPECT_EQ(read_bytes, static_cast<ssize_t>(accumulated_data.size()));
    EXPECT_EQ(std::string(buffer.data(), buffer.size()), accumulated_data);

    mmap_io->close();
}

TEST_F(MMapIOTest, BinarySafeData) {
    auto mmap_io = std::make_unique<bitcask::MMapIOManager>(test_file_);

    // 测试二进制安全数据
    bitcask::TestDataGenerator generator;
    auto binary_data = generator.binary_safe_data(1024);

    ssize_t written = mmap_io->write(binary_data.data(), binary_data.size(), 0);
    EXPECT_EQ(written, static_cast<ssize_t>(binary_data.size()));

    std::vector<uint8_t> buffer(binary_data.size());
    ssize_t read_bytes = mmap_io->read(buffer.data(), buffer.size(), 0);
    EXPECT_EQ(read_bytes, static_cast<ssize_t>(binary_data.size()));
    EXPECT_EQ(buffer, binary_data);

    mmap_io->close();
}

// 性能对比测试（可选）
TEST_F(MMapIOTest, PerformanceComparison) {
    const size_t data_size = 10 * 1024; // 10KB
    const int iterations = 100;

    bitcask::TestDataGenerator generator;
    auto test_data = generator.random_bytes(data_size);

    // MMap性能测试
    bitcask::TestUtils::Timer mmap_timer;
    {
        auto mmap_io = std::make_unique<bitcask::MMapIOManager>(test_file_ + "_mmap");
        
        mmap_timer.start();
        for (int i = 0; i < iterations; ++i) {
            mmap_io->write(test_data.data(), test_data.size(), i * data_size);
        }
        mmap_io->sync();
        mmap_timer.stop();
        
        mmap_io->close();
    }

    std::cout << "MMap write time: " << mmap_timer.elapsed_ms() << " ms" << std::endl;
    EXPECT_GT(mmap_timer.elapsed_ms(), 0.0);
}
