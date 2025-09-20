#!/bin/bash

# Bitcask C++ 依赖安装脚本 for Ubuntu 22.04

set -e

echo "=== Bitcask C++ 依赖安装脚本 ==="

# 更新系统包
echo "1. 更新系统包..."
apt update

# 安装基础编译工具
echo "2. 安装编译工具..."
apt install -y build-essential cmake git pkg-config

# 安装必要的依赖库
echo "3. 安装依赖库..."
apt install -y zlib1g-dev libssl-dev

# 尝试安装libcrc32c（如果可用）
echo "4. 尝试安装libcrc32c..."
apt install -y libcrc32c-dev || echo "libcrc32c-dev 不可用，将使用zlib替代"

# 手动安装GoogleTest（如果网络下载失败）
echo "5. 准备GoogleTest..."
GTEST_DIR="/tmp/googletest"
if [ ! -d "$GTEST_DIR" ]; then
    echo "创建GoogleTest目录..."
    mkdir -p "$GTEST_DIR"
    cd "$GTEST_DIR"
    
    # 尝试从GitHub下载
    echo "尝试下载GoogleTest..."
    if ! wget -O gtest.zip "https://github.com/google/googletest/archive/release-1.12.1.zip" 2>/dev/null; then
        echo "GitHub下载失败，尝试其他镜像..."
        if ! wget -O gtest.zip "https://gitee.com/mirrors/googletest/repository/archive/release-1.12.1.zip" 2>/dev/null; then
            echo "所有下载源都失败，将创建本地GoogleTest实现"
            create_local_gtest
        fi
    fi
    
    if [ -f "gtest.zip" ]; then
        unzip -q gtest.zip || echo "解压失败"
        if [ -d "googletest-release-1.12.1" ]; then
            mv googletest-release-1.12.1/* .
        fi
    fi
fi

echo "6. 验证依赖..."
echo "GCC版本: $(gcc --version | head -n1)"
echo "CMake版本: $(cmake --version | head -n1)"
echo "依赖安装完成！"

# 创建本地GoogleTest实现的函数
create_local_gtest() {
    echo "创建简化的GoogleTest实现..."
    
    # 创建gtest目录结构
    mkdir -p include/gtest include/gmock
    mkdir -p src
    
    # 创建简化的gtest.h
    cat > include/gtest/gtest.h << 'EOF'
#ifndef GTEST_INCLUDE_GTEST_GTEST_H_
#define GTEST_INCLUDE_GTEST_GTEST_H_

#include <iostream>
#include <string>
#include <vector>
#include <functional>

// 简化的GoogleTest实现
namespace testing {

class Test {
public:
    virtual ~Test() {}
    virtual void SetUp() {}
    virtual void TearDown() {}
    virtual void TestBody() = 0;
};

struct TestInfo {
    std::string test_case_name;
    std::string test_name;
    std::function<Test*()> factory;
};

class TestRegistry {
public:
    static TestRegistry& Instance() {
        static TestRegistry instance;
        return instance;
    }
    
    void RegisterTest(const std::string& test_case, const std::string& test_name, 
                     std::function<Test*()> factory) {
        tests_.push_back({test_case, test_name, factory});
    }
    
    int RunAllTests() {
        int failed = 0;
        for (const auto& test : tests_) {
            std::cout << "[ RUN      ] " << test.test_case_name << "." << test.test_name << std::endl;
            try {
                auto* test_instance = test.factory();
                test_instance->SetUp();
                test_instance->TestBody();
                test_instance->TearDown();
                delete test_instance;
                std::cout << "[       OK ] " << test.test_case_name << "." << test.test_name << std::endl;
            } catch (const std::exception& e) {
                std::cout << "[  FAILED  ] " << test.test_case_name << "." << test.test_name 
                         << " - " << e.what() << std::endl;
                failed++;
            }
        }
        return failed;
    }
    
private:
    std::vector<TestInfo> tests_;
};

class AssertionException : public std::exception {
public:
    AssertionException(const std::string& msg) : msg_(msg) {}
    const char* what() const noexcept override { return msg_.c_str(); }
private:
    std::string msg_;
};

} // namespace testing

#define TEST(test_case_name, test_name) \
    class test_case_name##_##test_name##_Test : public ::testing::Test { \
    public: \
        test_case_name##_##test_name##_Test() {} \
    private: \
        virtual void TestBody(); \
        static ::testing::Test* Create() { \
            return new test_case_name##_##test_name##_Test; \
        } \
        static int register_; \
    }; \
    int test_case_name##_##test_name##_Test::register_ = \
        (::testing::TestRegistry::Instance().RegisterTest( \
            #test_case_name, #test_name, \
            test_case_name##_##test_name##_Test::Create), 0); \
    void test_case_name##_##test_name##_Test::TestBody()

#define TEST_F(test_fixture, test_name) \
    class test_fixture##_##test_name##_Test : public test_fixture { \
    public: \
        test_fixture##_##test_name##_Test() {} \
    private: \
        virtual void TestBody(); \
        static ::testing::Test* Create() { \
            return new test_fixture##_##test_name##_Test; \
        } \
        static int register_; \
    }; \
    int test_fixture##_##test_name##_Test::register_ = \
        (::testing::TestRegistry::Instance().RegisterTest( \
            #test_fixture, #test_name, \
            test_fixture##_##test_name##_Test::Create), 0); \
    void test_fixture##_##test_name##_Test::TestBody()

#define EXPECT_EQ(expected, actual) \
    if ((expected) != (actual)) { \
        throw ::testing::AssertionException("Expected: " + std::to_string(expected) + \
                                          ", Actual: " + std::to_string(actual)); \
    }

#define EXPECT_NE(val1, val2) \
    if ((val1) == (val2)) { \
        throw ::testing::AssertionException("Expected not equal"); \
    }

#define EXPECT_TRUE(condition) \
    if (!(condition)) { \
        throw ::testing::AssertionException("Expected true"); \
    }

#define EXPECT_FALSE(condition) \
    if (condition) { \
        throw ::testing::AssertionException("Expected false"); \
    }

#define EXPECT_THROW(statement, exception_type) \
    try { \
        statement; \
        throw ::testing::AssertionException("Expected exception not thrown"); \
    } catch (const exception_type&) { \
        /* Expected exception caught */ \
    }

#define EXPECT_NO_THROW(statement) \
    try { \
        statement; \
    } catch (...) { \
        throw ::testing::AssertionException("Unexpected exception thrown"); \
    }

#define ASSERT_EQ(expected, actual) EXPECT_EQ(expected, actual)
#define ASSERT_NE(val1, val2) EXPECT_NE(val1, val2)
#define ASSERT_TRUE(condition) EXPECT_TRUE(condition)
#define ASSERT_FALSE(condition) EXPECT_FALSE(condition)

int main(int argc, char** argv) {
    return ::testing::TestRegistry::Instance().RunAllTests();
}

#endif  // GTEST_INCLUDE_GTEST_GTEST_H_
EOF

    # 创建简化的gmock.h
    cat > include/gmock/gmock.h << 'EOF'
#ifndef GMOCK_INCLUDE_GMOCK_GMOCK_H_
#define GMOCK_INCLUDE_GMOCK_GMOCK_H_

#include "gtest/gtest.h"

// 简化的GMock实现 - 基本上是空的，因为我们的测试不需要mock
namespace testing {
    // 空的GMock命名空间
}

#endif  // GMOCK_INCLUDE_GMOCK_GMOCK_H_
EOF

    # 创建空的源文件
    touch src/gtest_main.cc
    
    echo "本地GoogleTest实现创建完成"
}
