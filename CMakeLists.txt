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

# ── SUAS Architecture Standards (SDF, SGW, SBR, SUCF, SSP, STS, SIS, SNCS) ──
# Freestanding static library implementing Structural Directional Framing
# and the full Unicode Bidirectional Algorithm (UAX #9) processing model,
# System Glyph Width & Monospace Grid (SGW) fixed-cell terminal metrics,
# System Boundary & Line Break Rules (SBR) single-pass line breaking,
# SuperUnicode Canonical Forms (SUCF) single-pass canonical transforms,
# the System Script Property (SSP) range-dispatched script resolver,
# System Text Segmentation (STS) single-pass grapheme/word/sentence
# boundaries (UAX #29 analog), System Identifiers and Syntax (SIS)
# single-pass identifier classes and lexical syntax (UAX #31 analog),
# System Named Character Sequences (SNCS) static-registry, O(1) resolution
# of multi-codepoint symbol names (UAX #34 analog), System Ideographic
# Properties (SIP) zero-allocation, O(1) ideographic attribute extraction
# (UAX #38 analog), System Specification Registry & Cross-Reference
# Engine (SSRE) embedded dependency metadata and build-time linkage
# validation (UAX #41 analog), and System Character Manifest & Schema
# Standard (SCMS) the line-oriented, binary-mappable manifest grammar
# (UAX #42 analog), and SuperUnicode Character Database (SUCD) the
# canonical architecture and zero-allocation binary memory
# representation of the character database itself (UAX #44 analog),
# and System Ideographic Source Registry (SISR) byte-packed source
# identities, registry classes, and canonical variant fallbacks
# (UAX #45 analog), and SuperUnicode Vertical Text Layout (SUVTL) packed
# vertical orientation, rotation, and baseline metrics (UAX #50 analog).
add_library(suas_static STATIC
    src/suas/suas_sdf.c
    src/suas/suas_sgw.c
    src/suas/suas_sbr.c
    src/suas/suas_sucf.c
    src/suas/suas_ssp.c
    src/suas/suas_sts.c
    src/suas/suas_sis.c
    src/suas/suas_sncs.c
    src/suas/suas_sip.c
    src/suas/suas_ssre.c
    src/suas/suas_scms.c
    src/suas/suas_sucd.c
    src/suas/suas_sisr.c
    src/suas/suas_suvtl.c
    src/suas/suas_suamr.c
    src/suas/suas_sukemet.c
    src/suas/suas_sueasd.c
    src/suas/suas_skto.c
    src/suas/suas_stim.c
    src/suas/suas_sucha.c
    src/suas/suas_stx.c
    src/suas/suas_sktc.c
    src/suas/suas_subsystems.c
    src/suas/suas_core.c
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

add_executable(test_suas_sncs tests/test_suas_sncs.c)
target_link_libraries(test_suas_sncs PRIVATE suas_static)
add_test(NAME test_suas_sncs COMMAND test_suas_sncs)

add_executable(test_suas_sip tests/test_suas_sip.c)
target_link_libraries(test_suas_sip PRIVATE suas_static)
add_test(NAME test_suas_sip COMMAND test_suas_sip)

add_executable(test_suas_ssre tests/test_suas_ssre.c)
target_link_libraries(test_suas_ssre PRIVATE suas_static)
add_test(NAME test_suas_ssre COMMAND test_suas_ssre)

add_executable(test_suas_scms tests/test_suas_scms.c)
target_link_libraries(test_suas_scms PRIVATE suas_static)
add_test(NAME test_suas_scms COMMAND test_suas_scms)

add_executable(test_suas_sucd tests/test_suas_sucd.c)
target_link_libraries(test_suas_sucd PRIVATE suas_static)
add_test(NAME test_suas_sucd COMMAND test_suas_sucd)

add_executable(test_suas_sisr tests/test_suas_sisr.c)
target_link_libraries(test_suas_sisr PRIVATE suas_static)
add_test(NAME test_suas_sisr COMMAND test_suas_sisr)

add_executable(test_suas_suvtl tests/test_suas_suvtl.c)
target_link_libraries(test_suas_suvtl PRIVATE suas_static)
add_test(NAME test_suas_suvtl COMMAND test_suas_suvtl)

add_executable(test_suas_suamr tests/test_suas_suamr.c)
target_link_libraries(test_suas_suamr PRIVATE suas_static)
add_test(NAME test_suas_suamr COMMAND test_suas_suamr)

add_executable(test_suas_sukemet tests/test_suas_sukemet.c)
target_link_libraries(test_suas_sukemet PRIVATE suas_static)
add_test(NAME test_suas_sukemet COMMAND test_suas_sukemet)

add_executable(test_suas_sueasd tests/test_suas_sueasd.c)
target_link_libraries(test_suas_sueasd PRIVATE suas_static)
add_test(NAME test_suas_sueasd COMMAND test_suas_sueasd)

add_executable(test_suas_skto tests/test_suas_skto.c)
target_link_libraries(test_suas_skto PRIVATE suas_static)
add_test(NAME test_suas_skto COMMAND test_suas_skto)

# 32-bit ABI evidence. The main suite runs on whatever pointer width the build
# host uses; on a 64-bit host its sizeof/offsetof assertions describe LP64 only.
# This target therefore exists only when the build itself is 32-bit (for
# example a VS x86 configure), so the ILP32 layout assertions are compiled by a
# real 32-bit compiler rather than inferred. A 64-bit build deliberately does
# not add it: the program exits early on sizeof(void*) != 4.
if(CMAKE_SIZEOF_VOID_P EQUAL 4)
    add_executable(test_skto_32bit tests/test_skto_32bit.c)
    target_link_libraries(test_skto_32bit PRIVATE suas_static)
    add_test(NAME test_skto_32bit COMMAND test_skto_32bit)
endif()

# Deterministic property/fuzz harness. Fixed seed, fixed default iteration
# count; SKTO_FUZZ_ITER raises the run length for a manual soak without making
# CTest nondeterministic or slow.
add_executable(test_skto_fuzz tests/test_skto_fuzz.c)
target_link_libraries(test_skto_fuzz PRIVATE suas_static)
add_test(NAME test_skto_fuzz COMMAND test_skto_fuzz)

# KT-1 compatibility adapter: bounded admission, containment, and generation-
# stamped publication at the OWFS/USFS name-field boundary. Host-side evidence
# only; see compat/ow_skto_name.h for the lifetime contract.
add_executable(test_compat_skto_name
    compat/test_compat_skto_name.c
    compat/ow_skto_name.c
    compat/OpenWindows-storage/common/src/ow_string.c
    compat/OpenWindows-storage/common/src/ow_mem.c
)
target_include_directories(test_compat_skto_name PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/compat
    ${CMAKE_CURRENT_SOURCE_DIR}/compat/OpenWindows-storage/common/include
    ${CMAKE_CURRENT_SOURCE_DIR}/compat/OpenWindows-storage/owfs/include
    ${CMAKE_CURRENT_SOURCE_DIR}/compat/OpenWindows-storage/usfs/include
)
target_link_libraries(test_compat_skto_name PRIVATE suas_static)
add_test(NAME test_compat_skto_name COMMAND test_compat_skto_name)

add_executable(test_suas_stim tests/test_suas_stim.c)
target_link_libraries(test_suas_stim PRIVATE suas_static)
add_test(NAME test_suas_stim COMMAND test_suas_stim)

add_executable(test_suas_subsystems tests/test_suas_subsystems.c)
target_link_libraries(test_suas_subsystems PRIVATE suas_static)
add_test(NAME test_suas_subsystems COMMAND test_suas_subsystems)

add_executable(test_subsystems tests/test_subsystems.c)
target_link_libraries(test_subsystems PRIVATE suts_static sutr_static suas_static)
add_test(NAME test_subsystems COMMAND test_subsystems)

add_executable(test_suas_core tests/test_suas_core.c)
target_link_libraries(test_suas_core PRIVATE suas_static)
add_test(NAME test_suas_core COMMAND test_suas_core)

# ── SUTS Technical Specifications ──────────────────────────────────
# Freestanding static library.
#   SUTS-001 (SUCA) — UCA(UTS #10)-equivalent multilevel collation.
#   SUTS-002 (SRE)  — UTS #18-analog regular expressions: zero-allocation,
#                     single-pass automaton matching over 64-bit ExtSUCS.
#   SUTS-003 (SLDF) — binary-compiled locale formatting: zero-allocation,
#                     O(1) RCCR-subtag resolution of numeric, temporal, and
#                     collation rules over 64-bit SUCS.
#   SUTS-004 (SIVD) — binary-compiled ideographic variation database:
#                     zero-allocation, O(1) (base, selector) -> glyph-index
#                     resolution over the Bridge and RNUR planes.
#   SUTS-005 (SCSS) — binary-compiled character security: zero-allocation,
#                     O(1) homograph (skeleton) resolution, identifier
#                     admissibility, and Bridge/Native script isolation.
#   SUTS-006 (SIDN) — internationalized domain & identifier network
#                     architecture: zero-allocation, single-pass label
#                     mapping, validation and canonical routing over 31-bit
#                     SUCS, replacing Punycode text conversion.
#   SUTS-007 (SUEPS) — emoji & presentation: zero-allocation, O(1) emoji
#                     presentation resolution and flattened ZWJ / modifier
#                     sequence tokens over 31-bit SUCS, enforcing the
#                     SuperUnicode "No Surrogates" mandate.
#   SUTS-008 (SUSCH) — source code handling: zero-allocation, single-pass
#                     source code safety attributes, identifier validation,
#                     bidi/invisible elimination, homoglyph risk, and UTS #55
#                     identifier chunk analysis over 31-bit SUCS.
#   SUTS-011 (SUSN) — set notation: binary-mapped character-set algebra,
#                     inversion lists, range-filtered O(1) membership, and
#                     RNUR conscript set integration over 31-bit SUCS (a
#                     UTS #61 analog). SUTS-010 is a deliberate gap.
# All operate over the 64-bit SUCS char space (extSUCS compatible).
add_library(suts_static STATIC
    src/suts/suts_suca.c
    src/suts/suts_sre.c
    src/suts/suts_sldf.c
    src/suts/suts_sivd.c
    src/suts/suts_scss.c
    src/suts/suts_sidn.c
    src/suts/suts_sueps.c
    src/suts/suts_susch.c
    src/suts/suts_suldf.c
    src/suts/suts_susn.c
    src/suts/suts_scts.c
    src/suts/suts_scte.c
    src/suts/suts_suicl.c
    src/suts/suts_sulp.c
    src/suts/suts_strs.c
    src/suts/suts_module.c
)

target_include_directories(suts_static PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
# SUTS-012 (SCTS) defers width, break, and cluster decisions to the SUAS
# standards, so the SUAS library is part of the SUTS link interface.
target_link_libraries(suts_static PUBLIC suas_static)
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

add_executable(test_suts_sldf tests/test_suts_sldf.c)
target_link_libraries(test_suts_sldf PRIVATE suts_static)
add_test(NAME test_suts_sldf COMMAND test_suts_sldf)

add_executable(test_suts_sivd tests/test_suts_sivd.c)
target_link_libraries(test_suts_sivd PRIVATE suts_static)
add_test(NAME test_suts_sivd COMMAND test_suts_sivd)

add_executable(test_suts_scss tests/test_suts_scss.c)
target_link_libraries(test_suts_scss PRIVATE suts_static)
add_test(NAME test_suts_scss COMMAND test_suts_scss)

add_executable(test_suts_sidn tests/test_suts_sidn.c)
target_link_libraries(test_suts_sidn PRIVATE suts_static)
add_test(NAME test_suts_sidn COMMAND test_suts_sidn)

add_executable(test_suts_sueps tests/test_suts_sueps.c)
target_link_libraries(test_suts_sueps PRIVATE suts_static)
add_test(NAME test_suts_sueps COMMAND test_suts_sueps)

add_executable(test_suts_susch tests/test_suts_susch.c)
target_link_libraries(test_suts_susch PRIVATE suts_static)
add_test(NAME test_suts_susch COMMAND test_suts_susch)

add_executable(test_suts_suldf tests/test_suts_suldf.c)
target_link_libraries(test_suts_suldf PRIVATE suts_static)
add_test(NAME test_suts_suldf COMMAND test_suts_suldf)

add_executable(test_suts_susn tests/test_suts_susn.c)
target_link_libraries(test_suts_susn PRIVATE suts_static)
add_test(NAME test_suts_susn COMMAND test_suts_susn)

add_executable(test_suts_scts tests/test_suts_scts.c)
target_link_libraries(test_suts_scts PRIVATE suts_static)
add_test(NAME test_suts_scts COMMAND test_suts_scts)

add_executable(test_suts_scte tests/test_suts_scte.c)
target_link_libraries(test_suts_scte PRIVATE suts_static)
add_test(NAME test_suts_scte COMMAND test_suts_scte)

add_executable(test_suts_module tests/test_suts_module.c)
target_link_libraries(test_suts_module PRIVATE suts_static)
add_test(NAME test_suts_module COMMAND test_suts_module)

# ── SUTR Technical Reports (with reference implementations) ────────
# Freestanding static library. SUTR-7 onward carry C reference
# implementations alongside their normative reports.
#   SUTR-012 (SUCSL) — Cuneiform sign lists: packed sign records,
#                     precomputed primary-value hashes, compound
#                     composition, and the zero-allocation O(1) sign
#                     lookup engine over 31-bit SUCS (a UTR #56 analog).
#   SUTR-011 (SUMS) — Mongolian shaping & vertical direction:
#                     zero-allocation, O(1) contextual positional-form
#                     resolution and flattened FVS variation-sequence
#                     resolution over 31-bit SUCS (a UTR #54 analog).
#   SUTR-010 (SUCM) — conformance model: the cumulative tiers, the
#                     clause registry, the claim identifier grammar, the
#                     gate records, and the published claim set.
#   SUTR-009 (SUMNLS) — mathematical notation and layout support:
#                     packed-mask math classification, the atom-class
#                     projection and spacing table, and the zero-allocation
#                     single-pass box-model layout engine (a UTR #25 analog).
#   SUTR-008 (SUCPM) — character property model: the range-dispatched
#                     dual lookup engine, the 32-bit core and 64-bit
#                     extended mask packing, and the O(1) property tests
#                     (a UTR #23 analog).
#   SUTR-007 (SUCEM) — character encoding model: the four levels, the
#                     forbidden-form-name rules, the repertoire sources,
#                     the SCP anchor, and the O(1) trap dispatch geometry
#                     (a UTR #17 analog).
add_library(sutr_static STATIC
    src/sutr/sutr_sucsl.c
    src/sutr/sutr_sueas.c
    src/sutr/sutr_sums.c
    src/sutr/sutr_sucem.c
    src/sutr/sutr_sucpm.c
    src/sutr/sutr_sumnls.c
    src/sutr/sutr_sucm.c
    src/sutr/sutr_oktp.c
    src/sutr/sutr_sie.c
    src/sutr/sutr_sfs.c
    src/sutr/sutr_sim.c
    src/sutr/sutr_skc.c
)

target_include_directories(sutr_static PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
if(MSVC)
    target_compile_options(sutr_static PRIVATE /X)
else()
    target_compile_options(sutr_static PRIVATE -std=c99 -nostdlib -ffreestanding)
endif()

# SUTR host unit tests
add_executable(test_sutr_sucsl tests/test_sutr_sucsl.c)
target_link_libraries(test_sutr_sucsl PRIVATE sutr_static suas_static)
add_test(NAME test_sutr_sucsl COMMAND test_sutr_sucsl)

add_executable(test_sutr_sums tests/test_sutr_sums.c)
target_link_libraries(test_sutr_sums PRIVATE sutr_static suas_static)
add_test(NAME test_sutr_sums COMMAND test_sutr_sums)

add_executable(test_sutr_sueas tests/test_sutr_sueas.c)
target_link_libraries(test_sutr_sueas PRIVATE sutr_static suas_static)
add_test(NAME test_sutr_sueas COMMAND test_sutr_sueas)

add_executable(test_sutr_sucem tests/test_sutr_sucem.c)
target_link_libraries(test_sutr_sucem PRIVATE sutr_static suas_static)
add_test(NAME test_sutr_sucem COMMAND test_sutr_sucem)

add_executable(test_sutr_sucpm tests/test_sutr_sucpm.c)
target_link_libraries(test_sutr_sucpm PRIVATE sutr_static suas_static)
add_test(NAME test_sutr_sucpm COMMAND test_sutr_sucpm)

add_executable(test_sutr_sumnls tests/test_sutr_sumnls.c)
target_link_libraries(test_sutr_sumnls PRIVATE sutr_static suas_static)
add_test(NAME test_sutr_sumnls COMMAND test_sutr_sumnls)

add_executable(test_sutr_sucm tests/test_sutr_sucm.c)
target_link_libraries(test_sutr_sucm PRIVATE sutr_static suas_static)
add_test(NAME test_sutr_sucm COMMAND test_sutr_sucm)

add_executable(test_sutr_oktp tests/test_sutr_oktp.c)
target_link_libraries(test_sutr_oktp PRIVATE sutr_static suas_static)
add_test(NAME test_sutr_oktp COMMAND test_sutr_oktp)

# ── Sub-projects ──────────────────────────────────────────────────
add_subdirectory(superunicode)
add_subdirectory(sutf)
add_subdirectory(superunicode_extended)
add_subdirectory(sust)
add_subdirectory(suf)
add_subdirectory(sutf_convert)

# Unified header-coexistence test (all module headers in one TU)
add_subdirectory(unified)
