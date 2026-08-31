add_library(flowterminal SHARED src/flow_terminal.c)
add_library(Flowcore::terminal ALIAS flowterminal)

target_include_directories(flowterminal
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
set_target_properties(flowterminal PROPERTIES
    OUTPUT_NAME flowterminal
    VERSION 1.0.0
    SOVERSION 1)

add_executable(flowterminal_provider_test tests/provider_test.c)
target_link_libraries(flowterminal_provider_test PRIVATE flowterminal util)
add_test(NAME flowterminal_provider COMMAND flowterminal_provider_test)

install(TARGETS flowterminal
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES include/flow_terminal.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/flowcore)
