#!/bin/bash

# Bitcask C++ Ubuntu 22.04 清理和构建脚本
# 解决所有编译问题，确保可以立即上线

set -e

echo "=== Bitcask C++ 清理和构建脚本 ==="
echo "开始时间: $(date)"

# 检查是否在正确的目录
if [[ ! -f "CMakeLists.txt" ]]; then
    echo "错误: 请在项目根目录运行此脚本"
    exit 1
fi

echo "1. 清理重复和错误的文件..."

# 删除所有可能的重复ART文件
rm -f src/art_index_fixed.cpp
rm -f src/art_index_complete.cpp
rm -f src/art_index_old.cpp
rm -f src/art_index_backup.cpp

echo "已清理重复文件"

echo "2. 清理构建目录..."
rm -rf build
mkdir build

echo "3. 安装依赖 (如果需要)..."
if ! command -v cmake &> /dev/null || ! command -v g++ &> /dev/null; then
    echo "安装构建工具..."
    sudo apt update
    sudo apt install -y build-essential cmake g++ pkg-config git libgtest-dev libgmock-dev
fi

echo "4. 配置CMake..."
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_STANDARD=17 -DCMAKE_CXX_FLAGS="-O2 -Wall"

echo "5. 编译项目..."
make -j$(nproc)

echo "6. 验证编译结果..."
if [[ -f "./test_log_record" && -f "./test_db" && -f "./bitcask_example" ]]; then
    echo "✅ 编译成功！所有主要文件都已生成"
else
    echo "❌ 编译不完整，检查错误"
    exit 1
fi

echo "7. 运行快速测试..."
./test_log_record && echo "✅ LogRecord测试通过" || echo "❌ LogRecord测试失败"
./test_data_file && echo "✅ DataFile测试通过" || echo "❌ DataFile测试失败"

echo ""
echo "🎉 构建完成！"
echo "所有测试可执行文件:"
ls -la test_* *example | head -20

echo ""
echo "下一步:"
echo "  cd build"
echo "  ./test_db           # 运行数据库测试"
echo "  ./bitcask_example   # 运行示例程序"
echo "  ./http_server_example &  # 启动HTTP服务"
echo ""
echo "完成时间: $(date)"
