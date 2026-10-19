/**
 * SUAS-009 SIP — System Ideographic Properties
 *
 * Unit tests for the zero-allocation, static-table, O(1) extraction of
 * ideographic character attributes: radical, stroke count, classification
 * flags, collation seed, and variant linkage (a UAX #38 analog).
 *
 * Covers: version/meta, the O(1) core query, radical-stroke extraction, the
 * SUCA sort-weight seed (stored and algorithmic fallback), the variant linkage
 * into the Native & Neography plane, classification flags, the ideographic
 * range dispatch, plane classification, the lazy / idempotent build, miss and
 * guard behaviour, enumeration and table self-consistency, and the hash.
 */

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <assert.h>

#include "suas/suas_sip.h"

static int g_fail = 0;
#define CHECK(cond) do { if (!(cond)) { \
    printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); g_fail = 1; \
} } while (0)

static void test_version_and_meta(void)
{
    CHECK(suas_sip_version_string() != NULL);
    CHECK(suas_sip_version_string()[0] == '1');
    CHECK(suas_sip_version_string()[2] == '0');
    CHECK(SUAS_SIP_SPEC_ID[0] == 'S');
    CHECK(SUAS_SIP_SPEC_TITLE[0] == 'S');
    CHECK(SUAS_SIP_REGISTRY_CAPACITY == 128U);
    CHECK(SUAS_SIP_INDEX_SIZE == 256U);

    /* The classification flags are single, disjoint bits (§2.2). */
    CHECK((SUAS_SIP_FLAG_UNIFIED & SUAS_SIP_FLAG_RADICAL) == 0u);
    CHECK((SUAS_SIP_FLAG_VARIANT & SUAS_SIP_FLAG_OBSOLETE) == 0u);
    CHECK((SUAS_SIP_FLAG_NUMERIC & SUAS_SIP_FLAG_NATIVE) == 0u);
    CHECK((SUAS_SIP_FLAG_COMPAT & SUAS_SIP_FLAG_TRADITIONAL) == 0u);

    /* The record is a fixed-size, byte-aligned packed struct (§3.1). */
    CHECK(sizeof(suas_glyph_prop_t) == 24U);
    CHECK(offsetof(suas_glyph_prop_t, codepoint) == 0U);
}

/* Exercises the lazy build: no explicit init before the first query. */
static void test_get_properties(void)
{
    suas_glyph_prop_t p;

    CHECK(suas_sip_get_properties(0x4E00ULL, &p));
    CHECK(p.codepoint == 0x4E00ULL);
    CHECK(p.radical == 1u);
    CHECK(p.strokes == 1u);
    CHECK((p.flags & SUAS_SIP_FLAG_UNIFIED) != 0u);
    CHECK((p.flags & SUAS_SIP_FLAG_RADICAL) != 0u);
    CHECK((p.flags & SUAS_SIP_FLAG_NUMERIC) == 0u);
    CHECK(p.sort_weight == 0x4E00u);
    CHECK(p.native_variant == 0ULL);

    /* A numeric ideograph carries the NUMERIC flag. */
    CHECK(suas_sip_get_properties(0x56DBULL, &p)); /* 四 */
    CHECK((p.flags & SUAS_SIP_FLAG_NUMERIC) != 0u);
    CHECK(p.radical == 31u && p.strokes == 5u);

    /* A high-stroke traditional form. */
    CHECK(suas_sip_get_properties(0x9F8DULL, &p)); /* 龍 */
    CHECK(p.radical == 212u && p.strokes == 16u);
    CHECK((p.flags & SUAS_SIP_FLAG_TRADITIONAL) != 0u);

    /* An RNUR neography logograph. */
    CHECK(suas_sip_get_properties(0x160100ULL, &p));
    CHECK((p.flags & SUAS_SIP_FLAG_NATIVE) != 0u);
}

static void test_init_idempotent(void)
{
    suas_glyph_prop_t p;

    suas_sip_init_engine();
    suas_sip_init_engine(); /* idempotent */
    CHECK(suas_sip_get_properties(0x5C71ULL, &p)); /* 山 */
    CHECK(p.radical == 46u && p.strokes == 3u);
}

static void test_radical_stroke(void)
{
    uint16_t radical = 0;
    uint8_t  strokes = 0;

    CHECK(suas_sip_get_radical_stroke(0x5728ULL, &radical, &strokes) == false);
    CHECK(suas_sip_get_radical_stroke(0x6728ULL, &radical, &strokes)); /* 木 */
    CHECK(radical == 75u && strokes == 4u);

    /* Either output may be NULL. */
    CHECK(suas_sip_get_radical_stroke(0x6728ULL, NULL, &strokes));
    CHECK(strokes == 4u);
    CHECK(suas_sip_get_radical_stroke(0x6728ULL, &radical, NULL));
    CHECK(radical == 75u);
    CHECK(suas_sip_get_radical_stroke(0x6728ULL, NULL, NULL));
    CHECK(!suas_sip_get_radical_stroke(0x0041ULL, &radical, &strokes));
}

static void test_sort_weight(void)
{
    uint32_t w = 0;

    /* Stored weight, equal to native order for a plain unified ideograph. */
    CHECK(suas_sip_get_sort_weight(0x4EBAULL, &w)); /* 人 */
    CHECK(w == 0x4EBAu);

    /* Compatibility ideographs fold onto their unified counterpart (§5.3). */
    CHECK(suas_sip_get_sort_weight(0xF900ULL, &w)); /* 豈 -> U+8C48 */
    CHECK(w == 0x00008C48u);
    CHECK(suas_sip_get_sort_weight(0x2F800ULL, &w)); /* 丽 -> U+4E3D */
    CHECK(w == 0x00004E3Du);

    /* An unregistered but valid ideograph gets the algorithmic weight. */
    CHECK(suas_sip_get_sort_weight(0x4E01ULL, &w));
    CHECK(w == 0x4E01u);

    /* A non-ideograph has no sort weight. */
    CHECK(!suas_sip_get_sort_weight(0x0041ULL, &w));
    CHECK(!suas_sip_get_sort_weight(0x4E00ULL, NULL));
}

static void test_variant_linkage(void)
{
    sucs_ex_char_t target = 0;

    CHECK(suas_sip_has_variant(0x9F8DULL)); /* 龍 */
    CHECK(suas_sip_variant_target(0x9F8DULL, &target));
    CHECK(target == 0x00160200ULL);
    CHECK(suas_sip_variant_target(0x9F99ULL, &target)); /* 龙 */
    CHECK(target == 0x00160201ULL);

    /* A variant with a further variant target (RNUR chain). */
    CHECK(suas_sip_variant_target(0x160101ULL, &target));
    CHECK(target == 0x00160102ULL);

    /* No variant: a plain ideograph. */
    CHECK(!suas_sip_has_variant(0x4E00ULL));
    CHECK(!suas_sip_variant_target(0x4E00ULL, &target));

    /* A VARIANT-flagged compatibility form may carry no native target. */
    CHECK(suas_sip_has_variant(0xF900ULL));
    CHECK(!suas_sip_variant_target(0xF900ULL, &target));

    CHECK(!suas_sip_variant_target(0x9F8DULL, NULL));
}

static void test_flags(void)
{
    CHECK((suas_sip_flags(0x5341ULL) & SUAS_SIP_FLAG_NUMERIC) != 0u); /* 十 */
    CHECK((suas_sip_flags(0xF900ULL) & SUAS_SIP_FLAG_COMPAT) != 0u);
    CHECK((suas_sip_flags(0x20000ULL) & SUAS_SIP_FLAG_UNIFIED) != 0u);
    CHECK(suas_sip_flags(0x0041ULL) == 0u);       /* unregistered */
    CHECK(suas_sip_flags(0x00110000ULL) == 0u);   /* SCP marker */
}

static void test_ideograph_dispatch(void)
{
    /* Registered ideographs. */
    CHECK(suas_sip_is_ideograph(0x4E00ULL));
    CHECK(suas_sip_is_ideograph(0x160100ULL));

    /* Unregistered but inside a defined ideographic block. */
    CHECK(suas_sip_is_ideograph(0x3400ULL));      /* Extension A */
    CHECK(suas_sip_is_ideograph(0x4E01ULL));      /* URO         */
    CHECK(suas_sip_is_ideograph(0xFA0FULL));      /* Compatibility */
    CHECK(suas_sip_is_ideograph(0x2000BULL));     /* Extension B */
    CHECK(suas_sip_is_ideograph(0x2A700ULL));     /* Extension C */
    CHECK(suas_sip_is_ideograph(0x2F801ULL));     /* Compat. Supplement */
    CHECK(suas_sip_is_ideograph(0x30000ULL));     /* Extension G */
    CHECK(suas_sip_is_ideograph(0x160500ULL));    /* RNUR logograph */

    /* Non-ideographs. */
    CHECK(!suas_sip_is_ideograph(0x0041ULL));     /* Latin       */
    CHECK(!suas_sip_is_ideograph(0x3042ULL));     /* Hiragana    */
    CHECK(!suas_sip_is_ideograph(0x00110000ULL)); /* SCP marker  */
    CHECK(!suas_sip_is_ideograph(0x7FFFFFF5ULL)); /* trap slot   */
    CHECK(!suas_sip_is_ideograph(0x7FFFFFFFULL)); /* Sentinel    */
}

static void test_plane_of(void)
{
    CHECK(suas_sip_plane_of(0x0041ULL) == SUAS_SIP_PLANE_BRIDGE);
    CHECK(suas_sip_plane_of(0x0010FFFFULL) == SUAS_SIP_PLANE_BRIDGE);
    CHECK(suas_sip_plane_of(0x00110000ULL) == SUAS_SIP_PLANE_SCP);
    CHECK(suas_sip_plane_of(0x0011FFFFULL) == SUAS_SIP_PLANE_SCP);
    CHECK(suas_sip_plane_of(0x00120000ULL) == SUAS_SIP_PLANE_NATIVE);
    CHECK(suas_sip_plane_of(0x00160000ULL) == SUAS_SIP_PLANE_RNUR);
    CHECK(suas_sip_plane_of(0x0016FFFFULL) == SUAS_SIP_PLANE_RNUR);
    CHECK(suas_sip_plane_of(0x7FFFFFEFULL) == SUAS_SIP_PLANE_NATIVE);
    CHECK(suas_sip_plane_of(0x7FFFFFF0ULL) == SUAS_SIP_PLANE_TRAP);
    CHECK(suas_sip_plane_of(0x7FFFFFFEULL) == SUAS_SIP_PLANE_TRAP);
    CHECK(suas_sip_plane_of(0x7FFFFFFFULL) == SUAS_SIP_PLANE_SENTINEL);
    CHECK(suas_sip_plane_of(0x80000000ULL) == SUAS_SIP_PLANE_EXTSUCS);
}

static void test_get_miss_and_guards(void)
{
    suas_glyph_prop_t p;

    CHECK(!suas_sip_get_properties(0x4E01ULL, &p));       /* unregistered */
    CHECK(!suas_sip_get_properties(0x0041ULL, &p));       /* Latin        */
    CHECK(!suas_sip_get_properties(0x00110000ULL, &p));   /* SCP marker   */
    CHECK(!suas_sip_get_properties(0x7FFFFFF5ULL, &p));   /* trap slot    */
    CHECK(!suas_sip_get_properties(0x7FFFFFFFULL, &p));   /* Sentinel     */
    CHECK(!suas_sip_get_properties(0x4E00ULL, NULL));
    CHECK(!suas_sip_is_registered(0x4E01ULL));
    CHECK(suas_sip_is_registered(0x4E00ULL));
}

static void test_enumeration(void)
{
    suas_glyph_prop_t p;
    uint32_t n = suas_sip_entry_count();
    uint32_t i;
    uint32_t natives = 0, compat = 0;

    CHECK(n >= 20u);
    CHECK(n <= SUAS_SIP_REGISTRY_CAPACITY);

    for (i = 0u; i < n; i++) {
        CHECK(suas_sip_entry_at(i, &p));
        CHECK(suas_sip_is_registered(p.codepoint));
        CHECK(suas_sip_is_ideograph(p.codepoint));
        /* SCP invariance: no record names the SCP, trap slots or Sentinel. */
        CHECK(!(p.codepoint >= SUAS_SIP_ZONE_SCP_MIN &&
                p.codepoint <= SUAS_SIP_ZONE_SCP_MAX));
        CHECK(!(p.codepoint >= SUAS_SIP_ZONE_TRAP_MIN &&
                p.codepoint <= SUAS_SIP_ZONE_SENTINEL));
        /* A native record lives in the RNUR carve-out. */
        if ((p.flags & SUAS_SIP_FLAG_NATIVE) != 0u)
            CHECK(suas_sip_plane_of(p.codepoint) == SUAS_SIP_PLANE_RNUR);
        /* A declared native variant resolves back through the index. */
        if (p.native_variant != 0ULL) {
            CHECK(suas_sip_has_variant(p.codepoint));
        }
        if ((p.flags & SUAS_SIP_FLAG_NATIVE) != 0u)
            natives++;
        if ((p.flags & SUAS_SIP_FLAG_COMPAT) != 0u)
            compat++;
    }
    CHECK(natives > 0u && compat > 0u);
    CHECK(!suas_sip_entry_at(n, &p));
    CHECK(!suas_sip_entry_at(0u, NULL));
}

static void test_hash(void)
{
    CHECK(suas_sip_hash(0x4E00ULL) == suas_sip_hash(0x4E00ULL));
    CHECK(suas_sip_hash(0x4E00ULL) != suas_sip_hash(0x4E8CULL));
    CHECK(suas_sip_hash(0x00160100ULL) != suas_sip_hash(0x4E00ULL));
    CHECK(suas_sip_hash(0ULL) != 0u);              /* not the identity */
    CHECK(suas_sip_hash(0ULL) == suas_sip_hash(0ULL));
}

int main(void)
{
    test_version_and_meta();
    test_get_properties();       /* lazy build path */
    test_init_idempotent();
    test_radical_stroke();
    test_sort_weight();
    test_variant_linkage();
    test_flags();
    test_ideograph_dispatch();
    test_plane_of();
    test_get_miss_and_guards();
    test_enumeration();
    test_hash();

    if (g_fail) {
        printf("SUAS-009 SIP tests FAILED\n");
        return 1;
    }
    printf("SUAS-009 SIP tests passed\n");
    return 0;
}
