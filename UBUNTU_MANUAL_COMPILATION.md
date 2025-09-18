# Ubuntu 22.04 手动编译完整指南

## 🚨 立即上线解决方案

### 第一步：清理环境

```bash
# 进入项目目录
cd /path/to/bitcask-cpp

# 清理所有重复和错误文件
rm -f src/art_index_fixed.cpp
rm -f src/art_index_complete.cpp
rm -f src/art_index_old.cpp
rm -f src/art_index_backup.cpp

# 清理构建目录
rm -rf build
mkdir build
```

### 第二步：安装依赖

```bash
# 更新包管理器
sudo apt update

# 安装必需工具
sudo apt install -y build-essential cmake g++ pkg-config git

# 安装测试框架
sudo apt install -y libgtest-dev libgmock-dev

# 验证版本
g++ --version    # 应该是 9.4.0 或更高
cmake --version  # 应该是 3.16.3 或更高
```

### 第三步：配置和编译

```bash
# 进入构建目录
cd build

# 配置CMake
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_STANDARD=17 \
    -DCMAKE_CXX_FLAGS="-O2 -Wall -Wextra"

# 编译项目
make -j$(nproc)

# 如果内存不足，使用较少线程
# make -j2
```

## 📋 每个模块的手动编译运行方式

### 1. 日志记录模块 (LogRecord)

**编译：**
```bash
# 在build目录中
make test_log_record
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
[ RUN      ] LogRecordTest.EncodeThenDecode
[       OK ] LogRecordTest.EncodeThenDecode (0 ms)
...
[==========] 15 tests from 5 test suites ran. (X ms total)
[  PASSED  ] 15 tests.
```

**测试内容：**
- 日志记录编码和解码
- CRC校验功能
- 变长整数编码
- 不同记录类型处理

---

### 2. 数据文件模块 (DataFile)

**编译：**
```bash
make test_data_file
```

**运行：**
```bash
./test_data_file
```

**预期输出：**
```
[  PASSED  ] 15 tests.
```

**测试内容：**
- 文件创建和写入
- 多条记录读写
- 文件同步操作
- 错误处理机制

---

### 3. IO管理器模块 (IOManager)

**编译：**
```bash
make test_io_manager
```

**运行：**
```bash
./test_io_manager
```

**预期输出：**
```
[  PASSED  ] 14 tests.
```

**测试内容：**
- 标准文件IO操作
- 内存映射IO
- 性能对比测试
- 大文件处理

---

### 4. 内存映射IO模块 (MMapIO)

**编译：**
```bash
make test_mmap_io
```

**运行：**
```bash
./test_mmap_io
```

**预期输出：**
```
[  PASSED  ] 8 tests.
```

**测试内容：**
- 内存映射读写
- 文件动态扩展
- 随机访问性能
- 二进制数据安全

---

### 5. 索引模块 (Index)

**编译：**
```bash
make test_index
```

**运行：**
```bash
./test_index
```

**预期输出：**
```
[  PASSED  ] 22 tests.
```

**测试内容：**
- 基础索引操作
- 索引迭代器
- 并发访问测试
- 性能基准测试

---

### 6. 数据库核心模块 (DB)

**编译：**
```bash
make test_db
```

**运行：**
```bash
./test_db
```

**预期输出：**
```
[  PASSED  ] 27 tests.
```

**测试内容：**
- 基础CRUD操作
- 数据持久化
- 大数据量处理
- 并发安全测试
- 完整错误处理

---

### 7. 高级索引模块 (AdvancedIndex)

**编译：**
```bash
make test_advanced_index
```

**运行：**
```bash
./test_advanced_index
```

**预期输出：**
```
[  PASSED  ] 15 tests.
```

**测试内容：**
- SkipList索引性能
- B+Tree范围查询
- ART索引前缀压缩
- 索引类型对比

---

### 8. ART索引专项测试

**编译：**
```bash
make test_art_index
```

**运行：**
```bash
./test_art_index
```

**预期输出：**
```
[  PASSED  ] 12 tests.
```

**测试内容：**
- ART树基础操作
- 前缀压缩算法
- 节点分裂和合并
- 大规模数据测试

---

### 9. 批量写入模块 (WriteBatch)

**编译：**
```bash
make test_write_batch
```

**运行：**
```bash
./test_write_batch
```

**预期输出：**
```
[  PASSED  ] 16 tests.
```

**测试内容：**
- 原子批量写入
- 事务隔离级别
- 错误回滚机制
- 批量操作性能

---

### 10. 迭代器模块 (Iterator)

**编译：**
```bash
make test_iterator
```

**运行：**
```bash
./test_iterator
```

**预期输出：**
```
[  PASSED  ] 20 tests.
```

**测试内容：**
- 正向和反向迭代
- 前缀过滤功能
- 并发迭代安全
- 边界条件处理

---

### 11. 数据合并模块 (Merge)

**编译：**
```bash
make test_merge
```

**运行：**
```bash
./test_merge
```

**预期输出：**
```
[  PASSED  ] 7 tests.
```

**测试内容：**
- 数据压缩合并
- 垃圾回收机制
- 并发合并安全
- 合并性能统计

---

### 12. 数据备份模块 (Backup)

**编译：**
```bash
make test_backup
```

**运行：**
```bash
./test_backup
```

**预期输出：**
```
[  PASSED  ] 8 tests.
```

**测试内容：**
- 完整数据备份
- 增量备份功能
- 备份数据恢复
- 并发备份安全

---

### 13. HTTP服务器模块 (HttpServer)

**编译：**
```bash
make test_http_server
```

**运行：**
```bash
./test_http_server
```

**预期输出：**
```
[  PASSED  ] 10 tests.
```

**测试内容：**
- HTTP请求解析
- RESTful API响应
- JSON数据处理
- 错误状态码

---

### 14. Redis协议模块 (Redis)

**编译：**
```bash
make test_redis
```

**运行：**
```bash
./test_redis
```

**预期输出：**
```
[  PASSED  ] 10 tests.
```

**测试内容：**
- Redis数据结构
- 协议兼容性
- 命令解析处理
- 数据序列化

---

### 15. 测试工具模块 (TestUtils)

**编译：**
```bash
make test_test_utils
```

**运行：**
```bash
./test_test_utils
```

**预期输出：**
```
[  PASSED  ] 16 tests.
```

**测试内容：**
- 随机数据生成
- 性能计时工具
- 文件操作辅助
- 测试环境管理

---

## 🎯 示例程序手动编译运行

### 1. 基础操作示例

**编译：**
```bash
make bitcask_example
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
✅ Database opened successfully

1. Basic Put/Get operations:
   PUT: name = Bitcask
   GET: name = Bitcask
   PUT: version = 1.0.0
   GET: version = 1.0.0
   PUT: language = C++
   GET: language = C++

2. Delete operation:
   PUT: author = Anonymous
   DELETE: author
   GET: author = (not found) ✅

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
   Creating batch with 3 operations...
   ✅ Batch write completed successfully
   Verified all batch operations

✅ All example operations completed successfully!
Database closed.
```

---

### 2. HTTP服务器示例

**编译：**
```bash
make http_server_example
```

**运行：**
```bash
./http_server_example
```

**预期输出：**
```
Starting Bitcask HTTP Server...
✅ Database initialized successfully
✅ HTTP server started on 127.0.0.1:8080

Bitcask HTTP Server is running...
API endpoints:
  POST /bitcask/put     - Put key-value pairs
  GET  /bitcask/get     - Get value by key  
  DELETE /bitcask/delete - Delete key
  GET  /bitcask/listkeys - List all keys
  GET  /bitcask/stat    - Get database statistics

Press Ctrl+C to stop the server
```

**API测试命令：**
```bash
# 在另一个终端中测试
# PUT操作
curl -X POST -H "Content-Type: application/json" \
     -d '{"key1":"value1","key2":"value2","key3":"value3"}' \
     http://localhost:8080/bitcask/put

# GET操作
curl http://localhost:8080/bitcask/get?key=key1

# 列出所有键
curl http://localhost:8080/bitcask/listkeys

# 获取统计信息
curl http://localhost:8080/bitcask/stat

# DELETE操作
curl -X DELETE http://localhost:8080/bitcask/delete?key=key1
```

---

### 3. Redis服务器示例

**编译：**
```bash
make redis_server_example
```

**运行：**
```bash
./redis_server_example
```

**预期输出：**
```
Starting Bitcask Redis Server...
✅ Database initialized successfully
✅ Redis server started on 127.0.0.1:6380

Bitcask Redis Server is running...
Compatible with Redis protocol on port 6380

Supported commands:
  String: SET, GET, DEL, EXISTS
  Hash:   HSET, HGET, HDEL, HEXISTS
  Set:    SADD, SISMEMBER, SREM, SCARD
  List:   LPUSH, RPUSH, LPOP, RPOP, LLEN
  ZSet:   ZADD, ZSCORE, ZREM, ZCARD
  Other:  PING, QUIT, TYPE, FLUSHDB

Press Ctrl+C to stop the server
```

**Redis API测试命令：**
```bash
# 安装redis-cli (如果没有)
sudo apt install redis-tools

# 连接测试
redis-cli -p 6380 PING

# 字符串操作
redis-cli -p 6380 SET mykey myvalue
redis-cli -p 6380 GET mykey
redis-cli -p 6380 DEL mykey

# 哈希操作
redis-cli -p 6380 HSET myhash field1 value1
redis-cli -p 6380 HGET myhash field1
redis-cli -p 6380 HDEL myhash field1

# 集合操作
redis-cli -p 6380 SADD myset member1
redis-cli -p 6380 SISMEMBER myset member1
redis-cli -p 6380 SREM myset member1

# 列表操作
redis-cli -p 6380 LPUSH mylist element1
redis-cli -p 6380 RPUSH mylist element2
redis-cli -p 6380 LPOP mylist
redis-cli -p 6380 RPOP mylist

# 有序集合操作
redis-cli -p 6380 ZADD myzset 1.0 member1
redis-cli -p 6380 ZSCORE myzset member1
redis-cli -p 6380 ZREM myzset member1
```

---

## 🔧 故障排除

### 编译错误解决

**1. CMake版本过低**
```bash
# 错误信息: CMake 3.16 or higher is required
# 解决方案:
sudo apt remove cmake
sudo snap install cmake --classic
```

**2. GCC版本不支持C++17**
```bash
# 错误信息: This file requires compiler and library support for the ISO C++ 2017 standard
# 解决方案:
sudo apt install g++-9
sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-9 90
```

**3. Google Test未找到**
```bash
# 错误信息: Could NOT find GTest
# 解决方案:
sudo apt install libgtest-dev libgmock-dev
```

**4. 链接错误**
```bash
# 错误信息: undefined reference to `pthread_create`
# 解决方案:
export LDFLAGS="-lpthread"
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_STANDARD=17
make clean && make -j$(nproc)
```

### 运行时错误解决

**1. 权限问题**
```bash
# 错误信息: Permission denied
# 解决方案:
sudo chmod 777 /tmp
mkdir -p ~/.bitcask_temp
export TMPDIR=~/.bitcask_temp
```

**2. 端口占用**
```bash
# 错误信息: Address already in use
# 解决方案:
sudo netstat -tulpn | grep :8080
sudo kill -9 <PID>
```

**3. 内存不足**
```bash
# 错误信息: virtual memory exhausted
# 解决方案:
# 增加交换空间
sudo fallocate -l 2G /swapfile
sudo chmod 600 /swapfile
sudo mkswap /swapfile
sudo swapon /swapfile

# 或减少编译线程
make -j2
```

---

## ✅ 完整验证清单

运行以下命令验证所有模块：

```bash
# 进入构建目录
cd build

# 验证所有测试模块
echo "=== 运行所有测试模块 ==="
tests=(
    "test_log_record"
    "test_data_file"
    "test_io_manager"
    "test_mmap_io"
    "test_index"
    "test_db"
    "test_advanced_index"
    "test_art_index"
    "test_write_batch"
    "test_iterator"
    "test_merge"
    "test_backup"
    "test_http_server"
    "test_redis"
    "test_test_utils"
)

passed=0
failed=0

for test in "${tests[@]}"; do
    echo "运行 $test..."
    if ./$test > /tmp/${test}.log 2>&1; then
        echo "✅ $test 通过"
        ((passed++))
    else
        echo "❌ $test 失败"
        echo "错误日志:"
        tail -5 /tmp/${test}.log
        ((failed++))
    fi
done

echo ""
echo "=== 测试总结 ==="
echo "通过: $passed"
echo "失败: $failed"

if [[ $failed -eq 0 ]]; then
    echo "🎉 所有测试通过！项目已准备好上线！"
    
    echo ""
    echo "=== 启动示例程序 ==="
    echo "基础示例:"
    timeout 10 ./bitcask_example && echo "✅ 基础示例运行正常"
    
    echo ""
    echo "HTTP服务器 (后台启动):"
    ./http_server_example &
    HTTP_PID=$!
    sleep 2
    curl -s http://localhost:8080/bitcask/stat && echo "✅ HTTP服务器运行正常"
    kill $HTTP_PID
    
    echo ""
    echo "Redis服务器 (后台启动):"
    ./redis_server_example &
    REDIS_PID=$!
    sleep 2
    redis-cli -p 6380 PING && echo "✅ Redis服务器运行正常"
    kill $REDIS_PID
    
    echo ""
    echo "🚀 项目完全准备好部署到生产环境！"
else
    echo "❌ 部分测试失败，请检查错误信息"
fi
```

---

## 🚀 生产部署

验证成功后，可以进行生产部署：

```bash
# 创建生产目录
sudo mkdir -p /opt/bitcask
sudo chown $USER:$USER /opt/bitcask

# 复制可执行文件
cp build/http_server_example /opt/bitcask/
cp build/redis_server_example /opt/bitcask/
cp build/bitcask_example /opt/bitcask/

# 创建数据目录
mkdir -p /opt/bitcask/data

# 创建systemd服务
sudo tee /etc/systemd/system/bitcask-http.service > /dev/null <<EOF
[Unit]
Description=Bitcask HTTP Server
After=network.target

[Service]
Type=simple
User=$USER
Group=$USER
WorkingDirectory=/opt/bitcask
ExecStart=/opt/bitcask/http_server_example
Restart=always
RestartSec=5

[Install]
WantedBy=multi-user.target
EOF

# 启动服务
sudo systemctl daemon-reload
sudo systemctl enable bitcask-http
sudo systemctl start bitcask-http
sudo systemctl status bitcask-http
```

现在您的Bitcask C++存储引擎已经完全准备好在Ubuntu 22.04上运行了！
