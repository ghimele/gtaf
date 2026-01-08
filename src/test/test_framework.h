// test_framework.h - Simple test framework for GTAF
#pragma once

#include <iostream>
#include <functional>
#include <vector>
#include <string>
#include <sstream>

namespace gtaf::test {

class TestFailure : public std::runtime_error {
public:
    explicit TestFailure(const std::string& msg) : std::runtime_error(msg) {}
};

#define ASSERT_TRUE(condition) \
    do { \
        if (!(condition)) { \
            std::ostringstream oss; \
            oss << "Assertion failed: " << #condition \
                << " at " << __FILE__ << ":" << __LINE__; \
            throw gtaf::test::TestFailure(oss.str()); \
        } \
    } while (0)

#define ASSERT_FALSE(condition) ASSERT_TRUE(!(condition))

#define ASSERT_EQ(a, b) \
    do { \
        auto val_a = (a); \
        auto val_b = (b); \
        if (val_a != val_b) { \
            std::ostringstream oss; \
            oss << "Assertion failed: " << #a << " == " << #b \
                << " at " << __FILE__ << ":" << __LINE__; \
            throw gtaf::test::TestFailure(oss.str()); \
        } \
    } while (0)

#define ASSERT_NE(a, b) \
    do { \
        auto val_a = (a); \
        auto val_b = (b); \
        if (val_a == val_b) { \
            std::ostringstream oss; \
            oss << "Assertion failed: " << #a << " != " << #b \
                << " at " << __FILE__ << ":" << __LINE__; \
            throw gtaf::test::TestFailure(oss.str()); \
        } \
    } while (0)

class TestSuite {
public:
    struct TestCase {
        std::string name;
        std::function<void()> test_fn;
    };

    static TestSuite& instance() {
        static TestSuite suite;
        return suite;
    }

    void add_test(const std::string& name, std::function<void()> test_fn) {
        tests.push_back({name, std::move(test_fn)});
    }

    int run_all() {
        int passed = 0;
        int failed = 0;

        std::cout << "\n=== Running GTAF Unit Tests ===\n\n";

        for (const auto& test : tests) {
            std::cout << "[ RUN      ] " << test.name << "\n";
            try {
                test.test_fn();
                std::cout << "[       OK ] " << test.name << "\n";
                ++passed;
            } catch (const TestFailure& e) {
                std::cout << "[  FAILED  ] " << test.name << "\n";
                std::cout << "  " << e.what() << "\n";
                ++failed;
            } catch (const std::exception& e) {
                std::cout << "[  FAILED  ] " << test.name << "\n";
                std::cout << "  Unexpected exception: " << e.what() << "\n";
                ++failed;
            }
        }

        std::cout << "\n=== Test Results ===\n";
        std::cout << "Passed: " << passed << "\n";
        std::cout << "Failed: " << failed << "\n";
        std::cout << "Total:  " << (passed + failed) << "\n\n";

        return failed > 0 ? 1 : 0;
    }

private:
    std::vector<TestCase> tests;
};

class TestRegistrar {
public:
    TestRegistrar(const std::string& name, std::function<void()> test_fn) {
        TestSuite::instance().add_test(name, std::move(test_fn));
    }
};

#define TEST(suite_name, test_name) \
    void suite_name##_##test_name##_impl(); \
    static gtaf::test::TestRegistrar suite_name##_##test_name##_registrar( \
        #suite_name "." #test_name, suite_name##_##test_name##_impl); \
    void suite_name##_##test_name##_impl()

} // namespace gtaf::test
