/* Host unit tests for SUTS-002 (SRE) — SuperUnicode Regular Expressions.
 *
 * These tests exercise the reference engine in src/suts/suts_sre.c against
 * docs/suts/SUTS-002-sre.md. Inputs use 64-bit ExtSUCS codepoints, so the
 * same cases cover the Unicode Bridge (Base SUCS), the System Control Plane,
 * Native SUCS, and the unbounded ExtSUCS plugin space.
 */

#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <stddef.h>

#include "suts/suts_sre.h"

/* ── helpers ──────────────────────────────────────────────────────── */

static size_t strlen_ascii(const char* s)
{
    size_t n = 0;
    while (s[n]) n++;
    return n;
}

static suts_sre_compiled_dfa_t* compile_ok(const char* pat,
                                           suts_sre_flags_t flags,
                                           suts_sre_compiled_dfa_t* dfa)
{
    suts_sre_status_t st = suts_sre_compile(pat, strlen_ascii(pat), flags, dfa);
    assert(st == SUTS_SRE_OK);
    (void)st;
    return dfa;
}

static void ascii_to_cps(const char* s, sucs_ex_char_t* out, size_t* n)
{
    size_t i = 0;
    while (s[i]) { out[i] = (sucs_ex_char_t)(unsigned char)s[i]; i++; }
    *n = i;
}

/* search returns raw [start,end) offsets; helper compares against ASCII
 * substring expectations. */
static int search_ascii(const suts_sre_compiled_dfa_t* dfa,
                        const char* hay, size_t* s, size_t* e)
{
    sucs_ex_char_t buf[128];
    size_t n = 0;
    suts_sre_status_t st;
    ascii_to_cps(hay, buf, &n);
    st = suts_sre_search(dfa, buf, n, 0, s, e);
    return st == SUTS_SRE_MATCH;
}

/* ── version / properties ─────────────────────────────────────────── */

static void test_version(void)
{
    assert(suts_sre_version_string() != NULL);
    assert(suts_sre_version_string()[0] != '\0');
    printf("[PASS] test_version\n");
}

static void test_properties(void)
{
    assert(suts_sre_prop_match(SUTS_SRE_PROP_LETTER, 'A'));
    assert(suts_sre_prop_match(SUTS_SRE_PROP_LETTER, 'z'));
    assert(!suts_sre_prop_match(SUTS_SRE_PROP_LETTER, '5'));
    assert(suts_sre_prop_match(SUTS_SRE_PROP_NUMBER, '5'));
    assert(!suts_sre_prop_match(SUTS_SRE_PROP_NUMBER, 'x'));
    assert(suts_sre_prop_match(SUTS_SRE_PROP_WHITESPACE, ' '));
    assert(suts_sre_prop_match(SUTS_SRE_PROP_ASCII, 0x7F));
    assert(!suts_sre_prop_match(SUTS_SRE_PROP_ASCII, 0x80));
    assert(suts_sre_prop_match(SUTS_SRE_PROP_ANY, 0x12345678ULL));

    /* SCP / BANcode zones */
    assert(suts_sre_prop_match(SUTS_SRE_PROP_SCP, SRE_ZONE_SCP_MIN));
    assert(suts_sre_prop_match(SUTS_SRE_PROP_BANCODE, SRE_BANCODE_MIN));
    assert(!suts_sre_prop_match(SUTS_SRE_PROP_BANCODE, SRE_ZONE_SCP_MIN));

    /* Native + ExtSUCS */
    assert(suts_sre_prop_match(SUTS_SRE_PROP_NATIVE, SRE_ZONE_NATIVE_MIN));
    assert(suts_sre_prop_match(SUTS_SRE_PROP_EXTSUCS, 0x80000000ULL));

    /* RNUR: the SUTR-8 Native-plane neography registry (RCCR is a code
       registry, not a codepoint registry, and so exposes no property). */
    assert(suts_sre_prop_match(SUTS_SRE_PROP_RNUR, SRE_RNUR_MIN));
    assert(suts_sre_prop_match(SUTS_SRE_PROP_RNUR, SRE_RNUR_MAX));
    assert(!suts_sre_prop_match(SUTS_SRE_PROP_RNUR, SRE_RNUR_MAX + 1ULL));
    assert(!suts_sre_prop_match(SUTS_SRE_PROP_RNUR, SRE_RNUR_MIN - 1ULL));

    /* Arabic */
    assert(suts_sre_prop_match(SUTS_SRE_PROP_ARABIC, 0x0627ULL));
    printf("[PASS] test_properties\n");
}

/* ── literals, quantifiers, alternation, groups ───────────────────── */

static void test_literals_and_search(void)
{
    suts_sre_compiled_dfa_t dfa;
    size_t s, e;

    compile_ok("abc", SUTS_SRE_FLAG_NONE, &dfa);
    assert(search_ascii(&dfa, "xxabcyy", &s, &e));
    assert(s == 2 && e == 5);

    compile_ok("a.c", SUTS_SRE_FLAG_NONE, &dfa);
    assert(search_ascii(&dfa, "zaxc", &s, &e));
    assert(s == 1 && e == 4);

    compile_ok("ab*c", SUTS_SRE_FLAG_NONE, &dfa);
    assert(search_ascii(&dfa, "ac", &s, &e));
    assert(s == 0 && e == 2);
    assert(search_ascii(&dfa, "abbbc", &s, &e));
    assert(e == 5);

    compile_ok("ab+c", SUTS_SRE_FLAG_NONE, &dfa);
    assert(!search_ascii(&dfa, "ac", &s, &e));
    assert(search_ascii(&dfa, "abc", &s, &e));

    compile_ok("ab?c", SUTS_SRE_FLAG_NONE, &dfa);
    assert(search_ascii(&dfa, "ac", &s, &e));
    assert(search_ascii(&dfa, "abc", &s, &e));

    compile_ok("cat|dog", SUTS_SRE_FLAG_NONE, &dfa);
    assert(search_ascii(&dfa, "a dog here", &s, &e));
    assert(s == 2 && e == 5);

    compile_ok("(ab)+", SUTS_SRE_FLAG_NONE, &dfa);
    assert(search_ascii(&dfa, "zabab!", &s, &e));
    assert(s == 1 && e == 5);

    printf("[PASS] test_literals_and_search\n");
}

/* ── character classes and set operations ─────────────────────────── */

static void test_classes(void)
{
    suts_sre_compiled_dfa_t dfa;
    size_t s, e;

    compile_ok("[a-z]+", SUTS_SRE_FLAG_NONE, &dfa);
    assert(search_ascii(&dfa, "12abc34", &s, &e));
    assert(s == 2 && e == 5);
    assert(!search_ascii(&dfa, "12345", &s, &e));

    compile_ok("[^a-z]+", SUTS_SRE_FLAG_NONE, &dfa);
    assert(search_ascii(&dfa, "abc123", &s, &e));
    assert(s == 3 && e == 6);

    /* Subtraction: letters except vowels. */
    compile_ok("[a-z--[aeiou]]+", SUTS_SRE_FLAG_NONE, &dfa);
    assert(search_ascii(&dfa, "aeiouxyz", &s, &e));
    assert(s == 5 && e == 8);
    assert(!search_ascii(&dfa, "aeiou", &s, &e));

    /* Intersection. */
    compile_ok("[[a-f]&&[d-z]]+", SUTS_SRE_FLAG_NONE, &dfa);
    assert(search_ascii(&dfa, "abcdef", &s, &e));
    assert(s == 3 && e == 6);

    /* Symmetric difference: {a,b,c} xor {c,d,e} = {a,b,d,e}. */
    compile_ok("[abc~~[cde]]+", SUTS_SRE_FLAG_NONE, &dfa);
    assert(search_ascii(&dfa, "xyabde", &s, &e));
    assert(s == 2 && e == 6);
    assert(!search_ascii(&dfa, "cccc", &s, &e));

    /* Property inside a class, and \d shorthand. */
    compile_ok("[\\p{L}\\d]+", SUTS_SRE_FLAG_NONE, &dfa);
    assert(search_ascii(&dfa, "..a1b2..", &s, &e));
    assert(s == 2 && e == 6);

    compile_ok("\\d\\d", SUTS_SRE_FLAG_NONE, &dfa);
    assert(search_ascii(&dfa, "ab42cd", &s, &e));
    assert(s == 2 && e == 4);

    printf("[PASS] test_classes\n");
}

/* ── hex notation ─────────────────────────────────────────────────── */

static void test_hex_notation(void)
{
    suts_sre_compiled_dfa_t dfa;
    size_t s, e;
    sucs_ex_char_t buf[16];
    size_t n;

    /* Greek alpha via \u{...}. */
    compile_ok("\\u{3B1}", SUTS_SRE_FLAG_NONE, &dfa);
    buf[0] = 0x03B1; n = 1;
    assert(suts_sre_match_at(&dfa, buf, n, 0, &s, &e) == SUTS_SRE_MATCH);
    assert(s == 0 && e == 1);
    buf[0] = 0x03B2;
    assert(suts_sre_match_at(&dfa, buf, n, 0, &s, &e) == SUTS_SRE_NOMATCH);

    /* Multiple codepoints: \u{3B1 3B2 3B3} == concatenation. */
    compile_ok("\\u{3B1 3B2 3B3}", SUTS_SRE_FLAG_NONE, &dfa);
    buf[0] = 0x03B1; buf[1] = 0x03B2; buf[2] = 0x03B3; n = 3;
    assert(suts_sre_match_at(&dfa, buf, n, 0, &s, &e) == SUTS_SRE_MATCH);
    assert(e == 3);

    /* Fixed four-digit form. */
    compile_ok("\\u3042", SUTS_SRE_FLAG_NONE, &dfa);
    buf[0] = 0x3042; n = 1;
    assert(suts_sre_match_at(&dfa, buf, n, 0, &s, &e) == SUTS_SRE_MATCH);

    printf("[PASS] test_hex_notation\n");
}

/* ── anchors ──────────────────────────────────────────────────────── */

static void test_anchors(void)
{
    suts_sre_compiled_dfa_t dfa;
    size_t s, e;

    compile_ok("^abc", SUTS_SRE_FLAG_NONE, &dfa);
    assert(search_ascii(&dfa, "abcdef", &s, &e));
    assert(s == 0);
    assert(!search_ascii(&dfa, "zabc", &s, &e));

    compile_ok("abc$", SUTS_SRE_FLAG_NONE, &dfa);
    assert(search_ascii(&dfa, "zzabc", &s, &e));
    assert(e == 5);
    assert(!search_ascii(&dfa, "abcz", &s, &e));

    compile_ok("\\bword\\b", SUTS_SRE_FLAG_NONE, &dfa);
    assert(search_ascii(&dfa, "a word here", &s, &e));
    assert(s == 2 && e == 6);
    assert(!search_ascii(&dfa, "wordy", &s, &e));

    /* Multiline ^ / $ */
    compile_ok("^b", SUTS_SRE_FLAG_MULTILINE, &dfa);
    assert(search_ascii(&dfa, "a\nb", &s, &e));
    assert(s == 2 && e == 3);

    printf("[PASS] test_anchors\n");
}

/* ── case-insensitivity ───────────────────────────────────────────── */

static void test_ignore_case(void)
{
    suts_sre_compiled_dfa_t dfa;
    size_t s, e;

    compile_ok("abc", SUTS_SRE_FLAG_IGNORE_CASE, &dfa);
    assert(search_ascii(&dfa, "xxABCyy", &s, &e));
    assert(s == 2 && e == 5);

    compile_ok("[a-z]+", SUTS_SRE_FLAG_IGNORE_CASE, &dfa);
    assert(search_ascii(&dfa, "ZZZ", &s, &e));

    printf("[PASS] test_ignore_case\n");
}

/* ── SCP handling and ExtSUCS codepoints ──────────────────────────── */

static void test_scp_and_extsucs(void)
{
    suts_sre_compiled_dfa_t dfa;
    size_t s, e;
    sucs_ex_char_t buf[16];
    size_t n;

    /* By default SCP markers are transparent: "ab" matches "a<SCP>b". */
    compile_ok("ab", SUTS_SRE_FLAG_NONE, &dfa);
    buf[0] = 'a'; buf[1] = SRE_ZONE_SCP_MIN + 3; buf[2] = 'b'; n = 3;
    assert(suts_sre_match_at(&dfa, buf, n, 0, &s, &e) == SUTS_SRE_MATCH);

    /* Explicit \p{SCP} re-enables matching the control plane. */
    compile_ok("\\p{SCP}", SUTS_SRE_FLAG_NONE, &dfa);
    buf[0] = SRE_ZONE_SCP_MIN; n = 1;
    assert(suts_sre_match_at(&dfa, buf, n, 0, &s, &e) == SUTS_SRE_MATCH);

    /* NATIVE / ExtSUCS ranges. */
    compile_ok("\\p{ExtSUCS}+", SUTS_SRE_FLAG_NONE, &dfa);
    buf[0] = 0x80000005ULL; buf[1] = 0x123456789ULL; n = 2;
    assert(suts_sre_match_at(&dfa, buf, n, 0, &s, &e) == SUTS_SRE_MATCH);
    assert(e == 2);

    /* ExtSUCS literal via \U escape. */
    compile_ok("\\U00000012\\u{34567}", SUTS_SRE_FLAG_NONE, &dfa);
    buf[0] = 0x12; buf[1] = 0x34567; n = 2;
    assert(suts_sre_match_at(&dfa, buf, n, 0, &s, &e) == SUTS_SRE_MATCH);

    printf("[PASS] test_scp_and_extsucs\n");
}

/* ── SCF canonical equivalence ────────────────────────────────────── */

static void test_scf(void)
{
    suts_sre_compiled_dfa_t dfa;
    size_t s, e;
    sucs_ex_char_t buf[4];
    size_t n;

    /* With SCF flag, e + combining acute composes to U+00E9 and matches. */
    compile_ok("\\u{E9}", SUTS_SRE_FLAG_SCF_NORMALIZED, &dfa);
    buf[0] = 'e'; buf[1] = 0x0301; n = 2;
    assert(suts_sre_match_at(&dfa, buf, n, 0, &s, &e) == SUTS_SRE_MATCH);

    /* Without the flag, the decomposed pair does not match the precomposed. */
    compile_ok("\\u{E9}", SUTS_SRE_FLAG_NONE, &dfa);
    assert(suts_sre_match_at(&dfa, buf, n, 0, &s, &e) == SUTS_SRE_NOMATCH);

    printf("[PASS] test_scf\n");
}

/* ── streaming step engine ────────────────────────────────────────── */

static void test_step(void)
{
    suts_sre_compiled_dfa_t dfa;
    suts_sre_match_state_t m;

    compile_ok("ab", SUTS_SRE_FLAG_NONE, &dfa);
    suts_sre_init_match(&m, &dfa);
    assert(m.is_accepting == false);
    assert(suts_sre_step(&dfa, &m, 'a', 0) == false);
    assert(suts_sre_step(&dfa, &m, 'b', 1) == true);
    assert(m.match_end_offset == 2);

    printf("[PASS] test_step\n");
}

/* ── error handling ───────────────────────────────────────────────── */

static void test_errors(void)
{
    suts_sre_compiled_dfa_t dfa;

    assert(suts_sre_compile("[a-", 3, SUTS_SRE_FLAG_NONE, &dfa) == SUTS_SRE_ERR_BAD_PATTERN);
    assert(suts_sre_compile("\\p{NotAThing}", 13, SUTS_SRE_FLAG_NONE, &dfa) == SUTS_SRE_ERR_BAD_PATTERN);
    assert(suts_sre_compile("(ab", 3, SUTS_SRE_FLAG_NONE, &dfa) == SUTS_SRE_ERR_BAD_PATTERN);
    assert(suts_sre_compile(NULL, 1, SUTS_SRE_FLAG_NONE, &dfa) == SUTS_SRE_ERR_INVALID_ARG);
    assert(suts_sre_compile("a", 1, SUTS_SRE_FLAG_NONE, NULL) == SUTS_SRE_ERR_INVALID_ARG);

    printf("[PASS] test_errors\n");
}

int main(void)
{
    test_version();
    test_properties();
    test_literals_and_search();
    test_classes();
    test_hex_notation();
    test_anchors();
    test_ignore_case();
    test_scp_and_extsucs();
    test_scf();
    test_step();
    test_errors();
    printf("All SUTS-002 (SRE) tests passed.\n");
    return 0;
}
