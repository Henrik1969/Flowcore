add_library(frankencore_core_runtime STATIC runtime.cpp)
target_include_directories(frankencore_core_runtime PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)
target_compile_features(frankencore_core_runtime PUBLIC cxx_std_20)
target_compile_options(frankencore_core_runtime PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(frankencore_core_runtime PUBLIC ${CMAKE_DL_LIBS})
add_library(Frankencore::Runtime ALIAS frankencore_core_runtime)

add_executable(frankencore_runtime_probe probe.cpp)
target_link_libraries(frankencore_runtime_probe PRIVATE Frankencore::Runtime)
target_compile_features(frankencore_runtime_probe PRIVATE cxx_std_20)
target_compile_options(frankencore_runtime_probe PRIVATE -Wall -Wextra -Wpedantic)

add_test(NAME frankencore_runtime_probe
    COMMAND ${CMAKE_COMMAND} -E env
        RUNTIME_BIN=$<TARGET_FILE:frankencore_runtime_probe>
        ${CMAKE_CURRENT_LIST_DIR}/test-runtime.sh)
