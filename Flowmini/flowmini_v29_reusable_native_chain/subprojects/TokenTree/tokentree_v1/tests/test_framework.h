#ifndef TOKENTREE_TEST_FRAMEWORK_H
#define TOKENTREE_TEST_FRAMEWORK_H

#include <iostream>
#include <string_view>
#include <vector>

struct TestCase {
    std::string_view name;
    bool (*run)();
};

std::vector<TestCase> &test_cases();

struct RegisterTest {
    RegisterTest(std::string_view name, bool (*run)());
};

#define TT_TEST(name) \
    static bool name(); \
    static RegisterTest name##_registered{#name, name}; \
    static bool name()

#define TT_EXPECT(expr) \
    do { \
        if (!(expr)) { \
            std::cerr << "EXPECT failed: " #expr << " at " << __FILE__ << ':' << __LINE__ << '\n'; \
            return false; \
        } \
    } while (false)

#endif
