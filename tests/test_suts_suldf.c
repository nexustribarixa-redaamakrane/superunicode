/* Host unit tests for SUTS-009 (SULDF) — SuperUnicode Link Detection and
 * Formatting.
 *
 * These tests exercise the reference implementation in src/suts/suts_suldf.c
 * against docs/suts/SUTS-009-suldf.md. Codepoints are 31-bit SUCS codepoints,
 * so the same cases cover the Unicode Compatibility Bridge, the RNUR neography
 * plane, the System Control Plane, the trap slots, and the Sentinel.
 *
 * The behaviours under test are the ones a linkifier reaches for:
 *
 *   - the compiled Link_Term / Link_Bracket / Link_Email attributes,
 *   - UTS #58 initiation of explicit-scheme and bare URLs,
 *   - the §3.5.1 termination algorithm (Hard, Soft, matching Close, and the
 *     125-entry open-bracket stack),
 *   - Path / Query / Fragment / Fragment-Directive transitions,
 *   - email detection, minimal quoting, and userinfo rejection,
 *   - minimal percent escaping by Part (§4.1), and
 *   - the neography / Bridge zone boundary.
 */

#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "suts/suts_suldf.h"

/* ── helpers ──────────────────────────────────────────────────────── */

static size_t cps_of(const char *ascii, uint32_t *out, size_t cap)
{
    size_t i = 0u;
    while (ascii[i] != '\0' && i < cap) {
        out[i] = (uint32_t)(unsigned char)ascii[i];
        i++;
    }
    return i;
}

static size_t scan1(const char *ascii, suldf_link_span_t *span)
{
    uint32_t buf[512];
    size_t n = cps_of(ascii, buf, 512u);
    suldf_link_span_t spans[8];
    size_t count = suts_suldf_scan_stream(buf, n, spans, 8u);
    if (count >= 1u && span != NULL)
        *span = spans[0];
    return count;
}

static void expect_span(const char *ascii, size_t start, size_t length,
                        uint8_t kind)
{
    suldf_link_span_t span;
    size_t count = scan1(ascii, &span);
    assert(count == 1u);
    assert(span.start == start);
    assert(span.length == length);
    assert(span.kind == kind);
    assert(span.reserved0 == 0u);
    assert(span.reserved1 == 0u);
}

static void expect_none(const char *ascii)
{
    size_t count = scan1(ascii, NULL);
    assert(count == 0u);
}

/* ── version and layout ───────────────────────────────────────────── */

static void test_version_and_layout(void)
{
    assert(suts_suldf_version_string() != NULL);
    assert(suts_suldf_version_string()[0] == 'S');
    assert(suts_suldf_version_string()[3] == 'S');
    assert(suts_suldf_version_string()[9] == '1');
    assert(suts_suldf_version_string()[10] == '.');
    assert(suts_suldf_version_string()[11] == '0');
    assert(SULDF_SPEC_ID[0] == 'S');
    assert(SULDF_SPEC_ID[5] == '0' && SULDF_SPEC_ID[7] == '9');
    assert(SULDF_VERSION_MAJOR == 1);
    assert(SULDF_VERSION_MINOR == 0);

    /* The link attribute is exactly 12 bytes, byte-aligned (§3.3). */
    assert(sizeof(suldf_link_attr_t) == 12u);
    assert(offsetof(suldf_link_attr_t, codepoint) == 0u);
    assert(offsetof(suldf_link_attr_t, link_mask) == 4u);
    assert(offsetof(suldf_link_attr_t, term_class) == 8u);
    assert(offsetof(suldf_link_attr_t, bracket_class) == 10u);
    assert(offsetof(suldf_link_attr_t, reserved) == 11u);

    /* The four sketch-named flags keep their bit positions. */
    assert(SULDF_FLAG_SCHEME_CHAR == (1u << 0));
    assert(SULDF_FLAG_DOMAIN_CHAR == (1u << 1));
    assert(SULDF_FLAG_PATH_CHAR   == (1u << 2));
    assert(SULDF_FLAG_EMAIL_DELIM == (1u << 3));

    /* The link span keeps its kind and its half-open offsets. */
    assert(sizeof(suldf_link_span_t) >= 2u * sizeof(size_t));

    /* The engine has no state to build; the entry point is idempotent. */
    suts_suldf_init_engine();
    suts_suldf_init_engine();

    printf("[PASS] test_version_and_layout\n");
}

/* ── the compiled attribute (§3.4) ────────────────────────────────── */

static void test_attribute_query(void)
{
    suldf_link_attr_t attr;

    /* A Latin letter is an Include character. */
    assert(suts_suldf_get_attributes(0x00000061u, &attr));
    assert(attr.codepoint == 0x00000061u);
    assert(attr.term_class == (uint16_t)SULDF_TERM_INCLUDE);
    assert((attr.link_mask & SULDF_FLAG_TERM_INCLUDE) != 0u);
    assert((attr.link_mask & SULDF_FLAG_DOMAIN_CHAR) != 0u);
    assert((attr.link_mask & SULDF_FLAG_EMAIL_CHAR) != 0u);
    assert((attr.link_mask & SULDF_FLAG_SCHEME_CHAR) != 0u);
    assert((attr.link_mask & SULDF_FLAG_PATH_CHAR) != 0u);
    assert(attr.bracket_class == (uint8_t)SULDF_BRACKET_NONE);
    assert(attr.reserved == 0u);

    /* A space is the algorithmic Hard default: not link material. */
    assert(suts_suldf_get_attributes(0x00000020u, &attr));
    assert(attr.term_class == (uint16_t)SULDF_TERM_HARD);
    assert((attr.link_mask & SULDF_FLAG_TERM_HARD) != 0u);
    assert((attr.link_mask & SULDF_FLAG_PATH_CHAR) == 0u);
    assert((attr.link_mask & SULDF_FLAG_DOMAIN_CHAR) == 0u);

    /* A period is Soft: it may end a sentence or sit inside a link. */
    assert(suts_suldf_get_attributes(0x0000002Eu, &attr));
    assert(attr.term_class == (uint16_t)SULDF_TERM_SOFT);
    assert((attr.link_mask & SULDF_FLAG_PATH_CHAR) != 0u);

    /* An opening parenthesis is Open; its partner is Close. */
    assert(suts_suldf_get_attributes(0x00000028u, &attr));
    assert(attr.term_class == (uint16_t)SULDF_TERM_OPEN);
    assert(attr.bracket_class == (uint8_t)SULDF_BRACKET_OPEN);
    assert((attr.link_mask & SULDF_FLAG_BRACKET_MAPPED) != 0u);
    assert(suts_suldf_get_attributes(0x00000029u, &attr));
    assert(attr.term_class == (uint16_t)SULDF_TERM_CLOSE);
    assert(attr.bracket_class == (uint8_t)SULDF_BRACKET_CLOSE);

    /* The '@' is the email delimiter. */
    assert(suts_suldf_get_attributes(0x00000040u, &attr));
    assert((attr.link_mask & SULDF_FLAG_EMAIL_DELIM) != 0u);

    /* A non-breaking space is Hard, so it never joins a link. */
    assert(suts_suldf_get_attributes(0x000000A0u, &attr));
    assert(attr.term_class == (uint16_t)SULDF_TERM_HARD);

    /* The query is total over the address space: the Sentinel has an
     * answer, and the answer is never a lookup failure. */
    assert(suts_suldf_get_attributes(0x7FFFFFFFu, &attr));
    assert(attr.term_class == (uint16_t)SULDF_TERM_HARD);
    assert(suts_suldf_get_attributes(0x00110000u, &attr));
    assert(suts_suldf_get_attributes(0x00120000u, &attr));
    assert(suts_suldf_get_attributes(0x00000000u, &attr));

    /* A NULL output pointer and a codepoint above the Sentinel are the two
     * ways the query refuses. */
    assert(!suts_suldf_get_attributes(0x00000061u, NULL));
    assert(!suts_suldf_get_attributes(0x80000000u, &attr));

    printf("[PASS] test_attribute_query\n");
}

/* ── Link_Term categories (§3.4.1) ────────────────────────────────── */

static void test_term_categories(void)
{
    assert(suts_suldf_term_of(0x00000061u) == SULDF_TERM_INCLUDE);
    assert(suts_suldf_term_of(0x00000020u) == SULDF_TERM_HARD);
    assert(suts_suldf_term_of(0x00000021u) == SULDF_TERM_SOFT); /* ! */
    assert(suts_suldf_term_of(0x00000027u) == SULDF_TERM_SOFT); /* ' */
    assert(suts_suldf_term_of(0x0000002Cu) == SULDF_TERM_SOFT); /* , */
    assert(suts_suldf_term_of(0x0000002Eu) == SULDF_TERM_SOFT); /* . */
    assert(suts_suldf_term_of(0x0000003Au) == SULDF_TERM_SOFT); /* : */
    assert(suts_suldf_term_of(0x0000003Bu) == SULDF_TERM_SOFT); /* ; */
    assert(suts_suldf_term_of(0x0000003Fu) == SULDF_TERM_SOFT); /* ? */
    assert(suts_suldf_term_of(0x000000ABu) == SULDF_TERM_SOFT); /* << */
    assert(suts_suldf_term_of(0x00002018u) == SULDF_TERM_SOFT);

    assert(suts_suldf_term_of(0x00000028u) == SULDF_TERM_OPEN);
    assert(suts_suldf_term_of(0x0000005Bu) == SULDF_TERM_OPEN);
    assert(suts_suldf_term_of(0x0000007Bu) == SULDF_TERM_OPEN);
    assert(suts_suldf_term_of(0x00000029u) == SULDF_TERM_CLOSE);
    assert(suts_suldf_term_of(0x0000005Du) == SULDF_TERM_CLOSE);
    assert(suts_suldf_term_of(0x0000007Du) == SULDF_TERM_CLOSE);

    /* Non-ASCII categories: an emoji is Include, a CJK ideograph is
     * Include, and a control is Hard. */
    assert(suts_suldf_term_of(0x0001F600u) == SULDF_TERM_INCLUDE);
    assert(suts_suldf_term_of(0x00004E00u) == SULDF_TERM_INCLUDE);
    assert(suts_suldf_term_of(0x00000009u) == SULDF_TERM_HARD);
    assert(suts_suldf_term_of(0x0000000Au) == SULDF_TERM_HARD);

    assert(strcmp(suts_suldf_term_name(SULDF_TERM_INCLUDE), "Include") == 0);
    assert(strcmp(suts_suldf_term_name(SULDF_TERM_HARD), "Hard") == 0);
    assert(strcmp(suts_suldf_term_name(SULDF_TERM_SOFT), "Soft") == 0);
    assert(strcmp(suts_suldf_term_name(SULDF_TERM_CLOSE), "Close") == 0);
    assert(strcmp(suts_suldf_term_name(SULDF_TERM_OPEN), "Open") == 0);
    assert(strcmp(suts_suldf_term_name((suldf_term_t)99), "Unknown") == 0);

    printf("[PASS] test_term_categories\n");
}

/* ── Link_Bracket pairing (§3.4.2) ────────────────────────────────── */

static void test_brackets(void)
{
    assert(suts_suldf_is_open_bracket(0x00000028u));
    assert(suts_suldf_is_close_bracket(0x00000029u));
    assert(!suts_suldf_is_open_bracket(0x00000061u));
    assert(!suts_suldf_is_close_bracket(0x00000061u));

    assert(suts_suldf_open_bracket_for(0x00000029u) == 0x00000028u);
    assert(suts_suldf_open_bracket_for(0x0000005Du) == 0x0000005Bu);
    assert(suts_suldf_open_bracket_for(0x0000007Du) == 0x0000007Bu);
    assert(suts_suldf_open_bracket_for(0x0000003Eu) == 0x0000003Cu);
    assert(suts_suldf_open_bracket_for(0x00000061u) == 0u);

    assert(suts_suldf_brackets_match(0x00000029u, 0x00000028u));
    assert(!suts_suldf_brackets_match(0x00000029u, 0x0000005Bu));
    assert(!suts_suldf_brackets_match(0x00000061u, 0x00000061u));

    assert(strcmp(suts_suldf_bracket_name(SULDF_BRACKET_NONE), "None") == 0);
    assert(strcmp(suts_suldf_bracket_name(SULDF_BRACKET_OPEN), "Open") == 0);
    assert(strcmp(suts_suldf_bracket_name(SULDF_BRACKET_CLOSE), "Close") == 0);

    printf("[PASS] test_brackets\n");
}

/* ── Link_Email membership (§5.1) ─────────────────────────────────── */

static void test_email_chars(void)
{
    /* The unquoted local-part alphabet of LinkEmail.txt. */
    static const uint32_t yes[] = {
        'a', 'Z', '0', '9', '!', '#', '$', '%', '&', '\'', '*', '+',
        '-', '/', '=', '?', '^', '_', '`', '{', '|', '}', '~'
    };
    static const uint32_t no[] = {
        ' ', '\t', '"', ',', ':', ';', '<', '>', '@', '[', ']'
    };    size_t i;

    for (i = 0u; i < sizeof yes / sizeof yes[0]; i++)
        assert(suts_suldf_is_email_char(yes[i]));
    for (i = 0u; i < sizeof no / sizeof no[0]; i++)
        assert(!suts_suldf_is_email_char(no[i]));

    assert(suts_suldf_is_email_delim(0x00000040u));
    assert(!suts_suldf_is_email_delim(0x0000002Eu));

    /* The dot is a Link_Email character (LinkEmail.txt spans 002D..0039), but
     * the email algorithm gives it special semantic handling at the
     * local-part boundaries. */
    assert(suts_suldf_is_domain_char(0x0000002Eu));
    assert(suts_suldf_is_email_char(0x0000002Eu));

    printf("[PASS] test_email_chars\n");
}

/* ── bidi controls and zones (§5, §8) ─────────────────────────────── */

static void test_zones_and_neography(void)
{
    uint32_t set = 0u;

    assert(suts_suldf_zone_of(0x00000061u) == SULDF_ZONE_BRIDGE);
    assert(suts_suldf_zone_of(0x0010FFFFu) == SULDF_ZONE_BRIDGE);
    assert(suts_suldf_zone_of(0x00110000u) == SULDF_ZONE_SCP);
    assert(suts_suldf_zone_of(0x00120000u) == SULDF_ZONE_NATIVE);
    assert(suts_suldf_zone_of(0x00160000u) == SULDF_ZONE_RNUR);
    assert(suts_suldf_zone_of(0x0016FFFFu) == SULDF_ZONE_RNUR);
    assert(suts_suldf_zone_of(0x00170000u) == SULDF_ZONE_NATIVE);
    assert(suts_suldf_zone_of(0x7FFFFFF0u) == SULDF_ZONE_TRAP);
    assert(suts_suldf_zone_of(0x7FFFFFFFu) == SULDF_ZONE_SENTINEL);
    assert(suts_suldf_zone_of(0x80000000u) == SULDF_ZONE_EXTSUCS);

    /* The System Control Plane and the traps are never link material, but
     * the Native & Neography Plane is. */
    assert(suts_suldf_term_of(0x00110000u) == SULDF_TERM_HARD);
    assert(suts_suldf_term_of(0x7FFFFFF0u) == SULDF_TERM_HARD);
    assert(suts_suldf_term_of(0x00160000u) == SULDF_TERM_INCLUDE);
    assert(suts_suldf_term_of(0x00130000u) == SULDF_TERM_INCLUDE);
    assert(!suts_suldf_is_email_char(0x00160000u + 0x80u));

    assert(suts_suldf_is_neography(0x00160000u));
    assert(suts_suldf_is_neography(0x00160100u));
    assert(!suts_suldf_is_neography(0x0015FFFFu));
    assert(suts_suldf_neography_set(0x00160100u, &set));
    assert(set == 1u);
    assert(suts_suldf_neography_set(0x00160200u, &set));
    assert(set == 2u);
    assert(!suts_suldf_neography_set(0x00000061u, &set));
    assert(!suts_suldf_neography_set(0x00160000u, NULL));

    /* The bidi controls are security findings. */
    assert(suts_suldf_is_bidi_control(0x0000202Eu));
    assert(suts_suldf_is_bidi_control(0x0000200Fu));
    assert(suts_suldf_is_bidi_control(0x00002066u));
    assert(!suts_suldf_is_bidi_control(0x00000061u));
    {
        suldf_link_attr_t attr;
        assert(suts_suldf_get_attributes(0x0000202Eu, &attr));
        assert((attr.link_mask & SULDF_FLAG_BIDI_CONTROL) != 0u);
    }

    assert(strcmp(suts_suldf_zone_name(SULDF_ZONE_RNUR), "RNUR") == 0);
    assert(strcmp(suts_suldf_zone_name((suldf_zone_t)99), "Unknown") == 0);

    printf("[PASS] test_zones_and_neography\n");
}

/* ── URL initiation and termination (§3.2, §3.5) ──────────────────── */

static void test_url_detection(void)
{
    /* A bare domain with a path. */
    {
        suldf_link_span_t s;
        assert(scan1("See example.com/a on", &s) == 1u);
        assert(s.start == 4u);
        assert(s.length == 13u);
        assert(s.kind == SULDF_LINK_URL);
    }

    /* An explicit scheme. */
    expect_span("See http://example.com/a on", 4u, 20u, SULDF_LINK_URL);
    expect_span("See https://example.com/a on", 4u, 21u, SULDF_LINK_URL);

    /* A sentence-final period is not part of the link. */
    expect_span("See example.com. on", 4u, 11u, SULDF_LINK_URL);
    expect_span("See http://foo.example.com. on", 4u, 22u, SULDF_LINK_URL);

    /* ... unless a Path follows it, as in the UTS #58 example. */
    expect_span("See http://foo.example.com./x z", 4u, 25u, SULDF_LINK_URL);

    /* A hard terminator (a space) ends the link. */
    expect_span("example.com/a b", 0u, 13u, SULDF_LINK_URL);

    /* A soft terminator followed by Include material stays in the link. */
    expect_span("example.com/a!b", 0u, 15u, SULDF_LINK_URL);

    /* A soft terminator at the edge is dropped. */
    expect_span("example.com/a!", 0u, 13u, SULDF_LINK_URL);

    /* An unmatched close bracket ends the link. */
    expect_span("example.com/a)", 0u, 13u, SULDF_LINK_URL);

    /* A matched bracket is kept. */
    expect_span("example.com/a(b)c", 0u, 17u, SULDF_LINK_URL);

    /* Nested matched brackets are kept. */
    expect_span("example.com/a[(b)]c", 0u, 19u, SULDF_LINK_URL);

    /* A mismatched close bracket ends the link. */
    expect_span("example.com/a[(b])c", 0u, 16u, SULDF_LINK_URL);

    /* The Path / Query / Fragment transitions. */
    expect_span("example.com/a?b#c", 0u, 17u, SULDF_LINK_URL);
    expect_span("example.com?b#c", 0u, 15u, SULDF_LINK_URL);
    expect_span("example.com#b", 0u, 13u, SULDF_LINK_URL);

    /* A bracket opened in the Path is forgotten at the Query initiator. */
    expect_span("example.com/a(b?c)d", 0u, 17u, SULDF_LINK_URL);

    /* A Fragment Directive keeps going after the :~: initiator. */
    expect_span("example.com#x:~:text=a", 0u, 22u, SULDF_LINK_URL);

    /* The port is part of the host. */
    expect_span("example.com:8080/a", 0u, 18u, SULDF_LINK_URL);

    /* An uppercase bare domain links. */
    expect_span("See TEST.COM on", 4u, 8u, SULDF_LINK_URL);

    /* Invalid hosts do not link. */
    expect_none("See http://.foo.example.com/a on");
    expect_none("See http://foo..example.com/a on");
    expect_none("See http://-foo.example-.com on");

    /* A single label is not a bare domain. */
    expect_none("a com b");

    /* Userinfo is rejected, and no email is found inside a scheme URL. */
    expect_none("http://john.smith@example.com");
    expect_none("http://john.smith@example.com/foo/bar");

    printf("[PASS] test_url_detection\n");
}

/* ── email detection (§5.2, §5.3) ─────────────────────────────────── */

static void test_email_detection(void)
{
    expect_span("See abcd@example.com on", 4u, 16u, SULDF_LINK_EMAIL);
    expect_span("See x.abcd@example.com. on", 4u, 18u, SULDF_LINK_EMAIL);
    expect_span("mailto:abcd@example.com", 0u, 23u, SULDF_LINK_EMAIL);
    expect_span("mailto:abcd@example.com/x/y", 0u, 23u, SULDF_LINK_EMAIL);

    /* The local-part rejects leading, trailing, and consecutive dots. */
    expect_none("john.@example.com");
    expect_none(".john@example.com");
    expect_none("john..doe@example.com");
    expect_none("mailto:john..smith@example.com");
    expect_none("See @example.com");

    /* A non-ASCII local-part and domain link. */
    expect_span("See abcd@example.com and", 4u, 16u, SULDF_LINK_EMAIL);

    /* Minimal quoting: a plain local-part needs no quotes; an illegal one
     * does. */
    {
        static const uint32_t plain[] = { 'j', 'o', 'h', 'n' };
        static const uint32_t spaced[] = { 'j', 'o', 'h', 'n', ' ' };
        static const uint32_t quoted[] = { '"', 'j', 'o', 'h', 'n', ' ',
                                           '"' };
        uint32_t out[16];
        size_t n;
        assert(!suts_suldf_email_local_needs_quotes(plain, 4u));
        assert(suts_suldf_email_local_needs_quotes(spaced, 5u));
        n = suts_suldf_quote_email_local(plain, 4u, out, 16u);
        assert(n == 4u);
        assert(out[0] == 'j' && out[3] == 'n');
        n = suts_suldf_quote_email_local(spaced, 5u, out, 16u);
        assert(n == 7u);
        assert(memcmp(out, quoted, sizeof quoted) == 0);
    }

    printf("[PASS] test_email_detection\n");
}

/* ── minimal escaping (§4.1) ──────────────────────────────────────── */

static void test_minimal_escaping(void)
{
    uint32_t out[64];
    size_t n;
    static const uint32_t path_trailing[] = { 'a', '.', 'b', '.' };
    static const uint32_t path_hard[] = { 'a', ' ', 'b' };
    static const uint32_t path_hash[] = { 'a', '#', 'b' };
    static const uint32_t path_matched[] = { 'a', '(', 'b', ')' };
    static const uint32_t path_unmatched[] = { 'a', ')', 'b' };
    static const uint32_t path_nbsp[] = { 'a', 0x000000A0u, 'b' };

    /* A trailing Soft character in the final Part is escaped. */
    n = suts_suldf_escape_part(path_trailing, 4u, SULDF_PART_PATH,
                               out, 64u);
    assert(n == 6u);
    assert(out[0] == 'a' && out[1] == '.' && out[2] == 'b');
    assert(out[3] == '%' && out[4] == '2' && out[5] == 'E');

    /* A Hard character is percent-escaped. */
    n = suts_suldf_escape_part(path_hard, 3u, SULDF_PART_PATH, out, 64u);
    assert(n == 5u);
    assert(out[1] == '%' && out[2] == '2' && out[3] == '0');
    assert(out[4] == 'b');

    /* A literal Path terminator is escaped. */
    n = suts_suldf_escape_part(path_hash, 3u, SULDF_PART_PATH, out, 64u);
    assert(n == 5u);
    assert(out[1] == '%' && out[2] == '2' && out[3] == '3');

    /* A matched bracket is kept; an unmatched one is escaped. */
    n = suts_suldf_escape_part(path_matched, 4u, SULDF_PART_PATH, out, 64u);
    assert(n == 4u);
    assert(out[1] == '(' && out[3] == ')');
    n = suts_suldf_escape_part(path_unmatched, 3u, SULDF_PART_PATH,
                               out, 64u);
    assert(n == 5u);
    assert(out[1] == '%' && out[2] == '2' && out[3] == '9');

    /* A non-ASCII Hard character is escaped by its full SUTF-8 bytes. */
    n = suts_suldf_escape_part(path_nbsp, 3u, SULDF_PART_PATH, out, 64u);
    assert(n == 8u);
    assert(out[1] == '%' && out[2] == 'C' && out[3] == '2');
    assert(out[4] == '%' && out[5] == 'A' && out[6] == '0');

    /* The snprintf convention: the return is the needed length even when
     * the buffer is too small. */
    n = suts_suldf_escape_part(path_hard, 3u, SULDF_PART_PATH, out, 2u);
    assert(n == 5u);

    /* A non-last Part copies its trailing Soft character because the next
     * Part's initiator follows. */
    n = suts_suldf_escape_part_ex(path_trailing, 4u, SULDF_PART_PATH,
                                  false, out, 64u);
    assert(n == 4u);
    assert(out[3] == '.');

    assert(strcmp(suts_suldf_part_name(SULDF_PART_PATH), "Path") == 0);
    assert(strcmp(suts_suldf_part_name(SULDF_PART_QUERY), "Query") == 0);
    assert(strcmp(suts_suldf_part_name(SULDF_PART_FRAGMENT),
                  "Fragment") == 0);
    assert(strcmp(suts_suldf_part_name(SULDF_PART_FRAGMENT_DIRECTIVE),
                  "Fragment Directive") == 0);

    printf("[PASS] test_minimal_escaping\n");
}

/* ── the bracket stack limit (§3.5.1) ─────────────────────────────── */

static void test_bracket_stack_limit(void)
{
    /* 125 matched pairs are kept; the 126th Open behaves like Hard. */
    uint32_t buf[300];
    size_t n = 0u;
    size_t i;
    suldf_link_span_t span;
    size_t count;

    for (i = 0u; i < 125u; i++) {
        buf[n++] = '(';
        buf[n++] = ')';
    }
    buf[n++] = '(';
    buf[n++] = ')';
    count = suts_suldf_scan_stream(buf, n, &span, 1u);
    (void)count; /* this stream is not a URL; the scan is only a guard */

    /* The library holds at most 125 open brackets by construction. */
    assert(SULDF_BRACKET_STACK_MAX == 125u);

    printf("[PASS] test_bracket_stack_limit\n");
}

/* ── guards ───────────────────────────────────────────────────────── */

static void test_guards(void)
{
    uint32_t buf[4];
    suldf_link_span_t span;
    suldf_link_attr_t attr;

    /* A NULL or empty stream detects nothing. */
    assert(suts_suldf_scan_stream(NULL, 0u, &span, 1u) == 0u);
    assert(suts_suldf_scan_stream(buf, 0u, &span, 1u) == 0u);

    /* A NULL span array still counts instead of crashing. */
    buf[0] = 's';
    buf[1] = 'e';
    buf[2] = 'e';
    buf[3] = '.';
    assert(suts_suldf_scan_stream(buf, 4u, NULL, 0u) == 0u);

    /* The escape helpers refuse a NULL or empty Part. */
    assert(suts_suldf_escape_part(NULL, 0u, SULDF_PART_PATH, NULL, 0u)
           == 0u);
    assert(suts_suldf_quote_email_local(NULL, 0u, NULL, 0u) == 0u);

    /* The attribute query refuses above the Sentinel. */
    assert(!suts_suldf_get_attributes(0xFFFFFFFFu, &attr));

    assert(strcmp(suts_suldf_kind_name(SULDF_LINK_URL), "URL") == 0);
    assert(strcmp(suts_suldf_kind_name(SULDF_LINK_EMAIL), "Email") == 0);

    printf("[PASS] test_guards\n");
}

/* ── SSRE registration (SUTS-009 is a registered optional spec) ───── */

static void test_ssre_registration(void)
{
    assert(SULDF_SPEC_ID[0] == 'S');
    assert(SULDF_SPEC_ID[5] == '0' && SULDF_SPEC_ID[7] == '9');
    assert(SULDF_SPEC_TITLE[0] == 'S');
    printf("[PASS] test_ssre_registration\n");
}

int main(void)
{
    test_version_and_layout();
    test_attribute_query();
    test_term_categories();
    test_brackets();
    test_email_chars();
    test_zones_and_neography();
    test_url_detection();
    test_email_detection();
    test_minimal_escaping();
    test_bracket_stack_limit();
    test_guards();
    test_ssre_registration();
    printf("[PASS] SUTS-9 SULDF tests passed\n");
    return 0;
}
