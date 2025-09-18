#pragma once

#include "common.h"
#include <string>
#include <vector>
#include <random>
#include <cstdint>
#include <chrono>

namespace bitcask {

/**
 * @brief 测试数据生成工具类
 * 
 * 提供各种随机数据生成功能，用于测试和基准测试。
 */
class TestDataGenerator {
public:
    /**
     * @brief 构造函数
     * @param seed 随机种子，默认使用当前时间
     */
    explicit TestDataGenerator(uint32_t seed = 0);

    /**
     * @brief 生成随机字节数组
     * @param length 字节长度
     * @return 随机字节数组
     */
    Bytes random_bytes(size_t length);

    /**
     * @brief 生成随机字符串
     * @param length 字符串长度
     * @param charset 字符集，默认为字母数字
     * @return 随机字符串
     */
    std::string random_string(size_t length, 
                             const std::string& charset = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789");

    /**
     * @brief 生成随机key
     * @param min_length 最小长度
     * @param max_length 最大长度
     * @return 随机key
     */
    Bytes random_key(size_t min_length = 8, size_t max_length = 32);

    /**
     * @brief 生成随机value
     * @param min_length 最小长度
     * @param max_length 最大长度
     * @return 随机value
     */
    Bytes random_value(size_t min_length = 64, size_t max_length = 1024);

    /**
     * @brief 生成随机整数
     * @param min 最小值
     * @param max 最大值
     * @return 随机整数
     */
    int random_int(int min = 0, int max = 1000000);

    /**
     * @brief 生成随机浮点数
     * @param min 最小值
     * @param max 最大值
     * @return 随机浮点数
     */
    double random_double(double min = 0.0, double max = 1000.0);

    /**
     * @brief 生成一批随机key-value对
     * @param count 数量
     * @param key_size 键大小（0表示随机大小）
     * @param value_size 值大小（0表示随机大小）
     * @return key-value对的向量
     */
    std::vector<std::pair<Bytes, Bytes>> random_kv_pairs(
        size_t count, 
        size_t key_size = 0, 
        size_t value_size = 0);

    /**
     * @brief 生成有序的key-value对（用于测试迭代器）
     * @param count 数量
     * @param key_prefix 键前缀
     * @param value_size 值大小
     * @return 有序的key-value对
     */
    std::vector<std::pair<Bytes, Bytes>> ordered_kv_pairs(
        size_t count,
        const std::string& key_prefix = "key_",
        size_t value_size = 256);

    /**
     * @brief 生成重复的key（用于测试覆盖）
     * @param base_keys 基础键列表
     * @param repeat_factor 重复因子
     * @return 包含重复key的key-value对
     */
    std::vector<std::pair<Bytes, Bytes>> duplicate_key_pairs(
        const std::vector<Bytes>& base_keys,
        size_t repeat_factor = 3);

    /**
     * @brief 生成二进制安全的数据（包含所有字节值）
     * @param length 数据长度
     * @return 二进制数据
     */
    Bytes binary_safe_data(size_t length);

    /**
     * @brief 生成性能测试数据集
     * @param count 数据量
     * @param key_size 键大小
     * @param value_size 值大小
     * @param write_ratio 写操作比例（0.0-1.0）
     * @return 操作列表（true=写，false=读）和对应的数据
     */
    struct PerformanceTestData {
        std::vector<std::pair<Bytes, Bytes>> kv_pairs;
        std::vector<bool> operations; // true=写, false=读
        std::vector<size_t> indices;  // 对应kv_pairs的索引
    };
    
    PerformanceTestData performance_test_data(
        size_t count,
        size_t key_size = 16,
        size_t value_size = 256,
        double write_ratio = 0.7);

private:
    std::mt19937 rng_;              // 随机数生成器
    std::uniform_int_distribution<uint8_t> byte_dist_;  // 字节分布

    /**
     * @brief 生成指定长度的随机数据
     * @param length 长度
     * @param allow_zero 是否允许零字节
     * @return 随机数据
     */
    Bytes generate_random_data(size_t length, bool allow_zero = true);
};

/**
 * @brief 测试辅助函数集合
 */
class TestUtils {
public:
    /**
     * @brief 比较两个字节数组是否相等
     * @param a 数组a
     * @param b 数组b
     * @return 是否相等
     */
    static bool bytes_equal(const Bytes& a, const Bytes& b);

    /**
     * @brief 创建临时目录
     * @param prefix 目录前缀
     * @return 临时目录路径
     */
    static std::string create_temp_dir(const std::string& prefix = "bitcask_test_");

    /**
     * @brief 删除目录及其内容
     * @param dir_path 目录路径
     * @return 是否成功
     */
    static bool remove_dir(const std::string& dir_path);

    /**
     * @brief 获取目录大小
     * @param dir_path 目录路径
     * @return 目录大小（字节）
     */
    static size_t get_dir_size(const std::string& dir_path);

    /**
     * @brief 计算字节数组的校验和
     * @param data 数据
     * @return 校验和
     */
    static uint32_t checksum(const Bytes& data);

    /**
     * @brief 格式化字节大小为人类可读格式
     * @param bytes 字节数
     * @return 格式化字符串
     */
    static std::string format_bytes(size_t bytes);

    /**
     * @brief 性能计时器
     */
    class Timer {
    public:
        Timer();
        void start();
        void stop();
        double elapsed_ms() const;
        double elapsed_us() const;
        
    private:
        std::chrono::high_resolution_clock::time_point start_time_;
        std::chrono::high_resolution_clock::time_point end_time_;
        bool started_;
        bool stopped_;
    };
};

} // namespace bitcask
