add_executable(frankencore_contracts_probe main.cpp)
target_link_libraries(frankencore_contracts_probe PRIVATE Frankencore::Contracts)
target_compile_features(frankencore_contracts_probe PRIVATE cxx_std_20)
target_compile_options(frankencore_contracts_probe PRIVATE -Wall -Wextra -Wpedantic)

add_test(NAME frankencore_contracts_probe
    COMMAND frankencore_contracts_probe)
