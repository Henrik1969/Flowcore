cmake_minimum_required(VERSION 3.22)
project(flowbind LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(flowbind src/main.cpp)
target_compile_options(flowbind PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(flowbind PRIVATE flowcontracts ${CMAKE_DL_LIBS})

enable_testing()
add_test(NAME flowbind_provider
    COMMAND ${CMAKE_COMMAND} -E env
        FLOWBIND_BIN=$<TARGET_FILE:flowbind>
        FLOWANALYST_BIN=$<TARGET_FILE:flowanalyst>
        FLOWMINI_BIN=$<TARGET_FILE:flowmini>
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/run-flowbind-tests.sh)
add_test(NAME flowbind_fuzz
    COMMAND ${CMAKE_COMMAND} -E env
        FLOWBIND_BIN=$<TARGET_FILE:flowbind>
        ${CMAKE_CURRENT_SOURCE_DIR}/../tools/run-flowbind-fuzz.sh)
