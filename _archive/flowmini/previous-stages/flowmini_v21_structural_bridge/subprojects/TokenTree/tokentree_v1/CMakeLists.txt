cmake_minimum_required(VERSION 3.22)
project(TokenTree VERSION 0.1.0 LANGUAGES CXX)

include(CTest)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(TokenTree_objects OBJECT src/tokentree.cpp)
target_compile_definitions(TokenTree_objects PRIVATE TOKENTREE_BUILDING_SHARED)
target_include_directories(TokenTree_objects
        PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
        "$<INSTALL_INTERFACE:include>")
set_target_properties(TokenTree_objects PROPERTIES
        POSITION_INDEPENDENT_CODE ON
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN YES)

add_library(TokenTree SHARED $<TARGET_OBJECTS:TokenTree_objects>)
target_include_directories(TokenTree
        PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
        "$<INSTALL_INTERFACE:include>")
set_target_properties(TokenTree PROPERTIES
        OUTPUT_NAME TokenTree
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR})

add_library(TokenTree_static STATIC $<TARGET_OBJECTS:TokenTree_objects>)
target_include_directories(TokenTree_static
        PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
        "$<INSTALL_INTERFACE:include>")
set_target_properties(TokenTree_static PROPERTIES
        OUTPUT_NAME TokenTree)

install(TARGETS TokenTree TokenTree_static
        EXPORT TokenTreeTargets
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin)
install(FILES include/tokentree.h DESTINATION include/TokenTree)

if(BUILD_TESTING)
        add_executable(TokenTree_smoke_tests
                tests/test_main.cpp
                tests/smoke_tests.cpp)
        target_link_libraries(TokenTree_smoke_tests PRIVATE TokenTree_static)
        target_include_directories(TokenTree_smoke_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
        add_test(NAME TokenTree_smoke_tests COMMAND TokenTree_smoke_tests)
endif()
