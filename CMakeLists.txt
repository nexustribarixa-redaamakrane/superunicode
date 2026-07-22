cmake_minimum_required(VERSION 3.10)
project(SuperUnicodeWorkspace C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Enable testing at root before adding subdirectories
enable_testing()

# Sub-projects
add_subdirectory(superunicode)
add_subdirectory(sutf)
add_subdirectory(superunicode_extended)
