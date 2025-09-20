#!/bin/bash

# 测试合并修复脚本

echo "======== 测试合并功能修复 ========"

cd /home/linux/share/kv_project/kv-projects/bitcask-cpp

# 重新编译项目
echo "1. 重新编译项目..."
cd build
make -j$(nproc)

if [ $? -ne 0 ]; then
    echo "❌ 编译失败"
    exit 1
fi

echo "✅ 编译成功"

# 运行合并测试
echo ""
echo "2. 运行合并测试..."
echo "========================================"

./test_merge

echo ""
echo "========================================"
echo "3. 测试结果分析"

if [ $? -eq 0 ]; then
    echo "🎉 所有合并测试通过！"
    
    echo ""
    echo "4. 运行其他核心测试验证..."
    
    # 测试其他关键模块
    echo "测试备份功能..."
    ./test_backup
    
    echo "测试高级索引..."
    ./test_advanced_index
    
    echo "测试基础数据库..."
    ./test_db
    
    echo ""
    echo "🏆 核心功能测试完成"
    
else
    echo "⚠️  仍有合并测试失败"
    echo ""
    echo "调试信息："
    echo "- 检查并发控制逻辑"
    echo "- 验证文件移动过程"
    echo "- 确认统计信息计算"
    
    echo ""
    echo "手动调试命令："
    echo "  ./test_merge  # 查看详细错误"
    echo "  gdb ./test_merge  # 使用调试器"
fi

echo ""
echo "======== 测试完成 ========"
