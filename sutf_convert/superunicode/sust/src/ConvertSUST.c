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

    SUST-16 and SUST-32 are byte transports. The codepoint helpers therefore
    address bytes, while the SUCS helpers address sucs_char_t units. The
    canonical serialization used here is big-endian, matching sust16.h and
    sustfixed.h.

    NO SURROGATES: 0xD800-0xDFFF round-trip as ordinary PUA values. In
    SUST-16 they become the two-word form {0x8000|(cp>>16), cp & 0xFFFF}.
------------------------------------------------------------------------ */
#include "ConvertSUST.h"

typedef enum {
    SUST_DECODE_OK = 0,
    SUST_DECODE_INVALID,
    SUST_DECODE_TRUNCATED
} SustDecodeStatus;

/* ---------------------------------------------------------------------
    SUST-16 decode (canonical big-endian serialization)
------------------------------------------------------------------------ */
static SustDecodeStatus sust16_decode_unit(const SUST16* source,
                                           const SUST16* sourceEnd,
                                           sucs_char_t* out_cp,
                                           size_t* used) {
    uint16_t w0;
    sucs_char_t cp;
    size_t avail;

    *out_cp = SUCS_INVALID_CODEPOINT;
    *used = 0;

    if (!source || !sourceEnd || source >= sourceEnd) {
        return SUST_DECODE_TRUNCATED;
    }

    avail = (size_t)(sourceEnd - source);
    if (avail < 2) {
        *used = avail;
        return SUST_DECODE_TRUNCATED;
    }

    w0 = (uint16_t)(((uint16_t)source[0] << 8) | (uint16_t)source[1]);

    if (w0 & 0x8000U) {
        if (avail < 4) {
            *used = avail;
            return SUST_DECODE_TRUNCATED;
        }

        cp = (((sucs_char_t)(w0 & 0x7FFFU)) << 16) |
             (((sucs_char_t)source[2] << 8) | (sucs_char_t)source[3]);

        if (cp <= 0x7FFFUL) {
            *used = 4;
            return SUST_DECODE_INVALID; /* overlong */
        }
        if (!sucs_is_valid(cp)) {
            *used = 4;
            return SUST_DECODE_INVALID; /* trap range or Sentinel */
        }

        *out_cp = cp;
        *used = 4;
        return SUST_DECODE_OK;
    }

    cp = (sucs_char_t)w0;
    if (!sucs_is_valid(cp)) {
        *used = 2;
        return SUST_DECODE_INVALID;
    }

    *out_cp = cp;
    *used = 2;
    return SUST_DECODE_OK;
}

/* ---------------------------------------------------------------------
    SUST-32 decode (fixed 4-byte big-endian container)
------------------------------------------------------------------------ */
static SustDecodeStatus sust32_decode_unit(const SUST32* source,
                                           const SUST32* sourceEnd,
                                           sucs_char_t* out_cp,
                                           size_t* used) {
    sucs_char_t cp;
    size_t avail;

    *out_cp = SUCS_INVALID_CODEPOINT;
    *used = 0;

    if (!source || !sourceEnd || source >= sourceEnd) {
        return SUST_DECODE_TRUNCATED;
    }

    avail = (size_t)(sourceEnd - source);
    if (avail < SUST32_BYTES) {
        *used = avail;
        return SUST_DECODE_TRUNCATED;
    }

    cp = (((sucs_char_t)source[0]) << 24) |
         (((sucs_char_t)source[1]) << 16) |
         (((sucs_char_t)source[2]) << 8) |
         ((sucs_char_t)source[3]);

    if (!sucs_is_valid(cp)) {
        *used = SUST32_BYTES;
        return SUST_DECODE_INVALID; /* trap range or Sentinel */
    }

    *out_cp = cp;
    *used = SUST32_BYTES;
    return SUST_DECODE_OK;
}

/* ---------------------------------------------------------------------
    Encode helpers. Capacity has already been checked by the caller.
------------------------------------------------------------------------ */
static size_t sust16_put(SUST16* target, sucs_char_t cp) {
    return sust16_encode_bytes(cp, target, SUST16_MAX_BYTES);
}

static size_t sust32_put(SUST32* target, sucs_char_t cp) {
    return sust32_encode((sucs_ex_char_t)cp, target, SUST32_BYTES);
}

/* =====================================================================
    SUST-16 -> SUST-32
===================================================================== */
ConversionResult ConvertSUST16toSUST32(
        const SUST16** sourceStart, const SUST16* sourceEnd,
        SUST32** targetStart, SUST32* targetEnd, ConversionFlags flags) {
    const SUST16* source;
    SUST32* target;
    ConversionResult result = conversionOK;

    if (!sourceStart || !sourceEnd || !targetStart || !targetEnd) {
        return sourceIllegal;
    }

    source = *sourceStart;
    target = *targetStart;

    while (source < sourceEnd) {
        sucs_char_t cp;
        size_t used;
        SustDecodeStatus status = sust16_decode_unit(source, sourceEnd, &cp, &used);

        if (status == SUST_DECODE_TRUNCATED) {
            if (flags == lenientConversion) {
                if ((size_t)(targetEnd - target) < SUST32_BYTES) {
                    result = targetExhausted;
                    break;
                }
                target += sust32_put(target, SUST_REPLACEMENT_CHAR);
                source += used;
            }
            result = sourceExhausted;
            break;
        }

        if (status == SUST_DECODE_INVALID) {
            if (flags == strictConversion) {
                result = sourceIllegal;
                break;
            }
            if ((size_t)(targetEnd - target) < SUST32_BYTES) {
                result = targetExhausted;
                break;
            }
            target += sust32_put(target, SUST_REPLACEMENT_CHAR);
            source += used;
            continue;
        }

        if ((size_t)(targetEnd - target) < SUST32_BYTES) {
            result = targetExhausted;
            break;
        }
        target += sust32_put(target, cp);
        source += used;
    }

    *sourceStart = source;
    *targetStart = target;
    return result;
}

/* =====================================================================
    SUST-32 -> SUST-16
===================================================================== */
ConversionResult ConvertSUST32toSUST16(
        const SUST32** sourceStart, const SUST32* sourceEnd,
        SUST16** targetStart, SUST16* targetEnd, ConversionFlags flags) {
    const SUST32* source;
    SUST16* target;
    ConversionResult result = conversionOK;

    if (!sourceStart || !sourceEnd || !targetStart || !targetEnd) {
        return sourceIllegal;
    }

    source = *sourceStart;
    target = *targetStart;

    while (source < sourceEnd) {
        sucs_char_t cp;
        size_t used;
        SustDecodeStatus status = sust32_decode_unit(source, sourceEnd, &cp, &used);

        if (status == SUST_DECODE_TRUNCATED) {
            if (flags == lenientConversion) {
                size_t need = sust16_codepoint_bytes(SUST_REPLACEMENT_CHAR);
                if ((size_t)(targetEnd - target) < need) {
                    result = targetExhausted;
                    break;
                }
                target += sust16_put(target, SUST_REPLACEMENT_CHAR);
                source += used;
            }
            result = sourceExhausted;
            break;
        }

        if (status == SUST_DECODE_INVALID) {
            if (flags == strictConversion) {
                result = sourceIllegal;
                break;
            }
            {
                size_t need = sust16_codepoint_bytes(SUST_REPLACEMENT_CHAR);
                if ((size_t)(targetEnd - target) < need) {
                    result = targetExhausted;
                    break;
                }
                target += sust16_put(target, SUST_REPLACEMENT_CHAR);
            }
            source += used;
            continue;
        }

        {
            size_t need = sust16_codepoint_bytes(cp);
            if ((size_t)(targetEnd - target) < need) {
                result = targetExhausted;
                break;
            }
            target += sust16_put(target, cp);
            source += used;
        }
    }

    *sourceStart = source;
    *targetStart = target;
    return result;
}

/* =====================================================================
    SUST-16 -> sucs_char_t
===================================================================== */
ConversionResult ConvertSUST16toSUCS(
        const SUST16** sourceStart, const SUST16* sourceEnd,
        sucs_char_t** targetStart, sucs_char_t* targetEnd, ConversionFlags flags) {
    const SUST16* source;
    sucs_char_t* target;
    ConversionResult result = conversionOK;

    if (!sourceStart || !sourceEnd || !targetStart || !targetEnd) {
        return sourceIllegal;
    }

    source = *sourceStart;
    target = *targetStart;

    while (source < sourceEnd) {
        sucs_char_t cp;
        size_t used;
        SustDecodeStatus status = sust16_decode_unit(source, sourceEnd, &cp, &used);

        if (status == SUST_DECODE_TRUNCATED) {
            if (flags == lenientConversion) {
                if (target >= targetEnd) {
                    result = targetExhausted;
                    break;
                }
                *target++ = SUST_REPLACEMENT_CHAR;
                source += used;
            }
            result = sourceExhausted;
            break;
        }

        if (status == SUST_DECODE_INVALID) {
            if (flags == strictConversion) {
                result = sourceIllegal;
                break;
            }
            if (target >= targetEnd) {
                result = targetExhausted;
                break;
            }
            *target++ = SUST_REPLACEMENT_CHAR;
            source += used;
            continue;
        }

        if (target >= targetEnd) {
            result = targetExhausted;
            break;
        }
        *target++ = cp;
        source += used;
    }

    *sourceStart = source;
    *targetStart = target;
    return result;
}

/* =====================================================================
    sucs_char_t -> SUST-16
===================================================================== */
ConversionResult ConvertSUCStoSUST16(
        const sucs_char_t** sourceStart, const sucs_char_t* sourceEnd,
        SUST16** targetStart, SUST16* targetEnd, ConversionFlags flags) {
    const sucs_char_t* source;
    SUST16* target;
    ConversionResult result = conversionOK;

    if (!sourceStart || !sourceEnd || !targetStart || !targetEnd) {
        return sourceIllegal;
    }

    source = *sourceStart;
    target = *targetStart;

    while (source < sourceEnd) {
        sucs_char_t cp = *source;

        if (!sucs_is_valid(cp)) {
            if (flags == strictConversion) {
                result = sourceIllegal;
                break;
            }
            cp = SUST_REPLACEMENT_CHAR;
        }

        {
            size_t need = sust16_codepoint_bytes(cp);
            if ((size_t)(targetEnd - target) < need) {
                result = targetExhausted;
                break;
            }
            target += sust16_put(target, cp);
        }
        source++;
    }

    *sourceStart = source;
    *targetStart = target;
    return result;
}

/* =====================================================================
    SUST-32 -> sucs_char_t
===================================================================== */
ConversionResult ConvertSUST32toSUCS(
        const SUST32** sourceStart, const SUST32* sourceEnd,
        sucs_char_t** targetStart, sucs_char_t* targetEnd, ConversionFlags flags) {
    const SUST32* source;
    sucs_char_t* target;
    ConversionResult result = conversionOK;

    if (!sourceStart || !sourceEnd || !targetStart || !targetEnd) {
        return sourceIllegal;
    }

    source = *sourceStart;
    target = *targetStart;

    while (source < sourceEnd) {
        sucs_char_t cp;
        size_t used;
        SustDecodeStatus status = sust32_decode_unit(source, sourceEnd, &cp, &used);

        if (status == SUST_DECODE_TRUNCATED) {
            if (flags == lenientConversion) {
                if (target >= targetEnd) {
                    result = targetExhausted;
                    break;
                }
                *target++ = SUST_REPLACEMENT_CHAR;
                source += used;
            }
            result = sourceExhausted;
            break;
        }

        if (status == SUST_DECODE_INVALID) {
            if (flags == strictConversion) {
                result = sourceIllegal;
                break;
            }
            if (target >= targetEnd) {
                result = targetExhausted;
                break;
            }
            *target++ = SUST_REPLACEMENT_CHAR;
            source += used;
            continue;
        }

        if (target >= targetEnd) {
            result = targetExhausted;
            break;
        }
        *target++ = cp;
        source += used;
    }

    *sourceStart = source;
    *targetStart = target;
    return result;
}

/* =====================================================================
    sucs_char_t -> SUST-32
===================================================================== */
ConversionResult ConvertSUCStoSUST32(
        const sucs_char_t** sourceStart, const sucs_char_t* sourceEnd,
        SUST32** targetStart, SUST32* targetEnd, ConversionFlags flags) {
    const sucs_char_t* source;
    SUST32* target;
    ConversionResult result = conversionOK;

    if (!sourceStart || !sourceEnd || !targetStart || !targetEnd) {
        return sourceIllegal;
    }

    source = *sourceStart;
    target = *targetStart;

    while (source < sourceEnd) {
        sucs_char_t cp = *source;

        if (!sucs_is_valid(cp)) {
            if (flags == strictConversion) {
                result = sourceIllegal;
                break;
            }
            cp = SUST_REPLACEMENT_CHAR;
        }

        if ((size_t)(targetEnd - target) < SUST32_BYTES) {
            result = targetExhausted;
            break;
        }
        target += sust32_put(target, cp);
        source++;
    }

    *sourceStart = source;
    *targetStart = target;
    return result;
}

/* =====================================================================
    Legality helpers
===================================================================== */
Boolean isLegalSUST16Sequence(const SUST16* source, const SUST16* sourceEnd) {
    if (!source || !sourceEnd) {
        return 0;
    }

    while (source < sourceEnd) {
        sucs_char_t cp;
        size_t used;
        if (sust16_decode_unit(source, sourceEnd, &cp, &used) != SUST_DECODE_OK) {
            return 0;
        }
        source += used;
    }
    return 1;
}

Boolean isLegalSUST32Sequence(const SUST32* source, const SUST32* sourceEnd) {
    if (!source || !sourceEnd) {
        return 0;
    }

    while (source < sourceEnd) {
        sucs_char_t cp;
        size_t used;
        if (sust32_decode_unit(source, sourceEnd, &cp, &used) != SUST_DECODE_OK) {
            return 0;
        }
        source += used;
    }
    return 1;
}
