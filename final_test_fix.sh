#!/bin/bash

# 最终测试修复脚本
echo "=== Bitcask-cpp 最终测试修复脚本 ==="
echo "目标：确保所有8个备份测试用例通过"

set -e

# 检查是否在正确目录
if [ ! -f "CMakeLists.txt" ]; then
    echo "❌ 请在项目根目录运行此脚本"
    exit 1
fi

echo ">>> 清理并重新构建..."
rm -rf build
mkdir build
cd build

# 配置项目
cmake .. \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_CXX_STANDARD=17 \
    -DCMAKE_CXX_FLAGS="-O2 -D_GNU_SOURCE -pthread -fPIC -g" \
    -DCMAKE_VERBOSE_MAKEFILE=ON

# 编译
make -j$(nproc)

if [ $? -ne 0 ]; then
    echo "❌ 编译失败"
    exit 1
fi

echo "✅ 编译成功"

echo ""
echo ">>> 运行调试测试..."

# 运行单个测试查看详细信息
echo "运行 BackupAndRestore 测试..."
./test_backup --gtest_filter="BackupTest.BackupAndRestore" > debug_backup.log 2>&1

echo ""
echo "=== 调试分析 ==="

if grep -q "PASSED" debug_backup.log; then
    echo "✅ BackupAndRestore 测试通过！"
else
    echo "❌ BackupAndRestore 测试失败，分析调试信息..."
    
    echo ""
    echo ">>> 原始数据库创建："
    grep "\[DEBUG\] 开始加载索引" debug_backup.log | head -1
    grep "\[DEBUG\] 最终索引大小" debug_backup.log | head -1
    
    echo ""
    echo ">>> 备份过程："
    grep "\[DEBUG\] 开始备份" debug_backup.log
    grep "\[DEBUG\] 活跃文件复制" debug_backup.log
    
    echo ""
    echo ">>> 恢复数据库："
    grep "\[DEBUG\] 开始加载索引" debug_backup.log | tail -1
    grep "\[DEBUG\] load_data_files 找到" debug_backup.log | tail -1
    grep "\[DEBUG\] 总共需要处理" debug_backup.log | tail -1
    grep "\[DEBUG\] PUT操作数" debug_backup.log | tail -1
    grep "\[DEBUG\] DELETE操作数" debug_backup.log | tail -1
    grep "\[DEBUG\] 重建后索引大小" debug_backup.log | tail -1
    grep "\[DEBUG\] 最终索引大小" debug_backup.log | tail -1
    
    echo ""
    echo ">>> 错误信息："
    grep "Failure\|exception" debug_backup.log | head -3
fi

echo ""
echo ">>> 运行完整备份测试套件..."
./test_backup > full_backup.log 2>&1

# 统计结果
passed_tests=$(grep "PASSED" full_backup.log | wc -l)
failed_tests=$(grep "FAILED" full_backup.log | wc -l)

echo ""
echo "=== 最终测试结果 ==="
echo "通过测试数：$passed_tests"
echo "失败测试数：$failed_tests"

if [ "$failed_tests" -eq 0 ]; then
    echo "🎉 所有测试通过！备份功能修复成功！"
    echo ""
    echo "验证内容："
    echo "✅ 同步阻塞问题已解决"
    echo "✅ 备份功能正常工作"
    echo "✅ 数据恢复功能正常"
    echo "✅ 索引重建功能正常"
    echo "✅ 所有8个备份测试用例通过"
    
    echo ""
    echo "项目已准备好在Ubuntu 22.04上部署！"
    exit 0
else
    echo "❌ 仍有 $failed_tests 个测试失败"
    echo ""
    echo "失败的测试："
    grep "FAILED" full_backup.log
    
    echo ""
    echo "调试建议："
    echo "1. 查看详细日志: cat debug_backup.log | grep '\[DEBUG\]'"
    echo "2. 查看完整日志: cat full_backup.log"
    echo "3. 检查文件权限和磁盘空间"
    
    exit 1
fi
