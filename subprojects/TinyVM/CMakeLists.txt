cmake_minimum_required(VERSION 3.22)
project(tinyvm_recovered LANGUAGES C)

include(CTest)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

add_library(tinyvm_recovered STATIC
    src/instructions.c
    src/vm.c
)
target_include_directories(tinyvm_recovered PUBLIC include)
target_compile_options(tinyvm_recovered PRIVATE -Wall -Wextra -Wpedantic)
# vm.c deliberately uses GNU labels-as-values. Keep strict diagnostics for the
# portable handlers while acknowledging that one isolated extension boundary.
set_source_files_properties(src/vm.c PROPERTIES COMPILE_OPTIONS -Wno-pedantic)

add_executable(tinyvm_recovered_demo src/main.c)
target_link_libraries(tinyvm_recovered_demo PRIVATE tinyvm_recovered)

if(BUILD_TESTING)
    add_test(NAME tinyvm_recovered_demo COMMAND tinyvm_recovered_demo)
    set_tests_properties(tinyvm_recovered_demo PROPERTIES
        PASS_REGULAR_EXPRESSION "result=42;halted pc=4")
endif()
