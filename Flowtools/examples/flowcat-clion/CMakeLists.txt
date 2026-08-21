cmake_minimum_required(VERSION 3.20)
project(flowcat_flowcore LANGUAGES NONE)

include("${CMAKE_CURRENT_LIST_DIR}/../../cmake/FlowcoreProject.cmake")
flowcore_project_tools()

set(FLOWCORE_SOURCE "${CMAKE_CURRENT_LIST_DIR}/../../../Flowmini/flowmini_v25_symboltable_projection/examples/apps/flowcat/flowcat.flow" CACHE FILEPATH "Flow source file")
set(FLOWCORE_POLICY "${CMAKE_CURRENT_LIST_DIR}/../../../Flowmini/flowmini_v25_symboltable_projection/examples/apps/flowcat/policy.conf" CACHE FILEPATH "Flowbind policy")
add_custom_target(flowcore_sources SOURCES "${FLOWCORE_SOURCE}" "${FLOWCORE_POLICY}")
flowcore_pipeline(flowcat "${FLOWCORE_SOURCE}" "${FLOWCORE_POLICY}")

add_custom_target(flowcat_run
    COMMAND "${CMAKE_CURRENT_BINARY_DIR}/flowcore/flowcat/flowcore_program"
            "${CMAKE_CURRENT_LIST_DIR}/sample-alpha.txt"
            "${CMAKE_CURRENT_LIST_DIR}/sample-beta.txt"
    DEPENDS flowcore_flowcat_lower
    USES_TERMINAL)

enable_testing()
add_test(NAME flowcat_flowcore_pipeline
    COMMAND "${CMAKE_COMMAND}" --build "${CMAKE_BINARY_DIR}" --target flowcore_flowcat_lower)
