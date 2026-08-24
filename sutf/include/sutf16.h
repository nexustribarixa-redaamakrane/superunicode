#ifndef SUTF_SUTF16_H
#define SUTF_SUTF16_H

#include "sucs_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * SUTF-16 Text Formatting and Serialization Transport
 *
 * SUTF-16 defines 16-bit word-packing and memory layout transport rules
 * for storing and transmitting SUCS codepoints in 1 to 2 16-bit words.
 *
 * Framing (unambiguous by construction):
 * - 1-word form: 0x0000 - 0x7FFF  (literal value; bit 15 is clear).
 * - 2-word form: 0x8000 - 0x7FFFFFFF. The first word has bit 15 SET as the
 *   2-word marker and carries the high 15 bits; the second word carries the
 *   low 16 bits:
 *       word0 = 0x8000 | ((cp >> 16) & 0x7FFF)
 *       word1 = cp & 0xFFFF
 *   A word with bit 15 set is ALWAYS a marker and never a literal, so a
 *   stream such as {0x8000, 0xD800} is unambiguously one codepoint
 *   (0xD800), and a lone marker word is a detectable truncation error.
 *
 * Byte serialization (MANDATORY BIG-ENDIAN):
 * The word-array API above is endian-neutral (16-bit words in native
 * memory). Whenever a SUTF-16 stream is serialized to a BYTE medium —
 * files, sockets, buses, console dumps — the words MUST be written in
 * big-endian byte order (high byte first). There is deliberately NO byte
 * order mark: every word >= 0x8000 is a framing marker, so no signature
 * word can exist. Use sutf16_encode_bytes()/sutf16_decode_bytes() for all
 * byte-level I/O; hand-rolled packing is the one way to corrupt a SUTF-16
 * stream silently.
 */

/* Inline helper for SUTF-16 transport stream word length calculation */
static inline size_t sutf16_codepoint_length(sucs_char_t cp) {
    if (!sucs_is_valid(cp)) {
        return 0;
    }
    if (cp <= 0x7FFFUL) {
        return 1;
    } else {
        return 2;
    }
}

/**
 * Encodes a SUCS codepoint into the SUTF-16 text formatting and transport stream.
 * Returns 16-bit words written, or 0 on error.
 */
size_t sutf16_encode_char(sucs_char_t cp, uint16_t* out_words, size_t buf_words);

/**
 * Decodes a SUTF-16 transport stream into a SUCS character encoding codepoint.
 * Returns 16-bit words read, or 0 on error (yielding SUCS_INVALID_CODEPOINT).
 */
size_t sutf16_decode_char(const uint16_t* in_words, size_t buf_words, sucs_char_t* out_cp);

/**
 * Encodes a SUCS codepoint into a canonical BIG-ENDIAN SUTF-16 byte stream
 * (mandatory order for all serialized SUTF-16). Returns bytes written
 * (2 or 4), or 0 on error. Freestanding-safe: no allocation.
 */
size_t sutf16_encode_bytes(sucs_char_t cp, uint8_t* out_bytes, size_t buf_bytes);

/**
 * Decodes one codepoint from a canonical BIG-ENDIAN SUTF-16 byte stream.
 * Returns bytes read (2 or 4), or 0 on error (yielding
 * SUCS_INVALID_CODEPOINT). A stream produced with any other byte order
 * fails loudly here: swapped bytes flip the marker bit, so misframed data
 * is rejected instead of silently decoding wrong values. Freestanding-safe:
 * no allocation.
 */
size_t sutf16_decode_bytes(const uint8_t* in_bytes, size_t buf_bytes, sucs_char_t* out_cp);

#ifdef __cplusplus
}
#endif

#endif /* SUTF_SUTF16_H */
