#include "bitcask/index.h"
#include "bitcask/skiplist_index.h"
#include "bitcask/bplus_tree_index.h"
#include "bitcask/art_index.h"
#include <map>
#include <shared_mutex>
#include <algorithm>

namespace bitcask {

// BTreeIndex实现
BTreeIndex::BTreeIndex() = default;

std::unique_ptr<LogRecordPos> BTreeIndex::put(const Bytes& key, const LogRecordPos& pos) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    
    auto it = tree_.find(key);
    if (it != tree_.end()) {
        // 如果key已存在，返回旧位置
        auto old_pos = std::make_unique<LogRecordPos>(it->second);
        it->second = pos;
        return old_pos;
    } else {
        // 如果key不存在，插入新位置
        tree_[key] = pos;
        return nullptr;
    }
}

std::unique_ptr<LogRecordPos> BTreeIndex::get(const Bytes& key) {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    
    auto it = tree_.find(key);
    if (it != tree_.end()) {
        return std::make_unique<LogRecordPos>(it->second);
    }
    return nullptr;
}

std::pair<std::unique_ptr<LogRecordPos>, bool> BTreeIndex::remove(const Bytes& key) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    
    auto it = tree_.find(key);
    if (it != tree_.end()) {
        auto old_pos = std::make_unique<LogRecordPos>(it->second);
        tree_.erase(it);
        return {std::move(old_pos), true};
    }
    return {nullptr, false};
}

size_t BTreeIndex::size() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return tree_.size();
}

std::unique_ptr<IndexIterator> BTreeIndex::iterator(bool reverse) {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return std::make_unique<BTreeIterator>(tree_, reverse);
}

std::vector<Bytes> BTreeIndex::list_keys() {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    
    std::vector<Bytes> keys;
    keys.reserve(tree_.size());
    
    for (const auto& pair : tree_) {
        keys.push_back(pair.first);
    }
    
    return keys;
}

void BTreeIndex::close() {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    tree_.clear();
}

// BTreeIterator实现
BTreeIterator::BTreeIterator(const std::map<Bytes, LogRecordPos>& tree, bool reverse)
    : current_index_(0), reverse_(reverse) {
    
    // 将树中的数据复制到数组中
    items_.reserve(tree.size());
    for (const auto& pair : tree) {
        items_.emplace_back(pair.first, pair.second);
    }
    
    // 如果是反向迭代，反转数组
    if (reverse_) {
        std::reverse(items_.begin(), items_.end());
    }
}

void BTreeIterator::rewind() {
    current_index_ = 0;
}

void BTreeIterator::seek(const Bytes& key) {
    if (reverse_) {
        // 反向迭代：找到第一个小于等于key的位置
        for (size_t i = 0; i < items_.size(); ++i) {
            if (items_[i].first <= key) {
                current_index_ = i;
                return;
            }
        }
        current_index_ = items_.size(); // 未找到
    } else {
        // 正向迭代：找到第一个大于等于key的位置
        for (size_t i = 0; i < items_.size(); ++i) {
            if (items_[i].first >= key) {
                current_index_ = i;
                return;
            }
        }
        current_index_ = items_.size(); // 未找到
    }
}

void BTreeIterator::next() {
    if (current_index_ < items_.size()) {
        current_index_++;
    }
}

bool BTreeIterator::valid() const {
    return current_index_ < items_.size();
}

Bytes BTreeIterator::key() const {
    if (!valid()) {
        throw BitcaskException("Iterator is not valid");
    }
    return items_[current_index_].first;
}

LogRecordPos BTreeIterator::value() const {
    if (!valid()) {
        throw BitcaskException("Iterator is not valid");
    }
    return items_[current_index_].second;
}

void BTreeIterator::close() {
    items_.clear();
    current_index_ = 0;
}

// 工厂函数
std::unique_ptr<Indexer> create_indexer(IndexType type,
                                       const std::string& dir_path,
                                       bool /*sync*/) {
    switch (type) {
        case IndexType::BTREE:
            return std::make_unique<BTreeIndex>();
        case IndexType::SKIPLIST:
            return std::make_unique<SkipListIndex>();
        case IndexType::BPLUS_TREE:
            return std::make_unique<BPlusTreeIndex>(dir_path);
        case IndexType::ART:
            return std::make_unique<ARTIndex>();
        default:
            throw BitcaskException("Unsupported index type");
    }
}

}  // namespace bitcask
