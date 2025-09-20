cmake_minimum_required(VERSION 3.16)
project(bitcask-cpp VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置编译选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O2 -D_GNU_SOURCE")

# 针对Ubuntu 22.04的特殊优化
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -fPIC")
    # 添加对 fdatasync 的支持
    add_definitions(-D_POSIX_C_SOURCE=200112L)
    add_definitions(-D_DEFAULT_SOURCE)
endif()

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

# 添加 Google Test 支持
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/release-1.12.1.zip
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

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

# 完整服务器示例（HTTP + Redis）
add_executable(complete_server examples/complete_server.cpp)
target_link_libraries(complete_server bitcask)

# RPC服务器示例
add_executable(rpc_server_example examples/rpc_server_example.cpp)
target_link_libraries(rpc_server_example bitcask)

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
    test_art_index
    test_mmap_io
    test_test_utils
)

# 创建每个单独的测试可执行文件
foreach(test_name ${UNIT_TEST_FILES})
    add_executable(${test_name} tests/unit_tests/${test_name}.cpp)
    target_link_libraries(${test_name} bitcask gtest_main gmock_main)
    
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
target_link_libraries(unit_tests bitcask gtest_main gmock_main)

# 添加测试发现
include(GoogleTest)
gtest_discover_tests(unit_tests)

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
