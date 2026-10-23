/**
 * ConvertESUTF unit tests.
 *
 * Covers vSUTF base/extended regimes, ESUTF32/ESUTF64 compatibility views,
 * arbitrary-precision conversions, trap rejection, no 0x10FFFF ceiling,
 * truncation/target exhaustion, and legality.
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "ConvertESUTF.h"
#include "extsucs_big.h"
#include "vsutf.h"

/* Trap range in ExtSUCS */
#define ESUTF_TRAP_MIN 0x7FFFFFF0ULL
#define ESUTF_TRAP_MAX 0x7FFFFFFEULL

static void test_vbase_roundtrips(void) {
    sucs_ex_char_t cps[] = {
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
        0x7FFFFFFFULL,  /* valid in ExtSUCS, sentinel in Base */
        0x80000000ULL,
        0xFFFFFFFFULL
    };
    size_t i;

    for (i = 0; i < sizeof(cps) / sizeof(cps[0]); i++) {
        sucs_ex_char_t cp = cps[i];
        ESUTF8 buf[16];
        ESUTF8* dst = buf;
        const ESUTF8* src = buf;
        ESUTF64 out64[4];
        ESUTF64* d64 = out64;
        sucs_ex_char_t dval = 0;

        size_t n = vsutf_encode(cp, buf, sizeof(buf));
        /* 1-6 bytes in the base regime, 0xFE + 8 bytes above 0x7FFFFFFF. */
        assert(n > 0 && n <= VSUTF_EXT64_TOTAL);

        /* ConvertESUTF8toESUTF64 */
        src = buf;
        d64 = out64;
        ConversionResult r = ConvertESUTF8toESUTF64(&src, buf + n, &d64, out64 + 4, strictConversion);
        assert(r == conversionOK);
        assert(src == buf + n);
        assert(d64 == out64 + 1);
        assert(out64[0] == (ESUTF64)cp);

        /* Back */
        {
            const ESUTF64* s64 = out64;
            ESUTF8 bout[16];
            ESUTF8* bd = bout;
            r = ConvertESUTF64toESUTF8(&s64, out64 + 1, &bd, bout + sizeof(bout), strictConversion);
            assert(r == conversionOK);
            assert(s64 == out64 + 1);
            assert(bd == bout + n);
            assert(memcmp(bout, buf, n) == 0);
        }
        (void)dval;
    }
}

static void test_esutf32_view(void) {
    sucs_ex_char_t cp = 0x12345678ULL;
    ESUTF8 buf[12];
    size_t n = vsutf_encode(cp, buf, sizeof(buf));
    assert(n == 6); /* above 0x03FFFFFF, below 0x80000000 */

    {
        const ESUTF8* src = buf;
        ESUTF32 out32[4];
        ESUTF32* d32 = out32;
        ConversionResult r = ConvertESUTF8toESUTF32(&src, buf + n, &d32, out32 + 4, strictConversion);
        assert(r == conversionOK);
        assert(out32[0] == (ESUTF32)cp);
    }

    /* Above 0xFFFFFFFF is unrepresentable in ESUTF32 view */
    cp = 0x100000000ULL; /* 2^32 */
    n = vsutf_encode(cp, buf, sizeof(buf));
    assert(n == VSUTF_EXT64_TOTAL); /* 0xFE + 8 bytes */
    {
        const ESUTF8* src = buf;
        ESUTF32 out32[4];
        ESUTF32* d32 = out32;
        ConversionResult r = ConvertESUTF8toESUTF32(&src, buf + n, &d32, out32 + 4, strictConversion);
        assert(r == sourceIllegal);
        assert(src == buf); /* no consumption */
        assert(d32 == out32);

        src = buf;
        d32 = out32;
        r = ConvertESUTF8toESUTF32(&src, buf + n, &d32, out32 + 4, lenientConversion);
        assert(r == conversionOK);
        assert(src == buf + n);
        assert(d32 == out32 + 1);
        assert(out32[0] == (ESUTF32)ESUTF_REPLACEMENT_CHAR);
    }

    /* ESUTF64 -> ESUTF32 overflow */
    {
        ESUTF64 s64 = 0x100000000ULL;
        const ESUTF64* ss = &s64;
        ESUTF32 out32[4];
        ESUTF32* d32 = out32;
        ConversionResult r = ConvertESUTF64toESUTF32(&ss, &s64 + 1, &d32, out32 + 4, strictConversion);
        assert(r == sourceIllegal);
        assert(ss == &s64);
        ss = &s64;
        d32 = out32;
        r = ConvertESUTF64toESUTF32(&ss, &s64 + 1, &d32, out32 + 4, lenientConversion);
        assert(r == conversionOK);
        assert(ss == &s64 + 1);
        assert(d32 == out32 + 1);
        assert(out32[0] == (ESUTF32)ESUTF_REPLACEMENT_CHAR);
    }
}

static void test_trap_and_sentinel(void) {
    /* Trap range illegal in ExtSUCS. vsutf_encode() refuses to emit the trap
       range, so the six-byte trap frame is written directly:
       ESUTF_TRAP_MIN (0x7FFFFFF0) -> FD BF BF BF BF B0. */
    {
        static const ESUTF8 trap_frame[6] = { 0xFD, 0xBF, 0xBF, 0xBF, 0xBF, 0xB0 };
        const size_t n = sizeof(trap_frame);
        const ESUTF8* src = trap_frame;
        ESUTF64 out64[4];
        ESUTF64* d64 = out64;
        ConversionResult r = ConvertESUTF8toESUTF64(&src, trap_frame + n, &d64, out64 + 4, strictConversion);
        assert(r == sourceIllegal);
        assert(src == trap_frame);
        src = trap_frame;
        d64 = out64;
        r = ConvertESUTF8toESUTF64(&src, trap_frame + n, &d64, out64 + 4, lenientConversion);
        assert(r == conversionOK);
        assert(src == trap_frame + n);
        assert(d64 == out64 + 1);
        assert(out64[0] == (ESUTF64)ESUTF_REPLACEMENT_CHAR);
    }

    /* 0x7FFFFFFF is valid in ExtSUCS */
    {
        sucs_ex_char_t sent = 0x7FFFFFFFULL;
        ESUTF8 buf[16];
        size_t n = vsutf_encode(sent, buf, sizeof(buf));
        assert(n == 6);
        const ESUTF8* src = buf;
        ESUTF64 out64[4];
        ESUTF64* d64 = out64;
        ConversionResult r = ConvertESUTF8toESUTF64(&src, buf + n, &d64, out64 + 4, strictConversion);
        assert(r == conversionOK);
        assert(out64[0] == sent);
    }
}

static void test_big_arbitrary(void) {
    /* 2^64 as big frame: FF 09 01 00 00 00 00 00 00 00 00 */
    static const uint8_t big2_64[] = { 0xFF, 0x09, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    {
        const ESUTF8* src = big2_64;
        sucs_ex_big_t big;
        uint32_t limbs[4];
        extsucs_big_init(&big, limbs, 4);
        ConversionResult r = ConvertESUTF8toSUCSBig(&src, big2_64 + sizeof(big2_64), &big, strictConversion);
        assert(r == conversionOK);
        assert(src == big2_64 + sizeof(big2_64));
        /* 2^64 = limbs {0, 0, 1} (little-endian base 2^32). */
        assert(big.len == 3);
        assert(big.limbs[0] == 0);
        assert(big.limbs[1] == 0);
        assert(big.limbs[2] == 1);
    }

    /* roundtrip big -> esutf8 */
    {
        sucs_ex_big_t big;
        uint32_t limbs[4];
        extsucs_big_init(&big, limbs, 4);
        extsucs_big_clear(&big);
        limbs[0] = 0;
        limbs[1] = 0;
        limbs[2] = 1;
        big.len = 3;
        ESUTF8 bout[32];
        ESUTF8* bd = bout;
        ConversionResult r = ConvertSUCSBigtoESUTF8(&big, &bd, bout + sizeof(bout), strictConversion);
        assert(r == conversionOK);
        assert(bd - bout == (ptrdiff_t)sizeof(big2_64));
        assert(memcmp(bout, big2_64, sizeof(big2_64)) == 0);
    }

    /* 0xFF frame with LEB128 value < 9 is invalid */
    {
        static const uint8_t bad[] = { 0xFF, 0x01, 0x01 };
        const ESUTF8* src = bad;
        sucs_ex_big_t big;
        uint32_t limbs[4];
        extsucs_big_init(&big, limbs, 4);
        ConversionResult r = ConvertESUTF8toSUCSBig(&src, bad + sizeof(bad), &big, strictConversion);
        assert(r == sourceIllegal);
        assert(src == bad);
    }

    /* non-canonical LEB128 (trailing zero) */
    {
        static const uint8_t bad2[] = { 0xFF, 0x89, 0x00, 0x01 };
        const ESUTF8* src = bad2;
        sucs_ex_big_t big;
        uint32_t limbs[4];
        extsucs_big_init(&big, limbs, 4);
        ConversionResult r = ConvertESUTF8toSUCSBig(&src, bad2 + sizeof(bad2), &big, strictConversion);
        assert(r == sourceIllegal);
    }

    /* leading zero in magnitude: 9-byte payload starting with 0x00 */
    {
        static const uint8_t bad3[] = {
            0xFF, 0x09, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
        };
        const ESUTF8* src = bad3;
        sucs_ex_big_t big;
        uint32_t limbs[4];
        extsucs_big_init(&big, limbs, 4);
        ConversionResult r = ConvertESUTF8toSUCSBig(&src, bad3 + sizeof(bad3), &big, strictConversion);
        assert(r == sourceIllegal);
        assert(src == bad3);
    }
}

static void test_truncation_and_capacity(void) {
    /* truncated base frame */
    {
        static const uint8_t trunc[] = { 0xE2, 0x82 }; /* need third byte */
        const ESUTF8* src = trunc;
        ESUTF64 out64[4];
        ESUTF64* d64 = out64;
        ConversionResult r = ConvertESUTF8toESUTF64(&src, trunc + sizeof(trunc), &d64, out64 + 4, strictConversion);
        assert(r == sourceExhausted);
        assert(src == trunc);
        src = trunc;
        d64 = out64;
        r = ConvertESUTF8toESUTF64(&src, trunc + sizeof(trunc), &d64, out64 + 4, lenientConversion);
        assert(r == sourceExhausted);
        assert(src == trunc + sizeof(trunc));
        assert(d64 == out64 + 1);
        assert(out64[0] == (ESUTF64)ESUTF_REPLACEMENT_CHAR);
    }

    /* target exhausted */
    {
        sucs_ex_char_t cp = 0x123456ULL;
        ESUTF8 buf[16];
        size_t n = vsutf_encode(cp, buf, sizeof(buf));
        const ESUTF8* src = buf;
        ESUTF64 out64[1];
        ESUTF64* d64 = out64;
        ConversionResult r = ConvertESUTF8toESUTF64(&src, buf + n, &d64, out64 + 0, strictConversion); /* targetEnd == targetStart */
        assert(r == targetExhausted);
        assert(src == buf);
        assert(d64 == out64);
    }

    /* big target capacity exhausted */
    {
        static const uint8_t big2_64[] = { 0xFF, 0x09, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
        const ESUTF8* src = big2_64;
        sucs_ex_big_t big;
        uint32_t limbs[1];
        extsucs_big_init(&big, limbs, 1); /* need 3 limbs for 2^64 */
        ConversionResult r = ConvertESUTF8toSUCSBig(&src, big2_64 + sizeof(big2_64), &big, strictConversion);
        assert(r == targetExhausted);
        assert(src == big2_64);
    }
}

static void test_legality(void) {
    {
        static const uint8_t good[] = { 0x41, 0xE2, 0x82, 0xAC, 0xF0, 0x90, 0x80, 0x80 };
        assert(isLegalESUTF8Sequence(good, good + sizeof(good)));
    }
    {
        static const uint8_t good_sent[] = { 0xFD, 0xBF, 0xBF, 0xBF, 0xBF, 0xBF }; /* 0x7FFFFFFF */
        assert(isLegalESUTF8Sequence(good_sent, good_sent + sizeof(good_sent)));
    }
    {
        static const uint8_t bad_trap[] = { 0xFD, 0xBF, 0xBF, 0xBF, 0xBF, 0xB0 }; /* 0x7FFFFFF0 */
        assert(!isLegalESUTF8Sequence(bad_trap, bad_trap + sizeof(bad_trap)));
    }
    {
        static const uint8_t overlong[] = { 0xC0, 0x80 };
        assert(!isLegalESUTF8Sequence(overlong, overlong + 2));
    }
    {
        static const uint8_t bad_cont[] = { 0x80 };
        assert(!isLegalESUTF8Sequence(bad_cont, bad_cont + 1));
    }
}

int main(void) {
    test_vbase_roundtrips();
    test_esutf32_view();
    test_trap_and_sentinel();
    test_big_arbitrary();
    test_truncation_and_capacity();
    test_legality();

    printf("[PASS] test_convert_esutf\n");
    return 0;
}
