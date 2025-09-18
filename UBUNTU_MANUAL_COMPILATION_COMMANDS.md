# Ubuntu 22.04 手动编译命令详解

本文档提供在 Ubuntu 22.04 系统上手动编译和测试 bitcask-cpp 项目的具体命令，不使用脚本，所有命令可以逐一手动执行。

## 环境准备命令

### 1. 更新系统包
```bash
sudo apt update
sudo apt upgrade -y
```

### 2. 安装编译工具链
```bash
sudo apt install -y build-essential
sudo apt install -y cmake
sudo apt install -y gcc g++
sudo apt install -y pkg-config
```

### 3. 安装开发库
```bash
sudo apt install -y libssl-dev
sudo apt install -y zlib1g-dev
sudo apt install -y libcrc32c-dev
```

### 4. 安装测试框架
```bash
sudo apt install -y libgtest-dev
sudo apt install -y libgmock-dev
```

### 5. 安装调试和测试工具（可选）
```bash
sudo apt install -y valgrind
sudo apt install -y gdb
sudo apt install -y curl
sudo apt install -y netcat-openbsd
sudo apt install -y redis-tools
```

### 6. 验证安装
```bash
cmake --version
gcc --version
g++ --version
```

## 项目编译命令

### 1. 进入项目目录
```bash
cd /home/linux/share/kv_project/kv-projects/bitcask-cpp
```

### 2. 清理旧构建文件
```bash
rm -rf build
```

### 3. 创建构建目录
```bash
mkdir build
cd build
```

### 4. 配置项目
```bash
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_STANDARD=17 -DCMAKE_CXX_STANDARD_REQUIRED=ON
```

### 5. 编译项目
```bash
make -j$(nproc)
```

### 6. 验证编译结果
```bash
ls -la | grep -E "(libbitcask\.a|.*_example|.*test.*)"
```

## 手动编译单个模块

如果需要手动编译单个模块，可以使用以下命令：

### 1. 编译日志记录模块
```bash
g++ -std=c++17 -I../include -O2 -c ../src/log_record.cpp -o log_record.o
```

### 2. 编译IO管理器模块
```bash
g++ -std=c++17 -I../include -O2 -c ../src/io_manager.cpp -o io_manager.o
g++ -std=c++17 -I../include -O2 -c ../src/mmap_io.cpp -o mmap_io.o
```

### 3. 编译数据文件模块
```bash
g++ -std=c++17 -I../include -O2 -c ../src/data_file.cpp -o data_file.o
```

### 4. 编译索引模块
```bash
g++ -std=c++17 -I../include -O2 -c ../src/index.cpp -o index.o
g++ -std=c++17 -I../include -O2 -c ../src/art_index.cpp -o art_index.o
g++ -std=c++17 -I../include -O2 -c ../src/art_index_fixed.cpp -o art_index_fixed.o
g++ -std=c++17 -I../include -O2 -c ../src/art_index_complete.cpp -o art_index_complete.o
g++ -std=c++17 -I../include -O2 -c ../src/skiplist_index.cpp -o skiplist_index.o
g++ -std=c++17 -I../include -O2 -c ../src/bplus_tree_index.cpp -o bplus_tree_index.o
```

### 5. 编译数据库核心模块
```bash
g++ -std=c++17 -I../include -O2 -c ../src/db.cpp -o db.o
g++ -std=c++17 -I../include -O2 -c ../src/write_batch.cpp -o write_batch.o
g++ -std=c++17 -I../include -O2 -c ../src/iterator.cpp -o iterator.o
```

### 6. 编译工具模块
```bash
g++ -std=c++17 -I../include -O2 -c ../src/utils.cpp -o utils.o
g++ -std=c++17 -I../include -O2 -c ../src/test_utils.cpp -o test_utils.o
```

### 7. 编译服务器模块
```bash
g++ -std=c++17 -I../include -O2 -c ../src/http_server.cpp -o http_server.o
g++ -std=c++17 -I../include -O2 -c ../src/redis_server.cpp -o redis_server.o
g++ -std=c++17 -I../include -O2 -c ../src/redis_data_structure.cpp -o redis_data_structure.o
```

### 8. 创建静态库
```bash
ar rcs libbitcask.a *.o
```

## 手动编译测试程序

### 1. 编译日志记录测试
```bash
g++ -std=c++17 -I../include -O2 ../tests/unit_tests/test_log_record.cpp libbitcask.a -lgtest -lgtest_main -lpthread -lz -o test_log_record
```

### 2. 编译IO管理器测试
```bash
g++ -std=c++17 -I../include -O2 ../tests/unit_tests/test_io_manager.cpp libbitcask.a -lgtest -lgtest_main -lpthread -lz -o test_io_manager
```

### 3. 编译数据文件测试
```bash
g++ -std=c++17 -I../include -O2 ../tests/unit_tests/test_data_file.cpp libbitcask.a -lgtest -lgtest_main -lpthread -lz -o test_data_file
```

### 4. 编译索引测试
```bash
g++ -std=c++17 -I../include -O2 ../tests/unit_tests/test_index.cpp libbitcask.a -lgtest -lgtest_main -lpthread -lz -o test_index
```

### 5. 编译ART索引测试
```bash
g++ -std=c++17 -I../include -O2 ../tests/unit_tests/test_art_index.cpp libbitcask.a -lgtest -lgtest_main -lpthread -lz -o test_art_index
```

### 6. 编译数据库测试
```bash
g++ -std=c++17 -I../include -O2 ../tests/unit_tests/test_db.cpp libbitcask.a -lgtest -lgtest_main -lpthread -lz -o test_db
```

### 7. 编译批量写入测试
```bash
g++ -std=c++17 -I../include -O2 ../tests/unit_tests/test_write_batch.cpp libbitcask.a -lgtest -lgtest_main -lpthread -lz -o test_write_batch
```

### 8. 编译迭代器测试
```bash
g++ -std=c++17 -I../include -O2 ../tests/unit_tests/test_iterator.cpp libbitcask.a -lgtest -lgtest_main -lpthread -lz -o test_iterator
```

### 9. 编译内存映射IO测试
```bash
g++ -std=c++17 -I../include -O2 ../tests/unit_tests/test_mmap_io.cpp libbitcask.a -lgtest -lgtest_main -lpthread -lz -o test_mmap_io
```

### 10. 编译高级索引测试
```bash
g++ -std=c++17 -I../include -O2 ../tests/unit_tests/test_advanced_index.cpp libbitcask.a -lgtest -lgtest_main -lpthread -lz -o test_advanced_index
```

### 11. 编译合并测试
```bash
g++ -std=c++17 -I../include -O2 ../tests/unit_tests/test_merge.cpp libbitcask.a -lgtest -lgtest_main -lpthread -lz -o test_merge
```

### 12. 编译HTTP服务器测试
```bash
g++ -std=c++17 -I../include -O2 ../tests/unit_tests/test_http_server.cpp libbitcask.a -lgtest -lgtest_main -lpthread -lz -o test_http_server
```

### 13. 编译Redis兼容测试
```bash
g++ -std=c++17 -I../include -O2 ../tests/unit_tests/test_redis.cpp libbitcask.a -lgtest -lgtest_main -lpthread -lz -o test_redis
```

### 14. 编译备份测试
```bash
g++ -std=c++17 -I../include -O2 ../tests/unit_tests/test_backup.cpp libbitcask.a -lgtest -lgtest_main -lpthread -lz -o test_backup
```

### 15. 编译工具测试
```bash
g++ -std=c++17 -I../include -O2 ../tests/unit_tests/test_test_utils.cpp libbitcask.a -lgtest -lgtest_main -lpthread -lz -o test_test_utils
```

## 手动编译示例程序

### 1. 基本操作示例
```bash
g++ -std=c++17 -I../include -O2 ../examples/basic_operations.cpp libbitcask.a -lpthread -lz -o bitcask_example
```

### 2. HTTP服务器示例
```bash
g++ -std=c++17 -I../include -O2 ../examples/http_server_example.cpp libbitcask.a -lpthread -lz -o http_server_example
```

### 3. Redis服务器示例
```bash
g++ -std=c++17 -I../include -O2 ../examples/redis_server_example.cpp libbitcask.a -lpthread -lz -o redis_server_example
```

## 手动运行单元测试

### 1. 运行日志记录测试
```bash
./test_log_record
```

### 2. 运行IO管理器测试
```bash
./test_io_manager
```

### 3. 运行数据文件测试
```bash
./test_data_file
```

### 4. 运行索引测试
```bash
./test_index
```

### 5. 运行ART索引测试
```bash
./test_art_index
```

### 6. 运行数据库测试
```bash
./test_db
```

### 7. 运行批量写入测试
```bash
./test_write_batch
```

### 8. 运行迭代器测试
```bash
./test_iterator
```

### 9. 运行内存映射IO测试
```bash
./test_mmap_io
```

### 10. 运行高级索引测试
```bash
./test_advanced_index
```

### 11. 运行合并测试
```bash
./test_merge
```

### 12. 运行HTTP服务器测试
```bash
./test_http_server
```

### 13. 运行Redis兼容测试
```bash
./test_redis
```

### 14. 运行备份测试
```bash
./test_backup
```

### 15. 运行工具测试
```bash
./test_test_utils
```

## 手动运行特定测试用例

### 1. 运行特定测试类
```bash
./test_db --gtest_filter="DBTest.*"
./test_db --gtest_filter="DBLargeDataTest.*"
./test_db --gtest_filter="DBPersistenceTest.*"
```

### 2. 运行特定测试方法
```bash
./test_db --gtest_filter="DBTest.PutAndGet"
./test_db --gtest_filter="DBLargeDataTest.FileRotation"
./test_art_index --gtest_filter="ARTIndexTest.BasicPutAndGet"
```

### 3. 运行测试并显示详细输出
```bash
./test_db --gtest_filter="DBLargeDataTest.FileRotation" --gtest_also_run_disabled_tests --gtest_repeat=1 --gtest_shuffle
```

## 手动运行示例程序

### 1. 运行基本操作示例
```bash
./bitcask_example
```

### 2. 运行HTTP服务器示例（后台）
```bash
./http_server_example &
HTTP_PID=$!
```

#### 测试HTTP API
```bash
# PUT 操作
curl -X PUT http://localhost:8080/bitcask/test_key -d "test_value"

# GET 操作
curl http://localhost:8080/bitcask/test_key

# DELETE 操作
curl -X DELETE http://localhost:8080/bitcask/test_key

# 列出所有键
curl http://localhost:8080/bitcask/listkeys

# 获取统计信息
curl http://localhost:8080/bitcask/stat
```

#### 停止HTTP服务器
```bash
kill $HTTP_PID
```

### 3. 运行Redis服务器示例（后台）
```bash
./redis_server_example &
REDIS_PID=$!
```

#### 测试Redis兼容性（如果已安装redis-cli）
```bash
# 字符串操作
redis-cli -p 6380 set mykey myvalue
redis-cli -p 6380 get mykey

# 列表操作
redis-cli -p 6380 lpush mylist item1 item2
redis-cli -p 6380 lrange mylist 0 -1

# 哈希操作
redis-cli -p 6380 hset myhash field1 value1
redis-cli -p 6380 hget myhash field1

# 集合操作
redis-cli -p 6380 sadd myset member1 member2
redis-cli -p 6380 smembers myset

# 有序集合操作
redis-cli -p 6380 zadd myzset 1 member1 2 member2
redis-cli -p 6380 zrange myzset 0 -1
```

#### 停止Redis服务器
```bash
kill $REDIS_PID
```

## 调试和性能分析

### 1. 使用Valgrind检查内存泄漏
```bash
valgrind --tool=memcheck --leak-check=full ./bitcask_example
```

### 2. 使用GDB调试
```bash
gdb ./test_db
(gdb) set args --gtest_filter="DBLargeDataTest.FileRotation"
(gdb) run
```

### 3. 性能分析（如果安装了perf）
```bash
perf record -g ./test_db
perf report
```

## 清理构建文件

### 1. 清理对象文件
```bash
rm -f *.o
```

### 2. 清理可执行文件
```bash
rm -f test_* *_example
```

### 3. 清理库文件
```bash
rm -f libbitcask.a
```

### 4. 完全清理构建目录
```bash
cd ..
rm -rf build
```

## 故障排除

### 1. 编译错误
如果遇到编译错误，检查：
- C++编译器版本是否支持C++17
- 所有依赖库是否正确安装
- 头文件路径是否正确

### 2. 链接错误
如果遇到链接错误，检查：
- 库文件是否存在
- 链接顺序是否正确
- 系统库是否完整

### 3. 运行时错误
如果遇到运行时错误，检查：
- 文件权限是否正确
- 测试目录是否可写
- 端口是否被占用（服务器测试）

### 4. 测试失败
如果测试失败，可以：
- 运行单个失败的测试用例
- 使用GDB调试具体问题
- 检查测试数据目录权限

## 验证编译成功

成功编译后，应该有以下文件：
- `libbitcask.a` - 主要静态库
- `bitcask_example` - 基本操作示例
- `http_server_example` - HTTP服务器示例
- `redis_server_example` - Redis服务器示例
- `test_*` - 各种单元测试程序

所有测试程序应该能够成功运行并通过测试用例。
