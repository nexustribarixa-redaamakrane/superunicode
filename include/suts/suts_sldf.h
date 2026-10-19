#ifndef SUTS_SUTS_SLDF_H
#define SUTS_SUTS_SLDF_H

/* SUTS-003 — System Locale Data & Formatting (SLDF)
 *
 * The core OpenWindows kernel primitive governing locale-specific formatting:
 * number systems, date/time patterns, and collation seeds. SLDF compiles locale
 * definitions into an immutable, statically-linked binary table of fixed-size,
 * byte-aligned C structures (sldf_locale_t). A subsystem reads the decimal
 * separator, the negative-sign placement, or the date order directly from a
 * fixed memory offset; no markup is parsed and no buffer is allocated.
 *
 * Where a user-space internationalization layer such as UTS #35 LDML is a
 * text (XML) schema parsed at run time into heap-resident object trees, SLDF:
 *
 *   - is a compiled binary schema — fixed structs in read-only memory, not
 *     markup, so every field sits at a known offset with no parser;
 *   - obeys the zero-allocation mandate — lookup is an O(1) probe of a
 *     fixed-size open-addressing index built once over a static table, and
 *     never touches a heap;
 *   - is a kernel primitive — the microkernel, system loggers, and terminal
 *     drivers read formatting rules in-band, with no external library, user
 *     process, or service dependency.
 *
 * An RCCR subtag (the Reddit Conlang Code Registry, a BCP 47-style code
 * registry that allocates no codepoints) selects a block. The registry holds
 * natural-language locales plus registered conlang locales (for example
 * `lumer` and `kupon`); the tag is normalized, hashed with FNV-1a, and the
 * matching sldf_locale_t is returned directly.
 *
 * Freestanding C99, zero allocation: the registry is a static array of
 * sldf_locale_t and the lookup index is a fixed-size array of slots, neither of
 * which grows at run time.
 *
 * See docs/suts/SUTS-003-sldf.md for the normative specification.
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Specification identifiers ────────────────────────────────────── */
#define SUTS_SLDF_SPEC_ID        "SUTS-003"
#define SUTS_SLDF_SPEC_TITLE     "System Locale Data & Formatting Specification"
#define SUTS_SLDF_VERSION_MAJOR  1
#define SUTS_SLDF_VERSION_MINOR  0
#define SUTS_SLDF_VERSION_PATCH  0

/* ── Fixed limits (static, stack-allocatable blocks) ──────────────── */
/* Maximum bytes of an RCCR subtag key, including the terminating NUL. */
#define SUTS_SLDF_MAX_TAG_LEN       16U
/* Maximum codepoints in a currency-symbol token, including the terminator. */
#define SUTS_SLDF_MAX_SYMBOL_LEN    8U
/* Number of curated locale blocks the static registry can hold. */
#define SUTS_SLDF_REGISTRY_CAPACITY 64U
/* Number of open-addressing slots (a power of two > capacity). */
#define SUTS_SLDF_INDEX_SIZE        128U

/* ── 64-bit ExtSUCS codepoint type (guarded to coexist) ───────────── */
#ifndef EXTSUCS_CHAR_T_DEFINED
#define EXTSUCS_CHAR_T_DEFINED
typedef uint64_t sucs_ex_char_t;
#endif

/* ── Base SUCS zone boundaries (duplicated for freestanding SLDF) ─── */
#ifndef SUTS_SLDF_ZONE_UNICODE_MAX
#define SLDF_ZONE_UNICODE_MAX  ((sucs_ex_char_t)0x0010FFFFULL)
#define SLDF_ZONE_SCP_MIN      ((sucs_ex_char_t)0x00110000ULL)
#define SLDF_ZONE_SCP_MAX      ((sucs_ex_char_t)0x0011FFFFULL)
#define SLDF_ZONE_NATIVE_MIN   ((sucs_ex_char_t)0x00120000ULL)
#define SLDF_ZONE_NATIVE_LAST  ((sucs_ex_char_t)0x7FFFFFEFULL)
#define SLDF_ZONE_TRAP_MIN     ((sucs_ex_char_t)0x7FFFFFF0ULL)
#define SLDF_ZONE_TRAP_MAX     ((sucs_ex_char_t)0x7FFFFFFEULL)
#define SLDF_ZONE_SENTINEL     ((sucs_ex_char_t)0x7FFFFFFFULL)
#endif

/* RNUR / RCCR neography carve-out inside the Native plane (SUTR-8 §1.4).
 * RCCR allocates no codepoints; the RNUR registry supplies the compound
 * glyphs and logographs that a conlang locale formats with. */
#define SLDF_RNUR_MIN          ((sucs_ex_char_t)0x00160000ULL)
#define SLDF_RNUR_MAX          ((sucs_ex_char_t)0x0016FFFFULL)

/* ── Collation seeds (SLDF collation indexing domain, §2.4) ───────── */
/* A locale's collation index is `seed(plane) + collation_base_offset`, so
 * the seed maps the locale onto the 31-bit SUCS address space (Bridge) or the
 * RNUR neography plane without ever allocating a collation table. */
#define SLDF_COLLATION_BRIDGE_SEED ((uint32_t)0x00000000u)
#define SLDF_COLLATION_SCP_SEED    ((uint32_t)0x00110000u)
#define SLDF_COLLATION_NATIVE_SEED ((uint32_t)0x00120000u)
#define SLDF_COLLATION_RNUR_SEED   ((uint32_t)0x00160000u)

/* ── FNV-1a constants for the RCCR tag hash (§3.2) ────────────────── */
#define SUTS_SLDF_FNV_OFFSET_BASIS ((uint32_t)2166136261u)
#define SUTS_SLDF_FNV_PRIME        ((uint32_t)16777619u)

/* ── Date order vectors (§2.3) ────────────────────────────────────── */
typedef enum {
    SUTS_SLDF_DATE_YMD = 0, /* Year-Month-Day */
    SUTS_SLDF_DATE_MDY = 1, /* Month-Day-Year */
    SUTS_SLDF_DATE_DMY = 2  /* Day-Month-Year */
} suts_sldf_date_order_t;

/* ── Affix position for currency symbols and the sign (§2.2) ──────── */
typedef enum {
    SUTS_SLDF_AFFIX_PREFIX = 0, /* symbol/sign leads the number */
    SUTS_SLDF_AFFIX_SUFFIX = 1  /* symbol/sign trails the number */
} suts_sldf_affix_t;

/* ── Address-space planes for collation mapping (§5) ──────────────── */
typedef enum {
    SUTS_SLDF_PLANE_BRIDGE   = 0, /* 0x00000000-0x0010FFFF           */
    SUTS_SLDF_PLANE_SCP      = 1, /* 0x00110000-0x0011FFFF           */
    SUTS_SLDF_PLANE_NATIVE   = 2, /* 0x00120000-0x7FFFFFEF           */
    SUTS_SLDF_PLANE_RNUR     = 3, /* 0x00160000-0x0016FFFF           */
    SUTS_SLDF_PLANE_TRAP     = 4, /* 0x7FFFFFF0-0x7FFFFFFE           */
    SUTS_SLDF_PLANE_SENTINEL = 5, /* 0x7FFFFFFF                      */
    SUTS_SLDF_PLANE_EXTSUCS  = 6  /* > 0x7FFFFFFF (plugin space)     */
} suts_sldf_plane_t;

/* ── Status codes ─────────────────────────────────────────────────── */
typedef enum {
    SUTS_SLDF_OK                =  0,
    SUTS_SLDF_NOT_FOUND         =  1,
    SUTS_SLDF_ERR_INVALID_ARG   = -1,
    SUTS_SLDF_ERR_BAD_TAG       = -2,
    SUTS_SLDF_ERR_OVERFLOW      = -3
} suts_sldf_status_t;

/* ── Binary-compiled locale structures (zero-allocation) ──────────── */

/* Numeric formatting domain (§2.2). All separators and symbols are
 * codepoints, never `char`, so every SuperUnicode separator (Arabic decimal
 * separator U+066B, narrow no-break space U+202F, an RNUR glyph, ...) survives
 * intact. A currency symbol is a fixed codepoint array terminated by 0. */
typedef struct {
    sucs_ex_char_t decimal_separator;                     /* U+002E, U+066B, ... */
    sucs_ex_char_t grouping_separator;                    /* U+002C, U+202F, ... */
    uint8_t        grouping_primary;                      /* digits in the first (rightmost) group   */
    uint8_t        grouping_secondary;                    /* digits in later groups; 0 = unlimited   */
    sucs_ex_char_t currency_symbol[SUTS_SLDF_MAX_SYMBOL_LEN]; /* 0-terminated                      */
    uint8_t        currency_symbol_length;                /* meaningful codepoints (no terminator)   */
    uint8_t        currency_position;                     /* suts_sldf_affix_t                       */
    uint8_t        currency_space;                        /* 1 = significant space at the symbol side */
    sucs_ex_char_t minus_sign;                            /* U+002D, U+2212, ...                     */
    uint8_t        minus_position;                        /* suts_sldf_affix_t                       */
    uint8_t        negative_parenthesized;                /* 1 = accounting style, e.g. (1,234)      */
} sldf_numeric_format_t;

/* Temporal formatting domain (§2.3). `date_order` is an order vector, not a
 * formatted string: it names which component leads, and the separators and the
 * 24-hour flag complete the pattern without storing a pattern string. */
typedef struct {
    uint8_t        date_order;        /* suts_sldf_date_order_t                  */
    sucs_ex_char_t date_separator;    /* U+002F, U+002D, U+002E, ...             */
    sucs_ex_char_t time_separator;    /* U+003A                                  */
    uint8_t        use_24_hour;       /* 1 = 24-hour cycle, 0 = 12-hour cycle    */
    uint8_t        first_day_of_week; /* 0 = Sunday .. 6 = Saturday              */
    uint8_t        weekend_start;     /* first weekend day, same index           */
    uint8_t        weekend_end;       /* second weekend day, same index          */
    uint8_t        long_month_name;   /* 1 = long month form, 0 = numeric        */
} sldf_temporal_format_t;

/* The compiled locale block (§2.1). `tag_key` is the RCCR subtag that selects
 * the block; it lives in read-only memory with the rest of the table. */
typedef struct {
    char                   tag_key[SUTS_SLDF_MAX_TAG_LEN]; /* RCCR subtag key, NUL-terminated */
    sldf_numeric_format_t  numeric;
    sldf_temporal_format_t temporal;
    uint32_t               collation_base_offset;          /* seed added to the plane base    */
    uint8_t                collation_plane;                /* suts_sldf_plane_t               */
    uint8_t                is_rtl;                         /* 1 = RTL script (SDF hint)       */
    uint8_t                script;                         /* SSP script-id hint (opaque)     */
    uint8_t                reserved;
} sldf_locale_t;

/* ── Function declarations ────────────────────────────────────────── */

/* Version string, e.g. "1.0.0". */
const char* sldf_version_string(void);

/* Builds (or rebuilds) the SLDF kernel locale registry index. Idempotent and
 * allocation-free; the resolve entry point builds it lazily on first use if
 * this is not called first. */
void sldf_init_registry(void);

/* Resolves a compiled binary locale structure by its RCCR subtag key in O(1)
 * expected time. Returns a direct pointer to the immutable kernel data block,
 * or NULL when the tag is absent or malformed. The returned pointer is stable
 * for the lifetime of the image and must never be freed. */
const sldf_locale_t* sldf_resolve_locale(const char* rccr_tag);

/* ── Enumeration ──────────────────────────────────────────────────── */
/* Number of blocks in the static registry (O(1)). */
size_t sldf_locale_count(void);

/* Direct, stable pointer to registry block `index`, or NULL when out of range
 * (O(1)). The order is the static table order and is stable across builds. */
const sldf_locale_t* sldf_locale_at(size_t index);

/* ── RCCR tag helpers ─────────────────────────────────────────────── */
/* FNV-1a 32-bit hash over the normalized (lower-cased) tag bytes. */
uint32_t sldf_tag_hash(const char* tag);

/* True when `tag` is a well-formed RCCR subtag: 1..15 bytes of ASCII letters,
 * digits, and medial HYPHEN-MINUS, with a letter in the first position. */
bool sldf_is_valid_tag(const char* tag);

/* Case-insensitive, hyphen-normalized tag comparison (R2). */
bool sldf_tags_equal(const char* a, const char* b);

/* ── Plane mapping and collation seed ─────────────────────────────── */
/* Classifies a codepoint into its address-space plane (O(1), §5). */
suts_sldf_plane_t sldf_plane_of(sucs_ex_char_t cp);

/* The locale's collation index base: `seed(collation_plane) +
 * collation_base_offset` (O(1), §2.4). Returns 0 for a NULL locale. */
uint32_t sldf_collation_base(const sldf_locale_t* locale);

#ifdef __cplusplus
}
#endif

#endif /* SUTS_SUTS_SLDF_H */
