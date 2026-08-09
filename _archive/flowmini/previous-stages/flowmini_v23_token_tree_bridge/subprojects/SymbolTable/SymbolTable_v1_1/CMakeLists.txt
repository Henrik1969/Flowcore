cmake_minimum_required(VERSION 3.22)

project(SymbolTable
    VERSION 1.1.0
    DESCRIPTION "Policy-free symbol table for language development"
    LANGUAGES CXX
)

option(SYMBOLTABLE_BUILD_TESTS "Build SymbolTable tests" ON)
option(SYMBOLTABLE_BUILD_SHARED "Build shared SymbolTable library" ON)
option(SYMBOLTABLE_BUILD_STATIC "Build static SymbolTable library" ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(SYMBOLTABLE_PUBLIC_HEADERS
    include/symboltable/DefinitionState.hpp
    include/symboltable/Factoid.hpp
    include/symboltable/IdTypes.hpp
    include/symboltable/LookupDecision.hpp
    include/symboltable/Scope.hpp
    include/symboltable/ScopeKind.hpp
    include/symboltable/SourceLocation.hpp
    include/symboltable/Symbol.hpp
    include/symboltable/SymbolKind.hpp
    include/symboltable/SymbolPolicy.hpp
    include/symboltable/SymbolTable.hpp
    include/symboltable/SymbolTreeView.hpp
    include/symboltable/Visibility.hpp
)

set(SYMBOLTABLE_SOURCES
    src/SymbolTable.cpp
    src/SymbolPolicy.cpp
    src/SymbolTreeView.cpp
)

if(SYMBOLTABLE_BUILD_STATIC)
    add_library(symboltable_static STATIC ${SYMBOLTABLE_SOURCES} ${SYMBOLTABLE_PUBLIC_HEADERS})
    add_library(SymbolTable::static ALIAS symboltable_static)
    target_include_directories(symboltable_static PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    )
    target_compile_options(symboltable_static PRIVATE -Wall -Wextra -Wpedantic)
    set_target_properties(symboltable_static PROPERTIES OUTPUT_NAME symboltable)
endif()

if(SYMBOLTABLE_BUILD_SHARED)
    add_library(symboltable_shared SHARED ${SYMBOLTABLE_SOURCES} ${SYMBOLTABLE_PUBLIC_HEADERS})
    add_library(SymbolTable::shared ALIAS symboltable_shared)
    target_include_directories(symboltable_shared PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    )
    target_compile_options(symboltable_shared PRIVATE -Wall -Wextra -Wpedantic)
    set_target_properties(symboltable_shared PROPERTIES OUTPUT_NAME symboltable)
endif()

add_library(SymbolTable::headers INTERFACE IMPORTED GLOBAL)
set_target_properties(SymbolTable::headers PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

if(SYMBOLTABLE_BUILD_TESTS)
    enable_testing()
    add_executable(symboltable_tests tests/test_symboltable.cpp)
    target_include_directories(symboltable_tests PRIVATE include)
    target_compile_options(symboltable_tests PRIVATE -Wall -Wextra -Wpedantic)

    if(SYMBOLTABLE_BUILD_STATIC)
        target_link_libraries(symboltable_tests PRIVATE SymbolTable::static)
    elseif(SYMBOLTABLE_BUILD_SHARED)
        target_link_libraries(symboltable_tests PRIVATE SymbolTable::shared)
    else()
        target_sources(symboltable_tests PRIVATE ${SYMBOLTABLE_SOURCES})
    endif()

    add_test(NAME symboltable_tests COMMAND symboltable_tests)
endif()
