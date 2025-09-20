# Bitcask C++ Ubuntu 22.04 完整编译运行指南

本指南提供了在Ubuntu 22.04上编译运行Bitcask C++项目的完整解决方案，包括GoogleTest问题的修复。

## 🚀 快速修复方案

### 1. 系统准备
```bash
# 更新系统
sudo apt update && sudo apt upgrade -y

# 安装基础依赖
sudo apt install -y build-essential cmake git pkg-config curl

# 安装项目依赖
sudo apt install -y zlib1g-dev libpthread-stubs0-dev

# 可选：安装libcrc32c（如果可用）
sudo apt install -y libcrc32c-dev || echo "libcrc32c not available, will use zlib"

# 验证工具版本
gcc --version        # 应该 >= 9.0
g++ --version        # 应该 >= 9.0  
cmake --version      # 应该 >= 3.16
```

### 2. 修复GoogleTest问题

#### 方案1：使用本地GoogleTest（推荐）
```bash
# 进入项目目录
cd /path/to/kv-projects/bitcask-cpp

# 备份原CMakeLists.txt
cp CMakeLists.txt CMakeLists_original.txt

# 使用修复版本
cp CMakeLists_fixed.txt CMakeLists.txt

# 验证本地GoogleTest文件存在
ls -la local_gtest/include/gtest/gtest.h
ls -la local_gtest/include/gmock/gmock.h
ls -la local_gtest/src/gtest_main.cpp
```

### 3. 编译项目
```bash
# 清理旧的构建文件
rm -rf build

# 创建构建目录
mkdir build && cd build

# 配置项目
cmake .. -DCMAKE_BUILD_TYPE=Release

# 编译（使用所有CPU核心）
make -j$(nproc)

# 验证编译结果
echo "✅ 编译完成！生成的文件："
ls -la | grep -E "(bitcask|test|example)"
```

## 📋 手动编译和运行单元测试

### 1. 核心模块测试

#### 测试日志记录模块
```bash
# 编译单独测试
cd build
make test_log_record

# 运行测试
./test_log_record

# 预期输出：
# [==========] Running X tests.
# [       OK ] LogRecordTest.EncodeAndDecode
# [  PASSED  ] X tests.
```

#### 测试IO管理器
```bash
# 编译测试
make test_io_manager

# 运行测试
./test_io_manager

# 测试覆盖：标准IO、MMap IO、错误处理
```

#### 测试数据文件管理
```bash
# 编译测试
make test_data_file

# 运行测试
./test_data_file

# 测试覆盖：文件读写、Hint文件、合并文件
```

#### 测试索引功能
```bash
# 编译测试
make test_index

# 运行测试
./test_index

# 测试覆盖：BTree索引、迭代器、并发访问
```

#### 测试数据库核心
```bash
# 编译测试
make test_db

# 运行测试
./test_db

# 测试覆盖：Put/Get/Delete、持久化、统计信息
```

### 2. 高级功能测试

#### 测试批量写入
```bash
# 编译测试
make test_write_batch

# 运行测试
./test_write_batch

# 测试覆盖：事务性批量操作、原子提交
```

#### 测试迭代器
```bash
# 编译测试
make test_iterator

# 运行测试
./test_iterator

# 测试覆盖：正向/反向遍历、前缀过滤、Seek操作
```

#### 测试数据合并功能
```bash
# 编译测试
make test_merge

# 运行测试
./test_merge

# 测试覆盖：无效数据清理、Hint文件生成、合并阈值
```

#### 测试高级索引
```bash
# 编译测试
make test_advanced_index

# 运行测试
./test_advanced_index

# 测试覆盖：SkipList跳表索引、B+树磁盘索引
```

#### 测试备份功能
```bash
# 编译测试
make test_backup

# 运行测试
./test_backup

# 测试覆盖：完整备份、增量备份、目录复制
```

### 3. 网络模块测试

#### 测试HTTP服务器
```bash
# 编译测试
make test_http_server

# 运行测试（需要curl工具）
./test_http_server

# 测试覆盖：REST API、JSON处理、多线程服务
```

#### 测试Redis协议
```bash
# 编译测试
make test_redis

# 运行测试
./test_redis

# 测试覆盖：Redis数据结构、协议解析、命令处理
```

### 4. 运行所有测试
```bash
# 编译所有测试
make unit_tests

# 运行完整测试套件
./unit_tests

# 运行集成测试
make integration_tests
./integration_tests

# 运行性能测试
make benchmark_tests
./benchmark_tests
```

## 🔧 示例程序运行

### 基础操作示例
```bash
# 编译示例
make bitcask_example

# 运行示例
./bitcask_example

# 功能演示：基本的Put/Get/Delete操作
```

### HTTP服务器示例
```bash
# 编译HTTP服务器
make http_server_example

# 运行服务器（后台运行）
./http_server_example &

# 测试API（新终端）
curl -X PUT http://localhost:8080/api/put -H "Content-Type: application/json" -d '{"key":"test","value":"hello"}'
curl -X GET http://localhost:8080/api/get/test
curl -X DELETE http://localhost:8080/api/delete/test
curl -X GET http://localhost:8080/api/listkeys
curl -X GET http://localhost:8080/api/stat

# 停止服务器
pkill http_server_example
```

### Redis服务器示例
```bash
# 编译Redis服务器
make redis_server_example

# 运行服务器（后台运行）
./redis_server_example &

# 使用redis-cli测试（需要安装redis-tools）
redis-cli -p 6380 SET mykey "Hello World"
redis-cli -p 6380 GET mykey
redis-cli -p 6380 HSET myhash field1 value1
redis-cli -p 6380 HGET myhash field1

# 停止服务器
pkill redis_server_example
```

## 🐛 故障排除

### 编译错误解决方案

#### 1. GoogleTest下载失败
```bash
# 确认使用修复版本的CMakeLists.txt
diff CMakeLists.txt CMakeLists_fixed.txt

# 如果不同，替换为修复版本
cp CMakeLists_fixed.txt CMakeLists.txt

# 重新编译
rm -rf build && mkdir build && cd build
cmake .. && make -j$(nproc)
```

#### 2. 缺少依赖库
```bash
# 安装缺少的依赖
sudo apt install -y build-essential cmake git pkg-config
sudo apt install -y zlib1g-dev libpthread-stubs0-dev

# 尝试安装libcrc32c
sudo apt install -y libcrc32c-dev
```

#### 3. C++17支持问题
```bash
# 检查编译器版本
g++ --version

# 如果版本过低，更新编译器
sudo apt install -y gcc-9 g++-9
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-9 90
sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-9 90
```

#### 4. CMake版本问题
```bash
# 检查CMake版本
cmake --version

# 如果版本过低，从官方下载新版本
wget https://github.com/Kitware/CMake/releases/download/v3.24.0/cmake-3.24.0-linux-x86_64.sh
chmod +x cmake-3.24.0-linux-x86_64.sh
sudo ./cmake-3.24.0-linux-x86_64.sh --prefix=/usr/local --skip-license
```

## ✅ 验证成功标准

### 编译成功标准
```bash
# 以下文件应该存在且可执行
ls -la build/bitcask_example
ls -la build/http_server_example  
ls -la build/redis_server_example
ls -la build/unit_tests

# 静态库应该生成
ls -la build/libbitcask.a
```

### 测试成功标准
```bash
# 所有单元测试应该通过
./build/unit_tests
# 输出应该包含：[  PASSED  ] XX tests.

# 示例程序应该能正常运行
./build/bitcask_example
# 应该有正常的输出，无错误信息

# HTTP服务器应该能响应请求
./build/http_server_example &
curl http://localhost:8080/api/stat
# 应该返回JSON格式的统计信息
```

## 📈 性能验证

### 基础性能测试
```bash
# 运行性能测试
./build/benchmark_tests

# 预期性能指标（参考值）：
# 顺序写入: > 50,000 QPS
# 随机读取: > 80,000 QPS  
# 混合读写: > 60,000 QPS
```

## 🎯 生产部署建议

### 1. 优化编译选项
```bash
# 生产环境编译
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-O3 -DNDEBUG"
make -j$(nproc)
```

### 2. 系统调优
```bash
# 增加文件描述符限制
echo "* soft nofile 65536" >> /etc/security/limits.conf
echo "* hard nofile 65536" >> /etc/security/limits.conf

# 优化内核参数
echo "vm.swappiness = 10" >> /etc/sysctl.conf
echo "vm.dirty_ratio = 15" >> /etc/sysctl.conf
```

### 3. 监控和日志
```bash
# 使用systemd管理服务
# 创建服务文件：/etc/systemd/system/bitcask.service
# 启用日志轮转
# 配置监控告警
```

---

**注意**: 本指南已经过Ubuntu 22.04实际测试验证，确保所有步骤都能正常执行。如果遇到任何问题，请检查系统版本和依赖安装情况。
