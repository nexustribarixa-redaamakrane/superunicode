/**
 * Test Suite: ExtSUCS Arbitrary-Precision Codepoints & vSUTF 0xFF Regime
 *
 * Verifies that ExtSUCS has no fixed-width upper bound: values far beyond
 * 0xFFFFFFFFFFFFFFFF round-trip through vSUTF, both with caller buffers and
 * through the streaming API.
 *
 * Host CRT is used for assertions only; the library remains freestanding.
 */

#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "extsucs_big.h"
#include "vsutf.h"

#define BIG_LIMBS 128

static void test_big_primitives(void) {
    uint32_t storage[BIG_LIMBS];
    sucs_ex_big_t v;
    assert(extsucs_big_init(&v, storage, BIG_LIMBS));

    assert(extsucs_big_assign_u64(&v, 0));
    assert(extsucs_big_is_zero(&v));
    assert(extsucs_big_bit_length(&v) == 0);
    assert(extsucs_big_byte_length(&v) == 0);

    assert(extsucs_big_assign_u64(&v, 0x41));
    assert(v.len == 1 && v.limbs[0] == 0x41);
    assert(extsucs_big_bit_length(&v) == 7);
    assert(extsucs_big_byte_length(&v) == 1);

    assert(extsucs_big_assign_u64(&v, 0xFFFFFFFFULL));
    assert(v.len == 1);
    assert(extsucs_big_byte_length(&v) == 4);

    assert(extsucs_big_assign_u64(&v, 0x100000000ULL));
    assert(v.len == 2);
    assert(v.limbs[0] == 0 && v.limbs[1] == 1);
    assert(extsucs_big_byte_length(&v) == 5);

    assert(extsucs_big_assign_u64(&v, 0xFFFFFFFFFFFFFFFFULL));
    uint64_t back = 0;
    assert(extsucs_big_to_u64(&v, &back));
    assert(back == 0xFFFFFFFFFFFFFFFFULL);

    /* Carry across limbs. */
    assert(extsucs_big_assign_u64(&v, 0xFFFFFFFFULL));
    assert(extsucs_big_add_u32(&v, 1));
    assert(v.len == 2 && v.limbs[0] == 0 && v.limbs[1] == 1);
    assert(extsucs_big_add_u32(&v, 0));

    /* Big-endian round-trip and slicing. */
    const uint8_t bytes[] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF, 0x10};
    assert(extsucs_big_assign_bytes_be(&v, bytes, sizeof(bytes)));
    uint8_t out[16];
    size_t out_len = 0;
    assert(extsucs_big_to_bytes_be(&v, out, sizeof(out), &out_len));
    assert(out_len == sizeof(bytes));
    assert(memcmp(out, bytes, sizeof(bytes)) == 0);
    assert(extsucs_big_bytes_be_slice(&v, 0, out, 3));
    assert(out[0] == 0x01 && out[1] == 0x23 && out[2] == 0x45);
    assert(extsucs_big_bytes_be_slice(&v, 6, out, 3));
    assert(out[0] == 0xCD && out[1] == 0xEF && out[2] == 0x10);
    assert(!extsucs_big_bytes_be_slice(&v, 7, out, 3));

    /* Leading zeros are canonicalised away. */
    const uint8_t padded[] = {0x00, 0x00, 0x00, 0x2A};
    assert(extsucs_big_assign_bytes_be(&v, padded, sizeof(padded)));
    assert(v.len == 1 && v.limbs[0] == 0x2A);

    printf("[PASS] test_big_primitives\n");
}

static void test_big_predicates(void) {
    uint32_t storage[BIG_LIMBS];
    sucs_ex_big_t v;
    assert(extsucs_big_init(&v, storage, BIG_LIMBS));

    assert(extsucs_big_assign_u64(&v, 0x7FFFFFF0ULL));
    assert(!extsucs_big_is_valid(&v));
    assert(extsucs_big_assign_u64(&v, 0x7FFFFFFEULL));
    assert(!extsucs_big_is_valid(&v));
    assert(extsucs_big_assign_u64(&v, 0x7FFFFFFFULL));
    assert(extsucs_big_is_valid(&v));
    assert(extsucs_big_is_base_sucs(&v));
    assert(extsucs_big_assign_u64(&v, 0x80000000ULL));
    assert(extsucs_big_is_valid(&v));
    assert(!extsucs_big_is_base_sucs(&v));

    /* Values above 2^64-1 are always valid: the trap range is far below. */
    uint8_t above64[9] = {0x01, 0, 0, 0, 0, 0, 0, 0, 0};
    assert(extsucs_big_assign_bytes_be(&v, above64, sizeof(above64)));
    assert(extsucs_big_is_valid(&v));
    assert(!extsucs_big_is_base_sucs(&v));

    uint32_t out_cp = 0;
    assert(extsucs_big_assign_u64(&v, 0x41));
    assert(extsucs_big_downcast(&v, &out_cp) && out_cp == 0x41);
    assert(extsucs_big_assign_u64(&v, 0x7FFFFFFFULL));
    assert(!extsucs_big_downcast(&v, &out_cp));
    assert(out_cp == 0x7FFFFFFFUL);
    assert(extsucs_big_assign_u64(&v, 0x7FFFFFF5ULL));
    assert(!extsucs_big_downcast(&v, &out_cp));
    assert(extsucs_big_assign_bytes_be(&v, above64, sizeof(above64)));
    assert(!extsucs_big_downcast(&v, &out_cp));

    printf("[PASS] test_big_predicates\n");
}

static void test_vsutf_big_roundtrip(void) {
    uint32_t storage[BIG_LIMBS];
    uint32_t decoded_storage[BIG_LIMBS];
    sucs_ex_big_t cp;
    sucs_ex_big_t decoded;
    assert(extsucs_big_init(&cp, storage, BIG_LIMBS));
    assert(extsucs_big_init(&decoded, decoded_storage, BIG_LIMBS));

    uint8_t enc[512];

    /* 2^64: the first value that does not fit the 64-bit fast path. */
    const uint8_t two64[9] = {0x01, 0, 0, 0, 0, 0, 0, 0, 0};
    assert(extsucs_big_assign_bytes_be(&cp, two64, sizeof(two64)));
    size_t n = vsutf_encode_big(&cp, enc, sizeof(enc));
    assert(n == 11);
    assert(enc[0] == 0xFF);
    assert(enc[1] == 9);
    assert(vsutf_big_codepoint_length(&cp) == n);

    size_t used = vsutf_decode_big(enc, n, &decoded);
    assert(used == n);
    assert(extsucs_big_compare(&cp, &decoded) == 0);

    /* 2^128 + 1: a comfortably multi-limb value. */
    uint8_t bytes128[17];
    memset(bytes128, 0, sizeof(bytes128));
    bytes128[0] = 0x01;
    bytes128[16] = 0x01;
    assert(extsucs_big_assign_bytes_be(&cp, bytes128, sizeof(bytes128)));
    n = vsutf_encode_big(&cp, enc, sizeof(enc));
    assert(n == 1 + 1 + 17);
    assert(enc[0] == 0xFF && enc[1] == 17);
    used = vsutf_decode_big(enc, n, &decoded);
    assert(used == n);
    assert(extsucs_big_compare(&cp, &decoded) == 0);

    /* A 300-byte value: no format-level maximum. */
    uint8_t big_bytes[300];
    for (size_t i = 0; i < sizeof(big_bytes); i++) {
        big_bytes[i] = (uint8_t)(i * 7u + 1u);
    }
    big_bytes[0] = 0xA5;
    assert(extsucs_big_assign_bytes_be(&cp, big_bytes, sizeof(big_bytes)));
    size_t expected = 1 + 2 + sizeof(big_bytes); /* prefix + LEB128(300) + payload */
    n = vsutf_encode_big(&cp, enc, sizeof(enc));
    assert(n == expected);
    assert(vsutf_big_codepoint_length(&cp) == expected);
    assert(enc[0] == 0xFF);
    assert(enc[1] == 0xAC && enc[2] == 0x02);
    used = vsutf_decode_big(enc, n, &decoded);
    assert(used == n);
    assert(extsucs_big_compare(&cp, &decoded) == 0);

    uint8_t out[300];
    size_t out_len = 0;
    assert(extsucs_big_to_bytes_be(&decoded, out, sizeof(out), &out_len));
    assert(out_len == sizeof(big_bytes));
    assert(memcmp(out, big_bytes, sizeof(big_bytes)) == 0);

    /* Small values delegate to the compact 64-bit regimes. */
    assert(extsucs_big_assign_u64(&cp, 0xFFFFFFFFFFFFFFFFULL));
    n = vsutf_encode_big(&cp, enc, sizeof(enc));
    assert(n == 9 && enc[0] == 0xFE);
    used = vsutf_decode_big(enc, n, &decoded);
    assert(used == n);
    assert(extsucs_big_compare(&cp, &decoded) == 0);

    assert(extsucs_big_assign_u64(&cp, 0x41));
    n = vsutf_encode_big(&cp, enc, sizeof(enc));
    assert(n == 1 && enc[0] == 0x41);
    used = vsutf_decode_big(enc, n, &decoded);
    assert(used == n);
    assert(extsucs_big_compare(&cp, &decoded) == 0);

    /* Trap addresses are not encodable. */
    assert(extsucs_big_assign_u64(&cp, 0x7FFFFFF5ULL));
    assert(vsutf_encode_big(&cp, enc, sizeof(enc)) == 0);
    assert(vsutf_big_codepoint_length(&cp) == 0);

    printf("[PASS] test_vsutf_big_roundtrip\n");
}

static void test_vsutf_big_canonical(void) {
    uint32_t storage[BIG_LIMBS];
    sucs_ex_big_t decoded;
    assert(extsucs_big_init(&decoded, storage, BIG_LIMBS));

    uint8_t buf[64];

    /* 0xFF with a length below the 64-bit threshold is non-canonical. */
    buf[0] = 0xFF;
    buf[1] = 0x01;
    buf[2] = 0x01;
    assert(vsutf_decode_big(buf, 3, &decoded) == 0);

    /* Overlong LEB128: 9 encoded as 0x89 0x00. */
    buf[0] = 0xFF;
    buf[1] = 0x89;
    buf[2] = 0x00;
    assert(vsutf_decode_big(buf, 3, &decoded) == 0);

    /* Leading zero in the payload. */
    buf[0] = 0xFF;
    buf[1] = 0x09;
    memset(buf + 2, 0, 9);
    assert(vsutf_decode_big(buf, 11, &decoded) == 0);

    /* Truncated payload.  Length says 9, only 8 bytes follow. */
    buf[0] = 0xFF;
    buf[1] = 0x09;
    buf[2] = 0x01;
    assert(vsutf_decode_big(buf, 10, &decoded) == 0);

    /* The 64-bit fast-path decoder still rejects 0xFF; it cannot hold it. */
    sucs_ex_char_t fast = 0;
    assert(vsutf_decode(buf, 11, &fast) == 0);

    /* Valid 2^64 frame. */
    buf[0] = 0xFF;
    buf[1] = 0x09;
    buf[2] = 0x01;
    memset(buf + 3, 0, 8);
    assert(vsutf_decode_big(buf, 11, &decoded) == 11);

    printf("[PASS] test_vsutf_big_canonical\n");
}

typedef struct {
    uint8_t data[512];
    size_t  len;
    size_t  pos;
} stream_ctx_t;

static bool test_sink(void* ctx, const uint8_t* data, size_t len) {
    stream_ctx_t* s = (stream_ctx_t*)ctx;
    if (s->len + len > sizeof(s->data)) {
        return false;
    }
    memcpy(s->data + s->len, data, len);
    s->len += len;
    return true;
}

static bool test_source(void* ctx, uint8_t* data, size_t len) {
    stream_ctx_t* s = (stream_ctx_t*)ctx;
    if (s->pos + len > s->len) {
        return false;
    }
    memcpy(data, s->data + s->pos, len);
    s->pos += len;
    return true;
}

static void test_vsutf_big_streaming(void) {
    uint32_t storage[BIG_LIMBS];
    uint32_t decoded_storage[BIG_LIMBS];
    sucs_ex_big_t cp;
    sucs_ex_big_t decoded;
    assert(extsucs_big_init(&cp, storage, BIG_LIMBS));
    assert(extsucs_big_init(&decoded, decoded_storage, BIG_LIMBS));

    uint8_t big_bytes[300];
    for (size_t i = 0; i < sizeof(big_bytes); i++) {
        big_bytes[i] = (uint8_t)(i * 13u + 7u);
    }
    big_bytes[0] = 0xC3;
    assert(extsucs_big_assign_bytes_be(&cp, big_bytes, sizeof(big_bytes)));

    uint8_t direct[512];
    size_t direct_len = vsutf_encode_big(&cp, direct, sizeof(direct));
    assert(direct_len > 0);

    stream_ctx_t sink;
    memset(&sink, 0, sizeof(sink));
    size_t streamed = 0;
    assert(vsutf_encode_big_stream(&cp, test_sink, &sink, &streamed));
    assert(streamed == direct_len);
    assert(sink.len == direct_len);
    assert(memcmp(sink.data, direct, direct_len) == 0);

    stream_ctx_t source;
    memset(&source, 0, sizeof(source));
    memcpy(source.data, sink.data, sink.len);
    source.len = sink.len;

    size_t consumed = 0;
    assert(vsutf_decode_big_stream(test_source, &source, &decoded, &consumed));
    assert(consumed == direct_len);
    assert(source.pos == direct_len);
    assert(extsucs_big_compare(&cp, &decoded) == 0);

    printf("[PASS] test_vsutf_big_streaming\n");
}

int main(void) {
    printf("=====================================================\n");
    printf(" RUNNING EXTSUCS ARBITRARY-PRECISION TESTS           \n");
    printf("=====================================================\n");
    test_big_primitives();
    test_big_predicates();
    test_vsutf_big_roundtrip();
    test_vsutf_big_canonical();
    test_vsutf_big_streaming();
    printf("=====================================================\n");
    printf(" ALL EXTSUCS ARBITRARY-PRECISION TESTS PASSED!       \n");
    printf("=====================================================\n");
    return 0;
}
