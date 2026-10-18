/**
 * SUAS-008 SNCS — System Named Character Sequences
 *
 * Unit tests for the zero-allocation, static-registry, O(1) resolution of
 * multi-codepoint symbol names (a UAX #34 analog).
 *
 * Covers: version/meta, O(1) resolution of Bridge (Unicode) sequences, native
 * RNUR neography sequences, miss and guard behaviour, the lazy / idempotent
 * registry build, enumeration and dual-registry classification, name rules
 * (R1 validity, R3 uniqueness-ignoring comparison), sequence well-formedness,
 * and codepoint comparison.
 */

#include <stdio.h>
#include <stdint.h>
#include <assert.h>

#include "suas/suas_sncs.h"

static int g_fail = 0;
#define CHECK(cond) do { if (!(cond)) { \
    printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); g_fail = 1; \
} } while (0)

static void test_version_and_meta(void)
{
    CHECK(suas_sncs_version_string() != NULL);
    CHECK(suas_sncs_version_string()[0] == '1');
    CHECK(suas_sncs_version_string()[2] == '0');
    CHECK(SUAS_SNCS_SPEC_ID[0] == 'S');
    CHECK(SUAS_SNCS_SPEC_TITLE[0] == 'S');
    CHECK(SUAS_SNCS_MAX_SEQUENCE_LEN == 8U);
    CHECK(SUAS_SNCS_MAX_NAME_LEN >= 32U);
    CHECK(suas_sncs_registry_name(SUAS_SNCS_REGISTRY_BRIDGE)[0] == 'B');
    CHECK(suas_sncs_registry_name(SUAS_SNCS_REGISTRY_NATIVE)[0] == 'N');
}

/* Exercises the lazy build: no explicit init before the first resolve. */
static void test_resolve_bridge(void)
{
    suas_named_sequence_t s;

    CHECK(suas_sncs_resolve_sequence(
        "LATIN SMALL LETTER I WITH MACRON AND GRAVE", &s));
    CHECK(s.length == 2u);
    CHECK(s.codepoints[0] == 0x012BULL);
    CHECK(s.codepoints[1] == 0x0300ULL);
    CHECK(suas_sncs_registry_of(&s) == SUAS_SNCS_REGISTRY_BRIDGE);
    CHECK(suas_sncs_is_well_formed(&s));

    CHECK(suas_sncs_resolve_sequence(
        "LATIN SMALL LETTER A WITH CIRCUMFLEX AND ACUTE", &s));
    CHECK(s.length == 3u);
    CHECK(s.codepoints[0] == 0x0061ULL);
    CHECK(s.codepoints[2] == 0x0301ULL);

    CHECK(suas_sncs_resolve_sequence(
        "MODIFIER LETTER EXTRA-HIGH EXTRA-LOW CONTOUR TONE BAR", &s));
    CHECK(s.length == 2u);
    CHECK(s.codepoints[0] == 0x02E5ULL && s.codepoints[1] == 0x02E9ULL);

    CHECK(suas_sncs_resolve_sequence("KHMER CONSONANT SIGN COENG KA", &s));
    CHECK(s.length == 2u);
    CHECK(s.codepoints[0] == 0x17D2ULL && s.codepoints[1] == 0x1780ULL);

    CHECK(suas_sncs_resolve_sequence("TAMIL CONSONANT K", &s));
    CHECK(s.length == 2u);
    CHECK(s.codepoints[0] == 0x0B95ULL && s.codepoints[1] == 0x0BCDULL);

    /* An emoji tag sequence packs seven codepoints. */
    CHECK(suas_sncs_resolve_sequence("EMOJI FLAG ENGLAND", &s));
    CHECK(s.length == 7u);
    CHECK(s.codepoints[0] == 0x1F3F4ULL);
    CHECK(s.codepoints[6] == 0xE007FULL);

    /* An emoji ZWJ family sequence. */
    CHECK(suas_sncs_resolve_sequence("EMOJI FAMILY MAN WOMAN GIRL", &s));
    CHECK(s.length == 5u);
    CHECK(s.codepoints[1] == 0x200DULL);
}

static void test_init_idempotent(void)
{
    suas_named_sequence_t s;

    suas_sncs_init_registry();
    suas_sncs_init_registry(); /* idempotent */
    CHECK(suas_sncs_resolve_sequence("KHMER VOWEL SIGN OM", &s));
    CHECK(s.length == 2u);
    CHECK(s.codepoints[0] == 0x17BBULL && s.codepoints[1] == 0x17C6ULL);
}

static void test_resolve_native(void)
{
    suas_named_sequence_t s;

    CHECK(suas_sncs_resolve_sequence("RNUR COMPOUND SIGN OM", &s));
    CHECK(s.length == 3u);
    CHECK(s.codepoints[0] == 0x160010ULL);
    CHECK(suas_sncs_registry_of(&s) == SUAS_SNCS_REGISTRY_NATIVE);
    CHECK(suas_sncs_is_well_formed(&s));

    CHECK(suas_sncs_resolve_sequence("RNUR LOGOGRAPH DHARMA", &s));
    CHECK(suas_sncs_registry_of(&s) == SUAS_SNCS_REGISTRY_NATIVE);
}

static void test_resolve_miss_and_guards(void)
{
    suas_named_sequence_t s;

    CHECK(!suas_sncs_resolve_sequence("NOT A REAL SEQUENCE", &s));
    CHECK(!suas_sncs_resolve_sequence("TAMIL CONSONANT K", NULL));
    CHECK(!suas_sncs_resolve_sequence(NULL, &s));
    CHECK(!suas_sncs_resolve_sequence("", &s));
}

static void test_registry_enumeration(void)
{
    suas_named_sequence_t s;
    uint32_t n = suas_sncs_registry_count();
    uint32_t i;
    uint32_t bridges = 0, natives = 0;

    CHECK(n >= 20u);
    CHECK(n <= SUAS_SNCS_REGISTRY_CAPACITY);

    for (i = 0; i < n; i++) {
        CHECK(suas_sncs_sequence_at(i, &s));
        CHECK(suas_sncs_is_well_formed(&s));
        if (suas_sncs_registry_of(&s) == SUAS_SNCS_REGISTRY_NATIVE)
            natives++;
        else
            bridges++;
    }
    CHECK(bridges > 0u && natives > 0u);
    CHECK(!suas_sncs_sequence_at(n, &s));
    CHECK(!suas_sncs_sequence_at(0u, NULL));
}

static void test_zone_predicates(void)
{
    CHECK(suas_sncs_is_bridge_codepoint(0x0041ULL));
    CHECK(suas_sncs_is_bridge_codepoint(0x0010FFFFULL));
    CHECK(!suas_sncs_is_bridge_codepoint(0x00110000ULL));
    CHECK(!suas_sncs_is_bridge_codepoint(0x00120000ULL));

    CHECK(suas_sncs_is_native_codepoint(0x00120000ULL));
    CHECK(suas_sncs_is_native_codepoint(SUAS_SNCS_RNUR_MIN));
    CHECK(suas_sncs_is_native_codepoint(0x0016FFFFULL));
    CHECK(suas_sncs_is_native_codepoint(0x80000000ULL)); /* plugin plane */
    CHECK(!suas_sncs_is_native_codepoint(0x0010FFFFULL));
    CHECK(!suas_sncs_is_native_codepoint(0x7FFFFFFFULL)); /* Sentinel */
}

static void test_names(void)
{
    /* R1: valid names. */
    CHECK(suas_sncs_is_valid_name("LATIN SMALL LETTER A"));
    CHECK(suas_sncs_is_valid_name("EMOJI FLAG ENGLAND"));
    CHECK(suas_sncs_is_valid_name("LATIN LETTER -A")); /* hyphen-led word */
    CHECK(suas_sncs_is_valid_name("KHMER CONSONANT SIGN COENG KA"));

    /* R1: invalid names. */
    CHECK(!suas_sncs_is_valid_name(NULL));
    CHECK(!suas_sncs_is_valid_name(""));
    CHECK(!suas_sncs_is_valid_name("latin small letter a")); /* lower-case */
    CHECK(!suas_sncs_is_valid_name("LATIN_SMALL"));          /* underscore  */
    CHECK(!suas_sncs_is_valid_name("8 DIGIT"));              /* digit first */
    CHECK(!suas_sncs_is_valid_name("LATIN\tSMALL"));         /* tab         */

    /* R3: uniqueness-ignoring comparison. */
    CHECK(suas_sncs_names_equal("SARATI LETTER AA", "SARATI CHARACTER AA"));
    CHECK(suas_sncs_names_equal("KHMER VOWEL SIGN OM", "KHMERVOWELSIGNOM"));
    CHECK(suas_sncs_names_equal("A-B", "AB")); /* medial hyphen ignored */
    CHECK(!suas_sncs_names_equal("LATIN LETTER A", "LATIN LETTER B"));
    CHECK(!suas_sncs_names_equal(NULL, "A"));
}

static void test_wellformed_and_compare(void)
{
    suas_named_sequence_t a;
    suas_named_sequence_t b;

    CHECK(suas_sncs_resolve_sequence("KHMER VOWEL SIGN OM", &a));

    /* A named sequence is two or more codepoints (SNCS-D1). */
    a.length = 1u;
    CHECK(!suas_sncs_is_well_formed(&a));

    /* SCP markers, trap slots and the Sentinel are never admissible. */
    a.length = 2u;
    a.codepoints[1] = 0x00110000ULL;
    CHECK(!suas_sncs_is_well_formed(&a));
    a.codepoints[1] = 0x7FFFFFF5ULL;
    CHECK(!suas_sncs_is_well_formed(&a));
    a.codepoints[1] = 0x7FFFFFFFULL;
    CHECK(!suas_sncs_is_well_formed(&a));
    CHECK(!suas_sncs_is_well_formed(NULL));

    /* Sequence comparison ignores the name and compares codepoints. */
    CHECK(suas_sncs_sequence_at(0u, &a));
    CHECK(suas_sncs_resolve_sequence(a.name, &b));
    CHECK(suas_sncs_sequence_equals(&a, &b));
    CHECK(suas_sncs_resolve_sequence("RNUR LIGATURE KA DHI", &b));
    CHECK(!suas_sncs_sequence_equals(&a, &b));
    CHECK(!suas_sncs_sequence_equals(&a, NULL));
}

int main(void)
{
    test_version_and_meta();
    test_resolve_bridge();       /* lazy build path */
    test_init_idempotent();
    test_resolve_native();
    test_resolve_miss_and_guards();
    test_registry_enumeration();
    test_zone_predicates();
    test_names();
    test_wellformed_and_compare();

    if (g_fail) {
        printf("SUAS-008 SNCS tests FAILED\n");
        return 1;
    }
    printf("SUAS-008 SNCS tests passed\n");
    return 0;
}
