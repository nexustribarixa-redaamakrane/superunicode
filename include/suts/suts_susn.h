#ifndef SUTS_SUTS_SUSN_H
#define SUTS_SUTS_SUSN_H

/* SUTS-011 - SuperUnicode Set Notation (SUSN)
 *
 * A modular data specification within the OpenWindows microkernel ecosystem.
 * SUSN establishes the binary-mapped character-set algebra, the range-filtering
 * masks, and the O(1) membership query required to reason about sets of
 * SuperUnicode codepoints without parsing set notation at run time.
 *
 * Where a traditional Unicode implementation (such as UTS #61, "Unicode Set
 * Notation") reads a bracketed expression such as [A-Z]-[C] as text and
 * evaluates it with a parser and a mutable set object, SUSN compiles every
 * set into an immutable, sorted, non-overlapping array of codepoint ranges.
 * Union, intersection, difference, and complement are resolved once - at
 * build time or at image-construction time - into those arrays, so a kernel
 * consumer asks a single question, "does this set contain this codepoint?",
 * and receives an answer from a bounded probe.
 *
 * Three commitments separate SUSN from a text set-notation parser:
 *
 *   1. BINARY-MAPPED SET ALGEBRA. A set is a `susn_set_descriptor_t` and a
 *      flat `susn_codepoint_range_t` array. There is no expression tree, no
 *      token stream, and no run-time string parser on any code path.
 *   2. INVERSION LISTS. The `SUSN_FLAG_NEGATED` bit turns the same range array
 *      into an inversion list: the ranges name the members of the set, and
 *      the flag says that membership is the complement of that list over the
 *      31-bit universe. Complement, union, intersection, difference, and
 *      symmetric difference all accept negated operands directly.
 *   3. ZERO ALLOCATION, O(1) MEMBERSHIP. Every table is static, every builder
 *      is a fixed-size struct the caller owns, and every query is a bounded
 *      binary search whose depth is a compile-time constant of the image.
 *      No path reaches for malloc, stdio, or a libc string routine.
 *
 * The range filter masks (`SUSN_FILTER_*`) let a consumer restrict a set query
 * to an address-space zone - the Unicode Compatibility Bridge, the System
 * Control Plane, the RNUR neography carve-out, the Native plane, the trap
 * slots, or the Sentinel - without constructing a second set.
 *
 * Neography and conscript integration: the Native & Neography Plane
 * (0x00160000-0x0016FFFF) is addressable as 256 set-numbered blocks of 256
 * codepoints each. `suts_susn_neography_set` materializes one block as a
 * one-range descriptor in caller-owned storage, so a conscript registry can
 * compose a neography set with the same algebra it uses for Bridge sets.
 *
 * SUSN is a modular SUTS data specification: a minimal kernel may omit it and
 * still conform to the SuperUnicode architecture. A text editor, font
 * fallback, identifier validator, or permission policy opts in to obtain
 * predictable set membership without a heap.
 *
 * Freestanding C99, zero allocation.
 *
 * See docs/suts/SUTS-011-susn.md for the normative specification.
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Specification identifiers ─────────────────────────────────────── */
#define SUSN_SPEC_ID           "SUTS-011"
#define SUSN_SPEC_TITLE        "SuperUnicode Set Notation"
#define SUSN_VERSION_MAJOR     1
#define SUSN_VERSION_MINOR     0
#define SUSN_MODULE            "suts_susn"

/* ── Set descriptor flags ──────────────────────────────────────────── */
/* Bits 0-2 are the seeding sketch's flags and keep their positions. The
 * remaining bits are derived metadata the engine never needs in order to
 * answer a membership query. */
#define SUSN_FLAG_NEGATED        (1U << 0) /* inversion list: complement   */
#define SUSN_FLAG_PROPERTY_BASED (1U << 1) /* derived from a UCD property  */
#define SUSN_FLAG_RANGE_ARRAY    (1U << 2) /* stored as a range array      */
#define SUSN_FLAG_NEOSCRIPT      (1U << 3) /* RNUR / conscript block       */
#define SUSN_FLAG_COMPILED       (1U << 4) /* resolved by build-time tooling */
#define SUSN_FLAG_MASK_DEFINED   0x1Fu     /* every bit defined above      */

/* ── Address-space zones (§4) ──────────────────────────────────────── */
typedef enum {
    SUSN_ZONE_BRIDGE   = 0, /* 0x00000000-0x0010FFFF           */
    SUSN_ZONE_SCP      = 1, /* 0x00110000-0x0011FFFF           */
    SUSN_ZONE_RNUR     = 2, /* 0x00160000-0x0016FFFF           */
    SUSN_ZONE_NATIVE   = 3, /* rest of 0x00120000-0x7FFFFFEF   */
    SUSN_ZONE_TRAP     = 4, /* 0x7FFFFFF0-0x7FFFFFFE           */
    SUSN_ZONE_SENTINEL = 5, /* 0x7FFFFFFF                      */
    SUSN_ZONE_EXTSUCS  = 6  /* > 0x7FFFFFFF (plugin space)     */
} susn_zone_t;

/* ── Address-space filter masks (§4.2) ─────────────────────────────── */
/* A query may be restricted to any combination of zones. `SUSN_FILTER_ALL`
 * covers every zone that a 31-bit codepoint can inhabit; ExtSUCS has no bit
 * because a 31-bit set can never contain a codepoint above the Sentinel. */
#define SUSN_FILTER_BRIDGE   (1U << 0)
#define SUSN_FILTER_SCP      (1U << 1)
#define SUSN_FILTER_RNUR     (1U << 2)
#define SUSN_FILTER_NATIVE   (1U << 3)
#define SUSN_FILTER_TRAP     (1U << 4)
#define SUSN_FILTER_SENTINEL (1U << 5)
#define SUSN_FILTER_ALL      0x3Fu

/* ── Set algebra operators (§3) ────────────────────────────────────── */
typedef enum {
    SUSN_OP_UNION                = 0,
    SUSN_OP_INTERSECTION         = 1,
    SUSN_OP_DIFFERENCE           = 2,
    SUSN_OP_SYMMETRIC_DIFFERENCE = 3
} susn_set_op_t;

/* ── Built-in catalog identifiers (§6) ─────────────────────────────── */
/* The catalog is a curated, compiled set of named sets. A set identifier is
 * an opaque 32-bit key: values 1-25 name catalog entries, zero names the
 * empty set, and values at or above `SUSN_SETID_NEOSCRIPT_BASE` name RNUR
 * neography blocks. */
typedef enum {
    SUSN_SETID_NONE            = 0,
    SUSN_SETID_ASCII           = 1,
    SUSN_SETID_PRINTABLE_ASCII = 2,
    SUSN_SETID_DIGIT           = 3,
    SUSN_SETID_UPPER           = 4,
    SUSN_SETID_LOWER           = 5,
    SUSN_SETID_ASCII_PUNCT     = 6,
    SUSN_SETID_CONTROL         = 7,
    SUSN_SETID_WHITESPACE      = 8,
    SUSN_SETID_LATIN1          = 9,
    SUSN_SETID_GREEK           = 10,
    SUSN_SETID_CYRILLIC        = 11,
    SUSN_SETID_HEBREW          = 12,
    SUSN_SETID_ARABIC          = 13,
    SUSN_SETID_HIRAGANA        = 14,
    SUSN_SETID_KATAKANA        = 15,
    SUSN_SETID_HAN             = 16,
    SUSN_SETID_HANGUL          = 17,
    SUSN_SETID_EMOJI           = 18,
    SUSN_SETID_BIDI_CONTROL    = 19,
    SUSN_SETID_BRIDGE          = 20,
    SUSN_SETID_SCP             = 21,
    SUSN_SETID_RNUR            = 22,
    SUSN_SETID_NATIVE          = 23,
    SUSN_SETID_TRAP            = 24,
    SUSN_SETID_SENTINEL        = 25,
    SUSN_SETID_NEOSCRIPT_BASE  = 0x00160000
} susn_builtin_set_t;

/* ── Fixed limits ──────────────────────────────────────────────────── */
#define SUSN_UNIVERSE_MIN      0x00000000UL
#define SUSN_UNIVERSE_MAX      0x7FFFFFFFUL
#define SUSN_UNIVERSE_SIZE     0x80000000ULL
#define SUSN_RANGE_CAPACITY    4096U
#define SUSN_BUILDER_CAPACITY  256U
#define SUSN_CATALOG_CAPACITY  64U

/* The RNUR neography carve-out inside the Native plane (SUAS-005). */
#define SUSN_RNUR_MIN          0x00160000UL
#define SUSN_RNUR_MAX          0x0016FFFFUL
#define SUSN_RNUR_SET_SHIFT    8U
#define SUSN_RNUR_SET_SIZE     256U
#define SUSN_RNUR_SET_COUNT    256U

/* ── Structural types (zero-allocation) ────────────────────────────── */
/* One inclusive codepoint range. A canonical range array is sorted ascending
 * by `first`, and consecutive ranges are separated by at least one codepoint
 * that is not a member (two adjacent ranges are merged). */
typedef struct {
    uint32_t first;            /* first codepoint of the range        */
    uint32_t last;             /* last codepoint, inclusive           */
} susn_codepoint_range_t;

/* A compiled set. The four fields are the seeding sketch's layout; a consumer
 * written against the sketch compiles unchanged.
 *
 *   set_id       opaque identifier (catalog key, or 0 for an anonymous set)
 *   set_flags    SUSN_FLAG_* metadata; SUSN_FLAG_NEGATED selects inversion
 *   range_count  number of entries in `ranges`
 *   ranges       caller- or image-owned canonical range array
 *
 * A descriptor never owns its range array and never frees it. */
typedef struct {
    uint32_t set_id;                        /* opaque set identifier   */
    uint32_t set_flags;                     /* SUSN_FLAG_* metadata    */
    uint32_t range_count;                   /* entries in `ranges`     */
    const susn_codepoint_range_t *ranges;   /* canonical range array   */
} susn_set_descriptor_t;

/* A fixed-capacity builder. The caller owns the struct - usually on its own
 * frame - and the descriptor produced by `suts_susn_builder_finish` aliases
 * the builder's array, so the builder must outlive the descriptor. */
typedef struct {
    uint32_t flags;                          /* SUSN_FLAG_* metadata   */
    uint32_t count;                          /* ranges in use          */
    susn_codepoint_range_t ranges[SUSN_BUILDER_CAPACITY];
} susn_builder_t;

/* ── Version and setup ─────────────────────────────────────────────── */
/* The version string, a NUL-terminated ASCII literal ("SUTS-011/1.0"). */
const char* suts_susn_version_string(void);

/* Initializes the static SUSN engine. Idempotent and allocation-free. Every
 * query works without it; the call exists so a consumer can force a
 * deterministic point of first use. */
void suts_susn_init_engine(void);

/* ── Membership (§3.3) ─────────────────────────────────────────────── */
/* Answers whether `codepoint` is a member of `set` in O(1) time. The query is
 * total over the 31-bit SuperUnicode address space: an unlisted codepoint is a
 * non-member, and a codepoint above the Sentinel is never a member. A NULL
 * descriptor is the empty set. The `SUSN_FLAG_NEGATED` bit inverts the answer
 * over the 31-bit universe, which is what makes an inversion list a set.
 *
 * The result is independent of `set_id` and of every metadata flag except
 * SUSN_FLAG_NEGATED. */
bool suts_susn_contains(const susn_set_descriptor_t *set, uint32_t codepoint);

/* The zone-restricted form of `suts_susn_contains`. A codepoint whose zone is
 * not selected by `filter_mask` is a non-member even when the set contains it.
 * `filter_mask == SUSN_FILTER_ALL` is exactly `suts_susn_contains`. */
bool suts_susn_contains_in(const susn_set_descriptor_t *set, uint32_t codepoint,
                           uint32_t filter_mask);

/* The number of codepoints the set contains. A negated set is counted over the
 * 31-bit universe, so the result is in [0, 0x80000000]. */
uint64_t suts_susn_cardinality(const susn_set_descriptor_t *set);

/* ── Descriptor validation and canonicalization (§3.1) ─────────────── */
/* True when the descriptor is structurally valid: defined flags only, a
 * non-NULL range array when `range_count > 0`, every range inside the 31-bit
 * universe with first <= last, and every range separated from its predecessor
 * by at least one non-member codepoint. */
bool suts_susn_validate(const susn_set_descriptor_t *set);

/* Copies `in_count` ranges into `out` and returns their canonical count:
 * sorted by `first`, with overlapping and adjacent ranges merged. The function
 * needs `capacity >= in_count`; it returns 0 when the input is invalid or the
 * output is too small. An empty input returns 0. */
uint32_t suts_susn_normalize(const susn_codepoint_range_t *in, uint32_t in_count,
                             susn_codepoint_range_t *out, uint32_t capacity);

/* Fills a descriptor. Returns false when `range_count > 0` and `ranges` is
 * NULL. The flags are stored as supplied; the engine masks them on validation,
 * not on construction, so a caller can record provenance bits it alone reads. */
bool suts_susn_descriptor_init(susn_set_descriptor_t *out, uint32_t set_id,
                               uint32_t flags, uint32_t range_count,
                               const susn_codepoint_range_t *ranges);

/* ── Set algebra (§3.2) ────────────────────────────────────────────── */
/* Resolves `a <op> b` into a canonical positive range array. Negated operands
 * are honored directly, so `SUSN_OP_INTERSECTION` of two inversion lists needs
 * no materialization step. A NULL operand is the empty set. The output is
 * always positive (SUSN_FLAG_NEGATED is never implied).
 *
 * The return value is the number of canonical ranges the complete result
 * needs, independent of `capacity` (the snprintf convention). Up to `capacity`
 * ranges are written to `out`. A return of 0 means either an empty result or
 * invalid arguments; use `suts_susn_validate` when the distinction matters. */
uint32_t suts_susn_combine(susn_set_op_t op,
                           const susn_set_descriptor_t *a,
                           const susn_set_descriptor_t *b,
                           susn_codepoint_range_t *out, uint32_t capacity);

/* Convenience wrappers over `suts_susn_combine`. */
uint32_t suts_susn_union(const susn_set_descriptor_t *a,
                         const susn_set_descriptor_t *b,
                         susn_codepoint_range_t *out, uint32_t capacity);
uint32_t suts_susn_intersection(const susn_set_descriptor_t *a,
                                const susn_set_descriptor_t *b,
                                susn_codepoint_range_t *out, uint32_t capacity);
uint32_t suts_susn_difference(const susn_set_descriptor_t *a,
                              const susn_set_descriptor_t *b,
                              susn_codepoint_range_t *out, uint32_t capacity);
uint32_t suts_susn_symmetric_difference(const susn_set_descriptor_t *a,
                                        const susn_set_descriptor_t *b,
                                        susn_codepoint_range_t *out,
                                        uint32_t capacity);

/* The complement of `set` over the 31-bit universe, as a canonical positive
 * range array. This is the inverse-list operation: the result contains exactly
 * the codepoints the set does not. A NULL set complements to the full
 * universe. The return value follows the `suts_susn_combine` convention. */
uint32_t suts_susn_complement(const susn_set_descriptor_t *set,
                              susn_codepoint_range_t *out, uint32_t capacity);

/* ── Range filtering and probe masks (§4.2) ────────────────────────── */
/* Restricts `set` to the zones selected by `zone_mask`, resolving any negated
 * operand, and writes the canonical positive result. The return value follows
 * the `suts_susn_combine` convention. */
uint32_t suts_susn_filter(const susn_set_descriptor_t *set, uint32_t zone_mask,
                          susn_codepoint_range_t *out, uint32_t capacity);

/* Builds a 32-bit occupancy mask over a regular probe: bit i is set when the
 * set contains `first + i * stride`, for i in [0, min(count, 32)). Probes that
 * would pass the Sentinel are not sampled. `stride == 0` probes `first` once
 * and returns bit 0. This is the range-filtering primitive a tokenizer uses to
 * reject a whole block of codepoints with one mask test. */
uint32_t suts_susn_probe_mask(const susn_set_descriptor_t *set, uint32_t first,
                              uint32_t stride, uint32_t count);

/* ── Set relations ─────────────────────────────────────────────────── */
bool suts_susn_is_empty(const susn_set_descriptor_t *set);
bool suts_susn_is_universe(const susn_set_descriptor_t *set);
bool suts_susn_is_subset(const susn_set_descriptor_t *a,
                         const susn_set_descriptor_t *b);
bool suts_susn_is_disjoint(const susn_set_descriptor_t *a,
                           const susn_set_descriptor_t *b);
bool suts_susn_equals(const susn_set_descriptor_t *a,
                      const susn_set_descriptor_t *b);

/* ── Zone and neography classification (§4, §5) ────────────────────── */
susn_zone_t suts_susn_zone_of(uint32_t codepoint);

/* The filter bit that selects `zone`. EXTSUCS has no bit and yields 0. */
uint32_t suts_susn_zone_filter(susn_zone_t zone);

/* True inside the RNUR neography carve-out (0x00160000-0x0016FFFF). */
bool suts_susn_is_neography(uint32_t codepoint);

/* Set number of an RNUR coordinate: (cp - 0x00160000) >> 8. Returns false
 * outside the carve-out. */
bool suts_susn_neography_set_of(uint32_t codepoint, uint32_t *out_set);

/* The inclusive bounds of neography set `set_number`. Returns false when the
 * number is outside [0, 255]. */
bool suts_susn_neography_bounds(uint32_t set_number, uint32_t *out_first,
                                uint32_t *out_last);

/* True when `codepoint` belongs to neography set `set_number`. */
bool suts_susn_neography_contains(uint32_t set_number, uint32_t codepoint);

/* Materializes neography set `set_number` as a one-range descriptor.
 * `storage` is caller-owned and must remain alive as long as the descriptor is
 * used. The descriptor carries SUSN_FLAG_RANGE_ARRAY | SUSN_FLAG_NEOSCRIPT and
 * the identifier `SUSN_SETID_NEOSCRIPT_BASE + set_number`. */
bool suts_susn_neography_set(uint32_t set_number,
                             susn_codepoint_range_t *storage,
                             susn_set_descriptor_t *out);

/* ── Builder (§3.4) ────────────────────────────────────────────────── */
/* Builders accumulate positive ranges with merging on every insertion. The
 * builder never allocates; it refuses an insertion that would overflow
 * SUSN_BUILDER_CAPACITY. */
void suts_susn_builder_init(susn_builder_t *builder, uint32_t flags);
bool suts_susn_builder_add_codepoint(susn_builder_t *builder, uint32_t codepoint);
bool suts_susn_builder_add_range(susn_builder_t *builder, uint32_t first,
                                 uint32_t last);
/* Adds the members of `set`, honoring SUSN_FLAG_NEGATED. */
bool suts_susn_builder_add_set(susn_builder_t *builder,
                               const susn_set_descriptor_t *set);
/* Publishes the builder's array as a descriptor. The builder must outlive the
 * descriptor; `ranges` aliases `builder->ranges`. */
bool suts_susn_builder_finish(susn_builder_t *builder, uint32_t set_id,
                              susn_set_descriptor_t *out);

/* ── Catalog (§6) ──────────────────────────────────────────────────── */
uint32_t suts_susn_catalog_count(void);
const susn_set_descriptor_t *suts_susn_catalog_at(uint32_t index);
const susn_set_descriptor_t *suts_susn_catalog_find(uint32_t set_id);
const susn_set_descriptor_t *suts_susn_catalog_find_name(const char *name);

/* ── Taxonomy names ────────────────────────────────────────────────── */
const char* suts_susn_op_name(susn_set_op_t op);
const char* suts_susn_zone_name(susn_zone_t zone);
const char* suts_susn_set_name(uint32_t set_id);

#ifdef __cplusplus
}
#endif

#endif /* SUTS_SUTS_SUSN_H */
