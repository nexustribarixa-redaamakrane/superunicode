/* Host unit tests for SUTS-003 (SLDF) — System Locale Data & Formatting.
 *
 * These tests exercise the reference implementation in src/suts/suts_sldf.c
 * against docs/suts/SUTS-003-sldf.md. Separators and symbols are 64-bit SUCS
 * codepoints, so the same cases cover the Unicode Bridge, the System Control
 * Plane, the RNUR neography plane, and the unbounded ExtSUCS space.
 */

#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <stddef.h>

#include "suts/suts_sldf.h"

/* ── helpers ──────────────────────────────────────────────────────── */

static size_t strlen_ascii(const char* s)
{
    size_t n = 0;
    while (s[n]) n++;
    return n;
}

/* ── version ──────────────────────────────────────────────────────── */

static void test_version(void)
{
    assert(sldf_version_string() != NULL);
    assert(sldf_version_string()[0] != '\0');
    printf("[PASS] test_version\n");
}

/* ── RCCR tag helpers (§3) ────────────────────────────────────────── */

static void test_tag_helpers(void)
{
    /* FNV-1a is deterministic and case-insensitive after normalization. */
    assert(sldf_tag_hash("en-US") == sldf_tag_hash("EN-us"));
    assert(sldf_tag_hash("lumer") == sldf_tag_hash("LUMER"));
    assert(sldf_tag_hash(NULL) == SUTS_SLDF_FNV_OFFSET_BASIS);

    /* Well-formed RCCR subtags. */
    assert(sldf_is_valid_tag("en"));
    assert(sldf_is_valid_tag("en-US"));
    assert(sldf_is_valid_tag("zh-Hans-CN"));
    assert(sldf_is_valid_tag("es-419"));   /* region may start with a digit */
    assert(sldf_is_valid_tag("lumer"));
    assert(sldf_is_valid_tag("kupon"));

    /* Malformed tags. */
    assert(!sldf_is_valid_tag(""));
    assert(!sldf_is_valid_tag("-en"));
    assert(!sldf_is_valid_tag("en-"));
    assert(!sldf_is_valid_tag("en--US"));
    assert(!sldf_is_valid_tag("en_US"));
    assert(!sldf_is_valid_tag("1en"));
    assert(!sldf_is_valid_tag("abcdefghij-klmno")); /* subtag > 8 bytes */
    assert(!sldf_is_valid_tag("en-US-verylongtag")); /* key > 15 bytes */
    assert(!sldf_is_valid_tag(NULL));

    /* R2: case-insensitive, hyphen-normalized equality. */
    assert(sldf_tags_equal("en-US", "EN-us"));
    assert(sldf_tags_equal("lumer", "LUMER"));
    assert(!sldf_tags_equal("en-US", "en-GB"));
    assert(!sldf_tags_equal("en", "en-US"));
    assert(!sldf_tags_equal("en-US", NULL));

    printf("[PASS] test_tag_helpers\n");
}

/* ── registry and enumeration (§4.1) ──────────────────────────────── */

static void test_registry(void)
{
    size_t i, j, n;

    sldf_init_registry();
    sldf_init_registry(); /* idempotent */

    n = sldf_locale_count();
    assert(n >= 15u);
    assert(n <= SUTS_SLDF_REGISTRY_CAPACITY);

    assert(sldf_locale_at(0) != NULL);
    assert(sldf_locale_at(n - 1u) != NULL);
    assert(sldf_locale_at(n) == NULL);
    assert(sldf_locale_at(n + 5u) == NULL);

    /* Every tag is well-formed and unique under R2 (SLDF-R3). */
    for (i = 0; i < n; i++) {
        const sldf_locale_t* a = sldf_locale_at(i);
        assert(a != NULL);
        assert(sldf_is_valid_tag(a->tag_key));
        for (j = i + 1u; j < n; j++) {
            const sldf_locale_t* b = sldf_locale_at(j);
            assert(b != NULL);
            assert(!sldf_tags_equal(a->tag_key, b->tag_key));
        }
    }

    printf("[PASS] test_registry\n");
}

/* ── O(1) resolution (§4.2) ───────────────────────────────────────── */

static void test_resolve(void)
{
    const sldf_locale_t* loc;

    loc = sldf_resolve_locale("en-US");
    assert(loc != NULL);
    assert(sldf_tags_equal(loc->tag_key, "en-US"));

    /* Case-insensitive, hyphen-preserving lookup. */
    assert(sldf_resolve_locale("DE-de") != NULL);
    assert(sldf_resolve_locale("LUMER") == sldf_resolve_locale("lumer"));

    /* Every registry entry resolves back to its own block. */
    {
        size_t i;
        for (i = 0; i < sldf_locale_count(); i++) {
            const sldf_locale_t* a = sldf_locale_at(i);
            assert(a != NULL);
            assert(sldf_resolve_locale(a->tag_key) == a);
        }
    }

    /* Misses and malformed input. */
    assert(sldf_resolve_locale("xx-YY") == NULL);
    assert(sldf_resolve_locale("not valid!") == NULL);
    assert(sldf_resolve_locale("") == NULL);
    assert(sldf_resolve_locale(NULL) == NULL);

    printf("[PASS] test_resolve\n");
}

/* ── numeric formatting domain (§2.2) ─────────────────────────────── */

static void test_numeric(void)
{
    const sldf_locale_t* us = sldf_resolve_locale("en-US");
    const sldf_locale_t* de = sldf_resolve_locale("de-DE");
    const sldf_locale_t* fr = sldf_resolve_locale("fr-FR");
    const sldf_locale_t* hi = sldf_resolve_locale("hi-IN");
    const sldf_locale_t* ar = sldf_resolve_locale("ar-EG");
    const sldf_locale_t* ku = sldf_resolve_locale("kupon");

    assert(us && de && fr && hi && ar && ku);

    /* en-US: 1,234.56 with a prefixed '$'. */
    assert(us->numeric.decimal_separator == (sucs_ex_char_t)0x002E);
    assert(us->numeric.grouping_separator == (sucs_ex_char_t)0x002C);
    assert(us->numeric.grouping_primary == 3u);
    assert(us->numeric.grouping_secondary == 3u);
    assert(us->numeric.currency_symbol[0] == (sucs_ex_char_t)0x0024);
    assert(us->numeric.currency_symbol_length == 1u);
    assert(us->numeric.currency_position == SUTS_SLDF_AFFIX_PREFIX);
    assert(us->numeric.currency_space == 0u);

    /* de-DE: decimals and groups are transposed; '€' trails, spaced. */
    assert(de->numeric.decimal_separator == (sucs_ex_char_t)0x002C);
    assert(de->numeric.grouping_separator == (sucs_ex_char_t)0x002E);
    assert(de->numeric.currency_symbol[0] == (sucs_ex_char_t)0x20AC);
    assert(de->numeric.currency_position == SUTS_SLDF_AFFIX_SUFFIX);
    assert(de->numeric.currency_space == 1u);

    /* fr-FR uses the narrow no-break space as the grouping separator. */
    assert(fr->numeric.grouping_separator == (sucs_ex_char_t)0x202F);

    /* hi-IN: Indian lakh/crore grouping (3 then 2). */
    assert(hi->numeric.grouping_primary == 3u);
    assert(hi->numeric.grouping_secondary == 2u);
    assert(hi->numeric.currency_symbol[0] == (sucs_ex_char_t)0x20B9); /* ₹ */

    /* ar-EG: Arabic decimal / thousands separators survive as codepoints. */
    assert(ar->numeric.decimal_separator == (sucs_ex_char_t)0x066B);
    assert(ar->numeric.grouping_separator == (sucs_ex_char_t)0x066C);
    assert(ar->numeric.currency_symbol_length == 3u);

    /* kupon: RNUR separators, multi-codepoint trailing symbol. */
    assert(ku->numeric.decimal_separator == (sucs_ex_char_t)0x160012);
    assert(ku->numeric.grouping_separator == (sucs_ex_char_t)0x160013);
    assert(ku->numeric.currency_symbol_length == 2u);
    assert(ku->numeric.currency_position == SUTS_SLDF_AFFIX_SUFFIX);

    printf("[PASS] test_numeric\n");
}

/* ── temporal formatting domain (§2.3) ────────────────────────────── */

static void test_temporal(void)
{
    const sldf_locale_t* us = sldf_resolve_locale("en-US");
    const sldf_locale_t* de = sldf_resolve_locale("de-DE");
    const sldf_locale_t* jp = sldf_resolve_locale("ja-JP");
    const sldf_locale_t* ar = sldf_resolve_locale("ar-EG");
    const sldf_locale_t* th = sldf_resolve_locale("th-TH");

    assert(us && de && jp && ar && th);

    /* Order vectors. */
    assert(us->temporal.date_order == SUTS_SLDF_DATE_MDY);
    assert(de->temporal.date_order == SUTS_SLDF_DATE_DMY);
    assert(jp->temporal.date_order == SUTS_SLDF_DATE_YMD);

    /* Separators and the 12/24-hour flag. */
    assert(us->temporal.date_separator == (sucs_ex_char_t)0x002F);
    assert(us->temporal.time_separator == (sucs_ex_char_t)0x003A);
    assert(us->temporal.use_24_hour == 0u);
    assert(jp->temporal.use_24_hour == 1u);
    assert(de->temporal.date_separator == (sucs_ex_char_t)0x002E);
    assert(de->temporal.use_24_hour == 1u);

    /* Week bounds: ar-EG's week begins Saturday, weekend Fri–Sat. */
    assert(ar->temporal.first_day_of_week == 6u);
    assert(ar->temporal.weekend_start == 5u);
    assert(ar->temporal.weekend_end == 6u);
    assert(us->temporal.first_day_of_week == 0u);

    /* th-TH currency and long month form. */
    assert(th->numeric.currency_symbol[0] == (sucs_ex_char_t)0x0E3F); /* ฿ */
    assert(ar->temporal.long_month_name == 1u);

    printf("[PASS] test_temporal\n");
}

/* ── RTL hints (§2.1) ─────────────────────────────────────────────── */

static void test_rtl(void)
{
    const sldf_locale_t* ar = sldf_resolve_locale("ar-EG");
    const sldf_locale_t* he = sldf_resolve_locale("he-IL");
    const sldf_locale_t* us = sldf_resolve_locale("en-US");

    assert(ar && he && us);
    assert(ar->is_rtl == 1u);
    assert(he->is_rtl == 1u);
    assert(us->is_rtl == 0u);

    printf("[PASS] test_rtl\n");
}

/* ── plane mapping (§5) ───────────────────────────────────────────── */

static void test_plane(void)
{
    assert(sldf_plane_of((sucs_ex_char_t)0x0041) == SUTS_SLDF_PLANE_BRIDGE);
    assert(sldf_plane_of(SLDF_ZONE_UNICODE_MAX) == SUTS_SLDF_PLANE_BRIDGE);
    assert(sldf_plane_of(SLDF_ZONE_SCP_MIN) == SUTS_SLDF_PLANE_SCP);
    assert(sldf_plane_of(SLDF_ZONE_NATIVE_MIN) == SUTS_SLDF_PLANE_NATIVE);
    assert(sldf_plane_of(SLDF_RNUR_MIN) == SUTS_SLDF_PLANE_RNUR);
    assert(sldf_plane_of(SLDF_RNUR_MAX) == SUTS_SLDF_PLANE_RNUR);
    assert(sldf_plane_of(SLDF_RNUR_MAX + 1ULL) == SUTS_SLDF_PLANE_NATIVE);
    assert(sldf_plane_of(SLDF_ZONE_NATIVE_LAST) == SUTS_SLDF_PLANE_NATIVE);
    assert(sldf_plane_of(SLDF_ZONE_TRAP_MIN) == SUTS_SLDF_PLANE_TRAP);
    assert(sldf_plane_of(SLDF_ZONE_TRAP_MAX) == SUTS_SLDF_PLANE_TRAP);
    assert(sldf_plane_of(SLDF_ZONE_SENTINEL) == SUTS_SLDF_PLANE_SENTINEL);
    assert(sldf_plane_of((sucs_ex_char_t)0x80000000ULL) == SUTS_SLDF_PLANE_EXTSUCS);

    printf("[PASS] test_plane\n");
}

/* ── collation seeds (§2.4) ───────────────────────────────────────── */

static void test_collation(void)
{
    const sldf_locale_t* us = sldf_resolve_locale("en-US");
    const sldf_locale_t* lu = sldf_resolve_locale("lumer");
    const sldf_locale_t* ku = sldf_resolve_locale("kupon");

    assert(us && lu && ku);

    /* Bridge plane: base seed 0x000000 + per-locale offset. */
    assert(sldf_collation_base(us) == SLDF_COLLATION_BRIDGE_SEED + 0x0100u);
    /* RNUR plane: base seed 0x160000 + per-locale offset. */
    assert(sldf_collation_base(lu) == SLDF_COLLATION_RNUR_SEED + 0x0100u);
    assert(sldf_collation_base(ku) == SLDF_COLLATION_RNUR_SEED + 0x0200u);

    assert(sldf_collation_base(NULL) == 0u);

    printf("[PASS] test_collation\n");
}

int main(void)
{
    test_version();
    test_tag_helpers();
    test_registry();
    test_resolve();
    test_numeric();
    test_temporal();
    test_rtl();
    test_plane();
    test_collation();
    printf("All SUTS-003 (SLDF) tests passed.\n");
    return 0;
}
