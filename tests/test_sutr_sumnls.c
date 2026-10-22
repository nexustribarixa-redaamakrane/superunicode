/*
 * SUTR-9 SUMNLS - SuperUnicode Mathematical Notation and Layout Support
 *
 * Unit tests for the layout engine of SUTR-9: the classification of
 * §4.3, the atom-class projection and the spacing table of §5.8, the
 * normative reclassification of §5.8, the box model of §5.1, the single
 * pass of §5.2, the fraction of §5.3, the radical of §5.4, the
 * extensible-delimiter strategies of §5.5, the script tiers of §5.6,
 * the limits of §5.7, the System Control Plane math directives of §6.1,
 * the alignment tab stops of §6.4, and the conformance rules of §6.6.
 */

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>

#include "sutr/sutr_sumnls.h"

static int g_fail = 0;
#define CHECK(cond) do { if (!(cond)) { \
    printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); g_fail = 1; \
} } while (0)

/* The box pool every test lays out into. */
#define TEST_BOXES 512u
static sumnls_box_t g_boxes[TEST_BOXES];
static sumnls_context_t g_ctx;

/* The §6.1 directive block, spelled out. */
#define D_ISOLATE_START 0x00110030ULL
#define D_ISOLATE_END   0x00110031ULL
#define D_MODE_LTR      0x00110032ULL
#define D_MODE_RTL      0x00110033ULL
#define D_ALIGN_TAB     0x00110034ULL
#define D_ALIGN_ROW     0x00110035ULL
#define D_STYLE_PUSH    0x00110036ULL
#define D_STYLE_POP     0x00110037ULL
#define D_FRAC_START    0x00110038ULL
#define D_FRAC_MID      0x00110039ULL
#define D_FRAC_END      0x0011003AULL
#define D_RAD_START     0x0011003BULL
#define D_RAD_END       0x0011003CULL
#define D_LIM_OVER      0x0011003DULL
#define D_LIM_UNDER     0x0011003EULL
#define D_GROUP_END     0x0011003FULL

/* ── Version, limits, and identifiers ─────────────────────────────── */

static void test_version_and_meta(void)
{
    CHECK(sutr_sumnls_version_string() != NULL);
    CHECK(sutr_sumnls_version_string()[0] == '0');
    CHECK(sutr_sumnls_version_string()[2] == '1');

    CHECK(SUMNLS_SPEC_ID[0] == 'S');
    CHECK(SUMNLS_SPEC_TITLE[0] == 'S');
    CHECK(SUMNLS_MODULE[0] == 's');
    CHECK(SUMNLS_VERSION_MAJOR == 0);
    CHECK(SUMNLS_VERSION_MINOR == 1);
    CHECK(SUMNLS_VERSION_PATCH == 0);

    /* The directive block of §6.1. */
    CHECK(SUMNLS_MATH_DIRECTIVE_MIN == 0x00110030ULL);
    CHECK(SUMNLS_MATH_DIRECTIVE_MAX == 0x0011003FULL);

    /* The compile-time bounds of §5.1 and §6.4. */
    CHECK(SUMNLS_MAX_DEPTH >= 8u);
    CHECK(SUMNLS_ALIGN_MAX_COLUMNS >= 2u);
    CHECK(SUMNLS_ALIGN_MAX_ROWS >= 2u);
    CHECK(SUMNLS_ALIGN_MAX_CELLS >= 8u);
}

/* ── The directive decode of §6.1 ────────────────────────────────── */

static void test_directive_decode(void)
{
    CHECK(sutr_sumnls_directive_of(D_ISOLATE_START) == SUMNLS_DIRECTIVE_ISOLATE_START);
    CHECK(sutr_sumnls_directive_of(D_ISOLATE_END)   == SUMNLS_DIRECTIVE_ISOLATE_END);
    CHECK(sutr_sumnls_directive_of(D_MODE_LTR)      == SUMNLS_DIRECTIVE_MODE_LTR);
    CHECK(sutr_sumnls_directive_of(D_MODE_RTL)      == SUMNLS_DIRECTIVE_MODE_RTL);
    CHECK(sutr_sumnls_directive_of(D_ALIGN_TAB)     == SUMNLS_DIRECTIVE_ALIGN_TAB);
    CHECK(sutr_sumnls_directive_of(D_ALIGN_ROW)     == SUMNLS_DIRECTIVE_ALIGN_ROW);
    CHECK(sutr_sumnls_directive_of(D_STYLE_PUSH)    == SUMNLS_DIRECTIVE_STYLE_PUSH);
    CHECK(sutr_sumnls_directive_of(D_STYLE_POP)     == SUMNLS_DIRECTIVE_STYLE_POP);
    CHECK(sutr_sumnls_directive_of(D_FRAC_START)    == SUMNLS_DIRECTIVE_FRACTION_START);
    CHECK(sutr_sumnls_directive_of(D_FRAC_MID)      == SUMNLS_DIRECTIVE_FRACTION_MID);
    CHECK(sutr_sumnls_directive_of(D_FRAC_END)      == SUMNLS_DIRECTIVE_FRACTION_END);
    CHECK(sutr_sumnls_directive_of(D_RAD_START)     == SUMNLS_DIRECTIVE_RADICAL_START);
    CHECK(sutr_sumnls_directive_of(D_RAD_END)       == SUMNLS_DIRECTIVE_RADICAL_END);
    CHECK(sutr_sumnls_directive_of(D_LIM_OVER)      == SUMNLS_DIRECTIVE_LIMITS_OVER);
    CHECK(sutr_sumnls_directive_of(D_LIM_UNDER)     == SUMNLS_DIRECTIVE_LIMITS_UNDER);
    CHECK(sutr_sumnls_directive_of(D_GROUP_END)     == SUMNLS_DIRECTIVE_GROUP_END);

    /* The block is closed: a codepoint outside it is not a directive. */
    CHECK(sutr_sumnls_directive_of(0x0011002FULL) == SUMNLS_DIRECTIVE_NONE);
    CHECK(sutr_sumnls_directive_of(0x00110040ULL) == SUMNLS_DIRECTIVE_NONE);
    CHECK(sutr_sumnls_directive_of(0x00000041ULL) == SUMNLS_DIRECTIVE_NONE);

    /* The names of §6.1. */
    CHECK(sutr_sumnls_directive_name(SUMNLS_DIRECTIVE_ISOLATE_START)[0] == 'S');
    CHECK(sutr_sumnls_directive_name(SUMNLS_DIRECTIVE_FRACTION_START)[0] == 'S');
    CHECK(sutr_sumnls_directive_name(SUMNLS_DIRECTIVE_GROUP_END)[0] == 'S');
    CHECK(sutr_sumnls_directive_name(SUMNLS_DIRECTIVE_NONE)[0] == 'u');
}

/* ── The classification of §4.3 ──────────────────────────────────── */

static void test_classification(void)
{
    /* The mathematical punctuation. */
    CHECK(sutr_sumnls_is_math(0x0000002Bu) == true);   /* + */
    CHECK(sutr_sumnls_math_class(0x0000002Bu) == SUCPM_MC_B);
    CHECK(sutr_sumnls_is_math(0x0000003Du) == true);   /* = */
    CHECK(sutr_sumnls_math_class(0x0000003Du) == SUCPM_MC_R);
    CHECK(sutr_sumnls_math_class(0x00000028u) == SUCPM_MC_O);  /* ( */
    CHECK(sutr_sumnls_math_class(0x00000029u) == SUCPM_MC_C);  /* ) */
    CHECK(sutr_sumnls_math_class(0x0000002Cu) == SUCPM_MC_P);  /* , */
    CHECK(sutr_sumnls_math_class(0x0000007Cu) == SUCPM_MC_F);  /* | */
    CHECK(sutr_sumnls_math_class(0x00002211u) == SUCPM_MC_U);  /* n-ary sum */
    CHECK(sutr_sumnls_math_class(0x0000222Bu) == SUCPM_MC_U);  /* integral */

    /* An ordinary letter is not mathematical. */
    CHECK(sutr_sumnls_is_math(0x00000041u) == false);
    CHECK(sutr_sumnls_math_class(0x00000041u) == SUCPM_MC_N);

    /* The projection of §5.8. */
    CHECK(sutr_sumnls_project(SUCPM_MC_N) == SUMNLS_ATOM_ORD);
    CHECK(sutr_sumnls_project(SUCPM_MC_B) == SUMNLS_ATOM_BIN);
    CHECK(sutr_sumnls_project(SUCPM_MC_R) == SUMNLS_ATOM_REL);
    CHECK(sutr_sumnls_project(SUCPM_MC_O) == SUMNLS_ATOM_OPEN);
    CHECK(sutr_sumnls_project(SUCPM_MC_C) == SUMNLS_ATOM_CLOSE);
    CHECK(sutr_sumnls_project(SUCPM_MC_P) == SUMNLS_ATOM_PUNCT);
    CHECK(sutr_sumnls_project(SUCPM_MC_U) == SUMNLS_ATOM_ORD);
    CHECK(sutr_sumnls_project(SUCPM_MC_F) == SUMNLS_ATOM_ORD);
    CHECK(sutr_sumnls_project(SUCPM_MC_I) == SUMNLS_ATOM_ORD);
    CHECK(sutr_sumnls_project(SUCPM_MC_S) == SUMNLS_ATOM_ORD);

    /* The atom class of a codepoint. */
    CHECK(sutr_sumnls_atom_class_of(0x0000002Bu) == SUMNLS_ATOM_BIN);
    CHECK(sutr_sumnls_atom_class_of(0x0000003Du) == SUMNLS_ATOM_REL);
    CHECK(sutr_sumnls_atom_class_of(0x00000041u) == SUMNLS_ATOM_ORD);
}

/* ── The spacing table of §5.8 ───────────────────────────────────── */

static void test_spacing_table(void)
{
    /* The table of §5.8, cell by cell. */
    CHECK(sutr_sumnls_spacing(SUMNLS_ATOM_ORD, SUMNLS_ATOM_ORD)   == SUMNLS_SPACE_NONE);
    CHECK(sutr_sumnls_spacing(SUMNLS_ATOM_ORD, SUMNLS_ATOM_BIN)   == SUMNLS_SPACE_MEDIUM);
    CHECK(sutr_sumnls_spacing(SUMNLS_ATOM_ORD, SUMNLS_ATOM_REL)   == SUMNLS_SPACE_THICK);
    CHECK(sutr_sumnls_spacing(SUMNLS_ATOM_ORD, SUMNLS_ATOM_OPEN)  == SUMNLS_SPACE_NONE);
    CHECK(sutr_sumnls_spacing(SUMNLS_ATOM_ORD, SUMNLS_ATOM_CLOSE) == SUMNLS_SPACE_NONE);
    CHECK(sutr_sumnls_spacing(SUMNLS_ATOM_ORD, SUMNLS_ATOM_PUNCT) == SUMNLS_SPACE_NONE);

    CHECK(sutr_sumnls_spacing(SUMNLS_ATOM_BIN, SUMNLS_ATOM_ORD)   == SUMNLS_SPACE_MEDIUM);
    CHECK(sutr_sumnls_spacing(SUMNLS_ATOM_BIN, SUMNLS_ATOM_BIN)   == SUMNLS_SPACE_NONE);
    CHECK(sutr_sumnls_spacing(SUMNLS_ATOM_BIN, SUMNLS_ATOM_REL)   == SUMNLS_SPACE_NONE);
    CHECK(sutr_sumnls_spacing(SUMNLS_ATOM_BIN, SUMNLS_ATOM_OPEN)  == SUMNLS_SPACE_NONE);
    CHECK(sutr_sumnls_spacing(SUMNLS_ATOM_BIN, SUMNLS_ATOM_CLOSE) == SUMNLS_SPACE_NONE);
    CHECK(sutr_sumnls_spacing(SUMNLS_ATOM_BIN, SUMNLS_ATOM_PUNCT) == SUMNLS_SPACE_NONE);

    CHECK(sutr_sumnls_spacing(SUMNLS_ATOM_REL, SUMNLS_ATOM_ORD)   == SUMNLS_SPACE_THICK);
    CHECK(sutr_sumnls_spacing(SUMNLS_ATOM_REL, SUMNLS_ATOM_BIN)   == SUMNLS_SPACE_MEDIUM);
    CHECK(sutr_sumnls_spacing(SUMNLS_ATOM_REL, SUMNLS_ATOM_REL)   == SUMNLS_SPACE_NONE);
    CHECK(sutr_sumnls_spacing(SUMNLS_ATOM_REL, SUMNLS_ATOM_OPEN)  == SUMNLS_SPACE_NONE);
    CHECK(sutr_sumnls_spacing(SUMNLS_ATOM_REL, SUMNLS_ATOM_CLOSE) == SUMNLS_SPACE_NONE);
    CHECK(sutr_sumnls_spacing(SUMNLS_ATOM_REL, SUMNLS_ATOM_PUNCT) == SUMNLS_SPACE_NONE);

    CHECK(sutr_sumnls_spacing(SUMNLS_ATOM_OPEN, SUMNLS_ATOM_ORD)   == SUMNLS_SPACE_NONE);
    CHECK(sutr_sumnls_spacing(SUMNLS_ATOM_OPEN, SUMNLS_ATOM_BIN)   == SUMNLS_SPACE_NONE);
    CHECK(sutr_sumnls_spacing(SUMNLS_ATOM_OPEN, SUMNLS_ATOM_REL)   == SUMNLS_SPACE_NONE);
    CHECK(sutr_sumnls_spacing(SUMNLS_ATOM_OPEN, SUMNLS_ATOM_OPEN)  == SUMNLS_SPACE_NONE);
    CHECK(sutr_sumnls_spacing(SUMNLS_ATOM_OPEN, SUMNLS_ATOM_CLOSE) == SUMNLS_SPACE_NONE);
    CHECK(sutr_sumnls_spacing(SUMNLS_ATOM_OPEN, SUMNLS_ATOM_PUNCT) == SUMNLS_SPACE_NONE);

    CHECK(sutr_sumnls_spacing(SUMNLS_ATOM_CLOSE, SUMNLS_ATOM_ORD)   == SUMNLS_SPACE_NONE);
    CHECK(sutr_sumnls_spacing(SUMNLS_ATOM_CLOSE, SUMNLS_ATOM_BIN)   == SUMNLS_SPACE_MEDIUM);
    CHECK(sutr_sumnls_spacing(SUMNLS_ATOM_CLOSE, SUMNLS_ATOM_REL)   == SUMNLS_SPACE_THICK);
    CHECK(sutr_sumnls_spacing(SUMNLS_ATOM_CLOSE, SUMNLS_ATOM_OPEN)  == SUMNLS_SPACE_NONE);
    CHECK(sutr_sumnls_spacing(SUMNLS_ATOM_CLOSE, SUMNLS_ATOM_CLOSE) == SUMNLS_SPACE_NONE);
    CHECK(sutr_sumnls_spacing(SUMNLS_ATOM_CLOSE, SUMNLS_ATOM_PUNCT) == SUMNLS_SPACE_NONE);

    CHECK(sutr_sumnls_spacing(SUMNLS_ATOM_PUNCT, SUMNLS_ATOM_ORD)   == SUMNLS_SPACE_THIN);
    CHECK(sutr_sumnls_spacing(SUMNLS_ATOM_PUNCT, SUMNLS_ATOM_BIN)   == SUMNLS_SPACE_NONE);
    CHECK(sutr_sumnls_spacing(SUMNLS_ATOM_PUNCT, SUMNLS_ATOM_REL)   == SUMNLS_SPACE_NONE);
    CHECK(sutr_sumnls_spacing(SUMNLS_ATOM_PUNCT, SUMNLS_ATOM_OPEN)  == SUMNLS_SPACE_NONE);
    CHECK(sutr_sumnls_spacing(SUMNLS_ATOM_PUNCT, SUMNLS_ATOM_CLOSE) == SUMNLS_SPACE_NONE);
    CHECK(sutr_sumnls_spacing(SUMNLS_ATOM_PUNCT, SUMNLS_ATOM_PUNCT) == SUMNLS_SPACE_NONE);

    /* The names of the spaces. */
    CHECK(sutr_sumnls_space_name(SUMNLS_SPACE_NONE)[0] == 'n');
    CHECK(sutr_sumnls_space_name(SUMNLS_SPACE_THIN)[0] == 't');
    CHECK(sutr_sumnls_space_name(SUMNLS_SPACE_MEDIUM)[0] == 'm');
    CHECK(sutr_sumnls_space_name(SUMNLS_SPACE_THICK)[0] == 't');
}

/* ── The reclassification rule of §5.8 ───────────────────────────── */

static void test_reclassify(void)
{
    /* A binary operator that begins a formula is ordinary. */
    CHECK(sutr_sumnls_reclassify(SUMNLS_ATOM_BIN, SUMNLS_ATOM_NONE,
                                 true, false) == SUMNLS_ATOM_ORD);

    /* A binary operator that ends a formula is ordinary. */
    CHECK(sutr_sumnls_reclassify(SUMNLS_ATOM_BIN, SUMNLS_ATOM_ORD,
                                 false, true) == SUMNLS_ATOM_ORD);

    /* A binary operator that follows an opening fence is ordinary. */
    CHECK(sutr_sumnls_reclassify(SUMNLS_ATOM_BIN, SUMNLS_ATOM_OPEN,
                                 false, false) == SUMNLS_ATOM_ORD);

    /* A binary operator that follows another binary operator is ordinary:
     * this is what makes the `-` of `a + -b` unary. */
    CHECK(sutr_sumnls_reclassify(SUMNLS_ATOM_BIN, SUMNLS_ATOM_BIN,
                                 false, false) == SUMNLS_ATOM_ORD);

    /* A binary operator between two ordinary atoms stays binary. */
    CHECK(sutr_sumnls_reclassify(SUMNLS_ATOM_BIN, SUMNLS_ATOM_ORD,
                                 false, false) == SUMNLS_ATOM_BIN);

    /* A binary operator after a relation stays binary. */
    CHECK(sutr_sumnls_reclassify(SUMNLS_ATOM_BIN, SUMNLS_ATOM_REL,
                                 false, false) == SUMNLS_ATOM_BIN);

    /* Every other class passes through unchanged. */
    CHECK(sutr_sumnls_reclassify(SUMNLS_ATOM_ORD, SUMNLS_ATOM_NONE,
                                 true, true) == SUMNLS_ATOM_ORD);
    CHECK(sutr_sumnls_reclassify(SUMNLS_ATOM_REL, SUMNLS_ATOM_ORD,
                                 false, false) == SUMNLS_ATOM_REL);
    CHECK(sutr_sumnls_reclassify(SUMNLS_ATOM_OPEN, SUMNLS_ATOM_ORD,
                                 false, false) == SUMNLS_ATOM_OPEN);
    CHECK(sutr_sumnls_reclassify(SUMNLS_ATOM_CLOSE, SUMNLS_ATOM_ORD,
                                 false, false) == SUMNLS_ATOM_CLOSE);
    CHECK(sutr_sumnls_reclassify(SUMNLS_ATOM_PUNCT, SUMNLS_ATOM_ORD,
                                 false, false) == SUMNLS_ATOM_PUNCT);
}

/* ── The box model of §5.1 ───────────────────────────────────────── */

/* The number of children of a box, counted through the sibling links. */
static uint32_t child_count(const sumnls_context_t *ctx, uint32_t box)
{
    sumnls_box_t b;
    uint32_t count = 0u;
    uint32_t child;

    if (!sutr_sumnls_box_at(ctx, box, &b)) {
        return 0u;
    }
    child = b.first_child;
    while (child != SUMNLS_BOX_NONE) {
        count++;
        if (!sutr_sumnls_box_at(ctx, child, &b)) {
            break;
        }
        child = b.next_sibling;
    }
    return count;
}
static void test_box_model(void)
{
    sumnls_result_t result;
    sumnls_box_t root;
    sumnls_box_t child;
    uint64_t cps[3];

    CHECK(sutr_sumnls_context_init(&g_ctx, g_boxes, TEST_BOXES) == true);
    cps[0] = 0x00000041u;  /* A */
    cps[1] = 0x00000042u;  /* B */
    cps[2] = 0x00000043u;  /* C */
    CHECK(sutr_sumnls_layout(&g_ctx, cps, 3u, &result) == true);
    CHECK(result.ok == true);
    CHECK(result.box_count == sutr_sumnls_box_count(&g_ctx));

    /* The root is a horizontal box with one child per atom. */
    CHECK(sutr_sumnls_box_at(&g_ctx, result.root_box, &root) == true);
    CHECK(root.kind == SUMNLS_BOX_HORIZONTAL);
    CHECK(child_count(&g_ctx, result.root_box) == 3u);

    /* The children are the glyph boxes, in order, and they are linked by
     * the sibling chain. */
    CHECK(sutr_sumnls_box_at(&g_ctx, root.first_child, &child) == true);
    CHECK(child.kind == SUMNLS_BOX_HORIZONTAL);
    CHECK(child.first_child == SUMNLS_BOX_NONE);
    CHECK(child.next_sibling != SUMNLS_BOX_NONE);
    CHECK(sutr_sumnls_box_at(&g_ctx, child.next_sibling, &child) == true);
    CHECK(child.next_sibling != SUMNLS_BOX_NONE);

    /* The default metrics give each glyph a half-em advance. */
    CHECK(root.width == 1500u);
    CHECK(root.ascent == 750);
    CHECK(root.descent == 250);

    /* The accessors are bounded. */
    CHECK(sutr_sumnls_box_at(&g_ctx, result.box_count, &child) == false);
    CHECK(sutr_sumnls_box_at(&g_ctx, 0xFFFFFFFFu, &child) == false);

    /* An empty input still produces a root box. */
    CHECK(sutr_sumnls_layout(&g_ctx, NULL, 0u, &result) == true);
    CHECK(result.root_box == 0u);
    CHECK(sutr_sumnls_box_at(&g_ctx, result.root_box, &root) == true);
    CHECK(root.width == 0u);
}

/* ── The single pass of §5.2 and the spacing of §5.8 ─────────────── */

static void test_single_pass_spacing(void)
{
    sumnls_result_t result;
    sumnls_box_t root;
    sumnls_box_t first;
    sumnls_box_t second;
    sumnls_box_t third;
    uint64_t minus_x[2];
    uint64_t a_plus_minus_b[5];

    /* `-x`: the leading minus is reclassified as ordinary, so no space
     * separates it from the x. */
    CHECK(sutr_sumnls_context_init(&g_ctx, g_boxes, TEST_BOXES) == true);
    minus_x[0] = 0x0000002Du;  /* - */
    minus_x[1] = 0x00000078u;  /* x */
    CHECK(sutr_sumnls_layout(&g_ctx, minus_x, 2u, &result) == true);
    CHECK(sutr_sumnls_box_at(&g_ctx, result.root_box, &root) == true);
    CHECK(root.first_child != SUMNLS_BOX_NONE);
    CHECK(sutr_sumnls_box_at(&g_ctx, root.first_child, &first) == true);
    CHECK(sutr_sumnls_box_at(&g_ctx, first.next_sibling, &second) == true);
    /* The minus is the first atom, so it carries no space before it and
     * it is ordinary, so no space follows it either. */
    CHECK(first.gap_before == 0);
    CHECK(second.gap_before == 0);
    CHECK(root.width == 1000u);

    /* `a + -b`: the second minus follows a binary operator, so it is
     * ordinary; the plus keeps a medium space on either side. */
    CHECK(sutr_sumnls_context_init(&g_ctx, g_boxes, TEST_BOXES) == true);
    a_plus_minus_b[0] = 0x00000061u;  /* a */
    a_plus_minus_b[1] = 0x0000002Bu;  /* + */
    a_plus_minus_b[2] = 0x0000002Du;  /* - */
    a_plus_minus_b[3] = 0x00000062u;  /* b */
    CHECK(sutr_sumnls_layout(&g_ctx, a_plus_minus_b, 4u, &result) == true);
    CHECK(sutr_sumnls_box_at(&g_ctx, result.root_box, &root) == true);
    CHECK(sutr_sumnls_box_at(&g_ctx, root.first_child, &first) == true);
    CHECK(sutr_sumnls_box_at(&g_ctx, first.next_sibling, &second) == true);
    CHECK(sutr_sumnls_box_at(&g_ctx, second.next_sibling, &third) == true);
    CHECK(first.gap_before == 0);        /* a */
    CHECK(second.gap_before == 222);     /* + : Ord then Bin is medium */
    CHECK(third.gap_before == 222);      /* - : Bin then Ord is medium */

    /* The fourth atom b follows an ordinary atom, so it carries no
     * space before it. */
    {
        sumnls_box_t fourth;
        CHECK(sutr_sumnls_box_at(&g_ctx, third.next_sibling, &fourth) == true);
        CHECK(fourth.gap_before == 0);
    }
}

/* ── The fraction of §5.3 ─────────────────────────────────────────── */

static void test_fraction(void)
{
    sumnls_result_t result;
    sumnls_box_t root;
    sumnls_box_t fraction;
    sumnls_box_t numerator;
    sumnls_box_t denominator;
    sumnls_box_t rule;
    uint64_t cps[6];
    int32_t axis;
    int32_t rt;
    int32_t half;

    CHECK(sutr_sumnls_context_init(&g_ctx, g_boxes, TEST_BOXES) == true);
    axis = g_ctx.constants.axis_height;
    rt   = g_ctx.constants.rule_thickness;
    half = rt / 2;

    cps[0] = D_FRAC_START;
    cps[1] = 0x00000061u;  /* a */
    cps[2] = D_FRAC_MID;
    cps[3] = 0x00000062u;  /* b */
    cps[4] = D_FRAC_END;
    cps[5] = 0x00000000u;  /* filler, ignored by the engine */
    CHECK(sutr_sumnls_layout(&g_ctx, cps, 5u, &result) == true);

    /* The root holds one child: the fraction. */
    CHECK(sutr_sumnls_box_at(&g_ctx, result.root_box, &root) == true);
    CHECK(root.first_child != SUMNLS_BOX_NONE);
    CHECK(sutr_sumnls_box_at(&g_ctx, root.first_child, &fraction) == true);
    CHECK(fraction.kind == SUMNLS_BOX_VERTICAL);
    CHECK(fraction.axis_height == axis);

    /* The fraction holds the numerator, the rule, and the denominator. */
    CHECK(child_count(&g_ctx, root.first_child) == 3u);
    CHECK(sutr_sumnls_box_at(&g_ctx, fraction.first_child, &numerator) == true);
    CHECK(sutr_sumnls_box_at(&g_ctx, numerator.next_sibling, &rule) == true);
    CHECK(sutr_sumnls_box_at(&g_ctx, rule.next_sibling, &denominator) == true);

    /* The rule is as wide as the widest of the two and is centred on the
     * axis height. */
    CHECK(rule.width == 500u);
    CHECK(rule.ascent == rt);
    CHECK(rule.baseline_shift == axis - half);

    /* The numerator is shifted above the axis and the denominator below
     * it, each clear of the rule. */
    CHECK(numerator.baseline_shift == axis + half + g_ctx.constants.numerator_gap);
    CHECK(denominator.baseline_shift == axis - half - g_ctx.constants.denominator_gap);

    /* The fraction is as wide as the rule and as tall as the shifted
     * numerator. */
    CHECK(fraction.width == 500u);
    CHECK(fraction.ascent == numerator.baseline_shift + numerator.ascent);
    CHECK(fraction.descent ==
          denominator.descent - denominator.baseline_shift);
}

/* ── The radical of §5.4 ─────────────────────────────────────────── */

static void test_radical(void)
{
    sumnls_result_t result;
    sumnls_box_t root;
    sumnls_box_t radical;
    sumnls_box_t sign;
    sumnls_box_t wrap;
    sumnls_box_t radicand;
    uint64_t cps[5];
    int32_t rt;

    CHECK(sutr_sumnls_context_init(&g_ctx, g_boxes, TEST_BOXES) == true);
    rt = g_ctx.constants.rule_thickness;

    cps[0] = D_RAD_START;
    cps[1] = 0x0000221Au;  /* the radical sign */
    cps[2] = 0x00000078u;  /* x */
    cps[3] = D_RAD_END;
    cps[4] = 0x00000000u;
    CHECK(sutr_sumnls_layout(&g_ctx, cps, 4u, &result) == true);

    CHECK(sutr_sumnls_box_at(&g_ctx, result.root_box, &root) == true);
    CHECK(root.first_child != SUMNLS_BOX_NONE);
    CHECK(sutr_sumnls_box_at(&g_ctx, root.first_child, &radical) == true);
    CHECK(radical.kind == SUMNLS_BOX_HORIZONTAL);

    /* The radical holds the sign and the wrapped radicand. */
    CHECK(child_count(&g_ctx, root.first_child) == 2u);
    CHECK(sutr_sumnls_box_at(&g_ctx, radical.first_child, &sign) == true);
    CHECK(sutr_sumnls_box_at(&g_ctx, sign.next_sibling, &wrap) == true);
    CHECK(wrap.kind == SUMNLS_BOX_VERTICAL);

    /* The radicand is lowered by the rule thickness to leave room for
     * the vinculum, and the sign is stretched to cover it. */
    CHECK(sutr_sumnls_box_at(&g_ctx, wrap.first_child, &radicand) == true);
    CHECK(radicand.baseline_shift == -rt);
    CHECK(radicand.gap_before == 0);

    /* The sign's ascent covers the wrapped radicand plus the clearance,
     * which is what stretches the glyph. */
    CHECK(sign.ascent == wrap.ascent + g_ctx.constants.radical_clearance);
    CHECK(sign.descent == wrap.descent);
    CHECK(radical.width == 1000u);
}

/* ── The extensible delimiter strategies of §5.5 ─────────────────── */

static void test_stretch_select(void)
{
    sumnls_stretch_parts_t parts;
    sumnls_stretch_result_t result;

    parts.max_single_height = 1000u;
    parts.top_height        = 300u;
    parts.bottom_height     = 300u;
    parts.extender_height   = 200u;
    parts.overlap           = 0u;

    /* A request inside the single-glyph range uses the single glyph. */
    CHECK(sutr_sumnls_stretch_select(&parts, 500u, &result) == true);
    CHECK(result.strategy == SUMNLS_STRETCH_SINGLE);
    CHECK(result.extender_count == 0u);
    CHECK(result.total_height == 500u);

    /* A request beyond it is assembled from the extender. */
    CHECK(sutr_sumnls_stretch_select(&parts, 2000u, &result) == true);
    CHECK(result.strategy == SUMNLS_STRETCH_ASSEMBLED);
    CHECK(result.extender_count == 7u);
    CHECK(result.total_height == 2000u);

    /* A zero request is degenerate but well defined. */
    CHECK(sutr_sumnls_stretch_select(&parts, 0u, &result) == true);
    CHECK(result.strategy == SUMNLS_STRETCH_SINGLE);
    CHECK(result.total_height == 0u);

    /* The arguments are bounded. */
    CHECK(sutr_sumnls_stretch_select(NULL, 100u, &result) == false);
    CHECK(sutr_sumnls_stretch_select(&parts, 100u, NULL) == false);

    /* A font with no extender clamps to the tallest single glyph. */
    parts.extender_height = 0u;
    CHECK(sutr_sumnls_stretch_select(&parts, 5000u, &result) == true);
    CHECK(result.strategy == SUMNLS_STRETCH_SINGLE);
    CHECK(result.total_height == 1000u);
}

/* ── The script tiers of §5.6 ────────────────────────────────────── */

static void test_script_tiers(void)
{
    sumnls_result_t result;
    sumnls_box_t root;
    sumnls_box_t base;
    sumnls_box_t script;
    sumnls_token_t tokens[3];

    /* The style ladder. */
    CHECK(sutr_sumnls_script_style(SUMNLS_STYLE_DISPLAY) == SUMNLS_STYLE_SCRIPT);
    CHECK(sutr_sumnls_script_style(SUMNLS_STYLE_TEXT)    == SUMNLS_STYLE_SCRIPT);
    CHECK(sutr_sumnls_script_style(SUMNLS_STYLE_SCRIPT)  == SUMNLS_STYLE_SCRIPT_SCRIPT);
    CHECK(sutr_sumnls_script_style(SUMNLS_STYLE_SCRIPT_SCRIPT) == SUMNLS_STYLE_SCRIPT_SCRIPT);

    CHECK(sutr_sumnls_style_name(SUMNLS_STYLE_DISPLAY)[0] == 'd');
    CHECK(sutr_sumnls_style_name(SUMNLS_STYLE_TEXT)[0] == 't');
    CHECK(sutr_sumnls_style_name(SUMNLS_STYLE_SCRIPT)[0] == 's');
    CHECK(sutr_sumnls_style_name(SUMNLS_STYLE_SCRIPT_SCRIPT)[0] == 's');

    /* A superscript tier: the base, then the raised content. */
    CHECK(sutr_sumnls_context_init(&g_ctx, g_boxes, TEST_BOXES) == true);
    tokens[0].codepoint = 0x00000078u;  /* x */
    tokens[0].control   = SUMNLS_CTRL_NONE;
    tokens[1].codepoint = 0x00000032u;  /* 2 */
    tokens[1].control   = SUMNLS_CTRL_SUPERSCRIPT;
    tokens[2].codepoint = 0x00000000u;
    tokens[2].control   = SUMNLS_CTRL_NONE;
    CHECK(sutr_sumnls_layout_tokens(&g_ctx, tokens, 3u, &result) == true);

    CHECK(sutr_sumnls_box_at(&g_ctx, result.root_box, &root) == true);
    CHECK(child_count(&g_ctx, result.root_box) == 2u);
    CHECK(sutr_sumnls_box_at(&g_ctx, root.first_child, &base) == true);
    CHECK(sutr_sumnls_box_at(&g_ctx, base.next_sibling, &script) == true);
    CHECK(script.baseline_shift == g_ctx.constants.superscript_shift);
    CHECK(script.style == SUMNLS_STYLE_SCRIPT);

    /* A subscript tier drops the content instead. */
    CHECK(sutr_sumnls_context_init(&g_ctx, g_boxes, TEST_BOXES) == true);
    tokens[1].control = SUMNLS_CTRL_SUBSCRIPT;
    CHECK(sutr_sumnls_layout_tokens(&g_ctx, tokens, 3u, &result) == true);
    CHECK(sutr_sumnls_box_at(&g_ctx, result.root_box, &root) == true);
    CHECK(sutr_sumnls_box_at(&g_ctx, root.first_child, &base) == true);
    CHECK(sutr_sumnls_box_at(&g_ctx, base.next_sibling, &script) == true);
    CHECK(script.baseline_shift == -g_ctx.constants.subscript_shift);
    CHECK(script.style == SUMNLS_STYLE_SCRIPT);
}

/* ── The limits of §5.7 ──────────────────────────────────────────── */

static void test_limits(void)
{
    sumnls_result_t result;
    sumnls_box_t root;
    sumnls_box_t stack;
    sumnls_box_t limits;
    sumnls_box_t op;
    uint64_t cps[4];

    /* Limits above the operator: the operator is the pending atom of the
     * run when the directive arrives. */
    CHECK(sutr_sumnls_context_init(&g_ctx, g_boxes, TEST_BOXES) == true);
    cps[0] = 0x00002211u;  /* the big operator */
    cps[1] = D_LIM_OVER;
    cps[2] = 0x0000006Eu;  /* n */
    cps[3] = D_GROUP_END;
    CHECK(sutr_sumnls_layout(&g_ctx, cps, 4u, &result) == true);

    CHECK(sutr_sumnls_box_at(&g_ctx, result.root_box, &root) == true);
    CHECK(root.first_child != SUMNLS_BOX_NONE);
    CHECK(sutr_sumnls_box_at(&g_ctx, root.first_child, &stack) == true);
    CHECK(stack.kind == SUMNLS_BOX_VERTICAL);
    CHECK(child_count(&g_ctx, root.first_child) == 2u);

    /* The limits come first, the operator second. */
    CHECK(sutr_sumnls_box_at(&g_ctx, stack.first_child, &limits) == true);
    CHECK(sutr_sumnls_box_at(&g_ctx, limits.next_sibling, &op) == true);
    CHECK(op.baseline_shift == 0);
    CHECK(limits.baseline_shift == op.ascent + g_ctx.constants.script_gap);

    /* Limits below the operator: the order is reversed. */
    CHECK(sutr_sumnls_context_init(&g_ctx, g_boxes, TEST_BOXES) == true);
    cps[1] = D_LIM_UNDER;
    CHECK(sutr_sumnls_layout(&g_ctx, cps, 4u, &result) == true);
    CHECK(sutr_sumnls_box_at(&g_ctx, result.root_box, &root) == true);
    CHECK(sutr_sumnls_box_at(&g_ctx, root.first_child, &stack) == true);
    CHECK(sutr_sumnls_box_at(&g_ctx, stack.first_child, &op) == true);
    CHECK(op.baseline_shift == 0);
    CHECK(sutr_sumnls_box_at(&g_ctx, op.next_sibling, &limits) == true);
    CHECK(limits.baseline_shift == -(op.descent + g_ctx.constants.script_gap));
}

/* ── The isolation scope and the direction of §6.2 and §6.3 ──────── */

static void test_isolate_and_direction(void)
{
    sumnls_result_t result;
    sumnls_box_t root;
    sumnls_box_t isolate;
    uint64_t cps[5];

    /* An isolate brackets a formula and scopes the state inside it. */
    CHECK(sutr_sumnls_context_init(&g_ctx, g_boxes, TEST_BOXES) == true);
    cps[0] = D_ISOLATE_START;
    cps[1] = 0x00000078u;
    cps[2] = D_ISOLATE_END;
    cps[3] = 0x00000079u;
    cps[4] = 0x00000000u;
    CHECK(sutr_sumnls_layout(&g_ctx, cps, 4u, &result) == true);
    CHECK(sutr_sumnls_box_at(&g_ctx, result.root_box, &root) == true);
    /* The root holds the isolate and the trailing atom. */
    CHECK(child_count(&g_ctx, result.root_box) == 2u);
    CHECK(sutr_sumnls_box_at(&g_ctx, root.first_child, &isolate) == true);
    CHECK(isolate.kind == SUMNLS_BOX_HORIZONTAL);
    CHECK(child_count(&g_ctx, root.first_child) == 1u);

    /* The direction switch is scoped by the isolate. */
    CHECK(sutr_sumnls_context_init(&g_ctx, g_boxes, TEST_BOXES) == true);
    cps[0] = D_ISOLATE_START;
    cps[1] = D_MODE_RTL;
    cps[2] = 0x00000078u;
    cps[3] = D_ISOLATE_END;
    cps[4] = 0x00000000u;
    CHECK(sutr_sumnls_layout(&g_ctx, cps, 4u, &result) == true);
    CHECK(result.direction == SUMNLS_DIR_LTR);
}

/* ── The style stack of §5.6 ─────────────────────────────────────── */

static void test_style_stack(void)
{
    sumnls_result_t result;
    uint64_t cps[4];

    CHECK(sutr_sumnls_context_init(&g_ctx, g_boxes, TEST_BOXES) == true);
    cps[0] = D_STYLE_PUSH;
    cps[1] = 0x00000078u;
    cps[2] = D_STYLE_POP;
    cps[3] = 0x00000000u;
    CHECK(sutr_sumnls_layout(&g_ctx, cps, 3u, &result) == true);
    /* The push and the pop balance, so the style at the end is the one
     * the input began in. */
    CHECK(result.style == SUMNLS_STYLE_TEXT);
}

/* ── The alignment tab stops of §6.4 ─────────────────────────────── */

static void test_alignment(void)
{
    sumnls_result_t result;
    sumnls_box_t root;
    sumnls_box_t row;
    sumnls_box_t cell;
    uint64_t cps[9];

    CHECK(sutr_sumnls_context_init(&g_ctx, g_boxes, TEST_BOXES) == true);

    /* Two rows of two cells. The first cell of the first row is wide,
     * the second cell of the second row is narrower: the columns must
     * take the widest cell of each column. */
    cps[0] = D_ALIGN_ROW;
    cps[1] = 0x00000061u;  /* a */
    cps[2] = D_ALIGN_TAB;
    cps[3] = 0x00000062u;  /* b */
    cps[4] = D_ALIGN_ROW;
    cps[5] = 0x00000063u;  /* c */
    cps[6] = D_ALIGN_TAB;
    cps[7] = 0x00000064u;  /* d */
    cps[8] = D_ALIGN_ROW;
    CHECK(sutr_sumnls_layout(&g_ctx, cps, 9u, &result) == true);

    CHECK(sutr_sumnls_box_at(&g_ctx, result.root_box, &root) == true);
    /* The run box itself becomes the vertical container of §6.4. */
    CHECK(root.kind == SUMNLS_BOX_VERTICAL);
    CHECK(root.first_child != SUMNLS_BOX_NONE);

    /* Two rows plus the empty row the trailing directive opened. */
    CHECK(child_count(&g_ctx, result.root_box) == 3u);

    /* The first row holds two cells. */
    CHECK(sutr_sumnls_box_at(&g_ctx, root.first_child, &row) == true);
    CHECK(row.kind == SUMNLS_BOX_HORIZONTAL);
    CHECK(child_count(&g_ctx, root.first_child) == 2u);

    /* Every cell of a column takes the width of the widest cell of that
     * column; here both columns hold a single glyph, so every cell is
     * half an em wide. */
    CHECK(sutr_sumnls_box_at(&g_ctx, row.first_child, &cell) == true);
    CHECK(cell.width == 500u);
    CHECK(sutr_sumnls_box_at(&g_ctx, cell.next_sibling, &cell) == true);
    CHECK(cell.width == 500u);

    /* The container is as wide as its widest row. */
    CHECK(root.width == 1000u);
}

/* ── The conformance rules of §6.6 ───────────────────────────────── */

static void test_conformance(void)
{
    sumnls_result_t result;
    sumnls_box_t root;
    uint64_t unclosed[4];
    uint64_t deep[4];

    /* An unclosed group is closed at the end of the input. */
    CHECK(sutr_sumnls_context_init(&g_ctx, g_boxes, TEST_BOXES) == true);
    unclosed[0] = D_FRAC_START;
    unclosed[1] = 0x00000061u;
    unclosed[2] = D_FRAC_MID;
    unclosed[3] = 0x00000062u;
    CHECK(sutr_sumnls_layout(&g_ctx, unclosed, 4u, &result) == true);
    CHECK(result.ok == true);
    CHECK(sutr_sumnls_box_at(&g_ctx, result.root_box, &root) == true);
    CHECK(root.first_child != SUMNLS_BOX_NONE);
    /* The unclosed fraction became the root's only child. */
    CHECK(child_count(&g_ctx, result.root_box) == 1u);

    /* An unclosed isolate is closed at the end of the input too. */
    CHECK(sutr_sumnls_context_init(&g_ctx, g_boxes, TEST_BOXES) == true);
    deep[0] = D_ISOLATE_START;
    deep[1] = 0x00000061u;
    deep[2] = 0x00000062u;
    deep[3] = 0x00000000u;
    CHECK(sutr_sumnls_layout(&g_ctx, deep, 3u, &result) == true);
    CHECK(result.ok == true);
    CHECK(child_count(&g_ctx, result.root_box) == 1u);

    /* The box pool is bounded: an over-long input is clamped, not fatal,
     * but the engine reports failure rather than writing past the pool. */
    {
        sumnls_box_t tiny[4];
        sumnls_context_t small;
        uint64_t many[8];
        uint32_t i;

        CHECK(sutr_sumnls_context_init(&small, tiny, 4u) == true);
        for (i = 0u; i < 8u; i++) {
            many[i] = 0x00000061u;
        }
        CHECK(sutr_sumnls_layout(&small, many, 8u, &result) == false);
    }
}

int main(void)
{
    test_version_and_meta();
    test_directive_decode();
    test_classification();
    test_spacing_table();
    test_reclassify();
    test_box_model();
    test_single_pass_spacing();
    test_fraction();
    test_radical();
    test_stretch_select();
    test_script_tiers();
    test_limits();
    test_isolate_and_direction();
    test_style_stack();
    test_alignment();
    test_conformance();

    if (g_fail != 0) {
        printf("FAIL: SUTR-9 SUMNLS tests failed (%d)\n", g_fail);
        return 1;
    }
    printf("[PASS] SUTR-9 SUMNLS tests passed\n");
    return 0;
}
