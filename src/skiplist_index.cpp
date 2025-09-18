#include "bitcask/skiplist_index.h"
#include <memory>
#include <vector>
#include <algorithm>
#include <random>

namespace bitcask {

SkipListIndex::SkipListIndex() : level_(0), rng_(std::random_device{}()) {
    header_ = std::make_shared<SkipListNode>(MAX_LEVEL);
}

std::unique_ptr<LogRecordPos> SkipListIndex::put(const Bytes& key, const LogRecordPos& pos) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<std::shared_ptr<SkipListNode>> update(MAX_LEVEL + 1);
    auto node = find_node(key, update);
    
    std::unique_ptr<LogRecordPos> old_pos = nullptr;
    
    if (node && compare_keys(node->key, key) == 0) {
        // 键已存在，更新值
        old_pos = std::make_unique<LogRecordPos>(node->pos);
        node->pos = pos;
        return old_pos;
    }
    
    // 插入新节点
    int new_level = random_level();
    if (new_level > level_) {
        for (int i = level_ + 1; i <= new_level; i++) {
            update[i] = header_;
        }
        level_ = new_level;
    }
    
    auto new_node = std::make_shared<SkipListNode>(key, pos, new_level);
    
    for (int i = 0; i <= new_level; i++) {
        new_node->forward[i] = update[i]->forward[i];
        update[i]->forward[i] = new_node;
    }
    
    return old_pos;
}

std::unique_ptr<LogRecordPos> SkipListIndex::get(const Bytes& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<std::shared_ptr<SkipListNode>> update(MAX_LEVEL + 1);
    auto node = find_node(key, update);
    
    if (node && compare_keys(node->key, key) == 0) {
        return std::make_unique<LogRecordPos>(node->pos);
    }
    
    return nullptr;
}

std::pair<std::unique_ptr<LogRecordPos>, bool> SkipListIndex::remove(const Bytes& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<std::shared_ptr<SkipListNode>> update(MAX_LEVEL + 1);
    auto node = find_node(key, update);
    
    if (!node || compare_keys(node->key, key) != 0) {
        return {nullptr, false};
    }
    
    auto old_pos = std::make_unique<LogRecordPos>(node->pos);
    
    // 删除节点
    for (int i = 0; i <= level_; i++) {
        if (update[i]->forward[i] != node) {
            break;
        }
        update[i]->forward[i] = node->forward[i];
    }
    
    // 调整层级
    while (level_ > 0 && !header_->forward[level_]) {
        level_--;
    }
    
    return {std::move(old_pos), true};
}

std::vector<Bytes> SkipListIndex::list_keys() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<Bytes> keys;
    auto current = header_->forward[0];
    
    while (current) {
        keys.push_back(current->key);
        current = current->forward[0];
    }
    
    return keys;
}

size_t SkipListIndex::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    size_t count = 0;
    auto current = header_->forward[0];
    
    while (current) {
        count++;
        current = current->forward[0];
    }
    
    return count;
}

std::unique_ptr<IndexIterator> SkipListIndex::iterator(bool reverse) {
    IteratorOptions options;
    options.reverse = reverse;
    return std::make_unique<SkipListIterator>(header_, options);
}

void SkipListIndex::close() {
    std::lock_guard<std::mutex> lock(mutex_);
    header_.reset();
    level_ = 0;
}

int SkipListIndex::random_level() {
    int level = 0;
    std::uniform_real_distribution<float> dis(0.0f, 1.0f);
    
    while (dis(rng_) < SKIPLIST_P && level < MAX_LEVEL) {
        level++;
    }
    
    return level;
}

int SkipListIndex::compare_keys(const Bytes& a, const Bytes& b) const {
    size_t min_len = std::min(a.size(), b.size());
    
    for (size_t i = 0; i < min_len; i++) {
        if (a[i] < b[i]) return -1;
        if (a[i] > b[i]) return 1;
    }
    
    if (a.size() < b.size()) return -1;
    if (a.size() > b.size()) return 1;
    return 0;
}

std::shared_ptr<SkipListNode> SkipListIndex::find_node(const Bytes& key, 
                                                      std::vector<std::shared_ptr<SkipListNode>>& update) const {
    auto current = header_;
    
    for (int i = level_; i >= 0; i--) {
        while (current->forward[i] && compare_keys(current->forward[i]->key, key) < 0) {
            current = current->forward[i];
        }
        update[i] = current;
    }
    
    return current->forward[0];
}

// SkipListIterator实现
SkipListIterator::SkipListIterator(std::shared_ptr<SkipListNode> header, const IteratorOptions& options)
    : header_(header), current_(nullptr), options_(options), current_index_(0), use_items_(false) {
    
    if (options_.reverse) {
        init_items();
        use_items_ = true;
    }
    
    rewind();
}

void SkipListIterator::rewind() {
    if (use_items_) {
        current_index_ = 0;
    } else {
        current_ = header_->forward[0];
        
        // 如果有前缀过滤，找到第一个匹配的键
        while (current_ && !key_matches_prefix(current_->key)) {
            current_ = current_->forward[0];
        }
    }
}

void SkipListIterator::seek(const Bytes& key) {
    if (use_items_) {
        // 在排序的项目列表中查找
        for (size_t i = 0; i < items_.size(); i++) {
            if (compare_keys(items_[i].first, key) >= 0) {
                current_index_ = i;
                return;
            }
        }
        current_index_ = items_.size(); // 未找到，设置为无效位置
    } else {
        current_ = header_;
        
        // 从高层开始查找
        for (int level = 15; level >= 0; level--) {
            while (current_->forward[level] && 
                   compare_keys(current_->forward[level]->key, key) < 0) {
                current_ = current_->forward[level];
            }
        }
        
        current_ = current_->forward[0];
        
        // 如果有前缀过滤，确保键匹配前缀
        while (current_ && !key_matches_prefix(current_->key)) {
            current_ = current_->forward[0];
        }
    }
}

void SkipListIterator::next() {
    if (use_items_) {
        current_index_++;
    } else {
        if (current_) {
            current_ = current_->forward[0];
            
            // 如果有前缀过滤，跳过不匹配的键
            while (current_ && !key_matches_prefix(current_->key)) {
                current_ = current_->forward[0];
            }
        }
    }
}

bool SkipListIterator::valid() const {
    if (use_items_) {
        return current_index_ < items_.size();
    } else {
        return current_ != nullptr;
    }
}

Bytes SkipListIterator::key() const {
    if (use_items_) {
        if (current_index_ < items_.size()) {
            return items_[current_index_].first;
        }
    } else {
        if (current_) {
            return current_->key;
        }
    }
    return Bytes{};
}

LogRecordPos SkipListIterator::value() const {
    if (use_items_) {
        if (current_index_ < items_.size()) {
            return items_[current_index_].second;
        }
    } else {
        if (current_) {
            return current_->pos;
        }
    }
    return LogRecordPos{};
}

void SkipListIterator::close() {
    current_ = nullptr;
    items_.clear();
}

void SkipListIterator::init_items() {
    auto current = header_->forward[0];
    
    while (current) {
        if (key_matches_prefix(current->key)) {
            items_.emplace_back(current->key, current->pos);
        }
        current = current->forward[0];
    }
    
    // 反向排序
    if (options_.reverse) {
        std::reverse(items_.begin(), items_.end());
    }
}

bool SkipListIterator::key_matches_prefix(const Bytes& key) const {
    if (options_.prefix.empty()) {
        return true;
    }
    
    if (key.size() < options_.prefix.size()) {
        return false;
    }
    
    return std::equal(options_.prefix.begin(), options_.prefix.end(), key.begin());
}

int SkipListIterator::compare_keys(const Bytes& a, const Bytes& b) const {
    size_t min_len = std::min(a.size(), b.size());
    
    for (size_t i = 0; i < min_len; i++) {
        if (a[i] < b[i]) return -1;
        if (a[i] > b[i]) return 1;
    }
    
    if (a.size() < b.size()) return -1;
    if (a.size() > b.size()) return 1;
    return 0;
}

}  // namespace bitcask
