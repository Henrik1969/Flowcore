cmake_minimum_required(VERSION 3.22)
project(Flowcore VERSION 0.1.0 LANGUAGES CXX)

include(CTest)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(FLOWCORE_BUILD_ASTLIIB "Build AstLiib as the generic token-tree subproject" ON)
option(FLOWCORE_BUILD_PROTOTYPES "Build existing lexer/parser prototype tools" ON)
option(FLOWCORE_BUILD_STAGE_PLACEHOLDERS "Build placeholder compiler-chain stage tools" ON)

set(FLOWCORE_ASTLIIB_SOURCE_DIR
        "${CMAKE_CURRENT_LIST_DIR}/../AstLiib"
        CACHE PATH
        "Path to the AstLiib subproject source tree")

if(FLOWCORE_BUILD_ASTLIIB)
        if(NOT EXISTS "${FLOWCORE_ASTLIIB_SOURCE_DIR}/CMakeLists.txt")
                message(FATAL_ERROR "AstLiib subproject not found at: ${FLOWCORE_ASTLIIB_SOURCE_DIR}")
        endif()

        add_subdirectory("${FLOWCORE_ASTLIIB_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/subprojects/AstLiib")
endif()

if(FLOWCORE_BUILD_PROTOTYPES)
        add_subdirectory(lexers/flowlex_v0_1a)
        add_subdirectory(lexers/langlab_table_lexer_v0_1a)
        add_subdirectory(parsers/Handwritten_V1)
endif()

if(FLOWCORE_BUILD_STAGE_PLACEHOLDERS)
        add_subdirectory(stages/flowoptimize)
        add_subdirectory(stages/flowlink)
        add_subdirectory(stages/flowemit)
endif()

set(FLOWCORE_CHAIN_TARGETS)
foreach(candidate_target IN ITEMS
        AstLiib
        flowlex
        flowparse
        flowoptimize
        flowlink
        flowemit)
        if(TARGET "${candidate_target}")
                list(APPEND FLOWCORE_CHAIN_TARGETS "${candidate_target}")
        endif()
endforeach()

add_custom_target(flowcore_chain DEPENDS ${FLOWCORE_CHAIN_TARGETS})
