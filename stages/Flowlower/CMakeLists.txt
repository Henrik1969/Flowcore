add_executable(flowoptimize src/flowoptimize.cpp)
target_compile_features(flowoptimize PRIVATE cxx_std_20)
target_compile_options(flowoptimize PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)

if(BUILD_TESTING AND TARGET flowlex AND TARGET flowparse AND TARGET flowcheck)
        add_test(
                NAME flowoptimize_smoke
                COMMAND
                        "${CMAKE_CURRENT_SOURCE_DIR}/tests/smoke.sh"
                        "$<TARGET_FILE:flowlex>"
                        "$<TARGET_FILE:flowparse>"
                        "$<TARGET_FILE:flowcheck>"
                        "$<TARGET_FILE:flowoptimize>")
endif()
