#pragma once

#include <gtest/gtest.h>

// 简化的GMock实现
namespace testing {

// 占位符，大多数情况下我们不需要完整的GMock功能
#define MOCK_METHOD(ret, name, args) \
    virtual ret name args { return ret{}; }

} // namespace testing
