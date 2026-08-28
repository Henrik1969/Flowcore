add_executable(frankencore_clock main.cpp)
target_compile_options(frankencore_clock PRIVATE -Wall -Wextra -Wpedantic)

enable_testing()
add_test(NAME frankencore_clock_monotonic
    COMMAND frankencore_clock --clock monotonic)
add_test(NAME frankencore_clock_realtime
    COMMAND frankencore_clock --clock realtime)
add_test(NAME frankencore_clock_contract
    COMMAND ${CMAKE_COMMAND} -E env
        CLOCK_BIN=$<TARGET_FILE:frankencore_clock>
        ${CMAKE_CURRENT_LIST_DIR}/../../../tools/check-frankencore-clock.sh)
add_test(NAME frankencore_clock_rejects_unknown_clock
    COMMAND frankencore_clock --clock unknown)
set_tests_properties(frankencore_clock_rejects_unknown_clock PROPERTIES WILL_FAIL TRUE)
