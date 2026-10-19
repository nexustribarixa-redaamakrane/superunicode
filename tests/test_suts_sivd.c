/* Host unit tests for SUTS-004 (SIVD) — System Ideographic Variation Database.
 *
 * These tests exercise the reference implementation in src/suts/suts_sivd.c
 * against docs/suts/SUTS-004-sivd.md. Base codepoints and variation selectors
 * are 64-bit SUCS codepoints, so the same cases cover the Unicode Bridge, the
 * RNUR neography plane, the System Control Plane, the trap slots, and the
 * unbounded ExtSUCS space.
 */

#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <stddef.h>

#include "suts/suts_sivd.h"

/* ── helpers ──────────────────────────────────────────────────────── */

static int ascii_equal(const char* a, const char* b)
{
    size_t i = 0;
    if (!a || !b)
        return 0;
    while (a[i] && b[i]) {
        if (a[i] != b[i])
            return 0;
        i++;
    }
    return a[i] == b[i];
}

/* ── version ──────────────────────────────────────────────────────── */

static void test_version(void)
{
    assert(sivd_version_string() != NULL);
    assert(sivd_version_string()[0] != '\0');
    printf("[PASS] test_version\n");
}

/* ── selector classification (§2.3) ───────────────────────────────── */

static void test_selectors(void)
{
    /* VS1-VS16 (U+FE00-U+FE0F). */
    assert(sivd_is_variation_selector(SUTS_SIVD_VS1_MIN));
    assert(sivd_is_variation_selector(SUTS_SIVD_VS1_MAX));
    assert(!sivd_is_ivs_selector(SUTS_SIVD_VS1_MIN));

    /* VS17-VS256 (U+E0100-U+E01EF): the IVS range. */
    assert(sivd_is_variation_selector(SUTS_SIVD_VS17_MIN));
    assert(sivd_is_variation_selector(SUTS_SIVD_VS17_MAX));
    assert(sivd_is_ivs_selector(SUTS_SIVD_VS17_MIN));
    assert(sivd_is_ivs_selector(SUTS_SIVD_VS17_MAX));

    /* RNUR neography selectors extend the same mechanics. */
    assert(sivd_is_variation_selector(SIVD_RNUR_VS_MIN));
    assert(sivd_is_variation_selector(SIVD_RNUR_VS_MAX));
    assert(sivd_is_ivs_selector(SIVD_RNUR_VS_MIN));

    /* Just outside every admitted range. */
    assert(!sivd_is_variation_selector((sucs_ex_char_t)0x0000FDFFULL));
    assert(!sivd_is_variation_selector((sucs_ex_char_t)0x0000FE10ULL));
    assert(!sivd_is_variation_selector((sucs_ex_char_t)0x000E00FFULL));
    assert(!sivd_is_variation_selector((sucs_ex_char_t)0x000E01F0ULL));
    assert(!sivd_is_variation_selector((sucs_ex_char_t)0x0016FEFEULL));
    assert(!sivd_is_variation_selector((sucs_ex_char_t)0x00170000ULL));
    assert(!sivd_is_variation_selector((sucs_ex_char_t)0x0041ULL));

    printf("[PASS] test_selectors\n");
}

/* ── base classification and SCP invariance (§4.4) ────────────────── */

static void test_valid_base(void)
{
    assert(sivd_is_valid_base((sucs_ex_char_t)0x0000ULL));
    assert(sivd_is_valid_base((sucs_ex_char_t)0x4E00ULL));
    assert(sivd_is_valid_base(SIVD_RNUR_MIN));
    assert(sivd_is_valid_base(SIVD_RNUR_MAX));
    assert(sivd_is_valid_base(SIVD_ZONE_NATIVE_LAST));

    /* SCP markers, traps, and the Sentinel are never variation bases. */
    assert(!sivd_is_valid_base(SIVD_ZONE_SCP_MIN));
    assert(!sivd_is_valid_base(SIVD_ZONE_SCP_MAX));
    assert(!sivd_is_valid_base(SIVD_ZONE_TRAP_MIN));
    assert(!sivd_is_valid_base(SIVD_ZONE_TRAP_MAX));
    assert(!sivd_is_valid_base(SIVD_ZONE_SENTINEL));

    /* ExtSUCS plugin space above the 31-bit base is admitted. */
    assert(sivd_is_valid_base((sucs_ex_char_t)0x80000000ULL));

    printf("[PASS] test_valid_base\n");
}

/* ── registry and enumeration (§4.1) ──────────────────────────────── */

static void test_registry(void)
{
    size_t n, nc, i, j, covered = 0;

    sivd_init_registry();
    sivd_init_registry(); /* idempotent */

    n = sivd_entry_count();
    nc = sivd_collection_count();
    assert(n >= 20u);
    assert(n <= SUTS_SIVD_REGISTRY_CAPACITY);
    assert(nc >= 6u);
    assert(nc <= SUTS_SIVD_COLLECTION_CAPACITY);

    assert(sivd_entry_at(0) != NULL);
    assert(sivd_entry_at(n - 1u) != NULL);
    assert(sivd_entry_at(n) == NULL);
    assert(sivd_entry_at(n + 5u) == NULL);
    assert(sivd_collection_at(0) != NULL);
    assert(sivd_collection_at(nc - 1u) != NULL);
    assert(sivd_collection_at(nc) == NULL);

    /* Every entry is SCP-invariant, well-formed, and uniquely keyed. */
    for (i = 0; i < n; i++) {
        const sivd_variation_entry_t* a = sivd_entry_at(i);
        assert(a != NULL);
        assert(sivd_is_valid_base(a->base_codepoint));
        assert(sivd_is_variation_selector(a->variation_selector));
        assert(a->plane == (uint8_t)sivd_plane_of(a->base_codepoint));
        assert(a->collection_index < nc);
        for (j = i + 1u; j < n; j++) {
            const sivd_variation_entry_t* b = sivd_entry_at(j);
            assert(b != NULL);
            assert(!(a->base_codepoint == b->base_codepoint &&
                     a->variation_selector == b->variation_selector));
        }
    }

    /* The collection runs are contiguous, aligned, and cover the table. */
    for (i = 0; i < nc; i++) {
        const sivd_collection_t* c = sivd_collection_at(i);
        size_t k;
        assert(c != NULL);
        assert(c->id[0] != '\0');
        assert(c->id_pattern[0] != '\0');
        assert((size_t)c->first_entry == covered);
        assert((size_t)c->first_entry + (size_t)c->entry_count <= n);
        for (k = 0; k < (size_t)c->entry_count; k++) {
            const sivd_variation_entry_t* e =
                sivd_entry_at((size_t)c->first_entry + k);
            assert(e != NULL);
            assert(e->collection_index == (uint16_t)i);
            assert(e->plane == c->plane);
        }
        covered += (size_t)c->entry_count;
    }
    assert(covered == n);

    printf("[PASS] test_registry\n");
}

/* ── O(1) resolution (§4.2) ───────────────────────────────────────── */

static void test_resolve(void)
{
    const sivd_variation_entry_t* e;
    uint32_t glyph = 0u;
    size_t   i;

    /* The Annex B worked example: <82A6 E0134> -> glyph 80. */
    assert(sivd_resolve_variant(0x82A6ULL, 0xE0134ULL, &glyph));
    assert(glyph == 80u);

    /* A few concrete mappings. */
    assert(sivd_resolve_variant(0x3402ULL, 0xE0100ULL, &glyph));
    assert(glyph == 16u);
    assert(sivd_resolve_variant(0x4E00ULL, 0xE0102ULL, &glyph));
    assert(glyph == 49u);
    assert(sivd_resolve_variant(0x9FB4ULL, 0xE0100ULL, &glyph));
    assert(glyph == 64u);

    /* An RNUR neography variant. */
    assert(sivd_resolve_variant(0x160200ULL, 0x16FF01ULL, &glyph));
    assert(glyph == 100u);

    /* A NULL out pointer is tolerated (probe-only resolution). */
    assert(sivd_resolve_variant(0x82A6ULL, 0xE0134ULL, NULL));

    /* Every entry resolves back to its own glyph index. */
    for (i = 0; i < sivd_entry_count(); i++) {
        const sivd_variation_entry_t* a = sivd_entry_at(i);
        uint32_t g = 0u;
        assert(a != NULL);
        assert(sivd_resolve_variant(a->base_codepoint, a->variation_selector, &g));
        assert(g == a->resolved_glyph_index);
    }

    /* `sivd_lookup` returns the very same static record. */
    e = sivd_lookup(0x82A6ULL, 0xE0134ULL);
    assert(e != NULL);
    assert(e->base_codepoint == (sucs_ex_char_t)0x82A6);
    assert(e->variation_selector == (sucs_ex_char_t)0xE0134);
    assert(e == sivd_entry_at(14u));
    assert(sivd_lookup(0x3402ULL, 0xE0100ULL) == sivd_entry_at(0u));

    /* Misses: unregistered pair, malformed selector, and SCP-invading base. */
    assert(!sivd_resolve_variant(0x4E01ULL, 0xE0100ULL, &glyph));
    assert(sivd_lookup(0x4E01ULL, 0xE0100ULL) == NULL);
    assert(sivd_lookup(0x82A6ULL, 0x0041ULL) == NULL);      /* not a selector */
    assert(sivd_lookup(0x82A6ULL, 0xE01F0ULL) == NULL);     /* past VS256     */
    assert(sivd_lookup(0x110000ULL, 0xE0100ULL) == NULL);   /* SCP base       */
    assert(sivd_lookup(0x7FFFFFF5ULL, 0xE0100ULL) == NULL); /* trap base      */
    assert(sivd_lookup(0x7FFFFFFFULL, 0xE0100ULL) == NULL); /* Sentinel base  */
    assert(!sivd_is_registered(0x4E01ULL, 0xE0100ULL));
    assert(sivd_is_registered(0x82A6ULL, 0xE0134ULL));

    printf("[PASS] test_resolve\n");
}

/* ── collection attribution (§2.2) ────────────────────────────────── */

static void test_collections(void)
{
    const sivd_collection_t* c;

    c = sivd_collection_of(0x3402ULL, 0xE0101ULL);
    assert(c != NULL && ascii_equal(c->id, "Hanyo-Denshi"));

    c = sivd_collection_of(0x82A6ULL, 0xE0134ULL);
    assert(c != NULL && ascii_equal(c->id, "Example_names"));
    assert(ascii_equal(c->id_pattern, "[0-9]+"));
    assert(c->entry_count == 1u);

    c = sivd_collection_of(0x160200ULL, 0x16FF00ULL);
    assert(c != NULL && ascii_equal(c->id, "Lumer_Variants"));
    assert(c->plane == SUTS_SIVD_PLANE_RNUR);

    assert(sivd_collection_of(0x4E01ULL, 0xE0100ULL) == NULL);
    assert(sivd_collection_of(0x110000ULL, 0xE0100ULL) == NULL);

    /* Flags survive for stylistic and regional variants. */
    {
        const sivd_variation_entry_t* e = sivd_lookup(0x160100ULL, 0x16FF01ULL);
        assert(e != NULL);
        assert((e->flags & SUTS_SIVD_FLAG_STYLISTIC) != 0u);
        e = sivd_lookup(0x160200ULL, 0x16FF00ULL);
        assert(e != NULL);
        assert((e->flags & SUTS_SIVD_FLAG_REGIONAL) != 0u);
    }

    printf("[PASS] test_collections\n");
}

/* ── plane mapping (§5) ───────────────────────────────────────────── */

static void test_plane(void)
{
    assert(sivd_plane_of((sucs_ex_char_t)0x0041ULL) == SUTS_SIVD_PLANE_BRIDGE);
    assert(sivd_plane_of(SIVD_ZONE_UNICODE_MAX) == SUTS_SIVD_PLANE_BRIDGE);
    assert(sivd_plane_of(SIVD_ZONE_SCP_MIN) == SUTS_SIVD_PLANE_SCP);
    assert(sivd_plane_of(SIVD_ZONE_NATIVE_MIN) == SUTS_SIVD_PLANE_NATIVE);
    assert(sivd_plane_of(SIVD_RNUR_MIN) == SUTS_SIVD_PLANE_RNUR);
    assert(sivd_plane_of(SIVD_RNUR_MAX) == SUTS_SIVD_PLANE_RNUR);
    assert(sivd_plane_of(SIVD_RNUR_MAX + 1ULL) == SUTS_SIVD_PLANE_NATIVE);
    assert(sivd_plane_of(SIVD_ZONE_NATIVE_LAST) == SUTS_SIVD_PLANE_NATIVE);
    assert(sivd_plane_of(SIVD_ZONE_TRAP_MIN) == SUTS_SIVD_PLANE_TRAP);
    assert(sivd_plane_of(SIVD_ZONE_TRAP_MAX) == SUTS_SIVD_PLANE_TRAP);
    assert(sivd_plane_of(SIVD_ZONE_SENTINEL) == SUTS_SIVD_PLANE_SENTINEL);
    assert(sivd_plane_of((sucs_ex_char_t)0x80000000ULL) == SUTS_SIVD_PLANE_EXTSUCS);

    printf("[PASS] test_plane\n");
}

/* ── pair hash (§4.2) ─────────────────────────────────────────────── */

static void test_pair_hash(void)
{
    /* Deterministic. */
    assert(sivd_pair_hash(0x82A6ULL, 0xE0134ULL) ==
           sivd_pair_hash(0x82A6ULL, 0xE0134ULL));

    /* Order-sensitive and value-sensitive. */
    assert(sivd_pair_hash(0x3402ULL, 0xE0100ULL) !=
           sivd_pair_hash(0xE0100ULL, 0x3402ULL));
    assert(sivd_pair_hash(0x3402ULL, 0xE0100ULL) !=
           sivd_pair_hash(0x3402ULL, 0xE0101ULL));
    assert(sivd_pair_hash(0x3402ULL, 0xE0100ULL) !=
           sivd_pair_hash(0x3403ULL, 0xE0100ULL));

    printf("[PASS] test_pair_hash\n");
}

int main(void)
{
    test_version();
    test_selectors();
    test_valid_base();
    test_registry();
    test_resolve();
    test_collections();
    test_plane();
    test_pair_hash();
    printf("All SUTS-004 (SIVD) tests passed.\n");
    return 0;
}
