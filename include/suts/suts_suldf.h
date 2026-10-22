#ifndef SUTS_SUTS_SULDF_H
#define SUTS_SUTS_SULDF_H

/* SUTS-009 - SuperUnicode Link Detection and Formatting (SULDF)
 *
 * A modular data specification within the OpenWindows microkernel ecosystem.
 * SULDF establishes the binary-mapped link properties, the O(1) per-codepoint
 * attribute query, and the zero-allocation scanning and formatting engines
 * required to detect URLs and email addresses embedded in plain SuperUnicode
 * text and to serialize their Path, Query, and Fragment parts with minimal
 * percent escaping.
 *
 * Where traditional Unicode implementations (such as UTS #58, "Unicode Link
 * Detection and Formatting") load LinkTerm.txt, LinkBracket.txt and
 * LinkEmail.txt at run time and interpret them with a regex or a user-space
 * candidate finder, SULDF compiles the same properties into immutable,
 * byte-packed range tables. A kernel-hosted linkifier, chat client, mail
 * reader, or terminal can therefore answer every per-codepoint question with
 * a bounded probe and scan a codepoint stream in a single pass without a
 * single heap allocation (malloc/free).
 *
 * SULDF is a dual-layer system, in the same relationship SUAS-009 (SIP),
 * SUAS-012 (SUCD) and SUTS-005 (SCSS) have with their Unicode counterparts.
 * The offline reference layer is the SuperUnicode Character Database (SUCD /
 * SUAS-012), which mirrors the Unicode 18.0.0 linkification data files:
 *
 *   LinkTerm.txt     Link_Term = {Include, Hard, Soft, Close, Open}
 *   LinkBracket.txt  Link_Bracket(Close) = Open
 *   LinkEmail.txt    Link_Email = {Yes, No}
 *
 * Build-time tooling compiles those datasets into the immutable tables this
 * module embeds into kernel images. The kernel never reads a file at run time.
 *
 * Two commitments separate SULDF from a text-scanning linkifier:
 *
 *   1. ZERO-ALLOCATION SCANNING. Every query is a bounded probe of a static
 *      range table; the scanner holds one fixed 125-entry bracket stack and a
 *      fixed set of part registers. No path allocates or reaches for libc.
 *   2. A SINGLE LINEAR PASS. The scanner walks the codepoint stream once,
 *      applying UTS #58's termination algorithm with backup, so the cost is
 *      O(n) in the text length and O(1) in the size of the address space.
 *
 * Because core operating system kernel binaries and low-level freestanding C99
 * modules are authored in strict ASCII/Latin-1 subsets, SULDF is categorized
 * as a modular SUTS data specification: text-rendering subsystems, chat and
 * mail clients, terminal emulators, and graphical shells opt into SULDF to
 * guarantee predictable link boundaries.
 *
 * Neography and conscript link compatibility: unlike traditional standards
 * restricted to the Unicode Compatibility Bridge, SULDF admits the Native &
 * Neography Plane (0x00160000-0x0016FFFF) as link material. A Native or
 * neography codepoint in a Path, Query, or Fragment is an Include character
 * and is retained verbatim by the minimal-escaping formatter, so a custom
 * script's URL remains readable instead of being silently percent-encoded.
 * The System Control Plane, the trap slots, the Sentinel, and ExtSUCS are
 * never link material.
 *
 * Freestanding C99, zero allocation: the tables are static, every descriptor
 * is a fixed-size struct the caller owns, and no part of the engine reaches
 * for malloc, stdio or libc string routines.
 *
 * See docs/suts/SUTS-009-suldf.md for the normative specification.
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Specification identifiers ─────────────────────────────────────── */
#define SULDF_SPEC_ID           "SUTS-009"
#define SULDF_SPEC_TITLE        "SuperUnicode Link Detection and Formatting"
#define SULDF_VERSION_MAJOR     1
#define SULDF_VERSION_MINOR     0
#define SULDF_MODULE            "suts_suldf"

/* ── Link attribute flags (packed bitfield) ────────────────────────── */
/* The four bits below are the ones named by the seeding sketch; they are
 * preserved verbatim so a consumer written against the sketch compiles
 * unchanged. Bits 4 and above are derived properties the engine computes from
 * the same compiled tables. */
#define SULDF_FLAG_SCHEME_CHAR   (1U << 0) /* may appear in a URL scheme    */
#define SULDF_FLAG_DOMAIN_CHAR   (1U << 1) /* may appear in a domain label  */
#define SULDF_FLAG_PATH_CHAR     (1U << 2) /* may appear in path/query/frag */
#define SULDF_FLAG_EMAIL_DELIM   (1U << 3) /* the @ email local-part marker */

/* ── Derived link flags (bits 4-12) ────────────────────────────────── */
#define SULDF_FLAG_TERM_INCLUDE  (1U << 4)  /* Link_Term=Include           */
#define SULDF_FLAG_TERM_HARD     (1U << 5)  /* Link_Term=Hard              */
#define SULDF_FLAG_TERM_SOFT     (1U << 6)  /* Link_Term=Soft              */
#define SULDF_FLAG_TERM_CLOSE    (1U << 7)  /* Link_Term=Close             */
#define SULDF_FLAG_TERM_OPEN     (1U << 8)  /* Link_Term=Open              */
#define SULDF_FLAG_BRACKET_MAPPED (1U << 9) /* has a Link_Bracket mapping  */
#define SULDF_FLAG_EMAIL_CHAR    (1U << 10) /* Link_Email=Yes              */
#define SULDF_FLAG_NATIVE        (1U << 11) /* Native / neography plane    */
#define SULDF_FLAG_BIDI_CONTROL  (1U << 12) /* directional formatting ctrl */
#define SULDF_FLAG_MASK_DEFINED  0x1FFFu    /* every bit defined above     */

/* ── Link_Term categories (§3.4.1) ─────────────────────────────────── */
/* The numeric order matches LinkTerm.txt: Include, Hard, Soft, Close, Open.
 * Hard is the algorithmic default for every codepoint the compiled table does
 * not list. */
typedef enum {
    SULDF_TERM_INCLUDE = 0,
    SULDF_TERM_HARD    = 1,
    SULDF_TERM_SOFT    = 2,
    SULDF_TERM_CLOSE   = 3,
    SULDF_TERM_OPEN    = 4
} suldf_term_t;

/* ── Link_Bracket role (§3.4.2) ────────────────────────────────────── */
typedef enum {
    SULDF_BRACKET_NONE  = 0,
    SULDF_BRACKET_OPEN  = 1,
    SULDF_BRACKET_CLOSE = 2
} suldf_bracket_t;

/* ── Address-space zones (§5) ──────────────────────────────────────── */
typedef enum {
    SULDF_ZONE_BRIDGE   = 0, /* 0x00000000-0x0010FFFF           */
    SULDF_ZONE_SCP      = 1, /* 0x00110000-0x0011FFFF           */
    SULDF_ZONE_RNUR     = 2, /* 0x00160000-0x0016FFFF           */
    SULDF_ZONE_NATIVE   = 3, /* rest of 0x00120000-0x7FFFFFEF   */
    SULDF_ZONE_TRAP     = 4, /* 0x7FFFFFF0-0x7FFFFFFE           */
    SULDF_ZONE_SENTINEL = 5, /* 0x7FFFFFFF                      */
    SULDF_ZONE_EXTSUCS  = 6  /* > 0x7FFFFFFF (plugin space)     */
} suldf_zone_t;

/* ── Link kinds ────────────────────────────────────────────────────── */
typedef enum {
    SULDF_LINK_URL   = 0,
    SULDF_LINK_EMAIL = 1
} suldf_link_kind_t;

/* ── URL Part taxonomy (§3.5, §4.1) ────────────────────────────────── */
typedef enum {
    SULDF_PART_PATH               = 0,
    SULDF_PART_QUERY              = 1,
    SULDF_PART_FRAGMENT           = 2,
    SULDF_PART_FRAGMENT_DIRECTIVE = 3
} suldf_part_t;

/* ── Structural types (zero-allocation) ────────────────────────────── */
/* The compiled per-codepoint link attribute. The layout is the one named by
 * the seeding sketch and is exactly 12 bytes, byte-aligned:
 *
 *   codepoint      the raw 31-bit SuperUnicode codepoint
 *   link_mask      the packed SULDF_FLAG_* attribute word
 *   term_class     the Link_Term category (suldf_term_t)
 *   bracket_class  the Link_Bracket role (suldf_bracket_t)
 *   reserved       must be zero
 */
typedef struct {
    uint32_t codepoint;        /* Raw 31-bit SuperUnicode codepoint */
    uint32_t link_mask;        /* Packed link-detection attributes  */
    uint16_t term_class;       /* Link_Term category                */
    uint8_t  bracket_class;    /* Link_Bracket role                 */
    uint8_t  reserved;         /* must be zero                      */
} suldf_link_attr_t;

/* A detected link span. `start` and `length` are codepoint offsets into the
 * scanned stream; the span is half-open, [start, start + length). */
typedef struct {
    size_t start;              /* first codepoint of the link       */
    size_t length;             /* number of codepoints in the link  */
    uint8_t kind;              /* suldf_link_kind_t                 */
    uint8_t reserved0;         /* must be zero                      */
    uint16_t reserved1;        /* must be zero                      */
} suldf_link_span_t;

/* ── Fixed limits (static, stack-allocatable tables) ───────────────── */
/* The compiled term and email tables hold at most this many rows. The
 * capacities are compile-time checked against the shipped tables, so a data
 * update that overflows a table fails the build rather than the kernel. */
#define SULDF_TERM_CAPACITY      2048U
#define SULDF_EMAIL_CAPACITY     1536U
#define SULDF_BRACKET_CAPACITY   128U
#define SULDF_BRACKET_STACK_MAX  125U

/* ── Zone boundaries (duplicated for freestanding SULDF) ───────────── */
#define SULDF_LIMIT_UNICODE_MAX  0x0010FFFFUL
#define SULDF_LIMIT_SCP_MIN      0x00110000UL
#define SULDF_LIMIT_SCP_MAX      0x0011FFFFUL
#define SULDF_LIMIT_NATIVE_MIN   0x00120000UL
#define SULDF_LIMIT_NATIVE_MAX   0x7FFFFFEFUL
#define SULDF_LIMIT_TRAP_MIN     0x7FFFFFF0UL
#define SULDF_LIMIT_TRAP_MAX     0x7FFFFFFEUL
#define SULDF_LIMIT_SENTINEL     0x7FFFFFFFUL

/* The RNUR / RCCR neography carve-out inside the Native plane (SUAS-005). */
#define SULDF_RNUR_MIN           0x00160000UL
#define SULDF_RNUR_MAX           0x0016FFFFUL
#define SULDF_RNUR_SET_SHIFT     8U

/* ── Version and setup ─────────────────────────────────────────────── */
/* The version string, a NUL-terminated ASCII literal ("1.0"). */
const char* suts_suldf_version_string(void);

/* Initializes the static SULDF link detection engine. Idempotent and
 * allocation-free. Every query builds nothing and works without it; the call
 * exists so a linkifier can force initialization at a deterministic point and
 * so a future revision with build-time tables has a stable entry point. */
void suts_suldf_init_engine(void);

/* ── Attribute query (§3) ──────────────────────────────────────────── */
/* Evaluates a 31-bit codepoint's link attributes in O(1) time without heap
 * allocation or string parsing. The query is total over the 31-bit
 * SuperUnicode address space: every codepoint from 0x00000000 through the
 * Sentinel 0x7FFFFFFF has an answer, and an unlisted Bridge codepoint is
 * answered by the algorithmic default Link_Term=Hard of §3.4.1 rather than by
 * a lookup failure. On success `*out_attr` receives the compiled attribute and
 * the function returns true. It returns false - leaving `*out_attr` untouched
 * - only for a NULL output pointer or a codepoint above the Sentinel, which is
 * ExtSUCS plugin space and is not link material. */
bool suts_suldf_get_attributes(uint32_t codepoint, suldf_link_attr_t *out_attr);

/* ── Link property predicates (§3.4, §5.1) ─────────────────────────── */
/* The Link_Term category of the codepoint (O(1)). */
suldf_term_t suts_suldf_term_of(uint32_t codepoint);

/* True for Link_Email=Yes: a codepoint that may normally occur in the
 * unquoted local-part of an email address (O(1)). */
bool suts_suldf_is_email_char(uint32_t codepoint);

/* True for a codepoint that may occur in a URL scheme (O(1)). */
bool suts_suldf_is_scheme_char(uint32_t codepoint);

/* True for a codepoint that may occur in a domain label (O(1)). */
bool suts_suldf_is_domain_char(uint32_t codepoint);

/* True for a codepoint that may occur in a Path, Query, or Fragment (O(1)). */
bool suts_suldf_is_path_char(uint32_t codepoint);

/* True for the '@' email local-part delimiter (O(1)). */
bool suts_suldf_is_email_delim(uint32_t codepoint);

/* True for Link_Term=Open and Link_Term=Close respectively (O(1)). */
bool suts_suldf_is_open_bracket(uint32_t codepoint);
bool suts_suldf_is_close_bracket(uint32_t codepoint);

/* The Link_Bracket value of a Close codepoint, or 0 when the codepoint has no
 * mapping. `0` is never a valid Open bracket, so it is an unambiguous
 * sentinel (O(1)). */
uint32_t suts_suldf_open_bracket_for(uint32_t close_codepoint);

/* True when `close_codepoint` is paired with `open_codepoint` by
 * Link_Bracket (O(1)). */
bool suts_suldf_brackets_match(uint32_t close_codepoint,
                               uint32_t open_codepoint);

/* True for a directional formatting character: the explicit controls
 * (LRE, RLE, PDF, LRO, RLO), the implicit marks (LRM, RLM, ALM), and the
 * isolate initiators and terminator (LRI, RLI, FSI, PDI) (O(1)). A detected
 * link that contains one is a security finding under §8. */
bool suts_suldf_is_bidi_control(uint32_t codepoint);

/* ── Zone classification (§5) ──────────────────────────────────────── */
/* Classifies a 31-bit codepoint into its address-space zone (O(1)). A value
 * above the Sentinel is EXTSUCS. */
suldf_zone_t suts_suldf_zone_of(uint32_t codepoint);

/* True inside the RNUR neography carve-out (O(1)). */
bool suts_suldf_is_neography(uint32_t codepoint);

/* Set number of an RNUR coordinate: (cp - 0x00160000) >> 8. Returns false
 * outside the carve-out. */
bool suts_suldf_neography_set(uint32_t codepoint, uint32_t *out_set);

/* ── Stream scanning (§3, §5) ──────────────────────────────────────── */
/* Scans a codepoint stream for URLs and email addresses using the UTS #58
 * initiation, termination, and email algorithms in one linear pass.
 *
 * On return, up to `capacity` detected spans have been written to `out_spans`
 * in ascending, non-overlapping order. The return value is the total number of
 * links detected, independent of `capacity`, so a caller can size a buffer by
 * calling once with `out_spans == NULL` and `capacity == 0`. A zero-length
 * stream, a NULL stream, or a NULL output array detects nothing.
 *
 * A span carries its kind (URL or EMAIL) and half-open codepoint offsets. URL
 * spans include the scheme (when present), the host, the optional port, and
 * the terminated Path / Query / Fragment. Email spans include the local-part,
 * the '@', the domain, and a preceding case-insensitive "mailto:" when one is
 * immediately before the local-part. */
size_t suts_suldf_scan_stream(const uint32_t *stream, size_t length,
                              suldf_link_span_t *out_spans, size_t capacity);

/* ── Minimal escaping (§4.1) ───────────────────────────────────────── */
/* Serializes one URL Part's content (the initiator is not included) using the
 * UTS #58 minimal-escaping algorithm. `part_kind` selects the Part's
 * Terminator and ClearStackOpen sets. The caller is responsible for the
 * normal interior escaping of literal syntactic characters described in
 * UTS #58 §4.1.
 *
 * Codepoints are written to `out` as codepoints. A percent escape is emitted
 * as the three ASCII codepoints '%', hex, hex. Non-ASCII bytes use SUTF-8
 * (SUTR-1), so a codepoint above the Unicode Bridge is escaped by its full
 * byte sequence.
 *
 * The return value is the number of codepoints the complete result needs,
 * independent of `capacity` (the snprintf convention). If `out` is NULL or
 * `capacity` is too small, only the prefix that fits is written. */
size_t suts_suldf_escape_part(const uint32_t *part, size_t length,
                              suldf_part_t part_kind,
                              uint32_t *out, size_t capacity);

/* The position-aware form of suts_suldf_escape_part. `is_last` selects the
 * UTS #58 §4.1 trailing-soft rule: the final codepoint of the last Part is
 * escaped when it was skipped as Soft, while a non-last Part copies its
 * trailing Soft characters because the next Part's initiator follows. */ 
size_t suts_suldf_escape_part_ex(const uint32_t *part, size_t length,
                                 suldf_part_t part_kind, bool is_last,
                                 uint32_t *out, size_t capacity);

/* ── Email minimal quoting (§5.3) ──────────────────────────────────── */
/* True when an unquoted local-part is not valid according to Link_Email and
 * therefore needs the wrapped quoted form of UTS #58 §5.3. */
bool suts_suldf_email_local_needs_quotes(const uint32_t *local, size_t length);

/* Serializes an email local-part with minimal quoting. If every codepoint is
 * Link_Email=Yes, the local-part is copied unchanged. Otherwise it is wrapped
 * in ASCII double quotes and the two RFC 5322 specials (`"` and `\`) are
 * backslash-escaped. The return value follows the same snprintf convention as
 * suts_suldf_escape_part: the full output length in codepoints. */
size_t suts_suldf_quote_email_local(const uint32_t *local, size_t length,
                                    uint32_t *out, size_t capacity);

/* ── Taxonomy names ────────────────────────────────────────────────── */
const char* suts_suldf_term_name(suldf_term_t term);
const char* suts_suldf_bracket_name(suldf_bracket_t bracket);
const char* suts_suldf_zone_name(suldf_zone_t zone);
const char* suts_suldf_kind_name(suldf_link_kind_t kind);
const char* suts_suldf_part_name(suldf_part_t part);

#ifdef __cplusplus
}
#endif

#endif /* SUTS_SUTS_SULDF_H */
