/**
 * Unified Header Coexistence Test
 *
 * This single translation unit includes EVERY public header from all three
 * modules:
 *
 *   - Base SUCS            : superunicode/superunicode.h (+ sucs_trap.h)
 *   - SUTF transform       : sutf/sutf.h (sucs_types, sucs_mode, sutf8/16/4/2)
 *   - SUST serialization   : sust.h (sust16, sustfixed, esust)
 *   - Extended ExtSUCS     : extsucs_types.h, vsutf.h
 *   - Plugin subsystem     : plugin.h, plugin_checksum.h, plugin_stage.h,
 *                            plugin_boot.h, plugin_partition.h
 *   - SUAS architecture    : suas/suas_core.h, suas/suas_sucd.h, suas/suas_sdf.h,
 *                            suas/suas_sucf.h, suas/suas_ssp.h, suas/suas_sts.h,
 *                            suas/suas_sis.h, suas/suas_sncs.h, suas/suas_sip.h,
 *                            suas/suas_ssre.h
 *
 * Prior to the header-dedupe work, the identical constants (SUCS_INVALID_
 * CODEPOINT, SUCS_TRAP_RANGE_MIN/MAX, SUCS_MAX_CODEPOINT, ...), the
 * sucs_char_t typedef, and sucs_is_valid() were each defined in multiple
 * headers; this file would NOT compile. It now must compile cleanly and run.
 */

#include <stdio.h>
/* The coexistence checks use assert() to evaluate the calls themselves, so
 * they must stay active even when the Release build defines NDEBUG. */
#undef NDEBUG
#include <assert.h>
#include "superunicode/superunicode.h" // IWYU pragma: keep
#include "superunicode/sucs_trap.h"     // IWYU pragma: keep
#include "sutf.h"                       // IWYU pragma: keep
#include "sust.h"                       // IWYU pragma: keep
#include "extsucs_types.h"              // IWYU pragma: keep
#include "vsutf.h"                      // IWYU pragma: keep
#include "superunicode_extended/plugin.h"           // IWYU pragma: keep
#include "superunicode_extended/plugin_checksum.h"  // IWYU pragma: keep
#include "superunicode_extended/plugin_stage.h"     // IWYU pragma: keep
#include "superunicode_extended/plugin_boot.h"      // IWYU pragma: keep
#include "superunicode_extended/plugin_partition.h" // IWYU pragma: keep
#include "extsucs_conv.h"               // IWYU pragma: keep
#include "suf/suf_types.h"              // IWYU pragma: keep
#include "suf/suf_parser.h"             // IWYU pragma: keep
#include "suf/suf_conv.h"               // IWYU pragma: keep
#include "suas/suas_core.h"             // IWYU pragma: keep
#include "suas/suas_sucd.h"             // IWYU pragma: keep
#include "suas/suas_sdf.h"              // IWYU pragma: keep
#include "suas/suas_sgw.h"              // IWYU pragma: keep
#include "suas/suas_sbr.h"              // IWYU pragma: keep
#include "suas/suas_sucf.h"             // IWYU pragma: keep
#include "suas/suas_ssp.h"              // IWYU pragma: keep
#include "suas/suas_sts.h"              // IWYU pragma: keep
#include "suas/suas_sis.h"              // IWYU pragma: keep
#include "suas/suas_sncs.h"             // IWYU pragma: keep
#include "suas/suas_sip.h"              // IWYU pragma: keep
#include "suas/suas_ssre.h"             // IWYU pragma: keep
#include "suas/suas_scms.h"             // IWYU pragma: keep
#include "suas/suas_suvtl.h"            // IWYU pragma: keep
#include "suas/suas_suamr.h"            // IWYU pragma: keep
#include "suas/suas_sukemet.h"          // IWYU pragma: keep
#include "suas/suas_sueasd.h"           // IWYU pragma: keep
#include "suas/suas_skto.h"             // IWYU pragma: keep
#include "suas/suas_stim.h"             // IWYU pragma: keep
#include "suts/suts_suca.h"             // IWYU pragma: keep
#include "suts/suts_sre.h"              // IWYU pragma: keep
#include "suts/suts_sldf.h"             // IWYU pragma: keep
#include "suts/suts_sivd.h"             // IWYU pragma: keep
#include "suts/suts_scss.h"             // IWYU pragma: keep
#include "suts/suts_sidn.h"             // IWYU pragma: keep
#include "suts/suts_sueps.h"            // IWYU pragma: keep
#include "suts/suts_susch.h"            // IWYU pragma: keep
#include "suts/suts_suldf.h"            // IWYU pragma: keep
#include "suts/suts_susn.h"             // IWYU pragma: keep
#include "suts/suts_scts.h"             // IWYU pragma: keep
#include "suts/suts_scte.h"             // IWYU pragma: keep
#include "sutr/sutr_sums.h"             // IWYU pragma: keep
#include "sutr/sutr_sucsl.h"            // IWYU pragma: keep
#include "sutr/sutr_sueas.h"            // IWYU pragma: keep
#include "sutr/sutr_sucem.h"            // IWYU pragma: keep
#include "sutr/sutr_sucpm.h"            // IWYU pragma: keep
#include "sutr/sutr_sumnls.h"           // IWYU pragma: keep
#include "sutr/sutr_sucm.h"             // IWYU pragma: keep
#include "sutr/sutr_oktp.h"             // IWYU pragma: keep

int main(void) {

    /* --- Duplicated constants agree across modules --- */
    #ifndef SUTF_MASTER_H
    #error "SUTF_MASTER_H must be defined by sutf.h"
    #endif


    assert(SUCS_MAX_CODEPOINT == 0x7FFFFFFFUL);
    assert(SUCS_INVALID_CODEPOINT == 0x7FFFFFFFUL);
    assert(SUCS_TRAP_RANGE_MIN == SUCS_KERNEL_TRAP_MIN);
    assert(SUCS_TRAP_RANGE_MAX == SUCS_KERNEL_TRAP_MAX);
    assert(SUCS_KERNEL_TRAP_MIN == 0x7FFFFFF0UL);
    assert(SUCS_KERNEL_TRAP_MAX == 0x7FFFFFFEUL);

    /* --- The shared sucs_char_t / sucs_is_valid() coexist --- */
    assert(sucs_is_valid(0x41) == true);
    assert(sucs_is_valid(0x7FFFFFFFUL) == false);   /* sentinel */
    assert(sucs_is_valid(0x7FFFFFF5UL) == false);   /* trap range */
    assert(sucs_is_valid(0x7FFFFFEFUL) == true);

    /* --- ExtSUCS validator distinct and correct --- */
    assert(extsucs_is_valid(0x7FFFFFFFULL) == true);
    assert(extsucs_is_valid(0xFFFFFFFFFFFFFFFFULL) == true);
    assert(extsucs_is_valid(0x7FFFFFF5ULL) == false);   /* inherited trap range */

    /* --- Mode-switch enums from sutf/sucs_mode.h --- */
    assert(SUCS_MODE_BASE == 0);
    assert(SUCS_MODE_EXTENDED == 1);

    /* --- Classifier + trap math work with the shared constants --- */
    assert(sucs_classify_codepoint(0x41) == SUCS_TYPE_UNICODE_COMPAT);
    assert(sucs_classify_codepoint(0x7FFFFFF5UL) == SUCS_TYPE_INVALID);
    assert(sucs_bancode_to_trap(0x0011A080) == 0x7FFFFFF1);

    /* --- Real trap dispatch works from the unified header --- */
    assert(sucs_trap_dispatch(0x0011A005) == false); /* nothing registered */
    assert(sucs_trap_register_handler(14, NULL, NULL) == false); /* NULL handler */
    assert(sucs_trap_unregister_handler(99) == false);

    /* --- An actual SUTF-8 & SUTF-16 round trip using sutf_static --- */
    uint8_t buf[8];
    assert(sutf8_encode_char(0x110000, buf, sizeof(buf)) == 5);
    sucs_char_t cp = 0;
    assert(sutf8_decode_char(buf, 5, &cp) == 5);
    assert(cp == 0x110000);
    uint16_t wbuf[2];
    assert(sutf16_encode_char(0x41, wbuf, 2) == 1);

    /* --- SUST serialization transports coexist (headers + a quick call) --- */
    sucs_ex_char_t ex_decoded = 0;
    assert(sust64_encode(0xDEADBEEFCAFEULL, buf, sizeof(buf)) == 8);
    assert(sust64_decode(buf, 8, &ex_decoded) == 8);
    assert(ex_decoded == 0xDEADBEEFCAFEULL);
    assert(vsutf_encode(0x41ULL, buf, sizeof(buf)) == 1);
    assert(vsutf_decode(buf, 1, &ex_decoded) == 1);
    assert(ex_decoded == 0x41ULL);

    /* e-SUST page constants and structures */
    assert(ESUST_PAGE_SIZE == 4096ULL);
    assert(ESUST_IPC_FRAME_BYTES == 6);

    /* --- Plugin subsystem headers coexist with the encodings --- */
    assert(SUCS_PLUGIN_BLOB_MAGIC == 0x53435343UL);
    assert(SUCS_PLUGIN_BASE_LIMIT == 0x7FFFFFFFULL);
    assert(SUCS_PLUGIN_REBOOT_REQUIRED == 13);
    assert(SUCS_PARTITION_FS_OWFS == 1);
    assert(sucs_plugin_partition_fs_is_valid(SUCS_PARTITION_FS_USFS) == true);
    assert(sucs_plugin_get_pending_count() <= SUCS_PLUGIN_MAX_PENDING);

    sucs_fletcher64_state_t fstate;
    sucs_fletcher64_init(&fstate);

    sucs_plugin_boot_config_t boot_cfg = {0};
    assert(boot_cfg.mounted_count == 0);

    /* --- SuperUnicode Font (.suf) headers & SIMD types coexist --- */
    assert(SUF_MAGIC == 0x53554631UL);
    assert(sizeof(suf_metric_t) == 16);
    assert(sizeof(suf_header_t) == 128);
    assert(sizeof(suf_var_axis_t) == 48);
    assert(sizeof(suf_plugin_font_meta_t) == 128);
    assert(SUF_CMD_MOVE_TO == 0x01);
    assert(SUF_AXIS_WGHT == 0x77676874UL);
    assert(SUF_BANCODE_FATAL_MIN == 0x0011A000UL);
    assert(suf_validate_header(NULL, 0, NULL) == SUF_ERR_NULL_POINTER);

    /* --- SUAS architecture standard (SUAS-001 SDF) headers coexist --- */
    assert(SCP_DIR_LTR == 0x00110101UL);
    assert(SCP_DIR_RTL == 0x00110102UL);
    assert(SCP_DIR_ISOLATE_PUSH == 0x00110104UL);
    assert(SCP_DIR_ISOLATE_POP == 0x00110108UL);
    assert((suas_sucd_bidi(0x0041) & SUCD_BIDI_LTR) != 0);
    {
        suas_sdf_state_t st;
        suts32_framed_t fw;
        size_t cnt = 0;
        suas_sdf_init(&st, SUAS_DIR_LTR);
        assert(suas_sdf_process_codepoint(&st, 0x05D1, &fw, &cnt) == SUAS_SDF_OK);
        assert(cnt == 1);
        assert(SUAS_SDF_FRAMED_DIR(fw) == SUAS_DIR_RTL);
    }

    /* --- SUTS-001 SUCA header coexists; 64-bit extSUCS codepoints -- */
    {
        suts_suca_options_t so;
        suts_suca_options_default(&so);
        sucs_ex_char_t na[] = {0x00120000ULL};
        sucs_ex_char_t nb[] = {0x80000000ULL};
        int cr = 0;
        assert(suts_suca_compare(na,1,nb,1,&so,&cr) == SUTS_SUCA_OK);
        assert(cr < 0); /* native Base sorts before extSUCS plugin */
        assert(suts_suca_implicit_ce(0x0041ULL).l1 != 0);
    }

    /* --- SUTS-002 SRE header coexists; single-pass regex engine --- */
    {
        suts_sre_compiled_dfa_t sre;
        sucs_ex_char_t hay[3];
        size_t m = 0, es = 0, ee = 0;
        assert(suts_sre_compile("a.c", 3, SUTS_SRE_FLAG_NONE, &sre) == SUTS_SRE_OK);
        hay[0] = 'a'; hay[1] = 'b'; hay[2] = 'c';
        assert(suts_sre_search(&sre, hay, 3, 0, &es, &ee) == SUTS_SRE_MATCH);
        assert(es == 0 && ee == 3);
        assert(suts_sre_prop_match(SUTS_SRE_PROP_SCP, SRE_ZONE_SCP_MIN) == true);
        (void)m;
    }

    /* --- SUTS-003 SLDF header coexists; O(1) binary-locale lookup --- */
    {
        const sldf_locale_t* loc;
        sldf_init_registry();
        loc = sldf_resolve_locale("en-US");
        assert(loc != NULL);
        assert(loc->numeric.decimal_separator == (sucs_ex_char_t)0x002E);
        assert(loc->temporal.date_order == SUTS_SLDF_DATE_MDY);
        assert(sldf_resolve_locale("DE-de") != NULL);          /* case-insensitive */
        assert(sldf_resolve_locale("xx-YY") == NULL);
        assert(sldf_plane_of(SLDF_RNUR_MIN) == SUTS_SLDF_PLANE_RNUR);
        assert(sldf_collation_base(sldf_resolve_locale("lumer")) ==
               SLDF_COLLATION_RNUR_SEED + 0x0100u);
    }

    /* --- SUTS-004 SIVD header coexists; O(1) variation resolution --- */
    {
        const sivd_variation_entry_t* ivs;
        const sivd_collection_t* coll;
        uint32_t glyph = 0u;
        sivd_init_registry();
        assert(sivd_resolve_variant(0x82A6ULL, 0xE0134ULL, &glyph));
        assert(glyph == 80u);
        ivs = sivd_lookup(0x4E00ULL, 0xE0102ULL);
        assert(ivs != NULL);
        assert(ivs->resolved_glyph_index == 49u);
        assert(sivd_lookup(0x110000ULL, 0xE0100ULL) == NULL); /* SCP base */
        assert(sivd_is_variation_selector(SUTS_SIVD_VS17_MAX));
        assert(sivd_plane_of(SIVD_RNUR_VS_MIN) == SUTS_SIVD_PLANE_RNUR);
        coll = sivd_collection_of(0x160200ULL, 0x16FF00ULL);
        assert(coll != NULL);
        assert(coll->plane == SUTS_SIVD_PLANE_RNUR);
    }

    /* --- SUTS-005 SCSS header coexists; O(1) homograph resolution ---- */
    {
        scss_security_attr_t attr;
        (void)attr;
        scss_init_registry();
        assert(scss_get_flags(0x0041ULL) == SUTS_SCSS_FLAG_ALLOWED_IDENTIFIER);
        assert(scss_is_allowed_identifier(0x0041ULL));
        assert(scss_get_flags(0x0430ULL) == SUTS_SCSS_FLAG_CONFUSABLE);
        assert(scss_resolve_skeleton(0x0430ULL) == 0x0061ULL);
        assert(scss_confusable_with(0x0430ULL, 0x03B1ULL) == true);
        assert(scss_confusable_with(0x0430ULL, 0x0440ULL) == false);
        assert(scss_is_restricted(0x200BULL) == true);
        assert(scss_is_restricted(0x00110000ULL) == true);  /* SCP */
        assert(scss_is_neo_mixed(0x00160100ULL) == true);
        assert(scss_script_of(0x00160100ULL) == SUTS_SCSS_SCRIPT_NATIVE);
        assert(scss_crosses_bridge_boundary(0x0041ULL, 0x160100ULL) == true);
        assert(scss_is_mixed_script(0x0041ULL, 0x002EULL) == false);
        assert(scss_plane_of(0x00160000ULL) == SUTS_SCSS_PLANE_RNUR);
    }

    /* --- SUTS-006 SIDN header coexists; single-pass label routing ----- */
    {
        sidn_label_descriptor_t d;
        sidn_status_code_t err = SIDN_OK;
        uint32_t out[8];
        size_t n = 0u;
        static const uint32_t label[3] = { 0x0042u, 0x00FCu, 0x0063u };

        suts_sidn_init_engine();
        assert(suts_sidn_status_of(0x0041u) == SIDN_STATUS_MAPPED);
        assert(suts_sidn_status_of(0x00DFu) == SIDN_STATUS_DEVIATION);
        assert(suts_sidn_status_of(0x2665u) == SIDN_STATUS_DISALLOWED);
        assert(suts_sidn_is_label_separator(0x3002u) == true);
        assert(suts_sidn_zone_of(0x00160100u) == SIDN_ZONE_RNUR);
        assert(suts_sidn_is_neography(0x00160200u) == true);
        assert(suts_sidn_validate_label(label, 3u, &d, &err) == true);
        assert((d.flags & SIDN_FLAG_VALID_LABEL) != 0u);
        assert(suts_sidn_to_canonical(label, 3u, out, &n, &err) == true);
        assert(n == 3u && out[0] == 0x0062u && out[1] == 0x00FCu);
    }

    /* --- SUTS-007 SUEPS header coexists; O(1) emoji presentation ------ */
    {
        sueps_emoji_entry_t e;

        suts_sueps_init_registry();
        assert(suts_sueps_get_entry(0x1F600u, &e) == true);
        assert((e.presentation_flags & SUEPS_FLAG_EMOJI) != 0u);
        assert(suts_sueps_is_emoji_presentation(0x1F600u) == true);
        assert(suts_sueps_is_text_default(0x270Fu) == true);
        assert(suts_sueps_is_modifier(0x1F3FBu) == true);
        assert(suts_sueps_is_modifier_base(0x1F44Bu) == true);
        assert(suts_sueps_is_pua_half(0xDC00u) == true);
        assert(suts_sueps_is_renderable(0xDC00u) == true);
        assert(suts_sueps_resolve_sequence(0x1F44Bu, 0x1F3FBu, &e) == true);
        assert((e.presentation_flags & SUEPS_FLAG_ZWJ_SEQUENCE) != 0u);
        assert(suts_sueps_zone_of(0x00160100u) == SUEPS_ZONE_RNUR);
    }

    /* --- SUTS-008 SUSCH header coexists; source code safety screening -- */
    {
        susch_source_attr_t a;
        susch_token_status_t st = SUSCH_TOKEN_OK;
        uint32_t warn = 0u;
        size_t idx = 0u;
        size_t chunks = 0u;
        static const uint32_t tok[3] = { 0x0070u, 0x0430u, 0x0079u };
        static const uint32_t bidi[2] = { 0x0061u, 0x202Eu };

        suts_susch_init_engine();
        assert(suts_susch_get_attributes(0x0430u, &a) == true);
        assert((a.security_mask & SUSCH_FLAG_HOMOGLYPH_RISK) != 0u);
        assert(a.script_category == (uint16_t)SUSCH_SCRIPT_CYRILLIC);
        assert(a.identifier_status == (uint8_t)SUSCH_IDENT_RESTRICTED);
        assert(suts_susch_zone_of(0x00160100u) == SUSCH_ZONE_RNUR);
        assert(suts_susch_skeleton_of(0x0430u) == 0x0061u);
        assert(suts_susch_confusable_with(0x0430u, 0x0061u) == true);
        assert(suts_susch_chunk_count(tok, 3u, &chunks) == true);
        assert(chunks == 1u);
        assert(suts_susch_is_mixed_script(tok, 3u) == true);
        assert(suts_susch_validate_token_stream_ex(tok, 3u, &st, &warn, &idx)
               == true);
        assert(st == SUSCH_TOKEN_OK);
        assert((warn & SUSCH_WARN_MIXED_SCRIPT) != 0u);
        assert(suts_susch_validate_token_stream(bidi, 2u) == false);
    }

    /* --- SUTS-009 SULDF header coexists; UTS #58 link detection --------- */
    {
        suldf_link_attr_t la;
        suldf_link_span_t spans[4];
        size_t nspan;
        static const uint32_t url[19] = {
            'S', 'e', 'e', ' ', 'e', 'x', 'a', 'm', 'p', 'l', 'e', '.',
            'c', 'o', 'm', '/', 'a', ' ', 'x'
        };
        static const uint32_t mail[20] = {
            'S', 'e', 'e', ' ', 'a', 'b', 'c', 'd', '@', 'e', 'x', 'a',
            'm', 'p', 'l', 'e', '.', 'c', 'o', 'm'
        };

        suts_suldf_init_engine();
        assert(suts_suldf_get_attributes(0x0061u, &la) == true);
        assert(la.term_class == (uint16_t)SULDF_TERM_INCLUDE);
        assert((la.link_mask & SULDF_FLAG_DOMAIN_CHAR) != 0u);
        assert(suts_suldf_term_of(0x0020u) == SULDF_TERM_HARD);
        assert(suts_suldf_term_of(0x002Eu) == SULDF_TERM_SOFT);
        assert(suts_suldf_is_open_bracket(0x0028u) == true);
        assert(suts_suldf_open_bracket_for(0x0029u) == 0x0028u);
        assert(suts_suldf_zone_of(0x00160100u) == SULDF_ZONE_RNUR);

        nspan = suts_suldf_scan_stream(url, 19u, spans, 4u);
        assert(nspan == 1u);
        assert(spans[0].start == 4u);
        assert(spans[0].length == 13u);
        assert(spans[0].kind == SULDF_LINK_URL);

        nspan = suts_suldf_scan_stream(mail, 20u, spans, 4u);
        assert(nspan == 1u);
        assert(spans[0].start == 4u);
        assert(spans[0].length == 16u);
        assert(spans[0].kind == SULDF_LINK_EMAIL);
    }

    /* --- SUAS-002 SGW header coexists; zoned 64-bit grid metric -- */
    {
        suas_sgw_options_t go;
        suas_sgw_options_default(&go);
        assert(suas_sgw_resolve(0x4E00ULL, &go) == SUAS_SGW_W_WIDE);
        assert(suas_sgw_cells(0x4E00ULL, false, &go) == SUAS_SGW_GRID_TWO);
        assert(suas_sgw_cells(0x80000000ULL, false, &go) == SUAS_SGW_GRID_ONE);
        assert(suas_sgw_cells(0x00110001ULL, false, &go) == SUAS_SGW_GRID_NONE);
        assert(suas_sgw_resolve(0x00C5ULL, &go) == SUAS_SGW_W_NEUTRAL);
    }

    /* --- SUAS-003 SBR header coexists; single-pass break engine --- */
    {
        suas_sbr_options_t bo;
        suas_sbr_options_default(&bo);
        assert(SCP_BRK_MANDATORY == 0x00110020UL);
        assert(SCP_BRK_PROHIBITED == 0x00110021UL);
        assert(SCP_BRK_OPPORTUNISTIC == 0x00110022UL);
        assert(suas_sbr_classify(0x0041ULL, &bo) == SUAS_SBR_CLS_AL);
        assert(suas_sbr_pair(0x0041ULL, 0x0062ULL, &bo) == SUAS_BRK_NO_BREAK);
        assert(suas_sbr_pair(0x0041ULL, 0x000AULL, &bo) == SUAS_BRK_MUST_BREAK);
        assert(suas_sbr_is_native_word(0x00120000ULL) == true);
    }

    /* --- SUAS-004 SUCF header coexists; canonical (de)composition --- */
    {
        suas_sucf_options_t co;
        suas_sucf_options_default(&co);
        sucs_ex_char_t cbuf[8];
        size_t ccount = 0;
        const sucs_ex_char_t dec[] = { 0x0065ULL, 0x0301ULL };
        assert(suas_sucf_ccc(0x0301ULL, &co) == 230);
        assert(suas_sucf_is_scp(0x0011A080ULL) == true);
        assert(suas_sucf_is_invariant(0x00110020ULL) == true);
        assert(suas_sucf_transform(dec, 2, SUAS_SUCF_FORM_C, &co,
                                   cbuf, 8, &ccount) == SUAS_SUCF_OK);
        assert(ccount == 1 && cbuf[0] == 0x00E9ULL); /* e + acute → é */
    }

    /* --- SUAS-005 SSP header coexists; 16-bit script resolution --- */
    {
        assert(SUAS_SCRIPT_LATIN == 0x0002);
        assert(SUAS_SCRIPT_BRIDGE_MAX == 0x03FF);   /* SUCPM 10-bit window */
        assert(suas_ssp_get_script(0x0041ULL) == SUAS_SCRIPT_LATIN);
        assert(suas_ssp_get_script(0x160000ULL) == SUAS_SCRIPT_UNKNOWN);
        assert(suas_ssp_is_implicit(SUAS_SCRIPT_COMMON) == true);
        {
            uint64_t tok = suas_ssp_tag_codepoint(0x4E00ULL, SUAS_SCRIPT_HAN);
            assert(suas_ssp_token_script(tok) == SUAS_SCRIPT_HAN);
            assert(suas_ssp_token_codepoint(tok) == 0x4E00ULL);
        }
    }

    /* --- SUAS-006 STS header coexists; single-pass segmentation --- */
    {
        suas_sts_state_t sst;
        assert(SUAS_STS_WINDOW_SIZE == 4U);
        assert(SCP_STS_BASE == 0x00110040ULL);
        suas_sts_init(&sst, SUAS_STS_LEVEL_GRAPHEME);
        assert(suas_sts_evaluate(&sst, 0x0041ULL) == SUAS_STS_BOUNDARY_MUST);
        assert(suas_sts_evaluate(&sst, 0x0301ULL) == SUAS_STS_BOUNDARY_NONE);
        assert(suas_sts_classify_grapheme(0x0301ULL) == SUAS_STS_GCB_EXTEND);
        assert(suas_sts_flush(&sst) == SUAS_STS_BOUNDARY_MUST);
    }

    /* --- SUAS-007 SIS header coexists; single-pass identifier filter -- */
    {
        suas_sis_state_t sis;
        assert(SUAS_SIS_SPEC_ID[0] == 'S');
        assert(suas_sis_get_class(0x0061ULL) == SUAS_SIS_CLASS_START);
        assert(suas_sis_get_class(0x0031ULL) == SUAS_SIS_CLASS_CONTINUE);
        assert(suas_sis_get_class(0x00110000ULL) == SUAS_SIS_CLASS_INVALID);
        assert(suas_sis_is_scp(0x00110000ULL) == true);
        assert(suas_sis_get_class(0x00160000ULL) == SUAS_SIS_CLASS_START);
        assert(suas_sis_get_syntax(0x0020ULL) == SUAS_SIS_SYNTAX_WHITESPACE);
        assert(suas_sis_get_syntax(0x002BULL) == SUAS_SIS_SYNTAX_OPERATOR);
        suas_sis_init(&sis);
        assert(suas_sis_feed(&sis, 0x0061ULL) == true);
        assert(suas_sis_feed(&sis, 0x0031ULL) == true);
        assert(suas_sis_length(&sis) == 2u);
        assert(suas_sis_feed(&sis, 0x0021ULL) == false);
        assert(!suas_sis_is_started(&sis));
    }

    /* --- SUAS-008 SNCS header coexists; static named-sequence registry -- */
    {
        suas_named_sequence_t sncs;
        assert(suas_sncs_resolve_sequence(
            "LATIN SMALL LETTER I WITH MACRON AND GRAVE", &sncs) == true);
        assert(sncs.length == 2u);
        assert(sncs.codepoints[0] == 0x012BULL);
        assert(suas_sncs_registry_of(&sncs) == SUAS_SNCS_REGISTRY_BRIDGE);
        assert(suas_sncs_is_well_formed(&sncs) == true);
        assert(suas_sncs_resolve_sequence("RNUR LIGATURE KA DHE", &sncs)
               == true);
        assert(suas_sncs_registry_of(&sncs) == SUAS_SNCS_REGISTRY_NATIVE);
        assert(suas_sncs_resolve_sequence("NOT A REAL SEQUENCE", &sncs)
               == false);
        assert(suas_sncs_names_equal("SARATI LETTER AA",
                                     "SARATI CHARACTER AA") == true);
        assert(suas_sncs_is_native_codepoint(0x00160000ULL) == true);
    }

    /* --- SUAS-009 SIP header coexists; O(1) ideographic properties ---- */
    {
        suas_glyph_prop_t sip;
        sucs_ex_char_t nat = 0;
        assert(suas_sip_get_properties(0x4E00ULL, &sip) == true);
        assert(sip.radical == 1u && sip.strokes == 1u);
        assert((sip.flags & SUAS_SIP_FLAG_UNIFIED) != 0u);
        assert(suas_sip_get_radical_stroke(0x6728ULL, NULL, NULL) == true);
        assert(suas_sip_is_ideograph(0x20000ULL) == true);
        assert(suas_sip_is_ideograph(0x00110000ULL) == false);
        assert(suas_sip_plane_of(0x00160000ULL) == SUAS_SIP_PLANE_RNUR);
        assert(suas_sip_variant_target(0x9F8DULL, &nat) == true);
        assert(nat == 0x00160200ULL);
        assert(suas_sip_get_properties(0x0041ULL, &sip) == false);
    }

    /* --- SUAS-010 SSRE header coexists; embedded dependency registry ---- */
    {
        static const ssre_dependency_descriptor_t deps[3] = {
            { "SUAS-009", 1u, 0u, true  },  /* an invariant: mandatory    */
            { "SUTS-005", 1u, 0u, false },  /* a data spec: optional      */
            { "SUTS-003", 1u, 0u, true  }   /* a data spec: not invariant */
        };
        assert(suas_ssre_is_registered("SUAS-010") == true);
        assert(suas_ssre_is_registered("SUAS-014") == true);
        assert(suas_ssre_is_registered("SUAS-015") == true);
        assert(suas_ssre_is_registered("SUTS-007") == true);
        assert(suas_ssre_is_mandatory("SUTS-005") == false);
        assert(suas_ssre_tier_of("SUAS-009") == SUAS_SSRE_TIER_L0);
        assert(suas_ssre_verify_set(deps, 2u) == true);
        /* A mandatory claim against an optional data spec is rejected. */
        assert(suas_ssre_verify_set(deps, 3u) == false);
        assert(suas_ssre_first_missing(deps, 3u) == &deps[2]);
        assert(suas_ssre_dependency_count(deps, 3u) == 1u);
        assert(suas_ssre_spec_count() == 47u);
    }

    /* --- SUAS-011 SCMS header coexists; manifest grammar validation ------ */
    {
        scms_manifest_state_t st;
        scms_record_descriptor_t d;

        assert(suas_scms_line_kind("%manifest 1.0", 13u) == SCMS_LINE_DIRECTIVE);
        assert(suas_scms_validate_record("U+4E00\tU+4E00\t1\t1\t0x01\t19968",
                                         28u, &d) == true);
        assert(d.start_codepoint == 0x4E00u && d.is_binary_aligned == true);
        assert(suas_scms_codepoint_admissible(0x00160100u) == true);
        assert(suas_scms_codepoint_admissible(0x00110000u) == false);
        assert(suas_scms_codepoint_admissible(0x7FFFFFFFu) == false);
        assert(suas_scms_plane_mask(0x00160100u, 0x00160100u) ==
               (SUAS_SCMS_PLANE_NATIVE | SUAS_SCMS_PLANE_RNUR));

        suas_scms_begin(&st);
        assert(suas_scms_feed_line(&st, "%manifest 1.0", 13u) == true);
        assert(suas_scms_feed_line(&st, "%records 1", 10u) == true);
        assert(suas_scms_feed_line(&st, "%planes 0x01", 12u) == true);
        assert(suas_scms_feed_line(&st, "U+4E00\tU+4E00\t1\t1\t0x01\t19968",
                                   28u) == true);
        assert(suas_scms_end(&st) == true);
        assert(suas_scms_planes_declared(&st) == true);
        /* A manifest whose records fall outside its declared planes. */
        assert(suas_scms_feed_line(&st, "U+00160100\tU+00160100\t0\t0\t0x02\t0",
                                   32u) == true);
        assert(suas_scms_planes_declared(&st) == false);

        /* The SSRE linkage gate: SCMS is registered, and an invariant. */
        assert(suas_ssre_is_registered(SUAS_SCMS_SPEC_ID) == true);
        assert(suas_ssre_is_mandatory(SUAS_SCMS_SPEC_ID) == true);
        assert(suas_ssre_spec_count() == 47u);
    }

    /* --- SUAS-012 SUCD header coexists; a bound image answers O(1) -------- */
    {
        /* One uniform page and 32767 unassigned ones: an image with no dense
         * index at all, which is the smallest image the verifier accepts. */
        static union {
            uint64_t      align;
            unsigned char bytes[sizeof(sucd_header_t) + 32768u * 4u + 16u];
        } image;
        sucd_header_t *h = (sucd_header_t *)(void *)image.bytes;
        uint32_t *dir = (uint32_t *)(void *)(image.bytes + sizeof(sucd_header_t));
        sucd_record_t *rec = (sucd_record_t *)(void *)(image.bytes
                            + sizeof(sucd_header_t) + 32768u * 4u);
        sucd_record_t got;
        sucd_schema_link_t link;
        unsigned i;

        h->magic = SUAS_SUCD_MAGIC;
        h->version = 0x00010000u;
        h->total_records = 1u;
        h->plane_mask = SUCD_PLANE_BRIDGE;
        h->directory_count = SUAS_SUCD_DIRECTORY_SIZE;
        h->flags = SUCD_IMAGE_PARTITION;
        rec->first = 0u;
        rec->last = 0x0000FFFFu;
        rec->property_mask = SUCD_PROP_UNICODE_BRIDGED | SUCD_PROP_ALPHABETIC;
        rec->general_category = (uint16_t)SUCD_GC_LO;
        rec->script_id = 0u;
        rec->flags = SUCD_RECORD_PRINTABLE;
        for (i = 0u; i < SUAS_SUCD_DIRECTORY_SIZE; i++)
            dir[i] = SUAS_SUCD_NO_PAGE;
        dir[0] = SUAS_SUCD_UNIFORM_TAG | 0u;

        assert(suas_sucd_verify_image(image.bytes, sizeof image.bytes, NULL));
        assert(suas_sucd_init(image.bytes, sizeof image.bytes));
        assert(suas_sucd_is_loaded());
        assert(suas_sucd_header()->total_records == 1u);
        assert(suas_sucd_get_record(0x0041u, &got) == true);
        assert(got.general_category == (uint16_t)SUCD_GC_LO);
        assert(got.first == 0u && got.last == 0x0000FFFFu);
        assert(suas_sucd_property_mask(0x0041u) ==
               (SUCD_PROP_UNICODE_BRIDGED | SUCD_PROP_ALPHABETIC));
        /* An unassigned page answers with the default record, not a failure. */
        assert(suas_sucd_get_record(0x00010000u, &got) == true);
        assert(got.general_category == (uint16_t)SUCD_GC_CN);
        assert(got.property_mask == 0u);
        assert(suas_sucd_plane_of(SUAS_SUCD_MAX_CODEPOINT) ==
               SUCD_PLANE_SENTINEL);

        /* The legacy SUAS-001 projection still resolves through this header. */
        assert((suas_sucd_bidi(0x0041u) & SUCD_BIDI_LTR) != 0);

        /* The §7 linkage gate: SUCD compiles against SCMS, and is itself
         * registered in SSRE as a mandatory architecture specification. */
        link.schema_spec_id = SUAS_SCMS_SPEC_ID;
        link.schema_major = 1u;
        link.schema_minor = 0u;
        link.schema_mandatory = true;
        assert(suas_sucd_verify_schema_link(&link) == true);
        assert(suas_ssre_is_registered(SUAS_SUCD_SPEC_ID) == true);
        assert(suas_ssre_is_mandatory(SUAS_SUCD_SPEC_ID) == true);

        suas_sucd_shutdown();
        assert(!suas_sucd_is_loaded());
    }

    /* --- SUAS-014 SUVTL header coexists; vertical orientation is total --- */
    {
        suvtl_glyph_layout_t lay;

        /* A curated transformed-rotated form: fullwidth comma. */
        assert(suas_suvtl_get_layout_properties(0xFF0Cu, &lay) == true);
        assert((lay.orientation_flags & SUVTL_ORIENT_MASK) == SUVTL_ORIENT_TR);
        assert((lay.orientation_flags & SUVTL_ORIENT_TRANSFORM) != 0u);
        assert(suas_suvtl_is_rotated(0xFF0Cu) == true);
        assert(suas_suvtl_is_upright(0xFF0Cu) == false);
        assert(suas_suvtl_is_transformed(0xFF0Cu) == true);
        /* A curated squared word: upright fallback. */
        assert(suas_suvtl_get_layout_properties(0x3300u, &lay) == true);
        assert((lay.orientation_flags & SUVTL_ORIENT_MASK) == SUVTL_ORIENT_TU);
        assert((lay.orientation_flags & SUVTL_FLAG_SQUARED) != 0u);
        assert(suas_suvtl_is_upright(0x3300u) == true);
        assert(suas_suvtl_is_rotated(0x3300u) == false);
        /* An uncurated Latin letter defaults to rotated; the property is
         * total, so the query still succeeds. */
        assert(suas_suvtl_get_layout_properties(0x0061u, &lay) == true);
        assert((lay.orientation_flags & SUVTL_ORIENT_MASK) == SUVTL_ORIENT_R);
        /* An inadmissible zone is the only false. */
        assert(suas_suvtl_get_layout_properties(0x7FFFFFFFu, &lay) == false);
        assert(suas_suvtl_is_registered(0xFF0Cu) == true);
        assert(suas_ssre_is_registered(SUAS_SUVTL_SPEC_ID) == true);
        assert(suas_ssre_is_mandatory(SUAS_SUVTL_SPEC_ID) == true);
    }

    /* --- SUAS-015 SUAMR header coexists; mark layout and AMTRA are total -- */
    {
        suamr_mark_layout_t lay;
        suamr_reorder_status_t st;
        uint32_t cl[3];

        /* A curated tashkil: fatha stacks above its base. */
        assert(suas_suamr_get_mark_layout(0x0627u, 0x064Eu, &lay) == true);
        assert(lay.codepoint == 0x064Eu);
        assert((lay.positioning_flags & SUAMR_FLAG_STACK_ABOVE) != 0u);
        assert(suas_suamr_is_above(0x064Eu) == true);
        assert(suas_suamr_ccc(0x064Eu) == SUAMR_CCC_FATHA);

        /* The shadda sits closer to the base than a vowel. */
        assert(suas_suamr_stacking_level(0x0651u) <
               suas_suamr_stacking_level(0x064Fu));
        assert(suas_suamr_is_mcm(0x0654u) == true);
        assert(suas_suamr_is_shadda(0x0651u) == true);

        /* AMTRA moves the shadda in front of a preceding damma. */
        cl[0] = 0x0627u;
        cl[1] = 0x064Fu;
        cl[2] = 0x0651u;
        assert(suas_suamr_reorder_cluster(cl, 3u, &st) == true);
        assert(st == SUAMR_REORDER_OK);
        assert(cl[1] == 0x0651u && cl[2] == 0x064Fu);

        /* An inadmissible zone is the only false. */
        assert(suas_suamr_get_mark_layout(0x0627u, 0x7FFFFFFFu, &lay) == false);
        assert(suas_suamr_is_registered(0x0651u) == true);
        assert(suas_ssre_is_registered(SUAS_SUAMR_SPEC_ID) == true);
        assert(suas_ssre_is_mandatory(SUAS_SUAMR_SPEC_ID) == true);
    }

    /* --- SUAS-016 SUKEMET header coexists; hieroglyphs are O(1) and total - */
    {
        sukemet_hieroglyph_entry_t h;
        sukemet_metadata_t m;
        uint32_t alt = 0u;

        /* A curated Gardiner sign: A-01-001, Core, classifier. */
        assert(suas_sukemet_get_entry(0x13000u, &h) == true);
        assert(h.codepoint == 0x13000u);
        assert((h.sukemet_flags & SUKEMET_FLAG_GARDINER) != 0u);
        assert((h.sukemet_flags & SUKEMET_FLAG_CORE) != 0u);
        assert(h.gardiner_category == SUKEMET_GARDINER_A);
        assert(suas_sukemet_is_gardiner(0x13000u) == true);
        assert(suas_sukemet_core_class(0x13000u) == SUKEMET_CORE_C);
        assert(suas_sukemet_get_metadata(0x13000u, &m) == true);
        assert(m.catalog_index == 0x00010001u);
        assert(m.function_types == SUKEMET_FUNC_CLASSIFIER);

        /* A format control composes a cartouche; a Legacy sign mirrors. */
        assert(suas_sukemet_is_format_control(0x13430u) == true);
        assert(suas_sukemet_is_joiner(0x13430u) == true);
        assert(suas_sukemet_enclosure_kind(0x1343Cu) ==
               SUKEMET_ENCLOSURE_CARTOUCHE);
        assert(suas_sukemet_enclosure_kind(0x1343Eu) ==
               SUKEMET_ENCLOSURE_SEREKH);
        assert(suas_sukemet_is_legacy(0x1305Du) == true);
        assert(suas_sukemet_alt_seq_length(0x1305Du) == 2u);
        assert(suas_sukemet_alt_seq_at(0x1305Du, 1u, &alt) == true);
        assert(alt == 0x13440u);

        /* An uncurated codepoint in the repertoire is synthesized, not lost. */
        assert(suas_sukemet_get_entry(0x1603FFu, &h) == true);
        assert(h.sukemet_flags == SUKEMET_FLAG_NEOGRAPHY);
        assert(suas_sukemet_is_registered(0x1603FFu) == false);
        assert(suas_sukemet_get_entry(0x110000u, &h) == false);
        assert(suas_sukemet_entry_count() == 5109u);
        assert(suas_ssre_is_registered(SUKEMET_SPEC_ID) == true);
        assert(suas_ssre_is_mandatory(SUKEMET_SPEC_ID) == true);
    }

    /* --- SUAS-017 SUEASD header coexists; East Asian scripts are O(1) ----- */
    {
        sueasd_script_entry_t e;
        sueasd_metadata_t m;

        /* Han resolves from the classification range; core, ideographic. */
        assert(suas_sueasd_get_entry(0x4E00u, &e) == true);
        assert(e.primary_script == SUEASD_SCRIPT_HAN);
        assert(e.source_region == SUEASD_REGION_CHINA);
        assert((e.script_mask & SUEASD_FLAG_IDEOGRAPHIC) != 0u);
        assert((e.script_mask & SUEASD_FLAG_HAN_CORE) != 0u);
        assert(suas_sueasd_is_ideographic(0x4E00u) == true);
        assert(suas_sueasd_is_curated(0x4E00u) == false);

        /* A Kangxi radical and a compatibility ideograph carry their roles. */
        assert(suas_sueasd_is_radical(0x2F00u) == true);
        assert(suas_sueasd_is_compatibility(0xF900u) == true);

        /* A curated Jurchen sign carries its reading, source and RS data. */
        assert(suas_sueasd_get_entry(0x18E00u, &e) == true);
        assert(e.primary_script == SUEASD_SCRIPT_JURCHEN);
        assert(e.source_region == SUEASD_REGION_JURCHEN);
        assert(suas_sueasd_is_curated(0x18E00u) == true);
        assert(suas_sueasd_radical(0x18E00u) == 1u);
        assert(suas_sueasd_stroke_count(0x18E00u) == 3u);
        assert(suas_sueasd_reading_hash(0x18E00u) ==
               suas_sueasd_hash_bytes("nie", 3u));
        assert(suas_sueasd_get_metadata(0x18E00u, &m) == true);
        assert((m.source_mask & SUEASD_SOURCE_JURC_NCREADING) != 0u);

        /* Small Seal modern-CJK equivalence, Tangut numeric, Nushu reading. */
        assert(suas_sueasd_modern_cjk(0x3D000u) == 0x4E00u);
        assert(suas_sueasd_numeric_value(0x1736Bu) == 2u);
        assert(suas_sueasd_primary_script(0x1B170u) == SUEASD_SCRIPT_NUSHU);
        assert(suas_sueasd_reading_hash(0x1B170u) ==
               suas_sueasd_hash_bytes("i5", 2u));

        /* Kana, jamo and syllables are distinct roles. */
        assert(suas_sueasd_is_kana(0x3042u) == true);
        assert(suas_sueasd_is_jamo(0x1100u) == true);
        assert(suas_sueasd_is_syllable(0xAC00u) == true);

        /* The RNUR carve-out defaults to a custom script. */
        assert(suas_sueasd_get_entry(0x1603FFu, &e) == true);
        assert(e.script_mask == SUEASD_FLAG_CUSTOM_SCRIPT);
        assert(suas_sueasd_is_east_asian(0x1603FFu) == true);
        assert(suas_sueasd_get_entry(0x110000u, &e) == false);
        assert(suas_sueasd_entry_count() == 18815u);
        assert(suas_sueasd_range_count() == 78u);
        assert(suas_ssre_is_registered(SUEASD_SPEC_ID) == true);
        assert(suas_ssre_is_mandatory(SUEASD_SPEC_ID) == true);
    }

    /* --- SUTR-11 SUMS header coexists; shaping and FVS are O(1) ---------- */
    {
        sums_shaping_entry_t e;

        sutr_sums_init_registry();

        /* A curated cursive letter takes all four positional forms. */
        assert(sutr_sums_get_entry(0x1820u, &e) == true);
        assert((e.shaping_flags & SUMS_FLAG_CURSIVE) != 0u);
        assert((e.shaping_flags & SUMS_FLAG_VERTICAL) != 0u);

        /* Contextual shaping is a pure function of the neighbours. */
        assert(sutr_sums_positional_form(0u, 0x1820u, 0x1820u) == SUMS_FORM_INITIAL);
        assert(sutr_sums_positional_form(0x1820u, 0x1820u, 0x1820u) == SUMS_FORM_MEDIAL);
        assert(sutr_sums_positional_form(0x1820u, 0x1820u, 0u) == SUMS_FORM_FINAL);
        assert(sutr_sums_positional_form(0u, 0x1820u, 0u) == SUMS_FORM_ISOLATED);

        /* The flattened FVS sequence resolves to a precompiled token, O(1). */
        assert(sutr_sums_resolve_variant(0x183Cu, SUMS_FVS1, &e) == true);
        assert(e.codepoint == 0x00010001u);
        assert(sutr_sums_variant_token(0x183Cu, SUMS_FVS1) == 0x00010001u);

        /* Native vertical flow is a fixed-point byte offset in the record. */
        assert(sutr_sums_vertical_advance(0x1820u) == 0x0040);

        /* An inadmissible zone is the only false. */
        assert(sutr_sums_get_entry(0x00110000u, &e) == false);
        assert(sutr_sums_is_registered(0x1820u) == true);
        assert(suas_ssre_is_registered(SUMS_SPEC_ID) == true);
        assert(suas_ssre_kind_count(SUAS_SSRE_KIND_REFERENCE) == 15u);
        assert(suas_ssre_kind_count(SUAS_SSRE_KIND_NOTE) == 1u);
    }

    /* --- SUTR-012 SUCSL header coexists; sign lookup is O(1) ----------- */
    {
        sucsl_sign_entry_t e;
        sucsl_compound_t c;
        uint8_t major = 0u;
        uint8_t minor = 0u;

        sutr_sucsl_init_registry();

        /* A curated composite sign carries its flags and value hash. */
        assert(sutr_sucsl_get_sign_entry(0x12120u, &e) == true);
        assert((e.sign_flags & SUCSL_FLAG_COMPOSITE) != 0u);
        assert(e.primary_value_hash == sutr_sucsl_hash_value("am"));

        /* GUD×KUR decomposes to GUD + KUR; GEME₂ is a pure sequence. */
        assert(sutr_sucsl_get_composition(0x12120u, &c) == true);
        assert(c.component_a == 0x1211Eu && c.component_b == 0x121B3u);
        assert(sutr_sucsl_find_sequence(sutr_sucsl_hash_value("geme2"), &c) == true);
        assert(c.component_a == 0x122A9u && c.component_b == 0x121B3u);

        /* The numerical and structural classes are distinct projections. */
        assert(sutr_sucsl_is_numerical(0x12409u) == true);
        assert(sutr_sucsl_sign_category(0x12376u) == SUCSL_CATEGORY_COMPOSITE);

        /* An ordinary character and an inadmissible zone are both false. */
        assert(sutr_sucsl_get_sign_entry(0x41u, &e) == false);
        assert(sutr_sucsl_get_sign_entry(0x00110000u, &e) == false);
        assert(suas_ssre_is_registered(SUCSL_SPEC_ID) == true);
        assert(suas_ssre_version_of(SUCSL_SPEC_ID, &major, &minor) == true);
        assert(major == 0u && minor == 1u);
    }

    /* --- SUTR-013 SUEAS header coexists; width and spacing are O(1) ------ */
    {
        sueas_width_entry_t e;
        sueas_spacing_decision_t d;
        static const uint32_t stream[3] = { 0x00000041u, 0x00004E00u, 0x00000042u };
        uint16_t cols[4];
        uint32_t set = 0u;

        sutr_sueas_init_registry();

        /* A Wide ideograph is two columns; a Narrow letter is one. */
        assert(sutr_sueas_get_width_entry(0x00004E00u, &e) == true);
        assert(e.width_category == (uint8_t)SUEAS_WIDTH_WID);
        assert(e.column_units == 2u);
        assert((e.spacing_flags & SUEAS_FLAG_GRID_SNAP) != 0u);

        assert(sutr_sueas_get_width_entry(0x00000041u, &e) == true);
        assert(e.width_category == (uint8_t)SUEAS_WIDTH_NAR);
        assert(e.column_units == 1u);

        /* A Wide/Narrow boundary receives a visible auto-space; the same
         * pair in a Chinese context resolves a Conditional to Narrow too. */
        assert(sutr_sueas_spacing_decision(0x00004E00u, 0x00000041u, false,
                                           false, &d) == true);
        assert(d.space == 1u);

        /* The grid engine measures a stream in one pass, no allocation. */
        assert(sutr_sueas_measure_columns(stream, 3u, cols, 4u) == 4u);
        assert(cols[0] == 1u && cols[1] == 2u && cols[2] == 1u);
        assert(sutr_sueas_count_spaces(stream, 3u, false, false) == 2u);

        /* The neography carve-out and an inadmissible zone. */
        assert(sutr_sueas_neography_set(0x00160100u, &set) == true && set == 1u);
        assert(sutr_sueas_get_width_entry(0x00110000u, &e) == false);
        assert(suas_ssre_is_registered(SUEAS_SPEC_ID) == true);
    }

    /* --- SUTR-7 SUCEM header coexists; the levels are name-driven ------- */
    {
        uint32_t trap = 0u;

        /* A form name is valid at its own level and at no other. */
        assert(sutr_sucem_name_status("SUTF-8", SUCEM_LEVEL_NONE) == SUCEM_NAME_OK);
        assert(sutr_sucem_name_status("SUTF-8", SUCEM_LEVEL_CEF) == SUCEM_NAME_OK);
        assert(sutr_sucem_name_status("SUTF-8", SUCEM_LEVEL_CES) == SUCEM_NAME_WRONG_LEVEL);
        assert(sutr_sucem_is_cef_name("SUTF-8") == true);
        assert(sutr_sucem_is_ces_name("SUST-16") == true);

        /* The forbidden names of SUCEM §7 are rejected by name. */
        assert(sutr_sucem_name_status("SUTF-32", SUCEM_LEVEL_NONE)
               == SUCEM_NAME_FORBIDDEN_CEF);
        assert(sutr_sucem_name_status("SUST-8", SUCEM_LEVEL_NONE)
               == SUCEM_NAME_FORBIDDEN_CES);

        /* The BANcode -> trap mapping is O(1) arithmetic, not a table. */
        assert(sutr_sucem_bancode_to_trap(0x0011A000u, &trap) == true);
        assert(trap == 0x7FFFFFF0u);
        assert(sutr_sucem_bancode_to_trap(0x0011A780u, &trap) == false);

        assert(suas_ssre_is_registered(SUCEM_SPEC_ID) == true);
    }

    /* --- SUTR-8 SUCPM header coexists; the Bridge tier is a range table -- */
    {
        sucpm_flags64_t mask;

        assert(sutr_sucpm_tier_of(0x0010FFFFu) == SUCPM_TIER_BRIDGE);
        assert(sutr_sucpm_tier_of(0x00110000u) == SUCPM_TIER_SCP);
        assert(sutr_sucpm_bridge_range_count() > 0u);

        /* The Bridge tier is covered end to end by the range table. */
        {
            uint32_t n = sutr_sucpm_bridge_range_count();
            uint32_t low = 0u;
            uint32_t high = 0u;

            assert(sutr_sucpm_bridge_range_at(0u, &low, &high) == true);
            assert(low == 0u);
            assert(sutr_sucpm_bridge_range_at(n - 1u, &low, &high) == true);
            assert(high == 0x0010FFFFu);
        }

        /* The packed mask carries the Math flag and the MathClass. */
        mask = sutr_sucpm_props(0x0000002Bu);   /* PLUS SIGN */
        assert(sutr_sucpm_is_math(mask) == true);
        assert(sutr_sucpm_math_class(mask) == SUCPM_MC_B);

        assert(suas_ssre_is_registered(SUCPM_SPEC_ID) == true);
    }

    /* --- SUTR-9 SUMNLS header coexists; the atom projection is total ---- */
    {
        assert(sutr_sumnls_math_class(0x0000002Bu) == SUCPM_MC_B);
        assert(sutr_sumnls_math_class(0x0000222Bu) == SUCPM_MC_U);
        assert(sutr_sumnls_project(SUCPM_MC_B) == SUMNLS_ATOM_BIN);
        assert(sutr_sumnls_project(SUCPM_MC_U) == SUMNLS_ATOM_ORD);
        assert(sutr_sumnls_spacing(SUMNLS_ATOM_ORD, SUMNLS_ATOM_BIN)
               == SUMNLS_SPACE_MEDIUM);

        assert(suas_ssre_is_registered(SUMNLS_SPEC_ID) == true);
    }

    /* --- SUTR-10 SUCM header coexists; the tier ladder is cumulative ---- */
    {
        sucm_claim_t claim;

        assert(sutr_sucm_clause_count() == 29u);
        assert(sutr_sucm_tier_includes(SUCM_TIER_L2, SUCM_TIER_L0) == true);
        assert(sutr_sucm_tier_includes(SUCM_TIER_L1, SUCM_TIER_L0) == true);
        assert(sutr_sucm_claim_find(SUCM_TIER_L0, "sis", "0.1.0", &claim) == true);

        assert(suas_ssre_is_registered(SUCM_SPEC_ID) == true);
    }

    /* --- SUAS-018 SKTO header coexists; bounded reads and names --------- */
    {
        skto_text_t text;
        skto_name_table_t names;
        skto_text_t name;
        uint32_t generation = 99u;
        uint8_t name_bytes[4] = { 'v', 'f', 's', 0u };
        size_t count = 0u;

        assert(suas_skto_init_borrowed(&text, "abc", 3u) == SKTO_OK);
        assert(suas_skto_count(&text, &count) == SKTO_OK);
        assert(count == 3u);

        suas_skto_names_init(&names);
        assert(suas_skto_init_borrowed(&name, name_bytes, 3u) == SKTO_OK);
        assert(suas_skto_mark_nul_safe(&name) == SKTO_OK);
        assert(suas_skto_publish(&names, &name, NULL, &generation) == SKTO_OK);
        assert(generation == 0u);

        assert(suas_ssre_is_registered(SUAS_SKTO_SPEC_ID) == true);
    }

    /* --- SUAS-019 STIM header coexists; one event becomes bounded text -- */
    {
        stim_binding_t binds[1];
        stim_composition_t comps[1];
        stim_layout_t layout;
        stim_state_t state;
        stim_output_t out;
        stim_event_t event;
        uint8_t text[8];
        const uint32_t a[1] = { 'a' };

        assert(suas_stim_layout_init(&layout, binds, 1u, comps, 1u) == STIM_OK);
        assert(suas_stim_layout_bind(&layout, 1u, 0u, 0u, STIM_BIND_TEXT,
                                     a, 1u) == STIM_OK);
        assert(suas_stim_init(&state, &layout) == STIM_OK);

        event.keycode = 1u;
        event.modifiers = 0u;
        event.action = (uint8_t)STIM_ACT_PRESS;
        event.reserved = 0u;
        assert(suas_stim_feed(&state, &event, text, sizeof text, &out) ==
               STIM_OK);
        assert(out.len == 1u && text[0] == 'a');
        assert(suas_stim_is_admissible(0xD800u) == false);

        assert(suas_ssre_is_registered(SUAS_STIM_SPEC_ID) == true);
    }

    /* --- SUTS-012 SCTS header coexists; SGW/SBR/STS are deferred to ----- */
    {
        scts_plane_t plane;
        scts_cell_t cells[4];
        uint8_t row[8];
        size_t row_len = 0u;

        assert(scts_plane_init(&plane, cells, 1u, 4u, NULL) == SCTS_OK);
        assert(scts_plane_write(&plane, (const uint8_t *)"ab", 2u) == SCTS_OK);
        assert(scts_plane_serialize_row(&plane, 0u, row, sizeof row, true,
                                        &row_len) == SCTS_OK);
        assert(row_len == 2u && row[0] == 'a' && row[1] == 'b');
        assert(suas_sgw_cells(0xFF21u, false, NULL) == SUAS_SGW_GRID_TWO);

        assert(suas_ssre_is_registered(SUTS_SCTS_SPEC_ID) == true);
    }

    /* --- SUTS-013 SCTE header coexists; controls drive an SCTS plane ----- */
    {
        scts_plane_t plane;
        scts_cell_t cells[8];
        scte_term_t term;

        assert(scts_plane_init(&plane, cells, 2u, 4u, NULL) == SCTS_OK);
        assert(suts_scte_init(&term, &plane, NULL, NULL, 0u) == SCTE_OK);
        assert(suts_scte_feed(&term, (const uint8_t *)"hi", 2u) == SCTE_OK);
        assert(plane.cells[0].cp == 'h' && plane.cells[1].cp == 'i');
        assert(suts_scte_feed(&term, (const uint8_t *)"\x1b[2C", 4u) == SCTE_OK);
        assert(plane.cursor_col == 3u);
        assert(suts_scte_finish(&term) == SCTE_OK);

        assert(suas_ssre_is_registered(SUTS_SCTE_SPEC_ID) == true);
    }

    /* --- SUTR-014 OKTP header coexists; every citation resolves --------- */
    {
        assert(sutr_oktp_matrix_count() == 40u);
        assert(sutr_oktp_verify(NULL, NULL) == OKTP_OK);
        assert(sutr_oktp_level_met(OKTP_LEVEL_KT0) == true);
        assert(suas_ssre_is_registered(SUTR_OKTP_SPEC_ID) == true);
    }

    printf("[PASS] test_unified (all module headers coexist in one TU)\n");
    return 0;
}



