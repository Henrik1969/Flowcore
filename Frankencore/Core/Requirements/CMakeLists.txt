add_library(frankencore_core_requirements STATIC requirements.cpp)
target_include_directories(frankencore_core_requirements
    PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)
target_compile_features(frankencore_core_requirements PUBLIC cxx_std_20)
target_compile_options(frankencore_core_requirements PRIVATE -Wall -Wextra -Wpedantic)

add_library(Frankencore::Requirements ALIAS frankencore_core_requirements)
