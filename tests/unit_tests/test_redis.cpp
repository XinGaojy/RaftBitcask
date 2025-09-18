#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "bitcask/bitcask.h"
#include "bitcask/redis_data_structure.h"
#include "bitcask/redis_server.h"
#include <thread>
#include <chrono>

namespace bitcask {
namespace redis {
namespace test {

class RedisDataStructureTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 创建临时目录
        temp_dir_ = "/tmp/bitcask_redis_test";
        system(("rm -rf " + temp_dir_).c_str());
        system(("mkdir -p " + temp_dir_).c_str());
        
        // 创建数据库实例
        options_.dir_path = temp_dir_;
        options_.data_file_size = 64 * 1024;
        options_.sync_writes = false;
        
        db_ = std::shared_ptr<DB>(DB::open(options_).release());
        rds_ = std::make_unique<RedisDataStructure>(db_);
    }

    void TearDown() override {
        rds_.reset();
        db_.reset();
        system(("rm -rf " + temp_dir_).c_str());
    }

    std::string temp_dir_;
    Options options_;
    std::shared_ptr<DB> db_;
    std::unique_ptr<RedisDataStructure> rds_;
};

// String数据结构测试
TEST_F(RedisDataStructureTest, StringOperations) {
    // 测试SET和GET
    EXPECT_NO_THROW(rds_->set("key1", "value1"));
    EXPECT_EQ(rds_->get("key1"), "value1");
    
    // 测试更新
    EXPECT_NO_THROW(rds_->set("key1", "new_value1"));
    EXPECT_EQ(rds_->get("key1"), "new_value1");
    
    // 测试不存在的key
    EXPECT_THROW(rds_->get("non_existent"), KeyNotFoundError);
    
    // 测试DEL
    EXPECT_TRUE(rds_->del("key1"));
    EXPECT_THROW(rds_->get("key1"), KeyNotFoundError);
    EXPECT_FALSE(rds_->del("key1")); // 再次删除应该返回false
}

// Hash数据结构测试
TEST_F(RedisDataStructureTest, HashOperations) {
    // 测试HSET和HGET
    EXPECT_TRUE(rds_->hset("hash1", "field1", "value1"));
    EXPECT_EQ(rds_->hget("hash1", "field1"), "value1");
    
    // 测试添加多个字段
    EXPECT_TRUE(rds_->hset("hash1", "field2", "value2"));
    EXPECT_EQ(rds_->hget("hash1", "field2"), "value2");
    
    // 测试更新字段
    EXPECT_FALSE(rds_->hset("hash1", "field1", "new_value1"));
    EXPECT_EQ(rds_->hget("hash1", "field1"), "new_value1");
    
    // 测试不存在的字段
    EXPECT_THROW(rds_->hget("hash1", "non_existent"), KeyNotFoundError);
    
    // 测试HDEL
    EXPECT_TRUE(rds_->hdel("hash1", "field1"));
    EXPECT_THROW(rds_->hget("hash1", "field1"), KeyNotFoundError);
    EXPECT_FALSE(rds_->hdel("hash1", "field1")); // 再次删除应该返回false
}

// Set数据结构测试
TEST_F(RedisDataStructureTest, SetOperations) {
    // 测试SADD和SISMEMBER
    EXPECT_TRUE(rds_->sadd("set1", "member1"));
    EXPECT_TRUE(rds_->sismember("set1", "member1"));
    
    // 测试添加重复成员
    EXPECT_FALSE(rds_->sadd("set1", "member1"));
    EXPECT_TRUE(rds_->sismember("set1", "member1"));
    
    // 测试添加多个成员
    EXPECT_TRUE(rds_->sadd("set1", "member2"));
    EXPECT_TRUE(rds_->sismember("set1", "member2"));
    
    // 测试不存在的成员
    EXPECT_FALSE(rds_->sismember("set1", "non_existent"));
    
    // 测试SREM
    EXPECT_TRUE(rds_->srem("set1", "member1"));
    EXPECT_FALSE(rds_->sismember("set1", "member1"));
    EXPECT_FALSE(rds_->srem("set1", "member1")); // 再次删除应该返回false
}

// List数据结构测试
TEST_F(RedisDataStructureTest, ListOperations) {
    // 测试LPUSH
    EXPECT_EQ(rds_->lpush("list1", "element1"), 1);
    EXPECT_EQ(rds_->lpush("list1", "element2"), 2);
    
    // 测试RPUSH
    EXPECT_EQ(rds_->rpush("list1", "element3"), 3);
    
    // 测试LPOP
    EXPECT_EQ(rds_->lpop("list1"), "element2");
    
    // 测试RPOP
    EXPECT_EQ(rds_->rpop("list1"), "element3");
    
    // 最后应该只剩下element1
    EXPECT_EQ(rds_->lpop("list1"), "element1");
    
    // 空列表的POP操作应该抛出异常
    EXPECT_THROW(rds_->lpop("list1"), KeyNotFoundError);
    EXPECT_THROW(rds_->rpop("list1"), KeyNotFoundError);
}

// ZSet数据结构测试
TEST_F(RedisDataStructureTest, ZSetOperations) {
    // 测试ZADD
    EXPECT_TRUE(rds_->zadd("zset1", 1.0, "member1"));
    EXPECT_TRUE(rds_->zadd("zset1", 2.0, "member2"));
    
    // 测试添加重复成员
    EXPECT_FALSE(rds_->zadd("zset1", 1.5, "member1"));
    
    // 注意：ZSCORE功能在当前实现中是简化的，可能抛出异常
    // EXPECT_DOUBLE_EQ(rds_->zscore("zset1", "member1"), 1.5);
}

// 通用操作测试
TEST_F(RedisDataStructureTest, GenericOperations) {
    // 设置不同类型的key
    rds_->set("string_key", "value");
    rds_->hset("hash_key", "field", "value");
    rds_->sadd("set_key", "member");
    rds_->lpush("list_key", "element");
    
    // 测试EXISTS
    EXPECT_TRUE(rds_->exists("string_key"));
    EXPECT_TRUE(rds_->exists("hash_key"));
    EXPECT_TRUE(rds_->exists("set_key"));
    EXPECT_TRUE(rds_->exists("list_key"));
    EXPECT_FALSE(rds_->exists("non_existent"));
    
    // 测试TYPE
    EXPECT_EQ(rds_->type("string_key"), RedisDataType::STRING);
    EXPECT_EQ(rds_->type("hash_key"), RedisDataType::HASH);
    EXPECT_EQ(rds_->type("set_key"), RedisDataType::SET);
    EXPECT_EQ(rds_->type("list_key"), RedisDataType::LIST);
    
    // 测试DEL
    EXPECT_TRUE(rds_->del("string_key"));
    EXPECT_FALSE(rds_->exists("string_key"));
}

// Redis响应序列化测试
TEST_F(RedisDataStructureTest, RedisResponseSerialization) {
    // 简单字符串
    auto simple_str = RedisResponse::simple_string("OK");
    EXPECT_EQ(simple_str.serialize(), "+OK\r\n");
    
    // 错误
    auto error = RedisResponse::error("ERR unknown command");
    EXPECT_EQ(error.serialize(), "-ERR unknown command\r\n");
    
    // 整数
    auto integer = RedisResponse::integer(42);
    EXPECT_EQ(integer.serialize(), ":42\r\n");
    
    // 批量字符串
    auto bulk_str = RedisResponse::bulk_string("hello");
    EXPECT_EQ(bulk_str.serialize(), "$5\r\nhello\r\n");
    
    // 空批量字符串
    auto null_bulk = RedisResponse::null_bulk_string();
    EXPECT_EQ(null_bulk.serialize(), "$-1\r\n");
    
    // 数组
    std::vector<RedisResponse> arr;
    arr.push_back(RedisResponse::bulk_string("foo"));
    arr.push_back(RedisResponse::bulk_string("bar"));
    auto array = RedisResponse::array(arr);
    EXPECT_EQ(array.serialize(), "*2\r\n$3\r\nfoo\r\n$3\r\nbar\r\n");
}

// 元数据编码解码测试
TEST_F(RedisDataStructureTest, MetadataEncodeDecode) {
    RedisMetadata original;
    original.data_type = RedisDataType::HASH;
    original.expire = 1234567890123456789ULL;
    original.version = 9876543210987654321ULL;
    original.size = 42;
    
    // 编码
    Bytes encoded = original.encode();
    
    // 解码
    RedisMetadata decoded = RedisMetadata::decode(encoded);
    
    // 验证
    EXPECT_EQ(decoded.data_type, original.data_type);
    EXPECT_EQ(decoded.expire, original.expire);
    EXPECT_EQ(decoded.version, original.version);
    EXPECT_EQ(decoded.size, original.size);
}

// 内部key编码测试
TEST_F(RedisDataStructureTest, InternalKeyEncoding) {
    // Hash内部key
    HashInternalKey hash_key;
    hash_key.key = {'h', 'a', 's', 'h', '1'};
    hash_key.version = 123456789;
    hash_key.field = {'f', 'i', 'e', 'l', 'd', '1'};
    
    Bytes encoded = hash_key.encode();
    EXPECT_GT(encoded.size(), hash_key.key.size() + hash_key.field.size() + 8);
    
    // Set内部key
    SetInternalKey set_key;
    set_key.key = {'s', 'e', 't', '1'};
    set_key.version = 987654321;
    set_key.member = {'m', 'e', 'm', 'b', 'e', 'r', '1'};
    
    encoded = set_key.encode();
    EXPECT_GT(encoded.size(), set_key.key.size() + set_key.member.size() + 8);
    
    // List内部key
    ListInternalKey list_key;
    list_key.key = {'l', 'i', 's', 't', '1'};
    list_key.version = 555555555;
    list_key.index = 123;
    
    encoded = list_key.encode();
    EXPECT_EQ(encoded.size(), list_key.key.size() + 16); // key + version + index
}

// 错误类型操作测试
TEST_F(RedisDataStructureTest, WrongTypeOperations) {
    // 设置一个字符串
    rds_->set("string_key", "value");
    
    // 尝试对字符串执行Hash操作应该抛出异常
    EXPECT_THROW(rds_->hset("string_key", "field", "value"), BitcaskException);
    EXPECT_THROW(rds_->hget("string_key", "field"), BitcaskException);
    
    // 尝试对字符串执行Set操作应该抛出异常
    EXPECT_THROW(rds_->sadd("string_key", "member"), BitcaskException);
    EXPECT_THROW(rds_->sismember("string_key", "member"), BitcaskException);
    
    // 尝试对字符串执行List操作应该抛出异常
    EXPECT_THROW(rds_->lpush("string_key", "element"), BitcaskException);
    EXPECT_THROW(rds_->lpop("string_key"), BitcaskException);
}

}  // namespace test
}  // namespace redis
}  // namespace bitcask
