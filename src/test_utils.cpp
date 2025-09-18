#include "bitcask/test_utils.h"
#include "bitcask/utils.h"
#include <chrono>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <map>
#include <thread>
#ifdef USE_ZLIB_CRC32
#include <zlib.h>
#else
#include <crc32c/crc32c.h>
#endif

namespace bitcask {

TestDataGenerator::TestDataGenerator(uint32_t seed)
    : rng_(seed == 0 ? std::chrono::steady_clock::now().time_since_epoch().count() : seed),
      byte_dist_(0, 255) {
}

Bytes TestDataGenerator::random_bytes(size_t length) {
    return generate_random_data(length, true);
}

std::string TestDataGenerator::random_string(size_t length, const std::string& charset) {
    if (charset.empty()) {
        return "";
    }
    
    std::string result;
    result.reserve(length);
    
    std::uniform_int_distribution<size_t> char_dist(0, charset.size() - 1);
    
    for (size_t i = 0; i < length; ++i) {
        result += charset[char_dist(rng_)];
    }
    
    return result;
}

Bytes TestDataGenerator::random_key(size_t min_length, size_t max_length) {
    std::uniform_int_distribution<size_t> length_dist(min_length, max_length);
    size_t length = length_dist(rng_);
    
    // 生成字母数字key，避免特殊字符
    std::string charset = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    std::string key_str = random_string(length, charset);
    
    return Bytes(key_str.begin(), key_str.end());
}

Bytes TestDataGenerator::random_value(size_t min_length, size_t max_length) {
    std::uniform_int_distribution<size_t> length_dist(min_length, max_length);
    size_t length = length_dist(rng_);
    return generate_random_data(length, true);
}

int TestDataGenerator::random_int(int min, int max) {
    std::uniform_int_distribution<int> int_dist(min, max);
    return int_dist(rng_);
}

double TestDataGenerator::random_double(double min, double max) {
    std::uniform_real_distribution<double> double_dist(min, max);
    return double_dist(rng_);
}

std::vector<std::pair<Bytes, Bytes>> TestDataGenerator::random_kv_pairs(
    size_t count, size_t key_size, size_t value_size) {
    
    std::vector<std::pair<Bytes, Bytes>> pairs;
    pairs.reserve(count);
    
    for (size_t i = 0; i < count; ++i) {
        Bytes key = (key_size == 0) ? random_key() : random_key(key_size, key_size);
        Bytes value = (value_size == 0) ? random_value() : random_value(value_size, value_size);
        pairs.emplace_back(std::move(key), std::move(value));
    }
    
    return pairs;
}

std::vector<std::pair<Bytes, Bytes>> TestDataGenerator::ordered_kv_pairs(
    size_t count, const std::string& key_prefix, size_t value_size) {
    
    std::vector<std::pair<Bytes, Bytes>> pairs;
    pairs.reserve(count);
    
    for (size_t i = 0; i < count; ++i) {
        // 生成有序的key
        std::ostringstream key_ss;
        key_ss << key_prefix << std::setfill('0') << std::setw(10) << i;
        std::string key_str = key_ss.str();
        Bytes key(key_str.begin(), key_str.end());
        
        // 生成随机value
        Bytes value = random_value(value_size, value_size);
        
        pairs.emplace_back(std::move(key), std::move(value));
    }
    
    return pairs;
}

std::vector<std::pair<Bytes, Bytes>> TestDataGenerator::duplicate_key_pairs(
    const std::vector<Bytes>& base_keys, size_t repeat_factor) {
    
    std::vector<std::pair<Bytes, Bytes>> pairs;
    pairs.reserve(base_keys.size() * repeat_factor);
    
    for (size_t repeat = 0; repeat < repeat_factor; ++repeat) {
        for (const auto& key : base_keys) {
            Bytes value = random_value();
            pairs.emplace_back(key, std::move(value));
        }
    }
    
    // 打乱顺序
    std::shuffle(pairs.begin(), pairs.end(), rng_);
    
    return pairs;
}

Bytes TestDataGenerator::binary_safe_data(size_t length) {
    Bytes data;
    data.reserve(length);
    
    for (size_t i = 0; i < length; ++i) {
        data.push_back(static_cast<uint8_t>(i % 256));
    }
    
    // 打乱以增加随机性
    std::shuffle(data.begin(), data.end(), rng_);
    
    return data;
}

TestDataGenerator::PerformanceTestData TestDataGenerator::performance_test_data(
    size_t count, size_t key_size, size_t value_size, double write_ratio) {
    
    PerformanceTestData data;
    
    // 生成key-value对
    data.kv_pairs = random_kv_pairs(count, key_size, value_size);
    
    // 生成操作序列
    data.operations.reserve(count);
    data.indices.reserve(count);
    
    std::uniform_real_distribution<double> operation_dist(0.0, 1.0);
    std::uniform_int_distribution<size_t> index_dist(0, count - 1);
    
    for (size_t i = 0; i < count; ++i) {
        bool is_write = operation_dist(rng_) < write_ratio;
        data.operations.push_back(is_write);
        
        if (is_write) {
            data.indices.push_back(i); // 写入新数据
        } else {
            data.indices.push_back(index_dist(rng_)); // 读取已有数据
        }
    }
    
    return data;
}

Bytes TestDataGenerator::generate_random_data(size_t length, bool allow_zero) {
    Bytes data;
    data.reserve(length);
    
    for (size_t i = 0; i < length; ++i) {
        uint8_t byte_val;
        do {
            byte_val = byte_dist_(rng_);
        } while (!allow_zero && byte_val == 0);
        
        data.push_back(byte_val);
    }
    
    return data;
}

// TestUtils implementation

bool TestUtils::bytes_equal(const Bytes& a, const Bytes& b) {
    return a == b;
}

std::string TestUtils::create_temp_dir(const std::string& prefix) {
    std::string base_path = "/tmp/";
    std::string dir_name = prefix + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    std::string full_path = base_path + dir_name;
    
    if (utils::create_dir(full_path)) {
        return full_path;
    }
    
    return "";
}

bool TestUtils::remove_dir(const std::string& dir_path) {
    return utils::remove_directory(dir_path);
}

size_t TestUtils::get_dir_size(const std::string& dir_path) {
    return utils::dir_size(dir_path);
}

uint32_t TestUtils::checksum(const Bytes& data) {
#ifdef USE_ZLIB_CRC32
    return crc32(0L, data.data(), data.size());
#else
    return crc32c::Crc32c(data.data(), data.size());
#endif
}

std::string TestUtils::format_bytes(size_t bytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unit_index = 0;
    double size = static_cast<double>(bytes);
    
    while (size >= 1024.0 && unit_index < 4) {
        size /= 1024.0;
        unit_index++;
    }
    
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2) << size << " " << units[unit_index];
    return ss.str();
}

// Timer implementation

TestUtils::Timer::Timer() : started_(false), stopped_(false) {}

void TestUtils::Timer::start() {
    start_time_ = std::chrono::high_resolution_clock::now();
    started_ = true;
    stopped_ = false;
}

void TestUtils::Timer::stop() {
    if (started_) {
        end_time_ = std::chrono::high_resolution_clock::now();
        stopped_ = true;
    }
}

double TestUtils::Timer::elapsed_ms() const {
    if (!started_) return 0.0;
    
    auto end = stopped_ ? end_time_ : std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start_time_);
    return duration.count() / 1000.0;
}

double TestUtils::Timer::elapsed_us() const {
    if (!started_) return 0.0;
    
    auto end = stopped_ ? end_time_ : std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start_time_);
    return static_cast<double>(duration.count());
}

} // namespace bitcask
