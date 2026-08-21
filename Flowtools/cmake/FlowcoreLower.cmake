execute_process(COMMAND "${FLOWCORE_FLOWLOWER}"
    --emit-llvm "${FLOWCORE_OUTPUT_DIR}/program.ll"
    --binding-report "${FLOWCORE_BINDING_REPORT}"
    "${FLOWCORE_OPTIMIZATION_REPORT}"
    RESULT_VARIABLE rc)
if(rc)
    message(FATAL_ERROR "Flowlower rejected the optimization/binding reports (${rc})")
endif()
execute_process(COMMAND "${FLOWCORE_CLANG}" "${FLOWCORE_OUTPUT_DIR}/program.ll"
    -o "${FLOWCORE_OUTPUT_DIR}/flowcore_program" RESULT_VARIABLE rc)
if(rc)
    message(FATAL_ERROR "clang failed to produce the Flowcore artifact (${rc})")
endif()
