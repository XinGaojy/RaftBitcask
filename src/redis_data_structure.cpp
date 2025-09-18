#include "bitcask/redis_data_structure.h"
#include <cstring>
#include <algorithm>
#include <sstream>

namespace bitcask {
namespace redis {

const uint64_t RedisDataStructure::INITIAL_LIST_MARK = UINT64_MAX / 2;

// RedisMetadata 实现
Bytes RedisMetadata::encode() const {
    Bytes result;
    
    // data_type (1 byte)
    result.push_back(static_cast<uint8_t>(data_type));
    
    // expire (8 bytes, little-endian)
    for (int i = 0; i < 8; ++i) {
        result.push_back(static_cast<uint8_t>((expire >> (i * 8)) & 0xFF));
    }
    
    // version (8 bytes, little-endian)
    for (int i = 0; i < 8; ++i) {
        result.push_back(static_cast<uint8_t>((version >> (i * 8)) & 0xFF));
    }
    
    // size (4 bytes, little-endian)
    for (int i = 0; i < 4; ++i) {
        result.push_back(static_cast<uint8_t>((size >> (i * 8)) & 0xFF));
    }
    
    // head and tail for List (16 bytes)
    if (data_type == RedisDataType::LIST) {
        for (int i = 0; i < 8; ++i) {
            result.push_back(static_cast<uint8_t>((head >> (i * 8)) & 0xFF));
        }
        for (int i = 0; i < 8; ++i) {
            result.push_back(static_cast<uint8_t>((tail >> (i * 8)) & 0xFF));
        }
    }
    
    return result;
}

RedisMetadata RedisMetadata::decode(const Bytes& data) {
    if (data.size() < 21) {  // Minimum size: 1 + 8 + 8 + 4
        throw BitcaskException("Invalid metadata size");
    }
    
    RedisMetadata metadata;
    size_t offset = 0;
    
    // data_type
    metadata.data_type = static_cast<RedisDataType>(data[offset++]);
    
    // expire
    metadata.expire = 0;
    for (int i = 0; i < 8; ++i) {
        metadata.expire |= (static_cast<uint64_t>(data[offset++]) << (i * 8));
    }
    
    // version
    metadata.version = 0;
    for (int i = 0; i < 8; ++i) {
        metadata.version |= (static_cast<uint64_t>(data[offset++]) << (i * 8));
    }
    
    // size
    metadata.size = 0;
    for (int i = 0; i < 4; ++i) {
        metadata.size |= (static_cast<uint32_t>(data[offset++]) << (i * 8));
    }
    
    // head and tail for List
    if (metadata.data_type == RedisDataType::LIST && data.size() >= 37) {
        metadata.head = 0;
        for (int i = 0; i < 8; ++i) {
            metadata.head |= (static_cast<uint64_t>(data[offset++]) << (i * 8));
        }
        metadata.tail = 0;
        for (int i = 0; i < 8; ++i) {
            metadata.tail |= (static_cast<uint64_t>(data[offset++]) << (i * 8));
        }
    }
    
    return metadata;
}

// HashInternalKey 实现
Bytes HashInternalKey::encode() const {
    Bytes result = key;
    
    // version (8 bytes, little-endian)
    for (int i = 0; i < 8; ++i) {
        result.push_back(static_cast<uint8_t>((version >> (i * 8)) & 0xFF));
    }
    
    // field
    result.insert(result.end(), field.begin(), field.end());
    
    return result;
}

// SetInternalKey 实现
Bytes SetInternalKey::encode() const {
    Bytes result = key;
    
    // version (8 bytes, little-endian)
    for (int i = 0; i < 8; ++i) {
        result.push_back(static_cast<uint8_t>((version >> (i * 8)) & 0xFF));
    }
    
    // member
    result.insert(result.end(), member.begin(), member.end());
    
    return result;
}

// ListInternalKey 实现
Bytes ListInternalKey::encode() const {
    Bytes result = key;
    
    // version (8 bytes, little-endian)
    for (int i = 0; i < 8; ++i) {
        result.push_back(static_cast<uint8_t>((version >> (i * 8)) & 0xFF));
    }
    
    // index (8 bytes, little-endian)
    for (int i = 0; i < 8; ++i) {
        result.push_back(static_cast<uint8_t>((index >> (i * 8)) & 0xFF));
    }
    
    return result;
}

// ZSetInternalKey 实现
Bytes ZSetInternalKey::encode() const {
    Bytes result = key;
    
    // version (8 bytes, little-endian)
    for (int i = 0; i < 8; ++i) {
        result.push_back(static_cast<uint8_t>((version >> (i * 8)) & 0xFF));
    }
    
    // score (8 bytes, as uint64_t representation of double)
    uint64_t score_bits;
    memcpy(&score_bits, &score, sizeof(double));
    for (int i = 0; i < 8; ++i) {
        result.push_back(static_cast<uint8_t>((score_bits >> (i * 8)) & 0xFF));
    }
    
    // member
    result.insert(result.end(), member.begin(), member.end());
    
    return result;
}

// RedisDataStructure 实现
RedisDataStructure::RedisDataStructure(std::shared_ptr<DB> db) : db_(db) {}

void RedisDataStructure::set(const std::string& key, const std::string& value, std::chrono::milliseconds ttl) {
    if (value.empty()) {
        return;
    }
    
    Bytes encoded_value;
    
    // 编码：type + expire + payload
    encoded_value.push_back(static_cast<uint8_t>(RedisDataType::STRING));
    
    // 过期时间
    uint64_t expire = 0;
    if (ttl.count() > 0) {
        expire = current_timestamp() + static_cast<uint64_t>(ttl.count()) * 1000000; // 转换为纳秒
    }
    
    for (int i = 0; i < 8; ++i) {
        encoded_value.push_back(static_cast<uint8_t>((expire >> (i * 8)) & 0xFF));
    }
    
    // 实际值
    Bytes value_bytes = string_to_bytes(value);
    encoded_value.insert(encoded_value.end(), value_bytes.begin(), value_bytes.end());
    
    // 存储
    db_->put(string_to_bytes(key), encoded_value);
}

std::string RedisDataStructure::get(const std::string& key) {
    try {
        Bytes encoded_value = db_->get(string_to_bytes(key));
        
        if (encoded_value.size() < 9) {
            throw KeyNotFoundError();
        }
        
        // 检查数据类型
        RedisDataType data_type = static_cast<RedisDataType>(encoded_value[0]);
        if (data_type != RedisDataType::STRING) {
            throw BitcaskException("Wrong type operation");
        }
        
        // 检查过期时间
        uint64_t expire = 0;
        for (int i = 0; i < 8; ++i) {
            expire |= (static_cast<uint64_t>(encoded_value[1 + i]) << (i * 8));
        }
        
        if (is_expired(expire)) {
            throw KeyNotFoundError();
        }
        
        // 提取实际值
        Bytes value_bytes(encoded_value.begin() + 9, encoded_value.end());
        return bytes_to_string(value_bytes);
        
    } catch (const KeyNotFoundError&) {
        throw;
    }
}

bool RedisDataStructure::hset(const std::string& key, const std::string& field, const std::string& value) {
    // 查找或创建元数据
    RedisMetadata metadata = find_metadata(key, RedisDataType::HASH);
    
    // 构造内部key
    HashInternalKey internal_key;
    internal_key.key = string_to_bytes(key);
    internal_key.version = metadata.version;
    internal_key.field = string_to_bytes(field);
    
    Bytes internal_key_bytes = internal_key.encode();
    
    // 检查字段是否已存在
    bool field_exists = false;
    try {
        db_->get(internal_key_bytes);
        field_exists = true;
    } catch (const KeyNotFoundError&) {
        // 字段不存在，这是正常的
    }
    
    // 存储字段值
    db_->put(internal_key_bytes, string_to_bytes(value));
    
    // 如果是新字段，更新元数据
    if (!field_exists) {
        metadata.size++;
        db_->put(string_to_bytes(key), metadata.encode());
    }
    
    return !field_exists;
}

std::string RedisDataStructure::hget(const std::string& key, const std::string& field) {
    // 查找元数据
    RedisMetadata metadata = find_metadata(key, RedisDataType::HASH);
    if (metadata.size == 0) {
        throw KeyNotFoundError();
    }
    
    // 构造内部key
    HashInternalKey internal_key;
    internal_key.key = string_to_bytes(key);
    internal_key.version = metadata.version;
    internal_key.field = string_to_bytes(field);
    
    Bytes value_bytes = db_->get(internal_key.encode());
    return bytes_to_string(value_bytes);
}

bool RedisDataStructure::hdel(const std::string& key, const std::string& field) {
    // 查找元数据
    RedisMetadata metadata = find_metadata(key, RedisDataType::HASH);
    if (metadata.size == 0) {
        return false;
    }
    
    // 构造内部key
    HashInternalKey internal_key;
    internal_key.key = string_to_bytes(key);
    internal_key.version = metadata.version;
    internal_key.field = string_to_bytes(field);
    
    try {
        db_->remove(internal_key.encode());
        
        // 更新元数据
        metadata.size--;
        if (metadata.size == 0) {
            db_->remove(string_to_bytes(key));
        } else {
            db_->put(string_to_bytes(key), metadata.encode());
        }
        
        return true;
    } catch (const KeyNotFoundError&) {
        return false;
    }
}

bool RedisDataStructure::sadd(const std::string& key, const std::string& member) {
    // 查找或创建元数据
    RedisMetadata metadata = find_metadata(key, RedisDataType::SET);
    
    // 构造内部key
    SetInternalKey internal_key;
    internal_key.key = string_to_bytes(key);
    internal_key.version = metadata.version;
    internal_key.member = string_to_bytes(member);
    
    Bytes internal_key_bytes = internal_key.encode();
    
    // 检查成员是否已存在
    try {
        db_->get(internal_key_bytes);
        return false; // 成员已存在
    } catch (const KeyNotFoundError&) {
        // 成员不存在，添加它
    }
    
    // 存储成员（值为空）
    db_->put(internal_key_bytes, Bytes{});
    
    // 更新元数据
    metadata.size++;
    db_->put(string_to_bytes(key), metadata.encode());
    
    return true;
}

bool RedisDataStructure::sismember(const std::string& key, const std::string& member) {
    // 查找元数据
    RedisMetadata metadata = find_metadata(key, RedisDataType::SET);
    if (metadata.size == 0) {
        return false;
    }
    
    // 构造内部key
    SetInternalKey internal_key;
    internal_key.key = string_to_bytes(key);
    internal_key.version = metadata.version;
    internal_key.member = string_to_bytes(member);
    
    try {
        db_->get(internal_key.encode());
        return true;
    } catch (const KeyNotFoundError&) {
        return false;
    }
}

bool RedisDataStructure::srem(const std::string& key, const std::string& member) {
    // 查找元数据
    RedisMetadata metadata = find_metadata(key, RedisDataType::SET);
    if (metadata.size == 0) {
        return false;
    }
    
    // 构造内部key
    SetInternalKey internal_key;
    internal_key.key = string_to_bytes(key);
    internal_key.version = metadata.version;
    internal_key.member = string_to_bytes(member);
    
    try {
        db_->remove(internal_key.encode());
        
        // 更新元数据
        metadata.size--;
        if (metadata.size == 0) {
            db_->remove(string_to_bytes(key));
        } else {
            db_->put(string_to_bytes(key), metadata.encode());
        }
        
        return true;
    } catch (const KeyNotFoundError&) {
        return false;
    }
}

uint32_t RedisDataStructure::lpush(const std::string& key, const std::string& element) {
    // 查找或创建元数据
    RedisMetadata metadata = find_metadata(key, RedisDataType::LIST);
    
    if (metadata.size == 0) {
        metadata.head = INITIAL_LIST_MARK;
        metadata.tail = INITIAL_LIST_MARK;
    }
    
    // 构造内部key
    ListInternalKey internal_key;
    internal_key.key = string_to_bytes(key);
    internal_key.version = metadata.version;
    internal_key.index = metadata.head - 1;
    
    // 存储元素
    db_->put(internal_key.encode(), string_to_bytes(element));
    
    // 更新元数据
    metadata.head--;
    metadata.size++;
    db_->put(string_to_bytes(key), metadata.encode());
    
    return metadata.size;
}

uint32_t RedisDataStructure::rpush(const std::string& key, const std::string& element) {
    // 查找或创建元数据
    RedisMetadata metadata = find_metadata(key, RedisDataType::LIST);
    
    if (metadata.size == 0) {
        metadata.head = INITIAL_LIST_MARK;
        metadata.tail = INITIAL_LIST_MARK;
    }
    
    // 构造内部key
    ListInternalKey internal_key;
    internal_key.key = string_to_bytes(key);
    internal_key.version = metadata.version;
    internal_key.index = metadata.tail;
    
    // 存储元素
    db_->put(internal_key.encode(), string_to_bytes(element));
    
    // 更新元数据
    metadata.tail++;
    metadata.size++;
    db_->put(string_to_bytes(key), metadata.encode());
    
    return metadata.size;
}

std::string RedisDataStructure::lpop(const std::string& key) {
    // 查找元数据
    RedisMetadata metadata = find_metadata(key, RedisDataType::LIST);
    if (metadata.size == 0) {
        throw KeyNotFoundError();
    }
    
    // 构造内部key
    ListInternalKey internal_key;
    internal_key.key = string_to_bytes(key);
    internal_key.version = metadata.version;
    internal_key.index = metadata.head;
    
    Bytes value_bytes = db_->get(internal_key.encode());
    std::string result = bytes_to_string(value_bytes);
    
    // 删除元素
    db_->remove(internal_key.encode());
    
    // 更新元数据
    metadata.head++;
    metadata.size--;
    if (metadata.size == 0) {
        db_->remove(string_to_bytes(key));
    } else {
        db_->put(string_to_bytes(key), metadata.encode());
    }
    
    return result;
}

std::string RedisDataStructure::rpop(const std::string& key) {
    // 查找元数据
    RedisMetadata metadata = find_metadata(key, RedisDataType::LIST);
    if (metadata.size == 0) {
        throw KeyNotFoundError();
    }
    
    // 构造内部key
    ListInternalKey internal_key;
    internal_key.key = string_to_bytes(key);
    internal_key.version = metadata.version;
    internal_key.index = metadata.tail - 1;
    
    Bytes value_bytes = db_->get(internal_key.encode());
    std::string result = bytes_to_string(value_bytes);
    
    // 删除元素
    db_->remove(internal_key.encode());
    
    // 更新元数据
    metadata.tail--;
    metadata.size--;
    if (metadata.size == 0) {
        db_->remove(string_to_bytes(key));
    } else {
        db_->put(string_to_bytes(key), metadata.encode());
    }
    
    return result;
}

bool RedisDataStructure::zadd(const std::string& key, double score, const std::string& member) {
    // 查找或创建元数据
    RedisMetadata metadata = find_metadata(key, RedisDataType::ZSET);
    
    // 构造内部key
    ZSetInternalKey internal_key;
    internal_key.key = string_to_bytes(key);
    internal_key.version = metadata.version;
    internal_key.score = score;
    internal_key.member = string_to_bytes(member);
    
    Bytes internal_key_bytes = internal_key.encode();
    
    // 检查成员是否已存在
    bool member_exists = false;
    try {
        db_->get(internal_key_bytes);
        member_exists = true;
    } catch (const KeyNotFoundError&) {
        // 成员不存在，这是正常的
    }
    
    // 存储成员和分数
    db_->put(internal_key_bytes, Bytes{});
    
    // 如果是新成员，更新元数据
    if (!member_exists) {
        metadata.size++;
        db_->put(string_to_bytes(key), metadata.encode());
    }
    
    return !member_exists;
}

double RedisDataStructure::zscore(const std::string& key, const std::string& /*member*/) {
    // 查找元数据
    RedisMetadata metadata = find_metadata(key, RedisDataType::ZSET);
    if (metadata.size == 0) {
        throw KeyNotFoundError();
    }
    
    // 由于我们需要查找特定成员的分数，这里简化实现
    // 在实际应用中，可能需要更复杂的索引结构
    throw BitcaskException("ZSet zscore operation not fully implemented");
}

bool RedisDataStructure::del(const std::string& key) {
    try {
        db_->remove(string_to_bytes(key));
        return true;
    } catch (const KeyNotFoundError&) {
        return false;
    }
}

bool RedisDataStructure::exists(const std::string& key) {
    try {
        db_->get(string_to_bytes(key));
        return true;
    } catch (const KeyNotFoundError&) {
        return false;
    }
}

RedisDataType RedisDataStructure::type(const std::string& key) {
    try {
        Bytes encoded_value = db_->get(string_to_bytes(key));
        if (encoded_value.empty()) {
            throw KeyNotFoundError();
        }
        return static_cast<RedisDataType>(encoded_value[0]);
    } catch (const KeyNotFoundError&) {
        throw;
    }
}

void RedisDataStructure::close() {
    if (db_) {
        db_->close();
    }
}

// 私有方法实现
RedisMetadata RedisDataStructure::find_metadata(const std::string& key, RedisDataType data_type) {
    try {
        Bytes metadata_bytes = db_->get(string_to_bytes(key));
        RedisMetadata metadata = RedisMetadata::decode(metadata_bytes);
        
        // 检查数据类型
        if (metadata.data_type != data_type) {
            throw BitcaskException("Wrong type operation");
        }
        
        // 检查过期时间
        if (is_expired(metadata.expire)) {
            // 过期了，删除key并返回新的元数据
            db_->remove(string_to_bytes(key));
            RedisMetadata new_metadata;
            new_metadata.data_type = data_type;
            new_metadata.version = current_timestamp();
            if (data_type == RedisDataType::LIST) {
                new_metadata.head = INITIAL_LIST_MARK;
                new_metadata.tail = INITIAL_LIST_MARK;
            }
            return new_metadata;
        }
        
        return metadata;
    } catch (const KeyNotFoundError&) {
        // Key不存在，创建新的元数据
        RedisMetadata metadata;
        metadata.data_type = data_type;
        metadata.version = current_timestamp();
        if (data_type == RedisDataType::LIST) {
            metadata.head = INITIAL_LIST_MARK;
            metadata.tail = INITIAL_LIST_MARK;
        }
        return metadata;
    }
}

bool RedisDataStructure::is_expired(uint64_t expire_time) const {
    return expire_time > 0 && expire_time <= current_timestamp();
}

uint64_t RedisDataStructure::current_timestamp() const {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
}

Bytes RedisDataStructure::string_to_bytes(const std::string& str) const {
    return Bytes(str.begin(), str.end());
}

std::string RedisDataStructure::bytes_to_string(const Bytes& bytes) const {
    return std::string(bytes.begin(), bytes.end());
}

}  // namespace redis
}  // namespace bitcask
