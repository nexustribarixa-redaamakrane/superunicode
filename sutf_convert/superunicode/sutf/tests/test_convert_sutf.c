/**
 * ConvertSUTF unit tests.
 *
 * Covers the full 31-bit Base SUCS space, the absence of surrogates,
 * canonical SUTF-8 framing, the 1/2-word SUTF-16 form, strict/lenient
 * error handling, target exhaustion, and legality helpers.
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "ConvertSUTF.h"
#include "sutf8.h"
#include "sutf16.h"

static const sucs_char_t kBoundaries[] = {
    0x00000000UL, 0x0000007FUL, 0x00000080UL, 0x000007FFUL,
    0x00000800UL, 0x0000FFFFUL, 0x00010000UL, 0x0010FFFFUL,
    0x00110000UL, 0x03FFFFFFUL, 0x04000000UL, 0x7FFFFFEFUL,
    0x0000D800UL, 0x0000DFFFUL, 0x0000FFFDUL
};

static void roundtrip_utf8(sucs_char_t cp) {
    SUTF32 source[1];
    SUTF8 encoded[16];
    SUTF32 decoded[1];
    const SUTF32* src32;
    const SUTF8* src8;
    SUTF32* out;
    SUTF8* dst;
    size_t length;
    ConversionResult result;

    source[0] = (SUTF32)cp;
    src32 = source;
    dst = encoded;
    result = ConvertSUTF32toSUTF8(&src32, source + 1, &dst, encoded + sizeof(encoded),
                                  strictConversion);
    assert(result == conversionOK);
    assert(src32 == source + 1);
    length = (size_t)(dst - encoded);
    assert(length == sutf8_codepoint_length(cp));

    src8 = encoded;
    out = decoded;
    result = ConvertSUTF8toSUTF32(&src8, encoded + length, &out, decoded + 1,
                                  strictConversion);
    assert(result == conversionOK);
    assert(src8 == encoded + length);
    assert(out == decoded + 1);
    assert(decoded[0] == (SUTF32)cp);
}

static void roundtrip_utf16(sucs_char_t cp) {
    SUTF32 source[1];
    SUTF16 encoded[4];
    SUTF32 decoded[1];
    const SUTF32* src32;
    const SUTF16* src16;
    SUTF32* out;
    SUTF16* dst;
    size_t length;
    ConversionResult result;

    source[0] = (SUTF32)cp;
    src32 = source;
    dst = encoded;
    result = ConvertSUTF32toSUTF16(&src32, source + 1, &dst, encoded + 4,
                                   strictConversion);
    assert(result == conversionOK);
    assert(src32 == source + 1);
    length = (size_t)(dst - encoded);
    assert(length == sutf16_codepoint_length(cp));

    src16 = encoded;
    out = decoded;
    result = ConvertSUTF16toSUTF32(&src16, encoded + length, &out, decoded + 1,
                                   strictConversion);
    assert(result == conversionOK);
    assert(src16 == encoded + length);
    assert(out == decoded + 1);
    assert(decoded[0] == (SUTF32)cp);
}

static void test_boundaries(void) {
    size_t i;
    for (i = 0; i < sizeof(kBoundaries) / sizeof(kBoundaries[0]); i++) {
        roundtrip_utf8(kBoundaries[i]);
        roundtrip_utf16(kBoundaries[i]);
    }
}

static void test_no_surrogates(void) {
    SUTF16 words[4];
    SUTF16* dst = words;
    SUTF32 source[1];
    const SUTF32* src = source;
    SUTF8 bytes[8];
    SUTF8* byte_dst = bytes;
    const SUTF8* byte_src;
    SUTF16 decoded[4];
    SUTF16* word_dst;
    ConversionResult result;

    /* 0xD800 is an ordinary PUA codepoint, not a surrogate half. */
    source[0] = 0x0000D800UL;
    result = ConvertSUTF32toSUTF16(&src, source + 1, &dst, words + 4, strictConversion);
    assert(result == conversionOK);
    assert(dst - words == 2);
    assert(words[0] == 0x8000U);
    assert(words[1] == 0xD800U);

    /* 0x7FFF is the largest one-word value. */
    src = source;
    source[0] = 0x00007FFFUL;
    dst = words;
    result = ConvertSUTF32toSUTF16(&src, source + 1, &dst, words + 4, strictConversion);
    assert(result == conversionOK);
    assert(dst - words == 1);
    assert(words[0] == 0x7FFFU);

    /* 0xFFFD is above 0x7FFF, so it uses the two-word form. */
    src = source;
    source[0] = (SUTF32)SUTF_REPLACEMENT_CHAR;
    dst = words;
    result = ConvertSUTF32toSUTF16(&src, source + 1, &dst, words + 4, strictConversion);
    assert(result == conversionOK);
    assert(dst - words == 2);
    assert(words[0] == 0x8000U);
    assert(words[1] == 0xFFFDU);

    /* SUTF-8 0xD800 is ED A0 80 and converts to {0x8000, 0xD800}. */
    {
        static const SUTF8 d800_bytes[3] = { 0xEDU, 0xA0U, 0x80U };
        byte_src = d800_bytes;
        word_dst = decoded;
        result = ConvertSUTF8toSUTF16(&byte_src, byte_src + 3, &word_dst, decoded + 4,
                                      strictConversion);
        assert(result == conversionOK);
        assert(byte_src == d800_bytes + 3);
    }
    assert(word_dst - decoded == 2);
    assert(decoded[0] == 0x8000U);
    assert(decoded[1] == 0xD800U);

    /* And back. */
    byte_src = bytes;
    {
        const SUTF16* ws = decoded;
        result = ConvertSUTF16toSUTF8(&ws, decoded + 2, &byte_dst, bytes + 8,
                                      strictConversion);
        assert(result == conversionOK);
        assert(byte_dst - bytes == 3);
        assert(memcmp(bytes, "\xED\xA0\x80", 3) == 0);
    }
}

static void test_strict_rejection(void) {
    static const sucs_char_t invalid[] = {
        0x7FFFFFF0UL, /* trap */
        0x7FFFFFFEUL, /* trap */
        0x7FFFFFFFUL, /* Sentinel */
        0x80000000UL  /* beyond Base SUCS */
    };
    size_t i;

    for (i = 0; i < sizeof(invalid) / sizeof(invalid[0]); i++) {
        SUTF32 source[1];
        SUTF8 encoded[8];
        const SUTF32* src;
        SUTF8* dst;
        ConversionResult result;

        source[0] = (SUTF32)invalid[i];
        src = source;
        dst = encoded;
        result = ConvertSUTF32toSUTF8(&src, source + 1, &dst, encoded + 8,
                                      strictConversion);
        assert(result == sourceIllegal);
        assert(src == source);
        assert(dst == encoded);

        /* Lenient mode emits one replacement character and consumes it. */
        src = source;
        dst = encoded;
        result = ConvertSUTF32toSUTF8(&src, source + 1, &dst, encoded + 8,
                                      lenientConversion);
        assert(result == conversionOK);
        assert(src == source + 1);
        assert(dst - encoded == 3);
        assert(memcmp(encoded, "\xEF\xBF\xBD", 3) == 0);
    }
}

static void test_utf16_illegal(void) {
    SUTF16 trap_words[2];
    const SUTF16* src;
    SUTF8 out[8];
    SUTF8* dst;
    ConversionResult result;
    SUTF16 lone[1];

    trap_words[0] = 0xFFFFU;
    trap_words[1] = 0xFFF0U; /* 0x7FFFFFF0 */
    src = trap_words;
    dst = out;
    result = ConvertSUTF16toSUTF8(&src, trap_words + 2, &dst, out + 8,
                                  strictConversion);
    assert(result == sourceIllegal);
    assert(src == trap_words);
    assert(dst == out);

    src = trap_words;
    dst = out;
    result = ConvertSUTF16toSUTF8(&src, trap_words + 2, &dst, out + 8,
                                  lenientConversion);
    assert(result == conversionOK);
    assert(src == trap_words + 2);
    assert(dst - out == 3);
    assert(memcmp(out, "\xEF\xBF\xBD", 3) == 0);

    /* A lone marker word is a truncated two-word sequence. */
    lone[0] = 0x8000U;
    src = lone;
    dst = out;
    result = ConvertSUTF16toSUTF8(&src, lone + 1, &dst, out + 8, strictConversion);
    assert(result == sourceExhausted);
    assert(src == lone);
    assert(dst == out);

    src = lone;
    dst = out;
    result = ConvertSUTF16toSUTF8(&src, lone + 1, &dst, out + 8, lenientConversion);
    assert(result == sourceExhausted);
    assert(src == lone + 1);
    assert(dst - out == 3);
    assert(memcmp(out, "\xEF\xBF\xBD", 3) == 0);
}

static void test_utf8_malformed(void) {
    SUTF8 bad[1];
    SUTF8 truncated[2];
    const SUTF8* src;
    SUTF16 out[4];
    SUTF16* dst;
    ConversionResult result;

    bad[0] = 0xFFU;
    src = bad;
    dst = out;
    result = ConvertSUTF8toSUTF16(&src, bad + 1, &dst, out + 4, strictConversion);
    assert(result == sourceIllegal);
    assert(src == bad);
    assert(dst == out);

    src = bad;
    dst = out;
    result = ConvertSUTF8toSUTF16(&src, bad + 1, &dst, out + 4, lenientConversion);
    assert(result == conversionOK);
    assert(src == bad + 1);
    assert(dst - out == 2);
    assert(out[0] == 0x8000U);
    assert(out[1] == 0xFFFDU);

    /* Truncated three-byte sequence. */
    truncated[0] = 0xE2U;
    truncated[1] = 0x82U;
    src = truncated;
    dst = out;
    result = ConvertSUTF8toSUTF16(&src, truncated + 2, &dst, out + 4, strictConversion);
    assert(result == sourceExhausted);
    assert(src == truncated);
    assert(dst == out);

    src = truncated;
    dst = out;
    result = ConvertSUTF8toSUTF16(&src, truncated + 2, &dst, out + 4, lenientConversion);
    assert(result == sourceExhausted);
    assert(src == truncated + 2);
    assert(dst - out == 2);
    assert(out[0] == 0x8000U);
    assert(out[1] == 0xFFFDU);
}

static void test_target_exhaustion(void) {
    SUTF32 source[1];
    const SUTF32* src;
    SUTF8 out8[3];
    SUTF8* dst8;
    SUTF16 out16[1];
    SUTF16* dst16;
    ConversionResult result;

    /* U+10000 needs four SUTF-8 bytes. */
    source[0] = 0x00010000UL;
    src = source;
    dst8 = out8;
    result = ConvertSUTF32toSUTF8(&src, source + 1, &dst8, out8 + 3, strictConversion);
    assert(result == targetExhausted);
    assert(src == source);
    assert(dst8 == out8);

    /* 0x8000 needs two SUTF-16 words. */
    source[0] = 0x00008000UL;
    src = source;
    dst16 = out16;
    result = ConvertSUTF32toSUTF16(&src, source + 1, &dst16, out16 + 1, strictConversion);
    assert(result == targetExhausted);
    assert(src == source);
    assert(dst16 == out16);
}

static void test_legality(void) {
    SUTF8 six[6];
    SUTF16 two[2];
    SUTF32 valid[3];
    SUTF32 illegal[1];
    SUTF8 overlong8[2];
    SUTF16 overlong16[2];
    SUTF8 truncated[2];
    size_t n;

    n = sutf8_encode_char(0x7FFFFFEFUL, six, sizeof(six));
    assert(n == 6);
    assert(isLegalSUTF8Sequence(six, six + n));

    n = sutf16_encode_char(0x0000D800UL, two, 2);
    assert(n == 2);
    assert(isLegalSUTF16Sequence(two, two + n));

    valid[0] = 0x00000000UL;
    valid[1] = 0x0000D800UL;
    valid[2] = 0x7FFFFFEFUL;
    assert(isLegalSUTF32Sequence(valid, valid + 3));

    illegal[0] = 0x7FFFFFF0UL;
    assert(!isLegalSUTF32Sequence(illegal, illegal + 1));

    overlong8[0] = 0xC0U;
    overlong8[1] = 0x80U;
    assert(!isLegalSUTF8Sequence(overlong8, overlong8 + 2));

    overlong16[0] = 0x8000U;
    overlong16[1] = 0x0000U;
    assert(!isLegalSUTF16Sequence(overlong16, overlong16 + 2));

    truncated[0] = 0xE2U;
    truncated[1] = 0x82U;
    assert(!isLegalSUTF8Sequence(truncated, truncated + 2));
}

int main(void) {
    test_boundaries();
    test_no_surrogates();
    test_strict_rejection();
    test_utf16_illegal();
    test_utf8_malformed();
    test_target_exhaustion();
    test_legality();

    printf("[PASS] test_convert_sutf\n");
    return 0;
}
