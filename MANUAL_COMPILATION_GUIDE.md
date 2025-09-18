# Bitcask C++ 手动编译运行完整指南

## 🚀 快速开始 (Ubuntu 22.04)

### 第一步：安装依赖
```bash
# 更新包管理器
sudo apt update

# 安装编译工具链
sudo apt install -y build-essential cmake g++ pkg-config git

# 安装Google Test框架
sudo apt install -y libgtest-dev libgmock-dev

# 验证安装
g++ --version  # 应该显示GCC 7+
cmake --version  # 应该显示CMake 3.10+
```

### 第二步：准备项目
```bash
# 进入项目目录
cd /path/to/kv-projects/bitcask-cpp

# 创建构建目录
mkdir -p build
cd build

# 配置CMake
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_STANDARD=17
```

### 第三步：编译项目
```bash
# 使用所有CPU核心编译
make -j$(nproc)

# 或者指定核心数（如果内存不足）
make -j2
```

## 📋 详细的单元测试手动编译运行指南

### 1. 日志记录模块 (LogRecord)

**编译：**
```bash
cd build
make test_log_record -j$(nproc)
```

**运行：**
```bash
./test_log_record
```

**预期输出：**
```
[==========] Running 15 tests from 5 test suites.
[----------] Global test environment set-up.
[----------] 6 tests from LogRecordTest
[ RUN      ] LogRecordTest.EncodeAndDecode
[       OK ] LogRecordTest.EncodeAndDecode (0 ms)
[  PASSED  ] 15 tests.
```

**测试覆盖：**
- 日志记录编码/解码
- CRC校验
- 变长整数编码
- 不同记录类型

---

### 2. 数据文件模块 (DataFile)

**编译：**
```bash
make test_data_file -j$(nproc)
```

**运行：**
```bash
./test_data_file
```

**预期输出：**
```
[==========] Running 15 tests from 5 test suites.
[  PASSED  ] 15 tests.
```

**测试覆盖：**
- 文件创建和写入
- 多条记录读写
- 错误处理
- 内存映射文件操作

---

### 3. IO管理器模块 (IOManager)

**编译：**
```bash
make test_io_manager -j$(nproc)
```

**运行：**
```bash
./test_io_manager
```

**预期输出：**
```
[==========] Running 14 tests from 3 test suites.
[  PASSED  ] 14 tests.
```

**测试覆盖：**
- 标准文件IO
- 内存映射IO
- 性能对比
- 大文件处理

---

### 4. 内存映射IO模块 (MMapIO)

**编译：**
```bash
make test_mmap_io -j$(nproc)
```

**运行：**
```bash
./test_mmap_io
```

**预期输出：**
```
[==========] Running 8 tests from 1 test suite.
[  PASSED  ] 8 tests.
```

**测试覆盖：**
- 内存映射读写
- 文件扩展
- 随机访问
- 二进制安全数据

---

### 5. 索引模块 (Index)

**编译：**
```bash
make test_index -j$(nproc)
```

**运行：**
```bash
./test_index
```

**预期输出：**
```
[==========] Running 22 tests from 5 test suites.
[  PASSED  ] 22 tests.
```

**测试覆盖：**
- 基础索引操作
- 索引迭代器
- 并发访问
- 性能测试

---

### 6. 数据库核心模块 (DB)

**编译：**
```bash
make test_db -j$(nproc)
```

**运行：**
```bash
./test_db
```

**预期输出：**
```
[==========] Running 27 tests from 7 test suites.
[  PASSED  ] 27 tests.
```

**测试覆盖：**
- 基础CRUD操作
- 数据持久化
- 大数据量测试
- 并发安全
- 错误处理

---

### 7. 高级索引模块 (AdvancedIndex)

**编译：**
```bash
make test_advanced_index -j$(nproc)
```

**运行：**
```bash
./test_advanced_index
```

**预期输出：**
```
[==========] Running 15 tests from 4 test suites.
[  PASSED  ] 15 tests.
```

**测试覆盖：**
- SkipList索引
- B+Tree索引
- ART索引
- 索引性能对比

---

### 8. ART索引专项测试

**编译：**
```bash
make test_art_index -j$(nproc)
```

**运行：**
```bash
./test_art_index
```

**预期输出：**
```
[==========] Running 12 tests from 1 test suite.
[  PASSED  ] 12 tests.
```

**测试覆盖：**
- ART树基础操作
- 前缀压缩
- 节点分裂和合并
- 大数据集测试

---

### 9. 批量写入模块 (WriteBatch)

**编译：**
```bash
make test_write_batch -j$(nproc)
```

**运行：**
```bash
./test_write_batch
```

**预期输出：**
```
[==========] Running 16 tests from 6 test suites.
[  PASSED  ] 16 tests.
```

**测试覆盖：**
- 原子批量写入
- 事务隔离
- 错误回滚
- 性能优化

---

### 10. 迭代器模块 (Iterator)

**编译：**
```bash
make test_iterator -j$(nproc)
```

**运行：**
```bash
./test_iterator
```

**预期输出：**
```
[==========] Running 20 tests from 6 test suites.
[  PASSED  ] 20 tests.
```

**测试覆盖：**
- 正向/反向迭代
- 前缀过滤
- 并发迭代
- 边界条件

---

### 11. 数据合并模块 (Merge)

**编译：**
```bash
make test_merge -j$(nproc)
```

**运行：**
```bash
./test_merge
```

**预期输出：**
```
[==========] Running 7 tests from 1 test suite.
[  PASSED  ] 7 tests.
```

**测试覆盖：**
- 数据压缩合并
- 垃圾回收
- 并发合并
- 合并统计

---

### 12. 数据备份模块 (Backup)

**编译：**
```bash
make test_backup -j$(nproc)
```

**运行：**
```bash
./test_backup
```

**预期输出：**
```
[==========] Running 8 tests from 1 test suite.
[  PASSED  ] 8 tests.
```

**测试覆盖：**
- 数据备份
- 备份恢复
- 增量备份
- 并发备份

---

### 13. HTTP服务器模块 (HttpServer)

**编译：**
```bash
make test_http_server -j$(nproc)
```

**运行：**
```bash
./test_http_server
```

**预期输出：**
```
[==========] Running 10 tests from 1 test suite.
[  PASSED  ] 10 tests.
```

**测试覆盖：**
- HTTP请求处理
- RESTful API
- JSON解析
- 错误响应

---

### 14. Redis协议模块 (Redis)

**编译：**
```bash
make test_redis -j$(nproc)
```

**运行：**
```bash
./test_redis
```

**预期输出：**
```
[==========] Running 10 tests from 1 test suite.
[  PASSED  ] 10 tests.
```

**测试覆盖：**
- Redis数据结构
- 协议兼容性
- 命令处理
- 数据编码

---

### 15. 测试工具模块 (TestUtils)

**编译：**
```bash
make test_test_utils -j$(nproc)
```

**运行：**
```bash
./test_test_utils
```

**预期输出：**
```
[==========] Running 16 tests from 2 test suites.
[  PASSED  ] 16 tests.
```

**测试覆盖：**
- 随机数据生成
- 性能计时
- 文件操作工具
- 测试辅助函数

---

## 🎯 示例程序手动编译运行

### 1. 基础操作示例

**编译：**
```bash
make bitcask_example -j$(nproc)
```

**运行：**
```bash
./bitcask_example
```

**预期输出：**
```
Bitcask C++ Example
===================
Using directory: /tmp/bitcask_example

Opening database...
1. Basic Put/Get operations:
name = Bitcask
version = 1.0.0
language = C++

2. Delete operation:
author key was deleted successfully

3. List all keys:
Total keys: 3
  - language
  - name  
  - version

4. Fold over all data:
  language => C++
  name => Bitcask
  version => 1.0.0

5. Using iterator:
Forward iteration:
  language => C++
  name => Bitcask
  version => 1.0.0
Reverse iteration:
  version => 1.0.0
  name => Bitcask
  language => C++

6. Batch write operations:
Batch write completed successfully
```

---

### 2. HTTP服务器示例

**编译：**
```bash
make http_server_example -j$(nproc)
```

**运行：**
```bash
./http_server_example
```

**预期输出：**
```
HTTP server started on 127.0.0.1:8080
Bitcask HTTP Server is running...
API endpoints:
  POST /bitcask/put    - Put key-value pairs
  GET  /bitcask/get    - Get value by key
  DELETE /bitcask/delete - Delete key
  GET  /bitcask/listkeys - List all keys
  GET  /bitcask/stat   - Get database statistics

Press Ctrl+C to stop the server
```

**测试API：**
```bash
# 在另一个终端中测试
curl -X POST -d '{"key1":"value1"}' http://localhost:8080/bitcask/put
curl http://localhost:8080/bitcask/get?key=key1
curl http://localhost:8080/bitcask/listkeys
curl http://localhost:8080/bitcask/stat
```

---

### 3. Redis服务器示例

**编译：**
```bash
make redis_server_example -j$(nproc)
```

**运行：**
```bash
./redis_server_example
```

**预期输出：**
```
Redis server started on 127.0.0.1:6380
Bitcask Redis Server is running...
Compatible with Redis protocol on port 6380

Supported commands:
  String: SET, GET, DEL
  Hash:   HSET, HGET, HDEL
  Set:    SADD, SISMEMBER, SREM
  List:   LPUSH, RPUSH, LPOP, RPOP
  ZSet:   ZADD, ZSCORE
  Other:  EXISTS, TYPE, PING, QUIT

Press Ctrl+C to stop the server
```

**测试Redis API：**
```bash
# 安装redis-cli (如果没有)
sudo apt install redis-tools

# 测试命令
redis-cli -p 6380 SET mykey myvalue
redis-cli -p 6380 GET mykey
redis-cli -p 6380 HSET myhash field1 value1
redis-cli -p 6380 HGET myhash field1
```

---

## 🔧 故障排除

### 编译错误

**1. CMake版本太低**
```bash
# 错误：CMake 3.5 or higher is required
sudo apt install cmake
# 或者从源码编译最新版本
```

**2. 缺少C++17支持**
```bash
# 错误：This file requires compiler and library support for the ISO C++ 2017 standard
sudo apt install g++-9
sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-9 90
```

**3. 找不到Google Test**
```bash
# 错误：Could NOT find GTest
sudo apt install libgtest-dev libgmock-dev
```

### 运行时错误

**1. 权限问题**
```bash
# 错误：Permission denied
sudo chown -R $USER:$USER /tmp/bitcask*
```

**2. 端口占用**
```bash
# 错误：Address already in use
sudo netstat -tulpn | grep :8080
sudo kill -9 <PID>
```

**3. 内存不足**
```bash
# 使用较少的编译线程
make -j2  # 而不是 make -j$(nproc)
```

---

## ✅ 验证清单

运行以下命令验证所有模块都正常工作：

```bash
# 1. 进入构建目录
cd build

# 2. 运行所有测试（每个都应该显示 [  PASSED  ] 所有测试）
./test_log_record    && echo "✅ LogRecord测试通过" || echo "❌ LogRecord测试失败"
./test_data_file     && echo "✅ DataFile测试通过" || echo "❌ DataFile测试失败"  
./test_io_manager    && echo "✅ IOManager测试通过" || echo "❌ IOManager测试失败"
./test_mmap_io       && echo "✅ MMapIO测试通过" || echo "❌ MMapIO测试失败"
./test_index         && echo "✅ Index测试通过" || echo "❌ Index测试失败"
./test_db            && echo "✅ DB测试通过" || echo "❌ DB测试失败"
./test_advanced_index && echo "✅ AdvancedIndex测试通过" || echo "❌ AdvancedIndex测试失败"
./test_art_index     && echo "✅ ARTIndex测试通过" || echo "❌ ARTIndex测试失败"
./test_write_batch   && echo "✅ WriteBatch测试通过" || echo "❌ WriteBatch测试失败"
./test_iterator      && echo "✅ Iterator测试通过" || echo "❌ Iterator测试失败"
./test_merge         && echo "✅ Merge测试通过" || echo "❌ Merge测试失败"
./test_backup        && echo "✅ Backup测试通过" || echo "❌ Backup测试失败"
./test_http_server   && echo "✅ HttpServer测试通过" || echo "❌ HttpServer测试失败"
./test_redis         && echo "✅ Redis测试通过" || echo "❌ Redis测试失败"
./test_test_utils    && echo "✅ TestUtils测试通过" || echo "❌ TestUtils测试失败"

# 3. 运行示例程序
timeout 10 ./bitcask_example && echo "✅ 基础示例运行成功" || echo "❌ 基础示例运行失败"

# 4. 检查可执行文件
ls -la *example && echo "✅ 所有示例程序编译成功" || echo "❌ 示例程序编译失败"
```

如果所有测试都显示"✅"，那么项目就完全准备好部署到生产环境了！

---

## 🚀 生产部署

所有测试通过后，可以使用以下优化选项重新编译生产版本：

```bash
# 清理构建目录
rm -rf build && mkdir build && cd build

# 生产优化编译
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-O3 -DNDEBUG -march=native"
make -j$(nproc)

# 安装到系统（可选）
sudo make install
```

现在您的Bitcask C++存储引擎已经完全准备好上线了！
