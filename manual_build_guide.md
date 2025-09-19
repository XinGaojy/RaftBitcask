# Bitcask C++ 手动编译指南

## 系统要求
- Ubuntu 22.04
- GCC 11.4.0 或更高版本
- CMake 3.22.1 或更高版本

## 依赖安装

```bash
# 更新包列表
sudo apt update

# 安装基本构建工具
sudo apt install -y build-essential cmake git

# 安装开发依赖
sudo apt install -y libgtest-dev libgmock-dev

# 如果系统没有预装GoogleTest，手动安装
cd /usr/src/gtest
sudo cmake CMakeLists.txt
sudo make
sudo cp lib/*.a /usr/lib/
```

## 手动编译步骤

### 1. 进入项目目录
```bash
cd /path/to/bitcask-cpp
```

### 2. 创建并进入构建目录
```bash
mkdir -p build
cd build
```

### 3. 配置CMake
```bash
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_STANDARD=17
```

### 4. 编译项目
```bash
make -j$(nproc)
```

### 5. 运行测试

#### 运行所有测试
```bash
# 基础功能测试
./test_basic
./test_iterator
./test_write_batch

# 关键测试 - 备份和合并
./test_backup
./test_merge

# 索引测试
./test_btree_index
./test_art_index
./test_skiplist_index
./test_bplus_tree_index

# 其他功能测试
./test_data_file
./test_log_record
./test_io_manager
./test_utils
```

#### 单独运行失败的测试
```bash
# 测试备份功能
./test_backup --gtest_filter=BackupTest.*

# 测试合并功能  
./test_merge --gtest_filter=MergeTest.*

# 详细输出模式
./test_backup --gtest_filter=BackupTest.BackupAndRestore -v
./test_merge --gtest_filter=MergeTest.BasicMerge -v
```

### 6. 编译示例程序
```bash
# 基础操作示例
g++ -std=c++17 -I../include -L. -lbitcask ../examples/basic_operations.cpp -o basic_example

# HTTP服务器示例
g++ -std=c++17 -I../include -L. -lbitcask ../examples/http_server_example.cpp -o http_example

# Redis服务器示例
g++ -std=c++17 -I../include -L. -lbitcask ../examples/redis_server_example.cpp -o redis_example
```

## 常见编译问题解决

### 1. 找不到GoogleTest
```bash
# 方法1：使用系统包管理器
sudo apt install libgtest-dev libgmock-dev

# 方法2：从源码编译
git clone https://github.com/google/googletest.git
cd googletest
mkdir build
cd build
cmake ..
make
sudo make install
```

### 2. C++17支持问题
```bash
# 确保GCC版本足够新
gcc --version  # 应该是11.4.0或更高

# 如果版本太老，升级GCC
sudo apt install gcc-11 g++-11
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-11 60
sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-11 60
```

### 3. 内存不足问题
```bash
# 减少并行编译进程数
make -j2  # 使用2个进程而不是全部CPU核心

# 或者单线程编译
make
```

## 测试通过标准

所有测试应该显示：
```
[==========] Running X tests from Y test suite.
[----------] Global test environment set-up.
...
[----------] Global test environment tear-down.
[==========] X tests from Y test suite ran. (XXXXms total)
[  PASSED  ] X tests.
```

如果看到 `[  FAILED  ]`，说明还有问题需要修复。

## 生产部署

编译成功后，可以将以下文件部署到生产环境：
- `libbitcask.a` - 静态库
- `include/bitcask/` - 头文件目录
- 示例程序（可选）

## 性能测试

```bash
# 编译性能测试程序
cd benchmarks
make benchmark

# 运行性能测试
./benchmark --benchmark_time_unit=ms
```
