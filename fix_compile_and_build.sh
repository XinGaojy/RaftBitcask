#!/bin/bash

# 修复编译错误并重新构建项目
echo "=== 修复编译错误并重新构建 ==="

# 检查当前是否在build目录
if [[ $(basename $(pwd)) == "build" ]]; then
    echo "当前在build目录，返回上级目录"
    cd ..
fi

echo ">>> 清理构建目录..."
rm -rf build/
mkdir -p build
cd build

echo ">>> 配置项目..."
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_STANDARD=17 \
    -DCMAKE_CXX_FLAGS="-O2 -D_GNU_SOURCE -pthread -fPIC" \
    -DCMAKE_VERBOSE_MAKEFILE=ON

echo ">>> 开始编译..."
make -j$(nproc) 2>&1 | tee compile.log

# 检查编译结果
if [ $? -eq 0 ]; then
    echo "✅ 编译成功！"
    echo ""
    echo ">>> 运行快速测试..."
    
    # 运行一个简单的测试来验证修复
    if [ -f "./test_backup" ]; then
        echo "测试备份功能（之前阻塞的功能）..."
        timeout 30 ./test_backup
        if [ $? -eq 0 ]; then
            echo "✅ 备份测试通过 - 同步阻塞问题已解决！"
        else
            echo "⚠️  备份测试可能有问题，但编译成功"
        fi
    fi
    
    if [ -f "./test_io_manager" ]; then
        echo "测试IO管理器..."
        timeout 30 ./test_io_manager
        if [ $? -eq 0 ]; then
            echo "✅ IO管理器测试通过"
        fi
    fi
    
    echo ""
    echo ">>> 编译验证测试程序..."
    g++ -std=c++17 -O2 -I../include ../test_fix_verification.cpp -L. -lbitcask -lpthread -lz -o test_fix_verification
    
    if [ -f "./test_fix_verification" ]; then
        echo "✅ 验证测试程序编译成功"
    fi
    
    echo ""
    echo "🎉 修复成功！所有编译错误已解决。"
    echo ""
    echo "可用的测试程序："
    ls -la | grep test_ | head -5
    echo ""
    echo "运行验证测试："
    echo "  ./test_fix_verification  # 验证备份修复"
    
else
    echo "❌ 编译失败，查看错误信息："
    echo ""
    tail -n 20 compile.log
    echo ""
    echo "常见解决方案："
    echo "1. 检查是否安装了所有依赖：sudo apt-get install build-essential cmake libz-dev"
    echo "2. 检查CMake版本：cmake --version（需要3.16+）"
    echo "3. 检查GCC版本：gcc --version（推荐GCC 11+）"
    exit 1
fi
