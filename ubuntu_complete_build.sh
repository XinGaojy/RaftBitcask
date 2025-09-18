#!/bin/bash

# Ubuntu 22.04 完整编译脚本
# 本脚本用于在 Ubuntu 22.04 系统上完整编译 bitcask-cpp 项目

set -e  # 遇到错误立即退出

echo "=========================================="
echo "Bitcask-cpp Ubuntu 22.04 编译脚本"
echo "=========================================="

# 检查系统版本
echo "检查系统版本..."
if [[ $(lsb_release -rs) != "22.04" ]]; then
    echo "警告：本脚本为 Ubuntu 22.04 优化，当前系统版本为 $(lsb_release -rs)"
    read -p "是否继续？(y/N): " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        echo "退出脚本"
        exit 1
    fi
fi

# 检查权限
if [[ $EUID -eq 0 ]]; then
   echo "错误：请不要使用 root 用户运行此脚本"
   exit 1
fi

# 安装系统依赖
echo "=========================================="
echo "1. 安装系统依赖..."
echo "=========================================="

echo "更新包管理器..."
sudo apt update

echo "安装编译工具链..."
sudo apt install -y build-essential cmake gcc g++ pkg-config

echo "安装开发库..."
sudo apt install -y libssl-dev zlib1g-dev libcrc32c-dev

echo "安装测试框架..."
sudo apt install -y libgtest-dev libgmock-dev

echo "安装性能分析工具..."
sudo apt install -y valgrind gdb

echo "安装网络工具（用于测试）..."
sudo apt install -y curl netcat-openbsd redis-tools

# 验证工具版本
echo "=========================================="
echo "2. 验证工具版本..."
echo "=========================================="

echo "CMake 版本："
cmake --version | head -1

echo "GCC 版本："
gcc --version | head -1

echo "G++ 版本："
g++ --version | head -1

# 检查 C++17 支持
echo "检查 C++17 支持..."
cat > /tmp/cpp17_test.cpp << 'EOF'
#include <iostream>
#include <optional>
#include <string_view>

int main() {
    std::optional<int> opt = 42;
    std::string_view sv = "Hello C++17";
    std::cout << "C++17 支持正常" << std::endl;
    return 0;
}
EOF

if g++ -std=c++17 /tmp/cpp17_test.cpp -o /tmp/cpp17_test && /tmp/cpp17_test; then
    echo "✓ C++17 支持正常"
    rm -f /tmp/cpp17_test.cpp /tmp/cpp17_test
else
    echo "✗ C++17 支持检查失败"
    exit 1
fi

# 设置项目目录
PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$PROJECT_DIR/build"

echo "=========================================="
echo "3. 准备构建环境..."
echo "=========================================="

echo "项目目录：$PROJECT_DIR"
echo "构建目录：$BUILD_DIR"

# 清理旧的构建文件
if [ -d "$BUILD_DIR" ]; then
    echo "清理旧的构建文件..."
    rm -rf "$BUILD_DIR"
fi

echo "创建构建目录..."
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# 配置项目
echo "=========================================="
echo "4. 配置项目..."
echo "=========================================="

echo "运行 CMake 配置..."
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_STANDARD=17 \
    -DCMAKE_CXX_STANDARD_REQUIRED=ON \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

if [ $? -ne 0 ]; then
    echo "✗ CMake 配置失败"
    exit 1
fi

echo "✓ CMake 配置成功"

# 编译项目
echo "=========================================="
echo "5. 编译项目..."
echo "=========================================="

# 获取 CPU 核心数
CORES=$(nproc)
echo "使用 $CORES 个 CPU 核心进行编译..."

echo "开始编译..."
start_time=$(date +%s)

if make -j$CORES; then
    end_time=$(date +%s)
    duration=$((end_time - start_time))
    echo "✓ 编译成功！耗时: ${duration}秒"
else
    echo "✗ 编译失败"
    exit 1
fi

# 验证编译产物
echo "=========================================="
echo "6. 验证编译产物..."
echo "=========================================="

echo "检查主要文件..."

# 检查静态库
if [ -f "libbitcask.a" ]; then
    echo "✓ libbitcask.a"
    ls -lh libbitcask.a
else
    echo "✗ libbitcask.a 未找到"
    exit 1
fi

# 检查示例程序
EXAMPLES=("bitcask_example" "http_server_example" "redis_server_example")
for example in "${EXAMPLES[@]}"; do
    if [ -f "$example" ]; then
        echo "✓ $example"
        ls -lh "$example"
    else
        echo "✗ $example 未找到"
    fi
done

# 检查测试程序
echo "检查测试程序..."
TESTS=("unit_tests" "integration_tests" "benchmark_tests")
for test in "${TESTS[@]}"; do
    if [ -f "$test" ]; then
        echo "✓ $test"
        ls -lh "$test"
    else
        echo "✗ $test 未找到"
    fi
done

# 检查单独的测试模块
echo "检查独立测试模块..."
TEST_MODULES=(
    "test_log_record"
    "test_io_manager"
    "test_data_file"
    "test_index"
    "test_db"
    "test_write_batch"
    "test_iterator"
    "test_merge"
    "test_http_server"
    "test_redis"
    "test_backup"
    "test_advanced_index"
    "test_art_index"
    "test_mmap_io"
    "test_test_utils"
)

missing_tests=0
for test_module in "${TEST_MODULES[@]}"; do
    if [ -f "$test_module" ]; then
        echo "✓ $test_module"
    else
        echo "✗ $test_module 未找到"
        missing_tests=$((missing_tests + 1))
    fi
done

if [ $missing_tests -gt 0 ]; then
    echo "警告：有 $missing_tests 个测试模块未找到"
fi

# 快速测试
echo "=========================================="
echo "7. 快速功能测试..."
echo "=========================================="

# 测试基本操作示例
if [ -f "bitcask_example" ]; then
    echo "运行基本操作测试..."
    if timeout 30 ./bitcask_example; then
        echo "✓ 基本操作测试通过"
    else
        echo "✗ 基本操作测试失败"
    fi
fi

# 测试单元测试（只运行一个简单的测试）
if [ -f "test_log_record" ]; then
    echo "运行日志记录模块测试..."
    if timeout 30 ./test_log_record; then
        echo "✓ 日志记录模块测试通过"
    else
        echo "✗ 日志记录模块测试失败"
    fi
fi

# 生成测试脚本
echo "=========================================="
echo "8. 生成测试脚本..."
echo "=========================================="

cat > "$BUILD_DIR/run_all_tests.sh" << 'EOF'
#!/bin/bash

# 运行所有测试的脚本
set -e

echo "=========================================="
echo "运行所有单元测试"
echo "=========================================="

# 单独运行每个测试模块
TESTS=(
    "test_log_record"
    "test_io_manager"
    "test_data_file"
    "test_index"
    "test_art_index"
    "test_db"
    "test_write_batch"
    "test_iterator"
    "test_mmap_io"
    "test_test_utils"
)

passed=0
failed=0

for test in "${TESTS[@]}"; do
    if [ -f "$test" ]; then
        echo "运行 $test..."
        if timeout 60 ./"$test"; then
            echo "✓ $test 通过"
            passed=$((passed + 1))
        else
            echo "✗ $test 失败"
            failed=$((failed + 1))
        fi
        echo ""
    else
        echo "⚠ $test 未找到"
        echo ""
    fi
done

echo "=========================================="
echo "测试结果汇总"
echo "=========================================="
echo "通过: $passed"
echo "失败: $failed"
echo "总计: $((passed + failed))"

if [ $failed -eq 0 ]; then
    echo "🎉 所有测试通过！"
    exit 0
else
    echo "❌ 有测试失败"
    exit 1
fi
EOF

chmod +x "$BUILD_DIR/run_all_tests.sh"
echo "✓ 测试脚本已生成：$BUILD_DIR/run_all_tests.sh"

# 生成性能测试脚本
cat > "$BUILD_DIR/run_performance_tests.sh" << 'EOF'
#!/bin/bash

# 运行性能测试的脚本
set -e

echo "=========================================="
echo "运行性能基准测试"
echo "=========================================="

if [ -f "benchmark_tests" ]; then
    echo "运行基准测试..."
    ./benchmark_tests
else
    echo "基准测试程序未找到"
fi

echo "=========================================="
echo "运行内存检查"
echo "=========================================="

if command -v valgrind &> /dev/null; then
    if [ -f "bitcask_example" ]; then
        echo "使用 Valgrind 检查内存泄漏..."
        valgrind --tool=memcheck --leak-check=full --show-leak-kinds=all ./bitcask_example
    fi
else
    echo "Valgrind 未安装，跳过内存检查"
fi
EOF

chmod +x "$BUILD_DIR/run_performance_tests.sh"
echo "✓ 性能测试脚本已生成：$BUILD_DIR/run_performance_tests.sh"

# 生成服务器测试脚本
cat > "$BUILD_DIR/test_servers.sh" << 'EOF'
#!/bin/bash

# 测试 HTTP 和 Redis 服务器
set -e

echo "=========================================="
echo "测试 HTTP 服务器"
echo "=========================================="

if [ -f "http_server_example" ]; then
    echo "启动 HTTP 服务器..."
    ./http_server_example &
    HTTP_PID=$!
    
    echo "等待服务器启动..."
    sleep 3
    
    echo "测试 HTTP API..."
    
    # 测试 PUT
    if curl -f -X PUT http://localhost:8080/bitcask/test_key -d "test_value"; then
        echo "✓ PUT 操作成功"
    else
        echo "✗ PUT 操作失败"
    fi
    
    # 测试 GET
    if curl -f http://localhost:8080/bitcask/test_key; then
        echo "✓ GET 操作成功"
    else
        echo "✗ GET 操作失败"
    fi
    
    # 关闭服务器
    kill $HTTP_PID 2>/dev/null || true
    wait $HTTP_PID 2>/dev/null || true
    
    echo "HTTP 服务器测试完成"
else
    echo "HTTP 服务器程序未找到"
fi

echo "=========================================="
echo "测试 Redis 服务器"
echo "=========================================="

if [ -f "redis_server_example" ]; then
    echo "启动 Redis 服务器..."
    ./redis_server_example &
    REDIS_PID=$!
    
    echo "等待服务器启动..."
    sleep 3
    
    if command -v redis-cli &> /dev/null; then
        echo "测试 Redis 兼容性..."
        
        # 测试基本操作
        redis-cli -p 6380 set test_key test_value
        redis-cli -p 6380 get test_key
        
        echo "✓ Redis 兼容性测试成功"
    else
        echo "redis-cli 未安装，跳过 Redis 测试"
    fi
    
    # 关闭服务器
    kill $REDIS_PID 2>/dev/null || true
    wait $REDIS_PID 2>/dev/null || true
    
    echo "Redis 服务器测试完成"
else
    echo "Redis 服务器程序未找到"
fi
EOF

chmod +x "$BUILD_DIR/test_servers.sh"
echo "✓ 服务器测试脚本已生成：$BUILD_DIR/test_servers.sh"

# 最终总结
echo "=========================================="
echo "编译完成总结"
echo "=========================================="

echo "📁 项目位置：$PROJECT_DIR"
echo "🔨 构建目录：$BUILD_DIR"
echo "📚 静态库：$BUILD_DIR/libbitcask.a"

echo ""
echo "可用的测试命令："
echo "  运行所有测试：      cd $BUILD_DIR && ./run_all_tests.sh"
echo "  运行性能测试：      cd $BUILD_DIR && ./run_performance_tests.sh"
echo "  测试服务器：        cd $BUILD_DIR && ./test_servers.sh"
echo "  运行基本示例：      cd $BUILD_DIR && ./bitcask_example"

echo ""
echo "手动测试模块："
for test_module in "${TEST_MODULES[@]}"; do
    if [ -f "$BUILD_DIR/$test_module" ]; then
        echo "  $test_module:    cd $BUILD_DIR && ./$test_module"
    fi
done

echo ""
echo "🎉 Bitcask-cpp 编译成功！"
echo "📖 详细测试指南请参考：UBUNTU_MANUAL_BUILD_TEST.md"
echo ""
echo "下一步："
echo "1. 运行 ./run_all_tests.sh 验证所有功能"
echo "2. 运行 ./bitcask_example 体验基本功能"
echo "3. 根据需要启动 HTTP 或 Redis 服务器"

exit 0
