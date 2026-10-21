#ifndef SUAS_SUAS_SISR_H
#define SUAS_SUAS_SISR_H

/* SUAS-013 — System Ideographic Source Registry (SISR)
 *
 * The core algorithmic subsystem governing the *identity* of ideographic
 * characters: which external source record a codepoint came from, which
 * registry class it belongs to, and which canonical glyph it falls back to
 * when the preferred form is unavailable.
 *
 * UAX #45, "U-Source Ideographs", describes that identity in prose and in a
 * text file. A U-source identifier such as UTC-00932 carries a two-or-three
 * letter authority prefix, a zero-padded serial, a status, an optional source
 * field, an IDS decomposition, radical-stroke data, and a first residual
 * stroke count — spread across lines, comments and narrative notes that every
 * implementation has to render into its own shape. SISR is the runtime
 * counterpart of that idea, in the same dual-layer relationship that SUAS-009
 * (SIP) has with UAX #38:
 *
 *   - the offline reference layer is the SuperUnicode Character Database
 *     (SUCD) and the published /Public tree: the `sources/Sources.txt`
 *     provenance manifests, the RNUR allocation coordinates defined by
 *     SUAS-005 (SSP) §5.4, and the name manifests that carry external aliases;
 *   - the runtime kernel layer is this header: a byte-packed source record, a
 *     byte-packed variant link, a fixed open-addressing index, and a bounded
 *     query surface.
 *
 * Three commitments separate SISR from a text-file registry:
 *
 *   1. ONE RECORD. A source identity is a fixed 16-byte descriptor, not a
 *      tuple of strings. Every field is byte-aligned and every reserved bit is
 *      proven zero before the table is admitted.
 *   2. A BOUNDED lookup. A query hashes the codepoint, probes a fixed index,
 *      and returns the record — O(1) expected, with a small constant number of
 *      probes, no allocation, no search, and no libc.
 *   3. A PROVEN TABLE. The verification surface names every defect a compiled
 *      registry can have: disorder, duplicates, an inadmissible codepoint, a
 *      bad authority, a bad serial, a reserved bit, a hash that does not match
 *      its canonical token, and a broken fallback link.
 *
 * SISR does not re-implement SUAS-009. SIP answers what an ideograph *is*
 * (radical, strokes, classification, collation seed); SISR answers where it
 * *came from* and what replaces it. Where both standards name a fallback, the
 * two tables must agree; the reference tests enforce that invariant.
 *
 * SUAS-013 is registered as a mandatory architecture specification. The engine,
 * the record layout and the verification rules are architectural. The registry
 * *contents*, however, are version data: an implementation with no ideographic
 * sources ships an empty registry and remains conformant (§1.4, §11).
 *
 * Freestanding C99, zero allocation: the tables are static, every descriptor
 * is a fixed-size struct the caller owns, and no part of the engine reaches for
 * malloc, stdio, or libc string routines.
 *
 * See docs/suas/SUAS-013-sisr.md for the normative specification.
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "suas/suas_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ── Specification identifiers ───────────────────────────────────── */
#define SUAS_SISR_SPEC_ID        "SUAS-013"
#define SUAS_SISR_SPEC_TITLE     "System Ideographic Source Registry"
#define SUAS_SISR_VERSION_MAJOR  1
#define SUAS_SISR_VERSION_MINOR  0
#define SUAS_SISR_VERSION_PATCH  0

/* ── Fixed limits (static, stack-allocatable packing) ─────────────── */
/* The curated registry holds at most this many source records. The index is
 * a power of two larger than the capacity, so the load factor stays below
 * one-half and the probe sequence stays short and bounded. */
#define SUAS_SISR_REGISTRY_CAPACITY 256U
#define SUAS_SISR_INDEX_SIZE        512U

/* Variant/fallback links. A link is not a source record: a codepoint may have
 * at most one fallback, so this table is smaller than the registry. */
#define SUAS_SISR_LINK_CAPACITY     64U
#define SUAS_SISR_LINK_INDEX_SIZE   128U

/* A source serial is 24 bits. Zero is never a valid serial: it is the "no
 * source identity" value a caller can test without consulting the index. */
#define SUAS_SISR_SERIAL_MIN        1UL
#define SUAS_SISR_SERIAL_MAX        0x00FFFFFFUL
#define SUAS_SISR_SERIAL_BITS       24U

/* Longest canonical token: a four-letter prefix, a hyphen, eight decimal
 * digits for a full 24-bit serial, and the terminating NUL. */
#define SUAS_SISR_TOKEN_MAX         24U

/* ── Base SUCS zone boundaries ───────────────────────────────────── */
/* SISR operates over the 31-bit Base SUCS space. The high Native-plane ranges
 * are duplicated here so the header is freestanding even when an ExtSUCS
 * consumer has already redefined the shared names. */
#ifndef SUAS_SISR_LIMIT_UNICODE_MAX
#define SUAS_SISR_LIMIT_UNICODE_MAX 0x0010FFFFUL
#define SUAS_SISR_LIMIT_SCP_MIN     0x00110000UL
#define SUAS_SISR_LIMIT_SCP_MAX     0x0011FFFFUL
#define SUAS_SISR_LIMIT_NATIVE_MIN  0x00120000UL
#define SUAS_SISR_LIMIT_NATIVE_MAX  0x7FFFFFEFUL
#define SUAS_SISR_LIMIT_TRAP_MIN    0x7FFFFFF0UL
#define SUAS_SISR_LIMIT_TRAP_MAX    0x7FFFFFFEUL
#define SUAS_SISR_LIMIT_SENTINEL    0x7FFFFFFFUL
#endif

/* RNUR / RCCR neography carve-out inside the Native plane (SUAS-005 §5.4).
 * The set number of an allocation is (cp - 0x00160000) >> 8. */
#define SUAS_SISR_RNUR_MIN          0x00160000UL
#define SUAS_SISR_RNUR_MAX          0x0016FFFFUL
#define SUAS_SISR_RNUR_SET_SHIFT    8U

/* ── Address-space zones (§5) ─────────────────────────────────────── */
/* SISR deliberately does not invent a second plane taxonomy. The zones below
 * are the SUCD/SIP planes with the RNUR carve-out made explicit, because that
 * carve-out is where neography source identities live. */
typedef enum {
    SUAS_SISR_ZONE_BRIDGE   = 0, /* 0x00000000-0x0010FFFF           */
    SUAS_SISR_ZONE_SCP      = 1, /* 0x00110000-0x0011FFFF           */
    SUAS_SISR_ZONE_RNUR     = 2, /* 0x00160000-0x0016FFFF           */
    SUAS_SISR_ZONE_NATIVE   = 3, /* rest of 0x00120000-0x7FFFFFEF   */
    SUAS_SISR_ZONE_TRAP     = 4, /* 0x7FFFFFF0-0x7FFFFFFE           */
    SUAS_SISR_ZONE_SENTINEL = 5, /* 0x7FFFFFFF                      */
    SUAS_SISR_ZONE_EXTSUCS  = 6  /* > 0x7FFFFFFF (plugin space)     */
} suas_sisr_zone_t;

/* ── Source authority (§4.1) ──────────────────────────────────────── */
/* The authority is the identifier prefix: who issued the source record. It is
 * a closed enumeration so the canonical token has a fixed spelling. */
typedef enum {
    SISR_SOURCE_TYPE_INVALID = 0,
    SISR_SOURCE_TYPE_UTC     = 1, /* UTC-##### : Unicode Technical Committee */
    SISR_SOURCE_TYPE_UCI     = 2, /* UCI-##### : Unihan CJK index            */
    SISR_SOURCE_TYPE_UK      = 3, /* UK-###### : UK source (historical)       */
    SISR_SOURCE_TYPE_RNUR    = 4, /* RNUR-#### : neography registry          */
    SISR_SOURCE_TYPE_CONLANG = 5, /* CONL-#### : conlang submission          */
    SISR_SOURCE_TYPE_CUSTOM  = 6  /* CUST-#### : implementation allocation   */
} suas_sisr_source_type_t;

/* ── Registry flags (§4.2) ────────────────────────────────────────── */
/* Six bits are assigned; the upper ten are reserved and must be zero. The two
 * RNUR set bits are mutually exclusive, not because RNUR has only two sets,
 * but because a single 16-bit record can classify only the two ideographic
 * sets this revision models; a future revision widens the field by a version
 * bump, never by reusing a reserved bit. */
#define SISR_SOURCE_RNUR_SET1     0x0001u /* RNUR ideographic set 1        */
#define SISR_SOURCE_RNUR_SET2     0x0002u /* RNUR ideographic set 2        */
#define SISR_SOURCE_CONLANG       0x0004u /* conlang-origin record         */
#define SISR_SOURCE_HISTORICAL    0x0008u /* historical / obsolete source  */
#define SISR_SOURCE_VERIFIED      0x0010u /* evidence meets the current bar */
#define SISR_SOURCE_DEPRECATED    0x0020u /* superseded duplicate record   */

#define SISR_SOURCE_SET_MASK      0x0003u
#define SISR_SOURCE_ASSIGNED_MASK 0x003Fu

/* ── Variant link flags (§6) ──────────────────────────────────────── */
/* A link describes *why* the fallback exists. The low eight bits are assigned;
 * the upper eight are reserved. A link with no flag is not a link: the
 * verifier rejects it, because an unclassified fallback cannot be audited. */
#define SISR_LINK_SIMPLIFIED      0x0001u /* simplified form               */
#define SISR_LINK_TRADITIONAL     0x0002u /* traditional form              */
#define SISR_LINK_Z_VARIANT       0x0004u /* z-variant                     */
#define SISR_LINK_COMPATIBILITY   0x0008u /* compatibility ideograph       */
#define SISR_LINK_REORDERED       0x0010u /* component-reordered variant   */
#define SISR_LINK_APPROXIMATE     0x0020u /* approximate IDS match         */
#define SISR_LINK_REGISTRY        0x0040u /* registry fallback to Native   */
#define SISR_LINK_CUSTOM          0x0080u /* implementation-defined link   */

#define SISR_LINK_ASSIGNED_MASK   0x00FFu

/* ── Verification outcomes (§9) ───────────────────────────────────── */
/* One closed enumeration for both tables. Every outcome names a defect a
 * compiler can report before it emits an image; `SISR_REGISTRY_OK` is the only
 * success value. An empty table is valid. */
typedef enum {
    SISR_REGISTRY_OK = 0,
    SISR_REGISTRY_NULL_TABLE,           /* count > 0, pointer NULL          */
    SISR_REGISTRY_COUNT_OVER,           /* count > static capacity          */
    SISR_REGISTRY_UNORDERED,            /* codepoints not ascending         */
    SISR_REGISTRY_DUPLICATE_CODEPOINT,  /* same codepoint twice             */
    SISR_REGISTRY_OUT_OF_SPACE,         /* codepoint > 0x7FFFFFFF           */
    SISR_REGISTRY_INADMISSIBLE_CODEPOINT, /* SCP, trap or Sentinel          */
    SISR_REGISTRY_BAD_SOURCE_ID,        /* serial zero or wider than 24 bits*/
    SISR_REGISTRY_BAD_SOURCE_TYPE,      /* authority outside the enumeration*/
    SISR_REGISTRY_BAD_FLAGS,            /* reserved or contradictory bits   */
    SISR_REGISTRY_BAD_RESERVED,         /* reserved bytes not zero          */
    SISR_REGISTRY_DUPLICATE_SOURCE_ID,  /* same authority and serial        */
    SISR_REGISTRY_HASH_MISMATCH,        /* hash != canonical token hash     */
    SISR_REGISTRY_BAD_LINKAGE           /* self-link or unflagged link      */
} sisr_registry_error_t;

/* ── The static source record (§3.1) ──────────────────────────────── */
/* Six fields, 16 bytes, no padding on any supported ABI. The seeding sketch
 * supplied `codepoint`, `source_id_hash`, `registry_flags`, `source_type` and
 * `reserved`; SISR adds `source_serial` because a 32-bit hash is an integrity
 * check, not an identity. The serial is the normative half of the identity;
 * the hash lets a caller compare a full external token without carrying it. */
typedef struct {
    uint32_t codepoint;      /* Base SUCS codepoint                   */
    uint32_t source_serial;  /* authority-local serial, 1..0xFFFFFF   */
    uint32_t source_id_hash; /* FNV-1a of the canonical token (§3.2)  */
    uint16_t registry_flags; /* SISR_SOURCE_* bitfield                */
    uint8_t  source_type;    /* suas_sisr_source_type_t               */
    uint8_t  reserved;       /* must be zero                           */
} sisr_source_entry_t;

/* ── The static variant link (§3.3) ───────────────────────────────── */
/* A separate table, for the same reason SIP's variant target is a separate
 * field: source identity and glyph preference are different questions. The
 * link table is one-to-one (at most one fallback per codepoint) but many
 * codepoints may share a fallback. */
typedef struct {
    uint32_t codepoint;        /* the source form                    */
    uint32_t fallback_codepoint; /* the canonical glyph to render    */
    uint16_t link_flags;       /* SISR_LINK_* bitfield               */
    uint16_t reserved;         /* must be zero                       */
} sisr_variant_link_t;

/* ── Setup ────────────────────────────────────────────────────────── */
const char* suas_sisr_version_string(void);

/* Initializes both static indexes. Idempotent and allocation-free. Every query
 * builds the indexes lazily on first use if this was not called first. */
void suas_sisr_init_registry(void);

/* ── Core query (§3.4) ────────────────────────────────────────────── */
/* O(1) source lookup. On success `*out_entry` receives a copy of the packed
 * record and the function returns true. On failure — an inadmissible zone, an
 * unregistered codepoint, or a NULL output — it returns false and leaves the
 * output untouched. */
bool suas_sisr_get_source_entry(uint32_t codepoint,
                                sisr_source_entry_t* out_entry);

/* True when a codepoint has a source record (O(1)). */
bool suas_sisr_is_registered(uint32_t codepoint);

/* ── Enumeration ──────────────────────────────────────────────────── */
uint32_t suas_sisr_entry_count(void);

/* Copy the i-th registry record (0-based, ascending codepoint order). Returns
 * false out of range or on a NULL output (O(1)). */
bool suas_sisr_entry_at(uint32_t index, sisr_source_entry_t* out_entry);

/* ── Variant and fallback linkage (§6) ────────────────────────────── */
/* The link record for a codepoint, or false when the codepoint has none. */
bool suas_sisr_get_variant_link(uint32_t codepoint,
                                sisr_variant_link_t* out_link);

/* Resolve only the fallback target. Returns false when there is no link. */
bool suas_sisr_fallback_target(uint32_t codepoint, uint32_t* out_codepoint);

uint32_t suas_sisr_link_count(void);

bool suas_sisr_link_at(uint32_t index, sisr_variant_link_t* out_link);

/* ── Canonical source tokens (§3.2) ───────────────────────────────── */
/* FNV-1a 32-bit over the little-endian four bytes of the codepoint. This is
 * the index key, not the source identity. */
uint32_t suas_sisr_hash_codepoint(uint32_t codepoint);

/* FNV-1a 32-bit over a canonical token. A NULL pointer or a zero length
 * returns zero, which no canonical token produces. */
uint32_t suas_sisr_hash_source_token(const char* token, size_t length);

/* Write the canonical token (`PREFIX-` followed by at least five decimal
 * digits, widening for serials above 99999) and NUL-terminate it. Returns false
 * on an invalid record, a token type outside the enumeration, or a buffer too
 * small for the whole token. The record's hash is not consulted. */
bool suas_sisr_format_source_id(const sisr_source_entry_t* entry,
                                char* buffer, size_t capacity);

/* True when `token` is the canonical token of `entry`. This is the comparison
 * the stored `source_id_hash` exists to make cheap; a caller that holds the
 * text can verify it byte-for-byte against the formatted form. */
bool suas_sisr_source_id_matches(const sisr_source_entry_t* entry,
                                 const char* token, size_t length);

/* ── Zone and classification predicates (§5) ──────────────────────── */
/* Classifies a 31-bit codepoint into its address-space zone (O(1)). A value
 * above SUAS_CODEPOINT_MAX is EXTSUCS. */
suas_sisr_zone_t suas_sisr_zone_of(uint32_t codepoint);

/* True for the zones a source identity may name: Bridge, RNUR and Native.
 * False for the Control Plane, the trap slots, the Sentinel and ExtSUCS. */
bool suas_sisr_is_source_space(uint32_t codepoint);

/* True inside the RNUR neography carve-out (O(1)). */
bool suas_sisr_is_neography(uint32_t codepoint);

/* Set number of an RNUR coordinate: (cp - 0x00160000) >> 8. Returns false
 * outside the carve-out. */
bool suas_sisr_neography_set(uint32_t codepoint, uint32_t* out_set);

/* ── Taxonomy names ───────────────────────────────────────────────── */
const char* suas_sisr_source_type_name(suas_sisr_source_type_t type);

/* Parse a source type by name. Accepts the canonical name and, for CONLANG,
 * the short prefix "CONL". A NULL pointer, an empty name or an unknown name
 * returns SISR_SOURCE_TYPE_INVALID. */
suas_sisr_source_type_t suas_sisr_source_type_by_name(const char* name,
                                                      size_t length);

/* The identifier prefix of a source type ("UTC", "UCI", "UK", "RNUR", "CONL",
 * "CUST"), or NULL for an invalid type. */
const char* suas_sisr_source_type_prefix(suas_sisr_source_type_t type);

/* The name of the lowest assigned bit in a link mask, "NONE" for zero, or
 * "UNKNOWN" when the mask carries an unassigned bit. */
const char* suas_sisr_link_name(uint16_t link_flags);

const char* suas_sisr_registry_error_name(sisr_registry_error_t error);

/* ── Verification (§9) ────────────────────────────────────────────── */
/* Verify a compiled source table. `entries` may be NULL only when `count` is
 * zero (an empty registry is conformant). On failure, `*out_error` receives
 * the first defect and the function returns false; on success `*out_error` is
 * SISR_REGISTRY_OK. Either output pointer may be NULL. */
bool suas_sisr_validate_table(const sisr_source_entry_t* entries,
                              uint32_t count,
                              sisr_registry_error_t* out_error);

/* Verify a compiled link table under the same contract. */
bool suas_sisr_validate_links(const sisr_variant_link_t* links,
                              uint32_t count,
                              sisr_registry_error_t* out_error);

#ifdef __cplusplus
}
#endif

#endif /* SUAS_SUAS_SISR_H */
