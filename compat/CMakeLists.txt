cmake_minimum_required(VERSION 3.10)
project(superunicode_ecosystem_compat C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# ---------------------------------------------------------------------------
# Ecosystem Compatibility Suite
#
# Cross-project verification against the sibling OpenWindows repos. Each
# test co-includes this workspace's public headers with one sibling project's
# headers in a single translation unit (the guarded-constants coexistence
# contract), then asserts runtime parity of the shared contracts:
#
#   BANcode             - Kernel Security Trap dispatch & registry geometry
#   OpenWindows-Storage - OWFS/USFS layout constants & SUTF-8 codec contract
#   vip                 - codec / trap-mapping / addressing parity
#   Modular-Bootloader  - vendored sutf copies, boot-config ABI, OWFS mirrors
#
# Sibling sources are compiled directly into the tests (source-level
# integration, mirroring how the kernel will consume them).
# ---------------------------------------------------------------------------

set(SUCS_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/../superunicode/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../sutf/include
)

set(BANCODE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../BANcode)
set(STORAGE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../OpenWindows-Storage)
set(MBL_DIR     ${CMAKE_CURRENT_SOURCE_DIR}/../Modular-Bootloader)
set(VIP_DIR     ${CMAKE_CURRENT_SOURCE_DIR}/../vip)

enable_testing()

# --- BANcode: registry constants + Kernel Security Trap dispatch parity ----
add_executable(test_compat_bancode
    test_compat_bancode.c
    ${BANCODE_DIR}/kernel_src/bancode/bancode.c
    ${BANCODE_DIR}/kernel_src/bancode/bancode_trap.c
)
target_include_directories(test_compat_bancode PRIVATE
    ${SUCS_INCLUDE_DIRS}
    ${BANCODE_DIR}/kernel_inc
)
add_test(NAME compat_bancode COMMAND test_compat_bancode)

# --- OpenWindows-Storage: layout constants + SUTF-8 codec contract ---------
add_executable(test_compat_storage
    test_compat_storage.c
    ${STORAGE_DIR}/common/src/ow_string.c
    ${STORAGE_DIR}/common/src/ow_mem.c
)
target_include_directories(test_compat_storage PRIVATE
    ${SUCS_INCLUDE_DIRS}
    ${STORAGE_DIR}/common/include
    ${STORAGE_DIR}/owfs/include
    ${STORAGE_DIR}/usfs/include
)
add_test(NAME compat_storage COMMAND test_compat_storage)

# --- vip: codec / trap-mapping / volume-addressing parity ------------------
add_executable(test_compat_vip
    test_compat_vip.c
    ${VIP_DIR}/univip_fvip.c
)
target_include_directories(test_compat_vip PRIVATE
    ${SUCS_INCLUDE_DIRS}
    ${BANCODE_DIR}/kernel_inc
    ${VIP_DIR}
)
target_compile_definitions(test_compat_vip PRIVATE
    COMPAT_HAVE_BANCODE=1
    COMPAT_HAVE_OWFS=1
)
add_test(NAME compat_vip COMMAND test_compat_vip)

# --- Modular-Bootloader: vendored sutf copies + boot config ABI ------------
add_executable(test_compat_mbl
    test_compat_mbl.c
    ${MBL_DIR}/src/sutf/sucs_mode.c
)
target_include_directories(test_compat_mbl PRIVATE
    ${MBL_DIR}/include
    ${SUCS_INCLUDE_DIRS}
)
add_test(NAME compat_mbl COMMAND test_compat_mbl)

set_target_properties(
    test_compat_bancode test_compat_storage test_compat_vip test_compat_mbl
    PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin
)
