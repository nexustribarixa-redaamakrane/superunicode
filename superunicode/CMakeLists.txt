cmake_minimum_required(VERSION 3.10)
project(superunicode C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Public Include Directory
include_directories(include)

# Static Library Target (Freestanding Core)
add_library(superunicode_static STATIC
    src/sutf_encode.c
    src/sutf_decode.c
    src/sucs_string.c
)

# Apply freestanding flags to core library
if(MSVC)
    target_compile_options(superunicode_static PRIVATE /X)
else()
    target_compile_options(superunicode_static PRIVATE -ffreestanding)
endif()

# Subdirectories
enable_testing()
add_subdirectory(tests)

# Inspector CLI tool target
add_executable(sucs_inspector tools/sucs_inspector.c)
target_link_libraries(sucs_inspector PRIVATE superunicode_static)
