add_executable(franken_ls main.cpp)
target_compile_features(franken_ls PRIVATE cxx_std_20)
target_compile_options(franken_ls PRIVATE -Wall -Wextra -Wpedantic)

add_test(NAME franken_ls_compatibility
    COMMAND "${CMAKE_CURRENT_LIST_DIR}/test-ls-facade.sh"
            $<TARGET_FILE:franken_ls>)
