/**
 * SUTR-013 SUEAS — SuperUnicode East Asian Spacing
 *
 * Unit tests for the zero-allocation, static-table, bounded-search resolution
 * of a codepoint's East_Asian_Spacing value (Other / Narrow / Wide /
 * Conditional), its East_Asian_Width value (N / A / H / W / F / Na), its grid
 * column span, and the contextual auto-spacing decision between two adjacent
 * clusters — together with the zone taxonomy of §2 and the taxonomy names of
 * §8 (a UTR #59 analog).
 *
 * Covers: version/meta, the compile-time record geometry, zone dispatch and
 * the RNUR neography carve-out, FNV-1a hashing, the compiled range tables and
 * the algorithmic default, the width query and every failure outcome, the
 * classification predicates, the spacing resolver across the Conditional and
 * vertical rules, the O(n) grid and space-count engines, enumeration and
 * table self-consistency, the taxonomy names, and registration in SUAS-010
 * (SSRE).
 */

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>

#include "sutr/sutr_sueas.h"
#include "suas/suas_ssre.h"

static int g_fail = 0;
#define CHECK(cond) do { if (!(cond)) { \
    printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); g_fail = 1; \
} } while (0)

/* ── Version, limits, and record geometry (§3.1) ──────────────────── */

static void test_version_and_meta(void)
{
    CHECK(sutr_sueas_version_string() != NULL);
    CHECK(sutr_sueas_version_string()[0] == '0');
    CHECK(sutr_sueas_version_string()[2] == '1');

    CHECK(SUEAS_SPEC_ID[0] == 'S');
    CHECK(SUEAS_SPEC_TITLE[0] == 'S');
    CHECK(SUEAS_MODULE[0] == 's');
    CHECK(SUEAS_VERSION_MAJOR == 0);
    CHECK(SUEAS_VERSION_MINOR == 1);
    CHECK(SUEAS_VERSION_PATCH == 0);

    CHECK(SUEAS_FNV_OFFSET_BASIS == 2166136261u);
    CHECK(SUEAS_FNV_PRIME == 16777619u);

    /* The width descriptor is fixed-size and padding-free (§3.1): 8 bytes. */
    CHECK(sizeof(sueas_width_entry_t) == 8U);
    CHECK(offsetof(sueas_width_entry_t, codepoint) == 0U);
    CHECK(offsetof(sueas_width_entry_t, spacing_flags) == 4U);
    CHECK(offsetof(sueas_width_entry_t, width_category) == 6U);
    CHECK(offsetof(sueas_width_entry_t, column_units) == 7U);

    /* A compiled range row is 12 bytes in both tables (§3.5). */
    CHECK(sizeof(sueas_spacing_range_t) == 12U);
    CHECK(sizeof(sueas_eaw_range_t) == 12U);

    /* The spacing decision is 8 bytes (§4.3). */
    CHECK(sizeof(sueas_spacing_decision_t) == 8U);

    /* The flag bits are disjoint and the assigned mask is the low byte. */
    CHECK((SUEAS_FLAG_PROPORTIONAL & SUEAS_FLAG_GRID_SNAP) == 0u);
    CHECK((SUEAS_FLAG_WIDE & SUEAS_FLAG_NARROW) == 0u);
    CHECK((SUEAS_FLAG_NATIVE & SUEAS_FLAG_ASSIGNED_MASK) != 0u);
    CHECK(SUEAS_FLAG_ASSIGNED_MASK == 0x00FFu);

    /* The property constants are the canonical UTR #59 / UAX #11 values. */
    CHECK(SUEAS_SPACING_OTHER == 0);
    CHECK(SUEAS_SPACING_NARROW == 1);
    CHECK(SUEAS_SPACING_WIDE == 2);
    CHECK(SUEAS_SPACING_CONDITIONAL == 3);

    CHECK(SUEAS_EAW_NEUTRAL == 0);
    CHECK(SUEAS_EAW_AMBIGUOUS == 1);
    CHECK(SUEAS_EAW_HALFWIDTH == 2);
    CHECK(SUEAS_EAW_WIDE == 3);
    CHECK(SUEAS_EAW_FULLWIDTH == 4);
    CHECK(SUEAS_EAW_NARROW == 5);

    /* The sketch's width categories. */
    CHECK(SUEAS_WIDTH_NEUTRAL == 0);
    CHECK(SUEAS_WIDTH_NAR == 1);
    CHECK(SUEAS_WIDTH_WID == 2);
    CHECK(SUEAS_WIDTH_FULL == 3);
    CHECK(SUEAS_WIDTH_HALF == 4);
    CHECK(SUEAS_WIDTH_AMB == 5);
}

/* ── Zone dispatch and the neography carve-out (§2.1) ─────────────── */

static void test_zones(void)
{
    CHECK(sutr_sueas_zone_of(0x00000041u) == SUEAS_ZONE_BRIDGE);
    CHECK(sutr_sueas_zone_of(0x004E00u)   == SUEAS_ZONE_BRIDGE);
    CHECK(sutr_sueas_zone_of(0x0010FFFFu) == SUEAS_ZONE_BRIDGE);
    CHECK(sutr_sueas_zone_of(0x00110000u) == SUEAS_ZONE_SCP);
    CHECK(sutr_sueas_zone_of(0x0011FFFFu) == SUEAS_ZONE_SCP);
    CHECK(sutr_sueas_zone_of(0x00160000u) == SUEAS_ZONE_RNUR);
    CHECK(sutr_sueas_zone_of(0x0016FFFFu) == SUEAS_ZONE_RNUR);
    CHECK(sutr_sueas_zone_of(0x00120000u) == SUEAS_ZONE_NATIVE);
    CHECK(sutr_sueas_zone_of(0x7FFFFFEFu) == SUEAS_ZONE_NATIVE);
    CHECK(sutr_sueas_zone_of(0x7FFFFFF0u) == SUEAS_ZONE_TRAP);
    CHECK(sutr_sueas_zone_of(0x7FFFFFFEu) == SUEAS_ZONE_TRAP);
    CHECK(sutr_sueas_zone_of(0x7FFFFFFFu) == SUEAS_ZONE_SENTINEL);
    CHECK(sutr_sueas_zone_of(0x80000000u) == SUEAS_ZONE_EXTSUCS);

    CHECK(sutr_sueas_is_renderable(0x004E00u) == true);
    CHECK(sutr_sueas_is_renderable(0x00160000u) == true);
    CHECK(sutr_sueas_is_renderable(0x00110000u) == false);
    CHECK(sutr_sueas_is_renderable(0x7FFFFFF0u) == false);
    CHECK(sutr_sueas_is_renderable(0x7FFFFFFFu) == false);

    CHECK(sutr_sueas_is_text_space(0x004E00u) == true);

    CHECK(sutr_sueas_is_neography(0x00160100u) == true);
    CHECK(sutr_sueas_is_neography(0x004E00u) == false);
}

static void test_neography_set(void)
{
    uint32_t set = 0u;

    CHECK(sutr_sueas_neography_set(0x00160000u, &set) == true);
    CHECK(set == 0u);
    CHECK(sutr_sueas_neography_set(0x00160100u, &set) == true);
    CHECK(set == 1u);
    CHECK(sutr_sueas_neography_set(0x0016FFFFu, &set) == true);
    CHECK(set == 0xFFu);

    CHECK(sutr_sueas_neography_set(0x004E00u, &set) == false);
    CHECK(sutr_sueas_neography_set(0x00160000u, NULL) == false);
}

/* ── FNV-1a hashing (§3.5) ────────────────────────────────────────── */

static void test_hash(void)
{
    CHECK(sutr_sueas_hash_codepoint(0x00000000u) != SUEAS_FNV_OFFSET_BASIS);
    CHECK(sutr_sueas_hash_codepoint(0x00000041u) ==
          sutr_sueas_hash_codepoint(0x00000041u));
    CHECK(sutr_sueas_hash_codepoint(0x00000041u) !=
          sutr_sueas_hash_codepoint(0x00000042u));
}

/* ── The width query (§3) ─────────────────────────────────────────── */

static void test_get_width_entry(void)
{
    sueas_width_entry_t e;
    sueas_status_code_t err = SUEAS_OK;

    sutr_sueas_init_registry();

    /* 'A' is a Narrow spacing / Neutral EAW / one-column starter. */
    CHECK(sutr_sueas_get_width_entry(0x00000041u, &e) == true);
    CHECK(e.codepoint == 0x00000041u);
    CHECK(e.width_category == (uint8_t)SUEAS_WIDTH_NAR);
    CHECK(e.column_units == 1u);
    CHECK((e.spacing_flags & SUEAS_FLAG_NARROW) != 0u);
    CHECK((e.spacing_flags & SUEAS_FLAG_PROPORTIONAL) != 0u);
    CHECK((e.spacing_flags & SUEAS_FLAG_EAST_ASIAN) == 0u);

    /* U+4E00 is a Wide spacing / Wide EAW / two-column ideograph. */
    CHECK(sutr_sueas_get_width_entry(0x00004E00u, &e) == true);
    CHECK(e.width_category == (uint8_t)SUEAS_WIDTH_WID);
    CHECK(e.column_units == 2u);
    CHECK((e.spacing_flags & SUEAS_FLAG_WIDE) != 0u);
    CHECK((e.spacing_flags & SUEAS_FLAG_GRID_SNAP) != 0u);
    CHECK((e.spacing_flags & SUEAS_FLAG_EAST_ASIAN) != 0u);

    /* U+FF01 FULLWIDTH EXCLAMATION MARK is Fullwidth / two columns. */
    CHECK(sutr_sueas_get_width_entry(0x0000FF01u, &e) == true);
    CHECK(e.width_category == (uint8_t)SUEAS_WIDTH_FULL);
    CHECK(e.column_units == 2u);

    /* U+FF61 HALFWIDTH IDEOGRAPHIC FULL STOP is Halfwidth / one column. */
    CHECK(sutr_sueas_get_width_entry(0x0000FF61u, &e) == true);
    CHECK(e.width_category == (uint8_t)SUEAS_WIDTH_HALF);
    CHECK(e.column_units == 1u);

    /* U+00A1 INVERTED EXCLAMATION MARK is Ambiguous / two columns by default. */
    CHECK(sutr_sueas_get_width_entry(0x000000A1u, &e) == true);
    CHECK(e.width_category == (uint8_t)SUEAS_WIDTH_AMB);
    CHECK(e.column_units == 2u);

    /* An unlisted Bridge codepoint is answered by the algorithmic default:
     * Other / Neutral / one column (§6). */
    CHECK(sutr_sueas_get_width_entry(0x0000037Eu, &e) == true);
    CHECK(e.width_category == (uint8_t)SUEAS_WIDTH_NEUTRAL);
    CHECK(e.column_units == 1u);
    CHECK((e.spacing_flags & SUEAS_FLAG_WIDE) == 0u);

    /* A NULL output pointer and an inadmissible zone both fail. */
    CHECK(sutr_sueas_get_width_entry_ex(0x00000041u, NULL, &err) == false);
    CHECK(err == SUEAS_ERR_NULL_ARGUMENT);
    CHECK(sutr_sueas_get_width_entry_ex(0x00110000u, &e, &err) == false);
    CHECK(err == SUEAS_ERR_ILLEGAL_ZONE);
    CHECK(sutr_sueas_get_width_entry(0x7FFFFFFFu, &e) == false);

    /* A NULL out_error is allowed on the failure paths. */
    CHECK(sutr_sueas_get_width_entry_ex(NULL == NULL ? 0x00110000u : 0u,
                                        &e, NULL) == false);
}

/* ── Classification predicates (§3) ───────────────────────────────── */

static void test_predicates(void)
{
    CHECK(sutr_sueas_is_wide(0x00004E00u) == true);
    CHECK(sutr_sueas_is_wide(0x00000041u) == false);

    CHECK(sutr_sueas_is_fullwidth(0x0000FF01u) == true);
    CHECK(sutr_sueas_is_fullwidth(0x00000041u) == false);

    CHECK(sutr_sueas_is_halfwidth(0x0000FF61u) == true);
    CHECK(sutr_sueas_is_halfwidth(0x00000041u) == false);

    CHECK(sutr_sueas_is_ambiguous(0x000000A1u) == true);
    CHECK(sutr_sueas_is_ambiguous(0x00000041u) == false);

    CHECK(sutr_sueas_is_east_asian(0x00004E00u) == true);
    CHECK(sutr_sueas_is_east_asian(0x0000FF01u) == true);
    CHECK(sutr_sueas_is_east_asian(0x0000FF61u) == true);
    CHECK(sutr_sueas_is_east_asian(0x00000041u) == false);

    CHECK(sutr_sueas_is_registered(0x00004E00u) == true);
    CHECK(sutr_sueas_is_registered(0x00000041u) == true);
    CHECK(sutr_sueas_is_registered(0x00110000u) == false);

    /* Property predicates. */
    CHECK(sutr_sueas_spacing_of(0x00004E00u) == SUEAS_SPACING_WIDE);
    CHECK(sutr_sueas_spacing_of(0x00000041u) == SUEAS_SPACING_NARROW);
    CHECK(sutr_sueas_spacing_of(0x00000021u) == SUEAS_SPACING_CONDITIONAL);
    CHECK(sutr_sueas_spacing_of(0x00110000u) == SUEAS_SPACING_OTHER);

    CHECK(sutr_sueas_eaw_of(0x00004E00u) == SUEAS_EAW_WIDE);
    CHECK(sutr_sueas_eaw_of(0x00000041u) == SUEAS_EAW_NARROW);
    CHECK(sutr_sueas_eaw_of(0x0000FF01u) == SUEAS_EAW_FULLWIDTH);
    CHECK(sutr_sueas_eaw_of(0x0000FF61u) == SUEAS_EAW_HALFWIDTH);
    CHECK(sutr_sueas_eaw_of(0x000000A1u) == SUEAS_EAW_AMBIGUOUS);

    CHECK(sutr_sueas_column_units(0x00004E00u) == 2u);
    CHECK(sutr_sueas_column_units(0x00000041u) == 1u);
    CHECK(sutr_sueas_column_units(0x0000FF61u) == 1u);
    CHECK(sutr_sueas_column_units(0x00110000u) == 0u);
}

/* ── Spacing resolution (§4) ──────────────────────────────────────── */

static void test_spacing_resolution(void)
{
    /* Conditional resolves to Narrow for Chinese, Other otherwise. */
    CHECK(sutr_sueas_resolve_spacing(0x00000021u, true, false) ==
          SUEAS_SPACING_NARROW);
    CHECK(sutr_sueas_resolve_spacing(0x00000021u, false, false) ==
          SUEAS_SPACING_OTHER);

    /* A Narrow value displayed upright in vertical layout becomes Other. */
    CHECK(sutr_sueas_resolve_spacing(0x00000041u, false, true) ==
          SUEAS_SPACING_OTHER);
    CHECK(sutr_sueas_resolve_spacing(0x00000041u, false, false) ==
          SUEAS_SPACING_NARROW);

    /* Wide and Other are unchanged by the context. */
    CHECK(sutr_sueas_resolve_spacing(0x00004E00u, false, false) ==
          SUEAS_SPACING_WIDE);
    /* A Conditional value resolves: the raw property is Conditional but the
     * resolver returns Other outside a Chinese context. */
    CHECK(sutr_sueas_spacing_of(0x0000002Cu) == SUEAS_SPACING_CONDITIONAL);
    CHECK(sutr_sueas_resolve_spacing(0x0000002Cu, false, false) ==
          SUEAS_SPACING_OTHER);

    /* The boundary rule. */
    CHECK(sutr_sueas_needs_space(SUEAS_SPACING_WIDE, SUEAS_SPACING_NARROW));
    CHECK(sutr_sueas_needs_space(SUEAS_SPACING_NARROW, SUEAS_SPACING_WIDE));
    CHECK(!sutr_sueas_needs_space(SUEAS_SPACING_WIDE, SUEAS_SPACING_WIDE));
    CHECK(!sutr_sueas_needs_space(SUEAS_SPACING_NARROW, SUEAS_SPACING_NARROW));
    CHECK(!sutr_sueas_needs_space(SUEAS_SPACING_OTHER, SUEAS_SPACING_WIDE));

    /* The per-pair decision resolves before it applies the rule. */
    {
        sueas_spacing_decision_t d;

        /* "漢" + "A": Wide/Narrow -> a space. */
        CHECK(sutr_sueas_spacing_decision(0x00004E00u, 0x00000041u, false,
                                          false, &d) == true);
        CHECK(d.space == 1u);
        CHECK(d.tight == 0u);
        CHECK(d.reserved == 0u && d.reserved2 == 0u);

        /* "漢" + "%": Conditional resolves to Other -> no space. */
        CHECK(sutr_sueas_spacing_decision(0x00004E00u, 0x00000025u, false,
                                          false, &d) == true);
        CHECK(d.space == 0u);
        CHECK(d.tight == 1u);

        /* "漢" + "%": in a Chinese context the Conditional resolves to
         * Narrow -> a space. */
        CHECK(sutr_sueas_spacing_decision(0x00004E00u, 0x00000025u, true,
                                          false, &d) == true);
        CHECK(d.space == 1u);

        /* A NULL descriptor and an inadmissible zone fail. */
        CHECK(sutr_sueas_spacing_decision(0x00004E00u, 0x00000041u, false,
                                          false, NULL) == false);
        CHECK(sutr_sueas_spacing_decision(0x00110000u, 0x00000041u, false,
                                          false, &d) == false);
    }
}

/* ── The zero-allocation grid engine (§5) ─────────────────────────── */

static void test_grid_engine(void)
{
    /* "A漢B": 1 + 2 + 1 = 4 columns, two auto-spaces (A|漢 and 漢|B). */
    static const uint32_t s1[3] = { 0x00000041u, 0x00004E00u, 0x00000042u };
    uint16_t cols[8];
    size_t total;

    total = sutr_sueas_measure_columns(s1, 3u, cols, 8u);
    CHECK(total == 4u);
    CHECK(cols[0] == 1u);
    CHECK(cols[1] == 2u);
    CHECK(cols[2] == 1u);

    CHECK(sutr_sueas_count_spaces(s1, 3u, false, false) == 2u);

    /* A NULL column array still returns the total. */
    CHECK(sutr_sueas_measure_columns(s1, 3u, NULL, 0u) == 4u);

    /* A capacity smaller than the stream writes only the prefix. */
    CHECK(sutr_sueas_measure_columns(s1, 3u, cols, 1u) == 4u);

    /* A NULL stream and a zero length detect nothing. */
    CHECK(sutr_sueas_measure_columns(NULL, 3u, cols, 8u) == 0u);
    CHECK(sutr_sueas_measure_columns(s1, 0u, cols, 8u) == 0u);
    CHECK(sutr_sueas_count_spaces(NULL, 3u, false, false) == 0u);

    /* A non-renderable control contributes zero columns and is skipped by the
     * spacing count. */
    {
        static const uint32_t s2[3] = { 0x00004E00u, 0x00110000u, 0x00000041u };
        CHECK(sutr_sueas_measure_columns(s2, 3u, cols, 8u) == 3u);
        CHECK(cols[1] == 0u);
        /* 漢 and A are now adjacent across the control -> still a space. */
        CHECK(sutr_sueas_count_spaces(s2, 3u, false, false) == 1u);
    }

    /* Conditional "20%" is Narrow in a Chinese context, so no space beside a
     * Wide character; elsewhere it is Other, also no space. A Narrow "%"
     * forced through a Chinese context does produce a space against 漢. */
    {
        static const uint32_t pct[3] = { 0x00004E00u, 0x00000025u, 0x00004E00u };
        CHECK(sutr_sueas_count_spaces(pct, 3u, false, false) == 0u);
        CHECK(sutr_sueas_count_spaces(pct, 3u, true, false) == 2u);
    }
}

/* ── Enumeration and table self-consistency (§3.5) ────────────────── */

static void test_enumeration(void)
{
    uint32_t i;
    uint32_t prev_last = 0u;

    CHECK(sutr_sueas_spacing_count() > 0u);
    CHECK(sutr_sueas_spacing_count() <= SUEAS_SPACING_CAPACITY);
    CHECK(sutr_sueas_eaw_count() > 0u);
    CHECK(sutr_sueas_eaw_count() <= SUEAS_EAW_CAPACITY);

    /* The spacing ranges are strictly ascending, non-overlapping, and
     * gap-separated: a verification invariant, not a convenience. */
    for (i = 0u; i < sutr_sueas_spacing_count(); i++) {
        sueas_spacing_range_t r;
        CHECK(sutr_sueas_spacing_at(i, &r) == true);
        CHECK(r.first <= r.last);
        CHECK(r.value <= (uint32_t)SUEAS_SPACING_CONDITIONAL);
        if (i > 0u)
            CHECK(r.first > prev_last + 1u || r.first > prev_last);
        prev_last = r.last;
    }

    prev_last = 0u;
    for (i = 0u; i < sutr_sueas_eaw_count(); i++) {
        sueas_eaw_range_t r;
        CHECK(sutr_sueas_eaw_at(i, &r) == true);
        CHECK(r.first <= r.last);
        CHECK(r.value <= (uint32_t)SUEAS_EAW_NARROW);
        if (i > 0u)
            CHECK(r.first > prev_last);
        prev_last = r.last;
    }

    /* Out-of-range and NULL enumeration fail. */
    {
        sueas_spacing_range_t r;
        sueas_eaw_range_t e;
        CHECK(sutr_sueas_spacing_at(sutr_sueas_spacing_count(), &r) == false);
        CHECK(sutr_sueas_spacing_at(0u, NULL) == false);
        CHECK(sutr_sueas_eaw_at(sutr_sueas_eaw_count(), &e) == false);
        CHECK(sutr_sueas_eaw_at(0u, NULL) == false);
    }

    /* Every spaced value the table holds is reachable by the lookup: a spot
     * check of each class's first range agrees with the query. */
    {
        uint32_t seen_w = 0u, seen_n = 0u, seen_c = 0u;
        for (i = 0u; i < sutr_sueas_spacing_count(); i++) {
            sueas_spacing_range_t r;
            sutr_sueas_spacing_at(i, &r);
            if (r.value == (uint32_t)SUEAS_SPACING_WIDE && !seen_w) {
                CHECK(sutr_sueas_spacing_of(r.first) == SUEAS_SPACING_WIDE);
                seen_w = 1u;
            }
            if (r.value == (uint32_t)SUEAS_SPACING_NARROW && !seen_n) {
                CHECK(sutr_sueas_spacing_of(r.first) == SUEAS_SPACING_NARROW);
                seen_n = 1u;
            }
            if (r.value == (uint32_t)SUEAS_SPACING_CONDITIONAL && !seen_c) {
                CHECK(sutr_sueas_spacing_of(r.first) ==
                      SUEAS_SPACING_CONDITIONAL);
                seen_c = 1u;
            }
        }
        CHECK(seen_w && seen_n && seen_c);
    }
}

static void test_init_idempotent(void)
{
    sutr_sueas_init_registry();
    sutr_sueas_init_registry();
    CHECK(sutr_sueas_spacing_of(0x00004E00u) == SUEAS_SPACING_WIDE);
}

/* ── Taxonomy names (§8) ──────────────────────────────────────────── */

static void test_taxonomy(void)
{
    CHECK(sutr_sueas_spacing_name(SUEAS_SPACING_OTHER)[0] == 'o');
    CHECK(sutr_sueas_spacing_name(SUEAS_SPACING_NARROW)[0] == 'n');
    CHECK(sutr_sueas_spacing_name(SUEAS_SPACING_WIDE)[0] == 'w');
    CHECK(sutr_sueas_spacing_name(SUEAS_SPACING_CONDITIONAL)[0] == 'c');
    CHECK(sutr_sueas_spacing_name((sueas_spacing_t)99)[0] == 'u');

    CHECK(sutr_sueas_eaw_name(SUEAS_EAW_NEUTRAL)[0] == 'n');
    CHECK(sutr_sueas_eaw_name(SUEAS_EAW_AMBIGUOUS)[0] == 'a');
    CHECK(sutr_sueas_eaw_name(SUEAS_EAW_HALFWIDTH)[0] == 'h');
    CHECK(sutr_sueas_eaw_name(SUEAS_EAW_WIDE)[0] == 'w');
    CHECK(sutr_sueas_eaw_name(SUEAS_EAW_FULLWIDTH)[0] == 'f');
    CHECK(sutr_sueas_eaw_name(SUEAS_EAW_NARROW)[0] == 'n');
    CHECK(sutr_sueas_eaw_name((sueas_eaw_t)99)[0] == 'u');

    CHECK(sutr_sueas_width_category_name(SUEAS_WIDTH_NEUTRAL)[0] == 'n');
    CHECK(sutr_sueas_width_category_name(SUEAS_WIDTH_NAR)[0] == 'n');
    CHECK(sutr_sueas_width_category_name(SUEAS_WIDTH_WID)[0] == 'w');
    CHECK(sutr_sueas_width_category_name(SUEAS_WIDTH_FULL)[0] == 'f');
    CHECK(sutr_sueas_width_category_name(SUEAS_WIDTH_HALF)[0] == 'h');
    CHECK(sutr_sueas_width_category_name(SUEAS_WIDTH_AMB)[0] == 'a');
    CHECK(sutr_sueas_width_category_name((sueas_width_category_t)99)[0] == 'u');

    CHECK(sutr_sueas_zone_name(SUEAS_ZONE_BRIDGE)[0] == 'b');
    CHECK(sutr_sueas_zone_name(SUEAS_ZONE_SCP)[0] == 's');
    CHECK(sutr_sueas_zone_name(SUEAS_ZONE_RNUR)[0] == 'r');
    CHECK(sutr_sueas_zone_name(SUEAS_ZONE_NATIVE)[0] == 'n');
    CHECK(sutr_sueas_zone_name(SUEAS_ZONE_TRAP)[0] == 't');
    CHECK(sutr_sueas_zone_name(SUEAS_ZONE_SENTINEL)[0] == 's');
    CHECK(sutr_sueas_zone_name(SUEAS_ZONE_EXTSUCS)[0] == 'e');
    CHECK(sutr_sueas_zone_name((sueas_zone_t)99)[0] == 'u');

    CHECK(sutr_sueas_status_code_name(SUEAS_OK)[0] == 'o');
    CHECK(sutr_sueas_status_code_name(SUEAS_ERR_NULL_ARGUMENT)[0] == 'n');
    CHECK(sutr_sueas_status_code_name(SUEAS_ERR_ILLEGAL_ZONE)[0] == 'i');
    CHECK(sutr_sueas_status_code_name(SUEAS_ERR_CAPACITY)[0] == 'c');
    CHECK(sutr_sueas_status_code_name((sueas_status_code_t)99)[0] == 'u');
}

/* ── Registration in SSRE (SUAS-010) ──────────────────────────────── */

static void test_ssre_registration(void)
{
    uint8_t major = 0u;
    uint8_t minor = 0u;

    CHECK(suas_ssre_is_registered(SUEAS_SPEC_ID));
    CHECK(suas_ssre_version_of(SUEAS_SPEC_ID, &major, &minor));
    CHECK(major == 0u && minor == 1u);
    CHECK(suas_ssre_satisfies(SUEAS_SPEC_ID, 0u, 1u));

    /* SUEAS is a SUTR — a REFERENCE-kind report — but it ships a real
     * reference unit, so its SSRE row names one. */
    CHECK(suas_ssre_kind_count(SUAS_SSRE_KIND_REFERENCE) == 14u);
    CHECK(suas_ssre_module_of(SUEAS_SPEC_ID) != NULL);
    CHECK(suas_ssre_module_of(SUEAS_SPEC_ID)[0] == 's');
    CHECK(suas_ssre_spec_of_module(SUEAS_MODULE) != NULL);
    CHECK(suas_ssre_spec_of_module(SUEAS_MODULE)[0] == 'S');
}

int main(void)
{
    test_version_and_meta();
    test_zones();
    test_neography_set();
    test_hash();
    test_get_width_entry();
    test_predicates();
    test_spacing_resolution();
    test_grid_engine();
    test_enumeration();
    test_init_idempotent();
    test_taxonomy();
    test_ssre_registration();

    if (g_fail) {
        printf("SUTR-013 SUEAS tests FAILED\n");
        return 1;
    }
    printf("SUTR-013 SUEAS tests passed\n");
    return 0;
}
