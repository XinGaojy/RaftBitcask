#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "bitcask/bitcask.h"
#include "bitcask/utils.h"
#include <random>
#include <thread>
#include <chrono>

namespace bitcask {
namespace test {

class MergeTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 创建临时目录
        temp_dir_ = "/tmp/bitcask_merge_test";
        if (utils::directory_exists(temp_dir_)) {
            utils::remove_directory(temp_dir_);
        }
        utils::create_directory(temp_dir_);
        
        // 设置测试选项
        options_.dir_path = temp_dir_;
        options_.data_file_size = 64 * 1024;  // 64KB for testing
        options_.data_file_merge_ratio = 0.5;
        options_.sync_writes = false;
    }

    void TearDown() override {
        if (utils::directory_exists(temp_dir_)) {
            utils::remove_directory(temp_dir_);
        }
    }

    Bytes string_to_bytes(const std::string& str) {
        return Bytes(str.begin(), str.end());
    }

    std::string bytes_to_string(const Bytes& bytes) {
        return std::string(bytes.begin(), bytes.end());
    }

    Bytes random_value(size_t size) {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_int_distribution<uint8_t> dis(0, 255);
        
        Bytes result(size);
        for (size_t i = 0; i < size; ++i) {
            result[i] = dis(gen);
        }
        return result;
    }

    std::string temp_dir_;
    Options options_;
};

// 测试空数据库的merge操作
TEST_F(MergeTest, MergeEmptyDatabase) {
    auto db = DB::open(options_);
    EXPECT_NO_THROW(db->merge());
    db->close();
}

// 测试没有达到merge阈值的情况
TEST_F(MergeTest, MergeRatioNotReached) {
    options_.data_file_merge_ratio = 0.9;  // 设置很高的阈值
    auto db = DB::open(options_);
    
    // 插入一些数据
    for (int i = 0; i < 100; ++i) {
        db->put(string_to_bytes("key" + std::to_string(i)), 
                string_to_bytes("value" + std::to_string(i)));
    }
    
    // merge应该抛出异常，因为没有达到阈值
    EXPECT_THROW(db->merge(), MergeRatioUnreachedError);
    db->close();
}

// 测试基本的merge功能
TEST_F(MergeTest, BasicMerge) {
    options_.data_file_merge_ratio = 0.0;  // 总是可以merge
    auto db = DB::open(options_);
    
    // 插入一些数据
    std::vector<std::string> keys;
    for (int i = 0; i < 1000; ++i) {
        std::string key = "key" + std::to_string(i);
        keys.push_back(key);
        db->put(string_to_bytes(key), random_value(1024));
    }
    
    // 删除一半的数据，产生无效数据
    for (size_t i = 0; i < keys.size() / 2; ++i) {
        db->remove(string_to_bytes(keys[i]));
    }
    
    // 执行merge
    EXPECT_NO_THROW(db->merge());
    
    // 验证剩余的数据仍然存在
    for (size_t i = keys.size() / 2; i < keys.size(); ++i) {
        EXPECT_NO_THROW(db->get(string_to_bytes(keys[i])));
    }
    
    // 验证被删除的数据不存在
    for (size_t i = 0; i < keys.size() / 2; ++i) {
        EXPECT_THROW(db->get(string_to_bytes(keys[i])), KeyNotFoundError);
    }
    
    db->close();
}

// 测试merge后重启数据库
TEST_F(MergeTest, MergeAndRestart) {
    options_.data_file_merge_ratio = 0.0;
    
    std::vector<std::pair<std::string, std::string>> test_data;
    
    {
        auto db = DB::open(options_);
        
        // 插入测试数据
        for (int i = 0; i < 500; ++i) {
            std::string key = "key" + std::to_string(i);
            std::string value = "value" + std::to_string(i);
            test_data.push_back({key, value});
            db->put(string_to_bytes(key), string_to_bytes(value));
        }
        
        // 删除一些数据
        for (int i = 0; i < 200; ++i) {
            db->remove(string_to_bytes("key" + std::to_string(i)));
        }
        
        // 更新一些数据
        for (int i = 200; i < 300; ++i) {
            std::string key = "key" + std::to_string(i);
            std::string new_value = "updated_value" + std::to_string(i);
            test_data[i] = {key, new_value};  // 更新测试数据
            db->put(string_to_bytes(key), string_to_bytes(new_value));
        }
        
        // 执行merge
        EXPECT_NO_THROW(db->merge());
        db->close();
    }
    
    // 重新打开数据库
    {
        auto db = DB::open(options_);
        
        // 验证被删除的数据不存在
        for (int i = 0; i < 200; ++i) {
            EXPECT_THROW(db->get(string_to_bytes("key" + std::to_string(i))), KeyNotFoundError);
        }
        
        // 验证剩余的数据正确
        for (int i = 200; i < 500; ++i) {
            auto value = db->get(string_to_bytes(test_data[i].first));
            EXPECT_EQ(bytes_to_string(value), test_data[i].second);
        }
        
        db->close();
    }
}

// 测试并发merge（应该抛出异常）
TEST_F(MergeTest, ConcurrentMerge) {
    options_.data_file_merge_ratio = 0.0;
    auto db = DB::open(options_);
    
    // 插入一些数据
    for (int i = 0; i < 100; ++i) {
        db->put(string_to_bytes("key" + std::to_string(i)), 
                string_to_bytes("value" + std::to_string(i)));
    }
    
    // 在一个线程中开始merge
    std::thread merge_thread([&db]() {
        try {
            db->merge();
        } catch (...) {
            // 忽略异常
        }
    });
    
    // 立即尝试再次merge，应该抛出异常
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    EXPECT_THROW(db->merge(), MergeInProgressError);
    
    merge_thread.join();
    db->close();
}

// 测试大量数据的merge
TEST_F(MergeTest, LargeDataMerge) {
    options_.data_file_merge_ratio = 0.0;
    options_.data_file_size = 32 * 1024;  // 32KB files
    auto db = DB::open(options_);
    
    // 插入大量数据，确保产生多个数据文件
    for (int i = 0; i < 5000; ++i) {
        db->put(string_to_bytes("key" + std::to_string(i)), random_value(100));
    }
    
    // 删除大量数据
    for (int i = 0; i < 2500; ++i) {
        db->remove(string_to_bytes("key" + std::to_string(i)));
    }
    
    // 执行merge
    EXPECT_NO_THROW(db->merge());
    
    // 验证剩余数据的正确性
    for (int i = 2500; i < 5000; ++i) {
        EXPECT_NO_THROW(db->get(string_to_bytes("key" + std::to_string(i))));
    }
    
    db->close();
}

// 测试merge后的统计信息
TEST_F(MergeTest, MergeStatistics) {
    options_.data_file_merge_ratio = 0.0;
    auto db = DB::open(options_);
    
    // 插入数据
    for (int i = 0; i < 1000; ++i) {
        db->put(string_to_bytes("key" + std::to_string(i)), random_value(100));
    }
    
    auto stat_before = db->stat();
    
    // 删除一半数据
    for (int i = 0; i < 500; ++i) {
        db->remove(string_to_bytes("key" + std::to_string(i)));
    }
    
    // 执行merge
    db->merge();
    
    auto stat_after = db->stat();
    
    // merge后，key数量应该减少，可回收空间应该减少
    EXPECT_LT(stat_after.key_num, stat_before.key_num);
    EXPECT_LT(stat_after.reclaimable_size, stat_before.reclaimable_size);
    
    db->close();
}

}  // namespace test
}  // namespace bitcask
