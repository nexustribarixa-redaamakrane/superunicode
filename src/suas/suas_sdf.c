/* SUAS-001 — Structural Directional Framing (SDF) — freestanding impl.
 *
 * Single-pass, zero-allocation, state-machine-driven bidirectional framing
 * with a Dual-Mode Directional Resolver over the SUCD BiDi database.
 * No heap allocation. Compiles with -std=c99 -ffreestanding.
 */

#include "suas/suas_sdf.h"

/* Validate a dir_type value. */
static int sdf_dir_valid(suas_dir_type_t d)
{
    switch (d) {
    case SUAS_DIR_NEUTRAL:
    case SUAS_DIR_LTR:
    case SUAS_DIR_RTL:
    case SUAS_DIR_MIRRORED_LTR:
    case SUAS_DIR_MIRRORED_RTL:
        return 1;
    default:
        return 0;
    }
}

static int sdf_is_writable_dir(suas_dir_type_t d)
{
    return (d == SUAS_DIR_LTR || d == SUAS_DIR_RTL);
}

/* ── SUCD BiDi lookup (embedded subset of sucd/Props/BidiProps.txt) ─ */

typedef struct {
    uint32_t lo;
    uint32_t hi;
    uint32_t mask;
} sucd_bidi_range_t;

static const sucd_bidi_range_t SUCD_BIDI_RANGES[] = {
    /* Whitespace / separators */
    { 0x0009, 0x000D, SUCD_BIDI_WHITESPACE },
    { 0x0020, 0x0020, SUCD_BIDI_WHITESPACE },
    { 0x00A0, 0x00A0, SUCD_BIDI_WHITESPACE },
    { 0x1680, 0x1680, SUCD_BIDI_WHITESPACE },
    { 0x2000, 0x200A, SUCD_BIDI_WHITESPACE },
    { 0x2028, 0x2029, SUCD_BIDI_WHITESPACE },
    { 0x202F, 0x202F, SUCD_BIDI_WHITESPACE },
    { 0x205F, 0x205F, SUCD_BIDI_WHITESPACE },
    { 0x3000, 0x3000, SUCD_BIDI_WHITESPACE },

    /* ASCII paired-format (mirrorable) glyphs */
    { 0x0028, 0x0029, SUCD_BIDI_MIRRORED },
    { 0x003C, 0x003E, SUCD_BIDI_MIRRORED },
    { 0x005B, 0x005D, SUCD_BIDI_MIRRORED },
    { 0x007B, 0x007D, SUCD_BIDI_MIRRORED },

    /* General Punctuation paired-format glyphs */
    { 0x2018, 0x2019, SUCD_BIDI_MIRRORED },
    { 0x201C, 0x201D, SUCD_BIDI_MIRRORED },
    { 0x2039, 0x203A, SUCD_BIDI_MIRRORED },

    /* Mathematical brackets (mirrorable) */
    { 0x27E6, 0x27EB, SUCD_BIDI_MIRRORED },

    /* Heavy brackets */
    { 0x2772, 0x2773, SUCD_BIDI_MIRRORED },

    /* White square brackets */
    { 0x301A, 0x301B, SUCD_BIDI_MIRRORED },

    /* Fullwidth forms (mirrorable) */
    { 0xFF08, 0xFF09, SUCD_BIDI_MIRRORED },
    { 0xFF3B, 0xFF3D, SUCD_BIDI_MIRRORED },
    { 0xFF5B, 0xFF5D, SUCD_BIDI_MIRRORED },

    /* Latin script (strong LTR) */
    { 0x0041, 0x005A, SUCD_BIDI_LTR },
    { 0x0061, 0x007A, SUCD_BIDI_LTR },
    { 0x00C0, 0x024F, SUCD_BIDI_LTR },
    { 0x0370, 0x03FF, SUCD_BIDI_LTR },  /* Greek */
    { 0x0400, 0x04FF, SUCD_BIDI_LTR },  /* Cyrillic */
    { 0x1E00, 0x1EFF, SUCD_BIDI_LTR },  /* Latin Extended Additional */

    /* Arabic script (right-to-left, ARABIC_AL) */
    { 0x0600, 0x06FF, SUCD_BIDI_ARABIC_AL },
    { 0x0750, 0x077F, SUCD_BIDI_ARABIC_AL },
    { 0x08A0, 0x08FF, SUCD_BIDI_ARABIC_AL },
    { 0xFB50, 0xFDFF, SUCD_BIDI_ARABIC_AL },
    { 0xFE70, 0xFEFF, SUCD_BIDI_ARABIC_AL },

    /* Hebrew script (strong RTL) */
    { 0x0590, 0x05FF, SUCD_BIDI_RTL },
    { 0xFB1D, 0xFB4F, SUCD_BIDI_RTL },

    /* Thaana (RTL) */
    { 0x0780, 0x07BF, SUCD_BIDI_RTL },

    /* N'Ko (RTL) */
    { 0x07C0, 0x07FF, SUCD_BIDI_RTL },

    /* Adlam (RTL) */
    { 0x1E900, 0x1E95F, SUCD_BIDI_RTL },

    /* Arabic-Indic digits (neutral) */
    { 0x0660, 0x0669, SUCD_BIDI_NEUTRAL },
    { 0x06F0, 0x06F9, SUCD_BIDI_NEUTRAL },
};

#define SUCD_BIDI_RANGE_COUNT \
    (sizeof(SUCD_BIDI_RANGES) / sizeof(SUCD_BIDI_RANGES[0]))

uint32_t suas_sucd_bidi(uint32_t cp)
{
    size_t i;

    if (!suas_sucd_is_unicode_bridge(cp)) {
        return SUCD_BIDI_NEUTRAL;
    }

    for (i = 0; i < SUCD_BIDI_RANGE_COUNT; ++i) {
        if (cp >= SUCD_BIDI_RANGES[i].lo && cp <= SUCD_BIDI_RANGES[i].hi) {
            return SUCD_BIDI_RANGES[i].mask;
        }
    }

    /* Unmapped codepoints within the bridge default to neutral. */
    return SUCD_BIDI_NEUTRAL;
}

/* ── Classification ────────────────────────────────────────────── */

suas_sdf_status_t suas_sdf_classify(uint32_t cp, suas_dir_type_t resolved,
                                    uint32_t bidi_mask,
                                    suas_dir_type_t* out_dir, int* out_mirror)
{
    if (out_dir == NULL || out_mirror == NULL || !sdf_dir_valid(resolved)) {
        return SUAS_SDF_ERR_INVALID_ARG;
    }

    /* `cp` is retained in the public signature for forward compatibility
     * (e.g. intrinsic script tables); classification is mask-driven. */
    (void)cp;

    *out_mirror = 0;

    /* 1. Paired-format glyph: mirror per active isolate direction. */
    if (bidi_mask & SUCD_BIDI_MIRRORED) {
        *out_mirror = 1;
        if (resolved == SUAS_DIR_RTL || resolved == SUAS_DIR_MIRRORED_RTL) {
            *out_dir = SUAS_DIR_MIRRORED_RTL;
        } else {
            *out_dir = SUAS_DIR_MIRRORED_LTR;
        }
        return SUAS_SDF_OK;
    }

    /* 2. Arabic letter or strong RTL resolves right-to-left. */
    if ((bidi_mask & SUCD_BIDI_ARABIC_AL) || (bidi_mask & SUCD_BIDI_RTL)) {
        *out_dir = SUAS_DIR_RTL;
        return SUAS_SDF_OK;
    }

    /* 3. Strong LTR resolves left-to-right. */
    if (bidi_mask & SUCD_BIDI_LTR) {
        *out_dir = SUAS_DIR_LTR;
        return SUAS_SDF_OK;
    }

    /* 4. Everything else (neutral / whitespace / unmapped) is neutral. */
    *out_dir = SUAS_DIR_NEUTRAL;
    return SUAS_SDF_OK;
}

/* ── State machine ─────────────────────────────────────────────── */

static suas_sdf_isolate_t* sdf_active(suas_sdf_state_t* st)
{
    return &st->stack[st->depth - 1];
}

void suas_sdf_init(suas_sdf_state_t* st, suas_dir_type_t initial_dir)
{
    size_t i;

    if (st == NULL) {
        return;
    }
    if (!sdf_dir_valid(initial_dir) || !sdf_is_writable_dir(initial_dir)) {
        initial_dir = SUAS_DIR_LTR;
    }
    for (i = 0; i < SUAS_SDF_STACK_DEPTH; ++i) {
        st->stack[i].base_dir = SUAS_DIR_LTR;
        st->stack[i].cur_dir  = SUAS_DIR_LTR;
    }
    st->depth       = 1; /* root isolate always present */
    st->initial_dir = initial_dir;
    st->runtime     = SUAS_SDF_RUNTIME_ACTIVE;
    st->stack[0].base_dir = initial_dir;
    st->stack[0].cur_dir  = initial_dir;
}

suas_sdf_status_t suas_sdf_process_codepoint(suas_sdf_state_t* st, uint32_t cp,
                                             suts32_framed_t* out, size_t* out_count)
{
    uint32_t bidi_mask;
    suas_dir_type_t dir;
    int mirror;

    if (st == NULL || out == NULL) {
        return SUAS_SDF_ERR_INVALID_ARG;
    }
    if (st->runtime == SUAS_SDF_RUNTIME_ERROR ||
        st->runtime == SUAS_SDF_RUNTIME_ENDED) {
        return SUAS_SDF_ERR_STATE;
    }

    /* ── Explicit Directorial Mode: SCP directives mutate the stack ── */
    switch (cp) {
    case SCP_DIR_LTR:
        sdf_active(st)->cur_dir = SUAS_DIR_LTR;
        return SUAS_SDF_OK;
    case SCP_DIR_RTL:
        sdf_active(st)->cur_dir = SUAS_DIR_RTL;
        return SUAS_SDF_OK;
    case SCP_DIR_ISOLATE_PUSH:
        if (st->depth >= SUAS_SDF_STACK_DEPTH) {
            st->runtime = SUAS_SDF_RUNTIME_ERROR;
            return SUAS_SDF_ERR_STACK_OVERFLOW;
        }
        {
            suas_sdf_isolate_t* parent = sdf_active(st);
            st->stack[st->depth].base_dir = parent->cur_dir;
            st->stack[st->depth].cur_dir  = parent->cur_dir;
            st->depth++;
        }
        return SUAS_SDF_OK;
    case SCP_DIR_ISOLATE_POP:
        if (st->depth <= 1) {
            st->runtime = SUAS_SDF_RUNTIME_ERROR;
            return SUAS_SDF_ERR_STACK_UNDERFLOW;
        }
        st->depth--;
        return SUAS_SDF_OK;
    default:
        break;
    }

    /* ── Ordinary codepoint: resolve via Dual-Mode resolver ───────── */
    bidi_mask = suas_sucd_bidi(cp);

    if (suas_sdf_classify(cp, sdf_active(st)->cur_dir, bidi_mask,
                          &dir, &mirror) != SUAS_SDF_OK) {
        st->runtime = SUAS_SDF_RUNTIME_ERROR;
        return SUAS_SDF_ERR_INVALID_ARG;
    }

    out->codepoint = cp & SUAS_CODEPOINT_MAX;
    out->dir_type  = (uint64_t)dir;
    out->mirrored  = (uint64_t)(mirror ? 1u : 0u);
    out->reserved  = 0;

    if (out_count != NULL) {
        (*out_count)++;
    }
    return SUAS_SDF_OK;
}

suas_sdf_status_t suas_sdf_finish(suas_sdf_state_t* st)
{
    if (st == NULL) {
        return SUAS_SDF_ERR_INVALID_ARG;
    }
    if (st->runtime == SUAS_SDF_RUNTIME_ERROR) {
        return SUAS_SDF_ERR_STATE;
    }
    st->runtime = SUAS_SDF_RUNTIME_ENDED;
    return SUAS_SDF_OK;
}

suas_dir_type_t suas_sdf_current_dir(const suas_sdf_state_t* st)
{
    if (st == NULL || st->depth < 1) {
        return SUAS_DIR_LTR;
    }
    return st->stack[st->depth - 1].cur_dir;
}

int suas_sdf_depth(const suas_sdf_state_t* st)
{
    if (st == NULL) {
        return 0;
    }
    return st->depth;
}

suas_sdf_runtime_t suas_sdf_runtime(const suas_sdf_state_t* st)
{
    if (st == NULL) {
        return SUAS_SDF_RUNTIME_ERROR;
    }
    return st->runtime;
}

/* ── One-shot full-buffer framing (streaming API is normative) ─── */

suas_sdf_status_t suas_sdf_frame(const uint32_t* cps, size_t cp_count,
                                 suts32_framed_t* out, size_t out_cap,
                                 size_t* out_count)
{
    suas_sdf_state_t st;
    size_t i;
    size_t written = 0;

    if (cps == NULL || out == NULL || out_cap == 0) {
        return SUAS_SDF_ERR_INVALID_ARG;
    }

    suas_sdf_init(&st, SUAS_DIR_LTR);

    for (i = 0; i < cp_count; ++i) {
        size_t before = written;
        suts32_framed_t scratch;
        suts32_framed_t* slot = (written < out_cap) ? &out[written] : &scratch;
        suas_sdf_status_t rc =
            suas_sdf_process_codepoint(&st, cps[i], slot, &written);
        if (rc == SUAS_SDF_ERR_STATE) {
            return SUAS_SDF_ERR_STATE;
        }
        if (written > before && slot == &scratch) {
            return SUAS_SDF_ERR_BUFFER_TOO_SMALL;
        }
    }

    if (out_count != NULL) {
        *out_count = written;
    }
    return SUAS_SDF_OK;
}
