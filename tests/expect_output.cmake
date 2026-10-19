execute_process(
    COMMAND "\${NIX_EXECUTABLE}" "\${SCRIPT_PATH}"
    INPUT_FILE "\${INPUT_PATH}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE actual
    ERROR_VARIABLE error
    TIMEOUT 10
)
if(NOT "\${result}" STREQUAL "0")
    message(FATAL_ERROR "NixScript program failed (exit=\${result}). stdout=\${actual}; stderr=\${error}")
endif()
file(READ "\${EXPECTED_PATH}" expected)
if(NOT "\${actual}" STREQUAL "\${expected}")
    message(FATAL_ERROR "Unexpected output.\nExpected: [\${expected}]\nActual: [\${actual}]\nStderr: \${error}")
endif()
