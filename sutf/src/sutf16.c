#include "sutf16.h"

size_t sutf16_encode_char(sucs_char_t cp, uint16_t* out_words, size_t buf_words) {
    if (!out_words || !sucs_is_valid(cp)) {
        return 0;
    }

    if (cp <= 0x7FFFUL) {
        if (buf_words < 1) return 0;
        out_words[0] = (uint16_t)(cp & 0x7FFFUL);
        return 1;
    } else {
        if (buf_words < 2) return 0;
        out_words[0] = (uint16_t)(0x8000U | ((cp >> 16) & 0x7FFFUL));
        out_words[1] = (uint16_t)(cp & 0xFFFFUL);
        return 2;
    }

    return 0;
}

size_t sutf16_decode_char(const uint16_t* in_words, size_t buf_words, sucs_char_t* out_cp) {
    if (!out_cp) {
        return 0;
    }
    *out_cp = SUCS_INVALID_CODEPOINT;

    if (!in_words || buf_words == 0) {
        return 0;
    }

    if (in_words[0] & 0x8000U) {
        /* Bit 15 is the 2-word marker and is never a literal, so a lone
         * marker word is a truncated 2-word sequence. */
        if (buf_words < 2) return 0;
        sucs_char_t cp = (((sucs_char_t)(in_words[0] & 0x7FFFU)) << 16) |
                         (sucs_char_t)in_words[1];
        /* Overlong: values that fit in the 1-word form (<= 0x7FFF) must not
         * be carried in the 2-word form. */
        if (cp <= 0x7FFFUL) return 0;
        if (!sucs_is_valid(cp)) return 0;
        *out_cp = cp;
        return 2;
    }

    sucs_char_t cp = (sucs_char_t)in_words[0];
    if (!sucs_is_valid(cp)) return 0;
    *out_cp = cp;
    return 1;
}

/* Canonical byte serialization: BIG-ENDIAN, high byte first. See the
 * header for why no byte order mark can exist in this framing. */

size_t sutf16_encode_bytes(sucs_char_t cp, uint8_t* out_bytes, size_t buf_bytes) {
    if (!out_bytes || !sucs_is_valid(cp)) {
        return 0;
    }

    if (cp <= 0x7FFFUL) {
        if (buf_bytes < 2) return 0;
        uint16_t w = (uint16_t)(cp & 0x7FFFUL);
        out_bytes[0] = (uint8_t)(w >> 8);
        out_bytes[1] = (uint8_t)(w & 0xFFU);
        return 2;
    }

    if (buf_bytes < 4) return 0;
    uint16_t w0 = (uint16_t)(0x8000U | ((cp >> 16) & 0x7FFFUL));
    uint16_t w1 = (uint16_t)(cp & 0xFFFFUL);
    out_bytes[0] = (uint8_t)(w0 >> 8);
    out_bytes[1] = (uint8_t)(w0 & 0xFFU);
    out_bytes[2] = (uint8_t)(w1 >> 8);
    out_bytes[3] = (uint8_t)(w1 & 0xFFU);
    return 4;
}

size_t sutf16_decode_bytes(const uint8_t* in_bytes, size_t buf_bytes, sucs_char_t* out_cp) {
    if (!out_cp) {
        return 0;
    }
    *out_cp = SUCS_INVALID_CODEPOINT;

    if (!in_bytes || buf_bytes < 2) {
        return 0;
    }

    /* First word decides the form; both bytes arrive big-endian. */
    uint16_t w0 = (uint16_t)(((uint16_t)in_bytes[0] << 8) | (uint16_t)in_bytes[1]);

    if (w0 & 0x8000U) {
        if (buf_bytes < 4) return 0;
        uint16_t w1 = (uint16_t)(((uint16_t)in_bytes[2] << 8) | (uint16_t)in_bytes[3]);
        sucs_char_t cp = (((sucs_char_t)(w0 & 0x7FFFU)) << 16) | (sucs_char_t)w1;
        if (cp <= 0x7FFFUL) return 0; /* overlong */
        if (!sucs_is_valid(cp)) return 0;
        *out_cp = cp;
        return 4;
    }

    sucs_char_t cp = (sucs_char_t)w0;
    if (!sucs_is_valid(cp)) return 0;
    *out_cp = cp;
    return 2;
}
