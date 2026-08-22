cmake_minimum_required(VERSION 3.22)
project(flowanalyst LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(flowanalyst src/main.cpp)
target_compile_options(flowanalyst PRIVATE -Wall -Wextra -Wpedantic)

enable_testing()
add_test(NAME flowanalyst_pipeline
    COMMAND ${CMAKE_COMMAND} -E env
        FLOWANALYST_BIN=$<TARGET_FILE:flowanalyst>
        FLOWMINI_BIN=$<TARGET_FILE:flowmini>
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/run-flowanalyst-tests.sh)
