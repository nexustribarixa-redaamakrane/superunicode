/**
 * SUAS-017 SUEASD - SuperUnicode East Asian Script Data
 *
 * Unit tests for the zero-allocation, static-table, O(1) classification of
 * East Asian scripts and the ancillary data UAX #60 publishes for the scripts
 * other than Han - the primary script, the regional source class, the
 * ideographic/radical/compatibility/kana/jamo/syllable roles, the UAX #60
 * source mask, the radical-stroke counts, the readings, the numeric values
 * and the Small Seal modern-CJK equivalence.
 *
 * Covers: version/meta, the compile-time 12-byte record geometry, zone
 * dispatch, the RNUR carve-out, FNV-1a, the classification ranges, every
 * script family, the ancillary metadata, the algorithmic default,
 * enumeration and table self-consistency, every verification outcome, the
 * taxonomy names, and registration in SUAS-010 (SSRE).
 */

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <assert.h>

#include "suas/suas_sueasd.h"
#include "suas/suas_ssre.h"

static int g_fail = 0;
#define CHECK(cond) do { if (!(cond)) { \
    printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); g_fail = 1; \
} } while (0)

/* -- Version, limits and record geometry -------------------------------- */

static void test_version_and_meta(void)
{
    CHECK(suas_sueasd_version_string() != NULL);
    CHECK(suas_sueasd_version_string()[0] == '1');
    CHECK(suas_sueasd_version_string()[2] == '0');
    CHECK(SUEASD_SPEC_ID[0] == 'S');
    CHECK(SUEASD_SPEC_TITLE[0] == 'S');
    CHECK(SUAS_SUEASD_SPEC_ID[0] == 'S');
    CHECK(SUEASD_VERSION_MAJOR == 1);
    CHECK(SUEASD_VERSION_MINOR == 0);
    CHECK(SUEASD_VERSION_PATCH == 0);
    CHECK(SUAS_SUEASD_REGISTRY_CAPACITY == 32768U);
    CHECK(SUAS_SUEASD_INDEX_SIZE == 65536U);
    CHECK((SUAS_SUEASD_INDEX_SIZE & (SUAS_SUEASD_INDEX_SIZE - 1U)) == 0U);
    CHECK(SUAS_SUEASD_INDEX_SIZE > SUAS_SUEASD_REGISTRY_CAPACITY);
    CHECK(SUAS_SUEASD_RANGE_CAPACITY == 256U);

    /* The seeding sketch's exact field order, 12 bytes. */
    CHECK(sizeof(sueasd_script_entry_t) == 12U);
    CHECK(offsetof(sueasd_script_entry_t, codepoint) == 0U);
    CHECK(offsetof(sueasd_script_entry_t, script_mask) == 4U);
    CHECK(offsetof(sueasd_script_entry_t, primary_script) == 8U);
    CHECK(offsetof(sueasd_script_entry_t, source_region) == 10U);
    CHECK(offsetof(sueasd_script_entry_t, reserved) == 11U);
    CHECK(sizeof(sueasd_metadata_t) == 24U);
    CHECK(sizeof(sueasd_range_t) == 12U);

    /* The four sketch flags, with the two typo'd macros repaired. */
    CHECK(SUEASD_FLAG_IDEOGRAPHIC == (1U << 0));
    CHECK(SUEASD_FLAG_RADICAL == (1U << 1));
    CHECK(SUEASD_FLAG_COMPATIBILITY == (1U << 2));
    CHECK(SUEASD_FLAG_CUSTOM_SCRIPT == (1U << 3));
    CHECK((SUEASD_FLAG_IDEOGRAPHIC & SUEASD_FLAG_RADICAL) == 0U);
    CHECK((SUEASD_FLAG_COMPATIBILITY & SUEASD_FLAG_CUSTOM_SCRIPT) == 0U);
    CHECK(SUEASD_FLAG_ASSIGNED_MASK == 0xFFFFU);

    CHECK(SUEASD_SCRIPT_UNKNOWN == 0U);
    CHECK(SUEASD_SCRIPT_HAN == 1U);
    CHECK(SUEASD_SCRIPT_HANGUL == 2U);
    CHECK(SUEASD_SCRIPT_HIRAGANA == 3U);
    CHECK(SUEASD_SCRIPT_KATAKANA == 4U);
    CHECK(SUEASD_SCRIPT_BOPOMOFO == 5U);
    CHECK(SUEASD_SCRIPT_JURCHEN == 6U);
    CHECK(SUEASD_SCRIPT_NUSHU == 7U);
    CHECK(SUEASD_SCRIPT_SEAL == 8U);
    CHECK(SUEASD_SCRIPT_TANGUT == 9U);
    CHECK(SUEASD_SCRIPT_KHITAN_SMALL == 10U);
    CHECK(SUEASD_SCRIPT_COUNT == 11U);

    CHECK(SUEASD_REGION_CHINA == 1U);
    CHECK(SUEASD_REGION_KOREA == 2U);
    CHECK(SUEASD_REGION_JAPAN == 3U);
    CHECK(SUEASD_REGION_JURCHEN == 4U);
    CHECK(SUEASD_REGION_NUSHU == 5U);
    CHECK(SUEASD_REGION_SEAL == 6U);
    CHECK(SUEASD_REGION_TANGUT == 7U);
    CHECK(SUEASD_REGION_KHITAN == 8U);

    CHECK(SUAS_SUEASD_RNUR_MIN == 0x00160000UL);
    CHECK(SUAS_SUEASD_RNUR_MAX == 0x0016FFFFUL);
    CHECK(SUAS_SUEASD_RNUR_SET_SHIFT == 8U);
}

/* -- Zone dispatch and the RNUR carve-out ------------------------------- */

static void test_zones(void)
{
    CHECK(suas_sueasd_zone_of(0x00004E00UL) == SUAS_SUEASD_ZONE_BRIDGE);
    CHECK(suas_sueasd_zone_of(0x0010FFFFUL) == SUAS_SUEASD_ZONE_BRIDGE);
    CHECK(suas_sueasd_zone_of(0x00110000UL) == SUAS_SUEASD_ZONE_SCP);
    CHECK(suas_sueasd_zone_of(0x0011FFFFUL) == SUAS_SUEASD_ZONE_SCP);
    CHECK(suas_sueasd_zone_of(0x00120000UL) == SUAS_SUEASD_ZONE_NATIVE);
    CHECK(suas_sueasd_zone_of(0x00160000UL) == SUAS_SUEASD_ZONE_RNUR);
    CHECK(suas_sueasd_zone_of(0x0016FFFFUL) == SUAS_SUEASD_ZONE_RNUR);
    CHECK(suas_sueasd_zone_of(0x00170000UL) == SUAS_SUEASD_ZONE_NATIVE);
    CHECK(suas_sueasd_zone_of(0x7FFFFFF0UL) == SUAS_SUEASD_ZONE_TRAP);
    CHECK(suas_sueasd_zone_of(0x7FFFFFFEUL) == SUAS_SUEASD_ZONE_TRAP);
    CHECK(suas_sueasd_zone_of(0x7FFFFFFFUL) == SUAS_SUEASD_ZONE_SENTINEL);
    CHECK(suas_sueasd_zone_of(0x80000000UL) == SUAS_SUEASD_ZONE_EXTSUCS);

    CHECK(suas_sueasd_is_text_space(0x00004E00UL));
    CHECK(suas_sueasd_is_text_space(0x00120000UL));
    CHECK(suas_sueasd_is_text_space(0x00160000UL));
    CHECK(!suas_sueasd_is_text_space(0x00110000UL));
    CHECK(!suas_sueasd_is_text_space(0x7FFFFFF0UL));
    CHECK(!suas_sueasd_is_text_space(0x7FFFFFFFUL));
    CHECK(!suas_sueasd_is_text_space(0x80000000UL));

    CHECK(suas_sueasd_is_neography(0x00160000UL));
    CHECK(suas_sueasd_is_neography(0x0016FFFFUL));
    CHECK(!suas_sueasd_is_neography(0x0012FFFFUL));
    CHECK(!suas_sueasd_is_neography(0x00004E00UL));

    {
        uint32_t set = 0xFFFFFFFFu;
        CHECK(suas_sueasd_neography_set(0x00160000UL, &set));
        CHECK(set == 0U);
        CHECK(suas_sueasd_neography_set(0x001603FFUL, &set));
        CHECK(set == 3U);
        CHECK(!suas_sueasd_neography_set(0x00120000UL, &set));
        CHECK(!suas_sueasd_neography_set(0x00160000UL, NULL));
    }

    CHECK(suas_sueasd_is_east_asian(0x00004E00UL));
    CHECK(suas_sueasd_is_east_asian(0x001603FFUL));
    CHECK(!suas_sueasd_is_east_asian(0x00000041UL));
    CHECK(!suas_sueasd_is_east_asian(0x00110000UL));
    CHECK(!suas_sueasd_is_east_asian(0x7FFFFFF0UL));
}

/* -- FNV-1a ------------------------------------------------------------- */

static void test_hash(void)
{
    CHECK(suas_sueasd_hash(0x00004E00UL) == 0x94A4F9BBu);
    CHECK(suas_sueasd_hash(0x00018E00UL) == 0x284F5212u);
    CHECK(suas_sueasd_hash(0x00018E00UL) != suas_sueasd_hash(0x00018E01UL));
    CHECK(suas_sueasd_hash_bytes("", 0U) == 2166136261u);
    CHECK(suas_sueasd_hash_bytes(NULL, 4U) == 0U);
    CHECK(suas_sueasd_hash_bytes("nie", 3U) == 0x16A40717u);
    CHECK(suas_sueasd_hash_bytes("nie", 3U) ==
          suas_sueasd_reading_hash(0x00018E00UL));
}

/* -- The classification ranges and the Han family ----------------------- */

static void test_han_classification(void)
{
    sueasd_script_entry_t e;

    /* A plain unified ideograph resolves from its range, not the table. */
    CHECK(suas_sueasd_get_entry(0x00004E00UL, &e));
    CHECK(e.codepoint == 0x00004E00UL);
    CHECK(e.primary_script == SUEASD_SCRIPT_HAN);
    CHECK(e.source_region == SUEASD_REGION_CHINA);
    CHECK((e.script_mask & SUEASD_FLAG_IDEOGRAPHIC) != 0U);
    CHECK((e.script_mask & SUEASD_FLAG_HAN_CORE) != 0U);
    CHECK((e.script_mask & SUEASD_FLAG_RADICAL) == 0U);
    CHECK((e.script_mask & SUEASD_FLAG_COMPATIBILITY) == 0U);
    CHECK(!suas_sueasd_is_curated(0x00004E00UL));
    CHECK(suas_sueasd_is_east_asian(0x00004E00UL));
    CHECK(suas_sueasd_is_ideographic(0x00004E00UL));
    CHECK(!suas_sueasd_is_radical(0x00004E00UL));

    /* A Kangxi radical carries the RADICAL role. */
    CHECK(suas_sueasd_is_radical(0x00002F00UL));
    CHECK((suas_sueasd_script_mask(0x00002F00UL) & SUEASD_FLAG_HAN_RADICAL)
          != 0U);
    CHECK(suas_sueasd_is_ideographic(0x00002F00UL));

    /* A compatibility ideograph carries the COMPATIBILITY role. */
    CHECK(suas_sueasd_is_compatibility(0x0000F900UL));
    CHECK((suas_sueasd_script_mask(0x0000F900UL) & SUEASD_FLAG_HAN_COMPAT)
          != 0U);
    CHECK(!suas_sueasd_is_radical(0x0000F900UL));

    /* The Supplementary Ideographic Plane resolves through its range. */
    CHECK(suas_sueasd_primary_script(0x00020000UL) == SUEASD_SCRIPT_HAN);
    CHECK(suas_sueasd_is_ideographic(0x00020000UL));

    /* Bopomofo is ideographic but its own script. */
    CHECK(suas_sueasd_get_entry(0x00003105UL, &e));
    CHECK(e.primary_script == SUEASD_SCRIPT_BOPOMOFO);
    CHECK(e.source_region == SUEASD_REGION_CHINA);
    CHECK((e.script_mask & SUEASD_FLAG_IDEOGRAPHIC) != 0U);
    CHECK(!suas_sueasd_is_kana(0x00003105UL));

    /* The range table is ordered and disjoint and passes its verifier. */
    {
        static sueasd_range_t ranges[SUAS_SUEASD_RANGE_CAPACITY];
        sueasd_registry_error_t err = SUEASD_REGISTRY_RANGE_UNORDERED;
        uint32_t i;
        uint32_t count = suas_sueasd_range_count();
        CHECK(count == 78U);
        CHECK(count <= SUAS_SUEASD_RANGE_CAPACITY);
        for (i = 0U; i < count; i++)
            CHECK(suas_sueasd_range_at(i, &ranges[i]));
        CHECK(suas_sueasd_validate_ranges(ranges, count, &err));
        CHECK(err == SUEASD_REGISTRY_OK);
    }
}

/* -- Kana and Hangul ---------------------------------------------------- */

static void test_kana_and_hangul(void)
{
    sueasd_script_entry_t e;

    CHECK(suas_sueasd_get_entry(0x00003042UL, &e));
    CHECK(suas_sueasd_primary_script(0x00003042UL) == SUEASD_SCRIPT_HIRAGANA);
    CHECK(suas_sueasd_source_region(0x00003042UL) == SUEASD_REGION_JAPAN);
    CHECK(suas_sueasd_is_kana(0x00003042UL));
    CHECK(!suas_sueasd_is_ideographic(0x00003042UL));

    CHECK(suas_sueasd_primary_script(0x000030A2UL) == SUEASD_SCRIPT_KATAKANA);
    CHECK(suas_sueasd_is_kana(0x000030A2UL));

    /* Hangul Jamo and precomposed syllables are disjoint roles. */
    CHECK(suas_sueasd_primary_script(0x00001100UL) == SUEASD_SCRIPT_HANGUL);
    CHECK(suas_sueasd_source_region(0x00001100UL) == SUEASD_REGION_KOREA);
    CHECK(suas_sueasd_is_jamo(0x00001100UL));
    CHECK(!suas_sueasd_is_syllable(0x00001100UL));

    CHECK(suas_sueasd_is_syllable(0x0000AC00UL));
    CHECK(!suas_sueasd_is_jamo(0x0000AC00UL));
    CHECK(suas_sueasd_is_ideographic(0x0000AC00UL) == false);
    CHECK(suas_sueasd_is_kana(0x0000AC00UL) == false);
}

/* -- The UAX #60 covered scripts ---------------------------------------- */

static void test_covered_scripts(void)
{
    sueasd_script_entry_t e;
    sueasd_metadata_t m;

    /* Jurchen: reading, source, radical-stroke and (for one sign) numeric. */
    CHECK(suas_sueasd_is_curated(0x00018E00UL));
    CHECK(suas_sueasd_get_entry(0x00018E00UL, &e));
    CHECK(e.primary_script == SUEASD_SCRIPT_JURCHEN);
    CHECK(e.source_region == SUEASD_REGION_JURCHEN);
    CHECK((e.script_mask & SUEASD_FLAG_SOURCE_DATA) != 0U);
    CHECK((e.script_mask & SUEASD_FLAG_READING) != 0U);
    CHECK((e.script_mask & SUEASD_FLAG_RS) != 0U);
    CHECK(suas_sueasd_radical(0x00018E00UL) == 1U);
    CHECK(suas_sueasd_stroke_count(0x00018E00UL) == 3U);
    CHECK(suas_sueasd_numeric_value(0x00018E00UL) == 0U);
    CHECK(suas_sueasd_get_metadata(0x00018E00UL, &m));
    CHECK((m.source_mask & SUEASD_SOURCE_JURC_SRC) != 0U);
    CHECK((m.source_mask & SUEASD_SOURCE_JURC_NCREADING) != 0U);
    CHECK((m.source_mask & SUEASD_SOURCE_JURC_RSUNICODE) != 0U);
    CHECK(m.source_count == 3U);
    CHECK(m.aux_hash == 0xEA421A5Fu); /* FNV-1a of "1.3" */
    CHECK(m.source_hash == 0xAA5125EAu); /* FNV-1a of "NC:002.02" */

    /* Jurchen 18E01 adds a numeric value of ten thousand. */
    CHECK(suas_sueasd_numeric_value(0x00018E01UL) == 10000U);
    CHECK((suas_sueasd_script_mask(0x00018E01UL) & SUEASD_FLAG_NUMERIC) != 0U);

    /* Nushu: a reading and the Duben source, no radical-stroke. */
    CHECK(suas_sueasd_primary_script(0x0001B170UL) == SUEASD_SCRIPT_NUSHU);
    CHECK(suas_sueasd_source_region(0x0001B170UL) == SUEASD_REGION_NUSHU);
    CHECK((suas_sueasd_source_mask(0x0001B170UL) & SUEASD_SOURCE_NSHU_READING)
          != 0U);
    CHECK(suas_sueasd_reading_hash(0x0001B170UL) == 0x8838EA63u); /* "i5" */
    CHECK(suas_sueasd_radical(0x0001B170UL) == 0U);
    CHECK(suas_sueasd_source_count(0x0001B170UL) == 2U);

    /* Small Seal: six source rows, a radical and a modern-CJK equivalent. */
    CHECK(suas_sueasd_primary_script(0x0003D000UL) == SUEASD_SCRIPT_SEAL);
    CHECK(suas_sueasd_source_region(0x0003D000UL) == SUEASD_REGION_SEAL);
    CHECK(suas_sueasd_is_ideographic(0x0003D000UL));
    CHECK(suas_sueasd_source_count(0x0003D000UL) == 6U);
    CHECK((suas_sueasd_source_mask(0x0003D000UL) & SUEASD_SOURCE_SEAL_MCJK)
          != 0U);
    CHECK((suas_sueasd_source_mask(0x0003D000UL) & SUEASD_SOURCE_SEAL_RAD)
          != 0U);
    CHECK(suas_sueasd_modern_cjk(0x0003D000UL) == 0x00004E00UL);
    CHECK(suas_sueasd_radical(0x0003D000UL) == 1U);
    CHECK((suas_sueasd_script_mask(0x0003D000UL) & SUEASD_FLAG_MODERN_CJK)
          != 0U);

    /* Tangut: a merged source and a radical-stroke, and a numeric sign. */
    CHECK(suas_sueasd_primary_script(0x00017000UL) == SUEASD_SCRIPT_TANGUT);
    CHECK(suas_sueasd_source_region(0x00017000UL) == SUEASD_REGION_TANGUT);
    CHECK(suas_sueasd_radical(0x00017000UL) == 1U);
    CHECK(suas_sueasd_stroke_count(0x00017000UL) == 6U);
    CHECK((suas_sueasd_source_mask(0x00017000UL) & SUEASD_SOURCE_TGT_MERGEDSRC)
          != 0U);
    CHECK(suas_sueasd_numeric_value(0x0001736BUL) == 2U);
    CHECK(suas_sueasd_radical(0x0001736BUL) == 46U);
    CHECK(suas_sueasd_stroke_count(0x0001736BUL) == 9U);

    /* Khitan Small Script is classified but carries no UAX #60 data yet. */
    CHECK(suas_sueasd_primary_script(0x00018B00UL) ==
          SUEASD_SCRIPT_KHITAN_SMALL);
    CHECK(suas_sueasd_source_region(0x00018B00UL) == SUEASD_REGION_KHITAN);
    CHECK(!suas_sueasd_is_curated(0x00018B00UL));
}

/* -- The algorithmic default and the query domain ----------------------- */

static void test_default_and_domain(void)
{
    sueasd_script_entry_t e;
    sueasd_metadata_t m;

    CHECK(suas_sueasd_get_entry(0x00004E00UL, NULL) == false);
    CHECK(suas_sueasd_get_metadata(0x00004E00UL, NULL) == false);

    /* Outside any East Asian range the record defaults to UNKNOWN/NONE. */
    CHECK(suas_sueasd_get_entry(0x00000041UL, &e));
    CHECK(e.codepoint == 0x00000041UL);
    CHECK(e.script_mask == 0U);
    CHECK(e.primary_script == SUEASD_SCRIPT_UNKNOWN);
    CHECK(e.source_region == SUEASD_REGION_NONE);
    CHECK(!suas_sueasd_is_east_asian(0x00000041UL));
    CHECK(suas_sueasd_get_metadata(0x00000041UL, &m));
    CHECK(m.source_mask == 0U);
    CHECK(!suas_sueasd_is_curated(0x00000041UL));
    CHECK(!suas_sueasd_is_ideographic(0x00000041UL));
    CHECK(suas_sueasd_primary_script(0x00000041UL) == SUEASD_SCRIPT_UNKNOWN);

    /* An inadmissible zone leaves the output untouched and returns false. */
    e.codepoint = 0xDEADBEEFu;
    CHECK(!suas_sueasd_get_entry(0x00110000UL, &e));
    CHECK(e.codepoint == 0xDEADBEEFu);
    CHECK(!suas_sueasd_get_entry(0x7FFFFFF0UL, &e));
    CHECK(!suas_sueasd_get_entry(0x7FFFFFFFUL, &e));
    CHECK(!suas_sueasd_get_entry(0x80000000UL, &e));

    /* A declared-but-uncurated RNUR codepoint still resolves total. */
    CHECK(suas_sueasd_is_east_asian(0x001603FFUL));
    CHECK(!suas_sueasd_is_curated(0x001603FFUL));
    CHECK(suas_sueasd_get_entry(0x001603FFUL, &e));
    CHECK(e.codepoint == 0x001603FFUL);
    CHECK(e.script_mask == SUEASD_FLAG_CUSTOM_SCRIPT);
    CHECK(e.primary_script == SUEASD_SCRIPT_UNKNOWN);
    CHECK(suas_sueasd_is_custom_script(0x001603FFUL));
    CHECK(!suas_sueasd_is_ideographic(0x001603FFUL));
    CHECK(suas_sueasd_get_metadata(0x001603FFUL, &m));
    CHECK(m.source_mask == 0U);
    CHECK(m.radical == 0U);
    CHECK(m.reading_hash == 0U);

    /* An unassigned slot inside an East Asian range still resolves from the
     * range's base record, though it is not curated. */
    CHECK(suas_sueasd_get_entry(0x00004E01UL, &e));
    CHECK(e.primary_script == SUEASD_SCRIPT_HAN);
    CHECK(!suas_sueasd_is_curated(0x00004E01UL));
}

/* -- Enumeration and compiled-table self-consistency -------------------- */

static void test_enumeration(void)
{
    sueasd_script_entry_t e;
    sueasd_script_entry_t prev;
    sueasd_metadata_t m;
    static sueasd_script_entry_t all[SUAS_SUEASD_REGISTRY_CAPACITY];
    sueasd_registry_error_t err = SUEASD_REGISTRY_RANGE_UNORDERED;
    uint32_t i;
    uint32_t count = suas_sueasd_entry_count();

    CHECK(count == 18815U);
    CHECK(count <= SUAS_SUEASD_REGISTRY_CAPACITY);
    CHECK(suas_sueasd_entry_at(0U, &e));
    CHECK(e.codepoint == 0x00017000UL);
    CHECK(suas_sueasd_entry_at(count - 1U, &e));
    CHECK(e.codepoint == 0x0003FC3FUL);
    CHECK(suas_sueasd_metadata_at(0U, &m));
    CHECK(m.radical == 1U);
    CHECK(!suas_sueasd_entry_at(count, &e));
    CHECK(!suas_sueasd_metadata_at(count, &m));
    CHECK(!suas_sueasd_entry_at(0U, NULL));
    CHECK(!suas_sueasd_metadata_at(0U, NULL));

    CHECK(suas_sueasd_entry_at(0U, &prev));
    all[0] = prev;
    for (i = 1U; i < count; i++) {
        CHECK(suas_sueasd_entry_at(i, &e));
        CHECK(suas_sueasd_metadata_at(i, &m));
        CHECK(e.codepoint > prev.codepoint);
        CHECK(e.primary_script < SUEASD_SCRIPT_COUNT);
        CHECK(e.source_region <= SUEASD_REGION_KHITAN);
        CHECK(e.reserved == 0U);
        CHECK(suas_sueasd_is_curated(e.codepoint));
        {
            sueasd_script_entry_t got;
            sueasd_metadata_t got_meta;
            CHECK(suas_sueasd_get_entry(e.codepoint, &got));
            CHECK(got.codepoint == e.codepoint);
            CHECK(got.script_mask == e.script_mask);
            CHECK(got.primary_script == e.primary_script);
            CHECK(got.source_region == e.source_region);
            CHECK(suas_sueasd_get_metadata(e.codepoint, &got_meta));
            CHECK(got_meta.source_mask == m.source_mask);
            CHECK(got_meta.radical == m.radical);
            CHECK(got_meta.reading_hash == m.reading_hash);
        }
        all[i] = e;
        prev = e;
    }

    /* The entire shipped table passes the section 8 verifier. */
    CHECK(suas_sueasd_validate_table(all, count, &err));
    CHECK(err == SUEASD_REGISTRY_OK);
}

/* -- Verification outcomes ---------------------------------------------- */

static void test_validate_table(void)
{
    sueasd_script_entry_t valid[2];
    sueasd_script_entry_t e;
    sueasd_registry_error_t err = SUEASD_REGISTRY_OK;

    CHECK(suas_sueasd_validate_table(NULL, 0U, &err));
    CHECK(err == SUEASD_REGISTRY_OK);
    CHECK(!suas_sueasd_validate_table(NULL, 1U, &err));
    CHECK(err == SUEASD_REGISTRY_NULL_TABLE);
    CHECK(!suas_sueasd_validate_table(&valid[0],
                                      SUAS_SUEASD_REGISTRY_CAPACITY + 1U,
                                      &err));
    CHECK(err == SUEASD_REGISTRY_COUNT_OVER);

    valid[0].codepoint = 0x00004E00UL;
    valid[0].script_mask = SUEASD_FLAG_IDEOGRAPHIC | SUEASD_FLAG_HAN_CORE;
    valid[0].primary_script = SUEASD_SCRIPT_HAN;
    valid[0].source_region = SUEASD_REGION_CHINA;
    valid[0].reserved = 0U;
    valid[1] = valid[0];
    valid[1].codepoint = 0x00004E01UL;
    CHECK(suas_sueasd_validate_table(valid, 2U, &err));
    CHECK(err == SUEASD_REGISTRY_OK);

    e = valid[0];
    e.codepoint = 0x80000000UL;
    CHECK(!suas_sueasd_validate_table(&e, 1U, &err));
    CHECK(err == SUEASD_REGISTRY_OUT_OF_SPACE);

    e = valid[0];
    e.codepoint = 0x00000041UL;
    e.script_mask = 0U;
    e.primary_script = SUEASD_SCRIPT_UNKNOWN;
    e.source_region = SUEASD_REGION_NONE;
    CHECK(!suas_sueasd_validate_table(&e, 1U, &err));
    CHECK(err == SUEASD_REGISTRY_INADMISSIBLE_CODEPOINT);

    e = valid[0];
    e.primary_script = SUEASD_SCRIPT_COUNT;
    CHECK(!suas_sueasd_validate_table(&e, 1U, &err));
    CHECK(err == SUEASD_REGISTRY_BAD_SCRIPT);

    e = valid[0];
    e.source_region = 0xFEu;
    CHECK(!suas_sueasd_validate_table(&e, 1U, &err));
    CHECK(err == SUEASD_REGISTRY_BAD_REGION);

    e = valid[0];
    e.primary_script = SUEASD_SCRIPT_HIRAGANA;
    CHECK(!suas_sueasd_validate_table(&e, 1U, &err));
    CHECK(err == SUEASD_REGISTRY_IDEOGRAPH_MISMATCH);

    e = valid[0];
    e.codepoint = 0x00003042UL;
    e.script_mask = SUEASD_FLAG_KANA;
    e.primary_script = SUEASD_SCRIPT_HAN;
    CHECK(!suas_sueasd_validate_table(&e, 1U, &err));
    CHECK(err == SUEASD_REGISTRY_KANA_MISMATCH);

    e = valid[0];
    e.codepoint = 0x0000AC00UL;
    e.script_mask = SUEASD_FLAG_JAMO | SUEASD_FLAG_SYLLABLE;
    e.primary_script = SUEASD_SCRIPT_HANGUL;
    CHECK(!suas_sueasd_validate_table(&e, 1U, &err));
    CHECK(err == SUEASD_REGISTRY_JAMO_MISMATCH);

    e = valid[0];
    e.reserved = 1U;
    CHECK(!suas_sueasd_validate_table(&e, 1U, &err));
    CHECK(err == SUEASD_REGISTRY_BAD_RESERVED);

    /* Ordering and duplication. */
    {
        sueasd_script_entry_t pair[2];
        pair[0] = valid[1];
        pair[1] = valid[0];
        CHECK(!suas_sueasd_validate_table(pair, 2U, &err));
        CHECK(err == SUEASD_REGISTRY_UNORDERED);
        pair[0] = valid[0];
        pair[1] = valid[0];
        CHECK(!suas_sueasd_validate_table(pair, 2U, &err));
        CHECK(err == SUEASD_REGISTRY_DUPLICATE_CODEPOINT);
    }
}

static void test_validate_ranges(void)
{
    sueasd_range_t valid[2];
    sueasd_range_t r;
    sueasd_registry_error_t err = SUEASD_REGISTRY_OK;

    CHECK(suas_sueasd_validate_ranges(NULL, 0U, &err));
    CHECK(err == SUEASD_REGISTRY_OK);
    CHECK(!suas_sueasd_validate_ranges(NULL, 1U, &err));
    CHECK(err == SUEASD_REGISTRY_NULL_TABLE);

    valid[0].first = 0x00004E00UL;
    valid[0].last = 0x00004EFFUL;
    valid[0].primary_script = SUEASD_SCRIPT_HAN;
    valid[0].base_flags = SUEASD_FLAG_IDEOGRAPHIC | SUEASD_FLAG_HAN_CORE;
    valid[1].first = 0x00004F00UL;
    valid[1].last = 0x00004FFFUL;
    valid[1].primary_script = SUEASD_SCRIPT_HAN;
    valid[1].base_flags = SUEASD_FLAG_IDEOGRAPHIC | SUEASD_FLAG_HAN_CORE;
    CHECK(suas_sueasd_validate_ranges(valid, 2U, &err));
    CHECK(err == SUEASD_REGISTRY_OK);

    r = valid[0];
    r.last = 0x80000000UL;
    CHECK(!suas_sueasd_validate_ranges(&r, 1U, &err));
    CHECK(err == SUEASD_REGISTRY_RANGE_OUT_OF_SPACE);

    r = valid[0];
    r.first = 0x00004F00UL;
    r.last = 0x00004E00UL;
    CHECK(!suas_sueasd_validate_ranges(&r, 1U, &err));
    CHECK(err == SUEASD_REGISTRY_RANGE_UNORDERED);

    r = valid[0];
    r.primary_script = SUEASD_SCRIPT_COUNT;
    CHECK(!suas_sueasd_validate_ranges(&r, 1U, &err));
    CHECK(err == SUEASD_REGISTRY_BAD_SCRIPT);

    /* Overlapping ranges. */
    {
        sueasd_range_t pair[2];
        pair[0] = valid[0];
        pair[1] = valid[1];
        pair[1].first = 0x00004E80UL;
        CHECK(!suas_sueasd_validate_ranges(pair, 2U, &err));
        CHECK(err == SUEASD_REGISTRY_RANGE_UNORDERED);
    }
}

/* -- Taxonomy names ----------------------------------------------------- */

static void test_taxonomy(void)
{
    CHECK(suas_sueasd_script_name(SUEASD_SCRIPT_HAN)[0] == 'H');
    CHECK(suas_sueasd_script_name(SUEASD_SCRIPT_HANGUL)[0] == 'H');
    CHECK(suas_sueasd_script_name(SUEASD_SCRIPT_HIRAGANA)[0] == 'H');
    CHECK(suas_sueasd_script_name(SUEASD_SCRIPT_KATAKANA)[0] == 'K');
    CHECK(suas_sueasd_script_name(SUEASD_SCRIPT_BOPOMOFO)[0] == 'B');
    CHECK(suas_sueasd_script_name(SUEASD_SCRIPT_JURCHEN)[0] == 'J');
    CHECK(suas_sueasd_script_name(SUEASD_SCRIPT_NUSHU)[0] == 'N');
    CHECK(suas_sueasd_script_name(SUEASD_SCRIPT_SEAL)[0] == 'S');
    CHECK(suas_sueasd_script_name(SUEASD_SCRIPT_TANGUT)[0] == 'T');
    CHECK(suas_sueasd_script_name(SUEASD_SCRIPT_KHITAN_SMALL)[0] == 'K');
    CHECK(suas_sueasd_script_name(0xFEu)[0] == 'U');

    CHECK(suas_sueasd_script_short_name(SUEASD_SCRIPT_HAN)[0] == 'H');
    CHECK(suas_sueasd_script_short_name(SUEASD_SCRIPT_UNKNOWN)[0] == 'Z');
    CHECK(suas_sueasd_script_short_name(0xFEu)[0] == 'Z');

    CHECK(suas_sueasd_region_name(SUEASD_REGION_NONE)[0] == 'n');
    CHECK(suas_sueasd_region_name(SUEASD_REGION_CHINA)[0] == 'C');
    CHECK(suas_sueasd_region_name(SUEASD_REGION_KOREA)[0] == 'K');
    CHECK(suas_sueasd_region_name(SUEASD_REGION_JAPAN)[0] == 'J');
    CHECK(suas_sueasd_region_name(SUEASD_REGION_JURCHEN)[0] == 'J');
    CHECK(suas_sueasd_region_name(SUEASD_REGION_NUSHU)[0] == 'N');
    CHECK(suas_sueasd_region_name(SUEASD_REGION_SEAL)[0] == 'S');
    CHECK(suas_sueasd_region_name(SUEASD_REGION_TANGUT)[0] == 'T');
    CHECK(suas_sueasd_region_name(SUEASD_REGION_KHITAN)[0] == 'K');
    CHECK(suas_sueasd_region_name(0xFEu)[0] == 'u');

    CHECK(suas_sueasd_source_name(SUEASD_SOURCE_JURC_SRC)[0] == 'j');
    CHECK(suas_sueasd_source_name(SUEASD_SOURCE_NSHU_READING)[0] == 'n');
    CHECK(suas_sueasd_source_name(SUEASD_SOURCE_SEAL_MCJK)[0] == 's');
    CHECK(suas_sueasd_source_name(SUEASD_SOURCE_TGT_MERGEDSRC)[0] == 't');
    CHECK(suas_sueasd_source_name(SUEASD_SOURCE_HAN)[0] == 'h');
    CHECK(suas_sueasd_source_name(0U)[0] == 'n');
    CHECK(suas_sueasd_source_name(0xFFFFu)[0] == 'c');

    CHECK(suas_sueasd_zone_name(SUAS_SUEASD_ZONE_BRIDGE)[0] == 'B');
    CHECK(suas_sueasd_zone_name(SUAS_SUEASD_ZONE_SCP)[0] == 'S');
    CHECK(suas_sueasd_zone_name(SUAS_SUEASD_ZONE_RNUR)[0] == 'N');
    CHECK(suas_sueasd_zone_name(SUAS_SUEASD_ZONE_NATIVE)[0] == 'N');
    CHECK(suas_sueasd_zone_name(SUAS_SUEASD_ZONE_TRAP)[0] == 'T');
    CHECK(suas_sueasd_zone_name(SUAS_SUEASD_ZONE_SENTINEL)[0] == 'S');
    CHECK(suas_sueasd_zone_name(SUAS_SUEASD_ZONE_EXTSUCS)[0] == 'E');
    CHECK(suas_sueasd_zone_name((suas_sueasd_zone_t)0xFEu)[0] == 'u');

    CHECK(suas_sueasd_registry_error_name(SUEASD_REGISTRY_OK)[0] == 'o');
    CHECK(suas_sueasd_registry_error_name(SUEASD_REGISTRY_NULL_TABLE)[0] == 'n');
    CHECK(suas_sueasd_registry_error_name(SUEASD_REGISTRY_RANGE_UNORDERED)[0]
          == 'u');
    CHECK(suas_sueasd_registry_error_name((sueasd_registry_error_t)0xFEu)[0]
          == 'u');
}

/* -- SSRE registration -------------------------------------------------- */

static void test_ssre_registration(void)
{
    const ssre_spec_record_t *rec;

    CHECK(suas_ssre_is_registered("SUAS-017"));
    rec = suas_ssre_lookup("SUAS-017");
    CHECK(rec != NULL);
    if (rec != NULL) {
        CHECK(rec->short_name[0] == 'S');
        CHECK(rec->module[0] == 's');
        CHECK(rec->kind == (uint8_t)SUAS_SSRE_KIND_MANDATORY);
        CHECK(rec->tier == SUAS_SSRE_TIER_L0);
    }
}

int main(void)
{
    test_version_and_meta();
    test_zones();
    test_hash();
    test_han_classification();
    test_kana_and_hangul();
    test_covered_scripts();
    test_default_and_domain();
    test_enumeration();
    test_validate_table();
    test_validate_ranges();
    test_taxonomy();
    test_ssre_registration();

    if (g_fail) {
        printf("SUAS-017 SUEASD tests FAILED\n");
        return 1;
    }
    printf("SUAS-017 SUEASD tests passed\n");
    return 0;
}
