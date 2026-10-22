/*
 * SUTR-9 — SuperUnicode Mathematical Notation and Layout Support (SUMNLS)
 *
 * The reference implementation of the layout engine of SUTR-9 §5 and the
 * System Control Plane math directives of §6.1.
 *
 * Architectural notes:
 *
 *   - Freestanding C99. No libc, no <string.h>, no heap. The only
 *     dependencies are the integer types and the SUCPM property model,
 *     from which every atom is classified.
 *   - Single pass. The engine walks the token stream once. The only
 *     look-ahead is the one-atom deferral of §5.8, which exists because
 *     the reclassification rule needs to know whether an atom closes its
 *     run.
 *   - O(1) per unit. Every table lookup is a direct index or a range
 *     test; every box operation is a bounded walk of the children of
 *     one box.
 *   - Bounded. Every array has a compile-time bound; an input that
 *     exceeds a bound is clamped, never fatal (§9 recommendation 7).
 *
 * The normative contract of every function below is the section of
 * docs/sutr/SUTR-9-sumnls.md named in its comment.
 */

#include "sutr/sutr_sumnls.h"

/* ── Identifiers (§1, §10) ───────────────────────────────────────── */

const char *sutr_sumnls_version_string(void)
{
    return "0.1.0";
}

/* ── A freestanding byte zeroer (no <string.h>) ──────────────────── */

static void sumnls_zero(void *target, size_t length)
{
    unsigned char *bytes = (unsigned char *)target;
    size_t i;

    for (i = 0u; i < length; i++) {
        bytes[i] = 0u;
    }
}

/* ── The math constants of §4.4 ──────────────────────────────────── */

void sutr_sumnls_default_constants(int32_t units_per_em,
                                   sumnls_constants_t *out)
{
    if (out == NULL) {
        return;
    }
    if (units_per_em <= 0) {
        units_per_em = 1000;
    }
    out->units_per_em        = units_per_em;
    out->axis_height         = units_per_em / 4;
    out->rule_thickness      = units_per_em / 20;
    out->numerator_gap       = units_per_em / 8;
    out->denominator_gap     = units_per_em / 8;
    out->superscript_shift   = units_per_em / 3;
    out->subscript_shift     = units_per_em / 5;
    out->script_gap          = units_per_em / 12;
    out->degree_shift        = units_per_em / 5;
    out->radical_clearance   = units_per_em / 20;
    out->thin_space          = units_per_em / 6;
    out->medium_space        = (int32_t)((uint32_t)units_per_em * 2u / 9u);
    out->thick_space         = (int32_t)((uint32_t)units_per_em * 5u / 18u);
}

void sutr_sumnls_default_metrics(uint64_t codepoint,
                                 sumnls_glyph_metrics_t *out)
{
    (void)codepoint;
    if (out == NULL) {
        return;
    }
    out->advance = 500u;
    out->ascent  = 750;
    out->descent = 250;
}

/* ── The directive decode of §6.1 ────────────────────────────────── */

/*
 * The block `0x00110030-0x0011003F` is a closed enumeration of sixteen
 * directives. The decode is one range test and one subtraction, so the
 * whole dispatch of §6.1 is O(1) per codepoint.
 */
sumnls_directive_t sutr_sumnls_directive_of(uint64_t codepoint)
{
    uint64_t offset;

    if (codepoint < SUMNLS_MATH_DIRECTIVE_MIN ||
        codepoint > SUMNLS_MATH_DIRECTIVE_MAX) {
        return SUMNLS_DIRECTIVE_NONE;
    }
    offset = codepoint - SUMNLS_MATH_DIRECTIVE_MIN;
    if (offset == 0u) {
        return SUMNLS_DIRECTIVE_ISOLATE_START;
    }
    if (offset == 1u) {
        return SUMNLS_DIRECTIVE_ISOLATE_END;
    }
    if (offset == 2u) {
        return SUMNLS_DIRECTIVE_MODE_LTR;
    }
    if (offset == 3u) {
        return SUMNLS_DIRECTIVE_MODE_RTL;
    }
    if (offset == 4u) {
        return SUMNLS_DIRECTIVE_ALIGN_TAB;
    }
    if (offset == 5u) {
        return SUMNLS_DIRECTIVE_ALIGN_ROW;
    }
    if (offset == 6u) {
        return SUMNLS_DIRECTIVE_STYLE_PUSH;
    }
    if (offset == 7u) {
        return SUMNLS_DIRECTIVE_STYLE_POP;
    }
    if (offset == 8u) {
        return SUMNLS_DIRECTIVE_FRACTION_START;
    }
    if (offset == 9u) {
        return SUMNLS_DIRECTIVE_FRACTION_MID;
    }
    if (offset == 10u) {
        return SUMNLS_DIRECTIVE_FRACTION_END;
    }
    if (offset == 11u) {
        return SUMNLS_DIRECTIVE_RADICAL_START;
    }
    if (offset == 12u) {
        return SUMNLS_DIRECTIVE_RADICAL_END;
    }
    if (offset == 13u) {
        return SUMNLS_DIRECTIVE_LIMITS_OVER;
    }
    if (offset == 14u) {
        return SUMNLS_DIRECTIVE_LIMITS_UNDER;
    }
    return SUMNLS_DIRECTIVE_GROUP_END;
}

const char *sutr_sumnls_directive_name(sumnls_directive_t directive)
{
    switch (directive) {
        case SUMNLS_DIRECTIVE_ISOLATE_START: return "SCP_MATH_ISOLATE_START";
        case SUMNLS_DIRECTIVE_ISOLATE_END:   return "SCP_MATH_ISOLATE_END";
        case SUMNLS_DIRECTIVE_MODE_LTR:      return "SCP_MATH_MODE_LTR";
        case SUMNLS_DIRECTIVE_MODE_RTL:      return "SCP_MATH_MODE_RTL";
        case SUMNLS_DIRECTIVE_ALIGN_TAB:     return "SCP_MATH_ALIGN_TAB";
        case SUMNLS_DIRECTIVE_ALIGN_ROW:     return "SCP_MATH_ALIGN_ROW";
        case SUMNLS_DIRECTIVE_STYLE_PUSH:    return "SCP_MATH_STYLE_PUSH";
        case SUMNLS_DIRECTIVE_STYLE_POP:     return "SCP_MATH_STYLE_POP";
        case SUMNLS_DIRECTIVE_FRACTION_START:return "SCP_MATH_FRACTION_START";
        case SUMNLS_DIRECTIVE_FRACTION_MID:  return "SCP_MATH_FRACTION_MID";
        case SUMNLS_DIRECTIVE_FRACTION_END:  return "SCP_MATH_FRACTION_END";
        case SUMNLS_DIRECTIVE_RADICAL_START: return "SCP_MATH_RADICAL_START";
        case SUMNLS_DIRECTIVE_RADICAL_END:   return "SCP_MATH_RADICAL_END";
        case SUMNLS_DIRECTIVE_LIMITS_OVER:   return "SCP_MATH_LIMITS_OVER";
        case SUMNLS_DIRECTIVE_LIMITS_UNDER:  return "SCP_MATH_LIMITS_UNDER";
        case SUMNLS_DIRECTIVE_GROUP_END:     return "SCP_MATH_GROUP_END";
        case SUMNLS_DIRECTIVE_NONE:
        default:                             return "unknown";
    }
}

/* ── The classification of §4.3 ──────────────────────────────────── */

/*
 * The classification reads the packed SUCPM mask: one shift-and-compare
 * for `Math`, one more for the four-bit `MathClass` field. SUMNLS owns
 * no character table of its own, so a native math plugin and the Bridge
 * repertoire behave identically without a table walk.
 */
bool sutr_sumnls_is_math(uint64_t codepoint)
{
    return sutr_sucpm_is_math(sutr_sucpm_props(codepoint));
}

sucpm_math_class_t sutr_sumnls_math_class(uint64_t codepoint)
{
    sucpm_flags64_t mask = sutr_sucpm_props(codepoint);

    if (!sutr_sucpm_is_math(mask)) {
        return SUCPM_MC_N;
    }
    return sutr_sucpm_math_class(mask);
}

/*
 * The projection of the ten UTR #25 `MathClass` values onto the six atom
 * classes the §5.8 spacing table is written over.
 *
 * The projection of `U` onto Ord is the interesting one: §5.8 itself
 * reclassifies a binary operator acting as a prefix "as ordinary
 * (unary)", so a unary operator and an ordinary atom share a column.
 * `F`, `I` and `S` have no spacing behaviour distinct from an ordinary
 * atom and share the same column.
 */
sumnls_atom_class_t sutr_sumnls_project(sucpm_math_class_t math_class)
{
    switch (math_class) {
        case SUCPM_MC_B: return SUMNLS_ATOM_BIN;
        case SUCPM_MC_R: return SUMNLS_ATOM_REL;
        case SUCPM_MC_O: return SUMNLS_ATOM_OPEN;
        case SUCPM_MC_C: return SUMNLS_ATOM_CLOSE;
        case SUCPM_MC_P: return SUMNLS_ATOM_PUNCT;
        case SUCPM_MC_N:
        case SUCPM_MC_U:
        case SUCPM_MC_F:
        case SUCPM_MC_I:
        case SUCPM_MC_S:
        default:         return SUMNLS_ATOM_ORD;
    }
}

sumnls_atom_class_t sutr_sumnls_atom_class_of(uint64_t codepoint)
{
    return sutr_sumnls_project(sutr_sumnls_math_class(codepoint));
}

/* ── The spacing table of §5.8 ───────────────────────────────────── */

/*
 * The table is indexed [left][right] over the atom classes. Column zero
 * is the "no previous atom" column and is empty everywhere: the first
 * atom of a run carries no space.
 */
static const uint8_t g_sumnls_spacing[SUMNLS_ATOM_COUNT][SUMNLS_ATOM_COUNT] = {
    /*             none  Ord   Bin   Rel  Open Close Punct */
    /*  no prev */ { 0u,   0u,   0u,   0u,   0u,   0u,   0u },
    /*  Ord     */ { 0u,   0u,   2u,   3u,   0u,   0u,   0u },
    /*  Bin     */ { 0u,   2u,   0u,   0u,   0u,   0u,   0u },
    /*  Rel     */ { 0u,   3u,   2u,   0u,   0u,   0u,   0u },
    /*  Open    */ { 0u,   0u,   0u,   0u,   0u,   0u,   0u },
    /*  Close   */ { 0u,   0u,   2u,   3u,   0u,   0u,   0u },
    /*  Punct   */ { 0u,   1u,   0u,   0u,   0u,   0u,   0u }
};

sumnls_space_t sutr_sumnls_spacing(sumnls_atom_class_t left,
                                    sumnls_atom_class_t right)
{
    if (left >= SUMNLS_ATOM_COUNT || right >= SUMNLS_ATOM_COUNT) {
        return SUMNLS_SPACE_NONE;
    }
    return (sumnls_space_t)g_sumnls_spacing[left][right];
}

const char *sutr_sumnls_space_name(sumnls_space_t space)
{
    switch (space) {
        case SUMNLS_SPACE_THIN:   return "thin";
        case SUMNLS_SPACE_MEDIUM: return "medium";
        case SUMNLS_SPACE_THICK:  return "thick";
        case SUMNLS_SPACE_NONE:
        default:                  return "none";
    }
}

/* ── The reclassification rule of §5.8 ───────────────────────────── */

/*
 * A binary operator that opens a run, closes a run, follows an opening
 * fence, or follows another binary operator is reclassified as ordinary
 * *before* the spacing is applied. The rule is what makes `-x` set
 * solid and `a + -b` space correctly.
 */
sumnls_atom_class_t sutr_sumnls_reclassify(sumnls_atom_class_t provisional,
                                            sumnls_atom_class_t previous,
                                            bool is_first, bool is_last)
{
    if (provisional != SUMNLS_ATOM_BIN) {
        return provisional;
    }
    if (is_first || is_last) {
        return SUMNLS_ATOM_ORD;
    }
    if (previous == SUMNLS_ATOM_OPEN || previous == SUMNLS_ATOM_BIN) {
        return SUMNLS_ATOM_ORD;
    }
    return provisional;
}

/* ── The extensible delimiter strategies of §5.5 ─────────────────── */

bool sutr_sumnls_stretch_select(const sumnls_stretch_parts_t *parts,
                                uint32_t target_height,
                                sumnls_stretch_result_t *out)
{
    uint32_t fixed;
    uint32_t room;
    uint32_t step;
    uint32_t count;

    if (parts == NULL || out == NULL) {
        return false;
    }
    if (target_height == 0u) {
        out->strategy     = SUMNLS_STRETCH_SINGLE;
        out->extender_count = 0u;
        out->total_height = 0u;
        return true;
    }
    if (target_height <= parts->max_single_height) {
        out->strategy      = SUMNLS_STRETCH_SINGLE;
        out->extender_count = 0u;
        out->total_height  = target_height;
        return true;
    }
    if (parts->extender_height == 0u) {
        /* The font offers no extender: the construct clamps to the
         * tallest single glyph (§9 recommendation 7). */
        out->strategy      = SUMNLS_STRETCH_SINGLE;
        out->extender_count = 0u;
        out->total_height  = parts->max_single_height;
        return true;
    }
    fixed = parts->top_height + parts->bottom_height;
    room  = (target_height > fixed) ? (target_height - fixed) : 0u;
    step  = parts->extender_height;
    if (step > parts->overlap) {
        step -= parts->overlap;
    }
    if (step == 0u) {
        step = 1u;
    }
    count = room / step;
    out->strategy      = SUMNLS_STRETCH_ASSEMBLED;
    out->extender_count = count;
    out->total_height  = fixed + (count * step);
    return true;
}

/* ── The style ladder of §5.6 ────────────────────────────────────── */

static sumnls_style_t sumnls_style_next(sumnls_style_t style)
{
    switch (style) {
        case SUMNLS_STYLE_DISPLAY: return SUMNLS_STYLE_TEXT;
        case SUMNLS_STYLE_TEXT:    return SUMNLS_STYLE_SCRIPT;
        case SUMNLS_STYLE_SCRIPT:  return SUMNLS_STYLE_SCRIPT_SCRIPT;
        case SUMNLS_STYLE_SCRIPT_SCRIPT:
        default:                    return SUMNLS_STYLE_SCRIPT_SCRIPT;
    }
}

sumnls_style_t sutr_sumnls_script_style(sumnls_style_t style)
{
    switch (style) {
        case SUMNLS_STYLE_DISPLAY:
        case SUMNLS_STYLE_TEXT:    return SUMNLS_STYLE_SCRIPT;
        case SUMNLS_STYLE_SCRIPT:  return SUMNLS_STYLE_SCRIPT_SCRIPT;
        case SUMNLS_STYLE_SCRIPT_SCRIPT:
        default:                    return SUMNLS_STYLE_SCRIPT_SCRIPT;
    }
}

const char *sutr_sumnls_style_name(sumnls_style_t style)
{
    switch (style) {
        case SUMNLS_STYLE_DISPLAY:       return "display";
        case SUMNLS_STYLE_TEXT:          return "text";
        case SUMNLS_STYLE_SCRIPT:        return "script";
        case SUMNLS_STYLE_SCRIPT_SCRIPT: return "script-script";
        default:                         return "unknown";
    }
}

/* ── The box helpers of §5.1 ─────────────────────────────────────── */

static uint32_t sumnls_new_box(sumnls_context_t *ctx, uint8_t kind)
{
    uint32_t index;
    sumnls_box_t *box;

    if (ctx->box_count >= ctx->box_capacity) {
        return SUMNLS_BOX_NONE;
    }
    index = ctx->box_count;
    ctx->box_count++;
    box = &ctx->boxes[index];
    box->kind           = kind;
    box->style          = ctx->style;
    box->width          = 0u;
    box->ascent         = 0;
    box->descent        = 0;
    box->baseline_shift = 0;
    box->gap_before     = 0;
    box->axis_height    = 0;
    box->first_child    = SUMNLS_BOX_NONE;
    box->last_child     = SUMNLS_BOX_NONE;
    box->next_sibling   = SUMNLS_BOX_NONE;
    return index;
}

static void sumnls_attach_child(sumnls_context_t *ctx, uint32_t parent,
                                uint32_t child)
{
    sumnls_box_t *box = &ctx->boxes[parent];

    if (child == SUMNLS_BOX_NONE) {
        return;
    }
    if (box->first_child == SUMNLS_BOX_NONE) {
        box->first_child = child;
    } else {
        ctx->boxes[box->last_child].next_sibling = child;
    }
    box->last_child = child;
}

/*
 * The metrics of a box are recomputed from its children when the box is
 * closed, so they are always consistent with the tree. A horizontal box
 * packs its children left to right and takes the sum of their widths; a
 * vertical box stacks them and takes the widest. Both take the ascent
 * and descent from the children's baseline shifts, which is what lets a
 * raised script or a degree sit inside a horizontal box.
 */
static void sumnls_finalize_box(sumnls_context_t *ctx, uint32_t index)
{
    sumnls_box_t *box = &ctx->boxes[index];
    uint32_t child;
    int32_t ascent = 0;
    int32_t descent = 0;
    uint32_t width = 0u;

    if (box->first_child == SUMNLS_BOX_NONE) {
        return;
    }
    for (child = box->first_child; child != SUMNLS_BOX_NONE;
         child = ctx->boxes[child].next_sibling) {
        const sumnls_box_t *c = &ctx->boxes[child];
        int32_t top = c->baseline_shift + c->ascent;
        int32_t bottom = c->descent - c->baseline_shift;

        if (top > ascent) {
            ascent = top;
        }
        if (bottom > descent) {
            descent = bottom;
        }
        if (box->kind == SUMNLS_BOX_HORIZONTAL) {
            uint32_t advance = c->width;
            if (c->gap_before > 0) {
                advance += (uint32_t)c->gap_before;
            }
            width += advance;
        } else if (c->width > width) {
            width = c->width;
        }
    }
    box->width   = width;
    box->ascent  = ascent;
    box->descent = descent;
}

/* ── The alignment bookkeeping of §6.4 ───────────────────────────── */

static void sumnls_align_reset(sumnls_context_t *ctx)
{
    uint32_t i;

    for (i = 0u; i < SUMNLS_ALIGN_MAX_COLUMNS; i++) {
        ctx->align_column_width[i] = 0u;
    }
    for (i = 0u; i < SUMNLS_ALIGN_MAX_CELLS; i++) {
        ctx->align_cell[i] = SUMNLS_BOX_NONE;
    }
    for (i = 0u; i < SUMNLS_ALIGN_MAX_ROWS; i++) {
        ctx->align_row_first[i] = SUMNLS_BOX_NONE;
        ctx->align_row_cells[i] = 0u;
    }
    ctx->align_row_count    = 0u;
    ctx->align_cell_count   = 0u;
    ctx->align_column_count = 0u;
}

static bool sumnls_align_record_cell(sumnls_context_t *ctx, uint32_t cell)
{
    if (cell == SUMNLS_BOX_NONE) {
        return false;
    }
    if (ctx->align_cell_count >= SUMNLS_ALIGN_MAX_CELLS) {
        return false;
    }
    ctx->align_cell[ctx->align_cell_count] = cell;
    ctx->align_cell_count++;
    return true;
}

static bool sumnls_align_record_row(sumnls_context_t *ctx, uint32_t row,
                                    uint32_t cells)
{
    if (row == SUMNLS_BOX_NONE) {
        return false;
    }
    if (ctx->align_row_count >= SUMNLS_ALIGN_MAX_ROWS) {
        return false;
    }
    ctx->align_row_first[ctx->align_row_count] = row;
    ctx->align_row_cells[ctx->align_row_count] = cells;
    ctx->align_row_count++;
    return true;
}

/*
 * The tab stop positions of §6.4: every cell in a column takes the
 * width of the widest cell in that column. The padding is applied to
 * the cell box itself, so the row widths follow from the cells.
 */
static void sumnls_align_measure(sumnls_context_t *ctx)
{
    uint32_t row;
    uint32_t cell;
    uint32_t column;
    uint32_t i;
    uint32_t next;

    for (i = 0u; i < SUMNLS_ALIGN_MAX_COLUMNS; i++) {
        ctx->align_column_width[i] = 0u;
    }
    next = 0u;
    for (row = 0u; row < ctx->align_row_count; row++) {
        uint32_t cells = ctx->align_row_cells[row];

        for (i = 0u; i < cells && next < ctx->align_cell_count; i++) {
            cell = ctx->align_cell[next];
            next++;
            if (cell == SUMNLS_BOX_NONE) {
                continue;
            }
            column = i;
            if (column >= SUMNLS_ALIGN_MAX_COLUMNS) {
                column = SUMNLS_ALIGN_MAX_COLUMNS - 1u;
            }
            if (ctx->boxes[cell].width > ctx->align_column_width[column]) {
                ctx->align_column_width[column] = ctx->boxes[cell].width;
            }
        }
    }
    next = 0u;
    for (row = 0u; row < ctx->align_row_count; row++) {
        uint32_t cells = ctx->align_row_cells[row];

        for (i = 0u; i < cells && next < ctx->align_cell_count; i++) {
            cell = ctx->align_cell[next];
            next++;
            if (cell == SUMNLS_BOX_NONE) {
                continue;
            }
            column = i;
            if (column >= SUMNLS_ALIGN_MAX_COLUMNS) {
                column = SUMNLS_ALIGN_MAX_COLUMNS - 1u;
            }
            ctx->boxes[cell].width = ctx->align_column_width[column];
        }
    }
    /* The rows are re-laid on the shared columns, so their widths are
     * recomputed after the padding. */
    for (row = 0u; row < ctx->align_row_count; row++) {
        sumnls_finalize_box(ctx, ctx->align_row_first[row]);
    }
    if (ctx->align_column_count < SUMNLS_ALIGN_MAX_COLUMNS) {
        uint32_t widest = 0u;

        for (i = 0u; i < ctx->align_cell_count; i++) {
            cell = ctx->align_cell[i];
            if (cell != SUMNLS_BOX_NONE &&
                ctx->boxes[cell].width > widest) {
                widest = ctx->boxes[cell].width;
            }
        }
        ctx->align_column_count = 0u;
        for (i = 0u; i < SUMNLS_ALIGN_MAX_COLUMNS; i++) {
            if (ctx->align_column_width[i] > 0u) {
                ctx->align_column_count = i + 1u;
            }
        }
        if (ctx->align_column_count == 0u && widest > 0u) {
            ctx->align_column_count = 1u;
        }
    }
}

/* ── The deferred spacing of §5.8 ────────────────────────────────── */

static int32_t sumnls_space_width(const sumnls_context_t *ctx,
                                   sumnls_space_t space)
{
    switch (space) {
        case SUMNLS_SPACE_THIN:   return ctx->constants.thin_space;
        case SUMNLS_SPACE_MEDIUM: return ctx->constants.medium_space;
        case SUMNLS_SPACE_THICK:  return ctx->constants.thick_space;
        case SUMNLS_SPACE_NONE:
        default:                  return 0;
    }
}

/*
 * The one-atom deferral. The pending atom is committed when the next
 * atom arrives — at which point the pending atom is known not to close
 * its run, so its class is final and the gap can be applied — and at the
 * end of a run, where it is committed as the last atom.
 */
static void sumnls_commit_pending(sumnls_context_t *ctx,
                                   sumnls_frame_t *frame, bool is_last)
{
    sumnls_atom_class_t final_class;
    sumnls_space_t space;
    int32_t gap;

    if (frame->have_pending == 0u) {
        return;
    }
    final_class = sutr_sumnls_reclassify(
        (sumnls_atom_class_t)frame->pending_class,
        (sumnls_atom_class_t)frame->prev_class,
        (frame->have_prev == 0u), is_last);
    space = sutr_sumnls_spacing((sumnls_atom_class_t)frame->prev_class,
                                final_class);
    gap = sumnls_space_width(ctx, space);

    /* A sub and a super on the same base keep a minimum gap (§5.6). */
    if (frame->pending_is_script != 0u && frame->last_was_script != 0u &&
        frame->pending_script_is_sup != frame->last_script_is_sup) {
        if (gap < ctx->constants.script_gap) {
            gap = ctx->constants.script_gap;
        }
    }
    if (frame->have_prev != 0u) {
        ctx->boxes[frame->pending_box].gap_before = gap;
    }
    sumnls_attach_child(ctx, frame->target_box, frame->pending_box);

    frame->prev_class         = (uint8_t)final_class;
    frame->have_prev          = 1u;
    frame->last_was_script    = frame->pending_is_script;
    frame->last_script_is_sup = frame->pending_script_is_sup;
    frame->have_pending       = 0u;
    frame->pending_is_script  = 0u;
}

/* ── The frame helpers ───────────────────────────────────────────── */

static bool sumnls_open_frame(sumnls_context_t *ctx, uint8_t kind)
{
    sumnls_frame_t *frame;
    uint32_t box;

    if (ctx->depth >= SUMNLS_MAX_DEPTH) {
        return false;
    }
    box = sumnls_new_box(ctx, SUMNLS_BOX_HORIZONTAL);
    if (box == SUMNLS_BOX_NONE) {
        return false;
    }
    frame = &ctx->frames[ctx->depth];
    sumnls_zero(frame, sizeof(*frame));
    frame->kind              = kind;
    frame->run_box           = box;
    frame->target_box        = box;
    frame->pending_box       = SUMNLS_BOX_NONE;
    frame->aux_box           = SUMNLS_BOX_NONE;
    frame->aux_box2          = SUMNLS_BOX_NONE;
    frame->align_row         = SUMNLS_BOX_NONE;
    frame->align_cell        = SUMNLS_BOX_NONE;
    frame->style_at_open     = ctx->style;
    frame->content_style     = ctx->style;
    frame->direction_at_open = ctx->direction;
    frame->style_depth_at_open = ctx->style_depth;
    ctx->depth++;
    return true;
}

static void sumnls_run_reset(sumnls_frame_t *frame)
{
    frame->prev_class      = (uint8_t)SUMNLS_ATOM_NONE;
    frame->have_prev       = 0u;
    frame->have_pending    = 0u;
    frame->pending_class   = (uint8_t)SUMNLS_ATOM_NONE;
    frame->pending_box     = SUMNLS_BOX_NONE;
    frame->pending_is_script = 0u;
    frame->last_was_script = 0u;
}

/* ── The builders of §5.3, §5.4, §5.6 and §5.7 ──────────────────── */

/*
 * The fraction of §5.3. The rule is centred on the axis height and the
 * numerator and the denominator are shifted clear of it by their gaps.
 */
static uint32_t sumnls_build_fraction(sumnls_context_t *ctx,
                                      sumnls_frame_t *frame)
{
    uint32_t numerator   = frame->aux_box;
    uint32_t denominator = frame->run_box;
    uint32_t fraction;
    uint32_t rule;
    int32_t axis = ctx->constants.axis_height;
    int32_t rt   = ctx->constants.rule_thickness;
    int32_t half = rt / 2;

    if (numerator == SUMNLS_BOX_NONE) {
        numerator = denominator;
    }
    if (denominator == SUMNLS_BOX_NONE) {
        denominator = numerator;
    }

    rule = sumnls_new_box(ctx, SUMNLS_BOX_HORIZONTAL);
    if (rule == SUMNLS_BOX_NONE) {
        return denominator;
    }
    ctx->boxes[rule].width = ctx->boxes[numerator].width;
    if (ctx->boxes[denominator].width > ctx->boxes[rule].width) {
        ctx->boxes[rule].width = ctx->boxes[denominator].width;
    }
    ctx->boxes[rule].ascent         = rt;
    ctx->boxes[rule].descent        = 0;
    ctx->boxes[rule].baseline_shift = axis - half;

    ctx->boxes[numerator].baseline_shift =
        axis + half + ctx->constants.numerator_gap;
    ctx->boxes[denominator].baseline_shift =
        axis - half - ctx->constants.denominator_gap;

    fraction = sumnls_new_box(ctx, SUMNLS_BOX_VERTICAL);
    if (fraction == SUMNLS_BOX_NONE) {
        return denominator;
    }
    ctx->boxes[fraction].axis_height = axis;
    sumnls_attach_child(ctx, fraction, numerator);
    sumnls_attach_child(ctx, fraction, rule);
    sumnls_attach_child(ctx, fraction, denominator);
    sumnls_finalize_box(ctx, fraction);
    return fraction;
}

/*
 * The radical of §5.4. The first atom of the group is the radical sign
 * and the rest is the radicand; an optional degree sign immediately
 * after the sign becomes the degree at the upper left. The sign is
 * stretched to cover the radicand and its clearance, and the radicand
 * is lowered by the rule thickness to leave room for the vinculum.
 */
static uint32_t sumnls_build_radical(sumnls_context_t *ctx,
                                     sumnls_frame_t *frame)
{
    uint32_t sign     = frame->aux_box;
    uint32_t degree   = frame->aux_box2;
    uint32_t radicand = frame->run_box;
    uint32_t wrap;
    uint32_t radical;
    int32_t rt = ctx->constants.rule_thickness;

    if (sign == SUMNLS_BOX_NONE) {
        sign = sumnls_new_box(ctx, SUMNLS_BOX_HORIZONTAL);
    }
    if (radicand == SUMNLS_BOX_NONE) {
        radicand = sumnls_new_box(ctx, SUMNLS_BOX_HORIZONTAL);
    }

    wrap = sumnls_new_box(ctx, SUMNLS_BOX_VERTICAL);
    if (wrap == SUMNLS_BOX_NONE) {
        return radicand;
    }
    ctx->boxes[radicand].baseline_shift = -rt;
    sumnls_attach_child(ctx, wrap, radicand);
    sumnls_finalize_box(ctx, wrap);

    /* The sign covers the radicand plus the clearance. */
    sumnls_finalize_box(ctx, sign);
    ctx->boxes[sign].ascent = ctx->boxes[wrap].ascent +
                              ctx->constants.radical_clearance;
    ctx->boxes[sign].descent = ctx->boxes[wrap].descent;

    radical = sumnls_new_box(ctx, SUMNLS_BOX_HORIZONTAL);
    if (radical == SUMNLS_BOX_NONE) {
        return wrap;
    }
    sumnls_attach_child(ctx, radical, sign);
    if (degree != SUMNLS_BOX_NONE) {
        ctx->boxes[degree].baseline_shift =
            ctx->boxes[wrap].ascent + ctx->constants.degree_shift;
        sumnls_attach_child(ctx, radical, degree);
    }
    sumnls_attach_child(ctx, radical, wrap);
    sumnls_finalize_box(ctx, radical);
    return radical;
}

/*
 * The script tier of §5.6. The content box was laid out at the script
 * style; the script box raises it by the superscript shift or drops it
 * by the subscript shift.
 */
static uint32_t sumnls_build_script(sumnls_context_t *ctx,
                                    sumnls_frame_t *frame, bool is_sup)
{
    uint32_t content = frame->run_box;
    uint32_t script;

    if (content == SUMNLS_BOX_NONE) {
        content = sumnls_new_box(ctx, SUMNLS_BOX_HORIZONTAL);
    }
    script = sumnls_new_box(ctx, SUMNLS_BOX_HORIZONTAL);
    if (script == SUMNLS_BOX_NONE) {
        return content;
    }
    sumnls_attach_child(ctx, script, content);
    ctx->boxes[script].style = frame->content_style;
    ctx->boxes[script].baseline_shift =
        is_sup ? ctx->constants.superscript_shift
               : (-ctx->constants.subscript_shift);
    sumnls_finalize_box(ctx, script);
    return script;
}

/*
 * The limits of §5.7. The operator is the pending atom of the run the
 * limits belong to; the construct stacks the limits above or below it.
 */
static uint32_t sumnls_build_limits(sumnls_context_t *ctx,
                                    sumnls_frame_t *frame, bool over)
{
    uint32_t limits = frame->run_box;
    uint32_t op     = frame->aux_box;
    uint32_t stack;
    int32_t gap = ctx->constants.script_gap;

    if (limits == SUMNLS_BOX_NONE) {
        limits = sumnls_new_box(ctx, SUMNLS_BOX_HORIZONTAL);
    }
    if (op == SUMNLS_BOX_NONE) {
        op = sumnls_new_box(ctx, SUMNLS_BOX_HORIZONTAL);
    }
    stack = sumnls_new_box(ctx, SUMNLS_BOX_VERTICAL);
    if (stack == SUMNLS_BOX_NONE) {
        return limits;
    }
    ctx->boxes[op].baseline_shift = 0;
    if (over) {
        ctx->boxes[limits].baseline_shift =
            ctx->boxes[op].ascent + gap;
        sumnls_attach_child(ctx, stack, limits);
        sumnls_attach_child(ctx, stack, op);
    } else {
        ctx->boxes[limits].baseline_shift =
            -(ctx->boxes[op].descent + gap);
        sumnls_attach_child(ctx, stack, op);
        sumnls_attach_child(ctx, stack, limits);
    }
    sumnls_finalize_box(ctx, stack);
    return stack;
}

/* ── The parent hand-off ─────────────────────────────────────────── */

/*
 * A finished group is an ordinary atom in its parent run. The parent's
 * pending atom is committed first, because the group follows it.
 */
static void sumnls_parent_receive(sumnls_context_t *ctx, uint32_t box)
{
    sumnls_frame_t *parent;

    if (ctx->depth == 0u) {
        return;
    }
    parent = &ctx->frames[ctx->depth - 1u];
    sumnls_commit_pending(ctx, parent, false);
    parent->pending_class    = (uint8_t)SUMNLS_ATOM_ORD;
    parent->pending_box      = box;
    parent->have_pending     = 1u;
    parent->pending_is_script = 0u;
}

static void sumnls_close_frame(sumnls_context_t *ctx)
{
    sumnls_frame_t *frame;
    uint32_t result;

    if (ctx->depth == 0u) {
        return;
    }
    frame = &ctx->frames[ctx->depth - 1u];

    /* The last atom of the run is committed as last. */
    sumnls_commit_pending(ctx, frame, true);
    if (frame->align_mode != 0u) {
        sumnls_finalize_box(ctx, frame->align_cell);
        sumnls_align_record_cell(ctx, frame->align_cell);
        frame->align_cells_in_row++;
        sumnls_finalize_box(ctx, frame->align_row);
        sumnls_align_record_row(ctx, frame->align_row,
                                frame->align_cells_in_row);
        sumnls_align_measure(ctx);
    }
    sumnls_finalize_box(ctx, frame->run_box);

    /* The style and the direction of the enclosing scope are restored
     * before the result is built, so the result boxes carry the style
     * of the scope the group belongs to. */
    ctx->style        = frame->style_at_open;
    ctx->direction    = frame->direction_at_open;
    ctx->style_depth  = frame->style_depth_at_open;
    ctx->depth--;

    switch (frame->kind) {
        case SUMNLS_GROUP_FRACTION:     result = sumnls_build_fraction(ctx, frame); break;
        case SUMNLS_GROUP_RADICAL:      result = sumnls_build_radical(ctx, frame);  break;
        case SUMNLS_GROUP_SCRIPT_SUP:   result = sumnls_build_script(ctx, frame, true);  break;
        case SUMNLS_GROUP_SCRIPT_SUB:   result = sumnls_build_script(ctx, frame, false); break;
        case SUMNLS_GROUP_LIMITS_OVER:  result = sumnls_build_limits(ctx, frame, true);  break;
        case SUMNLS_GROUP_LIMITS_UNDER: result = sumnls_build_limits(ctx, frame, false); break;
        default:                        result = frame->run_box;                   break;
    }
    sumnls_parent_receive(ctx, result);
}

/* ── The alignment transitions of §6.4 ────────────────────────────── */

static void sumnls_align_enter(sumnls_context_t *ctx,
                               sumnls_frame_t *frame)
{
    uint32_t row;
    uint32_t cell;
    uint32_t first;
    uint32_t last;

    /* The frame's run box becomes the vertical alignment container; the
     * atoms already gathered become the first cell of the first row. */
    ctx->boxes[frame->run_box].kind = SUMNLS_BOX_VERTICAL;

    first = ctx->boxes[frame->run_box].first_child;
    last  = ctx->boxes[frame->run_box].last_child;

    row = sumnls_new_box(ctx, SUMNLS_BOX_HORIZONTAL);
    if (row == SUMNLS_BOX_NONE) {
        return;
    }
    sumnls_attach_child(ctx, frame->run_box, row);

    cell = sumnls_new_box(ctx, SUMNLS_BOX_HORIZONTAL);
    if (cell == SUMNLS_BOX_NONE) {
        return;
    }
    sumnls_attach_child(ctx, row, cell);

    if (first != SUMNLS_BOX_NONE) {
        /* The atoms that were already gathered move into the first cell;
         * the row takes their place among the run box's children. */
        ctx->boxes[last].next_sibling = SUMNLS_BOX_NONE;
        ctx->boxes[cell].first_child  = first;
        ctx->boxes[cell].last_child   = last;
        ctx->boxes[frame->run_box].first_child = row;
        ctx->boxes[frame->run_box].last_child  = row;
    }
    frame->align_row    = row;
    frame->align_cell   = cell;
    frame->align_mode   = 1u;
    frame->align_column = 0u;
    frame->target_box   = cell;

    /* The pending atom joins the first cell. */
    sumnls_commit_pending(ctx, frame, true);
}

static void sumnls_align_tab(sumnls_context_t *ctx,
                             sumnls_frame_t *frame)
{
    uint32_t cell;

    if (frame->align_mode == 0u) {
        sumnls_align_enter(ctx, frame);
        return;
    }
    sumnls_commit_pending(ctx, frame, true);
    sumnls_finalize_box(ctx, frame->align_cell);
    sumnls_align_record_cell(ctx, frame->align_cell);
    frame->align_cells_in_row++;

    cell = sumnls_new_box(ctx, SUMNLS_BOX_HORIZONTAL);
    if (cell == SUMNLS_BOX_NONE) {
        return;
    }
    sumnls_attach_child(ctx, frame->align_row, cell);
    frame->align_cell = cell;
    frame->target_box  = cell;
    frame->align_column++;
    sumnls_run_reset(frame);
}

static void sumnls_align_row(sumnls_context_t *ctx,
                             sumnls_frame_t *frame)
{
    uint32_t row;
    uint32_t cell;

    if (frame->align_mode == 0u) {
        sumnls_align_enter(ctx, frame);
        return;
    }
    sumnls_commit_pending(ctx, frame, true);
    sumnls_finalize_box(ctx, frame->align_cell);
    sumnls_align_record_cell(ctx, frame->align_cell);
    frame->align_cells_in_row++;
    sumnls_finalize_box(ctx, frame->align_row);
    sumnls_align_record_row(ctx, frame->align_row, frame->align_cells_in_row);
    frame->align_cells_in_row = 0u;

    row = sumnls_new_box(ctx, SUMNLS_BOX_HORIZONTAL);
    if (row == SUMNLS_BOX_NONE) {
        return;
    }
    sumnls_attach_child(ctx, frame->run_box, row);
    cell = sumnls_new_box(ctx, SUMNLS_BOX_HORIZONTAL);
    if (cell == SUMNLS_BOX_NONE) {
        return;
    }
    sumnls_attach_child(ctx, row, cell);
    frame->align_row    = row;
    frame->align_cell   = cell;
    frame->align_column = 0u;
    frame->target_box   = cell;
    sumnls_run_reset(frame);
}

/* ── The atom path ───────────────────────────────────────────────── */

static void sumnls_get_metrics(sumnls_context_t *ctx, uint64_t codepoint,
                                sumnls_glyph_metrics_t *out)
{
    if (ctx->metrics != NULL) {
        if (ctx->metrics(ctx->metrics_user, codepoint, out) == true) {
            return;
        }
    }
    out->advance = (uint32_t)(ctx->constants.units_per_em / 2);
    out->ascent  = (int32_t)((uint32_t)ctx->constants.units_per_em * 3u / 4u);
    out->descent = (int32_t)(ctx->constants.units_per_em / 4u);
}

static uint32_t sumnls_glyph_box(sumnls_context_t *ctx, uint64_t codepoint)
{
    sumnls_glyph_metrics_t metrics;
    uint32_t box;

    sumnls_get_metrics(ctx, codepoint, &metrics);
    box = sumnls_new_box(ctx, SUMNLS_BOX_HORIZONTAL);
    if (box == SUMNLS_BOX_NONE) {
        return SUMNLS_BOX_NONE;
    }
    ctx->boxes[box].width   = metrics.advance;
    ctx->boxes[box].ascent  = metrics.ascent;
    ctx->boxes[box].descent = metrics.descent;
    return box;
}

/*
 * An ordinary atom. The glyph box is created at once, but the atom is
 * not committed until the next atom arrives, because the §5.8
 * reclassification needs to know whether this atom closes its run.
 */
static bool sumnls_add_atom(sumnls_context_t *ctx, uint64_t codepoint)
{
    sumnls_frame_t *frame = &ctx->frames[ctx->depth - 1u];
    sumnls_atom_class_t atom_class;
    uint32_t box;

    /* The radical frame routes its first atoms to the sign and to the
     * degree; the rest is the radicand. */
    if (frame->kind == SUMNLS_GROUP_RADICAL) {
        if (frame->sign_seen == 0u) {
            box = sumnls_glyph_box(ctx, codepoint);
            if (box == SUMNLS_BOX_NONE) {
                return false;
            }
            sumnls_attach_child(ctx, frame->aux_box, box);
            frame->sign_seen = 1u;
            return true;
        }
        if (frame->degree_seen == 0u && codepoint == 0x000000B0u) {
            uint32_t degree = sumnls_new_box(ctx, SUMNLS_BOX_HORIZONTAL);

            if (degree == SUMNLS_BOX_NONE) {
                return false;
            }
            box = sumnls_glyph_box(ctx, codepoint);
            if (box == SUMNLS_BOX_NONE) {
                return false;
            }
            sumnls_attach_child(ctx, degree, box);
            frame->aux_box2   = degree;
            frame->degree_seen = 1u;
            return true;
        }
    }

    box = sumnls_glyph_box(ctx, codepoint);
    if (box == SUMNLS_BOX_NONE) {
        return false;
    }
    atom_class = sutr_sumnls_atom_class_of(codepoint);

    /* The pending atom is followed by this one, so it is not last. */
    sumnls_commit_pending(ctx, frame, false);

    frame->pending_class    = (uint8_t)atom_class;
    frame->pending_box      = box;
    frame->have_pending     = 1u;
    frame->pending_is_script = 0u;
    return true;
}

/* ── The directive path of §6.1 ──────────────────────────────────── */

static void sumnls_push_style(sumnls_context_t *ctx)
{
    if (ctx->style_depth < SUMNLS_STYLE_STACK_DEPTH) {
        ctx->style_stack[ctx->style_depth] = ctx->style;
        ctx->style_depth++;
    }
    ctx->style = (uint8_t)sumnls_style_next((sumnls_style_t)ctx->style);
}

static void sumnls_pop_style(sumnls_context_t *ctx)
{
    if (ctx->style_depth == 0u) {
        return;
    }
    ctx->style_depth--;
    ctx->style = ctx->style_stack[ctx->style_depth];
}

static void sumnls_push_direction(sumnls_context_t *ctx, uint8_t direction)
{
    if (ctx->direction_depth < SUMNLS_DIRECTION_STACK_DEPTH) {
        ctx->direction_stack[ctx->direction_depth] = ctx->direction;
        ctx->direction_depth++;
    }
    ctx->direction = direction;
}

static bool sumnls_apply_directive(sumnls_context_t *ctx,
                                   sumnls_directive_t directive)
{
    sumnls_frame_t *frame = &ctx->frames[ctx->depth - 1u];

    switch (directive) {
        case SUMNLS_DIRECTIVE_ISOLATE_START:
            return sumnls_open_frame(ctx, SUMNLS_GROUP_ISOLATE);

        case SUMNLS_DIRECTIVE_ISOLATE_END:
        case SUMNLS_DIRECTIVE_GROUP_END:
            sumnls_close_frame(ctx);
            return true;

        case SUMNLS_DIRECTIVE_MODE_LTR:
            sumnls_push_direction(ctx, (uint8_t)SUMNLS_DIR_LTR);
            return true;

        case SUMNLS_DIRECTIVE_MODE_RTL:
            sumnls_push_direction(ctx, (uint8_t)SUMNLS_DIR_RTL);
            return true;

        case SUMNLS_DIRECTIVE_ALIGN_TAB:
            sumnls_align_tab(ctx, frame);
            return true;

        case SUMNLS_DIRECTIVE_ALIGN_ROW:
            sumnls_align_row(ctx, frame);
            return true;

        case SUMNLS_DIRECTIVE_STYLE_PUSH:
            sumnls_push_style(ctx);
            return true;

        case SUMNLS_DIRECTIVE_STYLE_POP:
            sumnls_pop_style(ctx);
            return true;

        case SUMNLS_DIRECTIVE_FRACTION_START:
            return sumnls_open_frame(ctx, SUMNLS_GROUP_FRACTION);

        case SUMNLS_DIRECTIVE_FRACTION_MID:
            if (frame->kind != SUMNLS_GROUP_FRACTION) {
                return true;
            }
            /* The numerator is closed and the denominator begins. */
            sumnls_commit_pending(ctx, frame, true);
            sumnls_finalize_box(ctx, frame->run_box);
            frame->aux_box = frame->run_box;
            frame->run_box = sumnls_new_box(ctx, SUMNLS_BOX_HORIZONTAL);
            if (frame->run_box == SUMNLS_BOX_NONE) {
                return false;
            }
            frame->target_box = frame->run_box;
            frame->phase = 1u;
            sumnls_run_reset(frame);
            return true;

        case SUMNLS_DIRECTIVE_FRACTION_END:
            if (frame->kind == SUMNLS_GROUP_FRACTION) {
                sumnls_close_frame(ctx);
            } else {
                sumnls_close_frame(ctx);
            }
            return true;

        case SUMNLS_DIRECTIVE_RADICAL_START: {
            uint32_t sign;
            uint32_t radicand;

            sign = sumnls_new_box(ctx, SUMNLS_BOX_HORIZONTAL);
            if (sign == SUMNLS_BOX_NONE) {
                return false;
            }
            if (!sumnls_open_frame(ctx, SUMNLS_GROUP_RADICAL)) {
                return false;
            }
            frame = &ctx->frames[ctx->depth - 1u];
            frame->aux_box  = sign;
            frame->aux_box2 = SUMNLS_BOX_NONE;
            radicand = frame->run_box;
            if (radicand == SUMNLS_BOX_NONE) {
                return false;
            }
            return true;
        }

        case SUMNLS_DIRECTIVE_RADICAL_END:
            sumnls_close_frame(ctx);
            return true;

        case SUMNLS_DIRECTIVE_LIMITS_OVER:
        case SUMNLS_DIRECTIVE_LIMITS_UNDER: {
            uint32_t op = SUMNLS_BOX_NONE;

            /* The operator is the pending atom of the enclosing run. */
            if (frame->have_pending != 0u) {
                op = frame->pending_box;
                frame->have_pending = 0u;
                frame->pending_box   = SUMNLS_BOX_NONE;
            }
            if (!sumnls_open_frame(ctx,
                    (directive == SUMNLS_DIRECTIVE_LIMITS_OVER)
                        ? SUMNLS_GROUP_LIMITS_OVER
                        : SUMNLS_GROUP_LIMITS_UNDER)) {
                return false;
            }
            frame = &ctx->frames[ctx->depth - 1u];
            frame->aux_box = op;
            return true;
        }

        case SUMNLS_DIRECTIVE_NONE:
        default:
            return true;
    }
}

/* ── The engine loop of §5.2 ─────────────────────────────────────── */

typedef struct {
    const uint64_t         *codepoints;
    const sumnls_control_t *controls;
    const sumnls_token_t   *tokens;
    uint32_t                count;
    uint32_t                index;
} sumnls_cursor_t;

static bool sumnls_cursor_next(sumnls_cursor_t *cursor, uint64_t *out_codepoint,
                                sumnls_control_t *out_control)
{
    sumnls_control_t control = SUMNLS_CTRL_NONE;
    uint64_t codepoint;

    if (cursor->index >= cursor->count) {
        return false;
    }
    if (cursor->tokens != NULL) {
        codepoint = cursor->tokens[cursor->index].codepoint;
        control   = cursor->tokens[cursor->index].control;
    } else {
        codepoint = cursor->codepoints[cursor->index];
        if (cursor->controls != NULL) {
            control = cursor->controls[cursor->index];
        }
    }
    cursor->index++;
    *out_codepoint = codepoint;
    *out_control   = control;
    return true;
}

static bool sumnls_engine_run(sumnls_context_t *ctx,
                              sumnls_cursor_t *cursor,
                              sumnls_result_t *out)
{
    uint64_t codepoint;
    sumnls_control_t control;
    sumnls_directive_t directive;
    sumnls_frame_t *frame;
    uint32_t root;

    ctx->box_count     = 0u;
    ctx->depth         = 0u;
    ctx->style         = (uint8_t)SUMNLS_STYLE_TEXT;
    ctx->style_depth   = 0u;
    ctx->direction     = (uint8_t)SUMNLS_DIR_LTR;
    ctx->direction_depth = 0u;
    sumnls_align_reset(ctx);

    if (!sumnls_open_frame(ctx, SUMNLS_GROUP_HORIZONTAL)) {
        return false;
    }
    root = ctx->frames[0].run_box;

    while (sumnls_cursor_next(cursor, &codepoint, &control)) {
        if (control == SUMNLS_CTRL_SUPERSCRIPT ||
            control == SUMNLS_CTRL_SUBSCRIPT) {
            /* The script tier of §5.6: the content is laid out one rung
             * down the style ladder and the enclosing style is restored
             * when the tier closes. */
            if (!sumnls_open_frame(ctx,
                    (control == SUMNLS_CTRL_SUPERSCRIPT)
                        ? SUMNLS_GROUP_SCRIPT_SUP
                        : SUMNLS_GROUP_SCRIPT_SUB)) {
                return false;
            }
            frame = &ctx->frames[ctx->depth - 1u];
            ctx->style = (uint8_t)sutr_sumnls_script_style(
                (sumnls_style_t)frame->style_at_open);
            frame->content_style = ctx->style;
            ctx->boxes[frame->run_box].style = ctx->style;
            continue;
        }
        directive = sutr_sumnls_directive_of(codepoint);
        if (directive != SUMNLS_DIRECTIVE_NONE) {
            if (!sumnls_apply_directive(ctx, directive)) {
                return false;
            }
            continue;
        }
        if (!sumnls_add_atom(ctx, codepoint)) {
            return false;
        }
    }

    /* An unclosed group is closed at the end of the input (§6.6). */
    while (ctx->depth > 0u) {
        sumnls_close_frame(ctx);
    }

    if (out != NULL) {
        out->root_box  = root;
        out->box_count = ctx->box_count;
        out->style     = ctx->style;
        out->direction = ctx->direction;
        out->ok        = true;
    }
    return true;
}

/* ── The public engine API ───────────────────────────────────────── */

bool sutr_sumnls_context_init(sumnls_context_t *ctx, sumnls_box_t *boxes,
                              uint32_t box_capacity)
{
    if (ctx == NULL || boxes == NULL || box_capacity == 0u) {
        return false;
    }
    sumnls_zero(ctx, sizeof(*ctx));
    ctx->boxes        = boxes;
    ctx->box_capacity = box_capacity;
    ctx->box_count    = 0u;
    ctx->metrics      = NULL;
    ctx->metrics_user = NULL;
    sutr_sumnls_default_constants(1000, &ctx->constants);
    ctx->depth          = 0u;
    ctx->style          = (uint8_t)SUMNLS_STYLE_TEXT;
    ctx->style_depth    = 0u;
    ctx->direction      = (uint8_t)SUMNLS_DIR_LTR;
    ctx->direction_depth = 0u;
    sumnls_align_reset(ctx);
    return true;
}

bool sutr_sumnls_layout(sumnls_context_t *ctx, const uint64_t *codepoints,
                        uint32_t count, sumnls_result_t *out)
{
    sumnls_cursor_t cursor;

    if (ctx == NULL || ctx->boxes == NULL || ctx->box_capacity == 0u) {
        return false;
    }
    if (codepoints == NULL && count > 0u) {
        return false;
    }
    cursor.codepoints = codepoints;
    cursor.controls   = NULL;
    cursor.tokens     = NULL;
    cursor.count      = count;
    cursor.index      = 0u;
    return sumnls_engine_run(ctx, &cursor, out);
}

bool sutr_sumnls_layout_tokens(sumnls_context_t *ctx,
                               const sumnls_token_t *tokens,
                               uint32_t count, sumnls_result_t *out)
{
    sumnls_cursor_t cursor;

    if (ctx == NULL || ctx->boxes == NULL || ctx->box_capacity == 0u) {
        return false;
    }
    if (tokens == NULL && count > 0u) {
        return false;
    }
    cursor.codepoints = NULL;
    cursor.controls   = NULL;
    cursor.tokens     = tokens;
    cursor.count      = count;
    cursor.index      = 0u;
    return sumnls_engine_run(ctx, &cursor, out);
}

uint32_t sutr_sumnls_box_count(const sumnls_context_t *ctx)
{
    if (ctx == NULL) {
        return 0u;
    }
    return ctx->box_count;
}

bool sutr_sumnls_box_at(const sumnls_context_t *ctx, uint32_t index,
                        sumnls_box_t *out)
{
    if (ctx == NULL || out == NULL || index >= ctx->box_count) {
        return false;
    }
    *out = ctx->boxes[index];
    return true;
}
