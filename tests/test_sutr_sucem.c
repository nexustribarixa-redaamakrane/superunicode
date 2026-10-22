/**
 * SUTR-7 SUCEM — SuperUnicode Character Encoding Model
 *
 * Unit tests for the four-level model (ACR / CCS / CEF / CES), the byte-order
 * test that discriminates Level 3 from Level 4, the three §7 naming rules
 * (no SUTF-32, no SUTF-16-LE, no SUST-8), the ACR repertoire sources, the
 * System Control Plane four-level anchor, and the O(1) trap dispatch geometry.
 */

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>

#include "sutr/sutr_sucem.h"
#include "suas/suas_ssre.h"

static int g_fail = 0;
#define CHECK(cond) do { if (!(cond)) { \
    printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); g_fail = 1; \
} } while (0)

/* ── Version, limits, and identifiers ──────────────────────────────── */

static void test_version_and_meta(void)
{
    CHECK(sutr_sucem_version_string() != NULL);
    CHECK(sutr_sucem_version_string()[0] == '0');
    CHECK(sutr_sucem_version_string()[2] == '1');

    CHECK(SUCEM_SPEC_ID[0] == 'S');
    CHECK(SUCEM_SPEC_TITLE[0] == 'S');
    CHECK(SUCEM_MODULE[0] == 's');
    CHECK(SUCEM_VERSION_MAJOR == 0);
    CHECK(SUCEM_VERSION_MINOR == 1);
    CHECK(SUCEM_VERSION_PATCH == 0);

    /* The four levels are numbered 1..4 (§1). */
    CHECK(SUCEM_LEVEL_ACR == 1);
    CHECK(SUCEM_LEVEL_CCS == 2);
    CHECK(SUCEM_LEVEL_CEF == 3);
    CHECK(SUCEM_LEVEL_CES == 4);
    CHECK(SUCEM_LEVEL_COUNT == 4U);

    /* The BANcode Registry is 3,840 codepoints, partitioned 2048/1024/512/256. */
    CHECK(SUCEM_BANCODE_REGISTRY_MAX - SUCEM_BANCODE_REGISTRY_MIN + 1u == 3840u);
    CHECK(SUCEM_BANCODE_BPLUS_MAX - SUCEM_BANCODE_BPLUS_MIN + 1u == 2048u);
    CHECK(SUCEM_BANCODE_WPLUS_MAX - SUCEM_BANCODE_WPLUS_MIN + 1u == 1024u);
    CHECK(SUCEM_BANCODE_CPLUS_MAX - SUCEM_BANCODE_CPLUS_MIN + 1u == 512u);
    CHECK(SUCEM_BANCODE_SPLUS_MAX - SUCEM_BANCODE_SPLUS_MIN + 1u == 256u);

    /* The trap geometry: 15 slots, 128 BANcodes each. */
    CHECK(SUCEM_TRAP_SLOT_COUNT == 15U);
    CHECK(SUCEM_BANCODES_PER_TRAP == 128U);
    CHECK(SUCEM_LIMIT_TRAP_MAX - SUCEM_LIMIT_TRAP_MIN + 1u == 15u);

    /* The identity Character Map covers exactly the Bridge. */
    CHECK(SUCEM_IDENTITY_CM_MAX == SUCEM_LIMIT_UNICODE_MAX);
}

/* ── The four levels and the byte-order test (§1, §5, §6, §7) ─────── */

static void test_levels(void)
{
    CHECK(sutr_sucem_level_name(SUCEM_LEVEL_ACR)[0] == 'A');
    CHECK(sutr_sucem_level_name(SUCEM_LEVEL_CCS)[0] == 'C');
    CHECK(sutr_sucem_level_name(SUCEM_LEVEL_CEF)[0] == 'C');
    CHECK(sutr_sucem_level_name(SUCEM_LEVEL_CES)[0] == 'C');
    CHECK(sutr_sucem_level_name(SUCEM_LEVEL_NONE)[0] == 'n');

    /* An SUTF is a CEF; a SUST is a CES. */
    CHECK(sutr_sucem_level_of(SUCEM_FORM_SUTF8) == SUCEM_LEVEL_CEF);
    CHECK(sutr_sucem_level_of(SUCEM_FORM_SUTF16) == SUCEM_LEVEL_CEF);
    CHECK(sutr_sucem_level_of(SUCEM_FORM_SUTF4) == SUCEM_LEVEL_CEF);
    CHECK(sutr_sucem_level_of(SUCEM_FORM_SUTF2) == SUCEM_LEVEL_CEF);
    CHECK(sutr_sucem_level_of(SUCEM_FORM_VSUTF) == SUCEM_LEVEL_CEF);
    CHECK(sutr_sucem_level_of(SUCEM_FORM_SUST16) == SUCEM_LEVEL_CES);
    CHECK(sutr_sucem_level_of(SUCEM_FORM_SUST32) == SUCEM_LEVEL_CES);
    CHECK(sutr_sucem_level_of(SUCEM_FORM_SUST64) == SUCEM_LEVEL_CES);
    CHECK(sutr_sucem_level_of(SUCEM_FORM_SUST128) == SUCEM_LEVEL_CES);
    CHECK(sutr_sucem_level_of(SUCEM_FORM_SUST256) == SUCEM_LEVEL_CES);
    CHECK(sutr_sucem_level_of(SUCEM_FORM_SUST512) == SUCEM_LEVEL_CES);
    CHECK(sutr_sucem_level_of(SUCEM_FORM_SUSTN) == SUCEM_LEVEL_CES);
    CHECK(sutr_sucem_level_of(SUCEM_FORM_ESUST) == SUCEM_LEVEL_CES);
    CHECK(sutr_sucem_level_of(SUCEM_FORM_NONE) == SUCEM_LEVEL_NONE);

    /* The byte-order test: a CEF would not change with the processor byte
     * order; a CES would. */
    CHECK(sutr_sucem_form_is_byte_order_sensitive(SUCEM_FORM_SUTF8) == false);
    CHECK(sutr_sucem_form_is_byte_order_sensitive(SUCEM_FORM_SUTF16) == false);
    CHECK(sutr_sucem_form_is_byte_order_sensitive(SUCEM_FORM_VSUTF) == false);
    CHECK(sutr_sucem_form_is_byte_order_sensitive(SUCEM_FORM_SUST16) == true);
    CHECK(sutr_sucem_form_is_byte_order_sensitive(SUCEM_FORM_ESUST) == true);

    /* The form names round-trip. */
    CHECK(sutr_sucem_form_name(SUCEM_FORM_SUTF8)[0] == 'S');
    CHECK(sutr_sucem_form_name(SUCEM_FORM_VSUTF)[0] == 'v');
    CHECK(sutr_sucem_form_name(SUCEM_FORM_ESUST)[0] == 'e');
    CHECK(sutr_sucem_form_name(SUCEM_FORM_NONE)[0] == 'u');
    CHECK(sutr_sucem_form_by_name("SUTF-8") == SUCEM_FORM_SUTF8);
    CHECK(sutr_sucem_form_by_name("SUTF-16") == SUCEM_FORM_SUTF16);
    CHECK(sutr_sucem_form_by_name("vSUTF") == SUCEM_FORM_VSUTF);
    CHECK(sutr_sucem_form_by_name("SUST-N") == SUCEM_FORM_SUSTN);
    CHECK(sutr_sucem_form_by_name("e-SUST") == SUCEM_FORM_ESUST);
    CHECK(sutr_sucem_form_by_name("nonesuch") == SUCEM_FORM_NONE);
    CHECK(sutr_sucem_form_by_name(NULL) == SUCEM_FORM_NONE);

    /* e-SUST is the Transfer Encoding Syntax; no SUTF is. */
    CHECK(sutr_sucem_is_tes(SUCEM_FORM_ESUST) == true);
    CHECK(sutr_sucem_is_tes(SUCEM_FORM_SUTF8) == false);
    CHECK(sutr_sucem_is_tes(SUCEM_FORM_SUST16) == false);
}

/* ── The three §7 consequences ─────────────────────────────────────── */

static void test_seven_consequences(void)
{
    /* §7.1 — a fixed 32-bit form is not an SUTF form. */
    CHECK(sutr_sucem_name_status("SUTF-32", SUCEM_LEVEL_NONE) == SUCEM_NAME_FORBIDDEN_CEF);
    CHECK(sutr_sucem_is_cef_name("SUTF-32") == false);
    CHECK(sutr_sucem_form_by_name("SUTF-32") == SUCEM_FORM_NONE);

    /* §7.2 — little-endian is not an SUTF form. */
    CHECK(sutr_sucem_name_status("SUTF-16-LE", SUCEM_LEVEL_NONE) == SUCEM_NAME_FORBIDDEN_CEF);
    CHECK(sutr_sucem_is_cef_name("SUTF-16-LE") == false);

    /* §7.3 — SUTF-8 is byte-oriented, so its CES is the identity. */
    CHECK(sutr_sucem_name_status("SUST-8", SUCEM_LEVEL_NONE) == SUCEM_NAME_FORBIDDEN_CES);
    CHECK(sutr_sucem_is_ces_name("SUST-8") == false);

    /* The legitimate forms are accepted at their own level. */
    CHECK(sutr_sucem_name_status("SUTF-8", SUCEM_LEVEL_NONE) == SUCEM_NAME_OK);
    CHECK(sutr_sucem_name_status("SUTF-16", SUCEM_LEVEL_NONE) == SUCEM_NAME_OK);
    CHECK(sutr_sucem_name_status("SUTF-4", SUCEM_LEVEL_NONE) == SUCEM_NAME_OK);
    CHECK(sutr_sucem_name_status("SUTF-2", SUCEM_LEVEL_NONE) == SUCEM_NAME_OK);
    CHECK(sutr_sucem_name_status("vSUTF", SUCEM_LEVEL_NONE) == SUCEM_NAME_OK);
    CHECK(sutr_sucem_name_status("SUST-16", SUCEM_LEVEL_NONE) == SUCEM_NAME_OK);
    CHECK(sutr_sucem_name_status("SUST-512", SUCEM_LEVEL_NONE) == SUCEM_NAME_OK);
    CHECK(sutr_sucem_name_status("SUST-N", SUCEM_LEVEL_NONE) == SUCEM_NAME_OK);
    CHECK(sutr_sucem_name_status("e-SUST", SUCEM_LEVEL_NONE) == SUCEM_NAME_OK);

    /* A CES name is not a CEF name and vice versa. */
    CHECK(sutr_sucem_is_cef_name("SUTF-8") == true);
    CHECK(sutr_sucem_is_ces_name("SUTF-8") == false);
    CHECK(sutr_sucem_is_ces_name("SUST-16") == true);
    CHECK(sutr_sucem_is_cef_name("SUST-16") == false);

    /* A name that exists at the other level reports the level mismatch. */
    CHECK(sutr_sucem_name_status("SUTF-8", SUCEM_LEVEL_CEF) == SUCEM_NAME_OK);
    CHECK(sutr_sucem_name_status("SUTF-8", SUCEM_LEVEL_CES) == SUCEM_NAME_WRONG_LEVEL);
    CHECK(sutr_sucem_name_status("SUST-16", SUCEM_LEVEL_CES) == SUCEM_NAME_OK);
    CHECK(sutr_sucem_name_status("SUST-16", SUCEM_LEVEL_CEF) == SUCEM_NAME_WRONG_LEVEL);

    /* A name that names no form at all. */
    CHECK(sutr_sucem_name_status("UTF-8", SUCEM_LEVEL_NONE) == SUCEM_NAME_UNKNOWN);
    CHECK(sutr_sucem_name_status("", SUCEM_LEVEL_NONE) == SUCEM_NAME_UNKNOWN);
    CHECK(sutr_sucem_name_status(NULL, SUCEM_LEVEL_NONE) == SUCEM_NAME_UNKNOWN);

    /* The status taxonomy. */
    CHECK(sutr_sucem_name_status_name(SUCEM_NAME_OK)[0] == 'o');
    CHECK(sutr_sucem_name_status_name(SUCEM_NAME_UNKNOWN)[0] == 'u');
    CHECK(sutr_sucem_name_status_name(SUCEM_NAME_FORBIDDEN_CEF)[0] == 'f');
    CHECK(sutr_sucem_name_status_name(SUCEM_NAME_FORBIDDEN_CES)[0] == 'f');
    CHECK(sutr_sucem_name_status_name(SUCEM_NAME_WRONG_LEVEL)[0] == 'w');
}

/* ── Code units and framed widths (§5, §6) ─────────────────────────── */

static void test_code_units(void)
{
    /* SUTF-8 spans 1 to 6 bytes over the 31-bit space. */
    CHECK(sutr_sucem_form_unit_count(0x00000041u, SUCEM_FORM_SUTF8) == 1U);
    CHECK(sutr_sucem_form_unit_count(0x0000007Fu, SUCEM_FORM_SUTF8) == 1U);
    CHECK(sutr_sucem_form_unit_count(0x00000080u, SUCEM_FORM_SUTF8) == 2U);
    CHECK(sutr_sucem_form_unit_count(0x000007FFu, SUCEM_FORM_SUTF8) == 2U);
    CHECK(sutr_sucem_form_unit_count(0x00000800u, SUCEM_FORM_SUTF8) == 3U);
    CHECK(sutr_sucem_form_unit_count(0x0000FFFFu, SUCEM_FORM_SUTF8) == 3U);
    CHECK(sutr_sucem_form_unit_count(0x00010000u, SUCEM_FORM_SUTF8) == 4U);
    CHECK(sutr_sucem_form_unit_count(0x0010FFFFu, SUCEM_FORM_SUTF8) == 4U);
    CHECK(sutr_sucem_form_unit_count(0x00110000u, SUCEM_FORM_SUTF8) == 5U);
    CHECK(sutr_sucem_form_unit_count(0x03FFFFFFu, SUCEM_FORM_SUTF8) == 5U);
    CHECK(sutr_sucem_form_unit_count(0x04000000u, SUCEM_FORM_SUTF8) == 6U);
    CHECK(sutr_sucem_form_unit_count(0x7FFFFFEFu, SUCEM_FORM_SUTF8) == 6U);

    /* SUTF-16: one word up to 0x7FFF, two words above. */
    CHECK(sutr_sucem_form_unit_count(0x00000000u, SUCEM_FORM_SUTF16) == 1U);
    CHECK(sutr_sucem_form_unit_count(0x00007FFFu, SUCEM_FORM_SUTF16) == 1U);
    CHECK(sutr_sucem_form_unit_count(0x00008000u, SUCEM_FORM_SUTF16) == 2U);
    CHECK(sutr_sucem_form_unit_count(0x0010FFFFu, SUCEM_FORM_SUTF16) == 2U);

    /* SUTF-4 and SUTF-2 are fixed-length. */
    CHECK(sutr_sucem_form_unit_count(0x00000000u, SUCEM_FORM_SUTF4) == 8U);
    CHECK(sutr_sucem_form_unit_count(0x7FFFFFEFu, SUCEM_FORM_SUTF4) == 8U);
    CHECK(sutr_sucem_form_unit_count(0x00000000u, SUCEM_FORM_SUTF2) == 16U);
    CHECK(sutr_sucem_form_unit_count(0x7FFFFFEFu, SUCEM_FORM_SUTF2) == 16U);

    /* vSUTF's unit count is not a function of the codepoint alone. */
    CHECK(sutr_sucem_form_unit_count(0x00000041u, SUCEM_FORM_VSUTF) == 0U);

    /* A CES has no code units; it frames bytes. */
    CHECK(sutr_sucem_form_unit_count(0x00000041u, SUCEM_FORM_SUST16) == 0U);
    CHECK(sutr_sucem_form_unit_bits(SUCEM_FORM_SUST16) == 0U);

    /* The code-unit widths. */
    CHECK(sutr_sucem_form_unit_bits(SUCEM_FORM_SUTF8) == 8U);
    CHECK(sutr_sucem_form_unit_bits(SUCEM_FORM_SUTF16) == 16U);
    CHECK(sutr_sucem_form_unit_bits(SUCEM_FORM_SUTF4) == 4U);
    CHECK(sutr_sucem_form_unit_bits(SUCEM_FORM_SUTF2) == 2U);
    CHECK(sutr_sucem_form_unit_bits(SUCEM_FORM_VSUTF) == 8U);

    /* The CES word widths. */
    CHECK(sutr_sucem_ces_word_bits(SUCEM_FORM_SUST16) == 16U);
    CHECK(sutr_sucem_ces_word_bits(SUCEM_FORM_SUST32) == 32U);
    CHECK(sutr_sucem_ces_word_bits(SUCEM_FORM_SUST64) == 64U);
    CHECK(sutr_sucem_ces_word_bits(SUCEM_FORM_SUST128) == 128U);
    CHECK(sutr_sucem_ces_word_bits(SUCEM_FORM_SUST256) == 256U);
    CHECK(sutr_sucem_ces_word_bits(SUCEM_FORM_SUST512) == 512U);
    CHECK(sutr_sucem_ces_word_bits(SUCEM_FORM_SUSTN) == 0U);
    CHECK(sutr_sucem_ces_word_bits(SUCEM_FORM_ESUST) == 0U);
    CHECK(sutr_sucem_ces_word_bits(SUCEM_FORM_SUTF8) == 0U);

    /* An e-SUST frame carries a BANcode in 6 bytes. */
    CHECK(sutr_sucem_esust_bancode_frame_bytes() == 6U);

    /* A trap slot and the Sentinel carry no encoded data. */
    CHECK(sutr_sucem_form_unit_count(0x7FFFFFF0u, SUCEM_FORM_SUTF8) == 0U);
    CHECK(sutr_sucem_form_unit_count(0x7FFFFFFFu, SUCEM_FORM_SUTF8) == 0U);
}

/* ── The repertoire sources (§3) ───────────────────────────────────── */

static void test_repertoire(void)
{
    CHECK(sutr_sucem_repertoire_of(0x00000000u) == SUCEM_REPERTOIRE_UNICODE);
    CHECK(sutr_sucem_repertoire_of(0x0010FFFFu) == SUCEM_REPERTOIRE_UNICODE);
    CHECK(sutr_sucem_repertoire_of(0x00110000u) == SUCEM_REPERTOIRE_SCC);
    CHECK(sutr_sucem_repertoire_of(0x0011FFFFu) == SUCEM_REPERTOIRE_SCC);
    CHECK(sutr_sucem_repertoire_of(0x00120000u) == SUCEM_REPERTOIRE_NATIVE);
    CHECK(sutr_sucem_repertoire_of(0x0015FFFFu) == SUCEM_REPERTOIRE_NATIVE);
    CHECK(sutr_sucem_repertoire_of(0x00160000u) == SUCEM_REPERTOIRE_RNUR);
    CHECK(sutr_sucem_repertoire_of(0x0016FFFFu) == SUCEM_REPERTOIRE_RNUR);
    CHECK(sutr_sucem_repertoire_of(0x00170000u) == SUCEM_REPERTOIRE_NATIVE);
    CHECK(sutr_sucem_repertoire_of(0x7FFFFFEFu) == SUCEM_REPERTOIRE_NATIVE);
    CHECK(sutr_sucem_repertoire_of(0x7FFFFFF0u) == SUCEM_REPERTOIRE_TRAP);
    CHECK(sutr_sucem_repertoire_of(0x7FFFFFFEu) == SUCEM_REPERTOIRE_TRAP);
    CHECK(sutr_sucem_repertoire_of(0x7FFFFFFFu) == SUCEM_REPERTOIRE_SENTINEL);
    CHECK(sutr_sucem_repertoire_of(0x80000000u) == SUCEM_REPERTOIRE_NONE);

    CHECK(sutr_sucem_is_unicode_repertoire(0x00000041u) == true);
    CHECK(sutr_sucem_is_scc_repertoire(0x00110000u) == true);
    CHECK(sutr_sucem_is_rnur_repertoire(0x00160000u) == true);
    CHECK(sutr_sucem_is_rnur_repertoire(0x00120000u) == false);

    CHECK(sutr_sucem_is_trap(0x7FFFFFF0u) == true);
    CHECK(sutr_sucem_is_trap(0x7FFFFFFFu) == false);
    CHECK(sutr_sucem_is_sentinel(0x7FFFFFFFu) == true);
    CHECK(sutr_sucem_is_sentinel(0x7FFFFFFEu) == false);

    /* The identity Character Map covers exactly the Bridge. */
    CHECK(sutr_sucem_identity_cm_covers(0x00000000u) == true);
    CHECK(sutr_sucem_identity_cm_covers(0x0010FFFFu) == true);
    CHECK(sutr_sucem_identity_cm_covers(0x00110000u) == false);

    /* The repertoire taxonomy names. */
    CHECK(sutr_sucem_repertoire_name(SUCEM_REPERTOIRE_UNICODE)[0] == 'u');
    CHECK(sutr_sucem_repertoire_name(SUCEM_REPERTOIRE_RNUR)[0] == 'r');
    CHECK(sutr_sucem_repertoire_name(SUCEM_REPERTOIRE_SCC)[0] == 's');
    CHECK(sutr_sucem_repertoire_name(SUCEM_REPERTOIRE_NONE)[0] == 'n');
}

/* ── The SCP four-level anchor (§8) ────────────────────────────────── */

static void test_scp_anchor(void)
{
    sucem_scp_anchor_t anchor;

    /* The codepoint 0x0011_AABB: class byte AAh, argument byte BBh. */
    CHECK(sutr_sucem_scp_class_byte(0x0011AABBu) == 0xAAu);
    CHECK(sutr_sucem_scp_argument_byte(0x0011AABBu) == 0xBBu);

    /* The severity clusters follow the directive class byte. */
    CHECK(sutr_sucem_scp_cluster_of(0x0011A000u) == SUCEM_SCP_CLUSTER_B_PLUS);
    CHECK(sutr_sucem_scp_cluster_of(0x0011A7FFu) == SUCEM_SCP_CLUSTER_B_PLUS);
    CHECK(sutr_sucem_scp_cluster_of(0x0011A800u) == SUCEM_SCP_CLUSTER_W_PLUS);
    CHECK(sutr_sucem_scp_cluster_of(0x0011ABFFu) == SUCEM_SCP_CLUSTER_W_PLUS);
    CHECK(sutr_sucem_scp_cluster_of(0x0011AC00u) == SUCEM_SCP_CLUSTER_C_PLUS);
    CHECK(sutr_sucem_scp_cluster_of(0x0011ADFFu) == SUCEM_SCP_CLUSTER_C_PLUS);
    CHECK(sutr_sucem_scp_cluster_of(0x0011AE00u) == SUCEM_SCP_CLUSTER_S_PLUS);
    CHECK(sutr_sucem_scp_cluster_of(0x0011AEFFu) == SUCEM_SCP_CLUSTER_S_PLUS);
    CHECK(sutr_sucem_scp_cluster_of(0x00119FFFu) == SUCEM_SCP_CLUSTER_NONE);
    CHECK(sutr_sucem_scp_cluster_of(0x00000041u) == SUCEM_SCP_CLUSTER_NONE);

    CHECK(sutr_sucem_is_bancode(0x0011A000u) == true);
    CHECK(sutr_sucem_is_warncode(0x0011A800u) == true);
    CHECK(sutr_sucem_is_comcode(0x0011AC00u) == true);
    CHECK(sutr_sucem_is_softcode(0x0011AE00u) == true);
    CHECK(sutr_sucem_is_bancode(0x0011A800u) == false);

    CHECK(sutr_sucem_scp_cluster_name(SUCEM_SCP_CLUSTER_B_PLUS)[0] == 'b');
    CHECK(sutr_sucem_scp_cluster_name(SUCEM_SCP_CLUSTER_NONE)[0] == 'n');

    /* The same codepoint wears a different role at each of the four levels. */
    CHECK(sutr_sucem_scp_anchor(0x0011A000u, (uint8_t)SUCEM_LEVEL_ACR,
                                &anchor) == true);
    CHECK(anchor.role == SUCEM_SCP_ROLE_DIRECTIVE);
    CHECK(anchor.cluster == SUCEM_SCP_CLUSTER_B_PLUS);
    CHECK(anchor.class_byte == 0xA0u);
    CHECK(anchor.argument_byte == 0x00u);
    CHECK(anchor.index == 0u);

    CHECK(sutr_sucem_scp_anchor(0x0011A000u, (uint8_t)SUCEM_LEVEL_CCS,
                                &anchor) == true);
    CHECK(anchor.role == SUCEM_SCP_ROLE_ADDRESS);

    CHECK(sutr_sucem_scp_anchor(0x0011A000u, (uint8_t)SUCEM_LEVEL_CEF,
                                &anchor) == true);
    CHECK(anchor.role == SUCEM_SCP_ROLE_TRANSFORMED);

    CHECK(sutr_sucem_scp_anchor(0x0011A000u, (uint8_t)SUCEM_LEVEL_CES,
                                &anchor) == true);
    CHECK(anchor.role == SUCEM_SCP_ROLE_FRAMED);

    /* The index is the offset within the severity cluster. */
    CHECK(sutr_sucem_scp_anchor(0x0011A55Au, (uint8_t)SUCEM_LEVEL_CCS,
                                &anchor) == true);
    CHECK(anchor.index == 0x55Au);
    CHECK(sutr_sucem_scp_anchor(0x0011A800u, (uint8_t)SUCEM_LEVEL_CCS,
                                &anchor) == true);
    CHECK(anchor.index == 0u);
    CHECK(anchor.cluster == SUCEM_SCP_CLUSTER_W_PLUS);

    /* Outside the four levels, and outside the SCP, the anchor fails. */
    CHECK(sutr_sucem_scp_anchor(0x0011A000u, 0u, &anchor) == false);
    CHECK(sutr_sucem_scp_anchor(0x0011A000u, 5u, &anchor) == false);
    CHECK(sutr_sucem_scp_anchor(0x00000041u, (uint8_t)SUCEM_LEVEL_CCS,
                                &anchor) == false);
    CHECK(sutr_sucem_scp_anchor(0x0011A000u, (uint8_t)SUCEM_LEVEL_CCS,
                                NULL) == false);
}

/* ── Trap dispatch geometry (§8.1) ─────────────────────────────────── */

static void test_trap_dispatch(void)
{
    uint32_t trap = 0u;
    uint32_t low  = 0u;
    uint32_t high = 0u;

    /* The first B+ BANcode maps to the first trap slot. */
    CHECK(sutr_sucem_bancode_to_trap(0x0011A000u, &trap) == true);
    CHECK(trap == 0x7FFFFFF0u);

    /* The mapping is O(1) arithmetic, not a table. */
    CHECK(sutr_sucem_bancode_to_trap(0x0011A07Fu, &trap) == true);
    CHECK(trap == 0x7FFFFFF0u);
    CHECK(sutr_sucem_bancode_to_trap(0x0011A080u, &trap) == true);
    CHECK(trap == 0x7FFFFFF1u);
    CHECK(sutr_sucem_bancode_to_trap(0x0011A77Fu, &trap) == true);
    CHECK(trap == 0x7FFFFFFEu);

    /* Partial coverage: the topmost 128 B+ codepoints are unmapped. */
    CHECK(sutr_sucem_bancode_to_trap(0x0011A780u, &trap) == false);
    CHECK(sutr_sucem_bancode_to_trap(0x0011A7FFu, &trap) == false);

    /* B+ only: W+, C+ and S+ are never dispatch targets. */
    CHECK(sutr_sucem_bancode_to_trap(0x0011A800u, &trap) == false);
    CHECK(sutr_sucem_bancode_to_trap(0x0011AC00u, &trap) == false);
    CHECK(sutr_sucem_bancode_to_trap(0x0011AE00u, &trap) == false);
    CHECK(sutr_sucem_bancode_to_trap(0x00000041u, &trap) == false);
    CHECK(sutr_sucem_bancode_to_trap(0x0011A000u, NULL) == false);

    /* The inverse is total over the 15 trap slots. */
    CHECK(sutr_sucem_trap_to_bancode_range(0x7FFFFFF0u, &low, &high) == true);
    CHECK(low == 0x0011A000u);
    CHECK(high == 0x0011A07Fu);
    CHECK(sutr_sucem_trap_to_bancode_range(0x7FFFFFFEu, &low, &high) == true);
    CHECK(low == 0x0011A700u);
    CHECK(high == 0x0011A77Fu);

    /* The Sentinel and every value outside the trap range are rejected. */
    CHECK(sutr_sucem_trap_to_bancode_range(0x7FFFFFFFu, &low, &high) == false);
    CHECK(sutr_sucem_trap_to_bancode_range(0x7FFFFFEFu, &low, &high) == false);
    CHECK(sutr_sucem_trap_to_bancode_range(0x00000041u, &low, &high) == false);
    CHECK(sutr_sucem_trap_to_bancode_range(0x7FFFFFF0u, NULL, &high) == false);
    CHECK(sutr_sucem_trap_to_bancode_range(0x7FFFFFF0u, &low, NULL) == false);

    /* The mapping round-trips for every covered BANcode. */
    {
        uint32_t cp;
        uint32_t count = 0u;

        for (cp = SUCEM_BANCODE_BPLUS_MIN; cp <= SUCEM_BANCODE_BPLUS_MAX; cp++) {
            if (sutr_sucem_bancode_to_trap(cp, &trap) == true) {
                CHECK(sutr_sucem_trap_to_bancode_range(trap, &low, &high) == true);
                CHECK(cp >= low && cp <= high);
                count++;
            }
        }
        CHECK(count == 1920u);
    }
}

/* ── Registration in SUAS-010 (SSRE) ────────────────────────────────── */

static void test_ssre_registration(void)
{
    CHECK(suas_ssre_is_registered(SUCEM_SPEC_ID) == true);
    CHECK(suas_ssre_module_of(SUCEM_SPEC_ID) != NULL);
    CHECK(suas_ssre_module_of(SUCEM_SPEC_ID)[0] == 's');
}

int main(void)
{
    test_version_and_meta();
    test_levels();
    test_seven_consequences();
    test_code_units();
    test_repertoire();
    test_scp_anchor();
    test_trap_dispatch();
    test_ssre_registration();

    if (g_fail != 0) {
        printf("FAIL: SUTR-7 SUCEM tests failed (%d)\n", g_fail);
        return 1;
    }
    printf("[PASS] SUTR-7 SUCEM tests passed\n");
    return 0;
}
