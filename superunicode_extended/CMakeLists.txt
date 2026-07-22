cmake_minimum_required(VERSION 3.10)
project(superunicode_extended C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Freestanding bare-metal flags — zero CRT dependencies
add_compile_options(-nostdlib -ffreestanding -Wall -Wextra -O2)

# Include directory
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# ============================================================================
# Static Library: libsuperunicode_extended
# ============================================================================
add_library(superunicode_extended_static STATIC
    src/extsutf_fixed.c
    src/vsutf.c
    src/esutf.c
)

set_target_properties(superunicode_extended_static PROPERTIES
    OUTPUT_NAME superunicode_extended
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin
)

# ============================================================================
# Test Targets (linked against host CRT for printf/assert)
# These are NOT part of the freestanding library — tests only.
# ============================================================================
add_executable(test_extsutf_all tests/test_extsutf_all.c)
target_link_libraries(test_extsutf_all superunicode_extended_static)
# Tests use host CRT (printf, assert) — remove freestanding for test targets
target_compile_options(test_extsutf_all PRIVATE -Wno-unused-parameter)
set_target_properties(test_extsutf_all PROPERTIES
    LINK_FLAGS ""
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin
)

enable_testing()
add_test(NAME test_extsutf_all COMMAND test_extsutf_all)
