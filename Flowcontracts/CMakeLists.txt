cmake_minimum_required(VERSION 3.22)
project(flowcontracts LANGUAGES CXX)

add_library(flowcontracts INTERFACE)
target_include_directories(flowcontracts INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(flowcontracts INTERFACE cxx_std_20)

add_executable(flowcontracts_tests tests/json_tests.cpp)
target_link_libraries(flowcontracts_tests PRIVATE flowcontracts)
target_compile_options(flowcontracts_tests PRIVATE -Wall -Wextra -Wpedantic)

enable_testing()
add_test(NAME flowcontracts_json COMMAND flowcontracts_tests)
