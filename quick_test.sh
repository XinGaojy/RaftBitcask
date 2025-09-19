#!/bin/bash

# 快速测试修复效果

set -e

echo "=== 快速测试修复效果 ==="

cd build

echo "重新编译..."
make clean > /dev/null 2>&1
make -j$(nproc) > /dev/null 2>&1

echo ""
echo "=== 测试backup功能（单个测试用例） ==="
timeout 60 ./test_backup --gtest_filter=BackupTest.BasicBackup
if [ $? -eq 0 ]; then
    echo "✓ BackupTest.BasicBackup 通过"
else
    echo "✗ BackupTest.BasicBackup 失败或超时"
fi

echo ""
echo "=== 测试merge功能（单个测试用例） ==="
timeout 60 ./test_merge --gtest_filter=MergeTest.BasicMerge
if [ $? -eq 0 ]; then
    echo "✓ MergeTest.BasicMerge 通过"
else
    echo "✗ MergeTest.BasicMerge 失败或超时"
fi

echo ""
echo "=== 测试并发merge ==="
timeout 60 ./test_merge --gtest_filter=MergeTest.ConcurrentMerge
if [ $? -eq 0 ]; then
    echo "✓ MergeTest.ConcurrentMerge 通过"
else
    echo "✗ MergeTest.ConcurrentMerge 失败或超时"
fi

echo ""
echo "=== 测试merge统计 ==="
timeout 60 ./test_merge --gtest_filter=MergeTest.MergeStatistics
if [ $? -eq 0 ]; then
    echo "✓ MergeTest.MergeStatistics 通过"
else
    echo "✗ MergeTest.MergeStatistics 失败或超时"
fi

echo ""
echo "=== 快速测试完成 ==="
