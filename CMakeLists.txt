cmake_minimum_required(VERSION 3.16)
project(nix_script LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(nix src/main.cpp)

enable_testing()
add_executable(async_runtime_test tests/async_runtime_test.cpp)
target_include_directories(async_runtime_test PRIVATE src)
add_test(NAME async_runtime_test COMMAND async_runtime_test)
