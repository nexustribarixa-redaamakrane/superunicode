#ifndef SUAS_SUAS_SUKEMET_H
#define SUAS_SUAS_SUKEMET_H

/* SUAS-016 - SuperUnicode Egyptian Hieroglyph Database (SUKEMET)
 *
 * The core data subsystem of the OpenWindows microkernel governing the
 * *classification, identity and layout role* of Egyptian hieroglyphs: the
 * Gardiner group a sign belongs to, its Core/Legacy status, its function, and
 * the format controls that build cartouches, enclosures, insertions, overlays
 * and stacked compounds.
 *
 * UAX #57, "Unicode Egyptian Hieroglyph Database (Unikemet)", publishes the
 * corpus as a UTF-8 text manifest (Unikemet.txt) whose properties must be
 * parsed, matched and interpreted at run time. SUKEMET is the runtime
 * counterpart of that database in the same dual-layer relationship SUAS-009
 * (SIP) has with UAX #38, SUAS-013 (SISR) has with UAX #45, SUAS-014 (SUVTL)
 * has with UAX #50 and SUAS-015 (SUAMR) has with UAX #53:
 *
 *   - the offline reference layer is the SuperUnicode Character Database
 *     (SUCD, SUAS-012): the Unikemet catalogue, the Gardiner taxonomy, the
 *     Core set, the mirror/rotate properties and the alternate sequences,
 *     together with the human-readable descriptions for Egyptologists and
 *     font engineers;
 *   - the runtime kernel layer is this header: a byte-packed 12-byte sign
 *     record, a parallel metadata table, a fixed open-addressing index and a
 *     bounded query surface.
 *
 * Three commitments separate SUKEMET from a text-manifest reader:
 *
 *   1. ONE RECORD. A hieroglyph's structural identity is a fixed 12-byte
 *      descriptor, not a row parsed from Unikemet.txt. Every field is
 *      byte-aligned, and the record is proven to be 12 bytes.
 *   2. A BOUNDED LOOKUP. A query hashes the codepoint, probes a fixed index
 *      and returns the record - O(1) expected, a small constant number of
 *      probes, no allocation, no libc.
 *   3. A TOTAL REPERTOIRE. Every codepoint in the Egyptian Hieroglyphs,
 *      Egyptian Hieroglyph Format Controls and Egyptian Hieroglyphs Extended-A
 *      blocks, plus the Native & Neography Plane carve-out, resolves to a
 *      record - curated or synthesized - so an epigraphic renderer never has
 *      to special-case a missing row.
 *
 * SUKEMET does not re-implement SUAS-006 (STS). STS decides where a grapheme
 * or cluster begins and ends; SUKEMET classifies the signs and controls the
 * caller supplies. The two compose; neither re-derives the other.
 *
 * SUAS-016 is registered as a mandatory architecture specification. The
 * engine, the record layout, the Gardiner taxonomy and the verification rules
 * are architectural; the registry *contents* are version data (an
 * implementation with an empty table still conforms, answering every
 * repertoire codepoint by the synthesized default of section 6).
 *
 * Freestanding C99, zero allocation: the table is static, every descriptor is
 * a fixed-size struct the caller owns, and no part of the engine reaches for
 * malloc, stdio or libc string routines.
 *
 * See docs/suas/SUAS-016-sukemet.md for the normative specification.
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "suas/suas_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -- Specification identifiers ------------------------------------------- */
#define SUKEMET_SPEC_ID           "SUAS-016"
#define SUKEMET_SPEC_TITLE        "SuperUnicode Egyptian Hieroglyph Database"
#define SUKEMET_VERSION_MAJOR     1
#define SUKEMET_VERSION_MINOR     0
#define SUKEMET_VERSION_PATCH     0

#define SUAS_SUKEMET_SPEC_ID      SUKEMET_SPEC_ID
#define SUAS_SUKEMET_SPEC_TITLE   SUKEMET_SPEC_TITLE
#define SUAS_SUKEMET_VERSION_MAJOR SUKEMET_VERSION_MAJOR
#define SUAS_SUKEMET_VERSION_MINOR SUKEMET_VERSION_MINOR
#define SUAS_SUKEMET_VERSION_PATCH SUKEMET_VERSION_PATCH

/* -- Fixed limits -------------------------------------------------------- */
/* The shipped registry holds the 5,067 Unikemet 18.0.0 signs, the 38
 * Egyptian format controls and modifiers, and the Native/neography
 * declarations. The capacity leaves room for a later Unikemet revision
 * without changing the index geometry. */
#define SUAS_SUKEMET_REGISTRY_CAPACITY 8192U
#define SUAS_SUKEMET_INDEX_SIZE        16384U

/* The alternate-sequence store is a flattened uint32_t array. Unikemet 18.0.0
 * declares at most seven codepoints in any kEH_AltSeq; the capacity is the
 * total number of alternate codepoints in the curated roster plus slack. */
#define SUAS_SUKEMET_ALT_CAPACITY      512U
#define SUAS_SUKEMET_ALT_MAX           16U

/* -- Base SUCS zone boundaries (duplicated for freestanding SUKEMET) ------ */
#ifndef SUAS_SUKEMET_LIMIT_UNICODE_MAX
#define SUAS_SUKEMET_LIMIT_UNICODE_MAX 0x0010FFFFUL
#define SUAS_SUKEMET_LIMIT_SCP_MIN     0x00110000UL
#define SUAS_SUKEMET_LIMIT_SCP_MAX     0x0011FFFFUL
#define SUAS_SUKEMET_LIMIT_NATIVE_MIN  0x00120000UL
#define SUAS_SUKEMET_LIMIT_NATIVE_MAX  0x7FFFFFEFUL
#define SUAS_SUKEMET_LIMIT_TRAP_MIN    0x7FFFFFF0UL
#define SUAS_SUKEMET_LIMIT_TRAP_MAX    0x7FFFFFFEUL
#define SUAS_SUKEMET_LIMIT_SENTINEL    0x7FFFFFFFUL
#endif

/* RNUR / RCCR neography carve-out inside the Native plane (SUAS-005 5.4).
 * The set number of an allocation is (cp - 0x00160000) >> 8. */
#define SUAS_SUKEMET_RNUR_MIN          0x00160000UL
#define SUAS_SUKEMET_RNUR_MAX          0x0016FFFFUL
#define SUAS_SUKEMET_RNUR_SET_SHIFT    8U

/* -- Egyptian repertoire block boundaries (UAX #57) ---------------------- */
#define SUKEMET_BLOCK_HIEROGLYPHS_MIN  0x00013000UL
#define SUKEMET_BLOCK_HIEROGLYPHS_MAX  0x0001342FUL
#define SUKEMET_BLOCK_CONTROLS_MIN     0x00013430UL
#define SUKEMET_BLOCK_CONTROLS_MAX     0x0001345FUL
#define SUKEMET_BLOCK_EXTENDED_A_MIN   0x00013460UL
#define SUKEMET_BLOCK_EXTENDED_A_MAX   0x000143FFUL

/* The named format controls of the block. */
#define SUKEMET_VERTICAL_JOINER        0x00013430UL
#define SUKEMET_HORIZONTAL_JOINER      0x00013431UL
#define SUKEMET_OVERLAY_MIDDLE         0x00013436UL
#define SUKEMET_BEGIN_SEGMENT          0x00013437UL
#define SUKEMET_END_SEGMENT            0x00013438UL
#define SUKEMET_BEGIN_ENCLOSURE        0x0001343CUL
#define SUKEMET_END_ENCLOSURE          0x0001343DUL
#define SUKEMET_BEGIN_WALLED_ENCLOSURE 0x0001343EUL
#define SUKEMET_END_WALLED_ENCLOSURE   0x0001343FUL
#define SUKEMET_MIRROR_HORIZONTALLY    0x00013440UL

/* -- Address-space zones (section 2) ------------------------------------- */
typedef enum {
    SUAS_SUKEMET_ZONE_BRIDGE   = 0, /* 0x00000000-0x0010FFFF           */
    SUAS_SUKEMET_ZONE_SCP      = 1, /* 0x00110000-0x0011FFFF           */
    SUAS_SUKEMET_ZONE_RNUR     = 2, /* 0x00160000-0x0016FFFF           */
    SUAS_SUKEMET_ZONE_NATIVE   = 3, /* rest of 0x00120000-0x7FFFFFEF   */
    SUAS_SUKEMET_ZONE_TRAP     = 4, /* 0x7FFFFFF0-0x7FFFFFFE           */
    SUAS_SUKEMET_ZONE_SENTINEL = 5, /* 0x7FFFFFFF                      */
    SUAS_SUKEMET_ZONE_EXTSUCS  = 6  /* > 0x7FFFFFFF (plugin space)     */
} suas_sukemet_zone_t;

/* -- Egyptian repertoire blocks ----------------------------------------- */
typedef enum {
    SUKEMET_BLOCK_NONE             = 0,
    SUKEMET_BLOCK_EGYPTIAN         = 1, /* U+13000-U+1342F             */
    SUKEMET_BLOCK_FORMAT_CONTROLS  = 2, /* U+13430-U+1345F             */
    SUKEMET_BLOCK_EXTENDED_A       = 3, /* U+13460-U+143FF             */
    SUKEMET_BLOCK_NEOGRAPHY        = 4  /* 0x160000-0x16FFFF           */
} sukemet_block_t;

/* -- Hieroglyphic classification and control flags (packed bitfields) ---- */
/* The seeding sketch's flag names, verbatim. They classify a record as a
 * Gardiner sign, a layout modifier, an enclosure or a composite cluster. */
#define SUKEMET_FLAG_GARDINER   (1U << 0) /* Standard Gardiner classification */
#define SUKEMET_FLAG_MODIFIER   (1U << 1) /* Layout control or stacking modifier */
#define SUKEMET_FLAG_ENCLOSURE  (1U << 2) /* Cartouche, serekh or walled frame */
#define SUKEMET_FLAG_COMPOSITE  (1U << 3) /* Multi-sign ligature cluster */

/* Additional orthogonal flags SUKEMET assigns (section 3.3). */
#define SUKEMET_FLAG_CORE       (1U << 4) /* kEH_Core = C                   */
#define SUKEMET_FLAG_LEGACY     (1U << 5) /* kEH_Core = L                   */
#define SUKEMET_FLAG_NO_MIRROR  (1U << 6) /* kEH_NoMirror = Y               */
#define SUKEMET_FLAG_NO_ROTATE  (1U << 7) /* kEH_NoRotate = Y               */
#define SUKEMET_FLAG_FORMAT     (1U << 8) /* Egyptian format control        */
#define SUKEMET_FLAG_STACKING   (1U << 9) /* Vertical/horizontal joiner     */
#define SUKEMET_FLAG_INSERTION  (1U << 10) /* Insert-at-* control           */
#define SUKEMET_FLAG_OVERLAY    (1U << 11) /* Overlay-middle control        */
#define SUKEMET_FLAG_SEGMENT    (1U << 12) /* Begin/end segment control     */
#define SUKEMET_FLAG_MIRROR     (1U << 13) /* Mirror-horizontally control   */
#define SUKEMET_FLAG_NEOGRAPHY  (1U << 14) /* Native & Neography Plane      */

/* Bit 15 is reserved and must be zero. */
#define SUKEMET_FLAG_ASSIGNED_MASK 0x7FFFu
#define SUKEMET_FLAG_RESERVED_MASK 0x8000u

/* -- Gardiner groups (A-Z plus AA; J is not a Gardiner group) ----------- */
typedef enum {
    SUKEMET_GARDINER_A  = 0,
    SUKEMET_GARDINER_B  = 1,
    SUKEMET_GARDINER_C  = 2,
    SUKEMET_GARDINER_D  = 3,
    SUKEMET_GARDINER_E  = 4,
    SUKEMET_GARDINER_F  = 5,
    SUKEMET_GARDINER_G  = 6,
    SUKEMET_GARDINER_H  = 7,
    SUKEMET_GARDINER_I  = 8,
    SUKEMET_GARDINER_K  = 9,
    SUKEMET_GARDINER_L  = 10,
    SUKEMET_GARDINER_M  = 11,
    SUKEMET_GARDINER_N  = 12,
    SUKEMET_GARDINER_O  = 13,
    SUKEMET_GARDINER_P  = 14,
    SUKEMET_GARDINER_Q  = 15,
    SUKEMET_GARDINER_R  = 16,
    SUKEMET_GARDINER_S  = 17,
    SUKEMET_GARDINER_T  = 18,
    SUKEMET_GARDINER_U  = 19,
    SUKEMET_GARDINER_V  = 20,
    SUKEMET_GARDINER_W  = 21,
    SUKEMET_GARDINER_X  = 22,
    SUKEMET_GARDINER_Y  = 23,
    SUKEMET_GARDINER_Z  = 24,
    SUKEMET_GARDINER_AA = 25,
    SUKEMET_GARDINER_COUNT = 26,
    SUKEMET_GARDINER_NONE  = 0xFF
} sukemet_gardiner_t;

/* -- kEH_Core values ---------------------------------------------------- */
typedef enum {
    SUKEMET_CORE_N = 0, /* Not in the Core set (Unikemet default) */
    SUKEMET_CORE_C = 1, /* Core                                  */
    SUKEMET_CORE_L = 2  /* Legacy                                */
} sukemet_core_t;

/* -- kEH_Func classes (a bitmask: a sign may have several) -------------- */
#define SUKEMET_FUNC_NONE          0x00u
#define SUKEMET_FUNC_PICTOGRAM     (1U << 0)
#define SUKEMET_FUNC_LOGOGRAM      (1U << 1)
#define SUKEMET_FUNC_PHONOGRAM     (1U << 2)
#define SUKEMET_FUNC_CLASSIFIER    (1U << 3)
#define SUKEMET_FUNC_PHONO_REPEATER (1U << 4)
#define SUKEMET_FUNC_RADICOGRAM    (1U << 5)
#define SUKEMET_FUNC_INTERPRETANT  (1U << 6)
#define SUKEMET_FUNC_CONTROL       (1U << 7)
#define SUKEMET_FUNC_ASSIGNED_MASK 0x00FFu

/* -- Modifier role classes ---------------------------------------------- */
/* The packing keeps every layout role in one byte, independent of the 16-bit
 * flag word. SHADING is a neography-declared role; Unicode 18.0.0 encodes no
 * standard Egyptian shading control, so a shading modifier is always inside
 * the RNUR carve-out. */
typedef enum {
    SUKEMET_MODIFIER_NONE      = 0,
    SUKEMET_MODIFIER_JOINER    = 1,
    SUKEMET_MODIFIER_INSERTION = 2,
    SUKEMET_MODIFIER_OVERLAY   = 3,
    SUKEMET_MODIFIER_SEGMENT   = 4,
    SUKEMET_MODIFIER_ENCLOSURE = 5,
    SUKEMET_MODIFIER_MIRROR    = 6,
    SUKEMET_MODIFIER_DAMAGE    = 7,
    SUKEMET_MODIFIER_SHADING   = 8
} sukemet_modifier_class_t;

/* -- Enclosure kinds ---------------------------------------------------- */
/* BEGIN/END ENCLOSURE build the rounded cartouche; BEGIN/END WALLED
 * ENCLOSURE build the rectangular serekh-style frame. A neography may declare
 * its own frame with SUKEMET_ENCLOSURE_CUSTOM. */
typedef enum {
    SUKEMET_ENCLOSURE_NONE      = 0,
    SUKEMET_ENCLOSURE_CARTOUCHE = 1,
    SUKEMET_ENCLOSURE_SEREKH    = 2,
    SUKEMET_ENCLOSURE_CUSTOM    = 3
} sukemet_enclosure_t;

/* -- Unikemet source mask ----------------------------------------------- */
#define SUKEMET_SOURCE_HG       (1U << 0) /* kEH_HG (Hieroglyphica)        */
#define SUKEMET_SOURCE_JSESH    (1U << 1) /* kEH_JSesh                     */
#define SUKEMET_SOURCE_IFAO     (1U << 2) /* kEH_IFAO                      */
#define SUKEMET_SOURCE_UNIK     (1U << 3) /* kEH_UniK catalogue index      */
#define SUKEMET_SOURCE_CAT      (1U << 4) /* kEH_Cat IFAO taxonomy         */
#define SUKEMET_SOURCE_DESC     (1U << 5) /* kEH_Desc                      */
#define SUKEMET_SOURCE_FUNC     (1U << 6) /* kEH_Func                      */
#define SUKEMET_SOURCE_FVAL     (1U << 7) /* kEH_FVal                      */
#define SUKEMET_SOURCE_ALTSEQ   (1U << 8) /* kEH_AltSeq                    */
#define SUKEMET_SOURCE_MIRROR   (1U << 9) /* kEH_NoMirror                  */
#define SUKEMET_SOURCE_ROTATE   (1U << 10) /* kEH_NoRotate                 */

/* -- Verification outcomes ---------------------------------------------- */
typedef enum {
    SUKEMET_REGISTRY_OK = 0,
    SUKEMET_REGISTRY_NULL_TABLE,          /* count > 0, pointer NULL       */
    SUKEMET_REGISTRY_COUNT_OVER,          /* count > static capacity       */
    SUKEMET_REGISTRY_UNORDERED,           /* codepoints not ascending      */
    SUKEMET_REGISTRY_DUPLICATE_CODEPOINT, /* same codepoint twice          */
    SUKEMET_REGISTRY_OUT_OF_SPACE,        /* codepoint > 0x7FFFFFFF        */
    SUKEMET_REGISTRY_INADMISSIBLE_CODEPOINT, /* SCP, trap or Sentinel     */
    SUKEMET_REGISTRY_BAD_FLAGS,           /* reserved or contradictory bits */
    SUKEMET_REGISTRY_BAD_CATEGORY,        /* category > AA and != NONE     */
    SUKEMET_REGISTRY_GARDINER_MISMATCH,   /* flag and category disagree    */
    SUKEMET_REGISTRY_CONTROL_MISMATCH,    /* format flag outside the block */
    SUKEMET_REGISTRY_NEOGRAPHY_MISMATCH,  /* NEOGRAPHY outside the RNUR    */
    SUKEMET_REGISTRY_CORE_CONTRADICTION   /* both CORE and LEGACY          */
} sukemet_registry_error_t;

/* -- The static hieroglyph record (12 bytes) ---------------------------- */
/* The seeding sketch's fields, verbatim and in order: `codepoint`,
 * `phonetic_hash`, `sukemet_flags`, `gardiner_category`, `sub_index`.
 * The record is exactly 12 bytes, no padding on any supported ABI. */
typedef struct {
    uint32_t codepoint;          /* Raw 31-bit SuperUnicode Hieroglyph codepoint */
    uint32_t phonetic_hash;      /* Precomputed hash of primary phonetic reading  */
    uint16_t sukemet_flags;      /* Packed classification and control attributes */
    uint8_t  gardiner_category;  /* Gardiner category letter index (A-Z, AA)      */
    uint8_t  sub_index;          /* Sequential index within Gardiner category     */
} sukemet_hieroglyph_entry_t;

/* -- The parallel Unikemet metadata record ----------------------------- */
/* The 12-byte sketch record has no room for the catalogue index, the source
 * mask, the Core class, the function mask or the hashes of the informative
 * fields. SUKEMET keeps them in a parallel table aligned with the main table,
 * exactly as SUAMR keeps the combining class beside its layout record. */
typedef struct {
    uint32_t catalog_index;      /* (cat << 24) | (subgroup << 16) | item      */
    uint32_t description_hash;   /* FNV-1a over UTF-8 kEH_Desc, 0 if absent    */
    uint32_t unik_hash;          /* FNV-1a over kEH_UniK, 0 if absent          */
    uint16_t source_mask;        /* SUKEMET_SOURCE_*                           */
    uint8_t  core_class;         /* SUKEMET_CORE_*                             */
    uint8_t  function_types;     /* SUKEMET_FUNC_* mask                        */
    uint8_t  modifier_class;     /* SUKEMET_MODIFIER_*                         */
    uint8_t  enclosure_kind;     /* SUKEMET_ENCLOSURE_*                        */
    uint8_t  alternate_length;   /* kEH_AltSeq codepoint count, 0 if absent    */
    uint8_t  reserved[3];        /* must be zero                               */
} sukemet_metadata_t;

/* -- Setup -------------------------------------------------------------- */
const char* suas_sukemet_version_string(void);

/* Initializes the static SUKEMET Egyptian hieroglyph registry and lookup
 * tables. Idempotent and allocation-free; the query entry points build the
 * index lazily on first use if this is not called first. */
void suas_sukemet_init_registry(void);

/* -- Core query (section 3.4) ------------------------------------------ */
/* Queries the pre-compiled hieroglyphic properties and Gardiner
 * classification in O(1) time without heap allocation or string parsing. On
 * success `*out_entry` receives a copy of the record and the function returns
 * true. When the codepoint is in a SUKEMET repertoire space (Egyptian
 * Hieroglyphs, Egyptian Hieroglyph Format Controls, Egyptian Hieroglyphs
 * Extended-A or the RNUR carve-out) but has no curated record, the function
 * synthesizes the algorithmic default of section 6 and returns *true*: the
 * property is total over the repertoire. It returns false only for an
 * inadmissible zone or a NULL output, leaving the output untouched. */
bool suas_sukemet_get_entry(uint32_t codepoint,
                            sukemet_hieroglyph_entry_t *out_entry);

/* The parallel metadata for a codepoint (O(1)). For a synthesized record the
 * metadata is all-zero. Returns false under the same conditions as
 * suas_sukemet_get_entry. */
bool suas_sukemet_get_metadata(uint32_t codepoint,
                               sukemet_metadata_t *out_metadata);

/* -- Classification predicates (section 3) ------------------------------ */
/* True for a curated hieroglyph, format control or modifier (O(1)). */
bool suas_sukemet_is_registered(uint32_t codepoint);

/* True for any codepoint in a SUKEMET repertoire block or the RNUR carve-out,
 * curated or not (O(1)). This is the totality domain of section 6. */
bool suas_sukemet_is_hieroglyph(uint32_t codepoint);

/* True when the record carries SUKEMET_FLAG_GARDINER. */
bool suas_sukemet_is_gardiner(uint32_t codepoint);

/* True when the record is a format control or a layout modifier. */
bool suas_sukemet_is_modifier(uint32_t codepoint);

/* True when the record is an enclosure frame. */
bool suas_sukemet_is_enclosure(uint32_t codepoint);

/* True when the record is a multi-sign composite or ligature. */
bool suas_sukemet_is_composite(uint32_t codepoint);

/* Core / Legacy membership (kEH_Core). */
bool suas_sukemet_is_core(uint32_t codepoint);
bool suas_sukemet_is_legacy(uint32_t codepoint);

/* Mirror / rotate eligibility: the true value means the sign may be mirrored
 * or rotated; kEH_NoMirror / kEH_NoRotate set the corresponding flag. */
bool suas_sukemet_is_mirrorable(uint32_t codepoint);
bool suas_sukemet_is_rotatable(uint32_t codepoint);

/* Format-control role predicates. */
bool suas_sukemet_is_format_control(uint32_t codepoint);
bool suas_sukemet_is_joiner(uint32_t codepoint);
bool suas_sukemet_is_insertion(uint32_t codepoint);
bool suas_sukemet_is_overlay(uint32_t codepoint);
bool suas_sukemet_is_segment(uint32_t codepoint);
bool suas_sukemet_is_mirror_control(uint32_t codepoint);
bool suas_sukemet_is_shading(uint32_t codepoint);
bool suas_sukemet_is_damage(uint32_t codepoint);

/* True inside the Native & Neography Plane carve-out (O(1)). */
bool suas_sukemet_is_neography(uint32_t codepoint);

/* -- Property queries (section 4) --------------------------------------- */
/* The Gardiner group of a record (SUKEMET_GARDINER_NONE when unclassified). */
uint8_t suas_sukemet_gardiner_category(uint32_t codepoint);

/* The kEH_Core class (N for an uncurated codepoint). */
uint8_t suas_sukemet_core_class(uint32_t codepoint);

/* The kEH_Func bitmask (SUKEMET_FUNC_NONE when undocumented). */
uint8_t suas_sukemet_function_types(uint32_t codepoint);

/* The layout role of a modifier (SUKEMET_MODIFIER_NONE for a sign). */
uint8_t suas_sukemet_modifier_class(uint32_t codepoint);

/* The enclosure kind of a frame control. */
uint8_t suas_sukemet_enclosure_kind(uint32_t codepoint);

/* The kEH_Cat catalogue index, packed as (cat << 24) | (subgroup << 16) |
 * item. Returns false when the record has no kEH_Cat. */
bool suas_sukemet_catalog_index(uint32_t codepoint, uint32_t *out_index);

/* Decode kEH_Cat into its three levels. Any output pointer may be NULL. */
bool suas_sukemet_catalog_of(uint32_t codepoint,
                             uint8_t *out_category,
                             uint8_t *out_subgroup,
                             uint16_t *out_item);

/* The Unikemet source mask (SUKEMET_SOURCE_*). */
uint16_t suas_sukemet_source_mask(uint32_t codepoint);

/* Hashes of the informative Unikemet fields (0 when absent). */
uint32_t suas_sukemet_description_hash(uint32_t codepoint);
uint32_t suas_sukemet_unik_hash(uint32_t codepoint);

/* The number of codepoints in kEH_AltSeq (0 when absent). */
uint8_t suas_sukemet_alt_seq_length(uint32_t codepoint);

/* Copy the i-th alternate-sequence codepoint (0-based). Returns false out of
 * range or on a NULL output. */
bool suas_sukemet_alt_seq_at(uint32_t codepoint,
                             uint8_t index,
                             uint32_t *out_codepoint);

/* -- Enumeration -------------------------------------------------------- */
/* Number of curated records in the static table (O(1)); excludes synthesized
 * defaults. */
uint32_t suas_sukemet_entry_count(void);

/* Copy the i-th curated record (0-based, ascending codepoint order). Returns
 * false out of range or on a NULL output (O(1)). */
bool suas_sukemet_entry_at(uint32_t index,
                           sukemet_hieroglyph_entry_t *out_entry);

/* Copy the i-th curated metadata record (0-based, same order as entry_at). */
bool suas_sukemet_metadata_at(uint32_t index,
                              sukemet_metadata_t *out_metadata);

/* -- Zone and block predicates (section 2) ------------------------------ */
/* Classifies a 31-bit codepoint into its address-space zone (O(1)). A value
 * above SUAS_CODEPOINT_MAX is EXTSUCS. */
suas_sukemet_zone_t suas_sukemet_zone_of(uint32_t codepoint);

/* True for the zones a SUKEMET query may name: Bridge, RNUR and Native.
 * False for the Control Plane, the trap slots, the Sentinel and ExtSUCS. */
bool suas_sukemet_is_text_space(uint32_t codepoint);

/* The Egyptian repertoire block of a codepoint. */
sukemet_block_t suas_sukemet_block_of(uint32_t codepoint);

/* Set number of an RNUR coordinate: (cp - 0x00160000) >> 8. Returns false
 * outside the carve-out. */
bool suas_sukemet_neography_set(uint32_t codepoint, uint32_t *out_set);

/* -- Hash --------------------------------------------------------------- */
/* FNV-1a 32-bit hash over the little-endian encoding of the codepoint (O(1)).
 * This is the index key, not an identity. */
uint32_t suas_sukemet_hash(uint32_t codepoint);

/* FNV-1a 32-bit hash over an arbitrary byte string (O(n)). This is the
 * function used to precompute `phonetic_hash`, `description_hash` and
 * `unik_hash` from the UTF-8 Unikemet fields. */
uint32_t suas_sukemet_hash_bytes(const char *bytes, size_t length);

/* -- Taxonomy names ----------------------------------------------------- */
/* The Gardiner letter ("A".."Z", "AA") or "?" for an invalid group. */
const char* suas_sukemet_gardiner_letter(uint8_t category);

/* The Gardiner group name ("Man and his occupations", ...). */
const char* suas_sukemet_gardiner_name(uint8_t category);

const char* suas_sukemet_core_name(uint8_t core_class);
const char* suas_sukemet_function_name(uint8_t function_type);
const char* suas_sukemet_modifier_class_name(uint8_t modifier_class);
const char* suas_sukemet_enclosure_name(uint8_t enclosure_kind);
const char* suas_sukemet_source_name(uint16_t source_bit);
const char* suas_sukemet_block_name(sukemet_block_t block);
const char* suas_sukemet_zone_name(suas_sukemet_zone_t zone);
const char* suas_sukemet_registry_error_name(sukemet_registry_error_t error);

/* -- Verification (section 9) ------------------------------------------- */
/* Verify a compiled hieroglyph table. `entries` may be NULL only when `count`
 * is zero (an empty registry is conformant). On failure, `*out_error`
 * receives the first defect and the function returns false; on success
 * `*out_error` is SUKEMET_REGISTRY_OK. Either output pointer may be NULL. */
bool suas_sukemet_validate_table(const sukemet_hieroglyph_entry_t *entries,
                                 uint32_t count,
                                 sukemet_registry_error_t *out_error);

#ifdef __cplusplus
}
#endif

#endif /* SUAS_SUAS_SUKEMET_H */
