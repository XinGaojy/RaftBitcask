# Bitcask C++ Ubuntu 22.04 编译运行指南

本指南详细说明了如何在Ubuntu 22.04系统上编译和运行Bitcask C++项目的完整步骤。

## 系统要求

- Ubuntu 22.04 LTS
- 至少4GB RAM
- 至少2GB可用磁盘空间

## 1. 安装依赖

### 1.1 更新系统包
```bash
sudo apt update
sudo apt upgrade -y
```

### 1.2 安装编译工具
```bash
# 安装基础编译工具
sudo apt install -y build-essential

# 安装CMake
sudo apt install -y cmake

# 安装Git
sudo apt install -y git

# 安装pkg-config
sudo apt install -y pkg-config
```

### 1.3 安装项目依赖库
```bash
# 安装CRC32C库（可选，如果没有会使用zlib）
sudo apt install -y libcrc32c-dev

# 安装zlib（备用CRC库）
sudo apt install -y zlib1g-dev

# 安装线程库（通常已包含在build-essential中）
sudo apt install -y libpthread-stubs0-dev
```

### 1.4 验证安装
```bash
# 检查编译器版本
gcc --version
g++ --version

# 检查CMake版本
cmake --version

# 检查pkg-config
pkg-config --version
```

## 2. 获取源代码

```bash
# 进入工作目录
cd /tmp

# 假设项目已经在当前目录，如果需要克隆：
# git clone <repository-url>
# cd bitcask-cpp

# 或者直接进入项目目录
cd kv-projects/bitcask-cpp
```

## 3. 编译项目

### 3.1 创建构建目录
```bash
mkdir -p build
cd build
```

### 3.2 配置项目
```bash
# 配置Release版本
cmake .. -DCMAKE_BUILD_TYPE=Release

# 或者配置Debug版本（用于开发和调试）
# cmake .. -DCMAKE_BUILD_TYPE=Debug
```

### 3.3 编译项目
```bash
# 使用所有可用CPU核心编译
make -j$(nproc)

# 或者指定核心数，例如使用4个核心
# make -j4
```

### 3.4 验证编译结果
```bash
# 检查生成的可执行文件
ls -la | grep -E "(bitcask|test|example)"

# 应该看到类似以下文件：
# - bitcask_example
# - http_server_example  
# - redis_server_example
# - test_*（各种测试文件）
# - unit_tests
# - integration_tests
# - benchmark_tests
```

## 4. 运行测试

### 4.1 运行所有单元测试
```bash
# 运行完整的单元测试套件
./unit_tests
```

### 4.2 运行单个模块测试
```bash
# 运行日志记录测试
./test_log_record

# 运行IO管理器测试
./test_io_manager

# 运行数据文件测试
./test_data_file

# 运行索引测试
./test_index

# 运行数据库测试
./test_db

# 运行批量写入测试
./test_write_batch

# 运行迭代器测试
./test_iterator

# 运行合并功能测试
./test_merge

# 运行HTTP服务器测试
./test_http_server

# 运行Redis协议测试
./test_redis
```

### 4.3 运行集成测试
```bash
./integration_tests
```

### 4.4 运行性能测试
```bash
./benchmark_tests
```

## 5. 运行示例程序

### 5.1 基本操作示例
```bash
# 运行基本的键值存储示例
./bitcask_example

# 程序会演示基本的put/get/delete操作
```

### 5.2 HTTP服务器示例
```bash
# 启动HTTP服务器（在8080端口）
./http_server_example

# 服务器启动后，在另一个终端测试：

# 添加数据
curl -X POST -d '{"key1":"value1","key2":"value2"}' \
     -H "Content-Type: application/json" \
     http://localhost:8080/bitcask/put

# 获取数据
curl "http://localhost:8080/bitcask/get?key=key1"

# 删除数据
curl -X DELETE "http://localhost:8080/bitcask/delete?key=key1"

# 列出所有键
curl "http://localhost:8080/bitcask/listkeys"

# 获取统计信息
curl "http://localhost:8080/bitcask/stat"

# 按Ctrl+C停止服务器
```

### 5.3 Redis服务器示例
```bash
# 启动Redis兼容服务器（在6380端口）
./redis_server_example

# 服务器启动后，可以使用redis-cli连接：

# 安装redis-cli（如果没有）
sudo apt install -y redis-tools

# 连接到服务器
redis-cli -p 6380

# 在redis-cli中测试命令：
# SET mykey myvalue
# GET mykey
# HSET myhash field1 value1
# HGET myhash field1
# SADD myset member1
# SISMEMBER myset member1
# LPUSH mylist element1
# LPOP mylist
# ZADD myzset 1.0 member1
# PING
# QUIT

# 按Ctrl+C停止服务器
```

## 6. 手动编译和运行单个测试

### 6.1 手动编译单个测试文件

如果需要手动编译单个测试文件，可以使用以下方法：

```bash
# 返回项目根目录
cd ..

# 手动编译日志记录测试
g++ -std=c++17 -I./include -I./build/_deps/googletest-src/googletest/include \
    -I./build/_deps/googletest-src/googlemock/include \
    tests/unit_tests/test_log_record.cpp \
    -L./build -lbitcask \
    -L./build/_deps/googletest-build/lib -lgtest_main -lgmock_main \
    -lpthread -lz -o test_log_record_manual

# 运行手动编译的测试
./test_log_record_manual
```

### 6.2 编译单个源文件

```bash
# 编译单个源文件为对象文件
g++ -std=c++17 -I./include -c src/log_record.cpp -o log_record.o

# 编译并链接创建可执行文件
g++ -std=c++17 -I./include src/log_record.cpp src/utils.cpp \
    tests/unit_tests/test_log_record.cpp \
    -lgtest_main -lgmock_main -lpthread -lz \
    -o test_log_record_standalone
```

## 7. 性能调优和优化

### 7.1 编译优化选项
```bash
# 重新配置为优化版本
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-O3 -march=native"
make clean
make -j$(nproc)
```

### 7.2 内存使用优化
```bash
# 设置内存相关环境变量
export MALLOC_TRIM_THRESHOLD=65536
export MALLOC_MMAP_THRESHOLD=65536

# 运行程序
./bitcask_example
```

## 8. 故障排除

### 8.1 常见编译错误

**错误：找不到头文件**
```bash
# 确保包含路径正确
pkg-config --cflags libcrc32c

# 如果没有crc32c，会自动使用zlib
```

**错误：链接错误**
```bash
# 检查库文件
ldconfig -p | grep -E "(crc32c|z)"

# 重新安装依赖
sudo apt install --reinstall libcrc32c-dev zlib1g-dev
```

**错误：CMake版本过低**
```bash
# 检查CMake版本
cmake --version

# 如果版本低于3.16，需要升级
sudo apt install cmake
```

### 8.2 运行时错误

**错误：权限拒绝**
```bash
# 确保可执行文件有执行权限
chmod +x ./bitcask_example
```

**错误：端口被占用**
```bash
# 检查端口使用情况
sudo netstat -tulpn | grep -E "(8080|6380)"

# 杀死占用端口的进程
sudo kill -9 <PID>
```

### 8.3 测试失败

**单元测试失败**
```bash
# 运行详细模式查看具体错误
./unit_tests --gtest_verbose

# 运行特定测试
./unit_tests --gtest_filter="*LogRecord*"
```

**内存泄漏检查**
```bash
# 安装valgrind
sudo apt install -y valgrind

# 使用valgrind检查内存泄漏
valgrind --leak-check=full ./bitcask_example
```

## 9. 部署到生产环境

### 9.1 创建系统服务

创建HTTP服务的systemd服务文件：
```bash
sudo tee /etc/systemd/system/bitcask-http.service > /dev/null <<EOF
[Unit]
Description=Bitcask HTTP Server
After=network.target

[Service]
Type=simple
User=bitcask
Group=bitcask
WorkingDirectory=/opt/bitcask
ExecStart=/opt/bitcask/http_server_example
Restart=always
RestartSec=10

[Install]
WantedBy=multi-user.target
EOF
```

创建Redis服务的systemd服务文件：
```bash
sudo tee /etc/systemd/system/bitcask-redis.service > /dev/null <<EOF
[Unit]
Description=Bitcask Redis Server
After=network.target

[Service]
Type=simple
User=bitcask
Group=bitcask
WorkingDirectory=/opt/bitcask
ExecStart=/opt/bitcask/redis_server_example
Restart=always
RestartSec=10

[Install]
WantedBy=multi-user.target
EOF
```

### 9.2 部署步骤
```bash
# 创建部署目录
sudo mkdir -p /opt/bitcask

# 创建用户
sudo useradd -r -s /bin/false bitcask

# 复制文件
sudo cp build/http_server_example /opt/bitcask/
sudo cp build/redis_server_example /opt/bitcask/
sudo cp build/libbitcask.a /opt/bitcask/

# 设置权限
sudo chown -R bitcask:bitcask /opt/bitcask
sudo chmod +x /opt/bitcask/*_example

# 启用和启动服务
sudo systemctl daemon-reload
sudo systemctl enable bitcask-http
sudo systemctl enable bitcask-redis
sudo systemctl start bitcask-http
sudo systemctl start bitcask-redis

# 检查服务状态
sudo systemctl status bitcask-http
sudo systemctl status bitcask-redis
```

## 10. 监控和日志

### 10.1 查看服务日志
```bash
# 查看HTTP服务日志
sudo journalctl -u bitcask-http -f

# 查看Redis服务日志
sudo journalctl -u bitcask-redis -f
```

### 10.2 性能监控
```bash
# 监控进程资源使用
top -p $(pgrep -f bitcask)

# 监控网络连接
ss -tulpn | grep -E "(8080|6380)"

# 监控磁盘IO
iostat -x 1
```

## 总结

本指南提供了在Ubuntu 22.04上编译和运行Bitcask C++项目的完整步骤。项目包含了所有主要功能模块：

- ✅ 核心存储引擎
- ✅ 数据合并(Merge)功能
- ✅ HTTP服务器模块
- ✅ Redis协议兼容模块
- ✅ 完整的单元测试套件
- ✅ 集成测试和性能测试

所有模块都经过充分测试，可以在生产环境中使用。如果遇到问题，请参考故障排除部分或查看项目文档。
