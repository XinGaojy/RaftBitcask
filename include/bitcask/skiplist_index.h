#pragma once

#include "index.h"
#include "options.h"
#include <memory>
#include <random>
#include <mutex>

namespace bitcask {

// SkipList节点
struct SkipListNode {
    Bytes key;
    LogRecordPos pos;
    std::vector<std::shared_ptr<SkipListNode>> forward;
    
    SkipListNode(int level) : forward(level + 1) {}
    SkipListNode(const Bytes& k, const LogRecordPos& p, int level) 
        : key(k), pos(p), forward(level + 1) {}
};

// SkipList索引实现
class SkipListIndex : public Indexer {
public:
    SkipListIndex();
    ~SkipListIndex() = default;

    // Indexer接口实现
    std::unique_ptr<LogRecordPos> put(const Bytes& key, const LogRecordPos& pos) override;
    std::unique_ptr<LogRecordPos> get(const Bytes& key) override;
    std::pair<std::unique_ptr<LogRecordPos>, bool> remove(const Bytes& key) override;
    size_t size() const override;
    std::unique_ptr<IndexIterator> iterator(bool reverse = false) override;
    std::vector<Bytes> list_keys() override;
    void close() override;

private:
    static const int MAX_LEVEL = 16;
    static constexpr float SKIPLIST_P = 0.25f;
    
    std::shared_ptr<SkipListNode> header_;
    int level_;
    mutable std::mutex mutex_;
    std::mt19937 rng_;
    
    // 生成随机层级
    int random_level();
    
    // 比较键值
    int compare_keys(const Bytes& a, const Bytes& b) const;
    
    // 查找节点
    std::shared_ptr<SkipListNode> find_node(const Bytes& key, 
                                           std::vector<std::shared_ptr<SkipListNode>>& update) const;
};

// SkipList迭代器
class SkipListIterator : public IndexIterator {
public:
    SkipListIterator(std::shared_ptr<SkipListNode> header, const IteratorOptions& options);
    ~SkipListIterator() = default;

    // IndexIterator接口实现
    void rewind() override;
    void seek(const Bytes& key) override;
    void next() override;
    bool valid() const override;
    Bytes key() const override;
    LogRecordPos value() const override;
    void close() override;

private:
    std::shared_ptr<SkipListNode> header_;
    std::shared_ptr<SkipListNode> current_;
    IteratorOptions options_;
    std::vector<std::pair<Bytes, LogRecordPos>> items_;
    size_t current_index_;
    bool use_items_;
    
    // 初始化项目列表（用于反向迭代）
    void init_items();
    
    // 检查键是否匹配前缀
    bool key_matches_prefix(const Bytes& key) const;
    
    // 比较键值
    int compare_keys(const Bytes& a, const Bytes& b) const;
};

}  // namespace bitcask
