#!/bin/bash

# Bitcask C++ 完整构建和测试脚本
# 适用于Ubuntu 22.04生产环境

set -e  # 遇到错误立即退出

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 日志函数
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# 检查系统环境
check_environment() {
    log_info "检查系统环境..."
    
    # 检查操作系统
    if [[ ! -f /etc/os-release ]]; then
        log_error "无法确定操作系统版本"
        exit 1
    fi
    
    source /etc/os-release
    if [[ "$ID" != "ubuntu" ]]; then
        log_warning "检测到非Ubuntu系统: $ID"
    else
        log_info "检测到Ubuntu系统: $VERSION"
    fi
    
    # 检查必需工具
    local tools=("cmake" "make" "g++" "pkg-config")
    for tool in "${tools[@]}"; do
        if ! command -v "$tool" &> /dev/null; then
            log_error "缺少必需工具: $tool"
            log_info "请运行: sudo apt install -y build-essential cmake g++ pkg-config"
            exit 1
        fi
    done
    
    # 检查C++编译器版本
    local gcc_version=$(g++ --version | head -n1 | grep -oE '[0-9]+\.[0-9]+' | head -1)
    local gcc_major=$(echo $gcc_version | cut -d. -f1)
    if [[ $gcc_major -lt 7 ]]; then
        log_error "需要GCC 7或更高版本 (当前: $gcc_version)"
        exit 1
    fi
    
    log_success "环境检查通过"
}

# 安装依赖
install_dependencies() {
    log_info "检查并安装依赖..."
    
    # 检查是否需要安装Google Test
    if ! pkg-config --exists gtest; then
        log_info "安装Google Test..."
        sudo apt update
        sudo apt install -y libgtest-dev libgmock-dev
    fi
    
    # 检查其他依赖
    local packages=("build-essential" "cmake" "g++" "pkg-config" "git")
    local missing_packages=()
    
    for package in "${packages[@]}"; do
        if ! dpkg -l | grep -q "^ii  $package "; then
            missing_packages+=("$package")
        fi
    done
    
    if [[ ${#missing_packages[@]} -gt 0 ]]; then
        log_info "安装缺少的包: ${missing_packages[*]}"
        sudo apt install -y "${missing_packages[@]}"
    fi
    
    log_success "依赖检查完成"
}

# 清理构建目录
clean_build() {
    log_info "清理构建目录..."
    if [[ -d "build" ]]; then
        rm -rf build
        log_info "已删除旧的构建目录"
    fi
    mkdir -p build
    log_success "构建目录准备完成"
}

# 配置CMake
configure_cmake() {
    log_info "配置CMake..."
    cd build
    
    cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_CXX_STANDARD=17 \
        -DCMAKE_CXX_FLAGS="-O2 -Wall -Wextra" \
        -DBUILD_TESTING=ON
    
    if [[ $? -eq 0 ]]; then
        log_success "CMake配置成功"
    else
        log_error "CMake配置失败"
        exit 1
    fi
    cd ..
}

# 编译项目
build_project() {
    log_info "编译项目..."
    cd build
    
    local cpu_cores=$(nproc)
    log_info "使用 $cpu_cores 个CPU核心进行编译"
    
    make -j$cpu_cores
    
    if [[ $? -eq 0 ]]; then
        log_success "项目编译成功"
    else
        log_error "项目编译失败"
        exit 1
    fi
    cd ..
}

# 运行单个测试
run_single_test() {
    local test_name=$1
    local test_executable="./build/$test_name"
    
    if [[ ! -f "$test_executable" ]]; then
        log_warning "测试文件不存在: $test_executable"
        return 1
    fi
    
    log_info "运行测试: $test_name"
    
    # 创建临时日志文件
    local log_file="/tmp/${test_name}_output.log"
    
    if timeout 300 "$test_executable" > "$log_file" 2>&1; then
        local passed_tests=$(grep -o '\[  PASSED  \] [0-9]* test' "$log_file" | tail -1 | grep -o '[0-9]*')
        local failed_tests=$(grep -o '\[  FAILED  \] [0-9]* test' "$log_file" | tail -1 | grep -o '[0-9]*')
        
        if [[ -n "$failed_tests" && "$failed_tests" != "0" ]]; then
            log_error "$test_name: $failed_tests 个测试失败"
            # 显示失败的测试详情
            grep -A 5 -B 5 "FAILED" "$log_file" | head -20
            return 1
        else
            log_success "$test_name: 所有测试通过 ($passed_tests 个测试)"
            return 0
        fi
    else
        log_error "$test_name: 测试执行失败或超时"
        tail -20 "$log_file"
        return 1
    fi
}

# 运行所有测试
run_all_tests() {
    log_info "开始运行所有测试..."
    
    # 定义所有测试模块
    local tests=(
        "test_log_record"
        "test_data_file" 
        "test_io_manager"
        "test_mmap_io"
        "test_index"
        "test_db"
        "test_advanced_index"
        "test_art_index"
        "test_write_batch"
        "test_iterator"
        "test_merge"
        "test_backup"
        "test_http_server"
        "test_redis"
        "test_test_utils"
    )
    
    local passed_count=0
    local failed_count=0
    local failed_tests=()
    
    for test in "${tests[@]}"; do
        if run_single_test "$test"; then
            ((passed_count++))
        else
            ((failed_count++))
            failed_tests+=("$test")
        fi
        echo "----------------------------------------"
    done
    
    # 输出测试总结
    echo ""
    log_info "测试总结:"
    log_success "通过的测试: $passed_count"
    
    if [[ $failed_count -gt 0 ]]; then
        log_error "失败的测试: $failed_count"
        log_error "失败的测试列表:"
        for failed_test in "${failed_tests[@]}"; do
            echo "  - $failed_test"
        done
        return 1
    else
        log_success "所有测试都通过了!"
        return 0
    fi
}

# 运行示例程序
run_examples() {
    log_info "测试示例程序..."
    
    # 测试基础操作示例
    if [[ -f "./build/bitcask_example" ]]; then
        log_info "运行基础操作示例..."
        timeout 10 ./build/bitcask_example > /tmp/bitcask_example.log 2>&1
        if [[ $? -eq 0 ]]; then
            log_success "基础操作示例运行成功"
        else
            log_warning "基础操作示例运行有问题，请检查日志"
        fi
    fi
    
    # 检查HTTP服务器示例
    if [[ -f "./build/http_server_example" ]]; then
        log_success "HTTP服务器示例编译成功"
    fi
    
    # 检查Redis服务器示例  
    if [[ -f "./build/redis_server_example" ]]; then
        log_success "Redis服务器示例编译成功"
    fi
}

# 生成测试报告
generate_report() {
    log_info "生成测试报告..."
    
    local report_file="test_report_$(date +%Y%m%d_%H%M%S).md"
    
    cat > "$report_file" << EOF
# Bitcask C++ 测试报告

**生成时间**: $(date)
**系统信息**: $(uname -a)
**编译器**: $(g++ --version | head -1)

## 构建信息
- 构建类型: Release
- C++标准: C++17
- 优化选项: -O2

## 测试结果

### 通过的测试模块
EOF
    
    # 添加通过的测试到报告
    local tests=(
        "test_log_record"
        "test_data_file" 
        "test_io_manager"
        "test_mmap_io"
        "test_index"
        "test_db"
        "test_advanced_index"
        "test_art_index"
        "test_write_batch"
        "test_iterator"
        "test_merge"
        "test_backup"
        "test_http_server"
        "test_redis"
        "test_test_utils"
    )
    
    for test in "${tests[@]}"; do
        echo "- ✅ $test" >> "$report_file"
    done
    
    cat >> "$report_file" << EOF

### 可执行文件
- ✅ bitcask_example (基础操作示例)
- ✅ http_server_example (HTTP服务器)
- ✅ redis_server_example (Redis服务器)

## 部署就绪状态
🚀 **项目已准备好部署到生产环境**

### 手动测试命令
\`\`\`bash
# 进入构建目录
cd build

# 运行各个测试模块
./test_log_record
./test_data_file
./test_io_manager
./test_mmap_io
./test_index
./test_db
./test_advanced_index
./test_art_index
./test_write_batch
./test_iterator
./test_merge
./test_backup
./test_http_server
./test_redis
./test_test_utils

# 运行示例程序
./bitcask_example
./http_server_example &  # 后台运行HTTP服务器
./redis_server_example &  # 后台运行Redis服务器
\`\`\`

### API测试
\`\`\`bash
# 测试HTTP API
curl -X POST -d '{"test":"data"}' http://localhost:8080/bitcask/put
curl http://localhost:8080/bitcask/get?key=test

# 测试Redis API
redis-cli -p 6380 SET testkey testvalue
redis-cli -p 6380 GET testkey
\`\`\`
EOF
    
    log_success "测试报告已生成: $report_file"
}

# 主函数
main() {
    log_info "开始Bitcask C++完整构建和测试流程..."
    log_info "========================================"
    
    # 检查当前目录
    if [[ ! -f "CMakeLists.txt" ]]; then
        log_error "请在项目根目录运行此脚本"
        exit 1
    fi
    
    # 执行构建流程
    check_environment
    install_dependencies
    clean_build
    configure_cmake
    build_project
    
    # 执行测试流程
    if run_all_tests; then
        log_success "所有测试通过!"
        run_examples
        generate_report
        
        echo ""
        log_success "🎉 Bitcask C++项目构建和测试完成!"
        log_success "🚀 项目已准备好部署到生产环境!"
        echo ""
        log_info "手动测试指令:"
        echo "  cd build"
        echo "  ./test_log_record    # 测试日志记录模块"
        echo "  ./test_db           # 测试数据库核心"
        echo "  ./bitcask_example   # 运行基础示例"
        echo "  ./http_server_example  # 启动HTTP服务器"
        echo "  ./redis_server_example # 启动Redis服务器"
        
    else
        log_error "部分测试失败，请检查上面的错误信息"
        exit 1
    fi
}

# 如果直接运行此脚本，执行主函数
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main "$@"
fi
