cmake_minimum_required(VERSION 3.16)
project(flowlex VERSION 0.1.0 LANGUAGES CXX)

add_executable(flowlex src/flowlex.cpp)
target_compile_features(flowlex PRIVATE cxx_std_17)
target_compile_options(flowlex PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
