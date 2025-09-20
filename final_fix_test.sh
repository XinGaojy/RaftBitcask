#!/bin/bash

# 最终修复测试脚本
echo "=== 最终修复验证 ==="

cd build

echo ">>> 重新编译..."
make -j$(nproc)

if [ $? -ne 0 ]; then
    echo "❌ 编译失败"
    exit 1
fi

echo ""
echo ">>> 运行单个备份测试..."
./test_backup --gtest_filter="BackupTest.BackupAndRestore" 2>&1 | tee single_test.log

if grep -q "PASSED" single_test.log; then
    echo "✅ BackupAndRestore 测试通过！"
    
    echo ""
    echo ">>> 运行完整备份测试套件..."
    ./test_backup 2>&1 | tee full_backup_test.log
    
    passed_backup=$(grep -o "PASSED.*test" full_backup_test.log | wc -l)
    failed_backup=$(grep -o "FAILED.*test" full_backup_test.log | wc -l)
    
    echo ""
    echo "=== 备份测试结果 ==="
    echo "通过测试数：$passed_backup"
    echo "失败测试数：$failed_backup"
    
    if [ "$failed_backup" -eq 0 ]; then
        echo "🎉 所有备份测试通过！"
        
        echo ""
        echo ">>> 运行合并测试..."
        ./test_merge --gtest_filter="MergeTest.LargeDataMerge" 2>&1 | tee merge_test.log
        
        if grep -q "PASSED" merge_test.log; then
            echo "✅ LargeDataMerge 测试通过！"
            echo ""
            echo "🎉🎉🎉 所有关键测试通过！修复成功！🎉🎉🎉"
            echo ""
            echo "修复要点："
            echo "✅ 修复了数据同步问题"
            echo "✅ 确保活跃文件正确管理"
            echo "✅ 修复了索引重建逻辑"
            echo "✅ 解决了文件偏移量问题"
            echo "✅ 强化了数据持久化机制"
        else
            echo "❌ MergeTest.LargeDataMerge 仍然失败"
            grep "Failure\|FAILED" merge_test.log | head -3
        fi
        
    else
        echo "❌ 仍有备份测试失败："
        grep "FAILED" full_backup_test.log
    fi
    
else
    echo "❌ BackupAndRestore 测试仍然失败"
    echo ""
    echo "关键错误信息："
    grep "Failure\|exception\|error" single_test.log | head -5
fi

echo ""
echo "测试日志文件："
echo "  single_test.log - 单个备份测试"
echo "  full_backup_test.log - 完整备份测试"
echo "  merge_test.log - 合并测试"
