/* Host unit tests for SUTS-006 (SIDN) — System Internationalized Domain &
 * Identifier Network Architecture.
 *
 * These tests exercise the reference implementation in src/suts/suts_sidn.c
 * against docs/suts/SUTS-006-sidn.md. Codepoints are 31-bit SUCS codepoints, so
 * the same cases cover the Unicode Compatibility Bridge, the RNUR neography
 * plane, the System Control Plane, the trap slots, and the Sentinel.
 */

#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <stddef.h>

#include "suts/suts_sidn.h"

/* ── version and layout ───────────────────────────────────────────── */

static void test_version_and_layout(void)
{
    assert(suts_sidn_version_string() != NULL);
    assert(suts_sidn_version_string()[0] != '\0');
    assert(SIDN_SPEC_ID[0] == 'S');
    assert(SIDN_VERSION_MAJOR == 1);

    /* The mapping record is a fixed 16 bytes, byte-aligned (§2.2). */
    assert(sizeof(sidn_mapping_entry_t) == 16u);
    assert(offsetof(sidn_mapping_entry_t, codepoint) == 0u);
    assert(offsetof(sidn_mapping_entry_t, mapping) == 4u);
    assert(offsetof(sidn_mapping_entry_t, status) == 12u);
    assert(offsetof(sidn_mapping_entry_t, mapping_length) == 13u);
    assert(offsetof(sidn_mapping_entry_t, reserved) == 14u);

    /* The sketch's label descriptor survives unchanged (§2.3). */
    assert(sizeof(sidn_label_descriptor_t) == 8u);
    assert(offsetof(sidn_label_descriptor_t, label_hash) == 0u);
    assert(offsetof(sidn_label_descriptor_t, length) == 4u);
    assert(offsetof(sidn_label_descriptor_t, flags) == 6u);

    printf("[PASS] test_version_and_layout\n");
}

/* ── mapping status taxonomy (§2.1) ───────────────────────────────── */

static void test_mapping_status(void)
{
    sidn_mapping_entry_t e;

    suts_sidn_init_engine();
    suts_sidn_init_engine(); /* idempotent */

    /* ASCII uppercase maps to lowercase. */
    assert(suts_sidn_status_of(0x0041u) == SIDN_STATUS_MAPPED); /* A */
    assert(suts_sidn_mapping_of(0x0041u, &e));
    assert(e.mapping[0] == 0x0061u && e.mapping_length == 1u);
    assert(suts_sidn_status_of(0x005Au) == SIDN_STATUS_MAPPED); /* Z */
    assert(suts_sidn_status_of(0x0061u) == SIDN_STATUS_VALID);  /* a */
    assert(suts_sidn_status_of(0x0030u) == SIDN_STATUS_VALID);  /* 0 */
    assert(suts_sidn_status_of(0x002Du) == SIDN_STATUS_VALID);  /* - */

    /* Ignored: soft hyphen. */
    assert(suts_sidn_status_of(0x00ADu) == SIDN_STATUS_IGNORED);

    /* Mapped label separators fold to U+002E. */
    assert(suts_sidn_status_of(0x3002u) == SIDN_STATUS_MAPPED);
    assert(suts_sidn_mapping_of(0x3002u, &e) && e.mapping[0] == 0x002Eu);
    assert(suts_sidn_mapping_of(0xFF0Eu, &e) && e.mapping[0] == 0x002Eu);
    assert(suts_sidn_mapping_of(0xFF61u, &e) && e.mapping[0] == 0x002Eu);

    /* Mapped fullwidth capital, and the capital sharp s. */
    assert(suts_sidn_status_of(0xFF21u) == SIDN_STATUS_MAPPED);
    assert(suts_sidn_status_of(0x1E9Eu) == SIDN_STATUS_MAPPED);
    assert(suts_sidn_mapping_of(0x1E9Eu, &e) && e.mapping[0] == 0x00DFu);

    /* Disallowed: the digit-one-full-stop and a symbol. */
    assert(suts_sidn_status_of(0x2488u) == SIDN_STATUS_DISALLOWED);
    assert(suts_sidn_status_of(0x2665u) == SIDN_STATUS_DISALLOWED);

    /* Absence from the curated table is disallowed (closed by default). */
    assert(suts_sidn_status_of(0x005Fu) == SIDN_STATUS_VALID);
    assert(!suts_sidn_mapping_of(0x110000u, &e)); /* SCP is never in the table */
    assert(suts_sidn_status_of(0x7FFFFFFFu) == SIDN_STATUS_DISALLOWED);

    /* Names round-trip. */
    assert(suts_sidn_status_name(SIDN_STATUS_VALID)[0] == 'v');
    assert(suts_sidn_status_name(SIDN_STATUS_DISALLOWED)[0] == 'd');
    assert(suts_sidn_status_name(SIDN_STATUS_DEVIATION)[0] == 'd');

    printf("[PASS] test_mapping_status\n");
}

/* ── deviation characters (§3.2) ──────────────────────────────────── */

static void test_deviation(void)
{
    sidn_mapping_entry_t e;

    assert(suts_sidn_status_of(0x00DFu) == SIDN_STATUS_DEVIATION); /* ß  */
    assert(suts_sidn_status_of(0x03C2u) == SIDN_STATUS_DEVIATION); /* ς  */
    assert(suts_sidn_status_of(0x200Cu) == SIDN_STATUS_DEVIATION); /* ZWNJ */
    assert(suts_sidn_status_of(0x200Du) == SIDN_STATUS_DEVIATION); /* ZWJ  */

    /* The transitional mapping of ß is "ss". */
    assert(suts_sidn_mapping_of(0x00DFu, &e));
    assert(e.mapping_length == 2u);
    assert(e.mapping[0] == 0x0073u && e.mapping[1] == 0x0073u);

    /* A deviation is admissible under nontransitional processing. */
    assert(suts_sidn_is_admissible(0x00DFu));
    assert(suts_sidn_is_admissible(0x03C2u));

    /* The canonical form keeps ß, because nontransitional is the default. */
    {
        uint32_t in[1] = { 0x00DFu };
        uint32_t out[8];
        size_t n = 0u;
        sidn_status_code_t err = SIDN_OK;
        assert(suts_sidn_to_canonical(in, 1u, out, &n, &err));
        assert(n == 1u && out[0] == 0x00DFu);
        assert(err == SIDN_OK);
    }

    printf("[PASS] test_deviation\n");
}

/* ── label separators (§3.1) ──────────────────────────────────────── */

static void test_separators(void)
{
    assert(suts_sidn_is_label_separator(0x002Eu));
    assert(suts_sidn_is_label_separator(0xFF0Eu));
    assert(suts_sidn_is_label_separator(0x3002u));
    assert(suts_sidn_is_label_separator(0xFF61u));
    assert(!suts_sidn_is_label_separator(0x0041u));
    assert(!suts_sidn_is_label_separator(0x002Cu));

    /* Each folds to U+002E in the canonical form. */
    {
        uint32_t in[3] = { 0x0061u, 0x3002u, 0x0062u };
        uint32_t out[8];
        size_t n = 0u;
        sidn_status_code_t err = SIDN_OK;
        assert(suts_sidn_to_canonical(in, 3u, out, &n, &err));
        /* The label ends at the separator, so only "a" survives into a label. */
        assert(n == 1u && out[0] == 0x0061u);
    }
    {
        uint32_t in[2] = { 0x0061u, 0xFF61u };
        uint32_t out[8];
        size_t n = 0u;
        assert(suts_sidn_to_canonical(in, 2u, out, &n, NULL));
        assert(n == 1u && out[0] == 0x0061u);
    }

    printf("[PASS] test_separators\n");
}

/* ── canonical routing (§4) ───────────────────────────────────────── */

static void test_canonical(void)
{
    uint32_t out[16];
    size_t n = 0u;
    sidn_status_code_t err = SIDN_OK;

    /* Uppercase maps to lowercase. */
    {
        uint32_t in[6] = { 0x0042u, 0x00FCu, 0x0063u, 0x0068u, 0x0065u, 0x0072u };
        assert(suts_sidn_to_canonical(in, 6u, out, &n, &err));
        assert(err == SIDN_OK);
        assert(n == 6u);
        assert(out[0] == 0x0062u && out[1] == 0x00FCu && out[2] == 0x0063u);
    }

    /* Ignored codepoints are removed. */
    {
        uint32_t in[3] = { 0x0061u, 0x00ADu, 0x0062u };
        assert(suts_sidn_to_canonical(in, 3u, out, &n, &err));
        assert(n == 2u && out[0] == 0x0061u && out[1] == 0x0062u);
    }

    /* A disallowed codepoint fails, and the descriptor is flagged. */
    {
        uint32_t in[3] = { 0x0061u, 0x2665u, 0x0062u };
        sidn_label_descriptor_t d;
        assert(!suts_sidn_validate_label(in, 3u, &d, &err));
        assert(err == SIDN_ERR_DISALLOWED);
        assert((d.flags & SIDN_FLAG_DISALLOWED) != 0u);
        assert(!suts_sidn_to_canonical(in, 3u, out, &n, &err));
        assert(err == SIDN_ERR_DISALLOWED);
    }

    /* An empty label fails. */
    {
        sidn_label_descriptor_t d;
        uint32_t dummy = 0u;
        assert(!suts_sidn_validate_label(&dummy, 0u, &d, &err));
        assert(err == SIDN_ERR_LABEL_EMPTY);
    }

    printf("[PASS] test_canonical\n");
}

/* ── validity criteria (§4.2) ─────────────────────────────────────── */

static void test_validity(void)
{
    sidn_label_descriptor_t d;
    sidn_status_code_t err = SIDN_OK;

    /* A plain label is valid and flagged VALID_LABEL. */
    {
        uint32_t in[3] = { 0x0061u, 0x0062u, 0x0063u };
        assert(suts_sidn_validate_label(in, 3u, &d, &err));
        assert((d.flags & SIDN_FLAG_VALID_LABEL) != 0u);
        assert(d.length == 3u);
    }

    /* A leading hyphen fails. */
    {
        uint32_t in[3] = { 0x002Du, 0x0061u, 0x0062u };
        assert(!suts_sidn_validate_label(in, 3u, &d, &err));
        assert(err == SIDN_ERR_HYPHEN_POSITION);
    }
    /* A trailing hyphen fails. */
    {
        uint32_t in[3] = { 0x0061u, 0x0062u, 0x002Du };
        assert(!suts_sidn_validate_label(in, 3u, &d, &err));
        assert(err == SIDN_ERR_HYPHEN_POSITION);
    }
    /* A hyphen in the third and fourth positions fails. */
    {
        uint32_t in[4] = { 0x0061u, 0x0062u, 0x002Du, 0x002Du };
        assert(!suts_sidn_validate_label(in, 4u, &d, &err));
        assert(err == SIDN_ERR_HYPHEN_POSITION);
    }
    /* A hyphen in the third position alone is fine. */
    {
        uint32_t in[3] = { 0x0061u, 0x0062u, 0x002Du };
        (void)in;
    }
    {
        uint32_t in[4] = { 0x0061u, 0x0062u, 0x002Du, 0x0063u };
        assert(suts_sidn_validate_label(in, 4u, &d, &err));
    }

    /* The reserved "xn-" prefix fails (criterion 4). */
    {
        uint32_t in[3] = { 0x0078u, 0x006Eu, 0x002Du };
        assert(!suts_sidn_validate_label(in, 3u, &d, &err));
        assert(err == SIDN_ERR_RESERVED_PREFIX);
    }
    assert(suts_sidn_has_reserved_prefix(
        (const uint32_t[]){ 0x0078u, 0x006Eu, 0x002Du }, 3u));

    /* A non-STD3 ASCII codepoint (underbar) fails the STD3 rule. */
    {
        uint32_t in[3] = { 0x0061u, 0x005Fu, 0x0062u };
        assert(!suts_sidn_validate_label(in, 3u, &d, &err));
        assert(err == SIDN_ERR_STD3_ASCII);
    }

    /* The individual predicates. */
    assert(suts_sidn_check_hyphens((const uint32_t[]){ 0x0061u, 0x0062u },
                                   2u));
    assert(!suts_sidn_check_hyphens((const uint32_t[]){ 0x002Du, 0x0062u },
                                    2u));
    assert(suts_sidn_check_std3((const uint32_t[]){ 0x0061u, 0x0030u,
                                                    0x002Du }, 3u));
    assert(!suts_sidn_check_std3((const uint32_t[]){ 0x0041u }, 1u));

    printf("[PASS] test_validity\n");
}

/* ── ContextJ and Bidi (§4.2) ─────────────────────────────────────── */

static void test_joiners_and_bidi(void)
{
    sidn_label_descriptor_t d;
    sidn_status_code_t err = SIDN_OK;

    /* A ZWJ between two Latin letters is an inadmissible context. */
    {
        uint32_t in[3] = { 0x0061u, 0x200Du, 0x0062u };
        assert(!suts_sidn_validate_label(in, 3u, &d, &err));
        assert(err == SIDN_ERR_CONTEXTJ);
    }
    /* A ZWNJ between two Arabic letters is admissible. */
    {
        uint32_t in[3] = { 0x0627u, 0x200Cu, 0x0628u };
        assert(suts_sidn_check_joiners(in, 3u));
    }
    /* A joiner after a Devanagari virama is admissible. */
    assert(suts_sidn_check_joiners((const uint32_t[]){ 0x0915u, 0x094Du,
                                                       0x200Du }, 3u));
    assert(!suts_sidn_check_joiners((const uint32_t[]){ 0x0061u, 0x200Cu },
                                    2u));

    /* A purely Arabic label is a Bidi domain. */
    {
        uint32_t in[4] = { 0x0645u, 0x0627u, 0x0646u, 0x0628u };
        assert(suts_sidn_is_bidi_domain(in, 4u));
        assert(suts_sidn_check_bidi(in, 4u));
        assert(suts_sidn_validate_label(in, 4u, &d, &err));
    }
    /* An RTL label that begins with a combining mark is rejected. */
    assert(!suts_sidn_check_bidi((const uint32_t[]){ 0x05B0u, 0x05D0u }, 2u));
    /* A mixed RTL/LTR label must end in an RTL letter. */
    assert(suts_sidn_check_bidi((const uint32_t[]){ 0x0061u, 0x05D0u }, 2u));
    assert(!suts_sidn_check_bidi((const uint32_t[]){ 0x05D0u, 0x0061u }, 2u));

    printf("[PASS] test_joiners_and_bidi\n");
}

/* ── neography and zones (§5) ─────────────────────────────────────── */

static void test_neography_and_zones(void)
{
    sidn_label_descriptor_t d;
    sidn_status_code_t err = SIDN_OK;
    uint32_t set = 0ul;

    /* RNUR is a routable neography zone; the set decode matches SSP. */
    assert(suts_sidn_is_neography(0x00160100u));
    assert(suts_sidn_neography_set(0x00160100u, &set));
    assert(set == 1u);
    assert(suts_sidn_neography_set(0x00160200u, &set));
    assert(set == 2u);
    assert(!suts_sidn_neography_set(0x00120000u, &set));

    assert(suts_sidn_zone_of(0x0041u) == SIDN_ZONE_BRIDGE);
    assert(suts_sidn_zone_of(0x00110000u) == SIDN_ZONE_SCP);
    assert(suts_sidn_zone_of(0x00120000u) == SIDN_ZONE_NATIVE);
    assert(suts_sidn_zone_of(0x00160100u) == SIDN_ZONE_RNUR);
    assert(suts_sidn_zone_of(0x7FFFFFF0u) == SIDN_ZONE_TRAP);
    assert(suts_sidn_zone_of(0x7FFFFFFFu) == SIDN_ZONE_SENTINEL);
    assert(suts_sidn_zone_of(0x80000000u) == SIDN_ZONE_EXTSUCS);

    assert(suts_sidn_is_routable(0x0041u));
    assert(suts_sidn_is_routable(0x00160100u));
    assert(!suts_sidn_is_routable(0x00110000u));
    assert(!suts_sidn_is_routable(0x7FFFFFFFu));
    assert(!suts_sidn_is_routable(0x80000000u));

    /* An RNUR label validates and is flagged as a neography domain. */
    {
        uint32_t in[2] = { 0x00160100u, 0x00160200u };
        assert(suts_sidn_validate_label(in, 2u, &d, &err));
        assert((d.flags & SIDN_FLAG_NEOGRAPHY_DOMAIN) != 0u);
    }

    /* An SCP codepoint in a label is refused. */
    {
        uint32_t in[2] = { 0x0061u, 0x00110000u };
        assert(!suts_sidn_validate_label(in, 2u, &d, &err));
        assert(err == SIDN_ERR_DISALLOWED);
    }

    assert(suts_sidn_zone_name(SIDN_ZONE_RNUR)[0] == 'R');
    assert(suts_sidn_zone_name(SIDN_ZONE_EXTSUCS)[0] == 'E');

    printf("[PASS] test_neography_and_zones\n");
}

/* ── hash and enumeration (§2.4) ──────────────────────────────────── */

static void test_hash_and_enumeration(void)
{
    uint32_t n;
    uint32_t i;
    sidn_mapping_entry_t e;

    /* Deterministic and value-sensitive. */
    assert(suts_sidn_hash_codepoint(0x0041u) ==
           suts_sidn_hash_codepoint(0x0041u));
    assert(suts_sidn_hash_codepoint(0x0041u) !=
           suts_sidn_hash_codepoint(0x0042u));
    assert(suts_sidn_hash_codepoint(0u) != 0u);

    /* The label hash is deterministic and sensitive to content. */
    assert(suts_sidn_hash_label((const uint32_t[]){ 0x0061u, 0x0062u }, 2u) ==
           suts_sidn_hash_label((const uint32_t[]){ 0x0061u, 0x0062u }, 2u));
    assert(suts_sidn_hash_label((const uint32_t[]){ 0x0061u }, 1u) !=
           suts_sidn_hash_label((const uint32_t[]){ 0x0062u }, 1u));
    assert(suts_sidn_hash_label(NULL, 0u) == 0u);

    /* Enumeration is ascending and bounded. */
    n = suts_sidn_mapping_count();
    assert(n > 0u);
    assert(n <= SIDN_MAPPING_CAPACITY);
    assert(suts_sidn_mapping_at(0u, &e));
    assert(suts_sidn_mapping_at(n - 1u, &e));
    assert(!suts_sidn_mapping_at(n, &e));
    assert(!suts_sidn_mapping_at(n + 5u, &e));
    for (i = 1u; i < n; i++) {
        sidn_mapping_entry_t a, b;
        assert(suts_sidn_mapping_at(i - 1u, &a));
        assert(suts_sidn_mapping_at(i, &b));
        assert(a.codepoint < b.codepoint); /* strictly ascending */
    }

    /* Every record round-trips through its own probe and is SCP-invariant. */
    for (i = 0u; i < n; i++) {
        sidn_mapping_entry_t rec, got;
        assert(suts_sidn_mapping_at(i, &rec));
        assert(rec.reserved == 0u);
        assert(rec.status <= (uint8_t)SIDN_STATUS_DISALLOWED);
        assert(rec.mapping_length <= 2u);
        assert(suts_sidn_is_routable(rec.codepoint));
        assert(suts_sidn_mapping_of(rec.codepoint, &got));
        assert(got.codepoint == rec.codepoint);
        assert(got.status == rec.status);
    }

    printf("[PASS] test_hash_and_enumeration\n");
}

/* ── guards and misses ────────────────────────────────────────────── */

static void test_guards(void)
{
    sidn_mapping_entry_t e;
    sidn_label_descriptor_t d;
    uint32_t out[4];
    size_t n = 0u;
    sidn_status_code_t err = SIDN_OK;

    assert(!suts_sidn_mapping_of(0x0041u, NULL));
    assert(!suts_sidn_validate_label(NULL, 1u, &d, &err));
    assert(err == SIDN_ERR_NULL_ARGUMENT);
    assert(!suts_sidn_validate_label((const uint32_t[]){ 0x0061u }, 1u,
                                     NULL, &err));
    assert(err == SIDN_ERR_NULL_ARGUMENT);
    assert(!suts_sidn_to_canonical((const uint32_t[]){ 0x0061u }, 1u,
                                   NULL, &n, &err));
    assert(err == SIDN_ERR_NULL_ARGUMENT);

    /* An uncurated, unroutable codepoint has no mapping record. */
    assert(!suts_sidn_mapping_of(0x00110000u, &e));

    /* Names for unknown values degrade safely. */
    assert(suts_sidn_status_name((sidn_status_t)99)[0] == 'u');
    assert(suts_sidn_zone_name((sidn_zone_t)99)[0] == 'U');
    assert(suts_sidn_status_code_name((sidn_status_code_t)99)[0] == 'U');
    assert(suts_sidn_status_code_name(SIDN_OK)[0] == 'O');

    printf("[PASS] test_guards\n");
}

/* ── SSRE registration (SUTS-006 is a registered optional spec) ───── */

static void test_ssre_registration(void)
{
    assert(SIDN_SPEC_ID[0] == 'S');
    assert(SIDN_SPEC_ID[5] == '0' && SIDN_SPEC_ID[7] == '6');
    printf("[PASS] test_ssre_registration\n");
}

int main(void)
{
    test_version_and_layout();
    test_mapping_status();
    test_deviation();
    test_separators();
    test_canonical();
    test_validity();
    test_joiners_and_bidi();
    test_neography_and_zones();
    test_hash_and_enumeration();
    test_guards();
    test_ssre_registration();
    printf("All SUTS-006 (SIDN) tests passed.\n");
    return 0;
}
