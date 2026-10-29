/**
 * SUTS-012 SCTS - System Console Text Service
 *
 * Unit tests for the optional console contract: SGW width deference, SBR
 * wrap opportunities, STS cluster atomicity, wide lead/tail pairs, scroll,
 * tabs, sink levels, serialization, and the glyph-zone registry.
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "suts/suts_scts.h"

static int g_fail = 0;
#define CHECK(cond) do { if (!(cond)) { \
    printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); g_fail = 1; \
} } while (0)

/* -- Capturing sink ---------------------------------------------------- */

static uint8_t  g_sink[512];
static size_t   g_sink_len;
static unsigned g_calls;

static void capture_sink(uintptr_t token, const uint8_t *b, uint32_t n)
{
    uint32_t i;
    (void)token;
    g_calls++;
    for (i = 0u; i < n && g_sink_len < sizeof g_sink; i++) {
        g_sink[g_sink_len++] = b[i];
    }
}

static void sink_reset(void)
{
    g_sink_len = 0u;
    g_calls = 0u;
    memset(g_sink, 0, sizeof g_sink);
}

/* -- Helpers ----------------------------------------------------------- */

static void init_plane(scts_plane_t *p, scts_cell_t *cells, uint16_t rows,
                       uint16_t cols, uint16_t tab, bool wrap,
                       bool wide_context)
{
    scts_plane_options_t o;
    scts_plane_options_default(&o);
    o.tab_width = tab;
    o.wrap = wrap;
    o.wide_context = wide_context;
    CHECK(scts_plane_init(p, cells, rows, cols, &o) == SCTS_OK);
}

static void expect_row(const scts_plane_t *p, uint16_t row, const char *want)
{
    uint8_t buf[128];
    size_t n = 0u;
    size_t wlen = strlen(want);
    CHECK(scts_plane_serialize_row(p, row, buf, sizeof buf, true, &n) ==
          SCTS_OK);
    CHECK(n == wlen);
    if (n == wlen) {
        CHECK(memcmp(buf, want, n) == 0);
    }
}

/* -- Tests ------------------------------------------------------------- */

static void test_version_and_status(void)
{
    CHECK(suts_scts_version_string() != NULL);
    CHECK(strstr(suts_scts_version_string(), "SUTS-012") != NULL);
    CHECK(strcmp(suts_scts_status_name(SCTS_OK), "SCTS_OK") == 0);
    CHECK(strcmp(suts_scts_status_name(SCTS_E_CLUSTER),
                 "SCTS_E_CLUSTER") == 0);
    CHECK(strcmp(suts_scts_status_name(SCTS_E_ZONE), "SCTS_E_ZONE") == 0);
    CHECK(strcmp(suts_scts_status_name((scts_status_t)999),
                 "SCTS_E_UNKNOWN") == 0);
}

static void test_init_and_geometry(void)
{
    scts_plane_t p;
    scts_cell_t cells[12];

    memset(&p, 0, sizeof p);
    CHECK(scts_plane_init(&p, NULL, 2, 2, NULL) == SCTS_E_NULL);
    CHECK(scts_plane_init(&p, cells, 0, 2, NULL) == SCTS_E_GEOMETRY);
    CHECK(scts_plane_init(&p, cells, 2, 0, NULL) == SCTS_E_GEOMETRY);

    CHECK(scts_plane_newline(&p) == SCTS_E_NOT_INIT);
    CHECK(scts_plane_write(&p, (const uint8_t *)"x", 1) == SCTS_E_NOT_INIT);
    CHECK(scts_plane_scroll(&p, 1) == SCTS_E_NOT_INIT);

    init_plane(&p, cells, 2, 3, 8, true, false);
    CHECK(p.rows == 2 && p.cols == 3);
    CHECK(p.cursor_row == 0 && p.cursor_col == 0);
    CHECK(cells[0].cp == SCTS_MARK_BLANK);
    CHECK(cells[0].attrs == 0);
    CHECK(scts_plane_row_dirty(&p, 0));
    CHECK(scts_plane_row_dirty(&p, 1));

    CHECK(scts_plane_serialize_row(&p, 2, (uint8_t *)g_sink, 8, true,
                                   &(size_t){0}) == SCTS_E_ROW_RANGE);
    {
        uint32_t out[4];
        size_t n = 0;
        CHECK(scts_plane_cluster_at(&p, 0, 0, out, 4, &n) == SCTS_E_CLUSTER);
    }

    CHECK(scts_plane_set_style(&p, 2, 3) == SCTS_OK);
    CHECK(scts_plane_put_cp(&p, 'A') == SCTS_OK);
    CHECK(cells[0].cp == 'A');
    CHECK((cells[0].attrs & SCTS_ATTR_FG_MASK) == 2);
    CHECK(((cells[0].attrs & SCTS_ATTR_BG_MASK) >> 4) == 3);
}

static void test_width_deference(void)
{
    scts_plane_t p;
    scts_cell_t cells[16];

    /* U+01D4 is Ambiguous: one cell in narrow context, two when wide. */
    init_plane(&p, cells, 1, 8, 8, true, false);
    CHECK(scts_plane_write(&p, (const uint8_t *)"\xC7\x94", 2) == SCTS_OK);
    CHECK(p.cursor_col == 1);
    CHECK(cells[0].cp == 0x01D4u);

    init_plane(&p, cells, 1, 8, 8, true, true);
    CHECK(scts_plane_write(&p, (const uint8_t *)"\xC7\x94", 2) == SCTS_OK);
    CHECK(p.cursor_col == 2);
    CHECK(cells[0].cp == 0x01D4u);
    CHECK((cells[0].attrs & SCTS_ATTR_WIDE_LEAD) != 0);
    CHECK(cells[1].cp == SCTS_MARK_CONT);
    CHECK((cells[1].attrs & SCTS_ATTR_WIDE_TAIL) != 0);

    /* Fullwidth A is always two cells. */
    init_plane(&p, cells, 1, 8, 8, true, false);
    CHECK(scts_plane_put_cp(&p, 0xFF21u) == SCTS_OK);
    CHECK(p.cursor_col == 2);
    CHECK(cells[0].cp == 0xFF21u);
    CHECK(cells[1].cp == SCTS_MARK_CONT);

    /* ASCII stays one cell. */
    init_plane(&p, cells, 1, 8, 8, true, false);
    CHECK(scts_plane_put_cp(&p, 'A') == SCTS_OK);
    CHECK(p.cursor_col == 1);
}

static void test_combining_attachment(void)
{
    scts_plane_t p;
    scts_cell_t cells[16];
    uint32_t cluster[8];
    size_t n = 0;

    /* a + COMBINING DIAERESIS is one cluster in one cell. */
    init_plane(&p, cells, 1, 8, 8, true, false);
    CHECK(scts_plane_write(&p, (const uint8_t *)"a\xCC\x88", 3) == SCTS_OK);
    CHECK(p.cursor_col == 1);
    CHECK(scts_plane_cluster_at(&p, 0, 0, cluster, 8, &n) == SCTS_OK);
    CHECK(n == 2);
    CHECK(cluster[0] == 0x0061u && cluster[1] == 0x0308u);
    expect_row(&p, 0, "a\xCC\x88");

    /* A mark cannot be its own base. */
    init_plane(&p, cells, 1, 8, 8, true, false);
    CHECK(scts_plane_write(&p, (const uint8_t *)"\xCC\x88", 2) ==
          SCTS_E_CLUSTER);
    CHECK(p.cursor_col == 0);

    /* A mark after a base is not dropped. */
    init_plane(&p, cells, 1, 8, 8, true, false);
    CHECK(scts_plane_write(&p, (const uint8_t *)"e\xCC\x81", 3) == SCTS_OK);
    expect_row(&p, 0, "e\xCC\x81");
    CHECK(p.cursor_col == 1);
}

static void test_wide_pairs(void)
{
    scts_plane_t p;
    scts_cell_t cells[16];
    uint8_t buf[32];
    size_t n = 0;

    init_plane(&p, cells, 2, 4, 8, true, false);
    CHECK(scts_plane_put_cp(&p, 0xFF21u) == SCTS_OK);
    CHECK(scts_plane_put_cp(&p, 'x') == SCTS_OK);
    CHECK(p.cursor_col == 3);
    CHECK(cells[1].cp == SCTS_MARK_CONT);
    CHECK(cells[2].cp == 'x');

    /* The tail marker never reaches the serializer. */
    CHECK(scts_plane_serialize_row(&p, 0, buf, sizeof buf, false, &n) ==
          SCTS_OK);
    CHECK(n == 5); /* EF BC A1 + 'x' + one trailing blank */
    CHECK(buf[0] == 0xEF && buf[1] == 0xBC && buf[2] == 0xA1);
    CHECK(buf[3] == 'x');
    CHECK(buf[4] == ' ');

    /* Overwriting the lead clears the tail. */
    CHECK(scts_plane_carriage_return(&p) == SCTS_OK);
    CHECK(scts_plane_put_cp(&p, 'y') == SCTS_OK);
    CHECK(cells[0].cp == 'y');
    CHECK(cells[1].cp == SCTS_MARK_BLANK);
    CHECK((cells[0].attrs & SCTS_ATTR_WIDE_LEAD) == 0);
    expect_row(&p, 0, "y x");

    /* Overwriting the tail clears the lead. */
    init_plane(&p, cells, 2, 4, 8, true, false);
    CHECK(scts_plane_put_cp(&p, 0xFF21u) == SCTS_OK);
    CHECK(scts_plane_backspace(&p) == SCTS_OK);
    CHECK(p.cursor_col == 1);
    CHECK(scts_plane_put_cp(&p, 'z') == SCTS_OK);
    CHECK(cells[0].cp == SCTS_MARK_BLANK);
    CHECK(cells[1].cp == 'z');
    expect_row(&p, 0, " z");

    /* A wide glyph at the right edge wraps whole. */
    init_plane(&p, cells, 2, 4, 8, true, false);
    CHECK(scts_plane_write(&p, (const uint8_t *)"abc", 3) == SCTS_OK);
    CHECK(scts_plane_put_cp(&p, 0xFF21u) == SCTS_OK);
    CHECK(p.cursor_row == 1);
    CHECK(p.cursor_col == 2);
    CHECK(cells[4].cp == 0xFF21u);
    CHECK(cells[5].cp == SCTS_MARK_CONT);
    expect_row(&p, 0, "abc");
    expect_row(&p, 1, "\xEF\xBC\xA1");
}

static void test_wrap_and_scroll(void)
{
    scts_plane_t p;
    scts_cell_t cells[32];

    /* A word is moved whole to the next row when SBR allows the break. The
     * separating space stays on the first row: it is content, not a blank
     * cell, so trimming does not remove it. */
    init_plane(&p, cells, 2, 5, 8, true, false);
    CHECK(scts_plane_write(&p, (const uint8_t *)"ab cdef", 7) == SCTS_OK);
    CHECK(p.cursor_row == 1);
    expect_row(&p, 0, "ab ");
    expect_row(&p, 1, "cdef");

    /* "abc def" in five columns: "def" starts the next row. */
    init_plane(&p, cells, 2, 5, 8, true, false);
    CHECK(scts_plane_write(&p, (const uint8_t *)"abc def", 7) == SCTS_OK);
    expect_row(&p, 0, "abc ");
    expect_row(&p, 1, "def");

    /* An unbreakable word longer than the row is broken only because there
     * is no alternative. */
    init_plane(&p, cells, 2, 4, 8, true, false);
    CHECK(scts_plane_write(&p, (const uint8_t *)"abcdef", 6) == SCTS_OK);
    expect_row(&p, 0, "abcd");
    expect_row(&p, 1, "ef");

    /* Wrapping disabled fails and leaves the plane unchanged. */
    init_plane(&p, cells, 1, 3, 8, false, false);
    CHECK(scts_plane_write(&p, (const uint8_t *)"abc", 3) == SCTS_OK);
    CHECK(scts_plane_write(&p, (const uint8_t *)"d", 1) == SCTS_E_COL_RANGE);
    CHECK(p.cursor_col == 3);
    expect_row(&p, 0, "abc");

    /* Scroll: rows rise, the bottom row clears. */
    init_plane(&p, cells, 3, 4, 8, true, false);
    CHECK(scts_plane_write(&p, (const uint8_t *)"aa\nbb\ncc\ndd", 11) ==
          SCTS_OK);
    CHECK(p.scrolls == 1);
    expect_row(&p, 0, "bb");
    expect_row(&p, 1, "cc");
    expect_row(&p, 2, "dd");

    /* CRLF scrolls once, not twice. */
    init_plane(&p, cells, 2, 4, 8, true, false);
    CHECK(scts_plane_write(&p, (const uint8_t *)"a\r\nb\r\nc", 8) == SCTS_OK);
    CHECK(p.scrolls == 1);
    expect_row(&p, 0, "b");
    expect_row(&p, 1, "c");

    /* CR alone returns to column zero without a line feed. */
    init_plane(&p, cells, 2, 4, 8, true, false);
    CHECK(scts_plane_write(&p, (const uint8_t *)"ab\rcd", 5) == SCTS_OK);
    CHECK(p.cursor_row == 0);
    expect_row(&p, 0, "cd");
}

static void test_tabs_and_backspace(void)
{
    scts_plane_t p;
    scts_cell_t cells[32];

    init_plane(&p, cells, 1, 20, 4, true, false);
    CHECK(scts_plane_write(&p, (const uint8_t *)"a\tb", 3) == SCTS_OK);
    CHECK(p.cursor_col == 5);
    expect_row(&p, 0, "a   b");

    init_plane(&p, cells, 1, 8, 8, false, false);
    CHECK(scts_plane_write(&p, (const uint8_t *)"\t", 1) == SCTS_E_COL_RANGE);
    CHECK(p.cursor_col == 0);

    init_plane(&p, cells, 2, 8, 8, true, false);
    CHECK(scts_plane_write(&p, (const uint8_t *)"\t", 1) == SCTS_OK);
    CHECK(p.cursor_row == 1);
    CHECK(p.cursor_col == 0);

    init_plane(&p, cells, 1, 8, 8, true, false);
    CHECK(scts_plane_write(&p, (const uint8_t *)"ab\bc", 4) == SCTS_OK);
    expect_row(&p, 0, "ac");
}

static void test_controls_and_encoding(void)
{
    scts_plane_t p;
    scts_cell_t cells[32];

    init_plane(&p, cells, 2, 8, 8, true, false);
    CHECK(scts_plane_write(&p, (const uint8_t *)"\x01", 1) == SCTS_E_CONTROL);
    CHECK(scts_plane_write(&p, (const uint8_t *)"\x7F", 1) == SCTS_E_CONTROL);
    CHECK(scts_plane_write(&p, (const uint8_t *)"\xC0\x80", 2) ==
          SCTS_E_ENCODING);
    CHECK(scts_plane_write(&p, (const uint8_t *)"\xE2\x82", 2) ==
          SCTS_E_ENCODING);
    CHECK(scts_plane_write(&p, (const uint8_t *)"\xF8\x84\x90\x80\x80", 5) ==
          SCTS_E_INADMISSIBLE);
    CHECK(scts_plane_write(&p, (const uint8_t *)"\xFD\xBF\xBF\xBF\xBF\xBF", 6) ==
          SCTS_E_INADMISSIBLE);

    /* NUL is ignored but ends the current cluster. */
    init_plane(&p, cells, 1, 8, 8, true, false);
    CHECK(scts_plane_write(&p, (const uint8_t *)"a\0b", 3) == SCTS_OK);
    CHECK(cells[0].cp == 'a');
    CHECK(cells[1].cp == 'b');
    CHECK(p.cursor_col == 2);

    /* put_cp applies the same admission rules. */
    CHECK(scts_plane_put_cp(&p, 0x110000u) == SCTS_E_INADMISSIBLE);
    CHECK(scts_plane_put_cp(&p, 0x7FFFFFFFu) == SCTS_E_INADMISSIBLE);
    CHECK(scts_plane_put_cp(&p, 0x0001u) == SCTS_E_CONTROL);
}

static void test_serialization(void)
{
    scts_plane_t p;
    scts_cell_t cells[16];
    uint8_t buf[32];
    size_t n = 0;

    init_plane(&p, cells, 1, 5, 8, true, false);
    CHECK(scts_plane_write(&p, (const uint8_t *)"a b", 3) == SCTS_OK);

    CHECK(scts_plane_serialize_row(&p, 0, buf, sizeof buf, true, &n) ==
          SCTS_OK);
    CHECK(n == 3 && memcmp(buf, "a b", 3) == 0);

    CHECK(scts_plane_serialize_row(&p, 0, buf, sizeof buf, false, &n) ==
          SCTS_OK);
    CHECK(n == 5 && memcmp(buf, "a b  ", 5) == 0);

    /* Capacity is a hard bound: no truncated sequence is emitted. */
    CHECK(scts_plane_serialize_row(&p, 0, buf, 4, false, &n) ==
          SCTS_E_CAPACITY);
    CHECK(n == 0);

    CHECK(scts_plane_serialize_row(&p, 0, NULL, 0, true, &n) ==
          SCTS_E_CAPACITY);

    /* Empty row serializes to zero bytes. */
    init_plane(&p, cells, 1, 4, 8, true, false);
    CHECK(scts_plane_serialize_row(&p, 0, NULL, 0, true, &n) == SCTS_OK);
    CHECK(n == 0);
}

static void test_sink_levels(void)
{
    scts_plane_t p;
    scts_cell_t cells[32];
    scts_sink_t sink;
    uint8_t scratch[32];

    sink.emit = capture_sink;
    sink.token = 0u;
    sink.level = SCTS_SINK_L0_BYTE;

    init_plane(&p, cells, 1, 6, 8, true, false);
    scts_plane_set_sink(&p, &sink);
    CHECK(scts_plane_write(&p, (const uint8_t *)"A\xC3\xA9\xEF\xBC\xA1", 8) ==
          SCTS_OK);
    sink_reset();
    CHECK(scts_plane_emit_row(&p, 0, NULL, 0) == SCTS_OK);
    CHECK(g_sink_len == 5);
    CHECK(g_sink[0] == 'A');
    CHECK(g_sink[1] == '?');  /* e-acute is lossy on a byte sink */
    CHECK(g_sink[2] == '?');  /* fullwidth A is lossy            */
    CHECK(g_sink[3] == ' ');  /* tail is skipped, never emitted  */
    CHECK(g_sink[4] == ' ');
    CHECK(p.lossy_substitutions == 2);
    /* No marker and no partial sequence ever reaches an L0 sink. */
    {
        size_t i;
        for (i = 0; i < g_sink_len; i++) {
            CHECK(g_sink[i] >= 0x20 && g_sink[i] <= 0x7E);
        }
    }

    sink.level = SCTS_SINK_L1_SUTF8;
    init_plane(&p, cells, 1, 6, 8, true, false);
    scts_plane_set_sink(&p, &sink);
    CHECK(scts_plane_write(&p, (const uint8_t *)"A\xC3\xA9\xEF\xBC\xA1", 8) ==
          SCTS_OK);
    sink_reset();
    CHECK(scts_plane_emit_row(&p, 0, NULL, 0) == SCTS_OK);
    CHECK(g_sink_len == 1 + 2 + 3 + 2);
    CHECK(memcmp(g_sink, "A\xC3\xA9\xEF\xBC\xA1  ", g_sink_len) == 0);
    CHECK(p.lossy_substitutions == 0);

    sink.level = SCTS_SINK_L2_ROW;
    scts_plane_set_sink(&p, &sink);
    sink_reset();
    CHECK(scts_plane_emit_row(&p, 0, scratch, sizeof scratch) == SCTS_OK);
    CHECK(g_sink_len == 1 + 2 + 3 + 2);
    CHECK(memcmp(g_sink, "A\xC3\xA9\xEF\xBC\xA1  ", g_sink_len) == 0);

    /* An L2 sink without scratch cannot emit a whole row. */
    CHECK(scts_plane_emit_row(&p, 0, NULL, 0) == SCTS_E_CAPACITY);

    /* No sink bound. */
    scts_plane_set_sink(&p, NULL);
    CHECK(scts_plane_emit_row(&p, 0, scratch, sizeof scratch) ==
          SCTS_E_NO_SINK);
}

static void test_dirty_tracking(void)
{
    scts_plane_t p;
    scts_cell_t cells[16];
    scts_sink_t sink;

    sink.emit = capture_sink;
    sink.token = 0u;
    sink.level = SCTS_SINK_L1_SUTF8;

    init_plane(&p, cells, 2, 4, 8, true, false);
    CHECK(scts_plane_row_dirty(&p, 0));
    CHECK(scts_plane_row_dirty(&p, 1));
    scts_plane_clear_dirty(&p);
    CHECK(!scts_plane_row_dirty(&p, 0));
    CHECK(!scts_plane_row_dirty(&p, 1));

    scts_plane_set_sink(&p, &sink);
    CHECK(scts_plane_put_cp(&p, 'A') == SCTS_OK);
    CHECK(scts_plane_row_dirty(&p, 0));
    CHECK(!scts_plane_row_dirty(&p, 1));

    sink_reset();
    CHECK(scts_plane_flush(&p, NULL, 0) == SCTS_OK);
    CHECK(!scts_plane_row_dirty(&p, 0));
    CHECK(g_sink_len == 4);
    CHECK(memcmp(g_sink, "A   ", 4) == 0);

    CHECK(scts_plane_scroll(&p, 1) == SCTS_OK);
    CHECK(scts_plane_row_dirty(&p, 0));
}

static void test_zones(void)
{
    scts_zone_table_t z;
    const scts_zone_t *zone;

    scts_zones_init(NULL);
    scts_zones_init(&z);
    CHECK(z.count == 2);

    zone = scts_zone_find(&z, SCTS_MARK_CONT);
    CHECK(zone != NULL);
    CHECK(zone->start == SCTS_MARK_CONT && zone->end == SCTS_MARK_CONT);
    CHECK(strcmp(zone->owner, "SCTS") == 0);
    CHECK(zone->purpose != NULL);
    CHECK(!zone->reusable);

    zone = scts_zone_find(&z, SCTS_MARK_BLANK);
    CHECK(zone != NULL && zone->start == SCTS_MARK_BLANK);

    CHECK(scts_zone_is_reserved_marker(SCTS_MARK_CONT));
    CHECK(scts_zone_is_reserved_marker(SCTS_MARK_BLANK));
    CHECK(!scts_zone_is_reserved_marker(0x0041u));
    CHECK(scts_zone_find(&z, 0x0041u) == NULL);

    /* Z1: zones are disjoint. */
    CHECK(scts_zone_register(&z, 0xEF00u, 0xEF3Fu, "bootvid",
                             "boot glyphs", false) == SCTS_OK);
    zone = scts_zone_find(&z, 0xEF10u);
    CHECK(zone != NULL && strcmp(zone->owner, "bootvid") == 0);
    CHECK(scts_zone_register(&z, 0xEF20u, 0xEF40u, "other",
                             "overlap", false) == SCTS_E_ZONE);
    CHECK(scts_zone_register(&z, SCTS_MARK_CONT, SCTS_MARK_CONT, "other",
                             "reuse of a marker", true) == SCTS_E_ZONE);
    CHECK(scts_zone_register(&z, 0x20u, 0x10u, "bad", "inverted", false) ==
          SCTS_E_ZONE);
    CHECK(scts_zone_register(NULL, 0x100u, 0x1FFu, "x", "y", false) ==
          SCTS_E_NULL);

    CHECK(scts_zone_overlaps(&z, 0xEF00u, 0xEF00u));
    CHECK(!scts_zone_overlaps(&z, 0xEF40u, 0xEF5Fu));
    CHECK(!scts_zone_overlaps(&z, 0x20u, 0x10u));

    /* Z2: the block is the unit; a neighbour does not grow into it. */
    CHECK(scts_zone_register(&z, 0xEF40u, 0xEF5Fu, "owm",
                             "window manager cells", false) == SCTS_OK);
    CHECK(!scts_zone_overlaps(&z, 0xEF60u, 0xEF7Fu));

    /* The table is fixed and full registration fails closed. */
    while (z.count < (size_t)SCTS_MAX_ZONES) {
        uint32_t cp = 0xF000u + (uint32_t)z.count;
        CHECK(scts_zone_register(&z, cp, cp, "fill", "fill", true) ==
              SCTS_OK);
    }
    CHECK(z.count == (size_t)SCTS_MAX_ZONES);
    CHECK(scts_zone_register(&z, 0xF100u, 0xF100u, "over", "full", true) ==
          SCTS_E_ZONE);
}

int main(void)
{
    test_version_and_status();
    test_init_and_geometry();
    test_width_deference();
    test_combining_attachment();
    test_wide_pairs();
    test_wrap_and_scroll();
    test_tabs_and_backspace();
    test_controls_and_encoding();
    test_serialization();
    test_sink_levels();
    test_dirty_tracking();
    test_zones();

    if (g_fail) {
        printf("SCTS tests FAILED\n");
        return 1;
    }
    printf("SCTS tests passed\n");
    return 0;
}
