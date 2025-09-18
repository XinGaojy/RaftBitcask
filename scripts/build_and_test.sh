#!/bin/bash

# Bitcask C++ 构建和测试脚本

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 打印带颜色的消息
print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# 检查依赖
check_dependencies() {
    print_info "检查构建依赖..."
    
    # 检查 CMake
    if ! command -v cmake &> /dev/null; then
        print_error "CMake 未安装"
        exit 1
    fi
    
    # 检查编译器
    if ! command -v g++ &> /dev/null && ! command -v clang++ &> /dev/null; then
        print_error "未找到 C++ 编译器 (g++ 或 clang++)"
        exit 1
    fi
    
    # 检查 pkg-config
    if ! command -v pkg-config &> /dev/null; then
        print_warning "pkg-config 未安装，可能影响依赖库查找"
    fi
    
    print_success "依赖检查完成"
}

# 清理构建目录
clean_build() {
    print_info "清理构建目录..."
    rm -rf build
    mkdir -p build
}

# 配置构建
configure_build() {
    print_info "配置构建..."
    cd build
    
    # CMake 配置
    cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
    
    cd ..
    print_success "构建配置完成"
}

# 编译项目
build_project() {
    print_info "编译项目..."
    cd build
    
    # 获取 CPU 核心数
    NPROC=$(nproc 2>/dev/null || echo 4)
    
    make -j${NPROC}
    
    cd ..
    print_success "项目编译完成"
}

# 运行单元测试
run_unit_tests() {
    print_info "运行单元测试..."
    cd build
    
    if [ -f "unit_tests" ]; then
        ./unit_tests --gtest_output=xml:unit_test_results.xml
        print_success "单元测试完成"
    else
        print_warning "单元测试可执行文件不存在"
    fi
    
    cd ..
}

# 运行集成测试
run_integration_tests() {
    print_info "运行集成测试..."
    cd build
    
    if [ -f "integration_tests" ]; then
        ./integration_tests --gtest_output=xml:integration_test_results.xml
        print_success "集成测试完成"
    else
        print_warning "集成测试可执行文件不存在"
    fi
    
    cd ..
}

# 运行性能测试
run_benchmark_tests() {
    print_info "运行性能测试..."
    cd build
    
    if [ -f "benchmark_tests" ]; then
        echo "性能测试结果:" > benchmark_results.txt
        ./benchmark_tests 2>&1 | tee -a benchmark_results.txt
        print_success "性能测试完成，结果保存到 benchmark_results.txt"
    else
        print_warning "性能测试可执行文件不存在"
    fi
    
    cd ..
}

# 运行示例程序
run_example() {
    print_info "运行示例程序..."
    cd build
    
    if [ -f "bitcask_example" ]; then
        ./bitcask_example
        print_success "示例程序运行完成"
    else
        print_warning "示例程序可执行文件不存在"
    fi
    
    cd ..
}

# 生成测试报告
generate_report() {
    print_info "生成测试报告..."
    cd build
    
    # 创建报告目录
    mkdir -p reports
    
    # 移动测试结果文件
    if [ -f "unit_test_results.xml" ]; then
        mv unit_test_results.xml reports/
    fi
    
    if [ -f "integration_test_results.xml" ]; then
        mv integration_test_results.xml reports/
    fi
    
    if [ -f "benchmark_results.txt" ]; then
        mv benchmark_results.txt reports/
    fi
    
    # 生成简单的 HTML 报告
    cat > reports/index.html << EOF
<!DOCTYPE html>
<html>
<head>
    <title>Bitcask C++ Test Report</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 40px; }
        .header { color: #333; border-bottom: 2px solid #ccc; padding-bottom: 10px; }
        .section { margin: 20px 0; }
        .success { color: green; }
        .warning { color: orange; }
        .error { color: red; }
    </style>
</head>
<body>
    <h1 class="header">Bitcask C++ Test Report</h1>
    <div class="section">
        <h2>Generated: $(date)</h2>
    </div>
    <div class="section">
        <h2>Test Results</h2>
        <ul>
            <li><a href="unit_test_results.xml">Unit Test Results (XML)</a></li>
            <li><a href="integration_test_results.xml">Integration Test Results (XML)</a></li>
            <li><a href="benchmark_results.txt">Benchmark Results</a></li>
        </ul>
    </div>
</body>
</html>
EOF
    
    cd ..
    print_success "测试报告生成完成，查看 build/reports/index.html"
}

# 代码覆盖率分析（可选）
run_coverage() {
    if command -v gcov &> /dev/null && command -v lcov &> /dev/null; then
        print_info "运行代码覆盖率分析..."
        
        # 重新配置为 Debug 模式并启用覆盖率
        cd build
        cmake .. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="--coverage"
        make clean
        make -j$(nproc)
        
        # 运行测试
        ./unit_tests || true
        ./integration_tests || true
        
        # 生成覆盖率报告
        lcov --capture --directory . --output-file coverage.info
        lcov --remove coverage.info '/usr/*' --output-file coverage.info
        lcov --remove coverage.info '*/tests/*' --output-file coverage.info
        
        genhtml coverage.info --output-directory coverage_report
        
        cd ..
        print_success "代码覆盖率报告生成完成，查看 build/coverage_report/index.html"
    else
        print_warning "gcov 或 lcov 未安装，跳过代码覆盖率分析"
    fi
}

# 主函数
main() {
    echo "======================================"
    echo "    Bitcask C++ 构建和测试脚本"
    echo "======================================"
    
    # 解析命令行参数
    SKIP_DEPS=false
    SKIP_CLEAN=false
    SKIP_TESTS=false
    RUN_COVERAGE=false
    
    while [[ $# -gt 0 ]]; do
        case $1 in
            --skip-deps)
                SKIP_DEPS=true
                shift
                ;;
            --skip-clean)
                SKIP_CLEAN=true
                shift
                ;;
            --skip-tests)
                SKIP_TESTS=true
                shift
                ;;
            --coverage)
                RUN_COVERAGE=true
                shift
                ;;
            -h|--help)
                echo "用法: $0 [选项]"
                echo "选项:"
                echo "  --skip-deps     跳过依赖检查"
                echo "  --skip-clean    跳过清理构建目录"
                echo "  --skip-tests    跳过测试运行"
                echo "  --coverage      运行代码覆盖率分析"
                echo "  -h, --help      显示帮助信息"
                exit 0
                ;;
            *)
                print_error "未知选项: $1"
                exit 1
                ;;
        esac
    done
    
    # 执行构建和测试步骤
    if [ "$SKIP_DEPS" = false ]; then
        check_dependencies
    fi
    
    if [ "$SKIP_CLEAN" = false ]; then
        clean_build
    fi
    
    configure_build
    build_project
    
    if [ "$SKIP_TESTS" = false ]; then
        run_unit_tests
        run_integration_tests
        run_benchmark_tests
        run_example
        generate_report
    fi
    
    if [ "$RUN_COVERAGE" = true ]; then
        run_coverage
    fi
    
    print_success "所有任务完成！"
    
    # 显示构建结果
    echo ""
    echo "======================================"
    echo "         构建结果"
    echo "======================================"
    
    cd build
    echo "可执行文件:"
    ls -la bitcask_* unit_tests integration_tests benchmark_tests 2>/dev/null || echo "  无"
    
    echo ""
    echo "库文件:"
    ls -la libbitcask.* 2>/dev/null || echo "  无"
    
    echo ""
    echo "测试报告: build/reports/"
    
    if [ "$RUN_COVERAGE" = true ]; then
        echo "覆盖率报告: build/coverage_report/"
    fi
    
    cd ..
}

# 运行主函数
main "$@"
