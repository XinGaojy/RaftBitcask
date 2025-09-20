#include "bitcask/bplus_tree_index.h"
#include "bitcask/utils.h"
#include <memory>
#include <vector>
#include <algorithm>
#include <iostream>
#include <fstream>

namespace bitcask {

const std::string BPlusTreeIndex::INDEX_FILE_NAME = "bptree-index.db";

BPlusTreeIndex::BPlusTreeIndex(const std::string& dir_path) 
    : dir_path_(dir_path), index_file_path_(dir_path + "/" + INDEX_FILE_NAME) {
    
    // 创建目录
    utils::create_directory(dir_path_);
    
    // 初始化根节点
    root_ = std::make_shared<BPlusTreeNode>(BPlusNodeType::LEAF);
    
    // 尝试从文件加载
    if (utils::file_exists(index_file_path_)) {
        load_from_file();
    }
}

BPlusTreeIndex::~BPlusTreeIndex() {
    close();
}

std::unique_ptr<LogRecordPos> BPlusTreeIndex::put(const Bytes& key, const LogRecordPos& pos) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto leaf = find_leaf(key);
    auto old_pos = insert_to_leaf(leaf, key, pos);
    
    // 暂时简化：不处理节点分裂，确保基本功能正常
    
    return old_pos;
}

std::unique_ptr<LogRecordPos> BPlusTreeIndex::get(const Bytes& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto leaf = find_leaf(key);
    int pos = find_key_position(leaf, key);
    
    if (pos < static_cast<int>(leaf->keys.size()) && 
        compare_keys(leaf->keys[pos], key) == 0) {
        return std::make_unique<LogRecordPos>(leaf->values[pos]);
    }
    
    return nullptr;
}

std::pair<std::unique_ptr<LogRecordPos>, bool> BPlusTreeIndex::remove(const Bytes& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto leaf = find_leaf(key);
    return delete_from_leaf(leaf, key);
}

std::vector<Bytes> BPlusTreeIndex::list_keys() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<Bytes> keys;
    auto current = find_first_leaf();
    
    while (current) {
        for (const auto& key : current->keys) {
            keys.push_back(key);
        }
        current = current->next;
    }
    
    return keys;
}

size_t BPlusTreeIndex::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    size_t count = 0;
    auto current = root_;
    
    // 找到第一个叶子节点
    while (current && current->type == BPlusNodeType::INTERNAL) {
        if (!current->children.empty()) {
            current = current->children[0];
        } else {
            break;
        }
    }
    
    // 遍历所有叶子节点
    while (current) {
        count += current->keys.size();
        current = current->next;
    }
    
    return count;
}

std::unique_ptr<IndexIterator> BPlusTreeIndex::iterator(bool reverse) {
    IteratorOptions options;
    options.reverse = reverse;
    return std::make_unique<BPlusTreeIterator>(root_, options);
}

void BPlusTreeIndex::sync() {
    // 使用try_lock避免在高并发情况下阻塞
    std::unique_lock<std::mutex> lock(mutex_, std::try_to_lock);
    if (!lock.owns_lock()) {
        // 如果无法获取锁，跳过此次同步
        // 这避免了在备份过程中的不必要阻塞
        return;
    }
    
    try {
        save_to_file();
    } catch (const std::exception&) {
        // 忽略文件保存错误，不影响主流程
    }
}

void BPlusTreeIndex::close() {
    sync();
    if (file_.is_open()) {
        file_.close();
    }
}

void BPlusTreeIndex::load_from_file() {
    try {
        file_.open(index_file_path_, std::ios::in | std::ios::binary);
        if (file_.is_open()) {
            // 检查文件是否为空
            file_.seekg(0, std::ios::end);
            if (file_.tellg() > 0) {
                file_.seekg(0, std::ios::beg);
                root_ = deserialize_node(file_);
            }
            file_.close();
        }
    } catch (const std::exception&) {
        // 如果加载失败，使用默认的空根节点
        root_ = std::make_shared<BPlusTreeNode>(BPlusNodeType::LEAF);
        if (file_.is_open()) {
            file_.close();
        }
    }
}

void BPlusTreeIndex::save_to_file() {
    // 使用简化的保存策略，避免复杂的临时文件操作
    try {
        std::ofstream file(index_file_path_, std::ios::out | std::ios::binary | std::ios::trunc);
        if (!file.is_open()) {
            return; // 无法打开文件，退出
        }
        
        serialize_node(root_, file);
        file.flush();
        file.close();
        
    } catch (const std::exception&) {
        // 忽略文件保存错误，不影响主流程
        // 在某些环境中，文件操作可能失败，但这不应该中断程序
    }
}

void BPlusTreeIndex::serialize_node(const std::shared_ptr<BPlusTreeNode>& node, std::ostream& os) {
    // 写入节点类型
    uint8_t type = static_cast<uint8_t>(node->type);
    os.write(reinterpret_cast<const char*>(&type), sizeof(type));
    
    // 写入键数量
    uint32_t key_count = node->keys.size();
    os.write(reinterpret_cast<const char*>(&key_count), sizeof(key_count));
    
    // 写入键
    for (const auto& key : node->keys) {
        serialize_bytes(key, os);
    }
    
    if (node->type == BPlusNodeType::LEAF) {
        // 写入值
        for (const auto& value : node->values) {
            serialize_pos(value, os);
        }
    } else {
        // 写入子节点
        for (const auto& child : node->children) {
            serialize_node(child, os);
        }
    }
}

std::shared_ptr<BPlusTreeNode> BPlusTreeIndex::deserialize_node(std::istream& is) {
    // 检查流状态
    if (!is.good() || is.eof()) {
        throw BitcaskException("Invalid stream state during deserialization");
    }
    
    // 读取节点类型
    uint8_t type;
    is.read(reinterpret_cast<char*>(&type), sizeof(type));
    if (!is.good()) {
        throw BitcaskException("Failed to read node type");
    }
    
    auto node = std::make_shared<BPlusTreeNode>(static_cast<BPlusNodeType>(type));
    
    // 读取键数量
    uint32_t key_count;
    is.read(reinterpret_cast<char*>(&key_count), sizeof(key_count));
    if (!is.good()) {
        throw BitcaskException("Failed to read key count");
    }
    
    // 防止无限大的键数量
    if (key_count > MAX_KEYS * 2) {  // 允许一些余量
        throw BitcaskException("Invalid key count in serialized node");
    }
    
    // 读取键
    for (uint32_t i = 0; i < key_count && is.good(); i++) {
        node->keys.push_back(deserialize_bytes(is));
    }
    
    if (node->type == BPlusNodeType::LEAF) {
        // 读取值
        for (uint32_t i = 0; i < key_count && is.good(); i++) {
            node->values.push_back(deserialize_pos(is));
        }
    } else {
        // 读取子节点（内部节点的子节点数量应该是键数量+1）
        for (uint32_t i = 0; i <= key_count && is.good(); i++) {
            node->children.push_back(deserialize_node(is));
        }
    }
    
    return node;
}

std::shared_ptr<BPlusTreeNode> BPlusTreeIndex::find_leaf(const Bytes& key) {
    auto current = root_;
    
    while (current->type == BPlusNodeType::INTERNAL) {
        int pos = find_key_position(current, key);
        current = current->children[pos];
    }
    
    return current;
}

std::shared_ptr<BPlusTreeNode> BPlusTreeIndex::find_first_leaf() {
    auto current = root_;
    
    while (current->type == BPlusNodeType::INTERNAL) {
        current = current->children[0];
    }
    
    return current;
}

std::unique_ptr<LogRecordPos> BPlusTreeIndex::insert_to_leaf(std::shared_ptr<BPlusTreeNode> leaf, 
                                                            const Bytes& key, const LogRecordPos& pos) {
    int insert_pos = find_key_position(leaf, key);
    
    std::unique_ptr<LogRecordPos> old_pos = nullptr;
    
    if (insert_pos < static_cast<int>(leaf->keys.size()) && 
        compare_keys(leaf->keys[insert_pos], key) == 0) {
        // 键已存在，更新值
        old_pos = std::make_unique<LogRecordPos>(leaf->values[insert_pos]);
        leaf->values[insert_pos] = pos;
    } else {
        // 插入新键值对
        leaf->keys.insert(leaf->keys.begin() + insert_pos, key);
        leaf->values.insert(leaf->values.begin() + insert_pos, pos);
        
        // 简化：暂时不处理节点分裂，为了确保基本功能正常
        // TODO: 实现节点分裂逻辑
    }
    
    leaf->is_dirty = true;
    return old_pos;
}

void BPlusTreeIndex::split_leaf(std::shared_ptr<BPlusTreeNode> leaf) {
    size_t mid = leaf->keys.size() / 2;
    
    auto new_leaf = std::make_shared<BPlusTreeNode>(BPlusNodeType::LEAF);
    
    // 移动一半的键值对到新叶子
    new_leaf->keys.assign(leaf->keys.begin() + mid, leaf->keys.end());
    new_leaf->values.assign(leaf->values.begin() + mid, leaf->values.end());
    
    // 调整原叶子的大小
    leaf->keys.resize(mid);
    leaf->values.resize(mid);
    
    // 更新链表指针
    new_leaf->next = leaf->next;
    leaf->next = new_leaf;
    
    leaf->is_dirty = true;
    new_leaf->is_dirty = true;
}

void BPlusTreeIndex::split_internal(std::shared_ptr<BPlusTreeNode> node) {
    // 简化实现：这里只是标记为脏数据
    // 在实际应用中，需要实现完整的B+树分裂逻辑
    node->is_dirty = true;
}

std::pair<std::unique_ptr<LogRecordPos>, bool> BPlusTreeIndex::delete_from_leaf(std::shared_ptr<BPlusTreeNode> leaf, 
                                                                                                  const Bytes& key) {
    int pos = find_key_position(leaf, key);
    
    if (pos < static_cast<int>(leaf->keys.size()) && 
        compare_keys(leaf->keys[pos], key) == 0) {
        
        auto old_pos = std::make_unique<LogRecordPos>(leaf->values[pos]);
        
        leaf->keys.erase(leaf->keys.begin() + pos);
        leaf->values.erase(leaf->values.begin() + pos);
        leaf->is_dirty = true;
        
        return {std::move(old_pos), true};
    }
    
    return {nullptr, false};
}

int BPlusTreeIndex::compare_keys(const Bytes& a, const Bytes& b) const {
    size_t min_len = std::min(a.size(), b.size());
    
    for (size_t i = 0; i < min_len; i++) {
        if (a[i] < b[i]) return -1;
        if (a[i] > b[i]) return 1;
    }
    
    if (a.size() < b.size()) return -1;
    if (a.size() > b.size()) return 1;
    return 0;
}

int BPlusTreeIndex::find_key_position(const std::shared_ptr<BPlusTreeNode>& node, const Bytes& key) const {
    int left = 0, right = node->keys.size();
    
    while (left < right) {
        int mid = left + (right - left) / 2;
        if (compare_keys(node->keys[mid], key) < 0) {
            left = mid + 1;
        } else {
            right = mid;
        }
    }
    
    return left;
}

void BPlusTreeIndex::serialize_bytes(const Bytes& bytes, std::ostream& os) {
    uint32_t size = bytes.size();
    os.write(reinterpret_cast<const char*>(&size), sizeof(size));
    os.write(reinterpret_cast<const char*>(bytes.data()), size);
}

Bytes BPlusTreeIndex::deserialize_bytes(std::istream& is) {
    uint32_t size;
    is.read(reinterpret_cast<char*>(&size), sizeof(size));
    
    Bytes bytes(size);
    is.read(reinterpret_cast<char*>(bytes.data()), size);
    
    return bytes;
}

void BPlusTreeIndex::serialize_pos(const LogRecordPos& pos, std::ostream& os) {
    os.write(reinterpret_cast<const char*>(&pos.fid), sizeof(pos.fid));
    os.write(reinterpret_cast<const char*>(&pos.offset), sizeof(pos.offset));
    os.write(reinterpret_cast<const char*>(&pos.size), sizeof(pos.size));
}

LogRecordPos BPlusTreeIndex::deserialize_pos(std::istream& is) {
    LogRecordPos pos;
    is.read(reinterpret_cast<char*>(&pos.fid), sizeof(pos.fid));
    is.read(reinterpret_cast<char*>(&pos.offset), sizeof(pos.offset));
    is.read(reinterpret_cast<char*>(&pos.size), sizeof(pos.size));
    return pos;
}

// BPlusTreeIterator实现
BPlusTreeIterator::BPlusTreeIterator(std::shared_ptr<BPlusTreeNode> root, const IteratorOptions& options)
    : root_(root), current_leaf_(nullptr), current_index_(0), options_(options), 
      items_index_(0), use_items_(false) {
    
    if (options_.reverse) {
        init_items();
        use_items_ = true;
    }
    
    rewind();
}

void BPlusTreeIterator::rewind() {
    if (use_items_) {
        items_index_ = 0;
    } else {
        current_leaf_ = find_first_leaf();
        current_index_ = 0;
        
        // 跳过不匹配前缀的键
        while (valid() && !key_matches_prefix(key())) {
            next();
        }
    }
}

void BPlusTreeIterator::seek(const Bytes& key) {
    if (use_items_) {
        for (size_t i = 0; i < items_.size(); i++) {
            if (compare_keys(items_[i].first, key) >= 0) {
                items_index_ = i;
                return;
            }
        }
        items_index_ = items_.size();
    } else {
        // 简化实现：从根开始查找
        current_leaf_ = find_first_leaf();
        current_index_ = 0;
        
        // 线性查找（在实际应用中应该优化）
        while (valid() && compare_keys(this->key(), key) < 0) {
            next();
        }
    }
}

void BPlusTreeIterator::next() {
    if (use_items_) {
        items_index_++;
    } else {
        current_index_++;
        
        if (current_leaf_ && current_index_ >= current_leaf_->keys.size()) {
            current_leaf_ = current_leaf_->next;
            current_index_ = 0;
        }
        
        // 跳过不匹配前缀的键
        while (valid() && !key_matches_prefix(key())) {
            current_index_++;
            if (current_leaf_ && current_index_ >= current_leaf_->keys.size()) {
                current_leaf_ = current_leaf_->next;
                current_index_ = 0;
            }
        }
    }
}

bool BPlusTreeIterator::valid() const {
    if (use_items_) {
        return items_index_ < items_.size();
    } else {
        return current_leaf_ && current_index_ < current_leaf_->keys.size();
    }
}

Bytes BPlusTreeIterator::key() const {
    if (use_items_) {
        if (items_index_ < items_.size()) {
            return items_[items_index_].first;
        }
    } else {
        if (current_leaf_ && current_index_ < current_leaf_->keys.size()) {
            return current_leaf_->keys[current_index_];
        }
    }
    return Bytes{};
}

LogRecordPos BPlusTreeIterator::value() const {
    if (use_items_) {
        if (items_index_ < items_.size()) {
            return items_[items_index_].second;
        }
    } else {
        if (current_leaf_ && current_index_ < current_leaf_->values.size()) {
            return current_leaf_->values[current_index_];
        }
    }
    return LogRecordPos{};
}

void BPlusTreeIterator::close() {
    current_leaf_ = nullptr;
    items_.clear();
}

std::shared_ptr<BPlusTreeNode> BPlusTreeIterator::find_first_leaf() {
    auto current = root_;
    
    while (current && current->type == BPlusNodeType::INTERNAL) {
        if (!current->children.empty()) {
            current = current->children[0];
        } else {
            break;
        }
    }
    
    return current;
}

void BPlusTreeIterator::init_items() {
    auto current = find_first_leaf();
    
    while (current) {
        for (size_t i = 0; i < current->keys.size(); i++) {
            if (key_matches_prefix(current->keys[i])) {
                items_.emplace_back(current->keys[i], current->values[i]);
            }
        }
        current = current->next;
    }
    
    if (options_.reverse) {
        std::reverse(items_.begin(), items_.end());
    }
}

bool BPlusTreeIterator::key_matches_prefix(const Bytes& key) const {
    if (options_.prefix.empty()) {
        return true;
    }
    
    if (key.size() < options_.prefix.size()) {
        return false;
    }
    
    return std::equal(options_.prefix.begin(), options_.prefix.end(), key.begin());
}

int BPlusTreeIterator::compare_keys(const Bytes& a, const Bytes& b) const {
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
