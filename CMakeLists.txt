cmake_minimum_required(VERSION 3.10)
project(SuperUnicodeWorkspace C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Enable testing at root before adding subdirectories
enable_testing()

# Export compile commands so IDEs/language servers (clangd) can resolve the
# project include paths for accurate diagnostics.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ── Specification & Architecture Include Paths ────────────────────
# These paths expose the SUAS/SUTS headers for IDE resolution and
# future conformance tests. They do NOT add build targets — the
# headers are spec-only and carry no compiled artefacts.

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# ── Documentation Verification ────────────────────────────────────
# Lightweight custom target that verifies the doc scaffold integrity.
# Runs via `cmake --build . --target verify-docs` but does NOT compile
# any source files. Uses its own CMake script for cross-platform
# file-presence checks.

add_custom_target(verify-docs
    COMMENT "Verifying documentation scaffold..."
    COMMAND ${CMAKE_COMMAND}
        -DDOCS_ROOT_DIR=${CMAKE_CURRENT_SOURCE_DIR}/docs
        -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/verify_docs.cmake
)

# ── SUAS Architecture Standard (SUAS-001 SDF) ──────────────────
# Freestanding static library implementing Structural Directional Framing
# and the full Unicode Bidirectional Algorithm (UAX #9) processing model.
add_library(suas_static STATIC
    src/suas/suas_sdf.c
    src/suas/suas_bidi.c
)

target_include_directories(suas_static PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
if(MSVC)
    target_compile_options(suas_static PRIVATE /X)
else()
    target_compile_options(suas_static PRIVATE -std=c99 -nostdlib -ffreestanding)
endif()

# SUAS host unit tests
add_executable(test_suas_sdf tests/test_suas_sdf.c)
target_link_libraries(test_suas_sdf PRIVATE suas_static)
add_test(NAME test_suas_sdf COMMAND test_suas_sdf)

# ── Sub-projects ──────────────────────────────────────────────────
add_subdirectory(superunicode)
add_subdirectory(sutf)
add_subdirectory(superunicode_extended)
add_subdirectory(sust)
add_subdirectory(suf)

# Unified header-coexistence test (all module headers in one TU)
add_subdirectory(unified)

