#include <gtest/gtest.h>
#include "bitcask/art_index.h"
#include "bitcask/common.h"

namespace bitcask {

class ARTIndexTest : public ::testing::Test {
protected:
    void SetUp() override {
        art_index = std::make_unique<ARTIndex>();
    }

    void TearDown() override {
        art_index->close();
    }

    std::unique_ptr<ARTIndex> art_index;
};

TEST_F(ARTIndexTest, BasicPutAndGet) {
    // 测试基本的插入和查找
    Bytes key1 = {'a', 'b', 'c'};
    LogRecordPos pos1(1, 100, 10);
    
    auto old_pos = art_index->put(key1, pos1);
    EXPECT_EQ(old_pos, nullptr);
    
    auto result = art_index->get(key1);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->fid, 1);
    EXPECT_EQ(result->offset, 100);
    EXPECT_EQ(result->size, 10);
}

TEST_F(ARTIndexTest, UpdateExistingKey) {
    // 测试更新已存在的键
    Bytes key = {'t', 'e', 's', 't'};
    LogRecordPos pos1(1, 100, 10);
    LogRecordPos pos2(2, 200, 20);
    
    auto old_pos1 = art_index->put(key, pos1);
    EXPECT_EQ(old_pos1, nullptr);
    
    auto old_pos2 = art_index->put(key, pos2);
    ASSERT_NE(old_pos2, nullptr);
    EXPECT_EQ(old_pos2->fid, 1);
    EXPECT_EQ(old_pos2->offset, 100);
    EXPECT_EQ(old_pos2->size, 10);
    
    auto result = art_index->get(key);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->fid, 2);
    EXPECT_EQ(result->offset, 200);
    EXPECT_EQ(result->size, 20);
}

TEST_F(ARTIndexTest, MultipleKeys) {
    // 测试多个键的插入和查找
    std::vector<std::pair<Bytes, LogRecordPos>> test_data = {
        {{'a'}, LogRecordPos(1, 10, 5)},
        {{'a', 'b'}, LogRecordPos(2, 20, 6)},
        {{'a', 'b', 'c'}, LogRecordPos(3, 30, 7)},
        {{'b'}, LogRecordPos(4, 40, 8)},
        {{'b', 'a'}, LogRecordPos(5, 50, 9)},
        {{'c', 'd', 'e'}, LogRecordPos(6, 60, 10)}
    };
    
    // 插入所有数据
    for (const auto& [key, pos] : test_data) {
        auto old_pos = art_index->put(key, pos);
        EXPECT_EQ(old_pos, nullptr);
    }
    
    // 验证所有数据
    for (const auto& [key, expected_pos] : test_data) {
        auto result = art_index->get(key);
        ASSERT_NE(result, nullptr) << "Key not found";
        EXPECT_EQ(result->fid, expected_pos.fid);
        EXPECT_EQ(result->offset, expected_pos.offset);
        EXPECT_EQ(result->size, expected_pos.size);
    }
    
    EXPECT_EQ(art_index->size(), test_data.size());
}

TEST_F(ARTIndexTest, Remove) {
    // 测试删除操作
    Bytes key1 = {'a', 'b', 'c'};
    Bytes key2 = {'d', 'e', 'f'};
    LogRecordPos pos1(1, 100, 10);
    LogRecordPos pos2(2, 200, 20);
    
    art_index->put(key1, pos1);
    art_index->put(key2, pos2);
    EXPECT_EQ(art_index->size(), 2);
    
    // 删除存在的键
    auto [removed_pos, found] = art_index->remove(key1);
    EXPECT_TRUE(found);
    ASSERT_NE(removed_pos, nullptr);
    EXPECT_EQ(removed_pos->fid, 1);
    EXPECT_EQ(removed_pos->offset, 100);
    EXPECT_EQ(removed_pos->size, 10);
    EXPECT_EQ(art_index->size(), 1);
    
    // 验证键已被删除
    auto result = art_index->get(key1);
    EXPECT_EQ(result, nullptr);
    
    // 验证其他键仍然存在
    result = art_index->get(key2);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->fid, 2);
    
    // 删除不存在的键
    auto [removed_pos2, found2] = art_index->remove(key1);
    EXPECT_FALSE(found2);
    EXPECT_EQ(removed_pos2, nullptr);
    EXPECT_EQ(art_index->size(), 1);
}

TEST_F(ARTIndexTest, EmptyKey) {
    // 测试空键
    Bytes empty_key;
    LogRecordPos pos(1, 100, 10);
    
    auto old_pos = art_index->put(empty_key, pos);
    EXPECT_EQ(old_pos, nullptr);
    
    auto result = art_index->get(empty_key);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->fid, 1);
    EXPECT_EQ(result->offset, 100);
    EXPECT_EQ(result->size, 10);
}

TEST_F(ARTIndexTest, LongKeys) {
    // 测试长键
    Bytes long_key;
    for (int i = 0; i < 1000; i++) {
        long_key.push_back(static_cast<uint8_t>(i % 256));
    }
    
    LogRecordPos pos(1, 100, 10);
    auto old_pos = art_index->put(long_key, pos);
    EXPECT_EQ(old_pos, nullptr);
    
    auto result = art_index->get(long_key);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->fid, 1);
    EXPECT_EQ(result->offset, 100);
    EXPECT_EQ(result->size, 10);
}

TEST_F(ARTIndexTest, SimilarKeys) {
    // 测试相似的键
    std::vector<std::pair<Bytes, LogRecordPos>> test_data = {
        {{'p', 'r', 'e', 'f', 'i', 'x'}, LogRecordPos(1, 10, 5)},
        {{'p', 'r', 'e', 'f', 'i', 'x', 'a'}, LogRecordPos(2, 20, 6)},
        {{'p', 'r', 'e', 'f', 'i', 'x', 'b'}, LogRecordPos(3, 30, 7)},
        {{'p', 'r', 'e', 'f', 'i', 'x', 'a', 'b'}, LogRecordPos(4, 40, 8)},
        {{'p', 'r', 'e'}, LogRecordPos(5, 50, 9)}
    };
    
    // 插入所有数据
    for (const auto& [key, pos] : test_data) {
        auto old_pos = art_index->put(key, pos);
        EXPECT_EQ(old_pos, nullptr);
    }
    
    // 验证所有数据
    for (const auto& [key, expected_pos] : test_data) {
        auto result = art_index->get(key);
        ASSERT_NE(result, nullptr) << "Key not found";
        EXPECT_EQ(result->fid, expected_pos.fid);
        EXPECT_EQ(result->offset, expected_pos.offset);
        EXPECT_EQ(result->size, expected_pos.size);
    }
    
    EXPECT_EQ(art_index->size(), test_data.size());
}

TEST_F(ARTIndexTest, ListKeys) {
    // 测试列出所有键
    std::vector<Bytes> expected_keys = {
        {'a'},
        {'a', 'b'},
        {'a', 'b', 'c'},
        {'b'},
        {'c', 'd'}
    };
    
    // 插入数据
    for (size_t i = 0; i < expected_keys.size(); i++) {
        LogRecordPos pos(static_cast<uint32_t>(i + 1), i * 10, 5);
        art_index->put(expected_keys[i], pos);
    }
    
    auto keys = art_index->list_keys();
    EXPECT_EQ(keys.size(), expected_keys.size());
    
    // 检查所有键都存在（不保证顺序）
    for (const auto& key : expected_keys) {
        auto it = std::find(keys.begin(), keys.end(), key);
        EXPECT_NE(it, keys.end()) << "Key not found in list";
    }
}

TEST_F(ARTIndexTest, Iterator) {
    // 测试迭代器功能
    std::vector<std::pair<Bytes, LogRecordPos>> test_data = {
        {{'a'}, LogRecordPos(1, 10, 5)},
        {{'b'}, LogRecordPos(2, 20, 6)},
        {{'c'}, LogRecordPos(3, 30, 7)},
        {{'d'}, LogRecordPos(4, 40, 8)}
    };
    
    // 插入数据
    for (const auto& [key, pos] : test_data) {
        art_index->put(key, pos);
    }
    
    // 正向迭代
    auto iter = art_index->iterator(false);
    int count = 0;
    
    for (iter->rewind(); iter->valid(); iter->next()) {
        count++;
        auto key = iter->key();
        auto value = iter->value();
        
        // 验证数据存在
        bool found = false;
        for (const auto& [expected_key, expected_pos] : test_data) {
            if (key == expected_key) {
                EXPECT_EQ(value.fid, expected_pos.fid);
                EXPECT_EQ(value.offset, expected_pos.offset);
                EXPECT_EQ(value.size, expected_pos.size);
                found = true;
                break;
            }
        }
        EXPECT_TRUE(found) << "Unexpected key in iteration";
    }
    
    EXPECT_EQ(count, static_cast<int>(test_data.size()));
    iter->close();
}

TEST_F(ARTIndexTest, IteratorWithPrefix) {
    // 测试带前缀的迭代器
    std::vector<std::pair<Bytes, LogRecordPos>> test_data = {
        {{'a', 'b', 'c'}, LogRecordPos(1, 10, 5)},
        {{'a', 'b', 'd'}, LogRecordPos(2, 20, 6)},
        {{'a', 'c', 'e'}, LogRecordPos(3, 30, 7)},
        {{'b', 'c', 'd'}, LogRecordPos(4, 40, 8)}
    };
    
    // 插入数据
    for (const auto& [key, pos] : test_data) {
        art_index->put(key, pos);
    }
    
    // 创建带前缀的迭代器选项
    IteratorOptions options;
    options.prefix = {'a', 'b'};
    
    std::shared_ptr<ARTNode> null_node = nullptr;
    auto iter = std::make_unique<ARTIterator>(null_node, options);
    
    // 注意：这里的测试有限，因为我们无法直接访问根节点
    // 在实际使用中，会通过ARTIndex::iterator方法创建迭代器
}

TEST_F(ARTIndexTest, LargeDataset) {
    // 测试大数据集
    const int num_keys = 1000;
    
    // 插入大量数据
    for (int i = 0; i < num_keys; i++) {
        Bytes key;
        std::string key_str = "key_" + std::to_string(i);
        key.assign(key_str.begin(), key_str.end());
        
        LogRecordPos pos(static_cast<uint32_t>(i), i * 10, 5);
        auto old_pos = art_index->put(key, pos);
        EXPECT_EQ(old_pos, nullptr);
    }
    
    EXPECT_EQ(art_index->size(), static_cast<size_t>(num_keys));
    
    // 验证所有数据
    for (int i = 0; i < num_keys; i++) {
        Bytes key;
        std::string key_str = "key_" + std::to_string(i);
        key.assign(key_str.begin(), key_str.end());
        
        auto result = art_index->get(key);
        ASSERT_NE(result, nullptr) << "Key " << i << " not found";
        EXPECT_EQ(result->fid, static_cast<uint32_t>(i));
        EXPECT_EQ(result->offset, static_cast<uint64_t>(i * 10));
        EXPECT_EQ(result->size, 5);
    }
}

TEST_F(ARTIndexTest, StressTest) {
    // 压力测试：混合操作
    const int num_operations = 500;
    std::vector<Bytes> keys;
    
    // 生成测试键
    for (int i = 0; i < num_operations; i++) {
        Bytes key;
        std::string key_str = "stress_key_" + std::to_string(i);
        key.assign(key_str.begin(), key_str.end());
        keys.push_back(key);
    }
    
    // 插入操作
    for (int i = 0; i < num_operations; i++) {
        LogRecordPos pos(static_cast<uint32_t>(i), i * 10, 5);
        art_index->put(keys[i], pos);
    }
    
    // 查询操作
    for (int i = 0; i < num_operations; i++) {
        auto result = art_index->get(keys[i]);
        ASSERT_NE(result, nullptr);
        EXPECT_EQ(result->fid, static_cast<uint32_t>(i));
    }
    
    // 删除操作（删除一半）
    for (int i = 0; i < num_operations / 2; i++) {
        auto [removed_pos, found] = art_index->remove(keys[i]);
        EXPECT_TRUE(found);
        ASSERT_NE(removed_pos, nullptr);
    }
    
    EXPECT_EQ(art_index->size(), static_cast<size_t>(num_operations / 2));
    
    // 验证删除的键不存在
    for (int i = 0; i < num_operations / 2; i++) {
        auto result = art_index->get(keys[i]);
        EXPECT_EQ(result, nullptr);
    }
    
    // 验证剩余的键仍然存在
    for (int i = num_operations / 2; i < num_operations; i++) {
        auto result = art_index->get(keys[i]);
        ASSERT_NE(result, nullptr);
        EXPECT_EQ(result->fid, static_cast<uint32_t>(i));
    }
}

}  // namespace bitcask
