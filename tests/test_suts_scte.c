/* Tests for SUTS-013 - System Console Terminal Emulation (SCTE). */

#include <stdio.h>
#include <string.h>

#include "suts/suts_scte.h"

static int g_failures;

#define CHECK(cond) do {                                                     \
    if (!(cond)) {                                                           \
        printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);               \
        g_failures++;                                                        \
    }                                                                        \
} while (0)

#define PLANE_ROWS 4u
#define PLANE_COLS 10u

static scts_cell_t g_cells[PLANE_ROWS * PLANE_COLS];

static uint32_t cell_cp(const scts_plane_t *p, uint16_t row, uint16_t col)
{
    return p->cells[(size_t)row * (size_t)p->cols + (size_t)col].cp;
}

static int plane_init(scts_plane_t *p)
{
    scts_plane_options_t o;

    scts_plane_options_default(&o);
    return (int)scts_plane_init(p, g_cells, PLANE_ROWS, PLANE_COLS, &o);
}

/* Encode one codepoint as SUTF-8; returns the byte count. */
static size_t enc(uint32_t cp, uint8_t *out)
{
    if (cp <= 0x7Fu) {
        out[0] = (uint8_t)cp;
        return 1u;
    }
    if (cp <= 0x7FFu) {
        out[0] = (uint8_t)(0xC0u | (cp >> 6));
        out[1] = (uint8_t)(0x80u | (cp & 0x3Fu));
        return 2u;
    }
    if (cp <= 0xFFFFu) {
        out[0] = (uint8_t)(0xE0u | (cp >> 12));
        out[1] = (uint8_t)(0x80u | ((cp >> 6) & 0x3Fu));
        out[2] = (uint8_t)(0x80u | (cp & 0x3Fu));
        return 3u;
    }
    if (cp <= 0x1FFFFFu) {
        out[0] = (uint8_t)(0xF0u | (cp >> 18));
        out[1] = (uint8_t)(0x80u | ((cp >> 12) & 0x3Fu));
        out[2] = (uint8_t)(0x80u | ((cp >> 6) & 0x3Fu));
        out[3] = (uint8_t)(0x80u | (cp & 0x3Fu));
        return 4u;
    }
    if (cp <= 0x3FFFFFFu) {
        out[0] = (uint8_t)(0xF8u | (cp >> 24));
        out[1] = (uint8_t)(0x80u | ((cp >> 18) & 0x3Fu));
        out[2] = (uint8_t)(0x80u | ((cp >> 12) & 0x3Fu));
        out[3] = (uint8_t)(0x80u | ((cp >> 6) & 0x3Fu));
        out[4] = (uint8_t)(0x80u | (cp & 0x3Fu));
        return 5u;
    }
    out[0] = (uint8_t)(0xFCu | (cp >> 30));
    out[1] = (uint8_t)(0x80u | ((cp >> 24) & 0x3Fu));
    out[2] = (uint8_t)(0x80u | ((cp >> 18) & 0x3Fu));
    out[3] = (uint8_t)(0x80u | ((cp >> 12) & 0x3Fu));
    out[4] = (uint8_t)(0x80u | ((cp >> 6) & 0x3Fu));
    out[5] = (uint8_t)(0x80u | (cp & 0x3Fu));
    return 6u;
}

static size_t put_cp(scts_plane_t *p, uint32_t cp)
{
    uint8_t b[8];
    size_t n = enc(cp, b);
    return (size_t)scts_plane_write(p, b, n);
}

static void test_lifecycle(void)
{
    scts_plane_t p;
    scte_term_t t;
    scts_plane_t uninit;

    CHECK(strstr(suts_scte_version_string(), "SUTS-013") != NULL);
    CHECK(strcmp(suts_scte_status_name(SCTE_OK), "SCTE_OK") == 0);
    CHECK(strcmp(suts_scte_status_name(SCTE_E_INCOMPLETE),
                 "SCTE_E_INCOMPLETE") == 0);
    CHECK(strcmp(suts_scte_status_name((scte_status_t)99),
                 "SCTE_E_UNKNOWN") == 0);

    CHECK(suts_scte_init(NULL, NULL, NULL, NULL, 0u) == SCTE_E_NULL);
    CHECK(plane_init(&p) == SCTS_OK);
    CHECK(suts_scte_init(&t, NULL, NULL, NULL, 0u) == SCTE_E_NULL);

    memset(&uninit, 0, sizeof uninit);
    CHECK(suts_scte_init(&t, &uninit, NULL, NULL, 0u) == SCTE_E_PLANE);

    CHECK(suts_scte_init(&t, &p, NULL, NULL, 0u) == SCTE_OK);
    CHECK(t.cursor_visible == true);
    CHECK(t.state == SCTE_GROUND);
    CHECK(suts_scte_finish(&t) == SCTE_OK);
    CHECK(suts_scte_finish(NULL) == SCTE_E_NULL);
    CHECK(suts_scte_feed(NULL, (const uint8_t *)"a", 1u) == SCTE_E_NULL);
    CHECK(suts_scte_feed(&t, NULL, 1u) == SCTE_E_NULL);
    CHECK(suts_scte_feed(&t, NULL, 0u) == SCTE_OK);
}

static void test_plain_text(void)
{
    scts_plane_t p;
    scte_term_t t;
    uint8_t b[8];
    size_t n;

    CHECK(plane_init(&p) == SCTS_OK);
    CHECK(suts_scte_init(&t, &p, NULL, NULL, 0u) == SCTE_OK);

    CHECK(suts_scte_feed(&t, (const uint8_t *)"abc", 3u) == SCTE_OK);
    CHECK(cell_cp(&p, 0u, 0u) == 'a');
    CHECK(cell_cp(&p, 0u, 1u) == 'b');
    CHECK(cell_cp(&p, 0u, 2u) == 'c');
    CHECK(p.cursor_col == 3u);

    n = enc(0x00E9u, b);
    CHECK(suts_scte_feed(&t, b, n) == SCTE_OK);
    CHECK(cell_cp(&p, 0u, 3u) == 0x00E9u);

    n = enc(0x4E00u, b);
    CHECK(suts_scte_feed(&t, b, n) == SCTE_OK);
    CHECK(cell_cp(&p, 0u, 4u) == 0x4E00u);
}

static void test_c0_controls(void)
{
    scts_plane_t p;
    scte_term_t t;
    uint8_t ctrl[3];

    CHECK(plane_init(&p) == SCTS_OK);
    CHECK(suts_scte_init(&t, &p, NULL, NULL, 0u) == SCTE_OK);

    CHECK(suts_scte_feed(&t, (const uint8_t *)"ab", 2u) == SCTE_OK);
    CHECK(suts_scte_feed(&t, (const uint8_t *)"\r", 1u) == SCTE_OK);
    CHECK(p.cursor_row == 0u && p.cursor_col == 0u);
    CHECK(suts_scte_feed(&t, (const uint8_t *)"\n", 1u) == SCTE_OK);
    CHECK(p.cursor_row == 1u && p.cursor_col == 0u);
    CHECK(suts_scte_feed(&t, (const uint8_t *)"xyz", 3u) == SCTE_OK);
    CHECK(suts_scte_feed(&t, (const uint8_t *)"\x08", 1u) == SCTE_OK);
    CHECK(p.cursor_col == 2u);

    p.cursor_col = 0u;
    CHECK(suts_scte_feed(&t, (const uint8_t *)"\t", 1u) == SCTE_OK);
    CHECK(p.cursor_col == 8u);

    p.cursor_row = 1u;
    p.cursor_col = 5u;
    ctrl[0] = 0x0B; /* VT */
    CHECK(suts_scte_feed(&t, ctrl, 1u) == SCTE_OK);
    CHECK(p.cursor_row == 2u && p.cursor_col == 0u);
    ctrl[0] = 0x0C; /* FF */
    CHECK(suts_scte_feed(&t, ctrl, 1u) == SCTE_OK);
    CHECK(p.cursor_row == 3u && p.cursor_col == 0u);

    /* NUL and DEL are ignored; SO/SI are ignored. */
    p.cursor_row = 0u;
    p.cursor_col = 0u;
    ctrl[0] = 0x00;
    CHECK(suts_scte_feed(&t, ctrl, 1u) == SCTE_OK);
    ctrl[0] = 0x7F;
    CHECK(suts_scte_feed(&t, ctrl, 1u) == SCTE_OK);
    ctrl[0] = 0x0E;
    CHECK(suts_scte_feed(&t, ctrl, 1u) == SCTE_OK);
    CHECK(p.cursor_row == 0u && p.cursor_col == 0u);
    CHECK(t.controls >= 8u);
}

static int g_bells;
static uintptr_t g_bell_token;

static void bell_cb(uintptr_t token)
{
    g_bells++;
    g_bell_token = token;
}

static void test_bell_and_scp(void)
{
    scts_plane_t p;
    scte_term_t t;
    uint8_t b[8];
    size_t n;

    CHECK(plane_init(&p) == SCTS_OK);
    CHECK(suts_scte_init(&t, &p, NULL, bell_cb, 0xABCDu) == SCTE_OK);
    g_bells = 0;
    CHECK(suts_scte_feed(&t, (const uint8_t *)"\x07", 1u) == SCTE_OK);
    CHECK(g_bells == 1);
    CHECK(g_bell_token == 0xABCDu);
    CHECK(cell_cp(&p, 0u, 0u) == SCTS_MARK_BLANK);

    /* SCP without a callback is counted and never placed. */
    n = enc(0x110000u, b);
    CHECK(suts_scte_feed(&t, b, n) == SCTE_OK);
    CHECK(t.directives == 1u);
    CHECK(cell_cp(&p, 0u, 0u) == SCTS_MARK_BLANK);

    /* SCP with a callback is delivered. */
    {
        uintptr_t seen = 0u;
        uint32_t seen_cp = 0u;

        /* Reuse the callback slot through a small local function is not
         * possible in C89-style code, so use a file-scope hook. */
        (void)seen;
        (void)seen_cp;
        (void)n;
    }
}

static uint32_t g_scp_cp;
static int g_scp_calls;

static void scp_cb(uintptr_t token, uint32_t cp)
{
    (void)token;
    g_scp_calls++;
    g_scp_cp = cp;
}

static void test_scp_delivery(void)
{
    scts_plane_t p;
    scte_term_t t;
    uint8_t b[8];
    size_t n;

    CHECK(plane_init(&p) == SCTS_OK);
    CHECK(suts_scte_init(&t, &p, scp_cb, NULL, 0u) == SCTE_OK);
    g_scp_calls = 0;
    n = enc(0x110123u, b);
    CHECK(suts_scte_feed(&t, b, n) == SCTE_OK);
    CHECK(g_scp_calls == 1);
    CHECK(g_scp_cp == 0x110123u);
    CHECK(t.directives == 1u);
    CHECK(cell_cp(&p, 0u, 0u) == SCTS_MARK_BLANK);
}

static void test_cursor_sequences(void)
{
    scts_plane_t p;
    scte_term_t t;

    CHECK(plane_init(&p) == SCTS_OK);
    CHECK(suts_scte_init(&t, &p, NULL, NULL, 0u) == SCTE_OK);

    CHECK(suts_scte_feed(&t, (const uint8_t *)"\x1b[100;100H", 10u) == SCTE_OK);
    CHECK(p.cursor_row == PLANE_ROWS - 1u);
    CHECK(p.cursor_col == PLANE_COLS - 1u);

    CHECK(suts_scte_feed(&t, (const uint8_t *)"\x1b[100A", 6u) == SCTE_OK);
    CHECK(p.cursor_row == 0u);
    CHECK(suts_scte_feed(&t, (const uint8_t *)"\x1b[100D", 6u) == SCTE_OK);
    CHECK(p.cursor_col == 0u);

    CHECK(suts_scte_feed(&t, (const uint8_t *)"\x1b[5C", 4u) == SCTE_OK);
    CHECK(p.cursor_col == 5u);
    CHECK(suts_scte_feed(&t, (const uint8_t *)"\x1b[C", 3u) == SCTE_OK);
    CHECK(p.cursor_col == 6u);
    CHECK(suts_scte_feed(&t, (const uint8_t *)"\x1b[3G", 4u) == SCTE_OK);
    CHECK(p.cursor_col == 2u);

    CHECK(suts_scte_feed(&t, (const uint8_t *)"\x1b[2B", 4u) == SCTE_OK);
    CHECK(p.cursor_row == 2u);
    CHECK(suts_scte_feed(&t, (const uint8_t *)"\x1b[2F", 4u) == SCTE_OK);
    CHECK(p.cursor_row == 0u && p.cursor_col == 0u);
    CHECK(suts_scte_feed(&t, (const uint8_t *)"\x1b[1E", 4u) == SCTE_OK);
    CHECK(p.cursor_row == 1u && p.cursor_col == 0u);

    CHECK(suts_scte_feed(&t, (const uint8_t *)"\x1b[2;3H", 6u) == SCTE_OK);
    CHECK(p.cursor_row == 1u && p.cursor_col == 2u);
    CHECK(t.sequences == 10u);
}

static void test_erase(void)
{
    scts_plane_t p;
    scte_term_t t;
    uint16_t c;

    CHECK(plane_init(&p) == SCTS_OK);
    CHECK(suts_scte_init(&t, &p, NULL, NULL, 0u) == SCTE_OK);

    CHECK(suts_scte_feed(&t, (const uint8_t *)"abcde", 5u) == SCTE_OK);
    CHECK(suts_scte_feed(&t, (const uint8_t *)"\x1b[2D", 4u) == SCTE_OK);
    CHECK(p.cursor_col == 3u);
    CHECK(suts_scte_feed(&t, (const uint8_t *)"\x1b[K", 3u) == SCTE_OK);
    CHECK(cell_cp(&p, 0u, 2u) == 'c');
    CHECK(cell_cp(&p, 0u, 3u) == SCTS_MARK_BLANK);
    CHECK(cell_cp(&p, 0u, 4u) == SCTS_MARK_BLANK);

    CHECK(suts_scte_feed(&t, (const uint8_t *)"\x1b[1K", 4u) == SCTE_OK);
    for (c = 0u; c <= 3u; c++) {
        CHECK(cell_cp(&p, 0u, c) == SCTS_MARK_BLANK);
    }

    CHECK(suts_scte_feed(&t, (const uint8_t *)"xyz", 3u) == SCTE_OK);
    CHECK(suts_scte_feed(&t, (const uint8_t *)"\x1b[2J", 4u) == SCTE_OK);
    for (c = 0u; c < PLANE_COLS; c++) {
        CHECK(cell_cp(&p, 0u, c) == SCTS_MARK_BLANK);
    }
}

static void test_insert_delete_lines(void)
{
    scts_plane_t p;
    scte_term_t t;

    CHECK(plane_init(&p) == SCTS_OK);
    CHECK(suts_scte_init(&t, &p, NULL, NULL, 0u) == SCTE_OK);

    CHECK(suts_scte_feed(&t, (const uint8_t *)"AAA", 3u) == SCTE_OK);
    CHECK(suts_scte_feed(&t, (const uint8_t *)"\nBBB", 4u) == SCTE_OK);

    p.cursor_row = 0u;
    p.cursor_col = 0u;
    CHECK(suts_scte_feed(&t, (const uint8_t *)"\x1b[L", 3u) == SCTE_OK);
    CHECK(cell_cp(&p, 0u, 0u) == SCTS_MARK_BLANK);
    CHECK(cell_cp(&p, 1u, 0u) == 'A');
    CHECK(cell_cp(&p, 2u, 0u) == 'B');

    p.cursor_row = 0u;
    p.cursor_col = 0u;
    CHECK(suts_scte_feed(&t, (const uint8_t *)"\x1b[M", 3u) == SCTE_OK);
    CHECK(cell_cp(&p, 0u, 0u) == 'A');
    CHECK(cell_cp(&p, 1u, 0u) == 'B');
    CHECK(cell_cp(&p, 2u, 0u) == SCTS_MARK_BLANK);
}

static void test_insert_delete_cells(void)
{
    scts_plane_t p;
    scte_term_t t;

    CHECK(plane_init(&p) == SCTS_OK);
    CHECK(suts_scte_init(&t, &p, NULL, NULL, 0u) == SCTE_OK);

    CHECK(suts_scte_feed(&t, (const uint8_t *)"abcd", 4u) == SCTE_OK);
    p.cursor_row = 0u;
    p.cursor_col = 1u;
    CHECK(suts_scte_feed(&t, (const uint8_t *)"\x1b[2@", 4u) == SCTE_OK);
    CHECK(cell_cp(&p, 0u, 0u) == 'a');
    CHECK(cell_cp(&p, 0u, 1u) == SCTS_MARK_BLANK);
    CHECK(cell_cp(&p, 0u, 2u) == SCTS_MARK_BLANK);
    CHECK(cell_cp(&p, 0u, 3u) == 'b');

    p.cursor_row = 0u;
    p.cursor_col = 1u;
    CHECK(suts_scte_feed(&t, (const uint8_t *)"\x1b[2P", 4u) == SCTE_OK);
    CHECK(cell_cp(&p, 0u, 0u) == 'a');
    CHECK(cell_cp(&p, 0u, 1u) == 'b');
    CHECK(cell_cp(&p, 0u, 2u) == 'c');
    CHECK(cell_cp(&p, 0u, 3u) == 'd');
    CHECK(cell_cp(&p, 0u, 4u) == SCTS_MARK_BLANK);
}

static void test_scroll(void)
{
    scts_plane_t p;
    scte_term_t t;

    CHECK(plane_init(&p) == SCTS_OK);
    CHECK(suts_scte_init(&t, &p, NULL, NULL, 0u) == SCTE_OK);

    CHECK(suts_scte_feed(&t, (const uint8_t *)"AAA", 3u) == SCTE_OK);
    CHECK(suts_scte_feed(&t, (const uint8_t *)"\nBBB", 4u) == SCTE_OK);

    p.cursor_row = 1u;
    p.cursor_col = 3u;
    CHECK(suts_scte_feed(&t, (const uint8_t *)"\x1b[S", 3u) == SCTE_OK);
    CHECK(cell_cp(&p, 0u, 0u) == 'B');
    CHECK(cell_cp(&p, 1u, 0u) == SCTS_MARK_BLANK);
    CHECK(p.cursor_row == 1u && p.cursor_col == 3u);

    CHECK(suts_scte_feed(&t, (const uint8_t *)"\x1b[T", 3u) == SCTE_OK);
    CHECK(cell_cp(&p, 0u, 0u) == SCTS_MARK_BLANK);
    CHECK(cell_cp(&p, 1u, 0u) == 'B');

    p.cursor_row = 0u;
    p.cursor_col = 0u;
    CHECK(suts_scte_feed(&t, (const uint8_t *)"\x1bM", 2u) == SCTE_OK);
    CHECK(cell_cp(&p, 0u, 0u) == SCTS_MARK_BLANK);
    CHECK(cell_cp(&p, 2u, 0u) == 'B');
}

static void test_sgr_and_modes(void)
{
    scts_plane_t p;
    scte_term_t t;

    CHECK(plane_init(&p) == SCTS_OK);
    CHECK(suts_scte_init(&t, &p, NULL, NULL, 0u) == SCTE_OK);

    CHECK(suts_scte_feed(&t, (const uint8_t *)"\x1b[31;44m", 8u) == SCTE_OK);
    CHECK(p.fg == 1u && p.bg == 4u);
    CHECK(suts_scte_feed(&t, (const uint8_t *)"Q", 1u) == SCTE_OK);
    CHECK((p.cells[0].attrs & SCTS_ATTR_FG_MASK) == 1u);
    CHECK((p.cells[0].attrs & SCTS_ATTR_BG_MASK) ==
          ((uint16_t)4u << SCTS_ATTR_BG_SHIFT));

    CHECK(suts_scte_feed(&t, (const uint8_t *)"\x1b[91;104m", 9u) == SCTE_OK);
    CHECK(p.fg == 9u && p.bg == 12u);
    CHECK(suts_scte_feed(&t, (const uint8_t *)"\x1b[0m", 4u) == SCTE_OK);
    CHECK(p.fg == 0u && p.bg == 0u);
    CHECK(suts_scte_feed(&t, (const uint8_t *)"\x1b[38;5;9m", 9u) == SCTE_OK);
    CHECK(p.fg == 0u);

    CHECK(p.wrap == true);
    CHECK(suts_scte_feed(&t, (const uint8_t *)"\x1b[?7l", 5u) == SCTE_OK);
    CHECK(p.wrap == false);
    CHECK(suts_scte_feed(&t, (const uint8_t *)"\x1b[?7h", 5u) == SCTE_OK);
    CHECK(p.wrap == true);

    CHECK(t.cursor_visible == true);
    CHECK(suts_scte_feed(&t, (const uint8_t *)"\x1b[?25l", 6u) == SCTE_OK);
    CHECK(t.cursor_visible == false);
    CHECK(suts_scte_feed(&t, (const uint8_t *)"\x1b[?25h", 6u) == SCTE_OK);
    CHECK(t.cursor_visible == true);

    {
        uint64_t before = t.unsupported;
        CHECK(suts_scte_feed(&t, (const uint8_t *)"\x1b[?1049h", 8u) == SCTE_OK);
        CHECK(t.unsupported == before + 1u);
    }
}

static void test_osc(void)
{
    scts_plane_t p;
    scte_term_t t;
    uint8_t stream[8u + 140u];
    size_t n = 0u;
    size_t i;

    CHECK(plane_init(&p) == SCTS_OK);
    CHECK(suts_scte_init(&t, &p, NULL, NULL, 0u) == SCTE_OK);

    stream[n++] = 0x1B;
    stream[n++] = ']';
    stream[n++] = '0';
    stream[n++] = ';';
    stream[n++] = 'x';
    stream[n++] = 0x07;
    CHECK(suts_scte_feed(&t, stream, n) == SCTE_OK);
    CHECK(t.state == SCTE_GROUND);
    CHECK(t.unsupported == 0u);
    CHECK(cell_cp(&p, 0u, 0u) == SCTS_MARK_BLANK);

    /* ESC \ terminator. */
    n = 0u;
    stream[n++] = 0x1B;
    stream[n++] = ']';
    stream[n++] = '2';
    stream[n++] = 0x1B;
    stream[n++] = '\\';
    CHECK(suts_scte_feed(&t, stream, n) == SCTE_OK);
    CHECK(t.state == SCTE_GROUND);

    /* Oversized OSC is consumed and counted. */
    n = 0u;
    stream[n++] = 0x1B;
    stream[n++] = ']';
    for (i = 0u; i < 140u; i++) {
        stream[n++] = 'a';
    }
    stream[n++] = 0x07;
    CHECK(suts_scte_feed(&t, stream, n) == SCTE_OK);
    CHECK(t.unsupported == 1u);
    CHECK(t.state == SCTE_GROUND);
}

static void test_partial_sequences(void)
{
    scts_plane_t p;
    scte_term_t t;
    uint8_t b[8];
    size_t n;

    CHECK(plane_init(&p) == SCTS_OK);
    CHECK(suts_scte_init(&t, &p, NULL, NULL, 0u) == SCTE_OK);

    /* A CSI split across calls executes only at the final byte. */
    CHECK(suts_scte_feed(&t, (const uint8_t *)"\x1b[", 2u) == SCTE_OK);
    CHECK(suts_scte_feed(&t, (const uint8_t *)"2", 1u) == SCTE_OK);
    CHECK(p.cursor_col == 0u);
    CHECK(suts_scte_finish(&t) == SCTE_E_INCOMPLETE);
    CHECK(suts_scte_feed(&t, (const uint8_t *)"C", 1u) == SCTE_OK);
    CHECK(p.cursor_col == 2u);
    CHECK(suts_scte_finish(&t) == SCTE_OK);

    /* A UTF-8 sequence split across calls. */
    n = enc(0x00E9u, b);
    CHECK(suts_scte_feed(&t, b, 1u) == SCTE_OK);
    CHECK(suts_scte_finish(&t) == SCTE_E_INCOMPLETE);
    CHECK(suts_scte_feed(&t, b + 1u, n - 1u) == SCTE_OK);
    CHECK(cell_cp(&p, 0u, 2u) == 0x00E9u);
    CHECK(suts_scte_finish(&t) == SCTE_OK);

    /* A lone ESC is incomplete until finished. */
    CHECK(suts_scte_feed(&t, (const uint8_t *)"\x1b", 1u) == SCTE_OK);
    CHECK(suts_scte_finish(&t) == SCTE_E_INCOMPLETE);
    suts_scte_reset(&t);
    CHECK(t.state == SCTE_GROUND);
    CHECK(suts_scte_finish(&t) == SCTE_OK);
}

static void test_encoding_errors(void)
{
    scts_plane_t p;
    scte_term_t t;
    uint8_t b[8];
    size_t n;

    CHECK(plane_init(&p) == SCTS_OK);
    CHECK(suts_scte_init(&t, &p, NULL, NULL, 0u) == SCTE_OK);

    /* Lead byte followed by a non-continuation. */
    b[0] = 0xE0u;
    b[1] = 'A';
    CHECK(suts_scte_feed(&t, b, 2u) == SCTE_E_ENCODING);
    CHECK(t.encoding_errors == 1u);
    CHECK(cell_cp(&p, 0u, 0u) == SCTS_MARK_BLANK);

    /* Recovery: the next valid byte is placed. */
    CHECK(suts_scte_feed(&t, (const uint8_t *)"B", 1u) == SCTE_OK);
    CHECK(cell_cp(&p, 0u, 0u) == 'B');

    /* A stray continuation byte is an error too. */
    b[0] = 0x80u;
    CHECK(suts_scte_feed(&t, b, 1u) == SCTE_E_ENCODING);
    CHECK(t.encoding_errors == 2u);

    /* Overlong C0 is rejected one byte at a time. */
    b[0] = 0xC0u;
    CHECK(suts_scte_feed(&t, b, 1u) == SCTE_E_ENCODING);
    CHECK(t.encoding_errors == 3u);
    b[0] = 0x80u;
    CHECK(suts_scte_feed(&t, b, 1u) == SCTE_E_ENCODING);
    CHECK(t.encoding_errors == 4u);

    /* Surrogate and trap are consumed but never placed. */
    n = enc(0xD800u, b);
    CHECK(suts_scte_feed(&t, b, n) == SCTE_OK);
    CHECK(t.unsupported == 1u);
    n = enc(0x7FFFFFF0u, b);
    CHECK(suts_scte_feed(&t, b, n) == SCTE_OK);
    CHECK(t.unsupported == 2u);
    n = enc(0x7FFFFFFFu, b);
    CHECK(suts_scte_feed(&t, b, n) == SCTE_OK);
    CHECK(t.unsupported == 3u);

    CHECK(suts_scte_finish(&t) == SCTE_OK);
}

static void test_unsupported_sequence(void)
{
    scts_plane_t p;
    scte_term_t t;

    CHECK(plane_init(&p) == SCTS_OK);
    CHECK(suts_scte_init(&t, &p, NULL, NULL, 0u) == SCTE_OK);

    CHECK(suts_scte_feed(&t, (const uint8_t *)"ok", 2u) == SCTE_OK);
    CHECK(suts_scte_feed(&t, (const uint8_t *)"\x1b[9z", 4u) == SCTE_OK);
    CHECK(t.unsupported == 1u);
    CHECK(t.sequences == 0u);
    CHECK(cell_cp(&p, 0u, 0u) == 'o');
    CHECK(cell_cp(&p, 0u, 1u) == 'k');
    CHECK(p.cursor_col == 2u);

    /* An unknown ESC sequence is consumed as a whole. */
    CHECK(suts_scte_feed(&t, (const uint8_t *)"\x1bQ", 2u) == SCTE_OK);
    CHECK(t.unsupported == 2u);
    CHECK(suts_scte_feed(&t, (const uint8_t *)"!", 1u) == SCTE_OK);
    CHECK(cell_cp(&p, 0u, 2u) == '!');
}

static void test_determinism(void)
{
    static scts_cell_t cells_a[PLANE_ROWS * PLANE_COLS];
    static scts_cell_t cells_b[PLANE_ROWS * PLANE_COLS];
    scts_plane_t a;
    scts_plane_t b;
    scte_term_t ta;
    scte_term_t tb;
    const uint8_t stream[] = "\x1b[31mabc\r\n\x1b[2Cde\x1b[1Kxyz";

    CHECK(scts_plane_init(&a, cells_a, PLANE_ROWS, PLANE_COLS, NULL) == SCTS_OK);
    CHECK(scts_plane_init(&b, cells_b, PLANE_ROWS, PLANE_COLS, NULL) == SCTS_OK);
    CHECK(suts_scte_init(&ta, &a, NULL, NULL, 0u) == SCTE_OK);
    CHECK(suts_scte_init(&tb, &b, NULL, NULL, 0u) == SCTE_OK);

    CHECK(suts_scte_feed(&ta, stream, sizeof stream - 1u) == SCTE_OK);
    CHECK(suts_scte_feed(&tb, stream, sizeof stream - 1u) == SCTE_OK);
    CHECK(memcmp(cells_a, cells_b, sizeof cells_a) == 0);
    CHECK(a.cursor_row == b.cursor_row && a.cursor_col == b.cursor_col);
    CHECK(ta.sequences == tb.sequences);
    CHECK(ta.unsupported == tb.unsupported);
    CHECK(ta.controls == tb.controls);
}

int main(void)
{
    test_lifecycle();
    test_plain_text();
    test_c0_controls();
    test_bell_and_scp();
    test_scp_delivery();
    test_cursor_sequences();
    test_erase();
    test_insert_delete_lines();
    test_insert_delete_cells();
    test_scroll();
    test_sgr_and_modes();
    test_osc();
    test_partial_sequences();
    test_encoding_errors();
    test_unsupported_sequence();
    test_determinism();

    if (g_failures != 0) {
        printf("%d SCTE test(s) failed\n", g_failures);
        return 1;
    }
    printf("[PASS] SUTS-013 SCTE tests\n");
    return 0;
}
