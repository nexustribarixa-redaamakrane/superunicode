/* SUTS-012 - System Console Text Service (SCTS)
 *
 * Reference implementation of the optional console contract. Width is taken
 * from SGW, break opportunities from SBR, cluster boundaries from STS. The
 * plane, its cells, and the sink are all caller-supplied; nothing allocates.
 *
 * See docs/suts/SUTS-012-scts.md for the normative specification.
 */

#include "suts/suts_scts.h"

/* -- Small helpers ----------------------------------------------------- */

static scts_cell_t *scts_cell_at(const scts_plane_t *p, uint16_t row,
                                 uint16_t col)
{
    return &p->cells[(size_t)row * (size_t)p->cols + (size_t)col];
}

static uint16_t scts_style(const scts_plane_t *p)
{
    return (uint16_t)((p->fg & SCTS_ATTR_FG_MASK) |
                      ((p->bg & 0x0Fu) << SCTS_ATTR_BG_SHIFT));
}

static uint16_t scts_blank_attrs(const scts_plane_t *p)
{
    return (uint16_t)((p->fg & SCTS_ATTR_FG_MASK) |
                      (((p->bg & 0x0Fu) << SCTS_ATTR_BG_SHIFT)));
}

static bool scts_is_cont(uint8_t b)
{
    return (b & 0xC0u) == 0x80u;
}

static scts_status_t scts_decode(const uint8_t *p, size_t n, size_t *used,
                                 uint32_t *out_cp)
{
    uint8_t b0;
    uint32_t cp;
    size_t need;
    size_t i;

    if (n == 0u) {
        return SCTS_E_ENCODING;
    }

    b0 = p[0];
    if (b0 < 0x80u) {
        cp = (uint32_t)b0;
        *used = 1u;
    } else if ((b0 & 0xE0u) == 0xC0u) {
        need = 2u;
        if (n < need) return SCTS_E_ENCODING;
        for (i = 1u; i < need; i++) if (!scts_is_cont(p[i])) return SCTS_E_ENCODING;
        cp = ((uint32_t)(b0 & 0x1Fu) << 6) | (uint32_t)(p[1] & 0x3Fu);
        if (cp < 0x80u) return SCTS_E_ENCODING;
        *used = need;
    } else if ((b0 & 0xF0u) == 0xE0u) {
        need = 3u;
        if (n < need) return SCTS_E_ENCODING;
        for (i = 1u; i < need; i++) if (!scts_is_cont(p[i])) return SCTS_E_ENCODING;
        cp = ((uint32_t)(b0 & 0x0Fu) << 12) |
             ((uint32_t)(p[1] & 0x3Fu) << 6) | (uint32_t)(p[2] & 0x3Fu);
        if (cp < 0x800u) return SCTS_E_ENCODING;
        if (cp >= 0xD800u && cp <= 0xDFFFu) return SCTS_E_ENCODING;
        *used = need;
    } else if ((b0 & 0xF8u) == 0xF0u) {
        need = 4u;
        if (n < need) return SCTS_E_ENCODING;
        for (i = 1u; i < need; i++) if (!scts_is_cont(p[i])) return SCTS_E_ENCODING;
        cp = ((uint32_t)(b0 & 0x07u) << 18) |
             ((uint32_t)(p[1] & 0x3Fu) << 12) |
             ((uint32_t)(p[2] & 0x3Fu) << 6) | (uint32_t)(p[3] & 0x3Fu);
        if (cp < 0x10000u || cp > 0x0010FFFFu) return SCTS_E_ENCODING;
        *used = need;
    } else if ((b0 & 0xFCu) == 0xF8u) {
        need = 5u;
        if (n < need) return SCTS_E_ENCODING;
        for (i = 1u; i < need; i++) if (!scts_is_cont(p[i])) return SCTS_E_ENCODING;
        cp = ((uint32_t)(b0 & 0x03u) << 24) |
             ((uint32_t)(p[1] & 0x3Fu) << 18) |
             ((uint32_t)(p[2] & 0x3Fu) << 12) |
             ((uint32_t)(p[3] & 0x3Fu) << 6) | (uint32_t)(p[4] & 0x3Fu);
        if (cp < 0x00110000u || cp > 0x03FFFFFFu) return SCTS_E_ENCODING;
        *used = need;
    } else if ((b0 & 0xFEu) == 0xFCu) {
        need = 6u;
        if (n < need) return SCTS_E_ENCODING;
        for (i = 1u; i < need; i++) if (!scts_is_cont(p[i])) return SCTS_E_ENCODING;
        cp = ((uint32_t)(b0 & 0x01u) << 30) |
             ((uint32_t)(p[1] & 0x3Fu) << 24) |
             ((uint32_t)(p[2] & 0x3Fu) << 18) |
             ((uint32_t)(p[3] & 0x3Fu) << 12) |
             ((uint32_t)(p[4] & 0x3Fu) << 6) | (uint32_t)(p[5] & 0x3Fu);
        if (cp < 0x04000000u) return SCTS_E_ENCODING;
        *used = need;
    } else {
        return SCTS_E_ENCODING;
    }

    if (cp >= 0x00110000u && cp <= 0x0011FFFFu) {
        return SCTS_E_INADMISSIBLE;
    }
    if (cp >= 0x7FFFFFF0u) {
        return SCTS_E_INADMISSIBLE;
    }

    *out_cp = cp;
    return SCTS_OK;
}

static size_t scts_encode(uint32_t cp, uint8_t *out)
{
    if (cp <= 0x7Fu) {
        out[0] = (uint8_t)cp;
        return 1u;
    }
    if (cp <= 0x7FFu) {
        out[0] = (uint8_t)(0xC0u | ((cp >> 6) & 0x1Fu));
        out[1] = (uint8_t)(0x80u | (cp & 0x3Fu));
        return 2u;
    }
    if (cp <= 0xFFFFu) {
        out[0] = (uint8_t)(0xE0u | ((cp >> 12) & 0x0Fu));
        out[1] = (uint8_t)(0x80u | ((cp >> 6) & 0x3Fu));
        out[2] = (uint8_t)(0x80u | (cp & 0x3Fu));
        return 3u;
    }
    if (cp <= 0x0010FFFFu) {
        out[0] = (uint8_t)(0xF0u | ((cp >> 18) & 0x07u));
        out[1] = (uint8_t)(0x80u | ((cp >> 12) & 0x3Fu));
        out[2] = (uint8_t)(0x80u | ((cp >> 6) & 0x3Fu));
        out[3] = (uint8_t)(0x80u | (cp & 0x3Fu));
        return 4u;
    }
    if (cp <= 0x03FFFFFFu) {
        out[0] = (uint8_t)(0xF8u | ((cp >> 24) & 0x03u));
        out[1] = (uint8_t)(0x80u | ((cp >> 18) & 0x3Fu));
        out[2] = (uint8_t)(0x80u | ((cp >> 12) & 0x3Fu));
        out[3] = (uint8_t)(0x80u | ((cp >> 6) & 0x3Fu));
        out[4] = (uint8_t)(0x80u | (cp & 0x3Fu));
        return 5u;
    }
    out[0] = (uint8_t)(0xFCu | ((cp >> 30) & 0x01u));
    out[1] = (uint8_t)(0x80u | ((cp >> 24) & 0x3Fu));
    out[2] = (uint8_t)(0x80u | ((cp >> 18) & 0x3Fu));
    out[3] = (uint8_t)(0x80u | ((cp >> 12) & 0x3Fu));
    out[4] = (uint8_t)(0x80u | ((cp >> 6) & 0x3Fu));
    out[5] = (uint8_t)(0x80u | (cp & 0x3Fu));
    return 6u;
}

/* -- Cluster records --------------------------------------------------- */

static int scts_find_cluster(const scts_plane_t *p, uint16_t row,
                             uint16_t col)
{
    uint16_t i;
    for (i = 0u; i < p->cluster_count; i++) {
        if (p->clusters[i].row == row && p->clusters[i].col == col) {
            return (int)i;
        }
    }
    return -1;
}

static void scts_remove_cluster_index(scts_plane_t *p, uint16_t idx)
{
    uint16_t i;
    if (idx >= p->cluster_count) {
        return;
    }
    for (i = (uint16_t)(idx + 1u); i < p->cluster_count; i++) {
        p->clusters[i - 1u] = p->clusters[i];
    }
    p->cluster_count--;
}

static void scts_remove_cluster_at(scts_plane_t *p, uint16_t row,
                                   uint16_t col)
{
    int idx = scts_find_cluster(p, row, col);
    if (idx >= 0) {
        scts_remove_cluster_index(p, (uint16_t)idx);
    }
}

static void scts_set_blank(scts_plane_t *p, uint16_t row, uint16_t col)
{
    scts_cell_t *c = scts_cell_at(p, row, col);
    c->cp = SCTS_MARK_BLANK;
    c->attrs = scts_blank_attrs(p);
}

/* Replace a cell with a blank, keeping wide pairs whole (section 5.4). */
static void scts_clear_cell(scts_plane_t *p, uint16_t row, uint16_t col)
{
    scts_cell_t *c = scts_cell_at(p, row, col);

    if ((c->attrs & SCTS_ATTR_WIDE_TAIL) != 0u) {
        scts_remove_cluster_at(p, row, (uint16_t)(col - 1u));
        if (col > 0u) {
            scts_set_blank(p, row, (uint16_t)(col - 1u));
        }
        scts_set_blank(p, row, col);
    } else if ((c->attrs & SCTS_ATTR_WIDE_LEAD) != 0u) {
        scts_remove_cluster_at(p, row, col);
        scts_set_blank(p, row, col);
        if ((uint32_t)col + 1u < (uint32_t)p->cols) {
            scts_set_blank(p, row, (uint16_t)(col + 1u));
        }
    } else {
        scts_remove_cluster_at(p, row, col);
        scts_set_blank(p, row, col);
    }
}

static void scts_mark_dirty(scts_plane_t *p, uint16_t row)
{
    if (row < 64u) {
        p->dirty |= ((uint64_t)1u << row);
    } else {
        p->dirty_all = true;
    }
}

/* Place a cluster at the cursor. The caller has already checked that it
 * fits. When count_write is false the placement is a move, not a new write. */
static scts_status_t scts_place_cluster(scts_plane_t *p, const uint32_t *cps,
                                        size_t clen, uint8_t width,
                                        bool count_write)
{
    uint16_t row = p->cursor_row;
    uint16_t col = p->cursor_col;
    uint16_t style = scts_style(p);
    uint16_t i;

    if (width == 0u || width > 2u) {
        return SCTS_E_CLUSTER;
    }
    if ((uint32_t)col + (uint32_t)width > (uint32_t)p->cols) {
        return SCTS_E_COL_RANGE;
    }

    for (i = 0u; i < (uint16_t)width; i++) {
        scts_clear_cell(p, row, (uint16_t)(col + i));
    }

    if (width == 1u) {
        scts_cell_t *c = scts_cell_at(p, row, col);
        c->cp = cps[0];
        c->attrs = style;
    } else {
        scts_cell_t *lead = scts_cell_at(p, row, col);
        scts_cell_t *tail = scts_cell_at(p, row, (uint16_t)(col + 1u));
        lead->cp = cps[0];
        lead->attrs = (uint16_t)(style | SCTS_ATTR_WIDE_LEAD);
        tail->cp = SCTS_MARK_CONT;
        tail->attrs = (uint16_t)(style | SCTS_ATTR_WIDE_TAIL);
    }

    if (p->cluster_count < (uint16_t)SCTS_MAX_CLUSTERS) {
        scts_cluster_t *rec = &p->clusters[p->cluster_count];
        size_t n = clen;
        if (n > (size_t)SCTS_MAX_CLUSTER_CP) {
            n = (size_t)SCTS_MAX_CLUSTER_CP;
        }
        rec->row = row;
        rec->col = col;
        rec->len = (uint8_t)n;
        rec->width = width;
        for (i = 0u; i < (uint16_t)n; i++) {
            rec->cps[i] = cps[i];
        }
        p->cluster_count++;
    }

    p->cursor_col = (uint16_t)(col + width);
    scts_mark_dirty(p, row);
    if (count_write) {
        p->writes++;
    }
    return SCTS_OK;
}

/* -- Cursor, wrap, scroll --------------------------------------------- */

static scts_status_t scts_scroll_impl(scts_plane_t *p, uint16_t n)
{
    uint16_t r;
    uint16_t c;
    uint16_t out;

    if (n == 0u) {
        return SCTS_OK;
    }
    if (n >= p->rows) {
        for (r = 0u; r < p->rows; r++) {
            for (c = 0u; c < p->cols; c++) {
                scts_set_blank(p, r, c);
            }
        }
        p->cluster_count = 0u;
        p->cursor_row = (uint16_t)(p->rows - 1u);
        p->cursor_col = 0u;
        p->scrolls += n;
        p->dirty_all = true;
        p->dirty = 0u;
        return SCTS_OK;
    }

    for (r = 0u; (uint32_t)r + (uint32_t)n < (uint32_t)p->rows; r++) {
        for (c = 0u; c < p->cols; c++) {
            *scts_cell_at(p, r, c) = *scts_cell_at(p, (uint16_t)(r + n), c);
        }
    }
    for (r = (uint16_t)(p->rows - n); r < p->rows; r++) {
        for (c = 0u; c < p->cols; c++) {
            scts_set_blank(p, r, c);
        }
    }

    out = 0u;
    for (r = 0u; r < p->cluster_count; r++) {
        if ((uint32_t)p->clusters[r].row >= (uint32_t)n) {
            p->clusters[out] = p->clusters[r];
            p->clusters[out].row = (uint16_t)(p->clusters[out].row - n);
            out++;
        }
    }
    p->cluster_count = out;

    p->cursor_row = (uint16_t)(p->rows - 1u);
    p->scrolls += n;
    p->dirty_all = true;
    p->dirty = 0u;
    return SCTS_OK;
}

static scts_status_t scts_newline_impl(scts_plane_t *p)
{
    if ((uint32_t)p->cursor_row + 1u < (uint32_t)p->rows) {
        p->cursor_row++;
    } else {
        scts_status_t st = scts_scroll_impl(p, 1u);
        if (st != SCTS_OK) {
            return st;
        }
    }
    p->cursor_col = 0u;
    return SCTS_OK;
}

scts_status_t scts_plane_newline(scts_plane_t *p)
{
    if (p == NULL || !p->initialized) {
        return SCTS_E_NOT_INIT;
    }
    return scts_newline_impl(p);
}

scts_status_t scts_plane_carriage_return(scts_plane_t *p)
{
    if (p == NULL || !p->initialized) {
        return SCTS_E_NOT_INIT;
    }
    p->cursor_col = 0u;
    return SCTS_OK;
}

scts_status_t scts_plane_backspace(scts_plane_t *p)
{
    if (p == NULL || !p->initialized) {
        return SCTS_E_NOT_INIT;
    }
    if (p->cursor_col > 0u) {
        p->cursor_col--;
    }
    return SCTS_OK;
}

scts_status_t scts_plane_scroll(scts_plane_t *p, uint16_t n)
{
    if (p == NULL || !p->initialized) {
        return SCTS_E_NOT_INIT;
    }
    return scts_scroll_impl(p, n);
}

scts_status_t scts_plane_tab(scts_plane_t *p)
{
    uint32_t next;
    uint32_t tw;

    if (p == NULL || !p->initialized) {
        return SCTS_E_NOT_INIT;
    }
    tw = (uint32_t)p->tab_width;
    if (tw == 0u) {
        tw = 1u;
    }
    next = (((uint32_t)p->cursor_col / tw) + 1u) * tw;
    if (next >= (uint32_t)p->cols) {
        if (!p->wrap) {
            return SCTS_E_COL_RANGE;
        }
        {
            scts_status_t st = scts_newline_impl(p);
            if (st != SCTS_OK) {
                return st;
            }
        }
        return SCTS_OK;
    }
    p->cursor_col = (uint16_t)next;
    return SCTS_OK;
}

/* -- Break-aware wrapping --------------------------------------------- */

/* Collect the cluster indices on a row, ordered by column. */
static uint16_t scts_row_clusters(const scts_plane_t *p, uint16_t row,
                                  uint16_t *idx, uint16_t cap)
{
    uint16_t i;
    uint16_t n = 0u;

    for (i = 0u; i < p->cluster_count && n < cap; i++) {
        if (p->clusters[i].row == row) {
            /* Insertion by column; the record set is small. */
            uint16_t j = n;
            while (j > 0u && p->clusters[idx[j - 1u]].col >
                             p->clusters[i].col) {
                idx[j] = idx[j - 1u];
                j--;
            }
            idx[j] = i;
            n++;
        }
    }
    return n;
}

static bool scts_break_before(const scts_plane_t *p, const uint16_t *idx,
                              uint16_t k)
{
    const scts_cluster_t *prev;
    const scts_cluster_t *cur;

    if (k == 0u) {
        return true;
    }
    prev = &p->clusters[idx[k - 1u]];
    cur = &p->clusters[idx[k]];
    if (prev->len == 0u || cur->len == 0u) {
        return true;
    }
    return suas_sbr_pair(prev->cps[prev->len - 1u], cur->cps[0], p->sbr) !=
           SUAS_BRK_NO_BREAK;
}

/* The column of the first cluster after the last break opportunity on the
 * row. Returns cursor_col when the row has no clusters. */
static uint16_t scts_last_break_col(const scts_plane_t *p, uint16_t row)
{
    uint16_t idx[SCTS_MAX_CLUSTERS];
    uint16_t n = scts_row_clusters(p, row, idx, (uint16_t)SCTS_MAX_CLUSTERS);
    uint16_t k;

    if (n == 0u) {
        return p->cursor_col;
    }
    for (k = (uint16_t)(n - 1u); k > 0u; k--) {
        if (scts_break_before(p, idx, k)) {
            return p->clusters[idx[k]].col;
        }
    }
    return p->clusters[idx[0]].col;
}

/* The rightmost cluster on the row, or -1 when the row is empty. */
static int scts_last_cluster_on_row(const scts_plane_t *p, uint16_t row)
{
    uint16_t idx[SCTS_MAX_CLUSTERS];
    uint16_t n = scts_row_clusters(p, row, idx, (uint16_t)SCTS_MAX_CLUSTERS);
    if (n == 0u) {
        return -1;
    }
    return (int)idx[n - 1u];
}

static scts_status_t scts_move_run(scts_plane_t *p, uint16_t from_row,
                                   uint16_t move_col)
{
    scts_cluster_t moved[SCTS_MAX_CLUSTERS];
    uint16_t moved_n = 0u;
    uint16_t i;
    uint16_t c;
    scts_status_t st;

    for (i = 0u; i < p->cluster_count; i++) {
        if (p->clusters[i].row == from_row &&
            p->clusters[i].col >= move_col &&
            moved_n < (uint16_t)SCTS_MAX_CLUSTERS) {
            moved[moved_n++] = p->clusters[i];
        }
    }
    if (moved_n == 0u) {
        return SCTS_OK;
    }

    /* Clear the moved cells and drop the old records. */
    for (i = 0u; i < moved_n; i++) {
        for (c = 0u; c < moved[i].width; c++) {
            scts_set_blank(p, moved[i].row, (uint16_t)(moved[i].col + c));
        }
    }
    {
        uint16_t out = 0u;
        for (i = 0u; i < p->cluster_count; i++) {
            if (p->clusters[i].row == from_row &&
                p->clusters[i].col >= move_col) {
                continue;
            }
            p->clusters[out++] = p->clusters[i];
        }
        p->cluster_count = out;
    }

    st = scts_newline_impl(p);
    if (st != SCTS_OK) {
        return st;
    }

    for (i = 0u; i < moved_n; i++) {
        st = scts_place_cluster(p, moved[i].cps, moved[i].len,
                                moved[i].width, false);
        if (st != SCTS_OK) {
            return st;
        }
    }
    return SCTS_OK;
}

static scts_status_t scts_flush_cluster(scts_plane_t *p, const uint32_t *cps,
                                        size_t clen)
{
    suas_sts_gcb_t gcb;
    uint32_t base;
    uint8_t width;
    scts_status_t st;

    if (clen == 0u) {
        return SCTS_OK;
    }
    base = cps[0];
    gcb = suas_sts_classify_grapheme((sucs_ex_char_t)base);
    if (gcb == SUAS_STS_GCB_EXTEND || gcb == SUAS_STS_GCB_ZWJ ||
        gcb == SUAS_STS_GCB_SPACINGMARK) {
        return SCTS_E_CLUSTER; /* a mark cannot be its own base */
    }

    width = (uint8_t)suas_sgw_cells((sucs_ex_char_t)base, p->wide_context,
                                    p->sgw);
    if (width == 0u || width > 2u) {
        return SCTS_E_CLUSTER;
    }

    if ((uint32_t)p->cursor_col + (uint32_t)width > (uint32_t)p->cols) {
        uint16_t row;
        bool break_here = true;

        if (!p->wrap) {
            return SCTS_E_COL_RANGE;
        }
        if ((uint32_t)width > (uint32_t)p->cols) {
            return SCTS_E_CLUSTER;
        }

        row = p->cursor_row;
        {
            int last = scts_last_cluster_on_row(p, row);
            if (last >= 0 && p->clusters[last].len > 0u) {
                const scts_cluster_t *prev = &p->clusters[last];
                break_here = suas_sbr_pair(prev->cps[prev->len - 1u], base,
                                           p->sbr) != SUAS_BRK_NO_BREAK;
            }
        }

        if (break_here) {
            /* The new cluster starts a new breakable unit: move only it. */
            st = scts_newline_impl(p);
            if (st != SCTS_OK) {
                return st;
            }
        } else {
            /* The new cluster continues an unbreakable run: move the whole
             * run to the next row when it fits there, otherwise break at the
             * run start. */
            uint16_t move_col = scts_last_break_col(p, row);
            uint32_t run_width = (uint32_t)p->cursor_col - (uint32_t)move_col;
            if (move_col > 0u && move_col < p->cursor_col &&
                run_width + (uint32_t)width <= (uint32_t)p->cols) {
                st = scts_move_run(p, row, move_col);
                if (st != SCTS_OK) {
                    return st;
                }
            } else {
                st = scts_newline_impl(p);
                if (st != SCTS_OK) {
                    return st;
                }
            }
        }
    }

    return scts_place_cluster(p, cps, clen, width, true);
}

/* -- Lifecycle --------------------------------------------------------- */

const char *suts_scts_version_string(void)
{
    return "SUTS-012 SCTS 1.0.0";
}

const char *suts_scts_status_name(scts_status_t st)
{
    switch (st) {
    case SCTS_OK:             return "SCTS_OK";
    case SCTS_E_NOT_INIT:     return "SCTS_E_NOT_INIT";
    case SCTS_E_NULL:         return "SCTS_E_NULL";
    case SCTS_E_ROW_RANGE:    return "SCTS_E_ROW_RANGE";
    case SCTS_E_COL_RANGE:    return "SCTS_E_COL_RANGE";
    case SCTS_E_CLUSTER:      return "SCTS_E_CLUSTER";
    case SCTS_E_CONTROL:      return "SCTS_E_CONTROL";
    case SCTS_E_ENCODING:     return "SCTS_E_ENCODING";
    case SCTS_E_INADMISSIBLE: return "SCTS_E_INADMISSIBLE";
    case SCTS_E_CAPACITY:     return "SCTS_E_CAPACITY";
    case SCTS_E_NO_SINK:      return "SCTS_E_NO_SINK";
    case SCTS_E_SINK:         return "SCTS_E_SINK";
    case SCTS_E_GEOMETRY:     return "SCTS_E_GEOMETRY";
    case SCTS_E_ZONE:         return "SCTS_E_ZONE";
    default:                  return "SCTS_E_UNKNOWN";
    }
}

void scts_plane_options_default(scts_plane_options_t *o)
{
    if (o == NULL) {
        return;
    }
    o->tab_width = 8u;
    o->wrap = true;
    o->wide_context = false;
}

scts_status_t scts_plane_init(scts_plane_t *p, scts_cell_t *cells,
                              uint16_t rows, uint16_t cols,
                              const scts_plane_options_t *o)
{
    if (p == NULL || cells == NULL) {
        return SCTS_E_NULL;
    }
    if (rows == 0u || cols == 0u) {
        return SCTS_E_GEOMETRY;
    }
    /* The cell product must fit size_t. uint16 * uint16 always fits a 32-bit
     * size_t, but the check is kept explicit for narrower targets. */
    if ((size_t)rows > (size_t)(~(size_t)0) / (size_t)cols) {
        return SCTS_E_GEOMETRY;
    }

    p->cells = cells;
    p->rows = rows;
    p->cols = cols;
    p->cursor_row = 0u;
    p->cursor_col = 0u;
    p->fg = 0u;
    p->bg = 0u;
    p->tab_width = (o != NULL && o->tab_width != 0u) ? (uint8_t)o->tab_width
                                                     : 8u;
    p->wrap = (o != NULL) ? o->wrap : true;
    p->wide_context = (o != NULL) ? o->wide_context : false;
    p->initialized = true;
    p->writes = 0u;
    p->scrolls = 0u;
    p->dirty = 0u;
    p->dirty_all = false;
    p->sgw = NULL;
    p->sbr = NULL;
    p->cluster_count = 0u;
    p->sink.emit = NULL;
    p->sink.token = 0u;
    p->sink.level = SCTS_SINK_L1_SUTF8;
    p->sink_bound = false;
    p->lossy_substitutions = 0u;

    return scts_plane_clear(p);
}

scts_status_t scts_plane_clear(scts_plane_t *p)
{
    uint16_t r;
    uint16_t c;

    if (p == NULL || !p->initialized) {
        return SCTS_E_NOT_INIT;
    }
    for (r = 0u; r < p->rows; r++) {
        for (c = 0u; c < p->cols; c++) {
            scts_set_blank(p, r, c);
        }
    }
    p->cursor_row = 0u;
    p->cursor_col = 0u;
    p->cluster_count = 0u;
    p->dirty_all = true;
    p->dirty = 0u;
    return SCTS_OK;
}

scts_status_t scts_plane_set_style(scts_plane_t *p, uint16_t fg, uint16_t bg)
{
    if (p == NULL || !p->initialized) {
        return SCTS_E_NOT_INIT;
    }
    p->fg = (uint16_t)(fg & 0x0Fu);
    p->bg = (uint16_t)(bg & 0x0Fu);
    return SCTS_OK;
}

/* -- Writing ----------------------------------------------------------- */

scts_status_t scts_plane_write(scts_plane_t *p, const uint8_t *sutf8,
                               size_t len)
{
    size_t pos = 0u;
    uint32_t cps[SCTS_MAX_CLUSTER_CP];
    size_t clen = 0u;
    suas_sts_state_t sts;
    bool prev_cr = false;
    scts_status_t st;

    if (p == NULL || !p->initialized) {
        return SCTS_E_NOT_INIT;
    }
    if (sutf8 == NULL && len != 0u) {
        return SCTS_E_NULL;
    }

    suas_sts_init(&sts, SUAS_STS_LEVEL_GRAPHEME);

    while (pos < len) {
        uint32_t cp = 0u;
        size_t used = 0u;

        st = scts_decode(sutf8 + pos, len - pos, &used, &cp);
        if (st != SCTS_OK) {
            return st;
        }
        pos += used;

        if (prev_cr) {
            prev_cr = false;
            if (cp == 0x000Au) {
                /* CRLF is one line break, not two. */
                st = scts_flush_cluster(p, cps, clen);
                if (st != SCTS_OK) return st;
                clen = 0u;
                suas_sts_init(&sts, SUAS_STS_LEVEL_GRAPHEME);
                st = scts_newline_impl(p);
                if (st != SCTS_OK) return st;
                continue;
            }
            st = scts_flush_cluster(p, cps, clen);
            if (st != SCTS_OK) return st;
            clen = 0u;
            suas_sts_init(&sts, SUAS_STS_LEVEL_GRAPHEME);
            st = scts_plane_carriage_return(p);
            if (st != SCTS_OK) return st;
            /* fall through and process cp below */
        }

        if (cp == 0x000Du) { /* CR */
            st = scts_flush_cluster(p, cps, clen);
            if (st != SCTS_OK) return st;
            clen = 0u;
            suas_sts_init(&sts, SUAS_STS_LEVEL_GRAPHEME);
            prev_cr = true;
            continue;
        }
        if (cp == 0x000Au) { /* LF */
            st = scts_flush_cluster(p, cps, clen);
            if (st != SCTS_OK) return st;
            clen = 0u;
            suas_sts_init(&sts, SUAS_STS_LEVEL_GRAPHEME);
            st = scts_newline_impl(p);
            if (st != SCTS_OK) return st;
            continue;
        }
        if (cp == 0x0009u) { /* HT */
            st = scts_flush_cluster(p, cps, clen);
            if (st != SCTS_OK) return st;
            clen = 0u;
            suas_sts_init(&sts, SUAS_STS_LEVEL_GRAPHEME);
            st = scts_plane_tab(p);
            if (st != SCTS_OK) return st;
            continue;
        }
        if (cp == 0x0008u) { /* BS */
            st = scts_flush_cluster(p, cps, clen);
            if (st != SCTS_OK) return st;
            clen = 0u;
            suas_sts_init(&sts, SUAS_STS_LEVEL_GRAPHEME);
            st = scts_plane_backspace(p);
            if (st != SCTS_OK) return st;
            continue;
        }
        if (cp == 0x0000u) { /* NUL is ignored, but it ends a cluster */
            st = scts_flush_cluster(p, cps, clen);
            if (st != SCTS_OK) return st;
            clen = 0u;
            suas_sts_init(&sts, SUAS_STS_LEVEL_GRAPHEME);
            continue;
        }
        if (cp < 0x0020u || (cp >= 0x007Fu && cp <= 0x009Fu)) {
            return SCTS_E_CONTROL;
        }

        {
            suas_sts_boundary_t b = suas_sts_evaluate(&sts, cp);
            if (clen > 0u && b != SUAS_STS_BOUNDARY_NONE) {
                st = scts_flush_cluster(p, cps, clen);
                if (st != SCTS_OK) return st;
                clen = 0u;
            }
            if (clen < (size_t)SCTS_MAX_CLUSTER_CP) {
                cps[clen++] = cp;
            } else {
                st = scts_flush_cluster(p, cps, clen);
                if (st != SCTS_OK) return st;
                cps[0] = cp;
                clen = 1u;
            }
        }
    }

    st = scts_flush_cluster(p, cps, clen);
    if (st != SCTS_OK) {
        return st;
    }
    if (prev_cr) {
        return scts_plane_carriage_return(p);
    }
    return SCTS_OK;
}

scts_status_t scts_plane_put_cp(scts_plane_t *p, uint32_t cp)
{
    uint32_t one[1];

    if (p == NULL || !p->initialized) {
        return SCTS_E_NOT_INIT;
    }
    if (cp >= 0x00110000u && cp <= 0x0011FFFFu) {
        return SCTS_E_INADMISSIBLE;
    }
    if (cp >= 0x0000D800u && cp <= 0x0000DFFFu) {
        return SCTS_E_INADMISSIBLE;
    }
    if (cp >= 0x7FFFFFF0u || cp > 0x7FFFFFEFu) {
        return SCTS_E_INADMISSIBLE;
    }
    if (cp < 0x0020u || (cp >= 0x007Fu && cp <= 0x009Fu)) {
        return SCTS_E_CONTROL;
    }
    one[0] = cp;
    return scts_flush_cluster(p, one, 1u);
}

/* -- Serialization ----------------------------------------------------- */

scts_status_t scts_plane_serialize_row(const scts_plane_t *p, uint16_t row,
                                       uint8_t *buf, size_t cap, bool trim,
                                       size_t *out_len)
{
    uint16_t end;
    uint16_t col;
    size_t n = 0u;

    if (out_len != NULL) {
        *out_len = 0u;
    }
    if (p == NULL || !p->initialized) {
        return SCTS_E_NOT_INIT;
    }
    if (row >= p->rows) {
        return SCTS_E_ROW_RANGE;
    }
    if (buf == NULL && cap != 0u) {
        return SCTS_E_NULL;
    }

    end = p->cols;
    if (trim) {
        while (end > 0u) {
            const scts_cell_t *c = scts_cell_at(p, row, (uint16_t)(end - 1u));
            if (c->cp == SCTS_MARK_BLANK &&
                (c->attrs & SCTS_ATTR_WIDE_TAIL) == 0u) {
                end--;
            } else {
                break;
            }
        }
    }

    for (col = 0u; col < end; col++) {
        const scts_cell_t *c = scts_cell_at(p, row, col);
        uint32_t emit[1 + SCTS_MAX_CLUSTER_CP];
        size_t emit_n = 0u;
        size_t i;
        int idx;

        if ((c->attrs & SCTS_ATTR_WIDE_TAIL) != 0u) {
            continue; /* the lead already emitted the whole cluster */
        }
        if (c->cp == SCTS_MARK_CONT) {
            continue; /* orphan marker: never serialized */
        }

        if (c->cp == SCTS_MARK_BLANK) {
            emit[emit_n++] = 0x0020u;
        } else {
            idx = scts_find_cluster(p, row, col);
            if (idx >= 0) {
                const scts_cluster_t *rec = &p->clusters[idx];
                size_t k;
                for (k = 0u; k < (size_t)rec->len; k++) {
                    emit[emit_n++] = rec->cps[k];
                }
            } else {
                emit[emit_n++] = c->cp;
            }
        }

        for (i = 0u; i < emit_n; i++) {
            uint8_t tmp[6];
            size_t k = scts_encode(emit[i], tmp);
            size_t j;
            if (n + k > cap) {
                if (out_len != NULL) *out_len = 0u;
                return SCTS_E_CAPACITY;
            }
            for (j = 0u; j < k; j++) {
                buf[n + j] = tmp[j];
            }
            n += k;
        }
    }

    if (out_len != NULL) {
        *out_len = n;
    }
    return SCTS_OK;
}

scts_status_t scts_plane_cluster_at(const scts_plane_t *p, uint16_t row,
                                    uint16_t col, uint32_t *out_cps,
                                    size_t cap, size_t *out_count)
{
    int idx;

    if (out_count != NULL) {
        *out_count = 0u;
    }
    if (p == NULL || !p->initialized || out_cps == NULL || out_count == NULL) {
        return SCTS_E_NULL;
    }
    if (row >= p->rows || col >= p->cols) {
        return SCTS_E_ROW_RANGE;
    }
    idx = scts_find_cluster(p, row, col);
    if (idx < 0) {
        return SCTS_E_CLUSTER;
    }
    {
        const scts_cluster_t *rec = &p->clusters[idx];
        size_t n = (size_t)rec->len;
        size_t i;
        if (n > cap) {
            return SCTS_E_CAPACITY;
        }
        for (i = 0u; i < n; i++) {
            out_cps[i] = rec->cps[i];
        }
        *out_count = n;
    }
    return SCTS_OK;
}

/* -- Sinks ------------------------------------------------------------- */

void scts_plane_set_sink(scts_plane_t *p, const scts_sink_t *sink)
{
    if (p == NULL) {
        return;
    }
    if (sink == NULL || sink->emit == NULL) {
        p->sink.emit = NULL;
        p->sink.token = 0u;
        p->sink.level = SCTS_SINK_L1_SUTF8;
        p->sink_bound = false;
        return;
    }
    p->sink = *sink;
    p->sink_bound = true;
}

bool scts_plane_row_dirty(const scts_plane_t *p, uint16_t row)
{
    if (p == NULL || !p->initialized || row >= p->rows) {
        return false;
    }
    if (p->dirty_all) {
        return true;
    }
    if (row < 64u) {
        return (p->dirty & ((uint64_t)1u << row)) != 0u;
    }
    return false;
}

void scts_plane_clear_dirty(scts_plane_t *p)
{
    if (p == NULL) {
        return;
    }
    p->dirty = 0u;
    p->dirty_all = false;
}

scts_status_t scts_plane_emit_row(scts_plane_t *p, uint16_t row,
                                  uint8_t *scratch, size_t scratch_cap)
{
    uint16_t col;

    if (p == NULL || !p->initialized) {
        return SCTS_E_NOT_INIT;
    }
    if (row >= p->rows) {
        return SCTS_E_ROW_RANGE;
    }
    if (!p->sink_bound || p->sink.emit == NULL) {
        return SCTS_E_NO_SINK;
    }

    if (p->sink.level == SCTS_SINK_L0_BYTE) {
        for (col = 0u; col < p->cols; col++) {
            const scts_cell_t *c = scts_cell_at(p, row, col);
            uint32_t cp;
            uint8_t b;
            uint32_t len;

            if ((c->attrs & SCTS_ATTR_WIDE_TAIL) != 0u) {
                continue;
            }
            if (c->cp == SCTS_MARK_CONT) {
                continue;
            }
            cp = (c->cp == SCTS_MARK_BLANK) ? 0x0020u : c->cp;
            if (cp >= 0x0020u && cp <= 0x007Eu) {
                b = (uint8_t)cp;
            } else {
                b = (uint8_t)'?';
                p->lossy_substitutions++;
            }
            len = 1u;
            p->sink.emit(p->sink.token, &b, len);
        }
    } else if (p->sink.level == SCTS_SINK_L1_SUTF8) {
        for (col = 0u; col < p->cols; col++) {
            const scts_cell_t *c = scts_cell_at(p, row, col);
            uint32_t cp;
            uint32_t len;
            uint8_t b[6];
            int idx;

            if ((c->attrs & SCTS_ATTR_WIDE_TAIL) != 0u) {
                continue;
            }
            if (c->cp == SCTS_MARK_CONT) {
                continue;
            }
            if (c->cp == SCTS_MARK_BLANK) {
                cp = 0x0020u;
            } else {
                idx = scts_find_cluster(p, row, col);
                if (idx >= 0) {
                    const scts_cluster_t *rec = &p->clusters[idx];
                    size_t k;
                    for (k = 0u; k < (size_t)rec->len; k++) {
                        uint32_t n = (uint32_t)scts_encode(rec->cps[k], b);
                        p->sink.emit(p->sink.token, b, n);
                    }
                    continue;
                }
                cp = c->cp;
            }
            len = (uint32_t)scts_encode(cp, b);
            p->sink.emit(p->sink.token, b, len);
        }
    } else {
        size_t n = 0u;
        scts_status_t st;
        if (scratch == NULL || scratch_cap == 0u) {
            return SCTS_E_CAPACITY;
        }
        st = scts_plane_serialize_row(p, row, scratch, scratch_cap, false, &n);
        if (st != SCTS_OK) {
            return st;
        }
        p->sink.emit(p->sink.token, scratch, (uint32_t)n);
    }

    if (row < 64u) {
        p->dirty &= ~((uint64_t)1u << row);
    }
    return SCTS_OK;
}

scts_status_t scts_plane_flush(scts_plane_t *p, uint8_t *scratch,
                               size_t scratch_cap)
{
    uint16_t row;

    if (p == NULL || !p->initialized) {
        return SCTS_E_NOT_INIT;
    }
    for (row = 0u; row < p->rows; row++) {
        if (scts_plane_row_dirty(p, row)) {
            scts_status_t st = scts_plane_emit_row(p, row, scratch,
                                                   scratch_cap);
            if (st != SCTS_OK) {
                return st;
            }
        }
    }
    scts_plane_clear_dirty(p);
    return SCTS_OK;
}

/* -- Glyph-zone registry ----------------------------------------------- */

static scts_status_t scts_zone_add(scts_zone_table_t *t, uint32_t start,
                                   uint32_t end, const char *owner,
                                   const char *purpose, bool reusable)
{
    size_t i;
    scts_zone_t *z;

    if (t == NULL) {
        return SCTS_E_NULL;
    }
    if (start > end) {
        return SCTS_E_ZONE;
    }
    for (i = 0u; i < t->count; i++) {
        if (!(end < t->zones[i].start || start > t->zones[i].end)) {
            return SCTS_E_ZONE; /* Z1: disjoint */
        }
    }
    if (t->count >= (size_t)SCTS_MAX_ZONES) {
        return SCTS_E_ZONE;
    }
    z = &t->zones[t->count];
    z->start = start;
    z->end = end;
    z->owner = owner;
    z->purpose = purpose;
    z->reusable = reusable ? 1u : 0u;
    z->used = 1u;
    t->count++;
    return SCTS_OK;
}

void scts_zones_init(scts_zone_table_t *t)
{
    size_t i;

    if (t == NULL) {
        return;
    }
    for (i = 0u; i < (size_t)SCTS_MAX_ZONES; i++) {
        t->zones[i].start = 0u;
        t->zones[i].end = 0u;
        t->zones[i].owner = NULL;
        t->zones[i].purpose = NULL;
        t->zones[i].reusable = 0u;
        t->zones[i].used = 0u;
    }
    t->count = 0u;

    /* Z4: markers are allocated before any renderable glyph. */
    (void)scts_zone_add(t, SCTS_MARK_CONT, SCTS_MARK_CONT, "SCTS",
                        "wide continuation marker", false);
    (void)scts_zone_add(t, SCTS_MARK_BLANK, SCTS_MARK_BLANK, "SCTS",
                        "blank cell marker", false);
}

scts_status_t scts_zone_register(scts_zone_table_t *t, uint32_t start,
                                 uint32_t end, const char *owner,
                                 const char *purpose, bool reusable)
{
    return scts_zone_add(t, start, end, owner, purpose, reusable);
}

const scts_zone_t *scts_zone_find(const scts_zone_table_t *t, uint32_t cp)
{
    size_t i;

    if (t == NULL) {
        return NULL;
    }
    for (i = 0u; i < t->count; i++) {
        if (cp >= t->zones[i].start && cp <= t->zones[i].end) {
            return &t->zones[i];
        }
    }
    return NULL;
}

bool scts_zone_overlaps(const scts_zone_table_t *t, uint32_t start,
                        uint32_t end)
{
    size_t i;

    if (t == NULL || start > end) {
        return false;
    }
    for (i = 0u; i < t->count; i++) {
        if (!(end < t->zones[i].start || start > t->zones[i].end)) {
            return true;
        }
    }
    return false;
}

bool scts_zone_is_reserved_marker(uint32_t cp)
{
    return cp == SCTS_MARK_CONT || cp == SCTS_MARK_BLANK;
}
