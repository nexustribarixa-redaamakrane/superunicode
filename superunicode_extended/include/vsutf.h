#ifndef VSUTF_H
#define VSUTF_H

/**
 * vSUTF (Variable SUTF) Transformation Format
 *
 * vSUTF is strictly a TRANSFORMATION FORMAT defining the variable-length
 * multi-byte mapping between ExtSUCS character encoding codepoints and
 * byte sequences, spanning the full unbounded (0 -> infinity) address space.
 *
 * NOTE: vSUTF is NOT LEB128 and NOT a uniform variable-length integer. It has
 * three regimes, distinguished by fixed prefix bytes.
 * Transport/0.1.0/transport/vsutf.txt previously described it as LEB128
 * while deferring here; this header is the normative definition.
 *
 * Stream Structure:
 * - Codepoints 0x00000000 to 0x7FFFFFFF (Base SUCS fast-path):
 *   Encoded using standard SUTF-8 transport framing (1 to 6 bytes), with
 *   exactly the SUTR-1 boundaries.
 *
 * - Codepoints 0x80000000 to 0xFFFFFFFFFFFFFFFF (64-bit extended range):
 *   Encoded with a 0xFE prefix + 8 bytes big-endian = 9 bytes total.
 *
 * - Codepoints above 0xFFFFFFFFFFFFFFFF (arbitrary-precision range):
 *     0xFF prefix
 *     + unsigned LEB128 byte count L (canonical, L >= 9)
 *     + L bytes of big-endian magnitude (no leading zero byte)
 *   There is NO maximum L. The size is limited only by the caller's buffer /
 *   limb capacity and the platform's address space, not by the format.
 *
 * Note: 0xFE and 0xFF are unused in SUTF-8 (which uses 0xC0-0xFD for lead
 * bytes), making them safe extension markers for vSUTF extended framing.
 * A LEB128-style variable-length integer could NOT coexist with SUTF-8 lead
 * bytes, because 0xFE and 0xFF would be ambiguous.
 *
 * Consequence: because the Base regime is byte-for-byte SUTF-8, any Base SUCS
 * stream is already a valid vSUTF stream. vSUTF is a strict superset of
 * SUTF-8; a decoder that understands the 0xFE and 0xFF escapes needs no
 * special case for Base text.
 *
 * Level: vSUTF emits bytes and exposes no host word, so it passes the
 * SUTR-7 byte-order test as endian-neutral -- it is a Level 3 CEF in nature,
 * not a Level 4 CES. See docs/sutr/SUTR-4-extsucs-transport.md.
 */

#include "extsucs_types.h"
#include "extsucs_big.h"

#ifdef __cplusplus
extern "C" {
#endif

/* vSUTF Transport Stream Constants */
#define VSUTF_EXT64_PREFIX    ((uint8_t)0xFE)  /* fixed 64-bit extended prefix */
#define VSUTF_BIG_PREFIX      ((uint8_t)0xFF)  /* arbitrary-precision prefix    */
#define VSUTF_RESERVED_PREFIX VSUTF_BIG_PREFIX /* legacy name; no longer unused */
#define VSUTF_EXT64_TOTAL     9                /* 1 prefix + 8 payload bytes   */
#define VSUTF_BASE_MAX_BYTES  6                /* max Base SUCS frame           */
#define VSUTF_EXT64_MAX_BYTES 9                /* max 64-bit fast-path frame    */

/*
 * Legacy compatibility constant: the maximum frame for the 64-bit fast path.
 * It is NOT a maximum for vSUTF as a whole; arbitrary-precision codepoints
 * have no fixed maximum frame size.
 */
#define VSUTF_MAX_BYTES       VSUTF_EXT64_MAX_BYTES

/**
 * Returns the vSUTF transport stream byte length for a 64-bit ExtSUCS
 * codepoint. Returns 0 if the codepoint is in the inherited trap range.
 *
 * For the Base range this reproduces the SUTF-8 length table exactly -- 1, 2,
 * 3, 4, 5, 6 bytes at the SUTR-1 boundaries -- so a Base codepoint's vSUTF
 * length equals its SUTF-8 length. Above 0x7FFFFFFF the length is constant
 * (VSUTF_EXT64_TOTAL), not variable.
 *
 * For arbitrary-precision values use vsutf_big_codepoint_length().
 */
static inline size_t vsutf_codepoint_length(sucs_ex_char_t ex_cp) {
    if (!extsucs_is_valid(ex_cp)) {
        return 0;
    }
    if (ex_cp <= 0x7FULL) {
        return 1;
    } else if (ex_cp <= 0x7FFULL) {
        return 2;
    } else if (ex_cp <= 0xFFFFULL) {
        return 3;
    } else if (ex_cp <= 0x0010FFFFULL) {
        /* 4-byte framing covers only the Unicode-compatible range */
        return 4;
    } else if (ex_cp <= 0x03FFFFFFULL) {
        /* 5-byte framing covers native extended space (0x110000-0x3FFFFFF) */
        return 5;
    } else if (ex_cp <= 0x7FFFFFFFULL) {
        return 6;
    } else {
        return VSUTF_EXT64_TOTAL; /* 0xFE prefix + 8 bytes */
    }
}

/**
 * Encodes a 64-bit ExtSUCS codepoint into the vSUTF variable-length stream.
 * Base SUCS range (0x00-0x7FFFFFFF): SUTF-8 framing (1-6 bytes).
 * Extended range (>0x7FFFFFFF): 0xFE prefix + 8-byte big-endian (9 bytes).
 * Returns bytes written, or 0 on error.
 *
 * This fast-path entry point cannot encode values above 2^64-1; use
 * vsutf_encode_big() for the full ExtSUCS space.
 */
size_t vsutf_encode(sucs_ex_char_t ex_cp, uint8_t* out_buf, size_t buf_size);

/**
 * Decodes a vSUTF transport stream into a 64-bit ExtSUCS codepoint.
 * Returns bytes consumed, or 0 on error (out_cp left unchanged on failure).
 *
 * A 0xFF (arbitrary-precision) frame is rejected by this entry point; use
 * vsutf_decode_big() when the stream may carry values above 2^64-1.
 */
size_t vsutf_decode(const uint8_t* in_buf, size_t buf_size, sucs_ex_char_t* out_cp);

/* ============================================================================
 * Arbitrary-Precision (Infinite) Regime
 * ============================================================================ */

/**
 * Returns the total vSUTF byte length of an arbitrary-precision codepoint.
 * Values in the 64-bit fast path produce the same length as
 * vsutf_codepoint_length(). Values above 2^64-1 produce
 * 1 + LEB128(L) + L, where L is the minimal big-endian byte length.
 * Returns 0 for NULL or a trap-range value.
 */
size_t vsutf_big_codepoint_length(const sucs_ex_big_t* cp);

/**
 * Encodes an arbitrary-precision ExtSUCS codepoint.
 * 64-bit values use the compact 1-9 byte regimes; larger values use
 * 0xFF + LEB128(L) + L big-endian bytes.
 * Returns bytes written, or 0 on error / insufficient capacity.
 */
size_t vsutf_encode_big(const sucs_ex_big_t* cp, uint8_t* out_buf, size_t buf_size);

/**
 * Decodes one vSUTF codepoint (any regime) into arbitrary precision.
 * `out` must be bound to caller storage with extsucs_big_init().
 * Returns bytes consumed, or 0 on error / insufficient limb capacity.
 */
size_t vsutf_decode_big(const uint8_t* in_buf, size_t buf_size,
                        sucs_ex_big_t* out_cp);

/* ============================================================================
 * Streaming (no whole-value buffer, no fixed maximum)
 *
 * These entry points let a codepoint larger than any single buffer be
 * emitted or received incrementally. The writer/reader must process exactly
 * `len` bytes and return true on success.
 * ============================================================================ */

typedef bool (*vsutf_write_fn)(void* ctx, const uint8_t* data, size_t len);
typedef bool (*vsutf_read_fn)(void* ctx, uint8_t* data, size_t len);

/**
 * Encodes `cp` to a sink. Returns true on success. `*out_bytes` receives the
 * number of bytes written. The payload is emitted in bounded chunks, so no
 * allocation proportional to the codepoint size is required.
 */
bool vsutf_encode_big_stream(const sucs_ex_big_t* cp,
                             vsutf_write_fn write, void* ctx,
                             size_t* out_bytes);

/**
 * Decodes one codepoint from a source into `out_cp` (caller-bound storage).
 * Returns true on success. `*consumed` receives the number of bytes read.
 * The payload is received in bounded chunks; capacity is checked before the
 * payload is read.
 */
bool vsutf_decode_big_stream(vsutf_read_fn read, void* ctx,
                             sucs_ex_big_t* out_cp, size_t* consumed);

#ifdef __cplusplus
}
#endif

#endif /* VSUTF_H */
