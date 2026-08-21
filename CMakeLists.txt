cmake_minimum_required(VERSION 3.22)
project(flowcore LANGUAGES C CXX)

include(CTest)
include(GNUInstallDirs)

# This is an additive superbuild. Each sibling keeps its own CMake project,
# tests, and artifact contract; the root only supplies one clean build graph.
add_subdirectory(Flowmini/flowmini_v25_symboltable_projection
    ${CMAKE_CURRENT_BINARY_DIR}/flowmini)
add_subdirectory(Flowanalyst ${CMAKE_CURRENT_BINARY_DIR}/flowanalyst)
add_subdirectory(Flowbind ${CMAKE_CURRENT_BINARY_DIR}/flowbind)
add_subdirectory(Flowoptimize ${CMAKE_CURRENT_BINARY_DIR}/flowoptimize)

# Flowtools owns the Frankencore reference providers and the Flowparallel
# subproject. Including it here avoids duplicating that target graph.
add_subdirectory(Flowtools ${CMAKE_CURRENT_BINARY_DIR}/flowtools)
add_subdirectory(Flowlower ${CMAKE_CURRENT_BINARY_DIR}/flowlower)
add_subdirectory(Flowkernel ${CMAKE_CURRENT_BINARY_DIR}/flowkernel)

add_test(NAME flowcore_integration_corpus
    COMMAND ${CMAKE_COMMAND} -E env
        FLOWMINI_BIN=$<TARGET_FILE:flowmini>
        FLOWANALYST_BIN=$<TARGET_FILE:flowanalyst>
        FLOWPARALLEL_BIN=$<TARGET_FILE:flowparallel>
        FLOWOPTIMIZE_BIN=$<TARGET_FILE:flowoptimize>
        ${CMAKE_CURRENT_SOURCE_DIR}/tools/run-flowcore-integration-corpus.sh)
set_tests_properties(flowcore_integration_corpus PROPERTIES
    DEPENDS "flowanalyst_pipeline")

add_test(NAME flowcore_pipeline_matrix
    COMMAND ${CMAKE_COMMAND} -E env
        FLOWCORE_ROOT=${CMAKE_CURRENT_SOURCE_DIR}
        FLOWMINI_BIN=$<TARGET_FILE:flowmini>
        FLOWANALYST_BIN=$<TARGET_FILE:flowanalyst>
        FLOWPARALLEL_BIN=$<TARGET_FILE:flowparallel>
        FLOWOPTIMIZE_BIN=$<TARGET_FILE:flowoptimize>
        FLOWBIND_BIN=$<TARGET_FILE:flowbind>
        FLOWLOWER_BIN=$<TARGET_FILE:flowlower>
        ${CMAKE_CURRENT_SOURCE_DIR}/tools/run-flowcore-pipeline-matrix.sh)

add_test(NAME flowcore_pass_corpus
    COMMAND ${CMAKE_COMMAND} -E env
        FLOWCORE_ROOT=${CMAKE_CURRENT_SOURCE_DIR}
        FLOWMINI_BIN=$<TARGET_FILE:flowmini>
        FLOWANALYST_BIN=$<TARGET_FILE:flowanalyst>
        FLOWPARALLEL_BIN=$<TARGET_FILE:flowparallel>
        FLOWOPTIMIZE_BIN=$<TARGET_FILE:flowoptimize>
        FLOWBIND_BIN=$<TARGET_FILE:flowbind>
        FLOWLOWER_BIN=$<TARGET_FILE:flowlower>
        ${CMAKE_CURRENT_SOURCE_DIR}/tools/run-flowcore-pass-corpus.sh)

add_test(NAME flowcore_stdlib_boundary
    COMMAND ${CMAKE_COMMAND} -E env
        FLOWCORE_ROOT=${CMAKE_CURRENT_SOURCE_DIR}
        FLOWMINI_BIN=$<TARGET_FILE:flowmini>
        FLOWANALYST_BIN=$<TARGET_FILE:flowanalyst>
        FLOWBIND_BIN=$<TARGET_FILE:flowbind>
        FLOWTESTABI_LAYOUT_BIN=$<TARGET_FILE:flowmini_testabi_layout>
        ${CMAKE_CURRENT_SOURCE_DIR}/tools/run-flowcore-stdlib-boundary.sh)
set_tests_properties(flowcore_stdlib_boundary PROPERTIES
    DEPENDS "flowbind_provider")

# Canonical installed shape. Debian packaging consumes this staged install and
# splits runtime, development, and documentation payloads without depending on
# the source-tree layout.
install(TARGETS
    flowmini
    flowanalyst
    flowbind
    flowoptimize
    flowlower
    flowkernel
    flowparallel
    franken_ls
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(TARGETS
    flowparallel_cpu_runtime
    TokenTree_static
    symboltable_static
    symboltable_shared
    frankencore_core_contracts
    frankencore_core_language
    frankencore_core_packages
    frankencore_core_provenance
    frankencore_core_requirements
    frankencore_core_runtime
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/flowcore
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/flowcore
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY
    ${CMAKE_CURRENT_SOURCE_DIR}/Flowmini/flowmini_v25_symboltable_projection/include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/flowcore/flowmini
    FILES_MATCHING PATTERN "*.h"
)

install(DIRECTORY
    ${CMAKE_CURRENT_SOURCE_DIR}/Flowparallel/include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/flowcore
    FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h"
)

install(DIRECTORY
    ${CMAKE_CURRENT_SOURCE_DIR}/Flowmini/flowmini_v25_symboltable_projection/subprojects/SymbolTable/SymbolTable_v1_1/include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/flowcore
    FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h"
)

install(DIRECTORY
    ${CMAKE_CURRENT_SOURCE_DIR}/Frankencore/Core/Contracts/include/
    ${CMAKE_CURRENT_SOURCE_DIR}/Frankencore/Core/Language/include/
    ${CMAKE_CURRENT_SOURCE_DIR}/Frankencore/Core/Packages/include/
    ${CMAKE_CURRENT_SOURCE_DIR}/Frankencore/Core/Policy/include/
    ${CMAKE_CURRENT_SOURCE_DIR}/Frankencore/Core/Provenance/include/
    ${CMAKE_CURRENT_SOURCE_DIR}/Frankencore/Core/Requirements/include/
    ${CMAKE_CURRENT_SOURCE_DIR}/Frankencore/Core/Runtime/include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/flowcore
    FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h"
)

install(DIRECTORY
    ${CMAKE_CURRENT_SOURCE_DIR}/Flowmini/flowmini_v25_symboltable_projection/std/
    DESTINATION ${CMAKE_INSTALL_DATADIR}/flowcore/std
    FILES_MATCHING PATTERN "*.flow"
)

install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/Flowmini/flowmini_v25_symboltable_projection/examples/apps/flowcat/policy.conf
    DESTINATION ${CMAKE_INSTALL_DATADIR}/flowcore/examples/flowcat
)

install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/README.md
    ${CMAKE_CURRENT_SOURCE_DIR}/CHANGELOG.md
    ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.md
    ${CMAKE_CURRENT_SOURCE_DIR}/docs/flowmini/stdlib-capability-matrix.md
    DESTINATION ${CMAKE_INSTALL_DOCDIR}
)
