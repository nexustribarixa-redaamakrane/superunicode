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
    Conversions between SUTF-8, SUTF-16, and SUTF-32 for Base SuperUnicode.

    Base SUCS is the 31-bit SuperUnicode character encoding described by
    SUTR-0 and SUTR-1. The valid allocatable range is 0x00000000 through
    0x7FFFFFEF; the fifteen Kernel Security Trap slots
    (0x7FFFFFF0-0x7FFFFFFE) and the in-band Sentinel (0x7FFFFFFF) are not
    codepoints and are rejected at every entry point.

    SUTF is a transformation format, not a serialization transport. These
    routines operate on code units: SUTF-8 bytes, SUTF-16 words, and
    SUTF-32 values. Byte order and stream framing belong to the SUST layer.

    NO SURROGATES: SuperUnicode has no surrogate mechanism. The range
    0xD800-0xDFFF is an ordinary block of valid PUA codepoints. It is encoded
    like any other value and is never paired, reserved, or rejected.
------------------------------------------------------------------------ */
#ifndef SUPERUNICODE_SUTF_CONVERT_SUTF_H
#define SUPERUNICODE_SUTF_CONVERT_SUTF_H

#include <stdint.h>
#include <stddef.h>

#include "sucs_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---------------------------------------------------------------------
    All public values are unsigned so that masks and shifts never trigger
    sign extension. Fixed-width integer types make the code-unit widths
    explicit and portable.
------------------------------------------------------------------------ */
#ifndef SUTF_CONVERT_TYPES_DEFINED
#define SUTF_CONVERT_TYPES_DEFINED
typedef uint32_t SUTF32; /* exactly one 32-bit code unit  */
typedef uint16_t SUTF16; /* exactly one 16-bit code unit  */
typedef uint8_t  SUTF8;  /* exactly one  8-bit code unit  */
#endif

#ifndef CONVERT_BOOLEAN_DEFINED
#define CONVERT_BOOLEAN_DEFINED
typedef unsigned char Boolean; /* 0 or 1 */
#endif

/* Fundamental Base SuperUnicode constants. */
#define SUTF_REPLACEMENT_CHAR   ((SUTF32)0x0000FFFD)
#define SUTF_MAX_BMP            ((SUTF32)0x0000FFFF)
#define SUTF_MAX_LEGACY         ((SUTF32)0x0010FFFF)
#define SUTF_MAX_SUTF16_LEGACY  SUTF_MAX_LEGACY
#define SUTF_MAX_SUTF32         ((SUTF32)SUCS_MAX_CODEPOINT)

/* ---------------------------------------------------------------------
    Result of a conversion. In strict mode an illegal or unrepresentable
    source value stops the conversion at that value. In lenient mode one
    replacement character (SUTF_REPLACEMENT_CHAR) is emitted for it and
    conversion continues.
------------------------------------------------------------------------ */
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

/* =====================================================================
    Stream conversions

    Each routine consumes a whole source sequence until the source is
    exhausted, the target is exhausted, or an illegal source value is found.
    On return, *sourceStart and *targetStart point at the first unconsumed
    source unit and the first unwritten target unit respectively.

    Strict semantics:
      - sourceIllegal   leaves the source at the offending value and the
                        target unchanged for that value.
      - sourceExhausted leaves the source at the start of the truncated
                        value and the target unchanged for it.
      - targetExhausted leaves the source at the start of the value that
                        did not fit and the target unchanged for it.

    Lenient semantics:
      - one replacement character is emitted and the malformed value is
                        consumed, then conversion continues.
      - a truncated value at the end of the source is replaced and consumed;
                        the result is still sourceExhausted.
===================================================================== */

ConversionResult ConvertSUTF8toSUTF16(
        const SUTF8** sourceStart, const SUTF8* sourceEnd,
        SUTF16** targetStart, SUTF16* targetEnd, ConversionFlags flags);

ConversionResult ConvertSUTF16toSUTF8(
        const SUTF16** sourceStart, const SUTF16* sourceEnd,
        SUTF8** targetStart, SUTF8* targetEnd, ConversionFlags flags);

ConversionResult ConvertSUTF8toSUTF32(
        const SUTF8** sourceStart, const SUTF8* sourceEnd,
        SUTF32** targetStart, SUTF32* targetEnd, ConversionFlags flags);

ConversionResult ConvertSUTF32toSUTF8(
        const SUTF32** sourceStart, const SUTF32* sourceEnd,
        SUTF8** targetStart, SUTF8* targetEnd, ConversionFlags flags);

ConversionResult ConvertSUTF16toSUTF32(
        const SUTF16** sourceStart, const SUTF16* sourceEnd,
        SUTF32** targetStart, SUTF32* targetEnd, ConversionFlags flags);

ConversionResult ConvertSUTF32toSUTF16(
        const SUTF32** sourceStart, const SUTF32* sourceEnd,
        SUTF16** targetStart, SUTF16* targetEnd, ConversionFlags flags);

/* =====================================================================
    Legality helpers

    Return true only when the entire sequence is well formed under strict
    decoding rules: canonical framing, a valid SUCS codepoint, and no
    truncated value at the end.
===================================================================== */
Boolean isLegalSUTF8Sequence(const SUTF8* source, const SUTF8* sourceEnd);
Boolean isLegalSUTF16Sequence(const SUTF16* source, const SUTF16* sourceEnd);
Boolean isLegalSUTF32Sequence(const SUTF32* source, const SUTF32* sourceEnd);

#ifdef __cplusplus
}
#endif

#endif /* SUPERUNICODE_SUTF_CONVERT_SUTF_H */
