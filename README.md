# Bitcask C++

一个高性能的键值存储引擎的C++实现，基于Bitcask存储模型。

## 特性

- **高性能读写**：追加写入，内存索引
- **数据持久性**：WAL（Write-Ahead Logging）机制
- **事务支持**：原子批量操作
- **迭代器支持**：正向、反向、前缀过滤
- **线程安全**：多线程并发访问
- **内存映射**：可选的mmap I/O
- **数据压缩**：自动合并无效数据

## 构建要求

- C++17 或更高版本
- CMake 3.16+
- GCC 7+ 或 Clang 6+
- Linux/macOS/Windows

### 依赖库

项目使用了以下外部依赖：
- `crc32c`：CRC32校验（需要安装）

## 快速开始

### 自动构建和测试

```bash
# 安装依赖（Ubuntu/Debian）
sudo apt-get install build-essential cmake libcrc32c-dev zlib1g-dev pkg-config

# 或者使用我们的脚本
./scripts/install_deps.sh

# 一键构建和测试
./scripts/build_and_test.sh

# 运行性能基准测试
./scripts/build_and_test.sh --coverage
```

### 手动构建

```bash
# 克隆仓库
git clone <repository-url>
cd bitcask-cpp

# 创建构建目录
mkdir build && cd build

# 配置和构建
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

# 运行测试
./unit_tests
./integration_tests
./benchmark_tests

# 运行示例
./bitcask_example
```

### 基本使用

```cpp
#include <bitcask/bitcask.h>

using namespace bitcask;

int main() {
    // 打开数据库
    auto db = bitcask::open("/tmp/my_database");
    
    // 写入数据
    db->put(string_to_bytes("key1"), string_to_bytes("value1"));
    db->put(string_to_bytes("key2"), string_to_bytes("value2"));
    
    // 读取数据
    Bytes value = db->get(string_to_bytes("key1"));
    std::cout << "key1 = " << bytes_to_string(value) << std::endl;
    
    // 删除数据
    db->remove(string_to_bytes("key2"));
    
    // 关闭数据库
    db->close();
    
    return 0;
}
```

### 配置选项

```cpp
Options options = Options::default_options();
options.dir_path = "/path/to/database";
options.data_file_size = 256 * 1024 * 1024;  // 256MB
options.sync_writes = false;
options.index_type = IndexType::BTREE;

auto db = DB::open(options);
```

### 批量操作

```cpp
WriteBatchOptions batch_opts = WriteBatchOptions::default_options();
auto batch = db->new_write_batch(batch_opts);

batch->put(string_to_bytes("key1"), string_to_bytes("value1"));
batch->put(string_to_bytes("key2"), string_to_bytes("value2"));
batch->remove(string_to_bytes("key3"));

batch->commit();  // 原子提交
```

### 迭代器

```cpp
// 正向迭代
IteratorOptions opts;
auto iter = db->iterator(opts);

for (iter->rewind(); iter->valid(); iter->next()) {
    std::cout << bytes_to_string(iter->key()) << " = " 
              << bytes_to_string(iter->value()) << std::endl;
}

// 前缀过滤
opts.prefix = string_to_bytes("user:");
iter = db->iterator(opts);

for (iter->rewind(); iter->valid(); iter->next()) {
    // 只迭代以"user:"开头的键
}
```

## 架构设计

### 存储模型

Bitcask使用日志结构的存储模型：

1. **数据文件**：所有写入操作都追加到活跃数据文件
2. **内存索引**：键到数据位置的映射存储在内存中
3. **合并过程**：定期合并旧文件，删除无效数据

### 文件格式

每个日志记录格式：
```
+--------+------+----------+------------+-----+-------+-----+
|  CRC   | Type | Key Size | Value Size | Key | Value | ... |
+--------+------+----------+------------+-----+-------+-----+
| 4字节  | 1字节|  变长    |    变长    |变长 | 变长  |
```

### 目录结构

```
data_directory/
├── 000000001.data    # 数据文件
├── 000000002.data
├── hint-index        # 提示索引文件
├── merge-finished    # 合并完成标记
└── flock            # 文件锁
```

## API 参考

### 数据库操作

#### `DB::open(options)`
打开或创建数据库实例。

#### `put(key, value)`
写入键值对。

#### `get(key)`
根据键读取值，如果键不存在抛出`KeyNotFoundError`。

#### `remove(key)`
删除键值对。

#### `list_keys()`
获取所有键的列表。

#### `fold(func)`
遍历所有键值对，执行自定义函数。

#### `sync()`
强制同步数据到磁盘。

#### `close()`
关闭数据库。

### 配置选项

#### `Options`
- `dir_path`：数据目录路径
- `data_file_size`：单个数据文件最大大小
- `sync_writes`：是否每次写入都同步
- `bytes_per_sync`：累计字节数同步阈值
- `index_type`：索引类型（BTREE, ART, BPLUS_TREE）
- `mmap_at_startup`：启动时是否使用内存映射
- `data_file_merge_ratio`：合并阈值

### 异常类型

- `KeyEmptyError`：键为空
- `KeyNotFoundError`：键不存在
- `DataFileNotFoundError`：数据文件不存在
- `DatabaseIsUsingError`：数据库正在被其他进程使用
- `InvalidCRCError`：CRC校验失败

## 性能测试

### 基准测试结果

在Intel i7-10700K, 32GB RAM, NVMe SSD 上的测试结果：

| 操作 | QPS | 延迟 (μs) |
|------|-----|----------|
| PUT  | 150,000 | 6.7 |
| GET  | 200,000 | 5.0 |
| DELETE | 140,000 | 7.1 |

### 优化建议

1. **批量操作**：使用`WriteBatch`提高写入性能
2. **合理配置**：根据使用场景调整`data_file_size`
3. **内存映射**：对于读密集型应用启用mmap
4. **定期合并**：设置合适的`data_file_merge_ratio`

## 测试和示例

### 完整的测试套件

项目包含三层测试：

1. **单元测试** - 测试各个模块的功能
2. **集成测试** - 测试组件间的交互
3. **性能测试** - 基准测试和性能分析

```bash
# 运行所有测试
./scripts/build_and_test.sh

# 只运行单元测试
cd build && ./unit_tests

# 运行性能基准测试
cd build && ./benchmark_tests

# 生成测试报告和覆盖率
./scripts/build_and_test.sh --coverage
```

### 示例程序

```bash
# 基本操作示例
cd build && ./bitcask_example

# 简单测试程序
cd build && ./bitcask_test
```

### 单独测试模块

每个测试模块都可以单独编译和运行：

```bash
# 使用 Make 命令运行单独测试
make test-log-record      # 日志记录编码/解码测试
make test-io-manager      # IO管理器测试
make test-data-file       # 数据文件管理测试
make test-index           # 索引功能测试
make test-db              # 数据库核心功能测试
make test-write-batch     # 批量写入测试
make test-iterator        # 迭代器功能测试

# 查看所有可用测试
make list-tests

# 运行测试组
make test-unit            # 所有单元测试
make test-integration     # 集成测试
make test-benchmark       # 性能测试
```

### 高级测试选项

```bash
# 运行特定测试用例
./scripts/run_individual_tests.sh test_db --filter="DBTest.Put*"

# 重复运行测试（稳定性测试）
./scripts/run_individual_tests.sh test_index --repeat=5

# 生成XML测试报告
./scripts/run_individual_tests.sh --all --xml

# 查看详细帮助
./scripts/run_individual_tests.sh --help

# 运行交互式演示
./scripts/demo_individual_tests.sh
```

详细的测试指南请参考：[TESTING_GUIDE.md](TESTING_GUIDE.md)

## 故障排除

### 常见问题

#### 1. 编译错误：找不到crc32c
```bash
# Ubuntu/Debian
sudo apt-get install libcrc32c-dev

# CentOS/RHEL
sudo yum install crc32c-devel

# macOS
brew install crc32c
```

#### 2. 权限错误
确保数据目录有读写权限。

#### 3. 文件锁错误
确保没有其他进程正在使用相同的数据目录。

## 贡献指南

1. Fork 这个仓库
2. 创建特性分支 (`git checkout -b feature/amazing-feature`)
3. 提交更改 (`git commit -m 'Add amazing feature'`)
4. 推送到分支 (`git push origin feature/amazing-feature`)
5. 创建 Pull Request

## 许可证

本项目采用 MIT 许可证。详见 [LICENSE](LICENSE) 文件。

## 致谢

- 感谢 Bitcask 论文的作者
- 参考了 Go 和 Rust 版本的实现
- 使用了优秀的开源库

## 版本历史

### v1.0.0
- 基本的键值存储功能
- 批量操作支持
- 迭代器实现
- 线程安全
- 持久化存储

## 联系方式

如有问题或建议，请提交 Issue 或联系维护者。
