#ifndef SUST_SUST16_H
#define SUST_SUST16_H

#include "sucs_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * SUST-16 Serialization Transport
 *
 * SUST-16 is the canonical BIG-ENDIAN BYTE serialization of the SUTF-16
 * word transformation (see <sutf16.h>). SUTF-16 defines the codepoint <->
 * 16-bit word sequence mapping (endian-neutral, native word order); SUST-16
 * defines how those words are physically packed onto a byte medium — files,
 * sockets, buses, console dumps.
 *
 * Byte serialization (MANDATORY BIG-ENDIAN):
 * Every word is written high byte first. There is deliberately NO byte order
 * mark: every word >= 0x8000 is a framing marker, so no signature word can
 * exist. Use sust16_encode_bytes()/sust16_decode_bytes() for all byte-level
 * I/O; hand-rolled packing is the one way to corrupt a SUTF-16 stream
 * silently — a swapped stream either fails loudly (marker bit flips) or
 * silently decodes a different valid codepoint.
 */

/* Inline helper for SUST-16 serialized byte length (2 or 4) */
static inline size_t sust16_codepoint_bytes(sucs_char_t cp) {
    if (!sucs_is_valid(cp)) {
        return 0;
    }
    if (cp <= 0x7FFFUL) {
        return 2;
    } else {
        return 4;
    }
}

/**
 * Encodes a SUCS codepoint into a canonical BIG-ENDIAN SUST-16 byte stream
 * (mandatory order for all serialized SUTF-16). Returns bytes written
 * (2 or 4), or 0 on error. Freestanding-safe: no allocation.
 */
size_t sust16_encode_bytes(sucs_char_t cp, uint8_t* out_bytes, size_t buf_bytes);

/**
 * Decodes one codepoint from a canonical BIG-ENDIAN SUST-16 byte stream.
 * Returns bytes read (2 or 4), or 0 on error (yielding
 * SUCS_INVALID_CODEPOINT). A stream produced with any other byte order
 * fails loudly here: swapped bytes flip the marker bit, so misframed data
 * is rejected instead of silently decoding wrong values. Freestanding-safe:
 * no allocation.
 */
size_t sust16_decode_bytes(const uint8_t* in_bytes, size_t buf_bytes, sucs_char_t* out_cp);

#ifdef __cplusplus
}
#endif

#endif /* SUST_SUST16_H */