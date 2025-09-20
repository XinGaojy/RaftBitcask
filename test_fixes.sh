#!/bin/bash

# 验证修复效果的测试脚本

set -e

echo "=== 验证修复效果 ==="

cd build

echo "重新编译项目..."
make clean > /dev/null 2>&1
make -j$(nproc) > /dev/null 2>&1

echo ""
echo "=== 测试backup功能 ==="

echo "测试BasicBackup..."
timeout 120 ./test_backup --gtest_filter=BackupTest.BasicBackup
if [ $? -eq 0 ]; then
    echo "✓ BackupTest.BasicBackup 通过"
else
    echo "✗ BackupTest.BasicBackup 失败或超时"
fi

echo ""
echo "测试BackupAndRestore..."
timeout 120 ./test_backup --gtest_filter=BackupTest.BackupAndRestore
if [ $? -eq 0 ]; then
    echo "✓ BackupTest.BackupAndRestore 通过"
else
    echo "✗ BackupTest.BackupAndRestore 失败或超时"
fi

echo ""
echo "=== 测试merge功能 ==="

echo "测试LargeDataMerge..."
timeout 180 ./test_merge --gtest_filter=MergeTest.LargeDataMerge
if [ $? -eq 0 ]; then
    echo "✓ MergeTest.LargeDataMerge 通过"
else
    echo "✗ MergeTest.LargeDataMerge 失败或超时"
fi

echo ""
echo "测试MergeStatistics..."
timeout 180 ./test_merge --gtest_filter=MergeTest.MergeStatistics
if [ $? -eq 0 ]; then
    echo "✓ MergeTest.MergeStatistics 通过"
else
    echo "✗ MergeTest.MergeStatistics 失败或超时"
fi

echo ""
echo "=== 测试db功能中的backup ==="

echo "测试DBBackupTest.BackupRestore..."
timeout 120 ./test_db --gtest_filter=DBBackupTest.BackupRestore
if [ $? -eq 0 ]; then
    echo "✓ DBBackupTest.BackupRestore 通过"
else
    echo "✗ DBBackupTest.BackupRestore 失败或超时"
fi

echo ""
echo "=== 修复验证完成 ==="

echo ""
echo "运行完整测试套件验证..."
echo "backup测试："
timeout 300 ./test_backup
backup_result=$?

echo ""
echo "merge测试："
timeout 300 ./test_merge
merge_result=$?

echo ""
echo "=== 最终结果 ==="
if [ $backup_result -eq 0 ] && [ $merge_result -eq 0 ]; then
    echo "✓ 所有测试通过！修复成功！"
    exit 0
else
    echo "✗ 仍有测试失败，需要进一步修复"
    exit 1
fi
