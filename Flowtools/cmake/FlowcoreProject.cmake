# Flowcore CMake/Ninja integration for IDE projects.
# Include this file from a project-specific CMakeLists.txt.

set(FLOWCORE_CMAKE_DIR "${CMAKE_CURRENT_LIST_DIR}" CACHE INTERNAL "Flowcore CMake helper directory")

set(FLOWCORE_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." CACHE PATH "Flowcore source root")
set(FLOWCORE_FLOWMINI "${FLOWCORE_ROOT}/Flowmini/flowmini_v25_symboltable_projection/cmake-build-debug/flowmini" CACHE FILEPATH "FlowMini executable")
set(FLOWCORE_FLOWANALYST "${FLOWCORE_ROOT}/Flowanalyst/build/flowanalyst" CACHE FILEPATH "Flowanalyst executable")
set(FLOWCORE_FLOWBIND "${FLOWCORE_ROOT}/Flowbind/build/flowbind" CACHE FILEPATH "Flowbind executable")
set(FLOWCORE_FLOWOPTIMIZE "${FLOWCORE_ROOT}/Flowoptimize/build/flowoptimize" CACHE FILEPATH "Flowoptimize executable")
set(FLOWCORE_FLOWLOWER "${FLOWCORE_ROOT}/Flowlower/build/flowlower" CACHE FILEPATH "Flowlower executable")
set(FLOWCORE_CLANG "clang" CACHE STRING "C compiler used for Flowcore lowered artifacts")

function(flowcore_project_tools)
    foreach(tool IN ITEMS FLOWCORE_FLOWMINI FLOWCORE_FLOWANALYST FLOWCORE_FLOWBIND FLOWCORE_FLOWOPTIMIZE FLOWCORE_FLOWLOWER)
        if(NOT EXISTS "${${tool}}")
            message(WARNING "${tool} does not exist yet: ${${tool}}. Build the Flowcore stages or override it in CLion's CMake cache.")
        endif()
    endforeach()
endfunction()

function(flowcore_pipeline NAME SOURCE POLICY)
    set(output_dir "${CMAKE_CURRENT_BINARY_DIR}/flowcore/${NAME}")
    add_custom_target("flowcore_${NAME}_analyze"
        COMMAND "${CMAKE_COMMAND}"
            "-DFLOWCORE_ROOT=${FLOWCORE_ROOT}"
            "-DFLOWCORE_FLOWMINI=${FLOWCORE_FLOWMINI}"
            "-DFLOWCORE_FLOWANALYST=${FLOWCORE_FLOWANALYST}"
            "-DFLOWCORE_FLOWBIND=${FLOWCORE_FLOWBIND}"
            "-DFLOWCORE_FLOWOPTIMIZE=${FLOWCORE_FLOWOPTIMIZE}"
            "-DFLOWCORE_SOURCE=${SOURCE}"
            "-DFLOWCORE_POLICY=${POLICY}"
            "-DFLOWCORE_OUTPUT_DIR=${output_dir}"
            -P "${FLOWCORE_CMAKE_DIR}/FlowcorePipeline.cmake"
        BYPRODUCTS
            "${output_dir}/frontend-bundle.json"
            "${output_dir}/semantic-report.json"
            "${output_dir}/binding-report.json"
            "${output_dir}/optimization-report.json"
        COMMENT "Flowcore analyze ${NAME}"
        VERBATIM)

    add_custom_target("flowcore_${NAME}_lower"
        COMMAND "${CMAKE_COMMAND}" --build "${CMAKE_BINARY_DIR}" --target "flowcore_${NAME}_analyze"
        COMMAND "${CMAKE_COMMAND}"
            "-DFLOWCORE_FLOWLOWER=${FLOWCORE_FLOWLOWER}"
            "-DFLOWCORE_CLANG=${FLOWCORE_CLANG}"
            "-DFLOWCORE_OPTIMIZATION_REPORT=${output_dir}/optimization-report.json"
            "-DFLOWCORE_BINDING_REPORT=${output_dir}/binding-report.json"
            "-DFLOWCORE_OUTPUT_DIR=${output_dir}"
            -P "${FLOWCORE_CMAKE_DIR}/FlowcoreLower.cmake"
        BYPRODUCTS "${output_dir}/${NAME}"
        DEPENDS "flowcore_${NAME}_analyze"
        COMMENT "Flowcore lower ${NAME}"
        VERBATIM)
endfunction()
