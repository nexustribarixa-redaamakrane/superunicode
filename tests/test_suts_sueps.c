/* Host unit tests for SUTS-007 (SUEPS) — SuperUnicode Emoji & Presentation
 * Specification.
 *
 * These tests exercise the reference implementation in src/suts/suts_sueps.c
 * against docs/suts/SUTS-007-sueps.md. Codepoints are 31-bit SUCS codepoints,
 * so the same cases cover the Unicode Compatibility Bridge, the RNUR neography
 * plane, the System Control Plane, the trap slots, and the Sentinel. The
 * "No Surrogates" mandate (SUTR-1 invariant 2) is tested directly: the block
 * U+D800-U+DFFF is ordinary PUA, never paired and never rejected.
 */

#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <stddef.h>

#include "suts/suts_sueps.h"

/* ── version and layout ───────────────────────────────────────────── */

static void test_version_and_layout(void)
{
    assert(suts_sueps_version_string() != NULL);
    assert(suts_sueps_version_string()[0] == '1');
    assert(SUEPS_SPEC_ID[0] == 'S');
    assert(SUEPS_VERSION_MAJOR == 1);
    assert(SUEPS_VERSION_MINOR == 0);

    /* The emoji record is exactly 12 bytes, byte-aligned (§3.2). */
    assert(sizeof(sueps_emoji_entry_t) == 12u);
    assert(offsetof(sueps_emoji_entry_t, codepoint) == 0u);
    assert(offsetof(sueps_emoji_entry_t, sequence_hash) == 4u);
    assert(offsetof(sueps_emoji_entry_t, presentation_flags) == 8u);
    assert(offsetof(sueps_emoji_entry_t, modifier_base) == 10u);
    assert(offsetof(sueps_emoji_entry_t, reserved) == 11u);

    printf("[PASS] test_version_and_layout\n");
}

/* ── the no-surrogates mandate (§2) ───────────────────────────────── */

static void test_no_surrogates(void)
{
    /* SUTR-1 invariant 2: U+D800-U+DFFF is ordinary valid PUA. SUEPS must not
     * pair, reserve or reject it — the predicate is a classifier, not a
     * defect test. */
    assert(SUEPS_ERR_ILLEGAL_ZONE != SUEPS_OK);
    assert(!suts_sueps_is_pua_half(0xD7FFu));
    assert(suts_sueps_is_pua_half(0xD800u));
    assert(suts_sueps_is_pua_half(0xDC00u));
    assert(suts_sueps_is_pua_half(0xDFFFu));
    assert(!suts_sueps_is_pua_half(0xE000u));

    /* A former-surrogate codepoint is renderable material, not an illegal
     * zone, and the emoji query chain answers it without pairing it. */
    assert(suts_sueps_is_renderable(0xD83Du) == true);
    assert(suts_sueps_zone_of(0xD83Du) == SUEPS_ZONE_BRIDGE);

    /* A Unicode-style emoji is one atomic scalar, and the two surrogate code
     * units of its UTF-16 escape are each standalone valid codepoints — not
     * halves of U+1F600. */
    assert(suts_sueps_is_emoji(0x1F600u) == true);
    assert(suts_sueps_is_pua_half(0x1F600u) == false);

    printf("[PASS] test_no_surrogates\n");
}

/* ── presentation flags taxonomy (§3) ─────────────────────────────── */

static void test_presentation_flags(void)
{
    sueps_emoji_entry_t e;

    suts_sueps_init_registry();
    suts_sueps_init_registry(); /* idempotent */

    /* A default emoji presentation face (ED-6). */
    assert(suts_sueps_get_entry(0x1F600u, &e));
    assert((e.presentation_flags & SUEPS_FLAG_EMOJI) != 0u);
    assert((e.presentation_flags & SUEPS_FLAG_TEXT_DEFAULT) == 0u);
    assert(suts_sueps_is_emoji_presentation(0x1F600u));
    assert(!suts_sueps_is_text_default(0x1F600u));

    /* A default text presentation symbol (ED-7). */
    assert(suts_sueps_get_entry(0x270Fu, &e)); /* ✏ pencil */
    assert((e.presentation_flags & SUEPS_FLAG_EMOJI) != 0u);
    assert((e.presentation_flags & SUEPS_FLAG_TEXT_DEFAULT) != 0u);
    assert(suts_sueps_is_text_default(0x270Fu));
    assert(!suts_sueps_is_emoji_presentation(0x270Fu));

    /* The five modifiers carry SUEPS_FLAG_MODIFIER (ED-11). */
    assert(suts_sueps_is_modifier(0x1F3FBu));
    assert(suts_sueps_is_modifier(0x1F3FFu));
    assert(!suts_sueps_is_modifier(0x1F600u));
    assert(!suts_sueps_is_modifier(0x1F3FAu));

    /* A modifier base carries SUEPS_FLAG_MODIFIER_BASE (ED-12). */
    assert(suts_sueps_is_modifier_base(0x1F44Bu)); /* 👍 */
    assert(suts_sueps_is_modifier_base(0x1F469u)); /* 👩 */
    assert(!suts_sueps_is_modifier_base(0x1F600u));/* 😀 is not a base */

    /* Names round-trip. */
    assert(suts_sueps_zone_name(SUEPS_ZONE_BRIDGE)[0] == 'b');
    assert(suts_sueps_zone_name(SUEPS_ZONE_RNUR)[0] == 'r');
    assert(suts_sueps_status_code_name(SUEPS_OK)[0] == 'o');
    assert(suts_sueps_status_code_name(SUEPS_ERR_BAD_MODIFIER)[0] == 'b');

    printf("[PASS] test_presentation_flags\n");
}

/* ── components and selectors (§3, §5) ────────────────────────────── */

static void test_components_and_selectors(void)
{
    /* Presentation selectors (ED-8, ED-9). */
    assert(suts_sueps_is_presentation_selector(0xFE0Eu));
    assert(suts_sueps_is_presentation_selector(0xFE0Fu));
    assert(!suts_sueps_is_presentation_selector(0x200Du));
    assert(suts_sueps_is_component(0xFE0Fu));
    assert(!suts_sueps_is_emoji(0xFE0Fu)); /* a selector is not an emoji alone */

    /* The zero-width joiner is a component, not an emoji (ED-5). */
    assert(suts_sueps_is_component(0x200Du));
    assert(!suts_sueps_is_emoji(0x200Du));

    /* Keycap bases are components (ED-14c). */
    assert(suts_sueps_is_component(0x0023u)); /* # */
    assert(suts_sueps_is_component(0x002Au)); /* * */
    assert(suts_sueps_is_component(0x0039u)); /* 9 */
    assert(suts_sueps_is_component(0x20E3u)); /* enclosing keycap */

    /* Regional Indicators (ED-14, Annex B). */
    assert(suts_sueps_is_regional_indicator(0x1F1E6u));
    assert(suts_sueps_is_regional_indicator(0x1F1FFu));
    assert(!suts_sueps_is_regional_indicator(0x1F1E5u));
    assert(suts_sueps_is_component(0x1F1E6u));

    /* Tag characters (ED-14a, Annex C). */
    assert(suts_sueps_is_tag_character(0xE0020u));
    assert(suts_sueps_is_tag_character(0xE007Fu));
    assert(!suts_sueps_is_tag_character(0xE001Fu));
    assert(!suts_sueps_is_tag_character(0xE0080u));

    /* Hair components (Emoji 11.0) are components. */
    assert(suts_sueps_is_component(0x1F9B0u));
    assert(suts_sueps_is_component(0x1F9B3u));

    /* Extended_Pictographic (ED-4) holds for the curated emoji and its
     * algorithmic default. ED-4 excludes some emoji components. */
    assert(suts_sueps_is_extended_pictographic(0x1F600u));
    assert(suts_sueps_is_extended_pictographic(0x1F3F4u)); /* black flag */
    assert(!suts_sueps_is_extended_pictographic(0x1F9B0u)); /* a component */

    printf("[PASS] test_components_and_selectors\n");
}

/* ── modifier sequence resolution (§4) ────────────────────────────── */

static void test_sequence_resolution(void)
{
    sueps_emoji_entry_t e;
    sueps_emoji_entry_t plain;
    sueps_status_code_t err = SUEPS_OK;

    /* 👍 + light skin tone resolves in O(1) to a flagged record. */
    assert(suts_sueps_resolve_sequence_ex(0x1F44Bu, 0x1F3FBu, &e, &err));
    assert(err == SUEPS_OK);
    assert((e.presentation_flags & SUEPS_FLAG_MODIFIER) != 0u);
    assert((e.presentation_flags & SUEPS_FLAG_ZWJ_SEQUENCE) != 0u);
    assert(e.codepoint == 0x1F44Bu);
    assert(e.modifier_base == 0u);
    assert(e.sequence_hash == suts_sueps_hash_sequence(0x1F44Bu, 0x1F3FBu));

    /* The sketch's entry point is the same resolution with no diagnostic. */
    assert(suts_sueps_resolve_sequence(0x1F469u, 0x1F3FFu, &e));

    /* The base record is unchanged when the pair is resolved (no mutation). */
    assert(suts_sueps_get_entry(0x1F44Bu, &plain));
    assert(plain.sequence_hash == 0u);
    assert(plain.modifier_base == 0u);

    /* A non-base base fails: 😀 is an emoji but not a modifier base. */
    assert(!suts_sueps_resolve_sequence_ex(0x1F600u, 0x1F3FBu, &e, &err));
    assert(err == SUEPS_ERR_BAD_BASE);

    /* A non-modifier second codepoint fails. */
    assert(!suts_sueps_resolve_sequence_ex(0x1F44Bu, 0x0041u, &e, &err));
    assert(err == SUEPS_ERR_BAD_MODIFIER);

    /* A well-formed but uncurated pair resolves with a zero token. */
    {
        static const uint32_t bases[] = { 0x1F466u, 0x1F467u, 0x1F44Du };
        uint32_t k;
        for (k = 0u; k < 3u; k++) {
            assert(suts_sueps_resolve_sequence(bases[k], 0x1F3FDu, &e));
            assert(e.modifier_base == 0u);
        }
    }

    printf("[PASS] test_sequence_resolution\n");
}

/* ── sequence hash (§3.3) ─────────────────────────────────────────── */

static void test_hash(void)
{
    /* Deterministic and value-sensitive. */
    assert(suts_sueps_hash_codepoint(0x1F600u) ==
           suts_sueps_hash_codepoint(0x1F600u));
    assert(suts_sueps_hash_codepoint(0x1F600u) !=
           suts_sueps_hash_codepoint(0x1F601u));
    assert(suts_sueps_hash_codepoint(0u) != 0u);

    /* The pair hash is order-sensitive: (base, mod) != (mod, base). */
    assert(suts_sueps_hash_sequence(0x1F44Bu, 0x1F3FBu) ==
           suts_sueps_hash_sequence(0x1F44Bu, 0x1F3FBu));
    assert(suts_sueps_hash_sequence(0x1F44Bu, 0x1F3FBu) !=
           suts_sueps_hash_sequence(0x1F3FBu, 0x1F44Bu));
    assert(suts_sueps_hash_sequence(0x1F44Bu, 0x1F3FBu) !=
           suts_sueps_hash_sequence(0x1F44Bu, 0x1F3FCu));

    printf("[PASS] test_hash\n");
}

/* ── zones and neography (§2.2) ───────────────────────────────────── */

static void test_zones_and_neography(void)
{
    uint32_t set = 0ul;

    assert(suts_sueps_zone_of(0x1F600u) == SUEPS_ZONE_BRIDGE);
    assert(suts_sueps_zone_of(0x00110000u) == SUEPS_ZONE_SCP);
    assert(suts_sueps_zone_of(0x00120000u) == SUEPS_ZONE_NATIVE);
    assert(suts_sueps_zone_of(0x00160100u) == SUEPS_ZONE_RNUR);
    assert(suts_sueps_zone_of(0x7FFFFFF0u) == SUEPS_ZONE_TRAP);
    assert(suts_sueps_zone_of(0x7FFFFFFFu) == SUEPS_ZONE_SENTINEL);
    assert(suts_sueps_zone_of(0x80000000u) == SUEPS_ZONE_EXTSUCS);

    assert(suts_sueps_is_renderable(0x1F600u));
    assert(suts_sueps_is_renderable(0x00160100u));
    assert(!suts_sueps_is_renderable(0x00110000u));
    assert(!suts_sueps_is_renderable(0x7FFFFFFFu));
    assert(!suts_sueps_is_renderable(0x80000000u));

    /* RNUR neography emoji: the set decode matches SSP. */
    assert(suts_sueps_is_neography(0x00160100u));
    assert(suts_sueps_neography_set(0x00160100u, &set));
    assert(set == 1u);
    assert(suts_sueps_neography_set(0x00160200u, &set));
    assert(set == 2u);
    assert(!suts_sueps_neography_set(0x1F600u, &set));
    assert(suts_sueps_is_emoji(0x00160100u));
    assert(suts_sueps_is_modifier_base(0x00160200u));

    printf("[PASS] test_zones_and_neography\n");
}

/* ── legality guards on a sequence (§4.4) ─────────────────────────── */

static void test_illegal_zones(void)
{
    sueps_emoji_entry_t e;
    sueps_status_code_t err = SUEPS_OK;

    /* An SCP codepoint is never emoji material. */
    assert(!suts_sueps_resolve_sequence_ex(0x00110000u, 0x1F3FBu, &e, &err));
    assert(err == SUEPS_ERR_ILLEGAL_ZONE);

    /* The Sentinel, a trap slot, and an ExtSUCS value are refused. */
    assert(!suts_sueps_resolve_sequence_ex(0x7FFFFFFFu, 0x1F3FBu, &e, &err));
    assert(err == SUEPS_ERR_ILLEGAL_ZONE);
    assert(!suts_sueps_resolve_sequence_ex(0x7FFFFFF0u, 0x1F3FBu, &e, &err));
    assert(err == SUEPS_ERR_ILLEGAL_ZONE);
    assert(!suts_sueps_resolve_sequence_ex(0x80000000u, 0x1F3FBu, &e, &err));
    assert(err == SUEPS_ERR_ILLEGAL_ZONE);

    /* But a former-surrogate base is admitted as ordinary PUA, not refused:
     * it simply has no emoji record. */
    assert(!suts_sueps_resolve_sequence_ex(0xDC00u, 0x1F3FBu, &e, &err));
    assert(err == SUEPS_ERR_NOT_EMOJI);

    printf("[PASS] test_illegal_zones\n");
}

/* ── enumeration (§3.3) ───────────────────────────────────────────── */

static void test_enumeration(void)
{
    uint32_t n;
    uint32_t i;
    sueps_emoji_entry_t e;

    n = suts_sueps_entry_count();
    assert(n > 0u);
    assert(n <= SUEPS_ENTRY_CAPACITY);
    assert(suts_sueps_entry_at(0u, &e));
    assert(suts_sueps_entry_at(n - 1u, &e));
    assert(!suts_sueps_entry_at(n, &e));
    assert(!suts_sueps_entry_at(n + 5u, &e));

    /* Enumeration is strictly ascending — a verification invariant. */
    for (i = 1u; i < n; i++) {
        sueps_emoji_entry_t a, b;
        assert(suts_sueps_entry_at(i - 1u, &a));
        assert(suts_sueps_entry_at(i, &b));
        assert(a.codepoint < b.codepoint);
    }

    /* Every record round-trips through its own probe and is well-formed. */
    for (i = 0u; i < n; i++) {
        sueps_emoji_entry_t rec, got;
        assert(suts_sueps_entry_at(i, &rec));
        assert(rec.reserved == 0u);
        assert((rec.presentation_flags & ~SUEPS_FLAG_QUALIFIED_MASK) == 0u);
        assert(suts_sueps_is_renderable(rec.codepoint));
        assert(suts_sueps_get_entry(rec.codepoint, &got));
        assert(got.codepoint == rec.codepoint);
        assert(got.presentation_flags == rec.presentation_flags);
    }

    printf("[PASS] test_enumeration\n");
}

/* ── guards and misses ────────────────────────────────────────────── */

static void test_guards(void)
{
    sueps_emoji_entry_t e;
    sueps_status_code_t err = SUEPS_OK;

    assert(!suts_sueps_get_entry(0x1F600u, NULL));
    assert(!suts_sueps_resolve_sequence(0x1F44Bu, 0x1F3FBu, NULL));
    assert(!suts_sueps_resolve_sequence_ex(0x1F44Bu, 0x1F3FBu, NULL, &err));
    assert(err == SUEPS_ERR_NULL_ARGUMENT);

    /* An uncurated, non-renderable codepoint has no record. */
    assert(!suts_sueps_get_entry(0x00110000u, &e));

    /* Names for unknown values degrade safely. */
    assert(suts_sueps_zone_name((sueps_zone_t)99)[0] == 'u');
    assert(suts_sueps_status_code_name((sueps_status_code_t)99)[0] == 'u');

    printf("[PASS] test_guards\n");
}

/* ── SSRE registration (SUTS-007 is a registered optional spec) ───── */

static void test_ssre_registration(void)
{
    assert(SUEPS_SPEC_ID[0] == 'S');
    assert(SUEPS_SPEC_ID[5] == '0' && SUEPS_SPEC_ID[7] == '7');
    printf("[PASS] test_ssre_registration\n");
}

int main(void)
{
    test_version_and_layout();
    test_no_surrogates();
    test_presentation_flags();
    test_components_and_selectors();
    test_sequence_resolution();
    test_hash();
    test_zones_and_neography();
    test_illegal_zones();
    test_enumeration();
    test_guards();
    test_ssre_registration();
    printf("All SUTS-007 (SUEPS) tests passed.\n");
    return 0;
}
