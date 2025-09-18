#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "bitcask/index.h"
#include <algorithm>
#include <random>
#include <thread>
#include <chrono>

using namespace bitcask;

class IndexTest : public ::testing::Test {
protected:
    void SetUp() override {
        indexer = create_indexer(IndexType::BTREE);
        
        // 创建测试数据
        test_keys = {
            {0x6b, 0x65, 0x79, 0x31}, // "key1"
            {0x6b, 0x65, 0x79, 0x32}, // "key2"
            {0x6b, 0x65, 0x79, 0x33}, // "key3"
            {0x6b, 0x65, 0x79, 0x34}, // "key4"
            {0x6b, 0x65, 0x79, 0x35}, // "key5"
        };
        
        test_positions = {
            LogRecordPos(1, 0, 100),
            LogRecordPos(1, 100, 150),
            LogRecordPos(1, 250, 200),
            LogRecordPos(2, 0, 120),
            LogRecordPos(2, 120, 180),
        };
    }
    
    void TearDown() override {
        indexer->close();
    }
    
    std::unique_ptr<Indexer> indexer;
    std::vector<Bytes> test_keys;
    std::vector<LogRecordPos> test_positions;
};

// 基本索引操作测试
TEST_F(IndexTest, PutAndGet) {
    // 插入数据
    auto old_pos = indexer->put(test_keys[0], test_positions[0]);
    EXPECT_EQ(old_pos, nullptr); // 第一次插入应该返回 nullptr
    
    // 获取数据
    auto retrieved_pos = indexer->get(test_keys[0]);
    ASSERT_NE(retrieved_pos, nullptr);
    EXPECT_EQ(retrieved_pos->fid, test_positions[0].fid);
    EXPECT_EQ(retrieved_pos->offset, test_positions[0].offset);
    EXPECT_EQ(retrieved_pos->size, test_positions[0].size);
}

TEST_F(IndexTest, UpdateExistingKey) {
    // 插入初始数据
    indexer->put(test_keys[0], test_positions[0]);
    
    // 更新相同的 key
    auto old_pos = indexer->put(test_keys[0], test_positions[1]);
    
    // 应该返回旧的位置信息
    ASSERT_NE(old_pos, nullptr);
    EXPECT_EQ(old_pos->fid, test_positions[0].fid);
    EXPECT_EQ(old_pos->offset, test_positions[0].offset);
    EXPECT_EQ(old_pos->size, test_positions[0].size);
    
    // 获取更新后的数据
    auto retrieved_pos = indexer->get(test_keys[0]);
    ASSERT_NE(retrieved_pos, nullptr);
    EXPECT_EQ(retrieved_pos->fid, test_positions[1].fid);
    EXPECT_EQ(retrieved_pos->offset, test_positions[1].offset);
    EXPECT_EQ(retrieved_pos->size, test_positions[1].size);
}

TEST_F(IndexTest, GetNonExistentKey) {
    auto pos = indexer->get({0x6e, 0x6f, 0x6e, 0x65}); // "none"
    EXPECT_EQ(pos, nullptr);
}

TEST_F(IndexTest, RemoveKey) {
    // 插入数据
    indexer->put(test_keys[0], test_positions[0]);
    
    // 删除数据
    auto [removed_pos, success] = indexer->remove(test_keys[0]);
    
    EXPECT_TRUE(success);
    ASSERT_NE(removed_pos, nullptr);
    EXPECT_EQ(removed_pos->fid, test_positions[0].fid);
    EXPECT_EQ(removed_pos->offset, test_positions[0].offset);
    EXPECT_EQ(removed_pos->size, test_positions[0].size);
    
    // 删除后应该无法获取
    auto pos = indexer->get(test_keys[0]);
    EXPECT_EQ(pos, nullptr);
}

TEST_F(IndexTest, RemoveNonExistentKey) {
    auto [removed_pos, success] = indexer->remove({0x6e, 0x6f, 0x6e, 0x65}); // "none"
    
    EXPECT_FALSE(success);
    EXPECT_EQ(removed_pos, nullptr);
}

TEST_F(IndexTest, Size) {
    EXPECT_EQ(indexer->size(), 0);
    
    // 插入数据
    for (size_t i = 0; i < test_keys.size(); ++i) {
        indexer->put(test_keys[i], test_positions[i]);
        EXPECT_EQ(indexer->size(), i + 1);
    }
    
    // 删除数据
    for (size_t i = 0; i < test_keys.size(); ++i) {
        indexer->remove(test_keys[i]);
        EXPECT_EQ(indexer->size(), test_keys.size() - i - 1);
    }
}

TEST_F(IndexTest, ListKeys) {
    // 插入数据
    for (size_t i = 0; i < test_keys.size(); ++i) {
        indexer->put(test_keys[i], test_positions[i]);
    }
    
    auto keys = indexer->list_keys();
    EXPECT_EQ(keys.size(), test_keys.size());
    
    // 排序后比较（因为 BTree 会自动排序）
    std::sort(keys.begin(), keys.end());
    auto sorted_test_keys = test_keys;
    std::sort(sorted_test_keys.begin(), sorted_test_keys.end());
    
    EXPECT_EQ(keys, sorted_test_keys);
}

// 迭代器测试
class IndexIteratorTest : public IndexTest {};

TEST_F(IndexIteratorTest, ForwardIteration) {
    // 插入数据
    for (size_t i = 0; i < test_keys.size(); ++i) {
        indexer->put(test_keys[i], test_positions[i]);
    }
    
    auto iter = indexer->iterator(false); // 正向迭代
    
    std::vector<Bytes> iterated_keys;
    for (iter->rewind(); iter->valid(); iter->next()) {
        iterated_keys.push_back(iter->key());
    }
    
    // 应该按排序顺序返回
    EXPECT_EQ(iterated_keys.size(), test_keys.size());
    
    // 验证排序
    auto sorted_keys = test_keys;
    std::sort(sorted_keys.begin(), sorted_keys.end());
    EXPECT_EQ(iterated_keys, sorted_keys);
}

TEST_F(IndexIteratorTest, ReverseIteration) {
    // 插入数据
    for (size_t i = 0; i < test_keys.size(); ++i) {
        indexer->put(test_keys[i], test_positions[i]);
    }
    
    auto iter = indexer->iterator(true); // 反向迭代
    
    std::vector<Bytes> iterated_keys;
    for (iter->rewind(); iter->valid(); iter->next()) {
        iterated_keys.push_back(iter->key());
    }
    
    // 应该按反向排序顺序返回
    EXPECT_EQ(iterated_keys.size(), test_keys.size());
    
    // 验证反向排序
    auto sorted_keys = test_keys;
    std::sort(sorted_keys.begin(), sorted_keys.end(), std::greater<Bytes>());
    EXPECT_EQ(iterated_keys, sorted_keys);
}

TEST_F(IndexIteratorTest, SeekOperation) {
    // 插入数据
    for (size_t i = 0; i < test_keys.size(); ++i) {
        indexer->put(test_keys[i], test_positions[i]);
    }
    
    auto iter = indexer->iterator(false);
    
    // 查找 "key3"
    iter->seek(test_keys[2]);
    EXPECT_TRUE(iter->valid());
    EXPECT_EQ(iter->key(), test_keys[2]);
    
    // 查找不存在的 key，应该定位到下一个更大的 key
    Bytes seek_key = {0x6b, 0x65, 0x79, 0x32, 0x35}; // "key25"，介于 key2 和 key3 之间
    iter->seek(seek_key);
    EXPECT_TRUE(iter->valid());
    EXPECT_EQ(iter->key(), test_keys[2]); // 应该定位到 key3
}

TEST_F(IndexIteratorTest, IteratorValue) {
    // 插入数据
    indexer->put(test_keys[0], test_positions[0]);
    
    auto iter = indexer->iterator(false);
    iter->rewind();
    
    EXPECT_TRUE(iter->valid());
    EXPECT_EQ(iter->key(), test_keys[0]);
    
    LogRecordPos pos = iter->value();
    EXPECT_EQ(pos.fid, test_positions[0].fid);
    EXPECT_EQ(pos.offset, test_positions[0].offset);
    EXPECT_EQ(pos.size, test_positions[0].size);
}

TEST_F(IndexIteratorTest, EmptyIndexIteration) {
    auto iter = indexer->iterator(false);
    
    iter->rewind();
    EXPECT_FALSE(iter->valid());
    
    // 在空索引上调用 key() 和 value() 应该抛出异常
    EXPECT_THROW(iter->key(), BitcaskException);
    EXPECT_THROW(iter->value(), BitcaskException);
}

TEST_F(IndexIteratorTest, IteratorClose) {
    // 插入数据
    indexer->put(test_keys[0], test_positions[0]);
    
    auto iter = indexer->iterator(false);
    iter->rewind();
    
    EXPECT_TRUE(iter->valid());
    
    // 关闭迭代器
    iter->close();
    
    // 关闭后应该无效
    EXPECT_FALSE(iter->valid());
}

// 并发测试
class IndexConcurrencyTest : public IndexTest {};

TEST_F(IndexConcurrencyTest, ConcurrentReads) {
    // 插入测试数据
    for (size_t i = 0; i < test_keys.size(); ++i) {
        indexer->put(test_keys[i], test_positions[i]);
    }
    
    std::vector<std::thread> threads;
    std::atomic<int> success_count{0};
    
    // 启动多个读线程
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([this, &success_count]() {
            for (int j = 0; j < 100; ++j) {
                for (size_t k = 0; k < test_keys.size(); ++k) {
                    auto pos = indexer->get(test_keys[k]);
                    if (pos != nullptr) {
                        success_count++;
                    }
                }
            }
        });
    }
    
    // 等待所有线程完成
    for (auto& thread : threads) {
        thread.join();
    }
    
    // 所有读取都应该成功
    EXPECT_EQ(success_count.load(), 10 * 100 * test_keys.size());
}

TEST_F(IndexConcurrencyTest, ConcurrentWrites) {
    std::vector<std::thread> threads;
    std::atomic<int> write_count{0};
    
    // 启动多个写线程
    for (int i = 0; i < 5; ++i) {
        threads.emplace_back([this, &write_count, i]() {
            for (int j = 0; j < 20; ++j) {
                Bytes key = {static_cast<uint8_t>(i), static_cast<uint8_t>(j)};
                LogRecordPos pos(i, j * 100, 50);
                indexer->put(key, pos);
                write_count++;
            }
        });
    }
    
    // 等待所有线程完成
    for (auto& thread : threads) {
        thread.join();
    }
    
    // 验证写入数量
    EXPECT_EQ(write_count.load(), 5 * 20);
    EXPECT_EQ(indexer->size(), 5 * 20);
}

TEST_F(IndexConcurrencyTest, MixedReadWrite) {
    std::vector<std::thread> threads;
    std::atomic<int> operation_count{0};
    
    // 读线程
    for (int i = 0; i < 3; ++i) {
        threads.emplace_back([this, &operation_count]() {
            for (int j = 0; j < 50; ++j) {
                for (size_t k = 0; k < test_keys.size(); ++k) {
                    indexer->get(test_keys[k]);
                    operation_count++;
                }
            }
        });
    }
    
    // 写线程
    for (int i = 0; i < 2; ++i) {
        threads.emplace_back([this, &operation_count, i]() {
            for (int j = 0; j < 30; ++j) {
                Bytes key = {static_cast<uint8_t>(i + 10), static_cast<uint8_t>(j)};
                LogRecordPos pos(i + 10, j * 100, 50);
                indexer->put(key, pos);
                operation_count++;
            }
        });
    }
    
    // 等待所有线程完成
    for (auto& thread : threads) {
        thread.join();
    }
    
    EXPECT_EQ(operation_count.load(), 3 * 50 * test_keys.size() + 2 * 30);
}

// 性能测试
class IndexPerformanceTest : public ::testing::Test {
protected:
    void SetUp() override {
        indexer = create_indexer(IndexType::BTREE);
        
        // 生成大量测试数据
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 255);
        
        for (int i = 0; i < 10000; ++i) {
            Bytes key(8);
            for (size_t j = 0; j < key.size(); ++j) {
                key[j] = dis(gen);
            }
            test_data.emplace_back(key, LogRecordPos(i / 1000, i * 100, 50 + i % 100));
        }
    }
    
    void TearDown() override {
        indexer->close();
    }
    
    std::unique_ptr<Indexer> indexer;
    std::vector<std::pair<Bytes, LogRecordPos>> test_data;
};

TEST_F(IndexPerformanceTest, InsertPerformance) {
    auto start = std::chrono::high_resolution_clock::now();
    
    for (const auto& [key, pos] : test_data) {
        indexer->put(key, pos);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "Insert " << test_data.size() << " items took: " 
              << duration.count() << " microseconds" << std::endl;
    std::cout << "Average insert time: " 
              << duration.count() / test_data.size() << " microseconds per item" << std::endl;
    
    EXPECT_EQ(indexer->size(), test_data.size());
}

TEST_F(IndexPerformanceTest, LookupPerformance) {
    // 先插入数据
    for (const auto& [key, pos] : test_data) {
        indexer->put(key, pos);
    }
    
    // 随机打乱查找顺序
    auto lookup_keys = test_data;
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(lookup_keys.begin(), lookup_keys.end(), g);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (const auto& [key, _] : lookup_keys) {
        auto pos = indexer->get(key);
        EXPECT_NE(pos, nullptr);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "Lookup " << test_data.size() << " items took: " 
              << duration.count() << " microseconds" << std::endl;
    std::cout << "Average lookup time: " 
              << duration.count() / test_data.size() << " microseconds per item" << std::endl;
}

TEST_F(IndexPerformanceTest, IterationPerformance) {
    // 先插入数据
    for (const auto& [key, pos] : test_data) {
        indexer->put(key, pos);
    }
    
    auto start = std::chrono::high_resolution_clock::now();
    
    auto iter = indexer->iterator(false);
    int count = 0;
    for (iter->rewind(); iter->valid(); iter->next()) {
        // 访问 key 和 value 来模拟实际使用
        auto key = iter->key();
        auto pos = iter->value();
        (void)pos; // 标记为有意未使用
        count++;
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "Iterate " << count << " items took: " 
              << duration.count() << " microseconds" << std::endl;
    std::cout << "Average iteration time: " 
              << duration.count() / count << " microseconds per item" << std::endl;
    
    EXPECT_EQ(count, test_data.size());
}

// 边界条件测试
class IndexEdgeCaseTest : public IndexTest {};

TEST_F(IndexEdgeCaseTest, EmptyKey) {
    Bytes empty_key;
    LogRecordPos pos(1, 0, 100);
    
    // 空 key 应该能正常处理
    EXPECT_NO_THROW(indexer->put(empty_key, pos));
    
    auto retrieved_pos = indexer->get(empty_key);
    ASSERT_NE(retrieved_pos, nullptr);
    EXPECT_EQ(retrieved_pos->fid, pos.fid);
}

TEST_F(IndexEdgeCaseTest, LargeKey) {
    // 创建大 key（1MB）
    Bytes large_key(1024 * 1024, 0xAB);
    LogRecordPos pos(1, 0, 100);
    
    EXPECT_NO_THROW(indexer->put(large_key, pos));
    
    auto retrieved_pos = indexer->get(large_key);
    ASSERT_NE(retrieved_pos, nullptr);
    EXPECT_EQ(retrieved_pos->fid, pos.fid);
}

TEST_F(IndexEdgeCaseTest, ManySmallKeys) {
    // 插入大量小 key
    for (int i = 0; i < 1000; ++i) {
        Bytes key = {static_cast<uint8_t>(i & 0xFF)};
        LogRecordPos pos(i / 100, i * 10, 20);
        indexer->put(key, pos);
    }
    
    EXPECT_EQ(indexer->size(), 256); // 只有 256 个不同的单字节 key
    
    // 验证最后插入的值
    for (int i = 0; i < 256; ++i) {
        Bytes key = {static_cast<uint8_t>(i)};
        auto pos = indexer->get(key);
        ASSERT_NE(pos, nullptr);
        
        // 应该是该字节值对应的最后一次插入
        int last_i = i;
        for (int j = i + 256; j < 1000; j += 256) {
            last_i = j;
        }
        EXPECT_EQ(pos->fid, last_i / 100);
        EXPECT_EQ(pos->offset, last_i * 10);
    }
}
