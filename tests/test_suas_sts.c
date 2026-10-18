/**
 * SUAS-006 STS — System Text Segmentation
 *
 * Unit tests for the zero-allocation, single-pass segmentation state machine.
 *
 * Covers: the Grapheme_Cluster_Break, Word_Break, and Sentence_Break engines
 * over the Unicode Bridge (combining marks, CR/LF, Hangul jamo, emoji ZWJ,
 * regional indicators, Latin words, digit runs, apostrophes), the three-tier
 * boundary status, the SCP segmentation directives (0x00110040), start/end of
 * text, and Native-codepoint opacity.
 */

#include <stdio.h>
#include <stdint.h>
#include <assert.h>

#include "suas/suas_sts.h"

static int g_fail = 0;
#define CHECK(cond) do { if (!(cond)) { \
    printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); g_fail = 1; \
} } while (0)

static void feed(suas_sts_state_t* st, const uint32_t* cps, size_t n,
                 suas_sts_boundary_t* out)
{
    size_t i;
    for (i = 0; i < n; i++)
        out[i] = suas_sts_evaluate(st, (sucs_ex_char_t)cps[i]);
}

static void test_version_and_meta(void)
{
    CHECK(suas_sts_version_string() != NULL);
    CHECK(suas_sts_version_string()[0] == '1');
    CHECK(SUAS_STS_SPEC_ID[0] == 'S');
    CHECK(SUAS_STS_WINDOW_SIZE == 4U);
    CHECK(suas_sts_level_name(SUAS_STS_LEVEL_GRAPHEME)[0] == 'G');
    CHECK(suas_sts_level_name(SUAS_STS_LEVEL_WORD)[0] == 'W');
    CHECK(suas_sts_level_name(SUAS_STS_LEVEL_SENTENCE)[0] == 'S');
}

static void test_classification(void)
{
    CHECK(suas_sts_classify_grapheme(0x0041) == SUAS_STS_GCB_OTHER);
    CHECK(suas_sts_classify_grapheme(0x0301) == SUAS_STS_GCB_EXTEND);
    CHECK(suas_sts_classify_grapheme(0x200D) == SUAS_STS_GCB_ZWJ);
    CHECK(suas_sts_classify_grapheme(0x000D) == SUAS_STS_GCB_CR);
    CHECK(suas_sts_classify_grapheme(0x000A) == SUAS_STS_GCB_LF);
    CHECK(suas_sts_classify_grapheme(0x1100) == SUAS_STS_GCB_L);
    CHECK(suas_sts_classify_grapheme(0x1161) == SUAS_STS_GCB_V);
    CHECK(suas_sts_classify_grapheme(0x11A8) == SUAS_STS_GCB_T);
    CHECK(suas_sts_classify_grapheme(0xAC00) == SUAS_STS_GCB_LV);
    CHECK(suas_sts_classify_grapheme(0xAC01) == SUAS_STS_GCB_LVT);
    CHECK(suas_sts_classify_grapheme(0x1F1E6) == SUAS_STS_GCB_RI);

    CHECK(suas_sts_classify_word(0x0061) == SUAS_STS_WB_ALETTER);
    CHECK(suas_sts_classify_word(0x0030) == SUAS_STS_WB_NUMERIC);
    CHECK(suas_sts_classify_word(0x0020) == SUAS_STS_WB_WSEGSPACE);
    CHECK(suas_sts_classify_word(0x0301) == SUAS_STS_WB_EXTEND);
    CHECK(suas_sts_classify_word(0x0027) == SUAS_STS_WB_SINGLE_QUOTE);
    CHECK(suas_sts_classify_word(0x30A2) == SUAS_STS_WB_KATAKANA);

    CHECK(suas_sts_classify_sentence(0x0041) == SUAS_STS_SB_UPPER);
    CHECK(suas_sts_classify_sentence(0x0061) == SUAS_STS_SB_LOWER);
    CHECK(suas_sts_classify_sentence(0x002E) == SUAS_STS_SB_ATERM);
    CHECK(suas_sts_classify_sentence(0x0021) == SUAS_STS_SB_STERM);
    CHECK(suas_sts_classify_sentence(0x0020) == SUAS_STS_SB_SP);
    CHECK(suas_sts_classify_sentence(0x06F1) == SUAS_STS_SB_NUMERIC);

    /* Native / plugin codepoints are opaque. */
    CHECK(suas_sts_classify_grapheme(0x00120000ULL) == SUAS_STS_GCB_OTHER);
    CHECK(suas_sts_classify_word(0x00170000ULL) == SUAS_STS_WB_OTHER);
    CHECK(suas_sts_classify_sentence(0x00170000ULL) == SUAS_STS_SB_OTHER);
}

static void test_grapheme(void)
{
    suas_sts_state_t st;
    suas_sts_boundary_t out[8];

    /* Start of text, then base + combining mark (no break). */
    {
        const uint32_t cps[] = { 0x0061, 0x0308 };
        suas_sts_init(&st, SUAS_STS_LEVEL_GRAPHEME);
        feed(&st, cps, 2, out);
        CHECK(out[0] == SUAS_STS_BOUNDARY_MUST);
        CHECK(out[1] == SUAS_STS_BOUNDARY_NONE);
    }

    /* CR LF is a single cluster; CR + letter is a hard break. */
    {
        const uint32_t cps[] = { 0x000D, 0x000A, 0x0061 };
        suas_sts_init(&st, SUAS_STS_LEVEL_GRAPHEME);
        feed(&st, cps, 3, out);
        CHECK(out[1] == SUAS_STS_BOUNDARY_NONE);
        CHECK(out[2] == SUAS_STS_BOUNDARY_MUST);
    }

    /* Two ordinary letters break between clusters. */
    {
        const uint32_t cps[] = { 0x0061, 0x0062 };
        suas_sts_init(&st, SUAS_STS_LEVEL_GRAPHEME);
        feed(&st, cps, 2, out);
        CHECK(out[1] == SUAS_STS_BOUNDARY_ALLOW);
    }

    /* Hangul jamo L V T form one cluster (GB6/GB7). */
    {
        const uint32_t cps[] = { 0x1100, 0x1161, 0x11A8 };
        suas_sts_init(&st, SUAS_STS_LEVEL_GRAPHEME);
        feed(&st, cps, 3, out);
        CHECK(out[1] == SUAS_STS_BOUNDARY_NONE);
        CHECK(out[2] == SUAS_STS_BOUNDARY_NONE);
    }

    /* Emoji ZWJ sequence: ExtPict ZWJ ExtPict (GB9 + GB11). */
    {
        const uint32_t cps[] = { 0x1F469, 0x200D, 0x1F469 };
        suas_sts_init(&st, SUAS_STS_LEVEL_GRAPHEME);
        feed(&st, cps, 3, out);
        CHECK(out[1] == SUAS_STS_BOUNDARY_NONE);
        CHECK(out[2] == SUAS_STS_BOUNDARY_NONE);
    }

    /* Regional-indicator flags pair up and then break (GB12/13). */
    {
        const uint32_t cps[] = { 0x1F1E6, 0x1F1E7, 0x1F1E8 };
        suas_sts_init(&st, SUAS_STS_LEVEL_GRAPHEME);
        feed(&st, cps, 3, out);
        CHECK(out[1] == SUAS_STS_BOUNDARY_NONE);
        CHECK(out[2] == SUAS_STS_BOUNDARY_ALLOW);
    }

    /* Devanagari conjunct: consonant linker consonant no-break (GB9c). */
    {
        const uint32_t cps[] = { 0x0915, 0x094D, 0x0937 };
        suas_sts_init(&st, SUAS_STS_LEVEL_GRAPHEME);
        feed(&st, cps, 3, out);
        CHECK(out[1] == SUAS_STS_BOUNDARY_NONE);
        CHECK(out[2] == SUAS_STS_BOUNDARY_NONE);
    }
}

static void test_word(void)
{
    suas_sts_state_t st;
    suas_sts_boundary_t out[8];

    /* Two letters do not break. */
    {
        const uint32_t cps[] = { 0x0061, 0x0062 };
        suas_sts_init(&st, SUAS_STS_LEVEL_WORD);
        feed(&st, cps, 2, out);
        CHECK(out[0] == SUAS_STS_BOUNDARY_MUST);
        CHECK(out[1] == SUAS_STS_BOUNDARY_NONE);
    }

    /* Space separates words. */
    {
        const uint32_t cps[] = { 0x0061, 0x0020, 0x0062 };
        suas_sts_init(&st, SUAS_STS_LEVEL_WORD);
        feed(&st, cps, 3, out);
        CHECK(out[1] == SUAS_STS_BOUNDARY_ALLOW);
        CHECK(out[2] == SUAS_STS_BOUNDARY_ALLOW);
    }

    /* Letters and digits stay together (WB9/WB10). */
    {
        const uint32_t cps[] = { 0x0061, 0x0031, 0x0062 };
        suas_sts_init(&st, SUAS_STS_LEVEL_WORD);
        feed(&st, cps, 3, out);
        CHECK(out[1] == SUAS_STS_BOUNDARY_NONE);
        CHECK(out[2] == SUAS_STS_BOUNDARY_NONE);
    }

    /* Decimal point inside a number (WB6/WB12, curated). */
    {
        const uint32_t cps[] = { 0x0033, 0x002E, 0x0034 };
        suas_sts_init(&st, SUAS_STS_LEVEL_WORD);
        feed(&st, cps, 3, out);
        CHECK(out[1] == SUAS_STS_BOUNDARY_NONE);
        CHECK(out[2] == SUAS_STS_BOUNDARY_NONE);
    }

    /* Apostrophe inside a contraction (WB6/WB7). */
    {
        const uint32_t cps[] = { 0x0063, 0x0061, 0x006E, 0x0027, 0x0074 };
        suas_sts_init(&st, SUAS_STS_LEVEL_WORD);
        feed(&st, cps, 5, out);
        CHECK(out[3] == SUAS_STS_BOUNDARY_NONE);
        CHECK(out[4] == SUAS_STS_BOUNDARY_NONE);
    }

    /* Newline is a mandatory break. */
    {
        const uint32_t cps[] = { 0x0061, 0x000A, 0x0062 };
        suas_sts_init(&st, SUAS_STS_LEVEL_WORD);
        feed(&st, cps, 3, out);
        CHECK(out[1] == SUAS_STS_BOUNDARY_MUST);
        CHECK(out[2] == SUAS_STS_BOUNDARY_MUST);
    }

    /* Regional indicators pair (WB15/16). */
    {
        const uint32_t cps[] = { 0x1F1E6, 0x1F1E7, 0x1F1E8 };
        suas_sts_init(&st, SUAS_STS_LEVEL_WORD);
        feed(&st, cps, 3, out);
        CHECK(out[1] == SUAS_STS_BOUNDARY_NONE);
        CHECK(out[2] == SUAS_STS_BOUNDARY_ALLOW);
    }
}

static void test_sentence(void)
{
    suas_sts_state_t st;
    suas_sts_boundary_t out[8];

    /* "Hi. Bye" -> a sentence boundary before the capital B. */
    {
        const uint32_t cps[] = { 0x0048, 0x0069, 0x002E, 0x0020,
                                 0x0042, 0x0079, 0x0065 };
        suas_sts_init(&st, SUAS_STS_LEVEL_SENTENCE);
        feed(&st, cps, 7, out);
        CHECK(out[3] == SUAS_STS_BOUNDARY_NONE);
        CHECK(out[4] == SUAS_STS_BOUNDARY_ALLOW);
    }

    /* "Hi. bye" -> no boundary (SB8 lower-case continuation). */
    {
        const uint32_t cps[] = { 0x0048, 0x0069, 0x002E, 0x0020,
                                 0x0062, 0x0079, 0x0065 };
        suas_sts_init(&st, SUAS_STS_LEVEL_SENTENCE);
        feed(&st, cps, 7, out);
        CHECK(out[4] == SUAS_STS_BOUNDARY_NONE);
    }

    /* "Hi! Bye" -> a strong terminator always breaks. */
    {
        const uint32_t cps[] = { 0x0048, 0x0069, 0x0021, 0x0020,
                                 0x0042, 0x0079, 0x0065 };
        suas_sts_init(&st, SUAS_STS_LEVEL_SENTENCE);
        feed(&st, cps, 7, out);
        CHECK(out[4] == SUAS_STS_BOUNDARY_ALLOW);
    }

    /* "3.4" -> no boundary (SB6 ATerm x Numeric). */
    {
        const uint32_t cps[] = { 0x0033, 0x002E, 0x0034 };
        suas_sts_init(&st, SUAS_STS_LEVEL_SENTENCE);
        feed(&st, cps, 3, out);
        CHECK(out[2] == SUAS_STS_BOUNDARY_NONE);
    }

    /* Paragraph separator forces a mandatory break (SB4). */
    {
        const uint32_t cps[] = { 0x0061, 0x2029, 0x0062 };
        suas_sts_init(&st, SUAS_STS_LEVEL_SENTENCE);
        feed(&st, cps, 3, out);
        CHECK(out[2] == SUAS_STS_BOUNDARY_MUST);
    }
}

static void test_directives_and_edges(void)
{
    suas_sts_state_t st;
    suas_sts_boundary_t out;

    /* Explicit SCP overrides. */
    suas_sts_init(&st, SUAS_STS_LEVEL_WORD);
    (void)suas_sts_evaluate(&st, 0x0061);
    out = suas_sts_evaluate(&st, SCP_STS_BREAK_FORCE);
    CHECK(out == SUAS_STS_BOUNDARY_MUST);
    out = suas_sts_evaluate(&st, SCP_STS_BREAK_BLOCK);
    CHECK(out == SUAS_STS_BOUNDARY_NONE);
    out = suas_sts_evaluate(&st, SCP_STS_BREAK_ALLOW);
    CHECK(out == SUAS_STS_BOUNDARY_ALLOW);

    /* Directive recognition helper. */
    CHECK(suas_sts_is_directive(SCP_STS_BREAK_FORCE, &out) &&
          out == SUAS_STS_BOUNDARY_MUST);
    CHECK(!suas_sts_is_directive(0x0061, &out));

    /* End of text is always a mandatory boundary. */
    CHECK(suas_sts_flush(&st) == SUAS_STS_BOUNDARY_MUST);

    /* Empty state and NULL guards. */
    suas_sts_init(&st, SUAS_STS_LEVEL_GRAPHEME);
    CHECK(suas_sts_level(&st) == SUAS_STS_LEVEL_GRAPHEME);
    CHECK(suas_sts_evaluate(NULL, 0x0061) == SUAS_STS_BOUNDARY_NONE);

    /* A Native codepoint is opaque but still starts a stream. */
    suas_sts_init(&st, SUAS_STS_LEVEL_GRAPHEME);
    CHECK(suas_sts_evaluate(&st, 0x00170000ULL) == SUAS_STS_BOUNDARY_MUST);
    CHECK(st.count == 1);
}

int main(void)
{
    test_version_and_meta();
    test_classification();
    test_grapheme();
    test_word();
    test_sentence();
    test_directives_and_edges();

    if (g_fail) {
        printf("SUAS-006 STS tests FAILED\n");
        return 1;
    }
    printf("SUAS-006 STS tests passed\n");
    return 0;
}
