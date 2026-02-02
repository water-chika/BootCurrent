cmake_minimum_required(VERSION 3.20)
project(uefi_boot_next)

add_executable(uefi_boot_next main.cpp)

target_compile_features(uefi_boot_next PUBLIC cxx_std_20)

target_link_libraries(uefi_boot_next PUBLIC win32_helper)
