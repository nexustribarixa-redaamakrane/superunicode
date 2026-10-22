/**
 * SUAS-016 SUKEMET - SuperUnicode Egyptian Hieroglyph Database
 *
 * Unit tests for the zero-allocation, static-table, O(1) classification of
 * Egyptian hieroglyphs and format controls - the Gardiner group, the Core /
 * Legacy class, the Unikemet catalogue index, the function mask, the mirror
 * and rotate properties, alternate sequences and the Native/Neography
 * modifier roles (a UAX #57 analog).
 *
 * Covers: version/meta, the compile-time 12-byte record geometry, zone
 * dispatch, the RNUR carve-out, FNV-1a, the catalogue and metadata queries,
 * the Egyptian format controls, the algorithmic default, enumeration and
 * table self-consistency, every verification outcome, the taxonomy names,
 * and registration in SUAS-010 (SSRE).
 */

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <assert.h>

#include "suas/suas_sukemet.h"
#include "suas/suas_ssre.h"

static int g_fail = 0;
#define CHECK(cond) do { if (!(cond)) { \
    printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); g_fail = 1; \
} } while (0)

/* -- Version, limits and record geometry -------------------------------- */

static void test_version_and_meta(void)
{
    CHECK(suas_sukemet_version_string() != NULL);
    CHECK(suas_sukemet_version_string()[0] == '1');
    CHECK(suas_sukemet_version_string()[2] == '0');
    CHECK(SUKEMET_SPEC_ID[0] == 'S');
    CHECK(SUKEMET_SPEC_TITLE[0] == 'S');
    CHECK(SUAS_SUKEMET_SPEC_ID[0] == 'S');
    CHECK(SUKEMET_VERSION_MAJOR == 1);
    CHECK(SUKEMET_VERSION_MINOR == 0);
    CHECK(SUKEMET_VERSION_PATCH == 0);
    CHECK(SUAS_SUKEMET_REGISTRY_CAPACITY == 8192U);
    CHECK(SUAS_SUKEMET_INDEX_SIZE == 16384U);
    CHECK((SUAS_SUKEMET_INDEX_SIZE & (SUAS_SUKEMET_INDEX_SIZE - 1U)) == 0U);
    CHECK(SUAS_SUKEMET_INDEX_SIZE > SUAS_SUKEMET_REGISTRY_CAPACITY);
    CHECK(SUAS_SUKEMET_ALT_MAX == 16U);

    /* Section 3.1: the seeding sketch's exact field order, 12 bytes. */
    CHECK(sizeof(sukemet_hieroglyph_entry_t) == 12U);
    CHECK(offsetof(sukemet_hieroglyph_entry_t, codepoint) == 0U);
    CHECK(offsetof(sukemet_hieroglyph_entry_t, phonetic_hash) == 4U);
    CHECK(offsetof(sukemet_hieroglyph_entry_t, sukemet_flags) == 8U);
    CHECK(offsetof(sukemet_hieroglyph_entry_t, gardiner_category) == 10U);
    CHECK(offsetof(sukemet_hieroglyph_entry_t, sub_index) == 11U);
    CHECK(sizeof(sukemet_metadata_t) == 24U);

    /* The four sketch flags are disjoint and stay inside the assigned mask. */
    CHECK((SUKEMET_FLAG_GARDINER & SUKEMET_FLAG_MODIFIER) == 0U);
    CHECK((SUKEMET_FLAG_ENCLOSURE & SUKEMET_FLAG_COMPOSITE) == 0U);
    CHECK((SUKEMET_FLAG_GARDINER & SUKEMET_FLAG_ASSIGNED_MASK) != 0U);
    CHECK((SUKEMET_FLAG_NEOGRAPHY & SUKEMET_FLAG_ASSIGNED_MASK) != 0U);
    CHECK(SUKEMET_FLAG_ASSIGNED_MASK == 0x7FFFU);
    CHECK(SUKEMET_FLAG_RESERVED_MASK == 0x8000U);
    CHECK((SUKEMET_FLAG_CORE & SUKEMET_FLAG_LEGACY) == 0U);

    CHECK(SUKEMET_GARDINER_A == 0U);
    CHECK(SUKEMET_GARDINER_K == 9U);
    CHECK(SUKEMET_GARDINER_AA == 25U);
    CHECK(SUKEMET_GARDINER_COUNT == 26U);
    CHECK(SUKEMET_GARDINER_NONE == 0xFFU);

    CHECK(SUKEMET_BLOCK_HIEROGLYPHS_MIN == 0x13000UL);
    CHECK(SUKEMET_BLOCK_HIEROGLYPHS_MAX == 0x1342FUL);
    CHECK(SUKEMET_BLOCK_CONTROLS_MIN == 0x13430UL);
    CHECK(SUKEMET_BLOCK_CONTROLS_MAX == 0x1345FUL);
    CHECK(SUKEMET_BLOCK_EXTENDED_A_MIN == 0x13460UL);
    CHECK(SUKEMET_BLOCK_EXTENDED_A_MAX == 0x143FFUL);
    CHECK(SUKEMET_MIRROR_HORIZONTALLY == 0x13440UL);
}

/* -- Zone dispatch and the RNUR carve-out ------------------------------- */

static void test_zones(void)
{
    CHECK(suas_sukemet_zone_of(0x00013000UL) == SUAS_SUKEMET_ZONE_BRIDGE);
    CHECK(suas_sukemet_zone_of(0x0010FFFFUL) == SUAS_SUKEMET_ZONE_BRIDGE);
    CHECK(suas_sukemet_zone_of(0x00110000UL) == SUAS_SUKEMET_ZONE_SCP);
    CHECK(suas_sukemet_zone_of(0x0011FFFFUL) == SUAS_SUKEMET_ZONE_SCP);
    CHECK(suas_sukemet_zone_of(0x00120000UL) == SUAS_SUKEMET_ZONE_NATIVE);
    CHECK(suas_sukemet_zone_of(0x00160000UL) == SUAS_SUKEMET_ZONE_RNUR);
    CHECK(suas_sukemet_zone_of(0x0016FFFFUL) == SUAS_SUKEMET_ZONE_RNUR);
    CHECK(suas_sukemet_zone_of(0x00170000UL) == SUAS_SUKEMET_ZONE_NATIVE);
    CHECK(suas_sukemet_zone_of(0x7FFFFFF0UL) == SUAS_SUKEMET_ZONE_TRAP);
    CHECK(suas_sukemet_zone_of(0x7FFFFFFEUL) == SUAS_SUKEMET_ZONE_TRAP);
    CHECK(suas_sukemet_zone_of(0x7FFFFFFFUL) == SUAS_SUKEMET_ZONE_SENTINEL);
    CHECK(suas_sukemet_zone_of(0x80000000UL) == SUAS_SUKEMET_ZONE_EXTSUCS);

    CHECK(suas_sukemet_is_text_space(0x00013000UL));
    CHECK(suas_sukemet_is_text_space(0x00120000UL));
    CHECK(suas_sukemet_is_text_space(0x00160000UL));
    CHECK(!suas_sukemet_is_text_space(0x00110000UL));
    CHECK(!suas_sukemet_is_text_space(0x7FFFFFF0UL));
    CHECK(!suas_sukemet_is_text_space(0x7FFFFFFFUL));
    CHECK(!suas_sukemet_is_text_space(0x80000000UL));

    CHECK(suas_sukemet_is_neography(0x00160000UL));
    CHECK(suas_sukemet_is_neography(0x0016FFFFUL));
    CHECK(!suas_sukemet_is_neography(0x0012FFFFUL));
    CHECK(!suas_sukemet_is_neography(0x00013000UL));

    {
        uint32_t set = 0xFFFFFFFFu;
        CHECK(suas_sukemet_neography_set(0x00160000UL, &set));
        CHECK(set == 0U);
        CHECK(suas_sukemet_neography_set(0x00160300UL, &set));
        CHECK(set == 3U);
        CHECK(suas_sukemet_neography_set(0x001603FFUL, &set));
        CHECK(set == 3U);
        CHECK(!suas_sukemet_neography_set(0x00120000UL, &set));
        CHECK(!suas_sukemet_neography_set(0x00160000UL, NULL));
    }

    CHECK(suas_sukemet_block_of(0x00013000UL) == SUKEMET_BLOCK_EGYPTIAN);
    CHECK(suas_sukemet_block_of(0x0001342FUL) == SUKEMET_BLOCK_EGYPTIAN);
    CHECK(suas_sukemet_block_of(0x00013430UL) == SUKEMET_BLOCK_FORMAT_CONTROLS);
    CHECK(suas_sukemet_block_of(0x0001345FUL) == SUKEMET_BLOCK_FORMAT_CONTROLS);
    CHECK(suas_sukemet_block_of(0x00013460UL) == SUKEMET_BLOCK_EXTENDED_A);
    CHECK(suas_sukemet_block_of(0x000143FFUL) == SUKEMET_BLOCK_EXTENDED_A);
    CHECK(suas_sukemet_block_of(0x00160300UL) == SUKEMET_BLOCK_NEOGRAPHY);
    CHECK(suas_sukemet_block_of(0x00000041UL) == SUKEMET_BLOCK_NONE);
    CHECK(suas_sukemet_block_of(0x00110000UL) == SUKEMET_BLOCK_NONE);

    CHECK(suas_sukemet_is_hieroglyph(0x00013000UL));
    CHECK(suas_sukemet_is_hieroglyph(0x00013430UL));
    CHECK(suas_sukemet_is_hieroglyph(0x00013460UL));
    CHECK(suas_sukemet_is_hieroglyph(0x00160300UL));
    CHECK(!suas_sukemet_is_hieroglyph(0x00000041UL));
    CHECK(!suas_sukemet_is_hieroglyph(0x00110000UL));
    CHECK(!suas_sukemet_is_hieroglyph(0x7FFFFFF0UL));
}

/* -- FNV-1a ------------------------------------------------------------- */

static void test_hash(void)
{
    CHECK(suas_sukemet_hash(0x00013000UL) == 0x13FEAF5Cu);
    CHECK(suas_sukemet_hash(0x00013001UL) != suas_sukemet_hash(0x00013000UL));
    CHECK(suas_sukemet_hash_bytes("A001", 4U) == 0xCC24E947u);
    CHECK(suas_sukemet_hash_bytes("", 0U) == 2166136261u);
    CHECK(suas_sukemet_hash_bytes(NULL, 4U) == 0U);
    CHECK(suas_sukemet_hash_bytes("A001", 4U) ==
          suas_sukemet_unik_hash(0x00013000UL));
}

/* -- The Gardiner taxonomy and Core class ------------------------------- */

static void test_gardiner_and_core(void)
{
    sukemet_hieroglyph_entry_t e;
    uint8_t category = 0xFFu;
    uint8_t subgroup = 0xFFu;
    uint16_t item = 0xFFFFu;

    CHECK(suas_sukemet_is_registered(0x00013000UL));
    CHECK(suas_sukemet_is_gardiner(0x00013000UL));
    CHECK(!suas_sukemet_is_modifier(0x00013000UL));
    CHECK(!suas_sukemet_is_enclosure(0x00013000UL));
    CHECK(!suas_sukemet_is_composite(0x00013000UL));
    CHECK(suas_sukemet_is_core(0x00013000UL));
    CHECK(!suas_sukemet_is_legacy(0x00013000UL));
    CHECK(suas_sukemet_core_class(0x00013000UL) == SUKEMET_CORE_C);
    CHECK(suas_sukemet_gardiner_category(0x00013000UL) == SUKEMET_GARDINER_A);
    CHECK(suas_sukemet_is_mirrorable(0x00013000UL));
    CHECK(suas_sukemet_is_rotatable(0x00013000UL));

    CHECK(suas_sukemet_get_entry(0x00013000UL, &e));
    CHECK(e.codepoint == 0x00013000UL);
    CHECK(e.sukemet_flags == (SUKEMET_FLAG_GARDINER | SUKEMET_FLAG_CORE));
    CHECK(e.gardiner_category == SUKEMET_GARDINER_A);
    CHECK(e.sub_index == 1U);
    CHECK(e.phonetic_hash == 0U);

    CHECK(suas_sukemet_catalog_of(0x00013000UL, &category, &subgroup, &item));
    CHECK(category == SUKEMET_GARDINER_A);
    CHECK(subgroup == 1U);
    CHECK(item == 1U);

    /* A-01-027 is the second sign in the shipped table. */
    CHECK(suas_sukemet_get_entry(0x00013001UL, &e));
    CHECK(e.gardiner_category == SUKEMET_GARDINER_A);
    CHECK(e.sub_index == 2U);
    CHECK(suas_sukemet_catalog_of(0x00013001UL, &category, &subgroup, &item));
    CHECK(category == SUKEMET_GARDINER_A);
    CHECK(subgroup == 1U);
    CHECK(item == 27U);

    /* A Legacy, composite sign with a two-codepoint alternate sequence. */
    CHECK(suas_sukemet_get_entry(0x0001305DUL, &e));
    CHECK(e.sukemet_flags == (SUKEMET_FLAG_GARDINER |
                              SUKEMET_FLAG_COMPOSITE |
                              SUKEMET_FLAG_LEGACY));
    CHECK(suas_sukemet_is_legacy(0x0001305DUL));
    CHECK(!suas_sukemet_is_core(0x0001305DUL));
    CHECK(suas_sukemet_is_composite(0x0001305DUL));
    CHECK(suas_sukemet_core_class(0x0001305DUL) == SUKEMET_CORE_L);

    /* kEH_NoMirror: a sign may be unmirrorable yet still rotatable. */
    CHECK(!suas_sukemet_is_mirrorable(0x00013081UL));
    CHECK(suas_sukemet_is_rotatable(0x00013081UL));

    /* A phonemogram, and a sign classified by one function only. */
    CHECK(suas_sukemet_function_types(0x0001300DUL) == SUKEMET_FUNC_PHONOGRAM);

    /* A Gardiner letter with no J and the AA catch-all. */
    CHECK(suas_sukemet_gardiner_letter(SUKEMET_GARDINER_A)[0] == 'A');
    CHECK(suas_sukemet_gardiner_letter(SUKEMET_GARDINER_K)[0] == 'K');
    CHECK(suas_sukemet_gardiner_letter(SUKEMET_GARDINER_AA)[0] == 'A');
    CHECK(suas_sukemet_gardiner_letter(SUKEMET_GARDINER_AA)[1] == 'A');
    CHECK(suas_sukemet_gardiner_letter(0xFEu)[0] == '?');
    CHECK(suas_sukemet_gardiner_name(SUKEMET_GARDINER_A)[0] == 'M');
    CHECK(suas_sukemet_gardiner_name(SUKEMET_GARDINER_AA)[0] == 'U');
}

/* -- Metadata, catalogue and source masks ------------------------------- */

static void test_metadata(void)
{
    sukemet_metadata_t m;
    uint32_t index = 0u;

    CHECK(suas_sukemet_get_metadata(0x00013000UL, &m));
    CHECK(m.catalog_index == 0x00010001u);
    CHECK(m.description_hash == 0x8C1DA676u);
    CHECK(m.unik_hash == 0xCC24E947u);
    CHECK(m.source_mask == 0x007Fu);
    CHECK(m.core_class == SUKEMET_CORE_C);
    CHECK(m.function_types == SUKEMET_FUNC_CLASSIFIER);
    CHECK(m.modifier_class == SUKEMET_MODIFIER_NONE);
    CHECK(m.enclosure_kind == SUKEMET_ENCLOSURE_NONE);
    CHECK(m.alternate_length == 0U);
    CHECK(m.reserved[0] == 0U && m.reserved[1] == 0U && m.reserved[2] == 0U);

    CHECK(suas_sukemet_catalog_index(0x00013000UL, &index));
    CHECK(index == 0x00010001u);
    CHECK(suas_sukemet_source_mask(0x00013000UL) == 0x007Fu);
    CHECK((suas_sukemet_source_mask(0x00013000UL) & SUKEMET_SOURCE_CAT) != 0U);
    CHECK((suas_sukemet_source_mask(0x00013000UL) & SUKEMET_SOURCE_UNIK) != 0U);
    CHECK((suas_sukemet_source_mask(0x00013000UL) & SUKEMET_SOURCE_DESC) != 0U);
    CHECK((suas_sukemet_source_mask(0x00013000UL) & SUKEMET_SOURCE_FUNC) != 0U);
    CHECK((suas_sukemet_source_mask(0x00013000UL) & SUKEMET_SOURCE_ALTSEQ) == 0U);
    CHECK(suas_sukemet_description_hash(0x00013000UL) == 0x8C1DA676u);
    CHECK(suas_sukemet_unik_hash(0x00013000UL) == 0xCC24E947u);

    /* Honest absence: a control has no catalogue or textual Unikemet field. */
    CHECK(!suas_sukemet_catalog_index(0x00013430UL, &index));
    CHECK(!suas_sukemet_catalog_of(0x00013430UL, NULL, NULL, NULL));
    CHECK(suas_sukemet_source_mask(0x00013430UL) == 0U);
    CHECK(suas_sukemet_description_hash(0x00013430UL) == 0U);
    CHECK(suas_sukemet_unik_hash(0x00013430UL) == 0U);
    CHECK(!suas_sukemet_catalog_index(0x00013000UL, NULL));

    /* A function may carry several classes. */
    CHECK((suas_sukemet_function_types(0x0001301FUL) &
           SUKEMET_FUNC_PHONOGRAM) != 0U);
    CHECK((suas_sukemet_function_types(0x0001301FUL) &
           SUKEMET_FUNC_CLASSIFIER) != 0U);
}

/* -- Alternate sequences ------------------------------------------------ */

static void test_alt_seq(void)
{
    uint32_t cp = 0u;

    CHECK(suas_sukemet_alt_seq_length(0x0001305DUL) == 2U);
    CHECK(suas_sukemet_alt_seq_at(0x0001305DUL, 0U, &cp));
    CHECK(cp == 0x0001305CUL);
    CHECK(suas_sukemet_alt_seq_at(0x0001305DUL, 1U, &cp));
    CHECK(cp == 0x00013440UL);
    CHECK(!suas_sukemet_alt_seq_at(0x0001305DUL, 2U, &cp));
    CHECK(!suas_sukemet_alt_seq_at(0x0001305DUL, 0U, NULL));
    CHECK(!suas_sukemet_alt_seq_at(0x00000041UL, 0U, &cp));

    CHECK(suas_sukemet_alt_seq_length(0x00013000UL) == 0U);
    CHECK(!suas_sukemet_alt_seq_at(0x00013000UL, 0U, &cp));

    CHECK(suas_sukemet_alt_seq_length(0x00013096UL) == 3U);
    CHECK(suas_sukemet_alt_seq_at(0x00013096UL, 2U, &cp));
    CHECK(cp == 0x0001335BUL);
    CHECK(suas_sukemet_alt_seq_length(0x000130B9UL) == 3U);
    CHECK(suas_sukemet_alt_seq_at(0x000130B9UL, 1U, &cp));
    CHECK(cp == 0x00013436UL);
}

/* -- Egyptian Hieroglyph Format Controls -------------------------------- */

static void test_controls(void)
{
    sukemet_hieroglyph_entry_t e;

    CHECK(suas_sukemet_is_registered(SUKEMET_VERTICAL_JOINER));
    CHECK(suas_sukemet_is_format_control(SUKEMET_VERTICAL_JOINER));
    CHECK(suas_sukemet_is_modifier(SUKEMET_VERTICAL_JOINER));
    CHECK(suas_sukemet_is_joiner(SUKEMET_VERTICAL_JOINER));
    CHECK(suas_sukemet_is_composite(SUKEMET_VERTICAL_JOINER));
    CHECK(!suas_sukemet_is_insertion(SUKEMET_VERTICAL_JOINER));
    CHECK(!suas_sukemet_is_overlay(SUKEMET_VERTICAL_JOINER));
    CHECK(!suas_sukemet_is_segment(SUKEMET_VERTICAL_JOINER));
    CHECK(!suas_sukemet_is_gardiner(SUKEMET_VERTICAL_JOINER));
    CHECK(!suas_sukemet_is_mirrorable(SUKEMET_VERTICAL_JOINER));
    CHECK(!suas_sukemet_is_rotatable(SUKEMET_VERTICAL_JOINER));
    CHECK(suas_sukemet_modifier_class(SUKEMET_VERTICAL_JOINER) ==
          SUKEMET_MODIFIER_JOINER);
    CHECK(suas_sukemet_function_types(SUKEMET_VERTICAL_JOINER) ==
          SUKEMET_FUNC_CONTROL);

    CHECK(suas_sukemet_get_entry(SUKEMET_VERTICAL_JOINER, &e));
    CHECK(e.gardiner_category == SUKEMET_GARDINER_NONE);
    CHECK(e.sukemet_flags == (SUKEMET_FLAG_MODIFIER |
                              SUKEMET_FLAG_FORMAT |
                              SUKEMET_FLAG_STACKING |
                              SUKEMET_FLAG_COMPOSITE));

    CHECK(suas_sukemet_is_joiner(SUKEMET_HORIZONTAL_JOINER));
    CHECK(suas_sukemet_is_insertion(0x00013432UL));
    CHECK(suas_sukemet_is_insertion(0x0001343BUL));
    CHECK(suas_sukemet_is_overlay(SUKEMET_OVERLAY_MIDDLE));
    CHECK(suas_sukemet_is_segment(SUKEMET_BEGIN_SEGMENT));
    CHECK(suas_sukemet_is_segment(SUKEMET_END_SEGMENT));
    CHECK(suas_sukemet_is_mirror_control(SUKEMET_MIRROR_HORIZONTALLY));
    CHECK(!suas_sukemet_is_joiner(SUKEMET_MIRROR_HORIZONTALLY));
    CHECK(suas_sukemet_modifier_class(SUKEMET_MIRROR_HORIZONTALLY) ==
          SUKEMET_MODIFIER_MIRROR);

    CHECK(suas_sukemet_is_enclosure(SUKEMET_BEGIN_ENCLOSURE));
    CHECK(suas_sukemet_is_enclosure(SUKEMET_END_ENCLOSURE));
    CHECK(suas_sukemet_enclosure_kind(SUKEMET_BEGIN_ENCLOSURE) ==
          SUKEMET_ENCLOSURE_CARTOUCHE);
    CHECK(suas_sukemet_enclosure_kind(SUKEMET_END_ENCLOSURE) ==
          SUKEMET_ENCLOSURE_CARTOUCHE);
    CHECK(suas_sukemet_enclosure_kind(SUKEMET_BEGIN_WALLED_ENCLOSURE) ==
          SUKEMET_ENCLOSURE_SEREKH);
    CHECK(suas_sukemet_enclosure_kind(SUKEMET_END_WALLED_ENCLOSURE) ==
          SUKEMET_ENCLOSURE_SEREKH);
    CHECK(suas_sukemet_modifier_class(SUKEMET_BEGIN_WALLED_ENCLOSURE) ==
          SUKEMET_MODIFIER_ENCLOSURE);

    /* Damage marks are modifiers but not Cf format controls. */
    CHECK(suas_sukemet_is_registered(0x00013447UL));
    CHECK(suas_sukemet_is_modifier(0x00013447UL));
    CHECK(!suas_sukemet_is_format_control(0x00013447UL));
    CHECK(suas_sukemet_is_damage(0x00013447UL));
    CHECK(suas_sukemet_modifier_class(0x00013447UL) ==
          SUKEMET_MODIFIER_DAMAGE);
    CHECK(suas_sukemet_is_damage(0x00013455UL));

    /* The blank/lost signs are signs, not controls. */
    CHECK(suas_sukemet_is_registered(0x00013441UL));
    CHECK(suas_sukemet_is_hieroglyph(0x00013441UL));
    CHECK(!suas_sukemet_is_modifier(0x00013441UL));
    CHECK(!suas_sukemet_is_format_control(0x00013441UL));
}

/* -- The Native & Neography Plane --------------------------------------- */

static void test_neography(void)
{
    sukemet_hieroglyph_entry_t e;

    CHECK(suas_sukemet_is_registered(0x00160300UL));
    CHECK(suas_sukemet_is_gardiner(0x00160300UL) == false);
    CHECK(suas_sukemet_get_entry(0x00160300UL, &e));
    CHECK((e.sukemet_flags & SUKEMET_FLAG_NEOGRAPHY) != 0U);
    CHECK(e.gardiner_category == SUKEMET_GARDINER_NONE);

    CHECK(suas_sukemet_is_shading(0x00160301UL));
    CHECK(suas_sukemet_is_modifier(0x00160301UL));
    CHECK(!suas_sukemet_is_format_control(0x00160301UL));
    CHECK(suas_sukemet_modifier_class(0x00160301UL) ==
          SUKEMET_MODIFIER_SHADING);
    CHECK(suas_sukemet_is_enclosure(0x00160302UL));
    CHECK(suas_sukemet_enclosure_kind(0x00160302UL) ==
          SUKEMET_ENCLOSURE_CUSTOM);
    CHECK(suas_sukemet_is_composite(0x00160303UL));

    /* A declared-but-uncatalogued RNUR codepoint still resolves total. */
    CHECK(suas_sukemet_is_hieroglyph(0x001603FFUL));
    CHECK(!suas_sukemet_is_registered(0x001603FFUL));
    CHECK(suas_sukemet_get_entry(0x001603FFUL, &e));
    CHECK(e.codepoint == 0x001603FFUL);
    CHECK(e.sukemet_flags == SUKEMET_FLAG_NEOGRAPHY);
    CHECK(e.gardiner_category == SUKEMET_GARDINER_NONE);
    CHECK(e.phonetic_hash == 0U);
    CHECK(suas_sukemet_is_shading(0x001603FFUL) == false);
}

/* -- The algorithmic default and the query domain ----------------------- */

static void test_default_and_domain(void)
{
    sukemet_hieroglyph_entry_t e;
    sukemet_metadata_t m;

    CHECK(suas_sukemet_get_entry(0x00013000UL, NULL) == false);
    CHECK(suas_sukemet_get_metadata(0x00013000UL, NULL) == false);

    e.codepoint = 0xDEADBEEFu;
    CHECK(!suas_sukemet_get_entry(0x00000041UL, &e));
    CHECK(e.codepoint == 0xDEADBEEFu); /* untouched on failure */
    CHECK(!suas_sukemet_get_entry(0x00110000UL, &e));
    CHECK(!suas_sukemet_get_entry(0x7FFFFFF0UL, &e));
    CHECK(!suas_sukemet_get_entry(0x7FFFFFFFUL, &e));
    CHECK(!suas_sukemet_get_entry(0x80000000UL, &e));
    CHECK(!suas_sukemet_get_metadata(0x00000041UL, &m));
    CHECK(!suas_sukemet_is_registered(0x00000041UL));
    CHECK(!suas_sukemet_is_gardiner(0x00000041UL));
    CHECK(!suas_sukemet_is_mirrorable(0x00000041UL));
    CHECK(suas_sukemet_core_class(0x00000041UL) == SUKEMET_CORE_N);
    CHECK(suas_sukemet_function_types(0x00000041UL) == SUKEMET_FUNC_NONE);

    /* An unassigned codepoint inside Extended-A is still a well-formed,
     * synthesized hieroglyph record. */
    CHECK(suas_sukemet_get_entry(0x000143FBUL, &e));
    CHECK(e.codepoint == 0x000143FBUL);
    CHECK(e.sukemet_flags == 0U);
    CHECK(e.gardiner_category == SUKEMET_GARDINER_NONE);
    CHECK(!suas_sukemet_is_registered(0x000143FBUL));
    CHECK(suas_sukemet_is_hieroglyph(0x000143FBUL));
    CHECK(suas_sukemet_get_metadata(0x000143FBUL, &m));
    CHECK(m.catalog_index == 0U);
    CHECK(m.source_mask == 0U);
    CHECK(m.core_class == SUKEMET_CORE_N);
    CHECK(m.alternate_length == 0U);

    /* The unassigned tail of the format-control block is in the domain too. */
    CHECK(suas_sukemet_get_entry(0x00013456UL, &e));
    CHECK(e.sukemet_flags == 0U);
    CHECK(!suas_sukemet_is_registered(0x00013456UL));
}

/* -- Enumeration and compiled-table self-consistency -------------------- */

static void test_enumeration(void)
{
    sukemet_hieroglyph_entry_t e;
    sukemet_hieroglyph_entry_t prev;
    sukemet_metadata_t m;
    static sukemet_hieroglyph_entry_t all[SUAS_SUKEMET_REGISTRY_CAPACITY];
    sukemet_registry_error_t err = SUKEMET_REGISTRY_CORE_CONTRADICTION;
    uint32_t i;
    uint32_t count = suas_sukemet_entry_count();

    CHECK(count == 5109U);
    CHECK(count <= SUAS_SUKEMET_REGISTRY_CAPACITY);
    CHECK(suas_sukemet_entry_at(0U, &e));
    CHECK(e.codepoint == 0x00013000UL);
    CHECK(suas_sukemet_entry_at(count - 1U, &e));
    CHECK(e.codepoint == 0x00160303UL);
    CHECK(suas_sukemet_metadata_at(0U, &m));
    CHECK(m.catalog_index == 0x00010001u);
    CHECK(!suas_sukemet_entry_at(count, &e));
    CHECK(!suas_sukemet_metadata_at(count, &m));
    CHECK(!suas_sukemet_entry_at(0U, NULL));
    CHECK(!suas_sukemet_metadata_at(0U, NULL));

    CHECK(suas_sukemet_entry_at(0U, &prev));
    all[0] = prev;
    for (i = 1U; i < count; i++) {
        CHECK(suas_sukemet_entry_at(i, &e));
        CHECK(suas_sukemet_metadata_at(i, &m));
        CHECK(e.codepoint > prev.codepoint);
        CHECK(e.gardiner_category == SUKEMET_GARDINER_NONE ||
              e.gardiner_category < SUKEMET_GARDINER_COUNT);
        CHECK((e.sukemet_flags & SUKEMET_FLAG_RESERVED_MASK) == 0U);
        CHECK(((e.sukemet_flags & SUKEMET_FLAG_GARDINER) != 0U) ==
              (e.gardiner_category != SUKEMET_GARDINER_NONE));
        CHECK(m.core_class <= SUKEMET_CORE_L);
        CHECK((m.function_types & (uint8_t)~SUKEMET_FUNC_ASSIGNED_MASK) == 0U);
        CHECK(m.modifier_class <= SUKEMET_MODIFIER_SHADING);
        CHECK(m.enclosure_kind <= SUKEMET_ENCLOSURE_CUSTOM);
        CHECK(m.alternate_length <= SUAS_SUKEMET_ALT_MAX);
        CHECK(suas_sukemet_is_registered(e.codepoint));
        {
            sukemet_hieroglyph_entry_t got;
            CHECK(suas_sukemet_get_entry(e.codepoint, &got));
            CHECK(got.codepoint == e.codepoint);
            CHECK(got.phonetic_hash == e.phonetic_hash);
            CHECK(got.sukemet_flags == e.sukemet_flags);
            CHECK(got.gardiner_category == e.gardiner_category);
            CHECK(got.sub_index == e.sub_index);
        }
        all[i] = e;
        prev = e;
    }

    /* The entire shipped table passes the section 9 verifier. */
    CHECK(suas_sukemet_validate_table(all, count, &err));
    CHECK(err == SUKEMET_REGISTRY_OK);
}

/* -- Verification outcomes ---------------------------------------------- */

static void test_validate_table(void)
{
    sukemet_hieroglyph_entry_t valid[2];
    sukemet_hieroglyph_entry_t e;
    sukemet_registry_error_t err = SUKEMET_REGISTRY_OK;

    CHECK(suas_sukemet_validate_table(NULL, 0U, &err));
    CHECK(err == SUKEMET_REGISTRY_OK);
    CHECK(!suas_sukemet_validate_table(NULL, 1U, &err));
    CHECK(err == SUKEMET_REGISTRY_NULL_TABLE);
    CHECK(!suas_sukemet_validate_table(&valid[0],
                                       SUAS_SUKEMET_REGISTRY_CAPACITY + 1U,
                                       &err));
    CHECK(err == SUKEMET_REGISTRY_COUNT_OVER);

    valid[0].codepoint = 0x00013000UL;
    valid[0].phonetic_hash = 0U;
    valid[0].sukemet_flags = SUKEMET_FLAG_GARDINER | SUKEMET_FLAG_CORE;
    valid[0].gardiner_category = SUKEMET_GARDINER_A;
    valid[0].sub_index = 1U;
    valid[1] = valid[0];
    valid[1].codepoint = 0x00013001UL;
    valid[1].sub_index = 2U;
    CHECK(suas_sukemet_validate_table(valid, 2U, &err));
    CHECK(err == SUKEMET_REGISTRY_OK);

    e = valid[0];
    e.codepoint = 0x80000000UL;
    CHECK(!suas_sukemet_validate_table(&e, 1U, &err));
    CHECK(err == SUKEMET_REGISTRY_OUT_OF_SPACE);

    e = valid[0];
    e.codepoint = 0x00000041UL;
    e.sukemet_flags = 0U;
    e.gardiner_category = SUKEMET_GARDINER_NONE;
    CHECK(!suas_sukemet_validate_table(&e, 1U, &err));
    CHECK(err == SUKEMET_REGISTRY_INADMISSIBLE_CODEPOINT);

    e = valid[0];
    e.sukemet_flags |= SUKEMET_FLAG_RESERVED_MASK;
    CHECK(!suas_sukemet_validate_table(&e, 1U, &err));
    CHECK(err == SUKEMET_REGISTRY_BAD_FLAGS);

    e = valid[0];
    e.sukemet_flags |= SUKEMET_FLAG_LEGACY;
    CHECK(!suas_sukemet_validate_table(&e, 1U, &err));
    CHECK(err == SUKEMET_REGISTRY_CORE_CONTRADICTION);

    e = valid[0];
    e.gardiner_category = 27U;
    CHECK(!suas_sukemet_validate_table(&e, 1U, &err));
    CHECK(err == SUKEMET_REGISTRY_BAD_CATEGORY);

    e = valid[0];
    e.sukemet_flags = SUKEMET_FLAG_CORE;
    CHECK(!suas_sukemet_validate_table(&e, 1U, &err));
    CHECK(err == SUKEMET_REGISTRY_GARDINER_MISMATCH);

    e = valid[0];
    e.gardiner_category = SUKEMET_GARDINER_NONE;
    CHECK(!suas_sukemet_validate_table(&e, 1U, &err));
    CHECK(err == SUKEMET_REGISTRY_GARDINER_MISMATCH);

    e = valid[0];
    e.sukemet_flags |= SUKEMET_FLAG_FORMAT | SUKEMET_FLAG_MODIFIER;
    CHECK(!suas_sukemet_validate_table(&e, 1U, &err));
    CHECK(err == SUKEMET_REGISTRY_CONTROL_MISMATCH);

    e = valid[0];
    e.sukemet_flags |= SUKEMET_FLAG_NEOGRAPHY;
    CHECK(!suas_sukemet_validate_table(&e, 1U, &err));
    CHECK(err == SUKEMET_REGISTRY_NEOGRAPHY_MISMATCH);

    e = valid[0];
    e.sukemet_flags |= SUKEMET_FLAG_FORMAT;
    CHECK(!suas_sukemet_validate_table(&e, 1U, &err));
    CHECK(err == SUKEMET_REGISTRY_CONTROL_MISMATCH);

    valid[1] = valid[0];
    CHECK(!suas_sukemet_validate_table(valid, 2U, &err));
    CHECK(err == SUKEMET_REGISTRY_DUPLICATE_CODEPOINT);

    valid[0].codepoint = 0x00013001UL;
    valid[1].codepoint = 0x00013000UL;
    CHECK(!suas_sukemet_validate_table(valid, 2U, &err));
    CHECK(err == SUKEMET_REGISTRY_UNORDERED);

    valid[0].codepoint = 0x00013000UL;
    valid[1].codepoint = 0x00013001UL;
    CHECK(suas_sukemet_validate_table(valid, 2U, &err));
    CHECK(err == SUKEMET_REGISTRY_OK);

    /* A valid control row exercises the format-control branch. */
    e.codepoint = 0x00013430UL;
    e.sukemet_flags = SUKEMET_FLAG_MODIFIER | SUKEMET_FLAG_FORMAT;
    e.gardiner_category = SUKEMET_GARDINER_NONE;
    e.sub_index = 0U;
    CHECK(suas_sukemet_validate_table(&e, 1U, &err));
    CHECK(err == SUKEMET_REGISTRY_OK);

    /* A valid RNUR row exercises the neography branch. */
    e.codepoint = 0x00160300UL;
    e.sukemet_flags = SUKEMET_FLAG_NEOGRAPHY;
    CHECK(suas_sukemet_validate_table(&e, 1U, &err));
    CHECK(err == SUKEMET_REGISTRY_OK);

    CHECK(suas_sukemet_validate_table(valid, 2U, NULL));
}

/* -- Taxonomy names ----------------------------------------------------- */

static void test_taxonomy(void)
{
    CHECK(suas_sukemet_gardiner_letter(SUKEMET_GARDINER_A)[0] == 'A');
    CHECK(suas_sukemet_gardiner_letter(SUKEMET_GARDINER_K)[0] == 'K');
    CHECK(suas_sukemet_gardiner_letter(SUKEMET_GARDINER_Z)[0] == 'Z');
    CHECK(suas_sukemet_gardiner_letter(SUKEMET_GARDINER_AA)[0] == 'A');
    CHECK(suas_sukemet_gardiner_letter(SUKEMET_GARDINER_AA)[1] == 'A');
    CHECK(suas_sukemet_gardiner_letter(SUKEMET_GARDINER_AA)[2] == '\0');
    CHECK(suas_sukemet_gardiner_letter(0xFEu)[0] == '?');
    CHECK(suas_sukemet_gardiner_name(SUKEMET_GARDINER_A)[0] == 'M');
    CHECK(suas_sukemet_gardiner_name(SUKEMET_GARDINER_AA)[0] == 'U');

    CHECK(suas_sukemet_core_name(SUKEMET_CORE_N)[0] == 'N');
    CHECK(suas_sukemet_core_name(SUKEMET_CORE_C)[0] == 'C');
    CHECK(suas_sukemet_core_name(SUKEMET_CORE_L)[0] == 'L');
    CHECK(suas_sukemet_core_name(0xFEu)[0] == 'U');

    CHECK(suas_sukemet_function_name(SUKEMET_FUNC_NONE)[0] == 'n');
    CHECK(suas_sukemet_function_name(SUKEMET_FUNC_PICTOGRAM)[0] == 'p');
    CHECK(suas_sukemet_function_name(SUKEMET_FUNC_LOGOGRAM)[0] == 'l');
    CHECK(suas_sukemet_function_name(SUKEMET_FUNC_PHONOGRAM)[0] == 'p');
    CHECK(suas_sukemet_function_name(SUKEMET_FUNC_CLASSIFIER)[0] == 'c');
    CHECK(suas_sukemet_function_name(SUKEMET_FUNC_PHONO_REPEATER)[0] == 'p');
    CHECK(suas_sukemet_function_name(SUKEMET_FUNC_RADICOGRAM)[0] == 'r');
    CHECK(suas_sukemet_function_name(SUKEMET_FUNC_INTERPRETANT)[0] == 'i');
    CHECK(suas_sukemet_function_name(SUKEMET_FUNC_CONTROL)[0] == 'c');
    CHECK(suas_sukemet_function_name(0xFEu)[0] == 'm');

    CHECK(suas_sukemet_modifier_class_name(SUKEMET_MODIFIER_NONE)[0] == 'n');
    CHECK(suas_sukemet_modifier_class_name(SUKEMET_MODIFIER_JOINER)[0] == 'j');
    CHECK(suas_sukemet_modifier_class_name(SUKEMET_MODIFIER_INSERTION)[0] == 'i');
    CHECK(suas_sukemet_modifier_class_name(SUKEMET_MODIFIER_OVERLAY)[0] == 'o');
    CHECK(suas_sukemet_modifier_class_name(SUKEMET_MODIFIER_SEGMENT)[0] == 's');
    CHECK(suas_sukemet_modifier_class_name(SUKEMET_MODIFIER_ENCLOSURE)[0] == 'e');
    CHECK(suas_sukemet_modifier_class_name(SUKEMET_MODIFIER_MIRROR)[0] == 'm');
    CHECK(suas_sukemet_modifier_class_name(SUKEMET_MODIFIER_DAMAGE)[0] == 'd');
    CHECK(suas_sukemet_modifier_class_name(SUKEMET_MODIFIER_SHADING)[0] == 's');
    CHECK(suas_sukemet_modifier_class_name(0xFEu)[0] == 'u');

    CHECK(suas_sukemet_enclosure_name(SUKEMET_ENCLOSURE_NONE)[0] == 'n');
    CHECK(suas_sukemet_enclosure_name(SUKEMET_ENCLOSURE_CARTOUCHE)[0] == 'c');
    CHECK(suas_sukemet_enclosure_name(SUKEMET_ENCLOSURE_SEREKH)[0] == 's');
    CHECK(suas_sukemet_enclosure_name(SUKEMET_ENCLOSURE_CUSTOM)[0] == 'c');
    CHECK(suas_sukemet_enclosure_name(0xFEu)[0] == 'u');

    CHECK(suas_sukemet_source_name(SUKEMET_SOURCE_HG)[0] == 'h');
    CHECK(suas_sukemet_source_name(SUKEMET_SOURCE_JSESH)[0] == 'j');
    CHECK(suas_sukemet_source_name(SUKEMET_SOURCE_IFAO)[0] == 'i');
    CHECK(suas_sukemet_source_name(SUKEMET_SOURCE_UNIK)[0] == 'u');
    CHECK(suas_sukemet_source_name(SUKEMET_SOURCE_CAT)[0] == 'c');
    CHECK(suas_sukemet_source_name(SUKEMET_SOURCE_DESC)[0] == 'd');
    CHECK(suas_sukemet_source_name(SUKEMET_SOURCE_FUNC)[0] == 'f');
    CHECK(suas_sukemet_source_name(SUKEMET_SOURCE_FVAL)[0] == 'f');
    CHECK(suas_sukemet_source_name(SUKEMET_SOURCE_ALTSEQ)[0] == 'a');
    CHECK(suas_sukemet_source_name(SUKEMET_SOURCE_MIRROR)[0] == 'm');
    CHECK(suas_sukemet_source_name(SUKEMET_SOURCE_ROTATE)[0] == 'r');
    CHECK(suas_sukemet_source_name(0U)[0] == 'n');
    CHECK(suas_sukemet_source_name(0xFFFFu)[0] == 'c');

    CHECK(suas_sukemet_block_name(SUKEMET_BLOCK_NONE)[0] == 'n');
    CHECK(suas_sukemet_block_name(SUKEMET_BLOCK_EGYPTIAN)[0] == 'E');
    CHECK(suas_sukemet_block_name(SUKEMET_BLOCK_FORMAT_CONTROLS)[0] == 'E');
    CHECK(suas_sukemet_block_name(SUKEMET_BLOCK_EXTENDED_A)[0] == 'E');
    CHECK(suas_sukemet_block_name(SUKEMET_BLOCK_NEOGRAPHY)[0] == 'N');
    CHECK(suas_sukemet_block_name((sukemet_block_t)0xFEu)[0] == 'u');

    CHECK(suas_sukemet_zone_name(SUAS_SUKEMET_ZONE_BRIDGE)[0] == 'B');
    CHECK(suas_sukemet_zone_name(SUAS_SUKEMET_ZONE_SCP)[0] == 'S');
    CHECK(suas_sukemet_zone_name(SUAS_SUKEMET_ZONE_RNUR)[0] == 'N');
    CHECK(suas_sukemet_zone_name(SUAS_SUKEMET_ZONE_NATIVE)[0] == 'N');
    CHECK(suas_sukemet_zone_name(SUAS_SUKEMET_ZONE_TRAP)[0] == 'T');
    CHECK(suas_sukemet_zone_name(SUAS_SUKEMET_ZONE_SENTINEL)[0] == 'S');
    CHECK(suas_sukemet_zone_name(SUAS_SUKEMET_ZONE_EXTSUCS)[0] == 'E');
    CHECK(suas_sukemet_zone_name((suas_sukemet_zone_t)0xFEu)[0] == 'u');

    CHECK(suas_sukemet_registry_error_name(SUKEMET_REGISTRY_OK)[0] == 'o');
    CHECK(suas_sukemet_registry_error_name(SUKEMET_REGISTRY_NULL_TABLE)[0] == 'n');
    CHECK(suas_sukemet_registry_error_name(SUKEMET_REGISTRY_CORE_CONTRADICTION)[0] == 'c');
    CHECK(suas_sukemet_registry_error_name((sukemet_registry_error_t)0xFEu)[0] == 'u');
}

/* -- SSRE registration -------------------------------------------------- */

static void test_ssre_registration(void)
{
    const ssre_spec_record_t *rec;

    CHECK(suas_ssre_is_registered("SUAS-016"));
    rec = suas_ssre_lookup("SUAS-016");
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
    test_gardiner_and_core();
    test_metadata();
    test_alt_seq();
    test_controls();
    test_neography();
    test_default_and_domain();
    test_enumeration();
    test_validate_table();
    test_taxonomy();
    test_ssre_registration();

    if (g_fail) {
        printf("SUAS-016 SUKEMET tests FAILED\n");
        return 1;
    }
    printf("SUAS-016 SUKEMET tests passed\n");
    return 0;
}
