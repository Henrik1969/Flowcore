add_library(frankencore_core_language STATIC language.cpp)
target_include_directories(frankencore_core_language
    PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)
target_link_libraries(frankencore_core_language PUBLIC Frankencore::Contracts)
target_compile_features(frankencore_core_language PUBLIC cxx_std_20)
target_compile_options(frankencore_core_language PRIVATE -Wall -Wextra -Wpedantic)

add_library(Frankencore::Language ALIAS frankencore_core_language)
