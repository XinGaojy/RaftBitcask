#!/bin/bash

echo "=== 全面测试修复结果 ==="

# 进入build目录
cd build 2>/dev/null || {
    mkdir -p build
    cd build
}

echo ">>> 1. 清理并重新构建..."
make clean 2>/dev/null
rm -rf *
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j$(nproc)

if [ $? -ne 0 ]; then
    echo "❌ 编译失败"
    exit 1
fi

echo ""
echo ">>> 2. 运行核心备份测试..."

# 测试BackupAndRestore
echo "测试 BackupTest.BackupAndRestore"
./test_backup --gtest_filter="BackupTest.BackupAndRestore" 2>&1 | tee backup_and_restore.log
if grep -q "PASSED" backup_and_restore.log && ! grep -q "FAILED" backup_and_restore.log; then
    echo "✅ BackupAndRestore 通过"
else
    echo "❌ BackupAndRestore 失败"
    echo "错误信息："
    grep -A3 -B3 "FAILED\|error\|Error" backup_and_restore.log | head -10
fi

echo ""
# 测试LargeDataBackup
echo "测试 BackupTest.LargeDataBackup"
./test_backup --gtest_filter="BackupTest.LargeDataBackup" 2>&1 | tee large_data_backup.log
if grep -q "PASSED" large_data_backup.log && ! grep -q "FAILED" large_data_backup.log; then
    echo "✅ LargeDataBackup 通过"
else
    echo "❌ LargeDataBackup 失败"
    echo "错误信息："
    grep -A3 -B3 "FAILED\|error\|Error" large_data_backup.log | head -10
fi

echo ""
echo ">>> 3. 运行merge测试..."
./test_merge --gtest_filter="MergeTest.LargeDataMerge" 2>&1 | tee merge_test.log
if grep -q "PASSED" merge_test.log && ! grep -q "FAILED" merge_test.log; then
    echo "✅ LargeDataMerge 通过"
else
    echo "❌ LargeDataMerge 失败"
    echo "错误信息："
    grep -A3 -B3 "FAILED\|error\|Error" merge_test.log | head -10
fi

echo ""
echo ">>> 4. 运行完整备份测试套件..."
./test_backup 2>&1 | tee full_backup_tests.log

passed_backup=$(grep -c "\[       OK \]" full_backup_tests.log)
failed_backup=$(grep -c "\[  FAILED  \]" full_backup_tests.log)

echo ""
echo "=== 最终结果 ==="
echo "备份测试通过数：$passed_backup"
echo "备份测试失败数：$failed_backup"

if [ "$failed_backup" -eq 0 ]; then
    echo "🎉 所有备份测试通过！"
    
    echo ""
    echo ">>> 运行完整merge测试套件..."
    ./test_merge 2>&1 | tee full_merge_tests.log
    
    passed_merge=$(grep -c "\[       OK \]" full_merge_tests.log)
    failed_merge=$(grep -c "\[  FAILED  \]" full_merge_tests.log)
    
    echo "合并测试通过数：$passed_merge"
    echo "合并测试失败数：$failed_merge"
    
    if [ "$failed_merge" -eq 0 ]; then
        echo ""
        echo "🎉🎉🎉 所有测试都通过了！修复完全成功！🎉🎉🎉"
        echo ""
        echo "关键修复点："
        echo "✅ 修复了init()中的文件偏移设置错误"
        echo "✅ 修复了语法错误和代码结构问题"
        echo "✅ 修复了merge中B+Tree索引重建问题"
        echo "✅ 改进了数据同步机制"
        echo "✅ 确保了文件管理的正确性"
    else
        echo "❌ 仍有合并测试失败："
        grep "\[  FAILED  \]" full_merge_tests.log
    fi
    
else
    echo "❌ 仍有备份测试失败："
    grep "\[  FAILED  \]" full_backup_tests.log
fi

echo ""
echo "测试日志文件："
echo "  backup_and_restore.log - BackupAndRestore测试"
echo "  large_data_backup.log - LargeDataBackup测试"  
echo "  merge_test.log - LargeDataMerge测试"
echo "  full_backup_tests.log - 完整备份测试"
echo "  full_merge_tests.log - 完整合并测试"
