#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <stddef.h>

#include "suas/suas_sdf.h"

#define SUAS_SUCD_MASK(cp) (suas_sucd_bidi((uint32_t)(cp)))

/* ── SUCD BiDi lookup ──────────────────────────────────────────── */

static void test_sucd_bidi_masks(void)
{
    /* Strong LTR */
    assert(suas_sucd_bidi(0x0041) == SUCD_BIDI_LTR);     /* 'A' */
    assert(suas_sucd_bidi(0x0061) == SUCD_BIDI_LTR);     /* 'a' */
    assert(suas_sucd_bidi(0x0416) == SUCD_BIDI_LTR);     /* Cyrillic Ж */

    /* Arabic letter (RTL) */
    assert((suas_sucd_bidi(0x0627) & SUCD_BIDI_ARABIC_AL) != 0); /* ا */
    assert((suas_sucd_bidi(0x0645) & SUCD_BIDI_ARABIC_AL) != 0); /* م */

    /* Hebrew (strong RTL) */
    assert((suas_sucd_bidi(0x05D0) & SUCD_BIDI_RTL) != 0); /* א */

    /* Mirrorable punctuation */
    assert(suas_sucd_bidi(0x0028) == SUCD_BIDI_MIRRORED); /* ( */
    assert(suas_sucd_bidi(0x0029) == SUCD_BIDI_MIRRORED); /* ) */
    assert(suas_sucd_bidi(0x005B) == SUCD_BIDI_MIRRORED); /* [ */
    assert(suas_sucd_bidi(0x007D) == SUCD_BIDI_MIRRORED); /* } */

    /* Whitespace */
    assert(suas_sucd_bidi(0x0020) == SUCD_BIDI_WHITESPACE);

    /* Unmapped defaults to neutral; outside bridge also neutral */
    assert(suas_sucd_bidi(0xE000) == SUCD_BIDI_NEUTRAL);  /* private use (unmapped) */
    assert(suas_sucd_bidi(0x00120000) == SUCD_BIDI_NEUTRAL); /* native, out of bridge */

    printf("[PASS] test_sucd_bidi_masks\n");
}

/* ── Classification ────────────────────────────────────────────── */

static void test_classify(void)
{
    suas_dir_type_t dir;
    int mirror;

    /* Latin 'A' under LTR */
    assert(suas_sdf_classify(0x0041, SUAS_DIR_LTR, SUAS_SUCD_MASK(0x0041),
                             &dir, &mirror) == SUAS_SDF_OK);
    assert(dir == SUAS_DIR_LTR && mirror == 0);

    /* Arabic letter under any resolved direction -> RTL */
    assert(suas_sdf_classify(0x0627, SUAS_DIR_LTR, suas_sucd_bidi(0x0627),
                             &dir, &mirror) == SUAS_SDF_OK);
    assert(dir == SUAS_DIR_RTL && mirror == 0);

    /* Mirrorable ')' under LTR -> MIRRORED_LTR, mirrored=1 */
    assert(suas_sdf_classify(0x0029, SUAS_DIR_LTR, suas_sucd_bidi(0x0029),
                             &dir, &mirror) == SUAS_SDF_OK);
    assert(dir == SUAS_DIR_MIRRORED_LTR && mirror == 1);

    /* Mirrorable '(' under RTL -> MIRRORED_RTL, mirrored=1 */
    assert(suas_sdf_classify(0x0028, SUAS_DIR_RTL, suas_sucd_bidi(0x0028),
                             &dir, &mirror) == SUAS_SDF_OK);
    assert(dir == SUAS_DIR_MIRRORED_RTL && mirror == 1);

    /* Neutral digit under RTL -> NEUTRAL, no mirror */
    assert(suas_sdf_classify(0x0030, SUAS_DIR_RTL, suas_sucd_bidi(0x0030),
                             &dir, &mirror) == SUAS_SDF_OK);
    assert(dir == SUAS_DIR_NEUTRAL && mirror == 0);

    /* Bad arguments rejected */
    assert(suas_sdf_classify(0x0041, SUAS_DIR_LTR, 0, NULL, &mirror) == SUAS_SDF_ERR_INVALID_ARG);
    assert(suas_sdf_classify(0x0041, SUAS_DIR_LTR, 0, &dir, NULL) == SUAS_SDF_ERR_INVALID_ARG);

    printf("[PASS] test_classify\n");
}

/* ── State machine / Dual-Mode resolver ───────────────────────── */


static void test_explicit_directives(void)
{
    suas_sdf_state_t st;
    suts32_framed_t w;
    size_t count = 0;

    /* Root isolate starts LTR */
    suas_sdf_init(&st, SUAS_DIR_LTR);
    assert(suas_sdf_depth(&st) == 1);
    assert(suas_sdf_current_dir(&st) == SUAS_DIR_LTR);
    assert(suas_sdf_runtime(&st) == SUAS_SDF_RUNTIME_ACTIVE);

    /* RTL directive switches active direction, emits nothing */
    count = 0;
    assert(suas_sdf_process_codepoint(&st, SCP_DIR_RTL, &w, &count) == SUAS_SDF_OK);
    assert(count == 0);
    assert(suas_sdf_current_dir(&st) == SUAS_DIR_RTL);

    /* Push isolate inherits RTL */
    assert(suas_sdf_process_codepoint(&st, SCP_DIR_ISOLATE_PUSH, &w, &count) == SUAS_SDF_OK);
    assert(count == 0);
    assert(suas_sdf_depth(&st) == 2);
    assert(suas_sdf_current_dir(&st) == SUAS_DIR_RTL);

    /* Inside isolate, switch to LTR */
    assert(suas_sdf_process_codepoint(&st, SCP_DIR_LTR, &w, &count) == SUAS_SDF_OK);
    assert(suas_sdf_current_dir(&st) == SUAS_DIR_LTR);

    /* Pop returns to parent RTL */
    assert(suas_sdf_process_codepoint(&st, SCP_DIR_ISOLATE_POP, &w, &count) == SUAS_SDF_OK);
    assert(count == 0);
    assert(suas_sdf_depth(&st) == 1);
    assert(suas_sdf_current_dir(&st) == SUAS_DIR_RTL);

    printf("[PASS] test_explicit_directives\n");
}

static void test_unicode_bridge_implicit(void)
{
    suas_sdf_state_t st;
    suts32_framed_t w;
    size_t count = 0;

    suas_sdf_init(&st, SUAS_DIR_LTR);

    /* Latin 'A' in the Unicode Bridge frames LTR, no mirror */
    count = 0;
    assert(suas_sdf_process_codepoint(&st, 0x0041, &w, &count) == SUAS_SDF_OK);
    assert(count == 1);
    assert(SUAS_SDF_FRAMED_CP(w) == 0x0041);
    assert(SUAS_SDF_FRAMED_DIR(w) == SUAS_DIR_LTR);
    assert(SUAS_SDF_FRAMED_MIRROR(w) == 0);

    /* Arabic letter frames RTL regardless of LTR context */
    assert(suas_sdf_process_codepoint(&st, 0x0627, &w, &count) == SUAS_SDF_OK);
    assert(SUAS_SDF_FRAMED_DIR(w) == SUAS_DIR_RTL);
    assert(SUAS_SDF_FRAMED_MIRROR(w) == 0);

    /* Mirrorable ')' in neutral-linked set -> mirrors in LTR context */
    st.runtime = SUAS_SDF_RUNTIME_ACTIVE;
    assert(suas_sdf_process_codepoint(&st, 0x0029, &w, &count) == SUAS_SDF_OK);
    assert(SUAS_SDF_FRAMED_DIR(w) == SUAS_DIR_MIRRORED_LTR);
    assert(SUAS_SDF_FRAMED_MIRROR(w) == 1);

    printf("[PASS] test_unicode_bridge_implicit\n");
}

static void test_native_inherit(void)
{
    suas_sdf_state_t st;
    suts32_framed_t w;
    size_t count = 0;

    /* Native SUCS codepoint under root LTR inherits (frames NEUTRAL) */
    suas_sdf_init(&st, SUAS_DIR_LTR);
    count = 0;
    assert(suas_sdf_process_codepoint(&st, 0x00120000, &w, &count) == SUAS_SDF_OK);
    assert(count == 1);
    assert(SUAS_SDF_FRAMED_CP(w) == 0x00120000);
    assert(SUAS_SDF_FRAMED_MIRROR(w) == 0);

    /* Under an RTL isolate, native codepoint still inherits RTL scope:
     * dir_type is NEUTRAL but resolves at render from current_dir. */
    assert(suas_sdf_process_codepoint(&st, SCP_DIR_RTL, &w, &count) == SUAS_SDF_OK);
    assert(suas_sdf_current_dir(&st) == SUAS_DIR_RTL);
    assert(suas_sdf_process_codepoint(&st, 0x0012FFFF, &w, &count) == SUAS_SDF_OK);
    assert(SUAS_SDF_FRAMED_CP(w) == 0x0012FFFF);
    assert(SUAS_SDF_FRAMED_DIR(w) == SUAS_DIR_NEUTRAL);

    printf("[PASS] test_native_inherit\n");
}

static void test_structural_errors(void)
{
    suas_sdf_state_t st;
    suts32_framed_t w;
    size_t count = 0;

    /* Popping the root isolate is a structural underflow error */
    suas_sdf_init(&st, SUAS_DIR_LTR);
    assert(suas_sdf_process_codepoint(&st, SCP_DIR_ISOLATE_POP, &w, &count) == SUAS_SDF_ERR_STACK_UNDERFLOW);
    assert(suas_sdf_runtime(&st) == SUAS_SDF_RUNTIME_ERROR);

    /* Stack overflow: push past SUAS_SDF_STACK_DEPTH */
    suas_sdf_init(&st, SUAS_DIR_LTR);
    for (int i = 0; i < SUAS_SDF_STACK_DEPTH - 1; ++i) {
        assert(suas_sdf_process_codepoint(&st, SCP_DIR_ISOLATE_PUSH, &w, &count) == SUAS_SDF_OK);
    }
    /* One more push exceeds depth */
    assert(suas_sdf_process_codepoint(&st, SCP_DIR_ISOLATE_PUSH, &w, &count) == SUAS_SDF_ERR_STACK_OVERFLOW);
    assert(suas_sdf_runtime(&st) == SUAS_SDF_RUNTIME_ERROR);

    /* Input after error is rejected */
    assert(suas_sdf_process_codepoint(&st, 0x0041, &w, &count) == SUAS_SDF_ERR_STATE);

    /* Input after finish() is rejected */
    suas_sdf_init(&st, SUAS_DIR_LTR);
    assert(suas_sdf_finish(&st) == SUAS_SDF_OK);
    assert(suas_sdf_runtime(&st) == SUAS_SDF_RUNTIME_ENDED);
    assert(suas_sdf_process_codepoint(&st, 0x0041, &w, &count) == SUAS_SDF_ERR_STATE);

    printf("[PASS] test_structural_errors\n");
}

static void test_one_shot(void)
{
    /* "A(ב" <- Latin A, '(' , Hebrew bet under root LTR */
    uint32_t cps[] = { 0x0041, 0x0028, 0x05D1 };
    suts32_framed_t out[8];
    size_t count = 0;

    suas_sdf_status_t rc = suas_sdf_frame(cps, 3, out, 8, &count);
    assert(rc == SUAS_SDF_OK);
    assert(count == 3);

    assert(SUAS_SDF_FRAMED_CP(out[0]) == 0x0041);
    assert(SUAS_SDF_FRAMED_DIR(out[0]) == SUAS_DIR_LTR);
    assert(SUAS_SDF_FRAMED_MIRROR(out[0]) == 0);

    assert(SUAS_SDF_FRAMED_CP(out[1]) == 0x0028);
    assert(SUAS_SDF_FRAMED_DIR(out[1]) == SUAS_DIR_MIRRORED_LTR);
    assert(SUAS_SDF_FRAMED_MIRROR(out[1]) == 1);

    assert(SUAS_SDF_FRAMED_CP(out[2]) == 0x05D1);
    assert(SUAS_SDF_FRAMED_DIR(out[2]) == SUAS_DIR_RTL);
    assert(SUAS_SDF_FRAMED_MIRROR(out[2]) == 0);

    /* Buffer-too-small */
    count = 0;
    rc = suas_sdf_frame(cps, 3, out, 2, &count);
    assert(rc == SUAS_SDF_ERR_BUFFER_TOO_SMALL);

    /* Bad args */
    assert(suas_sdf_frame(cps, 3, NULL, 8, &count) == SUAS_SDF_ERR_INVALID_ARG);
    assert(suas_sdf_frame(NULL, 3, out, 8, &count) == SUAS_SDF_ERR_INVALID_ARG);
    assert(suas_sdf_frame(cps, 3, out, 0, &count) == SUAS_SDF_ERR_INVALID_ARG);

    printf("[PASS] test_one_shot\n");
}

int main(void)
{
    printf("--- Running SUAS-001 SDF Unit Tests ---\n");
    test_sucd_bidi_masks();
    test_classify();
    test_explicit_directives();
    test_unicode_bridge_implicit();
    test_native_inherit();
    test_structural_errors();
    test_one_shot();
    printf("--- ALL SDF TESTS PASSED ---\n");
    return 0;
}
