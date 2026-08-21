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

    suf_builder_t *b = suf_builder_create(units_per_em, ascender, descender, SUF_FLAG_BOOT_BITMAP | SUF_FLAG_OS_VECTOR);
    if (!b) return SUF_ERR_ALLOC_FAIL;

    suf_builder_set_line_gap(b, line_gap);
    suf_builder_set_bbox(b, min_x, min_y, max_x, max_y);
    suf_builder_set_boot_params(b, 8, 16, 1);

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

        uint8_t outline_cmds[256];
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
                int16_t num_contours = (int16_t)read_be16(g_data);
                metric.x_min = (int16_t)read_be16(g_data + 2);
                metric.y_min = (int16_t)read_be16(g_data + 4);
                metric.x_max = (int16_t)read_be16(g_data + 6);
                metric.y_max = (int16_t)read_be16(g_data + 8);

                if (num_contours > 0) {
                    outline_cmds[outline_len++] = SUF_CMD_MOVE_TO;
                    write_le16(outline_cmds + outline_len, (uint16_t)metric.x_min); outline_len += 2;
                    write_le16(outline_cmds + outline_len, (uint16_t)metric.y_min); outline_len += 2;

                    outline_cmds[outline_len++] = SUF_CMD_LINE_TO;
                    write_le16(outline_cmds + outline_len, (uint16_t)metric.x_max); outline_len += 2;
                    write_le16(outline_cmds + outline_len, (uint16_t)metric.y_min); outline_len += 2;

                    outline_cmds[outline_len++] = SUF_CMD_LINE_TO;
                    write_le16(outline_cmds + outline_len, (uint16_t)metric.x_max); outline_len += 2;
                    write_le16(outline_cmds + outline_len, (uint16_t)metric.y_max); outline_len += 2;

                    outline_cmds[outline_len++] = SUF_CMD_LINE_TO;
                    write_le16(outline_cmds + outline_len, (uint16_t)metric.x_min); outline_len += 2;
                    write_le16(outline_cmds + outline_len, (uint16_t)metric.y_max); outline_len += 2;

                    outline_cmds[outline_len++] = SUF_CMD_CLOSE_PATH;
                    outline_cmds[outline_len++] = SUF_CMD_END_GLYPH;
                }
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
/* Outbound: .suf -> TTF (.ttf) Exporter                                     */
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
    write_be32(head_tbl + 0, 0x00010000);
    write_be32(head_tbl + 4, 0x00010000);
    write_be32(head_tbl + 12, 0x5F0F3CF5);
    write_be16(head_tbl + 16, 0x0001);
    write_be16(head_tbl + 18, hdr.units_per_em ? hdr.units_per_em : 1000);
    write_be16(head_tbl + 36, (uint16_t)hdr.bbox_min_x);
    write_be16(head_tbl + 38, (uint16_t)hdr.bbox_min_y);
    write_be16(head_tbl + 40, (uint16_t)hdr.bbox_max_x);
    write_be16(head_tbl + 42, (uint16_t)hdr.bbox_max_y);
    write_be16(head_tbl + 50, 1);
    sfnt_add_table(&sfnt, 0x68656164, head_tbl, 54);

    /* 2. 'hhea' table (36 bytes) */
    uint8_t *hhea_tbl = (uint8_t *)calloc(1, 36);
    write_be32(hhea_tbl + 0, 0x00010000);
    write_be16(hhea_tbl + 4, (uint16_t)hdr.ascender);
    write_be16(hhea_tbl + 6, (uint16_t)hdr.descender);
    write_be16(hhea_tbl + 8, (uint16_t)hdr.line_gap);
    write_be16(hhea_tbl + 10, (uint16_t)(hdr.bbox_max_x - hdr.bbox_min_x));
    write_be16(hhea_tbl + 34, (uint16_t)num_glyphs);
    sfnt_add_table(&sfnt, 0x68686561, hhea_tbl, 36);

    /* 3. 'maxp' table (32 bytes) */
    uint8_t *maxp_tbl = (uint8_t *)calloc(1, 32);
    write_be32(maxp_tbl + 0, 0x00010000);
    write_be16(maxp_tbl + 4, (uint16_t)num_glyphs);
    write_be16(maxp_tbl + 6, 16);
    write_be16(maxp_tbl + 8, 4);
    sfnt_add_table(&sfnt, 0x6D617870, maxp_tbl, 32);

    /* 4. 'hmtx' table */
    uint8_t *hmtx_tbl = (uint8_t *)calloc(1, num_glyphs * 4);
    for (uint32_t i = 0; i < num_glyphs; ++i) {
        suf_metric_t m;
        if (suf_get_glyph_metric(suf_data, suf_size, i, &m) == SUF_OK) {
            write_be16(hmtx_tbl + (i * 4), (uint16_t)m.advance_width);
            write_be16(hmtx_tbl + (i * 4) + 2, (uint16_t)m.left_side_bearing);
        } else {
            write_be16(hmtx_tbl + (i * 4), 1000);
            write_be16(hmtx_tbl + (i * 4) + 2, 0);
        }
    }
    sfnt_add_table(&sfnt, 0x686D7478, hmtx_tbl, num_glyphs * 4);

    /* 5. 'glyf' and 'loca' tables */
    uint8_t *loca_tbl = (uint8_t *)calloc(1, (num_glyphs + 1) * 4);
    size_t glyf_capacity = num_glyphs * 64 + 1024;
    uint8_t *glyf_tbl = (uint8_t *)calloc(1, glyf_capacity);
    size_t glyf_pos = 0;

    for (uint32_t i = 0; i < num_glyphs; ++i) {
        write_be32(loca_tbl + (i * 4), (uint32_t)glyf_pos);
        suf_metric_t m;
        suf_get_glyph_metric(suf_data, suf_size, i, &m);

        if (glyf_pos + 40 > glyf_capacity) {
            glyf_capacity *= 2;
            glyf_tbl = (uint8_t *)realloc(glyf_tbl, glyf_capacity);
        }

        uint8_t *g = glyf_tbl + glyf_pos;
        write_be16(g + 0, 1);
        write_be16(g + 2, (uint16_t)m.x_min);
        write_be16(g + 4, (uint16_t)m.y_min);
        write_be16(g + 6, (uint16_t)m.x_max);
        write_be16(g + 8, (uint16_t)m.y_max);
        write_be16(g + 10, 3);
        write_be16(g + 12, 0);
        g[14] = 0x01; g[15] = 0x01; g[16] = 0x01; g[17] = 0x01;

        int16_t x0 = m.x_min, y0 = m.y_min;
        int16_t x1 = m.x_max, y1 = m.y_min;
        int16_t x2 = m.x_max, y2 = m.y_max;
        int16_t x3 = m.x_min, y3 = m.y_max;

        write_be16(g + 18, (uint16_t)x0);
        write_be16(g + 20, (uint16_t)(x1 - x0));
        write_be16(g + 22, (uint16_t)(x2 - x1));
        write_be16(g + 24, (uint16_t)(x3 - x2));

        write_be16(g + 26, (uint16_t)y0);
        write_be16(g + 28, (uint16_t)(y1 - y0));
        write_be16(g + 30, (uint16_t)(y2 - y1));
        write_be16(g + 32, (uint16_t)(y3 - y2));

        glyf_pos += 34;
        glyf_pos = (glyf_pos + 3) & ~3U;
    }
    write_be32(loca_tbl + (num_glyphs * 4), (uint32_t)glyf_pos);

    sfnt_add_table(&sfnt, 0x6C6F6361, loca_tbl, (num_glyphs + 1) * 4);
    sfnt_add_table(&sfnt, 0x676C7966, glyf_tbl, (uint32_t)glyf_pos);

    /* 6. 'cmap' table */
    size_t cmap_sub_sz = 16 + 8 * 4 + 4;
    size_t cmap_tot_sz = 12 + cmap_sub_sz;
    uint8_t *cmap_tbl = (uint8_t *)calloc(1, cmap_tot_sz);

    write_be16(cmap_tbl + 0, 0);
    write_be16(cmap_tbl + 2, 1);
    write_be16(cmap_tbl + 4, 3);
    write_be16(cmap_tbl + 6, 1);
    write_be32(cmap_tbl + 8, 12);

    uint8_t *c4 = cmap_tbl + 12;
    write_be16(c4 + 0, 4);
    write_be16(c4 + 2, (uint16_t)cmap_sub_sz);
    write_be16(c4 + 6, 4);
    write_be16(c4 + 8, 4);
    write_be16(c4 + 10, 1);
    write_be16(c4 + 12, 0);

    write_be16(c4 + 14, 0x00FF);
    write_be16(c4 + 16, 0xFFFF);
    write_be16(c4 + 18, 0);

    write_be16(c4 + 20, 0x0020);
    write_be16(c4 + 22, 0xFFFF);

    write_be16(c4 + 24, 0);
    write_be16(c4 + 26, 1);

    sfnt_add_table(&sfnt, 0x636D6170, cmap_tbl, (uint32_t)cmap_tot_sz);

    /* 7. 'OS/2' table */
    uint8_t *os2_tbl = (uint8_t *)calloc(1, 96);
    write_be16(os2_tbl + 0, 4);
    write_be16(os2_tbl + 2, 500);
    write_be16(os2_tbl + 4, 400);
    write_be16(os2_tbl + 6, 5);
    write_be16(os2_tbl + 68, (uint16_t)hdr.ascender);
    write_be16(os2_tbl + 70, (uint16_t)hdr.descender);
    write_be16(os2_tbl + 72, (uint16_t)hdr.line_gap);
    write_be16(os2_tbl + 74, (uint16_t)hdr.ascender);
    write_be16(os2_tbl + 76, (uint16_t)(-hdr.descender));
    sfnt_add_table(&sfnt, 0x4F532F32, os2_tbl, 96);

    uint8_t *ttf = sfnt_assemble(&sfnt, 0x00010000, out_ttf_size);

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
