#ifndef SUTR_SUTR_SUEAS_H
#define SUTR_SUTR_SUEAS_H

/* SUTR-013 — SuperUnicode East Asian Spacing (SUEAS)
 *
 * A modular technical-report specification within the OpenWindows microkernel
 * ecosystem. SUEAS establishes the binary-mapped width metrics, the contextual
 * spacing rules, and the zero-allocation layout engines required to resolve
 * East Asian character widths (Fullwidth, Halfwidth, Narrow, Wide, Ambiguous,
 * Neutral) and proportional inter-character spacing across 31-bit
 * SuperUnicode text streams in O(1) constant time.
 *
 * Where a traditional implementation (UTR #59, "East Asian Spacing") resolves
 * spacing from a runtime East_Asian_Spacing property query plus a dynamic
 * East_Asian_Width (EAW) parse, and a layout engine inserts the visible space
 * through a heuristic grid-alignment pass, SUEAS compiles both the spacing
 * classification and the width classification into immutable, byte-aligned
 * binary lookup tables. A terminal emulator, a GUI text renderer, or a
 * microkernel layout pipeline can therefore compute a cell grid, wrap a line,
 * and decide an inter-character gap with a bounded probe and a single linear
 * pass — without a single heap allocation (malloc/free).
 *
 * Two commitments separate SUEAS from a user-space spacing library:
 *
 *   1. PRECOMPILED BINARY WIDTH CLASSIFICATION. The East_Asian_Spacing and
 *      East_Asian_Width properties are merged, sorted, non-overlapping ranges
 *      embedded directly in the image (§3). A codepoint resolves its width
 *      category, its grid column span, and its spacing flags with a bounded
 *      binary search — O(log R), no property-file parse, no libc.
 *   2. ZERO-ALLOCATION GRID AND SPACING RESOLUTION. The cell-grid engine and
 *      the spacing engine walk a codepoint stream in one pass, holding only a
 *      fixed 12-byte previous-cluster record on the frame, so a terminal row
 *      or a wrapped paragraph is laid out in O(n) with no heap (§4, §5).
 *
 * SUEAS is a dual-layer system, in the same relationship SUAS-009 (SIP),
 * SUAS-012 (SUCD), SUTS-005 (SCSS), SUTS-008 (SUSCH), SUTS-009 (SULDF), and
 * SUTR-11 (SUMS) have with their Unicode counterparts. The offline reference
 * layer is the SuperUnicode Character Database (SUCD / SUAS-012) — the
 * human-readable East_Asian_Spacing and East_Asian_Width manifests that
 * user-space toolchains read. Build-time tooling compiles those datasets into
 * the immutable, byte-packed binary lookup tables this module embeds into
 * display-subsystem images. The kernel never reads a file at run time.
 *
 * Because specialized East Asian grid formatting and proportional spacing
 * adjustments are modular display features rather than core microkernel
 * bootstrapping requirements, SUEAS is categorized as a modular technical
 * report: a headless server kernel may omit it entirely without impacting core
 * system integrity (§1.6).
 *
 * Freestanding C99, zero allocation: the tables are static, every descriptor
 * is a fixed-size struct the caller owns, and no part of the engine reaches
 * for malloc, stdio, or a libc string routine.
 *
 * See docs/sutr/SUTR-013-sueas.md for the normative specification.
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Specification identifiers ────────────────────────────────────── */

#define SUEAS_SPEC_ID           "SUTR-013"
#define SUEAS_SPEC_TITLE        "SuperUnicode East Asian Spacing"
#define SUEAS_VERSION_MAJOR     0
#define SUEAS_VERSION_MINOR     1
#define SUEAS_VERSION_PATCH     0
#define SUEAS_MODULE            "sutr_sueas"

/* ── Fixed limits (static, stack-allocatable tables) ──────────────── */

#define SUEAS_SPACING_CAPACITY  2048U /* compiled East_Asian_Spacing rows  */
#define SUEAS_EAW_CAPACITY      2048U /* compiled East_Asian_Width rows    */
#define SUEAS_GRID_CAPACITY     256U  /* maximum columns one call resolves */

#define SUEAS_FNV_OFFSET_BASIS  2166136261u
#define SUEAS_FNV_PRIME         16777619u

/* ── The East_Asian_Spacing property (UTR #59 §3.2) ───────────────── */

typedef enum {
    SUEAS_SPACING_OTHER       = 0, /* "O" — no auto-spacing             */
    SUEAS_SPACING_NARROW      = 1, /* "N" — spaces against "W"          */
    SUEAS_SPACING_WIDE        = 2, /* "W" — East Asian script           */
    SUEAS_SPACING_CONDITIONAL = 3  /* "C" — "N" for Chinese, else "O"   */
} sueas_spacing_t;

/* ── The East_Asian_Width property (UAX #11) ──────────────────────── */

typedef enum {
    SUEAS_EAW_NEUTRAL   = 0, /* "N"  */
    SUEAS_EAW_AMBIGUOUS = 1, /* "A"  */
    SUEAS_EAW_HALFWIDTH = 2, /* "H"  */
    SUEAS_EAW_WIDE      = 3, /* "W"  */
    SUEAS_EAW_FULLWIDTH = 4, /* "F"  */
    SUEAS_EAW_NARROW    = 5  /* "Na" */
} sueas_eaw_t;

/* ── Specified width category of the sketch (§2, §6) ──────────────── */

typedef enum {
    SUEAS_WIDTH_NEUTRAL = 0, /* neither East Asian nor ordinary starter    */
    SUEAS_WIDTH_NAR     = 1, /* Narrow: one column, ordinary starter       */
    SUEAS_WIDTH_WID     = 2, /* Wide: two columns, East Asian              */
    SUEAS_WIDTH_FULL    = 3, /* Fullwidth: two columns, East Asian forms   */
    SUEAS_WIDTH_HALF    = 4, /* Halfwidth: one column, East Asian forms    */
    SUEAS_WIDTH_AMB     = 5  /* Ambiguous: context-dependent width         */
} sueas_width_category_t;

/* ── Spacing-adjustment flags (packed bitfield, §3.3) ─────────────── */

#define SUEAS_FLAG_PROPORTIONAL  (1U << 0) /* allow proportional micro-spacing  */
#define SUEAS_FLAG_GRID_SNAP     (1U << 1) /* force alignment to the char grid  */
#define SUEAS_FLAG_PUNCT_ADJUST  (1U << 2) /* halfwidth punctuation centering   */
#define SUEAS_FLAG_WIDE          (1U << 3) /* has a Wide East_Asian_Spacing     */
#define SUEAS_FLAG_NARROW        (1U << 4) /* has a Narrow East_Asian_Spacing   */
#define SUEAS_FLAG_CONDITIONAL   (1U << 5) /* has a Conditional spacing value   */
#define SUEAS_FLAG_EAST_ASIAN    (1U << 6) /* belongs to an East Asian script   */
#define SUEAS_FLAG_NATIVE        (1U << 7) /* Native / neography plane          */
#define SUEAS_FLAG_ASSIGNED_MASK 0x00FFu   /* all defined flag bits             */

/* ── Structural types (zero-allocation, §3.1) ─────────────────────── */

/* The compiled range rows. A row is 12 bytes and byte-aligned, so an array
 * of rows maps directly from a compiled image with no decode step. */
typedef struct {
    uint32_t first;  /* first codepoint of the range  */
    uint32_t last;   /* last codepoint of the range   */
    uint32_t value;  /* sueas_spacing_t or sueas_eaw_t */
} sueas_spacing_range_t;

typedef struct {
    uint32_t first;
    uint32_t last;
    uint32_t value;
} sueas_eaw_range_t;

/* The per-codepoint width descriptor named by the seeding sketch: exactly
 * 8 bytes, byte-aligned, padding-free. */
typedef struct {
    uint32_t codepoint;       /* raw 31-bit SuperUnicode codepoint          */
    uint16_t spacing_flags;   /* packed spacing and adjustment flags (§3.3) */
    uint8_t  width_category;  /* sueas_width_category_t (§3.2)              */
    uint8_t  column_units;    /* grid column span: 1 ordinary, 2 wide (§3.4)*/
} sueas_width_entry_t;

/* The per-pair spacing decision of the layout engine (§4). */
typedef struct {
    uint8_t  space;          /* 1 when a visible space is inserted          */
    uint8_t  tight;          /* 1 when the pair must not break (no space)   */
    uint16_t reserved;       /* must be zero                                */
    uint32_t reserved2;      /* must be zero                                */
} sueas_spacing_decision_t;

/* ── Address-space zones (§2.1) ───────────────────────────────────── */

typedef enum {
    SUEAS_ZONE_BRIDGE   = 0, /* 0x00000000-0x0010FFFF           */
    SUEAS_ZONE_SCP      = 1, /* 0x00110000-0x0011FFFF           */
    SUEAS_ZONE_RNUR     = 2, /* 0x00160000-0x0016FFFF           */
    SUEAS_ZONE_NATIVE   = 3, /* rest of 0x00120000-0x7FFFFFEF   */
    SUEAS_ZONE_TRAP     = 4, /* 0x7FFFFFF0-0x7FFFFFFE           */
    SUEAS_ZONE_SENTINEL = 5, /* 0x7FFFFFFF                      */
    SUEAS_ZONE_EXTSUCS  = 6  /* > 0x7FFFFFFF (plugin space)     */
} sueas_zone_t;

/* ── Processing outcomes (§7) ─────────────────────────────────────── */

typedef enum {
    SUEAS_OK = 0,
    SUEAS_ERR_NULL_ARGUMENT,   /* a NULL descriptor                         */
    SUEAS_ERR_ILLEGAL_ZONE,    /* a Control Plane / trap / Sentinel argument */
    SUEAS_ERR_CAPACITY         /* the output buffer is too small            */
} sueas_status_code_t;

/* ── RNUR / neography carve-out (§2.2) ────────────────────────────── */

#define SUEAS_RNUR_MIN          0x00160000UL
#define SUEAS_RNUR_MAX          0x0016FFFFUL
#define SUEAS_RNUR_SET_SHIFT    8U

/* ── Base SUCS zone boundaries (duplicated for freestanding SUEAS) ── */

#ifndef SUEAS_LIMIT_UNICODE_MAX
#define SUEAS_LIMIT_UNICODE_MAX 0x0010FFFFUL
#define SUEAS_LIMIT_SCP_MIN     0x00110000UL
#define SUEAS_LIMIT_SCP_MAX     0x0011FFFFUL
#define SUEAS_LIMIT_NATIVE_MIN  0x00120000UL
#define SUEAS_LIMIT_NATIVE_MAX  0x7FFFFFEFUL
#define SUEAS_LIMIT_TRAP_MIN    0x7FFFFFF0UL
#define SUEAS_LIMIT_TRAP_MAX    0x7FFFFFFEUL
#define SUEAS_LIMIT_SENTINEL    0x7FFFFFFFUL
#endif

/* ── Setup ────────────────────────────────────────────────────────── */

/**
 * Returns the "MAJOR.MINOR.PATCH" version string of this module.
 * Freestanding: the returned pointer is to a static string, never NULL.
 */
const char *sutr_sueas_version_string(void);

/**
 * Initializes the static SUEAS East Asian spacing and width registry.
 * Idempotent; the compiled tables are static, so this is a no-op marker.
 */
void sutr_sueas_init_registry(void);

/* ── The width query (§3) ─────────────────────────────────────────── */

/**
 * Queries the pre-compiled East Asian width classifications and grid metrics
 * of a codepoint in O(log R) time without heap allocation or string parsing.
 *
 * The query is total over the renderable zones: an unlisted codepoint is
 * answered by the algorithmic default of §6 (spacing Other, EAW Neutral,
 * one column) rather than by a lookup failure. Returns false — leaving
 * `out_entry` untouched — only for a NULL pointer or a codepoint outside the
 * renderable zones (the System Control Plane, a trap slot, the Sentinel, or
 * ExtSUCS).
 */
bool sutr_sueas_get_width_entry(uint32_t codepoint,
                                sueas_width_entry_t *out_entry);

/**
 * As above, but names the failure in `out_error` (SUEAS_OK on success).
 */
bool sutr_sueas_get_width_entry_ex(uint32_t codepoint,
                                   sueas_width_entry_t *out_entry,
                                   sueas_status_code_t *out_error);

/* ── Property predicates (§3) ─────────────────────────────────────── */

sueas_spacing_t sutr_sueas_spacing_of(uint32_t codepoint);
sueas_eaw_t     sutr_sueas_eaw_of(uint32_t codepoint);
sueas_width_category_t sutr_sueas_width_category_of(uint32_t codepoint);

/**
 * The grid column span of a codepoint: 2 for a Wide, Fullwidth, or Ambiguous
 * (in an East Asian context) glyph, 1 otherwise (§3.4).
 */
uint8_t sutr_sueas_column_units(uint32_t codepoint);

bool sutr_sueas_is_wide(uint32_t codepoint);
bool sutr_sueas_is_fullwidth(uint32_t codepoint);
bool sutr_sueas_is_halfwidth(uint32_t codepoint);
bool sutr_sueas_is_ambiguous(uint32_t codepoint);
bool sutr_sueas_is_east_asian(uint32_t codepoint);
bool sutr_sueas_is_registered(uint32_t codepoint);

/* ── Spacing resolution (§4) ──────────────────────────────────────── */

/**
 * Resolves the East_Asian_Spacing value of a codepoint after applying the
 * contextual rules of §4: a Conditional value resolves to Narrow when the
 * language context is Chinese, and to Other otherwise; a Narrow value
 * displayed upright in vertical layout resolves to Other.
 */
sueas_spacing_t sutr_sueas_resolve_spacing(uint32_t codepoint,
                                           bool chinese_context,
                                           bool vertical_upright);

/**
 * Decides whether a visible auto-space is inserted between two adjacent
 * clusters with the given resolved spacing values (§4.3), in O(1):
 * a space is inserted exactly at a Wide/Narrow or Narrow/Wide boundary.
 */
bool sutr_sueas_needs_space(sueas_spacing_t left, sueas_spacing_t right);

/**
 * The full per-pair decision for two codepoints, resolving Conditional and
 * vertical values before applying the boundary rule.
 */
bool sutr_sueas_spacing_decision(uint32_t left_codepoint,
                                 uint32_t right_codepoint,
                                 bool chinese_context,
                                 bool vertical_upright,
                                 sueas_spacing_decision_t *out_decision);

/* ── The zero-allocation grid engine (§5) ─────────────────────────── */

/**
 * Walks a codepoint stream once and writes the number of grid columns each
 * codepoint occupies; the running total is the width of the whole stream in
 * terminal cells. In O(n) with no allocation.
 *
 * `out_columns` may be NULL, in which case only the total is computed. Up to
 * `capacity` entries are written. The return value is the total column span
 * of the whole stream, independent of `capacity` (the snprintf convention).
 */
size_t sutr_sueas_measure_columns(const uint32_t *stream, size_t length,
                                  uint16_t *out_columns, size_t capacity);

/**
 * As `sutr_sueas_measure_columns`, but resolves Conditional values through the
 * language context and vertical-upright overrides of §4.
 */
size_t sutr_sueas_measure_columns_ex(const uint32_t *stream, size_t length,
                                     bool chinese_context,
                                     bool vertical_upright,
                                     uint16_t *out_columns, size_t capacity);

/**
 * Counts the auto-spaces the stream would receive: the number of adjacent
 * cluster pairs at a Wide/Narrow or Narrow/Wide boundary, in O(n).
 */
size_t sutr_sueas_count_spaces(const uint32_t *stream, size_t length,
                               bool chinese_context, bool vertical_upright);

/* ── Zone and classification predicates (§2.1) ───────────────────── */

sueas_zone_t sutr_sueas_zone_of(uint32_t codepoint);
bool sutr_sueas_is_renderable(uint32_t codepoint);
bool sutr_sueas_is_text_space(uint32_t codepoint);
bool sutr_sueas_is_neography(uint32_t codepoint);
bool sutr_sueas_neography_set(uint32_t codepoint, uint32_t *out_set);

/* ── Hash and enumeration (§3.5) ─────────────────────────────────── */

uint32_t sutr_sueas_hash_codepoint(uint32_t codepoint);

uint32_t sutr_sueas_spacing_count(void);
bool sutr_sueas_spacing_at(uint32_t index, sueas_spacing_range_t *out_range);
uint32_t sutr_sueas_eaw_count(void);
bool sutr_sueas_eaw_at(uint32_t index, sueas_eaw_range_t *out_range);

/* ── Taxonomy names ───────────────────────────────────────────────── */

const char *sutr_sueas_spacing_name(sueas_spacing_t value);
const char *sutr_sueas_eaw_name(sueas_eaw_t value);
const char *sutr_sueas_width_category_name(sueas_width_category_t category);
const char *sutr_sueas_zone_name(sueas_zone_t zone);
const char *sutr_sueas_status_code_name(sueas_status_code_t code);

#ifdef __cplusplus
}
#endif

#endif /* SUTR_SUTR_SUEAS_H */
