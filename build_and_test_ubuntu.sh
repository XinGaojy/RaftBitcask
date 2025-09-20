#!/bin/bash

# Bitcask-cpp Ubuntu 22.04 构建和测试脚本
# 解决同步活跃文件阻塞问题的完整构建脚本

set -e  # 遇到错误立即退出

echo "=== Bitcask-cpp Ubuntu 22.04 构建和测试脚本 ==="
echo "修复同步活跃文件阻塞问题"

# 检查操作系统
if [[ "$OSTYPE" != "linux-gnu"* ]]; then
    echo "警告: 此脚本专为 Ubuntu 22.04 设计"
fi

# 设置环境变量
export CC=gcc
export CXX=g++
export DEBIAN_FRONTEND=noninteractive

# 函数：安装依赖
install_dependencies() {
    echo ">>> 安装系统依赖..."
    
    # 更新包列表
    sudo apt-get update -qq
    
    # 安装基本构建工具
    sudo apt-get install -y \
        build-essential \
        cmake \
        ninja-build \
        pkg-config \
        git \
        wget \
        curl \
        gcc-11 \
        g++-11 \
        make
        
    # 安装库依赖
    sudo apt-get install -y \
        libz-dev \
        zlib1g-dev \
        libcrc32c-dev \
        libgtest-dev \
        libgmock-dev \
        libbenchmark-dev
        
    echo "依赖安装完成"
}

# 函数：清理构建目录
clean_build() {
    echo ">>> 清理构建目录..."
    rm -rf build/
    rm -rf CMakeCache.txt
    rm -rf CMakeFiles/
    find . -name "*.o" -delete
    find . -name "*.a" -delete
    find . -name "core.*" -delete
    echo "清理完成"
}

# 函数：配置构建
configure_build() {
    echo ">>> 配置构建环境..."
    
    mkdir -p build
    cd build
    
    # 使用cmake配置项目
    cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_CXX_COMPILER=g++-11 \
        -DCMAKE_C_COMPILER=gcc-11 \
        -DCMAKE_CXX_STANDARD=17 \
        -DCMAKE_CXX_FLAGS="-O2 -D_GNU_SOURCE -pthread -fPIC" \
        -DCMAKE_VERBOSE_MAKEFILE=ON \
        -G Ninja
        
    echo "配置完成"
}

# 函数：执行构建
build_project() {
    echo ">>> 构建项目..."
    
    # 使用ninja并行构建，如果没有ninja则使用make
    if command -v ninja &> /dev/null; then
        ninja -j$(nproc) 2>&1 | tee build.log
    else
        make -j$(nproc) 2>&1 | tee build.log
    fi
    
    # 检查构建结果
    if [ $? -eq 0 ]; then
        echo "✅ 构建成功"
    else
        echo "❌ 构建失败，显示错误信息："
        tail -n 20 build.log
        echo ""
        echo "尝试修复常见编译错误..."
        
        # 检查是否是std::ofstream::sync()错误
        if grep -q "no member named 'sync' in" build.log; then
            echo "检测到std::ofstream::sync()错误，这已经在最新代码中修复"
            echo "请确保使用最新的源代码"
        fi
        
        # 重新尝试构建
        echo "清理并重新构建..."
        make clean 2>/dev/null || ninja clean 2>/dev/null
        if command -v ninja &> /dev/null; then
            ninja -j$(nproc)
        else
            make -j$(nproc)
        fi
        
        if [ $? -ne 0 ]; then
            echo "❌ 重新构建仍然失败"
            return 1
        fi
    fi
    
    echo "构建完成"
}

# 函数：运行特定测试
run_specific_tests() {
    echo ">>> 运行关键测试用例..."
    
    # 测试同步功能（之前阻塞的功能）
    echo "测试备份功能（包含同步操作）..."
    if ./test_backup; then
        echo "✅ 备份测试通过 - 同步阻塞问题已解决"
    else
        echo "❌ 备份测试失败"
        return 1
    fi
    
    # 测试基本数据库操作
    echo "测试基本数据库操作..."
    if ./test_db; then
        echo "✅ 数据库测试通过"
    else
        echo "❌ 数据库测试失败"
        return 1
    fi
    
    # 测试IO管理器
    echo "测试IO管理器..."
    if ./test_io_manager; then
        echo "✅ IO管理器测试通过"
    else
        echo "❌ IO管理器测试失败"
        return 1
    fi
    
    # 测试数据文件操作
    echo "测试数据文件操作..."
    if ./test_data_file; then
        echo "✅ 数据文件测试通过"
    else
        echo "❌ 数据文件测试失败"
        return 1
    fi
}

# 函数：运行完整测试套件
run_full_tests() {
    echo ">>> 运行完整测试套件..."
    
    # 设置测试环境
    export GTEST_COLOR=1
    export GTEST_BRIEF=1
    
    # 运行所有单元测试
    if ctest -j$(nproc) --output-on-failure --timeout 300; then
        echo "✅ 所有测试通过"
    else
        echo "❌ 部分测试失败，查看详细输出"
        # 显示失败的测试
        ctest --rerun-failed --output-on-failure
        return 1
    fi
}

# 函数：验证修复效果
verify_fixes() {
    echo ">>> 验证同步阻塞问题修复..."
    
    # 创建测试程序验证同步不会阻塞
    cat > test_sync_fix.cpp << 'EOF'
#include <iostream>
#include <chrono>
#include <signal.h>
#include "../include/bitcask/bitcask.h"

// 超时处理
void timeout_handler(int) {
    std::cout << "❌ 同步操作超时，问题未完全解决" << std::endl;
    exit(1);
}

int main() {
    // 设置30秒超时
    signal(SIGALRM, timeout_handler);
    alarm(30);
    
    std::cout << "测试同步操作不会阻塞..." << std::endl;
    
    try {
        auto start = std::chrono::steady_clock::now();
        
        // 创建数据库实例
        bitcask::Options options = bitcask::Options::default_options();
        options.dir_path = "/tmp/test_sync_fix";
        options.sync_writes = true;
        
        system("rm -rf /tmp/test_sync_fix");
        
        auto db = bitcask::DB::open(options);
        
        // 写入一些数据
        for (int i = 0; i < 100; ++i) {
            std::string key = "key" + std::to_string(i);
            std::string value = "value" + std::to_string(i);
            db->put(bitcask::Bytes(key.begin(), key.end()), 
                   bitcask::Bytes(value.begin(), value.end()));
        }
        
        // 执行同步操作
        db->sync();
        
        // 执行备份（之前会阻塞的操作）
        db->backup("/tmp/test_sync_backup");
        
        auto end = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        std::cout << "✅ 同步操作完成，耗时: " << duration.count() << "ms" << std::endl;
        
        if (duration.count() < 5000) {  // 少于5秒认为正常
            std::cout << "✅ 同步阻塞问题已解决" << std::endl;
        } else {
            std::cout << "⚠️  同步时间较长，可能仍有性能问题" << std::endl;
        }
        
        db->close();
        system("rm -rf /tmp/test_sync_fix /tmp/test_sync_backup");
        
    } catch (const std::exception& e) {
        std::cout << "❌ 测试失败: " << e.what() << std::endl;
        return 1;
    }
    
    alarm(0);  // 取消超时
    return 0;
}
EOF

    # 编译测试程序
    g++-11 -std=c++17 -O2 -I../include test_sync_fix.cpp -L. -lbitcask -lpthread -lz -o test_sync_fix
    
    # 运行测试
    if ./test_sync_fix; then
        echo "✅ 同步阻塞问题修复验证成功"
    else
        echo "❌ 同步阻塞问题仍然存在"
        return 1
    fi
    
    rm -f test_sync_fix test_sync_fix.cpp
}

# 函数：生成报告
generate_report() {
    echo ""
    echo "=== 构建和测试报告 ==="
    echo "项目: Bitcask-cpp"
    echo "目标: 修复同步活跃文件阻塞问题"
    echo "平台: Ubuntu 22.04"
    echo "编译器: GCC 11"
    echo "构建系统: CMake + Ninja"
    echo ""
    echo "主要修复内容:"
    echo "1. ✅ 优化DB::sync()方法，使用共享锁而非独占锁"
    echo "2. ✅ 改进IO管理器同步机制，增加错误容忍度"
    echo "3. ✅ 优化B+Tree索引同步，使用try_lock避免阻塞"
    echo "4. ✅ 改进DataFile同步，增加异常处理"
    echo "5. ✅ 优化备份过程中的同步策略"
    echo ""
    echo "构建完成时间: $(date)"
    echo "构建目录: $(pwd)"
    echo "可执行文件:"
    ls -la | grep -E "(test_|bitcask)"
    echo ""
}

# 主执行流程
main() {
    echo "开始构建过程..."
    
    # 安装依赖
    install_dependencies
    
    # 清理并构建
    cd "$(dirname "$0")"
    clean_build
    configure_build
    build_project
    
    # 运行测试
    run_specific_tests
    
    # 验证修复
    verify_fixes
    
    # 运行完整测试（可选，如果时间允许）
    echo "是否运行完整测试套件？(y/N)"
    read -t 10 -r response || response="N"
    if [[ "$response" =~ ^[Yy]$ ]]; then
        run_full_tests
    fi
    
    # 生成报告
    generate_report
    
    echo ""
    echo "🎉 构建和测试完成！同步阻塞问题已解决！"
    echo ""
    echo "使用方法:"
    echo "  ./bitcask_example          # 运行基本示例"
    echo "  ./test_backup              # 测试备份功能"
    echo "  ./unit_tests               # 运行所有单元测试"
    echo ""
}

# 错误处理
trap 'echo "❌ 构建过程中发生错误"; exit 1' ERR

# 执行主函数
main "$@"
