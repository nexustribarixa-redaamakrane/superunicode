#ifndef SUAS_SUAS_SUEASD_H
#define SUAS_SUAS_SUEASD_H

/* SUAS-017 - SuperUnicode East Asian Script Data (SUEASD)
 *
 * The core data subsystem of the OpenWindows microkernel governing the
 * *classification and routing* of East Asian text: the primary script of a
 * codepoint (Han, Hangul, Hiragana, Katakana, Bopomofo and the UAX #60
 * covered scripts Jurchen, Nushu, Small Seal and Tangut), its regional source
 * class, its ideographic and kana/jamo roles, and the ancillary source,
 * radical-stroke, reading, numeric and modern-CJK data that UAX #60 publishes
 * for the East Asian scripts other than Han.
 *
 * UAX #60, "Data for East Asian Scripts", publishes the ancillary corpus as
 * four UTF-8 text manifests (JurchenSources.txt, NushuSources.txt,
 * SealSources.txt, TangutSources.txt) whose three-field, tab-separated rows
 * must be parsed, joined and interpreted at run time, while Scripts.txt
 * carries the primary script assignment. SUEASD is the runtime counterpart of
 * that data in the same dual-layer relationship SUAS-009 (SIP) has with
 * UAX #38, SUAS-013 (SISR) has with UAX #45, SUAS-014 (SUVTL) has with
 * UAX #50, SUAS-015 (SUAMR) has with UAX #53 and SUAS-016 (SUKEMET) has with
 * UAX #57:
 *
 *   - the offline reference layer is the SuperUnicode Character Database
 *     (SUCD, SUAS-012): the Scripts.txt classification, the four UAX #60
 *     source files, and the human-readable descriptions for font engineers;
 *   - the runtime kernel layer is this specification: a byte-packed 12-byte
 *     script record, a compact classification range table, a parallel
 *     metadata table, a fixed open-addressing index, and a bounded query
 *     surface.
 *
 * No part of the engine reaches for malloc, stdio or <string.h>.
 *
 * ---------------------------------------------------------------------------
 * This header is freestanding C99. `<stdint.h>`, `<stdbool.h>` and `<stddef.h>`
 * are the only standard headers it uses. All record layouts are byte-aligned:
 * sueasd_script_entry_t is exactly 12 bytes and sueasd_metadata_t exactly 24
 * bytes on every supported ABI.
 * ------------------------------------------------------------------------- */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "suas/suas_core.h"

#if !defined(__STDC_HOSTED__) || __STDC_HOSTED__ == 0
/* Freestanding environment confirmed. */
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * SUAS-017 Specification Identifiers
 * ------------------------------------------------------------------------- */
#define SUEASD_SPEC_ID          "SUAS-017"
#define SUEASD_SPEC_TITLE       "SuperUnicode East Asian Script Data"
#define SUEASD_VERSION_MAJOR    1
#define SUEASD_VERSION_MINOR    0
#define SUEASD_VERSION_PATCH    0

#define SUAS_SUEASD_SPEC_ID       SUEASD_SPEC_ID
#define SUAS_SUEASD_SPEC_TITLE    SUEASD_SPEC_TITLE
#define SUAS_SUEASD_VERSION_MAJOR SUEASD_VERSION_MAJOR
#define SUAS_SUEASD_VERSION_MINOR SUEASD_VERSION_MINOR
#define SUAS_SUEASD_VERSION_PATCH SUEASD_VERSION_PATCH

/* -- Fixed limits -------------------------------------------------------- */
/* The curated layer holds the UAX #60 ancillary records of Unicode 18.0.0:
 * 914 Jurchen, 396 Nushu, 11,328 Small Seal and 6,177 Tangut codepoints, plus
 * the East Asian codepoints that also carry a Han source declaration. The
 * capacity leaves room for a later UAX #60 revision without changing the
 * index geometry. It stays below the uint16 index-slot ceiling (65534). */
#define SUAS_SUEASD_REGISTRY_CAPACITY 32768U
#define SUAS_SUEASD_INDEX_SIZE        65536U
#define SUAS_SUEASD_RANGE_CAPACITY    256U

/* -- Base SUCS zone boundaries (duplicated for freestanding SUEASD) ------ */
#ifndef SUAS_SUEASD_LIMIT_UNICODE_MAX
#define SUAS_SUEASD_LIMIT_UNICODE_MAX 0x0010FFFFUL
#define SUAS_SUEASD_LIMIT_SCP_MIN     0x00110000UL
#define SUAS_SUEASD_LIMIT_SCP_MAX     0x0011FFFFUL
#define SUAS_SUEASD_LIMIT_NATIVE_MIN  0x00120000UL
#define SUAS_SUEASD_LIMIT_NATIVE_MAX  0x7FFFFFEFUL
#define SUAS_SUEASD_LIMIT_TRAP_MIN    0x7FFFFFF0UL
#define SUAS_SUEASD_LIMIT_TRAP_MAX    0x7FFFFFFEUL
#define SUAS_SUEASD_LIMIT_SENTINEL    0x7FFFFFFFUL
#endif

/* RNUR / RCCR neography carve-out inside the Native plane (SUAS-005 5.4).
 * The set number of an allocation is (cp - 0x00160000) >> 8. */
#define SUAS_SUEASD_RNUR_MIN          0x00160000UL
#define SUAS_SUEASD_RNUR_MAX          0x0016FFFFUL
#define SUAS_SUEASD_RNUR_SET_SHIFT    8U

/* -- Address-space zones (section 2) ------------------------------------- */
typedef enum {
    SUAS_SUEASD_ZONE_BRIDGE   = 0, /* 0x00000000-0x0010FFFF           */
    SUAS_SUEASD_ZONE_SCP      = 1, /* 0x00110000-0x0011FFFF           */
    SUAS_SUEASD_ZONE_RNUR     = 2, /* 0x00160000-0x0016FFFF           */
    SUAS_SUEASD_ZONE_NATIVE   = 3, /* rest of 0x00120000-0x7FFFFFEF   */
    SUAS_SUEASD_ZONE_TRAP     = 4, /* 0x7FFFFFF0-0x7FFFFFFE           */
    SUAS_SUEASD_ZONE_SENTINEL = 5, /* 0x7FFFFFFF                      */
    SUAS_SUEASD_ZONE_EXTSUCS  = 6  /* > 0x7FFFFFFF (plugin space)     */
} suas_sueasd_zone_t;

/* -------------------------------------------------------------------------
 * East Asian Script Identifiers
 * ------------------------------------------------------------------------- */
/* The sketch's five East Asian scripts, verbatim and in order, followed by
 * the four scripts UAX #60 covers (the "covered scripts") and the one further
 * East Asian script that Unicode 18.0.0 encodes but UAX #60 does not yet
 * surface (Khitan Small Script). */
typedef enum {
    SUEASD_SCRIPT_UNKNOWN = 0,
    SUEASD_SCRIPT_HAN     = 1, /* Chinese / CJK Ideographs             */
    SUEASD_SCRIPT_HANGUL  = 2, /* Korean Syllables and Jamo            */
    SUEASD_SCRIPT_HIRAGANA= 3, /* Japanese Hiragana                    */
    SUEASD_SCRIPT_KATAKANA= 4, /* Japanese Katakana                    */
    SUEASD_SCRIPT_BOPOMOFO= 5, /* Zhuyin Phonetic symbols              */
    SUEASD_SCRIPT_JURCHEN = 6, /* Jurchen (UAX #60 covered script)     */
    SUEASD_SCRIPT_NUSHU   = 7, /* Nushu  (UAX #60 covered script)      */
    SUEASD_SCRIPT_SEAL    = 8, /* Small Seal (UAX #60 covered script)  */
    SUEASD_SCRIPT_TANGUT  = 9, /* Tangut (UAX #60 covered script)      */
    SUEASD_SCRIPT_KHITAN_SMALL = 10, /* Khitan Small Script              */
    SUEASD_SCRIPT_COUNT   = 11
} sueasd_script_id_t;

/* -------------------------------------------------------------------------
 * Script Property Flags (Packed Bitfields)
 * ------------------------------------------------------------------------- */
/* The seeding sketch's flag names, verbatim. The two macros that the sketch
 * printed without an '=' are defined here with the intended shift values. */
#define SUEASD_FLAG_IDEOGRAPHIC  (1U << 0) /* Participates in CJK grid and justification rules */
#define SUEASD_FLAG_RADICAL      (1U << 1) /* Serves as a primary CJK radical component */
#define SUEASD_FLAG_COMPATIBILITY (1U << 2) /* Compatibility ideograph variant */
#define SUEASD_FLAG_CUSTOM_SCRIPT (1U << 3) /* Neography / Conscript ideographic extension */

/* Additional orthogonal flags SUEASD assigns (section 3.3). */
#define SUEASD_FLAG_HAN_CORE      (1U << 4)  /* Unified ideograph (UAX #38 core)  */
#define SUEASD_FLAG_HAN_RADICAL   (1U << 5)  /* CJK/Kangxi Radical block          */
#define SUEASD_FLAG_HAN_COMPAT    (1U << 6)  /* Compatibility ideograph           */
#define SUEASD_FLAG_KANA          (1U << 7)  /* Hiragana or Katakana              */
#define SUEASD_FLAG_JAMO          (1U << 8)  /* Hangul Jamo                       */
#define SUEASD_FLAG_SYLLABLE      (1U << 9)  /* Precomposed Hangul syllable       */
#define SUEASD_FLAG_SOURCE_DATA   (1U << 10) /* Has UAX #60 ancillary source data */
#define SUEASD_FLAG_READING       (1U << 11) /* Has a reading (Jurchen/Nushu)     */
#define SUEASD_FLAG_RS            (1U << 12) /* Has radical-stroke data           */
#define SUEASD_FLAG_NUMERIC       (1U << 13) /* Has a numeric value               */
#define SUEASD_FLAG_MODERN_CJK    (1U << 14) /* Small Seal: kSEAL_MCJK present    */
#define SUEASD_FLAG_NEOGRAPHY     (1U << 15) /* Native & Neography Plane          */

#define SUEASD_FLAG_ASSIGNED_MASK 0xFFFFU

/* -------------------------------------------------------------------------
 * Regional source classes (the record's source_region byte)
 * ------------------------------------------------------------------------- */
typedef enum {
    SUEASD_REGION_NONE    = 0, /* No regional source class               */
    SUEASD_REGION_CHINA   = 1, /* Han, Bopomofo, Source Han traditions   */
    SUEASD_REGION_KOREA   = 2, /* Hangul                                 */
    SUEASD_REGION_JAPAN   = 3, /* Hiragana, Katakana                     */
    SUEASD_REGION_JURCHEN = 4, /* Jurchen                                */
    SUEASD_REGION_NUSHU   = 5, /* Nushu                                  */
    SUEASD_REGION_SEAL    = 6, /* Small Seal                             */
    SUEASD_REGION_TANGUT  = 7, /* Tangut                                 */
    SUEASD_REGION_KHITAN  = 8  /* Khitan Small Script                    */
} sueasd_region_t;

/* -------------------------------------------------------------------------
 * UAX #60 ancillary source mask (the metadata record's source_mask)
 * ------------------------------------------------------------------------- */
#define SUEASD_SOURCE_JURC_NCREADING (1U << 0)  /* kJURC_NCReading       */
#define SUEASD_SOURCE_JURC_NUMERIC   (1U << 1)  /* kJURC_Numeric         */
#define SUEASD_SOURCE_JURC_RSUNICODE (1U << 2)  /* kJURC_RSUnicode       */
#define SUEASD_SOURCE_JURC_SRC       (1U << 3)  /* kJURC_Src             */
#define SUEASD_SOURCE_NSHU_DUBENSRC  (1U << 4)  /* kNSHU_DubenSrc        */
#define SUEASD_SOURCE_NSHU_READING   (1U << 5)  /* kNSHU_Reading         */
#define SUEASD_SOURCE_SEAL_CCZSRC    (1U << 6)  /* kSEAL_CCZSrc          */
#define SUEASD_SOURCE_SEAL_DYCSRC    (1U << 7)  /* kSEAL_DYCSrc          */
#define SUEASD_SOURCE_SEAL_MCJK      (1U << 8)  /* kSEAL_MCJK            */
#define SUEASD_SOURCE_SEAL_QJZSRC    (1U << 9)  /* kSEAL_QJZSrc          */
#define SUEASD_SOURCE_SEAL_RAD       (1U << 10) /* kSEAL_Rad             */
#define SUEASD_SOURCE_SEAL_THXSRC    (1U << 11) /* kSEAL_THXSrc          */
#define SUEASD_SOURCE_TGT_MERGEDSRC  (1U << 12) /* kTGT_MergedSrc        */
#define SUEASD_SOURCE_TGT_NUMERIC    (1U << 13) /* kTGT_Numeric          */
#define SUEASD_SOURCE_TGT_RSUNICODE  (1U << 14) /* kTGT_RSUnicode        */
#define SUEASD_SOURCE_HAN            (1U << 15) /* A Han source (UAX #38) */

/* -------------------------------------------------------------------------
 * Verification outcomes
 * ------------------------------------------------------------------------- */
typedef enum {
    SUEASD_REGISTRY_OK = 0,
    SUEASD_REGISTRY_NULL_TABLE,            /* count > 0, pointer NULL       */
    SUEASD_REGISTRY_COUNT_OVER,            /* count > static capacity       */
    SUEASD_REGISTRY_UNORDERED,             /* codepoints not ascending      */
    SUEASD_REGISTRY_DUPLICATE_CODEPOINT,   /* same codepoint twice          */
    SUEASD_REGISTRY_OUT_OF_SPACE,          /* codepoint > 0x7FFFFFFF        */
    SUEASD_REGISTRY_INADMISSIBLE_CODEPOINT,/* SCP, trap or Sentinel         */
    SUEASD_REGISTRY_BAD_SCRIPT,            /* primary_script out of range   */
    SUEASD_REGISTRY_BAD_REGION,            /* source_region out of range    */
    SUEASD_REGISTRY_BAD_RESERVED,          /* reserved byte not zero        */
    SUEASD_REGISTRY_IDEOGRAPH_MISMATCH,    /* IDEOGRAPHIC flag/script differ */
    SUEASD_REGISTRY_KANA_MISMATCH,         /* KANA flag outside kana script */
    SUEASD_REGISTRY_JAMO_MISMATCH,         /* JAMO and SYLLABLE together    */
    SUEASD_REGISTRY_RANGE_UNORDERED,       /* classification ranges overlap */
    SUEASD_REGISTRY_RANGE_OUT_OF_SPACE     /* a range endpoint past 0x7FFF..*/
} sueasd_registry_error_t;

/* -------------------------------------------------------------------------
 * Structural Types (Zero-Allocation)
 * ------------------------------------------------------------------------- */
/* The seeding sketch's fields, verbatim and in order. Exactly 12 bytes, no
 * padding on any supported ABI. */
typedef struct {
    uint32_t codepoint;          /* Raw 31-bit SuperUnicode codepoint      */
    uint32_t script_mask;        /* Packed script property and behavior flags */
    uint16_t primary_script;     /* Assigned East Asian script identifier  */
    uint8_t  source_region;      /* Regional source tag                    */
    uint8_t  reserved;           /* must be zero                           */
} sueasd_script_entry_t;

/* -- A classification range --------------------------------------------- */
/* The Scripts.txt-derived classification is a list of disjoint ranges. A
 * range names the script and the base flags every codepoint in it shares;
 * a codepoint's full record is the range's base record refined by the
 * curated metadata when one exists. */
typedef struct {
    uint32_t first;              /* First codepoint of the range           */
    uint32_t last;               /* Last codepoint of the range            */
    uint16_t primary_script;     /* sueasd_script_id_t                     */
    uint16_t base_flags;         /* SUEASD_FLAG_* shared by the range      */
} sueasd_range_t;

/* -- The parallel UAX #60 metadata record ------------------------------- */
/* The 12-byte sketch record has no room for the source mask, the
 * radical-stroke counts, the numeric value, the hashes of the informative
 * fields or the modern-CJK equivalence. SUEASD keeps them in a parallel table
 * aligned with the main table, exactly as SUKEMET keeps the Unikemet
 * catalogue beside its sign record. Exactly 24 bytes. */
typedef struct {
    uint16_t source_mask;        /* SUEASD_SOURCE_*                        */
    uint16_t radical;            /* Radical/component number (0 if absent) */
    uint16_t numeric_value;      /* Integer numeric value (0 if absent)    */
    uint8_t  stroke_count;       /* Total strokes (0 if absent)            */
    uint8_t  source_count;       /* Number of distinct UAX #60 rows        */
    uint32_t reading_hash;       /* FNV-1a over the primary reading, 0 if absent */
    uint32_t source_hash;        /* FNV-1a over the primary source id, 0 if absent */
    uint32_t aux_hash;           /* FNV-1a over the raw RS/Rad value, 0 if absent */
    uint32_t modern_cjk;         /* kSEAL_MCJK modern CJK codepoint, 0 if absent */
} sueasd_metadata_t;

/* -------------------------------------------------------------------------
 * Setup
 * ------------------------------------------------------------------------- */
const char* suas_sueasd_version_string(void);

/**
 * Initializes the static SUEASD East Asian script data registry and lookup
 * tables. Idempotent and allocation-free; the query entry points build the
 * index lazily on first use if this is not called first.
 */
void suas_sueasd_init_registry(void);

/* -------------------------------------------------------------------------
 * Function Declarations
 * ------------------------------------------------------------------------- */

/**
 * Queries pre-compiled script classifications and ideographic properties
 * in O(1) time without heap allocation or string parsing.
 *
 * On success `*out_entry` receives a copy of the record and the function
 * returns true. The property is total over the East Asian repertoire: a
 * codepoint in any classification range resolves to the range's base record,
 * refined by the curated UAX #60 metadata when one exists. A codepoint in the
 * Native & Neography Plane carve-out resolves to a synthesized default. The
 * function returns false only for an inadmissible zone (SCP, trap, Sentinel,
 * ExtSUCS) or a NULL output, leaving the output untouched.
 */
bool suas_sueasd_get_entry(uint32_t codepoint, sueasd_script_entry_t *out_entry);

/* The parallel UAX #60 metadata for a codepoint (O(1)). For a codepoint
 * without curated ancillary data the metadata is all-zero. Returns false
 * under the same conditions as suas_sueasd_get_entry. */
bool suas_sueasd_get_metadata(uint32_t codepoint, sueasd_metadata_t *out_metadata);

/* -- Classification predicates ------------------------------------------ */
/* True for a codepoint that has a curated UAX #60 ancillary record (O(1)). */
bool suas_sueasd_is_curated(uint32_t codepoint);

/* True for any codepoint the SUEASD property is defined on: a member of a
 * classification range or the RNUR carve-out (O(1)). */
bool suas_sueasd_is_east_asian(uint32_t codepoint);

/* True when the record is an ideographic (Han-family) codepoint. */
bool suas_sueasd_is_ideographic(uint32_t codepoint);

/* True when the record is a CJK radical component. */
bool suas_sueasd_is_radical(uint32_t codepoint);

/* True when the record is a compatibility ideograph. */
bool suas_sueasd_is_compatibility(uint32_t codepoint);

/* True when the record is a neography / conscript declaration. */
bool suas_sueasd_is_custom_script(uint32_t codepoint);

/* True when the record is Hiragana or Katakana. */
bool suas_sueasd_is_kana(uint32_t codepoint);

/* True when the record is Hangul Jamo. */
bool suas_sueasd_is_jamo(uint32_t codepoint);

/* True when the record is a precomposed Hangul syllable. */
bool suas_sueasd_is_syllable(uint32_t codepoint);

/* True when the record carries UAX #60 ancillary source data. */
bool suas_sueasd_has_source_data(uint32_t codepoint);

/* True inside the Native & Neography Plane carve-out (O(1)). */
bool suas_sueasd_is_neography(uint32_t codepoint);

/* -- Property queries --------------------------------------------------- */
/* The primary East Asian script of a record (SUEASD_SCRIPT_UNKNOWN when the
 * codepoint is not East Asian). */
uint16_t suas_sueasd_primary_script(uint32_t codepoint);

/* The packed property flags of a record (0 when the codepoint is not East
 * Asian). */
uint32_t suas_sueasd_script_mask(uint32_t codepoint);

/* The regional source class of a record. */
uint8_t suas_sueasd_source_region(uint32_t codepoint);

/* The UAX #60 ancillary source mask (SUEASD_SOURCE_*). */
uint16_t suas_sueasd_source_mask(uint32_t codepoint);

/* The radical/component number (0 when absent). */
uint16_t suas_sueasd_radical(uint32_t codepoint);

/* The total stroke count, including the radical (0 when absent). */
uint8_t suas_sueasd_stroke_count(uint32_t codepoint);

/* The integer numeric value (0 when absent). */
uint16_t suas_sueasd_numeric_value(uint32_t codepoint);

/* The number of distinct UAX #60 rows carried by a record. */
uint8_t suas_sueasd_source_count(uint32_t codepoint);

/* The modern CJK equivalence of a Small Seal codepoint (0 when absent). */
uint32_t suas_sueasd_modern_cjk(uint32_t codepoint);

/* Hashes of the informative UAX #60 fields (0 when absent). */
uint32_t suas_sueasd_reading_hash(uint32_t codepoint);
uint32_t suas_sueasd_source_hash(uint32_t codepoint);
uint32_t suas_sueasd_aux_hash(uint32_t codepoint);

/* -- Enumeration -------------------------------------------------------- */
/* Number of curated ancillary records in the static table (O(1)). */
uint32_t suas_sueasd_entry_count(void);

/* Copy the i-th curated record (0-based, ascending codepoint order). O(1). */
bool suas_sueasd_entry_at(uint32_t index, sueasd_script_entry_t *out_entry);

/* Copy the i-th curated metadata record (0-based, same order). */
bool suas_sueasd_metadata_at(uint32_t index, sueasd_metadata_t *out_metadata);

/* Number of classification ranges in the static range table (O(1)). */
uint32_t suas_sueasd_range_count(void);

/* Copy the i-th classification range (0-based, ascending order). O(1). */
bool suas_sueasd_range_at(uint32_t index, sueasd_range_t *out_range);

/* -- Zone and block predicates (section 2) ------------------------------ */
/* Classifies a 31-bit codepoint into its address-space zone (O(1)). */
suas_sueasd_zone_t suas_sueasd_zone_of(uint32_t codepoint);

/* True for the zones a SUEASD query may name: Bridge, RNUR and Native. */
bool suas_sueasd_is_text_space(uint32_t codepoint);

/* Set number of an RNUR coordinate: (cp - 0x00160000) >> 8. Returns false
 * outside the carve-out. */
bool suas_sueasd_neography_set(uint32_t codepoint, uint32_t *out_set);

/* -- Hash --------------------------------------------------------------- */
/* FNV-1a 32-bit hash over the little-endian encoding of the codepoint (O(1)).
 * This is the index key, not an identity. */
uint32_t suas_sueasd_hash(uint32_t codepoint);

/* FNV-1a 32-bit hash over an arbitrary byte string (O(n)). This is the
 * function used to precompute `reading_hash`, `source_hash` and `aux_hash`
 * from the UTF-8 UAX #60 fields. */
uint32_t suas_sueasd_hash_bytes(const char *bytes, size_t length);

/* -- Taxonomy names ----------------------------------------------------- */
const char* suas_sueasd_script_name(uint16_t script);
const char* suas_sueasd_script_short_name(uint16_t script);
const char* suas_sueasd_region_name(uint8_t region);
const char* suas_sueasd_source_name(uint16_t source_bit);
const char* suas_sueasd_zone_name(suas_sueasd_zone_t zone);
const char* suas_sueasd_registry_error_name(sueasd_registry_error_t error);

/* -- Verification (section 8) ------------------------------------------- */
/* Verify a compiled script table. `entries` may be NULL only when `count` is
 * zero (an empty registry is conformant). On failure, `*out_error` receives
 * the first defect and the function returns false; on success `*out_error` is
 * SUEASD_REGISTRY_OK. Either output pointer may be NULL. */
bool suas_sueasd_validate_table(const sueasd_script_entry_t *entries,
                                uint32_t count,
                                sueasd_registry_error_t *out_error);

/* Verify a compiled classification range table. Ranges must be ordered and
 * disjoint. `ranges` may be NULL only when `count` is zero. */
bool suas_sueasd_validate_ranges(const sueasd_range_t *ranges,
                                 uint32_t count,
                                 sueasd_registry_error_t *out_error);

#ifdef __cplusplus
}
#endif

#endif /* SUAS_SUAS_SUEASD_H */
