cmake_minimum_required(VERSION 3.22)
project(flowlower LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(flowlower src/main.cpp)
target_compile_options(flowlower PRIVATE -Wall -Wextra -Wpedantic)

enable_testing()
add_test(NAME flowlower_pipeline
    COMMAND ${CMAKE_COMMAND} -E env
        FLOWLOWER_BIN=$<TARGET_FILE:flowlower>
        FLOWOPTIMIZE_BIN=${CMAKE_CURRENT_SOURCE_DIR}/../Flowoptimize/build/flowoptimize
        FLOWANALYST_BIN=${CMAKE_CURRENT_SOURCE_DIR}/../Flowanalyst/build/flowanalyst
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/run-flowlower-tests.sh)

add_test(NAME flowcore_pipeline_matrix
    COMMAND ${CMAKE_COMMAND} -E env
        FLOWCORE_ROOT=${CMAKE_CURRENT_SOURCE_DIR}/..
        FLOWMINI_BIN=${CMAKE_CURRENT_SOURCE_DIR}/../Flowmini/flowmini_v25_symboltable_projection/cmake-build-debug/flowmini
        FLOWANALYST_BIN=${CMAKE_CURRENT_SOURCE_DIR}/../Flowanalyst/build/flowanalyst
        FLOWOPTIMIZE_BIN=${CMAKE_CURRENT_SOURCE_DIR}/../Flowoptimize/build/flowoptimize
        FLOWLOWER_BIN=$<TARGET_FILE:flowlower>
        ${CMAKE_CURRENT_SOURCE_DIR}/../tools/run-flowcore-pipeline-matrix.sh)

add_test(NAME flowcore_pass_corpus
    COMMAND ${CMAKE_COMMAND} -E env
        FLOWCORE_ROOT=${CMAKE_CURRENT_SOURCE_DIR}/..
        FLOWMINI_BIN=${CMAKE_CURRENT_SOURCE_DIR}/../Flowmini/flowmini_v25_symboltable_projection/cmake-build-debug/flowmini
        FLOWANALYST_BIN=${CMAKE_CURRENT_SOURCE_DIR}/../Flowanalyst/build/flowanalyst
        FLOWOPTIMIZE_BIN=${CMAKE_CURRENT_SOURCE_DIR}/../Flowoptimize/build/flowoptimize
        FLOWLOWER_BIN=$<TARGET_FILE:flowlower>
        ${CMAKE_CURRENT_SOURCE_DIR}/../tools/run-flowcore-pass-corpus.sh)
