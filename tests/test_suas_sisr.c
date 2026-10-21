/**
 * SUAS-013 SISR — System Ideographic Source Registry
 *
 * Unit tests for the zero-allocation, static-table, O(1) resolution of
 * ideographic external source identities, registry classes, and canonical
 * variant fallbacks (a UAX #45 analog).
 *
 * Covers: version/meta, the compile-time record geometry, canonical source
 * tokens and their hashes, zone dispatch and the RNUR set decode, the O(1)
 * source and linkage queries, the lazy/idempotent build, enumeration and table
 * self-consistency, the agreement invariant against SUAS-009 (SIP), every
 * verification outcome for both tables, and registration in SUAS-010 (SSRE).
 */

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <assert.h>

#include "suas/suas_sisr.h"
#include "suas/suas_sip.h"
#include "suas/suas_ssre.h"

static int g_fail = 0;
#define CHECK(cond) do { if (!(cond)) { \
    printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); g_fail = 1; \
} } while (0)

/* ── Version, limits, and record geometry ─────────────────────────── */

static void test_version_and_meta(void)
{
    CHECK(suas_sisr_version_string() != NULL);
    CHECK(suas_sisr_version_string()[0] == '1');
    CHECK(suas_sisr_version_string()[2] == '0');
    CHECK(SUAS_SISR_SPEC_ID[0] == 'S');
    CHECK(SUAS_SISR_SPEC_TITLE[0] == 'S');
    CHECK(SUAS_SISR_VERSION_MAJOR == 1);
    CHECK(SUAS_SISR_VERSION_MINOR == 0);
    CHECK(SUAS_SISR_REGISTRY_CAPACITY == 256U);
    CHECK(SUAS_SISR_INDEX_SIZE == 512U);
    CHECK(SUAS_SISR_INDEX_SIZE > SUAS_SISR_REGISTRY_CAPACITY);
    CHECK(SUAS_SISR_LINK_INDEX_SIZE > SUAS_SISR_LINK_CAPACITY);

    /* The records are fixed-size, byte-aligned and padding-free (§3.1, §3.3). */
    CHECK(sizeof(sisr_source_entry_t) == 16U);
    CHECK(offsetof(sisr_source_entry_t, codepoint) == 0U);
    CHECK(offsetof(sisr_source_entry_t, source_serial) == 4U);
    CHECK(offsetof(sisr_source_entry_t, source_id_hash) == 8U);
    CHECK(offsetof(sisr_source_entry_t, registry_flags) == 12U);
    CHECK(offsetof(sisr_source_entry_t, source_type) == 14U);
    CHECK(offsetof(sisr_source_entry_t, reserved) == 15U);

    CHECK(sizeof(sisr_variant_link_t) == 12U);
    CHECK(offsetof(sisr_variant_link_t, codepoint) == 0U);
    CHECK(offsetof(sisr_variant_link_t, fallback_codepoint) == 4U);
    CHECK(offsetof(sisr_variant_link_t, link_flags) == 8U);
    CHECK(offsetof(sisr_variant_link_t, reserved) == 10U);

    /* Flag bits are disjoint (§4.2, §6). */
    CHECK((SISR_SOURCE_RNUR_SET1 & SISR_SOURCE_RNUR_SET2) == 0u);
    CHECK((SISR_SOURCE_CONLANG & SISR_SOURCE_HISTORICAL) == 0u);
    CHECK((SISR_SOURCE_VERIFIED & SISR_SOURCE_DEPRECATED) == 0u);
    CHECK((SISR_LINK_SIMPLIFIED & SISR_LINK_TRADITIONAL) == 0u);
    CHECK((SISR_LINK_COMPATIBILITY & SISR_LINK_REGISTRY) == 0u);
    CHECK((SISR_SOURCE_ASSIGNED_MASK & (uint16_t)~SISR_SOURCE_ASSIGNED_MASK) == 0u);
}

/* ── Zone dispatch and the RNUR carve-out (§5) ────────────────────── */

static void test_zones(void)
{
    CHECK(suas_sisr_zone_of(0x0041u) == SUAS_SISR_ZONE_BRIDGE);
    CHECK(suas_sisr_zone_of(0x0010FFFFu) == SUAS_SISR_ZONE_BRIDGE);
    CHECK(suas_sisr_zone_of(0x00110000u) == SUAS_SISR_ZONE_SCP);
    CHECK(suas_sisr_zone_of(0x0011FFFFu) == SUAS_SISR_ZONE_SCP);
    CHECK(suas_sisr_zone_of(0x00120000u) == SUAS_SISR_ZONE_NATIVE);
    CHECK(suas_sisr_zone_of(0x0015FFFFu) == SUAS_SISR_ZONE_NATIVE);
    CHECK(suas_sisr_zone_of(0x00160000u) == SUAS_SISR_ZONE_RNUR);
    CHECK(suas_sisr_zone_of(0x0016FFFFu) == SUAS_SISR_ZONE_RNUR);
    CHECK(suas_sisr_zone_of(0x00170000u) == SUAS_SISR_ZONE_NATIVE);
    CHECK(suas_sisr_zone_of(0x7FFFFFF0u) == SUAS_SISR_ZONE_TRAP);
    CHECK(suas_sisr_zone_of(0x7FFFFFFEu) == SUAS_SISR_ZONE_TRAP);
    CHECK(suas_sisr_zone_of(0x7FFFFFFFu) == SUAS_SISR_ZONE_SENTINEL);
    CHECK(suas_sisr_zone_of(0x80000000u) == SUAS_SISR_ZONE_EXTSUCS);
    CHECK(suas_sisr_zone_of(0xFFFFFFFFu) == SUAS_SISR_ZONE_EXTSUCS);

    /* A source identity may name Bridge, RNUR and Native — nothing else. */
    CHECK(suas_sisr_is_source_space(0x4E00u));
    CHECK(suas_sisr_is_source_space(0x00120000u));
    CHECK(suas_sisr_is_source_space(0x00160100u));
    CHECK(!suas_sisr_is_source_space(0x00110000u));
    CHECK(!suas_sisr_is_source_space(0x7FFFFFF0u));
    CHECK(!suas_sisr_is_source_space(0x7FFFFFFFu));
    CHECK(!suas_sisr_is_source_space(0x80000000u));

    CHECK(suas_sisr_is_neography(0x00160000u));
    CHECK(suas_sisr_is_neography(0x0016FFFFu));
    CHECK(!suas_sisr_is_neography(0x0015FFFFu));
}

static void test_neography_set(void)
{
    uint32_t set = 0xFFFFFFFFu;

    CHECK(suas_sisr_neography_set(0x00160100u, &set));
    CHECK(set == 1u);                    /* (0x100) >> 8 */
    CHECK(suas_sisr_neography_set(0x00160200u, &set));
    CHECK(set == 2u);
    CHECK(suas_sisr_neography_set(0x0016FFFFu, &set));
    CHECK(set == 0xFFu);

    CHECK(!suas_sisr_neography_set(0x00120000u, &set));
    CHECK(!suas_sisr_neography_set(0x00160100u, NULL));
}

/* ── Canonical tokens and hashes (§3.2) ───────────────────────────── */

static void test_hash(void)
{
    /* FNV-1a 32-bit over the four little-endian codepoint bytes. */
    CHECK(suas_sisr_hash_codepoint(0x00160100u) ==
          suas_sisr_hash_codepoint(0x00160100u));
    CHECK(suas_sisr_hash_codepoint(0x00160100u) !=
          suas_sisr_hash_codepoint(0x00160101u));
    CHECK(suas_sisr_hash_codepoint(0u) != 0u);

    /* The token hash is the FNV-1a of the bytes, and the empty or NULL form
     * yields the sentinel zero (§3.2). */
    CHECK(suas_sisr_hash_source_token("UTC-00932", 9u) == 0x970E732Eu);
    CHECK(suas_sisr_hash_source_token("RNUR-00256", 10u) == 0x0C6B4AFAu);
    CHECK(suas_sisr_hash_source_token(NULL, 4u) == 0u);
    CHECK(suas_sisr_hash_source_token("UTC-00932", 0u) == 0u);
}

static void test_token_format(void)
{
    sisr_source_entry_t e;
    char buf[SUAS_SISR_TOKEN_MAX];
    char small[4];

    CHECK(suas_sisr_get_source_entry(0x0000FA26u, &e));
    CHECK(suas_sisr_format_source_id(&e, buf, sizeof buf));
    CHECK(buf[0] == 'U' && buf[1] == 'T' && buf[2] == 'C' && buf[3] == '-');
    CHECK(buf[4] == '0' && buf[5] == '0' && buf[6] == '9' && buf[7] == '3');
    CHECK(buf[8] == '2' && buf[9] == '\0');   /* five digits, no widening */

    CHECK(suas_sisr_get_source_entry(0x00160100u, &e));
    CHECK(suas_sisr_format_source_id(&e, buf, sizeof buf));
    CHECK(buf[0] == 'R' && buf[4] == '-' && buf[5] == '0' && buf[9] == '6');
    CHECK(buf[10] == '\0');
    CHECK(suas_sisr_source_id_matches(&e, buf, 10u));
    CHECK(!suas_sisr_source_id_matches(&e, "RNUR-00257", 10u));
    CHECK(!suas_sisr_source_id_matches(&e, buf, 9u)); /* truncation */

    /* A widened serial and a too-small buffer. */
    e.source_serial = 0x0186A0u;   /* 100000, needs six digits */
    CHECK(suas_sisr_format_source_id(&e, buf, sizeof buf));
    CHECK(buf[5] == '1' && buf[11] == '\0');

    CHECK(!suas_sisr_format_source_id(&e, small, sizeof small));
    CHECK(!suas_sisr_format_source_id(&e, NULL, 32u));
    CHECK(!suas_sisr_format_source_id(NULL, buf, sizeof buf));

    /* A serial of zero and an unknown authority are not tokenizable. */
    e.source_serial = 0u;
    CHECK(!suas_sisr_format_source_id(&e, buf, sizeof buf));
    e.source_serial = 1u;
    e.source_type = 0u;
    CHECK(!suas_sisr_format_source_id(&e, buf, sizeof buf));
}

/* ── Taxonomy names ───────────────────────────────────────────────── */

static void test_taxonomy(void)
{
    CHECK(suas_sisr_source_type_by_name("UTC", 3u) == SISR_SOURCE_TYPE_UTC);
    CHECK(suas_sisr_source_type_by_name("UCI", 3u) == SISR_SOURCE_TYPE_UCI);
    CHECK(suas_sisr_source_type_by_name("UK", 2u) == SISR_SOURCE_TYPE_UK);
    CHECK(suas_sisr_source_type_by_name("RNUR", 4u) == SISR_SOURCE_TYPE_RNUR);
    CHECK(suas_sisr_source_type_by_name("CONLANG", 7u) ==
          SISR_SOURCE_TYPE_CONLANG);
    CHECK(suas_sisr_source_type_by_name("CONL", 4u) ==
          SISR_SOURCE_TYPE_CONLANG);
    CHECK(suas_sisr_source_type_by_name("CUSTOM", 6u) ==
          SISR_SOURCE_TYPE_CUSTOM);
    CHECK(suas_sisr_source_type_by_name("nope", 4u) ==
          SISR_SOURCE_TYPE_INVALID);
    CHECK(suas_sisr_source_type_by_name("UT", 2u) == SISR_SOURCE_TYPE_INVALID);
    CHECK(suas_sisr_source_type_by_name("UTCx", 4u) ==
          SISR_SOURCE_TYPE_INVALID);
    CHECK(suas_sisr_source_type_by_name(NULL, 3u) ==
          SISR_SOURCE_TYPE_INVALID);
    CHECK(suas_sisr_source_type_by_name("UTC", 0u) ==
          SISR_SOURCE_TYPE_INVALID);

    CHECK(suas_sisr_source_type_name(SISR_SOURCE_TYPE_CONLANG)[0] == 'C');
    CHECK(suas_sisr_source_type_name(SISR_SOURCE_TYPE_INVALID)[0] == 'I');
    CHECK(suas_sisr_source_type_prefix(SISR_SOURCE_TYPE_CONLANG)[0] == 'C');
    CHECK(suas_sisr_source_type_prefix(SISR_SOURCE_TYPE_CONLANG)[3] == 'L');
    CHECK(suas_sisr_source_type_prefix(SISR_SOURCE_TYPE_INVALID) == NULL);

    CHECK(suas_sisr_link_name(0u)[0] == 'N');
    CHECK(suas_sisr_link_name(SISR_LINK_COMPATIBILITY)[0] == 'C');
    CHECK(suas_sisr_link_name(SISR_LINK_TRADITIONAL)[0] == 'T');
    CHECK(suas_sisr_link_name(0x4000u)[0] == 'U'); /* unassigned bit */

    CHECK(suas_sisr_registry_error_name(SISR_REGISTRY_OK)[0] == 'O');
    CHECK(suas_sisr_registry_error_name(SISR_REGISTRY_HASH_MISMATCH)[0] == 'H');
    CHECK(suas_sisr_registry_error_name((sisr_registry_error_t)99)[0] == 'U');
}

/* ── The O(1) query surface (§3.4) ────────────────────────────────── */

static void test_get_source_entry(void)
{
    sisr_source_entry_t e;

    /* No explicit init: the first query builds the index lazily. */
    CHECK(suas_sisr_get_source_entry(0x00160100u, &e));
    CHECK(e.codepoint == 0x00160100u);
    CHECK(e.source_serial == 0x00000100u);
    CHECK(e.source_type == (uint8_t)SISR_SOURCE_TYPE_RNUR);
    CHECK(e.registry_flags == (SISR_SOURCE_RNUR_SET1 | SISR_SOURCE_VERIFIED));
    CHECK(e.reserved == 0u);
    CHECK(e.source_id_hash == suas_sisr_hash_source_token("RNUR-00256", 10u));

    CHECK(suas_sisr_get_source_entry(0x0000FA26u, &e));
    CHECK(e.source_type == (uint8_t)SISR_SOURCE_TYPE_UTC);
    CHECK((e.registry_flags & SISR_SOURCE_HISTORICAL) != 0u);
    CHECK((e.registry_flags & SISR_SOURCE_VERIFIED) != 0u);

    CHECK(suas_sisr_is_registered(0x00160200u));
    CHECK(!suas_sisr_is_registered(0x00120000u));  /* Native, no source    */
    CHECK(!suas_sisr_is_registered(0x00110001u));  /* inadmissible zone    */
    CHECK(!suas_sisr_is_registered(0x7FFFFFFFu));  /* Sentinel             */
    CHECK(!suas_sisr_get_source_entry(0x00120000u, &e));
    CHECK(!suas_sisr_get_source_entry(0x00160100u, NULL));
}

static void test_init_idempotent(void)
{
    sisr_source_entry_t a;
    sisr_source_entry_t b;

    suas_sisr_init_registry();
    suas_sisr_init_registry();
    CHECK(suas_sisr_get_source_entry(0x00009F8Du, &a));
    suas_sisr_init_registry();
    CHECK(suas_sisr_get_source_entry(0x00009F8Du, &b));
    CHECK(a.codepoint == b.codepoint && a.source_serial == b.source_serial);
}

/* ── Variant and fallback linkage (§6) ────────────────────────────── */

static void test_variant_linkage(void)
{
    sisr_variant_link_t l;
    uint32_t target = 0u;

    CHECK(suas_sisr_fallback_target(0x00009F8Du, &target));
    CHECK(target == 0x00160200u);                 /* 龍 -> RNUR set 2 */
    CHECK(suas_sisr_fallback_target(0x00009F99u, &target));
    CHECK(target == 0x00160201u);                 /* 龙 -> RNUR set 2 */
    CHECK(suas_sisr_fallback_target(0x00003402u, &target));
    CHECK(target == 0x00160100u);                 /* -> RNUR set 1    */
    CHECK(suas_sisr_fallback_target(0x0000FA26u, &target));
    CHECK(target == 0x000090FDu);                 /* compatibility    */

    CHECK(suas_sisr_get_variant_link(0x0002F800u, &l));
    CHECK(l.fallback_codepoint == 0x00004E3Du);
    CHECK((l.link_flags & SISR_LINK_COMPATIBILITY) != 0u);
    CHECK(l.reserved == 0u);

    CHECK(suas_sisr_get_variant_link(0x00009F8Du, &l));
    CHECK((l.link_flags & SISR_LINK_REGISTRY) != 0u);
    CHECK((l.link_flags & SISR_LINK_TRADITIONAL) != 0u);

    /* A registered codepoint may have no link; an unregistered one never does. */
    CHECK(!suas_sisr_get_variant_link(0x00160100u, &l));
    CHECK(!suas_sisr_fallback_target(0x00004E00u, &target));
    CHECK(!suas_sisr_fallback_target(0x00009F8Du, NULL));
    CHECK(!suas_sisr_get_variant_link(0x00009F8Du, NULL));

    /* Has-variant is exactly the presence of a link. */
    CHECK(suas_sisr_get_variant_link(0x00160300u, &l));
    CHECK(!suas_sisr_get_variant_link(0x00160301u, &l));
}

/* ── SUAS-009 agreement (§6.3) ────────────────────────────────────── */

static void test_sip_agreement(void)
{
    uint32_t i;
    uint32_t checked = 0u;

    /* Every SISR fallback whose source codepoint SIP also models must resolve
     * to the same target SIP publishes as its native variant. */
    for (i = 0u; i < suas_sisr_link_count(); i++) {
        sisr_variant_link_t l;
        sucs_ex_char_t sip_target = 0ULL;

        CHECK(suas_sisr_link_at(i, &l));
        if (suas_sip_get_properties((sucs_ex_char_t)l.codepoint, NULL))
            continue;
        if (suas_sip_variant_target((sucs_ex_char_t)l.codepoint, &sip_target)) {
            CHECK((uint32_t)sip_target == l.fallback_codepoint);
            checked++;
        }
    }
    CHECK(checked > 0u);
}

/* ── Enumeration and table self-consistency ───────────────────────── */

static void test_enumeration(void)
{
    uint32_t n = suas_sisr_entry_count();
    uint32_t i;
    uint32_t rnur = 0u;
    uint32_t conlang = 0u;
    sisr_source_entry_t e;
    sisr_source_entry_t prev;

    CHECK(n > 0u);
    CHECK(n <= SUAS_SISR_REGISTRY_CAPACITY);
    CHECK(suas_sisr_link_count() > 0u);
    CHECK(suas_sisr_link_count() <= SUAS_SISR_LINK_CAPACITY);

    for (i = 0u; i < n; i++) {
        char token[SUAS_SISR_TOKEN_MAX];
        CHECK(suas_sisr_entry_at(i, &e));
        CHECK(suas_sisr_is_registered(e.codepoint));
        CHECK(suas_sisr_is_source_space(e.codepoint));
        CHECK(e.reserved == 0u);
        CHECK((e.registry_flags & (uint16_t)~SISR_SOURCE_ASSIGNED_MASK) == 0u);
        CHECK((e.registry_flags & SISR_SOURCE_SET_MASK) != SISR_SOURCE_SET_MASK);

        /* The stored hash is exactly the canonical token's hash. */
        CHECK(suas_sisr_format_source_id(&e, token, sizeof token));
        {
            size_t len = 0u;
            while (token[len] != '\0')
                len++;
            CHECK(suas_sisr_hash_source_token(token, len) == e.source_id_hash);
        }

        /* Ascending, unique codepoints and unique identities. */
        if (i > 0u) {
            uint32_t j;
            CHECK(suas_sisr_entry_at(i - 1u, &prev));
            CHECK(prev.codepoint < e.codepoint);
            for (j = 0u; j < i; j++) {
                sisr_source_entry_t other;
                CHECK(suas_sisr_entry_at(j, &other));
                CHECK(!(other.source_type == e.source_type &&
                        other.source_serial == e.source_serial));
            }
        }

        if (suas_sisr_is_neography(e.codepoint))
            rnur++;
        if ((e.registry_flags & SISR_SOURCE_CONLANG) != 0u)
            conlang++;
    }
    CHECK(rnur > 0u && conlang > 0u);

    CHECK(!suas_sisr_entry_at(n, &e));
    CHECK(!suas_sisr_entry_at(0u, NULL));
    CHECK(!suas_sisr_link_at(suas_sisr_link_count(), NULL));
}

/* ── Verification: the source table (§9) ──────────────────────────── */

static void test_validate_table(void)
{
    sisr_source_entry_t good[3];
    sisr_source_entry_t bad[3];
    sisr_registry_error_t err = (sisr_registry_error_t)-1;

    /* An empty registry is conformant, with or without a pointer. */
    CHECK(suas_sisr_validate_table(NULL, 0u, &err));
    CHECK(err == SISR_REGISTRY_OK);
    CHECK(suas_sisr_validate_table(good, 0u, NULL));

    /* A well-formed pair. */
    good[0].codepoint = 0x00003402u;
    good[0].source_serial = 0x00000937u;
    good[0].registry_flags = SISR_SOURCE_HISTORICAL;
    good[0].source_type = (uint8_t)SISR_SOURCE_TYPE_UCI;
    good[0].reserved = 0u;
    good[0].source_id_hash = suas_sisr_hash_source_token("UCI-02359", 9u);
    good[1] = good[0];
    good[1].codepoint = 0x00160100u;
    good[1].source_serial = 0x00000100u;
    good[1].registry_flags = (SISR_SOURCE_RNUR_SET1 | SISR_SOURCE_VERIFIED);
    good[1].source_type = (uint8_t)SISR_SOURCE_TYPE_RNUR;
    good[1].source_id_hash = suas_sisr_hash_source_token("RNUR-00256", 10u);
    CHECK(suas_sisr_validate_table(good, 2u, &err));
    CHECK(err == SISR_REGISTRY_OK);

    /* NULL_TABLE: a count with no pointer. */
    CHECK(!suas_sisr_validate_table(NULL, 1u, &err));
    CHECK(err == SISR_REGISTRY_NULL_TABLE);

    /* COUNT_OVER. */
    CHECK(!suas_sisr_validate_table(good, SUAS_SISR_REGISTRY_CAPACITY + 1u,
                                    &err));
    CHECK(err == SISR_REGISTRY_COUNT_OVER);

    /* OUT_OF_SPACE: above the 31-bit Base space. */
    bad[0] = good[0];
    bad[0].codepoint = 0x80000000u;
    CHECK(!suas_sisr_validate_table(bad, 1u, &err));
    CHECK(err == SISR_REGISTRY_OUT_OF_SPACE);

    /* INADMISSIBLE_CODEPOINT: the Control Plane. */
    bad[0] = good[0];
    bad[0].codepoint = 0x00110000u;
    CHECK(!suas_sisr_validate_table(bad, 1u, &err));
    CHECK(err == SISR_REGISTRY_INADMISSIBLE_CODEPOINT);

    /* BAD_SOURCE_TYPE: authority zero. */
    bad[0] = good[0];
    bad[0].source_type = 0u;
    CHECK(!suas_sisr_validate_table(bad, 1u, &err));
    CHECK(err == SISR_REGISTRY_BAD_SOURCE_TYPE);

    /* BAD_SOURCE_ID: serial zero, and serial above 24 bits. */
    bad[0] = good[0];
    bad[0].source_serial = 0u;
    CHECK(!suas_sisr_validate_table(bad, 1u, &err));
    CHECK(err == SISR_REGISTRY_BAD_SOURCE_ID);
    bad[0] = good[0];
    bad[0].source_serial = 0x01000000u;
    CHECK(!suas_sisr_validate_table(bad, 1u, &err));
    CHECK(err == SISR_REGISTRY_BAD_SOURCE_ID);

    /* BAD_FLAGS: a reserved bit, and both RNUR set bits at once. */
    bad[0] = good[0];
    bad[0].registry_flags = (uint16_t)0x8000u;
    CHECK(!suas_sisr_validate_table(bad, 1u, &err));
    CHECK(err == SISR_REGISTRY_BAD_FLAGS);
    bad[0] = good[0];
    bad[0].registry_flags = (SISR_SOURCE_RNUR_SET1 | SISR_SOURCE_RNUR_SET2);
    CHECK(!suas_sisr_validate_table(bad, 1u, &err));
    CHECK(err == SISR_REGISTRY_BAD_FLAGS);

    /* BAD_RESERVED. */
    bad[0] = good[0];
    bad[0].reserved = 1u;
    CHECK(!suas_sisr_validate_table(bad, 1u, &err));
    CHECK(err == SISR_REGISTRY_BAD_RESERVED);

    /* UNORDERED and DUPLICATE_CODEPOINT. */
    bad[0] = good[1];
    bad[1] = good[0];
    CHECK(!suas_sisr_validate_table(bad, 2u, &err));
    CHECK(err == SISR_REGISTRY_UNORDERED);
    bad[0] = good[0];
    bad[1] = good[0];
    CHECK(!suas_sisr_validate_table(bad, 2u, &err));
    CHECK(err == SISR_REGISTRY_DUPLICATE_CODEPOINT);

    /* DUPLICATE_SOURCE_ID: a different codepoint, the same authority+serial. */
    bad[0] = good[0];
    bad[1] = good[0];
    bad[1].codepoint = 0x00009F99u;
    CHECK(!suas_sisr_validate_table(bad, 2u, &err));
    CHECK(err == SISR_REGISTRY_DUPLICATE_SOURCE_ID);

    /* HASH_MISMATCH: a stale hash. */
    bad[0] = good[0];
    bad[0].source_id_hash ^= 1u;
    CHECK(!suas_sisr_validate_table(bad, 1u, &err));
    CHECK(err == SISR_REGISTRY_HASH_MISMATCH);
}

/* ── Verification: the link table (§9) ────────────────────────────── */

static void test_validate_links(void)
{
    sisr_variant_link_t good[2];
    sisr_variant_link_t bad[2];
    sisr_registry_error_t err = (sisr_registry_error_t)-1;

    CHECK(suas_sisr_validate_links(NULL, 0u, &err));
    CHECK(err == SISR_REGISTRY_OK);

    good[0].codepoint = 0x00009F8Du;
    good[0].fallback_codepoint = 0x00160200u;
    good[0].link_flags = (SISR_LINK_REGISTRY | SISR_LINK_TRADITIONAL);
    good[0].reserved = 0u;
    good[1].codepoint = 0x0000F900u;
    good[1].fallback_codepoint = 0x00008C48u;
    good[1].link_flags = SISR_LINK_COMPATIBILITY;
    good[1].reserved = 0u;
    CHECK(suas_sisr_validate_links(good, 2u, &err));
    CHECK(err == SISR_REGISTRY_OK);

    CHECK(!suas_sisr_validate_links(NULL, 1u, &err));
    CHECK(err == SISR_REGISTRY_NULL_TABLE);

    CHECK(!suas_sisr_validate_links(good, SUAS_SISR_LINK_CAPACITY + 1u, &err));
    CHECK(err == SISR_REGISTRY_COUNT_OVER);

    bad[0] = good[0];
    bad[0].fallback_codepoint = 0x80000000u;
    CHECK(!suas_sisr_validate_links(bad, 1u, &err));
    CHECK(err == SISR_REGISTRY_OUT_OF_SPACE);

    bad[0] = good[0];
    bad[0].fallback_codepoint = 0x00110001u;
    CHECK(!suas_sisr_validate_links(bad, 1u, &err));
    CHECK(err == SISR_REGISTRY_INADMISSIBLE_CODEPOINT);

    bad[0] = good[0];
    bad[0].reserved = 7u;
    CHECK(!suas_sisr_validate_links(bad, 1u, &err));
    CHECK(err == SISR_REGISTRY_BAD_RESERVED);

    /* A reserved flag bit and an unflagged link. */
    bad[0] = good[0];
    bad[0].link_flags = (uint16_t)0x0100u;
    CHECK(!suas_sisr_validate_links(bad, 1u, &err));
    CHECK(err == SISR_REGISTRY_BAD_FLAGS);
    bad[0] = good[0];
    bad[0].link_flags = 0u;
    CHECK(!suas_sisr_validate_links(bad, 1u, &err));
    CHECK(err == SISR_REGISTRY_BAD_LINKAGE);

    /* A self-link. */
    bad[0] = good[0];
    bad[0].fallback_codepoint = bad[0].codepoint;
    CHECK(!suas_sisr_validate_links(bad, 1u, &err));
    CHECK(err == SISR_REGISTRY_BAD_LINKAGE);

    /* UNORDERED and duplicate source codepoints. */
    bad[0] = good[1];
    bad[1] = good[0];
    CHECK(!suas_sisr_validate_links(bad, 2u, &err));
    CHECK(err == SISR_REGISTRY_UNORDERED);
    bad[0] = good[0];
    bad[1] = good[0];
    CHECK(!suas_sisr_validate_links(bad, 2u, &err));
    CHECK(err == SISR_REGISTRY_DUPLICATE_CODEPOINT);

    /* The shipped tables pass their own verifier. */
    {
        uint32_t i;
        sisr_variant_link_t all[SUAS_SISR_LINK_CAPACITY];
        sisr_source_entry_t src[SUAS_SISR_REGISTRY_CAPACITY];
        for (i = 0u; i < suas_sisr_link_count(); i++)
            CHECK(suas_sisr_link_at(i, &all[i]));
        CHECK(suas_sisr_validate_links(all, suas_sisr_link_count(), &err));
        CHECK(err == SISR_REGISTRY_OK);
        for (i = 0u; i < suas_sisr_entry_count(); i++)
            CHECK(suas_sisr_entry_at(i, &src[i]));
        CHECK(suas_sisr_validate_table(src, suas_sisr_entry_count(), &err));
        CHECK(err == SISR_REGISTRY_OK);

        /* Every link's source form is itself a registered source. */
        for (i = 0u; i < suas_sisr_link_count(); i++)
            CHECK(suas_sisr_is_registered(all[i].codepoint));
    }
}

/* ── SSRE registration (§10) ──────────────────────────────────────── */

static void test_ssre_registration(void)
{
    uint8_t major = 0u;
    uint8_t minor = 0u;

    CHECK(suas_ssre_is_registered(SUAS_SISR_SPEC_ID));
    CHECK(suas_ssre_is_mandatory(SUAS_SISR_SPEC_ID));
    CHECK(suas_ssre_version_of(SUAS_SISR_SPEC_ID, &major, &minor));
    CHECK(major == 1u && minor == 0u);
    CHECK(suas_ssre_satisfies(SUAS_SISR_SPEC_ID, 1u, 0u));
    CHECK(suas_ssre_module_of(SUAS_SISR_SPEC_ID) != NULL);
    CHECK(suas_ssre_module_of(SUAS_SISR_SPEC_ID)[0] == 's');
}

int main(void)
{
    test_version_and_meta();
    test_zones();
    test_neography_set();
    test_hash();
    test_token_format();
    test_taxonomy();
    test_get_source_entry();     /* lazy build path */
    test_init_idempotent();
    test_variant_linkage();
    test_sip_agreement();
    test_enumeration();
    test_validate_table();
    test_validate_links();
    test_ssre_registration();

    if (g_fail) {
        printf("SUAS-013 SISR tests FAILED\n");
        return 1;
    }
    printf("SUAS-013 SISR tests passed\n");
    return 0;
}
