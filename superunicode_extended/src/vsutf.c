/**
 * vSUTF (Variable SUTF) Text Formatting and Serialization Transport
 *
 * Implementation of the vSUTF multi-byte variable-length stream transport
 * for ExtSUCS character encoding codepoints across the full unbounded
 * (0 -> infinity) address space.
 *
 * Base SUCS fast-path (0x00-0x7FFFFFFF): Standard SUTF-8 framing (1-6 bytes).
 * 64-bit range (0x80000000-0xFFFFFFFFFFFFFFFF): 0xFE prefix + 8 bytes = 9.
 * Arbitrary precision (> 0xFFFFFFFFFFFFFFFF):
 *   0xFF + unsigned LEB128 payload length + big-endian payload.
 *   The payload length has no format-imposed maximum.
 *
 * Zero standard library dependencies.
 */

#include "vsutf.h"

/* ============================================================================
 * vSUTF Encode: ExtSUCS codepoint -> variable-length byte stream
 *
 * Base SUCS range uses SUTF-8 compatible framing.
 * Extended range uses 0xFE prefix + 8-byte big-endian payload.
 * ============================================================================ */
size_t vsutf_encode(sucs_ex_char_t ex_cp, uint8_t* out_buf, size_t buf_size) {
    if (!out_buf || !extsucs_is_valid(ex_cp)) {
        return 0;
    }

    /* Base SUCS fast-path: SUTF-8 compatible framing (1-6 bytes) */
    if (ex_cp <= 0x7FULL) {
        if (buf_size < 1) return 0;
        out_buf[0] = (uint8_t)(ex_cp & 0x7FULL);
        return 1;
    } else if (ex_cp <= 0x7FFULL) {
        if (buf_size < 2) return 0;
        out_buf[0] = (uint8_t)(0xC0ULL | ((ex_cp >> 6) & 0x1FULL));
        out_buf[1] = (uint8_t)(0x80ULL | (ex_cp & 0x3FULL));
        return 2;
    } else if (ex_cp <= 0xFFFFULL) {
        if (buf_size < 3) return 0;
        out_buf[0] = (uint8_t)(0xE0ULL | ((ex_cp >> 12) & 0x0FULL));
        out_buf[1] = (uint8_t)(0x80ULL | ((ex_cp >> 6) & 0x3FULL));
        out_buf[2] = (uint8_t)(0x80ULL | (ex_cp & 0x3FULL));
        return 3;
    } else if (ex_cp <= 0x0010FFFFULL) {
        /* 4-byte framing covers only the Unicode-compatible range (0x10000-0x10FFFF) */
        if (buf_size < 4) return 0;
        out_buf[0] = (uint8_t)(0xF0ULL | ((ex_cp >> 18) & 0x07ULL));
        out_buf[1] = (uint8_t)(0x80ULL | ((ex_cp >> 12) & 0x3FULL));
        out_buf[2] = (uint8_t)(0x80ULL | ((ex_cp >> 6) & 0x3FULL));
        out_buf[3] = (uint8_t)(0x80ULL | (ex_cp & 0x3FULL));
        return 4;
    } else if (ex_cp <= 0x03FFFFFFULL) {
        /* 5-byte framing covers native extended space (0x110000-0x3FFFFFF) */
        if (buf_size < 5) return 0;
        out_buf[0] = (uint8_t)(0xF8ULL | ((ex_cp >> 24) & 0x03ULL));
        out_buf[1] = (uint8_t)(0x80ULL | ((ex_cp >> 18) & 0x3FULL));
        out_buf[2] = (uint8_t)(0x80ULL | ((ex_cp >> 12) & 0x3FULL));
        out_buf[3] = (uint8_t)(0x80ULL | ((ex_cp >> 6) & 0x3FULL));
        out_buf[4] = (uint8_t)(0x80ULL | (ex_cp & 0x3FULL));
        return 5;
    } else if (ex_cp <= 0x7FFFFFFFULL) {
        if (buf_size < 6) return 0;
        out_buf[0] = (uint8_t)(0xFCULL | ((ex_cp >> 30) & 0x01ULL));
        out_buf[1] = (uint8_t)(0x80ULL | ((ex_cp >> 24) & 0x3FULL));
        out_buf[2] = (uint8_t)(0x80ULL | ((ex_cp >> 18) & 0x3FULL));
        out_buf[3] = (uint8_t)(0x80ULL | ((ex_cp >> 12) & 0x3FULL));
        out_buf[4] = (uint8_t)(0x80ULL | ((ex_cp >> 6) & 0x3FULL));
        out_buf[5] = (uint8_t)(0x80ULL | (ex_cp & 0x3FULL));
        return 6;
    }

    /* Extended range (> 0x7FFFFFFF): 0xFE prefix + 8-byte big-endian */
    if (buf_size < VSUTF_EXT64_TOTAL) return 0;
    out_buf[0] = VSUTF_EXT64_PREFIX;
    out_buf[1] = (uint8_t)((ex_cp >> 56) & 0xFFULL);
    out_buf[2] = (uint8_t)((ex_cp >> 48) & 0xFFULL);
    out_buf[3] = (uint8_t)((ex_cp >> 40) & 0xFFULL);
    out_buf[4] = (uint8_t)((ex_cp >> 32) & 0xFFULL);
    out_buf[5] = (uint8_t)((ex_cp >> 24) & 0xFFULL);
    out_buf[6] = (uint8_t)((ex_cp >> 16) & 0xFFULL);
    out_buf[7] = (uint8_t)((ex_cp >> 8)  & 0xFFULL);
    out_buf[8] = (uint8_t)(ex_cp         & 0xFFULL);
    return VSUTF_EXT64_TOTAL;
}

/* ============================================================================
 * vSUTF Decode: variable-length byte stream -> ExtSUCS codepoint
 *
 * Detects lead byte pattern to determine framing:
 * - 0x00-0x7F: 1-byte ASCII
 * - 0xC0-0xDF: 2-byte SUTF-8
 * - 0xE0-0xEF: 3-byte SUTF-8
 * - 0xF0-0xF7: 4-byte SUTF-8
 * - 0xF8-0xFB: 5-byte SUTF-8
 * - 0xFC-0xFD: 6-byte SUTF-8
 * - 0xFE:      9-byte extended (64-bit)
 * - 0xFF:      arbitrary precision (use vsutf_decode_big)
 * ============================================================================ */
size_t vsutf_decode(const uint8_t* in_buf, size_t buf_size, sucs_ex_char_t* out_cp) {
    if (!in_buf || !out_cp || buf_size == 0) {
        return 0;
    }

    uint8_t u0 = in_buf[0];

    /* 0xFF: arbitrary-precision frame — not representable in this fast path */
    if (u0 == VSUTF_BIG_PREFIX) {
        return 0;
    }

    /* 0xFE: Extended 64-bit transport (1 prefix + 8 payload = 9 bytes) */
    if (u0 == VSUTF_EXT64_PREFIX) {
        if (buf_size < VSUTF_EXT64_TOTAL) return 0;
        sucs_ex_char_t cp = 0;
        cp |= ((sucs_ex_char_t)in_buf[1]) << 56;
        cp |= ((sucs_ex_char_t)in_buf[2]) << 48;
        cp |= ((sucs_ex_char_t)in_buf[3]) << 40;
        cp |= ((sucs_ex_char_t)in_buf[4]) << 32;
        cp |= ((sucs_ex_char_t)in_buf[5]) << 24;
        cp |= ((sucs_ex_char_t)in_buf[6]) << 16;
        cp |= ((sucs_ex_char_t)in_buf[7]) << 8;
        cp |= ((sucs_ex_char_t)in_buf[8]);
        if (cp <= 0x7FFFFFFFULL) return 0; /* overlong: must use base framing */
        if (!extsucs_is_valid(cp)) return 0;
        *out_cp = cp;
        return VSUTF_EXT64_TOTAL;
    }

    /* Base SUCS fast-path: SUTF-8 compatible decoding (1-6 bytes) */
    if ((u0 & 0x80U) == 0x00U) {
        sucs_ex_char_t cp = (sucs_ex_char_t)u0;
        if (!extsucs_is_valid(cp)) return 0;
        *out_cp = cp;
        return 1;
    } else if ((u0 & 0xE0U) == 0xC0U) {
        if (buf_size < 2) return 0;
        if ((in_buf[1] & 0xC0U) != 0x80U) return 0;
        sucs_ex_char_t cp = (((sucs_ex_char_t)(u0 & 0x1FU)) << 6) |
                            ((sucs_ex_char_t)(in_buf[1] & 0x3FU));
        if (cp < 0x80ULL) return 0; /* overlong */
        if (!extsucs_is_valid(cp)) return 0;
        *out_cp = cp;
        return 2;
    } else if ((u0 & 0xF0U) == 0xE0U) {
        if (buf_size < 3) return 0;
        if ((in_buf[1] & 0xC0U) != 0x80U || (in_buf[2] & 0xC0U) != 0x80U) return 0;
        sucs_ex_char_t cp = (((sucs_ex_char_t)(u0 & 0x0FU)) << 12) |
                            (((sucs_ex_char_t)(in_buf[1] & 0x3FU)) << 6) |
                            ((sucs_ex_char_t)(in_buf[2] & 0x3FU));
        if (cp < 0x800ULL) return 0; /* overlong */
        if (!extsucs_is_valid(cp)) return 0;
        *out_cp = cp;
        return 3;
    } else if ((u0 & 0xF8U) == 0xF0U) {
        if (buf_size < 4) return 0;
        if ((in_buf[1] & 0xC0U) != 0x80U || (in_buf[2] & 0xC0U) != 0x80U ||
            (in_buf[3] & 0xC0U) != 0x80U) return 0;
        sucs_ex_char_t cp = (((sucs_ex_char_t)(u0 & 0x07U)) << 18) |
                            (((sucs_ex_char_t)(in_buf[1] & 0x3FU)) << 12) |
                            (((sucs_ex_char_t)(in_buf[2] & 0x3FU)) << 6) |
                            ((sucs_ex_char_t)(in_buf[3] & 0x3FU));
        if (cp < 0x10000ULL) return 0;     /* overlong (must be 3-byte) */
        if (cp > 0x0010FFFFULL) return 0;  /* must use 5-byte framing */
        if (!extsucs_is_valid(cp)) return 0;
        *out_cp = cp;
        return 4;
    } else if ((u0 & 0xFCU) == 0xF8U) {
        if (buf_size < 5) return 0;
        if ((in_buf[1] & 0xC0U) != 0x80U || (in_buf[2] & 0xC0U) != 0x80U ||
            (in_buf[3] & 0xC0U) != 0x80U || (in_buf[4] & 0xC0U) != 0x80U) return 0;
        sucs_ex_char_t cp = (((sucs_ex_char_t)(u0 & 0x03U)) << 24) |
                            (((sucs_ex_char_t)(in_buf[1] & 0x3FU)) << 18) |
                            (((sucs_ex_char_t)(in_buf[2] & 0x3FU)) << 12) |
                            (((sucs_ex_char_t)(in_buf[3] & 0x3FU)) << 6) |
                            ((sucs_ex_char_t)(in_buf[4] & 0x3FU));
        if (cp < 0x00110000ULL) return 0;  /* overlong (must be 4-byte) */
        if (cp > 0x03FFFFFFULL) return 0;  /* must use 6-byte framing */
        if (!extsucs_is_valid(cp)) return 0;
        *out_cp = cp;
        return 5;
    } else if ((u0 & 0xFEU) == 0xFCU) {
        if (buf_size < 6) return 0;
        if ((in_buf[1] & 0xC0U) != 0x80U || (in_buf[2] & 0xC0U) != 0x80U ||
            (in_buf[3] & 0xC0U) != 0x80U || (in_buf[4] & 0xC0U) != 0x80U ||
            (in_buf[5] & 0xC0U) != 0x80U) return 0;
        sucs_ex_char_t cp = (((sucs_ex_char_t)(u0 & 0x01U)) << 30) |
                            (((sucs_ex_char_t)(in_buf[1] & 0x3FU)) << 24) |
                            (((sucs_ex_char_t)(in_buf[2] & 0x3FU)) << 18) |
                            (((sucs_ex_char_t)(in_buf[3] & 0x3FU)) << 12) |
                            (((sucs_ex_char_t)(in_buf[4] & 0x3FU)) << 6) |
                            ((sucs_ex_char_t)(in_buf[5] & 0x3FU));
        if (cp < 0x04000000ULL) return 0; /* overlong (must be 5-byte) */
        if (!extsucs_is_valid(cp)) return 0;
        *out_cp = cp;
        return 6;
    }

    /* Continuation byte or invalid lead byte */
    return 0;
}

/* ============================================================================
 * LEB128 helpers (used only by the arbitrary-precision 0xFF regime)
 * ============================================================================ */

static size_t vsutf_leb128_size(size_t value) {
    size_t n = 1;
    while (value >= 0x80u) {
        value >>= 7;
        n++;
    }
    return n;
}

static size_t vsutf_leb128_write(size_t value, uint8_t *out) {
    size_t n = 0;
    do {
        uint8_t byte = (uint8_t)(value & 0x7Fu);
        value >>= 7;
        if (value != 0) {
            byte |= 0x80u;
        }
        out[n++] = byte;
    } while (value != 0);
    return n;
}

static bool vsutf_leb128_read(const uint8_t *in, size_t avail,
                              size_t *value, size_t *used) {
    if (!in || !value || !used) {
        return false;
    }

    const size_t bits = sizeof(size_t) * 8;
    size_t result = 0;
    size_t shift = 0;

    for (size_t i = 0; i < avail; i++) {
        uint8_t byte = in[i];
        uint8_t digit = (uint8_t)(byte & 0x7Fu);

        if (shift >= bits) {
            return false;
        }
        if ((size_t)digit > (((size_t)-1) >> shift)) {
            return false;
        }
        result |= ((size_t)digit) << shift;

        if ((byte & 0x80u) == 0) {
            /* Minimal encoding: a multi-byte form may not end in a zero group. */
            if (i > 0 && digit == 0) {
                return false;
            }
            *value = result;
            *used = i + 1;
            return true;
        }
        shift += 7;
    }
    return false;
}

/* ============================================================================
 * Arbitrary-Precision Encoding
 * ============================================================================ */

size_t vsutf_big_codepoint_length(const sucs_ex_big_t* cp) {
    if (!cp || !extsucs_big_is_valid(cp)) {
        return 0;
    }

    uint64_t fast;
    if (extsucs_big_to_u64(cp, &fast)) {
        return vsutf_codepoint_length((sucs_ex_char_t)fast);
    }

    size_t payload = extsucs_big_byte_length(cp);
    if (payload == 0) {
        return 0;
    }
    size_t len_len = vsutf_leb128_size(payload);
    if (payload > ((size_t)-1) - 1 - len_len) {
        return 0;
    }
    return 1 + len_len + payload;
}

size_t vsutf_encode_big(const sucs_ex_big_t* cp, uint8_t* out_buf, size_t buf_size) {
    if (!cp || !out_buf || !extsucs_big_is_valid(cp)) {
        return 0;
    }

    uint64_t fast;
    if (extsucs_big_to_u64(cp, &fast)) {
        return vsutf_encode((sucs_ex_char_t)fast, out_buf, buf_size);
    }

    size_t payload = extsucs_big_byte_length(cp);
    if (payload == 0) {
        return 0;
    }
    size_t len_len = vsutf_leb128_size(payload);
    if (payload > ((size_t)-1) - 1 - len_len) {
        return 0;
    }
    size_t total = 1 + len_len + payload;
    if (buf_size < total) {
        return 0;
    }

    size_t pos = 0;
    out_buf[pos++] = VSUTF_BIG_PREFIX;
    pos += vsutf_leb128_write(payload, out_buf + pos);

    size_t wrote = 0;
    if (!extsucs_big_to_bytes_be(cp, out_buf + pos, payload, &wrote) ||
        wrote != payload) {
        return 0;
    }
    return total;
}

size_t vsutf_decode_big(const uint8_t* in_buf, size_t buf_size,
                        sucs_ex_big_t* out_cp) {
    if (!in_buf || !out_cp || buf_size == 0) {
        return 0;
    }

    if (in_buf[0] != VSUTF_BIG_PREFIX) {
        sucs_ex_char_t fast = 0;
        size_t used = vsutf_decode(in_buf, buf_size, &fast);
        if (used == 0) {
            return 0;
        }
        if (!extsucs_big_assign_u64(out_cp, (uint64_t)fast)) {
            return 0;
        }
        return used;
    }

    size_t payload = 0;
    size_t len_used = 0;
    if (!vsutf_leb128_read(in_buf + 1, buf_size - 1, &payload, &len_used)) {
        return 0;
    }
    if (payload < 9) {
        return 0; /* values <= 2^64-1 must use the compact 0xFE/base regimes */
    }

    size_t pos = 1 + len_used;
    if (pos > buf_size || payload > buf_size - pos) {
        return 0;
    }
    if (in_buf[pos] == 0) {
        return 0; /* non-canonical: leading zero payload byte */
    }
    if (!extsucs_big_assign_bytes_be(out_cp, in_buf + pos, payload)) {
        return 0;
    }
    if (extsucs_big_byte_length(out_cp) != payload) {
        return 0;
    }
    return pos + payload;
}

/* ============================================================================
 * Streaming API (bounded memory, unbounded value)
 * ============================================================================ */

bool vsutf_encode_big_stream(const sucs_ex_big_t* cp,
                             vsutf_write_fn write, void* ctx,
                             size_t* out_bytes) {
    if (out_bytes) {
        *out_bytes = 0;
    }
    if (!cp || !write || !extsucs_big_is_valid(cp)) {
        return false;
    }

    uint64_t fast;
    if (extsucs_big_to_u64(cp, &fast)) {
        uint8_t frame[VSUTF_EXT64_MAX_BYTES];
        size_t n = vsutf_encode((sucs_ex_char_t)fast, frame, sizeof(frame));
        if (n == 0 || !write(ctx, frame, n)) {
            return false;
        }
        if (out_bytes) {
            *out_bytes = n;
        }
        return true;
    }

    size_t payload = extsucs_big_byte_length(cp);
    if (payload == 0) {
        return false;
    }
    size_t len_len = vsutf_leb128_size(payload);
    if (payload > ((size_t)-1) - 1 - len_len) {
        return false;
    }

    uint8_t header[1 + ((sizeof(size_t) * 8 + 6) / 7)];
    size_t header_len = 0;
    header[header_len++] = VSUTF_BIG_PREFIX;
    header_len += vsutf_leb128_write(payload, header + header_len);
    if (!write(ctx, header, header_len)) {
        return false;
    }

    size_t total = header_len;
    uint8_t chunk[64];
    size_t offset = 0;
    while (offset < payload) {
        size_t take = payload - offset;
        if (take > sizeof(chunk)) {
            take = sizeof(chunk);
        }
        if (!extsucs_big_bytes_be_slice(cp, offset, chunk, take) ||
            !write(ctx, chunk, take)) {
            return false;
        }
        offset += take;
        total += take;
    }

    if (out_bytes) {
        *out_bytes = total;
    }
    return true;
}

bool vsutf_decode_big_stream(vsutf_read_fn read, void* ctx,
                             sucs_ex_big_t* out_cp, size_t* consumed) {
    if (consumed) {
        *consumed = 0;
    }
    if (!read || !out_cp) {
        return false;
    }

    uint8_t lead = 0;
    size_t used = 0;
    if (!read(ctx, &lead, 1)) {
        return false;
    }
    used++;

    if (lead != VSUTF_BIG_PREFIX) {
        uint8_t frame[VSUTF_EXT64_MAX_BYTES];
        size_t need = 0;
        frame[0] = lead;

        if (lead < 0x80u) {
            need = 1;
        } else if ((lead & 0xE0u) == 0xC0u) {
            need = 2;
        } else if ((lead & 0xF0u) == 0xE0u) {
            need = 3;
        } else if ((lead & 0xF8u) == 0xF0u) {
            need = 4;
        } else if ((lead & 0xFCu) == 0xF8u) {
            need = 5;
        } else if ((lead & 0xFEu) == 0xFCu) {
            need = 6;
        } else if (lead == VSUTF_EXT64_PREFIX) {
            need = VSUTF_EXT64_TOTAL;
        } else {
            return false;
        }

        if (need > 1) {
            if (!read(ctx, frame + 1, need - 1)) {
                return false;
            }
            used += need - 1;
        }

        sucs_ex_char_t fast = 0;
        if (vsutf_decode(frame, need, &fast) != need) {
            return false;
        }
        if (!extsucs_big_assign_u64(out_cp, (uint64_t)fast)) {
            return false;
        }
        if (consumed) {
            *consumed = used;
        }
        return true;
    }

    /* 0xFF: read the canonical LEB128 byte count one byte at a time. */
    size_t payload = 0;
    size_t shift = 0;
    size_t len_bytes = 0;
    uint8_t last = 0;
    bool done = false;
    const size_t bits = sizeof(size_t) * 8;

    while (!done) {
        uint8_t byte = 0;
        if (!read(ctx, &byte, 1)) {
            return false;
        }
        used++;
        len_bytes++;

        if (shift >= bits) {
            return false;
        }
        uint8_t digit = (uint8_t)(byte & 0x7Fu);
        if ((size_t)digit > (((size_t)-1) >> shift)) {
            return false;
        }
        payload |= ((size_t)digit) << shift;

        last = byte;
        if ((byte & 0x80u) == 0) {
            done = true;
        } else {
            shift += 7;
        }
    }

    if (len_bytes > 1 && (uint8_t)(last & 0x7Fu) == 0) {
        return false; /* overlong length */
    }
    if (payload < 9) {
        return false;
    }
    if (payload > ((size_t)-1) - 3) {
        return false;
    }

    size_t limbs = (payload + 3) / 4;
    if (!out_cp->limbs || limbs > out_cp->cap) {
        return false;
    }
    for (size_t i = 0; i < limbs; i++) {
        out_cp->limbs[i] = 0;
    }
    out_cp->len = limbs;

    size_t got = 0;
    while (got < payload) {
        uint8_t chunk[64];
        size_t take = payload - got;
        if (take > sizeof(chunk)) {
            take = sizeof(chunk);
        }
        if (!read(ctx, chunk, take)) {
            return false;
        }
        used += take;

        if (got == 0 && chunk[0] == 0) {
            return false; /* non-canonical leading zero */
        }

        for (size_t j = 0; j < take; j++) {
            size_t from_msb = got + j;
            size_t from_lsb = payload - 1 - from_msb;
            size_t limb = from_lsb / 4;
            unsigned sh = (unsigned)((from_lsb % 4) * 8);
            out_cp->limbs[limb] |= ((uint32_t)chunk[j]) << sh;
        }
        got += take;
    }

    while (out_cp->len > 0 && out_cp->limbs[out_cp->len - 1] == 0) {
        out_cp->len--;
    }
    if (extsucs_big_byte_length(out_cp) != payload) {
        return false;
    }

    if (consumed) {
        *consumed = used;
    }
    return true;
}
