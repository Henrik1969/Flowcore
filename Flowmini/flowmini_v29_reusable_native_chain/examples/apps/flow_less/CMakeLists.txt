# Application-owned transport providers. No compiler tool links these libraries.
add_library(flowpager_input SHARED providers/input.cpp)
add_library(flowpager_ncurses_input SHARED providers/input.cpp)
target_compile_definitions(flowpager_ncurses_input PRIVATE FLOW_PAGER_TERMINAL=1)
target_link_libraries(flowpager_ncurses_input PRIVATE ${CMAKE_DL_LIBS})
add_library(flowpager_output SHARED providers/output.cpp)
install(TARGETS flowpager_input flowpager_ncurses_input flowpager_output
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/flowcore/providers)
install(FILES flow_less.flow flow_less_ncurses.flow README.md expected-stdout.txt
    DESTINATION ${CMAKE_INSTALL_DATADIR}/flowcore/examples/flow_less)
install(PROGRAMS build-flow-less.sh run-flow-less.sh run-flow-less-ncurses.sh
    DESTINATION ${CMAKE_INSTALL_DATADIR}/flowcore/examples/flow_less)
