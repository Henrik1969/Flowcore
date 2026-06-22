cmake_minimum_required(VERSION 3.22)
project(flowmini LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(flowmini
    src/flow_common.cpp
    src/flowmini_payload.cpp
    src/flowmini_lexer.cpp
    src/flowmini_parser.cpp
    src/flowmini_runtime.cpp
    src/main.cpp
)

target_include_directories(flowmini PRIVATE include)

target_compile_options(flowmini PRIVATE
    -Wall
    -Wextra
    -pedantic
)
