cmake_minimum_required(VERSION 3.22.1)
project(nixscript_android LANGUAGES CXX)

add_library(nixscript SHARED jni_bridge.cpp)
target_compile_features(nixscript PRIVATE cxx_std_17)
