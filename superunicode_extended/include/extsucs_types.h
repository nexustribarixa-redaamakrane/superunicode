#ifndef EXTSUCS_TYPES_H
#define EXTSUCS_TYPES_H

/**
 * ExtSUCS (SuperUnicode Extended) Character Encoding Specification
 *
 * ExtSUCS is strictly a CHARACTER ENCODING defining an abstract,
 * unbounded codepoint numerical address space (0 -> infinity).
 * The encoding itself has NO upper limit — it is conceptually infinite.
 *
 * IMPLEMENTATION NOTE: The current C99 implementation uses uint64_t as a
 * finite container, which can address codepoints 0 through 2^64-1. This
 * does NOT define the encoding's boundary — it is merely the widest
 * integer type available in this implementation. Future implementations
 * may use wider types (uint128_t, arbitrary-precision integers, etc.)
 * to address higher regions of the infinite ExtSUCS encoding space.
 *
 * Within the current 64-bit container, ALL values are valid codepoint
 * addresses with the sole exception of the inherited Base SUCS Kernel
 * Security Trap Range (0x7FFFFFF0 - 0x7FFFFFFE).
 *
 * IMPORTANT: 0x7FFFFFFF (SUCS_INVALID_CODEPOINT) is the boundary sentinel
 * of the 31-bit Base SUCS encoding only. In ExtSUCS (0 -> infinity), the
 * value 0x7FFFFFFF IS a valid codepoint address — ExtSUCS has no upper
 * boundary and therefore no in-band sentinel of any kind.
 *
 * Error handling is strictly OUT-OF-BAND: functions return bool/size_t
 * success indicators and populate decoded codepoints via output pointers.
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* ============================================================================
 * ExtSUCS Character Encoding Type (64-bit implementation of unbounded space)
 * ============================================================================ */
typedef uint64_t sucs_ex_char_t;

/* ============================================================================
 * Base SUCS Character Encoding Type (31-bit bounded space, inherited)
 * ============================================================================ */
typedef uint32_t sucs_char_t;

/* ============================================================================
 * Base SUCS Character Encoding Sentinels & Boundaries
 *
 * These constants define the 31-bit Base SUCS bounded encoding:
 * - SUCS_INVALID_CODEPOINT (0x7FFFFFFF): Sentinel for Base SUCS ONLY.
 *   This value IS a valid ExtSUCS codepoint (ExtSUCS is unbounded).
 * - SUCS_TRAP_RANGE: Inherited by ExtSUCS — reserved across BOTH encodings.
 * ============================================================================ */
#define SUCS_INVALID_CODEPOINT  ((sucs_char_t)0x7FFFFFFFUL)
#define SUCS_MAX_CODEPOINT      ((sucs_char_t)0x7FFFFFFFUL)
#define SUCS_TRAP_RANGE_MIN     ((sucs_char_t)0x7FFFFFF0UL)
#define SUCS_TRAP_RANGE_MAX     ((sucs_char_t)0x7FFFFFFEUL)

/* ============================================================================
 * ExtSUCS Character Encoding Boundary
 *
 * Marks the upper limit of the Base SUCS 31-bit fast-path range within
 * the unbounded ExtSUCS space. NOT a sentinel — just a range marker.
 * ============================================================================ */
#define EXTSUCS_BASE_SUCS_MAX   ((sucs_ex_char_t)0x7FFFFFFFULL)

/* ============================================================================
 * Codepoint Validators
 *
 * extsucs_is_valid(): Validates an ExtSUCS 64-bit codepoint address.
 *   - Returns false ONLY for the inherited Kernel Security Trap Range
 *     (0x7FFFFFF0 - 0x7FFFFFFE). These addresses are reserved hardware
 *     trap slots across BOTH Base SUCS and ExtSUCS.
 *   - Returns true for ALL other 64-bit values, INCLUDING 0x7FFFFFFF
 *     (which is a valid codepoint in the unbounded ExtSUCS space).
 *
 * extsucs_is_base_sucs(): Tests whether an ExtSUCS codepoint falls within
 *   the Base SUCS 31-bit fast-path range (0x00000000 - 0x7FFFFFFF).
 * ============================================================================ */
static inline bool extsucs_is_valid(sucs_ex_char_t ex_cp) {
    /* Inherited Kernel Security Trap Range — reserved across both encodings */
    if (ex_cp >= (sucs_ex_char_t)SUCS_TRAP_RANGE_MIN &&
        ex_cp <= (sucs_ex_char_t)SUCS_TRAP_RANGE_MAX) {
        return false;
    }
    return true;
}

static inline bool extsucs_is_base_sucs(sucs_ex_char_t ex_cp) {
    return ex_cp <= EXTSUCS_BASE_SUCS_MAX;
}

/* ============================================================================
 * Zero-Cost Upcasting: Base SUCS (31-bit) -> ExtSUCS (64-bit)
 *
 * Pure widening cast. No validation needed — all valid Base SUCS codepoints
 * are valid ExtSUCS codepoints, and the Base SUCS sentinel 0x7FFFFFFF
 * becomes a valid codepoint in the unbounded ExtSUCS space.
 * ============================================================================ */
static inline sucs_ex_char_t sucs_upcast(sucs_char_t cp) {
    return (sucs_ex_char_t)cp;
}

/* ============================================================================
 * Safe Downcasting: ExtSUCS (64-bit) -> Base SUCS (31-bit)
 *
 * Returns true on success, false if the codepoint exceeds the Base SUCS
 * 31-bit boundary (> 0x7FFFFFFF), equals the Base SUCS sentinel
 * (0x7FFFFFFF), or falls in the trap range.
 *
 * Note: 0x7FFFFFFF is valid in ExtSUCS but is the sentinel in Base SUCS,
 * so downcasting it fails — it cannot be represented in Base SUCS.
 *
 * Out-of-band error signaling: success/failure via return value,
 * decoded codepoint via output pointer.
 * ============================================================================ */
static inline bool sucs_downcast(sucs_ex_char_t ex_cp, sucs_char_t* out_cp) {
    if (!out_cp) {
        return false;
    }
    if (ex_cp > EXTSUCS_BASE_SUCS_MAX) {
        *out_cp = SUCS_INVALID_CODEPOINT;
        return false;
    }
    sucs_char_t cp = (sucs_char_t)(ex_cp & 0x7FFFFFFFUL);
    /* Reject Base SUCS sentinel — valid in ExtSUCS but not in Base SUCS */
    if (cp == SUCS_INVALID_CODEPOINT) {
        *out_cp = SUCS_INVALID_CODEPOINT;
        return false;
    }
    /* Reject Kernel Security Trap Range */
    if (cp >= SUCS_TRAP_RANGE_MIN && cp <= SUCS_TRAP_RANGE_MAX) {
        *out_cp = SUCS_INVALID_CODEPOINT;
        return false;
    }
    *out_cp = cp;
    return true;
}

#endif /* EXTSUCS_TYPES_H */
