# Bitcask C++ 开发指南

本文档提供了 Bitcask C++ 项目的详细开发指南，包括架构设计、编码规范、测试策略等。

## 目录

1. [项目架构](#项目架构)
2. [开发环境设置](#开发环境设置)
3. [编码规范](#编码规范)
4. [测试策略](#测试策略)
5. [性能优化](#性能优化)
6. [调试指南](#调试指南)
7. [贡献指南](#贡献指南)

## 项目架构

### 整体架构

Bitcask C++ 采用分层架构设计：

```
┌─────────────────────────────────────┐
│            用户接口层                │
│  (DB, WriteBatch, Iterator)         │
├─────────────────────────────────────┤
│            索引管理层                │
│  (BTreeIndex, IndexIterator)        │
├─────────────────────────────────────┤
│            数据管理层                │
│  (DataFile, LogRecord)              │
├─────────────────────────────────────┤
│            IO 抽象层                │
│  (IOManager, FileIO, MMapIO)        │
└─────────────────────────────────────┘
```

### 核心组件

#### 1. 用户接口层 (User Interface Layer)

- **DB**: 主数据库类，提供 Put/Get/Delete 等基本操作
- **WriteBatch**: 批量写入接口，支持事务性操作
- **DBIterator**: 数据库迭代器，支持范围查询和前缀过滤

#### 2. 索引管理层 (Index Management Layer)

- **Indexer**: 索引接口抽象
- **BTreeIndex**: BTree 索引实现
- **IndexIterator**: 索引迭代器

#### 3. 数据管理层 (Data Management Layer)

- **DataFile**: 数据文件管理
- **LogRecord**: 日志记录格式
- **LogRecordPos**: 记录位置信息

#### 4. IO 抽象层 (IO Abstraction Layer)

- **IOManager**: IO 管理接口
- **FileIOManager**: 标准文件 IO 实现
- **MMapIOManager**: 内存映射 IO 实现

### 数据流

1. **写入流程**:
   ```
   用户调用 Put() → 编码 LogRecord → 写入 DataFile → 更新索引
   ```

2. **读取流程**:
   ```
   用户调用 Get() → 查询索引 → 根据位置读取 DataFile → 解码返回
   ```

3. **批量写入流程**:
   ```
   用户调用 WriteBatch → 缓存操作 → 批量写入 → 原子更新索引
   ```

## 开发环境设置

### 系统要求

- **操作系统**: Linux, macOS, Windows (WSL)
- **编译器**: GCC 7+ 或 Clang 6+
- **CMake**: 3.16+
- **依赖库**: zlib 或 crc32c

### 快速开始

```bash
# 1. 克隆项目
git clone <repository-url>
cd bitcask-cpp

# 2. 安装依赖
./scripts/install_deps.sh

# 3. 构建项目
./scripts/build_and_test.sh

# 4. 运行测试
./scripts/build_and_test.sh --coverage
```

### IDE 配置

#### VS Code

推荐插件：
- C/C++ Extension Pack
- CMake Tools
- Google Test Adapter

配置文件 `.vscode/settings.json`:
```json
{
    "cmake.buildDirectory": "${workspaceFolder}/build",
    "C_Cpp.default.configurationProvider": "ms-vscode.cmake-tools",
    "files.associations": {
        "*.h": "cpp",
        "*.cpp": "cpp"
    }
}
```

#### CLion

1. 打开项目根目录
2. CLion 会自动识别 CMakeLists.txt
3. 配置编译器和构建类型

## 编码规范

### 命名规范

- **类名**: PascalCase (如 `DataFile`, `LogRecord`)
- **函数名**: snake_case (如 `read_log_record`, `get_file_id`)
- **变量名**: snake_case (如 `file_id_`, `write_off_`)
- **常量名**: UPPER_CASE (如 `MAX_FILE_SIZE`, `DEFAULT_BATCH_SIZE`)
- **私有成员**: 以下划线结尾 (如 `file_id_`, `mutex_`)

### 代码风格

```cpp
// 好的例子
class DataFile {
public:
    explicit DataFile(const std::filesystem::path& path);
    
    ReadLogRecord read_log_record(uint64_t offset);
    void write_hint_record(const Bytes& key, const LogRecordPos& pos);
    
private:
    uint32_t file_id_;
    std::unique_ptr<IOManager> io_manager_;
};

// 避免的例子
class dataFile {  // 错误的命名
public:
    dataFile(std::filesystem::path path);  // 缺少 explicit
    ReadLogRecord readLogRecord(uint64_t);  // 错误的命名风格
    
private:
    uint32_t fileId;  // 错误的命名风格
};
```

### 错误处理

```cpp
// 使用异常处理错误
void DataFile::write(const Bytes& data) {
    if (data.empty()) {
        throw BitcaskException("Data cannot be empty");
    }
    
    size_t written = io_manager_->write(data.data(), data.size());
    if (written != data.size()) {
        throw BitcaskException("Failed to write all data");
    }
}

// 使用 RAII 管理资源
class FileIOManager {
private:
    int fd_;
    
public:
    FileIOManager(const std::filesystem::path& path) {
        fd_ = open(path.c_str(), O_RDWR | O_CREAT, 0644);
        if (fd_ == -1) {
            throw BitcaskException("Failed to open file");
        }
    }
    
    ~FileIOManager() {
        if (fd_ != -1) {
            close(fd_);
        }
    }
};
```

### 线程安全

```cpp
class BTreeIndex {
private:
    std::map<Bytes, LogRecordPos> tree_;
    mutable std::shared_mutex mutex_;
    
public:
    std::unique_ptr<LogRecordPos> get(const Bytes& key) {
        std::shared_lock<std::shared_mutex> lock(mutex_);  // 读锁
        // ... 实现
    }
    
    std::unique_ptr<LogRecordPos> put(const Bytes& key, const LogRecordPos& pos) {
        std::unique_lock<std::shared_mutex> lock(mutex_);  // 写锁
        // ... 实现
    }
};
```

## 测试策略

### 测试分层

1. **单元测试** (`tests/unit_tests/`):
   - 测试单个类或函数
   - 使用 Google Test 框架
   - 覆盖率目标: 90%+

2. **集成测试** (`tests/integration_tests/`):
   - 测试组件间交互
   - 端到端功能测试
   - 数据持久化测试

3. **性能测试** (`tests/benchmark_tests/`):
   - QPS 基准测试
   - 延迟分析
   - 内存使用监控

### 测试编写指南

```cpp
// 单元测试示例
class LogRecordTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_key = {0x74, 0x65, 0x73, 0x74};  // "test"
        test_value = {0x76, 0x61, 0x6c, 0x75, 0x65};  // "value"
    }
    
    Bytes test_key;
    Bytes test_value;
};

TEST_F(LogRecordTest, EncodeAndDecode) {
    LogRecord record(test_key, test_value, LogRecordType::NORMAL);
    
    auto [encoded, size] = record.encode();
    EXPECT_GT(size, 0);
    EXPECT_EQ(encoded.size(), size);
    
    // 验证 CRC 正确性
    uint32_t expected_crc = record.get_crc();
    EXPECT_GT(expected_crc, 0);
}
```

### 测试运行

```bash
# 运行所有测试
./scripts/build_and_test.sh

# 只运行单元测试
cd build && ./unit_tests

# 运行特定测试
cd build && ./unit_tests --gtest_filter="LogRecordTest.*"

# 生成覆盖率报告
./scripts/build_and_test.sh --coverage
```

## 性能优化

### 关键性能指标

- **写入 QPS**: > 50,000
- **读取 QPS**: > 100,000
- **批量写入**: > 200,000 records/sec
- **平均延迟**: < 100μs
- **P99 延迟**: < 1ms

### 优化策略

1. **内存管理**:
   ```cpp
   // 预分配容器大小
   std::vector<LogRecord> records;
   records.reserve(expected_size);
   
   // 使用移动语义
   return std::move(large_object);
   
   // 避免不必要的拷贝
   void process(const Bytes& data);  // 传引用而非值
   ```

2. **IO 优化**:
   ```cpp
   // 批量写入
   WriteBatchOptions options;
   options.max_batch_num = 10000;
   
   // 异步刷盘
   options.sync_writes = false;
   options.bytes_per_sync = 64 * 1024;  // 64KB
   ```

3. **索引优化**:
   ```cpp
   // 使用合适的数据结构
   std::map<Bytes, LogRecordPos> tree_;  // 有序访问
   
   // 读写锁分离
   std::shared_mutex mutex_;
   ```

### 性能分析工具

```bash
# CPU 性能分析
perf record ./benchmark_tests
perf report

# 内存分析
valgrind --tool=massif ./benchmark_tests
ms_print massif.out.xxx

# 锁竞争分析
valgrind --tool=drd ./benchmark_tests
```

## 调试指南

### 编译选项

```bash
# Debug 模式
cmake .. -DCMAKE_BUILD_TYPE=Debug

# 启用所有警告
cmake .. -DCMAKE_CXX_FLAGS="-Wall -Wextra -Werror"

# 启用地址消毒器
cmake .. -DCMAKE_CXX_FLAGS="-fsanitize=address"
```

### 调试技巧

1. **使用断言**:
   ```cpp
   #include <cassert>
   
   void DataFile::write(const Bytes& data) {
       assert(!data.empty());
       assert(io_manager_ != nullptr);
       // ...
   }
   ```

2. **日志输出**:
   ```cpp
   #ifdef DEBUG
   #define DEBUG_LOG(msg) std::cout << "[DEBUG] " << msg << std::endl
   #else
   #define DEBUG_LOG(msg)
   #endif
   
   DEBUG_LOG("Writing " << data.size() << " bytes");
   ```

3. **GDB 调试**:
   ```bash
   gdb ./unit_tests
   (gdb) break DataFile::write
   (gdb) run --gtest_filter="DataFileTest.WriteAndRead"
   (gdb) bt
   ```

### 常见问题

1. **内存泄漏**:
   - 使用智能指针管理内存
   - 定期运行 valgrind 检查

2. **死锁**:
   - 避免嵌套锁
   - 使用 RAII 锁管理
   - 统一锁获取顺序

3. **数据竞争**:
   - 使用线程安全的数据结构
   - 适当的同步机制

## 贡献指南

### 开发流程

1. **Fork 项目**
2. **创建特性分支**: `git checkout -b feature/amazing-feature`
3. **编写代码和测试**
4. **运行测试**: `./scripts/build_and_test.sh`
5. **提交更改**: `git commit -m 'Add amazing feature'`
6. **推送分支**: `git push origin feature/amazing-feature`
7. **创建 Pull Request**

### 代码审查清单

- [ ] 代码符合编码规范
- [ ] 包含足够的单元测试
- [ ] 测试覆盖率 > 90%
- [ ] 性能测试通过
- [ ] 文档已更新
- [ ] 无编译警告
- [ ] 内存检查通过

### 发布流程

1. **版本标记**: `git tag v1.x.x`
2. **生成变更日志**
3. **运行完整测试套件**
4. **构建发布包**
5. **更新文档**

## 附录

### 有用的命令

```bash
# 代码格式化
find . -name "*.cpp" -o -name "*.h" | xargs clang-format -i

# 静态分析
cppcheck --enable=all --std=c++17 src/ include/

# 生成文档
doxygen Doxyfile

# 清理构建
make clean && rm -rf build
```

### 参考资料

- [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html)
- [Effective Modern C++](https://www.oreilly.com/library/view/effective-modern-c/9781491908419/)
- [Google Test Documentation](https://google.github.io/googletest/)
- [CMake Documentation](https://cmake.org/documentation/)

---

*最后更新: 2024年*
