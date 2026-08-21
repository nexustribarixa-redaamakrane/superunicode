/**
 * @file suf_builder.c
 * @brief Serializer and Builder Implementation for .suf Files
 */

#include "suf/suf_builder.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef struct {
    uint64_t codepoint;
    suf_metric_t metric;
    uint8_t *boot_bitmap;
    size_t bitmap_len;
    uint8_t *outline_cmds;
    size_t outline_len;
} builder_glyph_t;

struct suf_builder {
    uint16_t version;
    uint16_t flags;
    uint16_t units_per_em;
    int16_t  ascender;
    int16_t  descender;
    int16_t  line_gap;
    int16_t  bbox_min_x;
    int16_t  bbox_min_y;
    int16_t  bbox_max_x;
    int16_t  bbox_max_y;
    uint8_t  boot_bitmap_width;
    uint8_t  boot_bitmap_height;
    uint8_t  boot_bitmap_bpp;

    builder_glyph_t *glyphs;
    size_t glyph_count;
    size_t glyph_capacity;

    suf_kern_pair_t *kern_pairs;
    size_t kern_count;
    size_t kern_capacity;

    suf_ligature_t *ligatures;
    size_t lig_count;
    size_t lig_capacity;

    suf_var_axis_t *axes;
    size_t axis_count;
    size_t axis_capacity;

    bool has_plugin_meta;
    suf_plugin_font_meta_t plugin_meta;
};

/* Fast standard CRC-32 implementation */
static uint32_t suf_crc32(const uint8_t *data, size_t length) {
    static uint32_t table[256];
    static bool table_init = false;

    if (!table_init) {
        for (uint32_t i = 0; i < 256; ++i) {
            uint32_t c = i;
            for (int j = 0; j < 8; ++j) {
                if (c & 1) {
                    c = 0xEDB88320UL ^ (c >> 1);
                } else {
                    c = c >> 1;
                }
            }
            table[i] = c;
        }
        table_init = true;
    }

    uint32_t crc = 0xFFFFFFFFUL;
    for (size_t i = 0; i < length; ++i) {
        crc = table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
    }
    return crc ^ 0xFFFFFFFFUL;
}

suf_builder_t *suf_builder_create(uint16_t units_per_em, int16_t ascender, int16_t descender, uint16_t flags) {
    suf_builder_t *b = (suf_builder_t *)calloc(1, sizeof(suf_builder_t));
    if (!b) return NULL;

    b->version = SUF_VERSION_CURRENT;
    b->flags = flags;
    b->units_per_em = units_per_em;
    b->ascender = ascender;
    b->descender = descender;
    b->line_gap = 0;
    b->boot_bitmap_width = 8;
    b->boot_bitmap_height = 16;
    b->boot_bitmap_bpp = 1;

    return b;
}

void suf_builder_set_boot_params(suf_builder_t *b, uint8_t width, uint8_t height, uint8_t bpp) {
    if (!b) return;
    b->boot_bitmap_width = width;
    b->boot_bitmap_height = height;
    b->boot_bitmap_bpp = (bpp == 8) ? 8 : 1;
}

void suf_builder_set_line_gap(suf_builder_t *b, int16_t line_gap) {
    if (!b) return;
    b->line_gap = line_gap;
}

void suf_builder_set_bbox(suf_builder_t *b, int16_t min_x, int16_t min_y, int16_t max_x, int16_t max_y) {
    if (!b) return;
    b->bbox_min_x = min_x;
    b->bbox_min_y = min_y;
    b->bbox_max_x = max_x;
    b->bbox_max_y = max_y;
}

uint32_t suf_builder_add_glyph(suf_builder_t *b, uint64_t codepoint, const suf_metric_t *metric,
                              const uint8_t *boot_bitmap, size_t bitmap_len,
                              const uint8_t *outline_cmds, size_t outline_len) {
    if (!b) return 0;

    if (codepoint > 0x7FFFFFFFUL) {
        b->flags |= SUF_FLAG_EXTSUCS;
    }

    if (b->glyph_count >= b->glyph_capacity) {
        size_t new_cap = (b->glyph_capacity == 0) ? 64 : (b->glyph_capacity * 2);
        builder_glyph_t *new_glyphs = (builder_glyph_t *)realloc(b->glyphs, new_cap * sizeof(builder_glyph_t));
        if (!new_glyphs) return 0;
        b->glyphs = new_glyphs;
        b->glyph_capacity = new_cap;
    }

    uint32_t gid = (uint32_t)b->glyph_count;
    builder_glyph_t *g = &b->glyphs[gid];
    memset(g, 0, sizeof(builder_glyph_t));

    g->codepoint = codepoint;
    if (metric) {
        g->metric = *metric;
    }

    if (boot_bitmap && bitmap_len > 0) {
        b->flags |= SUF_FLAG_BOOT_BITMAP;
        g->boot_bitmap = (uint8_t *)malloc(bitmap_len);
        if (g->boot_bitmap) {
            memcpy(g->boot_bitmap, boot_bitmap, bitmap_len);
            g->bitmap_len = bitmap_len;
        }
    }

    if (outline_cmds && outline_len > 0) {
        b->flags |= SUF_FLAG_OS_VECTOR;
        g->outline_cmds = (uint8_t *)malloc(outline_len);
        if (g->outline_cmds) {
            memcpy(g->outline_cmds, outline_cmds, outline_len);
            g->outline_len = outline_len;
        }
    }

    b->glyph_count++;
    return gid;
}

uint32_t suf_builder_add_bancode_glyph(suf_builder_t *b, uint32_t bancode_cp, const suf_metric_t *metric,
                                      const uint8_t *boot_bitmap, size_t bitmap_len,
                                      const uint8_t *outline_cmds, size_t outline_len) {
    if (!b) return 0;
    b->flags |= SUF_FLAG_BANCODE;
    return suf_builder_add_glyph(b, (uint64_t)bancode_cp, metric, boot_bitmap, bitmap_len, outline_cmds, outline_len);
}

bool suf_builder_add_kerning(suf_builder_t *b, uint32_t left_glyph, uint32_t right_glyph, int16_t kerning) {
    if (!b) return false;
    b->flags |= SUF_FLAG_KERNING;

    if (b->kern_count >= b->kern_capacity) {
        size_t new_cap = (b->kern_capacity == 0) ? 32 : (b->kern_capacity * 2);
        suf_kern_pair_t *new_pairs = (suf_kern_pair_t *)realloc(b->kern_pairs, new_cap * sizeof(suf_kern_pair_t));
        if (!new_pairs) return false;
        b->kern_pairs = new_pairs;
        b->kern_capacity = new_cap;
    }

    suf_kern_pair_t *kp = &b->kern_pairs[b->kern_count++];
    kp->left_glyph = left_glyph;
    kp->right_glyph = right_glyph;
    kp->kerning = kerning;
    kp->reserved = 0;
    return true;
}

bool suf_builder_add_ligature(suf_builder_t *b, uint32_t first_glyph, uint32_t second_glyph, uint32_t replacement_glyph) {
    if (!b) return false;
    b->flags |= SUF_FLAG_LIGATURES;

    if (b->lig_count >= b->lig_capacity) {
        size_t new_cap = (b->lig_capacity == 0) ? 16 : (b->lig_capacity * 2);
        suf_ligature_t *new_ligs = (suf_ligature_t *)realloc(b->ligatures, new_cap * sizeof(suf_ligature_t));
        if (!new_ligs) return false;
        b->ligatures = new_ligs;
        b->lig_capacity = new_cap;
    }

    suf_ligature_t *lig = &b->ligatures[b->lig_count++];
    lig->first_glyph = first_glyph;
    lig->second_glyph = second_glyph;
    lig->replacement_glyph = replacement_glyph;
    lig->reserved = 0;
    return true;
}

bool suf_builder_add_axis(suf_builder_t *b, uint32_t tag, const char *name, float min_val, float def_val, float max_val) {
    if (!b) return false;
    b->flags |= SUF_FLAG_VARIABLE;

    if (b->axis_count >= b->axis_capacity) {
        size_t new_cap = (b->axis_capacity == 0) ? 8 : (b->axis_capacity * 2);
        suf_var_axis_t *new_axes = (suf_var_axis_t *)realloc(b->axes, new_cap * sizeof(suf_var_axis_t));
        if (!new_axes) return false;
        b->axes = new_axes;
        b->axis_capacity = new_cap;
    }

    suf_var_axis_t *ax = &b->axes[b->axis_count++];
    memset(ax, 0, sizeof(suf_var_axis_t));
    ax->tag = tag;
    ax->min_val = min_val;
    ax->def_val = def_val;
    ax->max_val = max_val;
    ax->flags = 0;
    if (name) {
        strncpy(ax->name, name, sizeof(ax->name) - 1);
    }
    return true;
}

bool suf_builder_set_plugin_meta(suf_builder_t *b, const char *plugin_id,
                                uint8_t ver_major, uint8_t ver_minor, uint8_t ver_patch,
                                uint32_t range_count) {
    if (!b || !plugin_id) return false;
    b->flags |= SUF_FLAG_PLUGIN_FONT | SUF_FLAG_EXTSUCS;
    b->has_plugin_meta = true;

    memset(&b->plugin_meta, 0, sizeof(b->plugin_meta));
    strncpy(b->plugin_meta.plugin_id, plugin_id, sizeof(b->plugin_meta.plugin_id) - 1);
    b->plugin_meta.ver_major = ver_major;
    b->plugin_meta.ver_minor = ver_minor;
    b->plugin_meta.ver_patch = ver_patch;
    b->plugin_meta.fs_type = 1; /* OWFS */
    b->plugin_meta.range_count = range_count;
    b->plugin_meta.base_limit = 0x7FFFFFFFUL;
    b->plugin_meta.crc32c = 0;
    b->plugin_meta.fletcher64 = 0;
    return true;
}

static int compare_cmap_base(const void *a, const void *b) {
    const suf_cmap_entry_t *ea = (const suf_cmap_entry_t *)a;
    const suf_cmap_entry_t *eb = (const suf_cmap_entry_t *)b;
    if (ea->codepoint < eb->codepoint) return -1;
    if (ea->codepoint > eb->codepoint) return 1;
    return 0;
}

static int compare_cmap_ext(const void *a, const void *b) {
    const suf_cmap_ext_entry_t *ea = (const suf_cmap_ext_entry_t *)a;
    const suf_cmap_ext_entry_t *eb = (const suf_cmap_ext_entry_t *)b;
    if (ea->codepoint < eb->codepoint) return -1;
    if (ea->codepoint > eb->codepoint) return 1;
    return 0;
}

static int compare_kerning(const void *a, const void *b) {
    const suf_kern_pair_t *ka = (const suf_kern_pair_t *)a;
    const suf_kern_pair_t *kb = (const suf_kern_pair_t *)b;
    if (ka->left_glyph != kb->left_glyph) {
        return (ka->left_glyph < kb->left_glyph) ? -1 : 1;
    }
    if (ka->right_glyph < kb->right_glyph) return -1;
    if (ka->right_glyph > kb->right_glyph) return 1;
    return 0;
}

static int compare_ligatures(const void *a, const void *b) {
    const suf_ligature_t *la = (const suf_ligature_t *)a;
    const suf_ligature_t *lb = (const suf_ligature_t *)b;
    if (la->first_glyph != lb->first_glyph) {
        return (la->first_glyph < lb->first_glyph) ? -1 : 1;
    }
    if (la->second_glyph < lb->second_glyph) return -1;
    if (la->second_glyph > lb->second_glyph) return 1;
    return 0;
}

static inline uint32_t align_16(uint32_t v) {
    return (v + 15U) & ~15U;
}

suf_status_t suf_builder_serialize(const suf_builder_t *b, uint8_t **out_buffer, size_t *out_size) {
    if (!b || !out_buffer || !out_size) return SUF_ERR_NULL_POINTER;

    bool is_ext = (b->flags & SUF_FLAG_EXTSUCS) != 0;
    size_t cmap_entry_size = is_ext ? sizeof(suf_cmap_ext_entry_t) : sizeof(suf_cmap_entry_t);
    uint32_t cmap_size = align_16((uint32_t)(b->glyph_count * cmap_entry_size));
    uint32_t metrics_size = align_16((uint32_t)(b->glyph_count * sizeof(suf_metric_t)));

    uint32_t boot_bitmap_size = 0;
    uint32_t bytes_per_boot_glyph = 0;
    if (b->flags & SUF_FLAG_BOOT_BITMAP) {
        uint32_t row_bytes = (b->boot_bitmap_bpp == 1) ? ((b->boot_bitmap_width + 7) / 8) : (uint32_t)b->boot_bitmap_width;
        bytes_per_boot_glyph = row_bytes * b->boot_bitmap_height;
        boot_bitmap_size = align_16((uint32_t)(b->glyph_count * bytes_per_boot_glyph));
    }

    uint32_t raw_outlines_size = 0;
    if (b->flags & SUF_FLAG_OS_VECTOR) {
        for (size_t i = 0; i < b->glyph_count; ++i) {
            raw_outlines_size += 2 + (uint32_t)b->glyphs[i].outline_len;
        }
    }
    uint32_t outlines_size = align_16(raw_outlines_size);

    uint32_t kerning_size = 0;
    if ((b->flags & SUF_FLAG_KERNING) && b->kern_count > 0) {
        kerning_size = align_16((uint32_t)(b->kern_count * sizeof(suf_kern_pair_t)));
    }
    uint32_t ligatures_size = 0;
    if ((b->flags & SUF_FLAG_LIGATURES) && b->lig_count > 0) {
        ligatures_size = align_16((uint32_t)(b->lig_count * sizeof(suf_ligature_t)));
    }

    uint32_t var_axes_size = 0;
    if ((b->flags & SUF_FLAG_VARIABLE) && b->axis_count > 0) {
        var_axes_size = align_16((uint32_t)(b->axis_count * sizeof(suf_var_axis_t)));
    }

    uint32_t plugin_meta_size = 0;
    if ((b->flags & SUF_FLAG_PLUGIN_FONT) && b->has_plugin_meta) {
        plugin_meta_size = align_16((uint32_t)sizeof(suf_plugin_font_meta_t));
    }

    /* Calculate layout offsets starting after 128-byte header */
    uint32_t cur_offset = sizeof(suf_header_t);

    uint32_t cmap_offset = cur_offset;
    cur_offset += cmap_size;

    uint32_t metrics_offset = cur_offset;
    cur_offset += metrics_size;

    uint32_t boot_bitmap_offset = (boot_bitmap_size > 0) ? cur_offset : 0;
    cur_offset += boot_bitmap_size;

    uint32_t outlines_offset = (outlines_size > 0) ? cur_offset : 0;
    cur_offset += outlines_size;

    uint32_t kerning_offset = (kerning_size > 0) ? cur_offset : 0;
    cur_offset += kerning_size;

    uint32_t ligatures_offset = (ligatures_size > 0) ? cur_offset : 0;
    cur_offset += ligatures_size;

    uint32_t var_axes_offset = (var_axes_size > 0) ? cur_offset : 0;
    cur_offset += var_axes_size;

    uint32_t plugin_meta_offset = (plugin_meta_size > 0) ? cur_offset : 0;
    cur_offset += plugin_meta_size;

    uint32_t total_size = cur_offset;
    uint8_t *buf = (uint8_t *)calloc(1, total_size);
    if (!buf) return SUF_ERR_ALLOC_FAIL;

    /* Build Header */
    suf_header_t hdr;
    memset(&hdr, 0, sizeof(hdr));
    hdr.magic = SUF_MAGIC;
    hdr.version = b->version;
    hdr.flags = b->flags;
    hdr.glyph_count = (uint32_t)b->glyph_count;
    hdr.units_per_em = b->units_per_em;
    hdr.ascender = b->ascender;
    hdr.descender = b->descender;
    hdr.line_gap = b->line_gap;
    hdr.bbox_min_x = b->bbox_min_x;
    hdr.bbox_min_y = b->bbox_min_y;
    hdr.bbox_max_x = b->bbox_max_x;
    hdr.bbox_max_y = b->bbox_max_y;
    hdr.boot_bitmap_width = b->boot_bitmap_width;
    hdr.boot_bitmap_height = b->boot_bitmap_height;
    hdr.boot_bitmap_bpp = b->boot_bitmap_bpp;
    hdr.cmap_offset = cmap_offset;
    hdr.cmap_size = (uint32_t)(b->glyph_count * cmap_entry_size);
    hdr.metrics_offset = metrics_offset;
    hdr.metrics_size = (uint32_t)(b->glyph_count * sizeof(suf_metric_t));
    hdr.boot_bitmap_offset = boot_bitmap_offset;
    hdr.boot_bitmap_size = (b->flags & SUF_FLAG_BOOT_BITMAP) ? (uint32_t)(b->glyph_count * bytes_per_boot_glyph) : 0;
    hdr.outlines_offset = outlines_offset;
    hdr.outlines_size = raw_outlines_size;
    hdr.kerning_offset = kerning_offset;
    hdr.kerning_size = (uint32_t)(b->kern_count * sizeof(suf_kern_pair_t));
    hdr.ligatures_offset = ligatures_offset;
    hdr.ligatures_size = (uint32_t)(b->lig_count * sizeof(suf_ligature_t));
    hdr.var_axes_offset = var_axes_offset;
    hdr.var_axes_size = (uint32_t)(b->axis_count * sizeof(suf_var_axis_t));
    hdr.plugin_meta_offset = plugin_meta_offset;
    hdr.plugin_meta_size = plugin_meta_size;

    /* Build Cmap */
    if (is_ext) {
        suf_cmap_ext_entry_t *ext_entries = (suf_cmap_ext_entry_t *)(buf + cmap_offset);
        for (size_t i = 0; i < b->glyph_count; ++i) {
            ext_entries[i].codepoint = b->glyphs[i].codepoint;
            ext_entries[i].glyph_id = (uint32_t)i;
            ext_entries[i].reserved = 0;
        }
        qsort(ext_entries, b->glyph_count, sizeof(suf_cmap_ext_entry_t), compare_cmap_ext);
    } else {
        suf_cmap_entry_t *entries = (suf_cmap_entry_t *)(buf + cmap_offset);
        for (size_t i = 0; i < b->glyph_count; ++i) {
            entries[i].codepoint = (uint32_t)b->glyphs[i].codepoint;
            entries[i].glyph_id = (uint32_t)i;
        }
        qsort(entries, b->glyph_count, sizeof(suf_cmap_entry_t), compare_cmap_base);
    }

    /* Build Outlines and Metrics */
    suf_metric_t *metrics_dst = (suf_metric_t *)(buf + metrics_offset);
    uint32_t cur_outline_rel_offset = 0;

    for (size_t i = 0; i < b->glyph_count; ++i) {
        metrics_dst[i] = b->glyphs[i].metric;

        if (outlines_offset > 0 && b->glyphs[i].outline_cmds && b->glyphs[i].outline_len > 0) {
            metrics_dst[i].data_offset = cur_outline_rel_offset;
            uint8_t *out_ptr = buf + outlines_offset + cur_outline_rel_offset;
            uint16_t len = (uint16_t)b->glyphs[i].outline_len;
            out_ptr[0] = (uint8_t)(len & 0xFF);
            out_ptr[1] = (uint8_t)((len >> 8) & 0xFF);
            memcpy(out_ptr + 2, b->glyphs[i].outline_cmds, len);
            cur_outline_rel_offset += 2 + len;
        } else {
            metrics_dst[i].data_offset = 0;
        }

        if (boot_bitmap_offset > 0) {
            uint8_t *dst_bmp = buf + boot_bitmap_offset + (i * bytes_per_boot_glyph);
            if (b->glyphs[i].boot_bitmap && b->glyphs[i].bitmap_len > 0) {
                size_t cpy = (b->glyphs[i].bitmap_len < bytes_per_boot_glyph) ? b->glyphs[i].bitmap_len : bytes_per_boot_glyph;
                memcpy(dst_bmp, b->glyphs[i].boot_bitmap, cpy);
            }
        }
    }

    /* Build Kerning */
    if (kerning_offset > 0 && b->kern_count > 0) {
        suf_kern_pair_t *kp_dst = (suf_kern_pair_t *)(buf + kerning_offset);
        memcpy(kp_dst, b->kern_pairs, b->kern_count * sizeof(suf_kern_pair_t));
        qsort(kp_dst, b->kern_count, sizeof(suf_kern_pair_t), compare_kerning);
    }

    /* Build Ligatures */
    if (ligatures_offset > 0 && b->lig_count > 0) {
        suf_ligature_t *lig_dst = (suf_ligature_t *)(buf + ligatures_offset);
        memcpy(lig_dst, b->ligatures, b->lig_count * sizeof(suf_ligature_t));
        qsort(lig_dst, b->lig_count, sizeof(suf_ligature_t), compare_ligatures);
    }

    /* Build Variable Axes */
    if (var_axes_offset > 0 && b->axis_count > 0) {
        suf_var_axis_t *ax_dst = (suf_var_axis_t *)(buf + var_axes_offset);
        memcpy(ax_dst, b->axes, b->axis_count * sizeof(suf_var_axis_t));
    }

    /* Build Plugin Metadata */
    if (plugin_meta_offset > 0 && b->has_plugin_meta) {
        suf_plugin_font_meta_t *pm_dst = (suf_plugin_font_meta_t *)(buf + plugin_meta_offset);
        *pm_dst = b->plugin_meta;
    }

    /* Checksum and header copy */
    memcpy(buf, &hdr, sizeof(hdr));
    uint32_t checksum = suf_crc32(buf + sizeof(suf_header_t), total_size - sizeof(suf_header_t));
    ((suf_header_t *)buf)->checksum = checksum;

    *out_buffer = buf;
    *out_size = total_size;
    return SUF_OK;
}

suf_status_t suf_builder_write_file(const suf_builder_t *b, const char *filepath) {
    if (!b || !filepath) return SUF_ERR_NULL_POINTER;

    uint8_t *buf = NULL;
    size_t sz = 0;
    suf_status_t st = suf_builder_serialize(b, &buf, &sz);
    if (st != SUF_OK) return st;

    FILE *f = fopen(filepath, "wb");
    if (!f) {
        free(buf);
        return SUF_ERR_CORRUPT_DATA;
    }

    size_t written = fwrite(buf, 1, sz, f);
    fclose(f);
    free(buf);

    return (written == sz) ? SUF_OK : SUF_ERR_CORRUPT_DATA;
}

void suf_builder_free(suf_builder_t *b) {
    if (!b) return;

    if (b->glyphs) {
        for (size_t i = 0; i < b->glyph_count; ++i) {
            if (b->glyphs[i].boot_bitmap) free(b->glyphs[i].boot_bitmap);
            if (b->glyphs[i].outline_cmds) free(b->glyphs[i].outline_cmds);
        }
        free(b->glyphs);
    }

    if (b->kern_pairs) free(b->kern_pairs);
    if (b->ligatures) free(b->ligatures);
    if (b->axes) free(b->axes);

    free(b);
}
