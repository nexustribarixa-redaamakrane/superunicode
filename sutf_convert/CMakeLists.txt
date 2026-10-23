cmake_minimum_required(VERSION 3.10)
project(SUTFConvert C)

# Root of the repository (this directory's parent).
set(SU_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/..")

# Include directories
include_directories(
    ${SU_ROOT}/superunicode/include
    ${SU_ROOT}/superunicode_extended/include
    ${SU_ROOT}/sutf/include
    ${SU_ROOT}/sust/include
)

# Base SuperUnicode - SUTF
add_library(convert_sutf STATIC
    superunicode/sutf/src/ConvertSUTF.c
    ${SU_ROOT}/sutf/src/sutf8.c
    ${SU_ROOT}/sutf/src/sutf16.c
)
target_include_directories(convert_sutf PUBLIC
    superunicode/sutf/include
    ${SU_ROOT}/superunicode/include
    ${SU_ROOT}/sutf/include
)

# Base SuperUnicode - SUST
add_library(convert_sust STATIC
    superunicode/sust/src/ConvertSUST.c
    ${SU_ROOT}/sust/src/sust16.c
    ${SU_ROOT}/sust/src/sustfixed.c
)
target_include_directories(convert_sust PUBLIC
    superunicode/sust/include
    ${SU_ROOT}/superunicode/include
    ${SU_ROOT}/sust/include
)

# Extended SuperUnicode - ESUTF
add_library(convert_esutf STATIC
    extsuperunicode/sutf/src/ConvertESUTF.c
    ${SU_ROOT}/superunicode_extended/src/vsutf.c
    ${SU_ROOT}/superunicode_extended/src/extsucs_big.c
)
target_include_directories(convert_esutf PUBLIC
    extsuperunicode/sutf/include
    ${SU_ROOT}/superunicode/include
    ${SU_ROOT}/superunicode_extended/include
)

# Extended SuperUnicode - ESUST
add_library(convert_esust STATIC
    extsuperunicode/sust/src/ConvertESUST.c
    ${SU_ROOT}/sust/src/sustfixed.c
)
target_include_directories(convert_esust PUBLIC
    extsuperunicode/sust/include
    ${SU_ROOT}/superunicode/include
    ${SU_ROOT}/superunicode_extended/include
    ${SU_ROOT}/sust/include
)

# Unit tests. Release builds define NDEBUG, which disables assert(), so the
# test targets explicitly undefine it where the compiler supports the flag.
set(SU_TEST_ASSERT_FLAG "")
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    set(SU_TEST_ASSERT_FLAG "-UNDEBUG")
endif()

add_executable(test_convert_sutf superunicode/sutf/tests/test_convert_sutf.c)
target_link_libraries(test_convert_sutf PRIVATE convert_sutf)
target_include_directories(test_convert_sutf PRIVATE superunicode/sutf/include)
target_compile_options(test_convert_sutf PRIVATE ${SU_TEST_ASSERT_FLAG})

add_executable(test_convert_sust superunicode/sust/tests/test_convert_sust.c)
target_link_libraries(test_convert_sust PRIVATE convert_sust)
target_include_directories(test_convert_sust PRIVATE superunicode/sust/include)
target_compile_options(test_convert_sust PRIVATE ${SU_TEST_ASSERT_FLAG})

add_executable(test_convert_esutf extsuperunicode/sutf/tests/test_convert_esutf.c)
target_link_libraries(test_convert_esutf PRIVATE convert_esutf)
target_include_directories(test_convert_esutf PRIVATE extsuperunicode/sutf/include)
target_compile_options(test_convert_esutf PRIVATE ${SU_TEST_ASSERT_FLAG})

add_executable(test_convert_esust extsuperunicode/sust/tests/test_convert_esust.c)
target_link_libraries(test_convert_esust PRIVATE convert_esust)
target_include_directories(test_convert_esust PRIVATE extsuperunicode/sust/include)
target_compile_options(test_convert_esust PRIVATE ${SU_TEST_ASSERT_FLAG})

add_test(NAME test_convert_sutf COMMAND test_convert_sutf)
add_test(NAME test_convert_sust COMMAND test_convert_sust)
add_test(NAME test_convert_esutf COMMAND test_convert_esutf)
add_test(NAME test_convert_esust COMMAND test_convert_esust)

# Port of the original Unicode conversion harness.
add_executable(harness_su harness_su.c)
target_link_libraries(harness_su PRIVATE
    convert_sutf convert_sust convert_esutf convert_esust)
target_include_directories(harness_su PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/superunicode/sutf/include
    ${CMAKE_CURRENT_SOURCE_DIR}/superunicode/sust/include
    ${CMAKE_CURRENT_SOURCE_DIR}/extsuperunicode/sutf/include
    ${CMAKE_CURRENT_SOURCE_DIR}/extsuperunicode/sust/include
    ${SU_ROOT}/superunicode/include
    ${SU_ROOT}/superunicode_extended/include
    ${SU_ROOT}/sutf/include
    ${SU_ROOT}/sust/include
)

add_test(NAME harness_su COMMAND harness_su)
