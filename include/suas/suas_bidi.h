#ifndef SUAS_SUAS_BIDI_H
#define SUAS_SUAS_BIDI_H

/* SUAS-BIDI — Structural Unicode Bidirectional Algorithm Engine
 *
 * A freestanding, zero-allocation C99 port of the Unicode Bidirectional
 * Algorithm (UAX #9) as the processing model behind SUAS-001 Structural
 * Directional Framing (SDF). Where the SDF *framer* is a single-pass
 * state machine that produces framed output words at decode time, the
 * SUAS-BIDI *display resolver* implements the full UBA phases:
 *
 *   P1-P3   paragraph level (first-strong / auto)
 *   X1-X8   explicit embedding, override & isolate levels
 *   X9      remove explicit formatting & isolate control characters
 *   W1-W7   weak type resolution
 *   N0-N2   neutral + bracket-pair resolution
 *   I1-I2   implicit levels
 *   L1-L4   reordering (sep/line boundary, whitespace, mirror, reverse)
 *
 * The engine operates on caller-provided arrays only. No heap. Fixed
 * internal stacks guarded against overflow per UAX #9 BD3.
 *
 * See docs/suas/SUAS-001-sdf.md §5.5 and UAX #9 for normative rules.
 */

#include <stdint.h>
#include <stddef.h>

#include "suas/suas_core.h"
#include "suas/suas_sucd.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ── Compile-time configuration (overridable) ─────────────────── */

#ifndef SUAS_BIDI_MAX_LEN
#define SUAS_BIDI_MAX_LEN 4096   /* max codepoints / levels in a run */
#endif

#ifndef SUAS_BIDI_MAX_EXPLICIT
#define SUAS_BIDI_MAX_EXPLICIT 125 /* X9: max explicit chars per run */
#endif

/* ── Bidirectional character types (UAX #9 BD1 §3.3.2) ────────── */

typedef enum {
    SUAS_BIDI_L   =  0,  /* Left-to-right */
    SUAS_BIDI_R   =  1,  /* Right-to-left */
    SUAS_BIDI_AL  =  2,  /* Arabic letter */
    SUAS_BIDI_EN  =  3,  /* European number */
    SUAS_BIDI_ES  =  4,  /* European separator */
    SUAS_BIDI_ET  =  5,  /* European terminator */
    SUAS_BIDI_AN  =  6,  /* Arabic number */
    SUAS_BIDI_CS  =  7,  /* Common separator */
    SUAS_BIDI_NSM =  8,  /* Nonspacing mark */
    SUAS_BIDI_BN  =  9,  /* Boundary neutral */
    SUAS_BIDI_B   = 10,  /* Paragraph separator */
    SUAS_BIDI_S   = 11,  /* Segment separator */
    SUAS_BIDI_WS  = 12,  /* Whitespace */
    SUAS_BIDI_ON  = 13,  /* Other neutral */
    SUAS_BIDI_LRE = 14,  /* Left-to-right embedding */
    SUAS_BIDI_LRO = 15,  /* Left-to-right override */
    SUAS_BIDI_RLE = 16,  /* Right-to-left embedding */
    SUAS_BIDI_RLO = 17,  /* Right-to-left override */
    SUAS_BIDI_PDF = 18,  /* Pop directional formatting */
    SUAS_BIDI_LRI = 19,  /* Left-to-right isolate */
    SUAS_BIDI_RLI = 20,  /* Right-to-left isolate */
    SUAS_BIDI_FSI = 21,  /* First strong isolate */
    SUAS_BIDI_PDI = 22,  /* Pop directional isolate */
    SUAS_BIDI_COUNT
} suas_bidi_class_t;

/* Convenience classification predicates (BD1). */
static inline int suasi_bidi_is_strong(suas_bidi_class_t c)
{
    return c == SUAS_BIDI_L || c == SUAS_BIDI_R || c == SUAS_BIDI_AL;
}
static inline int suasi_bidi_is_weak(suas_bidi_class_t c)
{
    return (c >= SUAS_BIDI_EN && c <= SUAS_BIDI_NSM);
}
static inline int suasi_bidi_is_neutral(suas_bidi_class_t c)
{
    return (c == SUAS_BIDI_B || c == SUAS_BIDI_S || c == SUAS_BIDI_WS ||
            c == SUAS_BIDI_ON);
}
static inline int suasi_bidi_is_isolate(suas_bidi_class_t c)
{
    return (c == SUAS_BIDI_LRI || c == SUAS_BIDI_RLI || c == SUAS_BIDI_FSI);
}
static inline int suasi_bidi_is_embedding(suas_bidi_class_t c)
{
    return (c == SUAS_BIDI_LRE || c == SUAS_BIDI_RLE);
}
static inline int suasi_bidi_is_override(suas_bidi_class_t c)
{
    return (c == SUAS_BIDI_LRO || c == SUAS_BIDI_RLO);
}
static inline int suasi_bidi_is_control(suas_bidi_class_t c)
{
    return (c == SUAS_BIDI_LRE || c == SUAS_BIDI_LRO || c == SUAS_BIDI_RLE ||
            c == SUAS_BIDI_RLO || c == SUAS_BIDI_PDF || c == SUAS_BIDI_LRI ||
            c == SUAS_BIDI_RLI || c == SUAS_BIDI_FSI || c == SUAS_BIDI_PDI ||
            c == SUAS_BIDI_BN);
}

/* ── Directional formatting / isolate control characters (§2) ─── */

#define SUAS_BIDI_CP_LRM  0x200EUL  /* left-to-right mark         */
#define SUAS_BIDI_CP_RLM  0x200FUL  /* right-to-left mark         */
#define SUAS_BIDI_CP_ALM  0x061CUL  /* arabic letter mark         */
#define SUAS_BIDI_CP_LRE  0x202AUL
#define SUAS_BIDI_CP_RLE  0x202BUL
#define SUAS_BIDI_CP_PDF  0x202CUL
#define SUAS_BIDI_CP_LRO  0x202DUL
#define SUAS_BIDI_CP_RLO  0x202EUL
#define SUAS_BIDI_CP_LRI  0x2066UL
#define SUAS_BIDI_CP_RLI  0x2067UL
#define SUAS_BIDI_CP_FSI  0x2068UL
#define SUAS_BIDI_CP_PDI  0x2069UL

/* Paragraph direction selectors (P2/P3). */
typedef enum {
    SUAS_BIDI_PARAGRAPH_AUTO = 0,   /* first-strong (P2/P3)          */
    SUAS_BIDI_PARAGRAPH_LTR  = 1,   /* force paragraph level 0        */
    SUAS_BIDI_PARAGRAPH_RTL  = 2    /* force paragraph level 1        */
} suas_bidi_paragraph_dir_t;

/* ── Per-codepoint results ─────────────────────────────────────── */

typedef struct {
    uint32_t         cp;         /* the SUCS codepoint                    */
    suas_bidi_class_t cls;       /* resolved BiDi class                    */
    uint8_t          level;      /* resolved embedding level (0..126)       */
    int              mirrored;   /* 1 if glyph must be mirrored (L4)        */
    int              removed;    /* 1 if removed by X9 (no glyph rendered)  */
} suas_bidi_run_t;               /* one display element per input codepoint */

/* ── Status codes ──────────────────────────────────────────────── */

typedef enum {
    SUAS_BIDI_OK                =  0,
    SUAS_BIDI_ERR_INVALID_ARG   = -1,
    SUAS_BIDI_ERR_TOO_LONG      = -2, /* run exceeds SUAS_BIDI_MAX_LEN  */
    SUAS_BIDI_ERR_EXPLICIT_OVERFLOW = -3, /* explicit/isolate overflow (BD3) */
    SUAS_BIDI_ERR_PARITY        = -4 /* reserved */
} suas_bidi_status_t;

/* ── Full UBA entry point ──────────────────────────────────────── */

/**
 * Resolves the full bidirectional layout of one paragraph (a contiguous
 * run of codepoints ending at the paragraph separator, which is exempt
 * from reordering per L1).
 *
 * @param cps        the codepoints, in logical order, within one paragraph.
 * @param cp_count   number of codepoints (must be <= SUAS_BIDI_MAX_LEN).
 * @param paragraph  base direction selector (AUTO/LTR/RTL).
 * @param out        receives one suas_bidi_run_t per input codepoint,
 *                   in the same logical order (do NOT reorder `out`).
 * @param visual     optional output array of size cp_count receiving the
 *                   *visual* order as indices into `out`. May be NULL.
 * @param out_para_level receives the resolved paragraph level (0 or 1).
 * @return SUAS_BIDI_OK or a negative error.
 */
suas_bidi_status_t suas_bidi_resolve_paragraph(
    const uint32_t* cps, size_t cp_count,
    suas_bidi_paragraph_dir_t paragraph,
    suas_bidi_run_t* out,
    int* visual,
    uint8_t* out_para_level);

/**
 * Full building block without implicit-level resolution on the last
 * paragraph run used as input to a multi-paragraph reflow caller. This is
 * exposed for engines that reflow whole lines; single-paragraph callers
 * should use suas_bidi_resolve_paragraph().
 */
suas_bidi_status_t suas_bidi_resolve_paragraph_ex(
    const uint32_t* cps, size_t cp_count,
    suas_bidi_paragraph_dir_t paragraph,
    suas_bidi_run_t* out,
    int* visual,
    uint8_t* out_para_level,
    int last_paragraph);

/**
 * Classifies a codepoint relevant to the UBA. Returns SUAS_BIDI class for
 * Unicode-Bridge codepoints (using the embedded SUCD class table); SCP / SDF
 * directives are mapped onto the nearest UBA counterpart; native codepoints
 * default to L. Shared with the SDF resolver.
 */
suas_bidi_class_t suas_bidi_classify_cp(uint32_t cp);

/**
 * Returns the Bidi_Mirrored flag + paired-bracket codepoint for mirroring
 * (L4). Out-of-bridge codepoints and non-mirrorable glyphs return
 * paired = cp and mirrored = 0.
 */
void suas_bidi_mirror(uint32_t cp, uint32_t* paired, int* mirrored);

#ifdef __cplusplus
}
#endif

#endif /* SUAS_SUAS_BIDI_H */
