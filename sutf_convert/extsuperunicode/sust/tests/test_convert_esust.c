/**
 * ConvertESUST unit tests.
 *
 * Covers E-SUST-32 (4-byte) and E-SUST-64 (8-byte) frames, ExtSUCS validity
 * including 0x7FFFFFFF, downcast overflow, trap rejection, truncation/target
 * exhaustion, and legality.
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "ConvertESUST.h"

static const sucs_ex_char_t kBoundaries[] = {
    0x00000000ULL,
    0x0000007FULL,
    0x00000080ULL,
    0x000007FFULL,
    0x00000800ULL,
    0x0000FFFFULL,
    0x00010000ULL,
    0x0010FFFFULL,
    0x00110000ULL,
    0x03FFFFFFULL,
    0x04000000ULL,
    0x7FFFFFFFULL,  /* valid in ExtSUCS */
    0x80000000ULL,
    0xFFFFFFFFULL,
    0x100000000ULL
};

static void roundtrip_sust32(sucs_ex_char_t cp) {
    sucs_ex_char_t source[1];
    ESUST32 encoded[8];
    sucs_ex_char_t decoded[1];
    const sucs_ex_char_t* src;
    sucs_ex_char_t* out;
    ESUST32* dst;
    const ESUST32* src32;
    size_t length;
    ConversionResult result;

    source[0] = cp;
    src = source;
    dst = encoded;
    result = ConvertESUCStoESUST32(&src, source + 1, &dst, encoded + 8, strictConversion);
    assert(result == conversionOK);
    assert(src == source + 1);
    length = (size_t)(dst - encoded);
    assert(length == ESUST32_FRAME_BYTES);

    src32 = encoded;
    out = decoded;
    result = ConvertESUST32toESUCS(&src32, encoded + length, &out, decoded + 1,
                                    strictConversion);
    assert(result == conversionOK);
    assert(src32 == encoded + length);
    assert(out == decoded + 1);
    assert(decoded[0] == cp);
}

static void roundtrip_sust64(sucs_ex_char_t cp) {
    sucs_ex_char_t source[1];
    ESUST64 encoded[8];
    sucs_ex_char_t decoded[1];
    const sucs_ex_char_t* src;
    sucs_ex_char_t* out;
    ESUST64* dst;
    const ESUST64* src64;
    size_t length;
    ConversionResult result;

    source[0] = cp;
    src = source;
    dst = encoded;
    result = ConvertESUCStoESUST64(&src, source + 1, &dst, encoded + 8, strictConversion);
    assert(result == conversionOK);
    assert(src == source + 1);
    length = (size_t)(dst - encoded);
    assert(length == ESUST64_FRAME_BYTES);

    src64 = encoded;
    out = decoded;
    result = ConvertESUST64toESUCS(&src64, encoded + length, &out, decoded + 1,
                                    strictConversion);
    assert(result == conversionOK);
    assert(src64 == encoded + length);
    assert(out == decoded + 1);
    assert(decoded[0] == cp);
}

static void test_boundaries(void) {
    size_t i;
    for (i = 0; i < sizeof(kBoundaries) / sizeof(kBoundaries[0]); i++) {
        /* E-SUST-32 tops out at 0xFFFFFFFF; larger values need E-SUST-64. */
        if (kBoundaries[i] <= ESUST32_MAX) {
            roundtrip_sust32(kBoundaries[i]);
        }
        roundtrip_sust64(kBoundaries[i]);
    }
}

static void test_exact_framing(void) {
    /* 0x7FFFFFFF in E-SUST-32: 7F FF FF FF */
    {
        sucs_ex_char_t cp = 0x7FFFFFFFULL;
        ESUST32 out[4];
        ESUST32* d = out;
        const sucs_ex_char_t* src = &cp;
        ConversionResult r = ConvertESUCStoESUST32(&src, &cp + 1, &d, out + 4, strictConversion);
        assert(r == conversionOK);
        assert((size_t)(d - out) == 4);
        assert(out[0] == 0x7FU && out[1] == 0xFFU && out[2] == 0xFFU && out[3] == 0xFFU);

        const ESUST32* s2 = out;
        sucs_ex_char_t back[1];
        sucs_ex_char_t* b = back;
        r = ConvertESUST32toESUCS(&s2, out + 4, &b, back + 1, strictConversion);
        assert(r == conversionOK);
        assert(back[0] == 0x7FFFFFFFULL);
    }

    /* 0x100000000 in E-SUST-64: 00 00 00 01 00 00 00 00 */
    {
        sucs_ex_char_t cp = 0x100000000ULL;
        ESUST64 out[8];
        ESUST64* d = out;
        const sucs_ex_char_t* src = &cp;
        ConversionResult r = ConvertESUCStoESUST64(&src, &cp + 1, &d, out + 8, strictConversion);
        assert(r == conversionOK);
        assert((size_t)(d - out) == 8);
        assert(out[0] == 0x00U && out[1] == 0x00U && out[2] == 0x00U && out[3] == 0x01U);
        assert(out[4] == 0x00U && out[5] == 0x00U && out[6] == 0x00U && out[7] == 0x00U);
    }
}

static void test_transport_roundtrip(void) {
    /* E-SUST-32 can represent values through 0xFFFFFFFF only. */
    static const sucs_ex_char_t stream[] = { 0x00000001ULL, 0x7FFFFFFFULL };
    ESUST32 f32[24];
    ESUST32* f32d = f32;
    const sucs_ex_char_t* src = stream;
    ConversionResult r = ConvertESUCStoESUST32(&src, stream + 2, &f32d, f32 + 24, strictConversion);
    assert(r == conversionOK);
    assert(src == stream + 2);
    size_t len32 = (size_t)(f32d - f32);
    assert(len32 == 8);

    /* to ESUST64 */
    ESUST64 f64[32];
    ESUST64* f64d = f64;
    const ESUST32* s32 = f32;
    r = ConvertESUST32toESUST64(&s32, f32 + len32, &f64d, f64 + 32, strictConversion);
    assert(r == conversionOK);
    assert(s32 == f32 + len32);
    size_t len64 = (size_t)(f64d - f64);
    assert(len64 == 16);

    /* back to ESUST32 */
    ESUST32 b32[24];
    ESUST32* b32d = b32;
    const ESUST64* s64 = f64;
    r = ConvertESUST64toESUST32(&s64, f64 + len64, &b32d, b32 + 24, strictConversion);
    assert(r == conversionOK);
    assert(s64 == f64 + len64);
    size_t blen = (size_t)(b32d - b32);
    assert(blen == len32);
    assert(memcmp(b32, f32, len32) == 0);

    /* A value above 0xFFFFFFFF round-trips through E-SUST-64 only. */
    {
        sucs_ex_char_t big = 0x100000000ULL;
        const sucs_ex_char_t* sb = &big;
        ESUST64 big64[8];
        ESUST64* bd = big64;
        sucs_ex_char_t back[1];
        sucs_ex_char_t* bp = back;
        const ESUST64* bs;

        r = ConvertESUCStoESUST64(&sb, &big + 1, &bd, big64 + 8, strictConversion);
        assert(r == conversionOK);
        assert((size_t)(bd - big64) == 8);

        bs = big64;
        r = ConvertESUST64toESUCS(&bs, big64 + 8, &bp, back + 1, strictConversion);
        assert(r == conversionOK);
        assert(back[0] == big);
    }
}

static void test_invalid_and_overflow(void) {
    static const uint8_t trap32[] = { 0x7F, 0xFF, 0xFF, 0xF0 };
    static const uint8_t trap64[] = { 0x00, 0x00, 0x00, 0x00, 0x7F, 0xFF, 0xFF, 0xF0 };
    const ESUST32* s32;
    const ESUST64* s64;
    sucs_ex_char_t out[4];
    sucs_ex_char_t* d;
    ConversionResult r;

    /* E-SUST-32 trap */
    s32 = trap32;
    d = out;
    r = ConvertESUST32toESUCS(&s32, trap32 + 4, &d, out + 4, strictConversion);
    assert(r == sourceIllegal);
    assert(s32 == trap32);
    s32 = trap32;
    d = out;
    r = ConvertESUST32toESUCS(&s32, trap32 + 4, &d, out + 4, lenientConversion);
    assert(r == conversionOK);
    assert(s32 == trap32 + 4);
    assert(d == out + 1);
    assert(out[0] == (sucs_ex_char_t)ESUST_REPLACEMENT_CHAR);

    /* E-SUST-64 trap */
    s64 = trap64;
    d = out;
    r = ConvertESUST64toESUCS(&s64, trap64 + 8, &d, out + 4, strictConversion);
    assert(r == sourceIllegal);
    s64 = trap64;
    d = out;
    r = ConvertESUST64toESUCS(&s64, trap64 + 8, &d, out + 4, lenientConversion);
    assert(r == conversionOK);
    assert(s64 == trap64 + 8);
    assert(out[0] == (sucs_ex_char_t)ESUST_REPLACEMENT_CHAR);

    /* downcast overflow ESUST64 -> ESUST32 */
    sucs_ex_char_t bigv = 0x100000000ULL;
    ESUST64 f64[8];
    ESUST64* sfw = f64;
    ESUST32 f32[4];
    ESUST32* df = f32;
    const sucs_ex_char_t* sb = &bigv;
    r = ConvertESUCStoESUST64(&sb, &bigv + 1, &sfw, f64 + 8, strictConversion);
    assert(r == conversionOK);
    /* convert back as stream to ESUST32 */
    {
        const ESUST64* sf = f64;
        df = f32;
        r = ConvertESUST64toESUST32(&sf, f64 + 8, &df, f32 + 4, strictConversion);
        assert(r == sourceIllegal);
    }
    {
        const ESUST64* sf = f64;
        df = f32;
        r = ConvertESUST64toESUST32(&sf, f64 + 8, &df, f32 + 4, lenientConversion);
        assert(r == conversionOK);
        assert(sf == f64 + 8);
        assert(df == f32 + ESUST32_FRAME_BYTES);
        /* E-SUST-32 replacement frame: big-endian 0x0000FFFD. */
        assert(f32[0] == 0x00U && f32[1] == 0x00U &&
               f32[2] == 0xFFU && f32[3] == 0xFDU);
    }
}

static void test_truncation_and_target(void) {
    static const uint8_t short32[] = { 0x00, 0x01, 0x02 };
    static const uint8_t short64[] = { 0x00, 0x01 };
    const ESUST32* s32 = short32;
    const ESUST64* s64 = short64;
    sucs_ex_char_t out[4];
    sucs_ex_char_t* d = out;
    ConversionResult r;

    r = ConvertESUST32toESUCS(&s32, short32 + 3, &d, out + 4, strictConversion);
    assert(r == sourceExhausted);
    assert(s32 == short32);
    s32 = short32;
    d = out;
    r = ConvertESUST32toESUCS(&s32, short32 + 3, &d, out + 4, lenientConversion);
    assert(r == sourceExhausted);
    assert(s32 == short32 + 3);
    assert(out[0] == (sucs_ex_char_t)ESUST_REPLACEMENT_CHAR);

    s64 = short64;
    d = out;
    r = ConvertESUST64toESUCS(&s64, short64 + 2, &d, out + 4, strictConversion);
    assert(r == sourceExhausted);
    s64 = short64;
    d = out;
    r = ConvertESUST64toESUCS(&s64, short64 + 2, &d, out + 4, lenientConversion);
    assert(r == sourceExhausted);
    assert(out[0] == (sucs_ex_char_t)ESUST_REPLACEMENT_CHAR);

    /* target exhaustion */
    sucs_ex_char_t cp = 0x1234ULL;
    const sucs_ex_char_t* sc = &cp;
    ESUST32 f32[4];
    ESUST32* df2 = f32;
    r = ConvertESUCStoESUST32(&sc, &cp + 1, &df2, f32 + 0, strictConversion);
    assert(r == targetExhausted);
    assert(sc == &cp);
}

static void test_legality(void) {
    static const uint8_t good32[] = { 0x00, 0x00, 0x00, 0x01, 0x7F, 0xFF, 0xFF, 0xFF };
    static const uint8_t trap32[] = { 0x7F, 0xFF, 0xFF, 0xF0 };
    static const uint8_t short32[] = { 0x00, 0x01 };
    static const uint8_t good64[] = { 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00 };

    assert(isLegalESUST32Sequence(good32, good32 + sizeof(good32)));
    assert(!isLegalESUST32Sequence(trap32, trap32 + sizeof(trap32)));
    assert(!isLegalESUST32Sequence(short32, short32 + sizeof(short32)));
    assert(isLegalESUST64Sequence(good64, good64 + sizeof(good64)));
}

int main(void) {
    test_boundaries();
    test_exact_framing();
    test_transport_roundtrip();
    test_invalid_and_overflow();
    test_truncation_and_target();
    test_legality();

    printf("[PASS] test_convert_esust\n");
    return 0;
}
