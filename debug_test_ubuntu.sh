#!/bin/bash

# 调试版本的测试脚本
echo "=== 调试备份测试问题 ==="

# 检查当前目录
if [[ $(basename $(pwd)) != "build" ]]; then
    echo "请在build目录中运行此脚本"
    exit 1
fi

echo ">>> 重新编译项目..."
make -j$(nproc)

if [ $? -ne 0 ]; then
    echo "❌ 编译失败"
    exit 1
fi

echo ""
echo ">>> 运行一个简单的备份测试，查看调试输出..."

# 运行备份测试，只运行第一个失败的测试
./test_backup --gtest_filter="BackupTest.BackupAndRestore" > backup_debug.log 2>&1

echo ""
echo "=== 调试输出 ==="
cat backup_debug.log

echo ""
echo "=== 分析结果 ==="

if grep -q "\[DEBUG\]" backup_debug.log; then
    echo "✅ 调试信息已输出"
    
    echo ""
    echo "原始数据库信息："
    grep "\[DEBUG\] 开始备份" backup_debug.log -A 10
    
    echo ""
    echo "备份文件信息："
    grep "\[DEBUG\] 备份活跃文件" backup_debug.log -A 5
    
    echo ""
    echo "恢复数据库信息："
    grep "\[DEBUG\] 开始加载索引" backup_debug.log -A 20
    
else
    echo "❌ 没有找到调试信息"
fi

echo ""
echo "完整日志保存在 backup_debug.log"
