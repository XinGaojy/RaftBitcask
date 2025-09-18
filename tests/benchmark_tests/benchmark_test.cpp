#include <gtest/gtest.h>
#include "bitcask/bitcask.h"
#include "bitcask/utils.h"
#include <chrono>
#include <random>
#include <algorithm>
#include <iomanip>
#include <fstream>
#include<thread>

using namespace bitcask;

class BenchmarkTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir = "/tmp/bitcask_benchmark_test";
        utils::remove_directory(test_dir);
        
        // 准备测试数据
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 255);
        
        // 生成不同大小的测试数据
        for (int i = 0; i < NUM_KEYS; ++i) {
            // 生成随机 key
            Bytes key(16);
            for (size_t j = 0; j < key.size(); ++j) {
                key[j] = dis(gen);
            }
            
            // 生成不同大小的 value
            size_t value_size = 64 + (i % 256); // 64-320 字节
            Bytes value(value_size);
            for (size_t j = 0; j < value.size(); ++j) {
                value[j] = dis(gen);
            }
            
            test_keys.push_back(key);
            test_values.push_back(value);
        }
        
        // 创建随机访问顺序
        random_indices.resize(NUM_KEYS);
        std::iota(random_indices.begin(), random_indices.end(), 0);
        std::shuffle(random_indices.begin(), random_indices.end(), gen);
    }
    
    void TearDown() override {
        utils::remove_directory(test_dir);
    }
    
    // 性能测试辅助函数
    struct BenchmarkResult {
        double qps;
        double avg_latency_us;
        double p99_latency_us;
        size_t total_operations;
        std::chrono::microseconds total_time;
    };
    
    BenchmarkResult measure_performance(std::function<void()> operation, int num_operations) {
        std::vector<std::chrono::microseconds> latencies;
        latencies.reserve(num_operations);
        
        auto total_start = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < num_operations; ++i) {
            auto start = std::chrono::high_resolution_clock::now();
            operation();
            auto end = std::chrono::high_resolution_clock::now();
            
            latencies.push_back(std::chrono::duration_cast<std::chrono::microseconds>(end - start));
        }
        
        auto total_end = std::chrono::high_resolution_clock::now();
        auto total_time = std::chrono::duration_cast<std::chrono::microseconds>(total_end - total_start);
        
        // 计算统计信息
        std::sort(latencies.begin(), latencies.end());
        
        double avg_latency = 0;
        for (const auto& latency : latencies) {
            avg_latency += latency.count();
        }
        avg_latency /= latencies.size();
        
        double p99_latency = latencies[static_cast<size_t>(latencies.size() * 0.99)].count();
        double qps = (double)num_operations * 1000000 / total_time.count();
        
        return BenchmarkResult{qps, avg_latency, p99_latency, static_cast<size_t>(num_operations), total_time};
    }
    
    void print_benchmark_result(const std::string& test_name, const BenchmarkResult& result) {
        std::cout << "\n" << test_name << " Benchmark Results:" << std::endl;
        std::cout << "  QPS: " << std::fixed << std::setprecision(2) << result.qps << std::endl;
        std::cout << "  Avg Latency: " << std::fixed << std::setprecision(2) << result.avg_latency_us << " μs" << std::endl;
        std::cout << "  P99 Latency: " << std::fixed << std::setprecision(2) << result.p99_latency_us << " μs" << std::endl;
        std::cout << "  Total Operations: " << result.total_operations << std::endl;
        std::cout << "  Total Time: " << result.total_time.count() << " μs" << std::endl;
    }
    
    static const int NUM_KEYS = 50000;
    std::string test_dir;
    std::vector<Bytes> test_keys;
    std::vector<Bytes> test_values;
    std::vector<int> random_indices;
};

// 静态成员定义
const int BenchmarkTest::NUM_KEYS;

// 基础写入性能测试
TEST_F(BenchmarkTest, WritePerformance) {
    Options options = Options::default_options();
    options.dir_path = test_dir;
    options.data_file_size = 256 * 1024 * 1024; // 256MB
    options.sync_writes = false;
    
    auto db = bitcask::open(options);
    
    int operation_index = 0;
    auto result = measure_performance([&]() {
        db->put(test_keys[operation_index], test_values[operation_index]);
        operation_index++;
    }, NUM_KEYS);
    
    print_benchmark_result("Sequential Write", result);
    
    // 验证性能指标
    EXPECT_GT(result.qps, 10000); // 至少 10K QPS
    EXPECT_LT(result.avg_latency_us, 1000); // 平均延迟小于 1ms
    
    db->close();
}

// 随机写入性能测试
TEST_F(BenchmarkTest, RandomWritePerformance) {
    Options options = Options::default_options();
    options.dir_path = test_dir;
    options.data_file_size = 256 * 1024 * 1024;
    options.sync_writes = false;
    
    auto db = bitcask::open(options);
    
    int operation_index = 0;
    auto result = measure_performance([&]() {
        int index = random_indices[operation_index];
        db->put(test_keys[index], test_values[index]);
        operation_index++;
    }, NUM_KEYS);
    
    print_benchmark_result("Random Write", result);
    
    EXPECT_GT(result.qps, 8000); // 随机写入稍慢
    
    db->close();
}

// 读取性能测试
TEST_F(BenchmarkTest, ReadPerformance) {
    Options options = Options::default_options();
    options.dir_path = test_dir;
    options.sync_writes = false;
    
    auto db = bitcask::open(options);
    
    // 先写入数据
    for (int i = 0; i < NUM_KEYS; ++i) {
        db->put(test_keys[i], test_values[i]);
    }
    
    // 测试顺序读取
    int operation_index = 0;
    auto result = measure_performance([&]() {
        db->get(test_keys[operation_index]);
        operation_index++;
    }, NUM_KEYS);
    
    print_benchmark_result("Sequential Read", result);
    
    EXPECT_GT(result.qps, 50000); // 读取应该很快
    EXPECT_LT(result.avg_latency_us, 100); // 平均延迟小于 100μs
    
    db->close();
}

// 随机读取性能测试
TEST_F(BenchmarkTest, RandomReadPerformance) {
    Options options = Options::default_options();
    options.dir_path = test_dir;
    options.sync_writes = false;
    
    auto db = bitcask::open(options);
    
    // 先写入数据
    for (int i = 0; i < NUM_KEYS; ++i) {
        db->put(test_keys[i], test_values[i]);
    }
    
    // 测试随机读取
    int operation_index = 0;
    auto result = measure_performance([&]() {
        int index = random_indices[operation_index];
        db->get(test_keys[index]);
        operation_index++;
    }, NUM_KEYS);
    
    print_benchmark_result("Random Read", result);
    
    EXPECT_GT(result.qps, 30000); // 随机读取稍慢但仍应很快
    
    db->close();
}

// 混合读写性能测试
TEST_F(BenchmarkTest, MixedReadWritePerformance) {
    Options options = Options::default_options();
    options.dir_path = test_dir;
    options.sync_writes = false;
    
    auto db = bitcask::open(options);
    
    // 先写入一半数据
    for (int i = 0; i < NUM_KEYS / 2; ++i) {
        db->put(test_keys[i], test_values[i]);
    }
    
    // 混合操作：70% 读取，30% 写入
    int operation_index = 0;
    auto result = measure_performance([&]() {
        if (operation_index % 10 < 7) {
            // 读取操作
            int read_index = random_indices[operation_index % (NUM_KEYS / 2)];
            db->get(test_keys[read_index]);
        } else {
            // 写入操作
            int write_index = NUM_KEYS / 2 + (operation_index % (NUM_KEYS / 2));
            db->put(test_keys[write_index], test_values[write_index]);
        }
        operation_index++;
    }, NUM_KEYS);
    
    print_benchmark_result("Mixed Read/Write (70/30)", result);
    
    EXPECT_GT(result.qps, 20000);
    
    db->close();
}

// 批量写入性能测试
TEST_F(BenchmarkTest, BatchWritePerformance) {
    Options options = Options::default_options();
    options.dir_path = test_dir;
    options.sync_writes = false;
    
    auto db = bitcask::open(options);
    
    WriteBatchOptions batch_options = WriteBatchOptions::default_options();
    batch_options.sync_writes = false;
    
    const int BATCH_SIZE = 1000;
    const int NUM_BATCHES = NUM_KEYS / BATCH_SIZE;
    
    int operation_index = 0;
    auto result = measure_performance([&]() {
        auto batch = db->new_write_batch(batch_options);
        
        for (int i = 0; i < BATCH_SIZE; ++i) {
            int index = operation_index * BATCH_SIZE + i;
            if (index < NUM_KEYS) {
                batch->put(test_keys[index], test_values[index]);
            }
        }
        
        batch->commit();
        operation_index++;
    }, NUM_BATCHES);
    
    // 计算实际的 QPS（基于实际写入的记录数）
    double actual_qps = (double)NUM_KEYS * 1000000 / result.total_time.count();
    
    std::cout << "\nBatch Write Benchmark Results:" << std::endl;
    std::cout << "  Batch QPS: " << std::fixed << std::setprecision(2) << result.qps << std::endl;
    std::cout << "  Record QPS: " << std::fixed << std::setprecision(2) << actual_qps << std::endl;
    std::cout << "  Avg Batch Latency: " << std::fixed << std::setprecision(2) << result.avg_latency_us << " μs" << std::endl;
    std::cout << "  Records per Batch: " << BATCH_SIZE << std::endl;
    
    EXPECT_GT(actual_qps, 50000); // 批量写入应该更快
    
    db->close();
}

// 迭代器性能测试
TEST_F(BenchmarkTest, IteratorPerformance) {
    Options options = Options::default_options();
    options.dir_path = test_dir;
    options.sync_writes = false;
    
    auto db = bitcask::open(options);
    
    // 写入数据
    for (int i = 0; i < NUM_KEYS; ++i) {
        db->put(test_keys[i], test_values[i]);
    }
    
    // 测试全量迭代
    auto start = std::chrono::high_resolution_clock::now();
    
    IteratorOptions iter_options;
    auto iter = db->iterator(iter_options);
    
    int count = 0;
    for (iter->rewind(); iter->valid(); iter->next()) {
        // 访问 key 和 value 来模拟实际使用
        auto key = iter->key();
        auto value = iter->value();
        count++;
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    double iteration_qps = (double)count * 1000000 / duration.count();
    
    std::cout << "\nIterator Performance:" << std::endl;
    std::cout << "  Iteration QPS: " << std::fixed << std::setprecision(2) << iteration_qps << std::endl;
    std::cout << "  Total Records: " << count << std::endl;
    std::cout << "  Total Time: " << duration.count() << " μs" << std::endl;
    
    EXPECT_EQ(count, NUM_KEYS);
    EXPECT_GT(iteration_qps, 100000); // 迭代应该很快
    
    db->close();
}

// 不同数据大小的性能测试
TEST_F(BenchmarkTest, VariableDataSizePerformance) {
    Options options = Options::default_options();
    options.dir_path = test_dir;
    options.sync_writes = false;
    
    auto db = bitcask::open(options);
    
    // 测试不同大小的值
    std::vector<size_t> value_sizes = {64, 256, 1024, 4096, 16384};
    
    for (size_t value_size : value_sizes) {
        // 创建指定大小的测试数据
        std::vector<Bytes> sized_values;
        for (int i = 0; i < 1000; ++i) {
            sized_values.emplace_back(value_size, static_cast<uint8_t>(i % 256));
        }
        
        // 写入性能测试
        int operation_index = 0;
        auto write_result = measure_performance([&]() {
            db->put(test_keys[operation_index], sized_values[operation_index % sized_values.size()]);
            operation_index++;
        }, 1000);
        
        // 读取性能测试
        operation_index = 0;
        auto read_result = measure_performance([&]() {
            db->get(test_keys[operation_index]);
            operation_index++;
        }, 1000);
        
        std::cout << "\nValue Size " << value_size << " bytes:" << std::endl;
        std::cout << "  Write QPS: " << std::fixed << std::setprecision(2) << write_result.qps << std::endl;
        std::cout << "  Read QPS: " << std::fixed << std::setprecision(2) << read_result.qps << std::endl;
        
        // 清理数据
        for (int i = 0; i < 1000; ++i) {
            db->remove(test_keys[i]);
        }
    }
    
    db->close();
}

// 并发性能测试
TEST_F(BenchmarkTest, ConcurrentPerformance) {
    Options options = Options::default_options();
    options.dir_path = test_dir;
    options.sync_writes = false;
    
    auto db = bitcask::open(options);
    
    const int NUM_THREADS = 4;
    const int OPS_PER_THREAD = NUM_KEYS / NUM_THREADS;
    
    // 并发写入测试
    {
        std::vector<std::thread> threads;
        std::atomic<int> total_operations{0};
        
        auto start = std::chrono::high_resolution_clock::now();
        
        for (int t = 0; t < NUM_THREADS; ++t) {
            threads.emplace_back([&, t]() {
                for (int i = 0; i < OPS_PER_THREAD; ++i) {
                    int index = t * OPS_PER_THREAD + i;
                    if (index < NUM_KEYS) {
                        db->put(test_keys[index], test_values[index]);
                        total_operations++;
                    }
                }
            });
        }
        
        for (auto& thread : threads) {
            thread.join();
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        double concurrent_write_qps = (double)total_operations.load() * 1000000 / duration.count();
        
        std::cout << "\nConcurrent Write Performance (" << NUM_THREADS << " threads):" << std::endl;
        std::cout << "  QPS: " << std::fixed << std::setprecision(2) << concurrent_write_qps << std::endl;
        std::cout << "  Total Operations: " << total_operations.load() << std::endl;
        
        EXPECT_GT(concurrent_write_qps, 15000);
    }
    
    // 并发读取测试
    {
        std::vector<std::thread> threads;
        std::atomic<int> total_operations{0};
        
        auto start = std::chrono::high_resolution_clock::now();
        
        for (int t = 0; t < NUM_THREADS; ++t) {
            threads.emplace_back([&, t]() {
                for (int i = 0; i < OPS_PER_THREAD; ++i) {
                    int index = t * OPS_PER_THREAD + i;
                    if (index < NUM_KEYS) {
                        try {
                            db->get(test_keys[index]);
                            total_operations++;
                        } catch (...) {
                            // 忽略异常
                        }
                    }
                }
            });
        }
        
        for (auto& thread : threads) {
            thread.join();
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        double concurrent_read_qps = (double)total_operations.load() * 1000000 / duration.count();
        
        std::cout << "\nConcurrent Read Performance (" << NUM_THREADS << " threads):" << std::endl;
        std::cout << "  QPS: " << std::fixed << std::setprecision(2) << concurrent_read_qps << std::endl;
        std::cout << "  Total Operations: " << total_operations.load() << std::endl;
        
        EXPECT_GT(concurrent_read_qps, 50000);
    }
    
    db->close();
}

// 内存使用性能测试
TEST_F(BenchmarkTest, MemoryUsageTest) {
    Options options = Options::default_options();
    options.dir_path = test_dir;
    options.sync_writes = false;
    
    auto db = bitcask::open(options);
    
    // 写入数据并监控内存使用
    const int CHECKPOINT_INTERVAL = NUM_KEYS / 10;
    
    for (int i = 0; i < NUM_KEYS; ++i) {
        db->put(test_keys[i], test_values[i]);
        
        if (i % CHECKPOINT_INTERVAL == 0) {
            Stat stat = db->stat();
            std::cout << "After " << i << " operations:" << std::endl;
            std::cout << "  Key count: " << stat.key_num << std::endl;
            std::cout << "  Disk size: " << stat.disk_size << " bytes" << std::endl;
            std::cout << "  Reclaimable size: " << stat.reclaimable_size << " bytes" << std::endl;
        }
    }
    
    // 最终统计
    Stat final_stat = db->stat();
    std::cout << "\nFinal Statistics:" << std::endl;
    std::cout << "  Total keys: " << final_stat.key_num << std::endl;
    std::cout << "  Data files: " << final_stat.data_file_num << std::endl;
    std::cout << "  Disk size: " << final_stat.disk_size << " bytes" << std::endl;
    std::cout << "  Reclaimable size: " << final_stat.reclaimable_size << " bytes" << std::endl;
    
    EXPECT_EQ(final_stat.key_num, NUM_KEYS);
    EXPECT_GT(final_stat.disk_size, 0);
    
    db->close();
}

// 持久化性能测试
TEST_F(BenchmarkTest, PersistencePerformance) {
    Options options = Options::default_options();
    options.dir_path = test_dir;
    options.sync_writes = true; // 启用同步写入
    
    auto db = bitcask::open(options);
    
    // 测试同步写入性能
    int operation_index = 0;
    auto sync_result = measure_performance([&]() {
        db->put(test_keys[operation_index], test_values[operation_index]);
        operation_index++;
    }, 1000); // 减少操作数量，因为同步写入较慢
    
    print_benchmark_result("Synchronous Write", sync_result);
    
    // 同步写入应该较慢但仍有合理性能
    EXPECT_GT(sync_result.qps, 100);
    EXPECT_LT(sync_result.qps, 10000); // 应该明显慢于异步写入
    
    db->close();
    
    // 测试重启后的恢复性能
    auto recovery_start = std::chrono::high_resolution_clock::now();
    
    auto recovered_db = bitcask::open(options);
    
    auto recovery_end = std::chrono::high_resolution_clock::now();
    auto recovery_time = std::chrono::duration_cast<std::chrono::microseconds>(recovery_end - recovery_start);
    
    std::cout << "\nDatabase Recovery Performance:" << std::endl;
    std::cout << "  Recovery time: " << recovery_time.count() << " μs" << std::endl;
    std::cout << "  Records recovered: " << 1000 << std::endl;
    
    // 验证数据恢复正确性
    for (int i = 0; i < 1000; ++i) {
        Bytes value = recovered_db->get(test_keys[i]);
        EXPECT_EQ(value, test_values[i]);
    }
    
    recovered_db->close();
}
