#ifndef SUTR_SUTR_SUCPM_H
#define SUTR_SUTR_SUCPM_H

/* SUTR-8 — SuperUnicode Character Property Model (SUCPM)
 *
 * A modular technical-report specification within the OpenWindows microkernel
 * ecosystem. SUCPM is the SuperUnicode analog of UTR #23 *Unicode Character
 * Property Model*: it fixes the vocabulary of character properties (their
 * types, conformance status, stability, and derivation) and the *shape* of
 * the packed property mask that every consumer reads.
 *
 * The model is architectural, so this module makes the architecture
 * checkable. Everything a kernel face needs is a shift, a mask, and a compare:
 *
 *   1. THE RANGE-DISPATCHED DUAL ENGINE (§4.6). One function, one tier
 *      dispatch, one packed mask. The dispatch order is normative:
 *      Bridge -> SCP -> Native low -> RNUR registry -> Native high ->
 *      terminal 16 -> plugin. The registry carve-out is tested *before* the
 *      general Native range, and the terminal 16 values are rejected
 *      *before* ExtSUCS dispatch.
 *   2. THE 32-BIT CORE MASK (§5.1). The Bidi field in bits 0-5 mirrors the
 *      SUCD_BIDI_* layout; the common binary properties occupy bits 6-22;
 *      bit 23 is reserved by this report and is assigned by the companion
 *      SUTR-9 revision to the `Math` membership flag; the tier occupies bits
 *      24-27; bits 28-31 are reserved by this report and are assigned by the
 *      companion SUTR-9 revision to the 4-bit `MathClass` field.
 *   3. THE 64-BIT EXTENDED MASK (§5.2). The core mask sits in the low 32
 *      bits and the enumerated property *values* pack into the high 32:
 *      General_Category at 32-36, Canonical_Combining_Class at 37-44,
 *      Script_Identifier at 45-54, Line_Break_Class at 55-60, and
 *      Numeric_Type at 61-63.
 *   4. THE O(1) TESTS (§5.3). A binary property test is a mask-and-compare;
 *      an enumerated property test is a shift-and-compare. Neither
 *      allocates, and neither re-parses a property name.
 *   5. THE PROPERTY MODEL (§3). The vocabulary itself — the property type,
 *      the conformance status, the stability classification, the derivation
 *      class, the three property tiers of §1.3.1, and the special values of
 *      §8 — is registered as data, so a consumer can ask what kind of
 *      property it is holding.
 *
 * Freestanding C99, zero allocation: the Bridge table is a static sorted
 * range list, the Native and plugin range registers are fixed-size arrays
 * the caller publishes into, and every descriptor is a fixed-size struct the
 * caller owns.
 *
 * See docs/sutr/SUTR-8-sucpm.md for the normative specification.
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Specification identifiers ────────────────────────────────────── */

#define SUCPM_SPEC_ID            "SUTR-8"
#define SUCPM_SPEC_TITLE         "SuperUnicode Character Property Model"
#define SUCPM_VERSION_MAJOR      0
#define SUCPM_VERSION_MINOR      1
#define SUCPM_VERSION_PATCH      0
#define SUCPM_MODULE             "sutr_sucpm"

/* ── The packed masks (§5) ─────────────────────────────────────────── */

typedef uint32_t sucpm_flags32_t;
typedef uint64_t sucpm_flags64_t;

/* Bidi field, bits 0-5. The layout mirrors SUCD_BIDI_* exactly. */
#define SUCPM_BIDI_LTR        (1u << 0)
#define SUCPM_BIDI_RTL        (1u << 1)
#define SUCPM_BIDI_ARABIC_AL  (1u << 2)
#define SUCPM_BIDI_NEUTRAL    (1u << 3)
#define SUCPM_BIDI_MIRRORED   (1u << 4)
#define SUCPM_BIDI_WHITESPACE (1u << 5)
#define SUCPM_BIDI_ALL        0x3Fu

/* The common binary properties, bits 6-22. */
#define SUCPM_F_ALPHABETIC             (1u << 6)
#define SUCPM_F_UPPERCASE               (1u << 7)
#define SUCPM_F_LOWERCASE               (1u << 8)
#define SUCPM_F_WHITE_SPACE             (1u << 9)
#define SUCPM_F_MARK                    (1u << 10)
#define SUCPM_F_NUMBER                  (1u << 11)
#define SUCPM_F_PUNCTUATION             (1u << 12)
#define SUCPM_F_SYMBOL                  (1u << 13)
#define SUCPM_F_JOIN_CONTROL            (1u << 14)
#define SUCPM_F_DEFAULT_IGNORABLE       (1u << 15)
#define SUCPM_F_NONCHARACTER            (1u << 16)
#define SUCPM_F_ASSIGNED                (1u << 17)
#define SUCPM_F_CANONICAL_DECOMPOSABLE  (1u << 18)
#define SUCPM_F_COMPOSITION_EXCLUSION   (1u << 19)
#define SUCPM_F_BIDI_CONTROL            (1u << 20)
#define SUCPM_F_VARIATION_SELECTOR      (1u << 21)
#define SUCPM_F_RNUR                    (1u << 22)

/* Bit 23 is reserved by SUTR-8 §5.1. The companion SUTR-9 revision assigns
 * it to the `Math` membership flag (SUTR-9 §4.3). */
#define SUCPM_F_MATH                    (1u << 23)
#define SUCPM_CORE_RESERVED_23          (1u << 23)

/* The lookup tier, bits 24-27. */
#define SUCPM_TIER_SHIFT  24
#define SUCPM_TIER_MASK   (0xFu << 24)
#define SUCPM_TIER_BRIDGE (0u << 24) /* 0x000000-0x10FFFF */
#define SUCPM_TIER_SCP    (1u << 24) /* 0x110000-0x11FFFF */
#define SUCPM_TIER_NATIVE (2u << 24) /* 0x120000-0x7FFFFFEF */
#define SUCPM_TIER_RNUR   (3u << 24) /* 0x160000-0x16FFFF */
#define SUCPM_TIER_EXTSUCS (4u << 24)/* > 0x7FFFFFFF      */
#define SUCPM_TIER_RESERVED (0xFu << 24)

/* Bits 28-31 are reserved by SUTR-8 §5.1. The companion SUTR-9 revision
 * assigns them to the 4-bit MathClass field (SUTR-9 §4.3). */
#define SUCPM_MATHCLASS_SHIFT 28
#define SUCPM_MATHCLASS_MASK  (0xFu << 28)
#define SUCPM_CORE_RESERVED_HIGH (0xFu << 28)

/* The extended mask fields (§5.2). */
#define SUCPM_GC_SHIFT          32
#define SUCPM_GC_MASK           ((sucpm_flags64_t)0x1Fu << SUCPM_GC_SHIFT)
#define SUCPM_CCC_SHIFT         37
#define SUCPM_CCC_MASK          ((sucpm_flags64_t)0xFFu << SUCPM_CCC_SHIFT)
#define SUCPM_SCRIPT_SHIFT      45
#define SUCPM_SCRIPT_MASK       ((sucpm_flags64_t)0x3FFu << SUCPM_SCRIPT_SHIFT)
#define SUCPM_LINEBREAK_SHIFT   55
#define SUCPM_LINEBREAK_MASK    ((sucpm_flags64_t)0x3Fu << SUCPM_LINEBREAK_SHIFT)
#define SUCPM_NUMERICTYPE_SHIFT 61
#define SUCPM_NUMERICTYPE_MASK  ((sucpm_flags64_t)0x7u << SUCPM_NUMERICTYPE_SHIFT)

/* The mask returned for the terminal 16 values (traps and the Sentinel). */
#define SUCPM_MASK_RESERVED (sucpm_flags32_t)SUCPM_TIER_RESERVED

/* Mask composition helpers (§5). A consumer that assembles its own mask
 * uses these; they are pure shifts and never touch memory. */
#define SUCPM_M_TIER(t)  ((sucpm_flags64_t)(t))
#define SUCPM_M_BIDI(b)  ((sucpm_flags64_t)(b))
#define SUCPM_M_GC(v)    ((sucpm_flags64_t)(v) << SUCPM_GC_SHIFT)
#define SUCPM_M_CCC(v)   ((sucpm_flags64_t)(v) << SUCPM_CCC_SHIFT)
#define SUCPM_M_SCR(v)   ((sucpm_flags64_t)(v) << SUCPM_SCRIPT_SHIFT)
#define SUCPM_M_LB(v)    ((sucpm_flags64_t)(v) << SUCPM_LINEBREAK_SHIFT)
#define SUCPM_M_NT(v)    ((sucpm_flags64_t)(v) << SUCPM_NUMERICTYPE_SHIFT)
#define SUCPM_M_MC(v)    ((sucpm_flags64_t)(v) << SUCPM_MATHCLASS_SHIFT)

/* ── The enumerated property values ───────────────────────────────── */

/* General_Category (PD9, closed enumeration). */
typedef enum {
    SUCPM_GC_LU = 0,  /* Uppercase_Letter        */
    SUCPM_GC_LL = 1,  /* Lowercase_Letter        */
    SUCPM_GC_LT = 2,  /* Titlecase_Letter        */
    SUCPM_GC_LM = 3,  /* Modifier_Letter         */
    SUCPM_GC_LO = 4,  /* Other_Letter            */
    SUCPM_GC_MN = 5,  /* Nonspacing_Mark         */
    SUCPM_GC_MC = 6,  /* Spacing_Mark            */
    SUCPM_GC_ME = 7,  /* Enclosing_Mark          */
    SUCPM_GC_ND = 8,  /* Decimal_Number          */
    SUCPM_GC_NL = 9,  /* Letter_Number           */
    SUCPM_GC_NO = 10, /* Other_Number            */
    SUCPM_GC_PC = 11, /* Connector_Punctuation   */
    SUCPM_GC_PD = 12, /* Dash_Punctuation        */
    SUCPM_GC_PS = 13, /* Open_Punctuation        */
    SUCPM_GC_PE = 14, /* Close_Punctuation       */
    SUCPM_GC_PI = 15, /* Initial_Punctuation     */
    SUCPM_GC_PF = 16, /* Final_Punctuation       */
    SUCPM_GC_PO = 17, /* Other_Punctuation       */
    SUCPM_GC_SM = 18, /* Math_Symbol             */
    SUCPM_GC_SC = 19, /* Currency_Symbol         */
    SUCPM_GC_SK = 20, /* Modifier_Symbol         */
    SUCPM_GC_SO = 21, /* Other_Symbol            */
    SUCPM_GC_ZS = 22, /* Space_Separator         */
    SUCPM_GC_ZL = 23, /* Line_Separator          */
    SUCPM_GC_ZP = 24, /* Paragraph_Separator     */
    SUCPM_GC_CC = 25, /* Control                 */
    SUCPM_GC_CF = 26, /* Format                  */
    SUCPM_GC_CS = 27, /* Surrogate               */
    SUCPM_GC_CO = 28, /* Private_Use             */
    SUCPM_GC_CN = 29, /* Unassigned              */
    SUCPM_GC_COUNT = 30
} sucpm_gc_t;

/* Script_Identifier (PD14, catalog). */
typedef enum {
    SUCPM_SCRIPT_COMMON = 0,
    SUCPM_SCRIPT_LATIN = 1,
    SUCPM_SCRIPT_GREEK = 2,
    SUCPM_SCRIPT_CYRILLIC = 3,
    SUCPM_SCRIPT_HEBREW = 4,
    SUCPM_SCRIPT_ARABIC = 5,
    SUCPM_SCRIPT_DEVANAGARI = 6,
    SUCPM_SCRIPT_HIRAGANA = 7,
    SUCPM_SCRIPT_KATAKANA = 8,
    SUCPM_SCRIPT_HAN = 9,
    SUCPM_SCRIPT_MONGOLIAN = 10,
    SUCPM_SCRIPT_INHERITED = 11,
    SUCPM_SCRIPT_COUNT = 12
} sucpm_script_t;

/* Line_Break_Class (PD14, catalog). */
typedef enum {
    SUCPM_LB_AI = 0, SUCPM_LB_AL, SUCPM_LB_B2, SUCPM_LB_BA, SUCPM_LB_BB,
    SUCPM_LB_BK, SUCPM_LB_CB, SUCPM_LB_CL, SUCPM_LB_CM, SUCPM_LB_CP,
    SUCPM_LB_CR, SUCPM_LB_EB, SUCPM_LB_EM, SUCPM_LB_EX, SUCPM_LB_GL,
    SUCPM_LB_H2, SUCPM_LB_H3, SUCPM_LB_HL, SUCPM_LB_HY, SUCPM_LB_ID,
    SUCPM_LB_IN, SUCPM_LB_IS, SUCPM_LB_JL, SUCPM_LB_JT, SUCPM_LB_JV,
    SUCPM_LB_LF, SUCPM_LB_NL, SUCPM_LB_NS, SUCPM_LB_NU, SUCPM_LB_OP,
    SUCPM_LB_PO, SUCPM_LB_PR, SUCPM_LB_QU, SUCPM_LB_RI, SUCPM_LB_SA,
    SUCPM_LB_SG, SUCPM_LB_SP, SUCPM_LB_SY, SUCPM_LB_WJ, SUCPM_LB_ZW,
    SUCPM_LB_ZWNJ,
    SUCPM_LB_L,  /* LTR embedding marker (U+200E) */
    SUCPM_LB_R,  /* RTL embedding marker (U+200F) */
    SUCPM_LB_COUNT = 43
} sucpm_linebreak_t;

/* Numeric_Type (PD9). */
typedef enum {
    SUCPM_NT_NONE = 0,
    SUCPM_NT_DECIMAL = 1,
    SUCPM_NT_DIGIT = 2,
    SUCPM_NT_NUMERIC = 3
} sucpm_numeric_type_t;

/* MathClass (SUTR-9 §4.2), carried in the reserved core bits 28-31. */
typedef enum {
    SUCPM_MC_N = 0, /* Normal / Ord   */
    SUCPM_MC_B = 1, /* Binary         */
    SUCPM_MC_U = 2, /* Unary / Prefix */
    SUCPM_MC_R = 3, /* Relation       */
    SUCPM_MC_O = 4, /* Opening        */
    SUCPM_MC_C = 5, /* Closing        */
    SUCPM_MC_F = 6, /* Fence          */
    SUCPM_MC_P = 7, /* Punctuation    */
    SUCPM_MC_I = 8, /* Inner          */
    SUCPM_MC_S = 9  /* Special        */
} sucpm_math_class_t;

/* ── The System Control Plane decode (§4.2) ───────────────────────── */

/* The argument byte Y bears the directional flag, the isolate/embed
 * constraint, the break constraint, and the LTR/RTL marker. These are bit
 * positions *within Y*, not within the packed property mask: §4.2 fixes
 * that the test is a bit test on Y, so the mask carries only the tier and
 * the Bidi field and the consumer tests Y directly. These bit assignments
 * are this module's contract. */
#define SUCPM_SCP_ARG_RTL            (1u << 0)
#define SUCPM_SCP_ARG_LTR            (1u << 1)
#define SUCPM_SCP_ARG_ISOLATE        (1u << 2)
#define SUCPM_SCP_ARG_EMBED          (1u << 3)
#define SUCPM_SCP_ARG_BRK_MANDATORY  (1u << 4)
#define SUCPM_SCP_ARG_BRK_PROHIBITED (1u << 5)
#define SUCPM_SCP_ARG_ALL            0x3Fu

/* The severity clusters the directive class byte X bears. */
typedef enum {
    SUCPM_CLUSTER_NONE = 0,
    SUCPM_CLUSTER_B_PLUS = 1,
    SUCPM_CLUSTER_W_PLUS = 2,
    SUCPM_CLUSTER_C_PLUS = 3,
    SUCPM_CLUSTER_S_PLUS = 4
} sucpm_cluster_t;

/* ── The property model vocabulary (§3, §1.3.1) ────────────────────── */

typedef enum {
    SUCPM_TYPE_ENUMERATED = 0, /* PD9  */
    SUCPM_TYPE_BOOLEAN,        /* PD11 */
    SUCPM_TYPE_NUMERIC,        /* PD12 */
    SUCPM_TYPE_STRING,         /* PD13 */
    SUCPM_TYPE_IDENTIFIER,     /* PD13a*/
    SUCPM_TYPE_CATALOG,        /* PD14 */
    SUCPM_TYPE_MISCELLANEOUS,  /* PD15 */
    SUCPM_TYPE_COUNT
} sucpm_property_type_t;

typedef enum {
    SUCPM_STATUS_NORMATIVE = 0,   /* PD16 */
    SUCPM_STATUS_OVERRIDABLE,     /* PD17 */
    SUCPM_STATUS_INFORMATIVE,     /* PD18 */
    SUCPM_STATUS_PROVISIONAL,     /* PD19 */
    SUCPM_STATUS_COUNT
} sucpm_property_status_t;

typedef enum {
    SUCPM_CONTEXT_DEPENDENT = 0,  /* PD20 */
    SUCPM_CONTEXT_INDEPENDENT,    /* PD21 */
    SUCPM_CONTEXT_COUNT
} sucpm_context_t;

typedef enum {
    SUCPM_STABILITY_STABLE = 0,   /* PD23 */
    SUCPM_STABILITY_FIXED,        /* PD24 */
    SUCPM_STABILITY_IMMUTABLE,    /* PD25 */
    SUCPM_STABILITY_STABILIZED,   /* PD26 */
    SUCPM_STABILITY_DEPRECATED,   /* PD27 */
    SUCPM_STABILITY_COUNT
} sucpm_stability_t;

typedef enum {
    SUCPM_DERIVATION_SIMPLE = 0,  /* PD28 */
    SUCPM_DERIVATION_DERIVED,     /* PD29 */
    SUCPM_DERIVATION_COUNT
} sucpm_derivation_t;

/* The three property tiers (§1.3.1). A property's tier is a fact about the
 * property, not about its data source. */
typedef enum {
    SUCPM_TIER_PROPERTY_NORMATIVE = 0,
    SUCPM_TIER_PROPERTY_INFORMATIVE,
    SUCPM_TIER_PROPERTY_CONTRIBUTED,
    SUCPM_TIER_PROPERTY_COUNT
} sucpm_property_tier_t;

/* The special property values (§8). */
#define SUCPM_VALUE_NOT_APPLICABLE 0xFFu /* §8.1 — the explicit "NA" value */

/* A registered property descriptor. */
typedef struct {
    const char *alias;      /* PD30 — the property alias              */
    const char *long_name;  /* the long name                          */
    uint8_t     type;       /* sucpm_property_type_t                  */
    uint8_t     status;     /* sucpm_property_status_t                */
    uint8_t     context;    /* sucpm_context_t                        */
    uint8_t     stability;  /* sucpm_stability_t                      */
    uint8_t     derivation; /* sucpm_derivation_t                     */
    uint8_t     tier;       /* sucpm_property_tier_t                  */
    uint8_t     closed;     /* PD10 — closed enumeration              */
    uint8_t     reserved;   /* must be zero                           */
} sucpm_property_record_t;

/* ── Range registers (§4.3, §4.5) ─────────────────────────────────── */

#define SUCPM_NATIVE_REGISTER_CAPACITY 32u
#define SUCPM_PLUGIN_REGISTER_CAPACITY 16u
#define SUCPM_BRIDGE_RANGE_CAPACITY    128u

/* ── Zone boundaries ──────────────────────────────────────────────── */

#define SUCPM_BRIDGE_MAX   0x0010FFFFULL
#define SUCPM_SCP_MIN      0x00110000ULL
#define SUCPM_SCP_MAX      0x0011FFFFULL
#define SUCPM_NATIVE_LOW_MAX 0x0015FFFFULL
#define SUCPM_RNUR_MIN     0x00160000ULL
#define SUCPM_RNUR_MAX     0x0016FFFFULL
#define SUCPM_NATIVE_MIN   0x00120000ULL
#define SUCPM_NATIVE_MAX   0x7FFFFFEFULL
#define SUCPM_TRAP_MIN     0x7FFFFFF0ULL
#define SUCPM_SENTINEL     0x7FFFFFFFULL

/* ── The property evaluation engine (§4) ──────────────────────────── */

/**
 * The "MAJOR.MINOR.PATCH" version string of this module. The returned
 * pointer is to a static string, never NULL.
 */
const char *sutr_sucpm_version_string(void);

/**
 * The packed property set of a codepoint, in the normative dispatch order of
 * §4.6. Zero allocation, no name parsing, one packed mask.
 *
 * The dispatch is:
 *   Bridge          0x000000-0x10FFFF   §4.1  sorted range list
 *   SCP             0x110000-0x11FFFF   §4.2  decoded from the two bytes
 *   Native low      0x120000-0x15FFFF   §4.3  range registers
 *   RNUR registry   0x160000-0x16FFFF   §4.4  registry decode
 *   Native high     0x170000-0x7FFFFFEF §4.3  range registers
 *   terminal 16     0x7FFFFFF0-0x7FFFFFFF     reserved
 *   plugin          > 0x7FFFFFFF        §4.5  plugin manifest registers
 */
sucpm_flags64_t sutr_sucpm_props(uint64_t codepoint);

/**
 * The lookup tier of a codepoint, as the packed tier field of §5.1.
 */
uint32_t sutr_sucpm_tier_of(uint64_t codepoint);

/**
 * The number of ranges in the Bridge range list of §4.1. The list is the
 * only table whose size the lookup depends on, and the lookup is a binary
 * search over it (O(log R)); every other tier is O(1).
 */
uint32_t sutr_sucpm_bridge_range_count(void);

/**
 * The endpoints of the Bridge range at `index`, in ascending order. Returns
 * false and leaves the outputs untouched when the index is out of range.
 *
 * The accessors exist so that the two structural claims the lookup rests on
 * — the list is sorted by low endpoint, and no two ranges overlap — are
 * testable from outside the module rather than asserted in a comment.
 */
bool sutr_sucpm_bridge_range_at(uint32_t index, uint32_t *out_low,
                                uint32_t *out_high);

/**
 * The O(1) binary property test of §5.3: a mask-and-compare.
 */
bool sutr_sucpm_has(sucpm_flags64_t mask, uint64_t bits);

/**
 * The O(1) enumerated property test of §5.3: a shift-and-compare.
 */
bool sutr_sucpm_is(sucpm_flags64_t mask, uint64_t field_shift,
                   uint64_t value);

/* Enumerated property accessors (§5.2). */
uint8_t  sutr_sucpm_gc(sucpm_flags64_t mask);
uint8_t  sutr_sucpm_ccc(sucpm_flags64_t mask);
uint16_t sutr_sucpm_script(sucpm_flags64_t mask);
uint8_t  sutr_sucpm_linebreak(sucpm_flags64_t mask);
uint8_t  sutr_sucpm_numeric_type(sucpm_flags64_t mask);
uint8_t  sutr_sucpm_math_class(sucpm_flags64_t mask);
uint32_t sutr_sucpm_bidi(sucpm_flags64_t mask);

/**
 * The `Math` membership predicate of SUTR-9 §4.1, read straight from the
 * packed mask.
 */
bool sutr_sucpm_is_math(sucpm_flags64_t mask);

/* ── The System Control Plane decode (§4.2) ───────────────────────── */

/**
 * The directive class byte X of a System Control Plane codepoint: bits 8-15.
 */
uint8_t sutr_sucpm_scp_class_byte(uint64_t codepoint);

/**
 * The argument byte Y of a System Control Plane codepoint: bits 0-7.
 */
uint8_t sutr_sucpm_scp_argument_byte(uint64_t codepoint);

/**
 * The severity cluster a System Control Plane codepoint belongs to. The
 * cluster identity is borne by the directive class byte X (§4.2).
 */
sucpm_cluster_t sutr_sucpm_scp_cluster(uint64_t codepoint);

/**
 * The decoded flag set of the argument byte Y: the directional flag, the
 * isolate/embed constraint, and the break constraint. A bit test on Y, and
 * therefore O(1).
 */
uint8_t sutr_sucpm_scp_argument_flags(uint64_t codepoint);

bool sutr_sucpm_scp_is_rtl(uint64_t codepoint);
bool sutr_sucpm_scp_is_ltr(uint64_t codepoint);
bool sutr_sucpm_scp_is_isolate(uint64_t codepoint);
bool sutr_sucpm_scp_is_embed(uint64_t codepoint);
bool sutr_sucpm_scp_forces_break(uint64_t codepoint);
bool sutr_sucpm_scp_forbids_break(uint64_t codepoint);

/* ── Range register publication (§4.3, §4.5) ──────────────────────── */

/**
 * Publishes a Native-plane range register `<lo><hi><mask>`. The register
 * list is fixed-size and caller-published; the test remains O(1) because the
 * register count is a compile-time constant.
 */
bool sutr_sucpm_publish_native_range(uint64_t low, uint64_t high,
                                      sucpm_flags64_t mask);

/**
 * Publishes a plugin manifest range register for codepoints above the
 * Sentinel. Loaded at mount time, after the checksum gate (SUTR-5).
 */
bool sutr_sucpm_publish_plugin_range(uint64_t low, uint64_t high,
                                     sucpm_flags64_t mask);

/**
 * The number of published Native and plugin range registers.
 */
uint32_t sutr_sucpm_native_register_count(void);
uint32_t sutr_sucpm_plugin_register_count(void);

/**
 * Discards every published range register. Used by the image builder
 * between mounts.
 */
void sutr_sucpm_reset_registers(void);

/* ── The property registry (§3) ───────────────────────────────────── */

uint32_t sutr_sucpm_property_count(void);
const sucpm_property_record_t *sutr_sucpm_property_at(uint32_t index);
const sucpm_property_record_t *sutr_sucpm_property_by_alias(const char *alias);

/* ── Taxonomy names ───────────────────────────────────────────────── */

const char *sutr_sucpm_property_type_name(sucpm_property_type_t type);
const char *sutr_sucpm_property_status_name(sucpm_property_status_t status);
const char *sutr_sucpm_context_name(sucpm_context_t context);
const char *sutr_sucpm_stability_name(sucpm_stability_t stability);
const char *sutr_sucpm_derivation_name(sucpm_derivation_t derivation);
const char *sutr_sucpm_property_tier_name(sucpm_property_tier_t tier);
const char *sutr_sucpm_cluster_name(sucpm_cluster_t cluster);
const char *sutr_sucpm_gc_name(sucpm_gc_t gc);
const char *sutr_sucpm_math_class_name(sucpm_math_class_t math_class);
const char *sutr_sucpm_numeric_type_name(sucpm_numeric_type_t type);

#ifdef __cplusplus
}
#endif

#endif /* SUTR_SUTR_SUCPM_H */
