#ifndef SUAS_SUAS_SUCD_H
#define SUAS_SUAS_SUCD_H

/* SUAS-012 — SuperUnicode Character Database (SUCD)
 *
 * The canonical architecture, structural taxonomy, and zero-allocation binary
 * memory representation of the SuperUnicode Character Database itself: the
 * normative counterpart of Unicode's UAX #44, "Unicode Character Database"
 * annex.
 *
 * UAX #44 describes the UCD as a *collection of data files*. That framing is
 * accurate, and it is also the problem. A database defined as a folder of text
 * files has no architecture: no single statement of what a record is, no
 * guarantee that two files agree, no declared relationship between the human
 * form and the machine form, and no bound on what a query costs. An
 * implementer reconciles UnicodeData.txt against Blocks.txt against
 * DerivedCoreProperties.txt by hand, and every reconciliation is a chance to
 * diverge from every other implementation.
 *
 * SUCD closes that with three commitments UAX #44 never makes:
 *
 *   1. ONE binary image. The offline text manifests and the runtime table are
 *      two forms of one artefact, related by a specified compiler step, not
 *      two things an implementer keeps in sync by discipline.
 *   2. A BOUNDED lookup. A query is a directory load, at most one index load,
 *      and one bounds compare — O(1), no allocation, no search, no libc.
 *   3. A PARTITION. The compiled records are ordered and disjoint. UCD-style
 *      files are free to overlap and nest (BidiProps.txt places 0660..0669
 *      inside 0600..06FF and repeats 0041-005A twice; SUCSData.txt swallows
 *      the trap slots inside the Native row); an image may not.
 *
 * Dual-layer architecture, as with every other kernel primitive:
 *
 *   - the offline reference layer is the published /Public tree: sucd/ with
 *     its Props/, Derived/ and auxiliary/ subdirectories, and the sibling
 *     sudat/, names/, sources/ and partitions/ trees the database cites;
 *   - the runtime kernel layer is this header: the property taxonomy, the
 *     two-level page directory, and the bounded query surface.
 *
 * The relationship between the two is the whole point. The text layer is
 * allowed to be hierarchical, redundant, overlapping and convenient, because
 * the compiler resolves it into a flat partition. The binary layer assumes
 * nothing but the partition, because the compiler proved it.
 *
 * SUCD is deliberately permissive where SCMS is not. SUAS-011 (SCMS) refuses a
 * manifest that names a Control-Plane codepoint, a trap slot or the Sentinel,
 * because a manifest allocates and those are not allocations. SUCD *models*
 * them: the image carries records for the control plane, the trap range and
 * the Sentinel so a query against them answers rather than silently defaulting.
 * An admissible record and an allocated record are different questions, and
 * SUCD answers both.
 *
 * Freestanding C99, zero allocation: the image is caller-supplied and mapped,
 * every descriptor is a fixed-size struct the caller owns, and no part of the
 * engine reaches for malloc, stdio, or libc string routines.
 *
 * See docs/suas/SUAS-012-sucd.md for the normative specification.
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "suas/suas_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ── Specification identifiers ───────────────────────────────────── */
#define SUAS_SUCD_SPEC_ID        "SUAS-012"
#define SUAS_SUCD_SPEC_TITLE     "SuperUnicode Character Database"
#define SUAS_SUCD_VERSION_MAJOR  1
#define SUAS_SUCD_VERSION_MINOR  0
#define SUAS_SUCD_VERSION_PATCH  0

/* ── Binary image geometry (§5) ──────────────────────────────────── */
/* The Base SUCS space is 31 bits wide. Splitting it 16/15 divides it into
 * 32768 pages of 65536 codepoints, which is exhaustive: the highest page index
 * is 0x7FFFFFFF >> 16 == 0x7FFF. Exhaustive is what lets the directory carry
 * no terminator and the index need no length check.
 *
 * The split is not arbitrary. A page is the unit of *resolution*: a page whose
 * codepoints share one classification is stored as a single uniform slot, and
 * only a page whose classification varies pays for a per-codepoint index. The
 * Native plane — 0x00120000..7FFFFFEF, some 2.1 billion codepoints — is one
 * classification, so it costs one slot per page and no index at all; the ASCII
 * page carries a dozen distinct bidirectional classifications, so it costs one
 * 128 KiB index and nothing else. Storing every page densely would cost 4 GiB;
 * storing none densely would be unable to describe ASCII at all. */
#define SUAS_SUCD_PAGE_SHIFT       16U
#define SUAS_SUCD_PAGE_SIZE        ((uint32_t)1U << SUAS_SUCD_PAGE_SHIFT)
#define SUAS_SUCD_PAGE_MASK        (SUAS_SUCD_PAGE_SIZE - 1U)
#define SUAS_SUCD_DIRECTORY_SIZE   32768U
#define SUAS_SUCD_DIRECTORY_BYTES  (SUAS_SUCD_DIRECTORY_SIZE * 4U)
#define SUAS_SUCD_MAX_CODEPOINT    SUAS_CODEPOINT_MAX /* 0x7FFFFFFF */

/* A dense index entry, or a directory slot, holding this value names no
 * record: the codepoint takes the default record. It is also why the record
 * count is capped one below: an index must never collide with the marker. */
#define SUAS_SUCD_NO_RECORD        ((uint16_t)0xFFFFu)
#define SUAS_SUCD_RECORD_CAPACITY  65535U

/* Directory slot encoding. A slot is one of exactly three things:
 *
 *   SUAS_SUCD_NO_PAGE            — every codepoint in the page is unassigned;
 *                                  the whole page falls to the default record.
 *   SUAS_SUCD_UNIFORM_TAG | i    — the whole page is covered by record i, so a
 *                                  query needs no further index load. The
 *                                  verifier requires record i to contain the
 *                                  entire page.
 *   anything else                — a uint16 offset, in index units, into the
 *                                  dense per-codepoint index for that page.
 *                                  The page therefore carries 65536 entries.
 *
 * Three kinds, not two, because the space is simultaneously enormous and empty
 * in the middle and dense at the bottom — and a two-way encoding forces one of
 * those two truths to be paid for everywhere. */
#define SUAS_SUCD_NO_PAGE          0xFFFFFFFFu
#define SUAS_SUCD_UNIFORM_TAG      0x80000000u
#define SUAS_SUCD_UNIFORM_INDEX(s) ((s) & 0x7FFFFFFFu)

/* Binary signature 'SUCD', read as a big-endian fourcc. */
#define SUAS_SUCD_MAGIC            0x53554344u

/* Image-level flags, from sucd_header_t.flags. */
#define SUCD_IMAGE_PARTITION  0x01u /* records are ordered and disjoint (§4.4).
                                     * A compiler that proves the partition
                                     * must set it; a loader that sees it may
                                     * skip the proof. */

/* ── Plane taxonomy (§4.2) ───────────────────────────────────────── */
/* TRAP and SENTINEL are what separate this mask from the manifest mask in
 * SUAS-011 (SCMS): a manifest cannot name these, an image must be able to
 * describe them. */
#define SUCD_PLANE_BRIDGE    0x01u /* 0x00000000–0x0010FFFF  (1:1 with Unicode) */
#define SUCD_PLANE_SCP       0x02u /* 0x00110000–0x0011FFFF  (control plane)    */
#define SUCD_PLANE_NATIVE    0x04u /* 0x00120000–0x7FFFFFEF  (native/neography) */
#define SUCD_PLANE_RNUR      0x08u /* 0x00160000–0x0016FFFF  (inside NATIVE)    */
#define SUCD_PLANE_TRAP      0x10u /* 0x7FFFFFF0–0x7FFFFFFE  (trap slots)       */
#define SUCD_PLANE_SENTINEL  0x20u /* 0x7FFFFFFF             (end of stream)    */
#define SUCD_PLANE_MASK_ALL  0x3Fu

/* ── Property mask (§4.3) ────────────────────────────────────────── */
/* One bit per property named by a published SUCD data file. The spellings in
 * the comments are the file's own, because they are the names a compiler
 * matches and the names an API exposes. Seventeen bits are assigned; the upper
 * fifteen are reserved so a mask may widen without a record-size change. */
#define SUCD_PROP_UNICODE_BRIDGED  0x00000001u /* PropList: Unicode_Bridged   */
#define SUCD_PROP_CONTROL_PLANE    0x00000002u /* PropList: Control_Plane     */
#define SUCD_PROP_PRINTABLE        0x00000004u /* PropList: Printable         */
#define SUCD_PROP_DIAGNOSTIC_BAN   0x00000008u /* PropList: Diagnostic_BAN    */
#define SUCD_PROP_DIAGNOSTIC_WARN  0x00000010u /* PropList: Diagnostic_WARN   */
#define SUCD_PROP_DIAGNOSTIC_COM   0x00000020u /* PropList: Diagnostic_COM    */
#define SUCD_PROP_DIAGNOSTIC_SOFT  0x00000040u /* PropList: Diagnostic_SOFT   */
#define SUCD_PROP_TRAP_INSTRUCTION 0x00000080u /* PropList: Trap_Instruction  */
#define SUCD_PROP_SENTINEL_VALUE   0x00000100u /* PropList: Sentinel_Value    */
#define SUCD_PROP_ALPHABETIC       0x00000200u /* DerivedCoreProperties: Alphabetic */
#define SUCD_PROP_CONTROL          0x00000400u /* DerivedCoreProperties: Control    */
#define SUCD_PROP_BIDI_LTR         0x00000800u /* BidiProps: LTR              */
#define SUCD_PROP_BIDI_RTL         0x00001000u /* BidiProps: RTL              */
#define SUCD_PROP_BIDI_ARABIC_AL   0x00002000u /* BidiProps: ARABIC_AL        */
#define SUCD_PROP_BIDI_NEUTRAL     0x00004000u /* BidiProps: NEUTRAL          */
#define SUCD_PROP_BIDI_MIRRORED    0x00008000u /* BidiProps: MIRRORED         */
#define SUCD_PROP_BIDI_WHITESPACE  0x00010000u /* BidiProps: WHITESPACE       */
#define SUCD_PROP_ASSIGNED_MASK    0x0001FFFFu /* every bit a release may set */
#define SUCD_PROP_MASK_ALL         0xFFFFFFFFu

/* ── General category (§4.5) ─────────────────────────────────────── */
/* The thirty subcategories of Unicode's General_Category, renumbered densely
 * so that Cn — the value an unassigned codepoint takes, and the value the
 * default record carries — is zero. The eight group values (L, M, N, P, S, Z,
 * C, LC) are derivable unions and are deliberately not represented: a record
 * stores the finest classification it has, and a caller wanting "a letter"
 * tests four values rather than one. */
typedef enum {
    SUCD_GC_CN = 0,  /* unassigned / reserved — the default                 */
    SUCD_GC_LU = 1,  /* uppercase letter                                    */
    SUCD_GC_LL = 2,  /* lowercase letter                                    */
    SUCD_GC_LT = 3,  /* titlecase letter                                    */
    SUCD_GC_LM = 4,  /* modifier letter                                     */
    SUCD_GC_LO = 5,  /* other letter                                        */
    SUCD_GC_MN = 6,  /* nonspacing mark                                     */
    SUCD_GC_MC = 7,  /* spacing mark                                        */
    SUCD_GC_ME = 8,  /* enclosing mark                                      */
    SUCD_GC_ND = 9,  /* decimal number                                      */
    SUCD_GC_NL = 10, /* letter number                                       */
    SUCD_GC_NO = 11, /* other number                                        */
    SUCD_GC_PC = 12, /* connector punctuation                               */
    SUCD_GC_PD = 13, /* dash punctuation                                    */
    SUCD_GC_PS = 14, /* open punctuation                                    */
    SUCD_GC_PE = 15, /* close punctuation                                   */
    SUCD_GC_PI = 16, /* initial punctuation                                 */
    SUCD_GC_PF = 17, /* final punctuation                                   */
    SUCD_GC_PO = 18, /* other punctuation                                   */
    SUCD_GC_SM = 19, /* math symbol                                         */
    SUCD_GC_SC = 20, /* currency symbol                                     */
    SUCD_GC_SK = 21, /* modifier symbol                                     */
    SUCD_GC_SO = 22, /* other symbol                                        */
    SUCD_GC_ZS = 23, /* space separator                                     */
    SUCD_GC_ZL = 24, /* line separator                                      */
    SUCD_GC_ZP = 25, /* paragraph separator                                 */
    SUCD_GC_CC = 26, /* control                                             */
    SUCD_GC_CF = 27, /* format                                              */
    SUCD_GC_CS = 28, /* surrogate                                           */
    SUCD_GC_CO = 29, /* private use                                         */
    SUCD_GC_COUNT = 30
} sucd_general_category_t;

/* ── Record flags (§4.6) ─────────────────────────────────────────── */
/* The cursor contract lifted from SUCSData.txt's type column. Exactly one of
 * the two is always set: an image record describes either an allocation the
 * cursor may advance through, or a machine instruction that stops it. */
#define SUCD_RECORD_CONTROL    0x01u /* type 1 — cursor does NOT advance */
#define SUCD_RECORD_PRINTABLE  0x02u /* type 0 — cursor advances         */

/* ── Image-verification outcomes (§5.3) ──────────────────────────── */
typedef enum {
    SUCD_IMAGE_OK                     = 0,
    SUCD_IMAGE_NULL_IMAGE             = 1,
    SUCD_IMAGE_TRUNCATED              = 2, /* size below header + directory  */
    SUCD_IMAGE_BAD_MAGIC              = 3,
    SUCD_IMAGE_BAD_DIRECTORY_COUNT    = 4,
    SUCD_IMAGE_RECORD_COUNT_OVER      = 5, /* > SUAS_SUCD_RECORD_CAPACITY   */
    SUCD_IMAGE_SIZE_MISMATCH          = 6, /* layout not implied by the sizes */
    SUCD_IMAGE_BAD_RECORD_FLAG        = 7, /* neither, or both, cursor kinds */
    SUCD_IMAGE_RANGE_ORDER            = 8, /* first > last                   */
    SUCD_IMAGE_OUT_OF_SPACE           = 9, /* last > 0x7FFFFFFF              */
    SUCD_IMAGE_GC_OUT_OF_RANGE        = 10,
    SUCD_IMAGE_RESERVED_PROPERTY_BIT  = 11, /* mask outside SUCD_PROP_ASSIGNED */
    SUCD_IMAGE_UNORDERED_RECORDS      = 12, /* not sorted by first           */
    SUCD_IMAGE_OVERLAPPING_RECORDS    = 13,
    SUCD_IMAGE_DIRECTORY_OUT_OF_RANGE = 14, /* slot names no record          */
    SUCD_IMAGE_DENSE_OFFSET_OVER      = 15, /* slot past the dense index     */
    SUCD_IMAGE_PAGE_NOT_COVERED       = 16, /* uniform record misses page    */
    SUCD_IMAGE_UNKNOWN                = 17
} sucd_image_error_t;

/* ── Data-file range forms (§6.1) ────────────────────────────────── */
/* The published tree is inconsistent about how it spells a range, and a
 * compiler that reads it must not be. Both forms decode to the same
 * (first, last) pair, and the form is reported so a compiler can attribute a
 * file to a convention. */
typedef enum {
    SUCD_RANGE_FORM_NONE     = 0, /* not a range line                        */
    SUCD_RANGE_FORM_UCD      = 1, /* AAAAAA..BBBBBB  — Props/, Derived/, Blocks */
    SUCD_RANGE_FORM_HYPHEN   = 2, /* AAAAAAAA-BBBBBBBB — SUCSData.txt, sudat/ */
    SUCD_RANGE_FORM_POINT    = 3  /* AAAAAA — a lone codepoint               */
} sucd_range_form_t;

/* ── Structural types (zero-allocation image descriptors) ────────── */

/* The image header: the first 24 bytes of every mapped image. The three size
 * fields are enough to place every other part of the image exactly, so a
 * verifier can derive the dense-index extent rather than trusting it. */
typedef struct {
    uint32_t magic;             /* SUAS_SUCD_MAGIC                              */
    uint32_t version;           /* packed SUAS_SUCD_VERSION_*                   */
    uint32_t total_records;     /* range records, after the directory and index */
    uint32_t plane_mask;        /* SUCD_PLANE_* bits the records cover          */
    uint32_t directory_count;   /* always SUAS_SUCD_DIRECTORY_SIZE              */
    uint32_t flags;             /* SUCD_IMAGE_*                                 */
} sucd_header_t;                /* 24 bytes, 4-aligned                          */

/* One range record: a contiguous run of codepoints sharing one classification.
 * Range-compressed rather than per-codepoint, because every file in the
 * published tree is range-compressed and a point record would cost one row per
 * codepoint for data that describes a dozen spans. */
typedef struct {
    uint32_t first;             /* inclusive range start                        */
    uint32_t last;              /* inclusive range end                          */
    uint32_t property_mask;     /* SUCD_PROP_*                                  */
    uint16_t general_category;  /* sucd_general_category_t                      */
    uint8_t  script_id;         /* SUAS-005 (SSP) script identifier             */
    uint8_t  flags;             /* SUCD_RECORD_*                                */
} sucd_record_t;                /* 16 bytes, 4-aligned                          */

/* A manifest's target schema, for the SSRE linkage gate (§7). */
typedef struct {
    const char *schema_spec_id;
    uint8_t     schema_major;
    uint8_t     schema_minor;
    bool        schema_mandatory;
} sucd_schema_link_t;

/* ── Engine setup ────────────────────────────────────────────────── */

/* Version of this translation unit, e.g. "SUAS-012/1.0.0". Never NULL. */
const char* suas_sucd_version_string(void);

/* Checks the geometry's static invariants and arms the engine. Idempotent. */
void suas_sucd_init_engine(void);

/* Unbinds the image, so the next query reports a miss rather than reading a
 * stale mapping. O(1). */
void suas_sucd_shutdown(void);

/* True while an image is bound. O(1). */
bool suas_sucd_is_loaded(void);

/* ── Image binding and verification (§5) ─────────────────────────── */

/* Binds a mapped image after verifying it. The image is not copied and is not
 * mutated: the loader reads it in place, so a kernel may pass a read-only
 * mapping. Verification is O(image extent); on failure nothing is bound. */
bool suas_sucd_init(const void *binary_image, size_t image_size);

/* Verifies an image without binding it. On failure out_reason, when non-NULL,
 * receives the sucd_image_error_t naming the first defect. Zero allocation. */
bool suas_sucd_verify_image(const void *binary_image, size_t image_size,
                            sucd_image_error_t *out_reason);

/* Canonical name of an image outcome — "SUCD_IMAGE_OK", … — or
 * "SUCD_IMAGE_UNKNOWN". Never NULL. */
const char* suas_sucd_image_error_name(sucd_image_error_t error);

/* The bound image's header, or NULL when nothing is bound. The returned
 * pointer aliases the mapping and must not outlive it. */
const sucd_header_t* suas_sucd_header(void);

/* ── Queries (§5.2) ──────────────────────────────────────────────── */

/* The record covering a codepoint. O(1): one directory load, at most one index
 * load, one bounds compare. Unbound image, out-of-space, or an unassigned page
 * all yield the default record rather than a failure, because a database that
 * answers "not assigned" is answering the question. */
bool suas_sucd_get_record(uint32_t codepoint, sucd_record_t *out_record);

/* The record a codepoint outside every allocation receives: Cn, no properties,
 * the UNKNOWN script, and the PRINTABLE cursor contract. O(1). */
bool suas_sucd_default_record(sucd_record_t *out_record);

/* The SUCD_PROP_* bits for a codepoint. O(1). */
uint32_t suas_sucd_property_mask(uint32_t codepoint);

/* The single SUCD_PLANE_* bit for a codepoint. RNUR reports SUCD_PLANE_RNUR
 * rather than SUCD_PLANE_NATIVE so the carve-out stays addressable; a caller
 * wanting "is it native" tests both. O(1). */
uint8_t suas_sucd_plane_of(uint32_t codepoint);

/* Canonical name of a plane bit — "SUCD_PLANE_SCP" — or NULL for a bit that is
 * not a single plane. Never returns an empty string. */
const char* suas_sucd_plane_name(uint8_t plane_bit);

/* ── Taxonomy lookups (§4.3, §4.5) ───────────────────────────────── */

/* Canonical name of a general category — "Lu", "Cn" — or "??". Never NULL. */
const char* suas_sucd_gc_name(sucd_general_category_t gc);

/* The category named by a short alias, matched case-sensitively and
 * exactly. Cn when the name is unknown or the length is zero. O(len), bounded
 * by the longest alias. */
sucd_general_category_t suas_sucd_gc_by_name(const char *name, size_t length);

/* The single SUCD_PROP_* bit a property name carries, or 0 when the name is
 * not an assigned SUCD property. Names are the published spellings from
 * PropList.txt, DerivedCoreProperties.txt and BidiProps.txt. O(len), bounded. */
uint32_t suas_sucd_property_by_name(const char *name, size_t length);

/* Canonical name of an assigned property bit — "Printable", "Alphabetic" — or
 * NULL for 0 and for a bit that is not a single assigned property. */
const char* suas_sucd_property_name(uint32_t property_bit);

/* ── Data-file decoding (§6) ─────────────────────────────────────── */

/* The range form a data line uses. SUCD_RANGE_FORM_NONE for a comment, a blank
 * line, or a line that is not a range. O(len), bounded. */
sucd_range_form_t suas_sucd_range_form(const char *line, size_t length);

/* Decodes a range line in either convention into an inclusive (first, last)
 * pair, accepting 1 to 8 significant hex digits so that the published files —
 * which disagree among themselves — all parse. False for a malformed range, an
 * overflowing value, or a reversed pair. O(len), bounded. */
bool suas_sucd_parse_range(const char *line, size_t length,
                           uint32_t *out_first, uint32_t *out_last);

/* True when two records cover at least one codepoint in common. Adjacent
 * records — end + 1 == start of the next — do not overlap, because the ranges
 * are inclusive. O(1). */
bool suas_sucd_records_overlap(const sucd_record_t *a, const sucd_record_t *b);

/* True when a record set is a partition of the space it covers: ordered,
 * non-overlapping, and within the 31-bit space. This is the proof the
 * SUCD_IMAGE_PARTITION flag asserts, and a compiler must run it before setting
 * that flag. O(count). */
bool suas_sucd_is_partition(const sucd_record_t *records, uint32_t count);

/* ── SUAS-010 (SSRE) linkage gate (§7) ───────────────────────────── */

/* Confirms that the schema the image was compiled against — normally SUAS-011
 * (SCMS), the manifest grammar that produced it — is registered in the SSRE
 * cross-reference registry and satisfies the declared floor. False for a NULL
 * link or an unregistered schema. */
bool suas_sucd_verify_schema_link(const sucd_schema_link_t *link);

/* ── SUAS-001 (SDF) compatibility surface ────────────────────────── */
/* Before SUCD was a standard it was an eleven-line header that existed for one
 * caller: the Structural Directional Framing resolver in SUAS-001, which asked
 * a single question — "what is this codepoint's bidirectional role?" — against
 * a compiled subset of sucd/Props/BidiProps.txt.
 *
 * That question is now a specialization of the record query, but the bit
 * layout is load-bearing: SUAS-001 §6 and the on-disk BidiProps.txt projection
 * in SUTR-8 both name these six bits, and SDF compares them for equality. They
 * are therefore retained exactly, as the *legacy* projection, and deliberately
 * numbered from bit 0 rather than aliased onto SUCD_PROP_BIDI_* — the two masks
 * answer the same taxonomy in two different encodings, and pretending otherwise
 * would silently change the value SDF's tests assert.
 *
 * The accessor itself lives in suas_sdf.c, where the embedded range table does;
 * SUCD declares it so that a translation unit may include only this header. */
#define SUCD_BIDI_LTR        (1u << 0)  /* strong left-to-right              */
#define SUCD_BIDI_RTL        (1u << 1)  /* strong right-to-left              */
#define SUCD_BIDI_ARABIC_AL  (1u << 2)  /* arabic letter, resolves RTL       */
#define SUCD_BIDI_NEUTRAL    (1u << 3)  /* direction-neutral                 */
#define SUCD_BIDI_MIRRORED   (1u << 4)  /* paired-format glyph, auto-mirror  */
#define SUCD_BIDI_WHITESPACE (1u << 5)  /* separator / whitespace, neutral   */

#define SUCD_BIDI_ALL (SUCD_BIDI_LTR | SUCD_BIDI_RTL | SUCD_BIDI_ARABIC_AL | \
                       SUCD_BIDI_NEUTRAL | SUCD_BIDI_MIRRORED | \
                       SUCD_BIDI_WHITESPACE)

/* The legacy BiDi mask for a codepoint: the compiled BidiProps subset when the
 * codepoint is inside the Unicode Bridge, SUCD_BIDI_NEUTRAL otherwise. */
uint32_t suas_sucd_bidi(uint32_t codepoint);

/* Zone predicates, retained for the same caller and the same reason. */
static inline int suas_sucd_is_unicode_bridge(uint32_t cp)
{
    return (cp <= 0x0010FFFFUL);
}

static inline int suas_sucd_is_scp(uint32_t cp)
{
    return (cp >= SUAS_ZONE_SCP_MIN && cp <= SUAS_ZONE_SCP_MAX);
}

static inline int suas_sucd_is_native(uint32_t cp)
{
    return (cp >= SUAS_ZONE_NATIVE_MIN && cp <= SUAS_ZONE_NATIVE_MAX);
}

#ifdef __cplusplus
}
#endif

#endif /* SUAS_SUAS_SUCD_H */
