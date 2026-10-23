/**
 * ConvertSUST unit tests.
 *
 * Covers Base SUCS <-> SUST-16 <-> SUST-32 conversion, canonical big-endian
 * framing, PUA (non-surrogate) round-trips, trap/Sentinel rejection,
 * truncation, target exhaustion, and legality helpers.
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "ConvertSUST.h"

static const sucs_char_t kBoundaries[] = {
    0x00000000UL, 0x0000007FUL, 0x00000080UL, 0x000007FFUL,
    0x00000800UL, 0x00007FFFUL, 0x00008000UL, 0x0000D800UL,
    0x0000DFFFUL, 0x0000FFFFUL, 0x0010FFFFUL, 0x00110000UL,
    0x7FFFFFEFUL
};

static void roundtrip_sust16(sucs_char_t cp) {
    sucs_char_t source[1];
    SUST16 encoded[8];
    sucs_char_t decoded[1];
    const sucs_char_t* src;
    sucs_char_t* out;
    SUST16* dst;
    const SUST16* src16;
    size_t length;
    ConversionResult result;

    source[0] = cp;
    src = source;
    dst = encoded;
    result = ConvertSUCStoSUST16(&src, source + 1, &dst, encoded + 8, strictConversion);
    assert(result == conversionOK);
    assert(src == source + 1);
    length = (size_t)(dst - encoded);
    assert(length == sust16_codepoint_bytes(cp));

    src16 = encoded;
    out = decoded;
    result = ConvertSUST16toSUCS(&src16, encoded + length, &out, decoded + 1,
                                 strictConversion);
    assert(result == conversionOK);
    assert(src16 == encoded + length);
    assert(out == decoded + 1);
    assert(decoded[0] == cp);
}

static void roundtrip_sust32(sucs_char_t cp) {
    sucs_char_t source[1];
    SUST32 encoded[8];
    sucs_char_t decoded[1];
    const sucs_char_t* src;
    sucs_char_t* out;
    SUST32* dst;
    const SUST32* src32;
    size_t length;
    ConversionResult result;

    source[0] = cp;
    src = source;
    dst = encoded;
    result = ConvertSUCStoSUST32(&src, source + 1, &dst, encoded + 8, strictConversion);
    assert(result == conversionOK);
    assert(src == source + 1);
    length = (size_t)(dst - encoded);
    assert(length == SUST32_BYTES);

    src32 = encoded;
    out = decoded;
    result = ConvertSUST32toSUCS(&src32, encoded + length, &out, decoded + 1,
                                 strictConversion);
    assert(result == conversionOK);
    assert(src32 == encoded + length);
    assert(out == decoded + 1);
    assert(decoded[0] == cp);
}

static void test_boundaries(void) {
    size_t i;
    for (i = 0; i < sizeof(kBoundaries) / sizeof(kBoundaries[0]); i++) {
        roundtrip_sust16(kBoundaries[i]);
        roundtrip_sust32(kBoundaries[i]);
    }
}

static void test_exact_framing(void) {
    static const sucs_char_t cps[] = {
        0x00007FFFUL, 0x00008000UL, 0x0000D800UL,
        0x0000FFFDUL, 0x7FFFFFEFUL
    };
    static const uint8_t expected16[][4] = {
        { 0x7F, 0xFF, 0x00, 0x00 },
        { 0x80, 0x00, 0x80, 0x00 },
        { 0x80, 0x00, 0xD8, 0x00 },
        { 0x80, 0x00, 0xFF, 0xFD },
        { 0xFF, 0xFF, 0xFF, 0xEF }
    };
    static const uint8_t expected32[][4] = {
        { 0x00, 0x00, 0x7F, 0xFF },
        { 0x00, 0x00, 0x80, 0x00 },
        { 0x00, 0x00, 0xD8, 0x00 },
        { 0x00, 0x00, 0xFF, 0xFD },
        { 0x7F, 0xFF, 0xFF, 0xEF }
    };
    size_t i;

    for (i = 0; i < sizeof(cps) / sizeof(cps[0]); i++) {
        SUST16 out16[4];
        SUST32 out32[4];
        SUST16* d16 = out16;
        SUST32* d32 = out32;
        const sucs_char_t* src = cps + i;
        size_t n16;
        size_t n32;

        n16 = sust16_codepoint_bytes(cps[i]);
        assert(ConvertSUCStoSUST16(&src, cps + i + 1, &d16, out16 + 4,
                                   strictConversion) == conversionOK);
        assert((size_t)(d16 - out16) == n16);
        assert(memcmp(out16, expected16[i], n16) == 0);

        src = cps + i;
        assert(ConvertSUCStoSUST32(&src, cps + i + 1, &d32, out32 + 4,
                                   strictConversion) == conversionOK);
        n32 = (size_t)(d32 - out32);
        assert(n32 == SUST32_BYTES);
        assert(memcmp(out32, expected32[i], n32) == 0);
    }
}

static void test_transport_roundtrip(void) {
    static const uint8_t stream16[] = {
        0x80, 0x00, 0xD8, 0x00,   /* 0xD800 */
        0x7F, 0xFF,               /* 0x7FFF */
        0x80, 0x00, 0xFF, 0xFD    /* 0xFFFD */
    };
    SUST32 framed[24];
    SUST32* frame_dst = framed;
    SUST16 back[16];
    SUST16* back_dst = back;
    const SUST16* src16 = stream16;
    const SUST32* src32;
    ConversionResult result;
    size_t framed_len;
    size_t back_len;

    result = ConvertSUST16toSUST32(&src16, stream16 + sizeof(stream16),
                                   &frame_dst, framed + 24, strictConversion);
    assert(result == conversionOK);
    assert(src16 == stream16 + sizeof(stream16));
    framed_len = (size_t)(frame_dst - framed);
    assert(framed_len == 12);
    assert(memcmp(framed + 0, "\x00\x00\xD8\x00", 4) == 0);
    assert(memcmp(framed + 4, "\x00\x00\x7F\xFF", 4) == 0);
    assert(memcmp(framed + 8, "\x00\x00\xFF\xFD", 4) == 0);

    src32 = framed;
    result = ConvertSUST32toSUST16(&src32, framed + framed_len, &back_dst,
                                   back + 16, strictConversion);
    assert(result == conversionOK);
    assert(src32 == framed + framed_len);
    back_len = (size_t)(back_dst - back);
    assert(back_len == sizeof(stream16));
    assert(memcmp(back, stream16, sizeof(stream16)) == 0);
}

static void test_invalid_and_overflow(void) {
    static const uint8_t trap32[] = { 0x7F, 0xFF, 0xFF, 0xF0 };
    static const uint8_t sentinel32[] = { 0x7F, 0xFF, 0xFF, 0xFF };
    static const uint8_t trap16[] = { 0xFF, 0xFF, 0xFF, 0xF0 };
    const SUST32* src32;
    const SUST16* src16;
    sucs_char_t out[4];
    sucs_char_t* dst;
    ConversionResult result;

    src32 = trap32;
    dst = out;
    result = ConvertSUST32toSUCS(&src32, trap32 + 4, &dst, out + 4, strictConversion);
    assert(result == sourceIllegal);
    assert(src32 == trap32);
    assert(dst == out);

    src32 = trap32;
    dst = out;
    result = ConvertSUST32toSUCS(&src32, trap32 + 4, &dst, out + 4, lenientConversion);
    assert(result == conversionOK);
    assert(src32 == trap32 + 4);
    assert(dst - out == 1);
    assert(out[0] == SUST_REPLACEMENT_CHAR);

    src32 = sentinel32;
    dst = out;
    result = ConvertSUST32toSUCS(&src32, sentinel32 + 4, &dst, out + 4,
                                 strictConversion);
    assert(result == sourceIllegal);
    assert(src32 == sentinel32);

    src16 = trap16;
    dst = out;
    result = ConvertSUST16toSUCS(&src16, trap16 + 4, &dst, out + 4, strictConversion);
    assert(result == sourceIllegal);
    assert(src16 == trap16);

    src16 = trap16;
    dst = out;
    result = ConvertSUST16toSUCS(&src16, trap16 + 4, &dst, out + 4, lenientConversion);
    assert(result == conversionOK);
    assert(src16 == trap16 + 4);
    assert(dst - out == 1);
    assert(out[0] == SUST_REPLACEMENT_CHAR);
}

static void test_truncation(void) {
    static const uint8_t short16[] = { 0x80, 0x00 };
    static const uint8_t short32[] = { 0x00, 0x00 };
    const SUST16* src16;
    const SUST32* src32;
    sucs_char_t out[4];
    sucs_char_t* dst;
    ConversionResult result;

    src16 = short16;
    dst = out;
    result = ConvertSUST16toSUCS(&src16, short16 + 2, &dst, out + 4, strictConversion);
    assert(result == sourceExhausted);
    assert(src16 == short16);
    assert(dst == out);

    src16 = short16;
    dst = out;
    result = ConvertSUST16toSUCS(&src16, short16 + 2, &dst, out + 4, lenientConversion);
    assert(result == sourceExhausted);
    assert(src16 == short16 + 2);
    assert(dst - out == 1);
    assert(out[0] == SUST_REPLACEMENT_CHAR);

    src32 = short32;
    dst = out;
    result = ConvertSUST32toSUCS(&src32, short32 + 2, &dst, out + 4, strictConversion);
    assert(result == sourceExhausted);
    assert(src32 == short32);

    src32 = short32;
    dst = out;
    result = ConvertSUST32toSUCS(&src32, short32 + 2, &dst, out + 4, lenientConversion);
    assert(result == sourceExhausted);
    assert(src32 == short32 + 2);
    assert(dst - out == 1);
    assert(out[0] == SUST_REPLACEMENT_CHAR);
}

static void test_target_exhaustion(void) {
    sucs_char_t source[1];
    const sucs_char_t* src;
    SUST16 out16[2];
    SUST16* dst16;
    SUST32 out32[3];
    SUST32* dst32;
    ConversionResult result;

    source[0] = 0x00008000UL; /* four SUST-16 bytes */
    src = source;
    dst16 = out16;
    result = ConvertSUCStoSUST16(&src, source + 1, &dst16, out16 + 2, strictConversion);
    assert(result == targetExhausted);
    assert(src == source);
    assert(dst16 == out16);

    source[0] = 0x00001234UL; /* four SUST-32 bytes */
    src = source;
    dst32 = out32;
    result = ConvertSUCStoSUST32(&src, source + 1, &dst32, out32 + 3, strictConversion);
    assert(result == targetExhausted);
    assert(src == source);
    assert(dst32 == out32);
}

static void test_legality(void) {
    static const uint8_t valid16[] = { 0x80, 0x00, 0xD8, 0x00, 0x7F, 0xFF };
    static const uint8_t overlong16[] = { 0x80, 0x00, 0x00, 0x01 };
    static const uint8_t trap16[] = { 0xFF, 0xFF, 0xFF, 0xF0 };
    static const uint8_t short16[] = { 0x80, 0x00 };
    static const uint8_t valid32[] = { 0x00, 0x00, 0xD8, 0x00 };
    static const uint8_t sentinel32[] = { 0x7F, 0xFF, 0xFF, 0xFF };
    static const uint8_t short32[] = { 0x00 };

    assert(isLegalSUST16Sequence(valid16, valid16 + sizeof(valid16)));
    assert(!isLegalSUST16Sequence(overlong16, overlong16 + sizeof(overlong16)));
    assert(!isLegalSUST16Sequence(trap16, trap16 + sizeof(trap16)));
    assert(!isLegalSUST16Sequence(short16, short16 + sizeof(short16)));

    assert(isLegalSUST32Sequence(valid32, valid32 + sizeof(valid32)));
    assert(!isLegalSUST32Sequence(sentinel32, sentinel32 + sizeof(sentinel32)));
    assert(!isLegalSUST32Sequence(short32, short32 + sizeof(short32)));
}

int main(void) {
    test_boundaries();
    test_exact_framing();
    test_transport_roundtrip();
    test_invalid_and_overflow();
    test_truncation();
    test_target_exhaustion();
    test_legality();

    printf("[PASS] test_convert_sust\n");
    return 0;
}
