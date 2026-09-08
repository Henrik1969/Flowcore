cmake_minimum_required(VERSION 3.22)
project(flowinspect LANGUAGES CXX)

set(FLOWINSPECT_VERSION "0.1")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(flowinspect src/main.cpp)
target_compile_options(flowinspect PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(flowinspect PRIVATE flowcontracts)

enable_testing()
add_test(NAME flowinspect_cli
    COMMAND ${CMAKE_COMMAND} -E env
        FLOWINSPECT_BIN=$<TARGET_FILE:flowinspect>
        FLOWMINI_BIN=$<TARGET_FILE:flowmini>
        FLOWANALYST_BIN=$<TARGET_FILE:flowanalyst>
        FLOWPARALLEL_BIN=$<TARGET_FILE:flowparallel>
        FLOWOPTIMIZE_BIN=$<TARGET_FILE:flowoptimize>
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/run-flowinspect-tests.sh)
