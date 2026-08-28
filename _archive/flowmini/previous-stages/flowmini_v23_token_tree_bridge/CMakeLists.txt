cmake_minimum_required(VERSION 3.22)
project(flowmini LANGUAGES C CXX)

include(CTest)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(flowmini
    src/flow_common.cpp
    src/flowmini_payload.cpp
    src/flowmini_lexer.cpp
    src/flowmini_parser.cpp
    src/flowmini_runtime.cpp
    src/flowmini_structural.cpp
    src/flowmini_token_tree_bridge.cpp
    src/main.cpp
)

target_include_directories(flowmini PRIVATE include)

add_subdirectory(subprojects/TokenTree/tokentree_v1)
add_subdirectory(subprojects/SymbolTable/SymbolTable_v1_1)
target_link_libraries(flowmini PRIVATE TokenTree_static SymbolTable::static)

add_subdirectory(subprojects/testabi)
add_dependencies(flowmini flowmini_testabi)

target_compile_options(flowmini PRIVATE
    -Wall
    -Wextra
    -pedantic
)

find_library(DL_LIBRARY dl)
if(DL_LIBRARY)
    target_link_libraries(flowmini PRIVATE ${DL_LIBRARY})
endif()
