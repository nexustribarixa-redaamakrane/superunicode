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
    Conversions between the Base SUCS serialization transports SUST-16 and
    SUST-32, and between those transports and sucs_char_t codepoints.

    SUST-16 is the canonical big-endian byte serialization of the SUTF-16
    word stream: one codepoint occupies either 2 bytes (0x0000-0x7FFF) or
    4 bytes (0x8000-0x7FFFFFFF). SUST-32 is a fixed 4-byte big-endian
    container for the Base 31-bit space.

    These routines keep the ConvertUTF-style sourceStart/sourceEnd and
    targetStart/targetEnd convention. A source or target pointer addresses a
    unit of the named transport: one byte for SUST-16 and SUST-32, one
    sucs_char_t for the codepoint streams.

    NO SURROGATES: 0xD800-0xDFFF are ordinary valid PUA codepoints. The
    Kernel Security Trap range (0x7FFFFFF0-0x7FFFFFFE) and the Sentinel
    (0x7FFFFFFF) are rejected by strict decoding and replaced in lenient
    decoding.
------------------------------------------------------------------------ */
#ifndef SUPERUNICODE_SUST_CONVERT_SUST_H
#define SUPERUNICODE_SUST_CONVERT_SUST_H

#include <stdint.h>
#include <stddef.h>

/* Master SUST header: exports sucs_types.h, extsucs_types.h, sust16.h,
   sustfixed.h, and esust.h. */
#include "sust.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef SUST_CONVERT_TYPES_DEFINED
#define SUST_CONVERT_TYPES_DEFINED
typedef uint8_t SUST16; /* one serialized SUST-16 byte */
typedef uint8_t SUST32; /* one serialized SUST-32 byte */
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

#define SUST_REPLACEMENT_CHAR ((sucs_char_t)0x0000FFFDUL)

/* Largest serialized SUST-16 value: two 16-bit words, big-endian. */
#define SUST16_MAX_BYTES 4

/* =====================================================================
    SUST-16 <-> SUST-32
===================================================================== */
ConversionResult ConvertSUST16toSUST32(
        const SUST16** sourceStart, const SUST16* sourceEnd,
        SUST32** targetStart, SUST32* targetEnd, ConversionFlags flags);

ConversionResult ConvertSUST32toSUST16(
        const SUST32** sourceStart, const SUST32* sourceEnd,
        SUST16** targetStart, SUST16* targetEnd, ConversionFlags flags);

/* =====================================================================
    SUST transports <-> sucs_char_t codepoints

    The codepoint target/source pointer addresses one sucs_char_t per unit.
    SUST-16 and SUST-32 pointer arithmetic remains byte-addressed.
===================================================================== */
ConversionResult ConvertSUST16toSUCS(
        const SUST16** sourceStart, const SUST16* sourceEnd,
        sucs_char_t** targetStart, sucs_char_t* targetEnd, ConversionFlags flags);

ConversionResult ConvertSUCStoSUST16(
        const sucs_char_t** sourceStart, const sucs_char_t* sourceEnd,
        SUST16** targetStart, SUST16* targetEnd, ConversionFlags flags);

ConversionResult ConvertSUST32toSUCS(
        const SUST32** sourceStart, const SUST32* sourceEnd,
        sucs_char_t** targetStart, sucs_char_t* targetEnd, ConversionFlags flags);

ConversionResult ConvertSUCStoSUST32(
        const sucs_char_t** sourceStart, const sucs_char_t* sourceEnd,
        SUST32** targetStart, SUST32* targetEnd, ConversionFlags flags);

/* =====================================================================
    Legality helpers

    Return true only when the entire byte sequence is well formed under
    strict decoding rules: canonical framing, a valid Base SUCS codepoint,
    and no truncated value at the end.
===================================================================== */
Boolean isLegalSUST16Sequence(const SUST16* source, const SUST16* sourceEnd);
Boolean isLegalSUST32Sequence(const SUST32* source, const SUST32* sourceEnd);

#ifdef __cplusplus
}
#endif

#endif /* SUPERUNICODE_SUST_CONVERT_SUST_H */
