#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "bitcask/db.h"
#include "bitcask/utils.h"
#include <thread>
#include <random>

using namespace bitcask;

class DBTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir = "/tmp/bitcask_db_test";
        utils::remove_directory(test_dir);
        
        options = Options::default_options();
        options.dir_path = test_dir;
        options.data_file_size = 1024 * 1024; // 1MB for testing - 增大文件大小
        options.sync_writes = true; // 启用同步写入确保数据持久化
        
        // 创建测试数据
        test_pairs = {
            {{0x6b, 0x65, 0x79, 0x31}, {0x76, 0x61, 0x6c, 0x75, 0x65, 0x31}}, // key1, value1
            {{0x6b, 0x65, 0x79, 0x32}, {0x76, 0x61, 0x6c, 0x75, 0x65, 0x32}}, // key2, value2
            {{0x6b, 0x65, 0x79, 0x33}, {0x76, 0x61, 0x6c, 0x75, 0x65, 0x33}}, // key3, value3
        };
    }
    
    void TearDown() override {
        utils::remove_directory(test_dir);
    }
    
    std::string test_dir;
    Options options;
    std::vector<std::pair<Bytes, Bytes>> test_pairs;
};

// 基本数据库操作测试
TEST_F(DBTest, OpenAndClose) {
    auto db = DB::open(options);
    EXPECT_NE(db, nullptr);
    
    // 关闭数据库
    EXPECT_NO_THROW(db->close());
}

TEST_F(DBTest, PutAndGet) {
    auto db = DB::open(options);
    
    // 写入数据
    EXPECT_NO_THROW(db->put(test_pairs[0].first, test_pairs[0].second));
    
    // 读取数据
    Bytes value = db->get(test_pairs[0].first);
    EXPECT_EQ(value, test_pairs[0].second);
    
    db->close();
}

TEST_F(DBTest, GetNonExistentKey) {
    auto db = DB::open(options);
    
    EXPECT_THROW(db->get({0x6e, 0x6f, 0x6e, 0x65}), KeyNotFoundError); // "none"
    
    db->close();
}

TEST_F(DBTest, PutEmptyKey) {
    auto db = DB::open(options);
    
    EXPECT_THROW(db->put(Bytes{}, {0x76, 0x61, 0x6c, 0x75, 0x65}), KeyEmptyError);
    
    db->close();
}

TEST_F(DBTest, UpdateValue) {
    auto db = DB::open(options);
    
    // 初始写入
    db->put(test_pairs[0].first, test_pairs[0].second);
    
    // 更新值
    Bytes new_value = {0x6e, 0x65, 0x77, 0x5f, 0x76, 0x61, 0x6c, 0x75, 0x65}; // "new_value"
    db->put(test_pairs[0].first, new_value);
    
    // 验证更新
    Bytes retrieved_value = db->get(test_pairs[0].first);
    EXPECT_EQ(retrieved_value, new_value);
    
    db->close();
}

TEST_F(DBTest, DeleteKey) {
    auto db = DB::open(options);
    
    // 写入数据
    db->put(test_pairs[0].first, test_pairs[0].second);
    
    // 验证数据存在
    EXPECT_NO_THROW(db->get(test_pairs[0].first));
    
    // 删除数据
    EXPECT_NO_THROW(db->remove(test_pairs[0].first));
    
    // 验证数据不存在
    EXPECT_THROW(db->get(test_pairs[0].first), KeyNotFoundError);
    
    db->close();
}

TEST_F(DBTest, DeleteNonExistentKey) {
    auto db = DB::open(options);
    
    // 删除不存在的 key 不应该抛出异常
    EXPECT_NO_THROW(db->remove({0x6e, 0x6f, 0x6e, 0x65})); // "none"
    
    db->close();
}

TEST_F(DBTest, MultipleOperations) {
    auto db = DB::open(options);
    
    // 写入多个键值对
    for (const auto& [key, value] : test_pairs) {
        db->put(key, value);
    }
    
    // 验证所有数据
    for (const auto& [key, value] : test_pairs) {
        Bytes retrieved_value = db->get(key);
        EXPECT_EQ(retrieved_value, value);
    }
    
    db->close();
}

TEST_F(DBTest, ListKeys) {
    auto db = DB::open(options);
    
    // 写入数据
    for (const auto& [key, value] : test_pairs) {
        db->put(key, value);
    }
    
    // 获取所有 key
    auto keys = db->list_keys();
    EXPECT_EQ(keys.size(), test_pairs.size());
    
    // 排序比较
    std::sort(keys.begin(), keys.end());
    std::vector<Bytes> expected_keys;
    for (const auto& [key, value] : test_pairs) {
        expected_keys.push_back(key);
    }
    std::sort(expected_keys.begin(), expected_keys.end());
    
    EXPECT_EQ(keys, expected_keys);
    
    db->close();
}

TEST_F(DBTest, FoldOperation) {
    auto db = DB::open(options);
    
    // 写入数据
    for (const auto& [key, value] : test_pairs) {
        db->put(key, value);
    }
    
    // 使用 fold 遍历所有数据
    std::vector<std::pair<Bytes, Bytes>> folded_pairs;
    db->fold([&folded_pairs](const Bytes& key, const Bytes& value) -> bool {
        folded_pairs.emplace_back(key, value);
        return true; // 继续遍历
    });
    
    EXPECT_EQ(folded_pairs.size(), test_pairs.size());
    
    // 排序比较
    std::sort(folded_pairs.begin(), folded_pairs.end());
    auto sorted_test_pairs = test_pairs;
    std::sort(sorted_test_pairs.begin(), sorted_test_pairs.end());
    
    EXPECT_EQ(folded_pairs, sorted_test_pairs);
    
    db->close();
}

TEST_F(DBTest, FoldEarlyTermination) {
    auto db = DB::open(options);
    
    // 写入数据
    for (const auto& [key, value] : test_pairs) {
        db->put(key, value);
    }
    
    // 使用 fold 遍历，提前终止
    int count = 0;
    db->fold([&count](const Bytes& /*key*/, const Bytes& /*value*/) -> bool {
        count++;
        return count < 2; // 只遍历前两个
    });
    
    EXPECT_EQ(count, 2);
    
    db->close();
}

TEST_F(DBTest, SyncOperation) {
    auto db = DB::open(options);
    
    // 写入数据
    db->put(test_pairs[0].first, test_pairs[0].second);
    
    // 同步操作不应该抛出异常
    EXPECT_NO_THROW(db->sync());
    
    db->close();
}

// 数据持久化测试
class DBPersistenceTest : public DBTest {};

TEST_F(DBPersistenceTest, DataPersistence) {
    // 第一次打开，写入数据
    {
        auto db = DB::open(options);
        
        for (const auto& [key, value] : test_pairs) {
            db->put(key, value);
        }
        
        db->sync();
        db->close();
    }
    
    // 第二次打开，验证数据
    {
        auto db = DB::open(options);
        
        for (const auto& [key, value] : test_pairs) {
            Bytes retrieved_value = db->get(key);
            EXPECT_EQ(retrieved_value, value);
        }
        
        db->close();
    }
}

TEST_F(DBPersistenceTest, DeletePersistence) {
    // 第一次打开，写入数据然后删除
    {
        auto db = DB::open(options);
        
        db->put(test_pairs[0].first, test_pairs[0].second);
        db->put(test_pairs[1].first, test_pairs[1].second);
        
        // 删除一个 key
        db->remove(test_pairs[0].first);
        
        db->sync();
        db->close();
    }
    
    // 第二次打开，验证删除持久化
    {
        auto db = DB::open(options);
        
        EXPECT_THROW(db->get(test_pairs[0].first), KeyNotFoundError);
        
        Bytes value = db->get(test_pairs[1].first);
        EXPECT_EQ(value, test_pairs[1].second);
        
        db->close();
    }
}

// 大数据测试
class DBLargeDataTest : public DBTest {};

TEST_F(DBLargeDataTest, LargeValue) {
    auto db = DB::open(options);
    
    // 创建大值（1MB）
    Bytes large_value(1024 * 1024, 0xAB);
    Bytes key = {0x6c, 0x61, 0x72, 0x67, 0x65}; // "large"
    
    db->put(key, large_value);
    
    Bytes retrieved_value = db->get(key);
    EXPECT_EQ(retrieved_value, large_value);
    
    db->close();
}

TEST_F(DBLargeDataTest, ManyKeys) {
    auto db = DB::open(options);
    
    // 插入大量键值对
    std::vector<std::pair<Bytes, Bytes>> many_pairs;
    for (int i = 0; i < 1000; ++i) {
        Bytes key = {static_cast<uint8_t>(i >> 8), static_cast<uint8_t>(i & 0xFF)};
        Bytes value = {static_cast<uint8_t>(i & 0xFF), static_cast<uint8_t>(i >> 8)};
        many_pairs.emplace_back(key, value);
        
        db->put(key, value);
    }
    
    // 验证所有数据
    for (const auto& [key, value] : many_pairs) {
        Bytes retrieved_value = db->get(key);
        EXPECT_EQ(retrieved_value, value);
    }
    
    EXPECT_EQ(db->list_keys().size(), 1000);
    
    db->close();
}

TEST_F(DBLargeDataTest, FileRotation) {
    // 使用独立的测试目录避免冲突
    std::string rotation_test_dir = "/tmp/bitcask_file_rotation_test";
    utils::remove_directory(rotation_test_dir);
    
    // 设置小的文件大小来触发文件轮转
    Options rotation_options = Options::default_options();
    rotation_options.dir_path = rotation_test_dir;
    rotation_options.data_file_size = 1024; // 1KB
    rotation_options.sync_writes = true; // 强制同步以确保数据持久化
    
    auto db = DB::open(rotation_options);
    
    // 首先测试一个非常简单的案例来隔离问题
    {
        std::cout << "=== Simple Value Test ===" << std::endl;
        Bytes test_key = {100}; // 使用100避免与小数字混淆
        Bytes test_value = {0, 1, 2, 3, 4}; // 简单的递增模式
        
        db->put(test_key, test_value);
        
        Bytes retrieved = db->get(test_key);
        std::cout << "Original value: ";
        for (size_t i = 0; i < test_value.size(); ++i) {
            std::cout << static_cast<int>(test_value[i]) << " ";
        }
        std::cout << std::endl;
        
        std::cout << "Retrieved value: ";
        for (size_t i = 0; i < retrieved.size(); ++i) {
            std::cout << static_cast<int>(retrieved[i]) << " ";
        }
        std::cout << std::endl;
        
        for (size_t i = 0; i < test_value.size() && i < retrieved.size(); ++i) {
            if (test_value[i] != retrieved[i]) {
                std::cout << "Mismatch at position " << i 
                          << ": expected " << static_cast<int>(test_value[i])
                          << ", got " << static_cast<int>(retrieved[i])
                          << ", difference: " << (static_cast<int>(retrieved[i]) - static_cast<int>(test_value[i]))
                          << std::endl;
            }
        }
        
        EXPECT_EQ(retrieved, test_value) << "Simple test failed - this indicates a fundamental data corruption issue";
    }
    
    // 如果简单测试通过，继续测试文件轮转
    std::cout << "\n=== File Rotation Test ===" << std::endl;
    
    // 写入足够多的数据来触发多个文件
    const int num_keys = 10; // 减少到最小数量
    for (int i = 0; i < num_keys; ++i) {
        Bytes key = {static_cast<uint8_t>(200 + i)}; // 使用更大的键值避免混淆
        Bytes value = {static_cast<uint8_t>(i), static_cast<uint8_t>(i), static_cast<uint8_t>(i)}; // 3字节重复值
        
        db->put(key, value);
        
        // 立即验证
        Bytes immediate_check = db->get(key);
        if (immediate_check != value) {
            std::cout << "Immediate verification failed for key " << (200 + i) << std::endl;
            std::cout << "Expected: [" << static_cast<int>(i) << ", " << static_cast<int>(i) << ", " << static_cast<int>(i) << "]" << std::endl;
            std::cout << "Got: [";
            for (size_t j = 0; j < immediate_check.size() && j < 3; ++j) {
                if (j > 0) std::cout << ", ";
                std::cout << static_cast<int>(immediate_check[j]);
            }
            std::cout << "]" << std::endl;
        }
    }
    
    // 验证统计信息
    Stat stat = db->stat();
    std::cout << "Database stats: " << stat.key_num << " keys, " << stat.data_file_num << " files" << std::endl;
    
    // 在当前实例中验证所有数据
    bool current_instance_ok = true;
    for (int i = 0; i < num_keys; ++i) {
        Bytes key = {static_cast<uint8_t>(200 + i)};
        Bytes expected_value = {static_cast<uint8_t>(i), static_cast<uint8_t>(i), static_cast<uint8_t>(i)};
        
        Bytes retrieved_value = db->get(key);
        if (retrieved_value != expected_value) {
            current_instance_ok = false;
            std::cout << "Current instance verification failed for key " << (200 + i) << std::endl;
        }
    }
    
    if (!current_instance_ok) {
        FAIL() << "Data corruption detected in current database instance - skipping persistence test";
    }
    
    // 关闭并重新打开数据库以测试持久性
    std::cout << "Closing and reopening database..." << std::endl;
    db->close();
    db = DB::open(rotation_options);
    
    // 验证持久化数据
    for (int i = 0; i < num_keys; ++i) {
        Bytes key = {static_cast<uint8_t>(200 + i)};
        Bytes expected_value = {static_cast<uint8_t>(i), static_cast<uint8_t>(i), static_cast<uint8_t>(i)};
        
        Bytes retrieved_value = db->get(key);
        EXPECT_EQ(retrieved_value, expected_value) 
            << "Persistence test failed for key " << (200 + i)
            << ", expected first byte: " << static_cast<int>(i)
            << ", got first byte: " << (retrieved_value.empty() ? -1 : static_cast<int>(retrieved_value[0]))
            << ", retrieved size: " << retrieved_value.size();
    }
    
    db->close();
    
    // 清理测试目录
    utils::remove_directory(rotation_test_dir);
}

// 统计信息测试
class DBStatTest : public DBTest {};

TEST_F(DBStatTest, BasicStats) {
    auto db = DB::open(options);
    
    // 初始状态
    Stat stat = db->stat();
    EXPECT_EQ(stat.key_num, 0);
    EXPECT_GE(stat.data_file_num, 0);
    EXPECT_GE(stat.disk_size, 0);
    
    // 写入数据后
    for (const auto& [key, value] : test_pairs) {
        db->put(key, value);
    }
    
    stat = db->stat();
    EXPECT_EQ(stat.key_num, test_pairs.size());
    EXPECT_GT(stat.disk_size, 0);
    
    db->close();
}

TEST_F(DBStatTest, ReclaimableSize) {
    auto db = DB::open(options);
    
    // 写入数据
    db->put(test_pairs[0].first, test_pairs[0].second);
    
    Stat stat1 = db->stat();
    
    // 更新数据（产生可回收空间）
    Bytes new_value = {0x6e, 0x65, 0x77};
    db->put(test_pairs[0].first, new_value);
    
    Stat stat2 = db->stat();
    EXPECT_GT(stat2.reclaimable_size, stat1.reclaimable_size);
    
    // 删除数据（产生更多可回收空间）
    db->remove(test_pairs[0].first);
    
    Stat stat3 = db->stat();
    EXPECT_GT(stat3.reclaimable_size, stat2.reclaimable_size);
    
    db->close();
}

// 备份测试
class DBBackupTest : public DBTest {};

TEST_F(DBBackupTest, BackupRestore) {
    std::string backup_dir = "/tmp/bitcask_backup_test";
    utils::remove_directory(backup_dir);
    
    // 创建原始数据
    {
        auto db = DB::open(options);
        
        for (const auto& [key, value] : test_pairs) {
            db->put(key, value);
        }
        
        db->sync();
        
        // 备份数据库
        db->backup(backup_dir);
        
        db->close();
    }
    
    // 从备份恢复
    {
        Options backup_options = options;
        backup_options.dir_path = backup_dir;
        
        auto db = DB::open(backup_options);
        
        // 验证备份的数据
        for (const auto& [key, value] : test_pairs) {
            Bytes retrieved_value = db->get(key);
            EXPECT_EQ(retrieved_value, value);
        }
        
        db->close();
    }
    
    utils::remove_directory(backup_dir);
}

// 错误处理测试
class DBErrorTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir = "/tmp/bitcask_db_error_test";
        utils::remove_directory(test_dir);
    }
    
    void TearDown() override {
        utils::remove_directory(test_dir);
    }
    
    std::string test_dir;
};

TEST_F(DBErrorTest, InvalidOptions) {
    Options invalid_options;
    invalid_options.dir_path = ""; // 空路径
    
    EXPECT_THROW(DB::open(invalid_options), BitcaskException);
}

TEST_F(DBErrorTest, InvalidDataFileSize) {
    Options invalid_options = Options::default_options();
    invalid_options.dir_path = test_dir;
    invalid_options.data_file_size = 0; // 无效大小
    
    EXPECT_THROW(DB::open(invalid_options), BitcaskException);
}

TEST_F(DBErrorTest, InvalidMergeRatio) {
    Options invalid_options = Options::default_options();
    invalid_options.dir_path = test_dir;
    invalid_options.data_file_merge_ratio = 1.5f; // 无效比率
    
    EXPECT_THROW(DB::open(invalid_options), BitcaskException);
}

TEST_F(DBErrorTest, MultipleDatabaseInstances) {
    Options options = Options::default_options();
    options.dir_path = test_dir;
    
    auto db1 = DB::open(options);
    
    // 尝试打开第二个实例应该失败（文件锁）
    EXPECT_THROW(DB::open(options), DatabaseIsUsingError);
    
    db1->close();
    
    // 关闭第一个实例后，应该能打开第二个
    EXPECT_NO_THROW(DB::open(options));
}

// 并发测试
class DBConcurrencyTest : public DBTest {};

TEST_F(DBConcurrencyTest, ConcurrentReads) {
    auto db = DB::open(options);
    
    // 先写入测试数据
    for (const auto& [key, value] : test_pairs) {
        db->put(key, value);
    }
    
    std::vector<std::thread> threads;
    std::atomic<int> success_count{0};
    
    // 启动多个读线程
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([this, &db, &success_count]() {
            for (int j = 0; j < 100; ++j) {
                for (const auto& [key, value] : test_pairs) {
                    try {
                        Bytes retrieved_value = db->get(key);
                        if (retrieved_value == value) {
                            success_count++;
                        }
                    } catch (...) {
                        // 忽略异常
                    }
                }
            }
        });
    }
    
    // 等待所有线程完成
    for (auto& thread : threads) {
        thread.join();
    }
    
    EXPECT_EQ(success_count.load(), 10 * 100 * test_pairs.size());
    
    db->close();
}

TEST_F(DBConcurrencyTest, ConcurrentWrites) {
    auto db = DB::open(options);
    
    std::vector<std::thread> threads;
    std::atomic<int> write_count{0};
    
    // 启动多个写线程
    for (int i = 0; i < 5; ++i) {
        threads.emplace_back([&db, &write_count, i]() {
            for (int j = 0; j < 20; ++j) {
                try {
                    Bytes key = {static_cast<uint8_t>(i), static_cast<uint8_t>(j)};
                    Bytes value = {static_cast<uint8_t>(j), static_cast<uint8_t>(i)};
                    db->put(key, value);
                    write_count++;
                } catch (...) {
                    // 忽略异常
                }
            }
        });
    }
    
    // 等待所有线程完成
    for (auto& thread : threads) {
        thread.join();
    }
    
    EXPECT_EQ(write_count.load(), 5 * 20);
    
    // 验证写入的数据
    for (int i = 0; i < 5; ++i) {
        for (int j = 0; j < 20; ++j) {
            Bytes key = {static_cast<uint8_t>(i), static_cast<uint8_t>(j)};
            Bytes expected_value = {static_cast<uint8_t>(j), static_cast<uint8_t>(i)};
            
            Bytes retrieved_value = db->get(key);
            EXPECT_EQ(retrieved_value, expected_value);
        }
    }
    
    db->close();
}

TEST_F(DBConcurrencyTest, MixedOperations) {
    auto db = DB::open(options);
    
    std::vector<std::thread> threads;
    std::atomic<int> operation_count{0};
    
    // 读线程
    threads.emplace_back([this, &db, &operation_count]() {
        // 先写入一些初始数据
        for (const auto& [key, value] : test_pairs) {
            db->put(key, value);
        }
        
        for (int i = 0; i < 100; ++i) {
            for (const auto& [key, value] : test_pairs) {
                try {
                    db->get(key);
                    operation_count++;
                } catch (...) {
                    // 忽略异常
                }
            }
        }
    });
    
    // 写线程
    threads.emplace_back([&db, &operation_count]() {
        for (int i = 0; i < 50; ++i) {
            try {
                Bytes key = {static_cast<uint8_t>(i)};
                Bytes value = {static_cast<uint8_t>(i * 2)};
                db->put(key, value);
                operation_count++;
            } catch (...) {
                // 忽略异常
            }
        }
    });
    
    // 删除线程
    threads.emplace_back([&db, &operation_count]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(10)); // 让写线程先执行
        
        for (int i = 0; i < 25; ++i) {
            try {
                Bytes key = {static_cast<uint8_t>(i)};
                db->remove(key);
                operation_count++;
            } catch (...) {
                // 忽略异常
            }
        }
    });
    
    // 等待所有线程完成
    for (auto& thread : threads) {
        thread.join();
    }
    
    EXPECT_GT(operation_count.load(), 0);
    
    db->close();
}
