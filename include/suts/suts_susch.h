#ifndef SUTS_SUTS_SUSCH_H
#define SUTS_SUTS_SUSCH_H

/* SUTS-008 — SuperUnicode Source Code Handling (SUSCH)
 *
 * A modular data specification within the OpenWindows microkernel ecosystem.
 * SUSCH establishes the binary-mapped security profiles, identifier
 * validation rules, and zero-allocation screening engines required to
 * process, compile, and execute programming language source code containing
 * internationalized SuperUnicode characters in O(1) constant time.
 *
 * Where traditional Unicode implementations (such as UTS #55, "Unicode Source
 * Code Handling") rely on complex runtime text-scanning heuristics, regex
 * filters, and dynamic buffer parsing to mitigate source code security
 * vulnerabilities (bidirectional override exploits, homoglyph spoofing, and
 * invisible character injection), SUSCH compiles strict source code safety
 * attributes and character classification masks directly into immutable binary
 * lookup tables. This allows compilers, build toolchains, and kernel-level
 * script execution environments to validate source tokens instantly without a
 * single heap allocation (malloc/free).
 *
 * SUSCH is a dual-layer system, in the same relationship SUAS-009 (SIP),
 * SUAS-012 (SUCD) and SUTS-005 (SCSS) have with their Unicode counterparts.
 * The offline reference layer is the SuperUnicode Character Database (SUCD /
 * SUAS-012) — the human-readable confusables manifests, identifier status
 * tables, and bidi-control inventories that user-space compilers, linters,
 * IDEs and developer toolchains read. Build-time tooling compiles those
 * datasets into the immutable, byte-packed binary lookup tables this module
 * embeds into kernel images. The kernel never reads a file at run time.
 *
 * Two commitments separate SUSCH from a text-scanning linter:
 *
 *   1. ZERO-ALLOCATION SCREENING. Every query is a bounded probe of a static
 *      range table or a fixed binary search over one. No path allocates, grows
 *      a table, or reaches for libc.
 *   2. A SINGLE LINEAR PASS OVER A TOKEN STREAM. The identifier validator
 *      walks the codepoint stream exactly once, accumulating identifier
 *      status, chunk boundaries, and script evidence as it goes, so the cost
 *      of screening a source token is O(n) in the length of the token and
 *      O(1) in the size of the address space.
 *
 * Because core operating system kernel binaries and low-level freestanding C99
 * modules are authored in strict ASCII/Latin-1 subsets, SUSCH is categorized as
 * a modular SUTS data specification: user-space development environments,
 * multi-lingual programming languages, and high-level IDE compilers opt into
 * SUSCH to guarantee international source code safety.
 *
 * Neography and conscript source compatibility: unlike traditional standards
 * restricted to standard Unicode blocks, SUSCH admits the Native & Neography
 * Plane (0x00160000–0x0016FFFF) so that developers writing code in custom
 * conlang scripts or neographies may define strict identifier profiles and
 * script-mixing boundaries at build time. Mixing a neography character with a
 * Bridge-script character in one identifier chunk is a first-class diagnostic,
 * not an accident of the table layout.
 *
 * Freestanding C99, zero allocation: the tables are static, every descriptor
 * is a fixed-size struct the caller owns, and no part of the engine reaches
 * for malloc, stdio or libc string routines.
 *
 * See docs/suts/SUTS-008-susch.md for the normative specification.
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Specification identifiers ────────────────────────────────────── */
#define SUSCH_SPEC_ID           "SUTS-008"
#define SUSCH_SPEC_TITLE        "SuperUnicode Source Code Handling"
#define SUSCH_VERSION_MAJOR     1
#define SUSCH_VERSION_MINOR     0
#define SUSCH_MODULE            "suts_susch"

/* ── Source code security flags (packed bitfield) ─────────────────── */
/* The four bits below are the ones named by the seeding sketch; they are
 * preserved verbatim so a consumer written against the sketch compiles
 * unchanged. Bits 4 and above are derived properties the engine computes from
 * the same compiled tables. */
#define SUSCH_FLAG_VALID_IDENTIFIER (1U << 0) /* admissible in an identifier   */
#define SUSCH_FLAG_BIDI_CONTROL     (1U << 1) /* directional formatting ctrl  */
#define SUSCH_FLAG_INVISIBLE        (1U << 2) /* zero-width / invisible        */
#define SUSCH_FLAG_HOMOGLYPH_RISK   (1U << 3) /* high-risk confusable profile */

/* ── Derived source-code flags (bits 4-12) ────────────────────────── */
#define SUSCH_FLAG_LINE_BREAK       (1U << 4)  /* a line terminator candidate */
#define SUSCH_FLAG_JOINING_CONTROL  (1U << 5)  /* ZWNJ / ZWJ                   */
#define SUSCH_FLAG_WHITESPACE       (1U << 6)  /* horizontal or vertical space */
#define SUSCH_FLAG_PUNCTUATION      (1U << 7)  /* any punctuation              */
#define SUSCH_FLAG_DIGIT            (1U << 8)  /* a decimal digit              */
#define SUSCH_FLAG_COMBINING        (1U << 9)  /* a combining mark             */
#define SUSCH_FLAG_IGNORABLE        (1U << 10) /* default ignorable            */
#define SUSCH_FLAG_NATIVE_PLANE     (1U << 11) /* Native / neography plane     */
#define SUSCH_FLAG_QUOTATION        (1U << 12) /* a quotation mark             */
#define SUSCH_FLAG_MASK_DEFINED     0x1FFFu    /* every bit defined above      */

/* ── Script categories (§4.3) ──────────────────────────────────────── */
/* A coarse script classification, sufficient to tell a single-script
 * identifier chunk from a visibly mixed-script one. COMMON covers
 * script-neutral codepoints (digits, punctuation, symbols); INHERITED covers
 * combining marks that take the script of their base; NATIVE and NEOGRAPHY
 * cover the Native plane and the RNUR neography carve-out. */
typedef enum {
    SUSCH_SCRIPT_COMMON     = 0,  /* script-neutral digits, punctuation     */
    SUSCH_SCRIPT_LATIN      = 1,
    SUSCH_SCRIPT_GREEK      = 2,
    SUSCH_SCRIPT_CYRILLIC   = 3,
    SUSCH_SCRIPT_ARMENIAN   = 4,
    SUSCH_SCRIPT_HEBREW     = 5,
    SUSCH_SCRIPT_ARABIC     = 6,
    SUSCH_SCRIPT_SYRIAC     = 7,
    SUSCH_SCRIPT_DEVANAGARI = 8,
    SUSCH_SCRIPT_BENGALI    = 9,
    SUSCH_SCRIPT_THAI       = 10,
    SUSCH_SCRIPT_GEORGIAN   = 11,
    SUSCH_SCRIPT_HAN        = 12,
    SUSCH_SCRIPT_HIRAGANA   = 13,
    SUSCH_SCRIPT_KATAKANA   = 14,
    SUSCH_SCRIPT_HANGUL     = 15,
    SUSCH_SCRIPT_NATIVE     = 16, /* the Native plane outside the RNUR     */
    SUSCH_SCRIPT_NEOGRAPHY  = 17, /* the RNUR carve-out, 0x00160000+        */
    SUSCH_SCRIPT_INHERITED  = 18, /* combining marks and joiners           */
    SUSCH_SCRIPT_UNKNOWN    = 19  /* not classified by the curated table    */
} susch_script_t;

/* ── Identifier status (§3.2) ──────────────────────────────────────── */
/* The three-state admission verdict for one codepoint. ALLOWED is a clean
 * identifier character; RESTRICTED is admissible but carries a security
 * caveat (a confusable prototype, a joining control, a profile exclusion);
 * DISALLOWED may never appear in an identifier. */
typedef enum {
    SUSCH_IDENT_DISALLOWED = 0,
    SUSCH_IDENT_RESTRICTED = 1,
    SUSCH_IDENT_ALLOWED    = 2
} susch_identifier_status_t;

/* ── Address-space zones (§5) ──────────────────────────────────────── */
typedef enum {
    SUSCH_ZONE_BRIDGE   = 0, /* 0x00000000-0x0010FFFF           */
    SUSCH_ZONE_SCP      = 1, /* 0x00110000-0x0011FFFF           */
    SUSCH_ZONE_RNUR     = 2, /* 0x00160000-0x0016FFFF           */
    SUSCH_ZONE_NATIVE   = 3, /* rest of 0x00120000-0x7FFFFFEF   */
    SUSCH_ZONE_TRAP     = 4, /* 0x7FFFFFF0-0x7FFFFFFE           */
    SUSCH_ZONE_SENTINEL = 5, /* 0x7FFFFFFF                      */
    SUSCH_ZONE_EXTSUCS  = 6  /* > 0x7FFFFFFF (plugin space)     */
} susch_zone_t;

/* ── Token-stream validation outcomes (§6.2) ───────────────────────── */
typedef enum {
    SUSCH_TOKEN_OK = 0,
    SUSCH_TOKEN_ERR_NULL,          /* a NULL stream with a non-zero length   */
    SUSCH_TOKEN_ERR_EMPTY,         /* a zero-length identifier              */
    SUSCH_TOKEN_ERR_ILLEGAL_ZONE,  /* a trap slot, the Sentinel or ExtSUCS  */
    SUSCH_TOKEN_ERR_BIDI_CONTROL,  /* a directional formatting character    */
    SUSCH_TOKEN_ERR_INVISIBLE,     /* a zero-width or invisible character   */
    SUSCH_TOKEN_ERR_LINE_BREAK,    /* a line terminator inside the token    */
    SUSCH_TOKEN_ERR_INVALID_START, /* the first codepoint may not start one */
    SUSCH_TOKEN_ERR_INVALID_CONTINUE /* a later codepoint may not continue  */
} susch_token_status_t;

/* ── Advisory diagnostics (§6.3) ───────────────────────────────────── */
/* These never make a token invalid. They are the warnings a linter, an IDE
 * squiggle, or a CI gate surfaces on top of a verdict. */
#define SUSCH_WARN_NONE          0x0000u /* no advisory finding              */
#define SUSCH_WARN_HOMOGLYPH     (1u << 0) /* a confusable prototype present  */
#define SUSCH_WARN_MIXED_SCRIPT  (1u << 1) /* a chunk spans two scripts       */
#define SUSCH_WARN_CROSS_ZONE    (1u << 2) /* a Bridge and a Native character */
#define SUSCH_WARN_RESTRICTED    (1u << 3) /* a restricted character present  */
#define SUSCH_WARN_MASK_DEFINED  0x000Fu

/* ── Structural types (zero-allocation) ─────────────────────────────── */
/* The compiled per-codepoint source attribute. The layout is the one named
 * by the seeding sketch and is exactly 12 bytes, byte-aligned:
 *
 *   codepoint         the raw 31-bit SuperUnicode codepoint
 *   security_mask     the packed SUSCH_FLAG_* attribute word
 *   script_category   the susch_script_t classification
 *   identifier_status the susch_identifier_status_t admission verdict
 *   reserved          must be zero
 */
typedef struct {
    uint32_t codepoint;         /* Raw 31-bit SuperUnicode codepoint */
    uint32_t security_mask;     /* Packed source code safety attributes */
    uint16_t script_category;   /* Associated script classification */
    uint8_t  identifier_status; /* Allowed, Restricted, or Disallowed */
    uint8_t  reserved;          /* must be zero */
} susch_source_attr_t;

/* ── Fixed limits (static, stack-allocatable tables) ──────────────── */
/* The compiled attribute tables hold at most this many rows each. The
 * capacities are compile-time checked against the shipped tables, so a data
 * update that overflows a table fails the build rather than the kernel. */
#define SUSCH_RANGE_CAPACITY     256U
#define SUSCH_SKELETON_CAPACITY  320U
#define SUSCH_IDENT_CAPACITY     128U
#define SUSCH_SCRIPT_CAPACITY    128U
#define SUSCH_CASE_CAPACITY      32U

/* ── Zone boundaries (duplicated for freestanding SUSCH) ────────────── */
#define SUSCH_LIMIT_UNICODE_MAX  0x0010FFFFUL
#define SUSCH_LIMIT_SCP_MIN      0x00110000UL
#define SUSCH_LIMIT_SCP_MAX      0x0011FFFFUL
#define SUSCH_LIMIT_NATIVE_MIN   0x00120000UL
#define SUSCH_LIMIT_NATIVE_MAX   0x7FFFFFEFUL
#define SUSCH_LIMIT_TRAP_MIN     0x7FFFFFF0UL
#define SUSCH_LIMIT_TRAP_MAX     0x7FFFFFFEUL
#define SUSCH_LIMIT_SENTINEL     0x7FFFFFFFUL

/* The RNUR / RCCR neography carve-out inside the Native plane (SUAS-005). */
#define SUSCH_RNUR_MIN           0x00160000UL
#define SUSCH_RNUR_MAX           0x0016FFFFUL
#define SUSCH_RNUR_SET_SHIFT     8U

/* ── Version and setup ─────────────────────────────────────────────── */
/* The version string, a NUL-terminated ASCII literal ("1.0"). */
const char* suts_susch_version_string(void);

/* Initializes the static SUSCH source code screening engine. Idempotent and
 * allocation-free. Every query builds nothing and works without it; the call
 * exists so a toolchain can force initialization at a deterministic point and
 * so a future revision with build-time tables has a stable entry point. */
void suts_susch_init_engine(void);

/* ── Attribute query (§3) ──────────────────────────────────────────── */
/* Evaluates a 31-bit codepoint's source code safety attributes in O(1) time
 * without heap allocation or string parsing. The query is total over the
 * 31-bit SuperUnicode address space: every codepoint from 0x00000000 through
 * the Sentinel 0x7FFFFFFF has an answer, and an unclassified codepoint is
 * answered by the algorithmic default of §3.4 rather than by a lookup
 * failure. On success `*out_attr` receives the compiled attribute and the
 * function returns true. It returns false — leaving `*out_attr` untouched —
 * only for a NULL output pointer or a codepoint above the Sentinel, which is
 * ExtSUCS plugin space and is not source code. */
bool suts_susch_get_attributes(uint32_t codepoint, susch_source_attr_t *out_attr);

/* ── Token-stream validation (§6) ───────────────────────────────────── */
/* Validates an entire source code identifier token stream against the
 * spoofing, invisible-character, and bidi override rules in a single linear
 * pass. Returns true when the stream is a valid identifier; returns false on
 * any disqualifying defect. */
bool suts_susch_validate_token_stream(const uint32_t *token_stream, size_t length);

/* The same validation with a diagnostic channel. `out_status` (when not
 * NULL) receives the closed outcome code, `out_warnings` (when not NULL)
 * receives the advisory SUSCH_WARN_* bitmap, and `out_index` (when not NULL)
 * receives the index of the offending codepoint, or the stream length when
 * the defect is the stream as a whole. The advisory findings never change
 * the verdict: a homoglyph or a mixed-script chunk is a warning, while a
 * directional control or an invisible character is a defect. */
bool suts_susch_validate_token_stream_ex(const uint32_t *token_stream,
                                         size_t length,
                                         susch_token_status_t *out_status,
                                         uint32_t *out_warnings,
                                         size_t *out_index);

/* ── Classification predicates (§3, §4) ────────────────────────────── */
/* True for a directional formatting character: the explicit controls
 * (LRE, RLE, PDF, LRO, RLO), the implicit marks (LRM, RLM, ALM), and the
 * isolate initiators and terminator (LRI, RLI, FSI, PDI) (O(1)). */
bool suts_susch_is_bidi_control(uint32_t codepoint);

/* True for a zero-width, invisible, or otherwise default-ignorable
 * character — the "what the reader sees is not what the compiler reads"
 * class (O(1)). */
bool suts_susch_is_invisible(uint32_t codepoint);

/* True for a character with a high-risk confusable profile: a codepoint
 * whose skeleton prototype differs from itself, or one drawn from a curated
 * range of visually risky forms (O(1)). */
bool suts_susch_is_homoglyph_risk(uint32_t codepoint);

/* True when the codepoint may begin an identifier (O(1)). */
bool suts_susch_is_identifier_start(uint32_t codepoint);

/* True when the codepoint may continue an identifier (O(1)). */
bool suts_susch_is_identifier_continue(uint32_t codepoint);

/* The admission verdict for the codepoint (O(1)). */
susch_identifier_status_t suts_susch_identifier_status(uint32_t codepoint);

/* The script classification of the codepoint (O(1)). */
susch_script_t suts_susch_script_of(uint32_t codepoint);

/* The canonical prototype a codepoint folds to, or the codepoint itself
 * when it has no confusable prototype (O(1)). */
uint32_t suts_susch_skeleton_of(uint32_t codepoint);

/* True when two distinct codepoints share a skeleton prototype, i.e. when
 * they are visual homographs of one another (O(1)). */
bool suts_susch_confusable_with(uint32_t a, uint32_t b);

/* ── Chunk and script analysis (§4) ────────────────────────────────── */
/* True when any identifier chunk of the stream resolves to more than one
 * script, excluding the script-neutral COMMON and INHERITED
 * classifications. The chunk boundaries are the camel, hat, and snake
 * boundaries of UTS #55 §5.1.2.1 (O(n) in the stream length). */
bool suts_susch_is_mixed_script(const uint32_t *token_stream, size_t length);

/* The number of identifier chunks the stream resolves to under the UTS #55
 * §5.1.2.1 boundary rules. A zero-length stream has zero chunks (O(n)). */
bool suts_susch_chunk_count(const uint32_t *token_stream, size_t length,
                            size_t *out_chunks);

/* True when one codepoint is in the Unicode Compatibility Bridge and the
 * other is in the Native plane or the RNUR neography carve-out: the primary
 * boundary SUSCH protects a source file from (O(1)). */
bool suts_susch_crosses_bridge_boundary(uint32_t a, uint32_t b);

/* ── Zone classification (§5) ──────────────────────────────────────── */
/* Classifies a 31-bit codepoint into its address-space zone (O(1)). A value
 * above the Sentinel is EXTSUCS. */
susch_zone_t suts_susch_zone_of(uint32_t codepoint);

/* True inside the RNUR neography carve-out (O(1)). */
bool suts_susch_is_neography(uint32_t codepoint);

/* Set number of an RNUR coordinate: (cp - 0x00160000) >> 8. Returns false
 * outside the carve-out. */
bool suts_susch_neography_set(uint32_t codepoint, uint32_t *out_set);

/* ── Taxonomy names ───────────────────────────────────────────────── */
const char* suts_susch_zone_name(susch_zone_t zone);
const char* suts_susch_script_name(susch_script_t script);
const char* suts_susch_status_name(susch_token_status_t status);
const char* suts_susch_identifier_status_name(susch_identifier_status_t status);

#ifdef __cplusplus
}
#endif

#endif /* SUTS_SUTS_SUSCH_H */
