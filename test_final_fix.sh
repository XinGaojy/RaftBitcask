#!/bin/bash

echo "=== 最终修复验证测试 ==="

cd build 2>/dev/null || {
    mkdir -p build
    cd build
}

# 重新构建
echo "构建项目..."
make clean >/dev/null 2>&1
rm -rf * >/dev/null 2>&1
cmake .. >/dev/null 2>&1
make -j$(nproc) >/dev/null 2>&1

if [ $? -ne 0 ]; then
    echo "❌ 编译失败"
    exit 1
fi

echo "✅ 编译成功"

# 测试关键的备份功能
echo ""
echo "测试备份功能..."

echo -n "BackupAndRestore: "
if ./test_backup --gtest_filter="BackupTest.BackupAndRestore" 2>&1 | grep -q "PASSED" && ! ./test_backup --gtest_filter="BackupTest.BackupAndRestore" 2>&1 | grep -q "FAILED"; then
    echo "✅ 通过"
    backup_and_restore_pass=1
else
    echo "❌ 失败"
    backup_and_restore_pass=0
fi

echo -n "LargeDataBackup: "
if ./test_backup --gtest_filter="BackupTest.LargeDataBackup" 2>&1 | grep -q "PASSED" && ! ./test_backup --gtest_filter="BackupTest.LargeDataBackup" 2>&1 | grep -q "FAILED"; then
    echo "✅ 通过"
    large_data_backup_pass=1
else
    echo "❌ 失败"
    large_data_backup_pass=0
fi

echo -n "BackupToExistingDirectory: "
if ./test_backup --gtest_filter="BackupTest.BackupToExistingDirectory" 2>&1 | grep -q "PASSED" && ! ./test_backup --gtest_filter="BackupTest.BackupToExistingDirectory" 2>&1 | grep -q "FAILED"; then
    echo "✅ 通过"
    backup_existing_dir_pass=1
else
    echo "❌ 失败"
    backup_existing_dir_pass=0
fi

echo -n "ConcurrentBackup: "
if ./test_backup --gtest_filter="BackupTest.ConcurrentBackup" 2>&1 | grep -q "PASSED" && ! ./test_backup --gtest_filter="BackupTest.ConcurrentBackup" 2>&1 | grep -q "FAILED"; then
    echo "✅ 通过"
    concurrent_backup_pass=1
else
    echo "❌ 失败"
    concurrent_backup_pass=0
fi

echo -n "BackupStatistics: "
if ./test_backup --gtest_filter="BackupTest.BackupStatistics" 2>&1 | grep -q "PASSED" && ! ./test_backup --gtest_filter="BackupTest.BackupStatistics" 2>&1 | grep -q "FAILED"; then
    echo "✅ 通过"
    backup_statistics_pass=1
else
    echo "❌ 失败"
    backup_statistics_pass=0
fi

# 测试merge功能
echo ""
echo "测试merge功能..."
echo -n "LargeDataMerge: "
if ./test_merge --gtest_filter="MergeTest.LargeDataMerge" 2>&1 | grep -q "PASSED" && ! ./test_merge --gtest_filter="MergeTest.LargeDataMerge" 2>&1 | grep -q "FAILED"; then
    echo "✅ 通过"
    large_data_merge_pass=1
else
    echo "❌ 失败"
    large_data_merge_pass=0
fi

# 统计结果
echo ""
echo "=== 测试结果汇总 ==="

backup_total=$((backup_and_restore_pass + large_data_backup_pass + backup_existing_dir_pass + concurrent_backup_pass + backup_statistics_pass))
merge_total=$large_data_merge_pass

echo "备份测试通过：$backup_total/5"
echo "合并测试通过：$merge_total/1"

if [ $backup_total -eq 5 ] && [ $merge_total -eq 1 ]; then
    echo ""
    echo "🎉🎉🎉 所有关键测试都通过了！🎉🎉🎉"
    echo ""
    echo "修复成功！关键修复点："
    echo "✅ 修复了init()中的目录逻辑错误"
    echo "✅ 修复了init()中的文件偏移设置错误"
    echo "✅ 修复了backup()中的语法错误和逻辑问题"
    echo "✅ 修复了merge()中B+Tree索引重建问题"
    echo "✅ 确保了活跃文件ID正确管理"
    echo "✅ 改进了数据同步机制"
elif [ $backup_total -eq 5 ]; then
    echo ""
    echo "✅ 所有备份测试通过！"
    echo "❌ merge测试仍有问题"
elif [ $merge_total -eq 1 ]; then
    echo ""
    echo "✅ merge测试通过！"
    echo "❌ 备份测试仍有 $((5-backup_total)) 个失败"
else
    echo ""
    echo "❌ 仍有测试失败，需要进一步调试"
    echo "备份测试失败：$((5-backup_total)) 个"
    echo "合并测试失败：$((1-merge_total)) 个"
fi
