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
    set_tests_properties(async_language_test PROPERTIES PASS_REGULAR_EXPRESSION "s1s2e1e2answer=42")
    add_test(NAME async_failure_test
        COMMAND ${CMAKE_COMMAND}
            "-DNIX_EXECUTABLE=$<TARGET_FILE:nix>"
            "-DSCRIPT_PATH=${CMAKE_CURRENT_SOURCE_DIR}/tests/async_failure.ns"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/expect_async_failure.cmake")
endif()
