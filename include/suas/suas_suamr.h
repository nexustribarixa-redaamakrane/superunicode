#ifndef SUAS_SUAS_SUAMR_H
#define SUAS_SUAS_SUAMR_H

/* SUAS-015 — SuperUnicode Arabic Mark Rendering (SUAMR)
 *
 * The core algorithmic subsystem of the OpenWindows microkernel governing the
 * *positioning and visual ordering* of Arabic combining marks: how a mark
 * stacks above or below its base, how a run of marks staggers to avoid
 * collision, and the deterministic transient order in which a cluster's marks
 * are displayed.
 *
 * UAX #53, "Unicode Arabic Mark Rendering", defines the Arabic Mark Transient
 * Reordering Algorithm (AMTRA) in prose, and relies on an OpenType shaping
 * engine, dynamically parsed font metrics and runtime heuristics to place each
 * diacritic. SUAMR is the runtime counterpart of that algorithm — a UAX #53
 * analog — in the same dual-layer relationship SUAS-009 (SIP) has with UAX #38,
 * SUAS-013 (SISR) has with UAX #45, and SUAS-014 (SUVTL) has with UAX #50:
 *
 *   - the offline reference layer is the SuperUnicode Character Database
 *     (SUCD, SUAS-012): the Arabic mark rendering rules, the
 *     Modifier_Combining_Mark (MCM) list, the stacking precedence classes, and
 *     the human-readable positioning manifests for font engineers, shaping
 *     engines and developer toolchains;
 *   - the runtime kernel layer is this header: a byte-packed mark-layout
 *     record, a fixed open-addressing index, and a bounded query surface.
 *
 * The brief named the offline layer "SUCA". In this corpus SUCA is already the
 * SuperUnicode Collation Algorithm (SUTS-001), so SUAMR names *SUCD*, exactly
 * as SUAS-009 (SIP), SUTS-005 (SCSS), SUAS-013 (SISR) and SUAS-014 (SUVTL)
 * already do.
 *
 * Where UAX #53 is bound to the Arabic script of historical Unicode blocks,
 * SUAMR generalizes the same stacking and mark-positioning engine into the
 * Native & Neography Plane (0x160000-0x16FFFF): a custom abugida — a conlang
 * writing system such as the Kōpu abvukjelitsā abugida, or a script from the
 * Reddit Neographical Unicode Registry (RNUR) — may declare its own combining
 * mark layouts at build time, and carries `SUAMR_FLAG_CUSTOM_CLASS` so a
 * renderer knows the positioning came from a registry declaration rather than a
 * historical default.
 *
 * Three commitments separate SUAMR from a shaping engine:
 *
 *   1. ONE RECORD. A mark's layout is a fixed 12-byte descriptor, not a metric
 *      parsed from a font table. Every field is byte-aligned and the reserved
 *      byte is proven zero.
 *   2. A BOUNDED LOOKUP. A query hashes the (base, mark) pair, probes a fixed
 *      index, and returns the record — O(1) expected, a small constant number
 *      of probes, no allocation, no libc.
 *   3. A DETERMINISTIC ORDER. A mark cluster is reordered by the transient
 *      algorithm of §5 — NFD-stable, canonically-equivalent sequences render
 *      identically, and a combining grapheme joiner overrides the default.
 *
 * SUAMR does not re-implement SUAS-006 (STS). STS decides where a grapheme
 * cluster begins and ends; SUAMR takes the cluster the caller supplies and
 * reorders its marks (§5). The two compose; neither re-derives the other.
 *
 * SUAS-015 is registered as a mandatory architecture specification. The engine,
 * the record layout, the stacking flags and the transient ordering rules are
 * architectural; the registry *contents* are version data (an implementation
 * with no curated records still conforms, resolving every pair by the
 * algorithmic default of §6).
 *
 * Freestanding C99, zero allocation: the table is static, every descriptor is a
 * fixed-size struct the caller owns, and no part of the engine reaches for
 * malloc, stdio, or libc string routines.
 *
 * See docs/suas/SUAS-015-suamr.md for the normative specification.
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "suas/suas_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ── Specification identifiers ───────────────────────────────────── */
#define SUAS_SUAMR_SPEC_ID        "SUAS-015"
#define SUAS_SUAMR_SPEC_TITLE     "SuperUnicode Arabic Mark Rendering"
#define SUAS_SUAMR_VERSION_MAJOR  1
#define SUAS_SUAMR_VERSION_MINOR  0
#define SUAS_SUAMR_VERSION_PATCH  0

/* ── Fixed limits (static, stack-allocatable packing) ─────────────── */
/* The curated registry holds at most this many mark-layout records (the Arabic
 * combining-mark set plus the Native/neography declarations). The index is a
 * power of two larger than the capacity, so the load factor stays below
 * one-half and the probe sequence stays short and bounded. */
#define SUAS_SUAMR_REGISTRY_CAPACITY 256U
#define SUAS_SUAMR_INDEX_SIZE        512U

/* The largest cluster the transient reordering pass will process in one call.
 * A Combining Character Sequence with more marks than this is refused rather
 * than truncated, so the caller learns the bound instead of silently losing a
 * mark. */
#define SUAS_SUAMR_MAX_CLUSTER       32U

/* ── Base SUCS zone boundaries (duplicated for freestanding SUAMR) ── */
#ifndef SUAS_SUAMR_LIMIT_UNICODE_MAX
#define SUAS_SUAMR_LIMIT_UNICODE_MAX 0x0010FFFFUL
#define SUAS_SUAMR_LIMIT_SCP_MIN     0x00110000UL
#define SUAS_SUAMR_LIMIT_SCP_MAX     0x0011FFFFUL
#define SUAS_SUAMR_LIMIT_NATIVE_MIN  0x00120000UL
#define SUAS_SUAMR_LIMIT_NATIVE_MAX  0x7FFFFFEFUL
#define SUAS_SUAMR_LIMIT_TRAP_MIN    0x7FFFFFF0UL
#define SUAS_SUAMR_LIMIT_TRAP_MAX    0x7FFFFFFEUL
#define SUAS_SUAMR_LIMIT_SENTINEL    0x7FFFFFFFUL
#endif

/* RNUR / RCCR neography carve-out inside the Native plane (SUAS-005 §5.4).
 * The set number of an allocation is (cp - 0x00160000) >> 8. */
#define SUAS_SUAMR_RNUR_MIN          0x00160000UL
#define SUAS_SUAMR_RNUR_MAX          0x0016FFFFUL
#define SUAS_SUAMR_RNUR_SET_SHIFT    8U

/* ── Address-space zones (§2) ─────────────────────────────────────── */
/* SUAMR deliberately does not invent a second plane taxonomy. The zones below
 * are the SUCD/SSP planes with the RNUR carve-out made explicit, because that
 * carve-out is where a custom mark layout is declared. */
typedef enum {
    SUAS_SUAMR_ZONE_BRIDGE   = 0, /* 0x00000000-0x0010FFFF           */
    SUAS_SUAMR_ZONE_SCP      = 1, /* 0x00110000-0x0011FFFF           */
    SUAS_SUAMR_ZONE_RNUR     = 2, /* 0x00160000-0x0016FFFF           */
    SUAS_SUAMR_ZONE_NATIVE   = 3, /* rest of 0x00120000-0x7FFFFFEF   */
    SUAS_SUAMR_ZONE_TRAP     = 4, /* 0x7FFFFFF0-0x7FFFFFFE           */
    SUAS_SUAMR_ZONE_SENTINEL = 5, /* 0x7FFFFFFF                      */
    SUAS_SUAMR_ZONE_EXTSUCS  = 6  /* > 0x7FFFFFFF (plugin space)     */
} suas_suamr_zone_t;

/* ── Mark positioning flags (packed bitfields, §3.1) ──────────────── */
/* The seeding sketch's flag names, verbatim. They describe where a mark
 * renders relative to its base and whether it takes a stagger adjustment. */
#define SUAMR_FLAG_STACK_ABOVE  (1U << 0) /* Stack mark vertically above base */
#define SUAMR_FLAG_STACK_BELOW  (1U << 1) /* Stack mark vertically below base */
#define SUAMR_FLAG_STAGGER      (1U << 2) /* Apply horizontal/vertical stagger adjustment */
#define SUAMR_FLAG_CUSTOM_CLASS (1U << 3) /* Uses extended neography positioning */

/* Additional orthogonal flags SUAMR assigns (§3.1). */
#define SUAMR_FLAG_MCM          (1U << 4) /* Modifier Combining Mark (§4)     */
#define SUAMR_FLAG_SHADDA       (1U << 5) /* Shadda (ccc=33)                  */
#define SUAMR_FLAG_KASRA_LIKE   (1U << 6) /* Kasra / kasra-like ligating mark */
#define SUAMR_FLAG_TRANSPARENT   (1U << 7) /* Renders directly over the base   */

/* Every assigned bit, and the reserved-bit mask. The upper eight bits are
 * reserved and must be zero. */
#define SUAMR_FLAG_ASSIGNED_MASK 0x00FFu

/* ── Fixed-point offset scale (§3.2) ──────────────────────────────── */
/* `x_offset` and `y_offset` are signed fixed-point displacements in 1/64 em.
 * Zero is the default; a positive `y_offset` moves the mark toward the top of
 * the cell, and a positive `x_offset` moves it toward the end of the line. */
#define SUAMR_OFFSET_SCALE      64  /* sixty-fourths of an em per unit */

/* ── Canonical_Combining_Class values (§4, UAX #53 Table 1) ───────── */
/* The eleven non-zero ccc values of the Arabic script, plus the two
 * generalized classes (220 below, 230 above) and the ccc=0 starter class. The
 * values are the Unicode Canonical_Combining_Class assignments, carried as
 * closed constants so the engine needs no SUCD image. */
#define SUAMR_CCC_STARTER          0u /* ccc=0: a base / combining joiner    */
#define SUAMR_CCC_FATHATAN        27u
#define SUAMR_CCC_DAMMATAN        28u
#define SUAMR_CCC_KASRATAN        29u
#define SUAMR_CCC_FATHA           30u
#define SUAMR_CCC_DAMMA           31u
#define SUAMR_CCC_KASRA           32u
#define SUAMR_CCC_SHADDA          33u
#define SUAMR_CCC_SUKUN           34u
#define SUAMR_CCC_SUPERSCRIPT_ALEF 35u
#define SUAMR_CCC_BELOW          220u /* all other below marks               */
#define SUAMR_CCC_ABOVE          230u /* all other above marks               */

/* ── CGJ and the transients (§5) ──────────────────────────────────── */
#define SUAMR_CGJ                0x0000034FUL /* COMBINING GRAPHEME JOINER */
#define SUAMR_SHADDA_CP          0x00000651UL /* ARABIC SHADDA             */

/* ── Verification outcomes (§9) ───────────────────────────────────── */
/* One closed enumeration naming every defect a compiler can report before it
 * emits an image. `SUAMR_REGISTRY_OK` is the only success value; an empty
 * registry is valid. */
typedef enum {
    SUAMR_REGISTRY_OK = 0,
    SUAMR_REGISTRY_NULL_TABLE,          /* count > 0, pointer NULL          */
    SUAMR_REGISTRY_COUNT_OVER,          /* count > static capacity          */
    SUAMR_REGISTRY_UNORDERED,           /* codepoints not ascending         */
    SUAMR_REGISTRY_DUPLICATE_CODEPOINT, /* same codepoint twice             */
    SUAMR_REGISTRY_OUT_OF_SPACE,        /* codepoint > 0x7FFFFFFF           */
    SUAMR_REGISTRY_INADMISSIBLE_CODEPOINT, /* SCP, trap or Sentinel         */
    SUAMR_REGISTRY_BAD_CCC,             /* canonical combining class invalid */
    SUAMR_REGISTRY_BAD_FLAGS,           /* reserved or contradictory bits   */
    SUAMR_REGISTRY_BAD_RESERVED,        /* reserved byte not zero           */
    SUAMR_REGISTRY_CUSTOM_CLASS_MISMATCH, /* CUSTOM_CLASS outside RNUR set  */
    SUAMR_REGISTRY_STACK_CONTRADICTION  /* both STACK_ABOVE and STACK_BELOW */
} suamr_registry_error_t;

/* ── Transient reordering outcomes (§5.4) ─────────────────────────── */
typedef enum {
    SUAMR_REORDER_OK = 0,
    SUAMR_REORDER_NULL_ARGUMENT,
    SUAMR_REORDER_ILLEGAL_ZONE,
    SUAMR_REORDER_TOO_LONG,
    SUAMR_REORDER_NO_BASE
} suamr_reorder_status_t;

/* ── The static mark-layout record (§3.1) ─────────────────────────── */
/* The seeding sketch's fields, verbatim and in order: `codepoint`,
 * `x_offset`, `y_offset`, `positioning_flags`, `stacking_level`, `reserved`.
 * The record is exactly 12 bytes, no padding on any supported ABI. */
typedef struct {
    uint32_t codepoint;          /* Combining mark codepoint */
    int16_t  x_offset;           /* Fixed-point horizontal displacement */
    int16_t  y_offset;           /* Fixed-point vertical displacement */
    uint16_t positioning_flags;  /* Packed stacking and rendering flags */
    uint8_t  stacking_level;     /* Stacking depth index for multi-mark clusters */
    uint8_t  reserved;
} suamr_mark_layout_t;

/* ── Setup ────────────────────────────────────────────────────────── */
const char* suas_suamr_version_string(void);

/* Initializes the static SUAMR Arabic mark rendering engine and binding
 * tables. Idempotent and allocation-free; the query entry points build the
 * index lazily on first use if this is not called first. */
void suas_suamr_init_engine(void);

/* ── Core query (§3.3) ────────────────────────────────────────────── */
/* Queries the pre-compiled layout and positioning metrics for a combining mark
 * relative to a base, in O(1) time without heap allocation. On success
 * `*out_layout` receives a copy of the layout record and the function returns
 * true. When the (base, mark) pair has no curated record but both codepoints
 * are in an admissible zone, the function synthesizes the algorithmic default
 * record of §6 and returns *true*: the property is total, so a well-formed
 * mark always yields a layout. It returns false only for an inadmissible zone
 * (SCP, trap, Sentinel, ExtSUCS) or a NULL output, leaving the output
 * untouched. `base_codepoint` may be zero to ask for the mark's own
 * base-independent layout. */
bool suas_suamr_get_mark_layout(uint32_t base_codepoint,
                                uint32_t mark_codepoint,
                                suamr_mark_layout_t *out_layout);

/* ── Mark predicates (§3, §4) ─────────────────────────────────────── */
/* True when a codepoint is an Arabic combining mark the engine curates (O(1)). */
bool suas_suamr_is_mark(uint32_t codepoint);

/* True when a codepoint stacks above its base (O(1)). */
bool suas_suamr_is_above(uint32_t codepoint);

/* True when a codepoint stacks below its base (O(1)). */
bool suas_suamr_is_below(uint32_t codepoint);

/* True when a codepoint is a Modifier Combining Mark (§4): a mark that binds
 * tightly to the base and renders closer to it than the tashkil. */
bool suas_suamr_is_mcm(uint32_t codepoint);

/* True when a codepoint is the shadda (ccc=33). */
bool suas_suamr_is_shadda(uint32_t codepoint);

/* The Canonical_Combining_Class of a codepoint (§4). Returns
 * SUAMR_CCC_STARTER (0) for a starter or an uncarried codepoint, so the value
 * is total. */
uint8_t suas_suamr_ccc(uint32_t codepoint);

/* The stacking level of a mark: 0 for the base-most mark, increasing outward
 * (§3.1). Zero for a starter. */
uint8_t suas_suamr_stacking_level(uint32_t codepoint);

/* The signed horizontal displacement in 1/64 em for a (base, mark) pair (O(1));
 * 0 for an inadmissible zone. */
int suas_suamr_x_offset(uint32_t base_codepoint, uint32_t mark_codepoint);

/* The signed vertical displacement in 1/64 em for a (base, mark) pair (O(1));
 * 0 for an inadmissible zone. */
int suas_suamr_y_offset(uint32_t base_codepoint, uint32_t mark_codepoint);

/* ── Transient reordering (§5, AMTRA) ─────────────────────────────── */
/* Apply the Arabic Mark Transient Reordering Algorithm to one Combining
 * Character Sequence, in place. `codepoints` is the cluster stream, `count`
 * its length; the first codepoint is assumed to be the base (a starter). The
 * reordering is stable, names the marks it moves, and is a no-op on a
 * single-mark cluster. On success the function returns true and `*out_status`
 * is SUAMR_REORDER_OK. It returns false for a NULL stream, an empty stream, an
 * inadmissible zone, a cluster longer than SUAS_SUAMR_MAX_CLUSTER, or a stream
 * whose first codepoint is not a starter. `out_status` may be NULL.

 * AMTRA's steps are honoured exactly: shadda first, then any run of ccc=230 MCM
 * marks, then any run of ccc=220 MCM marks (§5.2). The combining grapheme
 * joiner is a barrier: a mark after a CGJ is never moved across it, so a caller
 * can pin an exceptional display order (§5.3). */
bool suas_suamr_reorder_cluster(uint32_t *codepoints,
                                size_t count,
                                suamr_reorder_status_t *out_status);

/* ── Enumeration ──────────────────────────────────────────────────── */
/* Number of curated records in the static layout table (O(1)); excludes the
 * algorithmic defaults. */
uint32_t suas_suamr_entry_count(void);

/* Copy the i-th curated record (0-based, ascending codepoint order). Returns
 * false out of range or on a NULL output (O(1)). */
bool suas_suamr_entry_at(uint32_t index, suamr_mark_layout_t *out_layout);

/* ── Zone and classification predicates (§2) ──────────────────────── */
/* Classifies a 31-bit codepoint into its address-space zone (O(1)). A value
 * above SUAS_CODEPOINT_MAX is EXTSUCS. */
suas_suamr_zone_t suas_suamr_zone_of(uint32_t codepoint);

/* True for the zones a layout query may name: Bridge, RNUR and Native. False
 * for the Control Plane, the trap slots, the Sentinel and ExtSUCS. */
bool suas_suamr_is_text_space(uint32_t codepoint);

/* True inside the RNUR neography carve-out (O(1)). */
bool suas_suamr_is_neography(uint32_t codepoint);

/* Set number of an RNUR coordinate: (cp - 0x00160000) >> 8. Returns false
 * outside the carve-out. */
bool suas_suamr_neography_set(uint32_t codepoint, uint32_t *out_set);

/* True when a codepoint has a curated record in the static table (O(1)). */
bool suas_suamr_is_registered(uint32_t codepoint);

/* ── Hash ─────────────────────────────────────────────────────────── */
/* FNV-1a 32-bit hash over the little-endian encoding of the codepoint (O(1),
 * §3.3). This is the index key, not an identity. */
uint32_t suas_suamr_hash(uint32_t codepoint);

/* FNV-1a 32-bit hash over the (base, mark) pair, order-sensitive (O(1)). */
uint32_t suas_suamr_hash_pair(uint32_t base_codepoint, uint32_t mark_codepoint);

/* ── Taxonomy names ───────────────────────────────────────────────── */
/* The name of a canonical combining class ("starter", "shadda", "above", ...). */
const char* suas_suamr_ccc_name(uint8_t ccc);

const char* suas_suamr_zone_name(suas_suamr_zone_t zone);

const char* suas_suamr_registry_error_name(suamr_registry_error_t error);

const char* suas_suamr_reorder_status_name(suamr_reorder_status_t status);

/* ── Verification (§9) ────────────────────────────────────────────── */
/* Verify a compiled mark-layout table. `entries` may be NULL only when `count`
 * is zero (an empty registry is conformant). On failure, `*out_error` receives
 * the first defect and the function returns false; on success `*out_error` is
 * SUAMR_REGISTRY_OK. Either output pointer may be NULL. */
bool suas_suamr_validate_table(const suamr_mark_layout_t *entries,
                               uint32_t count,
                               suamr_registry_error_t *out_error);

#ifdef __cplusplus
}
#endif

#endif /* SUAS_SUAS_SUAMR_H */
