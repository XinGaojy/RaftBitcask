#!/bin/bash

echo "=== 详细诊断测试失败原因 ==="

cd build 2>/dev/null || {
    echo "进入build目录..."
    mkdir -p build
    cd build
}

echo ">>> 1. 清理并重新构建..."
make clean 2>/dev/null
rm -rf *
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j$(nproc) 2>&1 | tee compile.log

if [ $? -ne 0 ]; then
    echo "❌ 编译失败，检查错误："
    tail -20 compile.log
    exit 1
fi

echo ""
echo ">>> 2. 运行单个备份测试 - 详细输出..."
echo "测试：BackupTest.BackupAndRestore"
./test_backup --gtest_filter="BackupTest.BackupAndRestore" --gtest_output=json:backup_result.json 2>&1 | tee backup_detail.log

echo ""
echo ">>> 3. 分析备份测试输出..."
if grep -q "PASSED" backup_detail.log; then
    echo "✅ BackupAndRestore 测试通过"
else
    echo "❌ BackupAndRestore 测试失败"
    echo "关键错误信息："
    grep -A5 -B5 "FAILED\|Failure\|Expected\|Actual" backup_detail.log
fi

echo ""
echo ">>> 4. 运行merge测试..."
./test_merge --gtest_filter="MergeTest.LargeDataMerge" 2>&1 | tee merge_detail.log

echo ""
echo ">>> 5. 分析merge测试输出..."
if grep -q "PASSED" merge_detail.log; then
    echo "✅ LargeDataMerge 测试通过"
else
    echo "❌ LargeDataMerge 测试失败"
    echo "关键错误信息："
    grep -A5 -B5 "FAILED\|Failure\|Expected\|Actual" merge_detail.log
fi

echo ""
echo ">>> 6. 运行全部备份测试..."
./test_backup 2>&1 | tee all_backup.log

failed_count=$(grep -c "FAILED" all_backup.log || echo "0")
passed_count=$(grep -c "PASSED" all_backup.log || echo "0")

echo ""
echo "=== 测试结果汇总 ==="
echo "通过的测试：$passed_count"
echo "失败的测试：$failed_count"

if [ "$failed_count" -gt 0 ]; then
    echo ""
    echo "失败的测试列表："
    grep "FAILED" all_backup.log
fi

echo ""
echo "诊断文件已生成："
echo "  compile.log - 编译日志"
echo "  backup_detail.log - 备份测试详细信息"
echo "  merge_detail.log - merge测试详细信息"
echo "  all_backup.log - 所有备份测试信息"
echo "  backup_result.json - JSON格式结果"
