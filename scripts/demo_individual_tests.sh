#!/bin/bash

# Bitcask C++ 单独测试演示脚本

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

print_info() {
    echo -e "${BLUE}[DEMO]${NC} $1"
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

print_separator() {
    echo "========================================"
}

demo_header() {
    clear
    print_separator
    echo -e "${BLUE}    Bitcask C++ 单独测试演示${NC}"
    print_separator
    echo ""
}

demo_build() {
    print_info "步骤 1: 构建项目"
    echo "运行命令: make build"
    echo ""
    
    if make build; then
        print_success "项目构建成功！"
    else
        print_error "项目构建失败！"
        exit 1
    fi
    echo ""
    read -p "按 Enter 继续..."
}

demo_list_tests() {
    print_info "步骤 2: 查看可用测试"
    echo "运行命令: make list-tests"
    echo ""
    
    make list-tests
    echo ""
    read -p "按 Enter 继续..."
}

demo_single_test() {
    print_info "步骤 3: 运行单个测试模块"
    echo "运行命令: make test-log-record"
    echo ""
    
    if make test-log-record; then
        print_success "日志记录测试完成！"
    else
        print_warning "日志记录测试有部分失败（这在演示中是正常的）"
    fi
    echo ""
    read -p "按 Enter 继续..."
}

demo_filtered_test() {
    print_info "步骤 4: 运行过滤的测试用例"
    echo "运行命令: ./scripts/run_individual_tests.sh test_log_record --filter=\"*Encode*\""
    echo ""
    
    if ./scripts/run_individual_tests.sh test_log_record --filter="*Encode*"; then
        print_success "过滤测试完成！"
    else
        print_warning "过滤测试有部分失败"
    fi
    echo ""
    read -p "按 Enter 继续..."
}

demo_repeat_test() {
    print_info "步骤 5: 重复运行测试（稳定性测试）"
    echo "运行命令: ./scripts/run_individual_tests.sh test_log_record --repeat=3"
    echo ""
    
    if ./scripts/run_individual_tests.sh test_log_record --repeat=3; then
        print_success "重复测试完成！"
    else
        print_warning "重复测试有部分失败"
    fi
    echo ""
    read -p "按 Enter 继续..."
}

demo_xml_report() {
    print_info "步骤 6: 生成XML测试报告"
    echo "运行命令: ./scripts/run_individual_tests.sh test_log_record --xml"
    echo ""
    
    if ./scripts/run_individual_tests.sh test_log_record --xml; then
        print_success "XML报告生成完成！"
        if [ -d "build/test_reports" ]; then
            echo "报告位置: build/test_reports/"
            ls -la build/test_reports/
        fi
    else
        print_warning "XML报告生成有问题"
    fi
    echo ""
    read -p "按 Enter 继续..."
}

demo_group_tests() {
    print_info "步骤 7: 运行测试组"
    echo "运行命令: make test-unit"
    echo ""
    
    echo "这将运行所有单元测试，可能需要一些时间..."
    if make test-unit; then
        print_success "单元测试组完成！"
    else
        print_warning "单元测试组有部分失败"
    fi
    echo ""
    read -p "按 Enter 继续..."
}

demo_direct_execution() {
    print_info "步骤 8: 直接执行测试可执行文件"
    echo "运行命令: cd build && ./test_log_record --gtest_list_tests"
    echo ""
    
    cd build
    if [ -f "test_log_record" ]; then
        echo "列出 test_log_record 中的所有测试用例："
        ./test_log_record --gtest_list_tests
        print_success "测试用例列表显示完成！"
    else
        print_error "test_log_record 可执行文件不存在"
    fi
    cd ..
    echo ""
    read -p "按 Enter 继续..."
}

demo_performance_check() {
    print_info "步骤 9: 性能测试演示"
    echo "运行命令: ./scripts/run_individual_tests.sh test_log_record --verbose"
    echo ""
    
    if ./scripts/run_individual_tests.sh test_log_record --verbose; then
        print_success "详细测试完成！"
    else
        print_warning "详细测试有部分问题"
    fi
    echo ""
    read -p "按 Enter 继续..."
}

demo_summary() {
    print_info "演示总结"
    print_separator
    echo ""
    echo "恭喜！你已经学会了如何使用 Bitcask C++ 的单独测试系统。"
    echo ""
    echo "主要命令回顾："
    echo "  make list-tests           - 列出所有测试"
    echo "  make test-<module>        - 运行特定模块测试"
    echo "  make test-unit            - 运行所有单元测试"
    echo "  make test-integration     - 运行集成测试"
    echo "  make test-benchmark       - 运行性能测试"
    echo ""
    echo "高级选项："
    echo "  --filter    - 过滤特定测试用例"
    echo "  --repeat    - 重复运行测试"
    echo "  --xml       - 生成XML报告"
    echo "  --verbose   - 详细输出"
    echo ""
    echo "更多信息请查看: TESTING_GUIDE.md"
    print_separator
}

main() {
    demo_header
    
    echo "这个演示将展示如何使用 Bitcask C++ 的单独测试功能。"
    echo ""
    read -p "按 Enter 开始演示..."
    
    demo_build
    demo_list_tests
    demo_single_test
    demo_filtered_test
    demo_repeat_test
    demo_xml_report
    demo_group_tests
    demo_direct_execution
    demo_performance_check
    demo_summary
    
    print_success "演示完成！感谢你的参与！"
}

# 检查是否在正确的目录
if [ ! -f "CMakeLists.txt" ] || [ ! -d "scripts" ]; then
    print_error "请在项目根目录运行此脚本"
    exit 1
fi

# 运行演示
main
