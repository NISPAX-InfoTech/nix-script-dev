execute_process(
    COMMAND "${NIX_EXECUTABLE}" "${SCRIPT_PATH}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE error
)
if("${result}" STREQUAL "0")
    message(FATAL_ERROR "Expected awaited async failure, but the program succeeded. Output: ${output}")
endif()
set(combined "${output}\n${error}")
if(NOT combined MATCHES "Unknown variable 'missing'")
    message(FATAL_ERROR "Async failure was not propagated as expected. Output: ${combined}")
endif()
