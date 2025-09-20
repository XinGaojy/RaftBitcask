# Ubuntu 22.04 手动编译测试指南

本指南提供了在 Ubuntu 22.04 系统上手动编译和测试 bitcask-cpp 项目的详细步骤。

## 环境准备

### 1. 系统要求
- Ubuntu 22.04 LTS
- 8GB RAM 以上
- 10GB 可用磁盘空间

### 2. 安装依赖
```bash
# 更新包管理器
sudo apt update && sudo apt upgrade -y

# 安装编译工具
sudo apt install -y build-essential cmake gcc g++

# 安装 C++ 开发库
sudo apt install -y libssl-dev zlib1g-dev

# 安装 Google Test 依赖
sudo apt install -y libgtest-dev libgmock-dev

# 验证安装
cmake --version
gcc --version
g++ --version
```

## 项目编译

### 1. 创建构建目录
```bash
cd /home/linux/share/kv_project/kv-projects/bitcask-cpp
rm -rf build
mkdir build
cd build
```

### 2. 配置项目
```bash
cmake .. -DCMAKE_BUILD_TYPE=Release
```

### 3. 编译项目
```bash
# 使用所有可用 CPU 核心编译
make -j$(nproc)

# 或者单线程编译（调试时使用）
# make

# 验证编译结果
ls -la
```

### 4. 编译产物验证
编译成功后，应该看到以下文件：
- `libbitcask.a` - 主要静态库
- `bitcask_example` - 基本操作示例
- `http_server_example` - HTTP 服务器示例
- `redis_server_example` - Redis 服务器示例
- `unit_tests` - 完整单元测试套件
- 各个独立的测试可执行文件

## 模块功能测试

### 1. 日志记录模块测试
```bash
# 运行日志记录测试
./test_log_record

# 预期输出：所有测试用例通过
# [==========] Running X tests from Y test suites.
# [----------] Global test environment set-up.
# ...
# [  PASSED  ] X tests.
```

### 2. IO管理器模块测试
```bash
# 运行 IO 管理器测试
./test_io_manager

# 测试标准文件 IO
echo "测试标准文件 IO..."

# 测试内存映射 IO
echo "测试内存映射 IO..."
```

### 3. 数据文件模块测试
```bash
# 运行数据文件测试
./test_data_file

# 预期测试：
# - 创建新数据文件
# - 写入日志记录
# - 读取日志记录
# - 同步操作
# - 关闭文件
```

### 4. 索引模块测试
```bash
# 运行所有索引测试
./test_index

# 测试 BTree 索引
echo "测试 BTree 索引基本操作..."

# 测试迭代器
echo "测试索引迭代器..."
```

### 5. ART索引模块测试
```bash
# 运行 ART 索引测试
./test_art_index

# 验证功能：
# - 基本插入和查找
# - 更新已存在的键
# - 多键操作
# - 删除操作
# - 空键处理
# - 长键处理
# - 相似键处理
# - 大数据集测试
# - 压力测试
```

### 6. 数据库核心模块测试
```bash
# 运行数据库核心测试
./test_db

# 测试功能：
# - 打开/关闭数据库
# - Put/Get/Remove 操作
# - 批量写入
# - 迭代器
# - 统计信息
# - 备份恢复
```

### 7. 批量写入模块测试
```bash
# 运行批量写入测试
./test_write_batch

# 测试功能：
# - 批量插入
# - 批量删除
# - 事务提交
# - 回滚操作
```

### 8. 迭代器模块测试
```bash
# 运行迭代器测试
./test_iterator

# 测试功能：
# - 正向迭代
# - 反向迭代
# - 前缀过滤
# - Seek 操作
```

### 9. HTTP服务器模块测试
```bash
# 运行 HTTP 服务器测试
./test_http_server

# 启动 HTTP 服务器示例（在后台）
./http_server_example &
SERVER_PID=$!

# 等待服务器启动
sleep 2

# 测试 HTTP API
echo "测试 PUT 操作..."
curl -X PUT http://localhost:8080/bitcask/key1 -d "value1"

echo "测试 GET 操作..."
curl http://localhost:8080/bitcask/key1

echo "测试 DELETE 操作..."
curl -X DELETE http://localhost:8080/bitcask/key1

echo "测试列出所有键..."
curl http://localhost:8080/bitcask/listkeys

echo "测试统计信息..."
curl http://localhost:8080/bitcask/stat

# 关闭服务器
kill $SERVER_PID
```

### 10. Redis兼容模块测试
```bash
# 运行 Redis 兼容测试
./test_redis

# 启动 Redis 服务器示例（在后台）
./redis_server_example &
REDIS_PID=$!

# 等待服务器启动
sleep 2

# 使用 redis-cli 测试（如果已安装）
if command -v redis-cli &> /dev/null; then
    echo "测试 Redis 兼容性..."
    
    # 字符串操作
    redis-cli -p 6380 set key1 value1
    redis-cli -p 6380 get key1
    
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
else
    echo "redis-cli 未安装，跳过 Redis 兼容性测试"
fi

# 关闭 Redis 服务器
kill $REDIS_PID
```

### 11. 内存映射IO模块测试
```bash
# 运行内存映射 IO 测试
./test_mmap_io

# 测试功能：
# - 文件映射
# - 内存读写
# - 同步操作
# - 解除映射
```

### 12. 高级索引模块测试
```bash
# 运行高级索引测试
./test_advanced_index

# 测试功能：
# - 跳跃列表索引
# - B+树索引
# - 性能比较
```

### 13. 合并操作测试
```bash
# 运行合并操作测试
./test_merge

# 测试功能：
# - 数据文件合并
# - 空间回收
# - 合并后验证
```

### 14. 备份功能测试
```bash
# 运行备份功能测试
./test_backup

# 测试功能：
# - 数据备份
# - 备份恢复
# - 增量备份
```

### 15. 工具模块测试
```bash
# 运行工具模块测试
./test_test_utils

# 测试功能：
# - 测试数据生成
# - 性能测量
# - 辅助函数
```

## 完整测试套件

### 运行所有单元测试
```bash
# 运行完整的单元测试套件
./unit_tests

# 运行特定测试组
./unit_tests --gtest_filter="ARTIndexTest.*"
./unit_tests --gtest_filter="DBTest.*"

# 生成详细测试报告
./unit_tests --gtest_output=xml:test_results.xml
```

### 集成测试
```bash
# 运行集成测试
./integration_tests

# 预期测试：
# - 多模块协作
# - 端到端功能
# - 数据一致性
```

### 性能基准测试
```bash
# 运行性能测试
./benchmark_tests

# 预期输出：
# - 写入性能指标
# - 读取性能指标
# - 内存使用情况
# - 磁盘IO统计
```

## 基本操作示例

### 运行基本操作示例
```bash
# 运行基本操作示例
./bitcask_example

# 示例操作：
# - 创建数据库
# - 插入数据
# - 查询数据
# - 删除数据
# - 统计信息
# - 关闭数据库
```

## 性能验证

### 1. 内存使用检查
```bash
# 运行内存使用检查
valgrind --tool=memcheck --leak-check=full ./unit_tests
```

### 2. 性能分析
```bash
# 运行性能分析
perf record -g ./benchmark_tests
perf report
```

### 3. 磁盘IO监控
```bash
# 监控磁盘IO
iostat -x 1 &
IOSTAT_PID=$!

# 运行测试
./benchmark_tests

# 停止监控
kill $IOSTAT_PID
```

## 故障排除

### 编译错误
如果遇到编译错误：

1. 检查依赖是否正确安装
2. 确认 C++ 版本支持 C++17
3. 清理构建目录重新编译：
```bash
cd ..
rm -rf build
mkdir build
cd build
cmake ..
make clean
make -j$(nproc)
```

### 测试失败
如果测试失败：

1. 检查文件权限
2. 确保有足够的磁盘空间
3. 检查端口是否被占用（HTTP/Redis测试）

### 运行时错误
如果运行时出错：

1. 检查数据目录权限
2. 确保没有其他进程使用数据库文件
3. 检查系统资源限制

## 预期结果

成功完成所有测试后，您应该看到：

1. **编译成功**：所有目标文件无错误生成
2. **单元测试通过**：所有测试用例返回 PASSED
3. **功能测试正常**：各模块功能按预期工作
4. **性能测试达标**：满足基本性能要求
5. **示例程序运行**：演示程序正常执行

这表明 bitcask-cpp 项目已经可以在 Ubuntu 22.04 上正常编译和运行，可以投入生产使用。

## 注意事项

1. 确保系统有足够的磁盘空间（至少 10GB）
2. 某些测试可能需要网络权限
3. 性能测试结果可能受系统负载影响
4. 建议在干净的环境中进行测试
5. 如果是生产环境，建议先在测试环境验证
