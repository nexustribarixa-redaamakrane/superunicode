#ifndef SUAS_SUAS_STS_H
#define SUAS_SUAS_STS_H

/* SUAS-006 — System Text Segmentation (STS)
 *
 * Core architecture for text segmentation over the full 64-bit SUCS /
 * ExtSUCS codepoint space: grapheme-cluster, word, and sentence boundaries
 * (a UAX #29 analog). Where UAX #29 relies on multi-pass algorithmic rulesets
 * and regular-expression lookahead, STS is a zero-allocation, single-pass
 * deterministic state machine that resolves each boundary in O(1) per
 * codepoint from a small stack-allocated sliding window and a bounded
 * carried-context register.
 *
 * Unicode Bridge behaviour follows UAX #29 via curated class tables; the
 * System Control Plane carries explicit segmentation directives in the block
 * 0x00110040-0x0011004F. Input codepoints are 64-bit ExtSUCS
 * (sucs_ex_char_t); Native/plugin codepoints default to an opaque boundary.
 *
 * Freestanding C99; no heap.
 *
 * See docs/suas/SUAS-006-sts.md for the normative specification.
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "suas/suas_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ── Specification identifiers ────────────────────────────────────── */
#define SUAS_STS_SPEC_ID       "SUAS-006"
#define SUAS_STS_SPEC_TITLE    "System Text Segmentation"
#define SUAS_STS_VERSION_MAJOR 1
#define SUAS_STS_VERSION_MINOR 0
#define SUAS_STS_VERSION_PATCH 0

/* ── 64-bit ExtSUCS codepoint type (guarded to coexist) ───────────── */
#ifndef EXTSUCS_CHAR_T_DEFINED
#define EXTSUCS_CHAR_T_DEFINED
typedef uint64_t sucs_ex_char_t;
#endif

/* ── Base SUCS zone boundaries (duplicated for freestanding STS) ──── */
#ifndef SUAS_STS_ZONE_UNICODE_MAX
#define SUAS_STS_ZONE_UNICODE_MAX  ((sucs_ex_char_t)0x0010FFFFULL)
#define SUAS_STS_ZONE_SCP_MIN      ((sucs_ex_char_t)0x00110000ULL)
#define SUAS_STS_ZONE_SCP_MAX      ((sucs_ex_char_t)0x0011FFFFULL)
#define SUAS_STS_ZONE_NATIVE_MIN   ((sucs_ex_char_t)0x00120000ULL)
#define SUAS_STS_BASE_SUCS_MAX     ((sucs_ex_char_t)0x7FFFFFFFULL)
#define SUAS_STS_BASE_TRAP_MIN     ((sucs_ex_char_t)0x7FFFFFF0ULL)
#define SUAS_STS_BASE_TRAP_MAX     ((sucs_ex_char_t)0x7FFFFFFEULL)
#define SUAS_STS_BASE_SENTINEL     ((sucs_ex_char_t)0x7FFFFFFFULL)
#endif

/* ── Explicit SCP Segmentation Directives (System Control Plane) ────
 * The block 0x00110040-0x0011004F is reserved for text segmentation. It is
 * disjoint from the SBR break markers (0x00110020-0x00110022), the SDF
 * directional block (0x00110100), the math directive block (0x00110030-
 * 0x0011003F), the SSP script directives (0x00112000-0x00119FFF) and the
 * BANcodes (0x0011A000+). */
#define SCP_STS_BASE         ((sucs_ex_char_t)0x00110040ULL)
#define SCP_STS_BREAK_FORCE  (SCP_STS_BASE + 0x00ULL) /* force a boundary    */
#define SCP_STS_BREAK_BLOCK  (SCP_STS_BASE + 0x01ULL) /* prohibit a boundary */
#define SCP_STS_BREAK_ALLOW  (SCP_STS_BASE + 0x02ULL) /* permit a boundary   */
#define SCP_STS_MAX          ((sucs_ex_char_t)0x0011004FULL)

/* ── Segmentation level ───────────────────────────────────────────── */
typedef enum {
    SUAS_STS_LEVEL_GRAPHEME = 0, /* user-perceived character     */
    SUAS_STS_LEVEL_WORD     = 1, /* word / selection boundary    */
    SUAS_STS_LEVEL_SENTENCE = 2  /* sentence / paragraph boundary */
} suas_sts_level_t;

/* ── Boundary status of a single offset ───────────────────────────── */
typedef enum {
    SUAS_STS_BOUNDARY_NONE  = 0, /* no boundary / do not break      */
    SUAS_STS_BOUNDARY_ALLOW = 1, /* permitted (opportunistic) break */
    SUAS_STS_BOUNDARY_MUST  = 2  /* mandatory break                 */
} suas_sts_boundary_t;

/* ── Grapheme_Cluster_Break value domain (curated UAX #29 set) ────── */
typedef enum {
    SUAS_STS_GCB_OTHER       = 0,
    SUAS_STS_GCB_CR          = 1,
    SUAS_STS_GCB_LF          = 2,
    SUAS_STS_GCB_CONTROL     = 3,
    SUAS_STS_GCB_EXTEND      = 4,
    SUAS_STS_GCB_ZWJ         = 5,
    SUAS_STS_GCB_RI          = 6,
    SUAS_STS_GCB_PREPEND     = 7,
    SUAS_STS_GCB_SPACINGMARK = 8,
    SUAS_STS_GCB_L           = 9,
    SUAS_STS_GCB_V           = 10,
    SUAS_STS_GCB_T           = 11,
    SUAS_STS_GCB_LV          = 12,
    SUAS_STS_GCB_LVT         = 13
} suas_sts_gcb_t;

/* ── Word_Break value domain (curated UAX #29 set) ────────────────── */
typedef enum {
    SUAS_STS_WB_OTHER        = 0,
    SUAS_STS_WB_CR           = 1,
    SUAS_STS_WB_LF           = 2,
    SUAS_STS_WB_NEWLINE      = 3,
    SUAS_STS_WB_EXTEND       = 4,
    SUAS_STS_WB_ZWJ          = 5,
    SUAS_STS_WB_FORMAT       = 6,
    SUAS_STS_WB_RI           = 7,
    SUAS_STS_WB_KATAKANA     = 8,
    SUAS_STS_WB_HEBREW       = 9,
    SUAS_STS_WB_ALETTER      = 10,
    SUAS_STS_WB_SINGLE_QUOTE = 11,
    SUAS_STS_WB_DOUBLE_QUOTE = 12,
    SUAS_STS_WB_MIDNUMLET    = 13,
    SUAS_STS_WB_MIDLETTER    = 14,
    SUAS_STS_WB_MIDNUM       = 15,
    SUAS_STS_WB_NUMERIC      = 16,
    SUAS_STS_WB_EXTENDNUMLET = 17,
    SUAS_STS_WB_WSEGSPACE    = 18
} suas_sts_wb_t;

/* ── Sentence_Break value domain (curated UAX #29 set) ────────────── */
typedef enum {
    SUAS_STS_SB_OTHER     = 0,
    SUAS_STS_SB_CR        = 1,
    SUAS_STS_SB_LF        = 2,
    SUAS_STS_SB_EXTEND    = 3,
    SUAS_STS_SB_SEP       = 4,
    SUAS_STS_SB_FORMAT    = 5,
    SUAS_STS_SB_SP        = 6,
    SUAS_STS_SB_LOWER     = 7,
    SUAS_STS_SB_UPPER     = 8,
    SUAS_STS_SB_OLETTER   = 9,
    SUAS_STS_SB_NUMERIC   = 10,
    SUAS_STS_SB_ATERM     = 11,
    SUAS_STS_SB_SCONTINUE = 12,
    SUAS_STS_SB_STERM     = 13,
    SUAS_STS_SB_CLOSE     = 14
} suas_sts_sb_t;

/* ── Zero-allocation sliding-window state ─────────────────────────
 * The window retains the last SUAS_STS_WINDOW_SIZE codepoints for
 * introspection and bounded look-back; the boundary decision itself is
 * driven by the carried-context registers below, which encode the UAX #29
 * "ignore" and run-length state so no unbounded history is ever needed. */
#define SUAS_STS_WINDOW_SIZE 4U

typedef struct {
    sucs_ex_char_t   codepoints[SUAS_STS_WINDOW_SIZE]; /* ring buffer      */
    uint8_t          head;    /* ring index of the oldest retained sample  */
    uint8_t          count;   /* valid samples, 0..SUAS_STS_WINDOW_SIZE    */
    suas_sts_level_t level;

    /* Grapheme_Cluster_Break carried context. */
    uint8_t          gcb_prev;   /* class of the previous codepoint       */
    uint8_t          gcb_ri;     /* consecutive RI parity (0/1)           */
    uint8_t          gcb_linker; /* InCB Linker seen through Extend*      */
    uint8_t          gcb_epi;    /* last significant class was ExtPict    */
    uint8_t          gcb_zwj;    /* ExtPict Extend* ZWJ seen (GB11)       */

    /* Word_Break carried context (WB4 "ignore" rule). */
    uint8_t          wb_prev;    /* last effective Word_Break class       */
    uint8_t          wb_before;  /* effective class before a mid punct    */
    uint8_t          wb_ri;      /* consecutive RI parity (0/1)           */
    uint8_t          wb_zwj;     /* previous effective token was ZWJ      */

    /* Sentence_Break carried context (SB5 "ignore" rule). */
    uint8_t          sb_prev;    /* last effective Sentence_Break class   */
    uint8_t          sb_before;  /* class before an ambiguous terminator  */
    uint8_t          sb_term;    /* SATerm Close* Sp* progress            */
    uint8_t          sb_aterm;   /* terminator sequence began with ATerm  */
} suas_sts_state_t;

/* ── Setup ────────────────────────────────────────────────────────── */
const char*      suas_sts_version_string(void);
const char*      suas_sts_level_name(suas_sts_level_t level);
void             suas_sts_init(suas_sts_state_t* st, suas_sts_level_t level);
suas_sts_level_t suas_sts_level(const suas_sts_state_t* st);

/* ── Classification (§4-§6) — works for any 64-bit codepoint ─────── */
suas_sts_gcb_t suas_sts_classify_grapheme(sucs_ex_char_t cp);
suas_sts_wb_t  suas_sts_classify_word(sucs_ex_char_t cp);
suas_sts_sb_t  suas_sts_classify_sentence(sucs_ex_char_t cp);

/* ── SCP segmentation directives ──────────────────────────────────── */
/* True when cp is an SCP segmentation directive; writes the boundary it
 * imposes to *out when out is non-NULL. Directives are transparent: they
 * are not pushed onto the window and do not alter the carried context. */
bool suas_sts_is_directive(sucs_ex_char_t cp, suas_sts_boundary_t* out);

/* ── Single-pass stream engine ─────────────────────────────────────
 * Feed one codepoint at a time. Returns the boundary status of the offset
 * between the previously consumed codepoint and cp. The first codepoint
 * returns SUAS_STS_BOUNDARY_MUST (start of text, GB1/WB1/SB1). Call
 * suas_sts_flush() at end of stream to obtain the end-of-text boundary. */
suas_sts_boundary_t suas_sts_evaluate(suas_sts_state_t* st, sucs_ex_char_t cp);

/* End-of-text boundary: always SUAS_STS_BOUNDARY_MUST (GB2/WB2/SB2). */
suas_sts_boundary_t suas_sts_flush(suas_sts_state_t* st);

#ifdef __cplusplus
}
#endif

#endif /* SUAS_SUAS_STS_H */
