add_executable(frankencore_packages_probe main.cpp)
target_link_libraries(frankencore_packages_probe PRIVATE Frankencore::Packages)
target_compile_features(frankencore_packages_probe PRIVATE cxx_std_20)
target_compile_options(frankencore_packages_probe PRIVATE -Wall -Wextra -Wpedantic)

add_test(NAME frankencore_packages_probe
    COMMAND frankencore_packages_probe)
