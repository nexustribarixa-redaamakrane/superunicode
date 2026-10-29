/* SUTS-013 - System Console Terminal Emulation (SCTE)
 *
 * Reference implementation of the optional terminal contract. The parser
 * consumes SUTF-8 and the ASCII control subset, drives an SCTS plane, and
 * routes SCP codepoints to a callback. It allocates nothing and never keeps a
 * second cell array.
 *
 * See docs/suts/SUTS-013-scte.md for the normative specification.
 */

#include "suts/suts_scte.h"

/* -- Small helpers ----------------------------------------------------- */

static scts_cell_t *scte_cell(const scts_plane_t *p, uint16_t row, uint16_t col)
{
    return &p->cells[(size_t)row * (size_t)p->cols + (size_t)col];
}

static uint16_t scte_blank_attrs(const scts_plane_t *p)
{
    return (uint16_t)((p->fg & SCTS_ATTR_FG_MASK) |
                      ((p->bg & 0x0Fu) << SCTS_ATTR_BG_SHIFT));
}

static void scte_drop_cluster(scts_plane_t *p, uint16_t row, uint16_t col)
{
    uint16_t i;

    for (i = 0u; i < p->cluster_count; i++) {
        if (p->clusters[i].row == row && p->clusters[i].col == col) {
            uint16_t j;
            for (j = (uint16_t)(i + 1u); j < p->cluster_count; j++) {
                p->clusters[j - 1u] = p->clusters[j];
            }
            p->cluster_count--;
            return;
        }
    }
}

static void scte_blank_raw(scts_plane_t *p, uint16_t row, uint16_t col)
{
    scts_cell_t *c = scte_cell(p, row, col);
    c->cp = SCTS_MARK_BLANK;
    c->attrs = scte_blank_attrs(p);
}

/* Blank one cell and keep a wide pair whole. */
static void scte_blank_cell(scts_plane_t *p, uint16_t row, uint16_t col)
{
    scts_cell_t *c = scte_cell(p, row, col);
    uint16_t attrs = c->attrs;
    uint16_t c2;

    scte_drop_cluster(p, row, col);

    if ((attrs & SCTS_ATTR_WIDE_TAIL) != 0u && col > 0u) {
        c2 = (uint16_t)(col - 1u);
        scte_drop_cluster(p, row, c2);
        scte_blank_raw(p, row, c2);
    }
    if ((attrs & SCTS_ATTR_WIDE_LEAD) != 0u &&
        (uint32_t)col + 1u < (uint32_t)p->cols) {
        c2 = (uint16_t)(col + 1u);
        scte_drop_cluster(p, row, c2);
        scte_blank_raw(p, row, c2);
    }

    scte_blank_raw(p, row, col);
}

static void scte_blank_row(scts_plane_t *p, uint16_t row)
{
    uint16_t col;

    for (col = 0u; col < p->cols; col++) {
        scte_blank_raw(p, row, col);
    }
}

static void scte_drop_row_clusters(scts_plane_t *p, uint16_t row)
{
    uint16_t i;
    uint16_t out = 0u;

    for (i = 0u; i < p->cluster_count; i++) {
        if (p->clusters[i].row == row) {
            continue;
        }
        p->clusters[out++] = p->clusters[i];
    }
    p->cluster_count = out;
}

/* Shift rows [top, rows) down by n, then blank the n rows opened at top. */
static void scte_rows_down(scts_plane_t *p, uint16_t top, uint16_t n)
{
    uint16_t rows = p->rows;
    uint16_t cols = p->cols;
    uint16_t r;
    uint16_t c;
    uint16_t i;
    uint16_t out;

    if (top >= rows || n == 0u) {
        return;
    }
    if ((uint32_t)n > (uint32_t)rows - (uint32_t)top) {
        n = (uint16_t)(rows - top);
    }

    for (r = rows; r > (uint16_t)(top + n); ) {
        r--;
        for (c = 0u; c < cols; c++) {
            *scte_cell(p, r, c) = *scte_cell(p, (uint16_t)(r - n), c);
        }
    }
    for (r = top; r < (uint16_t)(top + n); r++) {
        scte_blank_row(p, r);
    }

    out = 0u;
    for (i = 0u; i < p->cluster_count; i++) {
        scts_cluster_t rec = p->clusters[i];
        if (rec.row >= top) {
            if ((uint32_t)rec.row + (uint32_t)n < (uint32_t)rows) {
                rec.row = (uint16_t)(rec.row + n);
                p->clusters[out++] = rec;
            }
        } else {
            p->clusters[out++] = rec;
        }
    }
    p->cluster_count = out;

    p->dirty_all = true;
    p->dirty = 0u;
}

/* Shift rows [top, rows) up by n, then blank the n rows opened at the bottom. */
static void scte_rows_up(scts_plane_t *p, uint16_t top, uint16_t n)
{
    uint16_t rows = p->rows;
    uint16_t cols = p->cols;
    uint16_t r;
    uint16_t c;
    uint16_t i;
    uint16_t out;

    if (top >= rows || n == 0u) {
        return;
    }
    if ((uint32_t)n > (uint32_t)rows - (uint32_t)top) {
        n = (uint16_t)(rows - top);
    }

    for (r = top; (uint32_t)r + (uint32_t)n < (uint32_t)rows; r++) {
        for (c = 0u; c < cols; c++) {
            *scte_cell(p, r, c) = *scte_cell(p, (uint16_t)(r + n), c);
        }
    }
    for (r = (uint16_t)(rows - n); r < rows; r++) {
        scte_blank_row(p, r);
    }

    out = 0u;
    for (i = 0u; i < p->cluster_count; i++) {
        scts_cluster_t rec = p->clusters[i];
        if (rec.row >= top) {
            if (rec.row >= (uint16_t)(top + n)) {
                rec.row = (uint16_t)(rec.row - n);
                p->clusters[out++] = rec;
            }
        } else {
            p->clusters[out++] = rec;
        }
    }
    p->cluster_count = out;

    p->dirty_all = true;
    p->dirty = 0u;
}

static void scte_insert_cells(scts_plane_t *p, uint16_t row, uint16_t col,
                              uint16_t n)
{
    uint16_t cols = p->cols;
    uint16_t c;

    if (row >= p->rows || col >= cols || n == 0u) {
        return;
    }
    if ((uint32_t)n > (uint32_t)cols - (uint32_t)col) {
        n = (uint16_t)(cols - col);
    }

    scte_drop_row_clusters(p, row);

    for (c = cols; c > (uint16_t)(col + n); ) {
        c--;
        *scte_cell(p, row, c) = *scte_cell(p, row, (uint16_t)(c - n));
    }
    for (c = col; c < (uint16_t)(col + n); c++) {
        scte_blank_raw(p, row, c);
    }
    p->dirty_all = true;
}

static void scte_delete_cells(scts_plane_t *p, uint16_t row, uint16_t col,
                              uint16_t n)
{
    uint16_t cols = p->cols;
    uint16_t c;

    if (row >= p->rows || col >= cols || n == 0u) {
        return;
    }
    if ((uint32_t)n > (uint32_t)cols - (uint32_t)col) {
        n = (uint16_t)(cols - col);
    }

    scte_drop_row_clusters(p, row);

    for (c = col; (uint32_t)c + (uint32_t)n < (uint32_t)cols; c++) {
        *scte_cell(p, row, c) = *scte_cell(p, row, (uint16_t)(c + n));
    }
    for (c = (uint16_t)(cols - n); c < cols; c++) {
        scte_blank_raw(p, row, c);
    }
    p->dirty_all = true;
}

/* -- Parser state reset ------------------------------------------------ */

static void scte_csi_reset(scte_term_t *t)
{
    t->param_count = 0u;
    t->prefix = 0u;
    t->intermediate = 0u;
    t->have_param = false;
    t->overflow = false;
}

static void scte_parser_reset(scte_term_t *t)
{
    scte_csi_reset(t);
    t->carry_len = 0u;
    t->osc_len = 0u;
    t->osc_overflow = false;
    t->state = SCTE_GROUND;
}

/* -- Parameter handling ------------------------------------------------ */

static void scte_param_digit(scte_term_t *t, uint8_t digit)
{
    uint32_t v;

    if (t->overflow) {
        return;
    }
    if (t->param_count >= (uint8_t)SCTE_MAX_PARAMS) {
        t->overflow = true;
        return;
    }
    if (!t->have_param) {
        t->params[t->param_count] = 0u;
        t->have_param = true;
    }
    v = t->params[t->param_count];
    if (v > (SCTE_MAX_PARAM_VALUE - (uint32_t)digit) / 10u) {
        v = SCTE_MAX_PARAM_VALUE;
    } else {
        v = v * 10u + (uint32_t)digit;
    }
    t->params[t->param_count] = v;
}

static void scte_param_push(scte_term_t *t)
{
    if (t->overflow) {
        return;
    }
    if (t->param_count >= (uint8_t)SCTE_MAX_PARAMS) {
        t->overflow = true;
        return;
    }
    t->params[t->param_count] = t->have_param
                              ? t->params[t->param_count]
                              : SCTE_PARAM_MISSING;
    t->param_count++;
    t->have_param = false;
}

static uint32_t scte_param(const scte_term_t *t, uint8_t index,
                           uint32_t def)
{
    if (index >= t->param_count) {
        return def;
    }
    if (t->params[index] == SCTE_PARAM_MISSING) {
        return def;
    }
    return t->params[index] < SCTE_MAX_PARAM_VALUE
         ? t->params[index] : SCTE_MAX_PARAM_VALUE;
}

/* -- C0 controls (section 5) ------------------------------------------- */

static void scte_control(scte_term_t *t, uint8_t b)
{
    scts_plane_t *p = t->plane;

    t->controls++;

    switch (b) {
    case 0x00u:
        break;
    case 0x07u:
        if (t->bell != NULL) {
            t->bell(t->token);
        }
        break;
    case 0x08u:
        (void)scts_plane_backspace(p);
        break;
    case 0x09u:
        (void)scts_plane_tab(p);
        break;
    case 0x0Au:
    case 0x0Bu:
    case 0x0Cu:
        (void)scts_plane_newline(p);
        break;
    case 0x0Du:
        (void)scts_plane_carriage_return(p);
        break;
    case 0x0Eu:
    case 0x0Fu:
        break;
    case 0x1Bu:
        t->state = SCTE_ESC;
        break;
    case 0x7Fu:
        break;
    default:
        break;
    }
}

/* -- CSI execution (section 6.3) --------------------------------------- */

static uint16_t scte_clamp_row(const scts_plane_t *p, uint32_t row)
{
    if (row >= (uint32_t)p->rows) {
        return (uint16_t)(p->rows - 1u);
    }
    return (uint16_t)row;
}

static uint16_t scte_clamp_col(const scts_plane_t *p, uint32_t col)
{
    if (col >= (uint32_t)p->cols) {
        return (uint16_t)(p->cols - 1u);
    }
    return (uint16_t)col;
}

static void scte_erase_display(scte_term_t *t, uint32_t mode)
{
    scts_plane_t *p = t->plane;
    uint16_t r;
    uint16_t c;

    if (mode == 0u) {
        for (c = p->cursor_col; c < p->cols; c++) {
            scte_blank_cell(p, p->cursor_row, c);
        }
        for (r = (uint16_t)(p->cursor_row + 1u); r < p->rows; r++) {
            for (c = 0u; c < p->cols; c++) {
                scte_blank_cell(p, r, c);
            }
        }
    } else if (mode == 1u) {
        for (r = 0u; r < p->cursor_row; r++) {
            for (c = 0u; c < p->cols; c++) {
                scte_blank_cell(p, r, c);
            }
        }
        for (c = 0u; c <= p->cursor_col; c++) {
            scte_blank_cell(p, p->cursor_row, c);
        }
    } else if (mode == 2u || mode == 3u) {
        for (r = 0u; r < p->rows; r++) {
            for (c = 0u; c < p->cols; c++) {
                scte_blank_cell(p, r, c);
            }
        }
    }
    p->dirty_all = true;
}

static void scte_erase_line(scte_term_t *t, uint32_t mode)
{
    scts_plane_t *p = t->plane;
    uint16_t c;

    if (mode == 0u) {
        for (c = p->cursor_col; c < p->cols; c++) {
            scte_blank_cell(p, p->cursor_row, c);
        }
    } else if (mode == 1u) {
        for (c = 0u; c <= p->cursor_col; c++) {
            scte_blank_cell(p, p->cursor_row, c);
        }
    } else if (mode == 2u) {
        for (c = 0u; c < p->cols; c++) {
            scte_blank_cell(p, p->cursor_row, c);
        }
    }
    p->dirty_all = true;
}

static void scte_sgr(scte_term_t *t)
{
    scts_plane_t *p = t->plane;
    uint8_t i;

    if (t->param_count == 0u) {
        p->fg = 0u;
        p->bg = 0u;
        return;
    }

    for (i = 0u; i < t->param_count; i++) {
        uint32_t v = scte_param(t, i, 0u);

        if (v == 0u) {
            p->fg = 0u;
            p->bg = 0u;
        } else if (v >= 30u && v <= 37u) {
            p->fg = (uint16_t)(v - 30u);
        } else if (v == 39u) {
            p->fg = 0u;
        } else if (v >= 40u && v <= 47u) {
            p->bg = (uint16_t)(v - 40u);
        } else if (v == 49u) {
            p->bg = 0u;
        } else if (v >= 90u && v <= 97u) {
            p->fg = (uint16_t)(v - 90u + 8u);
        } else if (v >= 100u && v <= 107u) {
            p->bg = (uint16_t)(v - 100u + 8u);
        }
    }
}

static void scte_mode(scte_term_t *t, bool set)
{
    scts_plane_t *p = t->plane;
    uint8_t i;

    if (t->prefix != (uint8_t)'?') {
        t->unsupported++;
        return;
    }

    for (i = 0u; i < t->param_count; i++) {
        uint32_t v = scte_param(t, i, 0u);

        if (v == 7u) {
            p->wrap = set;
        } else if (v == 25u) {
            t->cursor_visible = set;
        } else {
            t->unsupported++;
        }
    }
    if (t->param_count == 0u) {
        t->unsupported++;
    }
}

static void scte_execute_csi(scte_term_t *t, uint8_t final)
{
    scts_plane_t *p = t->plane;

    if (t->prefix != 0u) {
        if (final == (uint8_t)'h') {
            scte_mode(t, true);
            return;
        }
        if (final == (uint8_t)'l') {
            scte_mode(t, false);
            return;
        }
        t->unsupported++;
        return;
    }

    switch (final) {
    case 'A': {
        uint32_t n = scte_param(t, 0u, 1u);
        p->cursor_row = (uint16_t)(n >= (uint32_t)p->cursor_row
                                 ? 0u
                                 : (uint32_t)p->cursor_row - n);
        break;
    }
    case 'B': {
        uint32_t n = scte_param(t, 0u, 1u);
        p->cursor_row = scte_clamp_row(p, (uint32_t)p->cursor_row + n);
        break;
    }
    case 'C': {
        uint32_t n = scte_param(t, 0u, 1u);
        p->cursor_col = scte_clamp_col(p, (uint32_t)p->cursor_col + n);
        break;
    }
    case 'D': {
        uint32_t n = scte_param(t, 0u, 1u);
        p->cursor_col = (uint16_t)(n >= (uint32_t)p->cursor_col
                                 ? 0u
                                 : (uint32_t)p->cursor_col - n);
        break;
    }
    case 'E': {
        uint32_t n = scte_param(t, 0u, 1u);
        p->cursor_row = scte_clamp_row(p, (uint32_t)p->cursor_row + n);
        p->cursor_col = 0u;
        break;
    }
    case 'F': {
        uint32_t n = scte_param(t, 0u, 1u);
        p->cursor_row = (uint16_t)(n >= (uint32_t)p->cursor_row
                                 ? 0u
                                 : (uint32_t)p->cursor_row - n);
        p->cursor_col = 0u;
        break;
    }
    case 'G': {
        uint32_t n = scte_param(t, 0u, 1u);
        p->cursor_col = scte_clamp_col(p, n == 0u ? 0u : n - 1u);
        break;
    }
    case 'H':
    case 'f': {
        uint32_t row = scte_param(t, 0u, 1u);
        uint32_t col = scte_param(t, 1u, 1u);
        p->cursor_row = scte_clamp_row(p, row == 0u ? 0u : row - 1u);
        p->cursor_col = scte_clamp_col(p, col == 0u ? 0u : col - 1u);
        break;
    }
    case 'J':
        scte_erase_display(t, scte_param(t, 0u, 0u));
        break;
    case 'K':
        scte_erase_line(t, scte_param(t, 0u, 0u));
        break;
    case 'L': {
        uint32_t n = scte_param(t, 0u, 1u);
        scte_rows_down(p, p->cursor_row, (uint16_t)n);
        break;
    }
    case 'M': {
        uint32_t n = scte_param(t, 0u, 1u);
        scte_rows_up(p, p->cursor_row, (uint16_t)n);
        break;
    }
    case 'P': {
        uint32_t n = scte_param(t, 0u, 1u);
        scte_delete_cells(p, p->cursor_row, p->cursor_col, (uint16_t)n);
        break;
    }
    case '@': {
        uint32_t n = scte_param(t, 0u, 1u);
        scte_insert_cells(p, p->cursor_row, p->cursor_col, (uint16_t)n);
        break;
    }
    case 'S': {
        uint32_t n = scte_param(t, 0u, 1u);
        uint16_t row = p->cursor_row;
        uint16_t col = p->cursor_col;
        (void)scts_plane_scroll(p, (uint16_t)n);
        p->cursor_row = row;
        p->cursor_col = col;
        break;
    }
    case 'T': {
        uint32_t n = scte_param(t, 0u, 1u);
        scte_rows_down(p, 0u, (uint16_t)n);
        break;
    }
    case 'm':
        scte_sgr(t);
        break;
    default:
        t->unsupported++;
        return;
    }

    t->sequences++;
}

/* -- ESC dispatch (section 6.1) ---------------------------------------- */

static void scte_esc_dispatch(scte_term_t *t, uint8_t b)
{
    scts_plane_t *p = t->plane;

    switch (b) {
    case '[':
        scte_csi_reset(t);
        t->state = SCTE_CSI_ENTRY;
        return;
    case ']':
        t->osc_len = 0u;
        t->osc_overflow = false;
        t->state = SCTE_OSC;
        return;
    case '7':
        t->saved_row = p->cursor_row;
        t->saved_col = p->cursor_col;
        t->state = SCTE_GROUND;
        return;
    case '8':
        p->cursor_row = scte_clamp_row(p, t->saved_row);
        p->cursor_col = scte_clamp_col(p, t->saved_col);
        t->state = SCTE_GROUND;
        return;
    case 'D':
        (void)scts_plane_newline(p);
        t->state = SCTE_GROUND;
        return;
    case 'E':
        (void)scts_plane_carriage_return(p);
        (void)scts_plane_newline(p);
        t->state = SCTE_GROUND;
        return;
    case 'M':
        if (p->cursor_row > 0u) {
            p->cursor_row--;
        } else {
            scte_rows_down(p, 0u, 1u);
        }
        t->state = SCTE_GROUND;
        return;
    case 'c':
        (void)scts_plane_clear(p);
        scte_parser_reset(t);
        return;
    case '\\':
        t->state = SCTE_GROUND;
        return;
    case 0x1Bu:
        t->state = SCTE_ESC;
        return;
    default:
        if (b < 0x20u || b == 0x7Fu) {
            scte_control(t, b);
            return;
        }
        t->unsupported++;
        t->state = SCTE_GROUND;
        return;
    }
}

/* -- Byte-stream state machine ----------------------------------------- */

static bool scte_decode(const uint8_t *c, uint8_t n, uint32_t *cp)
{
    uint32_t v;
    uint8_t i;

    if (n == 2u) {
        v = (uint32_t)(c[0] & 0x1Fu);
    } else if (n == 3u) {
        v = (uint32_t)(c[0] & 0x0Fu);
    } else if (n == 4u) {
        v = (uint32_t)(c[0] & 0x07u);
    } else if (n == 5u) {
        v = (uint32_t)(c[0] & 0x03u);
    } else {
        v = (uint32_t)(c[0] & 0x01u);
    }

    for (i = 1u; i < n; i++) {
        if ((c[i] & 0xC0u) != 0x80u) {
            return false;
        }
        v = (v << 6) | (uint32_t)(c[i] & 0x3Fu);
    }

    if (n == 2u && v < 0x80u)        return false;
    if (n == 3u && v < 0x800u)       return false;
    if (n == 4u && v < 0x10000u)     return false;
    if (n == 5u && v < 0x200000u)    return false;
    if (n == 6u && v < 0x4000000u)   return false;

    *cp = v;
    return true;
}

static scte_status_t scte_place_cp(scte_term_t *t, uint32_t cp)
{
    scts_status_t st;

    if (cp < 0x20u || cp == 0x7Fu) {
        scte_control(t, (uint8_t)cp);
        return SCTE_OK;
    }
    if (cp >= 0x00110000u && cp <= 0x0011FFFFu) {
        if (t->scp != NULL) {
            t->scp(t->token, cp);
        }
        t->directives++;
        return SCTE_OK;
    }
    if (cp >= 0x0000D800u && cp <= 0x0000DFFFu) {
        t->unsupported++;
        return SCTE_OK;
    }
    if (cp >= 0x7FFFFFF0u || cp > 0x7FFFFFEFu) {
        t->unsupported++;
        return SCTE_OK;
    }

    st = scts_plane_put_cp(t->plane, cp);
    if (st != SCTS_OK) {
        t->unsupported++;
    }
    return SCTE_OK;
}

static scte_status_t scte_ground_byte(scte_term_t *t, uint8_t b)
{
    uint8_t expected;
    uint32_t cp;

    if (t->carry_len == 0u) {
        if (b == 0x1Bu) {
            t->controls++;
            t->state = SCTE_ESC;
            return SCTE_OK;
        }
        if (b < 0x20u || b == 0x7Fu) {
            scte_control(t, b);
            return SCTE_OK;
        }
        if (b < 0x80u) {
            return scte_place_cp(t, (uint32_t)b);
        }
        if (b >= 0xC2u && b <= 0xDFu) {
            expected = 2u;
        } else if (b >= 0xE0u && b <= 0xEFu) {
            expected = 3u;
        } else if (b >= 0xF0u && b <= 0xF7u) {
            expected = 4u;
        } else if (b >= 0xF8u && b <= 0xFBu) {
            expected = 5u;
        } else if (b >= 0xFCu && b <= 0xFDu) {
            expected = 6u;
        } else {
            t->encoding_errors++;
            return SCTE_E_ENCODING;
        }
        t->carry[0] = b;
        t->carry_len = 1u;
        (void)expected;
        return SCTE_OK;
    }

    if (b < 0x80u || b > 0xBFu) {
        t->carry_len = 0u;
        t->encoding_errors++;
        return SCTE_E_ENCODING;
    }

    t->carry[t->carry_len++] = b;

    if (t->carry[0] >= 0xC2u && t->carry[0] <= 0xDFu) {
        expected = 2u;
    } else if (t->carry[0] >= 0xE0u && t->carry[0] <= 0xEFu) {
        expected = 3u;
    } else if (t->carry[0] >= 0xF0u && t->carry[0] <= 0xF7u) {
        expected = 4u;
    } else if (t->carry[0] >= 0xF8u && t->carry[0] <= 0xFBu) {
        expected = 5u;
    } else {
        expected = 6u;
    }

    if (t->carry_len < expected) {
        return SCTE_OK;
    }

    if (!scte_decode(t->carry, t->carry_len, &cp)) {
        t->carry_len = 0u;
        t->encoding_errors++;
        return SCTE_E_ENCODING;
    }
    t->carry_len = 0u;
    return scte_place_cp(t, cp);
}

static void scte_csi_byte(scte_term_t *t, uint8_t b)
{
    if (b < 0x20u || b == 0x7Fu) {
        scte_control(t, b);
        return;
    }

    if (b >= 0x30u && b <= 0x3Fu) {
        if (b == (uint8_t)'?' || b == (uint8_t)'>') {
            if (t->prefix == 0u && t->param_count == 0u &&
                t->intermediate == 0u && !t->have_param) {
                t->prefix = b;
            } else {
                t->overflow = true;
            }
        } else if (b >= (uint8_t)'0' && b <= (uint8_t)'9') {
            scte_param_digit(t, (uint8_t)(b - (uint8_t)'0'));
            if (t->state == SCTE_CSI_ENTRY) {
                t->state = SCTE_CSI_PARAM;
            }
        } else if (b == (uint8_t)';' || b == (uint8_t)':') {
            scte_param_push(t);
            if (t->state == SCTE_CSI_ENTRY) {
                t->state = SCTE_CSI_PARAM;
            }
        } else {
            t->overflow = true;
        }
        return;
    }

    if (b >= 0x20u && b <= 0x2Fu) {
        if (b == (uint8_t)'!' && t->prefix == 0u && t->intermediate == 0u &&
            t->param_count == 0u && !t->have_param) {
            t->prefix = b;
            t->state = SCTE_CSI_PARAM;
            return;
        }
        if (t->intermediate != 0u) {
            t->overflow = true;
        } else {
            t->intermediate = b;
        }
        t->state = SCTE_CSI_INTERMEDIATE;
        return;
    }

    if (b >= 0x40u && b <= 0x7Eu) {
        if (!t->overflow && (t->have_param || t->param_count > 0u)) {
            scte_param_push(t);
        }
        if (t->overflow) {
            t->unsupported++;
        } else {
            scte_execute_csi(t, b);
        }
        scte_csi_reset(t);
        t->state = SCTE_GROUND;
        return;
    }

    t->unsupported++;
    scte_csi_reset(t);
    t->state = SCTE_GROUND;
}

static void scte_osc_byte(scte_term_t *t, uint8_t b)
{
    if (b == 0x07u) {
        if (t->osc_overflow) {
            t->unsupported++;
        }
        t->osc_len = 0u;
        t->osc_overflow = false;
        t->state = SCTE_GROUND;
        return;
    }
    if (b == 0x1Bu) {
        t->state = SCTE_OSC_ESC;
        return;
    }
    if (t->osc_len < (uint16_t)SCTE_MAX_OSC) {
        t->osc_len++;
    } else {
        t->osc_overflow = true;
    }
}

static void scte_osc_esc_byte(scte_term_t *t, uint8_t b)
{
    if (b == (uint8_t)'\\') {
        if (t->osc_overflow) {
            t->unsupported++;
        }
        t->osc_len = 0u;
        t->osc_overflow = false;
        t->state = SCTE_GROUND;
        return;
    }
    t->state = SCTE_OSC;
    scte_osc_byte(t, b);
}

/* -- Public API -------------------------------------------------------- */

const char *suts_scte_version_string(void)
{
    return "SUTS-013 SCTE 1.0.0";
}

const char *suts_scte_status_name(scte_status_t st)
{
    switch (st) {
    case SCTE_OK:           return "SCTE_OK";
    case SCTE_E_NULL:       return "SCTE_E_NULL";
    case SCTE_E_PLANE:      return "SCTE_E_PLANE";
    case SCTE_E_ENCODING:   return "SCTE_E_ENCODING";
    case SCTE_E_INCOMPLETE: return "SCTE_E_INCOMPLETE";
    case SCTE_E_INTERNAL:   return "SCTE_E_INTERNAL";
    default:                return "SCTE_E_UNKNOWN";
    }
}

scte_status_t suts_scte_init(scte_term_t *term, scts_plane_t *plane,
                             scte_scp_fn scp, scte_bell_fn bell,
                             uintptr_t token)
{
    uint8_t i;

    if (term == NULL) {
        return SCTE_E_NULL;
    }
    if (plane == NULL) {
        return SCTE_E_NULL;
    }
    if (!plane->initialized) {
        return SCTE_E_PLANE;
    }

    term->plane = plane;
    term->state = SCTE_GROUND;
    term->carry_len = 0u;
    for (i = 0u; i < (uint8_t)SCTE_CARRY_BYTES; i++) {
        term->carry[i] = 0u;
    }
    term->param_count = 0u;
    term->prefix = 0u;
    term->intermediate = 0u;
    term->have_param = false;
    term->overflow = false;
    term->saved_row = 0u;
    term->saved_col = 0u;
    term->cursor_visible = true;
    term->origin_mode = false;
    term->osc_len = 0u;
    term->osc_overflow = false;
    term->controls = 0u;
    term->sequences = 0u;
    term->unsupported = 0u;
    term->directives = 0u;
    term->encoding_errors = 0u;
    term->scp = scp;
    term->bell = bell;
    term->token = token;
    return SCTE_OK;
}

scte_status_t suts_scte_feed(scte_term_t *term, const uint8_t *bytes,
                             size_t len)
{
    size_t i;
    scte_status_t st;

    if (term == NULL) {
        return SCTE_E_NULL;
    }
    if (term->plane == NULL) {
        return SCTE_E_NULL;
    }
    if (!term->plane->initialized) {
        return SCTE_E_PLANE;
    }
    if (bytes == NULL) {
        return len == 0u ? SCTE_OK : SCTE_E_NULL;
    }

    for (i = 0u; i < len; i++) {
        uint8_t b = bytes[i];

        switch (term->state) {
        case SCTE_GROUND:
            st = scte_ground_byte(term, b);
            if (st != SCTE_OK) {
                return st;
            }
            break;
        case SCTE_ESC:
            scte_esc_dispatch(term, b);
            break;
        case SCTE_CSI_ENTRY:
        case SCTE_CSI_PARAM:
        case SCTE_CSI_INTERMEDIATE:
            scte_csi_byte(term, b);
            break;
        case SCTE_OSC:
            scte_osc_byte(term, b);
            break;
        case SCTE_OSC_ESC:
            scte_osc_esc_byte(term, b);
            break;
        default:
            term->state = SCTE_GROUND;
            break;
        }
    }

    return SCTE_OK;
}

scte_status_t suts_scte_finish(scte_term_t *term)
{
    if (term == NULL) {
        return SCTE_E_NULL;
    }
    if (term->plane == NULL) {
        return SCTE_E_NULL;
    }
    if (term->state != SCTE_GROUND || term->carry_len != 0u) {
        return SCTE_E_INCOMPLETE;
    }
    return SCTE_OK;
}

void suts_scte_reset(scte_term_t *term)
{
    if (term != NULL) {
        scte_parser_reset(term);
    }
}
