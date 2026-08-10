#include <stdio.h>
#include <assert.h>
#include <stdbool.h>
#include "superunicode/superunicode.h"

void test_coordinate_extractions(void) {
    /* Test codepoint 0x12345678 */
    sucs_char_t cp = 0x12345678;

    /* Zone: Bits 24..30 -> 0x12 & 0x7F = 0x12 = 18 */
    assert(SUCS_GET_ZONE(cp) == 0x12);

    /* District: Bits 15..30 -> (0x12345678 >> 15) & 0xFFFF = 0x2468 */
    assert(SUCS_GET_DISTRICT(cp) == ((0x12345678 >> 15) & 0xFFFF));

    /* Plane: Bits 8..30 -> (0x12345678 >> 8) & 0x7FFFFF = 0x123456 */
    assert(SUCS_GET_PLANE(cp) == 0x123456);

    /* Offset: Bits 0..7 -> 0x78 */
    assert(SUCS_GET_OFFSET(cp) == 0x78);

    printf("[PASS] test_coordinate_extractions (0x12345678)\n");
}

void test_fixed_plane_checks(void) {
    /* Plane 0: Codepoint 0x00000000 to 0x000000FF */
    assert(sucs_is_fixed_plane(0x00000041) == true);

    /* Plane 1: Codepoint 0x00000100 to 0x000001FF */
    assert(sucs_is_fixed_plane(0x00000150) == true);

    /* Plane 2: Codepoint 0x00000200 */
    assert(sucs_is_fixed_plane(0x00000200) == false);

    printf("[PASS] test_fixed_plane_checks\n");
}

void test_codepoint_classifications(void) {
    /* Unicode Bridge Zone */
    assert(sucs_classify_codepoint(0x00000041) == SUCS_TYPE_UNICODE_COMPAT);
    assert(sucs_classify_codepoint(0x0010FFFF) == SUCS_TYPE_UNICODE_COMPAT);
    assert(sucs_is_unicode_compatible(0x0010FFFF) == true);

    /* System Control Plane (SCP) */
    assert(sucs_classify_codepoint(0x00110000) == SUCS_TYPE_SYS_FUNCTION);
    assert(sucs_classify_codepoint(SUCS_FMT_BOLD_ON) == SUCS_TYPE_SYS_FUNCTION);
    assert(sucs_classify_codepoint(SUCS_FMT_COLOR_RGB) == SUCS_TYPE_SYS_FUNCTION);
    assert(sucs_classify_codepoint(0x0011FFFF) == SUCS_TYPE_SYS_FUNCTION);
    assert(sucs_is_scp_plane(0x00110000) == true);
    assert(sucs_is_formatting_char(SUCS_FMT_ITALIC_ON) == true);

    /* Native Extended Space */
    assert(sucs_classify_codepoint(0x00120000) == SUCS_TYPE_NATIVE_ALLOC);
    assert(sucs_classify_codepoint(0x7FFFFFFF) == SUCS_TYPE_NATIVE_ALLOC);
    assert(sucs_is_native_extended(0x00120000) == true);
    assert(sucs_is_unicode_compatible(0x00120000) == false);

    printf("[PASS] test_codepoint_classifications\n");
}

void test_sucs_string_formatting_length(void) {
    /* Create a buffer containing:
     * 'H', 'e', 'l', 'l', 'o', [SUCS_FMT_BOLD_ON], 'W', 'o', 'r', 'l', 'd'
     */
    char buf[128];
    size_t offset = 0;
    size_t written = 0;

    sutf_encode_char('H', buf + offset, sizeof(buf) - offset, &written); offset += written;
    sutf_encode_char('e', buf + offset, sizeof(buf) - offset, &written); offset += written;
    sutf_encode_char('l', buf + offset, sizeof(buf) - offset, &written); offset += written;
    sutf_encode_char('l', buf + offset, sizeof(buf) - offset, &written); offset += written;
    sutf_encode_char('o', buf + offset, sizeof(buf) - offset, &written); offset += written;
    /* Inline System Control Point (formatting code) */
    sutf_encode_char(SUCS_FMT_BOLD_ON, buf + offset, sizeof(buf) - offset, &written); offset += written;
    sutf_encode_char('W', buf + offset, sizeof(buf) - offset, &written); offset += written;
    sutf_encode_char('o', buf + offset, sizeof(buf) - offset, &written); offset += written;
    sutf_encode_char('r', buf + offset, sizeof(buf) - offset, &written); offset += written;
    sutf_encode_char('l', buf + offset, sizeof(buf) - offset, &written); offset += written;
    sutf_encode_char('d', buf + offset, sizeof(buf) - offset, &written); offset += written;

    SUCS_STRING str;
    str.buffer = buf;
    str.length_bytes = (uint32_t)offset;
    str.capacity_bytes = sizeof(buf);

    size_t visual_len = 0;
    size_t total_cps = 0;

    int status = sucs_strlen(&str, &visual_len);
    assert(status == SUES_SUCCESS);

    status = sucs_codepoint_count(&str, &total_cps);
    assert(status == SUES_SUCCESS);

    /* Total codepoints = 11 (10 letters + 1 bold_on control code) */
    assert(total_cps == 11);
    /* Visual length = 10 (skipping 1 SCP control code) */
    assert(visual_len == 10);

    printf("[PASS] test_sucs_string_formatting_length (Total CPs: %zu, Visual Len: %zu)\n", total_cps, visual_len);
}

void test_bancode_registry_ranges(void) {
    /* Registry plugin range (0x0011A000 - 0x0011AEFF) */
    assert(sucs_is_bancode_registry(SUCS_BANCODE_REGISTRY_MIN) == true);
    assert(sucs_is_bancode_registry(SUCS_BANCODE_REGISTRY_MAX) == true);
    assert(sucs_is_bancode_registry(0x0011A7FF) == true);
    assert(sucs_is_bancode_registry(0x0011AEFF) == true);
    assert(sucs_is_bancode_registry(0x0011AFFF) == false);
    assert(sucs_is_bancode_registry(SUCS_FMT_RESET) == false);

    /* B+ BANcode (Fatal) */
    assert(sucs_is_bancode(SUCS_BANCODE_RANGE_MIN) == true);
    assert(sucs_is_bancode(SUCS_BANCODE_RANGE_MAX) == true);
    assert(sucs_is_bancode(0x0011A7FF) == true);
    assert(sucs_is_bancode(0x0011A800) == false);
    assert(sucs_is_bancode(0x0011A000 - 1) == false);

    /* W+ WARNcode */
    assert(sucs_is_warncode(SUCS_WARNCODE_RANGE_MIN) == true);
    assert(sucs_is_warncode(SUCS_WARNCODE_RANGE_MAX) == true);
    assert(sucs_is_warncode(0x0011A7FF) == false);
    assert(sucs_is_warncode(0x0011AC00) == false);

    /* C+ COMcode */
    assert(sucs_is_comcode(SUCS_COMCODE_RANGE_MIN) == true);
    assert(sucs_is_comcode(SUCS_COMCODE_RANGE_MAX) == true);
    assert(sucs_is_comcode(0x0011ABFF) == false);
    assert(sucs_is_comcode(0x0011AE00) == false);

    /* S+ SOFTcode */
    assert(sucs_is_softcode(SUCS_SOFTCODE_RANGE_MIN) == true);
    assert(sucs_is_softcode(SUCS_SOFTCODE_RANGE_MAX) == true);
    assert(sucs_is_softcode(0x0011ADFF) == false);
    assert(sucs_is_softcode(0x0011AF00) == false);

    /* Classification */
    assert(sucs_classify_bancode(0x0011A005) == SUCS_BANCODE_FATAL);
    assert(sucs_classify_bancode(0x0011A850) == SUCS_BANCODE_WARN);
    assert(sucs_classify_bancode(0x0011AC20) == SUCS_BANCODE_COM);
    assert(sucs_classify_bancode(0x0011AE10) == SUCS_BANCODE_SOFT);
    assert(sucs_classify_bancode(0x0011AFFF) == SUCS_BANCODE_NONE);
    assert(sucs_classify_bancode(0x00110000) == SUCS_BANCODE_NONE);

    printf("[PASS] test_bancode_registry_ranges\n");
}

void test_kernel_trap_dispatch(void) {
    /* Trap slot boundary sanity */
    assert(SUCS_KERNEL_TRAP_MAX - SUCS_KERNEL_TRAP_MIN + 1 == SUCS_TRAP_SLOT_COUNT);
    assert((SUCS_BANCODE_RANGE_MAX - SUCS_BANCODE_RANGE_MIN + 1) / SUCS_BANCODES_PER_TRAP >= SUCS_TRAP_SLOT_COUNT);

    /* Forward: B+ BANcode -> Kernel Security Trap */
    assert(sucs_bancode_to_trap(0x0011A000) == 0x7FFFFFF0);
    assert(sucs_bancode_to_trap(0x0011A005) == 0x7FFFFFF0);
    assert(sucs_bancode_to_trap(0x0011A07F) == 0x7FFFFFF0);
    assert(sucs_bancode_to_trap(0x0011A080) == 0x7FFFFFF1);
    assert(sucs_bancode_to_trap(0x0011A0FF) == 0x7FFFFFF1);
    assert(sucs_bancode_to_trap(0x0011A100) == 0x7FFFFFF2);
    assert(sucs_bancode_to_trap(0x0011A700) == 0x7FFFFFFE);
    assert(sucs_bancode_to_trap(0x0011A77F) == 0x7FFFFFFE);

    /* Slot 15 (0x0011A780 - 0x0011A7FF) has no assigned trap handler */
    assert(sucs_bancode_to_trap(0x0011A780) == SUCS_INVALID_CODEPOINT);
    assert(sucs_bancode_to_trap(0x0011A7FF) == SUCS_INVALID_CODEPOINT);

    /* Non-fatal / non-BANcode inputs resolve to the sentinel */
    assert(sucs_bancode_to_trap(0x0011A850) == SUCS_INVALID_CODEPOINT);
    assert(sucs_bancode_to_trap(0x0011AC20) == SUCS_INVALID_CODEPOINT);
    assert(sucs_bancode_to_trap(0x00110000) == SUCS_INVALID_CODEPOINT);

    /* Reverse: Kernel Security Trap -> managed B+ BANcode cluster */
    sucs_char_t lo = 0, hi = 0;
    assert(sucs_trap_to_bancode_range(0x7FFFFFF0, &lo, &hi) == true);
    assert(lo == 0x0011A000 && hi == 0x0011A07F);
    assert(sucs_trap_to_bancode_range(0x7FFFFFF1, &lo, &hi) == true);
    assert(lo == 0x0011A080 && hi == 0x0011A0FF);
    assert(sucs_trap_to_bancode_range(0x7FFFFFF2, &lo, &hi) == true);
    assert(lo == 0x0011A100 && hi == 0x0011A17F);
    assert(sucs_trap_to_bancode_range(0x7FFFFFFE, &lo, &hi) == true);
    assert(lo == 0x0011A700 && hi == 0x0011A77F);

    /* Invalid / non-trap codepoints rejected */
    assert(sucs_trap_to_bancode_range(0x7FFFFFFF, &lo, &hi) == false);
    assert(sucs_trap_to_bancode_range(0x7FFFFFEF, &lo, &hi) == false);
    assert(sucs_trap_to_bancode_range(0x0011A000, &lo, &hi) == false);
    assert(sucs_trap_to_bancode_range(0x7FFFFFF0, NULL, &hi) == false);

    /* Trap classification */
    assert(sucs_is_kernel_trap(0x7FFFFFF0) == true);
    assert(sucs_is_kernel_trap(0x7FFFFFFE) == true);
    assert(sucs_is_kernel_trap(0x7FFFFFFF) == false);
    assert(sucs_is_kernel_trap(0x7FFFFFEF) == false);

    /* Bidirectional consistency across every trap slot */
    for (sucs_char_t trap = SUCS_KERNEL_TRAP_MIN; trap <= SUCS_KERNEL_TRAP_MAX; trap++) {
        assert(sucs_trap_to_bancode_range(trap, &lo, &hi) == true);
        assert(sucs_bancode_to_trap(lo) == trap);
        assert(sucs_bancode_to_trap(hi) == trap);
    }

    printf("[PASS] test_kernel_trap_dispatch\n");
}

int main(void) {
    printf("--- Running SUCS Plane & Classification Unit Tests ---\n");
    test_coordinate_extractions();
    test_fixed_plane_checks();
    test_codepoint_classifications();
    test_sucs_string_formatting_length();
    test_bancode_registry_ranges();
    test_kernel_trap_dispatch();
    printf("--- ALL PLANE & CLASSIFICATION TESTS PASSED ---\n");
    return 0;
}
