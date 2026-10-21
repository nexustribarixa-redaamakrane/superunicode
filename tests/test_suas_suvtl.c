/**
 * SUAS-014 SUVTL — SuperUnicode Vertical Text Layout
 *
 * Unit tests for the zero-allocation, static-table, O(1) resolution of a
 * character's vertical orientation, rotation, and baseline adjustment (a
 * UAX #50 analog).
 *
 * Covers: version/meta, the compile-time record geometry, the orientation
 * values and their derived predicates, zone dispatch and the RNUR carve-out,
 * the vertical-behaviour groups and the algorithmic default, the O(1) layout
 * query against curated Tu/Tr records, the grapheme-cluster rule, baseline
 * adjustments, the lazy/idempotent build, enumeration and table
 * self-consistency, every verification outcome, and registration in
 * SUAS-010 (SSRE).
 */

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <assert.h>

#include "suas/suas_suvtl.h"
#include "suas/suas_ssre.h"

static int g_fail = 0;
#define CHECK(cond) do { if (!(cond)) { \
    printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); g_fail = 1; \
} } while (0)

/* ── Version, limits, and record geometry ─────────────────────────── */

static void test_version_and_meta(void)
{
    CHECK(suas_suvtl_version_string() != NULL);
    CHECK(suas_suvtl_version_string()[0] == '1');
    CHECK(suas_suvtl_version_string()[2] == '0');
    CHECK(SUAS_SUVTL_SPEC_ID[0] == 'S');
    CHECK(SUAS_SUVTL_SPEC_TITLE[0] == 'S');
    CHECK(SUAS_SUVTL_VERSION_MAJOR == 1);
    CHECK(SUAS_SUVTL_VERSION_MINOR == 0);
    CHECK(SUAS_SUVTL_REGISTRY_CAPACITY == 512U);
    CHECK(SUAS_SUVTL_INDEX_SIZE == 1024U);
    CHECK(SUAS_SUVTL_INDEX_SIZE > SUAS_SUVTL_REGISTRY_CAPACITY);

    /* The record is fixed-size, byte-aligned and padding-free (§3.1, §3.3). */
    CHECK(sizeof(suvtl_glyph_layout_t) == 8U);
    CHECK(offsetof(suvtl_glyph_layout_t, codepoint) == 0U);
    CHECK(offsetof(suvtl_glyph_layout_t, orientation_flags) == 4U);
    CHECK(offsetof(suvtl_glyph_layout_t, baseline_offset) == 6U);
    CHECK(offsetof(suvtl_glyph_layout_t, reserved) == 7U);

    /* The four values are distinct (§3.1). */
    CHECK(SUVTL_ORIENT_U != SUVTL_ORIENT_R);
    CHECK(SUVTL_ORIENT_U != SUVTL_ORIENT_TU);
    CHECK(SUVTL_ORIENT_R != SUVTL_ORIENT_TR);
    CHECK(SUVTL_ORIENT_TU != SUVTL_ORIENT_TR);
    CHECK(SUVTL_ORIENT_U == SUVTL_ORIENT_UPRIGHT);
    CHECK(SUVTL_ORIENT_R == SUVTL_ORIENT_ROTATED);
    CHECK(SUVTL_ORIENT_TU == (SUVTL_ORIENT_UPRIGHT | SUVTL_ORIENT_TRANSFORM));
    CHECK(SUVTL_ORIENT_TR == (SUVTL_ORIENT_ROTATED | SUVTL_ORIENT_TRANSFORM));

    /* Flag bits are disjoint. */
    CHECK((SUVTL_ORIENT_UPRIGHT & SUVTL_ORIENT_ROTATED) == 0u);
    CHECK((SUVTL_FLAG_CUSTOM_FLOW & SUVTL_FLAG_ASSIGNED_MASK) != 0u);
    CHECK((SUVTL_FLAG_ASSIGNED_MASK & (uint16_t)~SUVTL_FLAG_ASSIGNED_MASK) == 0u);
    CHECK(SUVTL_BASELINE_SCALE == 16);
}

/* ── Zone dispatch and the RNUR carve-out (§5) ────────────────────── */

static void test_zones(void)
{
    CHECK(suas_suvtl_zone_of(0x0041u) == SUAS_SUVTL_ZONE_BRIDGE);
    CHECK(suas_suvtl_zone_of(0x0010FFFFu) == SUAS_SUVTL_ZONE_BRIDGE);
    CHECK(suas_suvtl_zone_of(0x00110000u) == SUAS_SUVTL_ZONE_SCP);
    CHECK(suas_suvtl_zone_of(0x0011FFFFu) == SUAS_SUVTL_ZONE_SCP);
    CHECK(suas_suvtl_zone_of(0x00120000u) == SUAS_SUVTL_ZONE_NATIVE);
    CHECK(suas_suvtl_zone_of(0x0015FFFFu) == SUAS_SUVTL_ZONE_NATIVE);
    CHECK(suas_suvtl_zone_of(0x00160000u) == SUAS_SUVTL_ZONE_RNUR);
    CHECK(suas_suvtl_zone_of(0x0016FFFFu) == SUAS_SUVTL_ZONE_RNUR);
    CHECK(suas_suvtl_zone_of(0x00170000u) == SUAS_SUVTL_ZONE_NATIVE);
    CHECK(suas_suvtl_zone_of(0x7FFFFFF0u) == SUAS_SUVTL_ZONE_TRAP);
    CHECK(suas_suvtl_zone_of(0x7FFFFFFEu) == SUAS_SUVTL_ZONE_TRAP);
    CHECK(suas_suvtl_zone_of(0x7FFFFFFFu) == SUAS_SUVTL_ZONE_SENTINEL);
    CHECK(suas_suvtl_zone_of(0x80000000u) == SUAS_SUVTL_ZONE_EXTSUCS);
    CHECK(suas_suvtl_zone_of(0xFFFFFFFFu) == SUAS_SUVTL_ZONE_EXTSUCS);

    /* A vertical layout query may name Bridge, RNUR and Native only. */
    CHECK(suas_suvtl_is_text_space(0x4E00u));
    CHECK(suas_suvtl_is_text_space(0x00120000u));
    CHECK(suas_suvtl_is_text_space(0x00160100u));
    CHECK(!suas_suvtl_is_text_space(0x00110000u));
    CHECK(!suas_suvtl_is_text_space(0x7FFFFFF0u));
    CHECK(!suas_suvtl_is_text_space(0x7FFFFFFFu));
    CHECK(!suas_suvtl_is_text_space(0x80000000u));

    CHECK(suas_suvtl_is_neography(0x00160000u));
    CHECK(suas_suvtl_is_neography(0x0016FFFFu));
    CHECK(!suas_suvtl_is_neography(0x0015FFFFu));
}

static void test_neography_set(void)
{
    uint32_t set = 0xFFFFFFFFu;

    CHECK(suas_suvtl_neography_set(0x00160100u, &set));
    CHECK(set == 1u);                    /* (0x100) >> 8 */
    CHECK(suas_suvtl_neography_set(0x00160200u, &set));
    CHECK(set == 2u);
    CHECK(suas_suvtl_neography_set(0x0016FFFFu, &set));
    CHECK(set == 0xFFu);

    CHECK(!suas_suvtl_neography_set(0x00120000u, &set));
    CHECK(!suas_suvtl_neography_set(0x00160100u, NULL));
}

/* ── Hash (§3.4) ──────────────────────────────────────────────────── */

static void test_hash(void)
{
    /* FNV-1a 32-bit over the four little-endian codepoint bytes. */
    CHECK(suas_suvtl_hash(0x00160100u) == suas_suvtl_hash(0x00160100u));
    CHECK(suas_suvtl_hash(0x00160100u) != suas_suvtl_hash(0x00160101u));
    CHECK(suas_suvtl_hash(0u) != 0u);
    CHECK(suas_suvtl_hash(0xFFFFFFFFu) != 0u);
}

/* ── Orientation values and the derived predicates (§3.1) ─────────── */

static void test_orientation_values(void)
{
    suvtl_glyph_layout_t lay;

    /* A curated transformed-rotated form: fullwidth comma, Tr. */
    CHECK(suas_suvtl_get_layout_properties(0xFF0Cu, &lay));
    CHECK((lay.orientation_flags & SUVTL_ORIENT_MASK) == SUVTL_ORIENT_TR);
    CHECK((lay.orientation_flags & SUVTL_ORIENT_ROTATED) != 0u);
    CHECK((lay.orientation_flags & SUVTL_ORIENT_TRANSFORM) != 0u);
    CHECK(suas_suvtl_orientation_of(0xFF0Cu) == SUVTL_ORIENT_TR);
    CHECK(suas_suvtl_is_transformed(0xFF0Cu));
    CHECK(suas_suvtl_is_rotated(0xFF0Cu)); /* R or Tr: the rotated view */
    CHECK(suas_suvtl_orientation_name(SUVTL_ORIENT_TR)[0] == 'T');
    CHECK(suas_suvtl_orientation_name(SUVTL_ORIENT_TR)[1] == 'r');
    CHECK(suas_suvtl_orientation_long_name(SUVTL_ORIENT_TR)[0] == 'T');

    /* A curated squared word: transformed-upright, Tu. */
    CHECK(suas_suvtl_get_layout_properties(0x3300u, &lay));
    CHECK((lay.orientation_flags & SUVTL_ORIENT_MASK) == SUVTL_ORIENT_TU);
    CHECK((lay.orientation_flags & SUVTL_FLAG_SQUARED) != 0u);
    CHECK(suas_suvtl_is_upright(0x3300u));
    CHECK(suas_suvtl_is_transformed(0x3300u));
    CHECK(!suas_suvtl_is_rotated(0x3300u));

    /* A curated quotation mark: it is also tagged QUOTATION. */
    CHECK(suas_suvtl_get_layout_properties(0x201Cu, &lay));
    CHECK((lay.orientation_flags & SUVTL_FLAG_QUOTATION) != 0u);
    CHECK((lay.orientation_flags & SUVTL_ORIENT_MASK) == SUVTL_ORIENT_TR);

    /* A curated upright neography record: U with CUSTOM_FLOW. */
    CHECK(suas_suvtl_get_layout_properties(0x00160100u, &lay));
    CHECK((lay.orientation_flags & SUVTL_ORIENT_MASK) == SUVTL_ORIENT_U);
    CHECK((lay.orientation_flags & SUVTL_FLAG_CUSTOM_FLOW) != 0u);
    CHECK(suas_suvtl_is_upright(0x00160100u));
}

static void test_derived_bits_consistent(void)
{
    uint32_t i;
    uint32_t n = suas_suvtl_entry_count();

    for (i = 0u; i < n; i++) {
        suvtl_glyph_layout_t e;
        uint16_t v;
        bool up;
        bool rot;
        bool tr;

        CHECK(suas_suvtl_entry_at(i, &e));
        v = (uint16_t)(e.orientation_flags & SUVTL_ORIENT_MASK);
        up  = (e.orientation_flags & SUVTL_ORIENT_UPRIGHT) != 0u;
        rot = (e.orientation_flags & SUVTL_ORIENT_ROTATED) != 0u;
        tr  = (e.orientation_flags & SUVTL_ORIENT_TRANSFORM) != 0u;

        CHECK(up == (v == SUVTL_ORIENT_U || v == SUVTL_ORIENT_TU));
        CHECK(rot == (v == SUVTL_ORIENT_R || v == SUVTL_ORIENT_TR));
        CHECK(tr == (v == SUVTL_ORIENT_TU || v == SUVTL_ORIENT_TR));
        /* A record is never both upright and rotated. */
        CHECK(!(up && rot));
    }
}

/* ── The algorithmic default (§5.3) ───────────────────────────────── */

static void test_groups_and_default(void)
{
    suvtl_glyph_layout_t lay;

    /* Upright East Asian groups. */
    CHECK(suas_suvtl_group_of(0x4E00u) == SUAS_SUVTL_GROUP_CJK);
    CHECK(suas_suvtl_group_of(0x3042u) == SUAS_SUVTL_GROUP_CJK);
    CHECK(suas_suvtl_group_of(0xAC00u) == SUAS_SUVTL_GROUP_CJK);
    /* Rotating Latin/Greek/Cyrillic groups. */
    CHECK(suas_suvtl_group_of(0x0041u) == SUAS_SUVTL_GROUP_ROTATING);
    CHECK(suas_suvtl_group_of(0x0391u) == SUAS_SUVTL_GROUP_ROTATING);
    CHECK(suas_suvtl_group_of(0x0410u) == SUAS_SUVTL_GROUP_ROTATING);
    /* The RNUR carve-out is the Native group. */
    CHECK(suas_suvtl_group_of(0x00160000u) == SUAS_SUVTL_GROUP_NATIVE);
    /* The remainder is OTHER. */
    CHECK(suas_suvtl_group_of(0x0020u) == SUAS_SUVTL_GROUP_OTHER);

    CHECK(suas_suvtl_group_name(SUAS_SUVTL_GROUP_CJK)[0] == 'C');
    CHECK(suas_suvtl_group_name(SUAS_SUVTL_GROUP_ROTATING)[0] == 'R');
    CHECK(suas_suvtl_group_name((suas_suvtl_group_t)99)[0] == 'I');

    /* An uncurated codepoint is still answered: the property is total. */
    CHECK(suas_suvtl_get_layout_properties(0x0061u, &lay)); /* 'a' */
    CHECK((lay.orientation_flags & SUVTL_ORIENT_MASK) == SUVTL_ORIENT_R);
    CHECK(lay.codepoint == 0x0061u);
    CHECK(lay.reserved == 0u);

    /* An uncurated Han ideograph defaults upright. */
    CHECK(suas_suvtl_get_layout_properties(0x4E01u, &lay));
    CHECK((lay.orientation_flags & SUVTL_ORIENT_MASK) == SUVTL_ORIENT_U);

    /* An uncurated Native-plane codepoint defaults upright. */
    CHECK(suas_suvtl_get_layout_properties(0x00120000u, &lay));
    CHECK((lay.orientation_flags & SUVTL_ORIENT_MASK) == SUVTL_ORIENT_U);

    /* Inadmissible zones are the only false. */
    CHECK(!suas_suvtl_get_layout_properties(0x00110000u, &lay));
    CHECK(!suas_suvtl_get_layout_properties(0x7FFFFFF0u, &lay));
    CHECK(!suas_suvtl_get_layout_properties(0x7FFFFFFFu, &lay));
    CHECK(!suas_suvtl_get_layout_properties(0x80000000u, &lay));
    CHECK(!suas_suvtl_get_layout_properties(0x0041u, NULL));

    /* The orientation of an inadmissible zone is the upright sentinel. */
    CHECK(suas_suvtl_orientation_of(0x7FFFFFFFu) == SUVTL_ORIENT_U);
}

/* ── Baseline adjustment (§3.2) ───────────────────────────────────── */

static void test_baseline(void)
{
    /* Curated zero baseline. */
    CHECK(suas_suvtl_baseline_offset(0xFF0Cu) == 0);
    /* A curated positive adjustment (+4 sixteenths). */
    CHECK(suas_suvtl_baseline_offset(0x00160100u) == 4);
    /* A curated negative adjustment (0xFF == -1 sixteenth). */
    CHECK(suas_suvtl_baseline_offset(0x00160200u) == -1);
    /* Default is zero, including inadmissible zones. */
    CHECK(suas_suvtl_baseline_offset(0x0061u) == 0);
    CHECK(suas_suvtl_baseline_offset(0x7FFFFFFFu) == 0);
}

/* ── The grapheme-cluster rule (§4) ───────────────────────────────── */

static void test_cluster_rule(void)
{
    uint32_t latin[1];

    /* NULL / empty is upright. */
    CHECK(suas_suvtl_cluster_orientation(NULL, 0u) == SUVTL_ORIENT_U);
    CHECK(suas_suvtl_cluster_orientation(latin, 0u) == SUVTL_ORIENT_U);

    /* A single rotated character keeps its orientation. */
    latin[0] = 0x0061u;                 /* 'a' -> R */
    CHECK(suas_suvtl_cluster_orientation(latin, 1u) == SUVTL_ORIENT_R);

    /* The cluster takes the first character's orientation. */
    {
        uint32_t cl[2];
        cl[0] = 0x0061u;                /* 'a' -> R */
        cl[1] = 0x0301u;                /* combining acute (not Me) */
        CHECK(suas_suvtl_cluster_orientation(cl, 2u) == SUVTL_ORIENT_R);
    }

    /* An enclosing mark anywhere forces the whole cluster upright. */
    {
        uint32_t cl[3];
        cl[0] = 0x0061u;                /* 'a' -> R */
        cl[1] = 0x0301u;
        cl[2] = 0x20DDu;                /* combining enclosing circle, Me */
        CHECK(suas_suvtl_cluster_orientation(cl, 3u) == SUVTL_ORIENT_U);
    }

    /* Me detection is itself exposed and curated. */
    CHECK(suas_suvtl_is_enclosing_mark(0x20DDu));
    CHECK(suas_suvtl_is_enclosing_mark(0xFE20u));
    CHECK(suas_suvtl_is_enclosing_mark(0x0488u));
    CHECK(!suas_suvtl_is_enclosing_mark(0x0301u));
    CHECK(!suas_suvtl_is_enclosing_mark(0x0061u));
}

/* ── Enumeration and table self-consistency ───────────────────────── */

static void test_enumeration(void)
{
    uint32_t n = suas_suvtl_entry_count();
    uint32_t i;
    uint32_t squared = 0u;
    uint32_t custom = 0u;
    suvtl_glyph_layout_t e;
    suvtl_glyph_layout_t prev;

    CHECK(n > 0u);
    CHECK(n <= SUAS_SUVTL_REGISTRY_CAPACITY);

    for (i = 0u; i < n; i++) {
        CHECK(suas_suvtl_entry_at(i, &e));
        CHECK(suas_suvtl_is_registered(e.codepoint));
        CHECK(suas_suvtl_is_text_space(e.codepoint));
        CHECK(e.reserved == 0u);
        CHECK((e.orientation_flags & (uint16_t)~SUVTL_FLAG_ASSIGNED_MASK) == 0u);
        CHECK((e.orientation_flags & SUVTL_ORIENT_MASK) <= SUVTL_ORIENT_TR);

        /* Ascending, unique codepoints. */
        if (i > 0u) {
            CHECK(suas_suvtl_entry_at(i - 1u, &prev));
            CHECK(prev.codepoint < e.codepoint);
        }

        if ((e.orientation_flags & SUVTL_FLAG_SQUARED) != 0u)
            squared++;
        if ((e.orientation_flags & SUVTL_FLAG_CUSTOM_FLOW) != 0u)
            custom++;
    }
    CHECK(squared > 0u && custom > 0u);

    CHECK(!suas_suvtl_entry_at(n, &e));
    CHECK(!suas_suvtl_entry_at(0u, NULL));
}

static void test_init_idempotent(void)
{
    suvtl_glyph_layout_t a;
    suvtl_glyph_layout_t b;

    suas_suvtl_init_engine();
    suas_suvtl_init_engine();
    CHECK(suas_suvtl_get_layout_properties(0xFF0Cu, &a));
    suas_suvtl_init_engine();
    CHECK(suas_suvtl_get_layout_properties(0xFF0Cu, &b));
    CHECK(a.codepoint == b.codepoint &&
          a.orientation_flags == b.orientation_flags);
}

/* ── Verification (§9) ────────────────────────────────────────────── */

static void test_validate_table(void)
{
    suvtl_glyph_layout_t good[3];
    suvtl_glyph_layout_t bad[3];
    suvtl_registry_error_t err = (suvtl_registry_error_t)-1;

    /* An empty registry is conformant, with or without a pointer. */
    CHECK(suas_suvtl_validate_table(NULL, 0u, &err));
    CHECK(err == SUVTL_REGISTRY_OK);
    CHECK(suas_suvtl_validate_table(good, 0u, NULL));

    /* A well-formed pair. */
    good[0].codepoint = 0x00003001u;
    good[0].orientation_flags =
        (uint16_t)(SUVTL_ORIENT_TR | SUVTL_ORIENT_ROTATED |
                   SUVTL_ORIENT_TRANSFORM);
    good[0].baseline_offset = 0u;
    good[0].reserved = 0u;
    good[1].codepoint = 0x00003300u;
    good[1].orientation_flags =
        (uint16_t)(SUVTL_ORIENT_TU | SUVTL_ORIENT_UPRIGHT |
                   SUVTL_ORIENT_TRANSFORM | SUVTL_FLAG_SQUARED);
    good[1].baseline_offset = 0u;
    good[1].reserved = 0u;
    CHECK(suas_suvtl_validate_table(good, 2u, &err));
    CHECK(err == SUVTL_REGISTRY_OK);

    /* NULL_TABLE: a count with no pointer. */
    CHECK(!suas_suvtl_validate_table(NULL, 1u, &err));
    CHECK(err == SUVTL_REGISTRY_NULL_TABLE);

    /* COUNT_OVER. */
    CHECK(!suas_suvtl_validate_table(good, SUAS_SUVTL_REGISTRY_CAPACITY + 1u,
                                     &err));
    CHECK(err == SUVTL_REGISTRY_COUNT_OVER);

    /* OUT_OF_SPACE: above the 31-bit Base space. */
    bad[0] = good[0];
    bad[0].codepoint = 0x80000000u;
    CHECK(!suas_suvtl_validate_table(bad, 1u, &err));
    CHECK(err == SUVTL_REGISTRY_OUT_OF_SPACE);

    /* INADMISSIBLE_CODEPOINT: the Control Plane. */
    bad[0] = good[0];
    bad[0].codepoint = 0x00110000u;
    CHECK(!suas_suvtl_validate_table(bad, 1u, &err));
    CHECK(err == SUVTL_REGISTRY_INADMISSIBLE_CODEPOINT);

    /* BAD_FLAGS: a reserved bit. */
    bad[0] = good[0];
    bad[0].orientation_flags = (uint16_t)0x8000u;
    CHECK(!suas_suvtl_validate_table(bad, 1u, &err));
    CHECK(err == SUVTL_REGISTRY_BAD_FLAGS);

    /* BAD_ORIENTATION: TRANSFORM with no base orientation bit. */
    bad[0] = good[0];
    bad[0].orientation_flags = (uint16_t)(SUVTL_ORIENT_TRANSFORM);
    CHECK(!suas_suvtl_validate_table(bad, 1u, &err));
    CHECK(err == SUVTL_REGISTRY_BAD_ORIENTATION);

    /* BAD_ORIENTATION: no orientation bit at all. */
    bad[0] = good[0];
    bad[0].orientation_flags = 0u;
    CHECK(!suas_suvtl_validate_table(bad, 1u, &err));
    CHECK(err == SUVTL_REGISTRY_BAD_ORIENTATION);

    /* BAD_ORIENTATION: both base orientation bits set at once. */
    bad[0] = good[0];
    bad[0].orientation_flags =
        (uint16_t)(SUVTL_ORIENT_UPRIGHT | SUVTL_ORIENT_ROTATED);
    CHECK(!suas_suvtl_validate_table(bad, 1u, &err));
    CHECK(err == SUVTL_REGISTRY_BAD_ORIENTATION);

    /* BAD_RESERVED. */
    bad[0] = good[0];
    bad[0].reserved = 1u;
    CHECK(!suas_suvtl_validate_table(bad, 1u, &err));
    CHECK(err == SUVTL_REGISTRY_BAD_RESERVED);

    /* CUSTOM_FLOW_MISMATCH: custom flow outside the RNUR set. */
    bad[0] = good[0];
    bad[0].orientation_flags = (uint16_t)(good[0].orientation_flags |
                                          SUVTL_FLAG_CUSTOM_FLOW);
    CHECK(!suas_suvtl_validate_table(bad, 1u, &err));
    CHECK(err == SUVTL_REGISTRY_CUSTOM_FLOW_MISMATCH);
    /* ...but it is fine inside the RNUR set. */
    bad[0] = good[0];
    bad[0].codepoint = 0x00160100u;
    bad[0].orientation_flags = (uint16_t)(SUVTL_ORIENT_U |
                                          SUVTL_ORIENT_UPRIGHT |
                                          SUVTL_FLAG_CUSTOM_FLOW);
    CHECK(suas_suvtl_validate_table(bad, 1u, &err));
    CHECK(err == SUVTL_REGISTRY_OK);

    /* UNORDERED and DUPLICATE_CODEPOINT. */
    bad[0] = good[1];
    bad[1] = good[0];
    CHECK(!suas_suvtl_validate_table(bad, 2u, &err));
    CHECK(err == SUVTL_REGISTRY_UNORDERED);
    bad[0] = good[0];
    bad[1] = good[0];
    CHECK(!suas_suvtl_validate_table(bad, 2u, &err));
    CHECK(err == SUVTL_REGISTRY_DUPLICATE_CODEPOINT);

    /* The shipped table passes its own verifier. */
    {
        uint32_t i;
        suvtl_glyph_layout_t all[SUAS_SUVTL_REGISTRY_CAPACITY];
        for (i = 0u; i < suas_suvtl_entry_count(); i++)
            CHECK(suas_suvtl_entry_at(i, &all[i]));
        CHECK(suas_suvtl_validate_table(all, suas_suvtl_entry_count(), &err));
        CHECK(err == SUVTL_REGISTRY_OK);
    }
}

/* ── Taxonomy names ───────────────────────────────────────────────── */

static void test_taxonomy(void)
{
    CHECK(suas_suvtl_orientation_name(SUVTL_ORIENT_U)[0] == 'U');
    CHECK(suas_suvtl_orientation_name(SUVTL_ORIENT_R)[0] == 'R');
    CHECK(suas_suvtl_orientation_name(SUVTL_ORIENT_TU)[0] == 'T');
    CHECK(suas_suvtl_orientation_name(SUVTL_ORIENT_TU)[1] == 'u');
    CHECK(suas_suvtl_orientation_name(SUVTL_ORIENT_TR)[1] == 'r');
    CHECK(suas_suvtl_orientation_long_name(SUVTL_ORIENT_U)[0] == 'U');
    CHECK(suas_suvtl_orientation_long_name(SUVTL_ORIENT_R)[0] == 'R');

    CHECK(suas_suvtl_group_name(SUAS_SUVTL_GROUP_OTHER)[0] == 'O');
    CHECK(suas_suvtl_group_name(SUAS_SUVTL_GROUP_NATIVE)[0] == 'N');

    CHECK(suas_suvtl_registry_error_name(SUVTL_REGISTRY_OK)[0] == 'O');
    CHECK(suas_suvtl_registry_error_name(SUVTL_REGISTRY_UNORDERED)[0] == 'U');
    CHECK(suas_suvtl_registry_error_name((suvtl_registry_error_t)99)[0] == 'U');
}

/* ── SSRE registration (§10) ──────────────────────────────────────── */

static void test_ssre_registration(void)
{
    uint8_t major = 0u;
    uint8_t minor = 0u;

    CHECK(suas_ssre_is_registered(SUAS_SUVTL_SPEC_ID));
    CHECK(suas_ssre_is_mandatory(SUAS_SUVTL_SPEC_ID));
    CHECK(suas_ssre_version_of(SUAS_SUVTL_SPEC_ID, &major, &minor));
    CHECK(major == 1u && minor == 0u);
    CHECK(suas_ssre_satisfies(SUAS_SUVTL_SPEC_ID, 1u, 0u));
    CHECK(suas_ssre_module_of(SUAS_SUVTL_SPEC_ID) != NULL);
    CHECK(suas_ssre_module_of(SUAS_SUVTL_SPEC_ID)[0] == 's');
}

int main(void)
{
    test_version_and_meta();
    test_zones();
    test_neography_set();
    test_hash();
    test_orientation_values();
    test_derived_bits_consistent();
    test_groups_and_default();
    test_baseline();
    test_cluster_rule();
    test_enumeration();
    test_init_idempotent();
    test_validate_table();
    test_taxonomy();
    test_ssre_registration();

    if (g_fail) {
        printf("SUAS-014 SUVTL tests FAILED\n");
        return 1;
    }
    printf("SUAS-014 SUVTL tests passed\n");
    return 0;
}
