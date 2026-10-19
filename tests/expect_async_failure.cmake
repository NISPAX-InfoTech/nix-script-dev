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
if(DEFINED EXPECTED_DIAGNOSTIC)
    set(expected_diagnostic "${EXPECTED_DIAGNOSTIC}")
else()
    set(expected_diagnostic "Unknown variable 'missing'")
endif()
if(NOT combined MATCHES "${expected_diagnostic}")
    message(FATAL_ERROR "Async failure did not contain expected diagnostic [${expected_diagnostic}]. Output: ${combined}")
endif()
if(DEFINED EXPECT_EMPTY_OUTPUT AND EXPECT_EMPTY_OUTPUT AND NOT "${output}" STREQUAL "")
    message(FATAL_ERROR "Unsafe synchronous call produced output before rejection: [${output}]")
endif()
