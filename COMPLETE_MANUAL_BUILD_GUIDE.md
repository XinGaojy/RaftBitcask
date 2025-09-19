# Bitcask C++ 完整手动编译指南 (Ubuntu 22.04)

## 🎯 系统环境准备

```bash
# 更新系统
sudo apt update && sudo apt upgrade -y

# 安装基础开发工具
sudo apt install -y build-essential cmake git

# 安装Google Test (必需)
sudo apt install -y libgtest-dev libgmock-dev

# 安装其他依赖
sudo apt install -y pkg-config zlib1g-dev
```

## 📁 项目结构验证

确保项目包含以下文件：

```
kv-projects/bitcask-cpp/
├── src/
│   ├── art_index.cpp
│   ├── art_index_complete.cpp
│   ├── art_index_fixed.cpp
│   ├── bplus_tree_index.cpp
│   ├── data_file.cpp
│   ├── db.cpp
│   ├── http_server.cpp
│   ├── index.cpp
│   ├── io_manager.cpp
│   ├── iterator.cpp
│   ├── log_record.cpp
│   ├── mmap_io.cpp
│   ├── redis_data_structure.cpp
│   ├── redis_server.cpp
│   ├── skiplist_index.cpp
│   ├── test_utils.cpp
│   ├── utils.cpp
│   └── write_batch.cpp
├── include/bitcask/
│   ├── art_index.h
│   ├── bitcask.h
│   ├── bplus_tree_index.h
│   ├── common.h
│   ├── data_file.h
│   ├── db.h
│   ├── http_server.h
│   ├── index.h
│   ├── io_manager.h
│   ├── log_record.h
│   ├── mmap_io.h
│   ├── options.h
│   ├── redis_data_structure.h
│   ├── redis_server.h
│   ├── skiplist_index.h
│   ├── test_utils.h
│   └── utils.h
└── tests/unit_tests/
    ├── test_*.cpp (15个测试文件)
```

## 🔧 手动编译步骤

### 第一步：编译核心源文件

```bash
cd kv-projects/bitcask-cpp
mkdir -p build && cd build

# 编译基础模块
g++ -std=c++17 -I../include -c ../src/log_record.cpp -o log_record.o -DUSE_ZLIB_CRC32
g++ -std=c++17 -I../include -c ../src/utils.cpp -o utils.o
g++ -std=c++17 -I../include -c ../src/io_manager.cpp -o io_manager.o
g++ -std=c++17 -I../include -c ../src/mmap_io.cpp -o mmap_io.o
g++ -std=c++17 -I../include -c ../src/data_file.cpp -o data_file.o

# 编译索引模块
g++ -std=c++17 -I../include -c ../src/index.cpp -o index.o
g++ -std=c++17 -I../include -c ../src/skiplist_index.cpp -o skiplist_index.o
g++ -std=c++17 -I../include -c ../src/bplus_tree_index.cpp -o bplus_tree_index.o
g++ -std=c++17 -I../include -c ../src/art_index.cpp -o art_index.o
g++ -std=c++17 -I../include -c ../src/art_index_fixed.cpp -o art_index_fixed.o
g++ -std=c++17 -I../include -c ../src/art_index_complete.cpp -o art_index_complete.o

# 编译核心数据库模块
g++ -std=c++17 -I../include -c ../src/iterator.cpp -o iterator.o
g++ -std=c++17 -I../include -c ../src/write_batch.cpp -o write_batch.o
g++ -std=c++17 -I../include -c ../src/db.cpp -o db.o

# 编译服务器模块
g++ -std=c++17 -I../include -c ../src/redis_data_structure.cpp -o redis_data_structure.o
g++ -std=c++17 -I../include -c ../src/redis_server.cpp -o redis_server.o
g++ -std=c++17 -I../include -c ../src/http_server.cpp -o http_server.o
g++ -std=c++17 -I../include -c ../src/test_utils.cpp -o test_utils.o
```

### 第二步：创建静态库

```bash
# 收集所有对象文件
CORE_OBJS="log_record.o utils.o io_manager.o mmap_io.o data_file.o index.o skiplist_index.o bplus_tree_index.o art_index.o art_index_fixed.o art_index_complete.o iterator.o write_batch.o db.o redis_data_structure.o redis_server.o http_server.o test_utils.o"

# 创建静态库
ar rcs libbitcask.a $CORE_OBJS
```

### 第三步：编译单元测试

#### 基础模块测试

```bash
# test_log_record
g++ -std=c++17 -I../include -I/usr/include/gtest \
    ../tests/unit_tests/test_log_record.cpp \
    libbitcask.a \
    -lgtest -lgtest_main -lpthread -lz \
    -o test_log_record

# test_io_manager
g++ -std=c++17 -I../include -I/usr/include/gtest \
    ../tests/unit_tests/test_io_manager.cpp \
    libbitcask.a \
    -lgtest -lgtest_main -lpthread -lz \
    -o test_io_manager

# test_data_file
g++ -std=c++17 -I../include -I/usr/include/gtest \
    ../tests/unit_tests/test_data_file.cpp \
    libbitcask.a \
    -lgtest -lgtest_main -lpthread -lz \
    -o test_data_file

# test_mmap_io
g++ -std=c++17 -I../include -I/usr/include/gtest \
    ../tests/unit_tests/test_mmap_io.cpp \
    libbitcask.a \
    -lgtest -lgtest_main -lpthread -lz \
    -o test_mmap_io
```

#### 索引模块测试

```bash
# test_index
g++ -std=c++17 -I../include -I/usr/include/gtest \
    ../tests/unit_tests/test_index.cpp \
    libbitcask.a \
    -lgtest -lgtest_main -lpthread -lz \
    -o test_index

# test_advanced_index
g++ -std=c++17 -I../include -I/usr/include/gtest \
    ../tests/unit_tests/test_advanced_index.cpp \
    libbitcask.a \
    -lgtest -lgtest_main -lpthread -lz \
    -o test_advanced_index

# test_art_index
g++ -std=c++17 -I../include -I/usr/include/gtest \
    ../tests/unit_tests/test_art_index.cpp \
    libbitcask.a \
    -lgtest -lgtest_main -lpthread -lz \
    -o test_art_index
```

#### 核心数据库测试

```bash
# test_db
g++ -std=c++17 -I../include -I/usr/include/gtest \
    ../tests/unit_tests/test_db.cpp \
    libbitcask.a \
    -lgtest -lgtest_main -lpthread -lz \
    -o test_db

# test_write_batch
g++ -std=c++17 -I../include -I/usr/include/gtest \
    ../tests/unit_tests/test_write_batch.cpp \
    libbitcask.a \
    -lgtest -lgtest_main -lpthread -lz \
    -o test_write_batch

# test_iterator
g++ -std=c++17 -I../include -I/usr/include/gtest \
    ../tests/unit_tests/test_iterator.cpp \
    libbitcask.a \
    -lgtest -lgtest_main -lpthread -lz \
    -o test_iterator

# test_merge
g++ -std=c++17 -I../include -I/usr/include/gtest \
    ../tests/unit_tests/test_merge.cpp \
    libbitcask.a \
    -lgtest -lgtest_main -lpthread -lz \
    -o test_merge

# test_backup
g++ -std=c++17 -I../include -I/usr/include/gtest \
    ../tests/unit_tests/test_backup.cpp \
    libbitcask.a \
    -lgtest -lgtest_main -lpthread -lz \
    -o test_backup
```

#### 服务器模块测试

```bash
# test_redis
g++ -std=c++17 -I../include -I/usr/include/gtest \
    ../tests/unit_tests/test_redis.cpp \
    libbitcask.a \
    -lgtest -lgtest_main -lpthread -lz \
    -o test_redis

# test_http_server
g++ -std=c++17 -I../include -I/usr/include/gtest \
    ../tests/unit_tests/test_http_server.cpp \
    libbitcask.a \
    -lgtest -lgtest_main -lpthread -lz \
    -o test_http_server

# test_test_utils
g++ -std=c++17 -I../include -I/usr/include/gtest \
    ../tests/unit_tests/test_test_utils.cpp \
    libbitcask.a \
    -lgtest -lgtest_main -lpthread -lz \
    -o test_test_utils
```

### 第四步：编译示例程序

```bash
# 基础操作示例
g++ -std=c++17 -I../include \
    ../examples/basic_operations.cpp \
    libbitcask.a \
    -lpthread -lz \
    -o basic_operations

# HTTP服务器示例
g++ -std=c++17 -I../include \
    ../examples/http_server_example.cpp \
    libbitcask.a \
    -lpthread -lz \
    -o http_server_example

# Redis服务器示例
g++ -std=c++17 -I../include \
    ../examples/redis_server_example.cpp \
    libbitcask.a \
    -lpthread -lz \
    -o redis_server_example
```

## 🧪 运行测试

### 按优先级运行测试

```bash
# 1. 基础模块测试
echo "=== 基础模块测试 ==="
./test_log_record
./test_io_manager
./test_data_file
./test_mmap_io

# 2. 索引模块测试
echo "=== 索引模块测试 ==="
./test_index
./test_advanced_index
./test_art_index

# 3. 核心数据库测试
echo "=== 核心数据库测试 ==="
./test_db
./test_write_batch
./test_iterator

# 4. 高级功能测试
echo "=== 高级功能测试 ==="
./test_merge
./test_backup

# 5. 服务器模块测试
echo "=== 服务器模块测试 ==="
./test_redis
./test_http_server
./test_test_utils
```

### 运行示例程序

```bash
# 基础操作示例
echo "=== 运行基础操作示例 ==="
./basic_operations

# HTTP服务器示例（后台运行）
echo "=== 启动HTTP服务器示例 ==="
./http_server_example &
HTTP_PID=$!

# 测试HTTP服务器
sleep 2
curl -X POST http://localhost:8080/put -d '{"key":"test", "value":"hello"}'
curl http://localhost:8080/get/test

# 停止HTTP服务器
kill $HTTP_PID

# Redis服务器示例（后台运行）
echo "=== 启动Redis服务器示例 ==="
./redis_server_example &
REDIS_PID=$!

# 测试Redis服务器
sleep 2
echo -e "SET test hello\r\nGET test\r\nQUIT\r\n" | nc localhost 6379

# 停止Redis服务器
kill $REDIS_PID
```

## 🔍 故障排除

### 常见编译错误

1. **找不到头文件**
   ```bash
   # 确保include路径正确
   -I../include
   ```

2. **找不到Google Test**
   ```bash
   # 安装Google Test
   sudo apt install libgtest-dev libgmock-dev
   
   # 或者指定路径
   -I/usr/include/gtest
   ```

3. **链接错误**
   ```bash
   # 确保链接所有必要的库
   -lgtest -lgtest_main -lpthread -lz
   ```

4. **CRC32函数未定义**
   ```bash
   # 添加zlib支持
   -DUSE_ZLIB_CRC32 -lz
   ```

### 运行时错误

1. **无法创建目录**
   ```bash
   # 确保有写权限
   sudo chown -R $USER:$USER /tmp/bitcask*
   ```

2. **文件锁错误**
   ```bash
   # 清理测试目录
   rm -rf /tmp/bitcask*
   ```

3. **内存错误**
   ```bash
   # 使用valgrind检查
   sudo apt install valgrind
   valgrind --leak-check=full ./test_db
   ```

## ✅ 验证安装

创建验证脚本：

```bash
#!/bin/bash
echo "🚀 Bitcask C++ 验证测试"
echo "======================"

# 测试计数器
PASSED=0
TOTAL=0

# 测试函数
run_test() {
    local test_name=$1
    local test_cmd=$2
    
    echo -n "测试 $test_name ... "
    TOTAL=$((TOTAL + 1))
    
    if $test_cmd > /dev/null 2>&1; then
        echo "✅ 通过"
        PASSED=$((PASSED + 1))
    else
        echo "❌ 失败"
    fi
}

# 运行基础测试
run_test "日志记录" "./test_log_record"
run_test "数据文件" "./test_data_file"
run_test "数据库核心" "./test_db"
run_test "批量写入" "./test_write_batch"
run_test "迭代器" "./test_iterator"

# 运行示例
run_test "基础操作" "./basic_operations"

# 输出结果
echo "======================"
echo "测试结果: $PASSED/$TOTAL 通过"

if [ $PASSED -eq $TOTAL ]; then
    echo "🎉 所有测试通过！Bitcask C++ 已准备好用于生产！"
    exit 0
else
    echo "⚠️  有测试失败，请检查错误信息"
    exit 1
fi
```

保存为 `verify.sh` 并运行：

```bash
chmod +x verify.sh
./verify.sh
```

## 📊 性能优化

### 编译器优化

```bash
# 发布版本优化
CXXFLAGS="-std=c++17 -O3 -DNDEBUG -march=native -mtune=native"

# 调试版本
CXXFLAGS="-std=c++17 -g -O0 -DDEBUG"
```

### 运行时配置

```cpp
// 高性能配置
Options options = Options::default_options();
options.sync_writes = false;           // 异步写入
options.data_file_size = 256 * 1024 * 1024; // 256MB
options.mmap_at_startup = true;        // 内存映射
options.index_type = IndexType::BPLUS_TREE; // B+树索引
```

这个指南提供了在Ubuntu 22.04上完整手动编译Bitcask C++项目的所有步骤，确保每个模块都能正确编译和通过测试。
