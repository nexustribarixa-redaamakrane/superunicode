#ifndef SUTR_SUTR_SUMS_H
#define SUTR_SUTR_SUMS_H

/* SUTR-11 — SuperUnicode Mongolian Shaping & Vertical Direction (SUMS)
 *
 * A modular technical-report specification within the OpenWindows microkernel
 * ecosystem. SUMS establishes the binary-mapped schemas, contextual shaping
 * rules, and zero-allocation lookup engines required to render and process
 * vertical script flows and cursive Mongolian text streams in O(1) constant
 * time.
 *
 * Where a traditional implementation (UTR #54, "Unicode Mongolian 12.1
 * Snapshot") documents Mongolian glyph variation as a mass of code-chart
 * annotations, and a shaping engine resolves it through dynamic Free Variation
 * Selector (FVS) sequence parsing plus runtime state machines, SUMS compiles
 * the same outcome into an immutable, byte-packed, static table. Mongolian
 * positional forms (initial, medial, final, isolated) and the standardized
 * variation sequences are pre-computed at build time; a character slot directly
 * indexes its contextual variant without runtime string scanning.
 *
 * SUMS is a dual-layer system, in the same relationship SUAS-009 (SIP),
 * SUAS-012 (SUCD), SUTS-006 (SIDN), SUTS-007 (SUEPS) and SUTS-008 (SUSCH)
 * have with their Unicode
 * counterparts. The offline reference layer is the SuperUnicode Character
 * Database (SUCD / SUAS-012) — the human-readable Mongolian shaping manifests,
 * variation-sequence charts and vertical-metric tables that font designers and
 * developer toolchains read. Build-time tooling compiles those datasets into the
 * immutable, byte-packed binary lookup tables this module embeds into
 * graphical subsystem images. The kernel never reads a file at run time.
 *
 * Two commitments separate SUMS from a text shaping library:
 *
 *   1. NO RUNTIME VARIANT-SELECTOR PARSING. An FVS sequence (a Mongolian letter
 *      plus a Free Variation Selector, VS1–VS3) is flattened at build time into
 *      a static binary token table. A renderer resolves a (base, selector) pair
 *      by direct pointer indexing, O(1), with no malloc/free, no libc and no
 *      sequence scan (§4).
 *   2. NATIVE VERTICAL FLOW. Mongolian flows top-to-bottom and left-to-right.
 *      SUMS does not require a CSS or OpenType vertical-layout pass: the
 *      vertical advance, the baseline rotation and the word-boundary spacing are
 *      stored as fixed-point byte offsets directly in the character's binary
 *      record, and integrate natively with SUAS-014 (SUVTL) (§5).
 *
 * Because core operating system primitives, file paths and kernel IPC channels
 * require strict text safety rather than cursive display, SUMS is categorized as
 * a modular technical report: a lightweight server kernel or a headless
 * environment may omit SUMS entirely without impacting core system integrity
 * (§1.6). The engine, the record layouts and the resolution contract are
 * architectural; the Mongolian *contents* are version data compiled from the
 * SUCD manifests (§8).
 *
 * Freestanding C99, zero allocation: the tables are static, every descriptor is
 * a fixed-size struct the caller owns, and no part of the engine reaches for
 * malloc, stdio or libc string routines.
 *
 * See docs/sutr/SUTR-11-sums.md for the normative specification.
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Specification identifiers ────────────────────────────────────── */

#define SUMS_SPEC_ID            "SUTR-11"
#define SUMS_SPEC_TITLE         "SuperUnicode Mongolian Shaping & Vertical Direction"
#define SUMS_VERSION_MAJOR      0
#define SUMS_VERSION_MINOR      1
#define SUMS_VERSION_PATCH      0
#define SUMS_MODULE             "sutr_sums"

/* ── Fixed limits (static, stack-allocatable tables) ──────────────── */

#define SUMS_ENTRY_CAPACITY     128U
#define SUMS_ENTRY_INDEX_SIZE   256U
#define SUMS_VARIANT_CAPACITY   128U
#define SUMS_VARIANT_INDEX_SIZE 256U

#define SUMS_FNV_OFFSET_BASIS   2166136261u
#define SUMS_FNV_PRIME          16777619u

/* ── Shaping flags (packed bitfield, §3.2) ────────────────────────── */

#define SUMS_FLAG_CURSIVE        (1U << 0) /* joins on at least one side   */
#define SUMS_FLAG_VERTICAL       (1U << 1) /* has a native vertical form   */
#define SUMS_FLAG_HAS_INITIAL    (1U << 2) /* has an initial positional form */
#define SUMS_FLAG_HAS_MEDIAL     (1U << 3) /* has a medial positional form */
#define SUMS_FLAG_HAS_FINAL      (1U << 4) /* has a final positional form  */
#define SUMS_FLAG_VARIANT_SOURCE (1U << 5) /* subject to an FVS sequence   */
#define SUMS_FLAG_FVS            (1U << 6) /* is itself a variation selector */
#define SUMS_FLAG_FEMININE       (1U << 7) /* a feminine/alt stem form     */
#define SUMS_FLAG_ASSIGNED_MASK  0x00FFu   /* all defined flag bits        */

/* ── Positional form (the contextual shape of a letter, §3.3) ─────── */

#define SUMS_FORM_ISOLATED      0u /* no join on either side            */
#define SUMS_FORM_INITIAL       1u /* joins forward only                */
#define SUMS_FORM_MEDIAL        2u /* joins both sides                  */
#define SUMS_FORM_FINAL         3u /* joins backward only               */

/* ── Free Variation Selectors (UTR #54 §2, §4) ────────────────────── */

#define SUMS_FVS1               0x0000180BU /* MONGOLIAN FREE VARIATION SELECTOR ONE */
#define SUMS_FVS2               0x0000180CU /* MONGOLIAN FREE VARIATION SELECTOR TWO */
#define SUMS_FVS3               0x0000180Du /* MONGOLIAN FREE VARIATION SELECTOR THREE */
#define SUMS_FVS_MIN            0x0000180BU
#define SUMS_FVS_MAX            0x0000180Du

#define SUMS_MVS                0x0000180Eu /* MONGOLIAN VOWEL SEPARATOR */
#define SUMS_NNBSP              0x0000202Fu /* NARROW NO-BREAK SPACE     */

/* ── Mongolian blocks (Bridge plane, §2) ──────────────────────────── */

#define SUMS_BLOCK_MONGOLIAN        0x00001800UL
#define SUMS_BLOCK_MONGOLIAN_MAX    0x000018AFUL
#define SUMS_BLOCK_SUPPLEMENT       0x00116600UL
#define SUMS_BLOCK_SUPPLEMENT_MAX   0x001167FFUL

/* ── RNUR / neography carve-out (SUAS-005 §5.4) ───────────────────── */

#define SUMS_RNUR_MIN           0x00160000UL
#define SUMS_RNUR_MAX           0x0016FFFFUL
#define SUMS_RNUR_SET_SHIFT     8U

/* ── Base SUCS zone boundaries (duplicated for freestanding SUMS) ─── */

#ifndef SUMS_LIMIT_UNICODE_MAX
#define SUMS_LIMIT_UNICODE_MAX  0x0010FFFFUL
#define SUMS_LIMIT_SCP_MIN      0x00110000UL
#define SUMS_LIMIT_SCP_MAX      0x0011FFFFUL
#define SUMS_LIMIT_NATIVE_MIN   0x00120000UL
#define SUMS_LIMIT_NATIVE_MAX   0x7FFFFFEFUL
#define SUMS_LIMIT_TRAP_MIN     0x7FFFFFF0UL
#define SUMS_LIMIT_TRAP_MAX     0x7FFFFFFEUL
#define SUMS_LIMIT_SENTINEL     0x7FFFFFFFUL
#endif

/* ── Address-space zones (§2.3) ───────────────────────────────────── */

typedef enum {
    SUMS_ZONE_BRIDGE   = 0, /* 0x00000000-0x0010FFFF           */
    SUMS_ZONE_SCP      = 1, /* 0x00110000-0x0011FFFF           */
    SUMS_ZONE_RNUR     = 2, /* 0x00160000-0x0016FFFF           */
    SUMS_ZONE_NATIVE   = 3, /* rest of 0x00120000-0x7FFFFFEF   */
    SUMS_ZONE_TRAP     = 4, /* 0x7FFFFFF0-0x7FFFFFFE           */
    SUMS_ZONE_SENTINEL = 5, /* 0x7FFFFFFF                      */
    SUMS_ZONE_EXTSUCS  = 6  /* > 0x7FFFFFFF (plugin space)     */
} sums_zone_t;

/* ── Processing outcomes (§4.4) ───────────────────────────────────── */

typedef enum {
    SUMS_OK = 0,
    SUMS_ERR_NULL_ARGUMENT,       /* a NULL descriptor                       */
    SUMS_ERR_NOT_MONGOLIAN,       /* the base carries no Mongolian record    */
    SUMS_ERR_BAD_SELECTOR,        /* the second codepoint is not an FVS      */
    SUMS_ERR_ILLEGAL_ZONE,        /* a Control Plane / trap / Sentinel arg   */
    SUMS_ERR_NO_VARIANT           /* the pair has no standardized variant    */
} sums_status_code_t;

/* ── The compiled Mongolian shaping record (§3.2) ─────────────────── */

typedef struct {
    uint32_t codepoint;          /* Direct 31-bit codepoint            */
    int16_t  vertical_advance;   /* Fixed-point vertical advance, 1/64 */
    uint16_t shaping_flags;      /* Packed shaping flags               */
    uint8_t  positional_forms;   /* Bit i set: form i is distinct (§3.3)*/
    uint8_t  reserved;           /* must be zero                        */
} sums_shaping_entry_t;

/* ── The flattened variation-sequence token (§4.3) ────────────────── */

typedef struct {
    uint32_t base;               /* Mongolian letter                   */
    uint32_t selector;           /* FVS1 / FVS2 / FVS3                 */
    uint32_t token;              /* Precompiled variant glyph token    */
} sums_variant_pair_t;

/* ── Setup ────────────────────────────────────────────────────────── */

/**
 * Returns the "MAJOR.MINOR.PATCH" version string of this module.
 * Freestanding: the returned pointer is to a static string, never NULL.
 */
const char* sutr_sums_version_string(void);

/**
 * Builds the shaping and variant indexes if they are not already built.
 * Idempotent; O(n) once, n <= SUMS_ENTRY_CAPACITY. No allocation.
 */
void sutr_sums_init_registry(void);

/* ── Shaping query (§3) ───────────────────────────────────────────── */

/**
 * Resolves a Mongolian codepoint to its compiled shaping record in O(1) with one
 * hash and a bounded probe. Returns false for a codepoint outside a text space,
 * or for a Mackenzie-ordinary character with no curated record.
 */
bool sutr_sums_get_entry(uint32_t codepoint, sums_shaping_entry_t *out_entry);

/* ── Variation-sequence resolution (§4) ──────────────────────────── */

/**
 * Resolves a (base, FVS) pair to its standardized variant record. The variant
 * record is the base's shaping record with the variant glyph token folded into
 * the codepoint, so a downstream renderer treats it uniformly.
 */
bool sutr_sums_resolve_variant(uint32_t base_codepoint,
                               uint32_t selector_codepoint,
                               sums_shaping_entry_t *out_entry);

/**
 * As above, but names the failure in `out_error` (SUMS_OK on success).
 */
bool sutr_sums_resolve_variant_ex(uint32_t base_codepoint,
                                  uint32_t selector_codepoint,
                                  sums_shaping_entry_t *out_entry,
                                  sums_status_code_t *out_error);

/**
 * The precompiled token a (base, selector) pair maps to, or 0 when the pair has
 * no standardized variant. O(1).
 */
uint32_t sutr_sums_variant_token(uint32_t base_codepoint,
                                 uint32_t selector_codepoint);

/* ── Contextual positional shaping (§3.3) ────────────────────────── */

/**
 * The positional form of a letter given its neighbours, in O(1): a letter joins
 * backward when the preceding character can precede it in a cursive run, and
 * forward when the following character can follow it. A non-joining neighbour
 * (a space, a numeral, MVS, or a Mongolian letter that does not join on that
 * side) breaks the run.
 */
uint8_t sutr_sums_positional_form(uint32_t prev_codepoint,
                                  uint32_t cur_codepoint,
                                  uint32_t next_codepoint);

/**
 * The canonical name of a positional form ("isolated", "initial", "medial",
 * "final", or "unknown").
 */
const char *sutr_sums_form_name(uint8_t form);

/* ── Vertical metrics (§5) ────────────────────────────────────────── */

/**
 * The fixed-point vertical advance of a codepoint in 1/64 em, in O(1). A
 * codepoint with no curated record falls to the algorithmic default of §6.
 * Returns 0 for an inadmissible zone.
 */
int16_t sutr_sums_vertical_advance(uint32_t codepoint);

/* ── Classification predicates (§2, §5) ──────────────────────────── */

bool sutr_sums_is_mongolian(uint32_t codepoint);
bool sutr_sums_is_cursive(uint32_t codepoint);
bool sutr_sums_is_vertical(uint32_t codepoint);
bool sutr_sums_is_fvs(uint32_t codepoint);
bool sutr_sums_is_variant_source(uint32_t codepoint);
bool sutr_sums_is_feminine(uint32_t codepoint);
bool sutr_sums_has_form(uint32_t codepoint, uint8_t form);
bool sutr_sums_is_registered(uint32_t codepoint);

/* ── Zone and classification predicates (§2.3) ───────────────────── */

sums_zone_t sutr_sums_zone_of(uint32_t codepoint);
bool sutr_sums_is_text_space(uint32_t codepoint);
bool sutr_sums_is_renderable(uint32_t codepoint);
bool sutr_sums_is_neography(uint32_t codepoint);
bool sutr_sums_neography_set(uint32_t codepoint, uint32_t *out_set);

/* ── Hash and enumeration (§3.3) ─────────────────────────────────── */

uint32_t sutr_sums_hash_codepoint(uint32_t codepoint);
uint32_t sutr_sums_hash_pair(uint32_t base_codepoint,
                             uint32_t selector_codepoint);

uint32_t sutr_sums_entry_count(void);
bool sutr_sums_entry_at(uint32_t index, sums_shaping_entry_t *out_entry);

uint32_t sutr_sums_variant_count(void);
bool sutr_sums_variant_at(uint32_t index, sums_variant_pair_t *out_pair);

/* ── Taxonomy names ───────────────────────────────────────────────── */

const char *sutr_sums_zone_name(sums_zone_t zone);
const char *sutr_sums_status_code_name(sums_status_code_t code);

#ifdef __cplusplus
}
#endif

#endif /* SUTR_SUTR_SUMS_H */
