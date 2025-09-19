#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "bitcask/bitcask.h"
#include "bitcask/skiplist_index.h"
#include "bitcask/bplus_tree_index.h"
#include "bitcask/art_index.h"
#include <random>
#include <algorithm>

namespace bitcask {
namespace test {

class AdvancedIndexTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 创建临时目录
        temp_dir_ = "/tmp/bitcask_advanced_index_test";
        system(("rm -rf " + temp_dir_).c_str());
        system(("mkdir -p " + temp_dir_).c_str());
    }

    void TearDown() override {
        system(("rm -rf " + temp_dir_).c_str());
    }

    Bytes string_to_bytes(const std::string& str) {
        return Bytes(str.begin(), str.end());
    }

    std::string bytes_to_string(const Bytes& bytes) {
        return std::string(bytes.begin(), bytes.end());
    }

    LogRecordPos create_test_pos(uint32_t file_id, uint64_t offset, uint32_t size) {
        LogRecordPos pos;
        pos.fid = file_id;
        pos.offset = offset;
        pos.size = size;
        return pos;
    }

    std::string temp_dir_;
};

// SkipList索引测试
class SkipListIndexTest : public AdvancedIndexTest {
protected:
    void SetUp() override {
        AdvancedIndexTest::SetUp();
        index_ = std::make_unique<SkipListIndex>();
    }

    std::unique_ptr<SkipListIndex> index_;
};

TEST_F(SkipListIndexTest, BasicOperations) {
    // 测试插入
    auto pos1 = create_test_pos(1, 100, 50);
    auto old_pos = index_->put(string_to_bytes("key1"), pos1);
    EXPECT_EQ(old_pos, nullptr);

    // 测试获取
    auto retrieved_pos = index_->get(string_to_bytes("key1"));
    ASSERT_NE(retrieved_pos, nullptr);
    EXPECT_EQ(retrieved_pos->fid, pos1.fid);
    EXPECT_EQ(retrieved_pos->offset, pos1.offset);
    EXPECT_EQ(retrieved_pos->size, pos1.size);

    // 测试更新
    auto pos2 = create_test_pos(2, 200, 60);
    old_pos = index_->put(string_to_bytes("key1"), pos2);
    ASSERT_NE(old_pos, nullptr);
    EXPECT_EQ(old_pos->fid, pos1.fid);

    // 验证更新后的值
    retrieved_pos = index_->get(string_to_bytes("key1"));
    ASSERT_NE(retrieved_pos, nullptr);
    EXPECT_EQ(retrieved_pos->fid, pos2.fid);
}

TEST_F(SkipListIndexTest, MultipleKeys) {
    // 插入多个键
    for (int i = 0; i < 100; ++i) {
        std::string key = "key" + std::to_string(i);
        auto pos = create_test_pos(1, i * 100, 50);
        index_->put(string_to_bytes(key), pos);
    }

    // 验证所有键都能正确获取
    for (int i = 0; i < 100; ++i) {
        std::string key = "key" + std::to_string(i);
        auto pos = index_->get(string_to_bytes(key));
        ASSERT_NE(pos, nullptr);
        EXPECT_EQ(pos->offset, static_cast<uint64_t>(i * 100));
    }

    // 测试列出所有键
    auto keys = index_->list_keys();
    EXPECT_EQ(keys.size(), 100);
}

TEST_F(SkipListIndexTest, RemoveOperations) {
    // 插入一些键
    for (int i = 0; i < 10; ++i) {
        std::string key = "remove_key" + std::to_string(i);
        auto pos = create_test_pos(1, i * 100, 50);
        index_->put(string_to_bytes(key), pos);
    }

    // 删除一些键
    for (int i = 0; i < 5; ++i) {
        std::string key = "remove_key" + std::to_string(i);
        auto old_pos = index_->remove(string_to_bytes(key));
        ASSERT_NE(old_pos.first, nullptr);
        EXPECT_EQ(old_pos.first->offset, static_cast<uint64_t>(i * 100));
    }

    // 验证被删除的键不存在
    for (int i = 0; i < 5; ++i) {
        std::string key = "remove_key" + std::to_string(i);
        auto pos = index_->get(string_to_bytes(key));
        EXPECT_EQ(pos, nullptr);
    }

    // 验证未被删除的键仍然存在
    for (int i = 5; i < 10; ++i) {
        std::string key = "remove_key" + std::to_string(i);
        auto pos = index_->get(string_to_bytes(key));
        ASSERT_NE(pos, nullptr);
        EXPECT_EQ(pos->offset, static_cast<uint64_t>(i * 100));
    }
}

TEST_F(SkipListIndexTest, Iterator) {
    // 插入有序数据
    std::vector<std::string> keys;
    for (int i = 0; i < 20; ++i) {
        std::string key = "iter_key" + std::to_string(i);
        keys.push_back(key);
        auto pos = create_test_pos(1, i * 100, 50);
        index_->put(string_to_bytes(key), pos);
    }

    // 测试正向迭代
    {
        IteratorOptions options;
        auto iter = index_->iterator(options.reverse);
        
        std::vector<std::string> iterated_keys;
        iter->rewind();
        while (iter->valid()) {
            iterated_keys.push_back(bytes_to_string(iter->key()));
            iter->next();
        }
        
        // 验证顺序
        EXPECT_EQ(iterated_keys.size(), keys.size());
        std::sort(keys.begin(), keys.end());
        EXPECT_EQ(iterated_keys, keys);
    }

    // 测试反向迭代
    {
        IteratorOptions options;
        options.reverse = true;
        auto iter = index_->iterator(options.reverse);
        
        std::vector<std::string> iterated_keys;
        iter->rewind();
        while (iter->valid()) {
            iterated_keys.push_back(bytes_to_string(iter->key()));
            iter->next();
        }
        
        // 验证反向顺序
        EXPECT_EQ(iterated_keys.size(), keys.size());
        std::sort(keys.rbegin(), keys.rend());
        EXPECT_EQ(iterated_keys, keys);
    }
}

// B+树索引测试
class BPlusTreeIndexTest : public AdvancedIndexTest {
protected:
    void SetUp() override {
        AdvancedIndexTest::SetUp();
        index_ = std::make_unique<BPlusTreeIndex>(temp_dir_);
    }

    void TearDown() override {
        index_.reset();  // 确保索引关闭
        AdvancedIndexTest::TearDown();
    }

    std::unique_ptr<BPlusTreeIndex> index_;
};

TEST_F(BPlusTreeIndexTest, BasicOperations) {
    // 测试插入
    auto pos1 = create_test_pos(1, 100, 50);
    auto old_pos = index_->put(string_to_bytes("bplus_key1"), pos1);
    EXPECT_EQ(old_pos, nullptr);

    // 测试获取
    auto retrieved_pos = index_->get(string_to_bytes("bplus_key1"));
    ASSERT_NE(retrieved_pos, nullptr);
    EXPECT_EQ(retrieved_pos->fid, pos1.fid);
    EXPECT_EQ(retrieved_pos->offset, pos1.offset);
    EXPECT_EQ(retrieved_pos->size, pos1.size);

    // 测试更新
    auto pos2 = create_test_pos(2, 200, 60);
    old_pos = index_->put(string_to_bytes("bplus_key1"), pos2);
    ASSERT_NE(old_pos, nullptr);
    EXPECT_EQ(old_pos->fid, pos1.fid);
}

TEST_F(BPlusTreeIndexTest, Persistence) {
    // 插入数据
    std::vector<std::pair<std::string, LogRecordPos>> test_data;
    for (int i = 0; i < 50; ++i) {
        std::string key = "persist_key" + std::to_string(i);
        auto pos = create_test_pos(1, i * 100, 50);
        test_data.push_back({key, pos});
        index_->put(string_to_bytes(key), pos);
    }

    // 同步到磁盘
    index_->sync();

    // 关闭当前索引
    index_.reset();

    // 重新打开索引
    index_ = std::make_unique<BPlusTreeIndex>(temp_dir_);

    // 验证数据持久化
    for (const auto& [key, expected_pos] : test_data) {
        auto retrieved_pos = index_->get(string_to_bytes(key));
        ASSERT_NE(retrieved_pos, nullptr);
        EXPECT_EQ(retrieved_pos->fid, expected_pos.fid);
        EXPECT_EQ(retrieved_pos->offset, expected_pos.offset);
        EXPECT_EQ(retrieved_pos->size, expected_pos.size);
    }
}

TEST_F(BPlusTreeIndexTest, LargeDataSet) {
    // 插入大量数据
    const int DATA_SIZE = 1000;
    for (int i = 0; i < DATA_SIZE; ++i) {
        std::string key = "large_key" + std::to_string(i);
        auto pos = create_test_pos(i / 100 + 1, (i % 100) * 100, 50);
        index_->put(string_to_bytes(key), pos);
    }

    // 验证所有数据
    for (int i = 0; i < DATA_SIZE; ++i) {
        std::string key = "large_key" + std::to_string(i);
        auto pos = index_->get(string_to_bytes(key));
        ASSERT_NE(pos, nullptr);
        EXPECT_EQ(pos->fid, static_cast<uint32_t>(i / 100 + 1));
        EXPECT_EQ(pos->offset, static_cast<uint64_t>((i % 100) * 100));
    }

    // 测试列出所有键
    auto keys = index_->list_keys();
    EXPECT_EQ(keys.size(), DATA_SIZE);
}

// 数据库使用高级索引的集成测试
class DatabaseAdvancedIndexTest : public AdvancedIndexTest {
protected:
    void SetUp() override {
        AdvancedIndexTest::SetUp();
        
        // 设置测试选项
        options_ = Options::default_options();
        options_.dir_path = temp_dir_;
        options_.data_file_size = 64 * 1024;
        options_.sync_writes = false;
    }

    Options options_;
};

TEST_F(DatabaseAdvancedIndexTest, SkipListIndex) {
    options_.index_type = IndexType::SKIPLIST;
    auto db = DB::open(options_);

    // 插入数据
    for (int i = 0; i < 100; ++i) {
        std::string key = "skiplist_db_key" + std::to_string(i);
        std::string value = "skiplist_db_value" + std::to_string(i);
        db->put(string_to_bytes(key), string_to_bytes(value));
    }

    // 验证数据
    for (int i = 0; i < 100; ++i) {
        std::string key = "skiplist_db_key" + std::to_string(i);
        std::string expected_value = "skiplist_db_value" + std::to_string(i);
        auto actual_value = db->get(string_to_bytes(key));
        EXPECT_EQ(bytes_to_string(actual_value), expected_value);
    }

    // 测试删除
    for (int i = 0; i < 50; ++i) {
        std::string key = "skiplist_db_key" + std::to_string(i);
        db->remove(string_to_bytes(key));
    }

    // 验证删除结果
    for (int i = 0; i < 50; ++i) {
        std::string key = "skiplist_db_key" + std::to_string(i);
        EXPECT_THROW(db->get(string_to_bytes(key)), KeyNotFoundError);
    }

    for (int i = 50; i < 100; ++i) {
        std::string key = "skiplist_db_key" + std::to_string(i);
        std::string expected_value = "skiplist_db_value" + std::to_string(i);
        auto actual_value = db->get(string_to_bytes(key));
        EXPECT_EQ(bytes_to_string(actual_value), expected_value);
    }

    db->close();
}

TEST_F(DatabaseAdvancedIndexTest, BPlusTreeIndex) {
    options_.index_type = IndexType::BPLUS_TREE;
    auto db = DB::open(options_);

    // 插入数据
    for (int i = 0; i < 100; ++i) {
        std::string key = "bplus_db_key" + std::to_string(i);
        std::string value = "bplus_db_value" + std::to_string(i);
        db->put(string_to_bytes(key), string_to_bytes(value));
    }

    // 验证数据
    for (int i = 0; i < 100; ++i) {
        std::string key = "bplus_db_key" + std::to_string(i);
        std::string expected_value = "bplus_db_value" + std::to_string(i);
        auto actual_value = db->get(string_to_bytes(key));
        EXPECT_EQ(bytes_to_string(actual_value), expected_value);
    }

    db->close();

    // 重新打开数据库验证持久化
    auto db2 = DB::open(options_);
    
    for (int i = 0; i < 100; ++i) {
        std::string key = "bplus_db_key" + std::to_string(i);
        std::string expected_value = "bplus_db_value" + std::to_string(i);
        auto actual_value = db2->get(string_to_bytes(key));
        EXPECT_EQ(bytes_to_string(actual_value), expected_value);
    }

    db2->close();
}

// 性能比较测试
TEST_F(DatabaseAdvancedIndexTest, IndexPerformanceComparison) {
    const int DATA_SIZE = 1000;
    std::vector<std::pair<std::string, std::string>> test_data;
    
    // 准备测试数据
    for (int i = 0; i < DATA_SIZE; ++i) {
        test_data.push_back({
            "perf_key" + std::to_string(i),
            "perf_value" + std::to_string(i)
        });
    }

    // 测试BTree索引
    {
        Options btree_options = options_;
        btree_options.index_type = IndexType::BTREE;
        btree_options.dir_path = temp_dir_ + "_btree";
        system(("mkdir -p " + btree_options.dir_path).c_str());

        auto start = std::chrono::high_resolution_clock::now();
        
        auto db = DB::open(btree_options);
        for (const auto& [key, value] : test_data) {
            db->put(string_to_bytes(key), string_to_bytes(value));
        }
        db->close();
        
        auto end = std::chrono::high_resolution_clock::now();
        auto btree_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        std::cout << "BTree index write time: " << btree_time.count() << " microseconds" << std::endl;
        system(("rm -rf " + btree_options.dir_path).c_str());
    }

    // 测试SkipList索引
    {
        Options skiplist_options = options_;
        skiplist_options.index_type = IndexType::SKIPLIST;
        skiplist_options.dir_path = temp_dir_ + "_skiplist";
        system(("mkdir -p " + skiplist_options.dir_path).c_str());

        auto start = std::chrono::high_resolution_clock::now();
        
        auto db = DB::open(skiplist_options);
        for (const auto& [key, value] : test_data) {
            db->put(string_to_bytes(key), string_to_bytes(value));
        }
        db->close();
        
        auto end = std::chrono::high_resolution_clock::now();
        auto skiplist_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        std::cout << "SkipList index write time: " << skiplist_time.count() << " microseconds" << std::endl;
        system(("rm -rf " + skiplist_options.dir_path).c_str());
    }
}

// ART索引测试
TEST_F(AdvancedIndexTest, ARTIndexBasicOperations) {
    auto art_index = std::make_unique<ARTIndex>();
    
    // 测试基本插入和查找
    Bytes key1 = string_to_bytes("test_key_1");
    LogRecordPos pos1(1, 100, 10);
    
    auto old_pos = art_index->put(key1, pos1);
    EXPECT_EQ(old_pos, nullptr);
    
    auto result = art_index->get(key1);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->fid, 1);
    EXPECT_EQ(result->offset, 100);
    EXPECT_EQ(result->size, 10);
    
    // 测试更新
    LogRecordPos pos2(2, 200, 20);
    old_pos = art_index->put(key1, pos2);
    ASSERT_NE(old_pos, nullptr);
    EXPECT_EQ(old_pos->fid, 1);
    
    result = art_index->get(key1);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->fid, 2);
    EXPECT_EQ(result->offset, 200);
    EXPECT_EQ(result->size, 20);
    
    // 测试删除
    auto [removed_pos, found] = art_index->remove(key1);
    EXPECT_TRUE(found);
    ASSERT_NE(removed_pos, nullptr);
    EXPECT_EQ(removed_pos->fid, 2);
    
    result = art_index->get(key1);
    EXPECT_EQ(result, nullptr);
    
    art_index->close();
}

TEST_F(AdvancedIndexTest, ARTIndexMultipleKeys) {
    auto art_index = std::make_unique<ARTIndex>();
    
    // 插入多个键
    std::vector<std::pair<std::string, LogRecordPos>> test_data = {
        {"apple", LogRecordPos(1, 10, 5)},
        {"application", LogRecordPos(2, 20, 6)},
        {"app", LogRecordPos(3, 30, 7)},
        {"banana", LogRecordPos(4, 40, 8)},
        {"band", LogRecordPos(5, 50, 9)}
    };
    
    for (const auto& [key_str, pos] : test_data) {
        Bytes key = string_to_bytes(key_str);
        auto old_pos = art_index->put(key, pos);
        EXPECT_EQ(old_pos, nullptr);
    }
    
    EXPECT_EQ(art_index->size(), test_data.size());
    
    // 验证所有键
    for (const auto& [key_str, expected_pos] : test_data) {
        Bytes key = string_to_bytes(key_str);
        auto result = art_index->get(key);
        ASSERT_NE(result, nullptr) << "Key " << key_str << " not found";
        EXPECT_EQ(result->fid, expected_pos.fid);
        EXPECT_EQ(result->offset, expected_pos.offset);
        EXPECT_EQ(result->size, expected_pos.size);
    }
    
    art_index->close();
}

TEST_F(AdvancedIndexTest, ARTIndexIterator) {
    auto art_index = std::make_unique<ARTIndex>();
    
    std::vector<std::string> keys = {"apple", "banana", "cherry", "date"};
    
    for (size_t i = 0; i < keys.size(); i++) {
        Bytes key = string_to_bytes(keys[i]);
        LogRecordPos pos(static_cast<uint32_t>(i + 1), i * 10, 5);
        art_index->put(key, pos);
    }
    
    // 测试迭代器
    auto iter = art_index->iterator(false);
    int count = 0;
    
    for (iter->rewind(); iter->valid(); iter->next()) {
        count++;
        auto key = iter->key();
        (void)iter->value(); // 避免未使用变量警告
        
        std::string key_str = bytes_to_string(key);
        auto it = std::find(keys.begin(), keys.end(), key_str);
        EXPECT_NE(it, keys.end()) << "Unexpected key: " << key_str;
    }
    
    EXPECT_EQ(count, static_cast<int>(keys.size()));
    
    iter->close();
    art_index->close();
}

TEST_F(AdvancedIndexTest, ARTIndexWithDB) {
    Options options = Options::default_options();
    options.dir_path = temp_dir_ + "/art_db";
    options.index_type = IndexType::ART;
    
    system(("rm -rf " + options.dir_path).c_str());
    system(("mkdir -p " + options.dir_path).c_str());
    
    auto db = DB::open(options);
    
    // 插入测试数据
    std::vector<std::pair<std::string, std::string>> test_data = {
        {"art_key_1", "art_value_1"},
        {"art_key_2", "art_value_2"},
        {"art_key_3", "art_value_3"},
        {"prefix_test_1", "prefix_value_1"},
        {"prefix_test_2", "prefix_value_2"}
    };
    
    for (const auto& [key, value] : test_data) {
        db->put(string_to_bytes(key), string_to_bytes(value));
    }
    
    // 验证数据
    for (const auto& [key, expected_value] : test_data) {
        auto result = db->get(string_to_bytes(key));
        ASSERT_FALSE(result.empty()) << "Key " << key << " not found";
        EXPECT_EQ(bytes_to_string(result), expected_value);
    }
    
    // 测试删除
    db->remove(string_to_bytes("art_key_1"));
    EXPECT_THROW(db->get(string_to_bytes("art_key_1")), KeyNotFoundError);
    
    // 验证其他键仍然存在
    auto result = db->get(string_to_bytes("art_key_2"));
    EXPECT_FALSE(result.empty());
    EXPECT_EQ(bytes_to_string(result), "art_value_2");
    
    db->close();
    system(("rm -rf " + options.dir_path).c_str());
}

TEST_F(AdvancedIndexTest, CompareAllIndexTypes) {
    const int num_operations = 1000;
    
    // 生成测试数据
    std::vector<std::pair<std::string, std::string>> test_data;
    test_data.reserve(num_operations);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1000, 9999);
    
    for (int i = 0; i < num_operations; i++) {
        std::string key = "test_key_" + std::to_string(dis(gen)) + "_" + std::to_string(i);
        std::string value = "test_value_" + std::to_string(i);
        test_data.emplace_back(key, value);
    }
    
    // 测试不同索引类型的性能
    std::vector<IndexType> index_types = {
        IndexType::BTREE,
        IndexType::SKIPLIST,
        IndexType::ART
    };
    
    std::vector<std::string> index_names = {
        "BTree",
        "SkipList", 
        "ART"
    };
    
    for (size_t i = 0; i < index_types.size(); i++) {
        Options options = Options::default_options();
        options.dir_path = temp_dir_ + "/" + index_names[i] + "_compare_test";
        options.index_type = index_types[i];
        options.data_file_size = 64 * 1024 * 1024;  // 64MB
        
        system(("rm -rf " + options.dir_path).c_str());
        system(("mkdir -p " + options.dir_path).c_str());
        
        auto start = std::chrono::high_resolution_clock::now();
        (void)start; // 避免未使用变量警告
        
        auto db = DB::open(options);
        
        // 写入测试
        auto write_start = std::chrono::high_resolution_clock::now();
        for (const auto& [key, value] : test_data) {
            db->put(string_to_bytes(key), string_to_bytes(value));
        }
        auto write_end = std::chrono::high_resolution_clock::now();
        
        // 读取测试
        auto read_start = std::chrono::high_resolution_clock::now();
        for (const auto& [key, expected_value] : test_data) {
            auto result = db->get(string_to_bytes(key));
            EXPECT_FALSE(result.empty());
        }
        auto read_end = std::chrono::high_resolution_clock::now();
        
        db->close();
        
        auto write_time = std::chrono::duration_cast<std::chrono::microseconds>(write_end - write_start);
        auto read_time = std::chrono::duration_cast<std::chrono::microseconds>(read_end - read_start);
        
        std::cout << index_names[i] << " Index Performance:" << std::endl;
        std::cout << "  Write time: " << write_time.count() << " microseconds" << std::endl;
        std::cout << "  Read time: " << read_time.count() << " microseconds" << std::endl;
        std::cout << "  Write QPS: " << (num_operations * 1000000.0 / write_time.count()) << std::endl;
        std::cout << "  Read QPS: " << (num_operations * 1000000.0 / read_time.count()) << std::endl;
        std::cout << std::endl;
        
        system(("rm -rf " + options.dir_path).c_str());
    }
}

}  // namespace test
}  // namespace bitcask
