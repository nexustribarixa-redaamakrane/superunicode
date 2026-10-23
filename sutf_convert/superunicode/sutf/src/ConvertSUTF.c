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

    Base SUCS is 31 bits wide. SUTF-8 uses one to six bytes and SUTF-16 uses
    one or two words; neither form is capped at Unicode's 0x10FFFF. The
    four-byte SUTF-8 and five-byte SUTF-8 framing boundaries are canonical
    encoding boundaries, not domain limits.

    NO SURROGATES: 0xD800-0xDFFF are ordinary valid PUA codepoints. There is
    no surrogate-pair arithmetic anywhere in this file.
------------------------------------------------------------------------ */
#include "ConvertSUTF.h"

#include "sutf8.h"
#include "sutf16.h"

/* ---------------------------------------------------------------------
    Internal decode result. Truncation is separated from illegality so the
    caller can return sourceExhausted rather than sourceIllegal.
------------------------------------------------------------------------ */
typedef enum {
    SUTF_DECODE_OK = 0,
    SUTF_DECODE_INVALID,
    SUTF_DECODE_TRUNCATED
} SutfDecodeStatus;

/* ---------------------------------------------------------------------
    SUTF-8 decoding

    On every return, *used is the number of source bytes the caller should
    treat as belonging to the decoded value:
      - OK:        the complete canonical sequence.
      - INVALID:   one byte for a bad lead/continuation, or the complete
                   sequence for a structurally complete but illegal value
                   (overlong, out of range, trap, or Sentinel).
      - TRUNCATED: every byte available for the incomplete sequence.
------------------------------------------------------------------------ */
static SutfDecodeStatus sutf8_decode_unit(const SUTF8* source,
                                          const SUTF8* sourceEnd,
                                          sucs_char_t* out_cp,
                                          size_t* used) {
    size_t avail;
    size_t length;
    uint8_t u0;
    sucs_char_t cp;

    *out_cp = SUCS_INVALID_CODEPOINT;
    *used = 0;

    if (!source || !sourceEnd || source >= sourceEnd) {
        return SUTF_DECODE_TRUNCATED;
    }

    avail = (size_t)(sourceEnd - source);
    u0 = source[0];

    if (u0 < 0x80U) {
        *out_cp = (sucs_char_t)u0;
        *used = 1;
        return SUTF_DECODE_OK;
    }

    if (u0 < 0xC0U) {
        /* Bare continuation byte. */
        *used = 1;
        return SUTF_DECODE_INVALID;
    }

    if (u0 < 0xE0U) {
        length = 2;
    } else if (u0 < 0xF0U) {
        length = 3;
    } else if (u0 < 0xF8U) {
        length = 4;
    } else if (u0 < 0xFCU) {
        length = 5;
    } else if (u0 < 0xFEU) {
        length = 6;
    } else {
        /* 0xFE and 0xFF are not SUTF-8 lead bytes (vSUTF escapes only). */
        *used = 1;
        return SUTF_DECODE_INVALID;
    }

    if (avail < length) {
        *used = avail;
        return SUTF_DECODE_TRUNCATED;
    }

    {
        size_t i;
        for (i = 1; i < length; i++) {
            if ((source[i] & 0xC0U) != 0x80U) {
                *used = 1;
                return SUTF_DECODE_INVALID;
            }
        }
    }

    cp = 0;
    switch (length) {
    case 2:
        cp = (((sucs_char_t)(u0 & 0x1FU)) << 6) |
             ((sucs_char_t)(source[1] & 0x3FU));
        if (cp < 0x80UL) {
            *used = length;
            return SUTF_DECODE_INVALID; /* overlong */
        }
        break;
    case 3:
        cp = (((sucs_char_t)(u0 & 0x0FU)) << 12) |
             (((sucs_char_t)(source[1] & 0x3FU)) << 6) |
             ((sucs_char_t)(source[2] & 0x3FU));
        if (cp < 0x800UL) {
            *used = length;
            return SUTF_DECODE_INVALID; /* overlong */
        }
        break;
    case 4:
        cp = (((sucs_char_t)(u0 & 0x07U)) << 18) |
             (((sucs_char_t)(source[1] & 0x3FU)) << 12) |
             (((sucs_char_t)(source[2] & 0x3FU)) << 6) |
             ((sucs_char_t)(source[3] & 0x3FU));
        if (cp < 0x10000UL || cp > 0x0010FFFFUL) {
            *used = length;
            return SUTF_DECODE_INVALID; /* overlong, or must use 5 bytes */
        }
        break;
    case 5:
        cp = (((sucs_char_t)(u0 & 0x03U)) << 24) |
             (((sucs_char_t)(source[1] & 0x3FU)) << 18) |
             (((sucs_char_t)(source[2] & 0x3FU)) << 12) |
             (((sucs_char_t)(source[3] & 0x3FU)) << 6) |
             ((sucs_char_t)(source[4] & 0x3FU));
        if (cp < 0x00110000UL || cp > 0x03FFFFFFUL) {
            *used = length;
            return SUTF_DECODE_INVALID; /* overlong, or must use 6 bytes */
        }
        break;
    default: /* 6 */
        cp = (((sucs_char_t)(u0 & 0x01U)) << 30) |
             (((sucs_char_t)(source[1] & 0x3FU)) << 24) |
             (((sucs_char_t)(source[2] & 0x3FU)) << 18) |
             (((sucs_char_t)(source[3] & 0x3FU)) << 12) |
             (((sucs_char_t)(source[4] & 0x3FU)) << 6) |
             ((sucs_char_t)(source[5] & 0x3FU));
        if (cp < 0x04000000UL) {
            *used = length;
            return SUTF_DECODE_INVALID; /* overlong */
        }
        break;
    }

    if (!sucs_is_valid(cp)) {
        *used = length;
        return SUTF_DECODE_INVALID; /* trap range or Sentinel */
    }

    *out_cp = cp;
    *used = length;
    return SUTF_DECODE_OK;
}

/* ---------------------------------------------------------------------
    SUTF-16 decoding

    A word with bit 15 clear is a literal in 0x0000-0x7FFF. A word with bit
    15 set is the first of exactly two words; a lone marker is a truncated
    sequence. Two-word encodings of values <= 0x7FFF are overlong.
------------------------------------------------------------------------ */
static SutfDecodeStatus sutf16_decode_unit(const SUTF16* source,
                                           const SUTF16* sourceEnd,
                                           sucs_char_t* out_cp,
                                           size_t* used) {
    uint16_t w0;
    sucs_char_t cp;

    *out_cp = SUCS_INVALID_CODEPOINT;
    *used = 0;

    if (!source || !sourceEnd || source >= sourceEnd) {
        return SUTF_DECODE_TRUNCATED;
    }

    w0 = source[0];

    if ((w0 & 0x8000U) == 0) {
        cp = (sucs_char_t)w0;
        if (!sucs_is_valid(cp)) {
            *used = 1;
            return SUTF_DECODE_INVALID;
        }
        *out_cp = cp;
        *used = 1;
        return SUTF_DECODE_OK;
    }

    if ((size_t)(sourceEnd - source) < 2) {
        *used = 1;
        return SUTF_DECODE_TRUNCATED;
    }

    cp = (((sucs_char_t)(w0 & 0x7FFFU)) << 16) |
         ((sucs_char_t)source[1]);

    if (cp <= 0x7FFFUL) {
        *used = 2;
        return SUTF_DECODE_INVALID; /* overlong */
    }
    if (!sucs_is_valid(cp)) {
        *used = 2;
        return SUTF_DECODE_INVALID; /* trap range or Sentinel */
    }

    *out_cp = cp;
    *used = 2;
    return SUTF_DECODE_OK;
}

/* ---------------------------------------------------------------------
    Small encode helpers. Capacity has already been checked by the caller.
------------------------------------------------------------------------ */
static size_t sutf8_put(SUTF8* target, sucs_char_t cp) {
    return sutf8_encode_char(cp, target, sutf8_codepoint_length(cp));
}

static size_t sutf16_put(SUTF16* target, sucs_char_t cp) {
    return sutf16_encode_char(cp, target, sutf16_codepoint_length(cp));
}

/* =====================================================================
    SUTF-8 -> SUTF-16
===================================================================== */
ConversionResult ConvertSUTF8toSUTF16(
        const SUTF8** sourceStart, const SUTF8* sourceEnd,
        SUTF16** targetStart, SUTF16* targetEnd, ConversionFlags flags) {
    const SUTF8* source;
    SUTF16* target;
    ConversionResult result = conversionOK;

    if (!sourceStart || !sourceEnd || !targetStart || !targetEnd) {
        return sourceIllegal;
    }

    source = *sourceStart;
    target = *targetStart;

    while (source < sourceEnd) {
        sucs_char_t cp;
        size_t used;
        SutfDecodeStatus status = sutf8_decode_unit(source, sourceEnd, &cp, &used);

        if (status == SUTF_DECODE_TRUNCATED) {
            if (flags == lenientConversion) {
                size_t need = sutf16_codepoint_length((sucs_char_t)SUTF_REPLACEMENT_CHAR);
                if ((size_t)(targetEnd - target) < need) {
                    result = targetExhausted;
                    break;
                }
                target += sutf16_put(target, (sucs_char_t)SUTF_REPLACEMENT_CHAR);
                source += used;
            }
            result = sourceExhausted;
            break;
        }

        if (status == SUTF_DECODE_INVALID) {
            if (flags == strictConversion) {
                result = sourceIllegal;
                break;
            }
            {
                size_t need = sutf16_codepoint_length((sucs_char_t)SUTF_REPLACEMENT_CHAR);
                if ((size_t)(targetEnd - target) < need) {
                    result = targetExhausted;
                    break;
                }
                target += sutf16_put(target, (sucs_char_t)SUTF_REPLACEMENT_CHAR);
            }
            source += used;
            continue;
        }

        {
            size_t need = sutf16_codepoint_length(cp);
            if ((size_t)(targetEnd - target) < need) {
                result = targetExhausted;
                break;
            }
            target += sutf16_put(target, cp);
            source += used;
        }
    }

    *sourceStart = source;
    *targetStart = target;
    return result;
}

/* =====================================================================
    SUTF-16 -> SUTF-8
===================================================================== */
ConversionResult ConvertSUTF16toSUTF8(
        const SUTF16** sourceStart, const SUTF16* sourceEnd,
        SUTF8** targetStart, SUTF8* targetEnd, ConversionFlags flags) {
    const SUTF16* source;
    SUTF8* target;
    ConversionResult result = conversionOK;

    if (!sourceStart || !sourceEnd || !targetStart || !targetEnd) {
        return sourceIllegal;
    }

    source = *sourceStart;
    target = *targetStart;

    while (source < sourceEnd) {
        sucs_char_t cp;
        size_t used;
        SutfDecodeStatus status = sutf16_decode_unit(source, sourceEnd, &cp, &used);

        if (status == SUTF_DECODE_TRUNCATED) {
            if (flags == lenientConversion) {
                size_t need = sutf8_codepoint_length((sucs_char_t)SUTF_REPLACEMENT_CHAR);
                if ((size_t)(targetEnd - target) < need) {
                    result = targetExhausted;
                    break;
                }
                target += sutf8_put(target, (sucs_char_t)SUTF_REPLACEMENT_CHAR);
                source += used;
            }
            result = sourceExhausted;
            break;
        }

        if (status == SUTF_DECODE_INVALID) {
            if (flags == strictConversion) {
                result = sourceIllegal;
                break;
            }
            {
                size_t need = sutf8_codepoint_length((sucs_char_t)SUTF_REPLACEMENT_CHAR);
                if ((size_t)(targetEnd - target) < need) {
                    result = targetExhausted;
                    break;
                }
                target += sutf8_put(target, (sucs_char_t)SUTF_REPLACEMENT_CHAR);
            }
            source += used;
            continue;
        }

        {
            size_t need = sutf8_codepoint_length(cp);
            if ((size_t)(targetEnd - target) < need) {
                result = targetExhausted;
                break;
            }
            target += sutf8_put(target, cp);
            source += used;
        }
    }

    *sourceStart = source;
    *targetStart = target;
    return result;
}

/* =====================================================================
    SUTF-8 -> SUTF-32
===================================================================== */
ConversionResult ConvertSUTF8toSUTF32(
        const SUTF8** sourceStart, const SUTF8* sourceEnd,
        SUTF32** targetStart, SUTF32* targetEnd, ConversionFlags flags) {
    const SUTF8* source;
    SUTF32* target;
    ConversionResult result = conversionOK;

    if (!sourceStart || !sourceEnd || !targetStart || !targetEnd) {
        return sourceIllegal;
    }

    source = *sourceStart;
    target = *targetStart;

    while (source < sourceEnd) {
        sucs_char_t cp;
        size_t used;
        SutfDecodeStatus status = sutf8_decode_unit(source, sourceEnd, &cp, &used);

        if (status == SUTF_DECODE_TRUNCATED) {
            if (flags == lenientConversion) {
                if (target >= targetEnd) {
                    result = targetExhausted;
                    break;
                }
                *target++ = SUTF_REPLACEMENT_CHAR;
                source += used;
            }
            result = sourceExhausted;
            break;
        }

        if (status == SUTF_DECODE_INVALID) {
            if (flags == strictConversion) {
                result = sourceIllegal;
                break;
            }
            if (target >= targetEnd) {
                result = targetExhausted;
                break;
            }
            *target++ = SUTF_REPLACEMENT_CHAR;
            source += used;
            continue;
        }

        if (target >= targetEnd) {
            result = targetExhausted;
            break;
        }
        *target++ = (SUTF32)cp;
        source += used;
    }

    *sourceStart = source;
    *targetStart = target;
    return result;
}

/* =====================================================================
    SUTF-32 -> SUTF-8
===================================================================== */
ConversionResult ConvertSUTF32toSUTF8(
        const SUTF32** sourceStart, const SUTF32* sourceEnd,
        SUTF8** targetStart, SUTF8* targetEnd, ConversionFlags flags) {
    const SUTF32* source;
    SUTF8* target;
    ConversionResult result = conversionOK;

    if (!sourceStart || !sourceEnd || !targetStart || !targetEnd) {
        return sourceIllegal;
    }

    source = *sourceStart;
    target = *targetStart;

    while (source < sourceEnd) {
        sucs_char_t cp = (sucs_char_t)(*source);

        if (!sucs_is_valid(cp)) {
            if (flags == strictConversion) {
                result = sourceIllegal;
                break;
            }
            cp = (sucs_char_t)SUTF_REPLACEMENT_CHAR;
        }

        {
            size_t need = sutf8_codepoint_length(cp);
            if ((size_t)(targetEnd - target) < need) {
                result = targetExhausted;
                break;
            }
            target += sutf8_put(target, cp);
        }
        source++;
    }

    *sourceStart = source;
    *targetStart = target;
    return result;
}

/* =====================================================================
    SUTF-16 -> SUTF-32
===================================================================== */
ConversionResult ConvertSUTF16toSUTF32(
        const SUTF16** sourceStart, const SUTF16* sourceEnd,
        SUTF32** targetStart, SUTF32* targetEnd, ConversionFlags flags) {
    const SUTF16* source;
    SUTF32* target;
    ConversionResult result = conversionOK;

    if (!sourceStart || !sourceEnd || !targetStart || !targetEnd) {
        return sourceIllegal;
    }

    source = *sourceStart;
    target = *targetStart;

    while (source < sourceEnd) {
        sucs_char_t cp;
        size_t used;
        SutfDecodeStatus status = sutf16_decode_unit(source, sourceEnd, &cp, &used);

        if (status == SUTF_DECODE_TRUNCATED) {
            if (flags == lenientConversion) {
                if (target >= targetEnd) {
                    result = targetExhausted;
                    break;
                }
                *target++ = SUTF_REPLACEMENT_CHAR;
                source += used;
            }
            result = sourceExhausted;
            break;
        }

        if (status == SUTF_DECODE_INVALID) {
            if (flags == strictConversion) {
                result = sourceIllegal;
                break;
            }
            if (target >= targetEnd) {
                result = targetExhausted;
                break;
            }
            *target++ = SUTF_REPLACEMENT_CHAR;
            source += used;
            continue;
        }

        if (target >= targetEnd) {
            result = targetExhausted;
            break;
        }
        *target++ = (SUTF32)cp;
        source += used;
    }

    *sourceStart = source;
    *targetStart = target;
    return result;
}

/* =====================================================================
    SUTF-32 -> SUTF-16
===================================================================== */
ConversionResult ConvertSUTF32toSUTF16(
        const SUTF32** sourceStart, const SUTF32* sourceEnd,
        SUTF16** targetStart, SUTF16* targetEnd, ConversionFlags flags) {
    const SUTF32* source;
    SUTF16* target;
    ConversionResult result = conversionOK;

    if (!sourceStart || !sourceEnd || !targetStart || !targetEnd) {
        return sourceIllegal;
    }

    source = *sourceStart;
    target = *targetStart;

    while (source < sourceEnd) {
        sucs_char_t cp = (sucs_char_t)(*source);

        if (!sucs_is_valid(cp)) {
            if (flags == strictConversion) {
                result = sourceIllegal;
                break;
            }
            cp = (sucs_char_t)SUTF_REPLACEMENT_CHAR;
        }

        {
            size_t need = sutf16_codepoint_length(cp);
            if ((size_t)(targetEnd - target) < need) {
                result = targetExhausted;
                break;
            }
            target += sutf16_put(target, cp);
        }
        source++;
    }

    *sourceStart = source;
    *targetStart = target;
    return result;
}

/* =====================================================================
    Legality helpers
===================================================================== */
Boolean isLegalSUTF8Sequence(const SUTF8* source, const SUTF8* sourceEnd) {
    if (!source || !sourceEnd) {
        return 0;
    }

    while (source < sourceEnd) {
        sucs_char_t cp;
        size_t used;
        if (sutf8_decode_unit(source, sourceEnd, &cp, &used) != SUTF_DECODE_OK) {
            return 0;
        }
        source += used;
    }
    return 1;
}

Boolean isLegalSUTF16Sequence(const SUTF16* source, const SUTF16* sourceEnd) {
    if (!source || !sourceEnd) {
        return 0;
    }

    while (source < sourceEnd) {
        sucs_char_t cp;
        size_t used;
        if (sutf16_decode_unit(source, sourceEnd, &cp, &used) != SUTF_DECODE_OK) {
            return 0;
        }
        source += used;
    }
    return 1;
}

Boolean isLegalSUTF32Sequence(const SUTF32* source, const SUTF32* sourceEnd) {
    if (!source || !sourceEnd) {
        return 0;
    }

    while (source < sourceEnd) {
        if (!sucs_is_valid((sucs_char_t)(*source))) {
            return 0;
        }
        source++;
    }
    return 1;
}
