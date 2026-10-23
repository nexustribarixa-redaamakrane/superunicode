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
    Conversions between E-SUST-32, E-SUST-64, and ExtSUCS codepoints.

    E-SUST-32 is a 4-byte big-endian fixed-width container; E-SUST-64 is the
    8-byte big-endian full 64-bit fast path. Both use the framing defined by
    sustfixed.c. The ExtSUCS domain is unbounded, so only the addressable
    width of a container limits these conversions.

    The inherited trap range is rejected. 0x7FFFFFFF is a valid ExtSUCS
    codepoint and is preserved, unlike in Base SUCS where it is the Sentinel.
------------------------------------------------------------------------ */
#include "ConvertESUST.h"

typedef enum {
    ESUST_DECODE_OK = 0,
    ESUST_DECODE_INVALID,
    ESUST_DECODE_TRUNCATED
} EsustDecodeStatus;

/* ---------------------------------------------------------------------
    E-SUST-32 decode (4-byte big-endian)
------------------------------------------------------------------------ */
static EsustDecodeStatus esust32_decode_unit(const ESUST32* source,
                                             const ESUST32* sourceEnd,
                                             sucs_ex_char_t* out_cp,
                                             size_t* used) {
    sucs_ex_char_t cp;
    size_t avail;

    *out_cp = 0;
    *used = 0;

    if (!source || !sourceEnd || source >= sourceEnd) {
        return ESUST_DECODE_TRUNCATED;
    }

    avail = (size_t)(sourceEnd - source);
    if (avail < SUST32_BYTES) {
        *used = avail;
        return ESUST_DECODE_TRUNCATED;
    }

    cp = ((sucs_ex_char_t)source[0] << 24) |
         ((sucs_ex_char_t)source[1] << 16) |
         ((sucs_ex_char_t)source[2] << 8) |
         ((sucs_ex_char_t)source[3]);

    if (!extsucs_is_valid(cp)) {
        *used = SUST32_BYTES;
        return ESUST_DECODE_INVALID;
    }

    *out_cp = cp;
    *used = SUST32_BYTES;
    return ESUST_DECODE_OK;
}

/* ---------------------------------------------------------------------
    E-SUST-64 decode (8-byte big-endian)
------------------------------------------------------------------------ */
static EsustDecodeStatus esust64_decode_unit(const ESUST64* source,
                                             const ESUST64* sourceEnd,
                                             sucs_ex_char_t* out_cp,
                                             size_t* used) {
    sucs_ex_char_t cp;
    size_t avail;
    size_t i;

    *out_cp = 0;
    *used = 0;

    if (!source || !sourceEnd || source >= sourceEnd) {
        return ESUST_DECODE_TRUNCATED;
    }

    avail = (size_t)(sourceEnd - source);
    if (avail < SUST64_BYTES) {
        *used = avail;
        return ESUST_DECODE_TRUNCATED;
    }

    cp = 0;
    for (i = 0; i < SUST64_BYTES; i++) {
        cp = (cp << 8) | (sucs_ex_char_t)source[i];
    }

    if (!extsucs_is_valid(cp)) {
        *used = SUST64_BYTES;
        return ESUST_DECODE_INVALID;
    }

    *out_cp = cp;
    *used = SUST64_BYTES;
    return ESUST_DECODE_OK;
}

/* ---------------------------------------------------------------------
    Encode helpers. Capacity and domain have already been checked.
------------------------------------------------------------------------ */
static size_t esust32_put(ESUST32* target, sucs_ex_char_t cp) {
    return sust32_encode(cp, target, SUST32_BYTES);
}

static size_t esust64_put(ESUST64* target, sucs_ex_char_t cp) {
    return sust64_encode(cp, target, SUST64_BYTES);
}

/* =====================================================================
    E-SUST-32 -> E-SUST-64
===================================================================== */
ConversionResult ConvertESUST32toESUST64(
        const ESUST32** sourceStart, const ESUST32* sourceEnd,
        ESUST64** targetStart, ESUST64* targetEnd, ConversionFlags flags) {
    const ESUST32* source;
    ESUST64* target;
    ConversionResult result = conversionOK;

    if (!sourceStart || !sourceEnd || !targetStart || !targetEnd) {
        return sourceIllegal;
    }

    source = *sourceStart;
    target = *targetStart;

    while (source < sourceEnd) {
        sucs_ex_char_t cp;
        size_t used;
        EsustDecodeStatus status = esust32_decode_unit(source, sourceEnd, &cp, &used);

        if (status == ESUST_DECODE_TRUNCATED) {
            if (flags == lenientConversion) {
                if ((size_t)(targetEnd - target) < SUST64_BYTES) {
                    result = targetExhausted;
                    break;
                }
                target += esust64_put(target, ESUST_REPLACEMENT_CHAR);
                source += used;
            }
            result = sourceExhausted;
            break;
        }

        if (status == ESUST_DECODE_INVALID) {
            if (flags == strictConversion) {
                result = sourceIllegal;
                break;
            }
            if ((size_t)(targetEnd - target) < SUST64_BYTES) {
                result = targetExhausted;
                break;
            }
            target += esust64_put(target, ESUST_REPLACEMENT_CHAR);
            source += used;
            continue;
        }

        if ((size_t)(targetEnd - target) < SUST64_BYTES) {
            result = targetExhausted;
            break;
        }
        target += esust64_put(target, cp);
        source += used;
    }

    *sourceStart = source;
    *targetStart = target;
    return result;
}

/* =====================================================================
    E-SUST-64 -> E-SUST-32
===================================================================== */
ConversionResult ConvertESUST64toESUST32(
        const ESUST64** sourceStart, const ESUST64* sourceEnd,
        ESUST32** targetStart, ESUST32* targetEnd, ConversionFlags flags) {
    const ESUST64* source;
    ESUST32* target;
    ConversionResult result = conversionOK;

    if (!sourceStart || !sourceEnd || !targetStart || !targetEnd) {
        return sourceIllegal;
    }

    source = *sourceStart;
    target = *targetStart;

    while (source < sourceEnd) {
        sucs_ex_char_t cp;
        size_t used;
        EsustDecodeStatus status = esust64_decode_unit(source, sourceEnd, &cp, &used);

        if (status == ESUST_DECODE_TRUNCATED) {
            if (flags == lenientConversion) {
                if ((size_t)(targetEnd - target) < SUST32_BYTES) {
                    result = targetExhausted;
                    break;
                }
                target += esust32_put(target, ESUST_REPLACEMENT_CHAR);
                source += used;
            }
            result = sourceExhausted;
            break;
        }

        if (status == ESUST_DECODE_INVALID) {
            if (flags == strictConversion) {
                result = sourceIllegal;
                break;
            }
            if ((size_t)(targetEnd - target) < SUST32_BYTES) {
                result = targetExhausted;
                break;
            }
            target += esust32_put(target, ESUST_REPLACEMENT_CHAR);
            source += used;
            continue;
        }

        if (cp > ESUST32_MAX) {
            if (flags == strictConversion) {
                result = sourceIllegal;
                break;
            }
            cp = ESUST_REPLACEMENT_CHAR;
        }

        if ((size_t)(targetEnd - target) < SUST32_BYTES) {
            result = targetExhausted;
            break;
        }
        target += esust32_put(target, cp);
        source += used;
    }

    *sourceStart = source;
    *targetStart = target;
    return result;
}

/* =====================================================================
    E-SUST-32 -> sucs_ex_char_t
===================================================================== */
ConversionResult ConvertESUST32toESUCS(
        const ESUST32** sourceStart, const ESUST32* sourceEnd,
        sucs_ex_char_t** targetStart, sucs_ex_char_t* targetEnd,
        ConversionFlags flags) {
    const ESUST32* source;
    sucs_ex_char_t* target;
    ConversionResult result = conversionOK;

    if (!sourceStart || !sourceEnd || !targetStart || !targetEnd) {
        return sourceIllegal;
    }

    source = *sourceStart;
    target = *targetStart;

    while (source < sourceEnd) {
        sucs_ex_char_t cp;
        size_t used;
        EsustDecodeStatus status = esust32_decode_unit(source, sourceEnd, &cp, &used);

        if (status == ESUST_DECODE_TRUNCATED) {
            if (flags == lenientConversion) {
                if (target >= targetEnd) {
                    result = targetExhausted;
                    break;
                }
                *target++ = ESUST_REPLACEMENT_CHAR;
                source += used;
            }
            result = sourceExhausted;
            break;
        }

        if (status == ESUST_DECODE_INVALID) {
            if (flags == strictConversion) {
                result = sourceIllegal;
                break;
            }
            if (target >= targetEnd) {
                result = targetExhausted;
                break;
            }
            *target++ = ESUST_REPLACEMENT_CHAR;
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
    sucs_ex_char_t -> E-SUST-32
===================================================================== */
ConversionResult ConvertESUCStoESUST32(
        const sucs_ex_char_t** sourceStart, const sucs_ex_char_t* sourceEnd,
        ESUST32** targetStart, ESUST32* targetEnd, ConversionFlags flags) {
    const sucs_ex_char_t* source;
    ESUST32* target;
    ConversionResult result = conversionOK;

    if (!sourceStart || !sourceEnd || !targetStart || !targetEnd) {
        return sourceIllegal;
    }

    source = *sourceStart;
    target = *targetStart;

    while (source < sourceEnd) {
        sucs_ex_char_t cp = *source;

        if (!extsucs_is_valid(cp)) {
            if (flags == strictConversion) {
                result = sourceIllegal;
                break;
            }
            cp = ESUST_REPLACEMENT_CHAR;
        } else if (cp > ESUST32_MAX) {
            if (flags == strictConversion) {
                result = sourceIllegal;
                break;
            }
            cp = ESUST_REPLACEMENT_CHAR;
        }

        if ((size_t)(targetEnd - target) < SUST32_BYTES) {
            result = targetExhausted;
            break;
        }
        target += esust32_put(target, cp);
        source++;
    }

    *sourceStart = source;
    *targetStart = target;
    return result;
}

/* =====================================================================
    E-SUST-64 -> sucs_ex_char_t
===================================================================== */
ConversionResult ConvertESUST64toESUCS(
        const ESUST64** sourceStart, const ESUST64* sourceEnd,
        sucs_ex_char_t** targetStart, sucs_ex_char_t* targetEnd,
        ConversionFlags flags) {
    const ESUST64* source;
    sucs_ex_char_t* target;
    ConversionResult result = conversionOK;

    if (!sourceStart || !sourceEnd || !targetStart || !targetEnd) {
        return sourceIllegal;
    }

    source = *sourceStart;
    target = *targetStart;

    while (source < sourceEnd) {
        sucs_ex_char_t cp;
        size_t used;
        EsustDecodeStatus status = esust64_decode_unit(source, sourceEnd, &cp, &used);

        if (status == ESUST_DECODE_TRUNCATED) {
            if (flags == lenientConversion) {
                if (target >= targetEnd) {
                    result = targetExhausted;
                    break;
                }
                *target++ = ESUST_REPLACEMENT_CHAR;
                source += used;
            }
            result = sourceExhausted;
            break;
        }

        if (status == ESUST_DECODE_INVALID) {
            if (flags == strictConversion) {
                result = sourceIllegal;
                break;
            }
            if (target >= targetEnd) {
                result = targetExhausted;
                break;
            }
            *target++ = ESUST_REPLACEMENT_CHAR;
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
    sucs_ex_char_t -> E-SUST-64
===================================================================== */
ConversionResult ConvertESUCStoESUST64(
        const sucs_ex_char_t** sourceStart, const sucs_ex_char_t* sourceEnd,
        ESUST64** targetStart, ESUST64* targetEnd, ConversionFlags flags) {
    const sucs_ex_char_t* source;
    ESUST64* target;
    ConversionResult result = conversionOK;

    if (!sourceStart || !sourceEnd || !targetStart || !targetEnd) {
        return sourceIllegal;
    }

    source = *sourceStart;
    target = *targetStart;

    while (source < sourceEnd) {
        sucs_ex_char_t cp = *source;

        if (!extsucs_is_valid(cp)) {
            if (flags == strictConversion) {
                result = sourceIllegal;
                break;
            }
            cp = ESUST_REPLACEMENT_CHAR;
        }

        if ((size_t)(targetEnd - target) < SUST64_BYTES) {
            result = targetExhausted;
            break;
        }
        target += esust64_put(target, cp);
        source++;
    }

    *sourceStart = source;
    *targetStart = target;
    return result;
}

/* =====================================================================
    Legality helpers
===================================================================== */
Boolean isLegalESUST32Sequence(const ESUST32* source, const ESUST32* sourceEnd) {
    if (!source || !sourceEnd) {
        return 0;
    }

    while (source < sourceEnd) {
        sucs_ex_char_t cp;
        size_t used;
        if (esust32_decode_unit(source, sourceEnd, &cp, &used) != ESUST_DECODE_OK) {
            return 0;
        }
        source += used;
    }
    return 1;
}

Boolean isLegalESUST64Sequence(const ESUST64* source, const ESUST64* sourceEnd) {
    if (!source || !sourceEnd) {
        return 0;
    }

    while (source < sourceEnd) {
        sucs_ex_char_t cp;
        size_t used;
        if (esust64_decode_unit(source, sourceEnd, &cp, &used) != ESUST_DECODE_OK) {
            return 0;
        }
        source += used;
    }
    return 1;
}
