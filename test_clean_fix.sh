#!/bin/bash

# 清理版本的测试修复脚本
echo "=== 清理调试信息后的测试 ==="

cd build

echo ">>> 重新编译..."
make -j$(nproc)

if [ $? -ne 0 ]; then
    echo "❌ 编译失败"
    exit 1
fi

echo ""
echo ">>> 运行 BackupAndRestore 测试..."
./test_backup --gtest_filter="BackupTest.BackupAndRestore" > clean_test.log 2>&1

echo ""
echo "=== 测试结果 ==="
if grep -q "PASSED" clean_test.log; then
    echo "✅ BackupAndRestore 测试通过！"
    
    echo ""
    echo ">>> 运行完整备份测试套件..."
    ./test_backup > full_clean_test.log 2>&1
    
    passed_tests=$(grep "PASSED" full_clean_test.log | wc -l)
    failed_tests=$(grep "FAILED" full_clean_test.log | wc -l)
    
    echo ""
    echo "=== 完整测试结果 ==="
    echo "通过测试数：$passed_tests"
    echo "失败测试数：$failed_tests"
    
    if [ "$failed_tests" -eq 0 ]; then
        echo "🎉 所有备份测试通过！"
    else
        echo "❌ 仍有测试失败："
        grep "FAILED" full_clean_test.log
    fi
    
else
    echo "❌ BackupAndRestore 测试失败"
    echo ""
    echo "关键信息："
    grep "Failure\|exception\|error" clean_test.log | head -5
fi

echo ""
echo "测试日志保存在："
echo "  clean_test.log (单个测试)"
echo "  full_clean_test.log (完整测试套件)"
