#include "bitcask/art_index.h"
#include <algorithm>
#include <cstring>

namespace bitcask {

// ARTNode256实现 - 完整版本
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

// ARTNode256的grow方法已经在头文件中定义为返回nullptr
// 所以这里不需要重新定义

}  // namespace bitcask
