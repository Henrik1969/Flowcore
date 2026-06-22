cmake_minimum_required(VERSION 3.22)
project(refactored_flow_policy_envelope LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(flow_common STATIC
    flow_common.cpp
)
target_include_directories(flow_common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(flow_common PRIVATE -Wall -Wextra -pedantic)

add_executable(flowbase64
    base64_main.cpp
    base64_stage.cpp
)
target_link_libraries(flowbase64 PRIVATE flow_common)
target_compile_options(flowbase64 PRIVATE -Wall -Wextra -pedantic)

add_executable(flowhex
    hex_main.cpp
    hex_stage.cpp
)
target_link_libraries(flowhex PRIVATE flow_common)
target_compile_options(flowhex PRIVATE -Wall -Wextra -pedantic)

add_executable(bitart
    bitart_main.cpp
    bitart_stage.cpp
)
target_link_libraries(bitart PRIVATE flow_common)
target_compile_options(bitart PRIVATE -Wall -Wextra -pedantic)

add_executable(flowexec
    collatz_main.cpp
    collatz_pipeline.cpp
)
target_link_libraries(flowexec PRIVATE flow_common)
target_compile_options(flowexec PRIVATE -Wall -Wextra -pedantic)
