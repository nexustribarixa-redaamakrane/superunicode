/**
 * @file test_suf.c
 * @brief Comprehensive Test Suite for SuperUnicode Font (.suf) Engine & Toolchain
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "suf/suf_types.h"
#include "suf/suf_parser.h"
#include "suf/suf_builder.h"
#include "suf/suf_conv.h"

static void test_suf_types_and_alignment(void) {
    printf("[TEST] Verifying SIMD metric alignment, variable axes, and header sizes...\n");
    assert(sizeof(suf_metric_t) == 16);
    assert(sizeof(suf_header_t) == 128);
    assert(sizeof(suf_cmap_entry_t) == 8);
    assert(sizeof(suf_cmap_ext_entry_t) == 16);
    assert(sizeof(suf_kern_pair_t) == 12);
    assert(sizeof(suf_ligature_t) == 16);
    assert(sizeof(suf_var_axis_t) == 48);
    assert(sizeof(suf_plugin_font_meta_t) == 128);
    printf("       -> 16-byte SIMD layout, 128B header, and 48B axis descriptors confirmed.\n");
}

static void test_suf_builder_and_parser(void) {
    printf("[TEST] Testing builder creation, serialization, and freestanding parsing...\n");

    suf_builder_t *b = suf_builder_create(1000, 800, -200, SUF_FLAG_BOOT_BITMAP | SUF_FLAG_OS_VECTOR);
    assert(b != NULL);

    suf_builder_set_line_gap(b, 100);
    suf_builder_set_bbox(b, -50, -200, 1050, 800);
    suf_builder_set_boot_params(b, 8, 16, 1);

    /* Glyph 0: .notdef */
    suf_metric_t m0 = { .advance_width = 500, .left_side_bearing = 0, .x_min = 0, .y_min = -200, .x_max = 500, .y_max = 800, .data_offset = 0 };
    uint8_t bmp0[16] = { 0xFF, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0xFF };
    uint32_t g0 = suf_builder_add_glyph(b, 0, &m0, bmp0, sizeof(bmp0), NULL, 0);
    assert(g0 == 0);

    /* Glyph 1: 'A' (codepoint 0x41) */
    suf_metric_t m1 = { .advance_width = 600, .left_side_bearing = 50, .x_min = 50, .y_min = 0, .x_max = 550, .y_max = 700, .data_offset = 0 };
    uint8_t bmp1[16] = { 0x00, 0x18, 0x24, 0x42, 0x42, 0x7E, 0x42, 0x42, 0x42, 0x42, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    uint8_t outline1[] = {
        SUF_CMD_MOVE_TO, 0x32, 0x00, 0x00, 0x00,
        SUF_CMD_LINE_TO, 0x26, 0x01, 0xBC, 0x02,
        SUF_CMD_LINE_TO, 0x26, 0x02, 0x00, 0x00,
        SUF_CMD_CLOSE_PATH,
        SUF_CMD_END_GLYPH
    };
    uint32_t g1 = suf_builder_add_glyph(b, 0x41, &m1, bmp1, sizeof(bmp1), outline1, sizeof(outline1));
    assert(g1 == 1);

    /* Glyph 2: 'V' (codepoint 0x56) */
    suf_metric_t m2 = { .advance_width = 600, .left_side_bearing = 50, .x_min = 50, .y_min = 0, .x_max = 550, .y_max = 700, .data_offset = 0 };
    uint8_t bmp2[16] = { 0x00, 0x42, 0x42, 0x42, 0x42, 0x24, 0x24, 0x18, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    uint32_t g2 = suf_builder_add_glyph(b, 0x56, &m2, bmp2, sizeof(bmp2), NULL, 0);
    assert(g2 == 2);

    /* Glyph 3: ExtSUCS codepoint (0x100000000ULL) */
    suf_metric_t m3 = { .advance_width = 800, .left_side_bearing = 100, .x_min = 100, .y_min = 0, .x_max = 700, .y_max = 700, .data_offset = 0 };
    uint8_t bmp3[16] = { 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55 };
    uint32_t g3 = suf_builder_add_glyph(b, 0x100000000ULL, &m3, bmp3, sizeof(bmp3), NULL, 0);
    assert(g3 == 3);

    /* Add BANcode fatal crash glyph */
    uint32_t g_bancode = suf_builder_add_bancode_glyph(b, 0x0011A005, &m1, bmp1, sizeof(bmp1), outline1, sizeof(outline1));
    assert(g_bancode == 4);

    /* Add Kerning & Ligature */
    assert(suf_builder_add_kerning(b, g1, g2, -80) == true);
    assert(suf_builder_add_ligature(b, g1, g2, g3) == true);

    /* Add Variable Design Axes (Standard & Custom) */
    assert(suf_builder_add_axis(b, SUF_AXIS_WGHT, "Weight", 100.0f, 400.0f, 900.0f) == true);
    assert(suf_builder_add_axis(b, SUF_AXIS_WDTH, "Width", 50.0f, 100.0f, 200.0f) == true);
    assert(suf_builder_add_axis(b, SUF_AXIS_SLNT, "Slant", -20.0f, 0.0f, 20.0f) == true);
    assert(suf_builder_add_axis(b, SUF_AXIS_ITAL, "Italic", 0.0f, 0.0f, 1.0f) == true);
    assert(suf_builder_add_axis(b, SUF_AXIS_GRAD, "Grade", -200.0f, 0.0f, 200.0f) == true);
    assert(suf_builder_add_axis(b, SUF_AXIS_ROND, "Roundness", 0.0f, 0.0f, 100.0f) == true);
    assert(suf_builder_add_axis(b, SUF_AXIS_OPSZ, "Optical Size", 6.0f, 12.0f, 72.0f) == true);
    assert(suf_builder_add_axis(b, SUF_AXIS_TAG('X','O','P','Q'), "Custom Param", 0.0f, 50.0f, 100.0f) == true);

    /* Set Modded Plugin Font Metadata */
    assert(suf_builder_set_plugin_meta(b, "com.openwindows.font.cyberpunk", 2, 1, 0, 1) == true);

    /* Serialize */
    uint8_t *suf_buf = NULL;
    size_t suf_sz = 0;
    assert(suf_builder_serialize(b, &suf_buf, &suf_sz) == SUF_OK);
    assert(suf_buf != NULL && suf_sz > sizeof(suf_header_t));
    assert(suf_builder_write_file(b, "test_sample.suf") == SUF_OK);
    suf_builder_free(b);


    /* Parse and Validate */
    suf_header_t hdr;
    assert(suf_validate_header(suf_buf, suf_sz, &hdr) == SUF_OK);
    assert(hdr.magic == SUF_MAGIC);
    assert(hdr.glyph_count == 5);
    assert(hdr.flags & SUF_FLAG_EXTSUCS);
    assert(hdr.flags & SUF_FLAG_KERNING);
    assert(hdr.flags & SUF_FLAG_LIGATURES);
    assert(hdr.flags & SUF_FLAG_BANCODE);
    assert(hdr.flags & SUF_FLAG_VARIABLE);
    assert(hdr.flags & SUF_FLAG_PLUGIN_FONT);

    /* Lookups */
    uint32_t gid = 999;
    assert(suf_lookup_glyph_id(suf_buf, suf_sz, 0x41, &gid) == SUF_OK && gid == g1);
    assert(suf_lookup_glyph_id_ext(suf_buf, suf_sz, 0x100000000ULL, &gid) == SUF_OK && gid == g3);

    /* BANcode lookup & Badge rendering */
    uint32_t bgid = 0;
    assert(suf_lookup_bancode_glyph(suf_buf, suf_sz, 0x0011A005, &bgid) == SUF_OK && bgid == g_bancode);
    uint32_t fb[64 * 64] = {0};
    assert(suf_render_bancode_badge(suf_buf, suf_sz, 0x0011A005, fb, 64, 64, 64, 10, 10) == SUF_OK);

    /* Variable Axes Discovery & Metric Interpolation */
    uint32_t axis_count = 0;
    assert(suf_get_axis_count(suf_buf, suf_sz, &axis_count) == SUF_OK && axis_count == 8);
    suf_var_axis_t wght_ax;
    assert(suf_find_axis_by_tag(suf_buf, suf_sz, SUF_AXIS_WGHT, &wght_ax) == SUF_OK);
    assert(strcmp(wght_ax.name, "Weight") == 0);

    float axis_coords[8] = { 800.0f, 150.0f, 0.0f, 0.0f, 0.0f, 0.0f, 12.0f, 50.0f };
    suf_metric_t interp_m;
    assert(suf_interpolate_glyph_metric(suf_buf, suf_sz, g1, axis_coords, 8, &interp_m) == SUF_OK);
    assert(interp_m.advance_width > m1.advance_width); /* Expanded width & weight */

    /* Modded Plugin Metadata Inspection */
    suf_plugin_font_meta_t pm;
    assert(suf_get_plugin_meta(suf_buf, suf_sz, &pm) == SUF_OK);
    assert(strcmp(pm.plugin_id, "com.openwindows.font.cyberpunk") == 0);
    assert(pm.ver_major == 2 && pm.ver_minor == 1);

    /* Plugin Font Packaging (.scsp blob) */
    uint8_t *scsp_blob = NULL;
    size_t blob_sz = 0;
    assert(suf_conv_pack_plugin_font(suf_buf, suf_sz, "com.openwindows.font.cyberpunk", 2, 1, 0, &scsp_blob, &blob_sz) == SUF_OK);
    assert(scsp_blob != NULL && blob_sz > suf_sz);

    uint8_t *unpacked_suf = NULL;
    size_t unpacked_sz = 0;
    assert(suf_conv_unpack_plugin_font(scsp_blob, blob_sz, &unpacked_suf, &unpacked_sz) == SUF_OK);
    assert(unpacked_sz == suf_sz);
    assert(memcmp(unpacked_suf, suf_buf, suf_sz) == 0);
    free(scsp_blob);
    free(unpacked_suf);

    free(suf_buf);
    printf("       -> Builder, parser, BANcode badges, variable axes, and plugin packaging verified.\n");
}

static void test_bidirectional_conversions(void) {
    printf("[TEST] Testing bidirectional conversions (TTF, SFD, WOFF, EOT, OTF, PostScript)...\n");

    suf_builder_t *b = suf_builder_create(1000, 800, -200, SUF_FLAG_BOOT_BITMAP | SUF_FLAG_OS_VECTOR);
    suf_metric_t m = { .advance_width = 500, .left_side_bearing = 0, .x_min = 0, .y_min = 0, .x_max = 500, .y_max = 700, .data_offset = 0 };
    uint8_t bmp[16] = { 0x55, 0xAA, 0x55, 0xAA };
    suf_builder_add_glyph(b, 0, &m, bmp, 16, NULL, 0);
    suf_builder_add_glyph(b, 'A', &m, bmp, 16, NULL, 0);
    suf_builder_add_glyph(b, 'B', &m, bmp, 16, NULL, 0);

    uint8_t *orig_suf = NULL;
    size_t orig_suf_sz = 0;
    assert(suf_builder_serialize(b, &orig_suf, &orig_suf_sz) == SUF_OK);
    suf_builder_free(b);

    /* 1. SUF -> TTF -> SUF */
    uint8_t *ttf_data = NULL;
    size_t ttf_sz = 0;
    assert(suf_conv_suf_to_ttf(orig_suf, orig_suf_sz, &ttf_data, &ttf_sz) == SUF_OK);
    suf_builder_t *ttf_b = NULL;
    assert(suf_conv_ttf_to_suf(ttf_data, ttf_sz, &ttf_b) == SUF_OK);
    suf_builder_free(ttf_b);
    free(ttf_data);

    /* 2. SUF -> SFD -> SUF */
    char *sfd_text = NULL;
    size_t sfd_len = 0;
    assert(suf_conv_suf_to_sfd(orig_suf, orig_suf_sz, &sfd_text, &sfd_len) == SUF_OK);
    suf_builder_t *sfd_b = NULL;
    assert(suf_conv_sfd_to_suf(sfd_text, sfd_len, &sfd_b) == SUF_OK);
    suf_builder_free(sfd_b);
    free(sfd_text);

    /* 3. SUF -> WOFF -> SUF */
    uint8_t *woff_data = NULL;
    size_t woff_sz = 0;
    assert(suf_conv_suf_to_woff(orig_suf, orig_suf_sz, &woff_data, &woff_sz) == SUF_OK);
    suf_builder_t *woff_b = NULL;
    assert(suf_conv_woff_to_suf(woff_data, woff_sz, &woff_b) == SUF_OK);
    suf_builder_free(woff_b);
    free(woff_data);

    /* 4. SUF -> EOT -> SUF */
    uint8_t *eot_data = NULL;
    size_t eot_sz = 0;
    assert(suf_conv_suf_to_eot(orig_suf, orig_suf_sz, &eot_data, &eot_sz) == SUF_OK);
    suf_builder_t *eot_b = NULL;
    assert(suf_conv_eot_to_suf(eot_data, eot_sz, &eot_b) == SUF_OK);
    suf_builder_free(eot_b);
    free(eot_data);

    /* 5. SUF -> OTF -> SUF */
    uint8_t *otf_data = NULL;
    size_t otf_sz = 0;
    assert(suf_conv_suf_to_otf(orig_suf, orig_suf_sz, &otf_data, &otf_sz) == SUF_OK);
    suf_builder_t *otf_b = NULL;
    assert(suf_conv_otf_to_suf(otf_data, otf_sz, &otf_b) == SUF_OK);
    suf_builder_free(otf_b);
    free(otf_data);

    /* 6. SUF -> PostScript Type 1 / PFA -> SUF */
    char *ps_text = NULL;
    size_t ps_len = 0;
    assert(suf_conv_suf_to_ps(orig_suf, orig_suf_sz, &ps_text, &ps_len) == SUF_OK);
    suf_builder_t *ps_b = NULL;
    assert(suf_conv_ps_to_suf(ps_text, ps_len, &ps_b) == SUF_OK);
    suf_builder_free(ps_b);
    free(ps_text);

    /* 7. SUF -> PostScript Binary / PFB -> SUF */
    uint8_t *pfb_data = NULL;
    size_t pfb_sz = 0;
    assert(suf_conv_suf_to_pfb(orig_suf, orig_suf_sz, &pfb_data, &pfb_sz) == SUF_OK);
    suf_builder_t *pfb_b = NULL;
    assert(suf_conv_pfb_to_suf(pfb_data, pfb_sz, &pfb_b) == SUF_OK);
    suf_builder_free(pfb_b);
    free(pfb_data);

    free(orig_suf);
    printf("       -> All bidirectional converters (including PostScript PFA/PFB) passed successfully.\n");
}

int main(void) {
    printf("=================================================================\n");
    printf(" Running SuperUnicode Font (.suf) Engine Test Suite\n");
    printf("=================================================================\n");
    test_suf_types_and_alignment();
    test_suf_builder_and_parser();
    test_bidirectional_conversions();
    printf("=================================================================\n");
    printf("[ALL TESTS PASSED] SuperUnicode Font (.suf) Suite Verified.\n");
    printf("=================================================================\n");
    return 0;
}
