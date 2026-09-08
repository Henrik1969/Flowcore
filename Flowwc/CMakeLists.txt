cmake_minimum_required(VERSION 3.22)
project(flowwc LANGUAGES NONE)

enable_testing()
add_test(NAME flowwc_flowmini
    COMMAND ${CMAKE_COMMAND} -E env
        FLOWMINI_BIN=$<TARGET_FILE:flowmini>
        FLOWWC_SOURCE=${CMAKE_CURRENT_SOURCE_DIR}/src/flowwc.flow
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/run-flowwc-tests.sh)
