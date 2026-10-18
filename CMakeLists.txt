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

# ── SUAS Architecture Standards (SDF, SGW, SBR, SUCF, SSP, STS, SIS) ──
# Freestanding static library implementing Structural Directional Framing
# and the full Unicode Bidirectional Algorithm (UAX #9) processing model,
# System Glyph Width & Monospace Grid (SGW) fixed-cell terminal metrics,
# System Boundary & Line Break Rules (SBR) single-pass line breaking,
# SuperUnicode Canonical Forms (SUCF) single-pass canonical transforms,
# the System Script Property (SSP) range-dispatched script resolver,
# System Text Segmentation (STS) single-pass grapheme/word/sentence
# boundaries (UAX #29 analog), and System Identifiers and Syntax (SIS)
# single-pass identifier classes and lexical syntax (UAX #31 analog).
add_library(suas_static STATIC
    src/suas/suas_sdf.c
    src/suas/suas_sgw.c
    src/suas/suas_sbr.c
    src/suas/suas_sucf.c
    src/suas/suas_ssp.c
    src/suas/suas_sts.c
    src/suas/suas_sis.c
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

add_executable(test_suas_sgw tests/test_suas_sgw.c)
target_link_libraries(test_suas_sgw PRIVATE suas_static)
add_test(NAME test_suas_sgw COMMAND test_suas_sgw)

add_executable(test_suas_sbr tests/test_suas_sbr.c)
target_link_libraries(test_suas_sbr PRIVATE suas_static)
add_test(NAME test_suas_sbr COMMAND test_suas_sbr)

add_executable(test_suas_sucf tests/test_suas_sucf.c)
target_link_libraries(test_suas_sucf PRIVATE suas_static)
add_test(NAME test_suas_sucf COMMAND test_suas_sucf)

add_executable(test_suas_ssp tests/test_suas_ssp.c)
target_link_libraries(test_suas_ssp PRIVATE suas_static)
add_test(NAME test_suas_ssp COMMAND test_suas_ssp)

add_executable(test_suas_sts tests/test_suas_sts.c)
target_link_libraries(test_suas_sts PRIVATE suas_static)
add_test(NAME test_suas_sts COMMAND test_suas_sts)

add_executable(test_suas_sis tests/test_suas_sis.c)
target_link_libraries(test_suas_sis PRIVATE suas_static)
add_test(NAME test_suas_sis COMMAND test_suas_sis)

# ── SUTS Technical Specifications ──────────────────────────────────
# Freestanding static library.
#   SUTS-001 (SUCA) — UCA(UTS #10)-equivalent multilevel collation.
#   SUTS-002 (SRE)  — UTS #18-analog regular expressions: zero-allocation,
#                     single-pass automaton matching over 64-bit ExtSUCS.
# Both operate over the 64-bit SUCS char space (extSUCS compatible).
add_library(suts_static STATIC
    src/suts/suts_suca.c
    src/suts/suts_sre.c
)

target_include_directories(suts_static PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
if(MSVC)
    target_compile_options(suts_static PRIVATE /X)
else()
    target_compile_options(suts_static PRIVATE -std=c99 -nostdlib -ffreestanding)
endif()

# SUTS host unit tests
add_executable(test_suts_suca tests/test_suts_suca.c)
target_link_libraries(test_suts_suca PRIVATE suts_static)
add_test(NAME test_suts_suca COMMAND test_suts_suca)

add_executable(test_suts_sre tests/test_suts_sre.c)
target_link_libraries(test_suts_sre PRIVATE suts_static)
add_test(NAME test_suts_sre COMMAND test_suts_sre)

# ── Sub-projects ──────────────────────────────────────────────────
add_subdirectory(superunicode)
add_subdirectory(sutf)
add_subdirectory(superunicode_extended)
add_subdirectory(sust)
add_subdirectory(suf)

# Unified header-coexistence test (all module headers in one TU)
add_subdirectory(unified)

