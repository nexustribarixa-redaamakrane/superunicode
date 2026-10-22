#ifndef SUTR_SUTR_SUMNLS_H
#define SUTR_SUTR_SUMNLS_H

/* SUTR-9 — SuperUnicode Mathematical Notation and Layout Support (SUMNLS)
 *
 * A modular technical-report specification within the OpenWindows microkernel
 * ecosystem. SUMNLS is the SuperUnicode analog of UTR #25 *Unicode Support for
 * Mathematics* together with the OpenType MATH layout model: it fixes the
 * mathematical repertoire, the property vocabulary that classifies it, the
 * box-model layout engine that gives it geometry, and the in-band System
 * Control Plane directives that carry the structure characters cannot.
 *
 * The model is architectural, so this module makes the architecture
 * *checkable* rather than merely describing it. Every load-bearing claim of
 * SUTR-9 becomes an O(1), allocation-free operation:
 *
 *   1. THE PACKED CLASSIFICATION (§4.3). SUMNLS owns no character table.
 *      Every atom is classified by reading the packed SUCPM mask — one
 *      shift-and-compare for `Math`, one more for the four-bit `MathClass`
 *      field — so a native math plugin and the Bridge repertoire behave
 *      identically without a single table walk.
 *   2. THE SIX ATOM CLASSES AND THE SPACING TABLE (§5.8). The ten UTR #25
 *      `MathClass` values project onto the six atom classes the spacing
 *      table is written over, and the table is a single constant matrix.
 *   3. THE NORMATIVE RECLASSIFICATION (§5.8). A binary operator that opens a
 *      run, closes a run, follows an opening fence, or follows another
 *      binary operator is reclassified as ordinary *before* the spacing is
 *      applied. The rule is what makes `-x` and `a + -b` lay out correctly.
 *   4. THE SINGLE-PASS DEFERRAL. The reclassification needs to know whether
 *      an atom closes its run, which is a one-atom look-ahead. The engine
 *      holds exactly one pending atom, so the pass stays single and linear
 *      with no re-scan and no heap.
 *   5. THE BOX MODEL (§5.1). Boxes live in a caller-owned flat array; the
 *      engine never allocates. The array is walked as a tree through
 *      first-child / next-sibling links, so the traversal is O(1) per box.
 *   6. THE DIRECTIVE BLOCK (§6.1). `0x00110030–0x0011003F` is decoded in
 *      O(1) from the codepoint. The block is a closed enumeration, so the
 *      script tier — which the block does not name — is entered through the
 *      engine's own token stream rather than by inventing a codepoint.
 *
 * The engine is freestanding C99: no libc, no heap, no recursion over
 * unbounded input, and a compile-time bound on every table and stack.
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "sutr/sutr_sucpm.h"

/* ── Identifiers ──────────────────────────────────────────────────── */

#define SUMNLS_SPEC_ID            "SUTR-9"
#define SUMNLS_SPEC_TITLE         "SuperUnicode Mathematical Notation and Layout Support"
#define SUMNLS_VERSION_MAJOR      0
#define SUMNLS_VERSION_MINOR      1
#define SUMNLS_VERSION_PATCH      0
#define SUMNLS_MODULE             "sutr_sumnls"

/* ── Bounded capacities (§5.1, §6.4, §8) ──────────────────────────── */

#define SUMNLS_MAX_DEPTH            16u  /* group nesting (§5.1)      */
#define SUMNLS_ALIGN_MAX_COLUMNS     8u  /* alignment columns (§6.4)  */
#define SUMNLS_ALIGN_MAX_ROWS       16u  /* alignment rows (§6.4)     */
#define SUMNLS_ALIGN_MAX_CELLS      64u  /* alignment cells (§6.4)    */
#define SUMNLS_STYLE_STACK_DEPTH    16u  /* style stack (§5.6)        */
#define SUMNLS_DIRECTION_STACK_DEPTH 16u /* direction stack (§6.3)   */

/* ── The System Control Plane math directive block (§6.1) ─────────── */

#define SUMNLS_MATH_DIRECTIVE_MIN 0x00110030ULL
#define SUMNLS_MATH_DIRECTIVE_MAX 0x0011003FULL

/* ── The atom classes of §5.8 ─────────────────────────────────────── */

/*
 * The spacing table of §5.8 is written over six classes. The ten UTR #25
 * `MathClass` values of §4.2 project onto them:
 *
 *   N -> Ord      B -> Bin      R -> Rel      O -> Open
 *   C -> Close    P -> Punct
 *   U -> Ord      F -> Ord      I -> Ord      S -> Ord
 *
 * The four classes with no column of their own are projected onto Ord.
 * `U` is projected onto Ord because §5.8 itself reclassifies a binary
 * operator acting as a prefix "as ordinary (unary)"; `F`, `I` and `S` have
 * no spacing behaviour distinct from an ordinary atom.
 */
typedef enum {
    SUMNLS_ATOM_NONE = 0,  /* no atom yet                              */
    SUMNLS_ATOM_ORD,       /* Ord   — N, U, F, I, S                     */
    SUMNLS_ATOM_BIN,       /* Bin   — B                                */
    SUMNLS_ATOM_REL,       /* Rel   — R                                */
    SUMNLS_ATOM_OPEN,      /* Open  — O                                */
    SUMNLS_ATOM_CLOSE,     /* Close — C                                */
    SUMNLS_ATOM_PUNCT,     /* Punct — P                                */
    SUMNLS_ATOM_COUNT
} sumnls_atom_class_t;

/* ── The inter-atom spacing of §5.8 ───────────────────────────────── */

typedef enum {
    SUMNLS_SPACE_NONE = 0,
    SUMNLS_SPACE_THIN,
    SUMNLS_SPACE_MEDIUM,
    SUMNLS_SPACE_THICK,
    SUMNLS_SPACE_COUNT
} sumnls_space_t;

/* ── The math style ladder of §5.6 ────────────────────────────────── */

typedef enum {
    SUMNLS_STYLE_DISPLAY = 0,
    SUMNLS_STYLE_TEXT,
    SUMNLS_STYLE_SCRIPT,
    SUMNLS_STYLE_SCRIPT_SCRIPT,
    SUMNLS_STYLE_COUNT
} sumnls_style_t;

/* ── The math direction of §6.3 ───────────────────────────────────── */

typedef enum {
    SUMNLS_DIR_LTR = 0,
    SUMNLS_DIR_RTL
} sumnls_direction_t;

/* ── The box model of §5.1 ────────────────────────────────────────── */

typedef enum {
    SUMNLS_BOX_HORIZONTAL = 0, /* packs children left to right           */
    SUMNLS_BOX_VERTICAL         /* stacks children around a central axis  */
} sumnls_box_kind_t;

/* The sentinel for an absent box link. */
#define SUMNLS_BOX_NONE 0xFFFFFFFFu

/*
 * A box. The engine writes boxes into a caller-owned flat array; the array
 * is a storage pool, not a tree. The tree is recovered by walking
 * `first_child` and `next_sibling`, so a renderer may traverse it in O(1)
 * per box without any pointer chasing through a heap.
 *
 * `width`, `ascent` and `descent` are recomputed from the children
 * whenever a box is closed, so they are always consistent with the tree.
 */
typedef struct {
    uint8_t  kind;            /* sumnls_box_kind_t                        */
    uint8_t  style;           /* the math style of the box (§5.6)         */
    uint32_t width;           /* the advance width                         */
    int32_t  ascent;          /* the height above the baseline             */
    int32_t  descent;         /* the depth below the baseline              */
    int32_t  baseline_shift;  /* the baseline offset inside the parent     */
    int32_t  gap_before;      /* the space inserted before the box (§5.8)  */
    int32_t  axis_height;     /* a vertical box's axis above its baseline */
    uint32_t first_child;     /* the first child, or SUMNLS_BOX_NONE       */
    uint32_t last_child;      /* the last child, or SUMNLS_BOX_NONE        */
    uint32_t next_sibling;    /* the next sibling, or SUMNLS_BOX_NONE      */
} sumnls_box_t;

/* ── The glyph metrics of §4.4 ────────────────────────────────────── */

typedef struct {
    uint32_t advance; /* the horizontal advance                          */
    int32_t  ascent;  /* the height above the baseline                   */
    int32_t  descent; /* the depth below the baseline                    */
} sumnls_glyph_metrics_t;

/*
 * The SUF metric source of §5.9. The callback returns false when it has no
 * glyph for the codepoint, in which case the engine falls back to the
 * default metrics of the context.
 */
typedef bool (*sumnls_metrics_fn)(void *user, uint64_t codepoint,
                                  sumnls_glyph_metrics_t *out);

/* ── The math constants of §4.4 ────────────────────────────────────── */

/*
 * The vertical math constants the engine consumes. They live in the font,
 * not in the engine (§5.9, §9 recommendation 5), so the caller supplies
 * them. `sutr_sumnls_default_constants()` fills a reference set scaled to a
 * given units-per-em.
 */
typedef struct {
    int32_t units_per_em;
    int32_t axis_height;        /* the axis above the baseline             */
    int32_t rule_thickness;     /* the fraction bar thickness              */
    int32_t numerator_gap;      /* the gap above the rule                 */
    int32_t denominator_gap;    /* the gap below the rule                 */
    int32_t superscript_shift;  /* the raise of a superscript              */
    int32_t subscript_shift;    /* the drop of a subscript                */
    int32_t script_gap;         /* the minimum gap between a sub and super */
    int32_t degree_shift;       /* the raise of a radical degree          */
    int32_t radical_clearance;  /* the clearance above a radicand         */
    int32_t thin_space;         /* the width of a thin space              */
    int32_t medium_space;       /* the width of a medium space            */
    int32_t thick_space;        /* the width of a thick space             */
} sumnls_constants_t;

/* ── The extensible delimiter strategies of §5.5 ──────────────────── */

typedef enum {
    SUMNLS_STRETCH_SINGLE = 0,  /* one glyph of the needed size             */
    SUMNLS_STRETCH_ASSEMBLED,   /* top, repeated extender, bottom          */
    SUMNLS_STRETCH_DRAWN        /* a rule-based stroke from the outline     */
} sumnls_stretch_strategy_t;

/* The stretch data of §4.4, as the font supplies it. */
typedef struct {
    uint32_t max_single_height; /* the tallest single glyph available      */
    uint32_t top_height;        /* the height of the top part               */
    uint32_t bottom_height;     /* the height of the bottom part            */
    uint32_t extender_height;   /* the height of one extender part          */
    uint32_t overlap;           /* the allowed overlap between parts       */
} sumnls_stretch_parts_t;

typedef struct {
    sumnls_stretch_strategy_t strategy;
    uint32_t extender_count;    /* the number of extenders to repeat       */
    uint32_t total_height;      /* the height the construct covers         */
} sumnls_stretch_result_t;

/* ── The directive enumeration of §6.1 ────────────────────────────── */

typedef enum {
    SUMNLS_DIRECTIVE_NONE = 0,
    SUMNLS_DIRECTIVE_ISOLATE_START,
    SUMNLS_DIRECTIVE_ISOLATE_END,
    SUMNLS_DIRECTIVE_MODE_LTR,
    SUMNLS_DIRECTIVE_MODE_RTL,
    SUMNLS_DIRECTIVE_ALIGN_TAB,
    SUMNLS_DIRECTIVE_ALIGN_ROW,
    SUMNLS_DIRECTIVE_STYLE_PUSH,
    SUMNLS_DIRECTIVE_STYLE_POP,
    SUMNLS_DIRECTIVE_FRACTION_START,
    SUMNLS_DIRECTIVE_FRACTION_MID,
    SUMNLS_DIRECTIVE_FRACTION_END,
    SUMNLS_DIRECTIVE_RADICAL_START,
    SUMNLS_DIRECTIVE_RADICAL_END,
    SUMNLS_DIRECTIVE_LIMITS_OVER,
    SUMNLS_DIRECTIVE_LIMITS_UNDER,
    SUMNLS_DIRECTIVE_GROUP_END,
    SUMNLS_DIRECTIVE_COUNT
} sumnls_directive_t;

/* ── The engine's own script markers ──────────────────────────────── */

/*
 * The §6.1 directive block is a closed enumeration (§10) and names no
 * script-tier directive, so the script tier is entered through the engine's
 * token stream rather than through a codepoint the standard does not
 * define. A front end that has already lowered a formula emits the markers;
 * the marker is not a character and adds nothing to the repertoire.
 */
typedef enum {
    SUMNLS_CTRL_NONE = 0,
    SUMNLS_CTRL_SUPERSCRIPT,
    SUMNLS_CTRL_SUBSCRIPT
} sumnls_control_t;

/*
 * One input token: a codepoint and, optionally, an engine control marker.
 * A token with SUMNLS_CTRL_NONE is an ordinary atom or an SCP directive.
 */
typedef struct {
    uint64_t         codepoint;
    sumnls_control_t control;
} sumnls_token_t;

/* ── The group kinds of the layout engine ─────────────────────────── */

typedef enum {
    SUMNLS_GROUP_HORIZONTAL = 0, /* a plain run                             */
    SUMNLS_GROUP_FRACTION,       /* a stacked fraction (§5.3)               */
    SUMNLS_GROUP_RADICAL,        /* a radical (§5.4)                        */
    SUMNLS_GROUP_SCRIPT_SUP,     /* a superscript tier (§5.6)               */
    SUMNLS_GROUP_SCRIPT_SUB,     /* a subscript tier (§5.6)                 */
    SUMNLS_GROUP_LIMITS_OVER,    /* limits above an operator (§5.7)         */
    SUMNLS_GROUP_LIMITS_UNDER,   /* limits below an operator (§5.7)         */
    SUMNLS_GROUP_ISOLATE,        /* an inline isolation scope (§6.2)        */
    SUMNLS_GROUP_COUNT
} sumnls_group_kind_t;

/* ── The engine frame ─────────────────────────────────────────────── */

/*
 * One open group. The frame carries the deferred-spacing state of its
 * current run (§5.8), the boxes the group needs to build its result, and
 * the style and direction the group has to restore when it closes.
 */
typedef struct {
    uint8_t  kind;              /* sumnls_group_kind_t                     */
    uint8_t  phase;             /* the fraction phase: 0 numerator, 1 den. */
    uint8_t  prev_class;        /* the final class of the last atom        */
    uint8_t  have_prev;         /* whether there is a previous atom        */
    uint8_t  pending_class;     /* the provisional class of the pending atom*/
    uint8_t  have_pending;      /* whether an atom is pending              */
    uint8_t  last_was_script;   /* whether the last atom was a script       */
    uint8_t  last_script_is_sup;/* the kind of that script                  */
    uint8_t  pending_is_script; /* whether the pending atom is a script     */
    uint8_t  pending_script_is_sup;
    uint8_t  sign_seen;         /* a radical has taken its sign             */
    uint8_t  degree_seen;       /* a radical has taken its degree           */
    uint32_t pending_box;       /* the box of the pending atom              */
    uint32_t run_box;           /* the horizontal box of the current run    */
    uint32_t target_box;        /* the box the pending atom is attached to  */
    uint32_t aux_box;           /* the numerator / sign / operator box     */
    uint32_t aux_box2;          /* the second auxiliary box                 */
    uint32_t align_row;         /* the current alignment row box            */
    uint32_t align_cell;        /* the current alignment cell box           */
    uint32_t align_column;      /* the current alignment column index       */
    uint32_t align_cells_in_row;/* the number of cells closed in this row   */
    uint8_t  align_mode;        /* whether the run is an alignment run      */
    uint8_t  style_at_open;     /* the style to restore on close           */
    uint8_t  content_style;     /* the style the content was laid out at   */
    uint8_t  direction_at_open; /* the direction to restore on close        */
    uint32_t style_depth_at_open;
} sumnls_frame_t;

/* ── The engine context ───────────────────────────────────────────── */

/*
 * The caller owns the context and the box array. The engine writes into
 * them and never allocates.
 */
typedef struct {
    sumnls_box_t    *boxes;
    uint32_t         box_capacity;
    uint32_t         box_count;

    sumnls_metrics_fn metrics;
    void             *metrics_user;
    sumnls_constants_t constants;

    sumnls_frame_t   frames[SUMNLS_MAX_DEPTH];
    uint32_t         depth;

    uint8_t  style;
    uint8_t  style_stack[SUMNLS_STYLE_STACK_DEPTH];
    uint32_t style_depth;

    uint8_t  direction;
    uint8_t  direction_stack[SUMNLS_DIRECTION_STACK_DEPTH];
    uint32_t direction_depth;

    /* The alignment measure of §6.4. */
    uint32_t align_column_width[SUMNLS_ALIGN_MAX_COLUMNS];
    uint32_t align_cell[SUMNLS_ALIGN_MAX_CELLS];
    uint32_t align_row_first[SUMNLS_ALIGN_MAX_ROWS];
    uint32_t align_row_cells[SUMNLS_ALIGN_MAX_ROWS];
    uint32_t align_row_count;
    uint32_t align_cell_count;
    uint32_t align_column_count;
} sumnls_context_t;

/* ── The layout result ────────────────────────────────────────────── */

typedef struct {
    uint32_t root_box;  /* the index of the root box                     */
    uint32_t box_count; /* the number of boxes the layout produced      */
    uint8_t  style;     /* the style in force at the end of the input   */
    uint8_t  direction; /* the direction in force at the end of the input*/
    bool     ok;        /* false when the engine refused the input      */
} sumnls_result_t;

/* ── The engine API ───────────────────────────────────────────────── */

/**
 * The "MAJOR.MINOR.PATCH" version string of this module. The returned
 * pointer is to a static string, never NULL.
 */
const char *sutr_sumnls_version_string(void);

/**
 * A reference set of math constants, scaled to `units_per_em`. The values
 * are informative; a caller that has a font supplies the font's own.
 */
void sutr_sumnls_default_constants(int32_t units_per_em,
                                   sumnls_constants_t *out);

/**
 * The default glyph metrics: a fixed half-em advance on a four-quarter
 * em ascent and a quarter-em descent. Used when the context has no metric
 * callback, so that the engine is exercisable without a font.
 */
void sutr_sumnls_default_metrics(uint64_t codepoint,
                                 sumnls_glyph_metrics_t *out);

/**
 * The directive a codepoint names, or SUMNLS_DIRECTIVE_NONE when the
 * codepoint is not in the §6.1 block. The decode is O(1): one range test
 * and one subtraction.
 */
sumnls_directive_t sutr_sumnls_directive_of(uint64_t codepoint);

/**
 * The name of a directive, in the spelling of §6.1. Never NULL.
 */
const char *sutr_sumnls_directive_name(sumnls_directive_t directive);

/**
 * Whether a codepoint carries the `Math` property, read from the packed
 * SUCPM mask (§4.3).
 */
bool sutr_sumnls_is_math(uint64_t codepoint);

/**
 * The UTR #25 `MathClass` of a codepoint, read from the packed SUCPM mask.
 * A codepoint that is not mathematical reports SUCPM_MC_N.
 */
sucpm_math_class_t sutr_sumnls_math_class(uint64_t codepoint);

/**
 * The projection of a `MathClass` value onto the six atom classes of §5.8.
 */
sumnls_atom_class_t sutr_sumnls_project(sucpm_math_class_t math_class);

/**
 * The atom class of a codepoint: the projection of its `MathClass`. A
 * codepoint that is not mathematical is an ordinary atom.
 */
sumnls_atom_class_t sutr_sumnls_atom_class_of(uint64_t codepoint);

/**
 * The space the §5.8 table inserts between two adjacent atoms.
 */
sumnls_space_t sutr_sumnls_spacing(sumnls_atom_class_t left,
                                    sumnls_atom_class_t right);

/**
 * The name of a spacing value: "none", "thin", "medium" or "thick".
 */
const char *sutr_sumnls_space_name(sumnls_space_t space);

/**
 * The §5.8 reclassification. A binary operator that opens a run, closes a
 * run, follows an opening fence, or follows another binary operator is
 * reclassified as ordinary. Every other class passes through unchanged.
 *
 * `previous` is the final class of the atom before the one being
 * classified, or SUMNLS_ATOM_NONE when there is none.
 */
sumnls_atom_class_t sutr_sumnls_reclassify(sumnls_atom_class_t provisional,
                                            sumnls_atom_class_t previous,
                                            bool is_first, bool is_last);

/**
 * The §5.5 strategy selection. Chooses between a single glyph, an assembled
 * construct, and a drawn stroke, and reports how many extender parts the
 * assembly needs. The result is always bounded: a request beyond the font's
 * maximum clamps to the largest construct (§9 recommendation 7).
 */
bool sutr_sumnls_stretch_select(const sumnls_stretch_parts_t *parts,
                                uint32_t target_height,
                                sumnls_stretch_result_t *out);

/**
 * Prepares a context for a layout run. `boxes` is the caller-owned box
 * array; `box_capacity` is its length in boxes.
 */
bool sutr_sumnls_context_init(sumnls_context_t *ctx, sumnls_box_t *boxes,
                              uint32_t box_capacity);

/**
 * Lays out a plain codepoint sequence. The §6.1 directives inside the
 * sequence are decoded and applied; every other codepoint is an atom.
 */
bool sutr_sumnls_layout(sumnls_context_t *ctx, const uint64_t *codepoints,
                        uint32_t count, sumnls_result_t *out);

/**
 * Lays out a token sequence: the same codepoints, plus the engine's own
 * script markers (§5.6).
 */
bool sutr_sumnls_layout_tokens(sumnls_context_t *ctx,
                               const sumnls_token_t *tokens,
                               uint32_t count, sumnls_result_t *out);

/**
 * The number of boxes the last layout produced.
 */
uint32_t sutr_sumnls_box_count(const sumnls_context_t *ctx);

/**
 * A copy of the box at `index`, or false when the index is out of range.
 */
bool sutr_sumnls_box_at(const sumnls_context_t *ctx, uint32_t index,
                        sumnls_box_t *out);

/**
 * The style the engine pushes to when a script tier is entered: the next
 * rung of the §5.6 ladder below `style`.
 */
sumnls_style_t sutr_sumnls_script_style(sumnls_style_t style);

/**
 * The name of a style: "display", "text", "script" or "script-script".
 */
const char *sutr_sumnls_style_name(sumnls_style_t style);

#endif /* SUTR_SUTR_SUMNLS_H */
