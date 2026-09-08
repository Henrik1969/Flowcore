cmake_minimum_required(VERSION 3.22)
project(flowwc LANGUAGES NONE)

enable_testing()
add_test(NAME flowwc_flowmini
    COMMAND ${CMAKE_COMMAND} -E env
        FLOWMINI_BIN=$<TARGET_FILE:flowmini>
        FLOWANALYST_BIN=$<TARGET_FILE:flowanalyst>
        FLOWWC_SOURCE=${CMAKE_CURRENT_SOURCE_DIR}/src/flowwc.flow
        FLOWWC_STDIN_SOURCE=${CMAKE_CURRENT_SOURCE_DIR}/src/flowwc_stdin.flow
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/run-flowwc-tests.sh)
