# Bitcask C++ 项目结构

```
bitcask-cpp/
├── CMakeLists.txt          # CMake 构建配置
├── Makefile               # 简化构建的 Makefile
├── README.md              # 项目说明文档
├── PROJECT_STRUCTURE.md   # 项目结构说明
│
├── include/               # 头文件目录
│   └── bitcask/
│       ├── bitcask.h      # 主头文件
│       ├── common.h       # 公共定义和类型
│       ├── options.h      # 配置选项
│       ├── log_record.h   # 日志记录结构
│       ├── io_manager.h   # IO 管理器接口
│       ├── data_file.h    # 数据文件管理
│       ├── index.h        # 索引接口和实现
│       └── db.h           # 数据库主类
│
├── src/                   # 源文件目录
│   ├── log_record.cpp     # 日志记录实现
│   ├── io_manager.cpp     # IO 管理器实现
│   ├── data_file.cpp      # 数据文件管理实现
│   ├── index.cpp          # 索引实现
│   ├── db.cpp             # 数据库主类实现
│   ├── write_batch.cpp    # 批量写入实现
│   └── iterator.cpp       # 迭代器实现
│
├── examples/              # 示例代码
│   └── basic_operations.cpp # 基本操作示例
│
├── tests/                 # 测试代码
│   └── test_main.cpp      # 主测试程序
│
└── scripts/               # 构建脚本
    └── install_deps.sh    # 依赖安装脚本
```

## 核心组件

### 1. 数据存储层
- **log_record.h/cpp**: 日志记录的编码/解码
- **io_manager.h/cpp**: 文件IO抽象层，支持标准IO和mmap
- **data_file.h/cpp**: 数据文件管理，处理读写操作

### 2. 索引层
- **index.h/cpp**: 内存索引接口和BTree实现
- 支持正向/反向迭代
- 支持前缀过滤

### 3. 数据库层
- **db.h/cpp**: 主数据库类，协调各个组件
- **write_batch.cpp**: 事务性批量写入
- **iterator.cpp**: 数据库迭代器

### 4. 配置和工具
- **options.h**: 各种配置选项
- **common.h**: 公共类型定义和异常类
- **bitcask.h**: 用户友好的主接口

## 设计特点

1. **模块化设计**: 各组件职责清晰，易于测试和维护
2. **接口抽象**: IO管理器和索引都使用接口，便于扩展
3. **线程安全**: 使用读写锁保护共享数据
4. **异常安全**: 使用RAII和异常处理确保资源释放
5. **现代C++**: 使用C++17特性，智能指针，移动语义等

## 构建目标

- **libbitcask.a**: 静态库
- **bitcask_example**: 示例程序
- **bitcask_test**: 测试程序

## 使用方式

1. **作为库使用**: 链接 libbitcask.a 和头文件
2. **学习参考**: 查看示例代码了解用法
3. **测试验证**: 运行测试确保功能正确
