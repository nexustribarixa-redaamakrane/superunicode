#ifndef EXTSUCS_BIG_H
#define EXTSUCS_BIG_H

/**
 * ExtSUCS Arbitrary-Precision Codepoints
 *
 * ExtSUCS is an unbounded character encoding: a codepoint is a non-negative
 * integer with no maximum.  A fixed-width C integer cannot represent that
 * space, so this header defines the full-domain representation:
 *
 *   sucs_ex_big_t  - an arbitrary-length, little-endian base-2^32 integer.
 *
 * Storage is caller-owned and supplied through extsucs_big_init().  The
 * library imposes NO limb limit: a value may grow for as long as the caller
 * provides capacity.  There is no library-defined maximum codepoint and no
 * in-band sentinel.  (Any finite machine obviously has finite memory at a
 * given instant; "infinite" here means the format and API impose no bound.)
 *
 * `sucs_ex_char_t` (uint64_t, in extsucs_types.h) remains the 64-bit fast-path
 * container.  It is a convenience view of the low 2^64 addresses, NOT the
 * ExtSUCS address space.  Values above 0xFFFFFFFFFFFFFFFF are represented
 * with sucs_ex_big_t and transported through the 0xFF vSUTF regime.
 *
 * Canonical form: `len` is the number of significant limbs.  For a non-zero
 * value the most-significant limb MUST be non-zero.  Zero is represented by
 * `len == 0`.
 *
 * Freestanding C99: no allocation, no libc.
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t *limbs; /* little-endian base-2^32 digits, caller-owned */
    size_t    len;   /* significant limbs; 0 == the value zero          */
    size_t    cap;   /* capacity of limbs[], in limbs                   */
} sucs_ex_big_t;

/* ==========================================================================
 * Lifecycle
 * ========================================================================== */

/**
 * Binds caller storage to a codepoint and sets it to zero.
 * `storage` may be NULL only when `cap == 0` (a value that can hold only zero).
 * Returns false if the arguments are inconsistent.
 */
bool extsucs_big_init(sucs_ex_big_t *v, uint32_t *storage, size_t cap);

/** Sets the value to zero without changing its storage binding. */
void extsucs_big_clear(sucs_ex_big_t *v);

/* ==========================================================================
 * Assignment / conversion
 * ========================================================================== */

/** Assigns a 64-bit value. Returns false if capacity is insufficient. */
bool extsucs_big_assign_u64(sucs_ex_big_t *v, uint64_t value);

/** Deep-copies a value into dst. Returns false if dst capacity is insufficient. */
bool extsucs_big_copy(sucs_ex_big_t *dst, const sucs_ex_big_t *src);

/**
 * Assigns a big-endian byte string. Leading zero bytes are accepted and
 * canonicalised away. Returns false if capacity is insufficient.
 */
bool extsucs_big_assign_bytes_be(sucs_ex_big_t *v,
                                 const uint8_t *bytes, size_t n);

/**
 * Writes the minimal big-endian representation (no leading zero byte).
 * `*out_len` receives the required length.  If `cap` is too small, returns
 * false and still reports the required length.  Zero writes zero bytes.
 */
bool extsucs_big_to_bytes_be(const sucs_ex_big_t *v,
                             uint8_t *out, size_t cap, size_t *out_len);

/**
 * Writes a slice of the minimal big-endian representation.
 * `offset == 0` is the most-significant byte.  Returns false if the requested
 * slice lies outside the representation.
 */
bool extsucs_big_bytes_be_slice(const sucs_ex_big_t *v,
                                size_t offset, uint8_t *out, size_t len);

/** Extracts the low 64 bits. Returns false if the value needs more than 64 bits. */
bool extsucs_big_to_u64(const sucs_ex_big_t *v, uint64_t *out);

/* ==========================================================================
 * Inspection
 * ========================================================================== */

int    extsucs_big_compare(const sucs_ex_big_t *a, const sucs_ex_big_t *b);
bool   extsucs_big_is_zero(const sucs_ex_big_t *v);
size_t extsucs_big_bit_length(const sucs_ex_big_t *v);
size_t extsucs_big_byte_length(const sucs_ex_big_t *v);

/* Number of limbs needed to hold `byte_count` bytes. */
size_t extsucs_big_limbs_for_bytes(size_t byte_count);

/* ==========================================================================
 * ExtSUCS domain predicates
 * ========================================================================== */

/**
 * Valid for every non-negative value except the inherited Kernel Security
 * Trap Range (0x7FFFFFF0..0x7FFFFFFE).  Values above 0x7FFFFFFE are always
 * valid, because the trap range lies entirely in the 64-bit fast path.
 */
bool extsucs_big_is_valid(const sucs_ex_big_t *v);

/** True iff the value is inside the Base SUCS 31-bit fast path. */
bool extsucs_big_is_base_sucs(const sucs_ex_big_t *v);

/**
 * Safe narrow to Base SUCS.  Mirrors sucs_downcast(): rejects values above
 * 0x7FFFFFFF, the Base sentinel 0x7FFFFFFF, and the trap range.
 */
bool extsucs_big_downcast(const sucs_ex_big_t *v, uint32_t *out_cp);

/* ==========================================================================
 * Small arithmetic (sufficient for streaming and test construction)
 * ========================================================================== */

/** Adds a 32-bit addend in place. Returns false on capacity overflow. */
bool extsucs_big_add_u32(sucs_ex_big_t *v, uint32_t addend);

#ifdef __cplusplus
}
#endif

#endif /* EXTSUCS_BIG_H */
