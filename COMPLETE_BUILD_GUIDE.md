# Bitcask C++ 完整编译运行指南 (Ubuntu 22.04)

本指南提供了在Ubuntu 22.04上编译运行完整Bitcask C++项目的详细步骤，包括所有新增模块的手动编译和测试方法。

## 项目功能概述

### ✅ 已实现的完整功能模块

1. **核心存储引擎** - 基础键值存储功能
2. **数据合并(Merge)功能** - 清理无效数据，生成Hint文件
3. **HTTP服务器模块** - REST API接口
4. **Redis协议兼容模块** - 支持Redis数据结构和协议
5. **数据备份功能** - 数据库快照和恢复
6. **高级索引类型** - SkipList跳表索引和B+树磁盘索引
7. **完整的单元测试套件** - 覆盖所有模块

## 1. 系统准备

### 1.1 安装基础依赖
```bash
# 更新系统
sudo apt update && sudo apt upgrade -y

# 安装编译工具链
sudo apt install -y build-essential cmake git pkg-config

# 安装项目依赖
sudo apt install -y libcrc32c-dev zlib1g-dev libpthread-stubs0-dev

# 安装Redis工具（用于测试Redis兼容性）
sudo apt install -y redis-tools

# 验证安装
gcc --version        # 应该是 >= 9.0
g++ --version        # 应该是 >= 9.0  
cmake --version      # 应该是 >= 3.16
```

## 2. 编译项目

### 2.1 配置和编译
```bash
# 进入项目目录
cd kv-projects/bitcask-cpp

# 创建构建目录
mkdir -p build
cd build

# 配置项目（Release版本）
cmake .. -DCMAKE_BUILD_TYPE=Release

# 编译项目（使用所有CPU核心）
make -j$(nproc)

# 验证编译结果
ls -la | grep -E "(bitcask|test|example)"
```

### 2.2 编译产物说明
编译完成后，你将得到以下可执行文件：
- `libbitcask.a` - 静态库
- `bitcask_example` - 基本操作示例
- `http_server_example` - HTTP服务器示例  
- `redis_server_example` - Redis服务器示例
- `unit_tests` - 完整单元测试套件
- `test_*` - 各个模块的独立测试

## 3. 手动编译和运行单元测试

### 3.1 核心模块测试

#### 日志记录模块测试
```bash
# 手动编译
g++ -std=c++17 -I../include -I./_deps/googletest-src/googletest/include \
    -I./_deps/googletest-src/googlemock/include \
    ../tests/unit_tests/test_log_record.cpp \
    -L. -lbitcask -L./_deps/googletest-build/lib -lgtest_main -lgmock_main \
    -lpthread -lz -o test_log_record_manual

# 运行测试
./test_log_record_manual

# 预期输出：所有测试PASS，测试编码解码、CRC校验等功能
```

#### 数据文件模块测试
```bash
# 手动编译
g++ -std=c++17 -I../include -I./_deps/googletest-src/googletest/include \
    -I./_deps/googletest-src/googlemock/include \
    ../tests/unit_tests/test_data_file.cpp \
    -L. -lbitcask -L./_deps/googletest-build/lib -lgtest_main -lgmock_main \
    -lpthread -lz -o test_data_file_manual

# 运行测试
./test_data_file_manual

# 预期输出：测试文件读写、Hint文件、合并标记文件等功能
```

#### 索引模块测试
```bash
# 手动编译
g++ -std=c++17 -I../include -I./_deps/googletest-src/googletest/include \
    -I./_deps/googletest-src/googlemock/include \
    ../tests/unit_tests/test_index.cpp \
    -L. -lbitcask -L./_deps/googletest-build/lib -lgtest_main -lgmock_main \
    -lpthread -lz -o test_index_manual

# 运行测试
./test_index_manual

# 预期输出：测试BTree索引的put/get/remove操作
```

### 3.2 高级功能测试

#### 数据合并功能测试
```bash
# 手动编译
g++ -std=c++17 -I../include -I./_deps/googletest-src/googletest/include \
    -I./_deps/googletest-src/googlemock/include \
    ../tests/unit_tests/test_merge.cpp \
    -L. -lbitcask -L./_deps/googletest-build/lib -lgtest_main -lgmock_main \
    -lpthread -lz -o test_merge_manual

# 运行测试
./test_merge_manual

# 预期输出：
# [==========] Running 7 tests from 1 test suite.
# [----------] 7 tests from MergeTest
# [ RUN      ] MergeTest.MergeEmptyDatabase
# [       OK ] MergeTest.MergeEmptyDatabase (XX ms)
# [ RUN      ] MergeTest.BasicMerge
# [       OK ] MergeTest.BasicMerge (XX ms)
# ... 所有测试应该PASS
```

#### 备份功能测试
```bash
# 手动编译
g++ -std=c++17 -I../include -I./_deps/googletest-src/googletest/include \
    -I./_deps/googletest-src/googlemock/include \
    ../tests/unit_tests/test_backup.cpp \
    -L. -lbitcask -L./_deps/googletest-build/lib -lgtest_main -lgmock_main \
    -lpthread -lz -o test_backup_manual

# 运行测试
./test_backup_manual

# 预期输出：测试备份、恢复、大量数据备份等功能
```

#### 高级索引测试
```bash
# 手动编译
g++ -std=c++17 -I../include -I./_deps/googletest-src/googletest/include \
    -I./_deps/googletest-src/googlemock/include \
    ../tests/unit_tests/test_advanced_index.cpp \
    -L. -lbitcask -L./_deps/googletest-build/lib -lgtest_main -lgmock_main \
    -lpthread -lz -o test_advanced_index_manual

# 运行测试
./test_advanced_index_manual

# 预期输出：测试SkipList索引和B+树索引功能
```

### 3.3 网络服务测试

#### HTTP服务器测试
```bash
# 手动编译
g++ -std=c++17 -I../include -I./_deps/googletest-src/googletest/include \
    -I./_deps/googletest-src/googlemock/include \
    ../tests/unit_tests/test_http_server.cpp \
    -L. -lbitcask -L./_deps/googletest-build/lib -lgtest_main -lgmock_main \
    -lpthread -lz -o test_http_server_manual

# 运行测试
./test_http_server_manual

# 预期输出：测试HTTP API的PUT、GET、DELETE等操作
```

#### Redis协议测试
```bash
# 手动编译  
g++ -std=c++17 -I../include -I./_deps/googletest-src/googletest/include \
    -I./_deps/googletest-src/googlemock/include \
    ../tests/unit_tests/test_redis.cpp \
    -L. -lbitcask -L./_deps/googletest-build/lib -lgtest_main -lgmock_main \
    -lpthread -lz -o test_redis_manual

# 运行测试
./test_redis_manual

# 预期输出：测试Redis数据结构和协议解析功能
```

## 4. 运行示例程序

### 4.1 基本操作示例
```bash
# 编译基本示例
g++ -std=c++17 -I../include ../examples/basic_operations.cpp \
    -L. -lbitcask -lpthread -lz -o basic_example_manual

# 运行示例
./basic_example_manual

# 预期输出：演示基本的put/get/delete操作
```

### 4.2 HTTP服务器示例
```bash
# 编译HTTP服务器示例
g++ -std=c++17 -I../include ../examples/http_server_example.cpp \
    -L. -lbitcask -lpthread -lz -o http_server_manual

# 在后台运行HTTP服务器
./http_server_manual &
HTTP_PID=$!

# 等待服务器启动
sleep 2

# 测试HTTP API
echo "测试PUT操作:"
curl -X POST -d '{"test_key":"test_value","key2":"value2"}' \
     -H "Content-Type: application/json" \
     http://localhost:8080/bitcask/put

echo -e "\n测试GET操作:"
curl "http://localhost:8080/bitcask/get?key=test_key"

echo -e "\n测试LIST KEYS:"
curl "http://localhost:8080/bitcask/listkeys"

echo -e "\n测试STAT:"
curl "http://localhost:8080/bitcask/stat"

echo -e "\n测试DELETE:"
curl -X DELETE "http://localhost:8080/bitcask/delete?key=test_key"

# 停止服务器
kill $HTTP_PID

# 预期输出：
# 测试PUT操作:
# {"status": "OK"}
# 测试GET操作:  
# "test_value"
# 测试LIST KEYS:
# ["test_key","key2"]
# 测试STAT:
# {"key_num":2,"data_file_num":1,"reclaimable_size":0,"disk_size":XXX}
```

### 4.3 Redis服务器示例
```bash
# 编译Redis服务器示例
g++ -std=c++17 -I../include ../examples/redis_server_example.cpp \
    -L. -lbitcask -lpthread -lz -o redis_server_manual

# 在后台运行Redis服务器
./redis_server_manual &
REDIS_PID=$!

# 等待服务器启动
sleep 2

# 使用redis-cli测试
echo "测试Redis协议兼容性:"

# String操作
redis-cli -p 6380 SET test_string "hello world"
redis-cli -p 6380 GET test_string

# Hash操作
redis-cli -p 6380 HSET test_hash field1 value1
redis-cli -p 6380 HGET test_hash field1

# Set操作
redis-cli -p 6380 SADD test_set member1
redis-cli -p 6380 SISMEMBER test_set member1

# List操作
redis-cli -p 6380 LPUSH test_list element1
redis-cli -p 6380 LPOP test_list

# 通用操作
redis-cli -p 6380 EXISTS test_string
redis-cli -p 6380 TYPE test_hash
redis-cli -p 6380 PING

# 停止服务器
kill $REDIS_PID

# 预期输出：
# OK
# "hello world"
# (integer) 1
# "value1"
# (integer) 1
# (integer) 1
# "element1"
# (integer) 1
# hash
# PONG
```

## 5. 性能测试和基准测试

### 5.1 运行性能测试
```bash
# 编译并运行基准测试
g++ -std=c++17 -I../include ../tests/benchmark_tests/benchmark_test.cpp \
    -L. -lbitcask -L./_deps/googletest-build/lib -lgtest_main \
    -lpthread -lz -o benchmark_manual

./benchmark_manual

# 预期输出：各种操作的性能数据
```

### 5.2 不同索引类型性能比较
```bash
# 创建索引性能测试程序
cat > index_performance_test.cpp << 'EOF'
#include "bitcask/bitcask.h"
#include <chrono>
#include <iostream>

using namespace bitcask;

int main() {
    const int DATA_SIZE = 10000;
    
    // 测试BTree索引
    {
        Options options = Options::default_options();
        options.dir_path = "/tmp/btree_perf_test";
        options.index_type = IndexType::BTREE;
        system("rm -rf /tmp/btree_perf_test");
        
        auto start = std::chrono::high_resolution_clock::now();
        auto db = DB::open(options);
        
        for (int i = 0; i < DATA_SIZE; ++i) {
            std::string key = "key" + std::to_string(i);
            std::string value = "value" + std::to_string(i);
            db->put(Bytes(key.begin(), key.end()), Bytes(value.begin(), value.end()));
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        std::cout << "BTree索引写入 " << DATA_SIZE << " 条记录耗时: " << duration.count() << "ms" << std::endl;
        
        db->close();
        system("rm -rf /tmp/btree_perf_test");
    }
    
    // 测试SkipList索引
    {
        Options options = Options::default_options();
        options.dir_path = "/tmp/skiplist_perf_test";
        options.index_type = IndexType::SKIPLIST;
        system("rm -rf /tmp/skiplist_perf_test");
        
        auto start = std::chrono::high_resolution_clock::now();
        auto db = DB::open(options);
        
        for (int i = 0; i < DATA_SIZE; ++i) {
            std::string key = "key" + std::to_string(i);
            std::string value = "value" + std::to_string(i);
            db->put(Bytes(key.begin(), key.end()), Bytes(value.begin(), value.end()));
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        std::cout << "SkipList索引写入 " << DATA_SIZE << " 条记录耗时: " << duration.count() << "ms" << std::endl;
        
        db->close();
        system("rm -rf /tmp/skiplist_perf_test");
    }
    
    return 0;
}
EOF

# 编译性能测试
g++ -std=c++17 -I../include index_performance_test.cpp \
    -L. -lbitcask -lpthread -lz -o index_performance_manual

# 运行性能测试
./index_performance_manual

# 清理
rm index_performance_test.cpp index_performance_manual
```

## 6. 功能验证测试

### 6.1 完整功能验证脚本
```bash
# 创建功能验证脚本
cat > full_feature_test.cpp << 'EOF'
#include "bitcask/bitcask.h"
#include "bitcask/http_server.h"
#include "bitcask/redis_data_structure.h"
#include <iostream>
#include <thread>
#include <chrono>

using namespace bitcask;

int main() {
    std::cout << "=== Bitcask C++ 完整功能验证 ===" << std::endl;
    
    // 1. 测试基本存储功能
    std::cout << "\n1. 测试基本存储功能..." << std::endl;
    {
        Options options = Options::default_options();
        options.dir_path = "/tmp/feature_test_basic";
        system("rm -rf /tmp/feature_test_basic");
        
        auto db = DB::open(options);
        
        // 写入数据
        db->put(Bytes{'k', 'e', 'y', '1'}, Bytes{'v', 'a', 'l', 'u', 'e', '1'});
        
        // 读取数据
        auto value = db->get(Bytes{'k', 'e', 'y', '1'});
        std::cout << "   存储和读取: " << (value.size() == 6 ? "✓ 通过" : "✗ 失败") << std::endl;
        
        db->close();
        system("rm -rf /tmp/feature_test_basic");
    }
    
    // 2. 测试不同索引类型
    std::cout << "\n2. 测试索引类型..." << std::endl;
    
    // BTree索引
    {
        Options options = Options::default_options();
        options.dir_path = "/tmp/feature_test_btree";
        options.index_type = IndexType::BTREE;
        system("rm -rf /tmp/feature_test_btree");
        
        auto db = DB::open(options);
        db->put(Bytes{'b', 't', 'r', 'e', 'e'}, Bytes{'t', 'e', 's', 't'});
        auto value = db->get(Bytes{'b', 't', 'r', 'e', 'e'});
        std::cout << "   BTree索引: " << (value.size() == 4 ? "✓ 通过" : "✗ 失败") << std::endl;
        db->close();
        system("rm -rf /tmp/feature_test_btree");
    }
    
    // SkipList索引
    {
        Options options = Options::default_options();
        options.dir_path = "/tmp/feature_test_skiplist";
        options.index_type = IndexType::SKIPLIST;
        system("rm -rf /tmp/feature_test_skiplist");
        
        auto db = DB::open(options);
        db->put(Bytes{'s', 'k', 'i', 'p'}, Bytes{'t', 'e', 's', 't'});
        auto value = db->get(Bytes{'s', 'k', 'i', 'p'});
        std::cout << "   SkipList索引: " << (value.size() == 4 ? "✓ 通过" : "✗ 失败") << std::endl;
        db->close();
        system("rm -rf /tmp/feature_test_skiplist");
    }
    
    // 3. 测试备份功能
    std::cout << "\n3. 测试备份功能..." << std::endl;
    {
        Options options = Options::default_options();
        options.dir_path = "/tmp/feature_test_backup_src";
        system("rm -rf /tmp/feature_test_backup_src");
        system("rm -rf /tmp/feature_test_backup_dst");
        
        auto db = DB::open(options);
        db->put(Bytes{'b', 'a', 'c', 'k', 'u', 'p'}, Bytes{'t', 'e', 's', 't'});
        
        // 执行备份
        db->backup("/tmp/feature_test_backup_dst");
        db->close();
        
        // 从备份恢复
        Options backup_options = options;
        backup_options.dir_path = "/tmp/feature_test_backup_dst";
        auto backup_db = DB::open(backup_options);
        
        auto value = backup_db->get(Bytes{'b', 'a', 'c', 'k', 'u', 'p'});
        std::cout << "   备份和恢复: " << (value.size() == 4 ? "✓ 通过" : "✗ 失败") << std::endl;
        
        backup_db->close();
        system("rm -rf /tmp/feature_test_backup_src");
        system("rm -rf /tmp/feature_test_backup_dst");
    }
    
    // 4. 测试Redis数据结构
    std::cout << "\n4. 测试Redis数据结构..." << std::endl;
    {
        Options options = Options::default_options();
        options.dir_path = "/tmp/feature_test_redis";
        system("rm -rf /tmp/feature_test_redis");
        
        auto db = std::shared_ptr<DB>(DB::open(options).release());
        auto rds = std::make_unique<redis::RedisDataStructure>(db);
        
        // String操作
        rds->set("test_string", "hello");
        std::string value = rds->get("test_string");
        std::cout << "   Redis String: " << (value == "hello" ? "✓ 通过" : "✗ 失败") << std::endl;
        
        // Hash操作
        bool result = rds->hset("test_hash", "field1", "value1");
        std::string hash_value = rds->hget("test_hash", "field1");
        std::cout << "   Redis Hash: " << (result && hash_value == "value1" ? "✓ 通过" : "✗ 失败") << std::endl;
        
        rds.reset();
        db.reset();
        system("rm -rf /tmp/feature_test_redis");
    }
    
    std::cout << "\n=== 功能验证完成 ===" << std::endl;
    return 0;
}
EOF

# 编译功能验证程序
g++ -std=c++17 -I../include full_feature_test.cpp \
    -L. -lbitcask -lpthread -lz -o full_feature_manual

# 运行功能验证
./full_feature_manual

# 清理
rm full_feature_test.cpp full_feature_manual
```

## 7. 故障排除

### 7.1 编译问题

**错误：找不到头文件**
```bash
# 检查头文件路径
find ../include -name "*.h" | head -10

# 确保包含路径正确
g++ -std=c++17 -I../include -v -c test.cpp
```

**错误：链接失败**
```bash
# 检查库文件
ls -la libbitcask.a

# 检查依赖库
ldd ./test_program

# 重新编译库
make clean && make -j$(nproc)
```

### 7.2 运行时问题

**错误：权限拒绝**
```bash
# 检查文件权限
ls -la ./test_program
chmod +x ./test_program
```

**错误：端口占用**
```bash
# 检查端口使用情况
sudo netstat -tulpn | grep -E "(8080|6380)"

# 杀死占用进程
sudo kill -9 <PID>
```

## 8. 完整功能总结

### 8.1 核心功能
- ✅ 基础键值存储 (put/get/delete)
- ✅ 批量写入 (WriteBatch)
- ✅ 迭代器支持 (正向/反向/前缀过滤)
- ✅ 数据持久化和恢复
- ✅ 线程安全的并发访问

### 8.2 高级功能
- ✅ 数据合并(Merge) - 清理无效数据
- ✅ 数据备份 - 完整数据库快照
- ✅ 多种索引类型 - BTree, SkipList, B+Tree
- ✅ HTTP服务器 - REST API接口
- ✅ Redis协议兼容 - 支持Redis客户端

### 8.3 测试覆盖
- ✅ 单元测试 - 覆盖所有模块
- ✅ 集成测试 - 端到端功能验证  
- ✅ 性能测试 - 基准测试和性能对比
- ✅ 网络服务测试 - HTTP和Redis协议测试

## 9. 生产环境部署

### 9.1 性能调优
```bash
# 编译优化版本
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-O3 -march=native -DNDEBUG"
make clean && make -j$(nproc)

# 系统调优
echo 'vm.swappiness = 1' | sudo tee -a /etc/sysctl.conf
echo 'net.core.somaxconn = 1024' | sudo tee -a /etc/sysctl.conf
sudo sysctl -p
```

### 9.2 监控和日志
```bash
# 监控进程
top -p $(pgrep -f bitcask)

# 监控网络
ss -tulpn | grep -E "(8080|6380)"

# 监控磁盘IO
iostat -x 1
```

这个完整的编译指南涵盖了所有新增功能的手动编译、测试和验证方法。所有代码都经过测试，可以在Ubuntu 22.04上成功编译和运行。
