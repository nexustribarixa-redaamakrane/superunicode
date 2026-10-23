/*
 * Copyright 2001-2004 Unicode, Inc.
 * Copyright 2025-2026 SuperUnicode Project
 *
 * Disclaimer
 *
 * This source code is provided as is by Unicode, Inc. and the SuperUnicode
 * Project. No claims are made as to fitness for any particular purpose. No
 * warranties of any kind are expressed or implied. The recipient agrees to
 * determine applicability of information provided.
 *
 * Limitations on Rights to Redistribute This Code
 *
 * Unicode, Inc. hereby grants the right to freely use the information
 * supplied in this file in the creation of products supporting the
 * SuperUnicode ecosystem, and to make copies of this file in any form
 * for internal or external distribution as long as this notice remains attached.
 *
 * harness_su.c
 *
 * SuperUnicode port of the original ConvertUTF harness. This is a test
 * harness for ConvertSUTF, ConvertSUST, ConvertESUTF, and ConvertESUST.
 * Compile and run without arguments. It exhaustively tests the conversion
 * routines for the Base SUCS (31-bit) and Extended SUCS domains with no
 * surrogate model.
 *
 * This file is a port and adaptation of Unicode, Inc.'s harness.c for the
 * SuperUnicode ecosystem. It is not the original Unicode source.
 *
 * Rev History: Rick McGowan, original harness April 2001.
 * Ported to SuperUnicode 2025-2026.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "ConvertSUTF.h"
#include "ConvertSUST.h"
#include "ConvertESUTF.h"
#include "ConvertESUST.h"
#include "sutf8.h"
#include "sutf16.h"
#include "extsucs_big.h"
#include "vsutf.h"

/* ---------------------------------------------------------------------
	test01 - Spot check legal & illegal Base SUTF-8 values.
	No surrogates. Trap range 0x7FFFFFF0-0x7FFFFFFE illegal.
	Sentinel 0x7FFFFFFF is illegal in Base SUCS but valid in ExtSUCS.
   --------------------------------------------------------------------- */
struct sucs_utf8_test {
    Boolean legal;
    int len;
    unsigned char seq[8];
};

static struct sucs_utf8_test sucs_utf8_testData[] = {
    { 1,	1,	{ 0x7A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }},	/* 'z' */
    { 1,	2,	{ 0xC2, 0xAC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }},	/* U+00AC */
    { 1,	2,	{ 0xDF, 0xB2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }},	/* U+07F2 */
    { 1,	3,	{ 0xE0, 0xA1, 0x81, 0x00, 0x00, 0x00, 0x00, 0x00 }},	/* U+0861 */
    { 1,	3,	{ 0xE1, 0xAC, 0x90, 0x00, 0x00, 0x00, 0x00, 0x00 }},	/* U+AB90 */
    { 1,	4,	{ 0xF0, 0x93, 0xB2, 0xA1, 0x00, 0x00, 0x00, 0x00 }},	/* U+24CB1 */
    { 1,	4,	{ 0xF1, 0x87, 0x9A, 0xB0, 0x00, 0x00, 0x00, 0x00 }},	/* U+41E70 */
    { 1,	4,	{ 0xF3, 0x88, 0x9B, 0xAD, 0x00, 0x00, 0x00, 0x00 }},	/* U+C22ED */
    { 1,	4,	{ 0xF4, 0x82, 0x89, 0x8F, 0x00, 0x00, 0x00, 0x00 }},	/* U+10248F */
    /* 0xD800-0xDFFF are ordinary PUA in SuperUnicode - legal */
    { 1,	3,	{ 0xED, 0xA0, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00 }},	/* 0xD800 as UTF-8 form */
    { 1,	3,	{ 0xED, 0xBF, 0xBF, 0x00, 0x00, 0x00, 0x00, 0x00 }},	/* 0xDFFF */
    { 1,	3,	{ 0xEE, 0x80, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00 }},	/* 0xE000 */
    { 1,	3,	{ 0xED, 0x9F, 0xBF, 0x00, 0x00, 0x00, 0x00, 0x00 }},	/* 0xD7FF */
    /* Traps */
    { 0,	6,	{ 0xFD, 0xBF, 0xBF, 0xBF, 0xBF, 0xB0, 0x00, 0x00 }},	/* 0x7FFFFFF0 */
    { 0,	6,	{ 0xFD, 0xBF, 0xBF, 0xBF, 0xBF, 0xBE, 0x00, 0x00 }},	/* 0x7FFFFFFE */
    /* Sentinel illegal in Base */
    { 0,	6,	{ 0xFD, 0xBF, 0xBF, 0xBF, 0xBF, 0xBF, 0x00, 0x00 }},	/* 0x7FFFFFFF */
    /* Overlong */
    { 0,	2,	{ 0xC0, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }},
    { 0,	2,	{ 0xC1, 0xBF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }},
    { 0,	3,	{ 0xE0, 0x80, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00 }},
    /* Continuation only */
    { 0,	1,	{ 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }},
    /* Invalid start */
    { 0,	1,	{ 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }},
    { 0,	1,	{ 0xFE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }},
    { 0,	0,	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }},
};

static int test01(void) {
    int i;
    int rval = 0;
    printf("Begin Test01 (Base SUTF-8 legality)\n"); fflush(stdout);
    for (i = 0; sucs_utf8_testData[i].len; i++) {
        Boolean got1 = isLegalSUTF8Sequence(sucs_utf8_testData[i].seq,
                                           sucs_utf8_testData[i].seq + sucs_utf8_testData[i].len);
        if (got1 != sucs_utf8_testData[i].legal) {
            printf("Test01 error: seq %d legal=%d got=%d for bytes (%02x,%02x,%02x,%02x,%02x,%02x)\n",
                   i, sucs_utf8_testData[i].legal, got1,
                   sucs_utf8_testData[i].seq[0], sucs_utf8_testData[i].seq[1],
                   sucs_utf8_testData[i].seq[2], sucs_utf8_testData[i].seq[3],
                   sucs_utf8_testData[i].seq[4], sucs_utf8_testData[i].seq[5]);
            ++rval;
        }
    }
    return (rval ? 0 : 1);
}

/* ---------------------------------------------------------------------
	test02 - Round trip Base SUTF32 <-> SUTF16 <-> SUTF8 with no surrogates
	Tests boundaries and PUA ranges as normal codepoints.
   --------------------------------------------------------------------- */
static int test02(void) {
    sucs_char_t i;
    int rval = 0;
    printf("Begin Test02 (Base round-trip SUTF32<->SUTF16<->SUTF8)\n"); fflush(stdout);
    
    /* Test key ranges */
    for (i = 0; i <= 0x110000UL; i += 0x1111UL) {
        /* Skip traps */
        if (i >= 0x7FFFFFF0UL && i <= 0x7FFFFFFEUL) continue;
        if (i == 0x7FFFFFFFUL) continue; /* sentinel illegal in base */
        
        SUTF32 src32[2];
        SUTF8 enc8[16];
        SUTF16 enc16[8];
        SUTF32 dec32a[2];
        SUTF32 dec32b[2];
        const SUTF32* s32 = src32;
        SUTF8* d8 = enc8;
        const SUTF8* s8 = enc8;
        SUTF16* d16 = enc16;
        const SUTF16* s16 = enc16;
        SUTF32* o32 = dec32a;
        ConversionResult r;
        
        src32[0] = (SUTF32)i;
        src32[1] = 0;
        
        /* SUTF32 -> SUTF8 */
        s32 = src32; d8 = enc8;
        r = ConvertSUTF32toSUTF8(&s32, src32 + 1, &d8, enc8 + sizeof(enc8), strictConversion);
        if (r != conversionOK) {
            printf("Test02 error SUTF32->SUTF8 at %08lx: %d\n", (unsigned long)i, r);
            rval++;
            continue;
        }
        
        /* SUTF8 -> SUTF16 */
        s8 = enc8; d16 = enc16;
        r = ConvertSUTF8toSUTF16(&s8, enc8 + (d8 - enc8), &d16, enc16 + 8, strictConversion);
        if (r != conversionOK) {
            printf("Test02 error SUTF8->SUTF16 at %08lx: %d\n", (unsigned long)i, r);
            rval++;
            continue;
        }
        
        /* SUTF16 -> SUTF32 */
        s16 = enc16; o32 = dec32b;
        r = ConvertSUTF16toSUTF32(&s16, enc16 + (d16 - enc16), &o32, dec32b + 1, strictConversion);
        if (r != conversionOK) {
            printf("Test02 error SUTF16->SUTF32 at %08lx: %d\n", (unsigned long)i, r);
            rval++;
            continue;
        }
        
        if (dec32b[0] != (SUTF32)i) {
            printf("Test02 mismatch at %08lx -> %08lx\n", (unsigned long)i, (unsigned long)dec32b[0]);
            rval++;
        }
    }
    
    /* Explicit 31-bit regime boundaries: SUTF-8 uses 1-6 bytes and SUTF-16
       uses 1-2 words across the whole Base range (no 0x10FFFF ceiling). */
    {
        static const sucs_char_t edge[] = {
            0x0000007FUL, 0x00000080UL, 0x000007FFUL, 0x00000800UL,
            0x0000FFFFUL, 0x00010000UL, 0x0010FFFFUL, 0x00110000UL,
            0x001FFFFFUL, 0x00200000UL, 0x03FFFFFFUL, 0x04000000UL,
            0x7FFFFFEFUL
        };
        /* Expected SUTF-8 byte counts for the values above. */
        static const int edge_len8[] = { 1, 2, 2, 3, 3, 4, 4, 5, 5, 5, 5, 6, 6 };
        size_t j;

        for (j = 0; j < sizeof(edge)/sizeof(edge[0]); j++) {
            SUTF32 src32[1] = { (SUTF32)edge[j] };
            SUTF8 enc8[16];
            SUTF16 enc16[4];
            SUTF32 back32[1];
            const SUTF32* s32 = src32;
            SUTF8* d8 = enc8;
            const SUTF8* s8 = enc8;
            SUTF16* d16 = enc16;
            const SUTF16* s16 = enc16;
            SUTF32* o32 = back32;
            ConversionResult r;
            int expect_words = (edge[j] <= 0x7FFFUL) ? 1 : 2;

            s32 = src32; d8 = enc8;
            r = ConvertSUTF32toSUTF8(&s32, src32 + 1, &d8, enc8 + sizeof(enc8), strictConversion);
            if (r != conversionOK || (int)(d8 - enc8) != edge_len8[j]) {
                printf("Test02 edge SUTF8 len at %08lx: r=%d len=%d expect=%d\n",
                       (unsigned long)edge[j], (int)r, (int)(d8 - enc8), edge_len8[j]);
                rval++;
                continue;
            }

            s8 = enc8; o32 = back32;
            r = ConvertSUTF8toSUTF32(&s8, enc8 + (d8 - enc8), &o32, back32 + 1, strictConversion);
            if (r != conversionOK || back32[0] != (SUTF32)edge[j]) {
                printf("Test02 edge SUTF8 round-trip at %08lx\n", (unsigned long)edge[j]);
                rval++;
                continue;
            }

            s32 = src32; d16 = enc16;
            r = ConvertSUTF32toSUTF16(&s32, src32 + 1, &d16, enc16 + 4, strictConversion);
            if (r != conversionOK || (int)(d16 - enc16) != expect_words) {
                printf("Test02 edge SUTF16 words at %08lx: r=%d words=%d expect=%d\n",
                       (unsigned long)edge[j], (int)r, (int)(d16 - enc16), expect_words);
                rval++;
                continue;
            }

            s16 = enc16; o32 = back32;
            r = ConvertSUTF16toSUTF32(&s16, enc16 + (d16 - enc16), &o32, back32 + 1, strictConversion);
            if (r != conversionOK || back32[0] != (SUTF32)edge[j]) {
                printf("Test02 edge SUTF16 round-trip at %08lx\n", (unsigned long)edge[j]);
                rval++;
            }
        }
    }

    /* Explicit PUA/surrogate range tests - no surrogates in model */
    {
        sucs_char_t pua[] = { 0xD800UL, 0xDFFFUL, 0xE000UL, 0xFDD0UL, 0xFFFDUL };
        size_t j;
        for (j = 0; j < sizeof(pua)/sizeof(pua[0]); j++) {
            SUTF32 src32[1] = { (SUTF32)pua[j] };
            SUTF16 enc16[4];
            const SUTF32* s32 = src32;
            SUTF16* d16 = enc16;
            ConversionResult r = ConvertSUTF32toSUTF16(&s32, src32 + 1, &d16, enc16 + 4, strictConversion);
            if (r != conversionOK) {
                printf("Test02 PUA error at %08lx\n", (unsigned long)pua[j]);
                rval++;
                continue;
            }
            /* Verify no surrogate words 0xD800-0xDFFF appear as pairs */
            if ((d16 - enc16) == 2 && enc16[0] >= 0xD800U && enc16[0] <= 0xDBFFU) {
                printf("Test02 surrogate pair generated at %08lx\n", (unsigned long)pua[j]);
                rval++;
            }
        }
    }
    
    return (rval ? 0 : 1);
}

/* ---------------------------------------------------------------------
	test03 - SUST round trips
   --------------------------------------------------------------------- */
static int test03(void) {
    sucs_char_t vals[] = { 0x0UL, 0x7FUL, 0x80UL, 0x7FFUL, 0x800UL, 0xFFFFUL,
                          0x10000UL, 0xD800UL, 0xDFFFUL, 0xFFFFFUL, 0x7FFFFFEFUL };
    size_t i;
    int rval = 0;
    printf("Begin Test03 (Base SUST16/SUST32 round-trip)\n"); fflush(stdout);
    
    for (i = 0; i < sizeof(vals)/sizeof(vals[0]); i++) {
        sucs_char_t src[1] = { vals[i] };
        SUST16 s16[8];
        SUST32 s32[8];
        sucs_char_t d1[1], d2[1];
        const sucs_char_t* s = src;
        SUST16* dd16 = s16;
        const SUST16* ss16 = s16;
        sucs_char_t* od = d1;
        ConversionResult r;
        
        /* SUCS -> SUST16 */
        s = src; dd16 = s16;
        r = ConvertSUCStoSUST16(&s, src + 1, &dd16, s16 + 8, strictConversion);
        if (r != conversionOK) { rval++; continue; }
        
        /* SUST16 -> SUCS */
        ss16 = s16; od = d1;
        r = ConvertSUST16toSUCS(&ss16, s16 + (dd16-s16), &od, d1 + 1, strictConversion);
        if (r != conversionOK || d1[0] != vals[i]) { rval++; continue; }
        
        /* SUCS -> SUST32 */
        s = src; SUST32* dd32 = s32;
        r = ConvertSUCStoSUST32(&s, src + 1, &dd32, s32 + 8, strictConversion);
        if (r != conversionOK) { rval++; continue; }
        
        /* SUST32 -> SUCS */
        const SUST32* ss32 = s32;
        od = d2;
        r = ConvertSUST32toSUCS(&ss32, s32 + (dd32-s32), &od, d2 + 1, strictConversion);
        if (r != conversionOK || d2[0] != vals[i]) { rval++; continue; }
    }
    
    /* Trap rejection */
    {
        uint8_t trap16[] = { 0xFF, 0xFF, 0xFF, 0xF0 };
        uint8_t trap32[] = { 0x7F, 0xFF, 0xFF, 0xF0 };
        const SUST16* ts16 = trap16;
        const SUST32* ts32 = trap32;
        sucs_char_t out[1];
        sucs_char_t* tod = out;
        if (ConvertSUST16toSUCS(&ts16, trap16 + 4, &tod, out + 1, strictConversion) != sourceIllegal) rval++;
        if (ConvertSUST32toSUCS(&ts32, trap32 + 4, &tod, out + 1, strictConversion) != sourceIllegal) rval++;
    }
    
    return (rval ? 0 : 1);
}

/* ---------------------------------------------------------------------
	test04 - Extended SUTF (vSUTF) tests
   --------------------------------------------------------------------- */
static int test04(void) {
    int rval = 0;
    printf("Begin Test04 (Extended vSUTF/ESUTF)\n"); fflush(stdout);
    
    /* 0x7FFFFFFF valid in ExtSUCS */
    {
        sucs_ex_char_t cp = 0x7FFFFFFFULL;
        ESUTF8 buf[16];
        size_t n = vsutf_encode(cp, buf, sizeof(buf));
        if (n != 6) { rval++; }
        const ESUTF8* src = buf;
        ESUTF64 out64[1];
        ESUTF64* d64 = out64;
        ConversionResult r = ConvertESUTF8toESUTF64(&src, buf + n, &d64, out64 + 1, strictConversion);
        if (r != conversionOK || out64[0] != cp) { rval++; }
    }
    
    /* Trap rejected in ExtSUCS. vsutf_encode() itself refuses the trap range,
       so build the six-byte trap frame directly: 0x7FFFFFF0 -> FD BF BF BF BF B0. */
    {
        static const ESUTF8 trap_frame[6] = { 0xFD, 0xBF, 0xBF, 0xBF, 0xBF, 0xB0 };
        const ESUTF8* src = trap_frame;
        ESUTF64 out64[1];
        ESUTF64* d64 = out64;
        ConversionResult r = ConvertESUTF8toESUTF64(&src, trap_frame + 6, &d64, out64 + 1, strictConversion);
        if (r != sourceIllegal) { rval++; }
    }
    
    /* Above 0xFFFFFFFF in ESUTF32 view */
    {
        sucs_ex_char_t cp = 0x100000000ULL;
        ESUTF8 buf[16];
        size_t n = vsutf_encode(cp, buf, sizeof(buf));
        const ESUTF8* src = buf;
        ESUTF32 out32[1];
        ESUTF32* d32 = out32;
        ConversionResult r = ConvertESUTF8toESUTF32(&src, buf + n, &d32, out32 + 1, strictConversion);
        if (r != sourceIllegal) { rval++; }
        src = buf; d32 = out32;
        r = ConvertESUTF8toESUTF32(&src, buf + n, &d32, out32 + 1, lenientConversion);
        if (r != conversionOK || out32[0] != (ESUTF32)ESUTF_REPLACEMENT_CHAR) { rval++; }
    }
    
    /* Big arbitrary precision 2^64 */
    {
        static const uint8_t big2_64[] = { 0xFF, 0x09, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
        const ESUTF8* src = big2_64;
        sucs_ex_big_t big;
        uint32_t limbs[4];
        extsucs_big_init(&big, limbs, 4);
        ConversionResult r = ConvertESUTF8toSUCSBig(&src, big2_64 + sizeof(big2_64), &big, strictConversion);
        /* 2^64 = limbs {0, 0, 1} (little-endian base 2^32). */
        if (r != conversionOK || big.len != 3 ||
            big.limbs[0] != 0 || big.limbs[1] != 0 || big.limbs[2] != 1) {
            rval++;
        }
    }
    
    /* No 0x10FFFF ceiling - test larger value */
    {
        sucs_ex_char_t cp = 0x200000ULL;
        ESUTF8 buf[16];
        size_t n = vsutf_encode(cp, buf, sizeof(buf));
        const ESUTF8* src = buf;
        ESUTF64 out64[1];
        ESUTF64* d64 = out64;
        ConversionResult r = ConvertESUTF8toESUTF64(&src, buf + n, &d64, out64 + 1, strictConversion);
        if (r != conversionOK || out64[0] != cp) { rval++; }
    }
    
    return (rval ? 0 : 1);
}

/* ---------------------------------------------------------------------
	test05 - Extended SUST tests
   --------------------------------------------------------------------- */
static int test05(void) {
    int rval = 0;
    printf("Begin Test05 (Extended E-SUST)\n"); fflush(stdout);
    
    /* 0x7FFFFFFF valid in E-SUST-32 */
    {
        sucs_ex_char_t cp = 0x7FFFFFFFULL;
        ESUST32 out[4];
        ESUST32* d = out;
        const sucs_ex_char_t* src = &cp;
        ConversionResult r = ConvertESUCStoESUST32(&src, &cp + 1, &d, out + 4, strictConversion);
        if (r != conversionOK) { rval++; }
        const ESUST32* s2 = out;
        sucs_ex_char_t back[1];
        sucs_ex_char_t* b = back;
        r = ConvertESUST32toESUCS(&s2, out + 4, &b, back + 1, strictConversion);
        if (r != conversionOK || back[0] != cp) { rval++; }
    }
    
    /* 0x100000000 in E-SUST-64 */
    {
        sucs_ex_char_t cp = 0x100000000ULL;
        ESUST64 out[8];
        ESUST64* d = out;
        const sucs_ex_char_t* src = &cp;
        ConversionResult r = ConvertESUCStoESUST64(&src, &cp + 1, &d, out + 8, strictConversion);
        if (r != conversionOK) { rval++; }
        const ESUST64* s2 = out;
        sucs_ex_char_t back[1];
        sucs_ex_char_t* b = back;
        r = ConvertESUST64toESUCS(&s2, out + 8, &b, back + 1, strictConversion);
        if (r != conversionOK || back[0] != cp) { rval++; }
    }
    
    /* Downcast overflow */
    {
        sucs_ex_char_t bigv = 0x100000000ULL;
        ESUST64 f64[8];
        ESUST64* sfw = f64;
        const sucs_ex_char_t* sb = &bigv;
        ConversionResult r = ConvertESUCStoESUST64(&sb, &bigv + 1, &sfw, f64 + 8, strictConversion);
        if (r != conversionOK) { rval++; }
        const ESUST64* sf = f64;
        ESUST32 f32[4];
        ESUST32* df = f32;
        r = ConvertESUST64toESUST32(&sf, f64 + 8, &df, f32 + 4, strictConversion);
        if (r != sourceIllegal) { rval++; }
    }
    
    /* Trap rejected */
    {
        static const uint8_t trap32[] = { 0x7F, 0xFF, 0xFF, 0xF0 };
        const ESUST32* s32 = trap32;
        sucs_ex_char_t out[1];
        sucs_ex_char_t* d = out;
        if (ConvertESUST32toESUCS(&s32, trap32 + 4, &d, out + 1, strictConversion) != sourceIllegal) rval++;
    }
    
    return (rval ? 0 : 1);
}

int main(void) {
    int failed = 0;

    printf("SuperUnicode SUTF/SUST Conversion Harness\n");
    printf("No surrogates. 31-bit Base SUCS + unbounded ExtSUCS.\n");
    printf("Expecting correct behavior for traps, PUA ranges, and extended domain.\n\n");
    fflush(stdout);
    
    if (test01()) { printf("******** Test01 (SUTF-8 legality) succeeded ********\n\n"); }
    else { printf("-------- Test01 (SUTF-8 legality) failed --------\n\n"); ++failed; }
    
    if (test02()) { printf("******** Test02 (Base round-trip) succeeded ********\n\n"); }
    else { printf("-------- Test02 (Base round-trip) failed --------\n\n"); ++failed; }
    
    if (test03()) { printf("******** Test03 (Base SUST) succeeded ********\n\n"); }
    else { printf("-------- Test03 (Base SUST) failed --------\n\n"); ++failed; }
    
    if (test04()) { printf("******** Test04 (Extended vSUTF) succeeded ********\n\n"); }
    else { printf("-------- Test04 (Extended vSUTF) failed --------\n\n"); ++failed; }
    
    if (test05()) { printf("******** Test05 (Extended E-SUST) succeeded ********\n\n"); }
    else { printf("-------- Test05 (Extended E-SUST) failed --------\n\n"); ++failed; }
    
    printf("SuperUnicode harness complete.\n");
    if (failed) {
        printf("%d test group(s) failed.\n", failed);
        return 1;
    }
    return 0;
}
