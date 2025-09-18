#pragma once

#include "index.h"
#include "options.h"
#include <string>
#include <memory>
#include <map>
#include <fstream>
#include <mutex>

namespace bitcask {

// B+树节点类型
enum class BPlusNodeType {
    INTERNAL,
    LEAF
};

// B+树节点
struct BPlusTreeNode {
    BPlusNodeType type;
    std::vector<Bytes> keys;
    std::vector<LogRecordPos> values;  // 仅叶子节点使用
    std::vector<std::shared_ptr<BPlusTreeNode>> children;  // 仅内部节点使用
    std::shared_ptr<BPlusTreeNode> next;  // 叶子节点链表指针
    bool is_dirty;
    
    BPlusTreeNode(BPlusNodeType t) : type(t), is_dirty(false) {}
};

// B+树索引实现
class BPlusTreeIndex : public Indexer {
public:
    explicit BPlusTreeIndex(const std::string& dir_path);
    ~BPlusTreeIndex();

    // Indexer接口实现
    std::unique_ptr<LogRecordPos> put(const Bytes& key, const LogRecordPos& pos) override;
    std::unique_ptr<LogRecordPos> get(const Bytes& key) override;
    std::pair<std::unique_ptr<LogRecordPos>, bool> remove(const Bytes& key) override;
    size_t size() const override;
    std::unique_ptr<IndexIterator> iterator(bool reverse = false) override;
    std::vector<Bytes> list_keys() override;
    void close() override;

    // 同步到磁盘
    void sync();

private:
    static const int MAX_KEYS = 64;  // 每个节点的最大键数
    static const std::string INDEX_FILE_NAME;
    
    std::string dir_path_;
    std::string index_file_path_;
    std::shared_ptr<BPlusTreeNode> root_;
    std::fstream file_;
    mutable std::mutex mutex_;
    
    // 加载索引文件
    void load_from_file();
    
    // 保存到文件
    void save_to_file();
    
    // 序列化节点
    void serialize_node(const std::shared_ptr<BPlusTreeNode>& node, std::ostream& os);
    
    // 反序列化节点
    std::shared_ptr<BPlusTreeNode> deserialize_node(std::istream& is);
    
    // 查找叶子节点
    std::shared_ptr<BPlusTreeNode> find_leaf(const Bytes& key);
    
    // 插入到叶子节点
    std::unique_ptr<LogRecordPos> insert_to_leaf(std::shared_ptr<BPlusTreeNode> leaf, 
                                                 const Bytes& key, const LogRecordPos& pos);
    
    // 分裂叶子节点
    void split_leaf(std::shared_ptr<BPlusTreeNode> leaf);
    
    // 分裂内部节点
    void split_internal(std::shared_ptr<BPlusTreeNode> node);
    
    // 从叶子节点删除
    std::pair<std::unique_ptr<LogRecordPos>, bool> delete_from_leaf(std::shared_ptr<BPlusTreeNode> leaf, 
                                                                   const Bytes& key);
    
    // 比较键值
    int compare_keys(const Bytes& a, const Bytes& b) const;
    
    // 查找键在节点中的位置
    int find_key_position(const std::shared_ptr<BPlusTreeNode>& node, const Bytes& key) const;
    
    // 找到第一个叶子节点
    std::shared_ptr<BPlusTreeNode> find_first_leaf();
    
    // 序列化Bytes
    void serialize_bytes(const Bytes& bytes, std::ostream& os);
    
    // 反序列化Bytes
    Bytes deserialize_bytes(std::istream& is);
    
    // 序列化LogRecordPos
    void serialize_pos(const LogRecordPos& pos, std::ostream& os);
    
    // 反序列化LogRecordPos
    LogRecordPos deserialize_pos(std::istream& is);
};

// B+树迭代器
class BPlusTreeIterator : public IndexIterator {
public:
    BPlusTreeIterator(std::shared_ptr<BPlusTreeNode> root, const IteratorOptions& options);
    ~BPlusTreeIterator() = default;

    // IndexIterator接口实现
    void rewind() override;
    void seek(const Bytes& key) override;
    void next() override;
    bool valid() const override;
    Bytes key() const override;
    LogRecordPos value() const override;
    void close() override;

private:
    std::shared_ptr<BPlusTreeNode> root_;
    std::shared_ptr<BPlusTreeNode> current_leaf_;
    size_t current_index_;
    IteratorOptions options_;
    std::vector<std::pair<Bytes, LogRecordPos>> items_;
    size_t items_index_;
    bool use_items_;
    
    // 找到第一个叶子节点
    std::shared_ptr<BPlusTreeNode> find_first_leaf();
    
    // 初始化项目列表（用于反向迭代）
    void init_items();
    
    // 检查键是否匹配前缀
    bool key_matches_prefix(const Bytes& key) const;
    
    // 比较键值
    int compare_keys(const Bytes& a, const Bytes& b) const;
};

}  // namespace bitcask
