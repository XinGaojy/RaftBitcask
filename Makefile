# Bitcask C++ Makefile
# 简化构建过程

.PHONY: all build clean test example install

# 默认目标
all: build

# 构建项目
build:
	@echo "Building Bitcask C++..."
	@mkdir -p build
	@cd build && cmake .. && $(MAKE) -j$$(nproc)
	@echo "Build completed!"

# 清理构建文件
clean:
	@echo "Cleaning build files..."
	@rm -rf build
	@echo "Clean completed!"

# 运行所有测试
test: build
	@echo "Running all tests..."
	@./scripts/run_individual_tests.sh --all
	@echo "All tests completed!"

# 运行单元测试
test-unit: build
	@echo "Running unit tests..."
	@./scripts/run_individual_tests.sh --unit

# 运行集成测试
test-integration: build
	@echo "Running integration tests..."
	@./scripts/run_individual_tests.sh --integration

# 运行性能测试
test-benchmark: build
	@echo "Running benchmark tests..."
	@./scripts/run_individual_tests.sh --benchmark

# 单独的测试目标
test-log-record: build
	@./scripts/run_individual_tests.sh test_log_record

test-io-manager: build
	@./scripts/run_individual_tests.sh test_io_manager

test-data-file: build
	@./scripts/run_individual_tests.sh test_data_file

test-index: build
	@./scripts/run_individual_tests.sh test_index

test-db: build
	@./scripts/run_individual_tests.sh test_db

test-write-batch: build
	@./scripts/run_individual_tests.sh test_write_batch

test-iterator: build
	@./scripts/run_individual_tests.sh test_iterator

# 列出所有可用测试
list-tests:
	@./scripts/run_individual_tests.sh --list

# 运行示例
example: build
	@echo "Running example..."
	@cd build && ./bitcask_example
	@echo "Example completed!"

# 安装（需要root权限）
install: build
	@echo "Installing Bitcask C++..."
	@cd build && sudo $(MAKE) install
	@echo "Installation completed!"

# 格式化代码（需要clang-format）
format:
	@echo "Formatting code..."
	@find src include examples tests -name "*.cpp" -o -name "*.h" | xargs clang-format -i
	@echo "Code formatting completed!"

# 静态分析（需要cppcheck）
analyze:
	@echo "Running static analysis..."
	@cppcheck --enable=all --std=c++17 src/ include/ 2>/dev/null || echo "cppcheck not found, skipping analysis"

# 生成文档（需要doxygen）
docs:
	@echo "Generating documentation..."
	@doxygen Doxyfile 2>/dev/null || echo "doxygen not found, skipping documentation generation"

# 帮助信息
help:
	@echo "Bitcask C++ Build System"
	@echo "========================"
	@echo ""
	@echo "Build targets:"
	@echo "  all       - Build the project (default)"
	@echo "  build     - Build the project"
	@echo "  clean     - Clean build files"
	@echo "  install   - Install the library system-wide"
	@echo ""
	@echo "Test targets:"
	@echo "  test               - Run all tests"
	@echo "  test-unit          - Run all unit tests"
	@echo "  test-integration   - Run integration tests"
	@echo "  test-benchmark     - Run performance tests"
	@echo "  list-tests         - List all available tests"
	@echo ""
	@echo "Individual unit tests:"
	@echo "  test-log-record    - Test log record functionality"
	@echo "  test-io-manager    - Test IO manager"
	@echo "  test-data-file     - Test data file management"
	@echo "  test-index         - Test index functionality"
	@echo "  test-db            - Test database core"
	@echo "  test-write-batch   - Test batch operations"
	@echo "  test-iterator      - Test iterator functionality"
	@echo ""
	@echo "Other targets:"
	@echo "  example   - Build and run example"
	@echo "  format    - Format code with clang-format"
	@echo "  analyze   - Run static analysis with cppcheck"
	@echo "  docs      - Generate documentation with doxygen"
	@echo "  help      - Show this help message"
	@echo ""
	@echo "Examples:"
	@echo "  make build"
	@echo "  make test-db"
	@echo "  make test-unit"
	@echo "  make clean && make build"
	@echo ""
	@echo "Advanced test options (use script directly):"
	@echo "  ./scripts/run_individual_tests.sh test_db --filter=\"DBTest.Put*\""
	@echo "  ./scripts/run_individual_tests.sh test_index --repeat=5"
	@echo "  ./scripts/run_individual_tests.sh --all --xml"
