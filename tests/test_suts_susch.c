/* Host unit tests for SUTS-008 (SUSCH) — SuperUnicode Source Code Handling.
 *
 * These tests exercise the reference implementation in src/suts/suts_susch.c
 * against docs/suts/SUTS-008-susch.md. Codepoints are 31-bit SUCS codepoints,
 * so the same cases cover the Unicode Compatibility Bridge, the RNUR neography
 * plane, the System Control Plane, the trap slots, and the Sentinel.
 *
 * The security properties under test are the ones a source-code attacker
 * reaches for:
 *
 *   - bidi override vectors (RLO/LRO/PDF and the isolate controls),
 *   - invisible and zero-width character injection,
 *   - homoglyph spoofing (Cyrillic "a" for Latin "a", and friends),
 *   - identifier admissibility (start vs. continue),
 *   - identifier chunk boundaries (UTS #55 §5.1.2.1),
 *   - mixed-script chunks, and
 *   - the neography/Bridge zone boundary.
 */

#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <stddef.h>

#include "suts/suts_susch.h"

/* ── version and layout ───────────────────────────────────────────── */

static void test_version_and_layout(void)
{
    assert(suts_susch_version_string() != NULL);
    assert(suts_susch_version_string()[0] == '1');
    assert(suts_susch_version_string()[1] == '.');
    assert(suts_susch_version_string()[2] == '0');
    assert(SUSCH_SPEC_ID[0] == 'S');
    assert(SUSCH_SPEC_ID[5] == '0' && SUSCH_SPEC_ID[7] == '8');
    assert(SUSCH_SPEC_TITLE[0] == 'S');
    assert(SUSCH_VERSION_MAJOR == 1);
    assert(SUSCH_VERSION_MINOR == 0);
    assert(SUSCH_MODULE[0] == 's');

    /* The source attribute is exactly 12 bytes, byte-aligned (§3.4). */
    assert(sizeof(susch_source_attr_t) == 12u);
    assert(offsetof(susch_source_attr_t, codepoint) == 0u);
    assert(offsetof(susch_source_attr_t, security_mask) == 4u);
    assert(offsetof(susch_source_attr_t, script_category) == 8u);
    assert(offsetof(susch_source_attr_t, identifier_status) == 10u);
    assert(offsetof(susch_source_attr_t, reserved) == 11u);

    /* The four sketch-named flags keep their bit positions. */
    assert(SUSCH_FLAG_VALID_IDENTIFIER == (1u << 0));
    assert(SUSCH_FLAG_BIDI_CONTROL == (1u << 1));
    assert(SUSCH_FLAG_INVISIBLE == (1u << 2));
    assert(SUSCH_FLAG_HOMOGLYPH_RISK == (1u << 3));

    /* The engine has no state to build; the entry point is idempotent. */
    suts_susch_init_engine();
    suts_susch_init_engine();

    printf("[PASS] test_version_and_layout\n");
}

/* ── the compiled attribute (§3.4) ─────────────────────────────────── */

static void test_attribute_query(void)
{
    susch_source_attr_t attr;

    /* A plain Latin letter is admissible, Latin, and clean. */
    assert(suts_susch_get_attributes(0x00000061u, &attr));
    assert(attr.codepoint == 0x00000061u);
    assert(attr.security_mask == SUSCH_FLAG_VALID_IDENTIFIER);
    assert(attr.script_category == (uint16_t)SUSCH_SCRIPT_LATIN);
    assert(attr.identifier_status == (uint8_t)SUSCH_IDENT_ALLOWED);
    assert(attr.reserved == 0u);

    /* A Cyrillic "a" is admissible but flagged: the homoglyph risk is a
     * property of the compiled skeleton table, not of the script alone. */
    assert(suts_susch_get_attributes(0x00000430u, &attr));
    assert((attr.security_mask & SUSCH_FLAG_HOMOGLYPH_RISK) != 0u);
    assert((attr.security_mask & SUSCH_FLAG_VALID_IDENTIFIER) != 0u);
    assert(attr.script_category == (uint16_t)SUSCH_SCRIPT_CYRILLIC);
    assert(attr.identifier_status == (uint8_t)SUSCH_IDENT_RESTRICTED);

    /* A directional override is a bidi control and is invisible. */
    assert(suts_susch_get_attributes(0x0000202Eu, &attr));
    assert((attr.security_mask & SUSCH_FLAG_BIDI_CONTROL) != 0u);
    assert((attr.security_mask & SUSCH_FLAG_INVISIBLE) != 0u);
    assert((attr.security_mask & SUSCH_FLAG_VALID_IDENTIFIER) == 0u);
    assert(attr.identifier_status == (uint8_t)SUSCH_IDENT_DISALLOWED);

    /* A neography letter lives in the Native plane. */
    assert(suts_susch_get_attributes(0x00160000u, &attr));
    assert((attr.security_mask & SUSCH_FLAG_NATIVE_PLANE) != 0u);
    assert((attr.security_mask & SUSCH_FLAG_VALID_IDENTIFIER) != 0u);
    assert(attr.script_category == (uint16_t)SUSCH_SCRIPT_NEOGRAPHY);
    assert(attr.identifier_status == (uint8_t)SUSCH_IDENT_RESTRICTED);

    /* The query is total over the address space: the Sentinel still has an
     * answer, and an answer is never a lookup failure. */
    assert(suts_susch_get_attributes(0x7FFFFFFFu, &attr));
    assert(attr.identifier_status == (uint8_t)SUSCH_IDENT_DISALLOWED);
    assert(attr.reserved == 0u);
    assert(suts_susch_get_attributes(0x00110000u, &attr));
    assert(suts_susch_get_attributes(0x00120000u, &attr));
    assert(suts_susch_get_attributes(0x00000000u, &attr));

    /* A NULL output pointer and a codepoint above the Sentinel are the two
     * ways the query refuses. */
    assert(!suts_susch_get_attributes(0x00000061u, NULL));
    assert(!suts_susch_get_attributes(0x80000000u, &attr));

    printf("[PASS] test_attribute_query\n");
}

/* ── bidi override vectors (§3.6) ─────────────────────────────────── */

static void test_bidi_controls(void)
{
    static const uint32_t bidi[] = {
        0x0000202Au, /* LRE  left-to-right embedding          */
        0x0000202Bu, /* RLE  right-to-left embedding         */
        0x0000202Cu, /* PDF  pop directional formatting       */
        0x0000202Du, /* LRO  left-to-right override          */
        0x0000202Eu, /* RLO  right-to-left override          */
        0x0000200Eu, /* LRM  left-to-right mark              */
        0x0000200Fu, /* RLM  right-to-left mark              */
        0x0000061Cu, /* ALM  Arabic letter mark              */
        0x00002066u, /* LRI  left-to-right isolate           */
        0x00002067u, /* RLI  right-to-left isolate           */
        0x00002068u, /* FSI  first strong isolate            */
        0x00002069u  /* PDI  pop directional isolate         */
    };
    size_t i;

    for (i = 0u; i < sizeof bidi / sizeof bidi[0]; i++) {
        susch_source_attr_t attr;
        assert(suts_susch_is_bidi_control(bidi[i]));
        assert(suts_susch_get_attributes(bidi[i], &attr));
        assert((attr.security_mask & SUSCH_FLAG_BIDI_CONTROL) != 0u);
        assert((attr.security_mask & SUSCH_FLAG_VALID_IDENTIFIER) == 0u);
        assert(attr.identifier_status == (uint8_t)SUSCH_IDENT_DISALLOWED);
    }

    /* Ordinary text is not a bidi control. */
    assert(!suts_susch_is_bidi_control(0x00000061u));
    assert(!suts_susch_is_bidi_control(0x000000A0u));
    assert(!suts_susch_is_bidi_control(0x0000200Bu)); /* ZWSP is not bidi */

    /* The override vector is rejected outright by the validator. */
    {
        uint32_t stream[3];
        susch_token_status_t status = SUSCH_TOKEN_OK;
        size_t index = 0u;
        stream[0] = 0x00000061u; /* a */
        stream[1] = 0x0000202Eu; /* RLO */
        stream[2] = 0x00000062u; /* b */
        assert(!suts_susch_validate_token_stream_ex(stream, 3u, &status,
                                                     NULL, &index));
        assert(status == SUSCH_TOKEN_ERR_BIDI_CONTROL);
        assert(index == 1u);
    }

    printf("[PASS] test_bidi_controls\n");
}

/* ── invisible character injection (§3.6) ──────────────────────────── */

static void test_invisibles(void)
{
    static const uint32_t invisible[] = {
        0x000000ADu, /* SOFT HYPHEN                          */
        0x0000034Fu, /* COMBINING GRAPHEME JOINER            */
        0x0000115Fu, /* HANGUL CHOSEONG FILLER               */
        0x0000200Bu, /* ZERO WIDTH SPACE                     */
        0x00002060u, /* WORD JOINER                          */
        0x0000206Au, /* INVISIBLE TIMES                      */
        0x0000FEFFu, /* ZERO WIDTH NO-BREAK SPACE            */
        0x0000FFA0u, /* HALFWIDTH HANGUL FILLER              */
        0x0000FFF9u, /* INTERLINEAR ANNOTATION SEPARATOR     */
        0x0001BCA0u, /* SHORTHAND FORMAT LETTERS             */
        0x000E0001u  /* LANGUAGE TAG                         */
    };
    size_t i;

    for (i = 0u; i < sizeof invisible / sizeof invisible[0]; i++) {
        susch_source_attr_t attr;
        assert(suts_susch_is_invisible(invisible[i]));
        assert(suts_susch_get_attributes(invisible[i], &attr));
        assert((attr.security_mask & SUSCH_FLAG_INVISIBLE) != 0u);
        assert((attr.security_mask & SUSCH_FLAG_VALID_IDENTIFIER) == 0u);
        assert(attr.identifier_status == (uint8_t)SUSCH_IDENT_DISALLOWED);
    }

    /* The two joining controls are invisible but admissible under contextual
     * restriction, so they carry the joining-control flag and are
     * RESTRICTED rather than DISALLOWED. */
    {
        susch_source_attr_t attr;
        assert(suts_susch_is_invisible(0x0000200Cu));
        assert(suts_susch_get_attributes(0x0000200Cu, &attr));
        assert((attr.security_mask & SUSCH_FLAG_JOINING_CONTROL) != 0u);
        assert((attr.security_mask & SUSCH_FLAG_VALID_IDENTIFIER) != 0u);
        assert(attr.identifier_status == (uint8_t)SUSCH_IDENT_RESTRICTED);

        assert(suts_susch_is_invisible(0x0000200Du));
        assert(suts_susch_get_attributes(0x0000200Du, &attr));
        assert((attr.security_mask & SUSCH_FLAG_JOINING_CONTROL) != 0u);
        assert(attr.identifier_status == (uint8_t)SUSCH_IDENT_RESTRICTED);
    }

    /* Ordinary visible text is not invisible. */
    assert(!suts_susch_is_invisible(0x00000061u));
    assert(!suts_susch_is_invisible(0x00000020u));

    /* An invisible character inside a token is a defect. */
    {
        uint32_t stream[3];
        susch_token_status_t status = SUSCH_TOKEN_OK;
        size_t index = 0u;
        stream[0] = 0x00000061u;
        stream[1] = 0x0000200Bu; /* ZWSP */
        stream[2] = 0x00000062u;
        assert(!suts_susch_validate_token_stream_ex(stream, 3u, &status,
                                                     NULL, &index));
        assert(status == SUSCH_TOKEN_ERR_INVISIBLE);
        assert(index == 1u);
    }

    /* A joining control is admissible mid-token, and is reported through the
     * advisory channel instead of the defect channel. */
    {
        uint32_t stream[2];
        susch_token_status_t status = SUSCH_TOKEN_OK;
        uint32_t warnings = 0u;
        stream[0] = 0x00000061u;
        stream[1] = 0x0000200Cu; /* ZWNJ */
        assert(suts_susch_validate_token_stream_ex(stream, 2u, &status,
                                                    &warnings, NULL));
        assert(status == SUSCH_TOKEN_OK);
        assert((warnings & SUSCH_WARN_RESTRICTED) != 0u);
    }

    printf("[PASS] test_invisibles\n");
}

/* ── homoglyph spoofing (§3.5) ─────────────────────────────────────── */

static void test_homoglyphs(void)
{
    /* The skeleton folds a confusable onto the prototype a reader sees. */
    assert(suts_susch_skeleton_of(0x00000061u) == 0x00000061u);
    assert(suts_susch_skeleton_of(0x00000430u) == 0x00000061u); /* Cyrillic a */
    assert(suts_susch_skeleton_of(0x000003B1u) == 0x00000061u); /* Greek alpha */
    assert(suts_susch_skeleton_of(0x00000435u) == 0x00000065u); /* Cyrillic ie */
    assert(suts_susch_skeleton_of(0x0000FF21u) == 0x00000041u); /* fullwidth A */

    /* Two codepoints that share a prototype are visual homographs. */
    assert(suts_susch_confusable_with(0x00000061u, 0x00000430u));
    assert(suts_susch_confusable_with(0x00000061u, 0x000003B1u));
    assert(suts_susch_confusable_with(0x00000430u, 0x000003B1u));
    assert(!suts_susch_confusable_with(0x00000061u, 0x00000062u));
    assert(!suts_susch_confusable_with(0x00000061u, 0x00000061u));

    /* The risk predicate covers both the table and the curated risky spans. */
    assert(suts_susch_is_homoglyph_risk(0x00000430u));
    assert(suts_susch_is_homoglyph_risk(0x000003B1u));
    assert(suts_susch_is_homoglyph_risk(0x0000FF21u));
    assert(suts_susch_is_homoglyph_risk(0x0001D400u));
    assert(!suts_susch_is_homoglyph_risk(0x00000061u));
    assert(!suts_susch_is_homoglyph_risk(0x00000042u));

    printf("[PASS] test_homoglyphs\n");
}

/* ── identifier admissibility (§3.2) ───────────────────────────────── */

static void test_identifier_status(void)
{
    /* A letter is allowed, may start, and may continue. */
    assert(suts_susch_identifier_status(0x00000061u) == SUSCH_IDENT_ALLOWED);
    assert(suts_susch_is_identifier_start(0x00000061u));
    assert(suts_susch_is_identifier_continue(0x00000061u));

    /* The underscore is a letter for identifier purposes. */
    assert(suts_susch_is_identifier_start(0x0000005Fu));
    assert(suts_susch_is_identifier_continue(0x0000005Fu));

    /* A digit may continue but never start. */
    assert(suts_susch_is_identifier_continue(0x00000030u));
    assert(!suts_susch_is_identifier_start(0x00000030u));

    /* A confusable is admissible but restricted. */
    assert(suts_susch_identifier_status(0x00000430u) == SUSCH_IDENT_RESTRICTED);
    assert(suts_susch_is_identifier_start(0x00000430u));

    /* A directional control and an invisible are never identifier material. */
    assert(suts_susch_identifier_status(0x0000202Eu) == SUSCH_IDENT_DISALLOWED);
    assert(!suts_susch_is_identifier_start(0x0000202Eu));
    assert(!suts_susch_is_identifier_continue(0x0000202Eu));
    assert(suts_susch_identifier_status(0x0000200Bu) == SUSCH_IDENT_DISALLOWED);
    assert(!suts_susch_is_identifier_continue(0x0000200Bu));

    /* Punctuation and whitespace are not identifier material. */
    assert(suts_susch_identifier_status(0x0000002Du) == SUSCH_IDENT_DISALLOWED);
    assert(suts_susch_identifier_status(0x00000020u) == SUSCH_IDENT_DISALLOWED);
    assert(!suts_susch_is_identifier_start(0x00000020u));

    /* A neography letter is admissible but restricted: its identifier
     * profile is declared by the project, not by the shipped table. */
    assert(suts_susch_identifier_status(0x00160000u) == SUSCH_IDENT_RESTRICTED);
    assert(suts_susch_is_identifier_start(0x00160000u));

    /* A neography symbol position is not an identifier character. */
    assert(suts_susch_identifier_status(0x00160080u) == SUSCH_IDENT_DISALLOWED);
    assert(!suts_susch_is_identifier_start(0x00160080u));

    /* The Native plane outside the RNUR is a private allocation space and
     * never carries source text. */
    assert(suts_susch_identifier_status(0x00130000u) == SUSCH_IDENT_DISALLOWED);

    printf("[PASS] test_identifier_status\n");
}

/* ── identifier chunk boundaries (§4.2) ────────────────────────────── */

static size_t chunks_of(const uint32_t *stream, size_t length)
{
    size_t count = 0u;
    assert(suts_susch_chunk_count(stream, length, &count));
    return count;
}

static void test_chunk_boundaries(void)
{
    /* A camel-case name is two chunks: HTTP | Request. */
    {
        static const uint32_t s[] = { 'H', 'T', 'T', 'P', 'R', 'e', 'q',
                                      'u', 'e', 's', 't' };
        assert(chunks_of(s, 11u) == 2u);
    }

    /* A trailing acronym is a chunk of its own: Type | II. */
    {
        static const uint32_t s[] = { 'T', 'y', 'p', 'e', 'I', 'I' };
        assert(chunks_of(s, 6u) == 2u);
    }

    /* The hat boundary sits before the capital of a capitalized word. */
    {
        static const uint32_t s[] = { 'O', 'C', 'a', 'm', 'l' };
        assert(chunks_of(s, 5u) == 2u);
    }

    /* A one-letter camel transition is still a boundary. */
    {
        static const uint32_t s[] = { 'a', 'B' };
        assert(chunks_of(s, 2u) == 2u);
    }

    /* A plain word is one chunk. */
    {
        static const uint32_t s[] = { 'a', 'b', 'c' };
        assert(chunks_of(s, 3u) == 1u);
    }

    /* Snake case: the connector punctuation bounds a chunk on both sides. */
    {
        static const uint32_t s[] = { 'm', 'y', '_', 'v', 'a', 'r' };
        assert(chunks_of(s, 6u) == 3u);
    }

    /* An acronym embedded in a word opens a chunk of its own. */
    {
        static const uint32_t s[] = { 's', 'n', 'a', 'k', 'e', 'E', 'L',
                                      'E', 'P', 'H', 'A', 'N', 'T', 'S',
                                      'n', 'a', 'k', 'e' };
        assert(chunks_of(s, 18u) == 3u);
    }

    /* A combining mark belongs to the letter it follows: it never opens a
     * chunk of its own. */
    {
        static const uint32_t s[] = { 0x00000041u, 0x00000301u };
        assert(chunks_of(s, 2u) == 1u);
    }

    /* A zero-length stream has zero chunks. */
    {
        size_t count = 99u;
        assert(suts_susch_chunk_count(NULL, 0u, &count));
        assert(count == 0u);
        assert(!suts_susch_chunk_count(NULL, 0u, NULL));
    }

    printf("[PASS] test_chunk_boundaries\n");
}

/* ── mixed-script chunks (§4.2) ────────────────────────────────────── */

static void test_mixed_script(void)
{
    /* Latin "a" followed by Cyrillic "a" in one chunk: mixed script. */
    {
        static const uint32_t s[] = { 0x00000061u, 0x00000430u };
        uint32_t warnings = 0u;
        assert(suts_susch_is_mixed_script(s, 2u));
        assert(suts_susch_validate_token_stream_ex(s, 2u, NULL, &warnings,
                                                     NULL));
        assert((warnings & SUSCH_WARN_MIXED_SCRIPT) != 0u);
        assert((warnings & SUSCH_WARN_HOMOGLYPH) != 0u);
        assert((warnings & SUSCH_WARN_RESTRICTED) != 0u);
    }

    /* Latin "a" followed by a Han character in one chunk: mixed script. */
    {
        static const uint32_t s[] = { 0x00000061u, 0x00004E00u };
        assert(suts_susch_is_mixed_script(s, 2u));
    }

    /* A single-script word is not mixed. */
    {
        static const uint32_t s[] = { 'a', 'b', 'c' };
        assert(!suts_susch_is_mixed_script(s, 3u));
    }

    /* A camel boundary separates the two scripts, so the confusable pair
     * lands in two single-script chunks. The homoglyph warning still
     * reports it — that is the point of the two independent mechanisms. */
    {
        static const uint32_t s[] = { 0x000003BCu, 'B' }; /* Greek mu, Latin B */
        uint32_t warnings = 0u;
        assert(!suts_susch_is_mixed_script(s, 2u));
        assert(suts_susch_validate_token_stream_ex(s, 2u, NULL, &warnings,
                                                     NULL));
        assert((warnings & SUSCH_WARN_MIXED_SCRIPT) == 0u);
        assert((warnings & SUSCH_WARN_HOMOGLYPH) != 0u);
    }

    /* An empty or absent stream is not mixed. */
    assert(!suts_susch_is_mixed_script(NULL, 0u));

    printf("[PASS] test_mixed_script\n");
}

/* ── zones and the neography carve-out (§5) ────────────────────────── */

static void test_zones_and_neography(void)
{
    uint32_t set = 0u;

    assert(suts_susch_zone_of(0x00000061u) == SUSCH_ZONE_BRIDGE);
    assert(suts_susch_zone_of(0x0010FFFFu) == SUSCH_ZONE_BRIDGE);
    assert(suts_susch_zone_of(0x00110000u) == SUSCH_ZONE_SCP);
    assert(suts_susch_zone_of(0x00120000u) == SUSCH_ZONE_NATIVE);
    assert(suts_susch_zone_of(0x00160000u) == SUSCH_ZONE_RNUR);
    assert(suts_susch_zone_of(0x0016FFFFu) == SUSCH_ZONE_RNUR);
    assert(suts_susch_zone_of(0x00170000u) == SUSCH_ZONE_NATIVE);
    assert(suts_susch_zone_of(0x7FFFFFF0u) == SUSCH_ZONE_TRAP);
    assert(suts_susch_zone_of(0x7FFFFFFFu) == SUSCH_ZONE_SENTINEL);
    assert(suts_susch_zone_of(0x80000000u) == SUSCH_ZONE_EXTSUCS);

    /* The neography set decode matches the RNUR coordinate convention. */
    assert(suts_susch_is_neography(0x00160000u));
    assert(suts_susch_is_neography(0x00160100u));
    assert(!suts_susch_is_neography(0x0015FFFFu));
    assert(suts_susch_neography_set(0x00160100u, &set));
    assert(set == 1u);
    assert(suts_susch_neography_set(0x00160200u, &set));
    assert(set == 2u);
    assert(!suts_susch_neography_set(0x00000061u, &set));
    assert(!suts_susch_neography_set(0x00160000u, NULL));

    /* The Bridge boundary is the one SUSCH protects a source file from. */
    assert(suts_susch_crosses_bridge_boundary(0x00000061u, 0x00160000u));
    assert(suts_susch_crosses_bridge_boundary(0x00160000u, 0x00000061u));
    assert(suts_susch_crosses_bridge_boundary(0x00000061u, 0x00130000u));
    assert(!suts_susch_crosses_bridge_boundary(0x00000061u, 0x00000430u));

    /* A chunk that mixes a Bridge letter with a neography letter is a
     * cross-zone finding as well as a mixed-script one. */
    {
        static const uint32_t s[] = { 0x00000061u, 0x00160000u };
        uint32_t warnings = 0u;
        assert(suts_susch_validate_token_stream_ex(s, 2u, NULL, &warnings,
                                                     NULL));
        assert((warnings & SUSCH_WARN_CROSS_ZONE) != 0u);
        assert((warnings & SUSCH_WARN_MIXED_SCRIPT) != 0u);
        assert((warnings & SUSCH_WARN_RESTRICTED) != 0u);
    }

    printf("[PASS] test_zones_and_neography\n");
}

/* ── the token-stream validator (§6) ───────────────────────────────── */

static void test_validator_statuses(void)
{
    susch_token_status_t status = SUSCH_TOKEN_OK;
    size_t index = 0u;
    uint32_t warnings = 0u;

    /* A clean identifier is accepted. */
    {
        static const uint32_t s[] = { 'a', 'b', 'c' };
        assert(suts_susch_validate_token_stream(s, 3u));
        assert(suts_susch_validate_token_stream_ex(s, 3u, &status, &warnings,
                                                     &index));
        assert(status == SUSCH_TOKEN_OK);
        assert(warnings == SUSCH_WARN_NONE);
        assert(index == 3u);
    }

    /* An absent stream with a non-zero length is a null-stream defect. */
    assert(!suts_susch_validate_token_stream_ex(NULL, 4u, &status, NULL, NULL));
    assert(status == SUSCH_TOKEN_ERR_NULL);

    /* An absent or empty stream is an empty identifier. */
    assert(!suts_susch_validate_token_stream_ex(NULL, 0u, &status, NULL, NULL));
    assert(status == SUSCH_TOKEN_ERR_EMPTY);
    assert(!suts_susch_validate_token_stream_ex(NULL, 0u, &status, NULL, NULL));
    assert(status == SUSCH_TOKEN_ERR_EMPTY);

    /* The trap slots, the Sentinel, and the plugin space carry no source
     * text and are refused as an illegal zone. */
    {
        uint32_t s[1];
        s[0] = 0x7FFFFFF0u;
        assert(!suts_susch_validate_token_stream_ex(s, 1u, &status, NULL,
                                                     NULL));
        assert(status == SUSCH_TOKEN_ERR_ILLEGAL_ZONE);
        s[0] = 0x7FFFFFFFu;
        assert(!suts_susch_validate_token_stream_ex(s, 1u, &status, NULL,
                                                     NULL));
        assert(status == SUSCH_TOKEN_ERR_ILLEGAL_ZONE);
        s[0] = 0x80000000u;
        assert(!suts_susch_validate_token_stream_ex(s, 1u, &status, NULL,
                                                     NULL));
        assert(status == SUSCH_TOKEN_ERR_ILLEGAL_ZONE);
        s[0] = 0x00110000u;
        assert(!suts_susch_validate_token_stream_ex(s, 1u, &status, NULL,
                                                     NULL));
        assert(status == SUSCH_TOKEN_ERR_ILLEGAL_ZONE);
    }

    /* A line terminator inside a token is a defect. */
    {
        uint32_t s[2];
        s[0] = 0x00000061u;
        s[1] = 0x0000000Au;
        assert(!suts_susch_validate_token_stream_ex(s, 2u, &status, NULL,
                                                     &index));
        assert(status == SUSCH_TOKEN_ERR_LINE_BREAK);
        assert(index == 1u);
    }

    /* A digit may not start an identifier. */
    {
        uint32_t s[2];
        s[0] = 0x00000031u;
        s[1] = 0x00000061u;
        assert(!suts_susch_validate_token_stream_ex(s, 2u, &status, NULL,
                                                     &index));
        assert(status == SUSCH_TOKEN_ERR_INVALID_START);
        assert(index == 0u);
    }

    /* Punctuation may not continue one either. */
    {
        uint32_t s[3];
        s[0] = 0x00000061u;
        s[1] = 0x0000002Du;
        s[2] = 0x00000062u;
        assert(!suts_susch_validate_token_stream_ex(s, 3u, &status, NULL,
                                                     &index));
        assert(status == SUSCH_TOKEN_ERR_INVALID_CONTINUE);
        assert(index == 1u);
    }

    /* A digit continues an identifier happily. */
    {
        uint32_t s[2];
        s[0] = 0x00000061u;
        s[1] = 0x00000031u;
        assert(suts_susch_validate_token_stream(s, 2u));
    }

    printf("[PASS] test_validator_statuses\n");
}

/* ── advisory findings never change the verdict (§6.3) ─────────────── */

static void test_advisory_findings(void)
{
    uint32_t warnings = 0u;

    /* A clean token produces no advisory finding. */
    {
        static const uint32_t s[] = { 'a', 'b', 'c' };
        assert(suts_susch_validate_token_stream_ex(s, 3u, NULL, &warnings,
                                                     NULL));
        assert(warnings == SUSCH_WARN_NONE);
    }

    /* The advisory bitmap stays inside its declared mask. */
    {
        static const uint32_t s[] = { 0x00000061u, 0x00000430u,
                                       0x00160000u };
        assert(suts_susch_validate_token_stream_ex(s, 3u, NULL, &warnings,
                                                     NULL));
        assert((warnings & ~((uint32_t)SUSCH_WARN_MASK_DEFINED)) == 0u);
        assert((warnings & SUSCH_WARN_HOMOGLYPH) != 0u);
        assert((warnings & SUSCH_WARN_MIXED_SCRIPT) != 0u);
        assert((warnings & SUSCH_WARN_CROSS_ZONE) != 0u);
        assert((warnings & SUSCH_WARN_RESTRICTED) != 0u);
    }

    printf("[PASS] test_advisory_findings\n");
}

/* ── script classification (§4.3) ───────────────────────────────────── */

static void test_script_classification(void)
{
    assert(suts_susch_script_of(0x00000061u) == SUSCH_SCRIPT_LATIN);
    assert(suts_susch_script_of(0x00000030u) == SUSCH_SCRIPT_COMMON);
    assert(suts_susch_script_of(0x000003B1u) == SUSCH_SCRIPT_GREEK);
    assert(suts_susch_script_of(0x00000430u) == SUSCH_SCRIPT_CYRILLIC);
    assert(suts_susch_script_of(0x00000570u) == SUSCH_SCRIPT_ARMENIAN);
    assert(suts_susch_script_of(0x000005D0u) == SUSCH_SCRIPT_HEBREW);
    assert(suts_susch_script_of(0x00000628u) == SUSCH_SCRIPT_ARABIC);
    assert(suts_susch_script_of(0x00000710u) == SUSCH_SCRIPT_SYRIAC);
    assert(suts_susch_script_of(0x00000905u) == SUSCH_SCRIPT_DEVANAGARI);
    assert(suts_susch_script_of(0x000009B0u) == SUSCH_SCRIPT_BENGALI);
    assert(suts_susch_script_of(0x00000E01u) == SUSCH_SCRIPT_THAI);
    assert(suts_susch_script_of(0x000010D0u) == SUSCH_SCRIPT_GEORGIAN);
    assert(suts_susch_script_of(0x00004E00u) == SUSCH_SCRIPT_HAN);
    assert(suts_susch_script_of(0x00003044u) == SUSCH_SCRIPT_HIRAGANA);
    assert(suts_susch_script_of(0x000030A2u) == SUSCH_SCRIPT_KATAKANA);
    assert(suts_susch_script_of(0x0000AC00u) == SUSCH_SCRIPT_HANGUL);
    assert(suts_susch_script_of(0x00000301u) == SUSCH_SCRIPT_INHERITED);
    assert(suts_susch_script_of(0x00003001u) == SUSCH_SCRIPT_COMMON);
    assert(suts_susch_script_of(0x00160000u) == SUSCH_SCRIPT_NEOGRAPHY);
    assert(suts_susch_script_of(0x00130000u) == SUSCH_SCRIPT_NATIVE);
    assert(suts_susch_script_of(0x0000A720u) == SUSCH_SCRIPT_UNKNOWN);

    printf("[PASS] test_script_classification\n");
}

/* ── taxonomy names ────────────────────────────────────────────────── */

static void test_taxonomy_names(void)
{
    assert(suts_susch_zone_name(SUSCH_ZONE_BRIDGE)[0] == 'B');
    assert(suts_susch_zone_name(SUSCH_ZONE_SCP)[0] == 'S');
    assert(suts_susch_zone_name(SUSCH_ZONE_RNUR)[0] == 'R');
    assert(suts_susch_zone_name(SUSCH_ZONE_NATIVE)[0] == 'N');
    assert(suts_susch_zone_name(SUSCH_ZONE_TRAP)[0] == 'T');
    assert(suts_susch_zone_name(SUSCH_ZONE_SENTINEL)[0] == 'S');
    assert(suts_susch_zone_name(SUSCH_ZONE_EXTSUCS)[0] == 'E');

    assert(suts_susch_script_name(SUSCH_SCRIPT_LATIN)[0] == 'L');
    assert(suts_susch_script_name(SUSCH_SCRIPT_CYRILLIC)[0] == 'C');
    assert(suts_susch_script_name(SUSCH_SCRIPT_NEOGRAPHY)[0] == 'N');
    assert(suts_susch_script_name(SUSCH_SCRIPT_COMMON)[0] == 'C');
    assert(suts_susch_script_name(SUSCH_SCRIPT_INHERITED)[0] == 'I');

    assert(suts_susch_status_name(SUSCH_TOKEN_OK)[0] == 'O');
    assert(suts_susch_status_name(SUSCH_TOKEN_ERR_BIDI_CONTROL)[0] == 'B');
    assert(suts_susch_status_name(SUSCH_TOKEN_ERR_INVISIBLE)[0] == 'I');
    assert(suts_susch_status_name(SUSCH_TOKEN_ERR_INVALID_START)[0] == 'I');

    assert(suts_susch_identifier_status_name(SUSCH_IDENT_ALLOWED)[0] == 'A');
    assert(suts_susch_identifier_status_name(SUSCH_IDENT_RESTRICTED)[0] == 'R');
    assert(suts_susch_identifier_status_name(SUSCH_IDENT_DISALLOWED)[0] == 'D');

    /* Names for unknown values degrade safely. */
    assert(suts_susch_zone_name((susch_zone_t)99)[0] == 'U');
    assert(suts_susch_script_name((susch_script_t)99)[0] == 'U');
    assert(suts_susch_status_name((susch_token_status_t)99)[0] == 'U');
    assert(suts_susch_identifier_status_name((susch_identifier_status_t)99)[0]
           == 'U');

    printf("[PASS] test_taxonomy_names\n");
}

/* ── guards and misses ─────────────────────────────────────────────── */

static void test_guards(void)
{
    /* A NULL output pointer is refused. */
    assert(!suts_susch_get_attributes(0x00000061u, NULL));

    /* A codepoint above the Sentinel is ExtSUCS plugin space, not source
     * code, and has no compiled evidence. */
    assert(!suts_susch_get_attributes(0x80000000u, NULL));

    /* A NULL chunk counter is refused. */
    assert(!suts_susch_chunk_count(NULL, 0u, NULL));

    /* The simple validator is the extended one with no diagnostic channel. */
    {
        static const uint32_t s[] = { 'a', 0x0000202Eu };
        assert(!suts_susch_validate_token_stream(s, 2u));
    }

    printf("[PASS] test_guards\n");
}

/* ── SSRE registration (SUTS-008 is a registered optional spec) ───── */

static void test_ssre_registration(void)
{
    assert(SUSCH_SPEC_ID[0] == 'S');
    assert(SUSCH_SPEC_ID[5] == '0' && SUSCH_SPEC_ID[7] == '8');
    printf("[PASS] test_ssre_registration\n");
}

int main(void)
{
    test_version_and_layout();
    test_attribute_query();
    test_bidi_controls();
    test_invisibles();
    test_homoglyphs();
    test_identifier_status();
    test_chunk_boundaries();
    test_mixed_script();
    test_zones_and_neography();
    test_validator_statuses();
    test_advisory_findings();
    test_script_classification();
    test_taxonomy_names();
    test_guards();
    test_ssre_registration();
    printf("[PASS] SUTS-8 SUSCH tests passed\n");
    return 0;
}
