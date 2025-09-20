# Bitcask-cpp 生产就绪指南

## 概述

Bitcask-cpp 是一个高性能的键值存储引擎，基于 Bitcask 存储模型实现，支持 HTTP 和 Redis 协议。

## 核心特性

### ✅ 完整实现的功能

1. **核心存储引擎**
   - 高性能键值存储
   - 支持 PUT/GET/DELETE 操作
   - 原子批量写入 (WriteBatch)
   - 数据持久化和恢复

2. **数据备份和恢复**
   - 完整数据备份功能
   - 支持在线备份
   - 备份数据完整性验证
   - 支持从备份恢复数据库

3. **数据合并 (Merge)**
   - 自动清理无效数据
   - 可配置合并阈值
   - 并发安全的合并操作
   - 空间回收和优化

4. **多种索引支持**
   - B-Tree 索引
   - ART (Adaptive Radix Tree) 索引
   - Skip List 索引
   - B+ Tree 索引

5. **HTTP 服务器**
   - RESTful API 接口
   - JSON 格式数据交换
   - 多线程并发处理
   - 标准HTTP协议支持

6. **Redis 协议兼容**
   - Redis 协议解析
   - 支持多种数据类型：
     - String (GET, SET, DEL, EXISTS, STRLEN)
     - Hash (HGET, HSET, HDEL, HEXISTS, HLEN, HKEYS, HVALS)
     - Set (SADD, SREM, SISMEMBER, SCARD, SMEMBERS)
     - List (LPUSH, RPUSH, LPOP, RPOP, LLEN, LRANGE)
     - Sorted Set (ZADD, ZREM, ZSCORE, ZCARD, ZRANGE)

7. **高级功能**
   - 数据库迭代器
   - 统计信息查询
   - 内存映射 I/O 支持
   - 可配置同步策略

## 系统要求

- **操作系统**: Ubuntu 22.04 或更新版本
- **编译器**: GCC 9+ 或 Clang 10+
- **CMake**: 3.16 或更新版本
- **依赖库**: 
  - libzlib1g-dev (CRC32 计算)
  - libgtest-dev (单元测试)
  - libgmock-dev (Mock 测试)

## 快速开始

### 1. 编译项目

```bash
# 克隆项目（如果需要）
cd bitcask-cpp

# 运行完整构建脚本
./build_ubuntu_complete.sh
```

### 2. 启动完整服务器

```bash
cd build
./complete_server
```

服务器将在以下地址启动：
- **HTTP 服务**: http://192.168.197.132:8080
- **Redis 服务**: redis://192.168.197.132:6379

### 3. 使用 HTTP API

```bash
# 存储数据
curl -X POST http://192.168.197.132:8080/bitcask/put \
  -H "Content-Type: application/json" \
  -d '{"key": "test_key", "value": "test_value"}'

# 读取数据
curl http://192.168.197.132:8080/bitcask/get?key=test_key

# 删除数据
curl -X DELETE http://192.168.197.132:8080/bitcask/delete?key=test_key

# 获取所有键
curl http://192.168.197.132:8080/bitcask/listkeys

# 获取统计信息
curl http://192.168.197.132:8080/bitcask/stat
```

### 4. 使用 Redis 客户端

```bash
# 安装 Redis 客户端
sudo apt-get install redis-tools

# 连接到服务器
redis-cli -h 192.168.197.132 -p 6379

# Redis 命令示例
SET mykey "Hello World"
GET mykey
HSET user:1 name "John" age "30"
HGET user:1 name
SADD myset "value1" "value2"
SMEMBERS myset
```

## API 参考

### HTTP API

#### PUT 操作
- **URL**: `POST /bitcask/put`
- **Body**: `{"key": "string", "value": "string"}`
- **Response**: `{"success": true}`

#### GET 操作
- **URL**: `GET /bitcask/get?key=<key>`
- **Response**: `{"key": "string", "value": "string"}`

#### DELETE 操作
- **URL**: `DELETE /bitcask/delete?key=<key>`
- **Response**: `{"success": true}`

#### 列出所有键
- **URL**: `GET /bitcask/listkeys`
- **Response**: `{"keys": ["key1", "key2", ...]}`

#### 统计信息
- **URL**: `GET /bitcask/stat`
- **Response**: `{"key_num": 123, "data_size": 456789, ...}`

### Redis 命令

#### String 类型
```
SET key value
GET key
DEL key
EXISTS key
STRLEN key
```

#### Hash 类型
```
HSET key field value
HGET key field
HDEL key field
HEXISTS key field
HLEN key
HKEYS key
HVALS key
```

#### Set 类型
```
SADD key member
SREM key member
SISMEMBER key member
SCARD key
SMEMBERS key
```

#### List 类型
```
LPUSH key value
RPUSH key value
LPOP key
RPOP key
LLEN key
LRANGE key start stop
```

#### Sorted Set 类型
```
ZADD key score member
ZREM key member
ZSCORE key member
ZCARD key
ZRANGE key start stop
```

## 配置选项

可以通过修改代码中的 `Options` 结构来配置数据库：

```cpp
Options options = Options::default_options();
options.dir_path = "/path/to/data";           // 数据目录
options.data_file_size = 256 * 1024 * 1024;  // 数据文件大小 (256MB)
options.sync_writes = true;                   // 同步写入
options.index_type = IndexType::BTREE;        // 索引类型
options.data_file_merge_ratio = 0.5;          // 合并阈值
```

## 性能优化

1. **索引选择**
   - BTREE: 通用场景，平衡性能
   - ART: 字符串键优化
   - SKIPLIST: 范围查询优化
   - BPLUS_TREE: 大数据集优化

2. **I/O 优化**
   - 启用内存映射: `mmap_at_startup = true`
   - 调整文件大小: `data_file_size`
   - 批量写入: 使用 `WriteBatch`

3. **合并策略**
   - 调整合并阈值: `data_file_merge_ratio`
   - 定期执行合并操作

## 监控和运维

### 数据库统计
```cpp
auto stat = db->stat();
std::cout << "Keys: " << stat.key_num << std::endl;
std::cout << "Data size: " << stat.data_size << std::endl;
std::cout << "Reclaimable size: " << stat.reclaimable_size << std::endl;
```

### 数据备份
```cpp
db->backup("/path/to/backup/directory");
```

### 数据合并
```cpp
db->merge();  // 清理无效数据，回收空间
```

## 故障排除

### 常见问题

1. **编译错误**
   - 确保安装了所有依赖
   - 检查 GCC 版本 (需要 9+)
   - 运行 `./build_ubuntu_complete.sh`

2. **服务器启动失败**
   - 检查端口是否被占用
   - 确认 IP 地址配置正确
   - 检查防火墙设置

3. **数据访问错误**
   - 检查数据目录权限
   - 确认磁盘空间充足
   - 查看错误日志

### 测试验证

```bash
# 运行所有测试
cd build
./unit_tests

# 运行特定测试
./test_backup
./test_merge
./test_db
```

## 生产部署建议

1. **系统配置**
   - 设置合适的文件描述符限制
   - 配置系统内存参数
   - 启用 SSD 优化

2. **监控**
   - 监控磁盘使用情况
   - 跟踪数据库统计信息
   - 设置告警机制

3. **备份策略**
   - 定期自动备份
   - 异地备份存储
   - 备份数据验证

4. **高可用性**
   - 主从复制（需要额外实现）
   - 负载均衡
   - 故障转移机制

## 许可证

本项目使用 MIT 许可证。

## 联系方式

如有问题或建议，请通过以下方式联系：
- 项目地址: bitcask-cpp
- 服务器IP: 192.168.197.132
