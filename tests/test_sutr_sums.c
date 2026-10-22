/**
 * SUTR-11 SUMS — SuperUnicode Mongolian Shaping & Vertical Direction
 *
 * Unit tests for the zero-allocation, static-table, O(1) resolution of a
 * Mongolian letter's contextual positional form (initial / medial / final /
 * isolated), its fixed-point vertical advance, and its flattened Free
 * Variation Selector (FVS) variation sequence — together with the zone
 * taxonomy of §2 and the taxonomy names of §8 (a UTR #54 analog).
 *
 * Covers: version/meta, the compile-time 12-byte record geometry, zone
 * dispatch and the RNUR neography carve-out, FNV-1a hashing, the curated
 * shaping registry and its default fallback, positional-form resolution across
 * cursive runs, FVS sequence resolution and every error outcome, vertical
 * metrics, enumeration and table self-consistency, taxonomy names, and
 * registration in SUAS-010 (SSRE).
 */

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>

#include "sutr/sutr_sums.h"
#include "suas/suas_ssre.h"

static int g_fail = 0;
#define CHECK(cond) do { if (!(cond)) { \
    printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); g_fail = 1; \
} } while (0)

/* ── Version, limits, and record geometry (§3.2, §4.3) ────────────── */

static void test_version_and_meta(void)
{
    CHECK(sutr_sums_version_string() != NULL);
    CHECK(sutr_sums_version_string()[0] == '0');
    CHECK(sutr_sums_version_string()[2] == '1');

    CHECK(SUMS_SPEC_ID[0] == 'S');
    CHECK(SUMS_SPEC_TITLE[0] == 'S');
    CHECK(SUMS_MODULE[0] == 's');
    CHECK(SUMS_VERSION_MAJOR == 0);
    CHECK(SUMS_VERSION_MINOR == 1);
    CHECK(SUMS_VERSION_PATCH == 0);

    /* The index is a power of two and strictly larger than the capacity, so
     * the open-addressing probe always finds a free slot. */
    CHECK(SUMS_ENTRY_INDEX_SIZE > SUMS_ENTRY_CAPACITY);
    CHECK((SUMS_ENTRY_INDEX_SIZE & (SUMS_ENTRY_INDEX_SIZE - 1u)) == 0u);
    CHECK(SUMS_VARIANT_INDEX_SIZE > SUMS_VARIANT_CAPACITY);
    CHECK((SUMS_VARIANT_INDEX_SIZE & (SUMS_VARIANT_INDEX_SIZE - 1u)) == 0u);

    CHECK(SUMS_FNV_OFFSET_BASIS == 2166136261u);
    CHECK(SUMS_FNV_PRIME == 16777619u);

    /* The shaping record is fixed-size and padding-free (§3.2): 12 bytes. */
    CHECK(sizeof(sums_shaping_entry_t) == 12U);
    CHECK(offsetof(sums_shaping_entry_t, codepoint) == 0U);
    CHECK(offsetof(sums_shaping_entry_t, vertical_advance) == 4U);
    CHECK(offsetof(sums_shaping_entry_t, shaping_flags) == 6U);
    CHECK(offsetof(sums_shaping_entry_t, positional_forms) == 8U);
    CHECK(offsetof(sums_shaping_entry_t, reserved) == 9U);

    /* The flattened variation pair is 12 bytes (§4.3). */
    CHECK(sizeof(sums_variant_pair_t) == 12U);
    CHECK(offsetof(sums_variant_pair_t, base) == 0U);
    CHECK(offsetof(sums_variant_pair_t, selector) == 4U);
    CHECK(offsetof(sums_variant_pair_t, token) == 8U);

    /* The flag bits are disjoint and the assigned mask is the low byte. */
    CHECK((SUMS_FLAG_CURSIVE & SUMS_FLAG_VERTICAL) == 0u);
    CHECK((SUMS_FLAG_FVS & SUMS_FLAG_ASSIGNED_MASK) != 0u);
    CHECK((SUMS_FLAG_FEMININE & SUMS_FLAG_ASSIGNED_MASK) != 0u);
    CHECK(SUMS_FLAG_ASSIGNED_MASK == 0x00FFu);

    /* The positional-form constants are the four canonical shapes (§3.3). */
    CHECK(SUMS_FORM_ISOLATED == 0u);
    CHECK(SUMS_FORM_INITIAL == 1u);
    CHECK(SUMS_FORM_MEDIAL == 2u);
    CHECK(SUMS_FORM_FINAL == 3u);

    /* The Free Variation Selectors carry their UTR #54 codepoints (§4). */
    CHECK(SUMS_FVS1 == 0x0000180Bu);
    CHECK(SUMS_FVS2 == 0x0000180Cu);
    CHECK(SUMS_FVS3 == 0x0000180Du);
    CHECK(SUMS_FVS_MIN == 0x0000180Bu);
    CHECK(SUMS_FVS_MAX == 0x0000180Du);
    CHECK(SUMS_MVS == 0x0000180Eu);
    CHECK(SUMS_NNBSP == 0x0000202Fu);
}

/* ── Zone dispatch and the RNUR carve-out (§2.3) ──────────────────── */

static void test_zones(void)
{
    CHECK(sutr_sums_zone_of(0x1820u) == SUMS_ZONE_BRIDGE);
    CHECK(sutr_sums_zone_of(0x0010FFFFu) == SUMS_ZONE_BRIDGE);
    CHECK(sutr_sums_zone_of(0x00110000u) == SUMS_ZONE_SCP);
    CHECK(sutr_sums_zone_of(0x0011FFFFu) == SUMS_ZONE_SCP);
    CHECK(sutr_sums_zone_of(0x00120000u) == SUMS_ZONE_NATIVE);
    CHECK(sutr_sums_zone_of(0x0015FFFFu) == SUMS_ZONE_NATIVE);
    CHECK(sutr_sums_zone_of(0x00160000u) == SUMS_ZONE_RNUR);
    CHECK(sutr_sums_zone_of(0x0016FFFFu) == SUMS_ZONE_RNUR);
    CHECK(sutr_sums_zone_of(0x00170000u) == SUMS_ZONE_NATIVE);
    CHECK(sutr_sums_zone_of(0x7FFFFFF0u) == SUMS_ZONE_TRAP);
    CHECK(sutr_sums_zone_of(0x7FFFFFFEu) == SUMS_ZONE_TRAP);
    CHECK(sutr_sums_zone_of(0x7FFFFFFFu) == SUMS_ZONE_SENTINEL);
    CHECK(sutr_sums_zone_of(0x80000000u) == SUMS_ZONE_EXTSUCS);
    CHECK(sutr_sums_zone_of(0xFFFFFFFFu) == SUMS_ZONE_EXTSUCS);

    /* A shaping query may name Bridge, RNUR and Native only. */
    CHECK(sutr_sums_is_text_space(0x1820u));
    CHECK(sutr_sums_is_text_space(0x00120000u));
    CHECK(sutr_sums_is_text_space(0x00160100u));
    CHECK(!sutr_sums_is_text_space(0x00110000u));
    CHECK(!sutr_sums_is_text_space(0x7FFFFFF0u));
    CHECK(!sutr_sums_is_text_space(0x7FFFFFFFu));
    CHECK(!sutr_sums_is_text_space(0x80000000u));

    CHECK(sutr_sums_is_neography(0x00160000u));
    CHECK(sutr_sums_is_neography(0x0016FFFFu));
    CHECK(!sutr_sums_is_neography(0x0015FFFFu));
    CHECK(!sutr_sums_is_neography(0x1820u));
}

static void test_neography_set(void)
{
    uint32_t set = 0xFFFFFFFFu;

    CHECK(sutr_sums_neography_set(0x00160100u, &set));
    CHECK(set == 1u);                    /* (0x100) >> 8 */
    CHECK(sutr_sums_neography_set(0x00160200u, &set));
    CHECK(set == 2u);
    CHECK(sutr_sums_neography_set(0x0016FFFFu, &set));
    CHECK(set == 0xFFu);

    CHECK(!sutr_sums_neography_set(0x00120000u, &set));
    CHECK(!sutr_sums_neography_set(0x00160100u, NULL));
}

/* ── Hash (§3.3) ──────────────────────────────────────────────────── */

static void test_hash(void)
{
    /* FNV-1a 32-bit over the four little-endian codepoint bytes. */
    CHECK(sutr_sums_hash_codepoint(0x00160100u) ==
          sutr_sums_hash_codepoint(0x00160100u));
    CHECK(sutr_sums_hash_codepoint(0x00160100u) !=
          sutr_sums_hash_codepoint(0x00160101u));
    CHECK(sutr_sums_hash_codepoint(0u) != 0u);
    CHECK(sutr_sums_hash_codepoint(0xFFFFFFFFu) != 0u);

    /* The pair hash is order-sensitive: (base, selector) != (selector, base). */
    CHECK(sutr_sums_hash_pair(0x183Cu, SUMS_FVS1) ==
          sutr_sums_hash_pair(0x183Cu, SUMS_FVS1));
    CHECK(sutr_sums_hash_pair(0x183Cu, SUMS_FVS1) !=
          sutr_sums_hash_pair(SUMS_FVS1, 0x183Cu));
}

/* ── Curated records and the algorithmic default (§3, §6) ─────────── */

static void test_get_entry(void)
{
    sums_shaping_entry_t e;

    sutr_sums_init_registry();

    /* A curated cursive letter: codepoint echoed, cursive + vertical, three
     * distinct positional forms. */
    CHECK(sutr_sums_get_entry(0x1820u, &e));
    CHECK(e.codepoint == 0x1820u);
    CHECK((e.shaping_flags & SUMS_FLAG_CURSIVE) != 0u);
    CHECK((e.shaping_flags & SUMS_FLAG_VERTICAL) != 0u);
    CHECK(e.positional_forms == 0x0Fu);
    CHECK(e.reserved == 0u);

    /* ang (0x1829) joins on both sides but has no distinct initial form. */
    CHECK(sutr_sums_get_entry(0x1829u, &e));
    CHECK(e.positional_forms == 0x0Du);

    /* A variation source letter flags itself as such. */
    CHECK(sutr_sums_get_entry(0x1835u, &e));
    CHECK((e.shaping_flags & SUMS_FLAG_VARIANT_SOURCE) != 0u);

    /* The Free Variation Selectors are curated and flagged. */
    CHECK(sutr_sums_get_entry(SUMS_FVS1, &e));
    CHECK((e.shaping_flags & SUMS_FLAG_FVS) != 0u);
    CHECK(e.positional_forms == 0u);

    /* A Mongolian codepoint with no curated row is not unknown: it falls to
     * the derived default, which is vertical and non-joining. */
    CHECK(sutr_sums_get_entry(0x1843u, &e));
    CHECK(e.codepoint == 0x1843u);
    CHECK(e.positional_forms == 0u);
    CHECK((e.shaping_flags & SUMS_FLAG_VERTICAL) != 0u);
    CHECK((e.shaping_flags & SUMS_FLAG_CURSIVE) == 0u);

    /* A Mongolian Supplement letter is curated. */
    CHECK(sutr_sums_get_entry(0x116600u, &e));
    CHECK((e.shaping_flags & SUMS_FLAG_CURSIVE) != 0u);

    /* An ordinary starter has no Mongolian shaping. */
    CHECK(!sutr_sums_get_entry(0x0041u, &e));
    CHECK(!sutr_sums_get_entry(0x00110000u, &e)); /* SCP */
    CHECK(!sutr_sums_get_entry(0x7FFFFFFFu, &e)); /* Sentinel */
    CHECK(!sutr_sums_get_entry(0x1820u, NULL));
}

static void test_predicates(void)
{
    sutr_sums_init_registry();

    CHECK(sutr_sums_is_mongolian(0x1820u));
    CHECK(sutr_sums_is_mongolian(0x1843u));      /* default-derived */
    CHECK(!sutr_sums_is_mongolian(0x0041u));

    CHECK(sutr_sums_is_cursive(0x1820u));
    CHECK(!sutr_sums_is_cursive(0x1843u));

    CHECK(sutr_sums_is_vertical(0x1820u));
    CHECK(sutr_sums_is_vertical(0x1843u));       /* default is vertical */
    CHECK(!sutr_sums_is_vertical(0x0041u));

    CHECK(sutr_sums_is_fvs(SUMS_FVS1));
    CHECK(sutr_sums_is_fvs(SUMS_FVS3));
    CHECK(!sutr_sums_is_fvs(0x1820u));

    CHECK(sutr_sums_is_variant_source(0x1835u));
    CHECK(sutr_sums_is_variant_source(0x183Cu));
    CHECK(!sutr_sums_is_variant_source(0x1820u));

    CHECK(sutr_sums_is_feminine(0x116608u));
    CHECK(!sutr_sums_is_feminine(0x1820u));

    CHECK(sutr_sums_is_registered(0x1820u));
    CHECK(sutr_sums_is_registered(0x116600u));
    CHECK(!sutr_sums_is_registered(0x1843u));    /* default-derived, not curated */

    /* Positional-form membership. */
    CHECK(sutr_sums_has_form(0x1820u, SUMS_FORM_ISOLATED));
    CHECK(sutr_sums_has_form(0x1820u, SUMS_FORM_INITIAL));
    CHECK(sutr_sums_has_form(0x1820u, SUMS_FORM_MEDIAL));
    CHECK(sutr_sums_has_form(0x1820u, SUMS_FORM_FINAL));
    CHECK(!sutr_sums_has_form(0x1829u, SUMS_FORM_INITIAL));
    CHECK(sutr_sums_has_form(0x1829u, SUMS_FORM_MEDIAL));
    CHECK(!sutr_sums_has_form(0x0041u, SUMS_FORM_INITIAL));
    CHECK(!sutr_sums_has_form(0x1820u, 9u));     /* out-of-range form */
}

/* ── Contextual positional shaping (§3.3) ─────────────────────────── */

static void test_positional_form(void)
{
    sutr_sums_init_registry();

    /* Isolated on both sides: no neighbours. */
    CHECK(sutr_sums_positional_form(0u, 0x1820u, 0u) == SUMS_FORM_ISOLATED);

    /* Initial: joins forward only (a following letter that joins backward). */
    CHECK(sutr_sums_positional_form(0u, 0x1820u, 0x1820u) == SUMS_FORM_INITIAL);

    /* Medial: joins on both sides. */
    CHECK(sutr_sums_positional_form(0x1820u, 0x1820u, 0x1820u) ==
          SUMS_FORM_MEDIAL);

    /* Final: joins backward only. */
    CHECK(sutr_sums_positional_form(0x1820u, 0x1820u, 0u) == SUMS_FORM_FINAL);

    /* A space (ordinary starter) breaks the run on both sides. */
    CHECK(sutr_sums_positional_form(0x0020u, 0x1820u, 0x0020u) ==
          SUMS_FORM_ISOLATED);

    /* The Mongolian vowel separator does not join, so it breaks the run. */
    CHECK(sutr_sums_positional_form(SUMS_MVS, 0x1820u, 0u) ==
          SUMS_FORM_ISOLATED);

    /* A variation selector never takes a positional form of its own. */
    CHECK(sutr_sums_positional_form(0x1820u, SUMS_FVS1, 0x1820u) ==
          SUMS_FORM_ISOLATED);

    /* An uncurated codepoint is isolated. */
    CHECK(sutr_sums_positional_form(0x1820u, 0x1843u, 0x1820u) ==
          SUMS_FORM_ISOLATED);

    /* Every form has a canonical name. */
    CHECK(sutr_sums_form_name(SUMS_FORM_ISOLATED)[0] == 'i');
    CHECK(sutr_sums_form_name(SUMS_FORM_INITIAL)[0] == 'i');
    CHECK(sutr_sums_form_name(SUMS_FORM_MEDIAL)[0] == 'm');
    CHECK(sutr_sums_form_name(SUMS_FORM_FINAL)[0] == 'f');
    CHECK(sutr_sums_form_name(9u)[0] == 'u');   /* "unknown" */
}

/* ── Vertical metrics (§5) ────────────────────────────────────────── */

static void test_vertical_advance(void)
{
    sutr_sums_init_registry();

    /* A curated letter carries the default Mongolian stem. */
    CHECK(sutr_sums_vertical_advance(0x1820u) == 0x0040);
    /* A Supplement letter carries the supplement stem. */
    CHECK(sutr_sums_vertical_advance(0x116600u) == 0x0010);
    /* The separators carry no advance. */
    CHECK(sutr_sums_vertical_advance(SUMS_MVS) == 0);
    /* A default-derived Mongolian falls to the default stem. */
    CHECK(sutr_sums_vertical_advance(0x1843u) == 0x0040);
    /* An ordinary starter, and every inadmissible zone, is zero. */
    CHECK(sutr_sums_vertical_advance(0x0041u) == 0);
    CHECK(sutr_sums_vertical_advance(0x00110000u) == 0);
    CHECK(sutr_sums_vertical_advance(0x7FFFFFFFu) == 0);
    CHECK(sutr_sums_vertical_advance(0x80000000u) == 0);
}

/* ── Flattened variation-sequence resolution (§4) ─────────────────── */

static void test_variant_resolution(void)
{
    sums_shaping_entry_t e;
    sums_status_code_t st = SUMS_OK;

    sutr_sums_init_registry();

    /* A standardized (letter, FVS) pair resolves, and the variant record folds
     * the precompiled token into the codepoint. */
    CHECK(sutr_sums_resolve_variant(0x183Cu, SUMS_FVS1, &e));
    CHECK(e.codepoint == 0x00010001u);
    CHECK((e.shaping_flags & SUMS_FLAG_CURSIVE) != 0u);

    /* The token surface agrees, in O(1). */
    CHECK(sutr_sums_variant_token(0x183Cu, SUMS_FVS1) == 0x00010001u);
    CHECK(sutr_sums_variant_token(0x183Cu, SUMS_FVS2) == 0x00010002u);
    CHECK(sutr_sums_variant_token(0x1835u, SUMS_FVS3) == 0x00030003u);
    CHECK(sutr_sums_variant_token(0x182Au, SUMS_FVS1) == 0x00070001u);

    /* The _ex surface names success and every failure outcome. */
    CHECK(sutr_sums_resolve_variant_ex(0x183Cu, SUMS_FVS1, &e, &st));
    CHECK(st == SUMS_OK);

    /* A NULL descriptor is a null-argument failure. */
    CHECK(!sutr_sums_resolve_variant_ex(0x183Cu, SUMS_FVS1, NULL, &st));
    CHECK(st == SUMS_ERR_NULL_ARGUMENT);

    /* A base or selector in an inadmissible zone is an illegal-zone failure. */
    CHECK(!sutr_sums_resolve_variant_ex(0x00110000u, SUMS_FVS1, &e, &st));
    CHECK(st == SUMS_ERR_ILLEGAL_ZONE);
    CHECK(!sutr_sums_resolve_variant_ex(0x183Cu, 0x7FFFFFFFu, &e, &st));
    CHECK(st == SUMS_ERR_ILLEGAL_ZONE);

    /* An ordinary starter base is not Mongolian. */
    CHECK(!sutr_sums_resolve_variant_ex(0x0041u, SUMS_FVS1, &e, &st));
    CHECK(st == SUMS_ERR_NOT_MONGOLIAN);

    /* A second codepoint that is not an FVS is a bad-selector failure. */
    CHECK(!sutr_sums_resolve_variant_ex(0x183Cu, 0x1820u, &e, &st));
    CHECK(st == SUMS_ERR_BAD_SELECTOR);

    /* A Mongolian letter with no standardized variant is a no-variant miss. */
    CHECK(!sutr_sums_resolve_variant_ex(0x1820u, SUMS_FVS1, &e, &st));
    CHECK(st == SUMS_ERR_NO_VARIANT);

    /* The token surface returns 0 for a pair with no standardized variant. */
    CHECK(sutr_sums_variant_token(0x1820u, SUMS_FVS1) == 0u);

    /* A NULL error sink is allowed. */
    CHECK(!sutr_sums_resolve_variant(0x1820u, SUMS_FVS1, &e));
}

/* ── Enumeration and self-consistency (§3.3) ──────────────────────── */

static void test_enumeration(void)
{
    sums_shaping_entry_t e;
    sums_variant_pair_t p;
    uint32_t i;
    uint32_t n;
    uint32_t seen_variant_sources = 0;

    sutr_sums_init_registry();

    n = sutr_sums_entry_count();
    CHECK(n > 0u);
    CHECK(n <= SUMS_ENTRY_CAPACITY);

    /* The table is strictly ascending in codepoint — the invariant the probe
     * and enumeration both depend on. Any curated row must round-trip. */
    {
        uint32_t prev = 0u;
        for (i = 0u; i < n; i++) {
            CHECK(sutr_sums_entry_at(i, &e));
            if (i != 0u)
                CHECK(e.codepoint > prev);
            prev = e.codepoint;
            CHECK(e.reserved == 0u);
            CHECK(sutr_sums_is_registered(e.codepoint));
            if ((e.shaping_flags & SUMS_FLAG_VARIANT_SOURCE) != 0u)
                seen_variant_sources++;
        }
    }
    CHECK(seen_variant_sources > 0u);

    CHECK(!sutr_sums_entry_at(n, &e));
    CHECK(!sutr_sums_entry_at(0u, NULL));

    /* The flattened variation table round-trips and every row is distinct,
     * non-zero, and points at a curated variant-source base. */
    {
        uint32_t vn = sutr_sums_variant_count();
        uint32_t j;
        CHECK(vn > 0u);
        CHECK(vn <= SUMS_VARIANT_CAPACITY);
        for (i = 0u; i < vn; i++) {
            CHECK(sutr_sums_variant_at(i, &p));
            CHECK(p.token != 0u);
            CHECK(sutr_sums_is_registered(p.base));
            CHECK(sutr_sums_is_variant_source(p.base));
            CHECK(sutr_sums_is_fvs(p.selector));
            CHECK(sutr_sums_variant_token(p.base, p.selector) == p.token);
            for (j = i + 1u; j < vn; j++) {
                sums_variant_pair_t q;
                CHECK(sutr_sums_variant_at(j, &q));
                CHECK(!(p.base == q.base && p.selector == q.selector));
            }
        }
        CHECK(!sutr_sums_variant_at(vn, &p));
        CHECK(!sutr_sums_variant_at(0u, NULL));
    }
}

static void test_init_idempotent(void)
{
    uint32_t before = sutr_sums_entry_count();

    /* A second init must not change the registry — the guard is idempotent. */
    sutr_sums_init_registry();
    sutr_sums_init_registry();
    CHECK(sutr_sums_entry_count() == before);
    CHECK(sutr_sums_is_registered(0x1820u));
    CHECK(sutr_sums_variant_token(0x183Cu, SUMS_FVS1) == 0x00010001u);
}

/* ── Taxonomy names (§8) ──────────────────────────────────────────── */

static void test_taxonomy(void)
{
    CHECK(sutr_sums_zone_name(SUMS_ZONE_BRIDGE)[0] == 'b');
    CHECK(sutr_sums_zone_name(SUMS_ZONE_SCP)[0] == 's');
    CHECK(sutr_sums_zone_name(SUMS_ZONE_RNUR)[0] == 'r');
    CHECK(sutr_sums_zone_name(SUMS_ZONE_NATIVE)[0] == 'n');
    CHECK(sutr_sums_zone_name(SUMS_ZONE_TRAP)[0] == 't');
    CHECK(sutr_sums_zone_name(SUMS_ZONE_SENTINEL)[0] == 's');
    CHECK(sutr_sums_zone_name(SUMS_ZONE_EXTSUCS)[0] == 'e');
    CHECK(sutr_sums_zone_name((sums_zone_t)99)[0] == 'u');  /* "unknown" */

    CHECK(sutr_sums_status_code_name(SUMS_OK)[0] == 'o');
    CHECK(sutr_sums_status_code_name(SUMS_ERR_NULL_ARGUMENT)[0] == 'n');
    CHECK(sutr_sums_status_code_name(SUMS_ERR_NOT_MONGOLIAN)[0] == 'n');
    CHECK(sutr_sums_status_code_name(SUMS_ERR_BAD_SELECTOR)[0] == 'b');
    CHECK(sutr_sums_status_code_name(SUMS_ERR_ILLEGAL_ZONE)[0] == 'i');
    CHECK(sutr_sums_status_code_name(SUMS_ERR_NO_VARIANT)[0] == 'n');
    CHECK(sutr_sums_status_code_name((sums_status_code_t)99)[0] == 'u');
}

/* ── Registration in SSRE (SUAS-010) ──────────────────────────────── */

static void test_ssre_registration(void)
{
    uint8_t major = 0u;
    uint8_t minor = 0u;

    CHECK(suas_ssre_is_registered(SUMS_SPEC_ID));
    CHECK(suas_ssre_version_of(SUMS_SPEC_ID, &major, &minor));
    CHECK(major == 0u && minor == 1u);
    CHECK(suas_ssre_satisfies(SUMS_SPEC_ID, 0u, 1u));

    /* SUMS is a SUTR — a REFERENCE-kind report — but it now ships a real
     * reference unit, so its SSRE row names one. */
    CHECK(suas_ssre_kind_count(SUAS_SSRE_KIND_REFERENCE) == 13u);
    CHECK(suas_ssre_module_of(SUMS_SPEC_ID) != NULL);
    CHECK(suas_ssre_module_of(SUMS_SPEC_ID)[0] == 's');
    CHECK(suas_ssre_spec_of_module(SUMS_MODULE) != NULL);
    CHECK(suas_ssre_spec_of_module(SUMS_MODULE)[0] == 'S');
}

int main(void)
{
    test_version_and_meta();
    test_zones();
    test_neography_set();
    test_hash();
    test_get_entry();
    test_predicates();
    test_positional_form();
    test_vertical_advance();
    test_variant_resolution();
    test_enumeration();
    test_init_idempotent();
    test_taxonomy();
    test_ssre_registration();

    if (g_fail) {
        printf("SUTR-11 SUMS tests FAILED\n");
        return 1;
    }
    printf("SUTR-11 SUMS tests passed\n");
    return 0;
}
