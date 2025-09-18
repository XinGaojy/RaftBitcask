#!/bin/bash

# Bitcask C++ 单独测试运行脚本

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

# 定义所有测试模块
UNIT_TESTS=(
    "test_log_record"
    "test_io_manager" 
    "test_data_file"
    "test_index"
    "test_db"
    "test_write_batch"
    "test_iterator"
)

INTEGRATION_TESTS=(
    "integration_tests"
)

BENCHMARK_TESTS=(
    "benchmark_tests"
)

# 显示帮助信息
show_help() {
    echo "Bitcask C++ 单独测试运行脚本"
    echo ""
    echo "用法: $0 [选项] [测试名称]"
    echo ""
    echo "选项:"
    echo "  -h, --help          显示帮助信息"
    echo "  -l, --list          列出所有可用的测试"
    echo "  -a, --all           运行所有测试"
    echo "  -u, --unit          运行所有单元测试"
    echo "  -i, --integration   运行集成测试"
    echo "  -b, --benchmark     运行性能测试"
    echo "  -v, --verbose       详细输出"
    echo "  -x, --xml           生成XML测试报告"
    echo "  --filter <pattern>  运行匹配模式的测试用例"
    echo "  --repeat <n>        重复运行测试n次"
    echo ""
    echo "测试名称:"
    echo "  单元测试:"
    for test in "${UNIT_TESTS[@]}"; do
        echo "    $test"
    done
    echo "  集成测试:"
    for test in "${INTEGRATION_TESTS[@]}"; do
        echo "    $test"
    done
    echo "  性能测试:"
    for test in "${BENCHMARK_TESTS[@]}"; do
        echo "    $test"
    done
    echo ""
    echo "示例:"
    echo "  $0 test_log_record                    # 运行日志记录测试"
    echo "  $0 --unit                            # 运行所有单元测试"
    echo "  $0 test_db --filter=\"DBTest.Put*\"    # 运行特定测试用例"
    echo "  $0 test_index --repeat=5             # 重复运行5次"
    echo "  $0 --all --xml                       # 运行所有测试并生成XML报告"
}

# 列出所有测试
list_tests() {
    echo "可用的测试："
    echo ""
    echo "单元测试 (Unit Tests):"
    for test in "${UNIT_TESTS[@]}"; do
        echo "  ✓ $test"
    done
    echo ""
    echo "集成测试 (Integration Tests):"
    for test in "${INTEGRATION_TESTS[@]}"; do
        echo "  ✓ $test"
    done
    echo ""
    echo "性能测试 (Benchmark Tests):"
    for test in "${BENCHMARK_TESTS[@]}"; do
        echo "  ✓ $test"
    done
}

# 检查构建目录
check_build_dir() {
    if [ ! -d "build" ]; then
        print_error "构建目录不存在，请先运行构建："
        echo "  ./scripts/build_and_test.sh"
        exit 1
    fi
    
    cd build
}

# 运行单个测试
run_single_test() {
    local test_name=$1
    local filter_pattern=$2
    local repeat_count=${3:-1}
    local xml_output=$4
    local verbose=$5
    
    if [ ! -f "$test_name" ]; then
        print_error "测试可执行文件不存在: $test_name"
        return 1
    fi
    
    print_info "运行测试: $test_name"
    
    # 构建测试命令
    local cmd="./$test_name"
    
    if [ -n "$filter_pattern" ]; then
        cmd="$cmd --gtest_filter=\"$filter_pattern\""
    fi
    
    if [ "$xml_output" = "true" ]; then
        cmd="$cmd --gtest_output=xml:${test_name}_results.xml"
    fi
    
    if [ "$verbose" = "true" ]; then
        cmd="$cmd --gtest_print_time=1"
    fi
    
    # 重复运行测试
    local success_count=0
    for ((i=1; i<=repeat_count; i++)); do
        if [ $repeat_count -gt 1 ]; then
            print_info "运行第 $i/$repeat_count 次"
        fi
        
        if eval $cmd; then
            ((success_count++))
            if [ $repeat_count -gt 1 ]; then
                print_success "第 $i 次运行成功"
            fi
        else
            print_error "第 $i 次运行失败"
        fi
    done
    
    if [ $success_count -eq $repeat_count ]; then
        print_success "$test_name 测试完成 ($success_count/$repeat_count 成功)"
        return 0
    else
        print_error "$test_name 测试失败 ($success_count/$repeat_count 成功)"
        return 1
    fi
}

# 运行测试组
run_test_group() {
    local group_name=$1
    local tests_array=("${!2}")
    local filter_pattern=$3
    local repeat_count=$4
    local xml_output=$5
    local verbose=$6
    
    print_info "运行 $group_name 测试组"
    
    local total_tests=${#tests_array[@]}
    local passed_tests=0
    
    for test in "${tests_array[@]}"; do
        if run_single_test "$test" "$filter_pattern" "$repeat_count" "$xml_output" "$verbose"; then
            ((passed_tests++))
        fi
        echo ""
    done
    
    print_info "$group_name 测试组结果: $passed_tests/$total_tests 通过"
    
    if [ $passed_tests -eq $total_tests ]; then
        print_success "$group_name 测试组全部通过"
        return 0
    else
        print_error "$group_name 测试组有 $((total_tests - passed_tests)) 个测试失败"
        return 1
    fi
}

# 运行所有测试
run_all_tests() {
    local filter_pattern=$1
    local repeat_count=$2
    local xml_output=$3
    local verbose=$4
    
    print_info "运行所有测试"
    
    local total_groups=0
    local passed_groups=0
    
    # 运行单元测试
    ((total_groups++))
    if run_test_group "单元测试" UNIT_TESTS[@] "$filter_pattern" "$repeat_count" "$xml_output" "$verbose"; then
        ((passed_groups++))
    fi
    
    # 运行集成测试
    ((total_groups++))
    if run_test_group "集成测试" INTEGRATION_TESTS[@] "$filter_pattern" "$repeat_count" "$xml_output" "$verbose"; then
        ((passed_groups++))
    fi
    
    # 运行性能测试
    ((total_groups++))
    if run_test_group "性能测试" BENCHMARK_TESTS[@] "$filter_pattern" "$repeat_count" "$xml_output" "$verbose"; then
        ((passed_groups++))
    fi
    
    echo "========================================"
    print_info "所有测试结果: $passed_groups/$total_groups 测试组通过"
    
    if [ $passed_groups -eq $total_groups ]; then
        print_success "所有测试通过！"
        return 0
    else
        print_error "有 $((total_groups - passed_groups)) 个测试组失败"
        return 1
    fi
}

# 生成测试报告
generate_report() {
    if [ "$1" = "true" ]; then
        print_info "生成测试报告..."
        
        # 创建报告目录
        mkdir -p test_reports
        
        # 移动XML文件到报告目录
        if ls *_results.xml 1> /dev/null 2>&1; then
            mv *_results.xml test_reports/
            print_success "XML测试报告已生成到 test_reports/ 目录"
        fi
        
        # 生成简单的HTML报告索引
        cat > test_reports/index.html << EOF
<!DOCTYPE html>
<html>
<head>
    <title>Bitcask C++ Individual Test Reports</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 40px; }
        .header { color: #333; border-bottom: 2px solid #ccc; padding-bottom: 10px; }
        .test-group { margin: 20px 0; }
        .test-item { margin: 10px 0; }
    </style>
</head>
<body>
    <h1 class="header">Bitcask C++ Individual Test Reports</h1>
    <p>Generated: $(date)</p>
    
    <div class="test-group">
        <h2>单元测试报告</h2>
EOF

        for test in "${UNIT_TESTS[@]}"; do
            if [ -f "test_reports/${test}_results.xml" ]; then
                echo "        <div class=\"test-item\"><a href=\"${test}_results.xml\">$test</a></div>" >> test_reports/index.html
            fi
        done
        
        cat >> test_reports/index.html << EOF
    </div>
    
    <div class="test-group">
        <h2>集成测试报告</h2>
EOF

        for test in "${INTEGRATION_TESTS[@]}"; do
            if [ -f "test_reports/${test}_results.xml" ]; then
                echo "        <div class=\"test-item\"><a href=\"${test}_results.xml\">$test</a></div>" >> test_reports/index.html
            fi
        done
        
        cat >> test_reports/index.html << EOF
    </div>
    
    <div class="test-group">
        <h2>性能测试报告</h2>
EOF

        for test in "${BENCHMARK_TESTS[@]}"; do
            if [ -f "test_reports/${test}_results.xml" ]; then
                echo "        <div class=\"test-item\"><a href=\"${test}_results.xml\">$test</a></div>" >> test_reports/index.html
            fi
        done
        
        echo "    </div>" >> test_reports/index.html
        echo "</body>" >> test_reports/index.html
        echo "</html>" >> test_reports/index.html
        
        print_success "HTML报告索引已生成: test_reports/index.html"
    fi
}

# 主函数
main() {
    # 默认参数
    local test_name=""
    local run_all=false
    local run_unit=false
    local run_integration=false
    local run_benchmark=false
    local filter_pattern=""
    local repeat_count=1
    local xml_output=false
    local verbose=false
    
    # 解析命令行参数
    while [[ $# -gt 0 ]]; do
        case $1 in
            -h|--help)
                show_help
                exit 0
                ;;
            -l|--list)
                list_tests
                exit 0
                ;;
            -a|--all)
                run_all=true
                shift
                ;;
            -u|--unit)
                run_unit=true
                shift
                ;;
            -i|--integration)
                run_integration=true
                shift
                ;;
            -b|--benchmark)
                run_benchmark=true
                shift
                ;;
            -v|--verbose)
                verbose=true
                shift
                ;;
            -x|--xml)
                xml_output=true
                shift
                ;;
            --filter)
                filter_pattern="$2"
                shift 2
                ;;
            --repeat)
                repeat_count="$2"
                shift 2
                ;;
            -*)
                print_error "未知选项: $1"
                show_help
                exit 1
                ;;
            *)
                test_name="$1"
                shift
                ;;
        esac
    done
    
    # 检查构建目录
    check_build_dir
    
    # 执行测试
    local exit_code=0
    
    if [ "$run_all" = true ]; then
        run_all_tests "$filter_pattern" "$repeat_count" "$xml_output" "$verbose"
        exit_code=$?
    elif [ "$run_unit" = true ]; then
        run_test_group "单元测试" UNIT_TESTS[@] "$filter_pattern" "$repeat_count" "$xml_output" "$verbose"
        exit_code=$?
    elif [ "$run_integration" = true ]; then
        run_test_group "集成测试" INTEGRATION_TESTS[@] "$filter_pattern" "$repeat_count" "$xml_output" "$verbose"
        exit_code=$?
    elif [ "$run_benchmark" = true ]; then
        run_test_group "性能测试" BENCHMARK_TESTS[@] "$filter_pattern" "$repeat_count" "$xml_output" "$verbose"
        exit_code=$?
    elif [ -n "$test_name" ]; then
        run_single_test "$test_name" "$filter_pattern" "$repeat_count" "$xml_output" "$verbose"
        exit_code=$?
    else
        print_error "请指定要运行的测试或使用 --help 查看帮助"
        exit 1
    fi
    
    # 生成报告
    generate_report "$xml_output"
    
    cd ..
    exit $exit_code
}

# 运行主函数
main "$@"
