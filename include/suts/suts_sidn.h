#ifndef SUTS_SUTS_SIDN_H
#define SUTS_SUTS_SIDN_H

/* SUTS-006 — System Internationalized Domain & Identifier Network Architecture
 * (SIDN)
 *
 * The core OpenWindows kernel primitive governing internationalized network
 * identifiers: domain labels, IPC channel names, and domain routes expressed
 * over SuperUnicode string streams. SIDN maps a 31-bit codepoint to a compiled
 * IDNA mapping Status and, from a whole label stream, to a canonical binary
 * routing form — with no heap allocation and with no text-based Punycode
 * (ACE) conversion.
 *
 * Where UTS #46, "Unicode IDNA Compatibility Processing", describes a
 * multi-step text algorithm — Map, Normalize to NFC, Break at a label
 * separator, Convert (Punycode / RFC 3492), Validate — over dynamically
 * allocated strings, SIDN:
 *
 *   - discards the text-based Punycode `xn--` ASCII encoding loop entirely, in
 *     favour of direct 31-bit SuperUnicode stream routing: a network endpoint,
 *     a kernel IPC channel and a domain route name raw SUCS symbols, and there
 *     is no text-to-code conversion step at all;
 *   - obeys the zero-allocation mandate: a label is evaluated in a single
 *     linear pass against a compiled, immutable mapping table and a fixed-size
 *     open-addressing index, and no query path touches a heap;
 *   - is a kernel primitive: the microkernel, the shell parser and the network
 *     routing path decide identifier admissibility in-band, with no external
 *     file system, user process or service dependency.
 *
 * SIDN is a dual-layer system, in the same relationship SUAS-009 (SIP),
 * SUAS-012 (SUCD) and SUTS-005 (SCSS) have with their Unicode counterparts.
 * The offline reference layer is the SuperUnicode Character Database (SUCA /
 * SUCD) — the human-readable IDNA mapping tables, label restriction rules and
 * domain profile definitions that network compilers, user-space browsers and
 * developer toolchains read. Build-time tooling compiles those datasets into
 * the immutable, byte-aligned binary routing tables this module embeds in the
 * microkernel image, so the kernel never reads a file at run time.
 *
 * Three commitments separate SIDN from a text IDNA library:
 *
 *   1. COMPILED STATUS. Every codepoint resolves to one of five Status values
 *      — valid / ignored / mapped / deviation / disallowed — as a fixed-size
 *      record, not by tokenizing a semicolon-delimited data line (§2).
 *   2. A BOUNDED LOOKUP. A mapping query hashes the codepoint, probes a fixed
 *      index, and returns the record — O(1) expected, no allocation, no libc.
 *   3. A SINGLE LINEAR PASS. A label is scanned once; filtering, deviation
 *      handling, separator folding and the validity criteria all fall out of
 *      the same walk, and the canonical routing form is produced without
 *      re-entering the string (§4).
 *
 * SIDN natively integrates the Native & Neography Plane
 * (0x00160000-0x0016FFFF) and custom conscript registries such as the Reddit
 * Neographical Unicode Registry (RNUR), so a user-space operating system on
 * OpenWindows can operate a fully internationalized or conlang-based network
 * architecture.
 *
 * SUTS-006 is registered as an optional data specification. The engine, the
 * record layouts and the processing contract are architectural; the mapping
 * *contents* are version data, compiled from the SUCD manifests (§10).
 *
 * Freestanding C99, zero allocation: the tables are static, every descriptor
 * is a fixed-size struct the caller owns, and no part of the engine reaches
 * for malloc, stdio or libc string routines.
 *
 * See docs/suts/SUTS-006-sidn.md for the normative specification.
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Specification identifiers ────────────────────────────────────── */
#define SIDN_SPEC_ID             "SUTS-006"
#define SIDN_SPEC_TITLE          "System Internationalized Domain & Identifier Network Architecture"
#define SIDN_VERSION_MAJOR       1
#define SIDN_VERSION_MINOR       0

/* ── Fixed limits (static, stack-allocatable tables) ──────────────── */
/* The compiled mapping table holds at most this many curated records. The index
 * is a power of two larger than the capacity, so the load factor stays below
 * one-half and the probe sequence stays short and bounded. */
#define SIDN_MAPPING_CAPACITY    128U
#define SIDN_MAPPING_INDEX_SIZE  256U

/* Longest label a network identifier may carry, in codepoints, and the DNS
 * length limits of [STD13] in octets / codepoints (§4.3). */
#define SIDN_LABEL_MAX           64U
#define SIDN_LABEL_MAX_DNS       63U
#define SIDN_DOMAIN_MAX_DNS      253U

/* FNV-1a constants for the mapping key hash (§2.4). */
#define SIDN_FNV_OFFSET_BASIS    2166136261u
#define SIDN_FNV_PRIME           16777619u

/* ── Label validation flags (§2.5) ────────────────────────────────── */
/* The sketch called these SIDN_FLAG_*; they are preserved verbatim so a
 * consumer written against the seeding sketch compiles unchanged. */
#define SIDN_FLAG_VALID_LABEL      (1U << 0) /* Safe for network routing       */
#define SIDN_FLAG_TRANSITIONAL     (1U << 1) /* Requires compatibility mapping */
#define SIDN_FLAG_DISALLOWED       (1U << 2) /* Contains prohibited codepoints */
#define SIDN_FLAG_NEOGRAPHY_DOMAIN (1U << 3) /* Uses RNUR or custom script     */

/* ── Mapping Status taxonomy (§2.1, transcribed from UTS #46 §5) ──── */
typedef enum {
    SIDN_STATUS_VALID      = 0, /* kept unchanged; admissible               */
    SIDN_STATUS_IGNORED    = 1, /* removed (mapped to the empty string)     */
    SIDN_STATUS_MAPPED     = 2, /* replaced by the record's mapping value   */
    SIDN_STATUS_DEVIATION  = 3, /* mapped/valid depending on the processing */
    SIDN_STATUS_DISALLOWED = 4  /* not allowed                              */
} sidn_status_t;

/* ── Processing mode (§3, the UTS #46 C1 / C2 / C3 clauses) ───────── */
typedef enum {
    SIDN_MODE_NONTRANSITIONAL = 0, /* C2; the default and only normative mode */
    SIDN_MODE_TRANSITIONAL    = 1  /* C1 (deprecated)                          */
} sidn_mode_t;

/* ── Address-space zones (§5) ─────────────────────────────────────── */
/* SIDN does not invent a second plane taxonomy. The zones below are the
 * SUCD / SSP planes with the RNUR carve-out made explicit, because a neography
 * label lives there. */
typedef enum {
    SIDN_ZONE_BRIDGE   = 0, /* 0x00000000-0x0010FFFF           */
    SIDN_ZONE_SCP      = 1, /* 0x00110000-0x0011FFFF           */
    SIDN_ZONE_RNUR     = 2, /* 0x00160000-0x0016FFFF           */
    SIDN_ZONE_NATIVE   = 3, /* rest of 0x00120000-0x7FFFFFEF   */
    SIDN_ZONE_TRAP     = 4, /* 0x7FFFFFF0-0x7FFFFFFE           */
    SIDN_ZONE_SENTINEL = 5, /* 0x7FFFFFFF                      */
    SIDN_ZONE_EXTSUCS  = 6  /* > 0x7FFFFFFF (plugin space)     */
} sidn_zone_t;

/* ── RNUR / RCCR neography carve-out (SUAS-005 §5.4) ──────────────── */
#define SIDN_RNUR_MIN          0x00160000UL
#define SIDN_RNUR_MAX          0x0016FFFFUL
#define SIDN_RNUR_SET_SHIFT    8U

/* ── Base SUCS zone boundaries (duplicated for freestanding SIDN) ─── */
#ifndef SIDN_LIMIT_UNICODE_MAX
#define SIDN_LIMIT_UNICODE_MAX 0x0010FFFFUL
#define SIDN_LIMIT_SCP_MIN     0x00110000UL
#define SIDN_LIMIT_SCP_MAX     0x0011FFFFUL
#define SIDN_LIMIT_NATIVE_MIN  0x00120000UL
#define SIDN_LIMIT_NATIVE_MAX  0x7FFFFFEFUL
#define SIDN_LIMIT_TRAP_MIN    0x7FFFFFF0UL
#define SIDN_LIMIT_TRAP_MAX    0x7FFFFFFEUL
#define SIDN_LIMIT_SENTINEL    0x7FFFFFFFUL
#endif

/* ── Processing outcomes (§4) ─────────────────────────────────────── */
/* One closed enumeration naming every defect a label can have. SIDN_OK is the
 * only success value. */
typedef enum {
    SIDN_OK = 0,
    SIDN_ERR_NULL_ARGUMENT,       /* a NULL stream or descriptor           */
    SIDN_ERR_OUTPUT_TOO_SMALL,    /* output buffer cannot hold the result  */
    SIDN_ERR_LABEL_EMPTY,         /* the label is empty                    */
    SIDN_ERR_DISALLOWED,          /* a codepoint with Status disallowed    */
    SIDN_ERR_TRANSITIONAL_ONLY,   /* a deviation in a transitional mode    */
    SIDN_ERR_LEADING_COMBINING,   /* the label begins with a mark          */
    SIDN_ERR_INTERNAL_STOP,       /* a U+002E survived into a label        */
    SIDN_ERR_HYPHEN_POSITION,     /* a hyphen in the third and fourth slot */
    SIDN_ERR_EDGE_HYPHEN,         /* the label begins or ends with hyphen  */
    SIDN_ERR_RESERVED_PREFIX,     /* the label begins with the reserved pre */
    SIDN_ERR_STD3_ASCII,          /* a non-[a-z0-9-] ASCII codepoint       */
    SIDN_ERR_CONTEXTJ,            /* a joiner in an inadmissible context   */
    SIDN_ERR_BIDI,                /* a Bidi domain rule is violated        */
    SIDN_ERR_LABEL_TOO_LONG,      /* longer than the DNS label limit       */
    SIDN_ERR_NO_MAPPING           /* a codepoint carries no mapping record */
} sidn_status_code_t;

/* ── The compiled mapping record (§2.2) ───────────────────────────── */
/* One fixed-size record per curated codepoint. `mapping` is the replacement
 * for a mapped / deviation codepoint: a single codepoint for the vast majority
 * of cases, with `mapping_length` giving the length (0 for ignored and
 * disallowed, 2 for the ß -> "ss" transitional mapping). The sketch's
 * `status` / `mapping` pair is preserved; the extra fields are what make the
 * record self-describing and byte-alignable. */
typedef struct {
    uint32_t codepoint;      /* Base SUCS codepoint                    */
    uint32_t mapping[2];     /* replacement, up to two codepoints      */
    uint8_t  status;         /* sidn_status_t                          */
    uint8_t  mapping_length; /* 0, 1 or 2                              */
    uint16_t reserved;       /* must be zero                           */
} sidn_mapping_entry_t;

/* ── The label descriptor the sketch defines (§2.3) ───────────────── */
typedef struct {
    uint32_t label_hash;     /* Precomputed hash of normalized label stream */
    uint16_t length;         /* Codepoint count                            */
    uint16_t flags;          /* SIDN_FLAG_* bitfield                       */
} sidn_label_descriptor_t;

/* ── Setup ────────────────────────────────────────────────────────── */
const char* suts_sidn_version_string(void);

/* Initializes the static SIDN mapping index. Idempotent and allocation-free;
 * every query builds the index lazily on first use if this was not called. */
void suts_sidn_init_engine(void);

/* ── Mapping query (§2.4) ─────────────────────────────────────────── */
/* O(1) mapping lookup. On success `*out_entry` receives a copy of the packed
 * record and the function returns true. On failure — an inadmissible zone, an
 * uncurated codepoint, or a NULL output — it returns false and leaves the
 * output untouched. */
bool suts_sidn_mapping_of(uint32_t codepoint, sidn_mapping_entry_t* out_entry);

/* The Status of a codepoint. A codepoint with no curated record resolves to
 * SIDN_STATUS_DISALLOWED, so the mapping function is total over the space. */
sidn_status_t suts_sidn_status_of(uint32_t codepoint);

/* True when the codepoint may appear (directly) in a nontransitional label:
 * its Status is valid or deviation (§4.2). */
bool suts_sidn_is_admissible(uint32_t codepoint);

/* ── Label validation and canonical routing (§4) ──────────────────── */
/* Evaluates and validates an internationalized domain label stream in a single
 * linear pass, without heap allocation. `label_stream` holds `length`
 * codepoints; a label separator (any of the four label-separator codepoints,
 * §3.1) terminates processing early.

 * On success `*out_desc` receives the label descriptor (hash, codepoint count,
 * flags) and the function returns true. On failure it returns false, `*out_desc`
 * still receives the best-effort descriptor with SIDN_FLAG_DISALLOWED set, and
 * `*out_error` (when not NULL) receives the first defect. */
bool suts_sidn_validate_label(const uint32_t* label_stream, size_t length,
                              sidn_label_descriptor_t* out_desc,
                              sidn_status_code_t* out_error);

/* Resolves a network label to its canonical binary routing form: the mapped,
 * NFC-equivalent, single-label SUCS stream. Every label separator in the input
 * is folded to U+002E, and a surrounding single U+002E is treated as the label
 * boundary. On success `*output_len` receives the number of codepoints written
 * and the function returns true. On failure it returns false, `*output_len`
 * (when not NULL) receives the codepoints that were written before the defect,
 * and `*out_error` (when not NULL) receives the first defect. */
bool suts_sidn_to_canonical(const uint32_t* input_stream, size_t input_len,
                            uint32_t* output_stream, size_t* output_len,
                            sidn_status_code_t* out_error);

/* ── Validity criteria (§4.2, UTS #46 §4.1) ───────────────────────── */
/* Each predicate is O(1) once the label descriptor is known, and total over
 * any input. */
bool suts_sidn_check_hyphens(const uint32_t* label, size_t length);
bool suts_sidn_check_std3(const uint32_t* label, size_t length);
bool suts_sidn_check_joiners(const uint32_t* label, size_t length);
bool suts_sidn_check_bidi(const uint32_t* label, size_t length);
bool suts_sidn_is_bidi_domain(const uint32_t* label, size_t length);

/* ── Label separators (§3.1) ──────────────────────────────────────── */
/* True for the four label-separator codepoints, all of which fold to U+002E:
 * U+002E, U+FF0E, U+3002, U+FF61. */
bool suts_sidn_is_label_separator(uint32_t codepoint);

/* The canonical separator, U+002E. */
#define SIDN_LABEL_SEPARATOR 0x002EUL

/* The reserved prefix that CheckHyphens protects (UTS #46 validity criterion
 * 4): the three codepoints "xn-". */
#define SIDN_RESERVED_PREFIX_0 0x0078UL /* 'x' */
#define SIDN_RESERVED_PREFIX_1 0x006EUL /* 'n' */
#define SIDN_RESERVED_PREFIX_2 0x002DUL /* '-' */

bool suts_sidn_has_reserved_prefix(const uint32_t* label, size_t length);

/* ── Zone and classification predicates (§5) ──────────────────────── */
/* Classifies a 31-bit codepoint into its address-space zone (O(1)). A value
 * above the Sentinel is EXTSUCS. */
sidn_zone_t suts_sidn_zone_of(uint32_t codepoint);

/* True for the zones a network identifier may name: Bridge, RNUR and Native.
 * False for the Control Plane, the trap slots, the Sentinel and ExtSUCS. */
bool suts_sidn_is_routable(uint32_t codepoint);

/* True inside the RNUR neography carve-out (O(1)). */
bool suts_sidn_is_neography(uint32_t codepoint);

/* Set number of an RNUR coordinate: (cp - 0x00160000) >> 8. Returns false
 * outside the carve-out. */
bool suts_sidn_neography_set(uint32_t codepoint, uint32_t* out_set);

/* ── Hash and enumeration (§2.4) ──────────────────────────────────── */
/* FNV-1a 32-bit over the little-endian four bytes of the codepoint. This is the
 * mapping index key, not a labelled identity. */
uint32_t suts_sidn_hash_codepoint(uint32_t codepoint);

/* FNV-1a 32-bit over a label stream. A NULL pointer or a zero length returns
 * zero, which the label hash never produces for a non-empty label. */
uint32_t suts_sidn_hash_label(const uint32_t* label, size_t length);

uint32_t suts_sidn_mapping_count(void);
bool suts_sidn_mapping_at(uint32_t index, sidn_mapping_entry_t* out_entry);

/* ── Taxonomy names ───────────────────────────────────────────────── */
const char* suts_sidn_status_name(sidn_status_t status);
const char* suts_sidn_zone_name(sidn_zone_t zone);
const char* suts_sidn_status_code_name(sidn_status_code_t code);

#ifdef __cplusplus
}
#endif

#endif /* SUTS_SUTS_SIDN_H */
