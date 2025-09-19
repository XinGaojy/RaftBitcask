#!/bin/bash

# 测试sync修复效果的脚本

set -e

echo "=== 开始测试sync修复 ==="

# 进入构建目录
cd build

echo "=== 重新编译项目 ==="
make clean
make -j$(nproc)

echo "=== 编译调试程序 ==="
g++ -std=c++17 -I../include -L. -lbitcask ../debug_sync_test.cpp -o debug_sync_test

echo "=== 运行调试程序 ==="
if ./debug_sync_test; then
    echo "✓ 调试程序运行成功"
else
    echo "✗ 调试程序运行失败"
    exit 1
fi

echo ""
echo "=== 测试关键模块 ==="

echo "测试backup模块..."
if timeout 30 ./test_backup --gtest_filter=BackupTest.BasicBackup; then
    echo "✓ BackupTest.BasicBackup 通过"
else
    echo "✗ BackupTest.BasicBackup 失败"
fi

echo ""
echo "测试merge模块..."
if timeout 30 ./test_merge --gtest_filter=MergeTest.MergeEmptyDatabase; then
    echo "✓ MergeTest.MergeEmptyDatabase 通过"
else
    echo "✗ MergeTest.MergeEmptyDatabase 失败"
fi

echo ""
echo "测试iterator模块..."
if timeout 30 ./test_iterator --gtest_filter=DBIteratorTest.ForwardIteration; then
    echo "✓ DBIteratorTest.ForwardIteration 通过"
else
    echo "✗ DBIteratorTest.ForwardIteration 失败"
fi

echo ""
echo "=== 测试完成 ==="
