/* Host unit tests for SUTS-005 (SCSS) — System Character Security Specification.
 *
 * These tests exercise the reference implementation in src/suts/suts_scss.c
 * against docs/suts/SUTS-005-scss.md. Codepoints are 64-bit SUCS codepoints,
 * so the same cases cover the Unicode Compatibility Bridge, the RNUR neography
 * plane, the System Control Plane, the trap slots, and the unbounded ExtSUCS
 * space.
 */

#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <stddef.h>

#include "suts/suts_scss.h"

/* ── version ──────────────────────────────────────────────────────── */

static void test_version(void)
{
    assert(scss_version_string() != NULL);
    assert(scss_version_string()[0] != '\0');
    printf("[PASS] test_version\n");
}

/* ── security attribute word and predicates (§2.1, §3) ────────────── */

static void test_flags(void)
{
    /* ASCII identifier class is allowed by default. */
    assert(scss_get_flags(0x0041ULL) == SUTS_SCSS_FLAG_ALLOWED_IDENTIFIER);
    assert(scss_get_flags(0x0061ULL) == SUTS_SCSS_FLAG_ALLOWED_IDENTIFIER);
    assert(scss_get_flags(0x0030ULL) == SUTS_SCSS_FLAG_ALLOWED_IDENTIFIER);
    assert(scss_get_flags(0x005FULL) == SUTS_SCSS_FLAG_ALLOWED_IDENTIFIER); /* _ */
    assert(scss_get_flags(0x002DULL) == SUTS_SCSS_FLAG_ALLOWED_IDENTIFIER); /* - */
    assert(scss_is_allowed_identifier(0x0041ULL));
    assert(scss_is_allowed_identifier(0x0037ULL));
    assert(!scss_is_allowed_identifier(0x0040ULL)); /* '@' is not the identifier class */

    /* Curated allowed ideographs. */
    assert(scss_is_allowed_identifier(0x4E00ULL));
    assert(scss_is_allowed_identifier(0x4E8CULL));

    /* Confusable homographs are never allowed in a secure identifier. */
    assert(scss_get_flags(0x0430ULL) == SUTS_SCSS_FLAG_CONFUSABLE);
    assert(scss_is_confusable(0x0430ULL));
    assert(!scss_is_allowed_identifier(0x0430ULL));
    assert(scss_is_confusable(0x03B1ULL));
    assert(!scss_is_allowed_identifier(0x03B1ULL));

    /* RNUR neography lookalikes are confusable AND require isolation. */
    assert(scss_get_flags(0x160100ULL) ==
           (SUTS_SCSS_FLAG_CONFUSABLE | SUTS_SCSS_FLAG_NEOGRAPHY_MIXED));
    assert(scss_is_neo_mixed(0x160100ULL));
    assert(!scss_is_allowed_identifier(0x160100ULL));

    /* Restricted invisible / control codepoints. */
    assert(scss_get_flags(0x200BULL) == SUTS_SCSS_FLAG_RESTRICTED);
    assert(scss_is_restricted(0x200BULL));
    assert(scss_is_restricted(0x00ADULL));
    assert(!scss_is_allowed_identifier(0x200BULL));

    /* SCP markers, trap slots, and the Sentinel are restricted by default. */
    assert(scss_get_flags(SCSS_ZONE_SCP_MIN) == SUTS_SCSS_FLAG_RESTRICTED);
    assert(scss_is_restricted(SCSS_ZONE_SCP_MAX));
    assert(scss_is_restricted(SCSS_ZONE_TRAP_MIN));
    assert(scss_is_restricted(SCSS_ZONE_SENTINEL));
    assert(!scss_is_allowed_identifier(SCSS_ZONE_SCP_MIN));

    /* Any Native-plane codepoint is isolated by default. */
    assert(scss_is_neo_mixed(0x00120000ULL));
    assert(scss_is_neo_mixed(0x00160005ULL));
    assert(scss_get_flags(0x00160005ULL) == SUTS_SCSS_FLAG_NEOGRAPHY_MIXED);
    assert(!scss_is_neo_mixed(0x0041ULL));

    /* Unclassified codepoints carry no flags. */
    assert(scss_get_flags(0x0040ULL) == 0u);
    assert(scss_get_flags(0x0300ULL) == 0u); /* a combining mark, uncurated */

    printf("[PASS] test_flags\n");
}

/* ── binary skeleton mapping (§2.2) ───────────────────────────────── */

static void test_skeleton(void)
{
    /* Identity when unmapped. */
    assert(scss_resolve_skeleton(0x0041ULL) == 0x0041ULL);
    assert(scss_resolve_skeleton(0x4E00ULL) == 0x4E00ULL);
    assert(scss_resolve_skeleton(0x0061ULL) == 0x0061ULL);

    /* Curated mappings fold every script onto a Latin prototype. */
    assert(scss_resolve_skeleton(0x0430ULL) == 0x0061ULL); /* Cyrillic a */
    assert(scss_resolve_skeleton(0x03B1ULL) == 0x0061ULL); /* Greek a    */
    assert(scss_resolve_skeleton(0x160100ULL) == 0x0061ULL); /* RNUR a   */
    assert(scss_resolve_skeleton(0x043EULL) == 0x006FULL); /* Cyrillic o */
    assert(scss_resolve_skeleton(0x039FULL) == 0x004FULL); /* Greek O    */
    assert(scss_resolve_skeleton(0x0410ULL) == 0x0041ULL); /* Cyrillic A */
    assert(scss_resolve_skeleton(0x160200ULL) == 0x006FULL); /* RNUR o   */

    /* Cross-script homographs share a skeleton. */
    assert(scss_confusable_with(0x0430ULL, 0x03B1ULL));
    assert(scss_confusable_with(0x0430ULL, 0x160100ULL));
    assert(scss_confusable_with(0x0041ULL, 0x0410ULL));
    assert(scss_confusable_with(0x006FULL, 0x043EULL));

    /* Distinct skeletons are not confusable; a codepoint is not its own twin. */
    assert(!scss_confusable_with(0x0430ULL, 0x0440ULL));
    assert(!scss_confusable_with(0x0041ULL, 0x0430ULL)); /* caps vs smalls */
    assert(!scss_confusable_with(0x0061ULL, 0x0062ULL));
    assert(!scss_confusable_with(0x0061ULL, 0x0061ULL));

    printf("[PASS] test_skeleton\n");
}

/* ── registry and enumeration (§4.1) ──────────────────────────────── */

static void test_registry(void)
{
    size_t n, ns, i, j;

    scss_init_registry();
    scss_init_registry(); /* idempotent */

    n = scss_attr_count();
    ns = scss_skeleton_count();
    assert(n >= 30u);
    assert(n <= SUTS_SCSS_ATTR_CAPACITY);
    assert(ns >= 20u);
    assert(ns <= SUTS_SCSS_SKELETON_CAPACITY);

    assert(scss_attr_at(0) != NULL);
    assert(scss_attr_at(n - 1u) != NULL);
    assert(scss_attr_at(n) == NULL);
    assert(scss_attr_at(n + 5u) == NULL);
    assert(scss_skeleton_at(0) != NULL);
    assert(scss_skeleton_at(ns - 1u) != NULL);
    assert(scss_skeleton_at(ns) == NULL);
    assert(scss_skeleton_at(ns + 5u) == NULL);

    /* Every attribute record is SCP-invariant, uniquely keyed, and resolves
     * back to its own stable pointer. */
    for (i = 0; i < n; i++) {
        const scss_security_attr_t* a = scss_attr_at(i);
        assert(a != NULL);
        assert(a->flags != 0u);
        assert(scss_lookup_attr(a->codepoint) == a);
        {
            suts_scss_plane_t p = scss_plane_of(a->codepoint);
            assert(p != SUTS_SCSS_PLANE_SCP);
            assert(p != SUTS_SCSS_PLANE_TRAP);
            assert(p != SUTS_SCSS_PLANE_SENTINEL);
        }
        for (j = i + 1u; j < n; j++) {
            const scss_security_attr_t* b = scss_attr_at(j);
            assert(b != NULL);
            assert(a->codepoint != b->codepoint);
        }
    }

    /* Every skeleton record is confusable, non-trivial, and uniquely keyed. */
    for (i = 0; i < ns; i++) {
        const scss_skeleton_entry_t* s = scss_skeleton_at(i);
        assert(s != NULL);
        assert(s->codepoint != s->skeleton_codepoint);
        assert(scss_is_confusable(s->codepoint));
        assert(scss_lookup_skeleton(s->codepoint) == s);
        assert(scss_resolve_skeleton(s->codepoint) == s->skeleton_codepoint);
        for (j = i + 1u; j < ns; j++) {
            const scss_skeleton_entry_t* t = scss_skeleton_at(j);
            assert(t != NULL);
            assert(s->codepoint != t->codepoint);
        }
    }

    /* An uncurated codepoint has no explicit records. */
    assert(scss_lookup_attr(0x0041ULL) == NULL);
    assert(scss_lookup_skeleton(0x0041ULL) == NULL);

    printf("[PASS] test_registry\n");
}

/* ── script boundary isolation (§2.3) ─────────────────────────────── */

static void test_script(void)
{
    assert(scss_script_of(0x0041ULL) == SUTS_SCSS_SCRIPT_LATIN);
    assert(scss_script_of(0x0410ULL) == SUTS_SCSS_SCRIPT_CYRILLIC);
    assert(scss_script_of(0x03B1ULL) == SUTS_SCSS_SCRIPT_GREEK);
    assert(scss_script_of(0x4E00ULL) == SUTS_SCSS_SCRIPT_HAN);
    assert(scss_script_of(0x20000ULL) == SUTS_SCSS_SCRIPT_HAN);
    assert(scss_script_of(0x0627ULL) == SUTS_SCSS_SCRIPT_ARABIC);
    assert(scss_script_of(0x05D0ULL) == SUTS_SCSS_SCRIPT_HEBREW);
    assert(scss_script_of(0x0030ULL) == SUTS_SCSS_SCRIPT_COMMON);
    assert(scss_script_of(0x002EULL) == SUTS_SCSS_SCRIPT_COMMON);
    assert(scss_script_of(0x00160000ULL) == SUTS_SCSS_SCRIPT_NATIVE);
    assert(scss_script_of(0x00110000ULL) == SUTS_SCSS_SCRIPT_SCP);
    assert(scss_script_of(0x7FFFFFFFULL) == SUTS_SCSS_SCRIPT_SCP);
    assert(scss_script_of(0x80000000ULL) == SUTS_SCSS_SCRIPT_UNKNOWN);

    /* Same script, or a script-neutral partner, composes. */
    assert(scss_script_compatible(0x0041ULL, 0x0062ULL));
    assert(scss_script_compatible(0x4E00ULL, 0x20000ULL));
    assert(scss_script_compatible(0x0041ULL, 0x0030ULL)); /* Latin + digit */
    assert(scss_script_compatible(0x160100ULL, 0x160200ULL));

    /* Different scripts are a boundary violation. */
    assert(!scss_script_compatible(0x0041ULL, 0x0410ULL)); /* Latin + Cyrillic */
    assert(!scss_script_compatible(0x0041ULL, 0x03B1ULL)); /* Latin + Greek    */
    assert(!scss_script_compatible(0x4E00ULL, 0x0041ULL)); /* Han + Latin      */
    assert(scss_is_mixed_script(0x0041ULL, 0x0410ULL));
    assert(scss_is_mixed_script(0x0041ULL, 0x160100ULL)); /* Bridge + Native  */
    assert(!scss_is_mixed_script(0x0041ULL, 0x002EULL));
    assert(!scss_is_mixed_script(0x0430ULL, 0x0435ULL));

    /* The Bridge / Native boundary is the primary injection vector. */
    assert(scss_crosses_bridge_boundary(0x0041ULL, 0x160100ULL));
    assert(scss_crosses_bridge_boundary(0x160100ULL, 0x0041ULL));
    assert(scss_crosses_bridge_boundary(0x4E00ULL, 0x00120000ULL));
    assert(!scss_crosses_bridge_boundary(0x0041ULL, 0x0410ULL));
    assert(!scss_crosses_bridge_boundary(0x160100ULL, 0x160200ULL));

    printf("[PASS] test_script\n");
}

/* ── zone classification (§5) ─────────────────────────────────────── */

static void test_zone(void)
{
    assert(scss_is_bridge_codepoint(0x0041ULL));
    assert(scss_is_bridge_codepoint(SCSS_ZONE_UNICODE_MAX));
    assert(!scss_is_bridge_codepoint(SCSS_ZONE_SCP_MIN));

    assert(scss_is_native_codepoint(SCSS_ZONE_NATIVE_MIN));
    assert(scss_is_native_codepoint(SCSS_RNUR_MIN));
    assert(scss_is_native_codepoint(SCSS_ZONE_NATIVE_LAST));
    assert(!scss_is_native_codepoint(SCSS_ZONE_TRAP_MIN));
    assert(!scss_is_native_codepoint(0x80000000ULL)); /* ExtSUCS, not Native */

    assert(scss_plane_of(0x0041ULL) == SUTS_SCSS_PLANE_BRIDGE);
    assert(scss_plane_of(SCSS_ZONE_UNICODE_MAX) == SUTS_SCSS_PLANE_BRIDGE);
    assert(scss_plane_of(SCSS_ZONE_SCP_MIN) == SUTS_SCSS_PLANE_SCP);
    assert(scss_plane_of(SCSS_ZONE_NATIVE_MIN) == SUTS_SCSS_PLANE_NATIVE);
    assert(scss_plane_of(SCSS_RNUR_MIN) == SUTS_SCSS_PLANE_RNUR);
    assert(scss_plane_of(SCSS_RNUR_MAX) == SUTS_SCSS_PLANE_RNUR);
    assert(scss_plane_of(SCSS_RNUR_MAX + 1ULL) == SUTS_SCSS_PLANE_NATIVE);
    assert(scss_plane_of(SCSS_ZONE_NATIVE_LAST) == SUTS_SCSS_PLANE_NATIVE);
    assert(scss_plane_of(SCSS_ZONE_TRAP_MIN) == SUTS_SCSS_PLANE_TRAP);
    assert(scss_plane_of(SCSS_ZONE_TRAP_MAX) == SUTS_SCSS_PLANE_TRAP);
    assert(scss_plane_of(SCSS_ZONE_SENTINEL) == SUTS_SCSS_PLANE_SENTINEL);
    assert(scss_plane_of(0x80000000ULL) == SUTS_SCSS_PLANE_EXTSUCS);

    printf("[PASS] test_zone\n");
}

/* ── codepoint hash (§4.2) ────────────────────────────────────────── */

static void test_hash(void)
{
    /* Deterministic and value-sensitive. */
    assert(scss_hash(0x0041ULL) == scss_hash(0x0041ULL));
    assert(scss_hash(0x0041ULL) != scss_hash(0x0042ULL));
    assert(scss_hash(0x160100ULL) != scss_hash(0x160101ULL));

    /* The all-zero codepoint still mixes, so its hash is non-zero. */
    assert(scss_hash(0ULL) != 0u);

    printf("[PASS] test_hash\n");
}

int main(void)
{
    test_version();
    test_flags();
    test_skeleton();
    test_registry();
    test_script();
    test_zone();
    test_hash();
    printf("All SUTS-005 (SCSS) tests passed.\n");
    return 0;
}
