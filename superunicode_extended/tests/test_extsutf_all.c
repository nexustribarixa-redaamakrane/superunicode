/**
 * Test Suite: ExtSUCS Character Encoding & extSUTF Text Formatting Transports
 *
 * Verifies: ExtSUCS types/validators, upcast/downcast, SUTF-32/64/128/256/512/N,
 * vSUTF variable streaming, and e-SUTF hypervisor page-mapped IPC frames.
 *
 * NOTE: This test file uses host CRT (<stdio.h>, <assert.h>) for verification.
 * The library code under test remains strictly freestanding.
 */

#include <stdio.h>
#include <assert.h>
#include "extsucs_types.h"
#include "extsutf_fixed.h"
#include "vsutf.h"
#include "esutf.h"

/* ============================================================================
 * Test: ExtSUCS Character Encoding Validators
 * ============================================================================ */
void test_extsucs_validators(void) {
    /* All 64-bit values are valid EXCEPT the inherited trap range */
    assert(extsucs_is_valid(0x00000000ULL) == true);
    assert(extsucs_is_valid(0x00000041ULL) == true);
    assert(extsucs_is_valid(0x0010FFFFULL) == true);

    /* 0x7FFFFFFF IS valid in ExtSUCS (unbounded encoding, no sentinel) */
    assert(extsucs_is_valid(0x7FFFFFFFULL) == true);

    /* Inherited Kernel Security Trap Range — invalid in both encodings */
    assert(extsucs_is_valid(0x7FFFFFF0ULL) == false);
    assert(extsucs_is_valid(0x7FFFFFF5ULL) == false);
    assert(extsucs_is_valid(0x7FFFFFFEULL) == false);

    /* Extended range beyond Base SUCS — all valid */
    assert(extsucs_is_valid(0x80000000ULL) == true);
    assert(extsucs_is_valid(0xFFFFFFFFULL) == true);
    assert(extsucs_is_valid(0x100000000ULL) == true);
    assert(extsucs_is_valid(0xFFFFFFFFFFFFFFFFULL) == true);

    /* Base SUCS range detection */
    assert(extsucs_is_base_sucs(0x00000041ULL) == true);
    assert(extsucs_is_base_sucs(0x7FFFFFFFULL) == true);
    assert(extsucs_is_base_sucs(0x80000000ULL) == false);

    printf("[PASS] test_extsucs_validators\n");
}

/* ============================================================================
 * Test: Zero-Cost Upcast & Safe Downcast
 * ============================================================================ */
void test_upcast_downcast(void) {
    sucs_char_t base_cp = 0x41;
    sucs_ex_char_t ex_cp = sucs_upcast(base_cp);
    assert(ex_cp == 0x41ULL);

    /* Downcast valid Base SUCS codepoint */
    sucs_char_t out_cp = 0;
    assert(sucs_downcast(0x41ULL, &out_cp) == true);
    assert(out_cp == 0x41);

    /* Downcast extended range — fails (exceeds Base SUCS boundary) */
    assert(sucs_downcast(0x80000000ULL, &out_cp) == false);
    assert(out_cp == SUCS_INVALID_CODEPOINT);

    /* Downcast 0x7FFFFFFF — fails (Base SUCS sentinel, valid in ExtSUCS but not Base) */
    assert(sucs_downcast(0x7FFFFFFFULL, &out_cp) == false);

    /* Downcast trap range — fails */
    assert(sucs_downcast(0x7FFFFFF5ULL, &out_cp) == false);

    printf("[PASS] test_upcast_downcast\n");
}

/* ============================================================================
 * Test: SUTF-32 Fixed-Width Transport (4 bytes)
 * ============================================================================ */
void test_sutf32(void) {
    uint8_t buf[16];
    sucs_ex_char_t decoded = 0;

    /* Encode/decode 0x41 */
    assert(sutf32_encode(0x41ULL, buf, sizeof(buf)) == 4);
    assert(sutf32_decode(buf, 4, &decoded) == 4);
    assert(decoded == 0x41ULL);

    /* Encode/decode max 32-bit value */
    assert(sutf32_encode(0xFFFFFFFFULL, buf, sizeof(buf)) == 4);
    assert(sutf32_decode(buf, 4, &decoded) == 4);
    assert(decoded == 0xFFFFFFFFULL);

    /* Reject >32-bit codepoints */
    assert(sutf32_encode(0x100000000ULL, buf, sizeof(buf)) == 0);

    /* Reject trap range */
    assert(sutf32_encode(0x7FFFFFF5ULL, buf, sizeof(buf)) == 0);

    printf("[PASS] test_sutf32 (4-Byte Fixed)\n");
}

/* ============================================================================
 * Test: SUTF-64 Fixed-Width Transport (8 bytes)
 * ============================================================================ */
void test_sutf64(void) {
    uint8_t buf[16];
    sucs_ex_char_t decoded = 0;

    /* Full 64-bit roundtrip */
    assert(sutf64_encode(0xDEADBEEFCAFEULL, buf, sizeof(buf)) == 8);
    assert(sutf64_decode(buf, 8, &decoded) == 8);
    assert(decoded == 0xDEADBEEFCAFEULL);

    /* Max 64-bit value */
    assert(sutf64_encode(0xFFFFFFFFFFFFFFFFULL, buf, sizeof(buf)) == 8);
    assert(sutf64_decode(buf, 8, &decoded) == 8);
    assert(decoded == 0xFFFFFFFFFFFFFFFFULL);

    /* 0x7FFFFFFF is valid in ExtSUCS */
    assert(sutf64_encode(0x7FFFFFFFULL, buf, sizeof(buf)) == 8);
    assert(sutf64_decode(buf, 8, &decoded) == 8);
    assert(decoded == 0x7FFFFFFFULL);

    printf("[PASS] test_sutf64 (8-Byte Fixed)\n");
}

/* ============================================================================
 * Test: SUTF-128/256/512 Fixed-Width Transports
 * ============================================================================ */
void test_sutf_wide(void) {
    uint8_t buf128[16];
    uint8_t buf256[32];
    uint8_t buf512[64];
    sucs_ex_char_t decoded = 0;

    sucs_ex_char_t test_cp = 0xABCDEF0123456789ULL;

    /* SUTF-128 */
    assert(sutf128_encode(test_cp, buf128, sizeof(buf128)) == 16);
    assert(sutf128_decode(buf128, 16, &decoded) == 16);
    assert(decoded == test_cp);

    /* SUTF-256 */
    assert(sutf256_encode(test_cp, buf256, sizeof(buf256)) == 32);
    assert(sutf256_decode(buf256, 32, &decoded) == 32);
    assert(decoded == test_cp);

    /* SUTF-512 */
    assert(sutf512_encode(test_cp, buf512, sizeof(buf512)) == 64);
    assert(sutf512_decode(buf512, 64, &decoded) == 64);
    assert(decoded == test_cp);

    printf("[PASS] test_sutf_wide (128/256/512-Byte Fixed)\n");
}

/* ============================================================================
 * Test: SUTF-N Arbitrary Fixed-Width Transport
 * ============================================================================ */
void test_sutfn(void) {
    uint8_t buf[128];
    sucs_ex_char_t decoded = 0;

    sucs_ex_char_t test_cp = 0x123456789ABCDEF0ULL;

    /* 24-byte arbitrary slot */
    assert(sutfn_encode(test_cp, buf, 24) == 24);
    assert(sutfn_decode(buf, 24, &decoded) == 24);
    assert(decoded == test_cp);

    /* Minimum 8-byte slot */
    assert(sutfn_encode(test_cp, buf, 8) == 8);
    assert(sutfn_decode(buf, 8, &decoded) == 8);
    assert(decoded == test_cp);

    /* Reject slot < 8 bytes */
    assert(sutfn_encode(test_cp, buf, 4) == 0);

    printf("[PASS] test_sutfn (N-Byte Arbitrary Fixed)\n");
}

/* ============================================================================
 * Test: vSUTF Variable Streaming Transport
 * ============================================================================ */
void test_vsutf(void) {
    uint8_t buf[16];
    sucs_ex_char_t decoded = 0;

    /* 1-byte ASCII fast-path */
    assert(vsutf_encode(0x41ULL, buf, sizeof(buf)) == 1);
    assert(vsutf_decode(buf, 1, &decoded) == 1);
    assert(decoded == 0x41ULL);

    /* 3-byte (0xD800 — valid PUA in SUCS, no surrogates) */
    assert(vsutf_encode(0xD800ULL, buf, sizeof(buf)) == 3);
    assert(vsutf_decode(buf, 3, &decoded) == 3);
    assert(decoded == 0xD800ULL);

    /* 6-byte max Base SUCS (0x7FFFFFFF — valid in ExtSUCS) */
    assert(vsutf_encode(0x7FFFFFFFULL, buf, sizeof(buf)) == 6);
    assert(vsutf_decode(buf, 6, &decoded) == 6);
    assert(decoded == 0x7FFFFFFFULL);

    /* 9-byte extended (0xFE prefix + 8 bytes) */
    assert(vsutf_encode(0x80000000ULL, buf, sizeof(buf)) == 9);
    assert(buf[0] == 0xFE);
    assert(vsutf_decode(buf, 9, &decoded) == 9);
    assert(decoded == 0x80000000ULL);

    /* 9-byte extended (max 64-bit value) */
    assert(vsutf_encode(0xFFFFFFFFFFFFFFFFULL, buf, sizeof(buf)) == 9);
    assert(buf[0] == 0xFE);
    assert(vsutf_decode(buf, 9, &decoded) == 9);
    assert(decoded == 0xFFFFFFFFFFFFFFFFULL);

    /* Trap range rejection */
    assert(vsutf_encode(0x7FFFFFF5ULL, buf, sizeof(buf)) == 0);

    /* 0xFF reserved prefix rejection on decode */
    buf[0] = 0xFF;
    assert(vsutf_decode(buf, 9, &decoded) == 0);

    printf("[PASS] test_vsutf (Variable Streaming Transport)\n");
}

/* ============================================================================
 * Test: e-SUTF Hypervisor Page-Mapped IPC Transport
 * ============================================================================ */
void test_esutf(void) {
    uint32_t page_index = 0;
    uint16_t offset = 0;
    sucs_ex_char_t decoded = 0;
    uint8_t frame[8];

    /* Page 0, offset 0x41 = codepoint 0x41 */
    assert(esutf_translate_to_guest(0x41ULL, &page_index, &offset) == true);
    assert(page_index == 0);
    assert(offset == 0x41);

    /* Reconstruct back to host */
    assert(esutf_translate_to_host(0, 0x41, &decoded) == true);
    assert(decoded == 0x41ULL);

    /* Page boundary: codepoint 4096 = page 1, offset 0 */
    assert(esutf_translate_to_guest(4096ULL, &page_index, &offset) == true);
    assert(page_index == 1);
    assert(offset == 0);

    /* Large codepoint: 0x12345678 */
    assert(esutf_translate_to_guest(0x12345678ULL, &page_index, &offset) == true);
    assert(esutf_translate_to_host(page_index, offset, &decoded) == true);
    assert(decoded == 0x12345678ULL);

    /* IPC frame roundtrip */
    assert(esutf_encode_ipc(0x12345678ULL, frame, sizeof(frame)) == 6);
    assert(esutf_decode_ipc(frame, 6, &decoded) == 6);
    assert(decoded == 0x12345678ULL);

    /* Trap range rejection in IPC */
    assert(esutf_encode_ipc(0x7FFFFFF5ULL, frame, sizeof(frame)) == 0);

    /* Offset out of bounds rejection */
    assert(esutf_translate_to_host(0, 5000, &decoded) == false);

    printf("[PASS] test_esutf (Hypervisor Page-Mapped IPC)\n");
}

/* ============================================================================
 * Main
 * ============================================================================ */
int main(void) {
    printf("=====================================================\n");
    printf(" RUNNING ALL EXTSUTF SERIALIZATION TRANSPORT TESTS   \n");
    printf("=====================================================\n");
    test_extsucs_validators();
    test_upcast_downcast();
    test_sutf32();
    test_sutf64();
    test_sutf_wide();
    test_sutfn();
    test_vsutf();
    test_esutf();
    printf("=====================================================\n");
    printf(" ALL EXTSUTF SERIALIZATION TRANSPORT TESTS PASSED!   \n");
    printf("=====================================================\n");
    return 0;
}
