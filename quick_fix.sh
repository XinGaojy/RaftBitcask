#!/bin/bash

# 快速修复脚本 - 解决GoogleTest下载问题
# 这个脚本会创建一个简化的本地GoogleTest环境

echo "🔧 开始修复GoogleTest下载问题..."

# 创建本地GoogleTest目录
mkdir -p local_gtest/include/gtest
mkdir -p local_gtest/include/gmock
mkdir -p local_gtest/src

# 创建简化的gtest.h头文件
cat > local_gtest/include/gtest/gtest.h << 'EOF'
#pragma once

#include <iostream>
#include <functional>
#include <vector>
#include <string>
#include <stdexcept>

// 简化的GoogleTest实现
namespace testing {

class Test {
public:
    virtual ~Test() = default;
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
    static TestRegistry& GetInstance() {
        static TestRegistry instance;
        return instance;
    }
    
    void AddTest(const std::string& test_case, const std::string& test_name, std::function<Test*()> factory) {
        tests.push_back({test_case, test_name, factory});
    }
    
    int RunAllTests() {
        int failures = 0;
        std::cout << "[==========] Running " << tests.size() << " tests.\n";
        
        for (const auto& test : tests) {
            std::cout << "[ RUN      ] " << test.test_case_name << "." << test.test_name << "\n";
            
            try {
                auto* test_instance = test.factory();
                test_instance->SetUp();
                test_instance->TestBody();
                test_instance->TearDown();
                delete test_instance;
                std::cout << "[       OK ] " << test.test_case_name << "." << test.test_name << "\n";
            } catch (const std::exception& e) {
                std::cout << "[  FAILED  ] " << test.test_case_name << "." << test.test_name << " - " << e.what() << "\n";
                failures++;
            }
        }
        
        std::cout << "[==========] " << tests.size() << " tests ran.\n";
        if (failures == 0) {
            std::cout << "[  PASSED  ] " << tests.size() << " tests.\n";
        } else {
            std::cout << "[  FAILED  ] " << failures << " tests.\n";
        }
        
        return failures;
    }
    
private:
    std::vector<TestInfo> tests;
};

// 测试注册宏
#define TEST(test_case_name, test_name) \
    class test_case_name##_##test_name##_Test : public ::testing::Test { \
    public: \
        test_case_name##_##test_name##_Test() {} \
    private: \
        virtual void TestBody(); \
        static ::testing::Test* Create() { \
            return new test_case_name##_##test_name##_Test; \
        } \
        static int registered_; \
    }; \
    int test_case_name##_##test_name##_Test::registered_ = \
        (::testing::TestRegistry::GetInstance().AddTest(#test_case_name, #test_name, \
         test_case_name##_##test_name##_Test::Create), 0); \
    void test_case_name##_##test_name##_Test::TestBody()

// 断言宏
#define EXPECT_EQ(expected, actual) \
    do { \
        if ((expected) != (actual)) { \
            throw std::runtime_error("Expected: " + std::to_string(expected) + ", Actual: " + std::to_string(actual)); \
        } \
    } while (0)

#define EXPECT_TRUE(condition) \
    do { \
        if (!(condition)) { \
            throw std::runtime_error("Expected true, but got false"); \
        } \
    } while (0)

#define EXPECT_FALSE(condition) \
    do { \
        if (condition) { \
            throw std::runtime_error("Expected false, but got true"); \
        } \
    } while (0)

#define EXPECT_NE(not_expected, actual) \
    do { \
        if ((not_expected) == (actual)) { \
            throw std::runtime_error("Expected not equal, but values are equal"); \
        } \
    } while (0)

#define ASSERT_EQ EXPECT_EQ
#define ASSERT_TRUE EXPECT_TRUE
#define ASSERT_FALSE EXPECT_FALSE
#define ASSERT_NE EXPECT_NE

} // namespace testing

// 全局函数
int RUN_ALL_TESTS() {
    return testing::TestRegistry::GetInstance().RunAllTests();
}
EOF

# 创建简化的gmock.h头文件
cat > local_gtest/include/gmock/gmock.h << 'EOF'
#pragma once

#include <gtest/gtest.h>

// 简化的GMock实现
namespace testing {

// 占位符，大多数情况下我们不需要完整的GMock功能
#define MOCK_METHOD(ret, name, args) \
    virtual ret name args { return ret{}; }

} // namespace testing
EOF

# 创建gtest_main.cpp
cat > local_gtest/src/gtest_main.cpp << 'EOF'
#include <gtest/gtest.h>

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    return RUN_ALL_TESTS();
}
EOF

# 创建修改后的CMakeLists.txt
cat > CMakeLists_fixed.txt << 'EOF'
cmake_minimum_required(VERSION 3.16)
project(bitcask-cpp VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置编译选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O2")

# 查找依赖
find_package(Threads REQUIRED)

# 尝试查找 crc32c，如果找不到则使用 zlib
find_package(PkgConfig)
if(PkgConfig_FOUND)
    pkg_check_modules(CRC32C libcrc32c)
endif()

if(NOT CRC32C_FOUND)
    find_package(ZLIB REQUIRED)
    add_definitions(-DUSE_ZLIB_CRC32)
endif()

# 使用本地GoogleTest
set(GTEST_ROOT ${CMAKE_SOURCE_DIR}/local_gtest)
include_directories(${GTEST_ROOT}/include)

# 创建gtest库
add_library(gtest STATIC ${GTEST_ROOT}/src/gtest_main.cpp)
target_include_directories(gtest PUBLIC ${GTEST_ROOT}/include)

# 创建gtest_main和gmock_main别名
add_library(gtest_main ALIAS gtest)
add_library(gmock_main ALIAS gtest)

# 包含目录
include_directories(${CMAKE_SOURCE_DIR}/include)

# 源文件
file(GLOB_RECURSE SOURCES "src/*.cpp")

# 创建静态库
add_library(bitcask STATIC ${SOURCES})

# 链接依赖库
if(CRC32C_FOUND)
    target_link_libraries(bitcask Threads::Threads ${CRC32C_LIBRARIES})
    target_include_directories(bitcask PRIVATE ${CRC32C_INCLUDE_DIRS})
    target_compile_options(bitcask PRIVATE ${CRC32C_CFLAGS_OTHER})
else()
    target_link_libraries(bitcask Threads::Threads ZLIB::ZLIB)
endif()

# 设置头文件包含路径
target_include_directories(bitcask PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# 示例程序
add_executable(bitcask_example examples/basic_operations.cpp)
target_link_libraries(bitcask_example bitcask)

# HTTP服务器示例
add_executable(http_server_example examples/http_server_example.cpp)
target_link_libraries(http_server_example bitcask)

# Redis服务器示例
add_executable(redis_server_example examples/redis_server_example.cpp)
target_link_libraries(redis_server_example bitcask)

# 单元测试
enable_testing()

# 为每个测试文件创建独立的可执行文件
set(UNIT_TEST_FILES
    test_log_record
    test_io_manager
    test_data_file
    test_index
    test_db
    test_write_batch
    test_iterator
    test_merge
    test_http_server
    test_redis
    test_backup
    test_advanced_index
)

# 创建每个单独的测试可执行文件
foreach(test_name ${UNIT_TEST_FILES})
    add_executable(${test_name} tests/unit_tests/${test_name}.cpp)
    target_link_libraries(${test_name} bitcask gtest_main)
    
    # 添加到测试套件
    add_test(NAME ${test_name} COMMAND ${test_name})
    
    # 设置测试属性
    set_tests_properties(${test_name} PROPERTIES
        TIMEOUT 300
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
endforeach()

# 收集所有测试文件创建完整的单元测试套件
file(GLOB_RECURSE TEST_SOURCES "tests/unit_tests/*.cpp")
add_executable(unit_tests ${TEST_SOURCES})
target_link_libraries(unit_tests bitcask gtest_main)

# 集成测试
add_executable(integration_tests tests/integration_tests/integration_test.cpp)
target_link_libraries(integration_tests bitcask gtest_main)

# 性能测试
add_executable(benchmark_tests tests/benchmark_tests/benchmark_test.cpp)
target_link_libraries(benchmark_tests bitcask gtest_main)

# 原有的简单测试程序
add_executable(bitcask_test tests/test_main.cpp)
target_link_libraries(bitcask_test bitcask)

# 安装规则
install(TARGETS bitcask
    EXPORT bitcask-targets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

install(DIRECTORY include/ DESTINATION include)
EOF

echo "✅ 本地GoogleTest环境创建完成"
echo ""
echo "🔧 使用方法："
echo "1. cd到项目根目录"
echo "2. 备份原CMakeLists.txt: cp CMakeLists.txt CMakeLists_original.txt"
echo "3. 使用修复版本: cp CMakeLists_fixed.txt CMakeLists.txt"
echo "4. 清理构建目录: rm -rf build && mkdir build && cd build"
echo "5. 重新配置和编译: cmake .. -DCMAKE_BUILD_TYPE=Release && make -j\$(nproc)"
echo ""
echo "🎯 这个方案完全离线，不需要网络连接下载GoogleTest"

chmod +x quick_fix.sh
echo "✅ quick_fix.sh 脚本已创建并设置为可执行"