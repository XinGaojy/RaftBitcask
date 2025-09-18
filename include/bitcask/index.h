#pragma once

#include "common.h"
#include "log_record.h"
#include "options.h"
#include <memory>
#include <vector>
#include <map>
#include <shared_mutex>

namespace bitcask {

// 索引迭代器接口
class IndexIterator {
public:
    virtual ~IndexIterator() = default;

    // 重新回到起点
    virtual void rewind() = 0;

    // 根据key查找第一个大于等于的位置
    virtual void seek(const Bytes& key) = 0;

    // 移动到下一个位置
    virtual void next() = 0;

    // 检查是否有效
    virtual bool valid() const = 0;

    // 获取当前key
    virtual Bytes key() const = 0;

    // 获取当前value
    virtual LogRecordPos value() const = 0;

    // 关闭迭代器
    virtual void close() = 0;
};

// 索引接口
class Indexer {
public:
    virtual ~Indexer() = default;

    // 存储key对应的位置信息
    virtual std::unique_ptr<LogRecordPos> put(const Bytes& key, const LogRecordPos& pos) = 0;

    // 根据key获取位置信息
    virtual std::unique_ptr<LogRecordPos> get(const Bytes& key) = 0;

    // 删除key对应的位置信息
    virtual std::pair<std::unique_ptr<LogRecordPos>, bool> remove(const Bytes& key) = 0;

    // 获取索引中的数据量
    virtual size_t size() const = 0;

    // 创建迭代器
    virtual std::unique_ptr<IndexIterator> iterator(bool reverse = false) = 0;

    // 获取所有keys
    virtual std::vector<Bytes> list_keys() = 0;

    // 关闭索引
    virtual void close() = 0;
};

// BTree索引项
struct BTreeItem {
    Bytes key;
    LogRecordPos pos;

    BTreeItem() = default;
    BTreeItem(const Bytes& k, const LogRecordPos& p) : key(k), pos(p) {}

    bool operator<(const BTreeItem& other) const {
        return key < other.key;
    }
};

// BTree索引实现
class BTreeIndex : public Indexer {
public:
    BTreeIndex();
    ~BTreeIndex() override = default;

    std::unique_ptr<LogRecordPos> put(const Bytes& key, const LogRecordPos& pos) override;
    std::unique_ptr<LogRecordPos> get(const Bytes& key) override;
    std::pair<std::unique_ptr<LogRecordPos>, bool> remove(const Bytes& key) override;
    size_t size() const override;
    std::unique_ptr<IndexIterator> iterator(bool reverse = false) override;
    std::vector<Bytes> list_keys() override;
    void close() override;

private:
    std::map<Bytes, LogRecordPos> tree_;
    mutable std::shared_mutex mutex_;
};

// BTree迭代器实现
class BTreeIterator : public IndexIterator {
public:
    BTreeIterator(const std::map<Bytes, LogRecordPos>& tree, bool reverse);
    ~BTreeIterator() override = default;

    void rewind() override;
    void seek(const Bytes& key) override;
    void next() override;
    bool valid() const override;
    Bytes key() const override;
    LogRecordPos value() const override;
    void close() override;

private:
    std::vector<std::pair<Bytes, LogRecordPos>> items_;
    size_t current_index_;
    bool reverse_;
};

// 创建索引的工厂函数
std::unique_ptr<Indexer> create_indexer(IndexType type, 
                                       const std::string& dir_path = "",
                                       bool sync = false);

}  // namespace bitcask
