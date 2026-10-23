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
    Conversions between vSUTF (the Extended-domain transformation format),
    64-bit ExtSUCS fast-path values, 32-bit compatibility views, and
    arbitrary-precision sucs_ex_big_t codepoints.

    vSUTF has three regimes (see vsutf.h, the normative definition):

      0x00000000 - 0x7FFFFFFF  SUTF-8 framing, 1-6 bytes
      0x80000000 - 2^64-1      0xFE prefix + 8 big-endian bytes (9 bytes)
      above 2^64-1             0xFF + LEB128(L) + L big-endian bytes, L >= 9

    There is no upper bound and no in-band Sentinel. 0x7FFFFFFF is an
    ordinary valid ExtSUCS codepoint. Only the inherited Kernel Security
    Trap range (0x7FFFFFF0-0x7FFFFFFE) is rejected.

    ESUTF32 is a 32-bit compatibility view: it can hold ExtSUCS addresses
    through 0xFFFFFFFF. A value above that is valid ExtSUCS but cannot be
    represented by this view, so strict conversion reports sourceIllegal
    and lenient conversion emits the replacement character.

    NO SURROGATES: vSUTF carries codepoints, not UTF-16 words. 0xD800-0xDFFF
    are ordinary PUA values at every layer of this module.
------------------------------------------------------------------------ */
#ifndef EXTSUPERUNICODE_SUTF_CONVERT_ESUTF_H
#define EXTSUPERUNICODE_SUTF_CONVERT_ESUTF_H

#include <stdint.h>
#include <stddef.h>

#include "extsucs_types.h"
#include "extsucs_big.h"
#include "vsutf.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef ESUTF_CONVERT_TYPES_DEFINED
#define ESUTF_CONVERT_TYPES_DEFINED
typedef uint8_t  ESUTF8;  /* one vSUTF stream byte                  */
typedef uint64_t ESUTF64; /* 64-bit ExtSUCS fast-path code unit     */
typedef uint32_t ESUTF32; /* 32-bit ExtSUCS compatibility code unit */
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

#define ESUTF_REPLACEMENT_CHAR ((sucs_ex_char_t)0x0000FFFDULL)
#define ESUTF64_MAX            ((sucs_ex_char_t)0xFFFFFFFFFFFFFFFFULL)
#define ESUTF32_MAX            ((sucs_ex_char_t)0x00000000FFFFFFFFULL)

/* =====================================================================
    vSUTF <-> 64-bit ExtSUCS
===================================================================== */
ConversionResult ConvertESUTF8toESUTF64(
        const ESUTF8** sourceStart, const ESUTF8* sourceEnd,
        ESUTF64** targetStart, ESUTF64* targetEnd, ConversionFlags flags);

ConversionResult ConvertESUTF64toESUTF8(
        const ESUTF64** sourceStart, const ESUTF64* sourceEnd,
        ESUTF8** targetStart, ESUTF8* targetEnd, ConversionFlags flags);

/* =====================================================================
    vSUTF <-> 32-bit compatibility view
===================================================================== */
ConversionResult ConvertESUTF8toESUTF32(
        const ESUTF8** sourceStart, const ESUTF8* sourceEnd,
        ESUTF32** targetStart, ESUTF32* targetEnd, ConversionFlags flags);

ConversionResult ConvertESUTF32toESUTF8(
        const ESUTF32** sourceStart, const ESUTF32* sourceEnd,
        ESUTF8** targetStart, ESUTF8* targetEnd, ConversionFlags flags);

ConversionResult ConvertESUTF64toESUTF32(
        const ESUTF64** sourceStart, const ESUTF64* sourceEnd,
        ESUTF32** targetStart, ESUTF32* targetEnd, ConversionFlags flags);

ConversionResult ConvertESUTF32toESUTF64(
        const ESUTF32** sourceStart, const ESUTF32* sourceEnd,
        ESUTF64** targetStart, ESUTF64* targetEnd, ConversionFlags flags);

/* =====================================================================
    vSUTF <-> arbitrary-precision ExtSUCS

    These routines convert one codepoint per call. `target` must have been
    bound with extsucs_big_init() (or be a value with valid caller storage).
    A source value that needs more limbs than `target->cap` yields
    targetExhausted with the source left at the start of the frame.
===================================================================== */
ConversionResult ConvertESUTF8toSUCSBig(
        const ESUTF8** sourceStart, const ESUTF8* sourceEnd,
        sucs_ex_big_t* target, ConversionFlags flags);

ConversionResult ConvertSUCSBigtoESUTF8(
        const sucs_ex_big_t* source,
        ESUTF8** targetStart, ESUTF8* targetEnd, ConversionFlags flags);

/* =====================================================================
    Legality helper

    Validates the entire vSUTF sequence, including arbitrary-precision
    frames, without materialising any codepoint. A 0xFF frame is legal when
    its LEB128 length is canonical, is at least 9, its payload is present,
    and the payload has no leading zero byte.
===================================================================== */
Boolean isLegalESUTF8Sequence(const ESUTF8* source, const ESUTF8* sourceEnd);

#ifdef __cplusplus
}
#endif

#endif /* EXTSUPERUNICODE_SUTF_CONVERT_ESUTF_H */
