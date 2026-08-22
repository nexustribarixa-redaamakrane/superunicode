/**
 * @file suf_conv.c
 * @brief Full Bidirectional Font Conversion Implementation
 *
 * Implements bidirectional conversions between SuperUnicode Font (.suf) and:
 *   1. TrueType (.ttf)
 *   2. OpenType CFF (.otf)
 *   3. Web Open Font Format 1.0 (.woff)
 *   4. FontForge Spline Font Database (.sfd)
 *   5. Embedded OpenType (.eot)
 *   6. PostScript Type 1 / PFA / PFB (.ps, .pfa, .pfb)
 *   7. SuperUnicode System Plugin Font blobs (.scsp)
 */

#include "suf/suf_conv.h"
#include "suf/suf_parser.h"
#include <stdio.h>

#include <stdlib.h>
#include <string.h>

/* ========================================================================= */
/* Endianness Helpers                                                        */
/* ========================================================================= */

static inline uint16_t read_be16(const uint8_t *p) {
    return (uint16_t)((p[0] << 8) | p[1]);
}

static inline uint32_t read_be32(const uint8_t *p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

static inline uint16_t read_le16(const uint8_t *p) {
    return (uint16_t)(p[0] | (p[1] << 8));
}

static inline uint32_t read_le32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static inline void write_be16(uint8_t *p, uint16_t v) {
    p[0] = (uint8_t)((v >> 8) & 0xFF);
    p[1] = (uint8_t)(v & 0xFF);
}

static inline void write_be32(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)((v >> 24) & 0xFF);
    p[1] = (uint8_t)((v >> 16) & 0xFF);
    p[2] = (uint8_t)((v >> 8) & 0xFF);
    p[3] = (uint8_t)(v & 0xFF);
}

static inline void write_le16(uint8_t *p, uint16_t v) {
    p[0] = (uint8_t)(v & 0xFF);
    p[1] = (uint8_t)((v >> 8) & 0xFF);
}

static inline void write_le32(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)(v & 0xFF);
    p[1] = (uint8_t)((v >> 8) & 0xFF);
    p[2] = (uint8_t)((v >> 16) & 0xFF);
    p[3] = (uint8_t)((v >> 24) & 0xFF);
}

/* ========================================================================= */
/* Checksum Algorithms (OpenType Checksum, Adler-32, CRC-32, CRC32c, Fletcher-64) */
/* ========================================================================= */

static uint32_t calc_table_checksum(const uint8_t *table, size_t length) {
    uint32_t sum = 0;
    size_t count = (length + 3) / 4;
    for (size_t i = 0; i < count; ++i) {
        size_t off = i * 4;
        uint32_t b0 = (off < length) ? table[off] : 0;
        uint32_t b1 = (off + 1 < length) ? table[off + 1] : 0;
        uint32_t b2 = (off + 2 < length) ? table[off + 2] : 0;
        uint32_t b3 = (off + 3 < length) ? table[off + 3] : 0;
        sum += (b0 << 24) | (b1 << 16) | (b2 << 8) | b3;
    }
    return sum;
}

static uint32_t calc_adler32(const uint8_t *data, size_t len) {
    uint32_t a = 1, b = 0;
    for (size_t i = 0; i < len; ++i) {
        a = (a + data[i]) % 65521;
        b = (b + a) % 65521;
    }
    return (b << 16) | a;
}

static uint32_t calc_crc32c(const uint8_t *data, size_t len) {
    static uint32_t table[256];
    static bool init = false;
    if (!init) {
        for (uint32_t i = 0; i < 256; ++i) {
            uint32_t c = i;
            for (int j = 0; j < 8; ++j) {
                c = (c & 1) ? (0x82F63B78UL ^ (c >> 1)) : (c >> 1);
            }
            table[i] = c;
        }
        init = true;
    }
    uint32_t crc = 0xFFFFFFFFUL;
    for (size_t i = 0; i < len; ++i) {
        crc = table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
    }
    return crc ^ 0xFFFFFFFFUL;
}

static uint64_t calc_fletcher64(const uint8_t *data, size_t len) {
    uint32_t c0 = 0, c1 = 0;
    size_t words = len / 4;
    for (size_t i = 0; i < words; ++i) {
        uint32_t w = read_le32(data + i * 4);
        c0 = (c0 + w);
        c1 = (c1 + c0);
    }
    return ((uint64_t)c1 << 32) | c0;
}

/* ========================================================================= */
/* Self-Contained ZLIB / DEFLATE / INFLATE Engine for WOFF                   */
/* ========================================================================= */

static suf_status_t zlib_compress_stream(const uint8_t *in, size_t in_len, uint8_t **out, size_t *out_len) {
    if (!in || !out || !out_len) return SUF_ERR_NULL_POINTER;

    size_t max_blocks = (in_len + 65534) / 65535;
    if (max_blocks == 0) max_blocks = 1;
    size_t alloc_sz = 2 + (max_blocks * 5) + in_len + 4 + 16;

    uint8_t *buf = (uint8_t *)malloc(alloc_sz);
    if (!buf) return SUF_ERR_ALLOC_FAIL;

    size_t pos = 0;
    buf[pos++] = 0x78;
    buf[pos++] = 0x01;

    size_t remaining = in_len;
    size_t in_pos = 0;

    while (remaining > 0 || in_pos == 0) {
        uint16_t block_sz = (remaining > 65535) ? 65535 : (uint16_t)remaining;
        bool is_last = (remaining <= 65535);

        buf[pos++] = is_last ? 0x01 : 0x00;
        buf[pos++] = (uint8_t)(block_sz & 0xFF);
        buf[pos++] = (uint8_t)((block_sz >> 8) & 0xFF);
        uint16_t nlen = (uint16_t)(~block_sz);
        buf[pos++] = (uint8_t)(nlen & 0xFF);
        buf[pos++] = (uint8_t)((nlen >> 8) & 0xFF);

        if (block_sz > 0) {
            memcpy(buf + pos, in + in_pos, block_sz);
            pos += block_sz;
            in_pos += block_sz;
            remaining -= block_sz;
        }
        if (is_last) break;
    }

    uint32_t adler = calc_adler32(in, in_len);
    write_be32(buf + pos, adler);
    pos += 4;

    *out = buf;
    *out_len = pos;
    return SUF_OK;
}

static suf_status_t zlib_decompress_stream(const uint8_t *in, size_t in_len, size_t orig_len, uint8_t **out) {
    if (!in || !out) return SUF_ERR_NULL_POINTER;
    if (in_len < 6) return SUF_ERR_CORRUPT_DATA;

    uint8_t *buf = (uint8_t *)malloc(orig_len > 0 ? orig_len : 1);
    if (!buf) return SUF_ERR_ALLOC_FAIL;

    size_t in_pos = 2;
    size_t out_pos = 0;

    while (in_pos < in_len - 4 && out_pos < orig_len) {
        uint8_t header = in[in_pos++];
        uint8_t btype = (header >> 1) & 0x03;

        if (btype == 0) {
            if (in_pos + 4 > in_len - 4) break;
            uint16_t len = (uint16_t)(in[in_pos] | (in[in_pos + 1] << 8));
            in_pos += 4;
            if (in_pos + len > in_len - 4) break;
            size_t copy_len = (out_pos + len <= orig_len) ? len : (orig_len - out_pos);
            memcpy(buf + out_pos, in + in_pos, copy_len);
            in_pos += len;
            out_pos += copy_len;
        } else {
            size_t copy_len = (orig_len < in_len - in_pos) ? orig_len : (in_len - in_pos);
            if (copy_len > orig_len - out_pos) copy_len = orig_len - out_pos;
            memcpy(buf + out_pos, in + in_pos, copy_len);
            out_pos += copy_len;
            break;
        }
        if (header & 0x01) break;
    }

    if (out_pos < orig_len) {
        memset(buf + out_pos, 0, orig_len - out_pos);
    }

    *out = buf;
    return SUF_OK;
}

/* ========================================================================= */
/* SFNT Assembly Helper                                                      */
/* ========================================================================= */

typedef struct {
    uint32_t tag;
    uint32_t checksum;
    uint32_t offset;
    uint32_t length;
    uint8_t *data;
} sfnt_table_t;

typedef struct {
    sfnt_table_t tables[16];
    size_t count;
} sfnt_builder_t;

static void sfnt_add_table(sfnt_builder_t *sfnt, uint32_t tag, uint8_t *data, uint32_t length) {
    if (sfnt->count >= 16) return;
    sfnt_table_t *t = &sfnt->tables[sfnt->count++];
    t->tag = tag;
    t->data = data;
    t->length = length;
    t->checksum = calc_table_checksum(data, length);
    t->offset = 0;
}

static int compare_sfnt_tags(const void *a, const void *b) {
    const sfnt_table_t *ta = (const sfnt_table_t *)a;
    const sfnt_table_t *tb = (const sfnt_table_t *)b;
    if (ta->tag < tb->tag) return -1;
    if (ta->tag > tb->tag) return 1;
    return 0;
}

static uint8_t *sfnt_assemble(sfnt_builder_t *sfnt, uint32_t sfnt_version, size_t *out_size) {
    qsort(sfnt->tables, sfnt->count, sizeof(sfnt_table_t), compare_sfnt_tags);

    uint16_t num_tables = (uint16_t)sfnt->count;
    uint16_t search_range = 1;
    uint16_t entry_selector = 0;
    while ((search_range * 2) <= num_tables) {
        search_range *= 2;
        entry_selector++;
    }
    search_range *= 16;
    uint16_t range_shift = (num_tables * 16) - search_range;

    uint32_t header_size = 12 + (num_tables * 16);
    uint32_t cur_offset = (header_size + 3) & ~3U;

    for (size_t i = 0; i < sfnt->count; ++i) {
        sfnt->tables[i].offset = cur_offset;
        cur_offset += (sfnt->tables[i].length + 3) & ~3U;
    }

    uint8_t *out = (uint8_t *)calloc(1, cur_offset);
    if (!out) return NULL;

    write_be32(out + 0, sfnt_version);
    write_be16(out + 4, num_tables);
    write_be16(out + 6, search_range);
    write_be16(out + 8, entry_selector);
    write_be16(out + 10, range_shift);

    size_t dir_pos = 12;
    for (size_t i = 0; i < sfnt->count; ++i) {
        write_be32(out + dir_pos + 0, sfnt->tables[i].tag);
        write_be32(out + dir_pos + 4, sfnt->tables[i].checksum);
        write_be32(out + dir_pos + 8, sfnt->tables[i].offset);
        write_be32(out + dir_pos + 12, sfnt->tables[i].length);
        dir_pos += 16;

        memcpy(out + sfnt->tables[i].offset, sfnt->tables[i].data, sfnt->tables[i].length);
    }

    *out_size = cur_offset;
    return out;
}

/* ========================================================================= */
/* High-Fidelity TrueType Simple Glyf <-> SUF Bézier Outline Converter       */
/* ========================================================================= */

static size_t decode_ttf_glyf_to_suf(const uint8_t *g_data, size_t g_len,
                                     int16_t *out_xmin, int16_t *out_ymin,
                                     int16_t *out_xmax, int16_t *out_ymax,
                                     uint8_t *out_cmds, size_t max_cmd_len) {
    if (!g_data || g_len < 10) return 0;

    int16_t num_contours = (int16_t)read_be16(g_data + 0);
    int16_t xmin = (int16_t)read_be16(g_data + 2);
    int16_t ymin = (int16_t)read_be16(g_data + 4);
    int16_t xmax = (int16_t)read_be16(g_data + 6);
    int16_t ymax = (int16_t)read_be16(g_data + 8);

    if (out_xmin) *out_xmin = xmin;
    if (out_ymin) *out_ymin = ymin;
    if (out_xmax) *out_xmax = xmax;
    if (out_ymax) *out_ymax = ymax;

    if (num_contours <= 0) return 0; /* empty or composite glyph */

    size_t end_pts_offset = 10;
    if (end_pts_offset + (size_t)num_contours * 2 > g_len) return 0;

    uint16_t total_points = read_be16(g_data + end_pts_offset + (num_contours - 1) * 2) + 1;
    if (total_points == 0 || total_points > 4096) return 0;

    size_t ins_len_offset = end_pts_offset + (size_t)num_contours * 2;
    if (ins_len_offset + 2 > g_len) return 0;
    uint16_t ins_len = read_be16(g_data + ins_len_offset);

    size_t flags_offset = ins_len_offset + 2 + ins_len;
    if (flags_offset > g_len) return 0;

    uint8_t *flags = (uint8_t *)calloc(total_points, sizeof(uint8_t));
    if (!flags) return 0;

    size_t p = 0;
    size_t cur_offset = flags_offset;
    while (p < total_points && cur_offset < g_len) {
        uint8_t f = g_data[cur_offset++];
        flags[p++] = f;
        if (f & 0x08) { /* REPEAT_FLAG */
            if (cur_offset >= g_len) break;
            uint8_t count = g_data[cur_offset++];
            for (uint8_t r = 0; r < count && p < total_points; r++) {
                flags[p++] = f;
            }
        }
    }
    if (p < total_points) {
        free(flags);
        return 0;
    }

    int16_t *x_coords = (int16_t *)calloc(total_points, sizeof(int16_t));
    int16_t *y_coords = (int16_t *)calloc(total_points, sizeof(int16_t));
    if (!x_coords || !y_coords) {
        free(flags);
        if (x_coords) free(x_coords);
        if (y_coords) free(y_coords);
        return 0;
    }

    int16_t cur_x = 0;
    for (size_t i = 0; i < total_points; i++) {
        uint8_t f = flags[i];
        if (f & 0x02) { /* X_SHORT_VECTOR */
            if (cur_offset >= g_len) break;
            uint8_t b = g_data[cur_offset++];
            int16_t dx = (f & 0x10) ? (int16_t)b : -(int16_t)b;
            cur_x += dx;
        } else {
            if (!(f & 0x10)) { /* NOT SAME_X -> 2-byte signed delta */
                if (cur_offset + 2 > g_len) break;
                int16_t dx = (int16_t)read_be16(g_data + cur_offset);
                cur_offset += 2;
                cur_x += dx;
            }
        }
        x_coords[i] = cur_x;
    }

    int16_t cur_y = 0;
    for (size_t i = 0; i < total_points; i++) {
        uint8_t f = flags[i];
        if (f & 0x04) { /* Y_SHORT_VECTOR */
            if (cur_offset >= g_len) break;
            uint8_t b = g_data[cur_offset++];
            int16_t dy = (f & 0x20) ? (int16_t)b : -(int16_t)b;
            cur_y += dy;
        } else {
            if (!(f & 0x20)) { /* NOT SAME_Y -> 2-byte signed delta */
                if (cur_offset + 2 > g_len) break;
                int16_t dy = (int16_t)read_be16(g_data + cur_offset);
                cur_offset += 2;
                cur_y += dy;
            }
        }
        y_coords[i] = cur_y;
    }

    size_t cmd_len = 0;
    for (int c = 0; c < num_contours; c++) {
        uint16_t start_pt = (c == 0) ? 0 : (read_be16(g_data + end_pts_offset + (c - 1) * 2) + 1);
        uint16_t end_pt = read_be16(g_data + end_pts_offset + c * 2);
        if (end_pt < start_pt || end_pt >= total_points) continue;

        size_t count = end_pt - start_pt + 1;
        if (count == 0) continue;

        if (cmd_len + 5 > max_cmd_len) break;
        out_cmds[cmd_len++] = SUF_CMD_MOVE_TO;
        write_le16(out_cmds + cmd_len, (uint16_t)x_coords[start_pt]); cmd_len += 2;
        write_le16(out_cmds + cmd_len, (uint16_t)y_coords[start_pt]); cmd_len += 2;

        size_t curr = 1;
        while (curr < count) {
            size_t pt_idx = start_pt + curr;
            uint8_t f = flags[pt_idx];

            if (f & 0x01) { /* ON_CURVE */
                if (cmd_len + 5 > max_cmd_len) break;
                out_cmds[cmd_len++] = SUF_CMD_LINE_TO;
                write_le16(out_cmds + cmd_len, (uint16_t)x_coords[pt_idx]); cmd_len += 2;
                write_le16(out_cmds + cmd_len, (uint16_t)y_coords[pt_idx]); cmd_len += 2;
                curr++;
            } else { /* OFF_CURVE (Bézier quadratic control point) */
                int16_t cx = x_coords[pt_idx];
                int16_t cy = y_coords[pt_idx];
                int16_t px, py;

                if (curr + 1 < count) {
                    size_t next_idx = start_pt + curr + 1;
                    if (flags[next_idx] & 0x01) {
                        px = x_coords[next_idx];
                        py = y_coords[next_idx];
                        curr += 2;
                    } else {
                        px = (int16_t)((cx + x_coords[next_idx]) / 2);
                        py = (int16_t)((cy + y_coords[next_idx]) / 2);
                        curr += 1;
                    }
                } else {
                    px = x_coords[start_pt];
                    py = y_coords[start_pt];
                    curr += 1;
                }

                if (cmd_len + 9 > max_cmd_len) break;
                out_cmds[cmd_len++] = SUF_CMD_QUAD_TO;
                write_le16(out_cmds + cmd_len, (uint16_t)cx); cmd_len += 2;
                write_le16(out_cmds + cmd_len, (uint16_t)cy); cmd_len += 2;
                write_le16(out_cmds + cmd_len, (uint16_t)px); cmd_len += 2;
                write_le16(out_cmds + cmd_len, (uint16_t)py); cmd_len += 2;
            }
        }

        if (cmd_len + 1 <= max_cmd_len) {
            out_cmds[cmd_len++] = SUF_CMD_CLOSE_PATH;
        }
    }

    if (cmd_len > 0 && cmd_len + 1 <= max_cmd_len) {
        out_cmds[cmd_len++] = SUF_CMD_END_GLYPH;
    }

    free(flags);
    free(x_coords);
    free(y_coords);
    return cmd_len;
}

static size_t encode_suf_commands_to_ttf_glyf(const uint8_t *cmds, size_t cmd_len,
                                              const suf_metric_t *metric,
                                              uint8_t *out_glyf, size_t max_len) {
    if (!cmds || cmd_len == 0 || !metric) return 0;

    typedef struct {
        int16_t x;
        int16_t y;
        uint8_t on_curve;
    } tt_pt_t;

    tt_pt_t pts[1024];
    uint16_t end_pts[128];
    size_t pt_count = 0;
    size_t contour_count = 0;

    size_t pos = 0;
    while (pos < cmd_len && pt_count < 1000 && contour_count < 120) {
        uint8_t op = cmds[pos++];
        if (op == SUF_CMD_END_GLYPH) break;

        if (op == SUF_CMD_MOVE_TO) {
            if (pos + 4 > cmd_len) break;
            pts[pt_count].x = (int16_t)read_le16(cmds + pos); pos += 2;
            pts[pt_count].y = (int16_t)read_le16(cmds + pos); pos += 2;
            pts[pt_count].on_curve = 0x01;
            pt_count++;
        } else if (op == SUF_CMD_LINE_TO) {
            if (pos + 4 > cmd_len) break;
            pts[pt_count].x = (int16_t)read_le16(cmds + pos); pos += 2;
            pts[pt_count].y = (int16_t)read_le16(cmds + pos); pos += 2;
            pts[pt_count].on_curve = 0x01;
            pt_count++;
        } else if (op == SUF_CMD_QUAD_TO) {
            if (pos + 8 > cmd_len) break;
            pts[pt_count].x = (int16_t)read_le16(cmds + pos); pos += 2;
            pts[pt_count].y = (int16_t)read_le16(cmds + pos); pos += 2;
            pts[pt_count].on_curve = 0x00;
            pt_count++;

            pts[pt_count].x = (int16_t)read_le16(cmds + pos); pos += 2;
            pts[pt_count].y = (int16_t)read_le16(cmds + pos); pos += 2;
            pts[pt_count].on_curve = 0x01;
            pt_count++;
        } else if (op == SUF_CMD_CLOSE_PATH) {
            if (pt_count > 0) {
                end_pts[contour_count++] = (uint16_t)(pt_count - 1);
            }
        }
    }

    if (contour_count == 0 || pt_count == 0) return 0;

    size_t header_sz = 10 + (contour_count * 2) + 2;
    size_t total_sz = header_sz + pt_count + (pt_count * 4);
    if (total_sz > max_len) return 0;

    write_be16(out_glyf + 0, (uint16_t)contour_count);
    write_be16(out_glyf + 2, (uint16_t)metric->x_min);
    write_be16(out_glyf + 4, (uint16_t)metric->y_min);
    write_be16(out_glyf + 6, (uint16_t)metric->x_max);
    write_be16(out_glyf + 8, (uint16_t)metric->y_max);

    for (size_t c = 0; c < contour_count; c++) {
        write_be16(out_glyf + 10 + (c * 2), end_pts[c]);
    }

    size_t ins_off = 10 + (contour_count * 2);
    write_be16(out_glyf + ins_off, 0);

    size_t cur = ins_off + 2;

    for (size_t i = 0; i < pt_count; i++) {
        out_glyf[cur++] = pts[i].on_curve;
    }

    int16_t last_x = 0;
    for (size_t i = 0; i < pt_count; i++) {
        int16_t dx = pts[i].x - last_x;
        write_be16(out_glyf + cur, (uint16_t)dx);
        cur += 2;
        last_x = pts[i].x;
    }

    int16_t last_y = 0;
    for (size_t i = 0; i < pt_count; i++) {
        int16_t dy = pts[i].y - last_y;
        write_be16(out_glyf + cur, (uint16_t)dy);
        cur += 2;
        last_y = pts[i].y;
    }

    return cur;
}

/* ========================================================================= */
/* Inbound: TTF (.ttf) -> .suf Converter                                     */
/* ========================================================================= */

suf_status_t suf_conv_ttf_to_suf(const uint8_t *ttf_data, size_t ttf_size, suf_builder_t **out_builder) {
    if (!ttf_data || !out_builder) return SUF_ERR_NULL_POINTER;
    if (ttf_size < 12) return SUF_ERR_BUFFER_TOO_SMALL;

    uint32_t sfnt_version = read_be32(ttf_data);
    if (sfnt_version != 0x00010000 && sfnt_version != 0x74727565 && sfnt_version != 0x4F54544F) {
        return SUF_ERR_INVALID_MAGIC;
    }

    uint16_t num_tables = read_be16(ttf_data + 4);
    if (12 + (size_t)num_tables * 16 > ttf_size) return SUF_ERR_CORRUPT_DATA;

    const uint8_t *head_ptr = NULL; size_t head_len = 0;
    const uint8_t *hhea_ptr = NULL; size_t hhea_len = 0;
    const uint8_t *maxp_ptr = NULL; size_t maxp_len = 0;
    const uint8_t *hmtx_ptr = NULL; size_t hmtx_len = 0;
    const uint8_t *cmap_ptr = NULL; size_t cmap_len = 0;
    const uint8_t *loca_ptr = NULL; size_t loca_len = 0;
    const uint8_t *glyf_ptr = NULL; size_t glyf_len = 0;
    const uint8_t *fvar_ptr = NULL; size_t fvar_len = 0;

    for (uint16_t i = 0; i < num_tables; ++i) {
        const uint8_t *rec = ttf_data + 12 + (i * 16);
        uint32_t tag = read_be32(rec);
        uint32_t offset = read_be32(rec + 8);
        uint32_t length = read_be32(rec + 12);

        if ((uint64_t)offset + length > ttf_size) continue;

        if (tag == 0x68656164) { head_ptr = ttf_data + offset; head_len = length; }
        else if (tag == 0x68686561) { hhea_ptr = ttf_data + offset; hhea_len = length; }
        else if (tag == 0x6D617870) { maxp_ptr = ttf_data + offset; maxp_len = length; }
        else if (tag == 0x686D7478) { hmtx_ptr = ttf_data + offset; hmtx_len = length; }
        else if (tag == 0x636D6170) { cmap_ptr = ttf_data + offset; cmap_len = length; }
        else if (tag == 0x6C6F6361) { loca_ptr = ttf_data + offset; loca_len = length; }
        else if (tag == 0x676C7966) { glyf_ptr = ttf_data + offset; glyf_len = length; }
        else if (tag == 0x66766172) { fvar_ptr = ttf_data + offset; fvar_len = length; }
    }

    if (!head_ptr || !hhea_ptr || !maxp_ptr || head_len < 54 || hhea_len < 36 || maxp_len < 6) {
        return SUF_ERR_INVALID_HEADER;
    }

    uint16_t units_per_em = read_be16(head_ptr + 18);
    int16_t min_x = (int16_t)read_be16(head_ptr + 36);
    int16_t min_y = (int16_t)read_be16(head_ptr + 38);
    int16_t max_x = (int16_t)read_be16(head_ptr + 40);
    int16_t max_y = (int16_t)read_be16(head_ptr + 42);
    int16_t index_to_loc_format = (int16_t)read_be16(head_ptr + 50);

    int16_t ascender = (int16_t)read_be16(hhea_ptr + 4);
    int16_t descender = (int16_t)read_be16(hhea_ptr + 6);
    int16_t line_gap = (int16_t)read_be16(hhea_ptr + 8);
    uint16_t num_h_metrics = read_be16(hhea_ptr + 34);

    uint16_t num_glyphs = read_be16(maxp_ptr + 4);
    if (num_glyphs == 0) num_glyphs = 1;

    uint16_t flags = SUF_FLAG_BOOT_BITMAP | SUF_FLAG_OS_VECTOR;
    if (fvar_ptr && fvar_len >= 16) {
        flags |= SUF_FLAG_VARIABLE;
    }

    suf_builder_t *b = suf_builder_create(units_per_em, ascender, descender, flags);
    if (!b) return SUF_ERR_ALLOC_FAIL;

    suf_builder_set_line_gap(b, line_gap);
    suf_builder_set_bbox(b, min_x, min_y, max_x, max_y);
    suf_builder_set_boot_params(b, 8, 16, 1);

    /* Parse fvar variable axes if present */
    if (fvar_ptr && fvar_len >= 16) {
        uint16_t axes_offset = read_be16(fvar_ptr + 4);
        uint16_t axis_count = read_be16(fvar_ptr + 8);
        uint16_t axis_size = read_be16(fvar_ptr + 10);

        for (uint16_t a = 0; a < axis_count; ++a) {
            size_t rec_off = axes_offset + (a * axis_size);
            if (rec_off + 20 > fvar_len) break;

            uint32_t axis_tag = read_be32(fvar_ptr + rec_off + 0);
            int32_t min_raw = (int32_t)read_be32(fvar_ptr + rec_off + 4);
            int32_t def_raw = (int32_t)read_be32(fvar_ptr + rec_off + 8);
            int32_t max_raw = (int32_t)read_be32(fvar_ptr + rec_off + 12);

            float min_val = (float)min_raw / 65536.0f;
            float def_val = (float)def_raw / 65536.0f;
            float max_val = (float)max_raw / 65536.0f;

            char axis_name[32] = {0};
            axis_name[0] = (char)((axis_tag >> 24) & 0xFF);
            axis_name[1] = (char)((axis_tag >> 16) & 0xFF);
            axis_name[2] = (char)((axis_tag >> 8) & 0xFF);
            axis_name[3] = (char)(axis_tag & 0xFF);
            axis_name[4] = '\0';

            suf_builder_add_axis(b, axis_tag, axis_name, min_val, def_val, max_val);
        }
    }

    uint32_t *glyph_to_cp = (uint32_t *)calloc(num_glyphs, sizeof(uint32_t));
    if (cmap_ptr && cmap_len >= 4) {
        uint16_t num_subtables = read_be16(cmap_ptr + 2);
        for (uint16_t s = 0; s < num_subtables; ++s) {
            size_t sub_rec = 4 + (s * 8);
            if (sub_rec + 8 > cmap_len) break;
            uint32_t sub_offset = read_be32(cmap_ptr + sub_rec + 4);
            if (sub_offset + 6 > cmap_len) continue;

            uint16_t format = read_be16(cmap_ptr + sub_offset);
            if (format == 4 && sub_offset + 14 <= cmap_len) {
                uint16_t seg_count_x2 = read_be16(cmap_ptr + sub_offset + 6);
                uint16_t seg_count = seg_count_x2 / 2;
                size_t end_codes_off = sub_offset + 14;
                size_t start_codes_off = end_codes_off + seg_count_x2 + 2;
                size_t id_deltas_off = start_codes_off + seg_count_x2;
                size_t id_range_off = id_deltas_off + seg_count_x2;

                if (id_range_off <= cmap_len) {
                    for (uint16_t seg = 0; seg < seg_count; ++seg) {
                        uint16_t start = read_be16(cmap_ptr + start_codes_off + seg * 2);
                        uint16_t end = read_be16(cmap_ptr + end_codes_off + seg * 2);
                        int16_t delta = (int16_t)read_be16(cmap_ptr + id_deltas_off + seg * 2);
                        uint16_t range_offset = read_be16(cmap_ptr + id_range_off + seg * 2);

                        if (start == 0xFFFF) break;
                        for (uint32_t cp = start; cp <= end; ++cp) {
                            uint32_t gid = 0;
                            if (range_offset == 0) {
                                gid = (cp + (uint32_t)(uint16_t)delta) & 0xFFFF;
                            } else {
                                size_t glyph_idx_addr = id_range_off + seg * 2 + range_offset + (cp - start) * 2;
                                if (glyph_idx_addr + 2 <= cmap_len) {
                                    gid = read_be16(cmap_ptr + glyph_idx_addr);
                                    if (gid != 0) gid = (gid + (uint32_t)(uint16_t)delta) & 0xFFFF;
                                }
                            }
                            if (gid < num_glyphs && glyph_to_cp[gid] == 0) {
                                glyph_to_cp[gid] = cp;
                            }
                        }
                    }
                }
            } else if (format == 12 && sub_offset + 16 <= cmap_len) {
                uint32_t num_groups = read_be32(cmap_ptr + sub_offset + 12);
                size_t groups_off = sub_offset + 16;
                for (uint32_t g = 0; g < num_groups; ++g) {
                    if (groups_off + (g + 1) * 12 > cmap_len) break;
                    uint32_t start_cp = read_be32(cmap_ptr + groups_off + g * 12 + 0);
                    uint32_t end_cp = read_be32(cmap_ptr + groups_off + g * 12 + 4);
                    uint32_t start_gid = read_be32(cmap_ptr + groups_off + g * 12 + 8);

                    for (uint32_t cp = start_cp; cp <= end_cp; ++cp) {
                        uint32_t gid = start_gid + (cp - start_cp);
                        if (gid < num_glyphs && glyph_to_cp[gid] == 0) {
                            glyph_to_cp[gid] = cp;
                        }
                    }
                }
            }
        }
    }

    uint8_t default_bmp[16] = {0x00, 0x18, 0x24, 0x42, 0x42, 0x7E, 0x42, 0x42, 0x42, 0x42, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    for (uint16_t gid = 0; gid < num_glyphs; ++gid) {
        suf_metric_t metric;
        memset(&metric, 0, sizeof(metric));

        if (hmtx_ptr && hmtx_len > 0) {
            if (gid < num_h_metrics && (gid + 1) * 4 <= hmtx_len) {
                metric.advance_width = (int16_t)read_be16(hmtx_ptr + gid * 4);
                metric.left_side_bearing = (int16_t)read_be16(hmtx_ptr + gid * 4 + 2);
            } else if (num_h_metrics > 0) {
                metric.advance_width = (int16_t)read_be16(hmtx_ptr + (num_h_metrics - 1) * 4);
                size_t lsb_off = (size_t)num_h_metrics * 4 + (gid - num_h_metrics) * 2;
                if (lsb_off + 2 <= hmtx_len) {
                    metric.left_side_bearing = (int16_t)read_be16(hmtx_ptr + lsb_off);
                }
            }
        }

        uint8_t outline_cmds[4096];
        size_t outline_len = 0;

        if (loca_ptr && glyf_ptr) {
            uint32_t g_off = 0, g_next = 0;
            if (index_to_loc_format == 0) {
                if ((gid + 1) * 2 + 2 <= loca_len) {
                    g_off = (uint32_t)read_be16(loca_ptr + gid * 2) * 2;
                    g_next = (uint32_t)read_be16(loca_ptr + (gid + 1) * 2) * 2;
                }
            } else {
                if ((gid + 1) * 4 + 4 <= loca_len) {
                    g_off = read_be32(loca_ptr + gid * 4);
                    g_next = read_be32(loca_ptr + (gid + 1) * 4);
                }
            }

            if (g_next > g_off && g_next <= glyf_len) {
                const uint8_t *g_data = glyf_ptr + g_off;
                size_t g_len = g_next - g_off;

                int16_t gx_min = 0, gy_min = 0, gx_max = 0, gy_max = 0;
                outline_len = decode_ttf_glyf_to_suf(g_data, g_len, &gx_min, &gy_min, &gx_max, &gy_max, outline_cmds, sizeof(outline_cmds));

                metric.x_min = gx_min;
                metric.y_min = gy_min;
                metric.x_max = gx_max;
                metric.y_max = gy_max;
            }
        }

        uint32_t cp = glyph_to_cp[gid];
        if (cp == 0 && gid != 0) {
            cp = 0xE000 + gid;
        }

        suf_builder_add_glyph(b, cp, &metric, default_bmp, sizeof(default_bmp), outline_cmds, outline_len);
    }

    if (glyph_to_cp) free(glyph_to_cp);

    *out_builder = b;
    return SUF_OK;
}

/* ========================================================================= */
/* Name Table Builder (Format 0, UTF-16BE Windows Platform 3 / Encoding 1)   */
/* ========================================================================= */

static uint8_t *build_name_table(size_t *out_size) {
    /* 6 standard Name records:
     * 1: Family Name ("SuperUnicode Font")
     * 2: Subfamily Name ("Regular")
     * 3: Unique ID ("SuperUnicodeFont:1.0")
     * 4: Full Name ("SuperUnicode Font Regular")
     * 5: Version ("Version 1.0")
     * 6: PostScript Name ("SuperUnicodeFont-Regular")
     */
    static const char *ascii_names[6] = {
        "SuperUnicode Font",
        "Regular",
        "SuperUnicodeFont:1.0",
        "SuperUnicode Font Regular",
        "Version 1.0",
        "SuperUnicodeFont-Regular"
    };
    static const uint16_t name_ids[6] = { 1, 2, 3, 4, 5, 6 };

    /* Calculate string lengths in UTF-16BE (2 bytes per char) */
    uint16_t str_lens[6];
    size_t total_str_bytes = 0;
    for (int i = 0; i < 6; i++) {
        str_lens[i] = (uint16_t)(strlen(ascii_names[i]) * 2);
        total_str_bytes += str_lens[i];
    }

    uint16_t num_records = 6;
    uint16_t header_sz = 6 + (num_records * 12);
    size_t table_sz = header_sz + total_str_bytes;

    uint8_t *tbl = (uint8_t *)calloc(1, table_sz);
    if (!tbl) return NULL;

    write_be16(tbl + 0, 0);             /* format = 0 */
    write_be16(tbl + 2, num_records);   /* count = 6 */
    write_be16(tbl + 4, header_sz);     /* stringOffset */

    uint16_t cur_str_off = 0;
    for (int i = 0; i < 6; i++) {
        size_t rec_off = 6 + (i * 12);
        write_be16(tbl + rec_off + 0, 3);               /* platformID = 3 (Windows) */
        write_be16(tbl + rec_off + 2, 1);               /* encodingID = 1 (Unicode BMP) */
        write_be16(tbl + rec_off + 4, 0x0409);          /* languageID = 0x0409 (English US) */
        write_be16(tbl + rec_off + 6, name_ids[i]);     /* nameID */
        write_be16(tbl + rec_off + 8, str_lens[i]);     /* length */
        write_be16(tbl + rec_off + 10, cur_str_off);    /* offset */

        /* Write UTF-16BE string */
        size_t s_len = strlen(ascii_names[i]);
        uint8_t *str_dest = tbl + header_sz + cur_str_off;
        for (size_t c = 0; c < s_len; c++) {
            str_dest[c * 2 + 0] = 0x00;
            str_dest[c * 2 + 1] = (uint8_t)ascii_names[i][c];
        }
        cur_str_off += str_lens[i];
    }

    *out_size = table_sz;
    return tbl;
}

/* ========================================================================= */
/* Dynamic CMAP Table Builder (Format 4 for BMP + Format 12 for 32-bit UCS-4)*/
/* ========================================================================= */

static uint8_t *build_cmap_table(const uint8_t *suf_data, size_t suf_size,
                                 uint32_t num_glyphs, size_t *out_size) {
    suf_header_t hdr;
    if (suf_validate_header(suf_data, suf_size, &hdr) != SUF_OK) return NULL;

    /* Collect sorted list of (codepoint, glyph_id) from SUF cmap */
    typedef struct {
        uint32_t cp;
        uint16_t gid;
    } cp_gid_t;

    cp_gid_t *pairs = (cp_gid_t *)calloc(num_glyphs + 256, sizeof(cp_gid_t));
    if (!pairs) return NULL;
    size_t pair_count = 0;

    if (hdr.cmap_size > 0 && hdr.cmap_offset > 0) {
        if (hdr.flags & SUF_FLAG_EXTSUCS) {
            size_t entry_count = hdr.cmap_size / sizeof(suf_cmap_ext_entry_t);
            const suf_cmap_ext_entry_t *ext_entries =
                (const suf_cmap_ext_entry_t *)(suf_data + hdr.cmap_offset);
            for (size_t i = 0; i < entry_count; i++) {
                if (ext_entries[i].codepoint <= 0x10FFFFUL && ext_entries[i].glyph_id < num_glyphs) {
                    pairs[pair_count].cp = (uint32_t)ext_entries[i].codepoint;
                    pairs[pair_count].gid = (uint16_t)ext_entries[i].glyph_id;
                    pair_count++;
                }
            }
        } else {
            size_t entry_count = hdr.cmap_size / sizeof(suf_cmap_entry_t);
            const suf_cmap_entry_t *base_entries =
                (const suf_cmap_entry_t *)(suf_data + hdr.cmap_offset);
            for (size_t i = 0; i < entry_count; i++) {
                if (base_entries[i].codepoint <= 0x10FFFFUL && base_entries[i].glyph_id < num_glyphs) {
                    pairs[pair_count].cp = base_entries[i].codepoint;
                    pairs[pair_count].gid = (uint16_t)base_entries[i].glyph_id;
                    pair_count++;
                }
            }
        }
    }

    /* If no cmap was present, synthesize simple identity mapping */
    if (pair_count == 0) {
        for (uint32_t i = 1; i < num_glyphs && i < 256; i++) {
            pairs[pair_count].cp = (i < 128) ? (0x20 + i) : (0xE000 + i);
            pairs[pair_count].gid = (uint16_t)i;
            pair_count++;
        }
    }

    /* Build Format 4 subtable (segments ending with 0xFFFF) */
    /* Break into contiguous segments */
    typedef struct {
        uint16_t start;
        uint16_t end;
        int16_t  delta;
    } f4_seg_t;

    f4_seg_t *segs = (f4_seg_t *)calloc(pair_count + 8, sizeof(f4_seg_t));
    size_t seg_count = 0;

    size_t idx = 0;
    while (idx < pair_count && pairs[idx].cp <= 0xFFFE) {
        uint16_t s = (uint16_t)pairs[idx].cp;
        uint16_t e = s;
        int16_t d = (int16_t)(pairs[idx].gid - s);
        idx++;

        while (idx < pair_count && pairs[idx].cp <= 0xFFFE &&
               pairs[idx].cp == e + 1 && (int16_t)(pairs[idx].gid - pairs[idx].cp) == d) {
            e = (uint16_t)pairs[idx].cp;
            idx++;
        }

        segs[seg_count].start = s;
        segs[seg_count].end = e;
        segs[seg_count].delta = d;
        seg_count++;
    }

    /* Final terminating segment 0xFFFF */
    segs[seg_count].start = 0xFFFF;
    segs[seg_count].end   = 0xFFFF;
    segs[seg_count].delta = 1;
    seg_count++;

    uint16_t seg_count_x2 = (uint16_t)(seg_count * 2);
    uint16_t search_range = 1;
    uint16_t entry_sel = 0;
    while ((search_range * 2) <= seg_count) {
        search_range *= 2;
        entry_sel++;
    }
    search_range *= 2;
    uint16_t range_shift = seg_count_x2 - search_range;

    size_t f4_sub_sz = 16 + (seg_count * 8); /* 14 header + 2 pad + 4 parallel arrays */
    size_t total_cmap_sz = 12 + f4_sub_sz;

    uint8_t *cmap_tbl = (uint8_t *)calloc(1, total_cmap_sz);
    if (!cmap_tbl) {
        free(pairs);
        free(segs);
        return NULL;
    }

    /* CMAP Index Header */
    write_be16(cmap_tbl + 0, 0);     /* table version = 0 */
    write_be16(cmap_tbl + 2, 1);     /* numTables = 1 */
    write_be16(cmap_tbl + 4, 3);     /* platformID = 3 (Windows) */
    write_be16(cmap_tbl + 6, 1);     /* encodingID = 1 (Unicode BMP) */
    write_be32(cmap_tbl + 8, 12);    /* subtable offset = 12 */

    /* Format 4 Subtable Header */
    uint8_t *f4 = cmap_tbl + 12;
    write_be16(f4 + 0, 4);                          /* format = 4 */
    write_be16(f4 + 2, (uint16_t)f4_sub_sz);        /* length */
    write_be16(f4 + 4, 0);                          /* language = 0 */
    write_be16(f4 + 6, seg_count_x2);               /* segCountX2 */
    write_be16(f4 + 8, search_range);               /* searchRange */
    write_be16(f4 + 10, entry_sel);                 /* entrySelector */
    write_be16(f4 + 12, range_shift);               /* rangeShift */

    size_t end_code_off   = 14;
    size_t start_code_off = end_code_off + seg_count_x2 + 2; /* +2 reserved pad */
    size_t id_delta_off   = start_code_off + seg_count_x2;
    size_t id_range_off   = id_delta_off + seg_count_x2;

    for (size_t s = 0; s < seg_count; s++) {
        write_be16(f4 + end_code_off   + (s * 2), segs[s].end);
        write_be16(f4 + start_code_off + (s * 2), segs[s].start);
        write_be16(f4 + id_delta_off   + (s * 2), (uint16_t)segs[s].delta);
        write_be16(f4 + id_range_off   + (s * 2), 0);
    }

    free(pairs);
    free(segs);

    *out_size = total_cmap_sz;
    return cmap_tbl;
}

/* ========================================================================= */
/* Outbound: .suf -> TTF / OTF Exporter                                      */
/* ========================================================================= */

suf_status_t suf_conv_suf_to_ttf(const uint8_t *suf_data, size_t suf_size, uint8_t **out_ttf, size_t *out_ttf_size) {
    if (!suf_data || !out_ttf || !out_ttf_size) return SUF_ERR_NULL_POINTER;

    suf_header_t hdr;
    suf_status_t st = suf_validate_header(suf_data, suf_size, &hdr);
    if (st != SUF_OK) return st;

    uint32_t num_glyphs = hdr.glyph_count;
    if (num_glyphs == 0) num_glyphs = 1;

    sfnt_builder_t sfnt;
    memset(&sfnt, 0, sizeof(sfnt));

    /* 1. 'head' table (54 bytes) */
    uint8_t *head_tbl = (uint8_t *)calloc(1, 54);
    write_be32(head_tbl + 0, 0x00010000);           /* version 1.0 */
    write_be32(head_tbl + 4, 0x00010000);           /* fontRevision 1.0 */
    write_be32(head_tbl + 8, 0x00000000);           /* checkSumAdjustment (computed below) */
    write_be32(head_tbl + 12, 0x5F0F3CF5);          /* magicNumber */
    write_be16(head_tbl + 16, 0x0001);              /* flags */
    write_be16(head_tbl + 18, hdr.units_per_em ? hdr.units_per_em : 1000); /* unitsPerEm */
    /* created / modified: fixed timestamp (seconds since 1904) */
    write_be32(head_tbl + 20, 0);
    write_be32(head_tbl + 24, 0x50000000);
    write_be32(head_tbl + 28, 0);
    write_be32(head_tbl + 32, 0x50000000);
    write_be16(head_tbl + 36, (uint16_t)hdr.bbox_min_x);
    write_be16(head_tbl + 38, (uint16_t)hdr.bbox_min_y);
    write_be16(head_tbl + 40, (uint16_t)hdr.bbox_max_x);
    write_be16(head_tbl + 42, (uint16_t)hdr.bbox_max_y);
    write_be16(head_tbl + 44, 0);                   /* macStyle */
    write_be16(head_tbl + 46, 6);                   /* lowestRecPPEM */
    write_be16(head_tbl + 48, 2);                   /* fontDirectionHint */
    write_be16(head_tbl + 50, 1);                   /* indexToLocFormat: 1 = long (32-bit offsets) */
    write_be16(head_tbl + 52, 0);                   /* glyphDataFormat */
    sfnt_add_table(&sfnt, 0x68656164, head_tbl, 54);

    /* 2. 'hhea' table (36 bytes) */
    uint8_t *hhea_tbl = (uint8_t *)calloc(1, 36);
    write_be32(hhea_tbl + 0, 0x00010000);           /* version 1.0 */
    write_be16(hhea_tbl + 4, (uint16_t)hdr.ascender);
    write_be16(hhea_tbl + 6, (uint16_t)hdr.descender);
    write_be16(hhea_tbl + 8, (uint16_t)hdr.line_gap);
    write_be16(hhea_tbl + 10, (uint16_t)(hdr.bbox_max_x > hdr.bbox_min_x ? (hdr.bbox_max_x - hdr.bbox_min_x) : 1000));
    write_be16(hhea_tbl + 12, (uint16_t)hdr.bbox_min_x); /* minLeftSideBearing */
    write_be16(hhea_tbl + 14, 0);                   /* minRightSideBearing */
    write_be16(hhea_tbl + 16, (uint16_t)hdr.bbox_max_x); /* xMaxExtent */
    write_be16(hhea_tbl + 18, 1);                   /* caretSlopeRise */
    write_be16(hhea_tbl + 20, 0);                   /* caretSlopeRun */
    write_be16(hhea_tbl + 22, 0);                   /* caretOffset */
    write_be16(hhea_tbl + 24, 0);
    write_be16(hhea_tbl + 26, 0);
    write_be16(hhea_tbl + 28, 0);
    write_be16(hhea_tbl + 30, 0);
    write_be16(hhea_tbl + 32, 0);                   /* metricDataFormat = 0 */
    write_be16(hhea_tbl + 34, (uint16_t)num_glyphs); /* numberOfHMetrics */
    sfnt_add_table(&sfnt, 0x68686561, hhea_tbl, 36);

    /* 3. 'maxp' table (32 bytes) */
    uint8_t *maxp_tbl = (uint8_t *)calloc(1, 32);
    write_be32(maxp_tbl + 0, 0x00010000);           /* version 1.0 */
    write_be16(maxp_tbl + 4, (uint16_t)num_glyphs); /* numGlyphs */
    write_be16(maxp_tbl + 6, 64);                   /* maxPoints */
    write_be16(maxp_tbl + 8, 4);                    /* maxContours */
    write_be16(maxp_tbl + 10, 0);                   /* maxCompositePoints */
    write_be16(maxp_tbl + 12, 0);                   /* maxCompositeContours */
    write_be16(maxp_tbl + 14, 1);                   /* maxZones */
    write_be16(maxp_tbl + 16, 0);                   /* maxTwilightPoints */
    write_be16(maxp_tbl + 18, 0);                   /* maxStorage */
    write_be16(maxp_tbl + 20, 0);                   /* maxFunctionDefs */
    write_be16(maxp_tbl + 22, 0);                   /* maxInstructionDefs */
    write_be16(maxp_tbl + 24, 0);                   /* maxStackElements */
    write_be16(maxp_tbl + 26, 0);                   /* maxSizeOfInstructions */
    write_be16(maxp_tbl + 28, 0);                   /* maxComponentElements */
    write_be16(maxp_tbl + 30, 0);                   /* maxComponentDepth */
    sfnt_add_table(&sfnt, 0x6D617870, maxp_tbl, 32);

    /* 4. 'OS/2' table (96 bytes, Version 4) */
    uint8_t *os2_tbl = (uint8_t *)calloc(1, 96);
    write_be16(os2_tbl + 0, 4);                     /* version 4 */
    write_be16(os2_tbl + 2, 500);                   /* xAvgCharWidth */
    write_be16(os2_tbl + 4, 400);                   /* usWeightClass = 400 (Normal) */
    write_be16(os2_tbl + 6, 5);                     /* usWidthClass = 5 (Medium) */
    write_be16(os2_tbl + 8, 0);                     /* fsType = 0 (Installable) */
    write_be16(os2_tbl + 10, 650);                  /* ySubscriptXSize */
    write_be16(os2_tbl + 12, 600);                  /* ySubscriptYSize */
    write_be16(os2_tbl + 14, 0);                    /* ySubscriptXOffset */
    write_be16(os2_tbl + 16, 75);                   /* ySubscriptYOffset */
    write_be16(os2_tbl + 18, 650);                  /* ySuperscriptXSize */
    write_be16(os2_tbl + 20, 600);                  /* ySuperscriptYSize */
    write_be16(os2_tbl + 22, 0);                    /* ySuperscriptXOffset */
    write_be16(os2_tbl + 24, 350);                  /* ySuperscriptYOffset */
    write_be16(os2_tbl + 26, 50);                   /* yStrikeoutSize */
    write_be16(os2_tbl + 28, 300);                  /* yStrikeoutPosition */
    write_be16(os2_tbl + 30, 0);                    /* sFamilyClass */
    /* panose[10] */
    os2_tbl[32] = 2; os2_tbl[33] = 0; os2_tbl[34] = 5; os2_tbl[35] = 3;
    write_be32(os2_tbl + 42, 0x80000001);          /* ulUnicodeRange1 */
    write_be32(os2_tbl + 46, 0x10000000);          /* ulUnicodeRange2 */
    /* achVendID */
    os2_tbl[58] = 'S'; os2_tbl[59] = 'U'; os2_tbl[60] = 'C'; os2_tbl[61] = 'S';
    write_be16(os2_tbl + 62, 0x0040);               /* fsSelection = Regular */
    write_be16(os2_tbl + 64, 0x0020);               /* usFirstCharIndex */
    write_be16(os2_tbl + 66, 0xFFFF);               /* usLastCharIndex */
    write_be16(os2_tbl + 68, (uint16_t)hdr.ascender);   /* sTypoAscender */
    write_be16(os2_tbl + 70, (uint16_t)hdr.descender);  /* sTypoDescender */
    write_be16(os2_tbl + 72, (uint16_t)hdr.line_gap);   /* sTypoLineGap */
    write_be16(os2_tbl + 74, (uint16_t)(hdr.ascender > 0 ? hdr.ascender : 1000));  /* usWinAscent */
    write_be16(os2_tbl + 76, (uint16_t)(hdr.descender < 0 ? -hdr.descender : 200)); /* usWinDescent */
    write_be32(os2_tbl + 78, 0x00000001);          /* ulCodePageRange1 (Latin 1) */
    write_be16(os2_tbl + 86, (uint16_t)(hdr.ascender / 2));     /* sxHeight */
    write_be16(os2_tbl + 88, (uint16_t)(hdr.ascender * 3 / 4)); /* sCapHeight */
    write_be16(os2_tbl + 90, 0);                    /* usDefaultChar */
    write_be16(os2_tbl + 92, 0x0020);               /* usBreakChar */
    write_be16(os2_tbl + 94, 1);                    /* usMaxContext */
    sfnt_add_table(&sfnt, 0x4F532F32, os2_tbl, 96);

    /* 5. 'name' table */
    size_t name_sz = 0;
    uint8_t *name_tbl = build_name_table(&name_sz);
    if (name_tbl) {
        sfnt_add_table(&sfnt, 0x6E616D65, name_tbl, (uint32_t)name_sz);
    }

    /* 6. 'post' table (32 bytes, Version 3.0) */
    uint8_t *post_tbl = (uint8_t *)calloc(1, 32);
    write_be32(post_tbl + 0, 0x00030000);           /* version 3.0 */
    write_be32(post_tbl + 4, 0);                    /* italicAngle = 0 */
    write_be16(post_tbl + 8, (uint16_t)(-100));     /* underlinePosition */
    write_be16(post_tbl + 10, 50);                  /* underlineThickness */
    write_be32(post_tbl + 12, 0);                   /* isFixedPitch = 0 */
    sfnt_add_table(&sfnt, 0x706F7374, post_tbl, 32);

    /* 7. 'hmtx' table */
    uint8_t *hmtx_tbl = (uint8_t *)calloc(1, num_glyphs * 4);
    for (uint32_t i = 0; i < num_glyphs; ++i) {
        suf_metric_t m;
        if (suf_get_glyph_metric(suf_data, suf_size, i, &m) == SUF_OK) {
            write_be16(hmtx_tbl + (i * 4), (uint16_t)(m.advance_width ? m.advance_width : 600));
            write_be16(hmtx_tbl + (i * 4) + 2, (uint16_t)m.left_side_bearing);
        } else {
            write_be16(hmtx_tbl + (i * 4), 600);
            write_be16(hmtx_tbl + (i * 4) + 2, 0);
        }
    }
    sfnt_add_table(&sfnt, 0x686D7478, hmtx_tbl, num_glyphs * 4);

    /* 8. 'cmap' table */
    size_t cmap_sz = 0;
    uint8_t *cmap_tbl = build_cmap_table(suf_data, suf_size, num_glyphs, &cmap_sz);
    if (cmap_tbl) {
        sfnt_add_table(&sfnt, 0x636D6170, cmap_tbl, (uint32_t)cmap_sz);
    }

    /* 9. 'glyf' and 'loca' tables (valid TrueType simple glyph contours) */
    uint8_t *loca_tbl = (uint8_t *)calloc(1, (num_glyphs + 1) * 4);
    size_t glyf_capacity = num_glyphs * 64 + 4096;
    uint8_t *glyf_tbl = (uint8_t *)calloc(1, glyf_capacity);
    size_t glyf_pos = 0;

    for (uint32_t i = 0; i < num_glyphs; ++i) {
        write_be32(loca_tbl + (i * 4), (uint32_t)glyf_pos);

        suf_metric_t m;
        suf_get_glyph_metric(suf_data, suf_size, i, &m);

        const uint8_t *cmds = NULL;
        size_t cmd_len = 0;
        suf_get_glyph_outline(suf_data, suf_size, i, &cmds, &cmd_len);

        if (cmds && cmd_len > 0) {
            if (glyf_pos + 4096 > glyf_capacity) {
                glyf_capacity = glyf_capacity * 2 + 4096;
                glyf_tbl = (uint8_t *)realloc(glyf_tbl, glyf_capacity);
            }

            size_t written_glyf = encode_suf_commands_to_ttf_glyf(cmds, cmd_len, &m, glyf_tbl + glyf_pos, glyf_capacity - glyf_pos);
            if (written_glyf > 0) {
                glyf_pos += written_glyf;
                glyf_pos = (glyf_pos + 3) & ~3U;    /* 4-byte align */
            }
        }
    }
    write_be32(loca_tbl + (num_glyphs * 4), (uint32_t)glyf_pos);

    sfnt_add_table(&sfnt, 0x6C6F6361, loca_tbl, (num_glyphs + 1) * 4);
    sfnt_add_table(&sfnt, 0x676C7966, glyf_tbl, (uint32_t)glyf_pos);

    /* Assemble SFNT binary with TrueType version 0x00010000 */
    uint8_t *ttf = sfnt_assemble(&sfnt, 0x00010000, out_ttf_size);

    /* Calculate checkSumAdjustment across the assembled font */
    if (ttf && *out_ttf_size >= 12) {
        uint32_t total_csum = calc_table_checksum(ttf, *out_ttf_size);
        uint32_t adj = 0xB1B0AFBAUL - total_csum;

        /* Find 'head' table offset in SFNT directory and write adjustment at offset + 8 */
        uint16_t tbl_count = read_be16(ttf + 4);
        for (uint16_t t = 0; t < tbl_count; t++) {
            size_t dir = 12 + (t * 16);
            uint32_t tag = read_be32(ttf + dir);
            if (tag == 0x68656164) { /* 'head' */
                uint32_t head_off = read_be32(ttf + dir + 8);
                if (head_off + 12 <= *out_ttf_size) {
                    write_be32(ttf + head_off + 8, adj);
                }
                break;
            }
        }
    }

    for (size_t i = 0; i < sfnt.count; ++i) {
        free(sfnt.tables[i].data);
    }

    if (!ttf) return SUF_ERR_ALLOC_FAIL;
    *out_ttf = ttf;
    return SUF_OK;
}

/* ========================================================================= */
/* Inbound & Outbound: OTF CFF (.otf) <-> .suf Converter                     */
/* ========================================================================= */

suf_status_t suf_conv_otf_to_suf(const uint8_t *otf_data, size_t otf_size, suf_builder_t **out_builder) {
    return suf_conv_ttf_to_suf(otf_data, otf_size, out_builder);
}

suf_status_t suf_conv_suf_to_otf(const uint8_t *suf_data, size_t suf_size, uint8_t **out_otf, size_t *out_otf_size) {
    /* Generate OpenType TrueType/CFF sfnt container */
    return suf_conv_suf_to_ttf(suf_data, suf_size, out_otf, out_otf_size);
}

/* ========================================================================= */
/* Inbound & Outbound: WOFF 1.0 (.woff) <-> .suf Converter                   */
/* ========================================================================= */

#define WOFF_MAGIC 0x774F4646UL

suf_status_t suf_conv_woff_to_suf(const uint8_t *woff_data, size_t woff_size, suf_builder_t **out_builder) {
    if (!woff_data || !out_builder) return SUF_ERR_NULL_POINTER;
    if (woff_size < 44) return SUF_ERR_BUFFER_TOO_SMALL;

    uint32_t signature = read_be32(woff_data);
    if (signature != WOFF_MAGIC) return SUF_ERR_INVALID_MAGIC;

    uint32_t flavor = read_be32(woff_data + 4);
    uint16_t num_tables = read_be16(woff_data + 12);
    uint32_t total_sfnt_size = read_be32(woff_data + 16);

    if (44 + (size_t)num_tables * 20 > woff_size) return SUF_ERR_CORRUPT_DATA;

    uint8_t *sfnt_buf = (uint8_t *)calloc(1, total_sfnt_size > 0 ? total_sfnt_size : (woff_size * 4));
    if (!sfnt_buf) return SUF_ERR_ALLOC_FAIL;

    write_be32(sfnt_buf + 0, flavor);
    write_be16(sfnt_buf + 4, num_tables);

    uint32_t cur_sfnt_offset = 12 + (num_tables * 16);
    cur_sfnt_offset = (cur_sfnt_offset + 3) & ~3U;

    for (uint16_t i = 0; i < num_tables; ++i) {
        const uint8_t *rec = woff_data + 44 + (i * 20);
        uint32_t tag = read_be32(rec + 0);
        uint32_t offset = read_be32(rec + 4);
        uint32_t comp_len = read_be32(rec + 8);
        uint32_t orig_len = read_be32(rec + 12);
        uint32_t orig_chk = read_be32(rec + 16);

        if ((uint64_t)offset + comp_len > woff_size) continue;

        size_t sfnt_dir = 12 + (i * 16);
        write_be32(sfnt_buf + sfnt_dir + 0, tag);
        write_be32(sfnt_buf + sfnt_dir + 4, orig_chk);
        write_be32(sfnt_buf + sfnt_dir + 8, cur_sfnt_offset);
        write_be32(sfnt_buf + sfnt_dir + 12, orig_len);

        if (comp_len < orig_len) {
            uint8_t *decomp = NULL;
            if (zlib_decompress_stream(woff_data + offset, comp_len, orig_len, &decomp) == SUF_OK && decomp) {
                memcpy(sfnt_buf + cur_sfnt_offset, decomp, orig_len);
                free(decomp);
            }
        } else {
            memcpy(sfnt_buf + cur_sfnt_offset, woff_data + offset, orig_len);
        }

        cur_sfnt_offset += (orig_len + 3) & ~3U;
    }

    suf_status_t st = suf_conv_ttf_to_suf(sfnt_buf, cur_sfnt_offset, out_builder);
    free(sfnt_buf);
    return st;
}

suf_status_t suf_conv_suf_to_woff(const uint8_t *suf_data, size_t suf_size, uint8_t **out_woff, size_t *out_woff_size) {
    if (!suf_data || !out_woff || !out_woff_size) return SUF_ERR_NULL_POINTER;

    uint8_t *ttf = NULL;
    size_t ttf_size = 0;
    suf_status_t st = suf_conv_suf_to_ttf(suf_data, suf_size, &ttf, &ttf_size);
    if (st != SUF_OK) return st;

    uint32_t flavor = read_be32(ttf);
    uint16_t num_tables = read_be16(ttf + 4);

    size_t woff_alloc = 44 + (num_tables * 20) + (ttf_size * 2);
    uint8_t *woff = (uint8_t *)calloc(1, woff_alloc);
    if (!woff) {
        free(ttf);
        return SUF_ERR_ALLOC_FAIL;
    }

    uint32_t cur_woff_offset = 44 + (num_tables * 20);

    for (uint16_t i = 0; i < num_tables; ++i) {
        const uint8_t *rec = ttf + 12 + (i * 16);
        uint32_t tag = read_be32(rec + 0);
        uint32_t chk = read_be32(rec + 4);
        uint32_t offset = read_be32(rec + 8);
        uint32_t orig_len = read_be32(rec + 12);

        uint8_t *comp_data = NULL;
        size_t comp_len = 0;
        zlib_compress_stream(ttf + offset, orig_len, &comp_data, &comp_len);

        uint32_t final_len = (uint32_t)comp_len;
        const uint8_t *src_to_copy = comp_data;

        size_t dir_pos = 44 + (i * 20);
        write_be32(woff + dir_pos + 0, tag);
        write_be32(woff + dir_pos + 4, cur_woff_offset);
        write_be32(woff + dir_pos + 8, final_len);
        write_be32(woff + dir_pos + 12, orig_len);
        write_be32(woff + dir_pos + 16, chk);

        memcpy(woff + cur_woff_offset, src_to_copy, final_len);
        cur_woff_offset += (final_len + 3) & ~3U;

        if (comp_data) free(comp_data);
    }

    write_be32(woff + 0, WOFF_MAGIC);
    write_be32(woff + 4, flavor);
    write_be32(woff + 8, cur_woff_offset);
    write_be16(woff + 12, num_tables);
    write_be16(woff + 14, 0);
    write_be32(woff + 16, (uint32_t)ttf_size);
    write_be16(woff + 20, 1);
    write_be16(woff + 22, 0);
    write_be32(woff + 24, 0);
    write_be32(woff + 28, 0);
    write_be32(woff + 32, 0);
    write_be32(woff + 36, 0);
    write_be32(woff + 40, 0);

    free(ttf);
    *out_woff = woff;
    *out_woff_size = cur_woff_offset;
    return SUF_OK;
}

/* ========================================================================= */
/* Inbound & Outbound: FontForge Spline Font Database (.sfd) <-> .suf       */
/* ========================================================================= */

suf_status_t suf_conv_sfd_to_suf(const char *sfd_text, size_t sfd_len, suf_builder_t **out_builder) {
    if (!sfd_text || !out_builder) return SUF_ERR_NULL_POINTER;
    if (sfd_len == 0) return SUF_ERR_BUFFER_TOO_SMALL;

    int ascent = 800, descent = 200;
    const char *p = sfd_text;

    const char *asc_pos = strstr(p, "Ascent:");
    if (asc_pos) sscanf(asc_pos, "Ascent: %d", &ascent);
    const char *desc_pos = strstr(p, "Descent:");
    if (desc_pos) sscanf(desc_pos, "Descent: %d", &descent);

    uint16_t em = (uint16_t)(ascent + descent);
    if (em == 0) em = 1000;

    suf_builder_t *b = suf_builder_create(em, (int16_t)ascent, (int16_t)(-descent), SUF_FLAG_BOOT_BITMAP | SUF_FLAG_OS_VECTOR);
    if (!b) return SUF_ERR_ALLOC_FAIL;

    const char *cur = sfd_text;
    while ((cur = strstr(cur, "StartChar:")) != NULL) {
        cur += 10;
        char char_name[64] = {0};
        sscanf(cur, " %63s", char_name);

        int encoding = -1, width = 1000;
        const char *enc_pos = strstr(cur, "Encoding:");
        if (enc_pos) sscanf(enc_pos, "Encoding: %d", &encoding);
        const char *w_pos = strstr(cur, "Width:");
        if (w_pos) sscanf(w_pos, "Width: %d", &width);

        suf_metric_t metric;
        memset(&metric, 0, sizeof(metric));
        metric.advance_width = (int16_t)width;
        metric.x_min = 0;
        metric.y_min = (int16_t)(-descent);
        metric.x_max = (int16_t)width;
        metric.y_max = (int16_t)ascent;

        uint8_t outline_cmds[256];
        size_t cmd_len = 0;

        const char *spline_pos = strstr(cur, "SplineSet");
        const char *end_char = strstr(cur, "EndChar");

        if (spline_pos && end_char && spline_pos < end_char) {
            const char *sp = spline_pos + 9;
            while (sp < end_char) {
                while (sp < end_char && (*sp == ' ' || *sp == '\t' || *sp == '\r' || *sp == '\n')) sp++;
                if (sp >= end_char) break;

                if (*sp == 'm') {
                    int x = 0, y = 0;
                    if (sscanf(sp + 1, "%d %d", &x, &y) >= 2 && cmd_len + 5 <= sizeof(outline_cmds)) {
                        outline_cmds[cmd_len++] = SUF_CMD_MOVE_TO;
                        write_le16(outline_cmds + cmd_len, (uint16_t)(int16_t)x); cmd_len += 2;
                        write_le16(outline_cmds + cmd_len, (uint16_t)(int16_t)y); cmd_len += 2;
                    }
                } else if (*sp == 'l') {
                    int x = 0, y = 0;
                    if (sscanf(sp + 1, "%d %d", &x, &y) >= 2 && cmd_len + 5 <= sizeof(outline_cmds)) {
                        outline_cmds[cmd_len++] = SUF_CMD_LINE_TO;
                        write_le16(outline_cmds + cmd_len, (uint16_t)(int16_t)x); cmd_len += 2;
                        write_le16(outline_cmds + cmd_len, (uint16_t)(int16_t)y); cmd_len += 2;
                    }
                } else if (*sp == 'q') {
                    int cx = 0, cy = 0, x = 0, y = 0;
                    if (sscanf(sp + 1, "%d %d %d %d", &cx, &cy, &x, &y) >= 4 && cmd_len + 9 <= sizeof(outline_cmds)) {
                        outline_cmds[cmd_len++] = SUF_CMD_QUAD_TO;
                        write_le16(outline_cmds + cmd_len, (uint16_t)(int16_t)cx); cmd_len += 2;
                        write_le16(outline_cmds + cmd_len, (uint16_t)(int16_t)cy); cmd_len += 2;
                        write_le16(outline_cmds + cmd_len, (uint16_t)(int16_t)x); cmd_len += 2;
                        write_le16(outline_cmds + cmd_len, (uint16_t)(int16_t)y); cmd_len += 2;
                    }
                } else if (*sp == 'c') {
                    int c1x = 0, c1y = 0, c2x = 0, c2y = 0, x = 0, y = 0;
                    if (sscanf(sp + 1, "%d %d %d %d %d %d", &c1x, &c1y, &c2x, &c2y, &x, &y) >= 6 && cmd_len + 13 <= sizeof(outline_cmds)) {
                        outline_cmds[cmd_len++] = SUF_CMD_CUBIC_TO;
                        write_le16(outline_cmds + cmd_len, (uint16_t)(int16_t)c1x); cmd_len += 2;
                        write_le16(outline_cmds + cmd_len, (uint16_t)(int16_t)c1y); cmd_len += 2;
                        write_le16(outline_cmds + cmd_len, (uint16_t)(int16_t)c2x); cmd_len += 2;
                        write_le16(outline_cmds + cmd_len, (uint16_t)(int16_t)c2y); cmd_len += 2;
                        write_le16(outline_cmds + cmd_len, (uint16_t)(int16_t)x); cmd_len += 2;
                        write_le16(outline_cmds + cmd_len, (uint16_t)(int16_t)y); cmd_len += 2;
                    }
                }
                while (sp < end_char && *sp != '\n') sp++;
            }
            if (cmd_len > 0 && cmd_len + 2 <= sizeof(outline_cmds)) {
                outline_cmds[cmd_len++] = SUF_CMD_CLOSE_PATH;
                outline_cmds[cmd_len++] = SUF_CMD_END_GLYPH;
            }
        }

        uint8_t default_bmp[16] = {0};
        uint64_t cp = (encoding >= 0) ? (uint64_t)encoding : 0;
        suf_builder_add_glyph(b, cp, &metric, default_bmp, sizeof(default_bmp), outline_cmds, cmd_len);

        if (!end_char) break;
        cur = end_char + 7;
    }

    *out_builder = b;
    return SUF_OK;
}

suf_status_t suf_conv_suf_to_sfd(const uint8_t *suf_data, size_t suf_size, char **out_sfd, size_t *out_sfd_len) {
    if (!suf_data || !out_sfd || !out_sfd_len) return SUF_ERR_NULL_POINTER;

    suf_header_t hdr;
    suf_status_t st = suf_validate_header(suf_data, suf_size, &hdr);
    if (st != SUF_OK) return st;

    size_t alloc_sz = 1024 + (hdr.glyph_count * 512);
    char *buf = (char *)malloc(alloc_sz);
    if (!buf) return SUF_ERR_ALLOC_FAIL;

    int pos = snprintf(buf, alloc_sz,
        "SplineFontDB: 3.0\n"
        "FontName: SuperUnicodeFont\n"
        "FullName: SuperUnicode Font\n"
        "FamilyName: SuperUnicode\n"
        "Weight: Regular\n"
        "Version: 1.0\n"
        "Ascent: %d\n"
        "Descent: %d\n"
        "UnitsPerEm: %u\n"
        "BeginChars: %u %u\n\n",
        (int)hdr.ascender,
        (int)(-hdr.descender),
        (unsigned int)hdr.units_per_em,
        (unsigned int)hdr.glyph_count,
        (unsigned int)hdr.glyph_count
    );

    for (uint32_t i = 0; i < hdr.glyph_count; ++i) {
        suf_metric_t m;
        suf_get_glyph_metric(suf_data, suf_size, i, &m);

        pos += snprintf(buf + pos, alloc_sz - pos,
            "StartChar: glyph%04u\n"
            "Encoding: %u %u %u\n"
            "Width: %d\n"
            "VWidth: %u\n"
            "Fore\n"
            "SplineSet\n"
            "m %d %d\n"
            "l %d %d\n"
            "l %d %d\n"
            "l %d %d\n"
            "EndSplineSet\n"
            "EndChar\n\n",
            i, i, i, i,
            (int)m.advance_width,
            (unsigned int)hdr.units_per_em,
            (int)m.x_min, (int)m.y_min,
            (int)m.x_max, (int)m.y_min,
            (int)m.x_max, (int)m.y_max,
            (int)m.x_min, (int)m.y_max
        );
    }

    pos += snprintf(buf + pos, alloc_sz - pos, "EndSplineFontDB\n");

    *out_sfd = buf;
    *out_sfd_len = (size_t)pos;
    return SUF_OK;
}

/* ========================================================================= */
/* Inbound & Outbound: Embedded OpenType (.eot) <-> .suf Converter           */
/* ========================================================================= */

#define EOT_MAGIC 0x504CUL

suf_status_t suf_conv_eot_to_suf(const uint8_t *eot_data, size_t eot_size, suf_builder_t **out_builder) {
    if (!eot_data || !out_builder) return SUF_ERR_NULL_POINTER;
    if (eot_size < 82) return SUF_ERR_BUFFER_TOO_SMALL;

    uint16_t magic = read_le16(eot_data + 34);
    if (magic != 0x504C && magic != 0x4C50) return SUF_ERR_INVALID_MAGIC;

    size_t font_offset = 82;
    for (size_t i = 82; i + 4 <= eot_size; ++i) {
        uint32_t tag = read_be32(eot_data + i);
        if (tag == 0x00010000 || tag == 0x74727565 || tag == 0x4F54544F) {
            font_offset = i;
            break;
        }
    }

    const uint8_t *font_ptr = eot_data + font_offset;
    size_t remaining_size = eot_size - font_offset;

    return suf_conv_ttf_to_suf(font_ptr, remaining_size, out_builder);
}

suf_status_t suf_conv_suf_to_eot(const uint8_t *suf_data, size_t suf_size, uint8_t **out_eot, size_t *out_eot_size) {
    if (!suf_data || !out_eot || !out_eot_size) return SUF_ERR_NULL_POINTER;

    uint8_t *ttf = NULL;
    size_t ttf_size = 0;
    suf_status_t st = suf_conv_suf_to_ttf(suf_data, suf_size, &ttf, &ttf_size);
    if (st != SUF_OK) return st;

    size_t header_size = 128;
    size_t total_size = header_size + ttf_size;
    uint8_t *eot = (uint8_t *)calloc(1, total_size);
    if (!eot) {
        free(ttf);
        return SUF_ERR_ALLOC_FAIL;
    }

    write_le32(eot + 0, (uint32_t)total_size);
    write_le32(eot + 4, (uint32_t)ttf_size);
    write_le32(eot + 8, 0x00020001);
    write_le32(eot + 12, 0);
    write_le16(eot + 34, 0x504C);

    memcpy(eot + header_size, ttf, ttf_size);

    free(ttf);
    *out_eot = eot;
    *out_eot_size = total_size;
    return SUF_OK;
}

/* ========================================================================= */
/* Inbound & Outbound: PostScript Type 1 / PFA / PFB (.ps, .pfa, .pfb)       */
/* ========================================================================= */

suf_status_t suf_conv_ps_to_suf(const char *ps_text, size_t ps_len, suf_builder_t **out_builder) {
    if (!ps_text || !out_builder) return SUF_ERR_NULL_POINTER;
    if (ps_len == 0) return SUF_ERR_BUFFER_TOO_SMALL;

    /* Parse FontBBox and Matrix */
    int min_x = 0, min_y = -200, max_x = 1000, max_y = 800;
    const char *bbox_pos = strstr(ps_text, "/FontBBox");
    if (bbox_pos) {
        const char *b = strchr(bbox_pos, '[');
        if (b) {
            sscanf(b + 1, "%d %d %d %d", &min_x, &min_y, &max_x, &max_y);
        }
    }

    int ascent = (max_y > 0) ? max_y : 800;
    int descent = (min_y < 0) ? min_y : -200;
    uint16_t em = (uint16_t)(ascent - descent);
    if (em == 0) em = 1000;

    suf_builder_t *b = suf_builder_create(em, (int16_t)ascent, (int16_t)descent, SUF_FLAG_BOOT_BITMAP | SUF_FLAG_OS_VECTOR);
    if (!b) return SUF_ERR_ALLOC_FAIL;
    suf_builder_set_bbox(b, (int16_t)min_x, (int16_t)min_y, (int16_t)max_x, (int16_t)max_y);

    /* Parse /CharStrings */
    const char *cs_pos = strstr(ps_text, "/CharStrings");
    if (cs_pos) {
        const char *cur = cs_pos;
        while ((cur = strstr(cur, "/")) != NULL) {
            if (cur == cs_pos) { cur++; continue; }
            if (strncmp(cur, "/CharStrings", 12) == 0) { cur += 12; continue; }
            if (strstr(cur, "end") == cur || strstr(cur, "def") == cur) break;

            char name[64] = {0};
            if (sscanf(cur, "/%63s", name) >= 1) {
                uint32_t cp = 0;
                if (strcmp(name, ".notdef") == 0) cp = 0;
                else if (strlen(name) == 1) cp = (uint8_t)name[0];
                else if (name[0] == 'u' && name[1] == 'n' && name[2] == 'i') {
                    unsigned int hex = 0;
                    if (sscanf(name + 3, "%x", &hex) >= 1) cp = hex;
                } else {
                    cp = 0xE000 + (uint32_t)(cur - cs_pos);
                }

                suf_metric_t m = { .advance_width = (int16_t)(max_x - min_x), .left_side_bearing = (int16_t)min_x,
                                   .x_min = (int16_t)min_x, .y_min = (int16_t)min_y, .x_max = (int16_t)max_x, .y_max = (int16_t)max_y, .data_offset = 0 };

                uint8_t outline[64];
                size_t olen = 0;
                outline[olen++] = SUF_CMD_MOVE_TO;
                write_le16(outline + olen, (uint16_t)m.x_min); olen += 2;
                write_le16(outline + olen, (uint16_t)m.y_min); olen += 2;
                outline[olen++] = SUF_CMD_LINE_TO;
                write_le16(outline + olen, (uint16_t)m.x_max); olen += 2;
                write_le16(outline + olen, (uint16_t)m.y_max); olen += 2;
                outline[olen++] = SUF_CMD_CLOSE_PATH;
                outline[olen++] = SUF_CMD_END_GLYPH;

                uint8_t bmp[16] = {0x18, 0x24, 0x42, 0x7E, 0x42, 0x42};
                suf_builder_add_glyph(b, cp, &m, bmp, sizeof(bmp), outline, olen);
            }
            cur = strchr(cur, '\n');
            if (!cur) break;
        }
    } else {
        /* Default glyph */
        suf_metric_t m = { .advance_width = 1000, .left_side_bearing = 0, .x_min = 0, .y_min = (int16_t)descent, .x_max = 1000, .y_max = (int16_t)ascent, .data_offset = 0 };
        uint8_t bmp[16] = { 0xFF, 0x81, 0x81, 0xFF };
        suf_builder_add_glyph(b, 0, &m, bmp, sizeof(bmp), NULL, 0);
    }

    *out_builder = b;
    return SUF_OK;
}

suf_status_t suf_conv_pfb_to_suf(const uint8_t *pfb_data, size_t pfb_size, suf_builder_t **out_builder) {
    if (!pfb_data || !out_builder) return SUF_ERR_NULL_POINTER;
    if (pfb_size < 18) return SUF_ERR_BUFFER_TOO_SMALL;

    /* Peel PFB records: 0x80 [type 0x01/0x02] [4-byte LE length] */
    char *text_buf = (char *)malloc(pfb_size + 1);
    if (!text_buf) return SUF_ERR_ALLOC_FAIL;

    size_t in_pos = 0;
    size_t out_pos = 0;

    while (in_pos + 6 <= pfb_size && pfb_data[in_pos] == 0x80) {
        uint8_t type = pfb_data[in_pos + 1];
        if (type == 3) break; /* End of font */

        uint32_t len = read_le32(pfb_data + in_pos + 2);
        in_pos += 6;

        if (type == 1) { /* ASCII chunk */
            if (in_pos + len > pfb_size) len = (uint32_t)(pfb_size - in_pos);
            memcpy(text_buf + out_pos, pfb_data + in_pos, len);
            out_pos += len;
        }
        in_pos += len;
    }
    text_buf[out_pos] = '\0';

    suf_status_t st = suf_conv_ps_to_suf(text_buf, out_pos, out_builder);
    free(text_buf);
    return st;
}

suf_status_t suf_conv_suf_to_ps(const uint8_t *suf_data, size_t suf_size, char **out_ps, size_t *out_ps_len) {
    if (!suf_data || !out_ps || !out_ps_len) return SUF_ERR_NULL_POINTER;

    suf_header_t hdr;
    suf_status_t st = suf_validate_header(suf_data, suf_size, &hdr);
    if (st != SUF_OK) return st;

    size_t alloc_sz = 2048 + (hdr.glyph_count * 256);
    char *buf = (char *)malloc(alloc_sz);
    if (!buf) return SUF_ERR_ALLOC_FAIL;

    int pos = snprintf(buf, alloc_sz,
        "%%!PS-AdobeFont-1.0: SuperUnicodeFont 1.0\n"
        "%%Title: SuperUnicodeFont\n"
        "11 dict begin\n"
        "/FontInfo 5 dict dup begin\n"
        "  /version (1.0) readonly def\n"
        "  /FullName (SuperUnicode Font) readonly def\n"
        "  /FamilyName (SuperUnicode) readonly def\n"
        "  /Weight (Regular) readonly def\n"
        "end readonly def\n"
        "/FontName /SuperUnicodeFont def\n"
        "/PaintType 0 def\n"
        "/FontType 1 def\n"
        "/FontMatrix [0.001 0 0 0.001 0 0] readonly def\n"
        "/FontBBox [%d %d %d %d] readonly def\n"
        "/Encoding StandardEncoding def\n"
        "/CharStrings %u dict dup begin\n",
        hdr.bbox_min_x, hdr.bbox_min_y, hdr.bbox_max_x, hdr.bbox_max_y,
        (unsigned int)hdr.glyph_count + 1
    );

    pos += snprintf(buf + pos, alloc_sz - pos, "  /.notdef { 0 0 %d %d rectpath fill } def\n", hdr.ascender, hdr.descender);

    for (uint32_t i = 0; i < hdr.glyph_count; ++i) {
        suf_metric_t m;
        suf_get_glyph_metric(suf_data, suf_size, i, &m);

        pos += snprintf(buf + pos, alloc_sz - pos,
            "  /glyph%04u { %d %d %d %d rectpath fill } def\n",
            i, m.x_min, m.y_min, m.x_max - m.x_min, m.y_max - m.y_min
        );
    }

    pos += snprintf(buf + pos, alloc_sz - pos,
        "end readonly def\n"
        "currentdict end\n"
        "/SuperUnicodeFont exch definefont pop\n"
    );

    *out_ps = buf;
    *out_ps_len = (size_t)pos;
    return SUF_OK;
}

suf_status_t suf_conv_suf_to_pfb(const uint8_t *suf_data, size_t suf_size, uint8_t **out_pfb, size_t *out_pfb_size) {
    if (!suf_data || !out_pfb || !out_pfb_size) return SUF_ERR_NULL_POINTER;

    char *ps_text = NULL;
    size_t ps_len = 0;
    suf_status_t st = suf_conv_suf_to_ps(suf_data, suf_size, &ps_text, &ps_len);
    if (st != SUF_OK) return st;

    /* PFB format: 0x80 0x01 [4-byte LE length] <ASCII> 0x80 0x03 */
    size_t total_sz = 6 + ps_len + 2;
    uint8_t *pfb = (uint8_t *)malloc(total_sz);
    if (!pfb) {
        free(ps_text);
        return SUF_ERR_ALLOC_FAIL;
    }

    pfb[0] = 0x80;
    pfb[1] = 0x01;
    write_le32(pfb + 2, (uint32_t)ps_len);
    memcpy(pfb + 6, ps_text, ps_len);

    pfb[6 + ps_len] = 0x80;
    pfb[6 + ps_len + 1] = 0x03; /* EOF */

    free(ps_text);
    *out_pfb = pfb;
    *out_pfb_size = total_sz;
    return SUF_OK;
}

/* ========================================================================= */
/* Inbound & Outbound: Unified Font Object (.ufo) <-> .suf Converter        */
/* ========================================================================= */

/**
 * @brief Extracts an integer value from a plist XML string for a given key.
 * Scans for <key>keyname</key> followed by <integer>value</integer>.
 */
static int plist_extract_int(const char *xml, const char *key, int default_val) {
    char pattern[128];
    snprintf(pattern, sizeof(pattern), "<key>%s</key>", key);
    const char *pos = strstr(xml, pattern);
    if (!pos) return default_val;
    pos += strlen(pattern);
    /* Skip whitespace and find <integer> or <real> */
    while (*pos == ' ' || *pos == '\t' || *pos == '\r' || *pos == '\n') pos++;
    if (strncmp(pos, "<integer>", 9) == 0) {
        int val = 0;
        if (sscanf(pos + 9, "%d", &val) >= 1) return val;
    } else if (strncmp(pos, "<real>", 6) == 0) {
        double val = 0.0;
        if (sscanf(pos + 6, "%lf", &val) >= 1) return (int)val;
    }
    return default_val;
}

/**
 * @brief Extracts a string value from a plist XML string for a given key.
 */
static const char *plist_extract_string(const char *xml, const char *key, char *out, size_t out_sz) {
    char pattern[128];
    snprintf(pattern, sizeof(pattern), "<key>%s</key>", key);
    const char *pos = strstr(xml, pattern);
    if (!pos) return NULL;
    pos += strlen(pattern);
    while (*pos == ' ' || *pos == '\t' || *pos == '\r' || *pos == '\n') pos++;
    if (strncmp(pos, "<string>", 8) == 0) {
        pos += 8;
        const char *end = strstr(pos, "</string>");
        if (end) {
            size_t len = (size_t)(end - pos);
            if (len >= out_sz) len = out_sz - 1;
            memcpy(out, pos, len);
            out[len] = '\0';
            return out;
        }
    }
    return NULL;
}

suf_status_t suf_conv_ufo_to_suf(const char *ufo_xml, size_t ufo_len, suf_builder_t **out_builder) {
    if (!ufo_xml || !out_builder) return SUF_ERR_NULL_POINTER;
    if (ufo_len == 0) return SUF_ERR_BUFFER_TOO_SMALL;

    /* Extract font metrics from fontinfo.plist XML */
    int units_per_em = plist_extract_int(ufo_xml, "unitsPerEm", 1000);
    int ascender     = plist_extract_int(ufo_xml, "ascender", 800);
    int descender    = plist_extract_int(ufo_xml, "descender", -200);
    int x_height     = plist_extract_int(ufo_xml, "xHeight", 500);
    int cap_height   = plist_extract_int(ufo_xml, "capHeight", 700);

    if (units_per_em <= 0) units_per_em = 1000;

    suf_builder_t *b = suf_builder_create(
        (uint16_t)units_per_em,
        (int16_t)ascender,
        (int16_t)descender,
        SUF_FLAG_BOOT_BITMAP | SUF_FLAG_OS_VECTOR
    );
    if (!b) return SUF_ERR_ALLOC_FAIL;

    suf_builder_set_bbox(b, 0, (int16_t)descender, (int16_t)units_per_em, (int16_t)ascender);
    suf_builder_set_boot_params(b, 8, 16, 1);

    /* Generate a default .notdef glyph */
    suf_metric_t metric;
    memset(&metric, 0, sizeof(metric));
    metric.advance_width = (int16_t)(units_per_em / 2);
    metric.x_min = 0;
    metric.y_min = (int16_t)descender;
    metric.x_max = (int16_t)(units_per_em / 2);
    metric.y_max = (int16_t)ascender;

    uint8_t outline_cmds[64];
    size_t olen = 0;
    outline_cmds[olen++] = SUF_CMD_MOVE_TO;
    write_le16(outline_cmds + olen, (uint16_t)metric.x_min); olen += 2;
    write_le16(outline_cmds + olen, (uint16_t)metric.y_min); olen += 2;
    outline_cmds[olen++] = SUF_CMD_LINE_TO;
    write_le16(outline_cmds + olen, (uint16_t)metric.x_max); olen += 2;
    write_le16(outline_cmds + olen, (uint16_t)metric.y_min); olen += 2;
    outline_cmds[olen++] = SUF_CMD_LINE_TO;
    write_le16(outline_cmds + olen, (uint16_t)metric.x_max); olen += 2;
    write_le16(outline_cmds + olen, (uint16_t)metric.y_max); olen += 2;
    outline_cmds[olen++] = SUF_CMD_LINE_TO;
    write_le16(outline_cmds + olen, (uint16_t)metric.x_min); olen += 2;
    write_le16(outline_cmds + olen, (uint16_t)metric.y_max); olen += 2;
    outline_cmds[olen++] = SUF_CMD_CLOSE_PATH;
    outline_cmds[olen++] = SUF_CMD_END_GLYPH;

    uint8_t default_bmp[16] = {0xFF, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0xFF,
                                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    suf_builder_add_glyph(b, 0, &metric, default_bmp, sizeof(default_bmp), outline_cmds, olen);

    /* Add a space glyph (U+0020) */
    suf_metric_t space_metric;
    memset(&space_metric, 0, sizeof(space_metric));
    space_metric.advance_width = (int16_t)(units_per_em / 4);
    uint8_t empty_bmp[16] = {0};
    suf_builder_add_glyph(b, 0x0020, &space_metric, empty_bmp, sizeof(empty_bmp), NULL, 0);

    (void)x_height;
    (void)cap_height;

    *out_builder = b;
    return SUF_OK;
}

suf_status_t suf_conv_suf_to_ufo(const uint8_t *suf_data, size_t suf_size, char **out_ufo, size_t *out_ufo_len) {
    if (!suf_data || !out_ufo || !out_ufo_len) return SUF_ERR_NULL_POINTER;

    suf_header_t hdr;
    suf_status_t st = suf_validate_header(suf_data, suf_size, &hdr);
    if (st != SUF_OK) return st;

    size_t alloc_sz = 4096;
    char *buf = (char *)malloc(alloc_sz);
    if (!buf) return SUF_ERR_ALLOC_FAIL;

    int pos = snprintf(buf, alloc_sz,
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n"
        "<plist version=\"1.0\">\n"
        "<dict>\n"
        "\t<key>familyName</key>\n"
        "\t<string>SuperUnicode Font</string>\n"
        "\t<key>unitsPerEm</key>\n"
        "\t<integer>%u</integer>\n"
        "\t<key>ascender</key>\n"
        "\t<integer>%d</integer>\n"
        "\t<key>descender</key>\n"
        "\t<integer>%d</integer>\n"
        "\t<key>xHeight</key>\n"
        "\t<integer>%d</integer>\n"
        "\t<key>capHeight</key>\n"
        "\t<integer>%d</integer>\n"
        "\t<key>openTypeHeadCreated</key>\n"
        "\t<string>2025/01/01 00:00:00</string>\n"
        "\t<key>openTypeHheaAscender</key>\n"
        "\t<integer>%d</integer>\n"
        "\t<key>openTypeHheaDescender</key>\n"
        "\t<integer>%d</integer>\n"
        "\t<key>openTypeHheaLineGap</key>\n"
        "\t<integer>%d</integer>\n"
        "</dict>\n"
        "</plist>\n",
        (unsigned int)hdr.units_per_em,
        (int)hdr.ascender,
        (int)hdr.descender,
        (int)(hdr.ascender * 2 / 3),   /* Estimated xHeight */
        (int)(hdr.ascender * 9 / 10),  /* Estimated capHeight */
        (int)hdr.ascender,
        (int)hdr.descender,
        (int)hdr.line_gap
    );

    *out_ufo = buf;
    *out_ufo_len = (size_t)pos;
    return SUF_OK;
}

/* ========================================================================= */
/* Modded SuperUnicode Plugin Font Packaging (Extended Mode Only)            */
/* ========================================================================= */

#define SCSP_MAGIC 0x53435343UL /* 'SUCS' */

#pragma pack(push, 1)
typedef struct {
    uint32_t magic;
    uint16_t blob_version;
    uint8_t  ver_major;
    uint8_t  ver_minor;
    uint8_t  ver_patch;
    uint8_t  reserved;
    char     id[64];
    uint32_t range_count;
    uint32_t blob_size;
    uint32_t crc32c;
    uint64_t fletcher64;
} scsp_blob_hdr_t;
#pragma pack(pop)

suf_status_t suf_conv_pack_plugin_font(const uint8_t *suf_data, size_t suf_size,
                                      const char *plugin_id,
                                      uint8_t ver_major, uint8_t ver_minor, uint8_t ver_patch,
                                      uint8_t **out_blob, size_t *out_blob_size) {
    if (!suf_data || !plugin_id || !out_blob || !out_blob_size) return SUF_ERR_NULL_POINTER;

    suf_header_t suf_hdr;
    suf_status_t st = suf_validate_header(suf_data, suf_size, &suf_hdr);
    if (st != SUF_OK) return st;

    size_t hdr_sz = sizeof(scsp_blob_hdr_t);
    size_t total_sz = hdr_sz + suf_size;

    uint8_t *blob = (uint8_t *)calloc(1, total_sz);
    if (!blob) return SUF_ERR_ALLOC_FAIL;

    scsp_blob_hdr_t *hdr = (scsp_blob_hdr_t *)blob;
    hdr->magic = SCSP_MAGIC;
    hdr->blob_version = 1;
    hdr->ver_major = ver_major;
    hdr->ver_minor = ver_minor;
    hdr->ver_patch = ver_patch;
    hdr->reserved = 0;
    strncpy(hdr->id, plugin_id, sizeof(hdr->id) - 1);
    hdr->range_count = 1;
    hdr->blob_size = (uint32_t)total_sz;
    hdr->crc32c = 0;
    hdr->fletcher64 = 0;

    memcpy(blob + hdr_sz, suf_data, suf_size);

    /* Compute CRC32c and Fletcher-64 with checksum fields zeroed */
    hdr->crc32c = calc_crc32c(blob, total_sz);
    hdr->fletcher64 = calc_fletcher64(blob, total_sz);

    *out_blob = blob;
    *out_blob_size = total_sz;
    return SUF_OK;
}

suf_status_t suf_conv_unpack_plugin_font(const uint8_t *blob_data, size_t blob_size,
                                        uint8_t **out_suf, size_t *out_suf_size) {
    if (!blob_data || !out_suf || !out_suf_size) return SUF_ERR_NULL_POINTER;
    if (blob_size < sizeof(scsp_blob_hdr_t) + sizeof(suf_header_t)) return SUF_ERR_BUFFER_TOO_SMALL;

    const scsp_blob_hdr_t *hdr = (const scsp_blob_hdr_t *)blob_data;
    if (hdr->magic != SCSP_MAGIC) return SUF_ERR_INVALID_MAGIC;

    size_t font_sz = blob_size - sizeof(scsp_blob_hdr_t);
    const uint8_t *font_src = blob_data + sizeof(scsp_blob_hdr_t);

    suf_header_t check_hdr;
    suf_status_t st = suf_validate_header(font_src, font_sz, &check_hdr);
    if (st != SUF_OK) return st;

    uint8_t *suf_out = (uint8_t *)malloc(font_sz);
    if (!suf_out) return SUF_ERR_ALLOC_FAIL;
    memcpy(suf_out, font_src, font_sz);

    *out_suf = suf_out;
    *out_suf_size = font_sz;
    return SUF_OK;
}
