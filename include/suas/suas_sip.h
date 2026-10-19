#ifndef SUAS_SUAS_SIP_H
#define SUAS_SUAS_SIP_H

/* SUAS-009 — System Ideographic Properties (SIP)
 *
 * The core algorithmic subsystem of the OpenWindows microkernel governing
 * zero-allocation, O(1) querying and bitfield extraction of ideographic
 * character attributes: radical, stroke count, classification flags, sort
 * weight, and variant linkage.
 *
 * Where a traditional Unicode environment consults the text-heavy Unihan
 * Database (UAX #38) — a multi-megabyte flat-file collection laden with
 * historical narrative notes and string properties read at run time — SIP is a
 * dual-layer architecture:
 *
 *   - the offline reference layer is the SuperUnicode Character Database
 *     (SUCD), which carries the full text manifests and flat-file datasets for
 *     developer tooling, user-space applications, and offline documentation;
 *   - the runtime kernel layer is SIP: at build time the SUCD datasets are
 *     compiled into immutable, byte-packed binary tables (suas_glyph_prop_t),
 *     and the kernel executes the SIP algorithms directly against those static
 *     bitfields, with no text parsing and no memory allocation.
 *
 * A query discards the human-readable text keys (kTotalStrokes, kMandarin, …)
 * in favour of direct bitfield masking and a hash-indexed static array, so a
 * radical-stroke count, a sorting weight, or a variant linkage is evaluated in
 * O(1) with no heap, no string parsing, and no dynamic table growth.
 *
 * Ideographs that need a distinct regional or historical representation beyond
 * Unicode Han unification are routed directly into the Native & Neography Plane
 * (0x160000-0x16FFFF) and the SIVD variation database (SUTS-004): a flagged
 * entry carries a native variant target, and the suf vector font engine calls
 * SIP during single-pass text layout to resolve fallback rendering metrics and
 * spatial layout bounding for complex CJK and neography logographs.
 *
 * Freestanding C99, zero allocation: the engine is a static array of
 * suas_glyph_prop_t records plus a fixed-size open-addressing index, neither of
 * which grows at run time.
 *
 * See docs/suas/SUAS-009-sip.md for the normative specification.
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "suas/suas_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ── Specification identifiers ────────────────────────────────────── */
#define SUAS_SIP_SPEC_ID        "SUAS-009"
#define SUAS_SIP_SPEC_TITLE     "System Ideographic Properties"
#define SUAS_SIP_VERSION_MAJOR  1
#define SUAS_SIP_VERSION_MINOR  0
#define SUAS_SIP_VERSION_PATCH  0

/* ── Fixed limits (static, stack-allocatable packing) ─────────────── */
/* Number of curated ideographs the static property table can hold. */
#define SUAS_SIP_REGISTRY_CAPACITY 128U
/* Number of open-addressing slots (a power of two > capacity). */
#define SUAS_SIP_INDEX_SIZE        256U

/* ── 64-bit ExtSUCS codepoint type (guarded to coexist) ───────────── */
#ifndef EXTSUCS_CHAR_T_DEFINED
#define EXTSUCS_CHAR_T_DEFINED
typedef uint64_t sucs_ex_char_t;
#endif

/* ── Base SUCS zone boundaries (duplicated for freestanding SIP) ──── */
#ifndef SUAS_SIP_ZONE_UNICODE_MAX
#define SUAS_SIP_ZONE_UNICODE_MAX  ((sucs_ex_char_t)0x0010FFFFULL)
#define SUAS_SIP_ZONE_SCP_MIN      ((sucs_ex_char_t)0x00110000ULL)
#define SUAS_SIP_ZONE_SCP_MAX      ((sucs_ex_char_t)0x0011FFFFULL)
#define SUAS_SIP_ZONE_NATIVE_MIN   ((sucs_ex_char_t)0x00120000ULL)
#define SUAS_SIP_ZONE_NATIVE_LAST  ((sucs_ex_char_t)0x7FFFFFEFULL)
#define SUAS_SIP_ZONE_TRAP_MIN     ((sucs_ex_char_t)0x7FFFFFF0ULL)
#define SUAS_SIP_ZONE_TRAP_MAX     ((sucs_ex_char_t)0x7FFFFFFEULL)
#define SUAS_SIP_ZONE_SENTINEL     ((sucs_ex_char_t)0x7FFFFFFFULL)
#endif

/* RNUR / RCCR neography carve-out inside the Native plane (SUTR-8 §1.4).
 * RCCR allocates no codepoints; the RNUR registry supplies the ideograph-like
 * neography logographs whose properties SIP reports. */
#define SUAS_SIP_RNUR_MIN          ((sucs_ex_char_t)0x00160000ULL)
#define SUAS_SIP_RNUR_MAX          ((sucs_ex_char_t)0x0016FFFFULL)

/* ── Ideographic repertoire blocks (range dispatch, §2.1) ─────────── */
#define SUAS_SIP_BLOCK_EXT_A_MIN    ((sucs_ex_char_t)0x00003400ULL)
#define SUAS_SIP_BLOCK_EXT_A_MAX    ((sucs_ex_char_t)0x00004DBFULL)
#define SUAS_SIP_BLOCK_URO_MIN      ((sucs_ex_char_t)0x00004E00ULL)
#define SUAS_SIP_BLOCK_URO_MAX      ((sucs_ex_char_t)0x00009FFFULL)
#define SUAS_SIP_BLOCK_COMPAT_MIN   ((sucs_ex_char_t)0x0000F900ULL)
#define SUAS_SIP_BLOCK_COMPAT_MAX   ((sucs_ex_char_t)0x0000FAFFULL)
#define SUAS_SIP_BLOCK_EXT_B_MIN    ((sucs_ex_char_t)0x00020000ULL)
#define SUAS_SIP_BLOCK_EXT_B_MAX    ((sucs_ex_char_t)0x0002A6DFULL)
#define SUAS_SIP_BLOCK_EXT_C_MIN    ((sucs_ex_char_t)0x0002A700ULL)
#define SUAS_SIP_BLOCK_EXT_C_MAX    ((sucs_ex_char_t)0x0002EBEFULL)
#define SUAS_SIP_BLOCK_COMPAT2_MIN  ((sucs_ex_char_t)0x0002F800ULL)
#define SUAS_SIP_BLOCK_COMPAT2_MAX  ((sucs_ex_char_t)0x0002FA1FULL)
#define SUAS_SIP_BLOCK_EXT_G_MIN    ((sucs_ex_char_t)0x00030000ULL)
#define SUAS_SIP_BLOCK_EXT_G_MAX    ((sucs_ex_char_t)0x0003134FULL)

/* ── Address-space planes (§2.4) ──────────────────────────────────── */
typedef enum {
    SUAS_SIP_PLANE_BRIDGE   = 0, /* 0x00000000-0x0010FFFF           */
    SUAS_SIP_PLANE_SCP      = 1, /* 0x00110000-0x0011FFFF           */
    SUAS_SIP_PLANE_NATIVE   = 2, /* 0x00120000-0x7FFFFFEF           */
    SUAS_SIP_PLANE_RNUR     = 3, /* 0x00160000-0x0016FFFF           */
    SUAS_SIP_PLANE_TRAP     = 4, /* 0x7FFFFFF0-0x7FFFFFFE           */
    SUAS_SIP_PLANE_SENTINEL = 5, /* 0x7FFFFFFF                      */
    SUAS_SIP_PLANE_EXTSUCS  = 6  /* > 0x7FFFFFFF (plugin space)     */
} suas_sip_plane_t;

/* ── Classification flags (§2.2) ──────────────────────────────────── */
#define SUAS_SIP_FLAG_UNIFIED     0x01u /* Unified_Ideograph               */
#define SUAS_SIP_FLAG_RADICAL     0x02u /* radical-stroke data present     */
#define SUAS_SIP_FLAG_VARIANT     0x04u /* has a variant linkage           */
#define SUAS_SIP_FLAG_OBSOLETE    0x08u /* obsolete / deprecated form      */
#define SUAS_SIP_FLAG_NUMERIC     0x10u /* carries a numeric value         */
#define SUAS_SIP_FLAG_NATIVE      0x20u /* Native / RNUR-plane logograph   */
#define SUAS_SIP_FLAG_COMPAT      0x40u /* compatibility ideograph         */
#define SUAS_SIP_FLAG_TRADITIONAL 0x80u /* traditional (vs. simplified)    */

/* ── Structural type: the static property record (§3.1) ───────────── */
/* A single fixed-size, stack-allocatable record. The sketched core fields are
 * kept verbatim and in order; `codepoint` is widened to sucs_ex_char_t (64-bit)
 * so an RNUR neography logograph in 0x00160000-0x0016FFFF survives, and one
 * additive field, `native_variant`, carries the variant linkage into the
 * Native & Neography Plane (0 = none). */
typedef struct {
    sucs_ex_char_t codepoint;      /* the ideograph, e.g. U+4E00             */
    uint16_t       radical;        /* Kangxi radical (0 = none / native)     */
    uint8_t        strokes;        /* total stroke count (0 = unknown)       */
    uint8_t        flags;          /* SUAS_SIP_FLAG_* bitfield               */
    uint32_t       sort_weight;    /* SUCA collation seed (§2.3)             */
    sucs_ex_char_t native_variant; /* Native/RNUR variant target, or 0       */
} suas_glyph_prop_t;

/* ── Setup ────────────────────────────────────────────────────────── */
const char* suas_sip_version_string(void);

/* Initializes the SUAS-009 static ideographic property engine. Idempotent and
 * allocation-free; the query entry points build the index lazily on first use
 * if this is not called first. */
void suas_sip_init_engine(void);

/* ── Core query (§3.2) ────────────────────────────────────────────── */
/* Algorithmic O(1) lookup of the core ideographic properties of a codepoint,
 * evaluating bitfields and the static lookup table directly. On success
 * `*out_prop` receives a copy of the packed record and the function returns
 * true; on failure (an SCP marker, a trap slot, the Sentinel, an unregistered
 * codepoint, or a NULL output) it returns false and leaves the output
 * untouched. */
bool suas_sip_get_properties(sucs_ex_char_t codepoint, suas_glyph_prop_t* out_prop);

/* ── Enumeration ──────────────────────────────────────────────────── */
/* Number of curated ideographs in the static property table (O(1)). */
uint32_t suas_sip_entry_count(void);

/* Copy the i-th curated record (0-based). Returns false out of range or on a
 * NULL output (O(1)). */
bool suas_sip_entry_at(uint32_t index, suas_glyph_prop_t* out_prop);

/* ── Algorithmic property extraction (§2) ─────────────────────────── */
/* Radical and stroke count in one O(1) query. Either output may be NULL.
 * Returns false when the codepoint is unregistered. */
bool suas_sip_get_radical_stroke(sucs_ex_char_t codepoint,
                                 uint16_t* out_radical, uint8_t* out_strokes);

/* The SUCA collation seed for a codepoint (O(1)). Returns the curated sort
 * weight when the codepoint is registered; otherwise the algorithmic native
 * order weight for a valid ideograph. Returns false (and leaves the output
 * untouched) for a non-ideograph, an SCP marker, a trap slot, or the Sentinel. */
bool suas_sip_get_sort_weight(sucs_ex_char_t codepoint, uint32_t* out_weight);

/* The classification bitfield of a codepoint, or 0 when unregistered (O(1)). */
uint8_t suas_sip_flags(sucs_ex_char_t codepoint);

/* True when a registered codepoint carries a variant linkage (O(1)). */
bool suas_sip_has_variant(sucs_ex_char_t codepoint);

/* Resolves the Native & Neography Plane variant target of a registered
 * codepoint (O(1)). Returns false when the codepoint has no variant. */
bool suas_sip_variant_target(sucs_ex_char_t codepoint,
                             sucs_ex_char_t* out_native);

/* ── Classification predicates (§2.1, §2.4) ───────────────────────── */
/* True when a codepoint is in the ideographic repertoire: a registered
 * ideograph, a codepoint in a defined CJK ideographic block, or an RNUR
 * neography logograph (O(1) range dispatch). */
bool suas_sip_is_ideograph(sucs_ex_char_t codepoint);

/* True when a codepoint has a record in the static property table (O(1)). */
bool suas_sip_is_registered(sucs_ex_char_t codepoint);

/* Classifies a codepoint into its address-space plane (O(1), §2.4). */
suas_sip_plane_t suas_sip_plane_of(sucs_ex_char_t codepoint);

/* ── Hash ─────────────────────────────────────────────────────────── */
/* FNV-1a 32-bit hash over the little-endian encoding of the codepoint
 * (O(1), §3.3). */
uint32_t suas_sip_hash(sucs_ex_char_t codepoint);

#ifdef __cplusplus
}
#endif

#endif /* SUAS_SUAS_SIP_H */
