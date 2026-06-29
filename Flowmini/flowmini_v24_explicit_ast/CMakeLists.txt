# Flowmini Flowcore project build and control file
# ------------------------------------------------
#

# Required version is conservative Using 3.22 for now
cmake_minimum_required(VERSION 3.22)

# Setting the actual project
project(flowmini LANGUAGES C CXX)

# We test with ctest so we include it.
include(CTest)

# Again staying conservative so require C++20 instead of newer version
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# The only executable for now is the flowmini so just setting the sources and include
add_executable(flowmini
    src/flow_common.cpp
    src/flowmini_payload.cpp
    src/flowmini_lexer.cpp
    src/flowmini_parser.cpp
    src/flowmini_runtime.cpp
    src/flowmini_structural.cpp
    src/flowmini_token_tree_bridge.cpp
    src/main.cpp
        include/flowmini_ast.h
        src/flowmini_ast.cpp
        src/flowmini_ast_builder.cpp
        include/flowmini_ast_builder.h
)
target_include_directories(flowmini PRIVATE include)

# We use the two externals tokentree and Symtable that are related projects but are standalone builds
add_subdirectory(subprojects/TokenTree/tokentree_v1)
add_subdirectory(subprojects/SymbolTable/SymbolTable_v1_1)

#  We link static because we don't want these parts as dynamic for now
target_link_libraries(flowmini PRIVATE TokenTree_static SymbolTable::static)

# We add a test subdir and dependency
add_subdirectory(subprojects/testabi)
add_dependencies(flowmini flowmini_testabi)

# We set the compiler options for the default build
target_compile_options(flowmini PRIVATE
    -Wall
    -Wextra
    -pedantic
)

# Link
find_library(DL_LIBRARY dl)
if(DL_LIBRARY)
    target_link_libraries(flowmini PRIVATE ${DL_LIBRARY})
endif()

# ------------------------------------------------
# Project-visible workflow targets
# ------------------------------------------------
#
# These targets make the normal Flowmini workflows visible to CMake-aware tools
# such as CLion. Shell scripts may still exist as helpers, but the important
# project actions should be reachable as CMake targets.

add_custom_target(flowmini_suite
        COMMAND
        ${CMAKE_COMMAND} -E env
        FLOWMINI_ROOT=${CMAKE_SOURCE_DIR}
        FLOWMINI_BUILD_DIR=${CMAKE_BINARY_DIR}
        FLOWMINI_NO_BUILD=1
        FLOWMINI_JOBS=20
        ${CMAKE_SOURCE_DIR}/../../tools/run-flowmini-test-suite.sh
        DEPENDS flowmini
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Running Flowmini categorized test suite"
        VERBATIM
)

add_custom_target(flowmini_dump_token_tree_simple
        COMMAND
        $<TARGET_FILE:flowmini>
        --dump-token-tree-bridge
        simple
        examples/pass/fn_demo.flow
        DEPENDS flowmini
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Dumping Flowmini TokenTree bridge simple view"
        VERBATIM
)

add_custom_target(flowmini_dump_token_tree_json
        COMMAND
        $<TARGET_FILE:flowmini>
        --dump-token-tree-bridge
        json
        examples/pass/fn_demo.flow
        DEPENDS flowmini
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Dumping Flowmini TokenTree bridge JSON view"
        VERBATIM
)

add_custom_target(flowmini_dump_ast
    COMMAND
        $<TARGET_FILE:flowmini>
        --dump-ast
        examples/pass/fn_demo.flow
    DEPENDS flowmini
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Dumping Flowmini AST JSON view"
    VERBATIM
)