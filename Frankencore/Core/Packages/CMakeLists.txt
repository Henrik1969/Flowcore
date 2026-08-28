add_library(frankencore_core_packages STATIC packages.cpp)
target_include_directories(frankencore_core_packages
    PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)
target_compile_features(frankencore_core_packages PUBLIC cxx_std_20)
target_compile_options(frankencore_core_packages PRIVATE -Wall -Wextra -Wpedantic)

add_library(Frankencore::Packages ALIAS frankencore_core_packages)
