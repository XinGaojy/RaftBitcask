#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "bitcask/bitcask.h"
#include <thread>
#include <chrono>
#include <random>
#include <fstream>

namespace bitcask {
namespace test {

class BackupTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 创建临时目录
        temp_dir_ = "/tmp/bitcask_backup_test";
        backup_dir_ = "/tmp/bitcask_backup_test_backup";
        
        system(("rm -rf " + temp_dir_).c_str());
        system(("rm -rf " + backup_dir_).c_str());
        system(("mkdir -p " + temp_dir_).c_str());
        
        // 设置测试选项
        options_ = Options::default_options();
        options_.dir_path = temp_dir_;
        options_.data_file_size = 64 * 1024;  // 64KB for testing
        options_.sync_writes = false;
    }

    void TearDown() override {
        system(("rm -rf " + temp_dir_).c_str());
        system(("rm -rf " + backup_dir_).c_str());
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
    std::string backup_dir_;
    Options options_;
};

// 测试基本备份功能
TEST_F(BackupTest, BasicBackup) {
    auto db = DB::open(options_);
    
    // 插入一些测试数据
    for (int i = 0; i < 100; ++i) {
        std::string key = "key" + std::to_string(i);
        std::string value = "value" + std::to_string(i);
        db->put(string_to_bytes(key), string_to_bytes(value));
    }
    
    // 执行备份
    EXPECT_NO_THROW(db->backup(backup_dir_));
    
    // 验证备份目录存在
    struct stat st;
    EXPECT_EQ(stat(backup_dir_.c_str(), &st), 0);
    EXPECT_TRUE(S_ISDIR(st.st_mode));
    
    db->close();
}

// 测试备份后恢复数据
TEST_F(BackupTest, BackupAndRestore) {
    std::vector<std::pair<std::string, std::string>> test_data;
    
    // 创建原始数据库并插入数据
    {
        auto db = DB::open(options_);
        
        for (int i = 0; i < 500; ++i) {
            std::string key = "test_key_" + std::to_string(i);
            std::string value = "test_value_" + std::to_string(i);
            test_data.push_back({key, value});
            db->put(string_to_bytes(key), string_to_bytes(value));
        }
        
        // 删除一些数据
        for (int i = 0; i < 100; ++i) {
            std::string key = "test_key_" + std::to_string(i);
            db->remove(string_to_bytes(key));
        }
        
        // 执行备份
        EXPECT_NO_THROW(db->backup(backup_dir_));
        db->close();
    }
    
    // 从备份恢复数据库
    {
        Options backup_options = options_;
        backup_options.dir_path = backup_dir_;
        auto restored_db = DB::open(backup_options);
        
        // 验证被删除的数据不存在
        for (int i = 0; i < 100; ++i) {
            std::string key = "test_key_" + std::to_string(i);
            EXPECT_THROW(restored_db->get(string_to_bytes(key)), KeyNotFoundError);
        }
        
        // 验证剩余的数据存在且正确
        for (int i = 100; i < 500; ++i) {
            std::string key = "test_key_" + std::to_string(i);
            std::string expected_value = "test_value_" + std::to_string(i);
            auto actual_value = restored_db->get(string_to_bytes(key));
            EXPECT_EQ(bytes_to_string(actual_value), expected_value);
        }
        
        restored_db->close();
    }
}

// 测试大量数据的备份
TEST_F(BackupTest, LargeDataBackup) {
    auto db = DB::open(options_);
    
    // 插入大量数据，确保产生多个数据文件
    for (int i = 0; i < 5000; ++i) {
        std::string key = "large_key_" + std::to_string(i);
        Bytes value = random_value(200);  // 200字节随机数据
        db->put(string_to_bytes(key), value);
    }
    
    // 执行备份
    EXPECT_NO_THROW(db->backup(backup_dir_));
    
    // 验证备份的完整性
    Options backup_options = options_;
    backup_options.dir_path = backup_dir_;
    auto backup_db = DB::open(backup_options);
    
    // 验证数据数量
    auto original_keys = db->list_keys();
    auto backup_keys = backup_db->list_keys();
    EXPECT_EQ(original_keys.size(), backup_keys.size());
    
    // 验证随机抽样的数据
    for (int i = 0; i < 100; ++i) {
        int random_index = rand() % 5000;
        std::string key = "large_key_" + std::to_string(random_index);
        
        auto original_value = db->get(string_to_bytes(key));
        auto backup_value = backup_db->get(string_to_bytes(key));
        
        EXPECT_EQ(original_value, backup_value);
    }
    
    db->close();
    backup_db->close();
}

// 测试备份时排除文件锁
TEST_F(BackupTest, ExcludeFileLock) {
    auto db = DB::open(options_);
    
    // 插入一些数据
    for (int i = 0; i < 50; ++i) {
        db->put(string_to_bytes("key" + std::to_string(i)), 
                string_to_bytes("value" + std::to_string(i)));
    }
    
    // 执行备份
    EXPECT_NO_THROW(db->backup(backup_dir_));
    
    // 验证备份目录中不包含文件锁文件
    std::string lock_file_path = backup_dir_ + "/" + FILE_LOCK_NAME;
    struct stat st;
    EXPECT_NE(stat(lock_file_path.c_str(), &st), 0);  // 文件不应该存在
    
    // 但是应该包含数据文件
    std::string data_file_path = backup_dir_ + "/000000000.data";
    EXPECT_EQ(stat(data_file_path.c_str(), &st), 0);  // 数据文件应该存在
    
    db->close();
}

// 测试备份到已存在的目录
TEST_F(BackupTest, BackupToExistingDirectory) {
    auto db = DB::open(options_);
    
    // 插入数据
    db->put(string_to_bytes("test_key"), string_to_bytes("test_value"));
    
    // 创建备份目录并放入一些文件
    system(("mkdir -p " + backup_dir_).c_str());
    std::string dummy_file = backup_dir_ + "/dummy.txt";
    std::ofstream(dummy_file) << "dummy content";
    
    // 执行备份（应该覆盖现有内容）
    EXPECT_NO_THROW(db->backup(backup_dir_));
    
    // 验证数据正确备份
    Options backup_options = options_;
    backup_options.dir_path = backup_dir_;
    auto backup_db = DB::open(backup_options);
    
    auto value = backup_db->get(string_to_bytes("test_key"));
    EXPECT_EQ(bytes_to_string(value), "test_value");
    
    db->close();
    backup_db->close();
}

// 测试并发备份
TEST_F(BackupTest, ConcurrentBackup) {
    auto db = DB::open(options_);
    
    // 在一个线程中持续写入数据
    std::thread writer_thread([&db]() {
        for (int i = 0; i < 1000; ++i) {
            std::string key = "concurrent_key_" + std::to_string(i);
            std::string value = "concurrent_value_" + std::to_string(i);
            db->put(Bytes(key.begin(), key.end()), Bytes(value.begin(), value.end()));
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
    });
    
    // 等待一些数据写入
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    // 执行备份
    EXPECT_NO_THROW(db->backup(backup_dir_));
    
    // 等待写入线程完成
    writer_thread.join();
    
    // 验证备份包含一些数据
    Options backup_options = options_;
    backup_options.dir_path = backup_dir_;
    auto backup_db = DB::open(backup_options);
    
    auto backup_keys = backup_db->list_keys();
    EXPECT_GT(backup_keys.size(), 0);  // 应该至少有一些数据
    
    db->close();
    backup_db->close();
}

// 测试备份后的统计信息
TEST_F(BackupTest, BackupStatistics) {
    auto db = DB::open(options_);
    
    // 插入数据
    for (int i = 0; i < 200; ++i) {
        db->put(string_to_bytes("stat_key_" + std::to_string(i)), 
                random_value(150));
    }
    
    // 删除一些数据
    for (int i = 0; i < 50; ++i) {
        db->remove(string_to_bytes("stat_key_" + std::to_string(i)));
    }
    
    auto original_stat = db->stat();
    
    // 执行备份
    EXPECT_NO_THROW(db->backup(backup_dir_));
    
    // 检查备份数据库的统计信息
    Options backup_options = options_;
    backup_options.dir_path = backup_dir_;
    auto backup_db = DB::open(backup_options);
    
    auto backup_stat = backup_db->stat();
    
    // 备份的数据库应该有相同数量的有效键
    EXPECT_EQ(backup_stat.key_num, original_stat.key_num);
    
    db->close();
    backup_db->close();
}

// 测试空数据库备份
TEST_F(BackupTest, EmptyDatabaseBackup) {
    auto db = DB::open(options_);
    
    // 不插入任何数据，直接备份
    EXPECT_NO_THROW(db->backup(backup_dir_));
    
    // 验证可以从备份恢复空数据库
    Options backup_options = options_;
    backup_options.dir_path = backup_dir_;
    auto backup_db = DB::open(backup_options);
    
    auto keys = backup_db->list_keys();
    EXPECT_EQ(keys.size(), 0);
    
    db->close();
    backup_db->close();
}

}  // namespace test
}  // namespace bitcask
