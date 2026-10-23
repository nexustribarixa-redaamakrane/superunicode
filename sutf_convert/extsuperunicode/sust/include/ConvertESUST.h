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
 * SuperUnicode ecosystem, and to make copies of this file in any form for
 * internal or external distribution as long as this notice remains attached.
 *
 * This file is a port and adaptation of Unicode, Inc.'s ConvertUTF.h/.c for
 * the SuperUnicode ecosystem. It is not the original Unicode source.
 */
/* ---------------------------------------------------------------------
    Conversions between the ExtSUCS fixed-width serialization transports
    E-SUST-32 (4 bytes big-endian) and E-SUST-64 (8 bytes big-endian), and
    between those transports and sucs_ex_char_t codepoints.

    The transports are supplied by sustfixed.h. E-SUST-32 carries ExtSUCS
    addresses through 0xFFFFFFFF; E-SUST-64 carries the full 64-bit fast
    path. The inherited Kernel Security Trap range (0x7FFFFFF0-0x7FFFFFFE)
    is rejected in both. 0x7FFFFFFF is NOT a Sentinel in ExtSUCS; it is an
    ordinary valid codepoint and round-trips unchanged.

    Values above 0xFFFFFFFF are valid ExtSUCS but unrepresentable in
    E-SUST-32. Strict down-conversion reports sourceIllegal; lenient
    down-conversion substitutes the replacement character.

    NO SURROGATES: 0xD800-0xDFFF are ordinary PUA addresses here.
------------------------------------------------------------------------ */
#ifndef EXTSUPERUNICODE_SUST_CONVERT_ESUST_H
#define EXTSUPERUNICODE_SUST_CONVERT_ESUST_H

#include <stdint.h>
#include <stddef.h>

/* Master SUST header: exports extsucs_types.h, sust16.h, sustfixed.h,
   and esust.h. */
#include "sust.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef ESUST_CONVERT_TYPES_DEFINED
#define ESUST_CONVERT_TYPES_DEFINED
typedef uint8_t ESUST32; /* one serialized E-SUST-32 byte */
typedef uint8_t ESUST64; /* one serialized E-SUST-64 byte */
#endif

#ifndef CONVERT_BOOLEAN_DEFINED
#define CONVERT_BOOLEAN_DEFINED
typedef unsigned char Boolean; /* 0 or 1 */
#endif

#ifndef CONVERSION_RESULT_DEFINED
#define CONVERSION_RESULT_DEFINED
typedef enum {
    conversionOK,    /* conversion successful                                */
    sourceExhausted, /* partial character in source, but hit end             */
    targetExhausted, /* insufficient room in target for conversion           */
    sourceIllegal    /* source sequence is illegal/malformed/unrepresentable  */
} ConversionResult;

typedef enum {
    strictConversion = 0,
    lenientConversion
} ConversionFlags;
#endif

#define ESUST_REPLACEMENT_CHAR ((sucs_ex_char_t)0x0000FFFDULL)
#define ESUST32_MAX            ((sucs_ex_char_t)0x00000000FFFFFFFFULL)

/* Fixed frame sizes, re-exported for callers that only include this header. */
#define ESUST32_FRAME_BYTES SUST32_BYTES
#define ESUST64_FRAME_BYTES SUST64_BYTES

/* =====================================================================
    E-SUST-32 <-> E-SUST-64
===================================================================== */
ConversionResult ConvertESUST32toESUST64(
        const ESUST32** sourceStart, const ESUST32* sourceEnd,
        ESUST64** targetStart, ESUST64* targetEnd, ConversionFlags flags);

ConversionResult ConvertESUST64toESUST32(
        const ESUST64** sourceStart, const ESUST64* sourceEnd,
        ESUST32** targetStart, ESUST32* targetEnd, ConversionFlags flags);

/* =====================================================================
    E-SUST transports <-> sucs_ex_char_t codepoints
===================================================================== */
ConversionResult ConvertESUST32toESUCS(
        const ESUST32** sourceStart, const ESUST32* sourceEnd,
        sucs_ex_char_t** targetStart, sucs_ex_char_t* targetEnd,
        ConversionFlags flags);

ConversionResult ConvertESUCStoESUST32(
        const sucs_ex_char_t** sourceStart, const sucs_ex_char_t* sourceEnd,
        ESUST32** targetStart, ESUST32* targetEnd, ConversionFlags flags);

ConversionResult ConvertESUST64toESUCS(
        const ESUST64** sourceStart, const ESUST64* sourceEnd,
        sucs_ex_char_t** targetStart, sucs_ex_char_t* targetEnd,
        ConversionFlags flags);

ConversionResult ConvertESUCStoESUST64(
        const sucs_ex_char_t** sourceStart, const sucs_ex_char_t* sourceEnd,
        ESUST64** targetStart, ESUST64* targetEnd, ConversionFlags flags);

/* =====================================================================
    Legality helpers

    Return true only when the entire byte sequence consists of complete,
    valid frames with no truncated value at the end.
===================================================================== */
Boolean isLegalESUST32Sequence(const ESUST32* source, const ESUST32* sourceEnd);
Boolean isLegalESUST64Sequence(const ESUST64* source, const ESUST64* sourceEnd);

#ifdef __cplusplus
}
#endif

#endif /* EXTSUPERUNICODE_SUST_CONVERT_ESUST_H */
