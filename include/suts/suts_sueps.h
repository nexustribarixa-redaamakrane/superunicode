#ifndef SUTS_SUTS_SUEPS_H
#define SUTS_SUTS_SUEPS_H

/* SUTS-007 — SuperUnicode Emoji & Presentation Specification (SUEPS)
 *
 * A modular data specification within the OpenWindows microkernel ecosystem.
 * SUEPS establishes the binary-mapped schemas, presentation properties, and
 * zero-allocation lookup engines required to render and process emoji and
 * pictographic character streams in O(1) constant time.
 *
 * Where a traditional Unicode implementation (UTS #51, "Unicode Emoji")
 * describes an emoji above the BMP through a UTF-16 surrogate pair (e.g.
 * \uD83D\uDE00 for U+1F600), a string-based ZWJ-sequence parser, and
 * dynamically allocated annotation buffers, SUEPS inherits the SuperUnicode
 * "No Surrogates" rule — the SUTR-1 (SUTF) invariant #2: SuperUnicode has no
 * surrogate mechanism at all. Every emoji, skin tone modifier, presentation
 * sequence, flag pair, keycap and ZWJ cluster is mapped directly to a native,
 * single-word 31-bit SuperUnicode codepoint or to a pre-compiled binary
 * sequence token. Surrogate recombination and string parsing are eliminated
 * outright.
 *
 * SUEPS is a dual-layer system, in the same relationship SUAS-009 (SIP),
 * SUAS-012 (SUCD) and SUTS-006 (SIDN) have with their Unicode counterparts.
 * The offline reference layer is the SuperUnicode Character Database (SUCD /
 * SUCA) — the human-readable emoji annotation files, sequence charts and
 * presentation property manifests that user-space applications, font designers
 * and developer toolchains read. Build-time tooling compiles those datasets
 * into the immutable, byte-packed binary lookup tables this module embeds into
 * graphical subsystem images. The kernel never reads a file at run time.
 *
 * Two commitments separate SUEPS from a text emoji library:
 *
 *   1. THE "NO SURROGATES" MANDATE. An emoji is a single atomic 31-bit scalar.
 *      There is no surrogate pair, no surrogate code unit, and no recombination
 *      step (§2). A Unicode emoji encoded above the BMP is one surrogate pair
 *      in UTF-16; in SuperUnicode it is exactly one codepoint, and the Query
 *      surface never sees a half. The block U+D800-U+DFFF stays what SUTR-1
 *      says it is — ordinary valid PUA — so a codepoint in that block is an
 *      acceptable emoji component, never a malformed fragment. Every query is
 *      total over the address space and answers in one comparison chain or one
 *      bounded probe.
 *   2. ZERO-ALLOCATION ZWJ & MODIFIER RESOLUTION. Complex presentations —
 *      skin tones, hair components, gender signs, color and direction — are
 *      flattened at build time into a static binary transition table. A
 *      renderer resolves a base + modifier pair by direct pointer indexing,
 *      O(1), with no malloc/free and no libc (§4).
 *
 * Because core operating system primitives, file paths and kernel IPC channels
 * require strict text safety rather than graphical emoticons, SUEPS is
 * categorized as a modular SUTS data specification: a lightweight server
 * kernel or a headless environment may omit SUEPS entirely without impacting
 * core system integrity (§1.6). The engine, the record layouts and the
 * resolution contract are architectural; the emoji *contents* are version data
 * compiled from the SUCD manifests (§8).
 *
 * Freestanding C99, zero allocation: the tables are static, every descriptor
 * is a fixed-size struct the caller owns, and no part of the engine reaches
 * for malloc, stdio or libc string routines.
 *
 * See docs/suts/SUTS-007-sueps.md for the normative specification.
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Specification identifiers ────────────────────────────────────── */
#define SUEPS_SPEC_ID            "SUTS-007"
#define SUEPS_SPEC_TITLE         "SuperUnicode Emoji & Presentation Specification"
#define SUEPS_VERSION_MAJOR      1
#define SUEPS_VERSION_MINOR      0

/* ── Fixed limits (static, stack-allocatable tables) ──────────────── */
/* The compiled emoji table holds at most this many curated records. The index
 * is a power of two larger than the capacity, so the load factor stays below
 * one-half and the probe sequence stays short and bounded. */
#define SUEPS_ENTRY_CAPACITY     128U
#define SUEPS_ENTRY_INDEX_SIZE   256U

/* FNV-1a constants for the codepoint key hash and the sequence-token hash
 * (§3.3). */
#define SUEPS_FNV_OFFSET_BASIS   2166136261u
#define SUEPS_FNV_PRIME          16777619u

/* ── Emoji presentation flags (packed bitfield, ED-6 / §3.2) ──────── */
/* The sketch called these SUEPS_FLAG_*; they are preserved verbatim so a
 * consumer written against the seeding sketch compiles unchanged. */
#define SUEPS_FLAG_EMOJI          (1U << 0) /* valid pictographic emoji      */
#define SUEPS_FLAG_TEXT_DEFAULT   (1U << 1) /* defaults to text presentation */
#define SUEPS_FLAG_MODIFIER       (1U << 2) /* skin tone / structural modifier */
#define SUEPS_FLAG_ZWJ_SEQUENCE   (1U << 3) /* part of a compiled ZWJ cluster */

/* ── Derived presentation-flag bits (above the sketch's four) ─────── */
/* These sit in bits 4-7 of `presentation_flags`, which is a uint16_t, so the
 * whole mask fits the field with room for future additions. */
#define SUEPS_FLAG_MODIFIER_BASE  (1U << 4) /* ED-12: a modifier base (§4.2) */
#define SUEPS_FLAG_COMPONENT      (1U << 5) /* ED-5: an emoji component      */
#define SUEPS_FLAG_EXTENDED_PICT  (1U << 6) /* ED-4: Extended_Pictographic   */
#define SUEPS_FLAG_PRESENTATION_SEL (1U << 7) /* VS15 / VS16 selector        */
#define SUEPS_FLAG_QUALIFIED_MASK 0x00FFu   /* all defined flag bits         */

/* ── The former UTF-16 surrogate block (SUTR-1 invariant 2, §2) ───── */
/* SuperUnicode has no surrogate mechanism: U+D800-U+DFFF is an ordinary block
 * of valid PUA codepoints. SUEPS therefore never pairs, never reserves and
 * never rejects a codepoint in this block — it is ordinary emoji-component
 * material. The two boundaries are exported so a consumer can classify the
 * block explicitly rather than by a magic number. */
#define SUEPS_PUA_HALF_MIN       0x0000D800UL /* first former-surrogate slot */
#define SUEPS_PUA_HALF_MAX       0x0000DFFFUL /* last former-surrogate slot  */

/* ── Emoji modifier codepoints (ED-11, §4) ────────────────────────── */
/* The five Fitzpatrick skin-tone modifiers, U+1F3FB..U+1F3FF. */
#define SUEPS_MODIFIER_MIN       0x0001F3FBUL
#define SUEPS_MODIFIER_MAX       0x0001F3FFUL

/* Presentation selectors (ED-8, ED-9, §5). */
#define SUEPS_VS15               0x0000FE0EUL /* text presentation selector  */
#define SUEPS_VS16               0x0000FE0FUL /* emoji presentation selector */
#define SUEPS_ZWJ                0x0000200DUL /* zero-width joiner (ED-16)   */
#define SUEPS_KEYCAP             0x000020E3UL /* COMBINING ENCLOSING KEYCAP  */

/* Regional Indicators (ED-14, Annex B) and tag characters (ED-14a). */
#define SUEPS_RI_MIN             0x0001F1E6UL
#define SUEPS_RI_MAX             0x0001F1FFUL
#define SUEPS_TAG_MIN            0x000E0020UL
#define SUEPS_TAG_MAX            0x000E007FUL
#define SUEPS_TAG_TERM           0x000E007FUL /* CANCEL TAG (tag_end)        */

/* ── Address-space zones (§2.2) ───────────────────────────────────── */
/* SUEPS does not invent a second plane taxonomy; the zones below are the SUCS
 * planes with the RNUR carve-out made explicit, because an emoji may be
 * declared in the neography registry. */
typedef enum {
    SUEPS_ZONE_BRIDGE   = 0, /* 0x00000000-0x0010FFFF           */
    SUEPS_ZONE_SCP      = 1, /* 0x00110000-0x0011FFFF           */
    SUEPS_ZONE_RNUR     = 2, /* 0x00160000-0x0016FFFF           */
    SUEPS_ZONE_NATIVE   = 3, /* rest of 0x00120000-0x7FFFFFEF   */
    SUEPS_ZONE_TRAP     = 4, /* 0x7FFFFFF0-0x7FFFFFFE           */
    SUEPS_ZONE_SENTINEL = 5, /* 0x7FFFFFFF                      */
    SUEPS_ZONE_EXTSUCS  = 6  /* > 0x7FFFFFFF (plugin space)     */
} sueps_zone_t;

/* ── RNUR / RCCR neography carve-out (SUAS-005 §5.4) ──────────────── */
#define SUEPS_RNUR_MIN           0x00160000UL
#define SUEPS_RNUR_MAX           0x0016FFFFUL
#define SUEPS_RNUR_SET_SHIFT     8U

/* ── Base SUCS zone boundaries (duplicated for freestanding SUEPS) ── */
#ifndef SUEPS_LIMIT_UNICODE_MAX
#define SUEPS_LIMIT_UNICODE_MAX  0x0010FFFFUL
#define SUEPS_LIMIT_SCP_MIN      0x00110000UL
#define SUEPS_LIMIT_SCP_MAX      0x0011FFFFUL
#define SUEPS_LIMIT_NATIVE_MIN   0x00120000UL
#define SUEPS_LIMIT_NATIVE_MAX   0x7FFFFFEFUL
#define SUEPS_LIMIT_TRAP_MIN     0x7FFFFFF0UL
#define SUEPS_LIMIT_TRAP_MAX     0x7FFFFFFEUL
#define SUEPS_LIMIT_SENTINEL     0x7FFFFFFFUL
#endif

/* ── Processing outcomes (§4.4) ───────────────────────────────────── */
/* One closed enumeration naming every defect a sequence can have. SUEPS_OK is
 * the only success value. */
typedef enum {
    SUEPS_OK = 0,
    SUEPS_ERR_NULL_ARGUMENT,      /* a NULL descriptor                      */
    SUEPS_ERR_NOT_EMOJI,          /* the base carries no emoji record       */
    SUEPS_ERR_BAD_BASE,           /* the base is not an emoji modifier base */
    SUEPS_ERR_BAD_MODIFIER,       /* the second codepoint is not a modifier */
    SUEPS_ERR_ILLEGAL_ZONE,       /* a Control Plane / trap / Sentinel arg  */
    SUEPS_ERR_UNQUALIFIED         /* the pair is not a qualified sequence   */
} sueps_status_code_t;

/* ── The compiled emoji record (§3.2) ─────────────────────────────── */
/* One fixed-size record per curated emoji codepoint. The sketch's field layout
 * is preserved verbatim and is exactly 12 bytes, byte-aligned: `sequence_hash`
 * is the precomputed FNV-1a token hash for a ZWJ / modifier chain headed by
 * this codepoint, and `modifier_base` is the index of the associated base
 * character (or 0 when this codepoint is itself a base). */
typedef struct {
    uint32_t codepoint;          /* Direct 31-bit codepoint (No Surrogates) */
    uint32_t sequence_hash;      /* Precomputed token hash for chains        */
    uint16_t presentation_flags; /* Packed attribute flags                   */
    uint8_t  modifier_base;      /* Associated base character reference      */
    uint8_t  reserved;           /* must be zero                             */
} sueps_emoji_entry_t;

/* ── Setup ────────────────────────────────────────────────────────── */
/* The version string, a NUL-terminated ASCII literal ("1.0"). */
const char* suts_sueps_version_string(void);

/* Initializes the static SUEPS emoji presentation and sequence registry.
 * Idempotent and allocation-free; every query builds the index lazily on the
 * first use if this was not called. */
void suts_sueps_init_registry(void);

/* ── Presentation query (§3) ──────────────────────────────────────── */
/* Resolves emoji presentation properties for a 31-bit codepoint in O(1) time
 * without heap allocation and without any surrogate recombination step. On
 * success `*out_entry` receives a copy of the packed record and the function
 * returns true. On failure — an inadmissible zone, an uncurated codepoint, or a
 * NULL output — it returns false and leaves the output untouched. */
bool suts_sueps_get_entry(uint32_t codepoint, sueps_emoji_entry_t *out_entry);

/* ── Sequence resolution (§4) ─────────────────────────────────────── */
/* Evaluates a pre-compiled binary emoji sequence token in O(1) time: resolves
 * a base emoji followed by a skin-tone / structural modifier from the flattened
 * transition table. On success `*out_entry` receives the resolved record (a
 * copy of the base record with SUEPS_FLAG_MODIFIER applied and the resolved
 * `sequence_hash`) and the function returns true. `*out_error` (when not NULL)
 * names the first defect on failure. */
bool suts_sueps_resolve_sequence(uint32_t base_codepoint,
                                 uint32_t modifier_codepoint,
                                 sueps_emoji_entry_t *out_entry);

/* As above, but reports the closed outcome code. This is the same resolution
 * with a diagnostic channel; `resolve_sequence` is the sketch's entry point and
 * delegates here with a NULL error pointer. */
bool suts_sueps_resolve_sequence_ex(uint32_t base_codepoint,
                                    uint32_t modifier_codepoint,
                                    sueps_emoji_entry_t *out_entry,
                                    sueps_status_code_t *out_error);

/* ── Classification predicates (§2, §5) ───────────────────────────── */
/* True when the codepoint lies in the block U+D800-U+DFFF. This is NOT a
 * defect predicate: per SUTR-1 invariant 2 the block is ordinary valid PUA and
 * is never paired, reserved or rejected. It is exported so that a consumer can
 * recognize a legacy UTF-16 surrogate value that was handed to SUEPS as a
 * scalar — and, crucially, treat it as the standalone codepoint it is rather
 * than attempting to recombine it with a neighbour. */
bool suts_sueps_is_pua_half(uint32_t codepoint);

/* True for a character with the Emoji property (ED-3). A codepoint with no
 * curated record answers by the total algorithmic default of §3.4. */
bool suts_sueps_is_emoji(uint32_t codepoint);

/* True for a character with the Emoji_Modifier property (ED-11), i.e. one of
 * U+1F3FB..U+1F3FF. */
bool suts_sueps_is_modifier(uint32_t codepoint);

/* True for a character with the Emoji_Modifier_Base property (ED-12): an emoji
 * whose appearance a following modifier may change. */
bool suts_sueps_is_modifier_base(uint32_t codepoint);

/* True for an emoji component (ED-5): a character used in emoji sequences that
 * normally does not appear on an emoji keyboard as a separate choice, such as
 * a keycap base character or a Regional_Indicator. */
bool suts_sueps_is_component(uint32_t codepoint);

/* True for an Extended_Pictographic character (ED-4). */
bool suts_sueps_is_extended_pictographic(uint32_t codepoint);

/* True for a default emoji presentation character (ED-6): Emoji_Presentation=Yes. */
bool suts_sueps_is_emoji_presentation(uint32_t codepoint);

/* True for a default text presentation character (ED-7): Emoji=Yes with
 * Emoji_Presentation=No. */
bool suts_sueps_is_text_default(uint32_t codepoint);

/* True for U+FE0E (VS15) or U+FE0F (VS16), the two presentation selectors. */
bool suts_sueps_is_presentation_selector(uint32_t codepoint);

/* True for a Regional_Indicator codepoint, U+1F1E6..U+1F1FF (ED-14, Annex B). */
bool suts_sueps_is_regional_indicator(uint32_t codepoint);

/* True for a tag character, U+E0020..U+E007F (ED-14a, Annex C). */
bool suts_sueps_is_tag_character(uint32_t codepoint);

/* ── Zone and classification predicates (§2.2) ────────────────────── */
/* Classifies a 31-bit codepoint into its address-space zone (O(1)). A value
 * above the Sentinel is EXTSUCS. */
sueps_zone_t suts_sueps_zone_of(uint32_t codepoint);

/* True for the zones an emoji may inhabit: Bridge, RNUR and Native. False for
 * the Control Plane, the trap slots, the Sentinel and ExtSUCS. */
bool suts_sueps_is_renderable(uint32_t codepoint);

/* True inside the RNUR neography carve-out (O(1)). */
bool suts_sueps_is_neography(uint32_t codepoint);

/* Set number of an RNUR coordinate: (cp - 0x00160000) >> 8. Returns false
 * outside the carve-out. */
bool suts_sueps_neography_set(uint32_t codepoint, uint32_t *out_set);

/* ── Hash and enumeration (§3.3) ──────────────────────────────────── */
/* FNV-1a 32-bit over the little-endian four bytes of the codepoint. This is
 * the registry index key, not a labelled identity. */
uint32_t suts_sueps_hash_codepoint(uint32_t codepoint);

/* FNV-1a 32-bit over a (base, modifier) pair — the sequence token hash a
 * renderer keys a flattened transition on. Deterministic and value-sensitive;
 * equal pairs hash equally and no allocation occurs. */
uint32_t suts_sueps_hash_sequence(uint32_t base_codepoint,
                                  uint32_t modifier_codepoint);

uint32_t suts_sueps_entry_count(void);
bool suts_sueps_entry_at(uint32_t index, sueps_emoji_entry_t *out_entry);

/* ── Taxonomy names ───────────────────────────────────────────────── */
const char* suts_sueps_zone_name(sueps_zone_t zone);
const char* suts_sueps_status_code_name(sueps_status_code_t code);

#ifdef __cplusplus
}
#endif

#endif /* SUTS_SUTS_SUEPS_H */
