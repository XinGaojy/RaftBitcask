# Bitcask C++ 测试指南

本文档详细说明如何单独编译和运行每个测试模块。

## 快速开始

### 1. 构建项目
```bash
# 构建所有组件（包括单独的测试可执行文件）
make build

# 或者使用完整的构建脚本
./scripts/build_and_test.sh
```

### 2. 查看可用测试
```bash
# 列出所有可用的测试
make list-tests

# 或者直接使用脚本
./scripts/run_individual_tests.sh --list
```

## 单独测试运行

### 使用 Make 命令（推荐）

```bash
# 运行单个测试模块
make test-log-record      # 测试日志记录功能
make test-io-manager      # 测试IO管理器
make test-data-file       # 测试数据文件管理
make test-index           # 测试索引功能
make test-db              # 测试数据库核心
make test-write-batch     # 测试批量写入
make test-iterator        # 测试迭代器

# 运行测试组
make test-unit            # 运行所有单元测试
make test-integration     # 运行集成测试
make test-benchmark       # 运行性能测试

# 运行所有测试
make test
```

### 直接运行可执行文件

```bash
cd build

# 单独运行测试
./test_log_record
./test_io_manager
./test_data_file
./test_index
./test_db
./test_write_batch
./test_iterator

# 运行其他测试
./integration_tests
./benchmark_tests
```

### 使用测试脚本（高级选项）

```bash
# 基本用法
./scripts/run_individual_tests.sh test_log_record

# 运行特定测试用例
./scripts/run_individual_tests.sh test_db --filter="DBTest.Put*"

# 重复运行测试（用于稳定性测试）
./scripts/run_individual_tests.sh test_index --repeat=5

# 生成XML测试报告
./scripts/run_individual_tests.sh test_db --xml

# 详细输出
./scripts/run_individual_tests.sh test_db --verbose

# 运行所有测试并生成报告
./scripts/run_individual_tests.sh --all --xml
```

## 测试模块详解

### 1. test_log_record - 日志记录测试
**测试内容:**
- LogRecord 编码/解码
- 变长整数编码
- CRC32 校验
- 不同数据类型处理

**运行方式:**
```bash
make test-log-record
# 或
./scripts/run_individual_tests.sh test_log_record
```

### 2. test_io_manager - IO管理器测试
**测试内容:**
- 标准文件IO操作
- 内存映射IO操作
- 性能对比测试
- 错误处理

**运行方式:**
```bash
make test-io-manager
# 或
./scripts/run_individual_tests.sh test_io_manager
```

### 3. test_data_file - 数据文件测试
**测试内容:**
- 数据文件读写
- 多记录管理
- Hint文件操作
- IO管理器切换

**运行方式:**
```bash
make test-data-file
# 或
./scripts/run_individual_tests.sh test_data_file
```

### 4. test_index - 索引功能测试
**测试内容:**
- BTree索引操作
- 索引迭代器
- 并发访问测试
- 性能基准测试

**运行方式:**
```bash
make test-index
# 或
./scripts/run_individual_tests.sh test_index
```

### 5. test_db - 数据库核心测试
**测试内容:**
- 基本CRUD操作
- 数据持久化
- 并发操作
- 统计信息

**运行方式:**
```bash
make test-db
# 或
./scripts/run_individual_tests.sh test_db
```

### 6. test_write_batch - 批量写入测试
**测试内容:**
- 批量操作
- 事务语义
- 性能对比
- 持久化测试

**运行方式:**
```bash
make test-write-batch
# 或
./scripts/run_individual_tests.sh test_write_batch
```

### 7. test_iterator - 迭代器测试
**测试内容:**
- 正向/反向迭代
- 前缀过滤
- 大数据量迭代
- 并发迭代

**运行方式:**
```bash
make test-iterator
# 或
./scripts/run_individual_tests.sh test_iterator
```

## 高级测试选项

### 过滤特定测试用例

使用 Google Test 的过滤功能：

```bash
# 只运行 Put 相关的测试
./scripts/run_individual_tests.sh test_db --filter="*Put*"

# 运行特定测试类的所有测试
./scripts/run_individual_tests.sh test_db --filter="DBTest.*"

# 排除某些测试
./scripts/run_individual_tests.sh test_db --filter="*-*Performance*"
```

### 重复运行测试

用于稳定性和性能测试：

```bash
# 重复运行5次
./scripts/run_individual_tests.sh test_index --repeat=5

# 重复运行并生成详细报告
./scripts/run_individual_tests.sh test_db --repeat=10 --verbose --xml
```

### 生成测试报告

```bash
# 生成XML格式报告
./scripts/run_individual_tests.sh test_db --xml

# 运行所有测试并生成完整报告
./scripts/run_individual_tests.sh --all --xml

# 查看报告
open build/test_reports/index.html  # macOS
xdg-open build/test_reports/index.html  # Linux
```

### 并行运行测试

```bash
# 使用CMake的并行测试功能
cd build
ctest -j4  # 使用4个并行进程

# 或者手动并行运行
./test_log_record &
./test_io_manager &
./test_data_file &
wait  # 等待所有后台任务完成
```

## 调试测试

### 使用GDB调试

```bash
cd build

# 调试特定测试
gdb ./test_db
(gdb) run --gtest_filter="DBTest.Put"
(gdb) bt  # 查看调用栈
```

### 内存检查

```bash
# 使用Valgrind检查内存泄漏
cd build
valgrind --leak-check=full ./test_db

# 使用AddressSanitizer
cd ..
mkdir build_debug
cd build_debug
cmake .. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="-fsanitize=address"
make test_db
./test_db
```

### 性能分析

```bash
# 使用perf分析性能
cd build
perf record ./test_index
perf report

# 使用gprof
cd ..
mkdir build_profile
cd build_profile
cmake .. -DCMAKE_CXX_FLAGS="-pg"
make test_index
./test_index
gprof ./test_index gmon.out > analysis.txt
```

## 持续集成

### GitHub Actions 示例

```yaml
name: Individual Tests
on: [push, pull_request]
jobs:
  test:
    runs-on: ubuntu-latest
    steps:
    - uses: actions/checkout@v2
    - name: Install dependencies
      run: ./scripts/install_deps.sh
    - name: Build
      run: make build
    - name: Run individual tests
      run: |
        make test-log-record
        make test-io-manager
        make test-data-file
        make test-index
        make test-db
        make test-write-batch
        make test-iterator
```

### 测试矩阵

可以针对不同编译器和配置运行测试：

```bash
# GCC Debug模式
mkdir build_gcc_debug
cd build_gcc_debug
cmake .. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_COMPILER=g++
make
../scripts/run_individual_tests.sh --all

# Clang Release模式
mkdir build_clang_release
cd build_clang_release
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=clang++
make
../scripts/run_individual_tests.sh --all
```

## 故障排除

### 常见问题

1. **测试可执行文件不存在**
   ```bash
   # 确保已经构建
   make build
   # 检查构建目录
   ls build/test_*
   ```

2. **权限问题**
   ```bash
   chmod +x scripts/run_individual_tests.sh
   ```

3. **依赖库缺失**
   ```bash
   ./scripts/install_deps.sh
   ```

4. **测试超时**
   ```bash
   # 增加超时时间（在CMakeLists.txt中修改）
   set_tests_properties(${test_name} PROPERTIES TIMEOUT 600)
   ```

### 日志和调试

```bash
# 启用详细输出
./scripts/run_individual_tests.sh test_db --verbose

# 查看特定测试的输出
cd build
./test_db --gtest_print_time=1 --gtest_color=yes

# 保存测试输出到文件
./test_db > test_output.log 2>&1
```

## 最佳实践

1. **开发时使用单独测试**
   ```bash
   # 修改代码后只测试相关模块
   make test-db
   ```

2. **提交前运行完整测试**
   ```bash
   make test-unit
   ```

3. **性能回归测试**
   ```bash
   ./scripts/run_individual_tests.sh test_index --repeat=10
   make test-benchmark
   ```

4. **并行开发测试**
   ```bash
   # 团队成员可以并行测试不同模块
   make test-log-record  # 开发者A
   make test-index       # 开发者B
   make test-db          # 开发者C
   ```

通过这个测试系统，你可以高效地进行模块化开发和测试，确保代码质量和性能。
