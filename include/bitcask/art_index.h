#pragma once

#include "index.h"
#include <memory>
#include <vector>
#include <array>
#include <mutex>

namespace bitcask {

// ART节点类型
enum class ARTNodeType {
    NODE_4,   // 最多4个子节点
    NODE_16,  // 最多16个子节点
    NODE_48,  // 最多48个子节点
    NODE_256  // 最多256个子节点
};

// ART节点基类
struct ARTNode {
    ARTNodeType type;
    uint8_t num_children;
    uint8_t prefix_len;
    std::vector<uint8_t> prefix;
    
    ARTNode(ARTNodeType t) : type(t), num_children(0), prefix_len(0) {}
    virtual ~ARTNode() = default;
    
    virtual std::shared_ptr<ARTNode> find_child(uint8_t key) = 0;
    virtual void add_child(uint8_t key, std::shared_ptr<ARTNode> child) = 0;
    virtual void remove_child(uint8_t key) = 0;
    virtual bool is_full() const = 0;
    virtual std::shared_ptr<ARTNode> grow() = 0;
};

// 叶子节点（存储实际数据）
struct ARTLeaf : public ARTNode {
    Bytes key;
    LogRecordPos value;
    
    ARTLeaf(const Bytes& k, const LogRecordPos& v) 
        : ARTNode(ARTNodeType::NODE_4), key(k), value(v) {}
    
    std::shared_ptr<ARTNode> find_child(uint8_t) override { return nullptr; }
    void add_child(uint8_t, std::shared_ptr<ARTNode>) override {}
    void remove_child(uint8_t) override {}
    bool is_full() const override { return false; }
    std::shared_ptr<ARTNode> grow() override { return nullptr; }
};

// NODE_4: 最多4个子节点
struct ARTNode4 : public ARTNode {
    std::array<uint8_t, 4> keys;
    std::array<std::shared_ptr<ARTNode>, 4> children;
    
    ARTNode4() : ARTNode(ARTNodeType::NODE_4) {
        keys.fill(0);
        children.fill(nullptr);
    }
    
    std::shared_ptr<ARTNode> find_child(uint8_t key) override;
    void add_child(uint8_t key, std::shared_ptr<ARTNode> child) override;
    void remove_child(uint8_t key) override;
    bool is_full() const override { return num_children == 4; }
    std::shared_ptr<ARTNode> grow() override;
};

// NODE_16: 最多16个子节点
struct ARTNode16 : public ARTNode {
    std::array<uint8_t, 16> keys;
    std::array<std::shared_ptr<ARTNode>, 16> children;
    
    ARTNode16() : ARTNode(ARTNodeType::NODE_16) {
        keys.fill(0);
        children.fill(nullptr);
    }
    
    std::shared_ptr<ARTNode> find_child(uint8_t key) override;
    void add_child(uint8_t key, std::shared_ptr<ARTNode> child) override;
    void remove_child(uint8_t key) override;
    bool is_full() const override { return num_children == 16; }
    std::shared_ptr<ARTNode> grow() override;
};

// NODE_48: 最多48个子节点
struct ARTNode48 : public ARTNode {
    std::array<uint8_t, 256> key_indices;  // 索引映射
    std::array<std::shared_ptr<ARTNode>, 48> children;
    
    ARTNode48() : ARTNode(ARTNodeType::NODE_48) {
        key_indices.fill(0);
        children.fill(nullptr);
    }
    
    std::shared_ptr<ARTNode> find_child(uint8_t key) override;
    void add_child(uint8_t key, std::shared_ptr<ARTNode> child) override;
    void remove_child(uint8_t key) override;
    bool is_full() const override { return num_children == 48; }
    std::shared_ptr<ARTNode> grow() override;
};

// NODE_256: 最多256个子节点
struct ARTNode256 : public ARTNode {
    std::array<std::shared_ptr<ARTNode>, 256> children;
    
    ARTNode256() : ARTNode(ARTNodeType::NODE_256) {
        children.fill(nullptr);
    }
    
    std::shared_ptr<ARTNode> find_child(uint8_t key) override;
    void add_child(uint8_t key, std::shared_ptr<ARTNode> child) override;
    void remove_child(uint8_t key) override;
    bool is_full() const override { return false; }
    std::shared_ptr<ARTNode> grow() override { return nullptr; }
};

// ART索引实现
class ARTIndex : public Indexer {
public:
    ARTIndex();
    ~ARTIndex() = default;

    // Indexer接口实现
    std::unique_ptr<LogRecordPos> put(const Bytes& key, const LogRecordPos& pos) override;
    std::unique_ptr<LogRecordPos> get(const Bytes& key) override;
    std::pair<std::unique_ptr<LogRecordPos>, bool> remove(const Bytes& key) override;
    size_t size() const override;
    std::unique_ptr<IndexIterator> iterator(bool reverse = false) override;
    std::vector<Bytes> list_keys() override;
    void close() override;

private:
    std::shared_ptr<ARTNode> root_;
    size_t size_;
    mutable std::mutex mutex_;
    
    // 查找节点
    std::shared_ptr<ARTNode> search(const Bytes& key);
    
    // 插入节点
    std::shared_ptr<ARTNode> insert(std::shared_ptr<ARTNode> node, 
                                   const Bytes& key, 
                                   const LogRecordPos& value,
                                   int depth,
                                   std::unique_ptr<LogRecordPos>& old_value);
    
    // 删除节点
    std::shared_ptr<ARTNode> delete_node(std::shared_ptr<ARTNode> node,
                                        const Bytes& key,
                                        int depth,
                                        std::unique_ptr<LogRecordPos>& old_value,
                                        bool& found);
    
    // 检查前缀
    int check_prefix(std::shared_ptr<ARTNode> node, const Bytes& key, int depth);
    
    // 最长公共前缀
    int longest_common_prefix(std::shared_ptr<ARTNode> node, const Bytes& key, int depth);
    
    // 收集所有键值对
    void collect_all(std::shared_ptr<ARTNode> node, 
                    std::vector<std::pair<Bytes, LogRecordPos>>& result);
    
    // 判断是否是叶子节点
    bool is_leaf(std::shared_ptr<ARTNode> node);
    
    // 获取叶子节点
    std::shared_ptr<ARTLeaf> get_leaf(std::shared_ptr<ARTNode> node);
};

// ART迭代器
class ARTIterator : public IndexIterator {
public:
    ARTIterator(std::shared_ptr<ARTNode> root, const IteratorOptions& options);
    ~ARTIterator() = default;

    // IndexIterator接口实现
    void rewind() override;
    void seek(const Bytes& key) override;
    void next() override;
    bool valid() const override;
    Bytes key() const override;
    LogRecordPos value() const override;
    void close() override;

private:
    std::shared_ptr<ARTNode> root_;
    IteratorOptions options_;
    std::vector<std::pair<Bytes, LogRecordPos>> items_;
    size_t current_index_;
    
    // 收集所有项目
    void collect_items();
    
    // 检查键是否匹配前缀
    bool key_matches_prefix(const Bytes& key) const;
};

}  // namespace bitcask
