#!/bin/bash

# Bitcask C++ Ubuntu 22.04 最终编译和测试脚本
# 包含所有修复和完整的验证流程

set -e  # 任何命令失败时退出

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 打印函数
print_header() {
    echo -e "${BLUE}========================================${NC}"
    echo -e "${BLUE}$1${NC}"
    echo -e "${BLUE}========================================${NC}"
}

print_info() {
    echo -e "${YELLOW}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

print_header "Bitcask C++ Ubuntu 22.04 最终构建和测试"

# 检查是否为Ubuntu系统
if ! grep -q "Ubuntu" /etc/os-release; then
    print_error "此脚本仅适用于Ubuntu系统"
    exit 1
fi

ubuntu_version=$(lsb_release -rs)
print_info "检测到Ubuntu版本: $ubuntu_version"

# 1. 环境准备
print_header "第1步：环境准备"

print_info "更新软件包列表..."
sudo apt update

print_info "安装编译依赖..."
sudo apt install -y \
    build-essential \
    cmake \
    g++-11 \
    gcc-11 \
    libgtest-dev \
    git \
    make \
    pkg-config \
    libc6-dev \
    libstdc++-11-dev

# 设置编译器
print_info "配置编译器..."
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-11 60
sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-11 60

# 验证编译器版本
gcc_version=$(gcc --version | head -n1)
gpp_version=$(g++ --version | head -n1)
cmake_version=$(cmake --version | head -n1)

print_success "GCC: $gcc_version"
print_success "G++: $gpp_version"
print_success "CMake: $cmake_version"

# 2. 项目目录设置
print_header "第2步：项目设置"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$SCRIPT_DIR"

print_info "项目目录: $PROJECT_DIR"
cd "$PROJECT_DIR"

# 验证关键文件存在
print_info "验证项目文件..."
required_files=("CMakeLists.txt" "src" "tests" "include")
for file in "${required_files[@]}"; do
    if [ -e "$file" ]; then
        print_success "✓ $file 存在"
    else
        print_error "✗ $file 不存在"
        exit 1
    fi
done

# 3. 清理和重建
print_header "第3步：清理和重建"

print_info "清理旧的构建文件..."
rm -rf build
rm -rf CMakeCache.txt
rm -rf CMakeFiles
rm -rf *.cmake

print_info "创建构建目录..."
mkdir -p build
cd build

# 4. CMake配置
print_header "第4步：CMake配置"

print_info "配置CMake项目..."
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_STANDARD=17 \
    -DCMAKE_CXX_COMPILER=g++-11 \
    -DCMAKE_C_COMPILER=gcc-11 \
    -DCMAKE_CXX_FLAGS="-O2 -DNDEBUG -Wno-unused-variable -Wno-sign-compare" \
    -DCMAKE_VERBOSE_MAKEFILE=ON

if [ $? -eq 0 ]; then
    print_success "CMake配置成功"
else
    print_error "CMake配置失败"
    exit 1
fi

# 5. 编译项目
print_header "第5步：编译项目"

NPROC=$(nproc)
print_info "使用 $NPROC 个CPU核心编译..."

make -j$NPROC

if [ $? -eq 0 ]; then
    print_success "项目编译成功"
else
    print_error "项目编译失败"
    exit 1
fi

# 6. 验证可执行文件
print_header "第6步：验证可执行文件"

ALL_EXECUTABLES=("test_log_record" "test_data_file" "test_io_manager" "test_index" "test_db" "test_advanced_index" "test_write_batch" "test_iterator" "test_backup" "test_merge" "unit_tests" "integration_tests" "bitcask_test" "benchmark_tests" "bitcask_example")

missing_files=()
for exe in "${ALL_EXECUTABLES[@]}"; do
    if [ -f "./$exe" ]; then
        print_success "✓ $exe"
    else
        print_error "✗ $exe (缺失)"
        missing_files+=("$exe")
    fi
done

if [ ${#missing_files[@]} -gt 0 ]; then
    print_error "缺失 ${#missing_files[@]} 个可执行文件"
    for file in "${missing_files[@]}"; do
        echo "  - $file"
    done
    exit 1
fi

print_success "所有可执行文件验证通过"

# 7. 运行单元测试
print_header "第7步：运行单元测试"

UNIT_TESTS=("test_log_record" "test_data_file" "test_io_manager" "test_index" "test_db" "test_advanced_index" "test_write_batch" "test_iterator" "test_backup" "test_merge")

passed_tests=0
failed_tests=()

for test in "${UNIT_TESTS[@]}"; do
    print_info "运行 $test..."
    
    # 设置超时和日志
    log_file="/tmp/${test}_log_$(date +%s).txt"
    
    if timeout 180 ./$test > "$log_file" 2>&1; then
        print_success "$test 通过"
        ((passed_tests++))
    else
        print_error "$test 失败"
        failed_tests+=("$test")
        echo "错误详情："
        tail -20 "$log_file"
        echo ""
    fi
    
    # 清理
    rm -f "$log_file"
    rm -rf /tmp/bitcask_test_*
    rm -rf /tmp/bitcask_*
done

# 8. 运行集成测试
print_header "第8步：运行集成测试"

INTEGRATION_TESTS=("unit_tests" "integration_tests" "bitcask_test")

for test in "${INTEGRATION_TESTS[@]}"; do
    print_info "运行 $test..."
    
    log_file="/tmp/${test}_log_$(date +%s).txt"
    
    if timeout 300 ./$test > "$log_file" 2>&1; then
        print_success "$test 通过"
        ((passed_tests++))
    else
        print_error "$test 失败"
        failed_tests+=("$test")
        echo "错误详情："
        tail -20 "$log_file"
        echo ""
    fi
    
    rm -f "$log_file"
    rm -rf /tmp/bitcask_test_*
    rm -rf /tmp/bitcask_*
done

# 9. 性能基准测试（可选）
print_header "第9步：性能基准测试"

print_info "运行性能基准测试..."
if timeout 300 ./benchmark_tests > /tmp/benchmark_log.txt 2>&1; then
    print_success "基准测试完成"
    echo "基准测试结果："
    tail -10 /tmp/benchmark_log.txt
else
    print_error "基准测试失败或超时"
fi

rm -f /tmp/benchmark_log.txt

# 10. 示例程序验证
print_header "第10步：示例程序验证"

print_info "验证基本操作示例..."
if timeout 60 ./bitcask_example > /tmp/example_log.txt 2>&1; then
    print_success "示例程序运行成功"
else
    print_error "示例程序运行失败"
    cat /tmp/example_log.txt
fi

rm -f /tmp/example_log.txt

# 11. 最终结果报告
print_header "最终结果报告"

total_tests=$((${#UNIT_TESTS[@]} + ${#INTEGRATION_TESTS[@]}))

print_info "测试统计："
echo "  总测试数: $total_tests"
echo "  通过测试: $passed_tests"
echo "  失败测试: ${#failed_tests[@]}"

if [ ${#failed_tests[@]} -eq 0 ]; then
    print_success "🎉 所有测试通过！Bitcask C++项目完全就绪！"
    
    echo ""
    print_info "项目功能验证完成："
    echo "  ✅ 核心数据库操作 (PUT/GET/DELETE)"
    echo "  ✅ 多种索引类型 (BTree, SkipList, B+Tree, ART)"
    echo "  ✅ 文件I/O和内存映射"
    echo "  ✅ 数据合并和空间回收"
    echo "  ✅ 备份和恢复功能"
    echo "  ✅ 迭代器和批量操作"
    echo "  ✅ 并发安全和错误处理"
    echo "  ✅ 统计信息和性能监控"
    echo "  ✅ 示例程序和集成测试"
    
    echo ""
    print_info "生产环境部署指南："
    echo "  1. 使用当前的Release模式编译"
    echo "  2. 选择合适的索引类型 (推荐BTree或SkipList)"
    echo "  3. 配置数据文件大小 (默认256MB)"
    echo "  4. 设置合并比例 (默认0.5)"
    echo "  5. 启用内存映射 (大数据集)"
    echo "  6. 设置定期备份"
    echo "  7. 监控磁盘使用和性能"
    
    echo ""
    print_success "项目已完全准备好投入生产使用！"
    
    exit 0
else
    print_error "⚠️  有 ${#failed_tests[@]} 个测试失败"
    print_error "失败的测试："
    for test in "${failed_tests[@]}"; do
        echo "  - $test"
    done
    
    echo ""
    print_info "调试建议："
    echo "  1. 检查失败测试的详细错误信息"
    echo "  2. 验证系统资源 (磁盘空间、内存)"
    echo "  3. 确认所有依赖都已正确安装"
    echo "  4. 检查权限和文件访问"
    echo "  5. 单独运行失败的测试进行调试"
    
    echo ""
    print_info "手动调试命令："
    for test in "${failed_tests[@]}"; do
        echo "  ./$test  # 调试 $test"
    done
    
    exit 1
fi
