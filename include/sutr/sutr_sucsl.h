#ifndef SUTR_SUCSL_H
#define SUTR_SUCSL_H

/* SUTR-012 — SuperUnicode Cuneiform Sign Lists (SUCSL)
 *
 * A modular technical-report specification within the OpenWindows microkernel
 * ecosystem. SUCSL establishes the binary-mapped schemas, sign hierarchy
 * indices, and zero-allocation lookup engines required to index, parse, and
 * render ancient Cuneiform sign lists, values, and composite glyph structures
 * in O(1) constant time.
 *
 * Where a traditional Unicode implementation (UTR #56, "Unicode Cuneiform Sign
 * Lists") relies on complex, text-based catalog files, unstructured sign-name
 * cross-references, and dynamic string parsing engines to navigate thousands of
 * archaic logograms and syllabograms, SUCSL compiles comprehensive Cuneiform
 * sign lists directly into immutable, byte-aligned binary indexes. This allows
 * historical text parsers, archaeological font engines, and rendering pipelines
 * to resolve sign values, alternate readings, and compound numerical components
 * instantly without a single heap allocation (malloc/free).
 *
 * SUCSL is a dual-layer system, in the same relationship SUAS-009 (SIP),
 * SUAS-012 (SUCD), SUTS-007 (SUEPS), SUTS-008 (SUSCH) and SUTR-11 (SUMS) have
 * with their Unicode counterparts. The offline reference layer is the
 * SuperUnicode Character Database (SUCD / SUAS-012) — the human-readable Oracc
 * Sign List manifests, sign-value charts and glyph-style tables that
 * assyriologists, font designers and developer toolchains read. Build-time
 * tooling compiles those datasets into the immutable, byte-packed binary lookup
 * tables this module embeds into graphical subsystem images. The kernel never
 * reads a catalog file at run time.
 *
 * Two commitments separate SUCSL from a text-based sign catalog:
 *
 *   1. PRE-COMPILED BINARY SIGN HIERARCHIES. The clumsy text-based catalog
 *      parse is replaced by structured, byte-packed sign descriptors
 *      (sucsl_sign_entry_t). Each entry maps a 31-bit SuperUnicode Cuneiform
 *      codepoint directly to its historical sign category (Numerical,
 *      Syllabic, Logographic, Composite), its primary value hash, its wedge
 *      count and its chronological period tag (§4). Compound composition is a
 *      second fixed-size table, so GUD×KUR, SAL.KUR and GU₂.GAR₃ resolve by
 *      direct indexing rather than by a string split (§4.6).
 *   2. ZERO-ALLOCATION O(1) SIGN LOOKUP. During text rendering or database
 *      querying, the microkernel's script subsystem takes any Cuneiform
 *      codepoint and queries the pre-compiled binary sign records via direct
 *      memory indexing. A single FNV-1a hash and a bounded probe replace the
 *      string-scanning loop and the dynamic buffer allocation, fulfilling hard
 *      real-time kernel performance guarantees (§4.7).
 *
 * Because ancient script support and specialized epigraphic analysis are
 * modular reporting features rather than core microkernel bootstrapping
 * requirements, SUCSL is categorized as a modular SUTR technical specification
 * (§1.6). Lightweight or standard desktop kernels can incorporate SUCSL on
 * demand. The engine, the record layouts and the resolution contract are
 * architectural; the sign *contents* are version data compiled from the SUCD
 * manifests (§8).
 *
 * Extension to custom epigraphic and conscript sign lists: because SUCSL's
 * sign-list registry structure is fully generalized, its binary layout tables
 * can be extended by user-space toolchains to support custom logographic sign
 * lists and complex writing systems created for constructed languages within
 * the Native & Neography Plane (0x160000–0x16FFFF) (§5).
 *
 * Freestanding C99, zero allocation: the tables are static, every descriptor is
 * a fixed-size struct the caller owns, and no part of the engine reaches for
 * malloc, stdio or libc string routines.
 *
 * See docs/sutr/SUTR-012-sucsl.md for the normative specification.
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#if !defined(__STDC_HOSTED__) || __STDC_HOSTED__ == 0
/* Freestanding environment confirmed */
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * SUTR-012 Specification Identifiers
 * ------------------------------------------------------------------------- */
#define SUCSL_SPEC_ID           "SUTR-012"
#define SUCSL_SPEC_TITLE        "SuperUnicode Cuneiform Sign Lists"
#define SUCSL_VERSION_MAJOR     0
#define SUCSL_VERSION_MINOR     1
#define SUCSL_VERSION_PATCH     0
#define SUCSL_MODULE            "sutr_sucsl"

/* -------------------------------------------------------------------------
 * Fixed limits (static, stack-allocatable tables)
 * ------------------------------------------------------------------------- */
/* The curated sign table holds at most this many records. The index is a power
 * of two larger than the capacity, so the load factor stays below one-half and
 * the open-addressing probe stays short and bounded. */
#define SUCSL_SIGN_CAPACITY      160U
#define SUCSL_SIGN_INDEX_SIZE    512U
#define SUCSL_COMPOUND_CAPACITY  64U
#define SUCSL_SEQUENCE_CAPACITY  64U

/* FNV-1a constants for the codepoint key and the primary-value skeleton. */
#define SUCSL_FNV_OFFSET_BASIS   2166136261u
#define SUCSL_FNV_PRIME          16777619u

/* -------------------------------------------------------------------------
 * Cuneiform Sign Classification Flags (Packed Bitfields)
 * ------------------------------------------------------------------------- */
#define SUCSL_FLAG_LOGOGRAM     (1U << 0) /* Functions as an independent word sign */
#define SUCSL_FLAG_SYLLABIC     (1U << 1) /* Functions as a phonetic syllable value */
#define SUCSL_FLAG_NUMERICAL    (1U << 2) /* Part of a numerical notation system */
#define SUCSL_FLAG_COMPOSITE    (1U << 3) /* Built from multiple wedge components */

/* -------------------------------------------------------------------------
 * Derived sign-flag bits (above the sketch's four)
 * ------------------------------------------------------------------------- */
/* These sit in bits 4-7 of `sign_flags`. The field is a uint16_t, so bits 8-15
 * remain reserved for a future revision and must read as zero in a compiled
 * record. */
#define SUCSL_FLAG_DEPRECATED   (1U << 4) /* withdrawn / erroneous sign (§3)       */
#define SUCSL_FLAG_SEQUENCE     (1U << 5) /* encoded as a sequence of signs (§2.2) */
#define SUCSL_FLAG_VARIANT      (1U << 6) /* variant stacking / glyph family (§2.1.2) */
#define SUCSL_FLAG_UNCLASSIFIED (1U << 7) /* synthesized record, no curated flags  */
#define SUCSL_FLAG_ASSIGNED_MASK 0x00FFu  /* all defined low-byte flag bits        */

/* A synthesized RNUR record additionally carries this bit, so a consumer can
 * distinguish a custom epigraphic sign from a curated Bridge sign (§5). */
#define SUCSL_FLAG_EXTENSION    (1U << 8)

/* -------------------------------------------------------------------------
 * Structural Types (Zero-Allocation)
 * ------------------------------------------------------------------------- */
typedef struct {
    uint32_t codepoint;          /* Raw 31-bit SuperUnicode Cuneiform codepoint */
    uint32_t primary_value_hash; /* Precomputed hash of primary phonetic/logographic value */
    uint16_t sign_flags;         /* Packed sign classification flags */
    uint8_t  wedge_count;        /* Total constituent wedge count for rendering hints */
    uint8_t  historical_period;  /* Chronological classification tag */
} sucsl_sign_entry_t;

/* A compound or sequence descriptor (§4.6). `sign` is the atomic compound
 * codepoint, or 0 when the sign is represented in text by its components (a
 * pure sequence such as GEME₂ = SAL.KUR). `name_hash` is the FNV-1a hash of the
 * structural-name skeleton ("gudxkur", "geme2", "idigna"); it is the lookup key
 * for a sequence. `component_*` are the component codepoints in reading order. */
typedef struct {
    uint32_t sign;               /* Atomic compound codepoint, or 0 for a sequence */
    uint32_t name_hash;          /* FNV-1a of the structural-name skeleton        */
    uint32_t component_a;        /* First component codepoint                    */
    uint32_t component_b;        /* Second component codepoint, or 0             */
    uint32_t component_c;        /* Third component codepoint, or 0              */
    uint8_t  component_count;    /* 1..3                                         */
    uint8_t  compound_flags;     /* SUCSL_COMPOUND_* bits                        */
    uint16_t reserved;           /* must be zero                                 */
} sucsl_compound_t;

/* Compound/sequence flags (§4.6). */
#define SUCSL_COMPOUND_SEQUENCE   (1U << 0) /* text is the component sequence    */
#define SUCSL_COMPOUND_LIGATURE   (1U << 1) /* a style-dependent font ligature   */
#define SUCSL_COMPOUND_DEPRECATED (1U << 2) /* withdrawn / erroneous sign        */
#define SUCSL_COMPOUND_EARLY      (1U << 3) /* third-millennium-only analysis    */
#define SUCSL_COMPOUND_FLAG_MASK  0x00FFu

/* -------------------------------------------------------------------------
 * Sign categories (the four functional classes of §2.1)
 * ------------------------------------------------------------------------- */
#define SUCSL_CATEGORY_NUMERICAL    0u /* numerical notation                 */
#define SUCSL_CATEGORY_SYLLABIC     1u /* phonetic syllable                  */
#define SUCSL_CATEGORY_LOGOGRAPHIC  2u /* independent word sign              */
#define SUCSL_CATEGORY_COMPOSITE    3u /* structural compound only           */
#define SUCSL_CATEGORY_UNKNOWN      4u /* synthesized / unclassified record  */

/* -------------------------------------------------------------------------
 * Historical period tags (the `historical_period` byte, §4.5)
 * ------------------------------------------------------------------------- */
#define SUCSL_PERIOD_UNKNOWN        0u /* no chronological classification    */
#define SUCSL_PERIOD_EARLY_DYNASTIC 1u /* Early Dynastic IIIa-IIIb          */
#define SUCSL_PERIOD_UR_III         2u /* Ur III                             */
#define SUCSL_PERIOD_OLD_BABYLONIAN 3u /* Old Babylonian                     */
#define SUCSL_PERIOD_OLD_ASSYRIAN   4u /* Old Assyrian                       */
#define SUCSL_PERIOD_MIDDLE_ASSYRIAN 5u /* Middle Assyrian                   */
#define SUCSL_PERIOD_NEO_ASSYRIAN   6u /* Neo-Assyrian                       */
#define SUCSL_PERIOD_HELLENISTIC    7u /* Hellenistic                        */
#define SUCSL_PERIOD_ALL            8u /* attested across the whole record   */

/* -------------------------------------------------------------------------
 * Cuneiform blocks (Bridge plane, §4.1)
 * ------------------------------------------------------------------------- */
#define SUCSL_BLOCK_CUNEIFORM_MIN   0x00012000UL /* Cuneiform               */
#define SUCSL_BLOCK_CUNEIFORM_MAX   0x000123FFUL
#define SUCSL_BLOCK_NUMBERS_MIN     0x00012400UL /* Cuneiform Numbers and Punctuation */
#define SUCSL_BLOCK_NUMBERS_MAX     0x0001247FUL
#define SUCSL_BLOCK_EARLY_MIN       0x00012480UL /* Early Dynastic Cuneiform */
#define SUCSL_BLOCK_EARLY_MAX       0x0001254FUL

/* -------------------------------------------------------------------------
 * RNUR / neography carve-out (SUAS-005 §5.4, §5)
 * ------------------------------------------------------------------------- */
#define SUCSL_RNUR_MIN              0x00160000UL
#define SUCSL_RNUR_MAX              0x0016FFFFUL
#define SUCSL_RNUR_SET_SHIFT        8U
#define SUCSL_RNUR_SET_SIZE         (1UL << SUCSL_RNUR_SET_SHIFT) /* 256 codepoints per set */

/* -------------------------------------------------------------------------
 * Base SUCS zone boundaries (duplicated for freestanding SUCSL)
 * ------------------------------------------------------------------------- */
#ifndef SUCSL_LIMIT_UNICODE_MAX
#define SUCSL_LIMIT_UNICODE_MAX     0x0010FFFFUL
#define SUCSL_LIMIT_SCP_MIN         0x00110000UL
#define SUCSL_LIMIT_SCP_MAX         0x0011FFFFUL
#define SUCSL_LIMIT_NATIVE_MIN      0x00120000UL
#define SUCSL_LIMIT_NATIVE_MAX      0x7FFFFFEFUL
#define SUCSL_LIMIT_TRAP_MIN        0x7FFFFFF0UL
#define SUCSL_LIMIT_TRAP_MAX        0x7FFFFFFEUL
#define SUCSL_LIMIT_SENTINEL        0x7FFFFFFFUL
#endif

/* -------------------------------------------------------------------------
 * Address-space zones (§4.1)
 * ------------------------------------------------------------------------- */
typedef enum {
    SUCSL_ZONE_BRIDGE   = 0, /* 0x00000000-0x0010FFFF           */
    SUCSL_ZONE_SCP      = 1, /* 0x00110000-0x0011FFFF           */
    SUCSL_ZONE_RNUR     = 2, /* 0x00160000-0x0016FFFF           */
    SUCSL_ZONE_NATIVE   = 3, /* rest of 0x00120000-0x7FFFFFEF   */
    SUCSL_ZONE_TRAP     = 4, /* 0x7FFFFFF0-0x7FFFFFFE           */
    SUCSL_ZONE_SENTINEL = 5, /* 0x7FFFFFFF                      */
    SUCSL_ZONE_EXTSUCS  = 6  /* > 0x7FFFFFFF (plugin space)     */
} sucsl_zone_t;

/* -------------------------------------------------------------------------
 * Processing outcomes (§4.7)
 * ------------------------------------------------------------------------- */
typedef enum {
    SUCSL_OK = 0,
    SUCSL_ERR_NULL_ARGUMENT,      /* a NULL descriptor                      */
    SUCSL_ERR_NOT_CUNEIFORM,      /* the codepoint carries no sign record   */
    SUCSL_ERR_ILLEGAL_ZONE        /* SCP / trap / Sentinel / ExtSUCS arg    */
} sucsl_status_code_t;

/* -------------------------------------------------------------------------
 * Function Declarations
 * ------------------------------------------------------------------------- */

/**
 * Returns the "MAJOR.MINOR.PATCH" version string of this module.
 * Freestanding: the returned pointer is to a static string, never NULL.
 */
const char* sutr_sucsl_version_string(void);

/**
 * Initializes the static SUCSL Cuneiform sign list lookup engine.
 *
 * Idempotent and allocation-free; every query builds the index lazily on the
 * first use if this was not called.
 */
void sutr_sucsl_init_registry(void);

/**
 * Queries pre-compiled Cuneiform sign properties and classifications in O(1) time
 * without heap allocation or string scanning.
 *
 * On success `*out_entry` receives a copy of the packed record and the function
 * returns true. An uncurated codepoint inside the Cuneiform blocks or the RNUR
 * carve-out receives a synthesized record (§6); an ordinary starter, a
 * Control-Plane value, a trap or the Sentinel returns false and leaves the
 * output untouched.
 */
bool sutr_sucsl_get_sign_entry(uint32_t codepoint, sucsl_sign_entry_t *out_entry);

/**
 * As above, but names the failure in `out_error` (SUCSL_OK on success).
 */
bool sutr_sucsl_get_sign_entry_ex(uint32_t codepoint,
                                  sucsl_sign_entry_t *out_entry,
                                  sucsl_status_code_t *out_error);

/* ── Classification predicates (§2, §4.3) ──────────────────────────── */

bool sutr_sucsl_is_cuneiform(uint32_t codepoint);   /* in a Bridge Cuneiform block */
bool sutr_sucsl_is_sign(uint32_t codepoint);        /* curated sign record          */
bool sutr_sucsl_is_logogram(uint32_t codepoint);
bool sutr_sucsl_is_syllabic(uint32_t codepoint);
bool sutr_sucsl_is_numerical(uint32_t codepoint);
bool sutr_sucsl_is_composite(uint32_t codepoint);
bool sutr_sucsl_is_deprecated(uint32_t codepoint);
bool sutr_sucsl_is_variant(uint32_t codepoint);

/* Which one of the four functional categories the record belongs to (§2.1). */
uint8_t sutr_sucsl_category_of(uint16_t sign_flags);
uint8_t sutr_sucsl_sign_category(uint32_t codepoint);
const char *sutr_sucsl_category_name(uint8_t category);

/* ── Sign values and rendering hints (§4.4, §4.5) ─────────────────── */

/**
 * FNV-1a 32-bit over the ASCII bytes of a value or structural-name skeleton.
 * Deterministic, allocation-free, and identical to the hash stored in a record.
 */
uint32_t sutr_sucsl_hash_value(const char *text);

uint32_t sutr_sucsl_primary_value_hash(uint32_t codepoint);
uint8_t  sutr_sucsl_wedge_count(uint32_t codepoint);
uint8_t  sutr_sucsl_historical_period(uint32_t codepoint);
const char *sutr_sucsl_period_name(uint8_t period);

/* ── Compound composition and sequences (§4.6) ────────────────────── */

bool sutr_sucsl_get_composition(uint32_t sign, sucsl_compound_t *out_compound);
bool sutr_sucsl_find_sequence(uint32_t name_hash, sucsl_compound_t *out_sequence);

uint32_t sutr_sucsl_compound_count(void);
bool sutr_sucsl_compound_at(uint32_t index, sucsl_compound_t *out_compound);
uint32_t sutr_sucsl_sequence_count(void);
bool sutr_sucsl_sequence_at(uint32_t index, sucsl_compound_t *out_sequence);

/* ── Sign hierarchy indices and enumeration (§4.2, §4.7) ──────────── */

/**
 * The 1-based hierarchy index of a curated sign, or 0 when the codepoint has no
 * curated record. This is the SUCSL analog of a sign-list number.
 */
uint32_t sutr_sucsl_sign_index(uint32_t codepoint);

uint32_t sutr_sucsl_sign_count(void);
bool sutr_sucsl_sign_at(uint32_t index, sucsl_sign_entry_t *out_entry);

/* ── Zone and classification predicates (§4.1, §5) ────────────────── */

sucsl_zone_t sutr_sucsl_zone_of(uint32_t codepoint);
bool sutr_sucsl_is_text_space(uint32_t codepoint);
bool sutr_sucsl_is_native(uint32_t codepoint);
bool sutr_sucsl_is_neography(uint32_t codepoint);
bool sutr_sucsl_neography_set(uint32_t codepoint, uint32_t *out_set);

/* ── Hash and taxonomy names ───────────────────────────────────────── */

uint32_t sutr_sucsl_hash_codepoint(uint32_t codepoint);

const char *sutr_sucsl_zone_name(sucsl_zone_t zone);
const char *sutr_sucsl_status_code_name(sucsl_status_code_t code);

#ifdef __cplusplus
}
#endif

#endif /* SUTR_SUCSL_H */
