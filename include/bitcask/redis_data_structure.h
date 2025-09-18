#pragma once

#include "db.h"
#include "common.h"
#include <memory>
#include <string>
#include <vector>
#include <chrono>
#include <cstdint>

namespace bitcask {
namespace redis {

// Redis数据类型
enum class RedisDataType : uint8_t {
    STRING = 0,
    HASH = 1,
    SET = 2,
    LIST = 3,
    ZSET = 4
};

// Redis元数据结构
struct RedisMetadata {
    RedisDataType data_type;
    uint64_t expire;        // 过期时间戳（纳秒）
    uint64_t version;       // 版本号
    uint32_t size;          // 数据量
    uint64_t head;          // List专用：头部标记
    uint64_t tail;          // List专用：尾部标记
    
    RedisMetadata() : data_type(RedisDataType::STRING), expire(0), version(0), 
                     size(0), head(0), tail(0) {}
    
    // 编码元数据
    Bytes encode() const;
    
    // 解码元数据
    static RedisMetadata decode(const Bytes& data);
};

// Hash内部key结构
struct HashInternalKey {
    Bytes key;
    uint64_t version;
    Bytes field;
    
    Bytes encode() const;
};

// Set内部key结构
struct SetInternalKey {
    Bytes key;
    uint64_t version;
    Bytes member;
    
    Bytes encode() const;
};

// List内部key结构
struct ListInternalKey {
    Bytes key;
    uint64_t version;
    uint64_t index;
    
    Bytes encode() const;
};

// ZSet内部key结构
struct ZSetInternalKey {
    Bytes key;
    uint64_t version;
    double score;
    Bytes member;
    
    Bytes encode() const;
};

// Redis数据结构服务
class RedisDataStructure {
public:
    explicit RedisDataStructure(std::shared_ptr<DB> db);
    ~RedisDataStructure() = default;

    // =============== String 数据结构 ===============
    
    // 设置字符串值
    void set(const std::string& key, const std::string& value, std::chrono::milliseconds ttl = std::chrono::milliseconds(0));
    
    // 获取字符串值
    std::string get(const std::string& key);
    
    // =============== Hash 数据结构 ===============
    
    // 设置Hash字段
    bool hset(const std::string& key, const std::string& field, const std::string& value);
    
    // 获取Hash字段
    std::string hget(const std::string& key, const std::string& field);
    
    // 删除Hash字段
    bool hdel(const std::string& key, const std::string& field);
    
    // =============== Set 数据结构 ===============
    
    // 添加Set成员
    bool sadd(const std::string& key, const std::string& member);
    
    // 检查Set成员是否存在
    bool sismember(const std::string& key, const std::string& member);
    
    // 删除Set成员
    bool srem(const std::string& key, const std::string& member);
    
    // =============== List 数据结构 ===============
    
    // 从左侧推入List元素
    uint32_t lpush(const std::string& key, const std::string& element);
    
    // 从右侧推入List元素
    uint32_t rpush(const std::string& key, const std::string& element);
    
    // 从左侧弹出List元素
    std::string lpop(const std::string& key);
    
    // 从右侧弹出List元素
    std::string rpop(const std::string& key);
    
    // =============== ZSet 数据结构 ===============
    
    // 添加ZSet成员
    bool zadd(const std::string& key, double score, const std::string& member);
    
    // 获取ZSet成员分数
    double zscore(const std::string& key, const std::string& member);
    
    // =============== 通用操作 ===============
    
    // 删除key
    bool del(const std::string& key);
    
    // 检查key是否存在
    bool exists(const std::string& key);
    
    // 获取key的类型
    RedisDataType type(const std::string& key);
    
    // 关闭数据库
    void close();

private:
    // 查找或创建元数据
    RedisMetadata find_metadata(const std::string& key, RedisDataType data_type);
    
    // 检查是否过期
    bool is_expired(uint64_t expire_time) const;
    
    // 获取当前时间戳（纳秒）
    uint64_t current_timestamp() const;
    
    // 字符串转Bytes
    Bytes string_to_bytes(const std::string& str) const;
    
    // Bytes转字符串
    std::string bytes_to_string(const Bytes& bytes) const;

private:
    std::shared_ptr<DB> db_;
    static const uint64_t INITIAL_LIST_MARK;
};

}  // namespace redis
}  // namespace bitcask
