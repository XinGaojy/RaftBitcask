# Ubuntu 22.04 完整构建指南 - 最终版本

## 🚀 快速构建 (一键脚本)

```bash
#!/bin/bash
# 保存为 quick_build.sh 并执行: chmod +x quick_build.sh && ./quick_build.sh

set -e

echo "=== Bitcask C++ Ubuntu 22.04 构建脚本 ==="

# 1. 安装依赖
echo "安装依赖包..."
sudo apt update
sudo apt install -y build-essential cmake g++ pkg-config git libgtest-dev libgmock-dev

# 2. 检查编译器版本
gcc_version=$(g++ --version | head -n1 | grep -oE '[0-9]+\.[0-9]+' | head -1)
echo "检测到 GCC 版本: $gcc_version"

# 3. 创建构建目录
echo "准备构建目录..."
rm -rf build
mkdir build
cd build

# 4. 配置CMake
echo "配置 CMake..."
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_STANDARD=17

# 5. 编译项目
echo "编译项目..."
make -j$(nproc)

# 6. 运行测试
echo "运行测试..."
echo "=========================================="

test_passed=0
test_failed=0

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

for test in "${tests[@]}"; do
    if [[ -f "./$test" ]]; then
        echo "运行 $test..."
        if timeout 60 ./$test > /tmp/${test}.log 2>&1; then
            echo "✅ $test: 通过"
            ((test_passed++))
        else
            echo "❌ $test: 失败"
            echo "错误日志:"
            tail -10 /tmp/${test}.log
            ((test_failed++))
        fi
    else
        echo "⚠️  $test: 可执行文件不存在"
        ((test_failed++))
    fi
done

echo "=========================================="
echo "测试总结:"
echo "通过: $test_passed"
echo "失败: $test_failed"

if [[ $test_failed -eq 0 ]]; then
    echo "🎉 所有测试通过！项目构建成功！"
    echo ""
    echo "示例程序:"
    echo "  ./bitcask_example          # 基础操作示例"
    echo "  ./http_server_example      # HTTP服务器 (端口8080)"
    echo "  ./redis_server_example     # Redis服务器 (端口6380)"
    echo ""
    echo "API测试:"
    echo "  curl -X POST -d '{\"key\":\"value\"}' http://localhost:8080/bitcask/put"
    echo "  redis-cli -p 6380 SET mykey myvalue"
else
    echo "❌ 有测试失败，请检查错误信息"
    exit 1
fi
```

## 📋 手动构建步骤

### 步骤1: 环境准备

```bash
# 更新系统包
sudo apt update && sudo apt upgrade -y

# 安装构建工具
sudo apt install -y build-essential cmake g++ pkg-config git

# 安装测试框架
sudo apt install -y libgtest-dev libgmock-dev

# 验证安装
g++ --version    # 应该显示 9.4.0 或更高
cmake --version  # 应该显示 3.16 或更高
```

### 步骤2: 获取源码

```bash
# 如果是从git获取
git clone <your-repo-url>
cd bitcask-cpp

# 或者如果已有源码
cd /path/to/bitcask-cpp
```

### 步骤3: 配置和编译

```bash
# 创建构建目录
mkdir -p build
cd build

# 配置CMake (Release模式，C++17标准)
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_STANDARD=17 \
    -DCMAKE_CXX_FLAGS="-O2 -Wall"

# 编译 (使用所有CPU核心)
make -j$(nproc)

# 如果内存不足，使用较少核心
# make -j2
```

### 步骤4: 验证编译结果

```bash
# 检查生成的可执行文件
ls -la test_* *example

# 应该看到以下文件:
# test_log_record
# test_data_file  
# test_io_manager
# test_mmap_io
# test_index
# test_db
# test_advanced_index
# test_art_index
# test_write_batch
# test_iterator
# test_merge
# test_backup
# test_http_server
# test_redis
# test_test_utils
# bitcask_example
# http_server_example
# redis_server_example
```

## 🧪 详细测试指南

### 核心模块测试

#### 1. 日志记录模块
```bash
./test_log_record
# 预期输出: [  PASSED  ] 15 tests.
```

#### 2. 数据文件模块  
```bash
./test_data_file
# 预期输出: [  PASSED  ] 15 tests.
```

#### 3. IO管理器模块
```bash
./test_io_manager
# 预期输出: [  PASSED  ] 14 tests.
```

#### 4. 内存映射IO模块
```bash
./test_mmap_io
# 预期输出: [  PASSED  ] 8 tests.
```

#### 5. 索引模块
```bash
./test_index
# 预期输出: [  PASSED  ] 22 tests.
```

#### 6. 数据库核心模块
```bash
./test_db
# 预期输出: [  PASSED  ] 27 tests.
```

### 高级功能测试

#### 7. 高级索引模块
```bash
./test_advanced_index
# 预期输出: [  PASSED  ] 15 tests.
```

#### 8. ART索引模块
```bash
./test_art_index  
# 预期输出: [  PASSED  ] 12 tests.
```

#### 9. 批量写入模块
```bash
./test_write_batch
# 预期输出: [  PASSED  ] 16 tests.
```

#### 10. 迭代器模块
```bash
./test_iterator
# 预期输出: [  PASSED  ] 20 tests.
```

#### 11. 数据合并模块
```bash
./test_merge
# 预期输出: [  PASSED  ] 7 tests.
```

#### 12. 数据备份模块
```bash
./test_backup
# 预期输出: [  PASSED  ] 8 tests.
```

### 服务器功能测试

#### 13. HTTP服务器模块
```bash
./test_http_server
# 预期输出: [  PASSED  ] 10 tests.
```

#### 14. Redis协议模块
```bash
./test_redis
# 预期输出: [  PASSED  ] 10 tests.
```

#### 15. 测试工具模块
```bash
./test_test_utils
# 预期输出: [  PASSED  ] 16 tests.
```

## 🎯 示例程序运行

### 1. 基础操作示例
```bash
./bitcask_example

# 预期输出:
# Bitcask C++ Example
# ===================
# Using directory: /tmp/bitcask_example
# 
# Opening database...
# 1. Basic Put/Get operations:
# name = Bitcask
# version = 1.0.0
# language = C++
# ...
```

### 2. HTTP服务器
```bash
# 启动服务器
./http_server_example &

# 测试API
curl -X POST -H "Content-Type: application/json" \
     -d '{"key1":"value1","key2":"value2"}' \
     http://localhost:8080/bitcask/put

curl http://localhost:8080/bitcask/get?key=key1

curl http://localhost:8080/bitcask/listkeys

curl http://localhost:8080/bitcask/stat

# 停止服务器
pkill http_server_example
```

### 3. Redis服务器
```bash
# 启动服务器
./redis_server_example &

# 安装redis-cli (如果没有)
sudo apt install redis-tools

# 测试Redis命令
redis-cli -p 6380 SET mykey myvalue
redis-cli -p 6380 GET mykey
redis-cli -p 6380 HSET myhash field1 value1
redis-cli -p 6380 HGET myhash field1
redis-cli -p 6380 PING

# 停止服务器
pkill redis_server_example
```

## 🔧 故障排除

### 编译错误

**1. CMake版本过低**
```bash
# 错误: CMake 3.16 or higher is required
# 解决方案: 升级CMake
sudo apt remove cmake
sudo snap install cmake --classic
```

**2. C++17支持问题**
```bash
# 错误: This file requires compiler and library support for the ISO C++ 2017 standard
# 解决方案: 升级GCC
sudo apt install g++-9
sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-9 90
```

**3. Google Test未找到**
```bash
# 错误: Could NOT find GTest
# 解决方案: 安装Google Test
sudo apt install libgtest-dev libgmock-dev
```

**4. 链接错误**
```bash
# 错误: undefined reference to pthread_create
# 解决方案: 添加pthread链接
export LDFLAGS="-lpthread"
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_STANDARD=17
make clean && make -j$(nproc)
```

### 运行时错误

**1. 权限拒绝**
```bash
# 错误: Permission denied when creating /tmp files
# 解决方案: 修复权限
sudo chmod 777 /tmp
mkdir -p ~/.bitcask_temp
export TMPDIR=~/.bitcask_temp
```

**2. 端口占用**
```bash
# 错误: Address already in use
# 解决方案: 查找并终止占用进程
sudo netstat -tulpn | grep :8080
sudo kill -9 <PID>
```

**3. 内存不足**
```bash
# 错误: virtual memory exhausted
# 解决方案: 增加交换空间或减少编译并发
sudo fallocate -l 2G /swapfile
sudo chmod 600 /swapfile
sudo mkswap /swapfile
sudo swapon /swapfile

# 或者减少编译线程
make -j2  # 而不是 make -j$(nproc)
```

## ✅ 完整验证脚本

创建验证脚本 `verify_build.sh`:

```bash
#!/bin/bash

echo "=== Bitcask C++ 构建验证 ==="

# 检查所有可执行文件
echo "检查可执行文件..."
required_files=(
    "test_log_record" "test_data_file" "test_io_manager" "test_mmap_io"
    "test_index" "test_db" "test_advanced_index" "test_art_index"
    "test_write_batch" "test_iterator" "test_merge" "test_backup"
    "test_http_server" "test_redis" "test_test_utils"
    "bitcask_example" "http_server_example" "redis_server_example"
)

missing_files=0
for file in "${required_files[@]}"; do
    if [[ ! -f "./$file" ]]; then
        echo "❌ 缺少文件: $file"
        ((missing_files++))
    else
        echo "✅ 找到文件: $file"
    fi
done

if [[ $missing_files -gt 0 ]]; then
    echo "❌ 构建不完整，缺少 $missing_files 个文件"
    exit 1
fi

# 运行快速测试
echo ""
echo "运行快速验证测试..."
quick_tests=("test_log_record" "test_data_file" "test_index")
failed_tests=0

for test in "${quick_tests[@]}"; do
    echo "测试 $test..."
    if timeout 30 ./$test >/dev/null 2>&1; then
        echo "✅ $test 通过"
    else
        echo "❌ $test 失败"
        ((failed_tests++))
    fi
done

if [[ $failed_tests -eq 0 ]]; then
    echo ""
    echo "🎉 构建验证成功！"
    echo "所有文件存在且核心测试通过。"
    echo ""
    echo "下一步:"
    echo "1. 运行完整测试: for test in test_*; do echo \"Testing \$test\"; ./\$test; done"
    echo "2. 启动HTTP服务: ./http_server_example &"
    echo "3. 启动Redis服务: ./redis_server_example &"
    echo "4. 运行示例程序: ./bitcask_example"
else
    echo "❌ 验证失败，有 $failed_tests 个测试失败"
    exit 1
fi
```

使用验证脚本:
```bash
chmod +x verify_build.sh
./verify_build.sh
```

## 🚀 生产部署

构建成功后，可以进行生产部署:

```bash
# 1. 创建生产目录
sudo mkdir -p /opt/bitcask
sudo chown $USER:$USER /opt/bitcask

# 2. 复制可执行文件
cp http_server_example /opt/bitcask/
cp redis_server_example /opt/bitcask/
cp bitcask_example /opt/bitcask/

# 3. 创建数据目录
mkdir -p /opt/bitcask/data

# 4. 创建systemd服务文件
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

# 5. 启动服务
sudo systemctl daemon-reload
sudo systemctl enable bitcask-http
sudo systemctl start bitcask-http
sudo systemctl status bitcask-http
```

现在您的Bitcask C++存储引擎已经完全准备好在Ubuntu 22.04上运行了！
