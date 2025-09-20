#!/bin/bash

# 立即测试修复脚本
echo "=== 测试写入偏移修复 ==="

cd build

echo ">>> 重新编译..."
make -j$(nproc)

if [ $? -ne 0 ]; then
    echo "❌ 编译失败"
    exit 1
fi

echo ""
echo ">>> 运行单个测试用例 BackupAndRestore..."
./test_backup --gtest_filter="BackupTest.BackupAndRestore" 2>&1 | tee test_fix.log

echo ""
echo "=== 关键调试信息 ==="

echo ">>> 备份前原始数据库："
grep "\[DEBUG\] index_->size() =" test_fix.log | head -1

echo ""
echo ">>> 恢复时文件处理："
grep "\[DEBUG\] 尝试从偏移 0 读取记录" test_fix.log | head -3
grep "\[DEBUG\] 成功读取记录" test_fix.log | head -3
grep "\[DEBUG\] 文件 0 处理完成，记录数" test_fix.log | tail -1

echo ""
echo ">>> 最终索引大小："
grep "\[DEBUG\] 重建后索引大小" test_fix.log | tail -1

echo ""
echo ">>> 测试结果："
if grep -q "PASSED" test_fix.log; then
    echo "✅ 测试通过！修复成功！"
else
    echo "❌ 测试仍然失败"
    echo "关键错误："
    grep "Failure\|exception" test_fix.log | tail -2
fi

echo ""
echo "如果修复成功，运行全部测试："
echo "  ./test_backup"
