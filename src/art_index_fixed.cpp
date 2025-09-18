#include "bitcask/art_index.h"
#include <algorithm>
#include <cstring>

namespace bitcask {

// ARTNode48实现 - 修正版本
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

}  // namespace bitcask
