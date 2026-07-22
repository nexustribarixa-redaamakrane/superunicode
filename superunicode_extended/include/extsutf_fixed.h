#ifndef EXTSUTF_FIXED_H
#define EXTSUTF_FIXED_H

/**
 * extSUTF Fixed-Width Text Formatting and Serialization Transports
 *
 * These are strictly TEXT FORMATTING TRANSPORTS defining fixed-width
 * byte-packing and memory layout rules for storing and transmitting
 * ExtSUCS character encoding codepoints in aligned vector register slots.
 *
 * SUTF-32:  4-byte  (32-bit)  alignment — Base SUCS fast-path container
 * SUTF-64:  8-byte  (64-bit)  alignment — SIMD / AI tensor slot
 * SUTF-128: 16-byte (128-bit) alignment — SSE / NEON vector register slot
 * SUTF-256: 32-byte (256-bit) alignment — AVX-256 vector register slot
 * SUTF-512: 64-byte (512-bit) alignment — AVX-512 vector register slot
 * SUTF-N:   N-word  arbitrary  alignment — Multi-word block container
 *
 * All formats use big-endian byte order with zero-padding in upper bytes.
 * Error handling is strictly out-of-band via return values.
 */

#include "extsucs_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * SUTF-32 Fixed-Width Text Formatting Transport (4 bytes)
 *
 * Encodes ExtSUCS codepoints into 4-byte big-endian containers.
 * Can only represent codepoints 0x00000000 to 0xFFFFFFFF (32-bit range).
 * Returns false / 0 if the codepoint exceeds the 32-bit addressable range.
 * ============================================================================ */
#define SUTF32_BYTES 4

size_t sutf32_encode(sucs_ex_char_t ex_cp, uint8_t* out_buf, size_t buf_size);
size_t sutf32_decode(const uint8_t* in_buf, size_t buf_size, sucs_ex_char_t* out_cp);

/* ============================================================================
 * SUTF-64 Fixed-Width Text Formatting Transport (8 bytes)
 *
 * Encodes ExtSUCS codepoints into 8-byte big-endian containers for
 * SIMD / AI tensor alignment and 64-bit bus-width transports.
 * Full 64-bit ExtSUCS range supported.
 * ============================================================================ */
#define SUTF64_BYTES 8

size_t sutf64_encode(sucs_ex_char_t ex_cp, uint8_t* out_buf, size_t buf_size);
size_t sutf64_decode(const uint8_t* in_buf, size_t buf_size, sucs_ex_char_t* out_cp);

/* ============================================================================
 * SUTF-128 Fixed-Width Text Formatting Transport (16 bytes)
 *
 * Encodes ExtSUCS codepoints into 16-byte zero-padded, big-endian aligned
 * vector register slots (SSE / NEON width).
 * ============================================================================ */
#define SUTF128_BYTES 16

size_t sutf128_encode(sucs_ex_char_t ex_cp, uint8_t* out_buf, size_t buf_size);
size_t sutf128_decode(const uint8_t* in_buf, size_t buf_size, sucs_ex_char_t* out_cp);

/* ============================================================================
 * SUTF-256 Fixed-Width Text Formatting Transport (32 bytes)
 *
 * Encodes ExtSUCS codepoints into 32-byte zero-padded, big-endian aligned
 * vector register slots (AVX-256 width).
 * ============================================================================ */
#define SUTF256_BYTES 32

size_t sutf256_encode(sucs_ex_char_t ex_cp, uint8_t* out_buf, size_t buf_size);
size_t sutf256_decode(const uint8_t* in_buf, size_t buf_size, sucs_ex_char_t* out_cp);

/* ============================================================================
 * SUTF-512 Fixed-Width Text Formatting Transport (64 bytes)
 *
 * Encodes ExtSUCS codepoints into 64-byte zero-padded, big-endian aligned
 * vector register slots (AVX-512 width).
 * ============================================================================ */
#define SUTF512_BYTES 64

size_t sutf512_encode(sucs_ex_char_t ex_cp, uint8_t* out_buf, size_t buf_size);
size_t sutf512_decode(const uint8_t* in_buf, size_t buf_size, sucs_ex_char_t* out_cp);

/* ============================================================================
 * SUTF-N Arbitrary Fixed-Width Text Formatting Transport
 *
 * Encodes an ExtSUCS codepoint into an N-byte zero-padded, big-endian
 * aligned multi-word block container. The caller specifies slot_bytes.
 * Minimum slot_bytes is 8 (for full 64-bit coverage).
 * ============================================================================ */
size_t sutfn_encode(sucs_ex_char_t ex_cp, uint8_t* out_buf, size_t slot_bytes);
size_t sutfn_decode(const uint8_t* in_buf, size_t slot_bytes, sucs_ex_char_t* out_cp);

#ifdef __cplusplus
}
#endif

#endif /* EXTSUTF_FIXED_H */
