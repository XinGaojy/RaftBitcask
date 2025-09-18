#!/bin/bash

# 紧急修复脚本 - 彻底解决编译问题
set -e

echo "=== 紧急修复 Bitcask C++ 编译问题 ==="

# 检查当前目录
if [[ ! -f "CMakeLists.txt" ]]; then
    echo "错误: 请在项目根目录运行此脚本"
    exit 1
fi

echo "1. 彻底清理所有问题文件..."

# 删除所有可能导致冲突的文件
find . -name "*art_index_fixed*" -delete 2>/dev/null || true
find . -name "*art_index_complete*" -delete 2>/dev/null || true
find . -name "*art_index_backup*" -delete 2>/dev/null || true
find . -name "*art_index_old*" -delete 2>/dev/null || true

# 清理构建相关文件
rm -rf build
rm -rf CMakeFiles
rm -f CMakeCache.txt
rm -f cmake_install.cmake
rm -f Makefile

echo "2. 验证源文件状态..."
echo "ART索引相关文件:"
ls -la src/*art* 2>/dev/null || echo "没有找到ART相关文件"

# 确保只有正确的art_index.cpp文件
if [[ ! -f "src/art_index.cpp" ]]; then
    echo "错误: src/art_index.cpp 文件缺失"
    exit 1
fi

echo "✅ 源文件检查通过"

echo "3. 安装必需依赖..."
sudo apt update -qq
sudo apt install -y build-essential cmake g++ pkg-config git libgtest-dev libgmock-dev

echo "4. 创建新的构建目录..."
mkdir build
cd build

echo "5. 配置CMake..."
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_STANDARD=17 \
    -DCMAKE_CXX_FLAGS="-O2 -Wall" \
    -DCMAKE_VERBOSE_MAKEFILE=ON

echo "6. 编译项目..."
make -j2  # 使用较少线程避免内存问题

echo "7. 验证编译结果..."
if [[ -f "./test_log_record" && -f "./test_db" && -f "./bitcask_example" ]]; then
    echo "✅ 编译成功！"
    echo ""
    echo "生成的文件:"
    ls -la test_* *example | head -10
    echo ""
    echo "8. 运行快速验证测试..."
    ./test_log_record && echo "✅ LogRecord测试通过"
    ./test_data_file && echo "✅ DataFile测试通过"
    echo ""
    echo "🎉 修复完成！项目已准备好使用！"
else
    echo "❌ 编译失败，请检查错误信息"
    exit 1
fi
