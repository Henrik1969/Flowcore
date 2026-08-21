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
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/run-flowlower-tests.sh)
if(TARGET flowmini AND TARGET flowanalyst AND TARGET flowparallel AND TARGET flowoptimize AND TARGET flowbind)
    set_tests_properties(flowlower_pipeline PROPERTIES
        ENVIRONMENT "FLOWMINI_BIN=$<TARGET_FILE:flowmini>;FLOWANALYST_BIN=$<TARGET_FILE:flowanalyst>;FLOWPARALLEL_BIN=$<TARGET_FILE:flowparallel>;FLOWOPTIMIZE_BIN=$<TARGET_FILE:flowoptimize>;FLOWBIND_BIN=$<TARGET_FILE:flowbind>")
endif()

add_test(NAME flowcore_pipeline_matrix
    COMMAND ${CMAKE_COMMAND} -E env
        FLOWCORE_ROOT=${CMAKE_CURRENT_SOURCE_DIR}/..
        FLOWLOWER_BIN=$<TARGET_FILE:flowlower>
        ${CMAKE_CURRENT_SOURCE_DIR}/../tools/run-flowcore-pipeline-matrix.sh)

add_test(NAME flowcore_pass_corpus
    COMMAND ${CMAKE_COMMAND} -E env
        FLOWCORE_ROOT=${CMAKE_CURRENT_SOURCE_DIR}/..
        FLOWLOWER_BIN=$<TARGET_FILE:flowlower>
        ${CMAKE_CURRENT_SOURCE_DIR}/../tools/run-flowcore-pass-corpus.sh)
if(TARGET flowmini AND TARGET flowanalyst AND TARGET flowparallel AND TARGET flowoptimize AND TARGET flowbind)
    set_tests_properties(flowcore_pipeline_matrix flowcore_pass_corpus PROPERTIES
        ENVIRONMENT "FLOWMINI_BIN=$<TARGET_FILE:flowmini>;FLOWANALYST_BIN=$<TARGET_FILE:flowanalyst>;FLOWPARALLEL_BIN=$<TARGET_FILE:flowparallel>;FLOWOPTIMIZE_BIN=$<TARGET_FILE:flowoptimize>;FLOWBIND_BIN=$<TARGET_FILE:flowbind>")
endif()
