#!/bin/bash

# Bitcask C++ 简化编译脚本
# 适用于 Ubuntu 22.04

set -e  # 遇到错误立即退出

echo "=== Bitcask C++ 编译脚本 ==="

# 检查基本工具
echo "检查编译环境..."
if ! command -v gcc &> /dev/null; then
    echo "错误: GCC 未安装"
    echo "请运行: sudo apt install build-essential"
    exit 1
fi

if ! command -v cmake &> /dev/null; then
    echo "错误: CMake 未安装"
    echo "请运行: sudo apt install cmake"
    exit 1
fi

# 显示版本信息
echo "GCC 版本: $(gcc --version | head -n1)"
echo "CMake 版本: $(cmake --version | head -n1)"

# 创建构建目录
echo "创建构建目录..."
rm -rf build
mkdir -p build
cd build

# 配置CMake
echo "配置CMake..."
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_STANDARD=17

# 编译主库
echo "编译主库..."
make bitcask -j$(nproc)

# 编译示例程序
echo "编译示例程序..."
make bitcask_example

# 编译关键测试
echo "编译测试程序..."
make test_backup test_merge

echo "=== 编译完成 ==="

# 验证编译结果
echo "验证编译结果..."
if [ -f "libbitcask.a" ]; then
    echo "✓ 主库编译成功: libbitcask.a"
else
    echo "✗ 主库编译失败"
    exit 1
fi

if [ -f "bitcask_example" ]; then
    echo "✓ 示例程序编译成功: bitcask_example"
else
    echo "✗ 示例程序编译失败"
fi

if [ -f "test_backup" ]; then
    echo "✓ 备份测试编译成功: test_backup"
else
    echo "✗ 备份测试编译失败"
fi

if [ -f "test_merge" ]; then
    echo "✓ 合并测试编译成功: test_merge"
else
    echo "✗ 合并测试编译失败"
fi

echo ""
echo "=== 运行基本测试 ==="

# 创建测试目录
mkdir -p /tmp/bitcask_test
cd /tmp/bitcask_test

# 运行示例程序
echo "运行示例程序..."
if ../../../build/bitcask_example; then
    echo "✓ 示例程序运行成功"
else
    echo "✗ 示例程序运行失败"
fi

# 回到构建目录
cd ../../../build

echo ""
echo "=== 手动测试命令 ==="
echo "运行备份测试: ./test_backup"
echo "运行合并测试: ./test_merge"
echo "运行示例程序: ./bitcask_example"
echo ""
echo "运行特定测试:"
echo "./test_backup --gtest_filter=BackupTest.BasicBackup"
echo "./test_merge --gtest_filter=MergeTest.BasicMerge"
echo ""
echo "编译完成! 所有组件都可以正常使用。"
