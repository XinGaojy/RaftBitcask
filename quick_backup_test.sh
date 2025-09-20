#!/bin/bash

# 快速备份测试脚本
echo "=== 快速备份功能测试 ==="

# 进入构建目录
cd build

echo ">>> 重新编译..."
make -j$(nproc)

if [ $? -ne 0 ]; then
    echo "❌ 编译失败"
    exit 1
fi

echo ""
echo ">>> 运行单个备份测试，查看调试输出..."

# 只运行一个测试用例以查看详细调试信息
./test_backup --gtest_filter="BackupTest.BackupAndRestore" > test_debug.log 2>&1

echo ""
echo "=== 测试结果 ==="
if grep -q "PASSED" test_debug.log; then
    echo "✅ 测试通过！"
else
    echo "❌ 测试失败，查看调试信息："
    echo ""
    echo ">>> 原始数据库信息："
    grep "\[DEBUG\] 开始备份" test_debug.log -A 5 | head -10
    
    echo ""
    echo ">>> 文件加载信息："
    grep "\[DEBUG\] load_data_files" test_debug.log -A 5 | head -10
    
    echo ""
    echo ">>> 索引加载信息："
    grep "\[DEBUG\] 开始加载索引" test_debug.log -A 10 | head -15
    
    echo ""
    echo ">>> 文件处理信息："
    grep "\[DEBUG\] 总共需要处理" test_debug.log -A 5 | head -10
    
    echo ""
    echo ">>> 测试失败信息："
    grep "FAILED\|Failure\|C++ exception" test_debug.log | tail -5
fi

echo ""
echo "完整日志保存在 build/test_debug.log"
echo "运行以下命令查看完整调试信息："
echo "  cat build/test_debug.log | grep '\[DEBUG\]'"
