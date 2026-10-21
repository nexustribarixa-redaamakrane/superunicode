#ifndef SUAS_SUAS_SUVTL_H
#define SUAS_SUAS_SUVTL_H

/* SUAS-014 — SuperUnicode Vertical Text Layout (SUVTL)
 *
 * The core algorithmic subsystem of the OpenWindows microkernel governing the
 * *orientation* of a character when a text stream flows in vertical columns:
 * whether a glyph stands upright, rotates ninety degrees clockwise, or
 * requires a distinct vertical form, together with a fine-grained vertical
 * baseline adjustment.
 *
 * UAX #50, "Unicode Vertical Text Layout", defines the Vertical_Orientation
 * (vo) character property in prose, and ships it as the text file
 * `VerticalOrientation.txt`: semicolon-delimited ranges, a four-value tag
 * (U / R / Tu / Tr), and narrative notes that every implementation reads and
 * interprets at run time. SUVTL is the runtime counterpart of that property, in
 * the same dual-layer relationship that SUAS-009 (SIP) has with UAX #38:
 *
 *   - the offline reference layer is the SuperUnicode Character Database
 *     (SUCD, SUAS-012): the `VerticalOrientation.txt` manifests, the
 *     per-script vertical-flow definitions, and the human-readable glyph-change
 *     tables for font designers, user-space layout engines and developer
 *     toolchains;
 *   - the runtime kernel layer is this header: a byte-packed orientation
 *     record, a fixed open-addressing index, and a bounded query surface.
 *
 * The brief named the offline layer "SUCA". In this corpus SUCA is already the
 * SuperUnicode Collation Algorithm (SUTS-001), so SUVTL names *SUCD*, exactly
 * as SUAS-009 (SIP), SUTS-005 (SCSS) and SUAS-013 (SISR) already do.
 *
 * Where UAX #50 is bound strictly to historical Unicode blocks, SUVTL extends
 * the model into the Native & Neography Plane (0x160000-0x16FFFF): a custom
 * script — a user-created logography, or a writing system from the Reddit
 * Neographical Unicode Registry (RNUR) — may declare its own vertical flow
 * direction and baseline alignment at build time, and carries
 * `SUVTL_FLAG_CUSTOM_FLOW` so a renderer knows the orientation came from a
 * registry declaration rather than a historical default.
 *
 * Three commitments separate SUVTL from a text-file property:
 *
 *   1. ONE RECORD. An orientation is a fixed 8-byte descriptor, not a tag
 *      parsed from a line. Every field is byte-aligned and the reserved byte is
 *      proven zero.
 *   2. A BOUNDED LOOKUP. A query hashes the codepoint, probes a fixed index,
 *      and returns the record — O(1) expected, a small constant number of
 *      probes, no allocation, no libc.
 *   3. A TOTAL ANSWER. An uncurated codepoint is not "unknown": SUVTL falls
 *      back to an algorithmic default orientation derived from the codepoint's
 *      script and plane, so every codepoint in the 31-bit space has exactly one
 *      answer, and the property is total.
 *
 * SUVTL does not re-implement SUAS-006 (STS). STS decides where a grapheme
 * cluster begins and ends; SUVTL takes the *first* character of a cluster as
 * the cluster's orientation (§4), which is the UAX #50 grapheme rule expressed
 * as a caller-side contract rather than a re-derived boundary.
 *
 * SUAS-014 is registered as a mandatory architecture specification. The engine,
 * the record layout and the orientation rules are architectural; the registry
 * *contents* are version data (an implementation with no curated records still
 * conforms, resolving every codepoint by the algorithmic default of §5).
 *
 * Freestanding C99, zero allocation: the table is static, every descriptor is a
 * fixed-size struct the caller owns, and no part of the engine reaches for
 * malloc, stdio, or libc string routines.
 *
 * See docs/suas/SUAS-014-suvtl.md for the normative specification.
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "suas/suas_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ── Specification identifiers ───────────────────────────────────── */
#define SUAS_SUVTL_SPEC_ID        "SUAS-014"
#define SUAS_SUVTL_SPEC_TITLE     "SuperUnicode Vertical Text Layout"
#define SUAS_SUVTL_VERSION_MAJOR  1
#define SUAS_SUVTL_VERSION_MINOR  0
#define SUAS_SUVTL_VERSION_PATCH  0

/* ── Fixed limits (static, stack-allocatable packing) ─────────────── */
/* The curated registry holds at most this many orientation records (the full
 * UAX #50 Tu/Tr set plus the Native/neography declarations). The index is a
 * power of two larger than the capacity, so the load factor stays below
 * one-half and the probe sequence stays short and bounded. */
#define SUAS_SUVTL_REGISTRY_CAPACITY 512U
#define SUAS_SUVTL_INDEX_SIZE        1024U

/* ── Base SUCS zone boundaries (duplicated for freestanding SUVTL) ── */
#ifndef SUAS_SUVTL_LIMIT_UNICODE_MAX
#define SUAS_SUVTL_LIMIT_UNICODE_MAX 0x0010FFFFUL
#define SUAS_SUVTL_LIMIT_SCP_MIN     0x00110000UL
#define SUAS_SUVTL_LIMIT_SCP_MAX     0x0011FFFFUL
#define SUAS_SUVTL_LIMIT_NATIVE_MIN  0x00120000UL
#define SUAS_SUVTL_LIMIT_NATIVE_MAX  0x7FFFFFEFUL
#define SUAS_SUVTL_LIMIT_TRAP_MIN    0x7FFFFFF0UL
#define SUAS_SUVTL_LIMIT_TRAP_MAX    0x7FFFFFFEUL
#define SUAS_SUVTL_LIMIT_SENTINEL    0x7FFFFFFFUL
#endif

/* RNUR / RCCR neography carve-out inside the Native plane (SUAS-005 §5.4).
 * The set number of an allocation is (cp - 0x00160000) >> 8. */
#define SUAS_SUVTL_RNUR_MIN          0x00160000UL
#define SUAS_SUVTL_RNUR_MAX          0x0016FFFFUL
#define SUAS_SUVTL_RNUR_SET_SHIFT    8U

/* ── Address-space zones (§5) ─────────────────────────────────────── */
/* SUVTL deliberately does not invent a second plane taxonomy. The zones below
 * are the SUCD/SSP planes with the RNUR carve-out made explicit, because that
 * carve-out is where a custom vertical flow is declared. */
typedef enum {
    SUAS_SUVTL_ZONE_BRIDGE   = 0, /* 0x00000000-0x0010FFFF           */
    SUAS_SUVTL_ZONE_SCP      = 1, /* 0x00110000-0x0011FFFF           */
    SUAS_SUVTL_ZONE_RNUR     = 2, /* 0x00160000-0x0016FFFF           */
    SUAS_SUVTL_ZONE_NATIVE   = 3, /* rest of 0x00120000-0x7FFFFFEF   */
    SUAS_SUVTL_ZONE_TRAP     = 4, /* 0x7FFFFFF0-0x7FFFFFFE           */
    SUAS_SUVTL_ZONE_SENTINEL = 5, /* 0x7FFFFFFF                      */
    SUAS_SUVTL_ZONE_EXTSUCS  = 6  /* > 0x7FFFFFFF (plugin space)     */
} suas_suvtl_zone_t;

/* ── Script groups for algorithmic defaulting (§5.2) ──────────────── */
/* A closed enumeration of the *vertical behaviour groups* UAX #50's prose
 * describes: East Asian scripts that stand upright in vertical text, scripts
 * that rotate, and the remainder. It is deliberately coarser than SSAS-005
 * (SSP) because the default orientation only needs the group, not the script
 * identity; a caller that needs the script itself asks SSP. */
typedef enum {
    SUAS_SUVTL_GROUP_OTHER    = 0, /* default: upright (U)                 */
    SUAS_SUVTL_GROUP_CJK      = 1, /* Han, Kana, Hangul, Bopomofo: upright */
    SUAS_SUVTL_GROUP_ROTATING = 2, /* Latin, Greek, Cyrillic, ...: rotated */
    SUAS_SUVTL_GROUP_NATIVE   = 3  /* Native / RNUR custom flow            */
} suas_suvtl_group_t;

/* ── Vertical Orientation values (§3, UAX #50 Table 1) ────────────── */
/* UAX #50's four normative values are represented by combinations of the three
 * orientation bits below, exactly as the seeding sketch exposed them. U/R are
 * the plain orientations; Tu/Tr are the "transformed" ones, whose *fallback*
 * when no vertical glyph exists is U and R respectively. So the value is read
 * off the bits: UPRIGHT => U, ROTATED => R, UPRIGHT|TRANSFORM => Tu, and
 * ROTATED|TRANSFORM => Tr. Exactly one of UPRIGHT/ROTATED is set in a
 * well-formed record, and TRANSFORM distinguishes the transformed forms. */
#define SUVTL_ORIENT_MASK     0x0007u /* the three value-defining bits      */
#define SUVTL_ORIENT_U        0x0001u /* upright, as in the code charts     */
#define SUVTL_ORIENT_R        0x0002u /* rotated 90 degrees clockwise       */
#define SUVTL_ORIENT_TU       0x0005u /* transformed upright; fallback U    */
#define SUVTL_ORIENT_TR       0x0006u /* transformed rotated; fallback R    */

/* ── Vertical Orientation bitfield flags (§3.1) ───────────────────── */
/* The seeding sketch's flag names, verbatim. UPRIGHT and ROTATED select the
 * base orientation; TRANSFORM marks a character that needs a distinct vertical
 * glyph (UAX #50's Tu/Tr). They are the normative encoding of the value rather
 * than a separate field, so a caller that only wants "does this rotate?" tests
 * one bit. */
#define SUVTL_ORIENT_UPRIGHT    (1U << 0) /* render upright in vertical flows */
#define SUVTL_ORIENT_ROTATED    (1U << 1) /* rotate 90 degrees clockwise      */
#define SUVTL_ORIENT_TRANSFORM  (1U << 2) /* requires a vertical glyph        */
#define SUVTL_FLAG_CUSTOM_FLOW  (1U << 3) /* Native/Neography baseline rules  */
#define SUVTL_FLAG_SQUARED      (1U << 4) /* squared kata/ideographic word    */
#define SUVTL_FLAG_QUOTATION    (1U << 5) /* quotation mark: font-substituted */

/* Every assigned bit, and the reserved-bit mask. The upper ten bits are
 * reserved and must be zero. */
#define SUVTL_FLAG_ASSIGNED_MASK 0x003Fu

/* ── Vertical baseline adjustments (§3.2) ─────────────────────────── */
/* `baseline_offset` is a signed fine-tuning value in 1/16 em, added to the
 * character's vertical position within its column cell. Zero is the default;
 * positive shifts the glyph toward the end of the column. A curated record may
 * carry a nonzero value for a neography logograph whose declared baseline
 * differs from the ideographic default. */
#define SUVTL_BASELINE_SCALE    16  /* sixteenths of an em per unit       */

/* ── Verification outcomes (§9) ───────────────────────────────────── */
/* One closed enumeration naming every defect a compiler can report before it
 * emits an image. `SUVTL_REGISTRY_OK` is the only success value; an empty
 * registry is valid. */
typedef enum {
    SUVTL_REGISTRY_OK = 0,
    SUVTL_REGISTRY_NULL_TABLE,          /* count > 0, pointer NULL          */
    SUVTL_REGISTRY_COUNT_OVER,          /* count > static capacity          */
    SUVTL_REGISTRY_UNORDERED,           /* codepoints not ascending         */
    SUVTL_REGISTRY_DUPLICATE_CODEPOINT, /* same codepoint twice             */
    SUVTL_REGISTRY_OUT_OF_SPACE,        /* codepoint > 0x7FFFFFFF           */
    SUVTL_REGISTRY_INADMISSIBLE_CODEPOINT, /* SCP, trap or Sentinel         */
    SUVTL_REGISTRY_BAD_ORIENTATION,     /* a value outside U/R/Tu/Tr        */
    SUVTL_REGISTRY_BAD_FLAGS,           /* reserved or contradictory bits   */
    SUVTL_REGISTRY_BAD_RESERVED,        /* reserved byte not zero           */
    SUVTL_REGISTRY_CUSTOM_FLOW_MISMATCH /* CUSTOM_FLOW outside the RNUR set */
} suvtl_registry_error_t;

/* ── The static orientation record (§3.1) ─────────────────────────── */
/* The seeding sketch's fields, verbatim and in order: `codepoint`,
 * `orientation_flags`, `baseline_offset`, `reserved`. `orientation_flags`
 * carries the orientation value in bits 0-2 (UPRIGHT / ROTATED / TRANSFORM)
 * and the orthogonal SUVTL_FLAG_* bits above it. The record is exactly 8
 * bytes, no padding on any supported ABI. */
typedef struct {
    uint32_t codepoint;         /* Base SUCS codepoint                  */
    uint16_t orientation_flags; /* SUVTL_ORIENT_* + SUVTL_FLAG_*        */
    uint8_t  baseline_offset;   /* signed 1/16-em vertical adjustment   */
    uint8_t  reserved;          /* must be zero                          */
} suvtl_glyph_layout_t;

/* ── Setup ────────────────────────────────────────────────────────── */
const char* suas_suvtl_version_string(void);

/* Initializes the static SUVTL orientation engine. Idempotent and
 * allocation-free; the query entry points build the index lazily on first use
 * if this is not called first. */
void suas_suvtl_init_engine(void);

/* ── Core query (§3.3) ────────────────────────────────────────────── */
/* O(1) vertical layout lookup. On success `*out_layout` receives a copy of the
 * orientation record and the function returns true. When the codepoint has no
 * curated record but is in an admissible zone, the function synthesizes the
 * algorithmic default record of §5 and returns *true*: the property is total,
 * so a well-formed codepoint always yields an orientation. It returns false
 * only for an inadmissible zone (SCP, trap, Sentinel, ExtSUCS) or a NULL
 * output, leaving the output untouched. */
bool suas_suvtl_get_layout_properties(uint32_t codepoint,
                                      suvtl_glyph_layout_t* out_layout);

/* The orientation value (U/R/Tu/Tr) of a codepoint, or SUVTL_ORIENT_U on an
 * inadmissible zone (O(1)). */
uint16_t suas_suvtl_orientation_of(uint32_t codepoint);

/* True when a codepoint is rendered upright: value U or Tu (O(1)). */
bool suas_suvtl_is_upright(uint32_t codepoint);

/* True when a codepoint rotates 90 degrees clockwise: value R or Tr (O(1)). */
bool suas_suvtl_is_rotated(uint32_t codepoint);

/* True when a codepoint requires a distinct vertical glyph: value Tu or Tr
 * (O(1)). */
bool suas_suvtl_is_transformed(uint32_t codepoint);

/* The signed baseline adjustment in 1/16 em for a codepoint (O(1)); 0 for an
 * inadmissible zone. */
int suas_suvtl_baseline_offset(uint32_t codepoint);

/* ── Orientation and cluster helpers (§4) ─────────────────────────── */
/* Apply the UAX #50 grapheme-cluster rule: the orientation of a cluster is the
 * orientation of its *first* codepoint, unless the cluster contains an
 * enclosing combining mark (general category Me), in which case the whole
 * cluster is U. `codepoints` is the cluster stream and `count` its length. A
 * NULL stream or zero count is Upright. The engine does not compute cluster
 * boundaries itself (that is SUAS-006 / STS); the caller passes one cluster. */
uint16_t suas_suvtl_cluster_orientation(const uint32_t* codepoints,
                                        size_t count);

/* True when a codepoint is an enclosing combining mark (general category Me),
 * the sole exception to the first-character rule of §4. Uses the curated Me
 * ranges carried by the engine, so a caller need not have loaded SUCD. O(1). */
bool suas_suvtl_is_enclosing_mark(uint32_t codepoint);

/* ── Enumeration ──────────────────────────────────────────────────── */
/* Number of curated records in the static orientation table (O(1)); excludes
 * the algorithmic defaults. */
uint32_t suas_suvtl_entry_count(void);

/* Copy the i-th curated record (0-based, ascending codepoint order). Returns
 * false out of range or on a NULL output (O(1)). */
bool suas_suvtl_entry_at(uint32_t index, suvtl_glyph_layout_t* out_layout);

/* ── Zone, group and classification predicates (§5) ───────────────── */
/* Classifies a 31-bit codepoint into its address-space zone (O(1)). A value
 * above SUAS_CODEPOINT_MAX is EXTSUCS. */
suas_suvtl_zone_t suas_suvtl_zone_of(uint32_t codepoint);

/* True for the zones a layout query may name: Bridge, RNUR and Native. False
 * for the Control Plane, the trap slots, the Sentinel and ExtSUCS. */
bool suas_suvtl_is_text_space(uint32_t codepoint);

/* True inside the RNUR neography carve-out (O(1)). */
bool suas_suvtl_is_neography(uint32_t codepoint);

/* Set number of an RNUR coordinate: (cp - 0x00160000) >> 8. Returns false
 * outside the carve-out. */
bool suas_suvtl_neography_set(uint32_t codepoint, uint32_t* out_set);

/* The vertical-behaviour group of a codepoint (§5.2), O(1). */
suas_suvtl_group_t suas_suvtl_group_of(uint32_t codepoint);

/* True when a codepoint has a curated record in the static table (O(1)). */
bool suas_suvtl_is_registered(uint32_t codepoint);

/* ── Hash ─────────────────────────────────────────────────────────── */
/* FNV-1a 32-bit hash over the little-endian encoding of the codepoint
 * (O(1), §3.4). This is the index key, not an identity. */
uint32_t suas_suvtl_hash(uint32_t codepoint);

/* ── Taxonomy names ───────────────────────────────────────────────── */
/* The normative tag of an orientation value: "U", "R", "Tu" or "Tr". */
const char* suas_suvtl_orientation_name(uint16_t orientation);

/* The long-form name of an orientation value ("Upright", "Rotated",
 * "Transformed Upright", "Transformed Rotated"). */
const char* suas_suvtl_orientation_long_name(uint16_t orientation);

const char* suas_suvtl_group_name(suas_suvtl_group_t group);

const char* suas_suvtl_registry_error_name(suvtl_registry_error_t error);

/* ── Verification (§9) ────────────────────────────────────────────── */
/* Verify a compiled orientation table. `entries` may be NULL only when `count`
 * is zero (an empty registry is conformant). On failure, `*out_error` receives
 * the first defect and the function returns false; on success `*out_error` is
 * SUVTL_REGISTRY_OK. Either output pointer may be NULL. */
bool suas_suvtl_validate_table(const suvtl_glyph_layout_t* entries,
                               uint32_t count,
                               suvtl_registry_error_t* out_error);

#ifdef __cplusplus
}
#endif

#endif /* SUAS_SUAS_SUVTL_H */
