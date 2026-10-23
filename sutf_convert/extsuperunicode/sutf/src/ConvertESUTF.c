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
    vSUTF conversions for the unbounded ExtSUCS domain.

    The vSUTF frame layout is owned by vsutf.c; this file owns the streaming
    conversion semantics around it. Frame measurement is done locally so a
    truncated frame can be distinguished from an illegal one, and so an
    arbitrary-precision 0xFF frame can be skipped or capacity-checked before
    any payload is materialised.

    There is no 0x10FFFF ceiling in ExtSUCS and no in-band Sentinel. Only
    the inherited Kernel Security Trap range (0x7FFFFFF0-0x7FFFFFFE) is
    illegal. ESUTF32 is a compatibility view capped at 0xFFFFFFFF; values
    above it are legal ExtSUCS but unrepresentable in that view.
------------------------------------------------------------------------ */
#include "ConvertESUTF.h"

typedef enum {
    ESUTF_DECODE_OK = 0,
    ESUTF_DECODE_INVALID,
    ESUTF_DECODE_TRUNCATED,
    ESUTF_DECODE_UNREPRESENTABLE,
    ESUTF_DECODE_TARGET_EXHAUSTED
} EsutfDecodeStatus;

typedef enum {
    ESUTF_FRAME_OK = 0,
    ESUTF_FRAME_INVALID,
    ESUTF_FRAME_TRUNCATED
} EsutfFrameStatus;

/* ---------------------------------------------------------------------
    Measure one vSUTF frame.

    On success, *frame_len is the total frame length and *payload_len is the
    big-endian payload length (zero for the base and 0xFE regimes). On
    failure, *skip_len is the number of source bytes that belong to the
    malformed or truncated value for lenient resynchronisation.
------------------------------------------------------------------------ */
static EsutfFrameStatus esutf8_measure(const ESUTF8* source,
                                       const ESUTF8* sourceEnd,
                                       size_t* frame_len,
                                       size_t* payload_len,
                                       size_t* skip_len,
                                       int* is_big) {
    size_t avail;
    uint8_t u0;
    size_t length;

    *frame_len = 0;
    *payload_len = 0;
    *skip_len = 0;
    *is_big = 0;

    if (!source || !sourceEnd || source >= sourceEnd) {
        return ESUTF_FRAME_TRUNCATED;
    }

    avail = (size_t)(sourceEnd - source);
    u0 = source[0];

    if (u0 < 0x80U) {
        *frame_len = 1;
        return ESUTF_FRAME_OK;
    }

    if (u0 < 0xC0U) {
        *skip_len = 1; /* bare continuation byte */
        return ESUTF_FRAME_INVALID;
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
    } else if (u0 == 0xFEU) {
        length = VSUTF_EXT64_TOTAL;
    } else {
        /* 0xFF: arbitrary-precision regime. */
        const size_t bits = sizeof(size_t) * 8;
        size_t index = 1;
        size_t shift = 0;
        size_t value = 0;
        size_t count = 0;
        int done = 0;

        while (!done) {
            uint8_t byte;
            uint8_t digit;

            if (index >= avail) {
                *skip_len = avail;
                return ESUTF_FRAME_TRUNCATED;
            }

            byte = source[index++];
            count++;
            digit = (uint8_t)(byte & 0x7FU);

            if (shift >= bits ||
                (size_t)digit > (((size_t)-1) >> shift)) {
                *skip_len = index;
                return ESUTF_FRAME_INVALID;
            }
            value |= ((size_t)digit) << shift;

            if ((byte & 0x80U) == 0) {
                done = 1;
            } else {
                shift += 7;
            }
        }

        /* Canonical unsigned LEB128: no trailing zero group. */
        if (count > 1 && (source[index - 1] & 0x7FU) == 0) {
            *skip_len = index;
            return ESUTF_FRAME_INVALID;
        }

        /* Values through 2^64-1 must use the compact regimes. */
        if (value < 9) {
            *skip_len = index;
            return ESUTF_FRAME_INVALID;
        }

        if (value > ((size_t)-1) - index) {
            *skip_len = index;
            return ESUTF_FRAME_INVALID;
        }

        if (avail < index + value) {
            *skip_len = avail;
            return ESUTF_FRAME_TRUNCATED;
        }

        /* Canonical magnitude: no leading zero byte. */
        if (source[index] == 0) {
            *skip_len = index + value;
            return ESUTF_FRAME_INVALID;
        }

        *frame_len = index + value;
        *payload_len = value;
        *is_big = 1;
        return ESUTF_FRAME_OK;
    }

    if (avail < length) {
        *skip_len = avail;
        return ESUTF_FRAME_TRUNCATED;
    }

    if (length <= 6) {
        size_t i;
        for (i = 1; i < length; i++) {
            if ((source[i] & 0xC0U) != 0x80U) {
                *skip_len = 1;
                return ESUTF_FRAME_INVALID;
            }
        }
    }

    *frame_len = length;
    return ESUTF_FRAME_OK;
}

/* ---------------------------------------------------------------------
    Decode one frame into a 64-bit ExtSUCS value.
------------------------------------------------------------------------ */
static EsutfDecodeStatus esutf8_decode64(const ESUTF8* source,
                                         const ESUTF8* sourceEnd,
                                         sucs_ex_char_t* out_cp,
                                         size_t* used) {
    size_t frame_len;
    size_t payload_len;
    size_t skip_len;
    int is_big;
    EsutfFrameStatus frame;

    *out_cp = 0;
    *used = 0;

    frame = esutf8_measure(source, sourceEnd, &frame_len, &payload_len,
                           &skip_len, &is_big);
    if (frame == ESUTF_FRAME_TRUNCATED) {
        *used = skip_len;
        return ESUTF_DECODE_TRUNCATED;
    }
    if (frame == ESUTF_FRAME_INVALID) {
        *used = skip_len;
        return ESUTF_DECODE_INVALID;
    }

    (void)payload_len;

    if (is_big) {
        *used = frame_len;
        return ESUTF_DECODE_UNREPRESENTABLE;
    }

    {
        sucs_ex_char_t value = 0;
        if (vsutf_decode(source, frame_len, &value) != frame_len ||
            !extsucs_is_valid(value)) {
            *used = frame_len;
            return ESUTF_DECODE_INVALID;
        }
        *out_cp = value;
        *used = frame_len;
        return ESUTF_DECODE_OK;
    }
}

/* ---------------------------------------------------------------------
    Decode one frame into a 32-bit compatibility view.
------------------------------------------------------------------------ */
static EsutfDecodeStatus esutf8_decode32(const ESUTF8* source,
                                         const ESUTF8* sourceEnd,
                                         sucs_ex_char_t* out_cp,
                                         size_t* used) {
    EsutfDecodeStatus status = esutf8_decode64(source, sourceEnd, out_cp, used);

    if (status == ESUTF_DECODE_OK && *out_cp > ESUTF32_MAX) {
        return ESUTF_DECODE_UNREPRESENTABLE;
    }
    return status;
}

/* ---------------------------------------------------------------------
    Decode one frame into arbitrary precision.

    Capacity is checked before calling vsutf_decode_big(), so a failure
    leaves the caller's value untouched.
------------------------------------------------------------------------ */
static EsutfDecodeStatus esutf8_decode_big(const ESUTF8* source,
                                           const ESUTF8* sourceEnd,
                                           sucs_ex_big_t* target,
                                           size_t* used) {
    size_t frame_len;
    size_t payload_len;
    size_t skip_len;
    int is_big;
    EsutfFrameStatus frame;

    *used = 0;

    frame = esutf8_measure(source, sourceEnd, &frame_len, &payload_len,
                           &skip_len, &is_big);
    if (frame == ESUTF_FRAME_TRUNCATED) {
        *used = skip_len;
        return ESUTF_DECODE_TRUNCATED;
    }
    if (frame == ESUTF_FRAME_INVALID) {
        *used = skip_len;
        return ESUTF_DECODE_INVALID;
    }

    if (is_big) {
        size_t needed = extsucs_big_limbs_for_bytes(payload_len);
        if (!target || needed > target->cap ||
            (needed > 0 && !target->limbs)) {
            *used = frame_len;
            return ESUTF_DECODE_TARGET_EXHAUSTED;
        }
        if (vsutf_decode_big(source, frame_len, target) != frame_len) {
            *used = frame_len;
            return ESUTF_DECODE_INVALID;
        }
        *used = frame_len;
        return ESUTF_DECODE_OK;
    }

    {
        sucs_ex_char_t value = 0;
        size_t needed;

        if (vsutf_decode(source, frame_len, &value) != frame_len ||
            !extsucs_is_valid(value)) {
            *used = frame_len;
            return ESUTF_DECODE_INVALID;
        }

        needed = 0;
        if (value != 0) {
            needed = (value > 0xFFFFFFFFULL) ? 2 : 1;
        }
        if (!target || needed > target->cap ||
            (needed > 0 && !target->limbs)) {
            *used = frame_len;
            return ESUTF_DECODE_TARGET_EXHAUSTED;
        }
        if (!extsucs_big_assign_u64(target, (uint64_t)value)) {
            *used = frame_len;
            return ESUTF_DECODE_TARGET_EXHAUSTED;
        }
        *used = frame_len;
        return ESUTF_DECODE_OK;
    }
}

/* ---------------------------------------------------------------------
    Encode helpers. Return bytes written, or 0 when the target is too small.
------------------------------------------------------------------------ */
static size_t esutf8_put(ESUTF8* target, ESUTF8* targetEnd,
                         sucs_ex_char_t cp) {
    size_t need = vsutf_codepoint_length(cp);
    if (need == 0 || (size_t)(targetEnd - target) < need) {
        return 0;
    }
    return vsutf_encode(cp, target, need);
}

/* =====================================================================
    vSUTF -> ESUTF64
===================================================================== */
ConversionResult ConvertESUTF8toESUTF64(
        const ESUTF8** sourceStart, const ESUTF8* sourceEnd,
        ESUTF64** targetStart, ESUTF64* targetEnd, ConversionFlags flags) {
    const ESUTF8* source;
    ESUTF64* target;
    ConversionResult result = conversionOK;

    if (!sourceStart || !sourceEnd || !targetStart || !targetEnd) {
        return sourceIllegal;
    }

    source = *sourceStart;
    target = *targetStart;

    while (source < sourceEnd) {
        sucs_ex_char_t cp;
        size_t used;
        EsutfDecodeStatus status = esutf8_decode64(source, sourceEnd, &cp, &used);

        if (status == ESUTF_DECODE_TRUNCATED) {
            if (flags == lenientConversion) {
                if (target >= targetEnd) {
                    result = targetExhausted;
                    break;
                }
                *target++ = (ESUTF64)ESUTF_REPLACEMENT_CHAR;
                source += used;
            }
            result = sourceExhausted;
            break;
        }

        if (status == ESUTF_DECODE_INVALID ||
            status == ESUTF_DECODE_UNREPRESENTABLE) {
            if (flags == strictConversion) {
                result = sourceIllegal;
                break;
            }
            if (target >= targetEnd) {
                result = targetExhausted;
                break;
            }
            *target++ = (ESUTF64)ESUTF_REPLACEMENT_CHAR;
            source += used;
            continue;
        }

        if (status == ESUTF_DECODE_TARGET_EXHAUSTED) {
            result = targetExhausted;
            break;
        }

        if (target >= targetEnd) {
            result = targetExhausted;
            break;
        }
        *target++ = (ESUTF64)cp;
        source += used;
    }

    *sourceStart = source;
    *targetStart = target;
    return result;
}

/* =====================================================================
    ESUTF64 -> vSUTF
===================================================================== */
ConversionResult ConvertESUTF64toESUTF8(
        const ESUTF64** sourceStart, const ESUTF64* sourceEnd,
        ESUTF8** targetStart, ESUTF8* targetEnd, ConversionFlags flags) {
    const ESUTF64* source;
    ESUTF8* target;
    ConversionResult result = conversionOK;

    if (!sourceStart || !sourceEnd || !targetStart || !targetEnd) {
        return sourceIllegal;
    }

    source = *sourceStart;
    target = *targetStart;

    while (source < sourceEnd) {
        sucs_ex_char_t cp = (sucs_ex_char_t)(*source);
        size_t written;

        if (!extsucs_is_valid(cp)) {
            if (flags == strictConversion) {
                result = sourceIllegal;
                break;
            }
            cp = ESUTF_REPLACEMENT_CHAR;
        }

        written = esutf8_put(target, targetEnd, cp);
        if (written == 0) {
            result = targetExhausted;
            break;
        }
        target += written;
        source++;
    }

    *sourceStart = source;
    *targetStart = target;
    return result;
}

/* =====================================================================
    vSUTF -> ESUTF32
===================================================================== */
ConversionResult ConvertESUTF8toESUTF32(
        const ESUTF8** sourceStart, const ESUTF8* sourceEnd,
        ESUTF32** targetStart, ESUTF32* targetEnd, ConversionFlags flags) {
    const ESUTF8* source;
    ESUTF32* target;
    ConversionResult result = conversionOK;

    if (!sourceStart || !sourceEnd || !targetStart || !targetEnd) {
        return sourceIllegal;
    }

    source = *sourceStart;
    target = *targetStart;

    while (source < sourceEnd) {
        sucs_ex_char_t cp;
        size_t used;
        EsutfDecodeStatus status = esutf8_decode32(source, sourceEnd, &cp, &used);

        if (status == ESUTF_DECODE_TRUNCATED) {
            if (flags == lenientConversion) {
                if (target >= targetEnd) {
                    result = targetExhausted;
                    break;
                }
                *target++ = (ESUTF32)ESUTF_REPLACEMENT_CHAR;
                source += used;
            }
            result = sourceExhausted;
            break;
        }

        if (status == ESUTF_DECODE_INVALID ||
            status == ESUTF_DECODE_UNREPRESENTABLE) {
            if (flags == strictConversion) {
                result = sourceIllegal;
                break;
            }
            if (target >= targetEnd) {
                result = targetExhausted;
                break;
            }
            *target++ = (ESUTF32)ESUTF_REPLACEMENT_CHAR;
            source += used;
            continue;
        }

        if (status == ESUTF_DECODE_TARGET_EXHAUSTED) {
            result = targetExhausted;
            break;
        }

        if (target >= targetEnd) {
            result = targetExhausted;
            break;
        }
        *target++ = (ESUTF32)cp;
        source += used;
    }

    *sourceStart = source;
    *targetStart = target;
    return result;
}

/* =====================================================================
    ESUTF32 -> vSUTF
===================================================================== */
ConversionResult ConvertESUTF32toESUTF8(
        const ESUTF32** sourceStart, const ESUTF32* sourceEnd,
        ESUTF8** targetStart, ESUTF8* targetEnd, ConversionFlags flags) {
    const ESUTF32* source;
    ESUTF8* target;
    ConversionResult result = conversionOK;

    if (!sourceStart || !sourceEnd || !targetStart || !targetEnd) {
        return sourceIllegal;
    }

    source = *sourceStart;
    target = *targetStart;

    while (source < sourceEnd) {
        sucs_ex_char_t cp = (sucs_ex_char_t)(*source);
        size_t written;

        if (!extsucs_is_valid(cp)) {
            if (flags == strictConversion) {
                result = sourceIllegal;
                break;
            }
            cp = ESUTF_REPLACEMENT_CHAR;
        }

        written = esutf8_put(target, targetEnd, cp);
        if (written == 0) {
            result = targetExhausted;
            break;
        }
        target += written;
        source++;
    }

    *sourceStart = source;
    *targetStart = target;
    return result;
}

/* =====================================================================
    ESUTF64 -> ESUTF32
===================================================================== */
ConversionResult ConvertESUTF64toESUTF32(
        const ESUTF64** sourceStart, const ESUTF64* sourceEnd,
        ESUTF32** targetStart, ESUTF32* targetEnd, ConversionFlags flags) {
    const ESUTF64* source;
    ESUTF32* target;
    ConversionResult result = conversionOK;

    if (!sourceStart || !sourceEnd || !targetStart || !targetEnd) {
        return sourceIllegal;
    }

    source = *sourceStart;
    target = *targetStart;

    while (source < sourceEnd) {
        sucs_ex_char_t cp = (sucs_ex_char_t)(*source);

        if (!extsucs_is_valid(cp)) {
            if (flags == strictConversion) {
                result = sourceIllegal;
                break;
            }
            cp = ESUTF_REPLACEMENT_CHAR;
        } else if (cp > ESUTF32_MAX) {
            if (flags == strictConversion) {
                result = sourceIllegal;
                break;
            }
            cp = ESUTF_REPLACEMENT_CHAR;
        }

        if (target >= targetEnd) {
            result = targetExhausted;
            break;
        }
        *target++ = (ESUTF32)cp;
        source++;
    }

    *sourceStart = source;
    *targetStart = target;
    return result;
}

/* =====================================================================
    ESUTF32 -> ESUTF64
===================================================================== */
ConversionResult ConvertESUTF32toESUTF64(
        const ESUTF32** sourceStart, const ESUTF32* sourceEnd,
        ESUTF64** targetStart, ESUTF64* targetEnd, ConversionFlags flags) {
    const ESUTF32* source;
    ESUTF64* target;
    ConversionResult result = conversionOK;

    if (!sourceStart || !sourceEnd || !targetStart || !targetEnd) {
        return sourceIllegal;
    }

    source = *sourceStart;
    target = *targetStart;

    while (source < sourceEnd) {
        sucs_ex_char_t cp = (sucs_ex_char_t)(*source);

        if (!extsucs_is_valid(cp)) {
            if (flags == strictConversion) {
                result = sourceIllegal;
                break;
            }
            cp = ESUTF_REPLACEMENT_CHAR;
        }

        if (target >= targetEnd) {
            result = targetExhausted;
            break;
        }
        *target++ = (ESUTF64)cp;
        source++;
    }

    *sourceStart = source;
    *targetStart = target;
    return result;
}

/* =====================================================================
    vSUTF -> arbitrary precision (one codepoint per call)
===================================================================== */
ConversionResult ConvertESUTF8toSUCSBig(
        const ESUTF8** sourceStart, const ESUTF8* sourceEnd,
        sucs_ex_big_t* target, ConversionFlags flags) {
    sucs_ex_char_t cp;
    size_t used;
    EsutfDecodeStatus status;

    if (!sourceStart || !sourceEnd || !target) {
        return sourceIllegal;
    }

    status = esutf8_decode_big(*sourceStart, sourceEnd, target, &used);

    if (status == ESUTF_DECODE_OK || status == ESUTF_DECODE_UNREPRESENTABLE) {
        *sourceStart += used;
        return conversionOK;
    }

    if (status == ESUTF_DECODE_TARGET_EXHAUSTED) {
        return targetExhausted;
    }

    if (status == ESUTF_DECODE_TRUNCATED) {
        if (flags == lenientConversion) {
            if (target->cap < 1 || !target->limbs) {
                return targetExhausted;
            }
            cp = ESUTF_REPLACEMENT_CHAR;
            if (!extsucs_big_assign_u64(target, (uint64_t)cp)) {
                return targetExhausted;
            }
            *sourceStart += used;
        }
        return sourceExhausted;
    }

    /* ESUTF_DECODE_INVALID */
    if (flags == strictConversion) {
        return sourceIllegal;
    }
    if (target->cap < 1 || !target->limbs) {
        return targetExhausted;
    }
    cp = ESUTF_REPLACEMENT_CHAR;
    if (!extsucs_big_assign_u64(target, (uint64_t)cp)) {
        return targetExhausted;
    }
    *sourceStart += used;
    return conversionOK;
}

/* =====================================================================
    Arbitrary precision -> vSUTF (one codepoint per call)
===================================================================== */
ConversionResult ConvertSUCSBigtoESUTF8(
        const sucs_ex_big_t* source,
        ESUTF8** targetStart, ESUTF8* targetEnd, ConversionFlags flags) {
    size_t need;

    if (!source || !targetStart || !targetEnd) {
        return sourceIllegal;
    }

    need = vsutf_big_codepoint_length(source);
    if (need == 0) {
        if (flags == strictConversion) {
            return sourceIllegal;
        }
        need = vsutf_codepoint_length(ESUTF_REPLACEMENT_CHAR);
        if (need == 0 || (size_t)(targetEnd - *targetStart) < need) {
            return targetExhausted;
        }
        *targetStart += vsutf_encode(ESUTF_REPLACEMENT_CHAR, *targetStart, need);
        return conversionOK;
    }

    if ((size_t)(targetEnd - *targetStart) < need) {
        return targetExhausted;
    }

    if (vsutf_encode_big(source, *targetStart, need) != need) {
        return sourceIllegal;
    }

    *targetStart += need;
    return conversionOK;
}

/* =====================================================================
    Legality helper
===================================================================== */
Boolean isLegalESUTF8Sequence(const ESUTF8* source, const ESUTF8* sourceEnd) {
    if (!source || !sourceEnd) {
        return 0;
    }

    while (source < sourceEnd) {
        size_t frame_len;
        size_t payload_len;
        size_t skip_len;
        int is_big;
        EsutfFrameStatus frame = esutf8_measure(source, sourceEnd, &frame_len,
                                                &payload_len, &skip_len, &is_big);
        if (frame != ESUTF_FRAME_OK) {
            return 0;
        }

        if (!is_big) {
            sucs_ex_char_t cp = 0;
            if (vsutf_decode(source, frame_len, &cp) != frame_len ||
                !extsucs_is_valid(cp)) {
                return 0;
            }
        }
        /* 0xFF payloads above 2^64-1 are always valid ExtSUCS. */
        (void)payload_len;
        source += frame_len;
    }
    return 1;
}
