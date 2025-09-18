#pragma once

#include <iostream>
#include <functional>
#include <vector>
#include <string>
#include <stdexcept>

// 简化的GoogleTest实现
namespace testing {

class Test {
public:
    virtual ~Test() = default;
    virtual void SetUp() {}
    virtual void TearDown() {}
    virtual void TestBody() = 0;
};

struct TestInfo {
    std::string test_case_name;
    std::string test_name;
    std::function<Test*()> factory;
};

class TestRegistry {
public:
    static TestRegistry& GetInstance() {
        static TestRegistry instance;
        return instance;
    }
    
    void AddTest(const std::string& test_case, const std::string& test_name, std::function<Test*()> factory) {
        tests.push_back({test_case, test_name, factory});
    }
    
    int RunAllTests() {
        int failures = 0;
        std::cout << "[==========] Running " << tests.size() << " tests.\n";
        
        for (const auto& test : tests) {
            std::cout << "[ RUN      ] " << test.test_case_name << "." << test.test_name << "\n";
            
            try {
                auto* test_instance = test.factory();
                test_instance->SetUp();
                test_instance->TestBody();
                test_instance->TearDown();
                delete test_instance;
                std::cout << "[       OK ] " << test.test_case_name << "." << test.test_name << "\n";
            } catch (const std::exception& e) {
                std::cout << "[  FAILED  ] " << test.test_case_name << "." << test.test_name << " - " << e.what() << "\n";
                failures++;
            }
        }
        
        std::cout << "[==========] " << tests.size() << " tests ran.\n";
        if (failures == 0) {
            std::cout << "[  PASSED  ] " << tests.size() << " tests.\n";
        } else {
            std::cout << "[  FAILED  ] " << failures << " tests.\n";
        }
        
        return failures;
    }
    
private:
    std::vector<TestInfo> tests;
};

// 测试注册宏
#define TEST(test_case_name, test_name) \
    class test_case_name##_##test_name##_Test : public ::testing::Test { \
    public: \
        test_case_name##_##test_name##_Test() {} \
    private: \
        virtual void TestBody(); \
        static ::testing::Test* Create() { \
            return new test_case_name##_##test_name##_Test; \
        } \
        static int registered_; \
    }; \
    int test_case_name##_##test_name##_Test::registered_ = \
        (::testing::TestRegistry::GetInstance().AddTest(#test_case_name, #test_name, \
         test_case_name##_##test_name##_Test::Create), 0); \
    void test_case_name##_##test_name##_Test::TestBody()

// 断言宏
#define EXPECT_EQ(expected, actual) \
    do { \
        auto exp_val = (expected); \
        auto act_val = (actual); \
        if (exp_val != act_val) { \
            std::string msg = "Expected: " + std::to_string(exp_val) + ", Actual: " + std::to_string(act_val); \
            throw std::runtime_error(msg); \
        } \
    } while (0)

#define EXPECT_TRUE(condition) \
    do { \
        if (!(condition)) { \
            throw std::runtime_error("Expected true, but got false"); \
        } \
    } while (0)

#define EXPECT_FALSE(condition) \
    do { \
        if (condition) { \
            throw std::runtime_error("Expected false, but got true"); \
        } \
    } while (0)

#define EXPECT_NE(not_expected, actual) \
    do { \
        auto not_exp_val = (not_expected); \
        auto act_val = (actual); \
        if (not_exp_val == act_val) { \
            throw std::runtime_error("Expected not equal, but values are equal"); \
        } \
    } while (0)

#define EXPECT_GT(val1, val2) \
    do { \
        auto v1 = (val1); \
        auto v2 = (val2); \
        if (!(v1 > v2)) { \
            std::string msg = "Expected " + std::to_string(v1) + " > " + std::to_string(v2); \
            throw std::runtime_error(msg); \
        } \
    } while (0)

#define EXPECT_LT(val1, val2) \
    do { \
        auto v1 = (val1); \
        auto v2 = (val2); \
        if (!(v1 < v2)) { \
            std::string msg = "Expected " + std::to_string(v1) + " < " + std::to_string(v2); \
            throw std::runtime_error(msg); \
        } \
    } while (0)

#define EXPECT_GE(val1, val2) \
    do { \
        auto v1 = (val1); \
        auto v2 = (val2); \
        if (!(v1 >= v2)) { \
            std::string msg = "Expected " + std::to_string(v1) + " >= " + std::to_string(v2); \
            throw std::runtime_error(msg); \
        } \
    } while (0)

#define EXPECT_LE(val1, val2) \
    do { \
        auto v1 = (val1); \
        auto v2 = (val2); \
        if (!(v1 <= v2)) { \
            std::string msg = "Expected " + std::to_string(v1) + " <= " + std::to_string(v2); \
            throw std::runtime_error(msg); \
        } \
    } while (0)

#define ASSERT_EQ EXPECT_EQ
#define ASSERT_TRUE EXPECT_TRUE
#define ASSERT_FALSE EXPECT_FALSE
#define ASSERT_NE EXPECT_NE
#define ASSERT_GT EXPECT_GT
#define ASSERT_LT EXPECT_LT
#define ASSERT_GE EXPECT_GE
#define ASSERT_LE EXPECT_LE

} // namespace testing

// 全局函数
int RUN_ALL_TESTS() {
    return testing::TestRegistry::GetInstance().RunAllTests();
}
