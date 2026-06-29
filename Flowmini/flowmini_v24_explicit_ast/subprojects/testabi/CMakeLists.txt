add_library(flowmini_testabi SHARED testabi.c)
set_target_properties(flowmini_testabi PROPERTIES
    OUTPUT_NAME "flowmini_testabi"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)
