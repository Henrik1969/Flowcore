#include "test_framework.h"

#include <cstdlib>
#include <iostream>

std::vector<TestCase> &test_cases()
{
    static std::vector<TestCase> tests;
    return tests;
}

RegisterTest::RegisterTest(std::string_view name, bool (*run)())
{
    test_cases().push_back({name, run});
}

int main()
{
    int failures = 0;
    for (const auto &test : test_cases()) {
        if (!test.run()) {
            std::cerr << "FAIL " << test.name << '\n';
            ++failures;
        } else {
            std::cout << "PASS " << test.name << '\n';
        }
    }

    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
