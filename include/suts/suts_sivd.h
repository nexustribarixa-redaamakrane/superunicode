#ifndef SUTS_SUTS_SIVD_H
#define SUTS_SUTS_SIVD_H

/* SUTS-004 — System Ideographic Variation Database (SIVD)
 *
 * The core OpenWindows kernel primitive governing ideographic and stylistic
 * character variation sequences. SIVD maps a registered variation sequence —
 * a base codepoint followed by a variation selector — to a specific glyph in
 * the active font bundle. Variation definitions are compiled into an immutable,
 * statically-linked binary table of fixed-size, byte-aligned C structures
 * (sivd_variation_entry_t); the OpenWindows vector font engine (suf) queries
 * the table directly at a fixed offset and fetches the glyph index in O(1)
 * time. No flat file is parsed and no buffer is allocated.
 *
 * Where a user-space internationalization layer such as UTS #37 (the Unicode
 * Ideographic Variation Database) is a set of multi-megabyte text registries
 * (IVD_Collections.txt, IVD_Sequences.txt) parsed at run time into heap-resident
 * structures, SIVD:
 *
 *   - is a compiled binary schema — fixed structs in read-only memory, not
 *     markup, so every field sits at a known offset with no parser;
 *   - obeys the zero-allocation mandate — lookup is an O(1) probe of a
 *     fixed-size open-addressing index built once over a static table, and
 *     never touches a heap;
 *   - is a kernel primitive — the microkernel, the suf font pipeline, and the
 *     terminal drivers resolve variants in-band, with no external file system,
 *     user process, or service dependency.
 *
 * The registry is a single dual-plane index. The Bridge Variation Space
 * (0x000000-0x10FFFF) maps standard Ideographic Variation Sequences built on
 * Unicode variation selectors (U+E0100-U+E01EF and U+FE00-U+FE0F). The Native
 * & Neography Variation Space (0x160000-0x16FFFF) extends the same mechanics to
 * RNUR neography scripts and conlangs, so a conlang can trigger custom
 * contextual alternates, regional glyph forms, and stylistic flourishes
 * deterministically and without heap allocation.
 *
 * Freestanding C99, zero allocation: the registry is a static array of
 * sivd_variation_entry_t plus a static array of sivd_collection_t, and the
 * lookup index is a fixed-size array of slots, none of which grows at run time.
 *
 * See docs/suts/SUTS-004-sivd.md for the normative specification.
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Specification identifiers ────────────────────────────────────── */
#define SUTS_SIVD_SPEC_ID        "SUTS-004"
#define SUTS_SIVD_SPEC_TITLE     "System Ideographic Variation Database"
#define SUTS_SIVD_VERSION_MAJOR  1
#define SUTS_SIVD_VERSION_MINOR  0
#define SUTS_SIVD_VERSION_PATCH  0

/* ── Fixed limits (static, stack-allocatable blocks) ──────────────── */
/* Maximum bytes of a collection identifier, including the terminating NUL. */
#define SUTS_SIVD_MAX_COLLECTION_ID 32U
/* Maximum bytes of a sequence identifier, including the terminating NUL. */
#define SUTS_SIVD_MAX_ENTRY_ID      16U
/* Maximum bytes of a collection's identifier pattern, including the NUL. */
#define SUTS_SIVD_MAX_ID_PATTERN    32U
/* Number of curated variation entries the static registry can hold. */
#define SUTS_SIVD_REGISTRY_CAPACITY 128U
/* Number of curated collections the static registry can hold. */
#define SUTS_SIVD_COLLECTION_CAPACITY 16U
/* Number of open-addressing slots (a power of two > capacity). */
#define SUTS_SIVD_INDEX_SIZE        256U

/* ── 64-bit ExtSUCS codepoint type (guarded to coexist) ───────────── */
#ifndef EXTSUCS_CHAR_T_DEFINED
#define EXTSUCS_CHAR_T_DEFINED
typedef uint64_t sucs_ex_char_t;
#endif

/* ── Base SUCS zone boundaries (duplicated for freestanding SIVD) ─── */
#ifndef SUTS_SIVD_ZONE_UNICODE_MAX
#define SIVD_ZONE_UNICODE_MAX  ((sucs_ex_char_t)0x0010FFFFULL)
#define SIVD_ZONE_SCP_MIN      ((sucs_ex_char_t)0x00110000ULL)
#define SIVD_ZONE_SCP_MAX      ((sucs_ex_char_t)0x0011FFFFULL)
#define SIVD_ZONE_NATIVE_MIN   ((sucs_ex_char_t)0x00120000ULL)
#define SIVD_ZONE_NATIVE_LAST  ((sucs_ex_char_t)0x7FFFFFEFULL)
#define SIVD_ZONE_TRAP_MIN     ((sucs_ex_char_t)0x7FFFFFF0ULL)
#define SIVD_ZONE_TRAP_MAX     ((sucs_ex_char_t)0x7FFFFFFEULL)
#define SIVD_ZONE_SENTINEL     ((sucs_ex_char_t)0x7FFFFFFFULL)
#endif

/* RNUR / RCCR neography carve-out inside the Native plane (SUTR-8 §1.4).
 * RCCR allocates no codepoints; the RNUR registry supplies the compound
 * glyphs and logographs that a conlang varies. */
#define SIVD_RNUR_MIN          ((sucs_ex_char_t)0x00160000ULL)
#define SIVD_RNUR_MAX          ((sucs_ex_char_t)0x0016FFFFULL)

/* ── Variation-selector ranges (§2.3) ─────────────────────────────── */
/* VS1-VS16: the base-plane variation selectors (U+FE00-U+FE0F). */
#define SUTS_SIVD_VS1_MIN      ((sucs_ex_char_t)0x0000FE00ULL)
#define SUTS_SIVD_VS1_MAX      ((sucs_ex_char_t)0x0000FE0FULL)
/* VS17-VS256: the Ideographic Variation Selectors (U+E0100-U+E01EF), the
 * range UTS #37 admits for an Ideographic Variation Sequence. */
#define SUTS_SIVD_VS17_MIN     ((sucs_ex_char_t)0x000E0100ULL)
#define SUTS_SIVD_VS17_MAX     ((sucs_ex_char_t)0x000E01EFULL)
/* RNUR neography variation selectors: the top 256 slots of the RNUR
 * carve-out, disjoint from the RNUR separators and symbols SLDF allocates
 * at the bottom of the same carve-out. */
#define SIVD_RNUR_VS_MIN       ((sucs_ex_char_t)0x0016FF00ULL)
#define SIVD_RNUR_VS_MAX       ((sucs_ex_char_t)0x0016FFFFULL)

/* ── FNV-1a constants for the (base, selector) pair hash (§4.2) ───── */
#define SUTS_SIVD_FNV_OFFSET_BASIS ((uint32_t)2166136261u)
#define SUTS_SIVD_FNV_PRIME        ((uint32_t)16777619u)

/* ── Address-space planes (§5) ────────────────────────────────────── */
typedef enum {
    SUTS_SIVD_PLANE_BRIDGE   = 0, /* 0x00000000-0x0010FFFF           */
    SUTS_SIVD_PLANE_SCP      = 1, /* 0x00110000-0x0011FFFF           */
    SUTS_SIVD_PLANE_NATIVE   = 2, /* 0x00120000-0x7FFFFFEF           */
    SUTS_SIVD_PLANE_RNUR     = 3, /* 0x00160000-0x0016FFFF           */
    SUTS_SIVD_PLANE_TRAP     = 4, /* 0x7FFFFFF0-0x7FFFFFFE           */
    SUTS_SIVD_PLANE_SENTINEL = 5, /* 0x7FFFFFFF                      */
    SUTS_SIVD_PLANE_EXTSUCS  = 6  /* > 0x7FFFFFFF (plugin space)     */
} suts_sivd_plane_t;

/* ── Status codes ─────────────────────────────────────────────────── */
typedef enum {
    SUTS_SIVD_OK              =  0,
    SUTS_SIVD_NOT_FOUND       =  1,
    SUTS_SIVD_ERR_INVALID_ARG = -1,
    SUTS_SIVD_ERR_BAD_SELECTOR = -2,
    SUTS_SIVD_ERR_OVERFLOW    = -3
} suts_sivd_status_t;

/* ── Variation-class flags (§2.1) ─────────────────────────────────── */
#define SUTS_SIVD_FLAG_NONE       0x00u /* an ordinary glyphic subset      */
#define SUTS_SIVD_FLAG_STYLISTIC  0x01u /* a stylistic alternate           */
#define SUTS_SIVD_FLAG_REGIONAL   0x02u /* a regional glyph form           */
#define SUTS_SIVD_FLAG_CONTEXTUAL 0x04u /* a contextual alternate          */

/* ── Binary-compiled variation structures (zero-allocation) ───────── */

/* A registered variation sequence (§2.1). `base_codepoint` carries the
 * Ideographic property; `variation_selector` is a selector of §2.3;
 * `resolved_glyph_index` is the font-bundle-relative glyph id the suf vector
 * font engine renders. The codepoint fields are `sucs_ex_char_t` (64-bit),
 * never `uint32_t`, so an RNUR neography base in 0x00160000-0x0016FFFF and an
 * ExtSUCS plugin base above 0x7FFFFFFF survive intact. */
typedef struct {
    sucs_ex_char_t base_codepoint;       /* the ideograph, e.g. U+4E00      */
    sucs_ex_char_t variation_selector;   /* U+E0100-U+E01EF, or an RNUR VS  */
    uint32_t       resolved_glyph_index; /* suf glyph id (opaque, compiled) */
    uint16_t       collection_index;     /* index into the collection table */
    uint8_t        plane;                /* suts_sivd_plane_t               */
    uint8_t        flags;                /* SUTS_SIVD_FLAG_*                */
} sivd_variation_entry_t;

/* A registered collection (§2.2). Registered sequences are grouped into
 * collections so that the glyphic subsets of one user community are tracked
 * together. `id_pattern` is the collection's identifier pattern, stored as an
 * opaque descriptor: SIVD never evaluates it (there is no regex engine on the
 * lookup path); the registrar validates identifiers at compile time. Each
 * collection owns a contiguous run of entries, `[first_entry, first_entry +
 * entry_count)` in the static entry table. */
typedef struct {
    char     id[SUTS_SIVD_MAX_COLLECTION_ID];   /* e.g. "Adobe-Japan1"        */
    char     id_pattern[SUTS_SIVD_MAX_ID_PATTERN]; /* opaque, e.g. "[0-9]+"   */
    uint16_t first_entry;                        /* first owned entry          */
    uint16_t entry_count;                        /* number of owned entries    */
    uint8_t  plane;                              /* suts_sivd_plane_t          */
    uint8_t  reserved;                           /* zero; readers ignore it    */
} sivd_collection_t;

/* ── Function declarations ────────────────────────────────────────── */

/* Version string, e.g. "1.0.0". */
const char* sivd_version_string(void);

/* Builds (or rebuilds) the SIVD kernel variation registry index. Idempotent and
 * allocation-free; the lookup entry points build it lazily on first use if this
 * is not called first. */
void sivd_init_registry(void);

/* Resolves a base codepoint and variation selector pair to a specific variant
 * glyph index in O(1) expected time. Returns true if a valid mapping exists,
 * and writes the glyph index through `out_glyph_index` when it is non-NULL.
 * Returns false for an unregistered pair, a malformed selector, or an
 * SCP-invading base (§4.4). */
bool sivd_resolve_variant(sucs_ex_char_t base_cp, sucs_ex_char_t var_sel,
                          uint32_t* out_glyph_index);

/* Resolves a pair to its immutable entry in O(1) expected time, or NULL. The
 * returned pointer is stable for the lifetime of the image and must never be
 * freed. */
const sivd_variation_entry_t* sivd_lookup(sucs_ex_char_t base_cp,
                                          sucs_ex_char_t var_sel);

/* True when a registered mapping exists for the pair (O(1)). */
bool sivd_is_registered(sucs_ex_char_t base_cp, sucs_ex_char_t var_sel);

/* The collection that owns the pair, or NULL when the pair is unregistered. */
const sivd_collection_t* sivd_collection_of(sucs_ex_char_t base_cp,
                                            sucs_ex_char_t var_sel);

/* ── Enumeration ──────────────────────────────────────────────────── */
/* Number of entries in the static registry (O(1)). */
size_t sivd_entry_count(void);

/* Direct, stable pointer to registry entry `index`, or NULL when out of range
 * (O(1)). The order is the static table order and is stable across builds. */
const sivd_variation_entry_t* sivd_entry_at(size_t index);

/* Number of collections in the static registry (O(1)). */
size_t sivd_collection_count(void);

/* Direct, stable pointer to registry collection `index`, or NULL (O(1)). */
const sivd_collection_t* sivd_collection_at(size_t index);

/* ── Selector and base classification (§2.3, §4.4) ────────────────── */
/* True for any variation selector SIVD admits: VS1-VS16, the ideographic
 * VS17-VS256, or an RNUR neography selector (O(1)). */
bool sivd_is_variation_selector(sucs_ex_char_t cp);

/* True for a selector admissible in an Ideographic Variation Sequence: the
 * supplementary range VS17-VS256, or an RNUR neography selector (O(1)). */
bool sivd_is_ivs_selector(sucs_ex_char_t cp);

/* True when a codepoint may serve as a variation base: it is not an SCP
 * marker, a trap slot, or the Sentinel (O(1), §4.4). */
bool sivd_is_valid_base(sucs_ex_char_t cp);

/* Classifies a codepoint into its address-space plane (O(1), §5). */
suts_sivd_plane_t sivd_plane_of(sucs_ex_char_t cp);

/* ── Pair hash ────────────────────────────────────────────────────── */
/* FNV-1a 32-bit hash over the little-endian encodings of the base codepoint
 * and the variation selector (O(1), §4.2). */
uint32_t sivd_pair_hash(sucs_ex_char_t base_cp, sucs_ex_char_t var_sel);

#ifdef __cplusplus
}
#endif

#endif /* SUTS_SUTS_SIVD_H */
