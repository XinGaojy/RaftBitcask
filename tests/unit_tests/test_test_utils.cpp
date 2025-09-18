#include <gtest/gtest.h>
#include "bitcask/test_utils.h"
#include <set>
#include <algorithm>
#include <map>
#include <thread>
#include <chrono>

class TestUtilsTest : public ::testing::Test {
protected:
    void SetUp() override {
        generator_ = std::make_unique<bitcask::TestDataGenerator>(12345); // 固定种子以便复现
    }

    std::unique_ptr<bitcask::TestDataGenerator> generator_;
};

TEST_F(TestUtilsTest, RandomBytesGeneration) {
    size_t length = 100;
    auto bytes1 = generator_->random_bytes(length);
    auto bytes2 = generator_->random_bytes(length);

    EXPECT_EQ(bytes1.size(), length);
    EXPECT_EQ(bytes2.size(), length);
    EXPECT_NE(bytes1, bytes2); // 应该生成不同的数据
}

TEST_F(TestUtilsTest, RandomStringGeneration) {
    size_t length = 50;
    std::string charset = "ABC123";
    
    auto str1 = generator_->random_string(length, charset);
    auto str2 = generator_->random_string(length, charset);

    EXPECT_EQ(str1.size(), length);
    EXPECT_EQ(str2.size(), length);
    EXPECT_NE(str1, str2);

    // 检查字符是否都在指定字符集中
    for (char c : str1) {
        EXPECT_NE(charset.find(c), std::string::npos);
    }
}

TEST_F(TestUtilsTest, RandomKeyGeneration) {
    auto key1 = generator_->random_key(10, 20);
    auto key2 = generator_->random_key(10, 20);

    EXPECT_GE(key1.size(), 10u);
    EXPECT_LE(key1.size(), 20u);
    EXPECT_GE(key2.size(), 10u);
    EXPECT_LE(key2.size(), 20u);
    EXPECT_NE(key1, key2);
}

TEST_F(TestUtilsTest, RandomValueGeneration) {
    auto value1 = generator_->random_value(100, 200);
    auto value2 = generator_->random_value(100, 200);

    EXPECT_GE(value1.size(), 100u);
    EXPECT_LE(value1.size(), 200u);
    EXPECT_GE(value2.size(), 100u);
    EXPECT_LE(value2.size(), 200u);
    EXPECT_NE(value1, value2);
}

TEST_F(TestUtilsTest, RandomIntGeneration) {
    int min = 10, max = 100;
    std::set<int> generated_ints;

    for (int i = 0; i < 50; ++i) {
        int value = generator_->random_int(min, max);
        EXPECT_GE(value, min);
        EXPECT_LE(value, max);
        generated_ints.insert(value);
    }

    // 应该生成了多个不同的值
    EXPECT_GT(generated_ints.size(), 10u);
}

TEST_F(TestUtilsTest, RandomDoubleGeneration) {
    double min = 1.0, max = 10.0;
    std::set<double> generated_doubles;

    for (int i = 0; i < 50; ++i) {
        double value = generator_->random_double(min, max);
        EXPECT_GE(value, min);
        EXPECT_LE(value, max);
        generated_doubles.insert(value);
    }

    // 应该生成了多个不同的值
    EXPECT_GT(generated_doubles.size(), 10u);
}

TEST_F(TestUtilsTest, RandomKVPairsGeneration) {
    size_t count = 10;
    size_t key_size = 16;
    size_t value_size = 64;

    auto pairs = generator_->random_kv_pairs(count, key_size, value_size);

    EXPECT_EQ(pairs.size(), count);

    for (const auto& pair : pairs) {
        EXPECT_EQ(pair.first.size(), key_size);
        EXPECT_EQ(pair.second.size(), value_size);
    }

    // 检查key的唯一性
    std::set<bitcask::Bytes> keys;
    for (const auto& pair : pairs) {
        keys.insert(pair.first);
    }
    EXPECT_EQ(keys.size(), count); // 所有key应该是唯一的
}

TEST_F(TestUtilsTest, OrderedKVPairsGeneration) {
    size_t count = 10;
    std::string prefix = "test_key_";
    size_t value_size = 32;

    auto pairs = generator_->ordered_kv_pairs(count, prefix, value_size);

    EXPECT_EQ(pairs.size(), count);

    // 检查key是否有序
    for (size_t i = 1; i < pairs.size(); ++i) {
        EXPECT_LT(pairs[i-1].first, pairs[i].first);
    }

    // 检查key前缀
    for (const auto& pair : pairs) {
        std::string key_str(pair.first.begin(), pair.first.end());
        EXPECT_EQ(key_str.substr(0, prefix.size()), prefix);
        EXPECT_EQ(pair.second.size(), value_size);
    }
}

TEST_F(TestUtilsTest, DuplicateKeyPairsGeneration) {
    std::vector<bitcask::Bytes> base_keys = {
        {1, 2, 3}, {4, 5, 6}, {7, 8, 9}
    };
    size_t repeat_factor = 3;

    auto pairs = generator_->duplicate_key_pairs(base_keys, repeat_factor);

    EXPECT_EQ(pairs.size(), base_keys.size() * repeat_factor);

    // 统计每个key出现的次数
    std::map<bitcask::Bytes, int> key_counts;
    for (const auto& pair : pairs) {
        key_counts[pair.first]++;
    }

    EXPECT_EQ(key_counts.size(), base_keys.size());
    for (const auto& base_key : base_keys) {
        EXPECT_EQ(key_counts[base_key], static_cast<int>(repeat_factor));
    }
}

TEST_F(TestUtilsTest, BinarySafeDataGeneration) {
    size_t length = 256;
    auto data = generator_->binary_safe_data(length);

    EXPECT_EQ(data.size(), length);

    // 检查是否包含所有可能的字节值
    std::set<uint8_t> byte_values(data.begin(), data.end());
    EXPECT_EQ(byte_values.size(), length); // 对于256字节，应该包含所有0-255的值
}

TEST_F(TestUtilsTest, PerformanceTestDataGeneration) {
    size_t count = 100;
    size_t key_size = 16;
    size_t value_size = 64;
    double write_ratio = 0.7;

    auto test_data = generator_->performance_test_data(count, key_size, value_size, write_ratio);

    EXPECT_EQ(test_data.kv_pairs.size(), count);
    EXPECT_EQ(test_data.operations.size(), count);
    EXPECT_EQ(test_data.indices.size(), count);

    // 检查写操作比例
    int write_count = std::count(test_data.operations.begin(), test_data.operations.end(), true);
    double actual_ratio = static_cast<double>(write_count) / count;
    EXPECT_NEAR(actual_ratio, write_ratio, 0.1); // 允许10%的误差

    // 检查所有数据的大小
    for (const auto& pair : test_data.kv_pairs) {
        EXPECT_EQ(pair.first.size(), key_size);
        EXPECT_EQ(pair.second.size(), value_size);
    }
}

// TestUtils类测试
TEST(TestUtilsStaticTest, BytesEqual) {
    bitcask::Bytes a = {1, 2, 3, 4};
    bitcask::Bytes b = {1, 2, 3, 4};
    bitcask::Bytes c = {1, 2, 3, 5};

    EXPECT_TRUE(bitcask::TestUtils::bytes_equal(a, b));
    EXPECT_FALSE(bitcask::TestUtils::bytes_equal(a, c));
}

TEST(TestUtilsStaticTest, TempDirCreation) {
    std::string temp_dir = bitcask::TestUtils::create_temp_dir("test_");
    EXPECT_FALSE(temp_dir.empty());
    
    // 清理
    EXPECT_TRUE(bitcask::TestUtils::remove_dir(temp_dir));
}

TEST(TestUtilsStaticTest, Checksum) {
    bitcask::Bytes data = {1, 2, 3, 4, 5};
    uint32_t checksum1 = bitcask::TestUtils::checksum(data);
    uint32_t checksum2 = bitcask::TestUtils::checksum(data);
    
    EXPECT_EQ(checksum1, checksum2); // 相同数据应该产生相同校验和

    data.push_back(6);
    uint32_t checksum3 = bitcask::TestUtils::checksum(data);
    EXPECT_NE(checksum1, checksum3); // 不同数据应该产生不同校验和
}

TEST(TestUtilsStaticTest, FormatBytes) {
    EXPECT_EQ(bitcask::TestUtils::format_bytes(512), "512.00 B");
    EXPECT_EQ(bitcask::TestUtils::format_bytes(1024), "1.00 KB");
    EXPECT_EQ(bitcask::TestUtils::format_bytes(1536), "1.50 KB");
    EXPECT_EQ(bitcask::TestUtils::format_bytes(1024 * 1024), "1.00 MB");
}

TEST(TestUtilsStaticTest, Timer) {
    bitcask::TestUtils::Timer timer;
    
    timer.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    timer.stop();
    
    double elapsed_ms = timer.elapsed_ms();
    double elapsed_us = timer.elapsed_us();
    
    EXPECT_GE(elapsed_ms, 9.0); // 至少9ms
    EXPECT_LE(elapsed_ms, 50.0); // 不超过50ms (宽松的上限)
    EXPECT_GE(elapsed_us, elapsed_ms * 1000 - 1000); // 微秒应该约为毫秒的1000倍
}
