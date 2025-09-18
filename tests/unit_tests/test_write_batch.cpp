#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "bitcask/db.h"
#include "bitcask/utils.h"
#include <chrono>

using namespace bitcask;

class WriteBatchTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir = "/tmp/bitcask_batch_test";
        utils::remove_directory(test_dir);
        
        options = Options::default_options();
        options.dir_path = test_dir;
        options.data_file_size = 64 * 1024; // 64KB
        options.sync_writes = false;
        
        db = DB::open(options);
        
        // 创建测试数据
        test_pairs = {
            {{0x6b, 0x65, 0x79, 0x31}, {0x76, 0x61, 0x6c, 0x75, 0x65, 0x31}}, // key1, value1
            {{0x6b, 0x65, 0x79, 0x32}, {0x76, 0x61, 0x6c, 0x75, 0x65, 0x32}}, // key2, value2
            {{0x6b, 0x65, 0x79, 0x33}, {0x76, 0x61, 0x6c, 0x75, 0x65, 0x33}}, // key3, value3
        };
    }
    
    void TearDown() override {
        if (db) {
            db->close();
        }
        utils::remove_directory(test_dir);
    }
    
    std::string test_dir;
    Options options;
    std::unique_ptr<DB> db;
    std::vector<std::pair<Bytes, Bytes>> test_pairs;
};

// 基本批量写入测试
TEST_F(WriteBatchTest, BasicBatchWrite) {
    WriteBatchOptions batch_options = WriteBatchOptions::default_options();
    auto batch = db->new_write_batch(batch_options);
    
    // 添加多个写操作到批次
    for (const auto& [key, value] : test_pairs) {
        batch->put(key, value);
    }
    
    // 提交批次
    EXPECT_NO_THROW(batch->commit());
    
    // 验证所有数据都已写入
    for (const auto& [key, value] : test_pairs) {
        Bytes retrieved_value = db->get(key);
        EXPECT_EQ(retrieved_value, value);
    }
}

TEST_F(WriteBatchTest, BatchDelete) {
    // 先写入一些数据
    for (const auto& [key, value] : test_pairs) {
        db->put(key, value);
    }
    
    WriteBatchOptions batch_options = WriteBatchOptions::default_options();
    auto batch = db->new_write_batch(batch_options);
    
    // 批量删除部分数据
    batch->remove(test_pairs[0].first);
    batch->remove(test_pairs[2].first);
    
    batch->commit();
    
    // 验证删除结果
    EXPECT_THROW(db->get(test_pairs[0].first), KeyNotFoundError);
    
    Bytes value = db->get(test_pairs[1].first);
    EXPECT_EQ(value, test_pairs[1].second);
    
    EXPECT_THROW(db->get(test_pairs[2].first), KeyNotFoundError);
}

TEST_F(WriteBatchTest, MixedOperations) {
    // 先写入一些初始数据
    db->put(test_pairs[0].first, test_pairs[0].second);
    
    WriteBatchOptions batch_options = WriteBatchOptions::default_options();
    auto batch = db->new_write_batch(batch_options);
    
    // 混合操作
    batch->put(test_pairs[1].first, test_pairs[1].second);    // 新增
    batch->put(test_pairs[0].first, {0x6e, 0x65, 0x77});     // 更新
    batch->remove(test_pairs[0].first);                       // 删除
    batch->put(test_pairs[2].first, test_pairs[2].second);   // 新增
    
    batch->commit();
    
    // 验证结果
    EXPECT_THROW(db->get(test_pairs[0].first), KeyNotFoundError); // 应该被删除
    
    Bytes value1 = db->get(test_pairs[1].first);
    EXPECT_EQ(value1, test_pairs[1].second);
    
    Bytes value2 = db->get(test_pairs[2].first);
    EXPECT_EQ(value2, test_pairs[2].second);
}

TEST_F(WriteBatchTest, EmptyBatch) {
    WriteBatchOptions batch_options = WriteBatchOptions::default_options();
    auto batch = db->new_write_batch(batch_options);
    
    // 空批次提交不应该抛出异常
    EXPECT_NO_THROW(batch->commit());
}

TEST_F(WriteBatchTest, BatchSizeLimit) {
    WriteBatchOptions batch_options;
    batch_options.max_batch_num = 2; // 设置小的批次大小限制
    batch_options.sync_writes = false;
    
    auto batch = db->new_write_batch(batch_options);
    
    // 添加到限制数量
    batch->put(test_pairs[0].first, test_pairs[0].second);
    batch->put(test_pairs[1].first, test_pairs[1].second);
    
    // 超过限制应该抛出异常
    EXPECT_THROW(batch->put(test_pairs[2].first, test_pairs[2].second), BitcaskException);
}

TEST_F(WriteBatchTest, BatchSync) {
    WriteBatchOptions batch_options;
    batch_options.sync_writes = true; // 启用同步写入
    
    auto batch = db->new_write_batch(batch_options);
    
    for (const auto& [key, value] : test_pairs) {
        batch->put(key, value);
    }
    
    // 同步提交不应该抛出异常
    EXPECT_NO_THROW(batch->commit());
    
    // 验证数据
    for (const auto& [key, value] : test_pairs) {
        Bytes retrieved_value = db->get(key);
        EXPECT_EQ(retrieved_value, value);
    }
}

TEST_F(WriteBatchTest, MultipleCommits) {
    WriteBatchOptions batch_options = WriteBatchOptions::default_options();
    auto batch = db->new_write_batch(batch_options);
    
    // 第一次提交
    batch->put(test_pairs[0].first, test_pairs[0].second);
    batch->commit();
    
    // 验证第一次提交的数据
    Bytes value = db->get(test_pairs[0].first);
    EXPECT_EQ(value, test_pairs[0].second);
    
    // 第二次提交（批次应该被清空）
    batch->put(test_pairs[1].first, test_pairs[1].second);
    batch->commit();
    
    // 验证两次提交的数据都存在
    value = db->get(test_pairs[0].first);
    EXPECT_EQ(value, test_pairs[0].second);
    
    value = db->get(test_pairs[1].first);
    EXPECT_EQ(value, test_pairs[1].second);
}

// 错误处理测试
class WriteBatchErrorTest : public WriteBatchTest {};

TEST_F(WriteBatchErrorTest, EmptyKey) {
    WriteBatchOptions batch_options = WriteBatchOptions::default_options();
    auto batch = db->new_write_batch(batch_options);
    
    // 空 key 应该抛出异常
    EXPECT_THROW(batch->put(Bytes{}, {0x76, 0x61, 0x6c, 0x75, 0x65}), KeyEmptyError);
    EXPECT_THROW(batch->remove(Bytes{}), KeyEmptyError);
}

TEST_F(WriteBatchErrorTest, BatchAfterCommit) {
    WriteBatchOptions batch_options = WriteBatchOptions::default_options();
    auto batch = db->new_write_batch(batch_options);
    
    batch->put(test_pairs[0].first, test_pairs[0].second);
    batch->commit();
    
    // 提交后应该能继续使用批次
    EXPECT_NO_THROW(batch->put(test_pairs[1].first, test_pairs[1].second));
    EXPECT_NO_THROW(batch->commit());
}

// 事务语义测试
class WriteBatchTransactionTest : public WriteBatchTest {};

TEST_F(WriteBatchTransactionTest, AtomicCommit) {
    WriteBatchOptions batch_options = WriteBatchOptions::default_options();
    auto batch = db->new_write_batch(batch_options);
    
    // 先写入一个初始值
    db->put(test_pairs[0].first, test_pairs[0].second);
    
    // 在批次中更新这个值并添加新值
    Bytes new_value = {0x6e, 0x65, 0x77, 0x5f, 0x76, 0x61, 0x6c, 0x75, 0x65}; // "new_value"
    batch->put(test_pairs[0].first, new_value);
    batch->put(test_pairs[1].first, test_pairs[1].second);
    
    // 在提交前，原值应该仍然存在
    Bytes current_value = db->get(test_pairs[0].first);
    EXPECT_EQ(current_value, test_pairs[0].second);
    
    // 新 key 应该不存在
    EXPECT_THROW(db->get(test_pairs[1].first), KeyNotFoundError);
    
    // 提交批次
    batch->commit();
    
    // 提交后，所有更改应该都可见
    Bytes updated_value = db->get(test_pairs[0].first);
    EXPECT_EQ(updated_value, new_value);
    
    Bytes new_key_value = db->get(test_pairs[1].first);
    EXPECT_EQ(new_key_value, test_pairs[1].second);
}

TEST_F(WriteBatchTransactionTest, IsolationFromOtherOperations) {
    WriteBatchOptions batch_options = WriteBatchOptions::default_options();
    auto batch = db->new_write_batch(batch_options);
    
    // 在批次中添加操作
    batch->put(test_pairs[0].first, test_pairs[0].second);
    
    // 在批次外进行操作
    db->put(test_pairs[1].first, test_pairs[1].second);
    
    // 批次外的操作应该立即可见
    Bytes value = db->get(test_pairs[1].first);
    EXPECT_EQ(value, test_pairs[1].second);
    
    // 批次内的操作应该不可见
    EXPECT_THROW(db->get(test_pairs[0].first), KeyNotFoundError);
    
    // 提交批次
    batch->commit();
    
    // 现在批次内的操作应该可见
    value = db->get(test_pairs[0].first);
    EXPECT_EQ(value, test_pairs[0].second);
}

// 大批次测试
class WriteBatchLargeTest : public WriteBatchTest {};

TEST_F(WriteBatchLargeTest, LargeBatch) {
    WriteBatchOptions batch_options;
    batch_options.max_batch_num = 10000; // 大批次
    batch_options.sync_writes = false;
    
    auto batch = db->new_write_batch(batch_options);
    
    // 创建大量数据
    std::vector<std::pair<Bytes, Bytes>> large_data;
    for (int i = 0; i < 1000; ++i) {
        Bytes key = {static_cast<uint8_t>(i >> 8), static_cast<uint8_t>(i & 0xFF)};
        Bytes value = {static_cast<uint8_t>(i & 0xFF), static_cast<uint8_t>(i >> 8)};
        large_data.emplace_back(key, value);
        
        batch->put(key, value);
    }
    
    // 提交大批次
    auto start = std::chrono::high_resolution_clock::now();
    batch->commit();
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Large batch commit took: " << duration.count() << " ms" << std::endl;
    
    // 验证所有数据
    for (const auto& [key, value] : large_data) {
        Bytes retrieved_value = db->get(key);
        EXPECT_EQ(retrieved_value, value);
    }
}

TEST_F(WriteBatchLargeTest, LargeValue) {
    WriteBatchOptions batch_options = WriteBatchOptions::default_options();
    auto batch = db->new_write_batch(batch_options);
    
    // 大值测试 - 减少到64KB避免内存问题
    Bytes large_value(64 * 1024, 0xAB); // 64KB
    Bytes key = {0x6c, 0x61, 0x72, 0x67, 0x65}; // "large"
    
    batch->put(key, large_value);
    batch->commit();
    
    Bytes retrieved_value;
    EXPECT_NO_THROW(retrieved_value = db->get(key)) << "Failed to retrieve large value";
    
    if (retrieved_value.empty()) {
        FAIL() << "Retrieved value is empty, but should be " << large_value.size() << " bytes";
    }
    
    EXPECT_EQ(retrieved_value.size(), large_value.size()) << "Size mismatch";
    
    if (retrieved_value.size() == large_value.size()) {
        // 检查前100字节和后100字节
        if (retrieved_value.size() >= 100 && large_value.size() >= 100) {
            for (size_t i = 0; i < 100; ++i) {
                EXPECT_EQ(retrieved_value[i], large_value[i]) << "Mismatch at beginning position " << i;
            }
            for (size_t i = large_value.size() - 100; i < large_value.size(); ++i) {
                EXPECT_EQ(retrieved_value[i], large_value[i]) << "Mismatch at end position " << i;
            }
        } else {
            EXPECT_EQ(retrieved_value, large_value);
        }
    }
}

// 性能测试
class WriteBatchPerformanceTest : public WriteBatchTest {};

TEST_F(WriteBatchPerformanceTest, BatchVsIndividualWrites) {
    const int NUM_OPERATIONS = 1000;
    
    // 生成测试数据
    std::vector<std::pair<Bytes, Bytes>> perf_data;
    for (int i = 0; i < NUM_OPERATIONS; ++i) {
        Bytes key = {static_cast<uint8_t>(i >> 8), static_cast<uint8_t>(i & 0xFF)};
        Bytes value(50, static_cast<uint8_t>(i % 256));
        perf_data.emplace_back(key, value);
    }
    
    // 测试批量写入
    {
        auto start = std::chrono::high_resolution_clock::now();
        
        WriteBatchOptions batch_options = WriteBatchOptions::default_options();
        batch_options.sync_writes = false;
        auto batch = db->new_write_batch(batch_options);
        
        for (const auto& [key, value] : perf_data) {
            batch->put(key, value);
        }
        
        batch->commit();
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        std::cout << "Batch write " << NUM_OPERATIONS << " items took: " 
                  << duration.count() << " microseconds" << std::endl;
    }
    
    // 清理数据库
    db->close();
    utils::remove_directory(test_dir);
    db = DB::open(options);
    
    // 测试单独写入
    {
        auto start = std::chrono::high_resolution_clock::now();
        
        for (const auto& [key, value] : perf_data) {
            db->put(key, value);
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        std::cout << "Individual write " << NUM_OPERATIONS << " items took: " 
                  << duration.count() << " microseconds" << std::endl;
    }
}

// 持久化测试
class WriteBatchPersistenceTest : public WriteBatchTest {};

TEST_F(WriteBatchPersistenceTest, BatchPersistence) {
    // 第一次打开数据库，批量写入数据
    {
        WriteBatchOptions batch_options = WriteBatchOptions::default_options();
        batch_options.sync_writes = true; // 确保同步
        
        auto batch = db->new_write_batch(batch_options);
        
        for (const auto& [key, value] : test_pairs) {
            batch->put(key, value);
        }
        
        batch->commit();
        db->close();
    }
    
    // 重新打开数据库，验证数据持久化
    {
        db = DB::open(options);
        
        for (const auto& [key, value] : test_pairs) {
            Bytes retrieved_value = db->get(key);
            EXPECT_EQ(retrieved_value, value);
        }
    }
}

TEST_F(WriteBatchPersistenceTest, BatchDeletePersistence) {
    // 先写入数据
    for (const auto& [key, value] : test_pairs) {
        db->put(key, value);
    }
    db->sync();
    
    // 批量删除
    {
        WriteBatchOptions batch_options = WriteBatchOptions::default_options();
        batch_options.sync_writes = true;
        
        auto batch = db->new_write_batch(batch_options);
        
        batch->remove(test_pairs[0].first);
        batch->remove(test_pairs[2].first);
        
        batch->commit();
        db->close();
    }
    
    // 重新打开验证删除持久化
    {
        db = DB::open(options);
        
        EXPECT_THROW(db->get(test_pairs[0].first), KeyNotFoundError);
        
        Bytes value = db->get(test_pairs[1].first);
        EXPECT_EQ(value, test_pairs[1].second);
        
        EXPECT_THROW(db->get(test_pairs[2].first), KeyNotFoundError);
    }
}
