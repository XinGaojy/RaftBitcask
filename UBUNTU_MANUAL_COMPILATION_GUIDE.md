# Ubuntu 22.04 手动编译运行指南

## 系统要求
- Ubuntu 22.04 LTS
- GCC 11.4.0 或更高版本  
- CMake 3.22.1 或更高版本
- GoogleTest 库

## 准备工作

### 1. 安装编译依赖
```bash
# 更新包列表
sudo apt update

# 安装编译工具链
sudo apt install -y build-essential cmake git

# 安装GoogleTest开发库
sudo apt install -y libgtest-dev libgmock-dev

# 如果GoogleTest不可用，从源码安装
# git clone https://github.com/google/googletest.git
# cd googletest && mkdir build && cd build
# cmake .. && make -j$(nproc) && sudo make install
```

### 2. 验证环境
```bash
# 检查GCC版本（需要11.4.0或更高）
gcc --version

# 检查CMake版本（需要3.22.1或更高）
cmake --version

# 检查系统信息
uname -a
```

## 手动编译步骤

### 1. 清理并进入项目目录
```bash
cd /path/to/bitcask-cpp
rm -rf build
mkdir -p build
cd build
```

### 2. 配置CMake
```bash
cmake .. \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_STANDARD=17 \
  -DCMAKE_CXX_FLAGS="-Wall -Wextra -O2"
```

### 3. 编译主库
```bash
# 编译静态库
make bitcask -j$(nproc)

# 验证库文件生成
ls -la libbitcask.a
```

### 4. 编译所有测试程序
```bash
# 编译所有测试
make -j$(nproc)

# 验证测试程序生成
ls -la test_*
```

## 单元测试手动运行

### 1. 核心功能测试

#### 备份功能测试
```bash
# 运行完整备份测试套件
./test_backup

# 运行单个测试用例
./test_backup --gtest_filter=BackupTest.BasicBackup
./test_backup --gtest_filter=BackupTest.BackupAndRestore
./test_backup --gtest_filter=BackupTest.LargeDataBackup
./test_backup --gtest_filter=BackupTest.BackupToExistingDirectory
./test_backup --gtest_filter=BackupTest.ConcurrentBackup
./test_backup --gtest_filter=BackupTest.BackupStatistics
./test_backup --gtest_filter=BackupTest.EmptyDatabaseBackup

# 详细输出模式
./test_backup --gtest_filter=BackupTest.BasicBackup -v
```

#### 合并功能测试  
```bash
# 运行完整合并测试套件
./test_merge

# 运行单个测试用例
./test_merge --gtest_filter=MergeTest.MergeEmptyDatabase
./test_merge --gtest_filter=MergeTest.MergeRatioNotReached
./test_merge --gtest_filter=MergeTest.BasicMerge
./test_merge --gtest_filter=MergeTest.MergeAndRestart
./test_merge --gtest_filter=MergeTest.ConcurrentMerge
./test_merge --gtest_filter=MergeTest.LargeDataMerge
./test_merge --gtest_filter=MergeTest.MergeStatistics

# 详细输出模式
./test_merge --gtest_filter=MergeTest.BasicMerge -v
```

#### 迭代器功能测试
```bash
# 运行完整迭代器测试套件
./test_iterator

# 运行单个测试用例
./test_iterator --gtest_filter=DBIteratorTest.ForwardIteration
./test_iterator --gtest_filter=DBIteratorTest.ReverseIteration
./test_iterator --gtest_filter=DBIteratorTest.PrefixIteration
./test_iterator --gtest_filter=DBIteratorTest.SeekAndIterate
./test_iterator --gtest_filter=DBIteratorTest.EmptyDatabase
./test_iterator --gtest_filter=DBIteratorTest.SingleItem
./test_iterator --gtest_filter=DBIteratorTest.InvalidIterator
./test_iterator --gtest_filter=DBIteratorTest.ConcurrentIteration

# 详细输出模式
./test_iterator --gtest_filter=DBIteratorTest.ForwardIteration -v
```

### 2. 基础数据库功能测试

#### 数据库基础操作测试
```bash
# 运行完整数据库测试套件
./test_db

# 运行单个测试用例
./test_db --gtest_filter=DBTest.OpenAndClose
./test_db --gtest_filter=DBTest.PutAndGet
./test_db --gtest_filter=DBTest.GetNonExistentKey
./test_db --gtest_filter=DBTest.PutEmptyKey
./test_db --gtest_filter=DBTest.UpdateValue
./test_db --gtest_filter=DBTest.DeleteKey
./test_db --gtest_filter=DBTest.ListKeys
./test_db --gtest_filter=DBTest.DatabaseRestart
./test_db --gtest_filter=DBTest.ConcurrentOperations

# 详细输出模式
./test_db --gtest_filter=DBTest.PutAndGet -v
```

#### 批量写入测试
```bash
# 运行完整批量写入测试套件
./test_write_batch

# 运行单个测试用例
./test_write_batch --gtest_filter=WriteBatchTest.BasicBatchWrite
./test_write_batch --gtest_filter=WriteBatchTest.BatchDelete
./test_write_batch --gtest_filter=WriteBatchTest.MixedOperations
./test_write_batch --gtest_filter=WriteBatchTest.EmptyBatch
./test_write_batch --gtest_filter=WriteBatchTest.LargeBatch
./test_write_batch --gtest_filter=WriteBatchTest.ConcurrentBatches
./test_write_batch --gtest_filter=WriteBatchTest.BatchCommitRollback

# 详细输出模式
./test_write_batch --gtest_filter=WriteBatchTest.BasicBatchWrite -v
```

### 3. 索引功能测试

#### 基础索引测试
```bash
# 运行完整索引测试套件
./test_index

# 运行单个测试用例
./test_index --gtest_filter=BTreeIndexTest.*
./test_index --gtest_filter=IndexIteratorTest.*

# 详细输出模式
./test_index --gtest_filter=BTreeIndexTest.PutAndGet -v
```

#### 高级索引测试
```bash
# ART索引测试
./test_art_index

# 跳跃表索引测试
./test_advanced_index --gtest_filter=SkipListIndexTest.*

# 详细输出模式
./test_art_index -v
```

### 4. IO和数据文件测试

#### 数据文件测试
```bash
# 运行数据文件测试
./test_data_file

# 运行单个测试用例
./test_data_file --gtest_filter=DataFileTest.*

# 详细输出模式
./test_data_file -v
```

#### IO管理器测试
```bash
# 运行IO管理器测试
./test_io_manager

# 运行内存映射IO测试
./test_mmap_io

# 详细输出模式
./test_io_manager -v
./test_mmap_io -v
```

#### 日志记录测试
```bash
# 运行日志记录测试
./test_log_record

# 详细输出模式
./test_log_record -v
```

### 5. 网络服务测试

#### HTTP服务器测试
```bash
# 运行HTTP服务器测试
./test_http_server

# 详细输出模式
./test_http_server -v
```

#### Redis协议测试
```bash
# 运行Redis协议测试
./test_redis

# 详细输出模式
./test_redis -v
```

### 6. 工具类测试

#### 工具函数测试
```bash
# 运行工具函数测试
./test_test_utils

# 详细输出模式
./test_test_utils -v
```

## 示例程序编译和运行

### 1. 基础操作示例
```bash
# 编译基础示例
make bitcask_example

# 运行示例
./bitcask_example
```

### 2. HTTP服务器示例
```bash
# 编译HTTP服务器
make http_server_example

# 运行HTTP服务器（在后台）
./http_server_example &

# 测试HTTP接口
curl -X POST http://localhost:8080/put -d '{"key":"test","value":"hello"}'
curl http://localhost:8080/get/test

# 停止服务器
pkill http_server_example
```

### 3. Redis服务器示例
```bash
# 编译Redis服务器
make redis_server_example

# 运行Redis服务器（在后台）
./redis_server_example &

# 使用redis-cli测试
redis-cli -p 6379 SET mykey "hello"
redis-cli -p 6379 GET mykey

# 停止服务器
pkill redis_server_example
```

## 故障排除

### 编译错误

#### 1. GCC版本过低
```bash
# 安装更新的GCC
sudo apt install gcc-11 g++-11
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-11 60
sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-11 60
```

#### 2. GoogleTest未找到
```bash
# 手动安装GoogleTest
git clone https://github.com/google/googletest.git
cd googletest
mkdir build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local
make -j$(nproc)
sudo make install
sudo ldconfig
```

#### 3. 链接错误
```bash
# 清理重新编译
make clean
rm -rf CMakeCache.txt CMakeFiles/
cmake .. && make -j$(nproc)
```

### 运行时错误

#### 1. 权限错误
```bash
# 确保有足够权限访问/tmp目录
chmod 755 /tmp
mkdir -p /tmp/bitcask_test
chmod 777 /tmp/bitcask_test
```

#### 2. 文件系统问题
```bash
# 检查文件系统类型
df -T /tmp

# 如果是tmpfs，某些sync操作可能失败，这是正常的
# 测试程序已经处理了这种情况
```

#### 3. 内存不足
```bash
# 检查可用内存
free -h

# 减少并行编译进程
make -j2  # 使用2个进程而不是全部CPU核心
```

## 性能测试

### 基准测试
```bash
# 编译性能测试
make benchmark_tests

# 运行基准测试
./benchmark_tests

# 运行特定性能测试
./benchmark_tests --gtest_filter=BenchmarkTest.*
```

### 集成测试
```bash
# 编译集成测试
make integration_tests

# 运行集成测试
./integration_tests

# 详细输出模式
./integration_tests -v
```

## 验证完整性

### 运行所有测试
```bash
# 运行所有单元测试（使用ctest）
ctest -V

# 或手动运行所有测试程序
for test in test_*; do
    echo "Running $test..."
    ./"$test" || echo "FAILED: $test"
done
```

### 预期结果
所有测试应该显示类似以下的成功输出：
```
[==========] Running X tests from Y test suite.
[----------] Global test environment set-up.
...
[----------] Global test environment tear-down.
[==========] X tests from Y test suite ran. (XXXXms total)
[  PASSED  ] X tests.
```

如果看到 `[  FAILED  ]`，请检查具体的错误信息并参考故障排除部分。

## 部署准备

编译成功后，生产环境需要的文件：
```bash
# 静态库
libbitcask.a

# 头文件目录
include/bitcask/

# 示例程序（可选）
bitcask_example
http_server_example  
redis_server_example
```

所有文件都可以打包部署到目标系统。