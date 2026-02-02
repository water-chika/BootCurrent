cmake_minimum_required(VERSION 3.20)
project(bootcurrent)

add_executable(bootcurrent main.cpp)

target_compile_features(bootcurrent PUBLIC cxx_std_20)
