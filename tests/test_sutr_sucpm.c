/**
 * SUTR-8 SUCPM — SuperUnicode Character Property Model
 *
 * Unit tests for the range-dispatched dual property engine: the packed
 * 32-bit core mask and 64-bit extended mask of §5, the normative dispatch
 * order of §4.6, the O(1) property tests of §5.3, the System Control Plane
 * decode of §4.2, the range registers of §4.3 and §4.5, and the property
 * model vocabulary of §3.
 */

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>

#include "sutr/sutr_sucpm.h"
#include "suas/suas_ssre.h"

static int g_fail = 0;
#define CHECK(cond) do { if (!(cond)) { \
    printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); g_fail = 1; \
} } while (0)

/* ── Version, limits, and identifiers ──────────────────────────────── */

static void test_version_and_meta(void)
{
    CHECK(sutr_sucpm_version_string() != NULL);
    CHECK(sutr_sucpm_version_string()[0] == '0');
    CHECK(sutr_sucpm_version_string()[2] == '1');

    CHECK(SUCPM_SPEC_ID[0] == 'S');
    CHECK(SUCPM_SPEC_TITLE[0] == 'S');
    CHECK(SUCPM_MODULE[0] == 's');
    CHECK(SUCPM_VERSION_MAJOR == 0);
    CHECK(SUCPM_VERSION_MINOR == 1);
    CHECK(SUCPM_VERSION_PATCH == 0);

    /* The mask geometry of §5.1 and §5.2. */
    CHECK(SUCPM_BIDI_LTR        == (1u << 0));
    CHECK(SUCPM_BIDI_RTL        == (1u << 1));
    CHECK(SUCPM_BIDI_ARABIC_AL  == (1u << 2));
    CHECK(SUCPM_BIDI_NEUTRAL    == (1u << 3));
    CHECK(SUCPM_BIDI_MIRRORED   == (1u << 4));
    CHECK(SUCPM_BIDI_WHITESPACE == (1u << 5));
    CHECK(SUCPM_BIDI_ALL        == 0x3Fu);

    CHECK(SUCPM_F_ALPHABETIC            == (1u << 6));
    CHECK(SUCPM_F_UPPERCASE              == (1u << 7));
    CHECK(SUCPM_F_LOWERCASE              == (1u << 8));
    CHECK(SUCPM_F_WHITE_SPACE            == (1u << 9));
    CHECK(SUCPM_F_MARK                   == (1u << 10));
    CHECK(SUCPM_F_NUMBER                 == (1u << 11));
    CHECK(SUCPM_F_PUNCTUATION            == (1u << 12));
    CHECK(SUCPM_F_SYMBOL                 == (1u << 13));
    CHECK(SUCPM_F_JOIN_CONTROL           == (1u << 14));
    CHECK(SUCPM_F_DEFAULT_IGNORABLE      == (1u << 15));
    CHECK(SUCPM_F_NONCHARACTER           == (1u << 16));
    CHECK(SUCPM_F_ASSIGNED               == (1u << 17));
    CHECK(SUCPM_F_CANONICAL_DECOMPOSABLE == (1u << 18));
    CHECK(SUCPM_F_COMPOSITION_EXCLUSION  == (1u << 19));
    CHECK(SUCPM_F_BIDI_CONTROL           == (1u << 20));
    CHECK(SUCPM_F_VARIATION_SELECTOR     == (1u << 21));
    CHECK(SUCPM_F_RNUR                   == (1u << 22));
    CHECK(SUCPM_F_MATH                   == (1u << 23));

    CHECK(SUCPM_TIER_SHIFT == 24);
    CHECK(SUCPM_TIER_MASK  == (0xFu << 24));
    CHECK(SUCPM_MATHCLASS_SHIFT == 28);
    CHECK(SUCPM_MATHCLASS_MASK  == (0xFu << 28));

    CHECK(SUCPM_GC_SHIFT          == 32);
    CHECK(SUCPM_CCC_SHIFT         == 37);
    CHECK(SUCPM_SCRIPT_SHIFT      == 45);
    CHECK(SUCPM_LINEBREAK_SHIFT   == 55);
    CHECK(SUCPM_NUMERICTYPE_SHIFT == 61);

    /* The enumerated value sets fit their fields. */
    CHECK((uint32_t)SUCPM_GC_COUNT <= 0x1Fu);
    CHECK((uint32_t)SUCPM_LB_COUNT <= 0x3Fu);
    CHECK((uint32_t)SUCPM_SCRIPT_COUNT <= 0x3FFu);
    CHECK((uint32_t)SUCPM_MC_S <= 0xFu);
}

/* ── The Bridge tier (§4.1) ────────────────────────────────────────── */

static void test_bridge(void)
{
    sucpm_flags64_t m;

    /* 'A' — an uppercase Latin letter. */
    m = sutr_sucpm_props(0x00000041u);
    CHECK(sutr_sucpm_tier_of(0x00000041u) == SUCPM_TIER_BRIDGE);
    CHECK(sutr_sucpm_has(m, SUCPM_F_ALPHABETIC) == true);
    CHECK(sutr_sucpm_has(m, SUCPM_F_UPPERCASE) == true);
    CHECK(sutr_sucpm_has(m, SUCPM_F_LOWERCASE) == false);
    CHECK(sutr_sucpm_gc(m) == SUCPM_GC_LU);
    CHECK(sutr_sucpm_script(m) == SUCPM_SCRIPT_LATIN);
    CHECK(sutr_sucpm_bidi(m) == SUCPM_BIDI_LTR);
    CHECK(sutr_sucpm_ccc(m) == 0u);

    /* 'a' — a lowercase Latin letter. */
    m = sutr_sucpm_props(0x00000061u);
    CHECK(sutr_sucpm_has(m, SUCPM_F_ALPHABETIC) == true);
    CHECK(sutr_sucpm_has(m, SUCPM_F_LOWERCASE) == true);
    CHECK(sutr_sucpm_has(m, SUCPM_F_UPPERCASE) == false);
    CHECK(sutr_sucpm_gc(m) == SUCPM_GC_LL);

    /* '7' — a decimal number. */
    m = sutr_sucpm_props(0x00000037u);
    CHECK(sutr_sucpm_gc(m) == SUCPM_GC_ND);
    CHECK(sutr_sucpm_has(m, SUCPM_F_NUMBER) == true);
    CHECK(sutr_sucpm_numeric_type(m) == SUCPM_NT_DECIMAL);
    CHECK(sutr_sucpm_linebreak(m) == SUCPM_LB_NU);

    /* The space — white space, a separator. */
    m = sutr_sucpm_props(0x00000020u);
    CHECK(sutr_sucpm_gc(m) == SUCPM_GC_ZS);
    CHECK(sutr_sucpm_has(m, SUCPM_F_WHITE_SPACE) == true);
    CHECK(sutr_sucpm_bidi(m) == SUCPM_BIDI_WHITESPACE);

    /* A combining mark, with a combining class. */
    m = sutr_sucpm_props(0x00000300u);
    CHECK(sutr_sucpm_gc(m) == SUCPM_GC_MN);
    CHECK(sutr_sucpm_has(m, SUCPM_F_MARK) == true);
    CHECK(sutr_sucpm_ccc(m) == 230u);
    CHECK(sutr_sucpm_props(0x00000321u) != 0u);
    CHECK(sutr_sucpm_ccc(sutr_sucpm_props(0x00000321u)) == 202u);

    /* Hebrew is right-to-left; Arabic is an Arabic letter. */
    m = sutr_sucpm_props(0x000005D0u);
    CHECK(sutr_sucpm_bidi(m) == SUCPM_BIDI_RTL);
    CHECK(sutr_sucpm_script(m) == SUCPM_SCRIPT_HEBREW);
    m = sutr_sucpm_props(0x00000628u);
    CHECK(sutr_sucpm_bidi(m) == SUCPM_BIDI_ARABIC_AL);
    CHECK(sutr_sucpm_script(m) == SUCPM_SCRIPT_ARABIC);

    /* SuperUnicode has no surrogates: 0xD800 is ordinary private use. */
    m = sutr_sucpm_props(0x0000D800u);
    CHECK(sutr_sucpm_gc(m) == SUCPM_GC_CO);
    CHECK(sutr_sucpm_has(m, SUCPM_F_NONCHARACTER) == false);

    /* An unassigned Bridge codepoint reports Cn. */
    m = sutr_sucpm_props(0x00003780u);
    CHECK(sutr_sucpm_gc(m) == SUCPM_GC_CN);
    CHECK(sutr_sucpm_tier_of(0x00003780u) == SUCPM_TIER_BRIDGE);

    /* The Bridge ends at 0x10FFFF. */
    CHECK(sutr_sucpm_tier_of(0x0010FFFFu) == SUCPM_TIER_BRIDGE);
    CHECK(sutr_sucpm_tier_of(0x00110000u) == SUCPM_TIER_SCP);
}

/* ── The Bridge range list (§4.1) ───────────────────────────────────── */

static void test_bridge_range_list(void)
{
    uint32_t count = sutr_sucpm_bridge_range_count();
    uint32_t i;
    uint32_t prev_high = 0u;
    uint32_t low = 0u;
    uint32_t high = 0u;

    CHECK(count > 0u);

    /* The list is sorted by low endpoint and no two ranges overlap: the
     * binary search of the lookup rests on both claims, so they are
     * checked here rather than asserted in a comment. */
    for (i = 0u; i < count; i++) {
        CHECK(sutr_sucpm_bridge_range_at(i, &low, &high) == true);
        CHECK(low <= high);
        if (i > 0u) {
            CHECK(low > prev_high);
        }
        prev_high = high;
    }

    /* Every range lies inside the Bridge tier. */
    for (i = 0u; i < count; i++) {
        CHECK(sutr_sucpm_bridge_range_at(i, &low, &high) == true);
        CHECK((uint64_t)high <= SUCPM_BRIDGE_MAX);
    }

    /* The accessors are bounded. */
    CHECK(sutr_sucpm_bridge_range_at(count, &low, &high) == false);
    CHECK(sutr_sucpm_bridge_range_at(0xFFFFFFFFu, &low, &high) == false);
}

/* ── The mathematical repertoire (SUTR-9 §2, §4.2) ──────────────────── */

static void test_math_repertoire(void)
{
    sucpm_flags64_t m;

    /* The ASCII mathematical punctuation. */
    m = sutr_sucpm_props(0x00000028u);
    CHECK(sutr_sucpm_is_math(m) == true);
    CHECK(sutr_sucpm_math_class(m) == SUCPM_MC_O);

    m = sutr_sucpm_props(0x00000029u);
    CHECK(sutr_sucpm_math_class(m) == SUCPM_MC_C);

    m = sutr_sucpm_props(0x0000002Bu);
    CHECK(sutr_sucpm_is_math(m) == true);
    CHECK(sutr_sucpm_math_class(m) == SUCPM_MC_B);

    m = sutr_sucpm_props(0x0000002Du);
    CHECK(sutr_sucpm_math_class(m) == SUCPM_MC_B);

    m = sutr_sucpm_props(0x0000003Du);
    CHECK(sutr_sucpm_math_class(m) == SUCPM_MC_R);

    m = sutr_sucpm_props(0x0000002Cu);
    CHECK(sutr_sucpm_math_class(m) == SUCPM_MC_P);

    m = sutr_sucpm_props(0x0000007Cu);
    CHECK(sutr_sucpm_is_math(m) == true);
    CHECK(sutr_sucpm_math_class(m) == SUCPM_MC_F);

    /* The Mathematical Operators block. */
    m = sutr_sucpm_props(0x00002211u); /* n-ary summation */
    CHECK(sutr_sucpm_is_math(m) == true);
    CHECK(sutr_sucpm_math_class(m) == SUCPM_MC_U);

    m = sutr_sucpm_props(0x0000222Bu); /* integral */
    CHECK(sutr_sucpm_is_math(m) == true);
    CHECK(sutr_sucpm_math_class(m) == SUCPM_MC_U);

    m = sutr_sucpm_props(0x00002208u); /* element of */
    CHECK(sutr_sucpm_math_class(m) == SUCPM_MC_R);

    /* The Miscellaneous Mathematical Symbols blocks and the Supplemental
     * Mathematical Operators block. */
    m = sutr_sucpm_props(0x000027C8u);
    CHECK(sutr_sucpm_is_math(m) == true);
    CHECK(sutr_sucpm_gc(m) == SUCPM_GC_SO);

    m = sutr_sucpm_props(0x00002982u);
    CHECK(sutr_sucpm_is_math(m) == true);

    m = sutr_sucpm_props(0x00002A00u);
    CHECK(sutr_sucpm_is_math(m) == true);
    CHECK(sutr_sucpm_math_class(m) == SUCPM_MC_U);

    /* The Mathematical Alphanumeric Symbols block. */
    m = sutr_sucpm_props(0x0001D400u);
    CHECK(sutr_sucpm_is_math(m) == true);
    CHECK(sutr_sucpm_has(m, SUCPM_F_ALPHABETIC) == true);
    CHECK(sutr_sucpm_tier_of(0x0001D400u) == SUCPM_TIER_BRIDGE);

    m = sutr_sucpm_props(0x0001D7CEu);
    CHECK(sutr_sucpm_is_math(m) == true);
    CHECK(sutr_sucpm_gc(m) == SUCPM_GC_ND);

    /* The Arabic Mathematical Alphabetic Symbols block. */
    m = sutr_sucpm_props(0x0001EE00u);
    CHECK(sutr_sucpm_is_math(m) == true);
    CHECK(sutr_sucpm_bidi(m) == SUCPM_BIDI_RTL);

    /* The letterlike mathematics. */
    m = sutr_sucpm_props(0x00002102u); /* double-struck capital C */
    CHECK(sutr_sucpm_is_math(m) == true);
    CHECK(sutr_sucpm_gc(m) == SUCPM_GC_SO);

    /* A plain letter is not mathematical. */
    CHECK(sutr_sucpm_is_math(sutr_sucpm_props(0x00000041u)) == false);
}

/* ── The System Control Plane tier (§4.2) ──────────────────────────── */

static void test_scp(void)
{
    sucpm_flags64_t m;

    /* The class byte is bits 8-15; the argument byte is bits 0-7. */
    CHECK(sutr_sucpm_scp_class_byte(0x0011AABBu) == 0xAAu);
    CHECK(sutr_sucpm_scp_argument_byte(0x0011AABBu) == 0xBBu);

    /* The severity clusters follow the class byte. */
    CHECK(sutr_sucpm_scp_cluster(0x0011A000u) == SUCPM_CLUSTER_B_PLUS);
    CHECK(sutr_sucpm_scp_cluster(0x0011A7FFu) == SUCPM_CLUSTER_B_PLUS);
    CHECK(sutr_sucpm_scp_cluster(0x0011A800u) == SUCPM_CLUSTER_W_PLUS);
    CHECK(sutr_sucpm_scp_cluster(0x0011ABFFu) == SUCPM_CLUSTER_W_PLUS);
    CHECK(sutr_sucpm_scp_cluster(0x0011AC00u) == SUCPM_CLUSTER_C_PLUS);
    CHECK(sutr_sucpm_scp_cluster(0x0011ADFFu) == SUCPM_CLUSTER_C_PLUS);
    CHECK(sutr_sucpm_scp_cluster(0x0011AE00u) == SUCPM_CLUSTER_S_PLUS);
    CHECK(sutr_sucpm_scp_cluster(0x0011AEFFu) == SUCPM_CLUSTER_S_PLUS);
    CHECK(sutr_sucpm_scp_cluster(0x00119FFFu) == SUCPM_CLUSTER_NONE);
    CHECK(sutr_sucpm_scp_cluster(0x00000041u) == SUCPM_CLUSTER_NONE);

    /* The argument byte bears the directional flag and the constraints. */
    CHECK(sutr_sucpm_scp_argument_flags(0x0011A001u) ==
          (uint8_t)SUCPM_SCP_ARG_RTL);
    CHECK(sutr_sucpm_scp_is_rtl(0x0011A001u) == true);
    CHECK(sutr_sucpm_scp_is_ltr(0x0011A001u) == false);
    CHECK(sutr_sucpm_scp_is_ltr(0x0011A002u) == true);
    CHECK(sutr_sucpm_scp_is_isolate(0x0011A004u) == true);
    CHECK(sutr_sucpm_scp_is_embed(0x0011A008u) == true);
    CHECK(sutr_sucpm_scp_forces_break(0x0011A010u) == true);
    CHECK(sutr_sucpm_scp_forbids_break(0x0011A020u) == true);
    CHECK(sutr_sucpm_scp_forbids_break(0x0011A000u) == false);

    /* The mask carries the tier and the Bidi field. */
    m = sutr_sucpm_props(0x0011A001u);
    CHECK((m & SUCPM_TIER_MASK) == SUCPM_TIER_SCP);
    CHECK(sutr_sucpm_bidi(m) == SUCPM_BIDI_RTL);
    m = sutr_sucpm_props(0x0011A002u);
    CHECK(sutr_sucpm_bidi(m) == SUCPM_BIDI_LTR);
    m = sutr_sucpm_props(0x0011A000u);
    CHECK(sutr_sucpm_bidi(m) == SUCPM_BIDI_NEUTRAL);
}

/* ── The Native, RNUR, terminal, and plugin tiers (§4.3-§4.6) ──────── */

static void test_native_rnur_terminal_plugin(void)
{
    sucpm_flags64_t m;

    /* The RNUR registry carve-out is tested before the general Native
     * range: 0x160000 is RNUR, not Native. */
    CHECK(sutr_sucpm_tier_of(0x00160000u) == SUCPM_TIER_RNUR);
    CHECK(sutr_sucpm_tier_of(0x0016FFFFu) == SUCPM_TIER_RNUR);
    CHECK(sutr_sucpm_tier_of(0x0015FFFFu) == SUCPM_TIER_NATIVE);
    CHECK(sutr_sucpm_tier_of(0x00170000u) == SUCPM_TIER_NATIVE);

    m = sutr_sucpm_props(0x00160000u);
    CHECK((m & SUCPM_TIER_MASK) == SUCPM_TIER_RNUR);
    CHECK(sutr_sucpm_has(m, SUCPM_F_RNUR) == true);
    CHECK(sutr_sucpm_has(m, SUCPM_F_ALPHABETIC) == false);

    /* The Native Math ExtSUCS Space is Native and mathematical. */
    m = sutr_sucpm_props(0x00170000u);
    CHECK((m & SUCPM_TIER_MASK) == SUCPM_TIER_NATIVE);
    CHECK(sutr_sucpm_is_math(m) == true);

    /* The terminal 16 — the trap slots and the Sentinel — is reserved and
     * is rejected before the ExtSUCS dispatch. */
    CHECK(sutr_sucpm_tier_of(0x7FFFFFF0u) == SUCPM_TIER_RESERVED);
    CHECK(sutr_sucpm_tier_of(0x7FFFFFFEu) == SUCPM_TIER_RESERVED);
    CHECK(sutr_sucpm_tier_of(0x7FFFFFFFu) == SUCPM_TIER_RESERVED);
    CHECK(sutr_sucpm_props(0x7FFFFFFFu) == (sucpm_flags64_t)SUCPM_MASK_RESERVED);
    CHECK(sutr_sucpm_props(0x7FFFFFF0u) == (sucpm_flags64_t)SUCPM_MASK_RESERVED);

    /* Above the Sentinel is the plugin tier. */
    CHECK(sutr_sucpm_tier_of(0x80000000ULL) == SUCPM_TIER_EXTSUCS);
    m = sutr_sucpm_props(0x80000000ULL);
    CHECK((m & SUCPM_TIER_MASK) == SUCPM_TIER_EXTSUCS);
}

static void test_range_registers(void)
{
    sucpm_flags64_t m;

    sutr_sucpm_reset_registers();
    CHECK(sutr_sucpm_native_register_count() == 0u);
    CHECK(sutr_sucpm_plugin_register_count() == 0u);

    /* A published Native range register is consulted. */
    CHECK(sutr_sucpm_publish_native_range(0x00120000ULL, 0x0012FFFFULL,
                                          SUCPM_F_ASSIGNED |
                                          SUCPM_M_GC(SUCPM_GC_LO)) == true);
    CHECK(sutr_sucpm_native_register_count() == 1u);
    m = sutr_sucpm_props(0x00123456ULL);
    CHECK((m & SUCPM_TIER_MASK) == SUCPM_TIER_NATIVE);
    CHECK(sutr_sucpm_gc(m) == SUCPM_GC_LO);
    CHECK(sutr_sucpm_props(0x00130000ULL) != m);

    /* A published plugin manifest range register is consulted. */
    CHECK(sutr_sucpm_publish_plugin_range(0x80000000ULL, 0x8000FFFFULL,
                                           SUCPM_F_ASSIGNED |
                                           SUCPM_M_GC(SUCPM_GC_LO)) == true);
    CHECK(sutr_sucpm_plugin_register_count() == 1u);
    m = sutr_sucpm_props(0x80001234ULL);
    CHECK((m & SUCPM_TIER_MASK) == SUCPM_TIER_EXTSUCS);
    CHECK(sutr_sucpm_gc(m) == SUCPM_GC_LO);

    /* A malformed register is rejected. */
    CHECK(sutr_sucpm_publish_native_range(0x00140000ULL, 0x00130000ULL, 0u)
          == false);

    sutr_sucpm_reset_registers();
    CHECK(sutr_sucpm_native_register_count() == 0u);
    CHECK(sutr_sucpm_plugin_register_count() == 0u);
}

/* ── The O(1) property tests (§5.3) ────────────────────────────────── */

static void test_o1_tests(void)
{
    sucpm_flags64_t m = sutr_sucpm_props(0x00000041u);

    /* A binary property test is a mask-and-compare. */
    CHECK(sutr_sucpm_has(m, SUCPM_F_ALPHABETIC) == true);
    CHECK(sutr_sucpm_has(m, SUCPM_F_LOWERCASE) == false);
    CHECK(sutr_sucpm_has(m, SUCPM_F_ALPHABETIC | SUCPM_F_UPPERCASE) == true);
    CHECK(sutr_sucpm_has(m, SUCPM_F_ALPHABETIC | SUCPM_F_LOWERCASE) == false);

    /* An enumerated property test is a shift-and-compare. */
    CHECK(sutr_sucpm_is(m, SUCPM_GC_SHIFT, SUCPM_GC_LU) == true);
    CHECK(sutr_sucpm_is(m, SUCPM_GC_SHIFT, SUCPM_GC_LL) == false);
    CHECK(sutr_sucpm_is(m, SUCPM_SCRIPT_SHIFT, SUCPM_SCRIPT_LATIN) == true);
    CHECK(sutr_sucpm_is(m, SUCPM_SCRIPT_SHIFT, SUCPM_SCRIPT_GREEK) == false);
    CHECK(sutr_sucpm_is(m, SUCPM_CCC_SHIFT, 0u) == true);

    /* The Math membership flag and the MathClass field share the core
     * mask's reserved bits, so the two reports agree by construction. */
    m = sutr_sucpm_props(0x00002212u); /* U+2212 MINUS SIGN */
    CHECK(sutr_sucpm_is_math(m) == true);
    CHECK(sutr_sucpm_math_class(m) == SUCPM_MC_B);
    m = sutr_sucpm_props(0x00002211u); /* U+2211 N-ARY SUMMATION */
    CHECK(sutr_sucpm_is_math(m) == true);
    CHECK(sutr_sucpm_math_class(m) == SUCPM_MC_U);
    m = sutr_sucpm_props(0x00000041u);
    CHECK(sutr_sucpm_is_math(m) == false);
    CHECK(sutr_sucpm_math_class(m) == SUCPM_MC_N);
}

/* ── The property registry (§3) ────────────────────────────────────── */

static void test_property_registry(void)
{
    const sucpm_property_record_t *rec;
    uint32_t i;
    uint32_t n = sutr_sucpm_property_count();

    CHECK(n > 0u);

    for (i = 0u; i < n; i++) {
        rec = sutr_sucpm_property_at(i);
        CHECK(rec != NULL);
        if (rec == NULL) {
            continue;
        }
        CHECK(rec->alias != NULL);
        CHECK(rec->alias[0] != '\0');
        CHECK(rec->long_name != NULL);
        CHECK(rec->type < SUCPM_TYPE_COUNT);
        CHECK(rec->status < SUCPM_STATUS_COUNT);
        CHECK(rec->context < SUCPM_CONTEXT_COUNT);
        CHECK(rec->stability < SUCPM_STABILITY_COUNT);
        CHECK(rec->derivation < SUCPM_DERIVATION_COUNT);
        CHECK(rec->tier < SUCPM_TIER_PROPERTY_COUNT);
        CHECK(rec->reserved == 0u);
    }
    CHECK(sutr_sucpm_property_at(n) == NULL);

    rec = sutr_sucpm_property_by_alias("General_Category");
    CHECK(rec != NULL);
    if (rec != NULL) {
        CHECK(rec->type == SUCPM_TYPE_ENUMERATED);
        CHECK(rec->status == SUCPM_STATUS_NORMATIVE);
        CHECK(rec->closed == 1u);
        CHECK(rec->tier == SUCPM_TIER_PROPERTY_NORMATIVE);
    }

    rec = sutr_sucpm_property_by_alias("Math");
    CHECK(rec != NULL);
    if (rec != NULL) {
        CHECK(rec->type == SUCPM_TYPE_BOOLEAN);
        CHECK(rec->status == SUCPM_STATUS_INFORMATIVE);
    }

    rec = sutr_sucpm_property_by_alias("MathClass");
    CHECK(rec != NULL);
    if (rec != NULL) {
        CHECK(rec->type == SUCPM_TYPE_ENUMERATED);
        CHECK(rec->closed == 1u);
    }

    CHECK(sutr_sucpm_property_by_alias("No_Such_Property") == NULL);
    CHECK(sutr_sucpm_property_by_alias(NULL) == NULL);

    /* The explicit "not applicable" special value of §8.1. */
    CHECK(SUCPM_VALUE_NOT_APPLICABLE == 0xFFu);
}

/* ── Taxonomy names ────────────────────────────────────────────────── */

static void test_taxonomy_names(void)
{
    CHECK(sutr_sucpm_property_type_name(SUCPM_TYPE_ENUMERATED)[0] == 'e');
    CHECK(sutr_sucpm_property_type_name(SUCPM_TYPE_BOOLEAN)[0] == 'b');
    CHECK(sutr_sucpm_property_type_name(SUCPM_TYPE_CATALOG)[0] == 'c');
    CHECK(sutr_sucpm_property_type_name((sucpm_property_type_t)999)[0] == 'u');

    CHECK(sutr_sucpm_property_status_name(SUCPM_STATUS_NORMATIVE)[0] == 'n');
    CHECK(sutr_sucpm_property_status_name(SUCPM_STATUS_INFORMATIVE)[0] == 'i');
    CHECK(sutr_sucpm_property_status_name((sucpm_property_status_t)999)[0] == 'u');

    CHECK(sutr_sucpm_context_name(SUCPM_CONTEXT_INDEPENDENT)[0] == 'c');
    CHECK(sutr_sucpm_context_name((sucpm_context_t)999)[0] == 'u');

    CHECK(sutr_sucpm_stability_name(SUCPM_STABILITY_STABLE)[0] == 's');
    CHECK(sutr_sucpm_stability_name(SUCPM_STABILITY_IMMUTABLE)[0] == 'i');
    CHECK(sutr_sucpm_stability_name((sucpm_stability_t)999)[0] == 'u');

    CHECK(sutr_sucpm_derivation_name(SUCPM_DERIVATION_SIMPLE)[0] == 's');
    CHECK(sutr_sucpm_derivation_name(SUCPM_DERIVATION_DERIVED)[0] == 'd');
    CHECK(sutr_sucpm_derivation_name((sucpm_derivation_t)999)[0] == 'u');

    CHECK(sutr_sucpm_property_tier_name(SUCPM_TIER_PROPERTY_NORMATIVE)[0] == 'n');
    CHECK(sutr_sucpm_property_tier_name(SUCPM_TIER_PROPERTY_CONTRIBUTED)[0] == 'c');
    CHECK(sutr_sucpm_property_tier_name((sucpm_property_tier_t)999)[0] == 'u');

    CHECK(sutr_sucpm_cluster_name(SUCPM_CLUSTER_B_PLUS)[0] == 'b');
    CHECK(sutr_sucpm_cluster_name(SUCPM_CLUSTER_NONE)[0] == 'n');

    CHECK(sutr_sucpm_gc_name(SUCPM_GC_LU)[0] == 'L');
    CHECK(sutr_sucpm_gc_name(SUCPM_GC_CN)[0] == 'C');
    CHECK(sutr_sucpm_math_class_name(SUCPM_MC_B)[0] == 'B');
    CHECK(sutr_sucpm_math_class_name(SUCPM_MC_R)[0] == 'R');
    CHECK(sutr_sucpm_numeric_type_name(SUCPM_NT_DECIMAL)[0] == 'd');
}

/* ── Registration in SUAS-010 (SSRE) ────────────────────────────────── */

static void test_ssre_registration(void)
{
    CHECK(suas_ssre_is_registered(SUCPM_SPEC_ID) == true);
    CHECK(suas_ssre_module_of(SUCPM_SPEC_ID) != NULL);
    CHECK(suas_ssre_module_of(SUCPM_SPEC_ID)[0] == 's');
}

int main(void)
{
    test_version_and_meta();
    test_bridge();
    test_bridge_range_list();
    test_math_repertoire();
    test_scp();
    test_native_rnur_terminal_plugin();
    test_range_registers();
    test_o1_tests();
    test_property_registry();
    test_taxonomy_names();
    test_ssre_registration();

    if (g_fail != 0) {
        printf("FAIL: SUTR-8 SUCPM tests failed (%d)\n", g_fail);
        return 1;
    }
    printf("[PASS] SUTR-8 SUCPM tests passed\n");
    return 0;
}
