#ifndef SUTS_SUTS_SRE_H
#define SUTS_SUTS_SRE_H

/* SUTS-002 — SuperUnicode Regular Expressions (SRE).
 *
 * A zero-allocation, single-pass regular-expression engine, freestanding C99.
 * In keeping with OpenWindows kernel principles, SRE discards multi-pass
 * backtracking algorithms and dynamic NFA/DFA state-table growth: patterns
 * compile into a bounded automaton held entirely in caller-provided storage,
 * and matching is linear in the input length over the full 64-bit
 * SuperUnicode codepoint space (Base SUCS 31-bit + ExtSUCS unbounded).
 *
 * Input codepoints are 64-bit ExtSUCS codepoints (sucs_ex_char_t), so one
 * engine matches the Unicode Bridge, the System Control Plane (SCP), Native
 * SUCS, and the unbounded ExtSUCS plugin space uniformly.
 *
 * Reference implementation of docs/suts/SUTS-002-sre.md (UTS #18 analog).
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Version & identification ────────────────────────────────────── */
#define SUTS_SRE_VERSION_MAJOR 0
#define SUTS_SRE_VERSION_MINOR 1
#define SUTS_SRE_VERSION_PATCH 0

/* ── 64-bit ExtSUCS codepoint type (guarded to coexist) ───────────── */
#ifndef EXTSUCS_CHAR_T_DEFINED
#define EXTSUCS_CHAR_T_DEFINED
typedef uint64_t sucs_ex_char_t;
#endif

/* ── Zone boundaries (duplicated for freestanding SRE) ────────────── */
#define SRE_ZONE_UNICODE_MAX  ((sucs_ex_char_t)0x0010FFFFULL)
#define SRE_ZONE_SCP_MIN      ((sucs_ex_char_t)0x00110000ULL)
#define SRE_ZONE_SCP_MAX      ((sucs_ex_char_t)0x0011FFFFULL)
#define SRE_ZONE_NATIVE_MIN   ((sucs_ex_char_t)0x00120000ULL)
#define SRE_ZONE_NATIVE_MAX   ((sucs_ex_char_t)0x7FFFFFFEULL)
#define SRE_BANCODE_MIN       ((sucs_ex_char_t)0x0011A000ULL)
#define SRE_BANCODE_MAX       ((sucs_ex_char_t)0x0011AEFFULL)
#define SRE_BASE_SUCS_MAX     ((sucs_ex_char_t)0x7FFFFFFFULL)
#define SRE_BASE_TRAP_MIN     ((sucs_ex_char_t)0x7FFFFFF0ULL)
#define SRE_BASE_TRAP_MAX     ((sucs_ex_char_t)0x7FFFFFFEULL)

/* RNUR neography registry (SUCPM / SUTR-8) — a Native-plane codepoint
 * registry, whose contents are delivered as a SuperUnicode plugin. RCCR is a
 * code registry (BCP 47 tags), not codepoints, and has no property here. */
#define SRE_RNUR_MIN          ((sucs_ex_char_t)0x00160000ULL)  /* neographies   */
#define SRE_RNUR_MAX          ((sucs_ex_char_t)0x0016FFFFULL)

/* ── Execution limits & flags ─────────────────────────────────────── */
#define SUTS_SRE_MAX_STATES      64U   /* automaton states (stack frame)  */
#define SUTS_SRE_MAX_TRANSITIONS 128U  /* automaton edges                 */
#define SUTS_SRE_MAX_CLASSES     8U    /* character-class descriptors      */
#define SUTS_SRE_MAX_CLASS_ITEMS 12U   /* items per character class        */
#define SUTS_SRE_MAX_INPUT       512U  /* effect buffer for match helpers  */

typedef uint32_t suts_sre_flags_t;

#define SUTS_SRE_FLAG_NONE           0x00000000u
#define SUTS_SRE_FLAG_IGNORE_CASE    0x00000001u /* case-fold via SUCD-lite */
#define SUTS_SRE_FLAG_MATCH_SCP      0x00000002u /* match SCP in-band       */
#define SUTS_SRE_FLAG_SCF_NORMALIZED 0x00000004u /* enforced SUCF-C input   */
#define SUTS_SRE_FLAG_DIR_AWARE      0x00000008u /* SDF directional anchors */
#define SUTS_SRE_FLAG_MULTILINE      0x00000010u /* ^/$ at line boundaries  */

/* ── Property class matchers (SUCD lookup / direct bitmask) ───────── */
typedef enum {
    SUTS_SRE_PROP_ANY        = 0,  /* \p{any} — all codepoints        */
    SUTS_SRE_PROP_LETTER     = 1,  /* \p{L}  letters                  */
    SUTS_SRE_PROP_NUMBER     = 2,  /* \p{N}  numbers / digits         */
    SUTS_SRE_PROP_ARABIC     = 3,  /* \p{Arabic}                      */
    SUTS_SRE_PROP_RNUR       = 4,  /* \p{RNUR}  neography registry (SUTR-8)  */
    SUTS_SRE_PROP_SCP        = 5,  /* \p{SCP}    System Control Plane */
    SUTS_SRE_PROP_BANCODE    = 6,  /* \p{BANcode} kernel traps        */
    SUTS_SRE_PROP_WHITESPACE = 7,  /* \s                            */
    SUTS_SRE_PROP_WORD       = 8,  /* \w / word character           */
    SUTS_SRE_PROP_ASCII      = 9,  /* \p{ASCII}                     */
    SUTS_SRE_PROP_UPPER      = 10, /* \p{Lu}                        */
    SUTS_SRE_PROP_LOWER      = 11, /* \p{Ll}                        */
    SUTS_SRE_PROP_MARK       = 12, /* \p{M}  combining marks        */
    SUTS_SRE_PROP_NATIVE     = 13, /* \p{Native} native SUCS space   */
    SUTS_SRE_PROP_EXTSUCS    = 14, /* \p{ExtSUCS} plugin space       */
    SUTS_SRE_PROP_GREEK      = 15, /* \p{Greek}                     */
    SUTS_SRE_PROP_CYRILLIC   = 16, /* \p{Cyrillic}                  */
    SUTS_SRE_PROP_HAN        = 17, /* \p{Han}                       */
    SUTS_SRE_PROP_ASSIGNED   = 18, /* \p{Assigned}                  */
    SUTS_SRE_PROP_COUNT      = 19
} suts_sre_prop_t;

/* ── Zero-width assertions ────────────────────────────────────────── */
typedef enum {
    SUTS_SRE_ASSERT_BOL     = 0, /* ^  beginning of text / line       */
    SUTS_SRE_ASSERT_EOL     = 1, /* $  end of text / line             */
    SUTS_SRE_ASSERT_WORD    = 2, /* \b / \b{w} word boundary          */
    SUTS_SRE_ASSERT_NONWORD = 3, /* \B word non-boundary              */
    SUTS_SRE_ASSERT_LTR     = 4, /* \b{LTR} SDF scope boundary        */
    SUTS_SRE_ASSERT_RTL     = 5  /* \b{RTL} SDF scope boundary        */
} suts_sre_assert_t;

/* ── Predicate kinds carried by an automaton edge ─────────────────── */
typedef enum {
    SUTS_SRE_P_EPS    = 0, /* epsilon (no input consumed)          */
    SUTS_SRE_P_ANY    = 1, /* .   any single codepoint             */
    SUTS_SRE_P_EXACT  = 2, /* single codepoint                     */
    SUTS_SRE_P_RANGE  = 3, /* inclusive codepoint range            */
    SUTS_SRE_P_PROP   = 4, /* property class                       */
    SUTS_SRE_P_CLASS  = 5, /* character-class descriptor reference */
    SUTS_SRE_P_ASSERT = 6  /* zero-width assertion                 */
} suts_sre_pred_kind_t;

/* ── Character-class item kinds and set operators ─────────────────── */
typedef enum {
    SUTS_SRE_ITEM_EXACT = 0,
    SUTS_SRE_ITEM_RANGE = 1,
    SUTS_SRE_ITEM_PROP  = 2,
    SUTS_SRE_ITEM_CLASS = 3
} suts_sre_item_kind_t;

typedef enum {
    SUTS_SRE_OP_UNION     = 0, /* ||  (also the implicit default) */
    SUTS_SRE_OP_INTERSECT = 1, /* && */
    SUTS_SRE_OP_DIFF      = 2, /* -- */
    SUTS_SRE_OP_SYMDIFF   = 3  /* ~~ */
} suts_sre_op_t;

typedef struct {
    uint8_t              kind;        /* suts_sre_item_kind_t        */
    uint8_t              op;          /* suts_sre_op_t               */
    uint8_t              negate;      /* item-level \P{...}          */
    uint8_t              class_index; /* nested class (kind==CLASS)  */
    suts_sre_prop_t      property;
    sucs_ex_char_t       lo;
    sucs_ex_char_t       hi;
} suts_sre_class_item_t;

typedef struct {
    uint8_t               negate; /* leading [^ ... ] */
    uint8_t               count;
    suts_sre_class_item_t items[SUTS_SRE_MAX_CLASS_ITEMS];
} suts_sre_class_t;

/* ── Predicate descriptor (used while building edges / classes) ──────── */
typedef struct {
    suts_sre_pred_kind_t kind;          /* what the edge consumes / asserts */
    uint8_t              negate;        /* invert the predicate result      */
    suts_sre_prop_t      property_mask; /* for kind == SUTS_SRE_P_PROP      */
    uint8_t              assertion;     /* suts_sre_assert_t for ASSERT     */
    uint8_t              class_index;   /* for kind == SUTS_SRE_P_CLASS     */
    sucs_ex_char_t       cp_lo;         /* exact codepoint / range low      */
    sucs_ex_char_t       cp_hi;         /* range high (== cp_lo if exact)   */
    bool                 is_exact;
} suts_sre_pred_t;

/* ── Automaton edge (transition) ──────────────────────────────────── */
typedef struct {
    uint8_t              from_state;
    uint8_t              to_state;
    suts_sre_pred_kind_t kind;
    uint8_t              negate;
    suts_sre_prop_t      property_mask;
    uint8_t              assertion;   /* suts_sre_assert_t           */
    uint8_t              class_index;
    sucs_ex_char_t       cp_lo;       /* exact codepoint / range low  */
    sucs_ex_char_t       cp_hi;       /* range high (== cp_lo exact)  */
    bool                 is_exact;
} suts_sre_transition_t;

/* ── Compiled automaton (all storage caller-owned) ────────────────── */
typedef struct {
    suts_sre_transition_t transitions[SUTS_SRE_MAX_TRANSITIONS];
    suts_sre_class_t      classes[SUTS_SRE_MAX_CLASSES];
    uint16_t              transition_count;
    uint8_t               class_count;
    uint8_t               state_count;
    uint8_t               start_state;
    uint64_t              accept_state_mask; /* bitmask of accepting states */
    suts_sre_flags_t      flags;
} suts_sre_compiled_dfa_t;

/* ── Match-tracking context ───────────────────────────────────────── */
typedef struct {
    uint64_t current_state;     /* active automaton state-set bitmask    */
    uint32_t match_start_offset;
    uint32_t match_end_offset;
    bool     is_accepting;
    /* internal streaming bookkeeping (zero-allocation) */
    uint32_t start_of[SUTS_SRE_MAX_STATES];
    uint32_t best_start;
    uint32_t best_end;
    bool     have_match;
    bool     seeded;
    uint32_t prev_cp;
} suts_sre_match_state_t;

/* ── Status codes ─────────────────────────────────────────────────── */
typedef enum {
    SUTS_SRE_OK                   =  0,
    SUTS_SRE_MATCH                =  1,
    SUTS_SRE_NOMATCH              =  2,
    SUTS_SRE_ERR_INVALID_ARG      = -1,
    SUTS_SRE_ERR_STATE_OVERFLOW   = -2,
    SUTS_SRE_ERR_EDGE_OVERFLOW    = -3,
    SUTS_SRE_ERR_CLASS_OVERFLOW   = -4,
    SUTS_SRE_ERR_BAD_PATTERN      = -5,
    SUTS_SRE_ERR_UNSUPPORTED      = -6,
    SUTS_SRE_ERR_BUFFER_TOO_SMALL = -7
} suts_sre_status_t;

/* ── Setup ────────────────────────────────────────────────────────── */
const char* suts_sre_version_string(void);

/* Compile an ASCII-meta pattern of `len` bytes into `out`. Non-ASCII
 * literals are written with \u{...} / \uXXXX / \UXXXXXXXX escapes. */
suts_sre_status_t suts_sre_compile(const char* pattern, size_t len,
                                   suts_sre_flags_t flags,
                                   suts_sre_compiled_dfa_t* out);

/* ── Property test (SUCD lookup for Bridge, direct bitmask otherwise) */
bool suts_sre_prop_match(suts_sre_prop_t prop, sucs_ex_char_t cp);

/* ── Streaming step engine ────────────────────────────────────────── */
/* Initializes a match context; seeds the automaton at offset 0. */
void suts_sre_init_match(suts_sre_match_state_t* match,
                         const suts_sre_compiled_dfa_t* dfa);

/* Feeds a single SUCS codepoint in O(1). Updates *match and returns
 * *match->is_accepting. `current_offset` is the codepoint's index. */
bool suts_sre_step(const suts_sre_compiled_dfa_t* dfa,
                   suts_sre_match_state_t* match,
                   sucs_ex_char_t input_cp,
                   uint32_t current_offset);

/* ── Anchored and searching matchers ──────────────────────────────── */
/* Anchored at `start`: the match MUST begin exactly at `start`. The
 * longest match from that origin is reported. Returns SUTS_SRE_MATCH
 * or SUTS_SRE_NOMATCH (or a negative status). */
suts_sre_status_t suts_sre_match_at(const suts_sre_compiled_dfa_t* dfa,
                                    const sucs_ex_char_t* cps, size_t n,
                                    size_t start,
                                    size_t* out_start, size_t* out_end);

/* Unanchored leftmost, then longest, match at or after `start`. */
suts_sre_status_t suts_sre_search(const suts_sre_compiled_dfa_t* dfa,
                                  const sucs_ex_char_t* cps, size_t n,
                                  size_t start,
                                  size_t* out_start, size_t* out_end);

#ifdef __cplusplus
}
#endif

#endif /* SUTS_SUTS_SRE_H */
