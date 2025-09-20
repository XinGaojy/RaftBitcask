#!/bin/bash

# Bitcask C++ Ubuntu 22.04 快速构建脚本
# 使用方法: chmod +x quick_build_ubuntu.sh && ./quick_build_ubuntu.sh

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

log_info "=== Bitcask C++ Ubuntu 22.04 快速构建脚本 ==="
log_info "开始时间: $(date)"

# 检查操作系统
if [[ -f /etc/os-release ]]; then
    source /etc/os-release
    if [[ "$ID" == "ubuntu" ]]; then
        log_info "检测到 Ubuntu $VERSION"
    else
        log_warning "检测到非Ubuntu系统: $ID $VERSION"
        log_info "脚本仍将继续运行..."
    fi
else
    log_warning "无法检测操作系统版本"
fi

# 1. 安装依赖
log_info "步骤 1/6: 安装依赖包..."
sudo apt update -qq
sudo apt install -y build-essential cmake g++ pkg-config git libgtest-dev libgmock-dev

# 验证工具版本
gcc_version=$(g++ --version | head -n1 | grep -oE '[0-9]+\.[0-9]+' | head -1)
cmake_version=$(cmake --version | head -n1 | grep -oE '[0-9]+\.[0-9]+' | head -1)

log_info "GCC 版本: $gcc_version"
log_info "CMake 版本: $cmake_version"

# 检查版本要求
gcc_major=$(echo $gcc_version | cut -d. -f1)
if [[ $gcc_major -lt 7 ]]; then
    log_error "需要 GCC 7 或更高版本，当前版本: $gcc_version"
    exit 1
fi

# 2. 清理旧的构建
log_info "步骤 2/6: 清理构建目录..."
if [[ -d "build" ]]; then
    rm -rf build
    log_info "已删除旧的构建目录"
fi
mkdir build

# 3. 配置 CMake
log_info "步骤 3/6: 配置 CMake..."
cd build

cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_STANDARD=17 \
    -DCMAKE_CXX_FLAGS="-O2 -Wall -Wextra" \
    -DBUILD_TESTING=ON

if [[ $? -eq 0 ]]; then
    log_success "CMake 配置成功"
else
    log_error "CMake 配置失败"
    exit 1
fi

# 4. 编译项目
log_info "步骤 4/6: 编译项目..."
cpu_cores=$(nproc)
log_info "使用 $cpu_cores 个CPU核心进行编译"

# 检查可用内存
available_mem=$(free -m | awk 'NR==2{printf "%.0f", $7}')
log_info "可用内存: ${available_mem}MB"

# 根据内存调整编译线程数
if [[ $available_mem -lt 2000 ]]; then
    compile_jobs=2
    log_warning "内存不足，使用 2 个编译线程"
else
    compile_jobs=$cpu_cores
fi

make -j$compile_jobs

if [[ $? -eq 0 ]]; then
    log_success "项目编译成功"
else
    log_error "项目编译失败"
    exit 1
fi

# 5. 验证构建结果
log_info "步骤 5/6: 验证构建结果..."

# 检查必需的可执行文件
required_files=(
    "test_log_record" "test_data_file" "test_io_manager" "test_mmap_io"
    "test_index" "test_db" "test_advanced_index" "test_art_index"
    "test_write_batch" "test_iterator" "test_merge" "test_backup"
    "test_http_server" "test_redis" "test_test_utils"
    "bitcask_example" "http_server_example" "redis_server_example"
)

missing_files=0
for file in "${required_files[@]}"; do
    if [[ ! -f "./$file" ]]; then
        log_error "缺少文件: $file"
        ((missing_files++))
    fi
done

if [[ $missing_files -gt 0 ]]; then
    log_error "构建不完整，缺少 $missing_files 个文件"
    exit 1
else
    log_success "所有必需文件都已生成"
fi

# 6. 运行测试
log_info "步骤 6/6: 运行核心测试..."

# 定义测试列表
tests=(
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

passed_tests=0
failed_tests=0
failed_test_names=()

for test in "${tests[@]}"; do
    log_info "运行测试: $test"
    
    # 创建临时日志文件
    log_file="/tmp/${test}_output.log"
    
    if timeout 120 ./$test > "$log_file" 2>&1; then
        # 检查测试结果
        if grep -q "\[  PASSED  \]" "$log_file"; then
            passed_count=$(grep -o '\[  PASSED  \] [0-9]* test' "$log_file" | tail -1 | grep -o '[0-9]*' || echo "0")
            log_success "$test: $passed_count 个测试通过"
            ((passed_tests++))
        else
            log_error "$test: 测试格式异常"
            ((failed_tests++))
            failed_test_names+=("$test")
        fi
    else
        log_error "$test: 测试执行失败或超时"
        echo "错误日志:"
        tail -10 "$log_file" | sed 's/^/    /'
        ((failed_tests++))
        failed_test_names+=("$test")
    fi
    
    # 清理临时日志
    rm -f "$log_file"
done

# 输出测试总结
echo ""
log_info "=================== 测试总结 ==================="
log_success "通过的测试: $passed_tests"

if [[ $failed_tests -gt 0 ]]; then
    log_error "失败的测试: $failed_tests"
    log_error "失败的测试列表:"
    for failed_test in "${failed_test_names[@]}"; do
        echo "  - $failed_test"
    done
    echo ""
    log_error "构建完成但部分测试失败"
    log_info "您可以手动运行失败的测试进行调试"
    exit 1
else
    log_success "所有 $passed_tests 个测试都通过了！"
fi

# 运行示例程序验证
log_info "验证示例程序..."
if timeout 10 ./bitcask_example > /tmp/bitcask_example.log 2>&1; then
    log_success "基础示例程序运行正常"
else
    log_warning "基础示例程序运行异常，请手动检查"
fi

# 最终成功信息
echo ""
log_success "🎉 Bitcask C++ 构建完成！"
log_success "🚀 项目已准备好部署到生产环境！"
echo ""
log_info "下一步操作:"
echo "  cd build"
echo ""
log_info "运行单个测试:"
echo "  ./test_log_record    # 测试日志记录模块"
echo "  ./test_db           # 测试数据库核心模块"
echo "  ./test_art_index    # 测试ART索引模块"
echo ""
log_info "运行示例程序:"
echo "  ./bitcask_example          # 基础操作示例"
echo "  ./http_server_example &    # HTTP服务器 (后台运行)"
echo "  ./redis_server_example &   # Redis服务器 (后台运行)"
echo ""
log_info "测试API:"
echo "  # HTTP API"
echo "  curl -X POST -d '{\"key\":\"value\"}' http://localhost:8080/bitcask/put"
echo "  curl http://localhost:8080/bitcask/get?key=key"
echo ""
echo "  # Redis API (需要安装 redis-tools)"
echo "  sudo apt install redis-tools"
echo "  redis-cli -p 6380 SET mykey myvalue"
echo "  redis-cli -p 6380 GET mykey"
echo ""
log_info "生产部署:"
echo "  参考 UBUNTU_BUILD_FINAL.md 中的生产部署章节"
echo ""
log_info "构建完成时间: $(date)"
log_success "享受使用 Bitcask C++ 存储引擎！"
