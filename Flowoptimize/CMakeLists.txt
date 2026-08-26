cmake_minimum_required(VERSION 3.22)
project(flowoptimize LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(flowoptimize src/main.cpp)
target_compile_options(flowoptimize PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(flowoptimize PRIVATE flowcontracts)

enable_testing()
add_test(NAME flowoptimize_pipeline
    COMMAND ${CMAKE_COMMAND} -E env
        FLOWOPTIMIZE_BIN=$<TARGET_FILE:flowoptimize>
        FLOWPARALLEL_BIN=$<TARGET_FILE:flowparallel>
        FLOWANALYST_BIN=$<TARGET_FILE:flowanalyst>
        FLOWMINI_BIN=$<TARGET_FILE:flowmini>
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/run-flowoptimize-tests.sh)
