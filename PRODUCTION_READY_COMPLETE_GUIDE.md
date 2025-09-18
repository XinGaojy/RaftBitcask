# Bitcask-cpp 生产环境完整部署指南

本文档提供了 bitcask-cpp 项目在 Ubuntu 22.04 生产环境中的完整部署、编译、测试和维护指南。

## 项目概述

bitcask-cpp 是一个高性能的键值存储引擎，基于 Bitcask 模型实现，具有以下特点：

- **高性能写入**：追加写入模式，写入性能优异
- **快速读取**：内存索引，读取性能卓越
- **多种索引**：支持 BTree、ART、SkipList、B+Tree 等索引类型
- **数据持久化**：可靠的数据持久化机制
- **并发安全**：支持多线程并发读写
- **HTTP/Redis 兼容**：提供 HTTP API 和 Redis 协议兼容
- **完整测试**：全面的单元测试和集成测试

## 系统要求

### 硬件要求
- **CPU**: 2核以上（推荐4核+）
- **内存**: 4GB以上（推荐8GB+）
- **磁盘**: 20GB可用空间（推荐SSD）
- **网络**: 标准网络接口

### 软件要求
- **操作系统**: Ubuntu 22.04 LTS
- **编译器**: GCC 9.0+ 或 Clang 10.0+
- **构建工具**: CMake 3.16+
- **依赖库**: 详见下方安装说明

## 快速部署

### 1. 环境准备
```bash
# 更新系统
sudo apt update && sudo apt upgrade -y

# 安装编译环境
sudo apt install -y build-essential cmake gcc g++ pkg-config

# 安装依赖库
sudo apt install -y libssl-dev zlib1g-dev libcrc32c-dev
sudo apt install -y libgtest-dev libgmock-dev

# 安装调试工具（可选）
sudo apt install -y valgrind gdb curl redis-tools
```

### 2. 编译项目
```bash
cd /home/linux/share/kv_project/kv-projects/bitcask-cpp
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

### 3. 验证安装
```bash
# 运行基本测试
./test_db

# 运行示例程序
./bitcask_example
```

## 详细编译指南

### 使用 CMake（推荐）
```bash
# 配置项目
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_STANDARD=17 \
    -DCMAKE_CXX_STANDARD_REQUIRED=ON \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

# 编译项目
make -j$(nproc)

# 安装（可选）
sudo make install
```

### 手动编译
详细的手动编译命令请参考 `UBUNTU_MANUAL_COMPILATION_COMMANDS.md`

## 功能模块详解

### 1. 核心存储引擎
- **日志记录**: 高效的二进制日志格式
- **数据文件**: 追加写入的数据文件管理
- **索引系统**: 多种高性能索引实现
- **数据库引擎**: 完整的键值数据库功能

### 2. 索引类型
- **BTree**: 平衡二叉搜索树，适合通用场景
- **ART**: 自适应基数树，内存高效
- **SkipList**: 跳跃列表，并发友好
- **B+Tree**: B+树，适合范围查询

### 3. 存储特性
- **文件轮转**: 自动数据文件轮转和管理
- **数据合并**: 定期数据合并以回收空间
- **批量操作**: 高效的批量写入支持
- **事务支持**: ACID 事务特性

### 4. 网络服务
- **HTTP API**: RESTful HTTP 接口
- **Redis 兼容**: Redis 协议兼容层
- **并发处理**: 高并发网络请求处理

## 性能优化配置

### 1. 数据库配置
```cpp
Options options;
options.data_file_size = 256 * 1024 * 1024;  // 256MB 数据文件
options.sync_writes = false;                  // 异步写入
options.bytes_per_sync = 1024 * 1024;        // 1MB 同步阈值
options.index_type = IndexType::ART;         // 使用 ART 索引
options.mmap_at_startup = true;              // 启动时使用内存映射
options.data_file_merge_ratio = 0.5;         // 50% 合并阈值
```

### 2. 系统优化
```bash
# 增加文件描述符限制
echo "* soft nofile 65536" >> /etc/security/limits.conf
echo "* hard nofile 65536" >> /etc/security/limits.conf

# 优化内核参数
echo "vm.dirty_ratio = 5" >> /etc/sysctl.conf
echo "vm.dirty_background_ratio = 2" >> /etc/sysctl.conf
sysctl -p
```

## 生产部署配置

### 1. 服务配置
创建系统服务文件 `/etc/systemd/system/bitcask-http.service`：
```ini
[Unit]
Description=Bitcask HTTP Server
After=network.target

[Service]
Type=simple
User=bitcask
Group=bitcask
ExecStart=/opt/bitcask/bin/http_server_example
WorkingDirectory=/opt/bitcask
Restart=always
RestartSec=10

[Install]
WantedBy=multi-user.target
```

### 2. 用户权限
```bash
# 创建专用用户
sudo useradd -r -s /bin/false bitcask

# 创建数据目录
sudo mkdir -p /var/lib/bitcask
sudo chown bitcask:bitcask /var/lib/bitcask
sudo chmod 750 /var/lib/bitcask

# 创建日志目录
sudo mkdir -p /var/log/bitcask
sudo chown bitcask:bitcask /var/log/bitcask
```

### 3. 启动服务
```bash
sudo systemctl daemon-reload
sudo systemctl enable bitcask-http
sudo systemctl start bitcask-http
sudo systemctl status bitcask-http
```

## 监控和维护

### 1. 性能监控
```bash
# CPU 和内存使用
top -p $(pgrep bitcask)

# 磁盘IO监控
iostat -x 1

# 网络连接监控
netstat -tulpn | grep :8080
```

### 2. 日志管理
```bash
# 查看服务日志
sudo journalctl -u bitcask-http -f

# 查看应用日志
tail -f /var/log/bitcask/app.log
```

### 3. 数据备份
```bash
# 创建数据备份
sudo -u bitcask rsync -av /var/lib/bitcask/ /backup/bitcask/$(date +%Y%m%d)/

# 数据库热备份
curl -X POST http://localhost:8080/bitcask/backup -d '{"path": "/backup/bitcask/hot"}'
```

## 测试验证

### 1. 单元测试
```bash
# 运行全部单元测试
cd build
./unit_tests

# 运行特定模块测试
./test_db
./test_art_index
./test_http_server
```

### 2. 集成测试
```bash
# 运行集成测试
./integration_tests

# 性能基准测试
./benchmark_tests
```

### 3. 负载测试
```bash
# HTTP API 负载测试
curl -X PUT http://localhost:8080/bitcask/load_test_key -d "load_test_value"

# 批量写入测试
for i in {1..1000}; do
    curl -X PUT http://localhost:8080/bitcask/key_$i -d "value_$i"
done
```

## 故障排除

### 1. 常见问题
- **编译失败**: 检查依赖库安装
- **启动失败**: 检查权限和配置
- **性能问题**: 调整配置参数
- **内存泄漏**: 使用 Valgrind 检查

### 2. 调试工具
```bash
# 内存检查
valgrind --leak-check=full ./bitcask_example

# 核心转储分析
gdb ./http_server_example core

# 系统调用跟踪
strace -p $(pgrep bitcask)
```

### 3. 性能分析
```bash
# CPU 性能分析
perf record -g ./benchmark_tests
perf report

# 内存分析
heaptrack ./bitcask_example
```

## API 使用指南

### 1. HTTP API
```bash
# 基本操作
curl -X PUT http://localhost:8080/bitcask/mykey -d "myvalue"
curl http://localhost:8080/bitcask/mykey
curl -X DELETE http://localhost:8080/bitcask/mykey

# 批量操作
curl -X POST http://localhost:8080/bitcask/batch -d '{"operations": [...]}'

# 统计信息
curl http://localhost:8080/bitcask/stat
```

### 2. Redis 兼容
```bash
# 连接 Redis 客户端
redis-cli -p 6380

# 基本命令
SET mykey myvalue
GET mykey
DEL mykey
KEYS *
```

### 3. C++ API
```cpp
#include "bitcask/bitcask.h"

// 打开数据库
auto db = bitcask::open("/path/to/data");

// 基本操作
db->put(key, value);
auto value = db->get(key);
db->remove(key);

// 批量操作
auto batch = db->new_write_batch();
batch->put(key1, value1);
batch->put(key2, value2);
batch->commit();
```

## 升级和迁移

### 1. 版本升级
```bash
# 停止服务
sudo systemctl stop bitcask-http

# 备份数据
sudo -u bitcask cp -r /var/lib/bitcask /backup/before_upgrade

# 升级程序
sudo cp new_version/bin/* /opt/bitcask/bin/

# 启动服务
sudo systemctl start bitcask-http
```

### 2. 数据迁移
```bash
# 导出数据
./bitcask_export --input /old/data --output /tmp/export.json

# 导入数据
./bitcask_import --input /tmp/export.json --output /new/data
```

## 安全配置

### 1. 网络安全
```bash
# 防火墙配置
sudo ufw allow from 192.168.1.0/24 to any port 8080
sudo ufw allow from 192.168.1.0/24 to any port 6380
```

### 2. 访问控制
```cpp
// 配置认证
HttpServerOptions http_options;
http_options.enable_auth = true;
http_options.auth_token = "your-secret-token";
```

### 3. 数据加密（如需要）
```cpp
// 配置加密选项
Options options;
options.encryption_key = "your-encryption-key";
```

## 最佳实践

### 1. 数据设计
- 合理设计键的命名规则
- 控制值的大小（建议小于1MB）
- 定期清理过期数据

### 2. 性能优化
- 根据场景选择合适的索引类型
- 合理配置文件大小和合并参数
- 监控内存使用情况

### 3. 运维管理
- 定期备份数据
- 监控磁盘空间使用
- 及时处理告警信息

## 技术支持

### 1. 文档资源
- `README.md`: 项目概述
- `UBUNTU_MANUAL_BUILD_TEST.md`: 详细测试指南
- `UBUNTU_MANUAL_COMPILATION_COMMANDS.md`: 手动编译命令

### 2. 问题报告
如遇到问题，请提供：
- 系统环境信息
- 错误日志
- 复现步骤
- 配置信息

### 3. 性能调优
根据具体使用场景，可能需要调整：
- 内存分配策略
- 磁盘IO参数
- 网络连接配置
- 索引类型选择

## 结论

bitcask-cpp 是一个功能完整、性能优异的键值存储引擎，适用于各种生产环境。通过本指南的配置和优化，可以确保系统在生产环境中稳定高效地运行。

定期的监控、备份和维护是保证系统长期稳定运行的关键。建议根据实际使用情况调整配置参数，以获得最佳性能。
