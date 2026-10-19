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
 *                            suas/suas_sis.h, suas/suas_sncs.h, suas/suas_sip.h
 *
 * Prior to the header-dedupe work, the identical constants (SUCS_INVALID_
 * CODEPOINT, SUCS_TRAP_RANGE_MIN/MAX, SUCS_MAX_CODEPOINT, ...), the
 * sucs_char_t typedef, and sucs_is_valid() were each defined in multiple
 * headers; this file would NOT compile. It now must compile cleanly and run.
 */

#include <stdio.h>
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
#include "suts/suts_suca.h"             // IWYU pragma: keep
#include "suts/suts_sre.h"              // IWYU pragma: keep
#include "suts/suts_sldf.h"             // IWYU pragma: keep
#include "suts/suts_sivd.h"             // IWYU pragma: keep

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

    printf("[PASS] test_unified (all module headers coexist in one TU)\n");
    return 0;
}



