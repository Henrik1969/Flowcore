cmake_minimum_required(VERSION 3.20)
file(MAKE_DIRECTORY "${FLOWCORE_OUTPUT_DIR}")

execute_process(COMMAND "${FLOWCORE_FLOWMINI}" --dump-frontend-bundle "${FLOWCORE_SOURCE}"
    OUTPUT_FILE "${FLOWCORE_OUTPUT_DIR}/frontend-bundle.json" RESULT_VARIABLE rc)
if(rc)
    message(FATAL_ERROR "FlowMini failed (${rc})")
endif()
execute_process(COMMAND "${FLOWCORE_FLOWANALYST}"
    INPUT_FILE "${FLOWCORE_OUTPUT_DIR}/frontend-bundle.json"
    OUTPUT_FILE "${FLOWCORE_OUTPUT_DIR}/semantic-report.json" RESULT_VARIABLE rc)
if(rc)
    message(FATAL_ERROR "Flowanalyst rejected the source (${rc})")
endif()
execute_process(COMMAND "${FLOWCORE_FLOWBIND}" --policy "${FLOWCORE_POLICY}"
    INPUT_FILE "${FLOWCORE_OUTPUT_DIR}/semantic-report.json"
    OUTPUT_FILE "${FLOWCORE_OUTPUT_DIR}/binding-report.json" RESULT_VARIABLE rc)
if(rc)
    message(FATAL_ERROR "Flowbind rejected the capability policy (${rc})")
endif()
execute_process(COMMAND "${FLOWCORE_FLOWOPTIMIZE}"
    INPUT_FILE "${FLOWCORE_OUTPUT_DIR}/semantic-report.json"
    OUTPUT_FILE "${FLOWCORE_OUTPUT_DIR}/optimization-report.json" RESULT_VARIABLE rc)
if(rc)
    message(FATAL_ERROR "Flowoptimize rejected the semantic report (${rc})")
endif()
