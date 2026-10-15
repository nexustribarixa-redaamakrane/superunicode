/**
 * SUAS-005 SSP — System Script Property
 *
 * Unit tests for the range-dispatched O(1) script-resolution engine.
 *
 * Covers: the Script property across the Unicode Bridge (Latin/Greek/Cyrillic/
 * Arabic/Hebrew/Devanagari/Han/Kana/Hangul/Thai/Lao/Tibetan/Braille/...), the
 * implicit Unknown and Common values, the Script_Extensions property and its
 * scx-set membership test, SCP script-context directives, the RNUR neography
 * carve-out, context resolution (inheritance), name/alias lookup, and the
 * font-engine token packing.
 */

#include <stdio.h>
#include <assert.h>
#include "suas/suas_ssp.h"

static int g_fail = 0;
#define CHECK(cond) do { if (!(cond)) { \
    printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); g_fail = 1; \
} } while (0)

static void test_version(void)
{
    CHECK(suas_ssp_version_string() != NULL);
    CHECK(suas_ssp_version_string()[0] == '1');
    CHECK(SUAS_SSP_SPEC_ID[0] == 'S');
}

static void test_bridge_scripts(void)
{
    CHECK(suas_ssp_get_script(0x0041ULL) == SUAS_SCRIPT_LATIN);   /* A */
    CHECK(suas_ssp_get_script(0x00E9ULL) == SUAS_SCRIPT_LATIN);   /* é */
    CHECK(suas_ssp_get_script(0x0391ULL) == SUAS_SCRIPT_GREEK);   /* Α */
    CHECK(suas_ssp_get_script(0x03A9ULL) == SUAS_SCRIPT_GREEK);   /* Ω */
    CHECK(suas_ssp_get_script(0x0410ULL) == SUAS_SCRIPT_CYRILLIC);/* А */
    CHECK(suas_ssp_get_script(0x05D0ULL) == SUAS_SCRIPT_HEBREW);  /* א */
    CHECK(suas_ssp_get_script(0x0627ULL) == SUAS_SCRIPT_ARABIC);  /* ا */
    CHECK(suas_ssp_get_script(0x0905ULL) == SUAS_SCRIPT_DEVANAGARI);
    CHECK(suas_ssp_get_script(0x3042ULL) == SUAS_SCRIPT_HIRAGANA);/* あ */
    CHECK(suas_ssp_get_script(0x30A2ULL) == SUAS_SCRIPT_KATAKANA);/* ア */
    CHECK(suas_ssp_get_script(0x4E00ULL) == SUAS_SCRIPT_HAN);     /* 一 */
    CHECK(suas_ssp_get_script(0xAC00ULL) == SUAS_SCRIPT_HANGUL);  /* 가 */
    CHECK(suas_ssp_get_script(0x0E01ULL) == SUAS_SCRIPT_THAI);    /* ก */
    CHECK(suas_ssp_get_script(0x0E81ULL) == SUAS_SCRIPT_LAO);     /* ກ */
    CHECK(suas_ssp_get_script(0x0F40ULL) == SUAS_SCRIPT_TIBETAN);
    CHECK(suas_ssp_get_script(0x0531ULL) == SUAS_SCRIPT_ARMENIAN);
    CHECK(suas_ssp_get_script(0x10D0ULL) == SUAS_SCRIPT_GEORGIAN);
    CHECK(suas_ssp_get_script(0x2800ULL) == SUAS_SCRIPT_BRAILLE);
    CHECK(suas_ssp_get_script(0x0710ULL) == SUAS_SCRIPT_SYRIAC);
    CHECK(suas_ssp_get_script(0x03E2ULL) == SUAS_SCRIPT_COPTIC);
}

static void test_implicit_values(void)
{
    /* Shared punctuation, digits and symbols are Common. */
    CHECK(suas_ssp_get_script(0x0020ULL) == SUAS_SCRIPT_COMMON); /* space  */
    CHECK(suas_ssp_get_script(0x0030ULL) == SUAS_SCRIPT_COMMON); /* '0'    */
    CHECK(suas_ssp_get_script(0x002CULL) == SUAS_SCRIPT_COMMON); /* comma  */
    CHECK(suas_ssp_get_script(0x2009ULL) == SUAS_SCRIPT_COMMON); /* thin sp*/
    CHECK(suas_ssp_get_script(0x2018ULL) == SUAS_SCRIPT_COMMON); /* quote  */
    CHECK(suas_ssp_get_script(0x2100ULL) == SUAS_SCRIPT_COMMON); /* a/c    */
    CHECK(suas_ssp_get_script(0x00A9ULL) == SUAS_SCRIPT_COMMON); /* ©      */
    CHECK(suas_ssp_get_script(0x0301ULL) == SUAS_SCRIPT_COMMON); /* mark   */

    /* Unassigned / private-use / noncharacter are Unknown. */
    CHECK(suas_ssp_get_script(0x0378ULL) == SUAS_SCRIPT_UNKNOWN);
    CHECK(suas_ssp_get_script(0xE000ULL) == SUAS_SCRIPT_UNKNOWN);  /* PUA */
    CHECK(suas_ssp_get_script(0xFDD0ULL) == SUAS_SCRIPT_UNKNOWN);  /* noncharacter */

    CHECK(suas_ssp_is_implicit(SUAS_SCRIPT_UNKNOWN) == true);
    CHECK(suas_ssp_is_implicit(SUAS_SCRIPT_COMMON) == true);
    CHECK(suas_ssp_is_implicit(SUAS_SCRIPT_LATIN) == false);
}

static void test_scx(void)
{
    /* U+30FC KATAKANA-HIRAGANA PROLONGED SOUND MARK: Script=Common, but its
     * scx set is {Hiragana, Katakana}. */
    suas_ssp_scx_t scx = suas_ssp_get_script_extensions(0x30FCULL);
    CHECK(scx.n == 2);
    CHECK(suas_ssp_scx_contains(&scx, SUAS_SCRIPT_HIRAGANA) == true);
    CHECK(suas_ssp_scx_contains(&scx, SUAS_SCRIPT_KATAKANA) == true);
    CHECK(suas_ssp_scx_contains(&scx, SUAS_SCRIPT_LATIN) == false);
    CHECK(suas_ssp_get_script(0x30FCULL) == SUAS_SCRIPT_COMMON);
    CHECK(suas_ssp_matches_script(0x30FCULL, SUAS_SCRIPT_HIRAGANA) == true);
    CHECK(suas_ssp_matches_script(0x30FCULL, SUAS_SCRIPT_KATAKANA) == true);
    CHECK(suas_ssp_matches_script(0x30FCULL, SUAS_SCRIPT_LATIN) == false);

    /* U+0640 ARABIC TATWEEL: scx {Arabic, Syriac}. */
    scx = suas_ssp_get_script_extensions(0x0640ULL);
    CHECK(scx.n == 2);
    CHECK(suas_ssp_scx_contains(&scx, SUAS_SCRIPT_ARABIC) == true);
    CHECK(suas_ssp_scx_contains(&scx, SUAS_SCRIPT_SYRIAC) == true);
    CHECK(suas_ssp_matches_script(0x0640ULL, SUAS_SCRIPT_SYRIAC) == true);

    /* A plain letter defaults to the singleton { Script(cp) }. */
    scx = suas_ssp_get_script_extensions(0x0041ULL);
    CHECK(scx.n == 1);
    CHECK(scx.scripts[0] == SUAS_SCRIPT_LATIN);

    /* CJK punctuation U+3001 exposes three scripts. */
    scx = suas_ssp_get_script_extensions(0x3001ULL);
    CHECK(scx.n == 3);
    CHECK(suas_ssp_scx_contains(&scx, SUAS_SCRIPT_HAN) == true);
}

static void test_resolve_inheritance(void)
{
    /* A combining mark (Common/implicit) inherits an explicit context. */
    CHECK(suas_ssp_resolve(0x0301ULL, SUAS_SCRIPT_LATIN) == SUAS_SCRIPT_LATIN);
    CHECK(suas_ssp_resolve(0x0301ULL, SUAS_SCRIPT_ARABIC) == SUAS_SCRIPT_ARABIC);
    /* With no explicit context, the implicit value stands. */
    CHECK(suas_ssp_resolve(0x0301ULL, SUAS_SCRIPT_COMMON) == SUAS_SCRIPT_COMMON);
    CHECK(suas_ssp_resolve(0x0301ULL, SUAS_SCRIPT_UNKNOWN) == SUAS_SCRIPT_COMMON);
    /* An explicit codepoint keeps its own script regardless of context. */
    CHECK(suas_ssp_resolve(0x0041ULL, SUAS_SCRIPT_ARABIC) == SUAS_SCRIPT_LATIN);
}

static void test_scp_directives(void)
{
    suas_script_t s = SUAS_SCRIPT_UNKNOWN;
    sucs_ex_char_t latn = SUAS_SSP_SCP_SCRIPT(SUAS_SCRIPT_LATIN);

    CHECK(suas_ssp_is_script_directive(latn, &s) == true);
    CHECK(s == SUAS_SCRIPT_LATIN);
    CHECK(suas_ssp_get_script(latn) == SUAS_SCRIPT_LATIN);

    s = SUAS_SCRIPT_UNKNOWN;
    CHECK(suas_ssp_is_script_directive(
              SUAS_SSP_SCP_SCRIPT(SUAS_SCRIPT_ARABIC), &s) == true);
    CHECK(s == SUAS_SCRIPT_ARABIC);

    /* Non-directive SCP codepoints (BANcodes, SDF/SBR markers) carry no
     * script; the directive sub-range must not swallow them. */
    CHECK(suas_ssp_is_script_directive(0x00110101ULL, &s) == false);
    CHECK(suas_ssp_is_script_directive(0x00110020ULL, &s) == false);
    CHECK(suas_ssp_is_script_directive(0x0011A080ULL, &s) == false);
    CHECK(suas_ssp_get_script(0x00110101ULL) == SUAS_SCRIPT_UNKNOWN);
    CHECK(suas_ssp_get_script(0x0011A080ULL) == SUAS_SCRIPT_UNKNOWN);
}

static void test_rnur_registry(void)
{
    /* The RNUR neography carve-out 0x160000-0x16FFFF is defined, but no
     * registry script is mounted yet, so every coordinate is Unknown. The
     * registry supplies the set -> script mapping when it is mounted. */
    CHECK(suas_ssp_get_script(0x160000ULL) == SUAS_SCRIPT_UNKNOWN);
    CHECK(suas_ssp_get_script(0x1600FFULL) == SUAS_SCRIPT_UNKNOWN);
    CHECK(suas_ssp_get_script(0x160100ULL) == SUAS_SCRIPT_UNKNOWN);
    CHECK(suas_ssp_get_script(0x16FFFFULL) == SUAS_SCRIPT_UNKNOWN);

    /* Native plane outside the carve-out, the terminal 16, and ExtSUCS. */
    CHECK(suas_ssp_get_script(0x00120000ULL) == SUAS_SCRIPT_UNKNOWN);
    CHECK(suas_ssp_get_script(0x7FFFFFEFULL) == SUAS_SCRIPT_UNKNOWN);
    CHECK(suas_ssp_get_script(0x7FFFFFF5ULL) == SUAS_SCRIPT_UNKNOWN);
    CHECK(suas_ssp_get_script(0x7FFFFFFFULL) == SUAS_SCRIPT_UNKNOWN);
    CHECK(suas_ssp_get_script(0x80000000ULL) == SUAS_SCRIPT_UNKNOWN);

    CHECK(suas_ssp_matches_script(0x160000ULL, SUAS_SCRIPT_COMMON) == false);
    CHECK(suas_ssp_matches_script(0x160000ULL, SUAS_SCRIPT_LATIN) == false);
}

static void test_names(void)
{
    CHECK(suas_ssp_script_long_name(SUAS_SCRIPT_LATIN)[0] == 'L');
    CHECK(suas_ssp_script_short_name(SUAS_SCRIPT_LATIN)[0] == 'L');
    CHECK(suas_ssp_script_short_name(SUAS_SCRIPT_ARABIC)[0] == 'A');
    CHECK(suas_ssp_script_short_name(SUAS_SCRIPT_UNKNOWN)[0] == 'Z');

    {
        suas_script_t s = SUAS_SCRIPT_UNKNOWN;
        CHECK(suas_ssp_script_from_tag("Latn", &s) == true && s == SUAS_SCRIPT_LATIN);
        CHECK(suas_ssp_script_from_tag("latin", &s) == true && s == SUAS_SCRIPT_LATIN);
        CHECK(suas_ssp_script_from_tag("  LATN ", &s) == true && s == SUAS_SCRIPT_LATIN);
        CHECK(suas_ssp_script_from_tag("Arab", &s) == true && s == SUAS_SCRIPT_ARABIC);
        CHECK(suas_ssp_script_from_tag("Kana", &s) == true && s == SUAS_SCRIPT_KATAKANA);
        CHECK(suas_ssp_script_from_tag("Nope", &s) == false);
        CHECK(suas_ssp_script_from_tag(NULL, &s) == false);
    }
}

static void test_token_packing(void)
{
    /* The 16-bit script id packs into the low half of a character token so a
     * font engine can select a typeface block without a fallback search. */
    uint64_t tok = suas_ssp_tag_codepoint(0x4E00ULL, SUAS_SCRIPT_HAN);
    CHECK(suas_ssp_token_script(tok) == SUAS_SCRIPT_HAN);
    CHECK(suas_ssp_token_codepoint(tok) == 0x4E00ULL);

    tok = suas_ssp_tag_codepoint(0x160000ULL, SUAS_SCRIPT_COMMON);
    CHECK(suas_ssp_token_script(tok) == SUAS_SCRIPT_COMMON);
    CHECK(suas_ssp_token_codepoint(tok) == 0x160000ULL);

    /* Extension codepoints survive the 31-bit codepoint field. */
    tok = suas_ssp_tag_codepoint(0x7FFFFFEFULL, SUAS_SCRIPT_ARABIC);
    CHECK(suas_ssp_token_script(tok) == SUAS_SCRIPT_ARABIC);
    CHECK(suas_ssp_token_codepoint(tok) == 0x7FFFFFEFULL);
}

int main(void)
{
    test_version();
    test_bridge_scripts();
    test_implicit_values();
    test_scx();
    test_resolve_inheritance();
    test_scp_directives();
    test_rnur_registry();
    test_names();
    test_token_packing();

    if (g_fail) {
        printf("SUAS-005 SSP: FAIL\n");
        return 1;
    }
    printf("[PASS] test_suas_ssp\n");
    return 0;
}
