cmake_minimum_required(VERSION 3.22.1)
project(nixscript_android LANGUAGES CXX)

add_library(nixscript SHARED jni_bridge.cpp)
target_compile_features(nixscript PRIVATE cxx_std_17)
target_compile_definitions(nixscript PRIVATE NIX_ANDROID=1)
