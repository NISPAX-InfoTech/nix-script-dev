cmake_minimum_required(VERSION 3.16)
project(nix_script LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(nix src/main.cpp)

if(NOT ANDROID AND NOT NIX_ANDROID)
    enable_testing()
    add_executable(async_runtime_test tests/async_runtime_test.cpp)
    target_include_directories(async_runtime_test PRIVATE src)
    add_test(NAME async_runtime_test COMMAND async_runtime_test)
    add_test(NAME async_language_test
        COMMAND $<TARGET_FILE:nix> ${CMAKE_CURRENT_SOURCE_DIR}/tests/async_language.ns)
    # Require both the existing async output and the nested-await visit-count
    # assertion; the character class allows the match to cross printed lines.
    set_tests_properties(async_language_test PROPERTIES PASS_REGULAR_EXPRESSION "s1s2e1e2answer=42(.|[[:space:]])*nested-result=125")
    add_test(NAME async_method_test
        COMMAND ${CMAKE_COMMAND}
            "-DNIX_EXECUTABLE=$<TARGET_FILE:nix>"
            "-DSCRIPT_PATH=${CMAKE_CURRENT_SOURCE_DIR}/tests/async_method.ns"
            "-DINPUT_PATH=${CMAKE_CURRENT_SOURCE_DIR}/tests/async_method.input"
            "-DEXPECTED_PATH=${CMAKE_CURRENT_SOURCE_DIR}/tests/async_method.expected"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/expect_output.cmake")
    set_tests_properties(async_method_test PROPERTIES TIMEOUT 10)
    add_test(NAME async_sync_await_guard_test
        COMMAND ${CMAKE_COMMAND}
            "-DNIX_EXECUTABLE=$<TARGET_FILE:nix>"
            "-DSCRIPT_PATH=${CMAKE_CURRENT_SOURCE_DIR}/tests/async_sync_await_guard.ns"
            "-DEXPECTED_DIAGNOSTIC=Synchronous function 'legacyAsyncCaller' contains await"
            "-DEXPECT_EMPTY_OUTPUT=ON"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/expect_async_failure.cmake")
    set_tests_properties(async_sync_await_guard_test PROPERTIES TIMEOUT 10)
    add_test(NAME async_sync_recursion_test
        COMMAND ${CMAKE_COMMAND}
            "-DNIX_EXECUTABLE=$<TARGET_FILE:nix>"
            "-DSCRIPT_PATH=${CMAKE_CURRENT_SOURCE_DIR}/tests/async_sync_recursion.ns"
            "-DINPUT_PATH=${CMAKE_CURRENT_SOURCE_DIR}/tests/async_sync_recursion.input"
            "-DEXPECTED_PATH=${CMAKE_CURRENT_SOURCE_DIR}/tests/async_sync_recursion.expected"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/expect_output.cmake")
    set_tests_properties(async_sync_recursion_test PROPERTIES TIMEOUT 10)
    add_test(NAME async_failure_test
        COMMAND ${CMAKE_COMMAND}
            "-DNIX_EXECUTABLE=$<TARGET_FILE:nix>"
            "-DSCRIPT_PATH=${CMAKE_CURRENT_SOURCE_DIR}/tests/async_failure.ns"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/expect_async_failure.cmake")
    add_test(NAME async_unhandled_failure_test
        COMMAND ${CMAKE_COMMAND}
            "-DNIX_EXECUTABLE=$<TARGET_FILE:nix>"
            "-DSCRIPT_PATH=${CMAKE_CURRENT_SOURCE_DIR}/tests/async_unhandled_failure.ns"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/expect_async_failure.cmake")
    add_test(NAME uin_class_field_initialization_test
        COMMAND ${CMAKE_COMMAND}
            "-DNIX_EXECUTABLE=$<TARGET_FILE:nix>"
            "-DSCRIPT_PATH=${CMAKE_CURRENT_SOURCE_DIR}/tests/uin_class_field.ns"
            "-DINPUT_PATH=${CMAKE_CURRENT_SOURCE_DIR}/tests/uin_class_field.input"
            "-DEXPECTED_PATH=${CMAKE_CURRENT_SOURCE_DIR}/tests/uin_class_field.expected"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/expect_output.cmake")
    set_tests_properties(uin_class_field_initialization_test PROPERTIES TIMEOUT 10)
    add_test(NAME global_variables_test
        COMMAND ${CMAKE_COMMAND}
            "-DNIX_EXECUTABLE=$<TARGET_FILE:nix>"
            "-DSCRIPT_PATH=${CMAKE_CURRENT_SOURCE_DIR}/tests/global_variables.ns"
            "-DEXPECTED_PATH=${CMAKE_CURRENT_SOURCE_DIR}/tests/global_variables.expected"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/expect_output.cmake")
    set_tests_properties(global_variables_test PROPERTIES TIMEOUT 10)
endif()
