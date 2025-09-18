#include <gtest/gtest.h>
#include "bitcask/bitcask.h"
#include "bitcask/utils.h"
#include <thread>
#include <random>
#include <chrono>

using namespace bitcask;

class IntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir = "/tmp/bitcask_integration_test";
        utils::remove_directory(test_dir);
    }
    
    void TearDown() override {
        utils::remove_directory(test_dir);
    }
    
    std::string test_dir;
};

// 完整的数据库生命周期测试
TEST_F(IntegrationTest, CompleteDatabaseLifecycle) {
    Options options = Options::default_options();
    options.dir_path = test_dir;
    options.data_file_size = 1024 * 1024; // 1MB
    options.sync_writes = false;
    
    // 第一阶段：创建和填充数据库
    {
        auto db = bitcask::open(options);
        
        // 写入大量数据
        for (int i = 0; i < 1000; ++i) {
            std::string key = "key_" + std::to_string(i);
            std::string value = "value_" + std::to_string(i) + "_" + std::string(100, 'x');
            
            db->put(string_to_bytes(key), string_to_bytes(value));
        }
        
        // 更新一些数据
        for (int i = 0; i < 100; ++i) {
            std::string key = "key_" + std::to_string(i);
            std::string value = "updated_value_" + std::to_string(i);
            
            db->put(string_to_bytes(key), string_to_bytes(value));
        }
        
        // 删除一些数据
        for (int i = 500; i < 600; ++i) {
            std::string key = "key_" + std::to_string(i);
            db->remove(string_to_bytes(key));
        }
        
        db->sync();
        db->close();
    }
    
    // 第二阶段：重新打开并验证数据
    {
        auto db = bitcask::open(options);
        
        // 验证更新的数据
        for (int i = 0; i < 100; ++i) {
            std::string key = "key_" + std::to_string(i);
            std::string expected_value = "updated_value_" + std::to_string(i);
            
            Bytes value = db->get(string_to_bytes(key));
            EXPECT_EQ(bytes_to_string(value), expected_value);
        }
        
        // 验证未更新的数据
        for (int i = 100; i < 500; ++i) {
            std::string key = "key_" + std::to_string(i);
            std::string expected_value = "value_" + std::to_string(i) + "_" + std::string(100, 'x');
            
            Bytes value = db->get(string_to_bytes(key));
            EXPECT_EQ(bytes_to_string(value), expected_value);
        }
        
        // 验证删除的数据
        for (int i = 500; i < 600; ++i) {
            std::string key = "key_" + std::to_string(i);
            EXPECT_THROW(db->get(string_to_bytes(key)), KeyNotFoundError);
        }
        
        // 验证剩余数据
        for (int i = 600; i < 1000; ++i) {
            std::string key = "key_" + std::to_string(i);
            std::string expected_value = "value_" + std::to_string(i) + "_" + std::string(100, 'x');
            
            Bytes value = db->get(string_to_bytes(key));
            EXPECT_EQ(bytes_to_string(value), expected_value);
        }
        
        db->close();
    }
}

// 批量操作集成测试
TEST_F(IntegrationTest, BatchOperationsIntegration) {
    Options options = Options::default_options();
    options.dir_path = test_dir;
    
    auto db = bitcask::open(options);
    
    // 使用批量操作进行复杂的数据操作
    WriteBatchOptions batch_options = WriteBatchOptions::default_options();
    batch_options.sync_writes = true;
    
    // 第一批：插入初始数据
    {
        auto batch = db->new_write_batch(batch_options);
        
        for (int i = 0; i < 100; ++i) {
            std::string key = "batch_key_" + std::to_string(i);
            std::string value = "batch_value_" + std::to_string(i);
            batch->put(string_to_bytes(key), string_to_bytes(value));
        }
        
        batch->commit();
    }
    
    // 第二批：混合操作
    {
        auto batch = db->new_write_batch(batch_options);
        
        // 更新部分数据
        for (int i = 0; i < 50; ++i) {
            std::string key = "batch_key_" + std::to_string(i);
            std::string value = "updated_batch_value_" + std::to_string(i);
            batch->put(string_to_bytes(key), string_to_bytes(value));
        }
        
        // 删除部分数据
        for (int i = 50; i < 70; ++i) {
            std::string key = "batch_key_" + std::to_string(i);
            batch->remove(string_to_bytes(key));
        }
        
        // 添加新数据
        for (int i = 100; i < 150; ++i) {
            std::string key = "batch_key_" + std::to_string(i);
            std::string value = "new_batch_value_" + std::to_string(i);
            batch->put(string_to_bytes(key), string_to_bytes(value));
        }
        
        batch->commit();
    }
    
    // 验证批量操作结果
    // 验证更新的数据
    for (int i = 0; i < 50; ++i) {
        std::string key = "batch_key_" + std::to_string(i);
        std::string expected_value = "updated_batch_value_" + std::to_string(i);
        
        Bytes value = db->get(string_to_bytes(key));
        EXPECT_EQ(bytes_to_string(value), expected_value);
    }
    
    // 验证删除的数据
    for (int i = 50; i < 70; ++i) {
        std::string key = "batch_key_" + std::to_string(i);
        EXPECT_THROW(db->get(string_to_bytes(key)), KeyNotFoundError);
    }
    
    // 验证未变更的数据
    for (int i = 70; i < 100; ++i) {
        std::string key = "batch_key_" + std::to_string(i);
        std::string expected_value = "batch_value_" + std::to_string(i);
        
        Bytes value = db->get(string_to_bytes(key));
        EXPECT_EQ(bytes_to_string(value), expected_value);
    }
    
    // 验证新添加的数据
    for (int i = 100; i < 150; ++i) {
        std::string key = "batch_key_" + std::to_string(i);
        std::string expected_value = "new_batch_value_" + std::to_string(i);
        
        Bytes value = db->get(string_to_bytes(key));
        EXPECT_EQ(bytes_to_string(value), expected_value);
    }
    
    db->close();
}

// 迭代器集成测试
TEST_F(IntegrationTest, IteratorIntegration) {
    Options options = Options::default_options();
    options.dir_path = test_dir;
    
    auto db = bitcask::open(options);
    
    // 创建测试数据
    std::map<std::string, std::string> test_data;
    
    // 插入不同前缀的数据
    for (int i = 0; i < 50; ++i) {
        std::string key = "user_" + std::to_string(i);
        std::string value = "user_data_" + std::to_string(i);
        test_data[key] = value;
        db->put(string_to_bytes(key), string_to_bytes(value));
    }
    
    for (int i = 0; i < 30; ++i) {
        std::string key = "product_" + std::to_string(i);
        std::string value = "product_data_" + std::to_string(i);
        test_data[key] = value;
        db->put(string_to_bytes(key), string_to_bytes(value));
    }
    
    for (int i = 0; i < 20; ++i) {
        std::string key = "order_" + std::to_string(i);
        std::string value = "order_data_" + std::to_string(i);
        test_data[key] = value;
        db->put(string_to_bytes(key), string_to_bytes(value));
    }
    
    // 测试全量迭代
    {
        IteratorOptions iter_options;
        auto iter = db->iterator(iter_options);
        
        std::map<std::string, std::string> iterated_data;
        for (iter->rewind(); iter->valid(); iter->next()) {
            std::string key = bytes_to_string(iter->key());
            std::string value = bytes_to_string(iter->value());
            iterated_data[key] = value;
        }
        
        EXPECT_EQ(iterated_data, test_data);
    }
    
    // 测试前缀迭代
    {
        IteratorOptions iter_options;
        iter_options.prefix = string_to_bytes("user_");
        auto iter = db->iterator(iter_options);
        
        int user_count = 0;
        for (iter->rewind(); iter->valid(); iter->next()) {
            std::string key = bytes_to_string(iter->key());
            EXPECT_TRUE(key.substr(0, 5) == "user_");
            user_count++;
        }
        
        EXPECT_EQ(user_count, 50);
    }
    
    // 测试反向迭代
    {
        IteratorOptions iter_options;
        iter_options.reverse = true;
        auto iter = db->iterator(iter_options);
        
        std::vector<std::string> reverse_keys;
        for (iter->rewind(); iter->valid(); iter->next()) {
            reverse_keys.push_back(bytes_to_string(iter->key()));
        }
        
        // 验证是反向排序的
        std::vector<std::string> expected_keys;
        for (const auto& [key, value] : test_data) {
            expected_keys.push_back(key);
        }
        std::sort(expected_keys.rbegin(), expected_keys.rend());
        
        EXPECT_EQ(reverse_keys, expected_keys);
    }
    
    db->close();
}

// 多文件管理集成测试
TEST_F(IntegrationTest, MultiFileManagement) {
    Options options = Options::default_options();
    options.dir_path = test_dir;
    options.data_file_size = 4096; // 4KB，强制文件轮转
    options.sync_writes = true;
    
    auto db = bitcask::open(options);
    
    // 写入足够多的数据来触发多个文件
    std::map<std::string, std::string> all_data;
    
    for (int i = 0; i < 500; ++i) {
        std::string key = "multi_file_key_" + std::to_string(i);
        std::string value = std::string(100, 'A' + (i % 26)); // 100字节的值
        
        all_data[key] = value;
        db->put(string_to_bytes(key), string_to_bytes(value));
    }
    
    // 检查统计信息
    Stat stat = db->stat();
    EXPECT_GT(stat.data_file_num, 1); // 应该有多个文件
    EXPECT_EQ(stat.key_num, 500);
    EXPECT_GT(stat.disk_size, 0);
    
    // 更新一些数据（产生可回收空间）
    for (int i = 0; i < 100; ++i) {
        std::string key = "multi_file_key_" + std::to_string(i);
        std::string value = "updated_" + std::string(50, 'Z');
        
        all_data[key] = value;
        db->put(string_to_bytes(key), string_to_bytes(value));
    }
    
    stat = db->stat();
    EXPECT_GT(stat.reclaimable_size, 0); // 应该有可回收空间
    
    // 验证所有数据仍然可读
    for (const auto& [key, expected_value] : all_data) {
        Bytes value = db->get(string_to_bytes(key));
        EXPECT_EQ(bytes_to_string(value), expected_value);
    }
    
    db->close();
    
    // 重新打开数据库，验证多文件持久化
    {
        auto reopened_db = bitcask::open(options);
        
        for (const auto& [key, expected_value] : all_data) {
            Bytes value = reopened_db->get(string_to_bytes(key));
            EXPECT_EQ(bytes_to_string(value), expected_value);
        }
        
        reopened_db->close();
    }
}

// 错误恢复集成测试
TEST_F(IntegrationTest, ErrorRecoveryIntegration) {
    Options options = Options::default_options();
    options.dir_path = test_dir;
    
    // 第一次打开，写入数据
    {
        auto db = bitcask::open(options);
        
        for (int i = 0; i < 100; ++i) {
            std::string key = "recovery_key_" + std::to_string(i);
            std::string value = "recovery_value_" + std::to_string(i);
            db->put(string_to_bytes(key), string_to_bytes(value));
        }
        
        db->sync();
        db->close();
    }
    
    // 模拟部分数据损坏（在实际应用中这种情况很少见）
    // 这里我们只是验证数据库能够正确处理现有数据
    
    // 重新打开，验证数据恢复
    {
        auto db = bitcask::open(options);
        
        // 验证所有数据都能正确读取
        for (int i = 0; i < 100; ++i) {
            std::string key = "recovery_key_" + std::to_string(i);
            std::string expected_value = "recovery_value_" + std::to_string(i);
            
            Bytes value = db->get(string_to_bytes(key));
            EXPECT_EQ(bytes_to_string(value), expected_value);
        }
        
        // 继续写入新数据
        for (int i = 100; i < 200; ++i) {
            std::string key = "recovery_key_" + std::to_string(i);
            std::string value = "recovery_value_" + std::to_string(i);
            db->put(string_to_bytes(key), string_to_bytes(value));
        }
        
        // 验证新数据
        for (int i = 100; i < 200; ++i) {
            std::string key = "recovery_key_" + std::to_string(i);
            std::string expected_value = "recovery_value_" + std::to_string(i);
            
            Bytes value = db->get(string_to_bytes(key));
            EXPECT_EQ(bytes_to_string(value), expected_value);
        }
        
        db->close();
    }
}

// 并发操作集成测试
TEST_F(IntegrationTest, ConcurrentOperationsIntegration) {
    Options options = Options::default_options();
    options.dir_path = test_dir;
    
    auto db = bitcask::open(options);
    
    std::vector<std::thread> threads;
    std::atomic<int> operation_count{0};
    std::mutex result_mutex;
    std::map<std::string, std::string> final_data;
    
    // 写线程
    for (int t = 0; t < 3; ++t) {
        threads.emplace_back([&db, &operation_count, &result_mutex, &final_data, t]() {
            for (int i = 0; i < 100; ++i) {
                std::string key = "concurrent_key_" + std::to_string(t) + "_" + std::to_string(i);
                std::string value = "concurrent_value_" + std::to_string(t) + "_" + std::to_string(i);
                
                db->put(string_to_bytes(key), string_to_bytes(value));
                
                {
                    std::lock_guard<std::mutex> lock(result_mutex);
                    final_data[key] = value;
                }
                
                operation_count++;
            }
        });
    }
    
    // 读线程
    threads.emplace_back([&db, &operation_count]() {
        for (int i = 0; i < 500; ++i) {
            try {
                // 尝试读取可能存在的数据
                std::string key = "concurrent_key_0_" + std::to_string(i % 100);
                db->get(string_to_bytes(key));
                operation_count++;
            } catch (const KeyNotFoundError&) {
                // 忽略不存在的键
            }
            
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
    });
    
    // 批量操作线程
    threads.emplace_back([&db, &operation_count, &result_mutex, &final_data]() {
        WriteBatchOptions batch_options = WriteBatchOptions::default_options();
        auto batch = db->new_write_batch(batch_options);
        
        for (int i = 0; i < 50; ++i) {
            std::string key = "batch_concurrent_key_" + std::to_string(i);
            std::string value = "batch_concurrent_value_" + std::to_string(i);
            
            batch->put(string_to_bytes(key), string_to_bytes(value));
            
            {
                std::lock_guard<std::mutex> lock(result_mutex);
                final_data[key] = value;
            }
        }
        
        batch->commit();
        operation_count += 50;
    });
    
    // 等待所有线程完成
    for (auto& thread : threads) {
        thread.join();
    }
    
    std::cout << "Total operations: " << operation_count.load() << std::endl;
    
    // 验证所有写入的数据
    for (const auto& [key, expected_value] : final_data) {
        Bytes value = db->get(string_to_bytes(key));
        EXPECT_EQ(bytes_to_string(value), expected_value);
    }
    
    db->close();
}

// 备份和恢复集成测试
TEST_F(IntegrationTest, BackupRestoreIntegration) {
    Options options = Options::default_options();
    options.dir_path = test_dir;
    
    std::string backup_dir = "/tmp/bitcask_backup_integration";
    utils::remove_directory(backup_dir);
    
    std::map<std::string, std::string> original_data;
    
    // 创建原始数据库
    {
        auto db = bitcask::open(options);
        
        // 写入数据
        for (int i = 0; i < 200; ++i) {
            std::string key = "backup_key_" + std::to_string(i);
            std::string value = "backup_value_" + std::to_string(i) + "_" + std::string(50, 'B');
            
            original_data[key] = value;
            db->put(string_to_bytes(key), string_to_bytes(value));
        }
        
        // 更新部分数据
        for (int i = 0; i < 50; ++i) {
            std::string key = "backup_key_" + std::to_string(i);
            std::string value = "updated_backup_value_" + std::to_string(i);
            
            original_data[key] = value;
            db->put(string_to_bytes(key), string_to_bytes(value));
        }
        
        // 删除部分数据
        for (int i = 150; i < 200; ++i) {
            std::string key = "backup_key_" + std::to_string(i);
            original_data.erase(key);
            db->remove(string_to_bytes(key));
        }
        
        db->sync();
        
        // 执行备份
        db->backup(backup_dir);
        
        db->close();
    }
    
    // 从备份恢复
    {
        Options backup_options = options;
        backup_options.dir_path = backup_dir;
        
        auto backup_db = bitcask::open(backup_options);
        
        // 验证备份的数据
        for (const auto& [key, expected_value] : original_data) {
            Bytes value = backup_db->get(string_to_bytes(key));
            EXPECT_EQ(bytes_to_string(value), expected_value);
        }
        
        // 验证删除的数据确实不存在
        for (int i = 150; i < 200; ++i) {
            std::string key = "backup_key_" + std::to_string(i);
            EXPECT_THROW(backup_db->get(string_to_bytes(key)), KeyNotFoundError);
        }
        
        backup_db->close();
    }
    
    utils::remove_directory(backup_dir);
}

// 性能集成测试
TEST_F(IntegrationTest, PerformanceIntegration) {
    Options options = Options::default_options();
    options.dir_path = test_dir;
    options.sync_writes = false; // 关闭同步以提高性能
    
    auto db = bitcask::open(options);
    
    const int NUM_OPERATIONS = 10000;
    std::vector<std::string> keys;
    
    // 准备测试数据
    for (int i = 0; i < NUM_OPERATIONS; ++i) {
        keys.push_back("perf_key_" + std::to_string(i));
    }
    
    // 写入性能测试
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < NUM_OPERATIONS; ++i) {
        std::string value = "perf_value_" + std::to_string(i) + "_" + std::string(100, 'P');
        db->put(string_to_bytes(keys[i]), string_to_bytes(value));
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto write_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    double write_qps = (double)NUM_OPERATIONS * 1000000 / write_duration.count();
    std::cout << "Write QPS: " << write_qps << std::endl;
    
    // 读取性能测试
    start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < NUM_OPERATIONS; ++i) {
        db->get(string_to_bytes(keys[i]));
    }
    
    end = std::chrono::high_resolution_clock::now();
    auto read_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    double read_qps = (double)NUM_OPERATIONS * 1000000 / read_duration.count();
    std::cout << "Read QPS: " << read_qps << std::endl;
    
    // 随机读取性能测试
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, NUM_OPERATIONS - 1);
    
    start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < NUM_OPERATIONS / 10; ++i) {
        int index = dis(gen);
        db->get(string_to_bytes(keys[index]));
    }
    
    end = std::chrono::high_resolution_clock::now();
    auto random_read_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    double random_read_qps = (double)(NUM_OPERATIONS / 10) * 1000000 / random_read_duration.count();
    std::cout << "Random Read QPS: " << random_read_qps << std::endl;
    
    // 验证性能指标合理性
    EXPECT_GT(write_qps, 1000);  // 至少1000 QPS
    EXPECT_GT(read_qps, 5000);   // 读取应该更快
    EXPECT_GT(random_read_qps, 1000);
    
    db->close();
}
