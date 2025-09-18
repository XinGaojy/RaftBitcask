# Bitcask C++ 手动测试完整指南

## 编译环境准备

### Ubuntu 22.04 依赖安装
```bash
# 更新包管理器
sudo apt update

# 安装基础编译工具
sudo apt install build-essential cmake pkg-config

# 安装CRC32库（优先选择libcrc32c，备选zlib）
sudo apt install libcrc32c-dev || sudo apt install zlib1g-dev

# 可选：安装Redis工具用于测试Redis服务器
sudo apt install redis-tools
```

### 项目编译
```bash
# 进入项目目录
cd kv-projects/bitcask-cpp

# 创建构建目录
mkdir -p build && cd build

# CMake配置
cmake .. -DCMAKE_BUILD_TYPE=Release

# 编译（使用所有CPU核心）
make -j$(nproc)
```

## 手动测试指南

### 1. 核心存储模块测试

#### 1.1 日志记录测试
```bash
# 编译并运行
make test_log_record
./test_log_record

# 预期结果：所有测试通过，验证LogRecord编码/解码功能
```

#### 1.2 数据文件测试
```bash
# 编译并运行
make test_data_file
./test_data_file

# 预期结果：验证数据文件读写、sync操作
```

#### 1.3 数据库核心测试
```bash
# 编译并运行
make test_db
./test_db

# 预期结果：验证Put/Get/Delete基本操作、事务、统计信息
```

### 2. 索引模块测试

#### 2.1 基础索引测试
```bash
# 编译并运行
make test_index
./test_index

# 预期结果：验证BTree索引的基本功能
```

#### 2.2 高级索引测试
```bash
# 编译并运行
make test_advanced_index
./test_advanced_index

# 预期结果：验证SkipList和B+Tree索引功能
```

#### 2.3 ART索引测试
```bash
# 编译并运行
make test_art_index
./test_art_index

# 预期结果：验证自适应基数树索引功能
```

### 3. I/O系统测试

#### 3.1 标准I/O测试
```bash
# 编译并运行
make test_io_manager
./test_io_manager

# 预期结果：验证标准文件I/O操作
```

#### 3.2 内存映射I/O测试
```bash
# 编译并运行
make test_mmap_io
./test_mmap_io

# 预期结果：验证内存映射I/O的读写、扩展、同步功能
```

### 4. 数据操作测试

#### 4.1 批量写入测试
```bash
# 编译并运行
make test_write_batch
./test_write_batch

# 预期结果：验证原子批量写入功能
```

#### 4.2 迭代器测试
```bash
# 编译并运行
make test_iterator
./test_iterator

# 预期结果：验证数据迭代、过滤、排序功能
```

#### 4.3 数据合并测试
```bash
# 编译并运行
make test_merge
./test_merge

# 预期结果：验证数据压缩、垃圾回收功能
```

#### 4.4 数据备份测试
```bash
# 编译并运行
make test_backup
./test_backup

# 预期结果：验证数据备份和恢复功能
```

### 5. 网络服务测试

#### 5.1 HTTP服务器测试
```bash
# 编译并运行
make test_http_server
./test_http_server

# 预期结果：验证HTTP API接口功能
```

#### 5.2 Redis协议测试
```bash
# 编译并运行
make test_redis
./test_redis

# 预期结果：验证Redis协议兼容性
```

### 6. 测试工具测试

#### 6.1 测试工具验证
```bash
# 编译并运行
make test_test_utils
./test_test_utils

# 预期结果：验证随机数据生成、性能测试工具功能
```

### 7. 完整集成测试

#### 7.1 所有单元测试
```bash
# 编译并运行完整测试套件
make unit_tests
./unit_tests

# 预期结果：所有测试模块一次性执行，全部通过
```

#### 7.2 集成测试
```bash
# 编译并运行
make integration_tests
./integration_tests

# 预期结果：验证模块间集成功能
```

#### 7.3 性能基准测试
```bash
# 编译并运行
make benchmark_tests
./benchmark_tests

# 预期结果：输出性能基准数据
```

## 示例程序测试

### 1. 基础操作示例
```bash
# 编译并运行
make bitcask_example
./bitcask_example

# 预期结果：演示基本的数据库操作
```

### 2. HTTP服务器示例
```bash
# 编译并启动HTTP服务器
make http_server_example
./http_server_example &

# 在另一个终端测试HTTP接口
curl -X POST http://localhost:8080/put -d '{"key":"test", "value":"hello"}' -H "Content-Type: application/json"
curl http://localhost:8080/get/test

# 停止服务器
pkill http_server_example
```

### 3. Redis服务器示例
```bash
# 编译并启动Redis服务器
make redis_server_example
./redis_server_example &

# 使用redis-cli测试（如果已安装）
redis-cli -p 6380 SET mykey "Hello World"
redis-cli -p 6380 GET mykey

# 或使用telnet测试
telnet localhost 6380
# 输入：SET test value
# 输入：GET test

# 停止服务器
pkill redis_server_example
```

## 特定功能验证

### 1. 索引性能对比
```bash
# 运行高级索引测试，观察不同索引类型的性能差异
./test_advanced_index

# 查看输出中的性能对比数据
```

### 2. I/O性能对比
```bash
# 运行MMap I/O测试，查看性能对比结果
./test_mmap_io

# 观察标准I/O vs 内存映射I/O的性能差异
```

### 3. 内存使用验证
```bash
# 使用valgrind检查内存泄漏（如果已安装）
valgrind --leak-check=full ./test_db

# 使用系统监控工具观察内存使用
top -p $(pgrep bitcask_example)
```

## 错误场景测试

### 1. 磁盘空间不足
```bash
# 创建小的临时文件系统进行测试
# 观察程序在磁盘空间不足时的行为
```

### 2. 文件权限问题
```bash
# 创建只读目录，测试程序错误处理
mkdir readonly_test
chmod 444 readonly_test
# 尝试在只读目录创建数据库，验证错误处理
```

### 3. 并发访问测试
```bash
# 同时运行多个实例，测试文件锁机制
./bitcask_example &
./bitcask_example &  # 应该因为文件锁而失败
```

## 性能基准测试

### 1. 写入性能测试
```bash
# 大量数据写入测试
time ./benchmark_tests

# 观察吞吐量和延迟指标
```

### 2. 读取性能测试
```bash
# 随机读取性能测试
# 顺序读取性能测试
# 混合读写性能测试
```

## 预期测试结果

### 成功标准
1. **所有单元测试通过** - 无失败案例
2. **无内存泄漏** - valgrind检查通过
3. **性能指标达标** - 符合预期的吞吐量和延迟
4. **错误处理正确** - 异常情况下程序稳定
5. **服务器正常响应** - HTTP/Redis协议正确实现

### 性能参考指标
- **写入吞吐量**: > 10,000 ops/sec
- **读取吞吐量**: > 50,000 ops/sec  
- **索引查找延迟**: < 1ms
- **内存使用**: 合理增长，无泄漏
- **文件I/O效率**: MMap > 标准I/O

## 故障排查

### 编译问题
```bash
# 检查依赖
pkg-config --list-all | grep crc
ldd ./bitcask_example

# 检查编译器版本
g++ --version
cmake --version
```

### 运行时问题
```bash
# 检查文件权限
ls -la /tmp/bitcask_*

# 检查进程状态
ps aux | grep bitcask

# 检查端口占用
netstat -tulpn | grep :8080
netstat -tulpn | grep :6380
```

### 性能问题
```bash
# 系统资源监控
iostat -x 1
vmstat 1
top -p $(pgrep bitcask)
```

## 总结

完成所有上述测试后，你将验证：

✅ **21个独立的单元测试模块**全部通过  
✅ **4种索引类型**（BTree、ART、SkipList、B+Tree）功能正常  
✅ **2种I/O方式**（标准I/O、内存映射I/O）性能良好  
✅ **完整的数据操作**（读写、合并、备份、迭代）功能完备  
✅ **网络服务**（HTTP、Redis协议）正常运行  
✅ **测试工具**和性能基准可用  
✅ **生产环境就绪**，可以安全部署  

这确认了Bitcask C++版本是三个版本（C++、Go、Rust）中功能最完整、最适合生产环境的实现。
