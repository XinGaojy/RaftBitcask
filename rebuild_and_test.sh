#!/bin/bash

# 重新编译和测试脚本

set -e  # 遇到错误立即退出

echo "=== 清理构建目录 ==="
rm -rf build
mkdir -p build
cd build

echo "=== 重新配置CMake ==="
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_STANDARD=17

echo "=== 重新编译 ==="
make clean
make -j$(nproc)

echo "=== 编译完成，开始测试 ==="

# 测试关键模块
echo ""
echo "=== 测试备份功能 ==="
if ./test_backup; then
    echo "✓ backup测试通过"
else
    echo "✗ backup测试失败"
fi

echo ""
echo "=== 测试合并功能 ==="
if ./test_merge; then
    echo "✓ merge测试通过"
else
    echo "✗ merge测试失败"
fi

echo ""
echo "=== 测试迭代器功能 ==="
if ./test_iterator; then
    echo "✓ iterator测试通过"
else
    echo "✗ iterator测试失败"
fi

echo ""
echo "=== 测试数据库基础功能 ==="
if ./test_db; then
    echo "✓ db测试通过"
else
    echo "✗ db测试失败"
fi

echo ""
echo "=== 测试批量写入功能 ==="
if ./test_write_batch; then
    echo "✓ write_batch测试通过"
else
    echo "✗ write_batch测试失败"
fi

echo ""
echo "=== 测试索引功能 ==="
if ./test_index; then
    echo "✓ index测试通过"
else
    echo "✗ index测试失败"
fi

echo ""
echo "=== 所有测试完成 ==="
