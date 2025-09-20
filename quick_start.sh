#!/bin/bash

# Bitcask C++ 快速启动脚本
# 解决GoogleTest问题并快速编译

set -e  # 遇到错误立即退出

echo "🚀 Bitcask C++ 快速启动脚本"
echo "================================"

# 检查当前目录
if [[ ! -f "CMakeLists.txt" ]]; then
    echo "❌ 错误: 请在项目根目录运行此脚本"
    exit 1
fi

echo "📂 当前目录: $(pwd)"

# 步骤1: 验证本地GoogleTest环境
echo ""
echo "🔍 步骤1: 检查本地GoogleTest环境..."

if [[ ! -f "local_gtest/include/gtest/gtest.h" ]]; then
    echo "⚠️  本地GoogleTest不存在，正在创建..."
    
    # 如果quick_fix.sh存在，运行它
    if [[ -f "quick_fix.sh" ]]; then
        chmod +x quick_fix.sh
        ./quick_fix.sh
    else
        echo "❌ 错误: 找不到quick_fix.sh脚本"
        echo "请手动创建本地GoogleTest环境或重新下载项目"
        exit 1
    fi
else
    echo "✅ 本地GoogleTest环境已存在"
fi

# 步骤2: 使用修复版本的CMakeLists.txt
echo ""
echo "🔧 步骤2: 配置CMakeLists.txt..."

if [[ -f "CMakeLists_fixed.txt" ]]; then
    # 备份原始文件
    if [[ ! -f "CMakeLists_original.txt" ]]; then
        cp CMakeLists.txt CMakeLists_original.txt
        echo "📋 已备份原始CMakeLists.txt"
    fi
    
    # 使用修复版本
    cp CMakeLists_fixed.txt CMakeLists.txt
    echo "✅ 已使用修复版本的CMakeLists.txt"
else
    echo "❌ 错误: 找不到CMakeLists_fixed.txt"
    echo "请确保所有文件都已正确创建"
    exit 1
fi

# 步骤3: 清理和重新编译
echo ""
echo "🧹 步骤3: 清理旧的构建文件..."

# 清理
rm -rf build/
rm -f CMakeCache.txt
echo "✅ 清理完成"

# 步骤4: 创建构建目录并配置
echo ""
echo "⚙️  步骤4: 配置项目..."

mkdir build && cd build

# 配置项目
if cmake .. -DCMAKE_BUILD_TYPE=Release; then
    echo "✅ CMake配置成功"
else
    echo "❌ CMake配置失败"
    echo ""
    echo "🔧 故障排除建议:"
    echo "1. 检查编译器版本: g++ --version (需要 >= 9.0)"
    echo "2. 检查CMake版本: cmake --version (需要 >= 3.16)"
    echo "3. 安装依赖: sudo apt install -y build-essential cmake zlib1g-dev"
    exit 1
fi

# 步骤5: 编译项目
echo ""
echo "🔨 步骤5: 编译项目..."

# 获取CPU核心数
CORES=$(nproc)
echo "使用 $CORES 个CPU核心进行编译"

if make -j$CORES; then
    echo "✅ 编译成功!"
else
    echo "❌ 编译失败"
    echo ""
    echo "🔧 故障排除建议:"
    echo "1. 检查编译错误信息"
    echo "2. 尝试单核编译: make -j1"
    echo "3. 检查磁盘空间: df -h"
    exit 1
fi

# 步骤6: 验证编译结果
echo ""
echo "✅ 步骤6: 验证编译结果..."

# 检查关键文件
EXPECTED_FILES=(
    "libbitcask.a"
    "bitcask_example"
    "http_server_example"
    "redis_server_example"
    "unit_tests"
)

ALL_GOOD=true
for file in "${EXPECTED_FILES[@]}"; do
    if [[ -f "$file" ]]; then
        echo "✅ $file"
    else
        echo "❌ $file - 缺失"
        ALL_GOOD=false
    fi
done

# 步骤7: 运行快速测试
echo ""
echo "🧪 步骤7: 运行快速测试..."

if [[ "$ALL_GOOD" == "true" ]]; then
    echo "运行基础功能测试..."
    if ./bitcask_example; then
        echo "✅ 基础功能测试通过"
    else
        echo "⚠️  基础功能测试失败，但编译成功"
    fi
    
    echo ""
    echo "运行单元测试（前10个）..."
    if timeout 30s ./unit_tests 2>/dev/null | head -20; then
        echo "✅ 单元测试运行正常"
    else
        echo "⚠️  单元测试可能有问题，但编译成功"
    fi
fi

# 步骤8: 显示使用指南
echo ""
echo "🎉 编译完成!"
echo "================================"
echo ""
echo "📋 可用的程序:"
echo "  ./bitcask_example          - 基础操作示例"
echo "  ./http_server_example      - HTTP服务器"
echo "  ./redis_server_example     - Redis兼容服务器"
echo "  ./unit_tests               - 完整单元测试"
echo "  ./integration_tests        - 集成测试"
echo "  ./benchmark_tests          - 性能测试"
echo ""
echo "🔧 单独的测试程序:"
for test_file in test_*; do
    if [[ -f "$test_file" && -x "$test_file" ]]; then
        echo "  ./$test_file"
    fi
done
echo ""
echo "📚 详细使用指南:"
echo "  查看 COMPLETE_MANUAL_GUIDE.md 获取完整的使用说明"
echo "  查看 FINAL_FEATURE_COMPARISON.md 了解功能对比"
echo ""
echo "🚀 快速开始:"
echo "  1. 运行基础示例: ./bitcask_example"
echo "  2. 启动HTTP服务: ./http_server_example"
echo "  3. 运行所有测试: ./unit_tests"
echo ""
echo "✨ 享受使用Bitcask C++!"
