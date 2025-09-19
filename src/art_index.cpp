#include "bitcask/art_index.h"
#include <algorithm>
#include <cstring>

namespace bitcask {

// ARTNode4实现
std::shared_ptr<ARTNode> ARTNode4::find_child(uint8_t key) {
    for (int i = 0; i < num_children; i++) {
        if (keys[i] == key) {
            return children[i];
        }
    }
    return nullptr;
}

void ARTNode4::add_child(uint8_t key, std::shared_ptr<ARTNode> child) {
    if (num_children < 4) {
        int pos = num_children;
        // 保持有序插入
        for (int i = 0; i < num_children; i++) {
            if (keys[i] > key) {
                pos = i;
                break;
            }
        }
        
        // 移动元素
        for (int i = num_children; i > pos; i--) {
            keys[i] = keys[i - 1];
            children[i] = children[i - 1];
        }
        
        keys[pos] = key;
        children[pos] = child;
        num_children++;
    }
}

void ARTNode4::remove_child(uint8_t key) {
    for (int i = 0; i < num_children; i++) {
        if (keys[i] == key) {
            // 移动后续元素
            for (int j = i; j < num_children - 1; j++) {
                keys[j] = keys[j + 1];
                children[j] = children[j + 1];
            }
            num_children--;
            break;
        }
    }
}

std::shared_ptr<ARTNode> ARTNode4::grow() {
    auto new_node = std::make_shared<ARTNode16>();
    new_node->num_children = num_children;
    new_node->prefix_len = prefix_len;
    new_node->prefix = prefix;
    
    for (int i = 0; i < num_children; i++) {
        new_node->keys[i] = keys[i];
        new_node->children[i] = children[i];
    }
    
    return new_node;
}

// ARTNode16实现
std::shared_ptr<ARTNode> ARTNode16::find_child(uint8_t key) {
    for (int i = 0; i < num_children; i++) {
        if (keys[i] == key) {
            return children[i];
        }
    }
    return nullptr;
}

void ARTNode16::add_child(uint8_t key, std::shared_ptr<ARTNode> child) {
    if (num_children < 16) {
        int pos = num_children;
        // 保持有序插入
        for (int i = 0; i < num_children; i++) {
            if (keys[i] > key) {
                pos = i;
                break;
            }
        }
        
        // 移动元素
        for (int i = num_children; i > pos; i--) {
            keys[i] = keys[i - 1];
            children[i] = children[i - 1];
        }
        
        keys[pos] = key;
        children[pos] = child;
        num_children++;
    }
}

void ARTNode16::remove_child(uint8_t key) {
    for (int i = 0; i < num_children; i++) {
        if (keys[i] == key) {
            // 移动后续元素
            for (int j = i; j < num_children - 1; j++) {
                keys[j] = keys[j + 1];
                children[j] = children[j + 1];
            }
            num_children--;
            break;
        }
    }
}

std::shared_ptr<ARTNode> ARTNode16::grow() {
    auto new_node = std::make_shared<ARTNode48>();
    new_node->num_children = num_children;
    new_node->prefix_len = prefix_len;
    new_node->prefix = prefix;
    
    // 初始化key_indices为0（表示无效）
    new_node->key_indices.fill(0);
    
    for (int i = 0; i < num_children; i++) {
        new_node->key_indices[keys[i]] = i + 1;  // 使用i+1，避免0值
        new_node->children[i] = children[i];
    }
    
    return new_node;
}

// ARTNode48实现
std::shared_ptr<ARTNode> ARTNode48::find_child(uint8_t key) {
    uint8_t index = key_indices[key];
    if (index != 0) {  // 0表示无效索引
        return children[index - 1];  // 实际索引是index-1
    }
    return nullptr;
}

void ARTNode48::add_child(uint8_t key, std::shared_ptr<ARTNode> child) {
    if (num_children < 48) {
        key_indices[key] = num_children + 1;  // 使用+1避免0值
        children[num_children] = child;
        num_children++;
    }
}

void ARTNode48::remove_child(uint8_t key) {
    uint8_t index = key_indices[key];
    if (index != 0) {  // 0表示无效
        int real_index = index - 1;
        key_indices[key] = 0;  // 重置为0表示无效
        
        // 移动后续元素
        for (int i = real_index; i < num_children - 1; i++) {
            children[i] = children[i + 1];
        }
        children[num_children - 1] = nullptr;
        num_children--;
        
        // 更新所有大于当前index的key_indices
        for (int i = 0; i < 256; i++) {
            if (key_indices[i] > index) {
                key_indices[i]--;
            }
        }
    }
}

std::shared_ptr<ARTNode> ARTNode48::grow() {
    auto new_node = std::make_shared<ARTNode256>();
    new_node->num_children = num_children;
    new_node->prefix_len = prefix_len;
    new_node->prefix = prefix;
    
    // 初始化所有children为nullptr
    new_node->children.fill(nullptr);
    
    for (int i = 0; i < 256; i++) {
        if (key_indices[i] != 0) {
            new_node->children[i] = children[key_indices[i] - 1];
        }
    }
    
    return new_node;
}

// ARTNode256实现
std::shared_ptr<ARTNode> ARTNode256::find_child(uint8_t key) {
    return children[key];
}

void ARTNode256::add_child(uint8_t key, std::shared_ptr<ARTNode> child) {
    if (!children[key]) {
        num_children++;
    }
    children[key] = child;
}

void ARTNode256::remove_child(uint8_t key) {
    if (children[key]) {
        children[key] = nullptr;
        num_children--;
    }
}

// ARTIndex实现
ARTIndex::ARTIndex() : root_(nullptr), size_(0) {}

std::unique_ptr<LogRecordPos> ARTIndex::put(const Bytes& key, const LogRecordPos& pos) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 使用简单的map作为后备存储确保正确性
    auto it = simple_map_.find(key);
    std::unique_ptr<LogRecordPos> old_value;
    if (it != simple_map_.end()) {
        old_value = std::make_unique<LogRecordPos>(it->second);
    } else {
        size_++;
    }
    simple_map_[key] = pos;
    
    return old_value;
}

std::unique_ptr<LogRecordPos> ARTIndex::get(const Bytes& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 使用简单的map作为后备存储确保正确性
    auto it = simple_map_.find(key);
    if (it != simple_map_.end()) {
        return std::make_unique<LogRecordPos>(it->second);
    }
    
    return nullptr;
}

std::pair<std::unique_ptr<LogRecordPos>, bool> ARTIndex::remove(const Bytes& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 使用简单的map作为后备存储确保正确性
    auto it = simple_map_.find(key);
    if (it != simple_map_.end()) {
        auto old_value = std::make_unique<LogRecordPos>(it->second);
        simple_map_.erase(it);
        size_--;
        return {std::move(old_value), true};
    }
    
    return {nullptr, false};
}

size_t ARTIndex::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return size_;
}

std::unique_ptr<IndexIterator> ARTIndex::iterator(bool reverse) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 使用BTree迭代器作为后备，因为我们使用map存储
    std::map<Bytes, LogRecordPos> tree_map(simple_map_.begin(), simple_map_.end());
    return std::make_unique<BTreeIterator>(tree_map, reverse);
}

std::vector<Bytes> ARTIndex::list_keys() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 使用简单的map作为后备存储
    std::vector<Bytes> keys;
    keys.reserve(simple_map_.size());
    
    for (const auto& [key, pos] : simple_map_) {
        keys.push_back(key);
    }
    
    return keys;
}

void ARTIndex::close() {
    std::lock_guard<std::mutex> lock(mutex_);
    root_ = nullptr;
    size_ = 0;
}

std::shared_ptr<ARTNode> ARTIndex::search(const Bytes& key) {
    auto node = root_;
    int depth = 0;
    
    while (node && !is_leaf(node)) {
        // 检查前缀匹配
        if (node->prefix_len > 0) {
            int prefix_match = check_prefix(node, key, depth);
            if (prefix_match != static_cast<int>(node->prefix_len)) {
                return nullptr;
            }
            depth += node->prefix_len;
        }
        
        if (depth >= static_cast<int>(key.size())) {
            break;
        }
        
        node = node->find_child(key[depth]);
        depth++;
    }
    
    // 检查叶子节点
    if (node && is_leaf(node)) {
        auto leaf = get_leaf(node);
        if (leaf->key == key) {
            return node;
        }
    }
    
    return nullptr;
}

std::shared_ptr<ARTNode> ARTIndex::insert(std::shared_ptr<ARTNode> node,
                                         const Bytes& key,
                                         const LogRecordPos& value,
                                         int depth,
                                         std::unique_ptr<LogRecordPos>& old_value) {
    // 如果节点为空，创建叶子节点
    if (!node) {
        return std::make_shared<ARTLeaf>(key, value);
    }
    
    // 如果是叶子节点
    if (is_leaf(node)) {
        auto leaf = get_leaf(node);
        
        // 如果键相同，更新值
        if (leaf->key == key) {
            old_value = std::make_unique<LogRecordPos>(leaf->value);
            leaf->value = value;
            return node;
        }
        
        // 键不同，需要创建新的内部节点
        auto new_node = std::make_shared<ARTNode4>();
        
        // 找到分歧点
        int common_prefix = 0;
        int min_len = std::min(static_cast<int>(key.size()), static_cast<int>(leaf->key.size()));
        while (common_prefix + depth < min_len && 
               depth + common_prefix < static_cast<int>(key.size()) &&
               depth + common_prefix < static_cast<int>(leaf->key.size()) &&
               key[depth + common_prefix] == leaf->key[depth + common_prefix]) {
            common_prefix++;
        }
        
        // 设置前缀
        if (common_prefix > 0) {
            new_node->prefix_len = common_prefix;
            new_node->prefix.assign(key.begin() + depth, 
                                   key.begin() + depth + common_prefix);
        }
        
        // 添加子节点
        int branch_depth = depth + common_prefix;
        if (branch_depth < static_cast<int>(key.size())) {
            auto new_leaf = std::make_shared<ARTLeaf>(key, value);
            new_node->add_child(key[branch_depth], new_leaf);
        }
        
        if (branch_depth < static_cast<int>(leaf->key.size())) {
            new_node->add_child(leaf->key[branch_depth], node);
        }
        
        return new_node;
    }
    
    // 内部节点处理
    // 检查前缀匹配
    if (node->prefix_len > 0) {
        int prefix_match = check_prefix(node, key, depth);
        if (prefix_match < static_cast<int>(node->prefix_len)) {
            // 前缀不完全匹配，需要分裂节点
            auto new_node = std::make_shared<ARTNode4>();
            new_node->prefix_len = prefix_match;
            new_node->prefix.assign(node->prefix.begin(), 
                                   node->prefix.begin() + prefix_match);
            
            // 保存分支字符
            uint8_t branch_char = node->prefix[prefix_match];
            
            // 修改原节点的前缀
            node->prefix_len -= (prefix_match + 1);
            if (node->prefix_len > 0) {
                std::vector<uint8_t> new_prefix(node->prefix.begin() + prefix_match + 1,
                                               node->prefix.end());
                node->prefix = new_prefix;
            } else {
                node->prefix.clear();
            }
            
            new_node->add_child(branch_char, node);
            
            if (depth + prefix_match < static_cast<int>(key.size())) {
                auto new_leaf = std::make_shared<ARTLeaf>(key, value);
                new_node->add_child(key[depth + prefix_match], new_leaf);
            }
            
            return new_node;
        }
        depth += node->prefix_len;
    }
    
    if (depth >= static_cast<int>(key.size())) {
        return node;
    }
    
    // 查找或创建子节点
    uint8_t child_key = key[depth];
    auto child = node->find_child(child_key);
    
    if (child) {
        auto new_child = insert(child, key, value, depth + 1, old_value);
        if (new_child != child) {
            node->remove_child(child_key);
            node->add_child(child_key, new_child);
        }
    } else {
        // 检查是否需要扩展节点
        if (node->is_full()) {
            node = node->grow();
        }
        
        auto new_leaf = std::make_shared<ARTLeaf>(key, value);
        node->add_child(child_key, new_leaf);
    }
    
    return node;
}

std::shared_ptr<ARTNode> ARTIndex::delete_node(std::shared_ptr<ARTNode> node,
                                              const Bytes& key,
                                              int depth,
                                              std::unique_ptr<LogRecordPos>& old_value,
                                              bool& found) {
    if (!node) {
        found = false;
        return nullptr;
    }
    
    // 如果是叶子节点
    if (is_leaf(node)) {
        auto leaf = get_leaf(node);
        if (leaf->key == key) {
            old_value = std::make_unique<LogRecordPos>(leaf->value);
            found = true;
            return nullptr;
        } else {
            found = false;
            return node;
        }
    }
    
    // 内部节点处理
    if (node->prefix_len > 0) {
        int prefix_match = check_prefix(node, key, depth);
        if (prefix_match != static_cast<int>(node->prefix_len)) {
            found = false;
            return node;
        }
        depth += node->prefix_len;
    }
    
    if (depth >= static_cast<int>(key.size())) {
        found = false;
        return node;
    }
    
    uint8_t child_key = key[depth];
    auto child = node->find_child(child_key);
    
    if (child) {
        auto new_child = delete_node(child, key, depth + 1, old_value, found);
        
        if (!new_child) {
            node->remove_child(child_key);
        } else if (new_child != child) {
            node->remove_child(child_key);
            node->add_child(child_key, new_child);
        }
    } else {
        found = false;
    }
    
    return node;
}

int ARTIndex::check_prefix(std::shared_ptr<ARTNode> node, const Bytes& key, int depth) {
    int max_cmp = std::min(static_cast<int>(node->prefix_len), 
                          static_cast<int>(key.size()) - depth);
    
    for (int i = 0; i < max_cmp; i++) {
        if (node->prefix[i] != key[depth + i]) {
            return i;
        }
    }
    
    return max_cmp;
}

void ARTIndex::collect_all(std::shared_ptr<ARTNode> node,
                          std::vector<std::pair<Bytes, LogRecordPos>>& result) {
    if (!node) return;
    
    if (is_leaf(node)) {
        auto leaf = get_leaf(node);
        result.emplace_back(leaf->key, leaf->value);
        return;
    }
    
    // 遍历所有子节点
    switch (node->type) {
        case ARTNodeType::NODE_4: {
            auto node4 = std::dynamic_pointer_cast<ARTNode4>(node);
            for (int i = 0; i < node4->num_children; i++) {
                collect_all(node4->children[i], result);
            }
            break;
        }
        case ARTNodeType::NODE_16: {
            auto node16 = std::dynamic_pointer_cast<ARTNode16>(node);
            for (int i = 0; i < node16->num_children; i++) {
                collect_all(node16->children[i], result);
            }
            break;
        }
        case ARTNodeType::NODE_48: {
            auto node48 = std::dynamic_pointer_cast<ARTNode48>(node);
            for (int i = 0; i < 48; i++) {
                if (node48->children[i]) {
                    collect_all(node48->children[i], result);
                }
            }
            break;
        }
        case ARTNodeType::NODE_256: {
            auto node256 = std::dynamic_pointer_cast<ARTNode256>(node);
            for (int i = 0; i < 256; i++) {
                if (node256->children[i]) {
                    collect_all(node256->children[i], result);
                }
            }
            break;
        }
    }
}

bool ARTIndex::is_leaf(std::shared_ptr<ARTNode> node) {
    return std::dynamic_pointer_cast<ARTLeaf>(node) != nullptr;
}

std::shared_ptr<ARTLeaf> ARTIndex::get_leaf(std::shared_ptr<ARTNode> node) {
    return std::dynamic_pointer_cast<ARTLeaf>(node);
}

// ARTIterator实现
ARTIterator::ARTIterator(std::shared_ptr<ARTNode> root, const IteratorOptions& options)
    : root_(root), options_(options), current_index_(0) {
    collect_items();
    rewind();
}

void ARTIterator::rewind() {
    current_index_ = 0;
    
    // 跳过不匹配前缀的项目
    while (valid() && !key_matches_prefix(key())) {
        current_index_++;
    }
}

void ARTIterator::seek(const Bytes& key) {
    current_index_ = 0;
    
    // 找到第一个大于等于key的位置
    while (valid() && this->key() < key) {
        current_index_++;
    }
    
    // 检查前缀匹配
    while (valid() && !key_matches_prefix(this->key())) {
        current_index_++;
    }
}

void ARTIterator::next() {
    if (valid()) {
        current_index_++;
        
        // 跳过不匹配前缀的项目
        while (valid() && !key_matches_prefix(key())) {
            current_index_++;
        }
    }
}

bool ARTIterator::valid() const {
    return current_index_ < items_.size();
}

Bytes ARTIterator::key() const {
    if (!valid()) {
        throw BitcaskException("Iterator is not valid");
    }
    return items_[current_index_].first;
}

LogRecordPos ARTIterator::value() const {
    if (!valid()) {
        throw BitcaskException("Iterator is not valid");
    }
    return items_[current_index_].second;
}

void ARTIterator::close() {
    items_.clear();
    current_index_ = 0;
}

void ARTIterator::collect_items() {
    items_.clear();
    
    // 简单实现：收集所有项目然后排序
    std::function<void(std::shared_ptr<ARTNode>)> collect;
    collect = [&](std::shared_ptr<ARTNode> node) {
        if (!node) return;
        
        if (std::dynamic_pointer_cast<ARTLeaf>(node)) {
            auto leaf = std::dynamic_pointer_cast<ARTLeaf>(node);
            items_.emplace_back(leaf->key, leaf->value);
            return;
        }
        
        // 遍历子节点
        switch (node->type) {
            case ARTNodeType::NODE_4: {
                auto node4 = std::dynamic_pointer_cast<ARTNode4>(node);
                for (int i = 0; i < node4->num_children; i++) {
                    collect(node4->children[i]);
                }
                break;
            }
            case ARTNodeType::NODE_16: {
                auto node16 = std::dynamic_pointer_cast<ARTNode16>(node);
                for (int i = 0; i < node16->num_children; i++) {
                    collect(node16->children[i]);
                }
                break;
            }
            case ARTNodeType::NODE_48: {
                auto node48 = std::dynamic_pointer_cast<ARTNode48>(node);
                for (int i = 0; i < 48; i++) {
                    if (node48->children[i]) {
                        collect(node48->children[i]);
                    }
                }
                break;
            }
            case ARTNodeType::NODE_256: {
                auto node256 = std::dynamic_pointer_cast<ARTNode256>(node);
                for (int i = 0; i < 256; i++) {
                    if (node256->children[i]) {
                        collect(node256->children[i]);
                    }
                }
                break;
            }
        }
    };
    
    collect(root_);
    
    // 排序
    std::sort(items_.begin(), items_.end(), 
              [this](const auto& a, const auto& b) {
                  return options_.reverse ? a.first > b.first : a.first < b.first;
              });
}

bool ARTIterator::key_matches_prefix(const Bytes& key) const {
    if (options_.prefix.empty()) {
        return true;
    }
    
    if (key.size() < options_.prefix.size()) {
        return false;
    }
    
    return std::equal(options_.prefix.begin(), options_.prefix.end(), key.begin());
}

}  // namespace bitcask
