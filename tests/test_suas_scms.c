/**
 * SUAS-011 SCMS — System Character Manifest & Schema Standard
 *
 * Unit tests for the line-oriented, binary-mappable manifest grammar: the
 * UAX #42 analog in which a character database is a stream of bounded
 * tab-delimited records that compile straight into immutable kernel tables.
 *
 * Covers: version/meta, line classification, the record grammar on its happy
 * path (single codepoints, ranges, RNUR neography), every documented
 * validation outcome, column extraction, the directive form, the streaming
 * reader's completeness and integrity checks, plane admissibility and the
 * plane-mask cross-check, descriptor alignment, outcome naming, and the
 * SUAS-010 (SSRE) linkage gate.
 */

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>

#include "suas/suas_scms.h"
#include "suas/suas_ssre.h"

static int g_fail = 0;
#define CHECK(cond) do { if (!(cond)) { \
    printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); g_fail = 1; \
} } while (0)

/* Feeds a NUL-terminated line, the way a build tool hands over a buffer. */
static bool feed(scms_manifest_state_t *st, const char *line)
{
    return suas_scms_feed_line(st, line, strlen(line));
}

static bool valid_record(const char *line, scms_record_descriptor_t *desc)
{
    return suas_scms_validate_record(line, line ? strlen(line) : 0u, desc);
}

/* Asserts that a line is rejected with one specific outcome, so a regression
 * that produces the right kind of failure for the wrong reason is caught. */
static void expect_error(const char *line, scms_error_t want)
{
    scms_record_descriptor_t desc;

    if (valid_record(line, &desc)) {
        printf("FAIL %s:%d: expected rejection of <%s>\n",
               __FILE__, __LINE__, line);
        g_fail = 1;
        return;
    }
    if (suas_scms_last_error() != want) {
        printf("FAIL %s:%d: <%s> gave %s, wanted %s\n",
               __FILE__, __LINE__, line ? line : "(null)",
               suas_scms_error_name(suas_scms_last_error()),
               suas_scms_error_name(want));
        g_fail = 1;
    }
}

static void test_version_and_meta(void)
{
    const char* vs = suas_scms_version_string();

    CHECK(vs != NULL);
    CHECK(vs[0] == 'S');
    CHECK(vs[7] == '1');       /* "SUAS-011/1.0.0" */
    CHECK(vs[8] == '/');
    CHECK(vs[9] == '1');
    CHECK(vs[13] == '0');
    CHECK(vs[14] == '\0');
    CHECK(SUAS_SCMS_SPEC_ID[5] == '0');
    CHECK(SUAS_SCMS_SPEC_ID[6] == '1');
    CHECK(SUAS_SCMS_SPEC_ID[7] == '1');
    CHECK(SUAS_SCMS_SPEC_TITLE[0] == 'S');
    CHECK(SUAS_SCMS_VERSION_MAJOR == 1);

    CHECK(SUAS_SCMS_MAX_LINE == 128U);
    CHECK(SUAS_SCMS_RECORD_FIELD_COUNT == 6U);
    CHECK(SUAS_SCMS_CODEPOINT_HEX_MAX == 8U);
    CHECK(SUAS_SCMS_RADICAL_MAX == 214U);
    CHECK(SUAS_SCMS_STROKES_MAX == 255U);
    CHECK(SUAS_SCMS_FLAGS_MAX == 0xFFFFU);

    /* The descriptor is fixed-size and caller-owned (§4.4). */
    CHECK(offsetof(scms_record_descriptor_t, start_codepoint) == 0U);
    CHECK(sizeof(scms_record_descriptor_t) == 24U);

    /* Init is idempotent and arms the engine. */
    suas_scms_init_engine();
    suas_scms_init_engine();
    CHECK(suas_scms_last_error() == SCMS_OK);
}

static void test_line_classification(void)
{
    CHECK(suas_scms_line_kind("", 0u) == SCMS_LINE_BLANK);
    CHECK(suas_scms_line_kind("# comment", 9u) == SCMS_LINE_COMMENT);
    CHECK(suas_scms_line_kind("%manifest 1.0", 13u) == SCMS_LINE_DIRECTIVE);
    CHECK(suas_scms_line_kind("U+4E00\tU+4E00\t1\t1\t0x01\t19968", 28u)
          == SCMS_LINE_RECORD);
    CHECK(suas_scms_line_kind(NULL, 0u) == SCMS_LINE_INVALID);
    CHECK(suas_scms_line_kind("x", SUAS_SCMS_MAX_LINE) == SCMS_LINE_INVALID);
}

/* ── The record grammar, happy path ───────────────────────────────── */

static void test_valid_records(void)
{
    scms_record_descriptor_t d;

    /* A single codepoint is one kernel table slot. */
    CHECK(valid_record("U+4E00\tU+4E00\t1\t1\t0x01\t19968", &d));
    CHECK(d.start_codepoint == 0x4E00u);
    CHECK(d.end_codepoint == 0x4E00u);
    CHECK(d.field_count == 6u);
    CHECK(d.is_binary_aligned == true);
    CHECK(suas_scms_is_binary_aligned(&d));

    /* A range is not binary-aligned: the compiler must expand it. */
    CHECK(valid_record("U+4E00\tU+9FFF\t1\t1\t0x01\t0", &d));
    CHECK(d.start_codepoint == 0x4E00u);
    CHECK(d.end_codepoint == 0x9FFFu);
    CHECK(d.is_binary_aligned == false);
    CHECK(!suas_scms_is_binary_aligned(&d));
    CHECK(suas_scms_is_binary_aligned(NULL) == false);

    /* Lowercase hex and the u+ form are accepted. */
    CHECK(valid_record("u+4e00\tu+4e00\t1\t1\t0x01\t0", &d));
    CHECK(d.start_codepoint == 0x4E00u);

    /* RNUR neography logographs survive the 64-bit round trip. */
    CHECK(valid_record("U+00160100\tU+00160100\t0\t0\t0x02\t0", &d));
    CHECK(d.start_codepoint == 0x00160100u);
    CHECK(d.is_binary_aligned == true);

    /* Bounds at every column. */
    CHECK(valid_record("U+00000000\tU+00000000\t0\t0\t0x0\t0", &d));
    CHECK(valid_record("U+10FFFF\tU+10FFFF\t214\t255\t0xFFFF\t4294967295", &d));
    CHECK(d.start_codepoint == 0x10FFFFu);
}

/* ── The record grammar, every documented outcome ─────────────────── */

static void test_record_errors(void)
{
    /* Structural. */
    expect_error("", SCMS_ERR_BAD_FIELD_COUNT);
    expect_error("# comment", SCMS_ERR_BAD_FIELD_COUNT);
    expect_error("%manifest 1.0", SCMS_ERR_BAD_FIELD_COUNT);
    expect_error(NULL, SCMS_ERR_NULL_INPUT);
    expect_error("U+4E00\tU+4E00\t1\t1\t0x01\t19968\t9", SCMS_ERR_BAD_FIELD_COUNT);
    expect_error("U+4E00\tU+4E00\t1\t1\t0x01", SCMS_ERR_BAD_FIELD_COUNT);
    expect_error("U+4E00\t\t1\t1\t0x01\t19968", SCMS_ERR_EMPTY_FIELD);
    /* A trailing tab yields only five fields, not an empty sixth. */
    expect_error("U+4E00\tU+4E00\t1\t1\t0x01\t", SCMS_ERR_BAD_FIELD_COUNT);

    /* Codepoint form. */
    expect_error("4E00\tU+4E00\t1\t1\t0x01\t0", SCMS_ERR_BAD_CODEPOINT);
    expect_error("U4E00\tU+4E00\t1\t1\t0x01\t0", SCMS_ERR_BAD_CODEPOINT);
    expect_error("U+4E00G\tU+4E00\t1\t1\t0x01\t0", SCMS_ERR_BAD_CODEPOINT);
    expect_error("U+4E0000000\tU+4E00\t1\t1\t0x01\t0", SCMS_ERR_BAD_CODEPOINT);
    expect_error("U+\tU+4E00\t1\t1\t0x01\t0", SCMS_ERR_BAD_CODEPOINT);
    /* Leading whitespace is not permitted anywhere in a record. */
    expect_error(" U+4E00\tU+4E00\t1\t1\t0x01\t0", SCMS_ERR_BAD_CODEPOINT);

    /* Range semantics. */
    expect_error("U+9FFF\tU+4E00\t1\t1\t0x01\t0", SCMS_ERR_RANGE_ORDER);

    /* SCP invariance: the Control Plane is never data (§4.3). */
    expect_error("U+00110000\tU+00110000\t0\t0\t0x0\t0",
                 SCMS_ERR_INADMISSIBLE_CODEPOINT);
    expect_error("U+0011FFFF\tU+0011FFFF\t0\t0\t0x0\t0",
                 SCMS_ERR_INADMISSIBLE_CODEPOINT);
    /* The 15 trap slots and the Sentinel. */
    expect_error("U+7FFFFFF0\tU+7FFFFFF0\t0\t0\t0x0\t0",
                 SCMS_ERR_INADMISSIBLE_CODEPOINT);
    expect_error("U+7FFFFFFF\tU+7FFFFFFF\t0\t0\t0x0\t0",
                 SCMS_ERR_INADMISSIBLE_CODEPOINT);
    /* Rejecting both endpoints is not enough: a range whose interior swallows
     * the Control Plane describes Control Plane codepoints as data. */
    expect_error("U+10FFFF\tU+00120000\t0\t0\t0x0\t0",
                 SCMS_ERR_INADMISSIBLE_CODEPOINT);
    expect_error("U+10FFFE\tU+7FFFFFFF\t0\t0\t0x0\t0",
                 SCMS_ERR_INADMISSIBLE_CODEPOINT);
    /* A range wide enough to breach the span ceiling, without crossing an
     * inadmissible zone: Native plane through ExtSUCS. */
    expect_error("U+00120000\tU+00230000\t0\t0\t0x0\t0",
                 SCMS_ERR_RANGE_TOO_WIDE);

    /* Payload bounds. */
    expect_error("U+4E00\tU+4E00\t215\t1\t0x01\t0", SCMS_ERR_BAD_RADICAL);
    expect_error("U+4E00\tU+4E00\t-1\t1\t0x01\t0", SCMS_ERR_BAD_RADICAL);
    expect_error("U+4E00\tU+4E00\t1\t256\t0x01\t0", SCMS_ERR_BAD_STROKES);
    expect_error("U+4E00\tU+4E00\t1\t1\t0x10000\t0", SCMS_ERR_BAD_FLAGS);
    expect_error("U+4E00\tU+4E00\t1\t1\t1\t0", SCMS_ERR_BAD_FLAGS);
    expect_error("U+4E00\tU+4E00\t1\t1\t0x01\t4294967296", SCMS_ERR_BAD_WEIGHT);
    expect_error("U+4E00\tU+4E00\t1\t1\t0x01\t19968x", SCMS_ERR_BAD_WEIGHT);

    /* The line budget. */
    {
        char big[SUAS_SCMS_MAX_LINE + 8u];
        memset(big, 'a', sizeof big);
        big[0] = 'U';
        CHECK(!suas_scms_validate_record(big, SUAS_SCMS_MAX_LINE, NULL));
        CHECK(suas_scms_last_error() == SCMS_ERR_NULL_INPUT);
        CHECK(!suas_scms_validate_record(big, sizeof big,
                                         (scms_record_descriptor_t*)big));
        CHECK(suas_scms_last_error() == SCMS_ERR_LINE_TOO_LONG);
    }
}

/* ── Column extraction ────────────────────────────────────────────── */

static void test_field_at(void)
{
    const char* line = "U+4E00\tU+9FFF\t7\t42\t0x01\t19968";
    uint32_t v = 0u;

    CHECK(suas_scms_field_at(line, strlen(line), 0u, &v));
    CHECK(v == 0x4E00u);
    CHECK(suas_scms_field_at(line, strlen(line), 1u, &v));
    CHECK(v == 0x9FFFu);
    CHECK(suas_scms_field_at(line, strlen(line), 2u, &v));
    CHECK(v == 7u);
    CHECK(suas_scms_field_at(line, strlen(line), 3u, &v));
    CHECK(v == 42u);
    CHECK(suas_scms_field_at(line, strlen(line), 4u, &v));
    CHECK(v == 0x01u);
    CHECK(suas_scms_field_at(line, strlen(line), 5u, &v));
    CHECK(v == 19968u);

    CHECK(!suas_scms_field_at(line, strlen(line), 6u, &v));
    CHECK(!suas_scms_field_at(line, strlen(line), 0u, NULL));
    CHECK(!suas_scms_field_at(NULL, 0u, 0u, &v));
    CHECK(!suas_scms_field_at("x", 1u, 0u, &v));
}

/* ── Directives ───────────────────────────────────────────────────── */

static void test_directives(void)
{
    CHECK(suas_scms_validate_directive("%manifest 1.0.0", 15u));
    CHECK(suas_scms_validate_directive("%planes 0x0D", 12u));
    CHECK(suas_scms_validate_directive("%records 3", 10u));
    /* A toolchain-private directive is well-formed and therefore legal. */
    CHECK(suas_scms_validate_directive("%toolchain acme-build-7", 22u));

    CHECK(!suas_scms_validate_directive("%", 1u));
    CHECK(!suas_scms_validate_directive("%manifest", 9u));
    CHECK(!suas_scms_validate_directive("%1manifest x", 12u));
    CHECK(!suas_scms_validate_directive("%averyveryverylongname x", 23u));
    /* A whitespace run between name and value is legal. */
    CHECK(suas_scms_validate_directive("%manifest \t1.0", 14u));
    CHECK(!suas_scms_validate_directive(NULL, 0u));
}

/* ── Admissibility and the plane mask ─────────────────────────────── */

static void test_planes(void)
{
    CHECK(suas_scms_codepoint_admissible(0x0000u));
    CHECK(suas_scms_codepoint_admissible(0x4E00u));
    CHECK(suas_scms_codepoint_admissible(0x10FFFFu));
    CHECK(suas_scms_codepoint_admissible(0x00160100u));  /* RNUR         */
    CHECK(suas_scms_codepoint_admissible(0x00200000u));  /* ExtSUCS      */
    CHECK(!suas_scms_codepoint_admissible(0x00110000u)); /* SCP floor    */
    CHECK(!suas_scms_codepoint_admissible(0x0011FFFFu)); /* SCP ceiling  */
    CHECK(!suas_scms_codepoint_admissible(0x7FFFFFF0u)); /* trap slot    */
    CHECK(!suas_scms_codepoint_admissible(0x7FFFFFFEu)); /* trap slot    */
    CHECK(!suas_scms_codepoint_admissible(0x7FFFFFFFu)); /* the Sentinel */

    CHECK(suas_scms_range_admissible(0x4E00u, 0x9FFFu));
    CHECK(suas_scms_range_admissible(0x4E00u, 0x4E00u));
    CHECK(!suas_scms_range_admissible(0x9FFFu, 0x4E00u));
    CHECK(!suas_scms_range_admissible(0x4E00u, 0x00110000u));
    /* Admissible endpoints, inadmissible interior (§4.3). */
    CHECK(!suas_scms_range_admissible(0x10FFFFu, 0x00120000u));
    CHECK(!suas_scms_range_admissible(0x0016FFFFu, 0x7FFFFFFFu));
    CHECK(!suas_scms_range_admissible(0x4E00u, 0x4E00u + SUAS_SCMS_RANGE_MAX_SPAN + 1u));

    /* Plane classification. */
    CHECK(suas_scms_plane_mask(0x4E00u, 0x9FFFu) == SUAS_SCMS_PLANE_BRIDGE);
    CHECK(suas_scms_plane_mask(0x00160100u, 0x00160100u) ==
          (SUAS_SCMS_PLANE_NATIVE | SUAS_SCMS_PLANE_RNUR));
    CHECK(suas_scms_plane_mask(0x10FFFFu, 0x00120000u) ==
          (SUAS_SCMS_PLANE_BRIDGE | SUAS_SCMS_PLANE_SCP |
           SUAS_SCMS_PLANE_NATIVE));
    CHECK(suas_scms_plane_mask(0x9FFFu, 0x4E00u) == 0u);
}

/* ── The streaming reader ─────────────────────────────────────────── */

static void test_streaming_valid_manifest(void)
{
    scms_manifest_state_t st;

    /* A manifest covering the Bridge and the RNUR carve-out. */
    static const char* const lines[] = {
        "# a SuperUnicode character manifest",
        "%manifest 1.0.0",
        "%records 3",
        "%planes 0x0D",
        "",
        "U+4E00\tU+4E00\t1\t1\t0x01\t19968",
        "U+4E01\tU+4E02\t1\t2\t0x01\t19969",
        "U+00160100\tU+00160100\t0\t0\t0x02\t0"
    };
    size_t i;

    /* begin() is optional — the engine arms lazily — but is the documented
     * toolchain entry point, so it is exercised here. */
    suas_scms_begin(&st);
    CHECK(st.records == 0u && st.errors == 0u);

    for (i = 0u; i < sizeof lines / sizeof lines[0]; i++)
        CHECK(feed(&st, lines[i]));

    CHECK(st.lines == 8u);
    CHECK(st.records == 3u);
    CHECK(st.comments == 1u);
    CHECK(st.directives == 3u);
    CHECK(st.errors == 0u);
    CHECK(st.first_error_line == 0u);
    CHECK(st.saw_manifest == true);
    CHECK(st.saw_records == true);
    CHECK(st.header.manifest_version != NULL);
    CHECK(st.header.manifest_version[0] == '1');
    CHECK(st.header.total_records == 3u);
    CHECK(st.header.plane_mask == 0x0Du);
    CHECK(st.observed_planes == 0x0Du);

    CHECK(suas_scms_end(&st));
    CHECK(suas_scms_planes_declared(&st));
}

static void test_streaming_failures(void)
{
    scms_manifest_state_t st;

    /* A count that does not match the %records directive. */
    suas_scms_begin(&st);
    CHECK(feed(&st, "%manifest 1.0"));
    CHECK(feed(&st, "%records 2"));
    CHECK(feed(&st, "U+4E00\tU+4E00\t1\t1\t0x01\t0"));
    CHECK(suas_scms_end(&st) == false);
    CHECK(suas_scms_last_error() == SCMS_ERR_RECORD_COUNT_MISMATCH);

    /* No %manifest directive at all. */
    {
        scms_manifest_state_t st2;
        suas_scms_begin(&st2);
        CHECK(feed(&st2, "U+4E00\tU+4E00\t1\t1\t0x01\t0"));
        CHECK(suas_scms_end(&st2) == false);
        CHECK(suas_scms_last_error() == SCMS_ERR_MISSING_HEADER);
    }

    /* A header and no records. */
    {
        scms_manifest_state_t st3;
        suas_scms_begin(&st3);
        CHECK(feed(&st3, "%manifest 1.0"));
        CHECK(feed(&st3, "# nothing here yet"));
        CHECK(suas_scms_end(&st3) == false);
        CHECK(suas_scms_last_error() == SCMS_ERR_EMPTY_MANIFEST);
    }

    /* A bad record poisons the whole manifest, and the state names the line. */
    {
        scms_manifest_state_t st4;
        suas_scms_begin(&st4);
        CHECK(feed(&st4, "%manifest 1.0"));
        CHECK(feed(&st4, "U+4E00\tU+4E00\t1\t1\t0x01\t0") == true);
        CHECK(feed(&st4, "U+9FFF\tU+4E00\t1\t1\t0x01\t0") == false);
        CHECK(st4.errors == 1u);
        CHECK(st4.first_error_line == 3u);   /* 1-based */
        CHECK(st4.last_error == (uint8_t)SCMS_ERR_RANGE_ORDER);
        CHECK(st4.records == 1u);
        CHECK(suas_scms_end(&st4) == false);
    }

    /* A record written outside the planes the manifest declared. */
    {
        scms_manifest_state_t st5;
        suas_scms_begin(&st5);
        CHECK(feed(&st5, "%manifest 1.0"));
        CHECK(feed(&st5, "%planes 0x01"));
        CHECK(feed(&st5, "U+4E00\tU+4E00\t1\t1\t0x01\t0"));
        CHECK(feed(&st5, "U+00160100\tU+00160100\t0\t0\t0x02\t0"));
        CHECK(suas_scms_end(&st5));            /* structurally complete  */
        CHECK(suas_scms_planes_declared(&st5) == false);
        CHECK(st5.observed_planes == 0x0Du);
        CHECK(st5.header.plane_mask == 0x01u);
    }

    /* A manifest that declares no planes at all cannot pass the cross-check. */
    {
        scms_manifest_state_t st6;
        suas_scms_begin(&st6);
        CHECK(feed(&st6, "%manifest 1.0"));
        CHECK(feed(&st6, "U+4E00\tU+4E00\t1\t1\t0x01\t0"));
        CHECK(suas_scms_end(&st6));
        CHECK(suas_scms_planes_declared(&st6) == false);
    }

    /* NULL state and NULL line. */
    CHECK(suas_scms_feed_line(NULL, "x", 1u) == false);
    CHECK(suas_scms_last_error() == SCMS_ERR_NULL_INPUT);
    CHECK(suas_scms_end(NULL) == false);
    CHECK(suas_scms_planes_declared(NULL) == false);
    /* The state from the top of this function still carries one record
     * against a declared total of two, so it cannot close. */
    CHECK(suas_scms_end(&st) == false);
    CHECK(suas_scms_last_error() == SCMS_ERR_RECORD_COUNT_MISMATCH);
}

/* ── Outcome naming ───────────────────────────────────────────────── */

static void test_error_names(void)
{
    int i;

    CHECK(suas_scms_error_name(SCMS_OK)[0] == 'S');
    CHECK(strcmp(suas_scms_error_name(SCMS_ERR_RANGE_ORDER),
                 "SCMS_ERR_RANGE_ORDER") == 0);
    CHECK(strcmp(suas_scms_error_name(SCMS_ERR_EMPTY_MANIFEST),
                 "SCMS_ERR_EMPTY_MANIFEST") == 0);
    CHECK(strcmp(suas_scms_error_name((scms_error_t)999),
                 "SCMS_ERR_UNKNOWN") == 0);

    /* Every declared outcome must have a name of its own. */
    for (i = (int)SCMS_OK; i <= (int)SCMS_ERR_EMPTY_MANIFEST; i++)
        CHECK(strcmp(suas_scms_error_name((scms_error_t)i),
                     "SCMS_ERR_UNKNOWN") != 0);
}

/* ── The SUAS-010 (SSRE) linkage gate ─────────────────────────────── */

static void test_ssre_link(void)
{
    scms_schema_link_t link;

    /* A manifest feeding SIP's property table: SUAS-009 is registered and an
     * invariant, so a mandatory claim against it is sound. */
    link.schema_spec_id   = "SUAS-009";
    link.schema_major     = 1u;
    link.schema_minor     = 0u;
    link.schema_mandatory = true;
    CHECK(suas_scms_verify_schema_link(&link));

    /* The same mandatory claim against SCSS, an optional data spec, is a
     * linkage error and must not compile a blob. */
    link.schema_spec_id = "SUTS-005";
    CHECK(suas_scms_verify_schema_link(&link) == false);

    link.schema_spec_id = "SUTS-005";
    link.schema_mandatory = false;
    CHECK(suas_scms_verify_schema_link(&link));

    /* An unregistered schema, and an over-strict floor. */
    link.schema_spec_id = "SUAS-015";
    CHECK(suas_scms_verify_schema_link(&link) == false);
    link.schema_spec_id = "SUAS-009";
    link.schema_major   = 2u;
    CHECK(suas_scms_verify_schema_link(&link) == false);

    CHECK(suas_scms_verify_schema_link(NULL) == false);
    link.schema_spec_id = NULL;
    CHECK(suas_scms_verify_schema_link(&link) == false);
}

/* SCMS and SSRE must agree that SCMS is part of the architecture. */
static void test_scms_is_registered(void)
{
    CHECK(suas_ssre_is_registered(SUAS_SCMS_SPEC_ID));
    CHECK(suas_ssre_is_mandatory(SUAS_SCMS_SPEC_ID));
    CHECK(suas_ssre_module_of(SUAS_SCMS_SPEC_ID) != NULL);
}

int main(void)
{
    test_version_and_meta();
    test_line_classification();
    test_valid_records();
    test_record_errors();
    test_field_at();
    test_directives();
    test_planes();
    test_streaming_valid_manifest();
    test_streaming_failures();
    test_error_names();
    test_ssre_link();
    test_scms_is_registered();

    if (g_fail) {
        printf("SUAS-011 SCMS tests FAILED\n");
        return 1;
    }
    printf("SUAS-011 SCMS tests passed\n");
    return 0;
}
