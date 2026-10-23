/* Host unit tests for SUTS-011 (SUSN) - SuperUnicode Set Notation.
 *
 * These tests exercise the reference implementation in src/suts/suts_susn.c
 * against docs/suts/SUTS-011-susn.md. Codepoints are 31-bit SUCS codepoints,
 * so the same cases cover the Unicode Compatibility Bridge, the RNUR
 * neography plane, the System Control Plane, the trap slots, and the
 * Sentinel.
 *
 * The behaviours under test are the ones a set consumer reaches for:
 *
 *   - the sketch identifiers, flags, range type, and descriptor layout,
 *   - binary-mapped membership and inversion-list negation,
 *   - union, intersection, difference, symmetric difference, and complement,
 *   - range filtering by address-space zone and the 32-bit probe mask,
 *   - the fixed-capacity builder and canonical range normalization,
 *   - RNUR / conscript set-numbered blocks,
 *   - descriptor and argument guards, and
 *   - the curated catalog's structural invariants.
 */

#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "suts/suts_susn.h"

#define CHECK(cond) assert(cond)

/* ── version and layout ───────────────────────────────────────────── */

static void test_version_and_layout(void)
{
    const char *version = suts_susn_version_string();

    CHECK(version != NULL);
    CHECK(strcmp(version, "SUTS-011/1.0") == 0);
    CHECK(strcmp(SUSN_SPEC_ID, "SUTS-011") == 0);
    CHECK(strcmp(SUSN_SPEC_TITLE, "SuperUnicode Set Notation") == 0);
    CHECK(strcmp(SUSN_MODULE, "suts_susn") == 0);
    CHECK(SUSN_VERSION_MAJOR == 1);
    CHECK(SUSN_VERSION_MINOR == 0);

    /* The three sketch flags keep their bit positions. */
    CHECK(SUSN_FLAG_NEGATED == (1u << 0));
    CHECK(SUSN_FLAG_PROPERTY_BASED == (1u << 1));
    CHECK(SUSN_FLAG_RANGE_ARRAY == (1u << 2));

    /* The inclusive range is exactly two 32-bit words. */
    CHECK(sizeof(susn_codepoint_range_t) == 8u);
    CHECK(offsetof(susn_codepoint_range_t, first) == 0u);
    CHECK(offsetof(susn_codepoint_range_t, last) == 4u);

    /* The sketch descriptor keeps set_id, set_flags, range_count, ranges. */
    CHECK(offsetof(susn_set_descriptor_t, set_id) == 0u);
    CHECK(offsetof(susn_set_descriptor_t, set_flags) == 4u);
    CHECK(offsetof(susn_set_descriptor_t, range_count) == 8u);
    CHECK(offsetof(susn_set_descriptor_t, ranges) >= 12u);

    /* The builder owns its range storage inline. */
    CHECK(sizeof(susn_builder_t) >=
          2u * sizeof(uint32_t) +
          SUSN_BUILDER_CAPACITY * sizeof(susn_codepoint_range_t));
    CHECK(SUSN_BUILDER_CAPACITY == 256u);

    /* The universe is the full 31-bit SuperUnicode address space. */
    CHECK(SUSN_UNIVERSE_MAX == 0x7FFFFFFFu);
    CHECK(SUSN_UNIVERSE_SIZE == 0x80000000ULL);

    /* The entry point is idempotent and has no state to build. */
    suts_susn_init_engine();
    suts_susn_init_engine();

    printf("[PASS] test_version_and_layout\n");
}

/* ── the curated catalog (§6) ─────────────────────────────────────── */

static void test_catalog(void)
{
    uint32_t i;
    uint32_t seen = 0u;

    CHECK(suts_susn_catalog_count() == 25u);
    CHECK(suts_susn_catalog_at(25u) == NULL);

    for (i = 0u; i < suts_susn_catalog_count(); i++) {
        const susn_set_descriptor_t *set = suts_susn_catalog_at(i);

        CHECK(set != NULL);
        CHECK(suts_susn_validate(set));
        CHECK(set->range_count > 0u);
        CHECK((set->set_flags & SUSN_FLAG_RANGE_ARRAY) != 0u);
        CHECK(suts_susn_catalog_find(set->set_id) == set);
        CHECK(suts_susn_catalog_find_name(suts_susn_set_name(set->set_id)) ==
              set);
        seen |= (1u << i);
    }
    CHECK(seen == ((1u << 25) - 1u));

    /* Basic Latin. */
    {
        const susn_set_descriptor_t *ascii =
            suts_susn_catalog_find(SUSN_SETID_ASCII);
        const susn_set_descriptor_t *printable =
            suts_susn_catalog_find(SUSN_SETID_PRINTABLE_ASCII);
        const susn_set_descriptor_t *digit =
            suts_susn_catalog_find(SUSN_SETID_DIGIT);
        const susn_set_descriptor_t *upper =
            suts_susn_catalog_find(SUSN_SETID_UPPER);
        const susn_set_descriptor_t *lower =
            suts_susn_catalog_find(SUSN_SETID_LOWER);

        CHECK(ascii != NULL && printable != NULL && digit != NULL);
        CHECK(upper != NULL && lower != NULL);

        CHECK(suts_susn_contains(ascii, 0x0000u));
        CHECK(suts_susn_contains(ascii, 0x0041u));
        CHECK(suts_susn_contains(ascii, 0x007Fu));
        CHECK(!suts_susn_contains(ascii, 0x0080u));

        CHECK(!suts_susn_contains(printable, 0x001Fu));
        CHECK(suts_susn_contains(printable, 0x0020u));
        CHECK(suts_susn_contains(printable, 0x007Eu));
        CHECK(!suts_susn_contains(printable, 0x007Fu));

        CHECK(suts_susn_contains(digit, 0x0030u));
        CHECK(suts_susn_contains(digit, 0x0039u));
        CHECK(!suts_susn_contains(digit, 0x002Fu));
        CHECK(!suts_susn_contains(digit, 0x003Au));

        CHECK(suts_susn_contains(upper, 0x0041u));
        CHECK(!suts_susn_contains(upper, 0x0061u));
        CHECK(suts_susn_contains(lower, 0x0061u));
        CHECK(!suts_susn_contains(lower, 0x0041u));

        CHECK(suts_susn_cardinality(ascii) == 128u);
        CHECK(suts_susn_cardinality(digit) == 10u);
        CHECK(suts_susn_cardinality(upper) == 26u);
        CHECK(suts_susn_cardinality(lower) == 26u);
    }

    /* Properties and scripts. */
    {
        const susn_set_descriptor_t *ws =
            suts_susn_catalog_find(SUSN_SETID_WHITESPACE);
        const susn_set_descriptor_t *bidi =
            suts_susn_catalog_find(SUSN_SETID_BIDI_CONTROL);

        CHECK(suts_susn_contains(ws, 0x0009u));
        CHECK(suts_susn_contains(ws, 0x0020u));
        CHECK(suts_susn_contains(ws, 0x00A0u));
        CHECK(suts_susn_contains(ws, 0x3000u));
        CHECK(!suts_susn_contains(ws, 0x0041u));
        CHECK(!suts_susn_contains(ws, 0x200Bu));

        CHECK(suts_susn_contains(bidi, 0x061Cu));
        CHECK(suts_susn_contains(bidi, 0x200Eu));
        CHECK(suts_susn_contains(bidi, 0x202Eu));
        CHECK(suts_susn_contains(bidi, 0x2069u));
        CHECK(!suts_susn_contains(bidi, 0x202Fu));
        CHECK(!suts_susn_contains(bidi, 0x206Au));

        CHECK(suts_susn_contains(suts_susn_catalog_find(SUSN_SETID_GREEK),
                                 0x03B1u));
        CHECK(suts_susn_contains(suts_susn_catalog_find(SUSN_SETID_CYRILLIC),
                                 0x0410u));
        CHECK(suts_susn_contains(suts_susn_catalog_find(SUSN_SETID_HEBREW),
                                 0x05D0u));
        CHECK(suts_susn_contains(suts_susn_catalog_find(SUSN_SETID_ARABIC),
                                 0x0627u));
        CHECK(suts_susn_contains(suts_susn_catalog_find(SUSN_SETID_HIRAGANA),
                                 0x3042u));
        CHECK(suts_susn_contains(suts_susn_catalog_find(SUSN_SETID_KATAKANA),
                                 0x30A2u));
        CHECK(suts_susn_contains(suts_susn_catalog_find(SUSN_SETID_HAN),
                                 0x4E00u));
        CHECK(suts_susn_contains(suts_susn_catalog_find(SUSN_SETID_HAN),
                                 0x20000u));
        CHECK(suts_susn_contains(suts_susn_catalog_find(SUSN_SETID_HANGUL),
                                 0xAC00u));
        CHECK(suts_susn_contains(suts_susn_catalog_find(SUSN_SETID_EMOJI),
                                 0x1F600u));
        CHECK(!suts_susn_contains(suts_susn_catalog_find(SUSN_SETID_EMOJI),
                                  0x0041u));
    }

    /* Zone sets. */
    {
        const susn_set_descriptor_t *bridge =
            suts_susn_catalog_find(SUSN_SETID_BRIDGE);
        const susn_set_descriptor_t *scp =
            suts_susn_catalog_find(SUSN_SETID_SCP);
        const susn_set_descriptor_t *rnur =
            suts_susn_catalog_find(SUSN_SETID_RNUR);
        const susn_set_descriptor_t *native =
            suts_susn_catalog_find(SUSN_SETID_NATIVE);
        const susn_set_descriptor_t *trap =
            suts_susn_catalog_find(SUSN_SETID_TRAP);
        const susn_set_descriptor_t *sentinel =
            suts_susn_catalog_find(SUSN_SETID_SENTINEL);

        CHECK(suts_susn_contains(bridge, 0x00000000u));
        CHECK(suts_susn_contains(bridge, 0x0010FFFFu));
        CHECK(!suts_susn_contains(bridge, 0x00110000u));

        CHECK(suts_susn_contains(scp, 0x00110000u));
        CHECK(suts_susn_contains(scp, 0x0011FFFFu));
        CHECK(!suts_susn_contains(scp, 0x00120000u));

        CHECK(suts_susn_contains(rnur, 0x00160000u));
        CHECK(suts_susn_contains(rnur, 0x0016FFFFu));
        CHECK(!suts_susn_contains(rnur, 0x0015FFFFu));

        CHECK(suts_susn_contains(native, 0x00120000u));
        CHECK(suts_susn_contains(native, 0x00160000u));
        CHECK(suts_susn_contains(native, 0x7FFFFFEFu));
        CHECK(!suts_susn_contains(native, 0x0011FFFFu));
        CHECK(!suts_susn_contains(native, 0x7FFFFFF0u));

        CHECK(suts_susn_contains(trap, 0x7FFFFFF0u));
        CHECK(suts_susn_contains(trap, 0x7FFFFFFEu));
        CHECK(!suts_susn_contains(trap, 0x7FFFFFFFu));

        CHECK(suts_susn_contains(sentinel, 0x7FFFFFFFu));
        CHECK(!suts_susn_contains(sentinel, 0x7FFFFFFEu));
    }

    CHECK(strcmp(suts_susn_set_name(SUSN_SETID_ASCII), "ASCII") == 0);
    CHECK(strcmp(suts_susn_set_name(SUSN_SETID_HAN), "Han") == 0);
    CHECK(strcmp(suts_susn_set_name(SUSN_SETID_NONE), "Empty") == 0);
    CHECK(strcmp(suts_susn_set_name(0xDEADBEEFu), "Unknown") == 0);

    printf("[PASS] test_catalog\n");
}

/* ── membership and inversion lists (§3.3) ────────────────────────── */

static void test_membership_negation(void)
{
    static const susn_codepoint_range_t ascii_ranges[] = {
        { 0x00000000u, 0x0000007Fu }
    };
    susn_set_descriptor_t empty;
    susn_set_descriptor_t ascii;
    susn_set_descriptor_t not_ascii;

    CHECK(suts_susn_descriptor_init(&empty, 0u, SUSN_FLAG_RANGE_ARRAY,
                                    0u, NULL));
    CHECK(suts_susn_descriptor_init(&ascii, SUSN_SETID_ASCII,
                                    SUSN_FLAG_RANGE_ARRAY, 1u, ascii_ranges));
    CHECK(suts_susn_descriptor_init(&not_ascii, 100u,
                                    SUSN_FLAG_RANGE_ARRAY | SUSN_FLAG_NEGATED,
                                    1u, ascii_ranges));

    CHECK(suts_susn_validate(&ascii));
    CHECK(suts_susn_validate(&not_ascii));
    CHECK(suts_susn_is_empty(&empty));
    CHECK(!suts_susn_is_empty(&ascii));
    CHECK(!suts_susn_is_universe(&empty));
    CHECK(suts_susn_is_universe(&not_ascii) == false);
    CHECK(suts_susn_cardinality(&empty) == 0u);
    CHECK(suts_susn_cardinality(&ascii) == 128u);
    CHECK(suts_susn_cardinality(&not_ascii) ==
          SUSN_UNIVERSE_SIZE - 128u);

    /* A NULL descriptor is the empty set. */
    CHECK(!suts_susn_contains(NULL, 0x0041u));
    CHECK(suts_susn_is_empty(NULL));

    /* An empty set contains nothing; its inversion contains everything. */
    CHECK(!suts_susn_contains(&empty, 0x0000u));
    CHECK(!suts_susn_contains(&empty, 0x7FFFFFFFu));

    /* A positive set. */
    CHECK(suts_susn_contains(&ascii, 0x0000u));
    CHECK(suts_susn_contains(&ascii, 0x0041u));
    CHECK(suts_susn_contains(&ascii, 0x007Fu));
    CHECK(!suts_susn_contains(&ascii, 0x0080u));
    CHECK(!suts_susn_contains(&ascii, 0x7FFFFFFFu));

    /* The inversion list. */
    CHECK(!suts_susn_contains(&not_ascii, 0x0041u));
    CHECK(suts_susn_contains(&not_ascii, 0x0080u));
    CHECK(suts_susn_contains(&not_ascii, 0x00110000u));
    CHECK(suts_susn_contains(&not_ascii, 0x7FFFFFFFu));

    /* Above the Sentinel is ExtSUCS, never a member of a 31-bit set. */
    CHECK(!suts_susn_contains(&not_ascii, 0x80000000u));
    CHECK(!suts_susn_contains(&not_ascii, 0xFFFFFFFFu));

    /* The zone filter restricts the query without materializing a second set. */
    CHECK(suts_susn_contains_in(&not_ascii, 0x0080u, SUSN_FILTER_BRIDGE));
    CHECK(suts_susn_contains_in(&not_ascii, 0x00110000u, SUSN_FILTER_ALL));
    CHECK(!suts_susn_contains_in(&not_ascii, 0x00110000u,
                                 SUSN_FILTER_BRIDGE));
    CHECK(!suts_susn_contains_in(&not_ascii, 0x0080u, 0u));
    CHECK(!suts_susn_contains_in(&ascii, 0x0080u, SUSN_FILTER_ALL));

    /* A full inversion is the universe. */
    {
        susn_set_descriptor_t universe;

        CHECK(suts_susn_descriptor_init(&universe, 0u,
                                        SUSN_FLAG_RANGE_ARRAY |
                                            SUSN_FLAG_NEGATED,
                                        0u, NULL));
        CHECK(suts_susn_is_universe(&universe));
        CHECK(!suts_susn_is_empty(&universe));
        CHECK(suts_susn_cardinality(&universe) == SUSN_UNIVERSE_SIZE);
        CHECK(suts_susn_contains(&universe, 0x0000u));
        CHECK(suts_susn_contains(&universe, 0x7FFFFFFFu));
        CHECK(!suts_susn_contains(&universe, 0x80000000u));
    }

    printf("[PASS] test_membership_negation\n");
}

/* ── canonicalization and validation (§3.1) ───────────────────────── */

static void test_validation_and_normalize(void)
{
    susn_codepoint_range_t out[8];
    susn_codepoint_range_t bad[3];
    susn_set_descriptor_t set;

    /* Normalization sorts and merges overlapping and adjacent ranges. */
    {
        static const susn_codepoint_range_t in[] = {
            { 0x00000020u, 0x0000002Eu },
            { 0x00000000u, 0x0000000Fu },
            { 0x00000010u, 0x0000001Fu },
            { 0x00000005u, 0x00000008u },
            { 0x00000030u, 0x0000003Fu }
        };
        uint32_t n = suts_susn_normalize(in, 5u, out, 8u);

        CHECK(n == 2u);
        CHECK(out[0].first == 0x00000000u && out[0].last == 0x0000002Eu);
        CHECK(out[1].first == 0x00000030u && out[1].last == 0x0000003Fu);
    }

    CHECK(suts_susn_normalize(NULL, 0u, NULL, 0u) == 0u);

    /* Capacity is a hard requirement for the sort. */
    {
        static const susn_codepoint_range_t in[] = {
            { 0x00000001u, 0x00000001u },
            { 0x00000000u, 0x00000000u }
        };
        CHECK(suts_susn_normalize(in, 2u, out, 1u) == 0u);
        CHECK(suts_susn_normalize(in, 2u, NULL, 2u) == 0u);
    }

    /* Invalid ranges are refused. */
    bad[0].first = 0x00000010u;
    bad[0].last = 0x0000000Fu;
    CHECK(suts_susn_normalize(bad, 1u, out, 8u) == 0u);

    /* A non-canonical descriptor fails validation. */
    {
        static const susn_codepoint_range_t overlapping[] = {
            { 0x00000000u, 0x00000010u },
            { 0x00000010u, 0x00000020u }
        };
        static const susn_codepoint_range_t adjacent[] = {
            { 0x00000000u, 0x00000010u },
            { 0x00000011u, 0x00000020u }
        };

        CHECK(suts_susn_descriptor_init(&set, 0u, SUSN_FLAG_RANGE_ARRAY,
                                        2u, overlapping));
        CHECK(!suts_susn_validate(&set));
        CHECK(suts_susn_descriptor_init(&set, 0u, SUSN_FLAG_RANGE_ARRAY,
                                        2u, adjacent));
        CHECK(!suts_susn_validate(&set));
    }

    CHECK(!suts_susn_validate(NULL));
    CHECK(!suts_susn_descriptor_init(NULL, 0u, 0u, 0u, NULL));
    CHECK(!suts_susn_descriptor_init(&set, 0u, 0u, 1u, NULL));

    /* An undefined flag bit is not a valid descriptor. */
    {
        static const susn_codepoint_range_t one[] = {
            { 0x00000000u, 0x00000000u }
        };
        CHECK(suts_susn_descriptor_init(&set, 0u, 0x80000000u, 1u, one));
        CHECK(!suts_susn_validate(&set));
    }

    printf("[PASS] test_validation_and_normalize\n");
}

/* ── set algebra (§3.2) ───────────────────────────────────────────── */

static void test_set_algebra(void)
{
    const susn_set_descriptor_t *ascii =
        suts_susn_catalog_find(SUSN_SETID_ASCII);
    const susn_set_descriptor_t *digit =
        suts_susn_catalog_find(SUSN_SETID_DIGIT);
    const susn_set_descriptor_t *greek =
        suts_susn_catalog_find(SUSN_SETID_GREEK);
    const susn_set_descriptor_t *bridge =
        suts_susn_catalog_find(SUSN_SETID_BRIDGE);
    const susn_set_descriptor_t *rnur =
        suts_susn_catalog_find(SUSN_SETID_RNUR);
    susn_codepoint_range_t out[16];

    CHECK(ascii != NULL && digit != NULL && greek != NULL);
    CHECK(bridge != NULL && rnur != NULL);

    /* Union: ASCII union Greek. */
    {
        uint32_t n = suts_susn_union(ascii, greek, out, 16u);

        CHECK(n >= 3u);
        CHECK(out[0].first == 0x00000000u && out[0].last == 0x0000007Fu);
        CHECK(suts_susn_contains(ascii, 0x0041u));
        CHECK(suts_susn_contains(greek, 0x03B1u));
    }

    /* Intersection: digit is ASCII intersect digit. */
    {
        uint32_t n = suts_susn_intersection(ascii, digit, out, 16u);

        CHECK(n == 1u);
        CHECK(out[0].first == 0x00000030u && out[0].last == 0x00000039u);
    }

    /* Difference: ASCII minus digit. */
    {
        uint32_t n = suts_susn_difference(ascii, digit, out, 16u);

        CHECK(n == 2u);
        CHECK(out[0].first == 0x00000000u && out[0].last == 0x0000002Fu);
        CHECK(out[1].first == 0x0000003Au && out[1].last == 0x0000007Fu);
    }

    /* Symmetric difference: ASCII xor digit is ASCII minus digit. */
    {
        uint32_t n = suts_susn_symmetric_difference(ascii, digit, out, 16u);

        CHECK(n == 2u);
        CHECK(out[0].first == 0x00000000u && out[0].last == 0x0000002Fu);
        CHECK(out[1].first == 0x0000003Au && out[1].last == 0x0000007Fu);
    }

    /* Complement: the complement of a complement is the original. */
    {
        uint32_t n1 = suts_susn_complement(ascii, out, 16u);
        susn_set_descriptor_t comp;
        susn_codepoint_range_t back[8];
        uint32_t n2;

        CHECK(n1 == 1u);
        CHECK(out[0].first == 0x00000080u);
        CHECK(out[0].last == SUSN_UNIVERSE_MAX);
        CHECK(suts_susn_descriptor_init(&comp, 0u, SUSN_FLAG_RANGE_ARRAY,
                                        n1, out));
        CHECK(!suts_susn_equals(ascii, &comp));
        n2 = suts_susn_complement(&comp, back, 8u);
        CHECK(n2 == 1u);
        CHECK(back[0].first == 0x00000000u && back[0].last == 0x0000007Fu);
    }

    /* Complement of NULL is the universe. */
    {
        uint32_t n = suts_susn_complement(NULL, out, 16u);

        CHECK(n == 1u);
        CHECK(out[0].first == 0x00000000u);
        CHECK(out[0].last == SUSN_UNIVERSE_MAX);
    }

    /* Negated operands are resolved directly by the sweep. */
    {
        susn_set_descriptor_t not_ascii;
        susn_codepoint_range_t u[8];
        uint32_t n;

        CHECK(suts_susn_descriptor_init(&not_ascii, 0u,
                                        SUSN_FLAG_RANGE_ARRAY |
                                            SUSN_FLAG_NEGATED,
                                        ascii->range_count, ascii->ranges));

        /* not_ascii union ascii is the universe. */
        n = suts_susn_union(&not_ascii, ascii, u, 8u);
        CHECK(n == 1u);
        CHECK(u[0].first == 0x00000000u && u[0].last == SUSN_UNIVERSE_MAX);

        /* not_ascii intersect ascii is empty. */
        CHECK(suts_susn_intersection(&not_ascii, ascii, u, 8u) == 0u);

        /* ascii minus not_ascii is ascii (the operands are disjoint). */
        n = suts_susn_difference(ascii, &not_ascii, u, 8u);
        CHECK(n == 1u);
        CHECK(u[0].first == 0x00000000u);
        CHECK(u[0].last == 0x0000007Fu);

        /* not_ascii minus ascii is not_ascii. */
        n = suts_susn_difference(&not_ascii, ascii, u, 8u);
        CHECK(n == 1u);
        CHECK(u[0].first == 0x00000080u);
        CHECK(u[0].last == SUSN_UNIVERSE_MAX);
    }

    /* A NULL operand is the empty set. */
    {
        CHECK(suts_susn_union(ascii, NULL, out, 16u) == 1u);
        CHECK(suts_susn_intersection(ascii, NULL, out, 16u) == 0u);
        CHECK(suts_susn_difference(ascii, NULL, out, 16u) == 1u);
        CHECK(suts_susn_symmetric_difference(NULL, NULL, out, 16u) == 0u);
    }

    /* The snprintf-style capacity contract: needed is independent of capacity. */
    {
        uint32_t needed = suts_susn_difference(ascii, digit, NULL, 0u);
        susn_codepoint_range_t tiny[1];
        uint32_t written = suts_susn_difference(ascii, digit, tiny, 1u);

        CHECK(needed == 2u);
        CHECK(written == 2u);
        CHECK(tiny[0].first == 0x00000000u && tiny[0].last == 0x0000002Fu);
    }

    /* Relations. */
    CHECK(suts_susn_is_subset(digit, ascii));
    CHECK(!suts_susn_is_subset(ascii, digit));
    CHECK(suts_susn_is_subset(NULL, ascii));
    CHECK(suts_susn_is_subset(ascii, ascii));
    CHECK(suts_susn_is_disjoint(ascii, greek));
    CHECK(!suts_susn_is_disjoint(ascii, digit));
    CHECK(suts_susn_is_disjoint(NULL, ascii));
    CHECK(suts_susn_equals(ascii, ascii));
    CHECK(!suts_susn_equals(ascii, digit));

    /* A zone set and the Bridge carve-out. */
    CHECK(suts_susn_is_subset(rnur, bridge) == false);
    CHECK(suts_susn_is_disjoint(bridge, rnur));

    printf("[PASS] test_set_algebra\n");
}

/* ── range filtering and probe masks (§4.2) ───────────────────────── */

static void test_filter_and_probe(void)
{
    const susn_set_descriptor_t *native =
        suts_susn_catalog_find(SUSN_SETID_NATIVE);
    const susn_set_descriptor_t *rnur =
        suts_susn_catalog_find(SUSN_SETID_RNUR);
    const susn_set_descriptor_t *ascii =
        suts_susn_catalog_find(SUSN_SETID_ASCII);
    const susn_set_descriptor_t *digit =
        suts_susn_catalog_find(SUSN_SETID_DIGIT);
    susn_codepoint_range_t out[8];

    CHECK(native != NULL && rnur != NULL && ascii != NULL && digit != NULL);

    /* NATIVE filtered to the Bridge is empty. */
    CHECK(suts_susn_filter(native, SUSN_FILTER_BRIDGE, out, 8u) == 0u);

    /* BRIDGE filtered to RNUR is empty. */
    CHECK(suts_susn_filter(suts_susn_catalog_find(SUSN_SETID_BRIDGE),
                           SUSN_FILTER_RNUR, out, 8u) == 0u);

    /* RNUR filtered to RNUR is the neography carve-out. */
    {
        uint32_t n = suts_susn_filter(rnur, SUSN_FILTER_RNUR, out, 8u);

        CHECK(n == 1u);
        CHECK(out[0].first == SUSN_RNUR_MIN);
        CHECK(out[0].last == SUSN_RNUR_MAX);
    }

    /* Native filtered to RNUR equals the RNUR set. */
    {
        uint32_t n = suts_susn_filter(native, SUSN_FILTER_RNUR, out, 8u);

        CHECK(n == 1u);
        CHECK(out[0].first == SUSN_RNUR_MIN);
        CHECK(out[0].last == SUSN_RNUR_MAX);
    }

    /* Selecting NATIVE and RNUR together must not duplicate the block. */
    {
        uint32_t n = suts_susn_filter(
            native, SUSN_FILTER_NATIVE | SUSN_FILTER_RNUR, out, 8u);

        CHECK(n == 1u);
        CHECK(out[0].first == 0x00120000u);
        CHECK(out[0].last == 0x7FFFFFEFu);
    }

    /* A full inversion filtered to two adjacent zones yields one range. */
    {
        susn_set_descriptor_t universe;
        uint32_t n;

        CHECK(suts_susn_descriptor_init(&universe, 0u,
                                        SUSN_FLAG_RANGE_ARRAY |
                                            SUSN_FLAG_NEGATED,
                                        0u, NULL));
        n = suts_susn_filter(&universe,
                             SUSN_FILTER_TRAP | SUSN_FILTER_SENTINEL,
                             out, 8u);
        CHECK(n == 1u);
        CHECK(out[0].first == 0x7FFFFFF0u && out[0].last == 0x7FFFFFFFu);
    }

    /* An empty zone mask filters everything away. */
    CHECK(suts_susn_filter(ascii, 0u, out, 8u) == 0u);

    /* Probe masks. */
    CHECK(suts_susn_probe_mask(ascii, 0x0030u, 1u, 10u) == 0x3FFu);
    CHECK(suts_susn_probe_mask(digit, 0x002Fu, 1u, 4u) == 0xEu);
    CHECK(suts_susn_probe_mask(ascii, 0x007Fu, 1u, 4u) == 0x1u);
    CHECK(suts_susn_probe_mask(ascii, 0x0041u, 0u, 4u) == 0x1u);
    CHECK(suts_susn_probe_mask(digit, 0x0041u, 0u, 4u) == 0x0u);
    CHECK(suts_susn_probe_mask(ascii, 0x0030u, 2u, 5u) == 0x1Fu);
    CHECK(suts_susn_probe_mask(ascii, 0x0080u, 0x10000000u, 32u) == 0x0u);
    CHECK(suts_susn_probe_mask(ascii, 0x0030u, 1u, 0u) == 0u);
    CHECK(suts_susn_probe_mask(NULL, 0x0030u, 1u, 8u) == 0u);

    printf("[PASS] test_filter_and_probe\n");
}

/* ── the fixed-capacity builder (§3.4) ────────────────────────────── */

static void test_builder(void)
{
    susn_builder_t builder;
    susn_set_descriptor_t set;

    /* Order-independent insertion with merging. */
    {
        suts_susn_builder_init(&builder, SUSN_FLAG_COMPILED);
        CHECK(builder.count == 0u);
        CHECK(suts_susn_builder_add_range(&builder, 0x0010u, 0x0020u));
        CHECK(suts_susn_builder_add_range(&builder, 0x0021u, 0x0030u));
        CHECK(builder.count == 1u);
        CHECK(builder.ranges[0].first == 0x0010u);
        CHECK(builder.ranges[0].last == 0x0030u);

        CHECK(suts_susn_builder_add_range(&builder, 0x0000u, 0x0005u));
        CHECK(builder.count == 2u);
        CHECK(suts_susn_builder_add_range(&builder, 0x0004u, 0x0012u));
        CHECK(builder.count == 1u);
        CHECK(builder.ranges[0].first == 0x0000u);
        CHECK(builder.ranges[0].last == 0x0030u);

        CHECK(suts_susn_builder_add_codepoint(&builder, 0x0040u));
        CHECK(suts_susn_builder_add_codepoint(&builder, 0x0031u));
        CHECK(builder.count == 2u);
        CHECK(builder.ranges[0].first == 0x0000u);
        CHECK(builder.ranges[0].last == 0x0031u);
        CHECK(builder.ranges[1].first == 0x0040u);
        CHECK(builder.ranges[1].last == 0x0040u);

        CHECK(suts_susn_builder_finish(&builder, 7u, &set));
        CHECK(set.set_id == 7u);
        CHECK(set.set_flags ==
              (SUSN_FLAG_COMPILED | SUSN_FLAG_RANGE_ARRAY));
        CHECK(set.range_count == 2u);
        CHECK(set.ranges == builder.ranges);
        CHECK(suts_susn_validate(&set));
        CHECK(suts_susn_contains(&set, 0x0000u));
        CHECK(suts_susn_contains(&set, 0x0030u));
        CHECK(suts_susn_contains(&set, 0x0031u));
        CHECK(!suts_susn_contains(&set, 0x003Fu));
        CHECK(suts_susn_contains(&set, 0x0040u));
    }

    /* Adding a negated set adds its complement. */
    {
        susn_set_descriptor_t not_ascii;

        suts_susn_builder_init(&builder, 0u);
        CHECK(suts_susn_descriptor_init(&not_ascii, 0u,
                                        SUSN_FLAG_RANGE_ARRAY |
                                            SUSN_FLAG_NEGATED,
                                        1u,
                                        suts_susn_catalog_find(
                                            SUSN_SETID_ASCII)->ranges));
        CHECK(suts_susn_builder_add_set(&builder, &not_ascii));
        CHECK(builder.count == 1u);
        CHECK(builder.ranges[0].first == 0x00000080u);
        CHECK(builder.ranges[0].last == SUSN_UNIVERSE_MAX);
        CHECK(suts_susn_builder_finish(&builder, 0u, &set));
        CHECK(suts_susn_contains(&set, 0x0080u));
        CHECK(!suts_susn_contains(&set, 0x0041u));
    }

    /* Adding a positive set. */
    {
        suts_susn_builder_init(&builder, 0u);
        CHECK(suts_susn_builder_add_set(
            &builder, suts_susn_catalog_find(SUSN_SETID_ASCII)));
        CHECK(builder.count == 1u);
        CHECK(builder.ranges[0].first == 0x00000000u);
        CHECK(builder.ranges[0].last == 0x0000007Fu);
    }

    /* The capacity is a hard ceiling. */
    {
        uint32_t i;

        suts_susn_builder_init(&builder, 0u);
        for (i = 0u; i < SUSN_BUILDER_CAPACITY; i++) {
            CHECK(suts_susn_builder_add_range(&builder, i * 2u, i * 2u));
        }
        CHECK(builder.count == SUSN_BUILDER_CAPACITY);
        CHECK(!suts_susn_builder_add_range(&builder,
                                           SUSN_BUILDER_CAPACITY * 2u,
                                           SUSN_BUILDER_CAPACITY * 2u));
    }

    /* Guards. */
    CHECK(!suts_susn_builder_add_range(NULL, 0u, 0u));
    CHECK(!suts_susn_builder_add_codepoint(NULL, 0u));
    CHECK(!suts_susn_builder_add_set(NULL, NULL));
    CHECK(!suts_susn_builder_finish(NULL, 0u, &set));
    CHECK(!suts_susn_builder_finish(&builder, 0u, NULL));
    suts_susn_builder_init(NULL, 0u);

    printf("[PASS] test_builder\n");
}

/* ── RNUR neography integration (§5) ──────────────────────────────── */

static void test_neography(void)
{
    const susn_set_descriptor_t *rnur =
        suts_susn_catalog_find(SUSN_SETID_RNUR);
    susn_codepoint_range_t storage;
    susn_set_descriptor_t set;
    uint32_t value = 999u;
    uint32_t first = 0u;
    uint32_t last = 0u;

    CHECK(suts_susn_is_neography(SUSN_RNUR_MIN));
    CHECK(suts_susn_is_neography(SUSN_RNUR_MAX));
    CHECK(!suts_susn_is_neography(SUSN_RNUR_MIN - 1u));
    CHECK(!suts_susn_is_neography(SUSN_RNUR_MAX + 1u));

    CHECK(suts_susn_neography_set_of(0x00160000u, &value));
    CHECK(value == 0u);
    CHECK(suts_susn_neography_set_of(0x001600FFu, &value));
    CHECK(value == 0u);
    CHECK(suts_susn_neography_set_of(0x00160100u, &value));
    CHECK(value == 1u);
    CHECK(suts_susn_neography_set_of(0x0016FFFFu, &value));
    CHECK(value == 255u);
    CHECK(!suts_susn_neography_set_of(0x0015FFFFu, &value));
    CHECK(!suts_susn_neography_set_of(0x00170000u, &value));
    CHECK(!suts_susn_neography_set_of(0x00160000u, NULL));

    CHECK(suts_susn_neography_bounds(0u, &first, &last));
    CHECK(first == 0x00160000u && last == 0x001600FFu);
    CHECK(suts_susn_neography_bounds(255u, &first, &last));
    CHECK(first == 0x0016FF00u && last == 0x0016FFFFu);
    CHECK(!suts_susn_neography_bounds(256u, &first, &last));
    CHECK(suts_susn_neography_bounds(1u, NULL, NULL));

    CHECK(suts_susn_neography_contains(1u, 0x00160100u));
    CHECK(!suts_susn_neography_contains(1u, 0x001600FFu));
    CHECK(!suts_susn_neography_contains(256u, 0x00160000u));

    CHECK(suts_susn_neography_set(1u, &storage, &set));
    CHECK(set.set_id ==
          (uint32_t)SUSN_SETID_NEOSCRIPT_BASE + 1u);
    CHECK(set.set_flags ==
          (SUSN_FLAG_RANGE_ARRAY | SUSN_FLAG_NEOSCRIPT));
    CHECK(set.range_count == 1u);
    CHECK(set.ranges == &storage);
    CHECK(storage.first == 0x00160100u);
    CHECK(storage.last == 0x001601FFu);
    CHECK(suts_susn_validate(&set));
    CHECK(suts_susn_contains(&set, 0x00160100u));
    CHECK(!suts_susn_contains(&set, 0x001600FFu));
    CHECK(strcmp(suts_susn_set_name(set.set_id), "Neography Block") == 0);

    /* A neography block is a subset of the whole RNUR carve-out. */
    CHECK(suts_susn_is_subset(&set, rnur));

    /* Two adjacent blocks are disjoint and their union is contiguous. */
    {
        susn_codepoint_range_t storage2;
        susn_set_descriptor_t set2;
        susn_codepoint_range_t out[2];

        CHECK(suts_susn_neography_set(2u, &storage2, &set2));
        CHECK(suts_susn_is_disjoint(&set, &set2));
        CHECK(suts_susn_union(&set, &set2, out, 2u) == 1u);
        CHECK(out[0].first == 0x00160100u);
        CHECK(out[0].last == 0x001602FFu);
    }

    /* Guards. */
    CHECK(!suts_susn_neography_set(1u, NULL, &set));
    CHECK(!suts_susn_neography_set(1u, &storage, NULL));
    CHECK(!suts_susn_neography_set(256u, &storage, &set));

    printf("[PASS] test_neography\n");
}

/* ── zones and taxonomy ───────────────────────────────────────────── */

static void test_zones_and_names(void)
{
    CHECK(suts_susn_zone_of(0x00000000u) == SUSN_ZONE_BRIDGE);
    CHECK(suts_susn_zone_of(0x0010FFFFu) == SUSN_ZONE_BRIDGE);
    CHECK(suts_susn_zone_of(0x00110000u) == SUSN_ZONE_SCP);
    CHECK(suts_susn_zone_of(0x0011FFFFu) == SUSN_ZONE_SCP);
    CHECK(suts_susn_zone_of(0x00120000u) == SUSN_ZONE_NATIVE);
    CHECK(suts_susn_zone_of(0x0015FFFFu) == SUSN_ZONE_NATIVE);
    CHECK(suts_susn_zone_of(0x00160000u) == SUSN_ZONE_RNUR);
    CHECK(suts_susn_zone_of(0x0016FFFFu) == SUSN_ZONE_RNUR);
    CHECK(suts_susn_zone_of(0x00170000u) == SUSN_ZONE_NATIVE);
    CHECK(suts_susn_zone_of(0x7FFFFFEFu) == SUSN_ZONE_NATIVE);
    CHECK(suts_susn_zone_of(0x7FFFFFF0u) == SUSN_ZONE_TRAP);
    CHECK(suts_susn_zone_of(0x7FFFFFFEu) == SUSN_ZONE_TRAP);
    CHECK(suts_susn_zone_of(0x7FFFFFFFu) == SUSN_ZONE_SENTINEL);
    CHECK(suts_susn_zone_of(0x80000000u) == SUSN_ZONE_EXTSUCS);
    CHECK(suts_susn_zone_of(0xFFFFFFFFu) == SUSN_ZONE_EXTSUCS);

    CHECK(suts_susn_zone_filter(SUSN_ZONE_BRIDGE) == SUSN_FILTER_BRIDGE);
    CHECK(suts_susn_zone_filter(SUSN_ZONE_SCP) == SUSN_FILTER_SCP);
    CHECK(suts_susn_zone_filter(SUSN_ZONE_RNUR) == SUSN_FILTER_RNUR);
    CHECK(suts_susn_zone_filter(SUSN_ZONE_NATIVE) == SUSN_FILTER_NATIVE);
    CHECK(suts_susn_zone_filter(SUSN_ZONE_TRAP) == SUSN_FILTER_TRAP);
    CHECK(suts_susn_zone_filter(SUSN_ZONE_SENTINEL) ==
          SUSN_FILTER_SENTINEL);
    CHECK(suts_susn_zone_filter(SUSN_ZONE_EXTSUCS) == 0u);

    CHECK(strcmp(suts_susn_zone_name(SUSN_ZONE_BRIDGE), "Bridge") == 0);
    CHECK(strcmp(suts_susn_zone_name(SUSN_ZONE_RNUR), "RNUR") == 0);
    CHECK(strcmp(suts_susn_zone_name((susn_zone_t)99), "Unknown") == 0);

    CHECK(strcmp(suts_susn_op_name(SUSN_OP_UNION), "Union") == 0);
    CHECK(strcmp(suts_susn_op_name(SUSN_OP_INTERSECTION),
                 "Intersection") == 0);
    CHECK(strcmp(suts_susn_op_name(SUSN_OP_DIFFERENCE), "Difference") == 0);
    CHECK(strcmp(suts_susn_op_name(SUSN_OP_SYMMETRIC_DIFFERENCE),
                 "Symmetric Difference") == 0);
    CHECK(strcmp(suts_susn_op_name((susn_set_op_t)99), "Unknown") == 0);

    printf("[PASS] test_zones_and_names\n");
}

/* ── guards ───────────────────────────────────────────────────────── */

static void test_guards(void)
{
    susn_codepoint_range_t out[2];
    susn_set_descriptor_t set;

    CHECK(!suts_susn_contains(NULL, 0x0000u));
    CHECK(!suts_susn_contains_in(NULL, 0x0000u, SUSN_FILTER_ALL));
    CHECK(suts_susn_cardinality(NULL) == 0u);

    CHECK(suts_susn_combine((susn_set_op_t)99, NULL, NULL, out, 2u) == 0u);
    CHECK(suts_susn_combine(SUSN_OP_UNION, NULL, NULL, NULL, 1u) == 0u);
    CHECK(suts_susn_complement(NULL, NULL, 1u) == 0u);
    CHECK(suts_susn_filter(NULL, SUSN_FILTER_ALL, NULL, 1u) == 0u);

    /* A descriptor whose count claims ranges it does not have is refused. */
    {
        susn_set_descriptor_t broken;

        broken.set_id = 0u;
        broken.set_flags = SUSN_FLAG_RANGE_ARRAY;
        broken.range_count = 4u;
        broken.ranges = NULL;

        CHECK(!suts_susn_contains(&broken, 0x0000u));
        CHECK(suts_susn_cardinality(&broken) == 0u);
        CHECK(!suts_susn_validate(&broken));
        CHECK(suts_susn_is_empty(&broken));
        CHECK(suts_susn_combine(SUSN_OP_UNION, &broken, NULL, out, 2u) == 0u);
    }

    CHECK(suts_susn_catalog_at(0xFFFFFFFFu) == NULL);
    CHECK(suts_susn_catalog_find(0xDEADBEEFu) == NULL);
    CHECK(suts_susn_catalog_find_name(NULL) == NULL);
    CHECK(suts_susn_catalog_find_name("No Such Set") == NULL);

    CHECK(suts_susn_descriptor_init(&set, 0u, 0u, 0u, NULL));

    printf("[PASS] test_guards\n");
}

/* ── SSRE registration identifiers ────────────────────────────────── */

static void test_ssre_registration(void)
{
    CHECK(SUSN_SPEC_ID[0] == 'S');
    CHECK(SUSN_SPEC_ID[5] == '0');
    CHECK(SUSN_SPEC_ID[6] == '1');
    CHECK(SUSN_SPEC_ID[7] == '1');
    CHECK(SUSN_SPEC_TITLE[0] == 'S');
    CHECK(SUSN_MODULE[0] == 's');

    printf("[PASS] test_ssre_registration\n");
}

int main(void)
{
    test_version_and_layout();
    test_catalog();
    test_membership_negation();
    test_validation_and_normalize();
    test_set_algebra();
    test_filter_and_probe();
    test_builder();
    test_neography();
    test_zones_and_names();
    test_guards();
    test_ssre_registration();
    printf("[PASS] SUTS-11 SUSN tests passed\n");
    return 0;
}
