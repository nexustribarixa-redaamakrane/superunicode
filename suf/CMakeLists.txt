cmake_minimum_required(VERSION 3.10)
project(suf C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Include paths
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../superunicode/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../sutf/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../superunicode_extended/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../superunicode_extended/plugin/include
)

# Core freestanding parser & builder library
add_library(suf_static STATIC
    src/suf_parser.c
    src/suf_builder.c
    src/suf_conv.c
)
target_include_directories(suf_static PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# CLI Tools output directory
set(SUF_TOOLS_BIN ${CMAKE_CURRENT_BINARY_DIR}/tools)

# Inspector CLI
add_executable(suf_inspector tools/suf_inspector.c)
target_link_libraries(suf_inspector suf_static)

# Inbound converters
add_executable(ttf2suf tools/ttf2suf.c)
target_link_libraries(ttf2suf suf_static)

add_executable(otf2suf tools/otf2suf.c)
target_link_libraries(otf2suf suf_static)

add_executable(woff2suf tools/woff2suf.c)
target_link_libraries(woff2suf suf_static)

add_executable(sfd2suf tools/sfd2suf.c)
target_link_libraries(sfd2suf suf_static)

add_executable(eot2suf tools/eot2suf.c)
target_link_libraries(eot2suf suf_static)

add_executable(ps2suf tools/ps2suf.c)
target_link_libraries(ps2suf suf_static)

add_executable(pfb2suf tools/pfb2suf.c)
target_link_libraries(pfb2suf suf_static)

# Outbound exporters
add_executable(suf2ttf tools/suf2ttf.c)
target_link_libraries(suf2ttf suf_static)

add_executable(suf2otf tools/suf2otf.c)
target_link_libraries(suf2otf suf_static)

add_executable(suf2woff tools/suf2woff.c)
target_link_libraries(suf2woff suf_static)

add_executable(suf2sfd tools/suf2sfd.c)
target_link_libraries(suf2sfd suf_static)

add_executable(suf2eot tools/suf2eot.c)
target_link_libraries(suf2eot suf_static)

add_executable(suf2ps tools/suf2ps.c)
target_link_libraries(suf2ps suf_static)

add_executable(suf2pfb tools/suf2pfb.c)
target_link_libraries(suf2pfb suf_static)

# Plugin fontmaking CLI tool
add_executable(suf_plugin_tool tools/suf_plugin_tool.c)
target_link_libraries(suf_plugin_tool suf_static)

# Test suite
add_executable(test_suf tests/test_suf.c)
target_link_libraries(test_suf suf_static)

enable_testing()
add_test(NAME test_suf COMMAND test_suf)
