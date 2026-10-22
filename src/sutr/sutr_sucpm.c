/* SUTR-8 — SuperUnicode Character Property Model (SUCPM)
 *
 * Reference implementation. See include/sutr/sutr_sucpm.h for the API and
 * docs/sutr/SUTR-8-sucpm.md for the normative specification.
 *
 * Freestanding C99: static tables only, no libc, no allocation.
 */

#include "sutr/sutr_sucpm.h"

/* ── Mask composition helpers ──────────────────────────────────────── */

#define SUCPM_BRIDGE_BASE (SUCPM_M_TIER(SUCPM_TIER_BRIDGE) | SUCPM_F_ASSIGNED)

/* ── The Bridge range list (§4.1) ──────────────────────────────────────
 *
 * A sorted, non-overlapping list of `<lo><hi><mask>` triples. The lookup is
 * a binary search: O(log R), the one tier whose cost depends on table size.
 *
 * The list below is a curated sample of the SUCD property data. It is
 * version data, not engine logic: the engine's contract is the *shape* of
 * the table and the dispatch, not the contents of this particular snapshot.
 */

typedef struct {
    uint32_t        low;
    uint32_t        high;
    sucpm_flags64_t mask;
} sucpm_bridge_range_t;

static const sucpm_bridge_range_t g_sucpm_bridge[] = {
    /* ── C0 and C1 controls ── */
    { 0x0000u, 0x0008u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_CC) |
                          SUCPM_M_LB(SUCPM_LB_CM) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x0009u, 0x0009u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_CC) |
                          SUCPM_M_LB(SUCPM_LB_BA) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x000Au, 0x000Au, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_CC) |
                          SUCPM_M_LB(SUCPM_LB_LF) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x000Bu, 0x000Bu, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_CC) |
                          SUCPM_M_LB(SUCPM_LB_BK) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x000Cu, 0x000Cu, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_CC) |
                          SUCPM_M_LB(SUCPM_LB_CB) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x000Du, 0x000Du, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_CC) |
                          SUCPM_M_LB(SUCPM_LB_CR) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x000Eu, 0x001Fu, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_CC) |
                          SUCPM_M_LB(SUCPM_LB_CM) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },

    /* ── Basic Latin ── */
    { 0x0020u, 0x0020u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_ZS) |
                          SUCPM_F_WHITE_SPACE | SUCPM_M_LB(SUCPM_LB_SP) |
                          SUCPM_M_BIDI(SUCPM_BIDI_WHITESPACE) |
                          SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x0021u, 0x0023u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_PO) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x0024u, 0x0024u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_SC) |
                          SUCPM_M_LB(SUCPM_LB_OP) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x0025u, 0x0027u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_PO) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x0028u, 0x0028u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_PS) |
                          SUCPM_F_MATH | SUCPM_M_MC(SUCPM_MC_O) |
                          SUCPM_M_LB(SUCPM_LB_OP) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x0029u, 0x0029u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_PE) |
                          SUCPM_F_MATH | SUCPM_M_MC(SUCPM_MC_C) |
                          SUCPM_M_LB(SUCPM_LB_CL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x002Au, 0x002Bu, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_PO) |
                          SUCPM_F_MATH | SUCPM_M_MC(SUCPM_MC_B) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x002Cu, 0x002Cu, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_PO) |
                          SUCPM_F_MATH | SUCPM_M_MC(SUCPM_MC_P) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x002Du, 0x002Du, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_PD) |
                          SUCPM_F_MATH | SUCPM_M_MC(SUCPM_MC_B) |
                          SUCPM_M_LB(SUCPM_LB_HY) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x002Eu, 0x002Eu, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_PO) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x002Fu, 0x002Fu, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_PO) |
                          SUCPM_F_MATH | SUCPM_M_MC(SUCPM_MC_B) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x0030u, 0x0039u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_ND) |
                          SUCPM_F_NUMBER | SUCPM_M_NT(SUCPM_NT_DECIMAL) |
                          SUCPM_M_LB(SUCPM_LB_NU) |
                          SUCPM_M_BIDI(SUCPM_BIDI_NEUTRAL) |
                          SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x003Au, 0x003Bu, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_PO) |
                          SUCPM_F_MATH | SUCPM_M_MC(SUCPM_MC_P) |
                          SUCPM_M_LB(SUCPM_LB_IS) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x003Cu, 0x003Eu, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_SM) |
                          SUCPM_F_MATH | SUCPM_M_MC(SUCPM_MC_R) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x003Fu, 0x0040u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_PO) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x0041u, 0x005Au, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_LU) |
                          SUCPM_F_ALPHABETIC | SUCPM_F_UPPERCASE |
                          SUCPM_M_BIDI(SUCPM_BIDI_LTR) |
                          SUCPM_M_SCR(SUCPM_SCRIPT_LATIN) },
    { 0x005Bu, 0x005Bu, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_PS) |
                          SUCPM_F_MATH | SUCPM_M_MC(SUCPM_MC_O) |
                          SUCPM_M_LB(SUCPM_LB_OP) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x005Cu, 0x005Cu, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_PO) |
                          SUCPM_M_LB(SUCPM_LB_PR) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x005Du, 0x005Du, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_PE) |
                          SUCPM_F_MATH | SUCPM_M_MC(SUCPM_MC_C) |
                          SUCPM_M_LB(SUCPM_LB_CL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x005Eu, 0x0060u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_SM) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x0061u, 0x007Au, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_LL) |
                          SUCPM_F_ALPHABETIC | SUCPM_F_LOWERCASE |
                          SUCPM_M_BIDI(SUCPM_BIDI_LTR) |
                          SUCPM_M_SCR(SUCPM_SCRIPT_LATIN) },
    { 0x007Bu, 0x007Bu, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_PS) |
                          SUCPM_F_MATH | SUCPM_M_MC(SUCPM_MC_O) |
                          SUCPM_M_LB(SUCPM_LB_OP) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x007Cu, 0x007Cu, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_SM) |
                          SUCPM_F_MATH | SUCPM_M_MC(SUCPM_MC_F) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x007Du, 0x007Du, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_PE) |
                          SUCPM_F_MATH | SUCPM_M_MC(SUCPM_MC_C) |
                          SUCPM_M_LB(SUCPM_LB_CL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x007Eu, 0x007Eu, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_SM) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },

    { 0x007Fu, 0x009Fu, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_CC) |
                          SUCPM_M_LB(SUCPM_LB_CM) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },

    /* ── Latin-1 Supplement ── */
    { 0x00A0u, 0x00A0u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_ZS) |
                          SUCPM_F_WHITE_SPACE | SUCPM_M_LB(SUCPM_LB_SP) |
                          SUCPM_M_BIDI(SUCPM_BIDI_WHITESPACE) |
                          SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x00A1u, 0x00A1u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_PO) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x00A2u, 0x00A5u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_SC) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x00A6u, 0x00A9u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_SO) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x00AAu, 0x00AAu, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_LO) |
                          SUCPM_F_ALPHABETIC | SUCPM_M_BIDI(SUCPM_BIDI_LTR) |
                          SUCPM_M_SCR(SUCPM_SCRIPT_LATIN) },
    { 0x00ABu, 0x00ABu, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_PO) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x00ACu, 0x00ACu, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_SM) |
                          SUCPM_F_MATH | SUCPM_M_MC(SUCPM_MC_R) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x00ADu, 0x00ADu, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_CF) |
                          SUCPM_F_DEFAULT_IGNORABLE | SUCPM_F_BIDI_CONTROL |
                          SUCPM_M_LB(SUCPM_LB_CM) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x00AEu, 0x00AFu, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_SO) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x00B0u, 0x00B0u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_SO) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x00B1u, 0x00B1u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_SM) |
                          SUCPM_F_MATH | SUCPM_M_MC(SUCPM_MC_B) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x00B2u, 0x00B3u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_NO) |
                          SUCPM_F_NUMBER | SUCPM_M_NT(SUCPM_NT_NUMERIC) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x00B4u, 0x00B4u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_LM) |
                          SUCPM_F_ALPHABETIC | SUCPM_M_BIDI(SUCPM_BIDI_LTR) |
                          SUCPM_M_SCR(SUCPM_SCRIPT_LATIN) },
    { 0x00B5u, 0x00B5u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_LL) |
                          SUCPM_F_ALPHABETIC | SUCPM_F_LOWERCASE |
                          SUCPM_M_BIDI(SUCPM_BIDI_LTR) |
                          SUCPM_M_SCR(SUCPM_SCRIPT_LATIN) },
    { 0x00B6u, 0x00B8u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_PO) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x00B9u, 0x00B9u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_NO) |
                          SUCPM_F_NUMBER | SUCPM_M_NT(SUCPM_NT_NUMERIC) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x00BAu, 0x00BAu, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_LO) |
                          SUCPM_F_ALPHABETIC | SUCPM_M_BIDI(SUCPM_BIDI_LTR) |
                          SUCPM_M_SCR(SUCPM_SCRIPT_LATIN) },
    { 0x00BBu, 0x00BBu, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_PO) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x00BCu, 0x00BEu, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_NO) |
                          SUCPM_F_NUMBER | SUCPM_M_NT(SUCPM_NT_NUMERIC) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x00BFu, 0x00BFu, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_PO) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x00C0u, 0x00D6u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_LU) |
                          SUCPM_F_ALPHABETIC | SUCPM_F_UPPERCASE |
                          SUCPM_M_BIDI(SUCPM_BIDI_LTR) |
                          SUCPM_M_SCR(SUCPM_SCRIPT_LATIN) },
    { 0x00D7u, 0x00D7u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_SM) |
                          SUCPM_F_MATH | SUCPM_M_MC(SUCPM_MC_B) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x00D8u, 0x00DEu, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_LU) |
                          SUCPM_F_ALPHABETIC | SUCPM_F_UPPERCASE |
                          SUCPM_M_BIDI(SUCPM_BIDI_LTR) |
                          SUCPM_M_SCR(SUCPM_SCRIPT_LATIN) },
    { 0x00DFu, 0x00DFu, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_LL) |
                          SUCPM_F_ALPHABETIC | SUCPM_F_LOWERCASE |
                          SUCPM_M_BIDI(SUCPM_BIDI_LTR) |
                          SUCPM_M_SCR(SUCPM_SCRIPT_LATIN) },
    { 0x00E0u, 0x00F6u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_LL) |
                          SUCPM_F_ALPHABETIC | SUCPM_F_LOWERCASE |
                          SUCPM_M_BIDI(SUCPM_BIDI_LTR) |
                          SUCPM_M_SCR(SUCPM_SCRIPT_LATIN) },
    { 0x00F7u, 0x00F7u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_SM) |
                          SUCPM_F_MATH | SUCPM_M_MC(SUCPM_MC_B) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x00F8u, 0x00FFu, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_LL) |
                          SUCPM_F_ALPHABETIC | SUCPM_F_LOWERCASE |
                          SUCPM_M_BIDI(SUCPM_BIDI_LTR) |
                          SUCPM_M_SCR(SUCPM_SCRIPT_LATIN) },

    /* ── Latin Extended-A ── */
    { 0x0100u, 0x012Fu, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_LU) |
                          SUCPM_F_ALPHABETIC | SUCPM_F_UPPERCASE |
                          SUCPM_M_BIDI(SUCPM_BIDI_LTR) |
                          SUCPM_M_SCR(SUCPM_SCRIPT_LATIN) },
    { 0x0130u, 0x0148u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_LL) |
                          SUCPM_F_ALPHABETIC | SUCPM_F_LOWERCASE |
                          SUCPM_M_BIDI(SUCPM_BIDI_LTR) |
                          SUCPM_M_SCR(SUCPM_SCRIPT_LATIN) },
    { 0x0149u, 0x017Fu, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_LU) |
                          SUCPM_F_ALPHABETIC | SUCPM_F_UPPERCASE |
                          SUCPM_M_BIDI(SUCPM_BIDI_LTR) |
                          SUCPM_M_SCR(SUCPM_SCRIPT_LATIN) },

    /* ── Combining diacritical marks, with their combining classes ── */
    { 0x0300u, 0x0314u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_MN) |
                          SUCPM_F_MARK | SUCPM_M_CCC(230) |
                          SUCPM_M_SCR(SUCPM_SCRIPT_INHERITED) },
    { 0x0315u, 0x0315u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_MN) |
                          SUCPM_F_MARK | SUCPM_M_CCC(232) |
                          SUCPM_M_SCR(SUCPM_SCRIPT_INHERITED) },
    { 0x0316u, 0x0319u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_MN) |
                          SUCPM_F_MARK | SUCPM_M_CCC(220) |
                          SUCPM_M_SCR(SUCPM_SCRIPT_INHERITED) },
    { 0x031Au, 0x031Au, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_MN) |
                          SUCPM_F_MARK | SUCPM_M_CCC(232) |
                          SUCPM_M_SCR(SUCPM_SCRIPT_INHERITED) },
    { 0x031Bu, 0x031Bu, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_MN) |
                          SUCPM_F_MARK | SUCPM_M_CCC(216) |
                          SUCPM_M_SCR(SUCPM_SCRIPT_INHERITED) },
    { 0x031Cu, 0x0320u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_MN) |
                          SUCPM_F_MARK | SUCPM_M_CCC(220) |
                          SUCPM_M_SCR(SUCPM_SCRIPT_INHERITED) },
    { 0x0321u, 0x0322u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_MN) |
                          SUCPM_F_MARK | SUCPM_M_CCC(202) |
                          SUCPM_M_SCR(SUCPM_SCRIPT_INHERITED) },
    { 0x0323u, 0x0326u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_MN) |
                          SUCPM_F_MARK | SUCPM_M_CCC(220) |
                          SUCPM_M_SCR(SUCPM_SCRIPT_INHERITED) },
    { 0x0327u, 0x0328u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_MN) |
                          SUCPM_F_MARK | SUCPM_M_CCC(202) |
                          SUCPM_M_SCR(SUCPM_SCRIPT_INHERITED) },
    { 0x0329u, 0x0333u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_MN) |
                          SUCPM_F_MARK | SUCPM_M_CCC(220) |
                          SUCPM_M_SCR(SUCPM_SCRIPT_INHERITED) },
    { 0x0334u, 0x0338u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_MN) |
                          SUCPM_F_MARK | SUCPM_M_CCC(1) |
                          SUCPM_M_SCR(SUCPM_SCRIPT_INHERITED) },
    { 0x0339u, 0x033Fu, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_MN) |
                          SUCPM_F_MARK | SUCPM_M_CCC(220) |
                          SUCPM_M_SCR(SUCPM_SCRIPT_INHERITED) },
    { 0x0340u, 0x0346u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_MN) |
                          SUCPM_F_MARK | SUCPM_M_CCC(230) |
                          SUCPM_M_SCR(SUCPM_SCRIPT_INHERITED) },
    { 0x0347u, 0x034Bu, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_MN) |
                          SUCPM_F_MARK | SUCPM_M_CCC(220) |
                          SUCPM_M_SCR(SUCPM_SCRIPT_INHERITED) },
    { 0x034Cu, 0x034Eu, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_MN) |
                          SUCPM_F_MARK | SUCPM_M_CCC(230) |
                          SUCPM_M_SCR(SUCPM_SCRIPT_INHERITED) },
    { 0x034Fu, 0x035Fu, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_MN) |
                          SUCPM_F_MARK | SUCPM_M_CCC(220) |
                          SUCPM_M_SCR(SUCPM_SCRIPT_INHERITED) },
    { 0x0360u, 0x0361u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_MN) |
                          SUCPM_F_MARK | SUCPM_M_CCC(234) |
                          SUCPM_M_SCR(SUCPM_SCRIPT_INHERITED) },
    { 0x0362u, 0x036Fu, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_MN) |
                          SUCPM_F_MARK | SUCPM_M_CCC(233) |
                          SUCPM_M_SCR(SUCPM_SCRIPT_INHERITED) },

    /* ── Greek ── */
    { 0x0370u, 0x0373u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_LU) |
                          SUCPM_F_ALPHABETIC | SUCPM_F_UPPERCASE |
                          SUCPM_M_BIDI(SUCPM_BIDI_LTR) |
                          SUCPM_M_SCR(SUCPM_SCRIPT_GREEK) },
    { 0x0374u, 0x0375u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_SK) |
                          SUCPM_M_SCR(SUCPM_SCRIPT_GREEK) },
    { 0x0376u, 0x0377u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_LU) |
                          SUCPM_F_ALPHABETIC | SUCPM_F_UPPERCASE |
                          SUCPM_M_BIDI(SUCPM_BIDI_LTR) |
                          SUCPM_M_SCR(SUCPM_SCRIPT_GREEK) },
    { 0x037Au, 0x037Du, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_LU) |
                          SUCPM_F_ALPHABETIC | SUCPM_F_UPPERCASE |
                          SUCPM_M_BIDI(SUCPM_BIDI_LTR) |
                          SUCPM_M_SCR(SUCPM_SCRIPT_GREEK) },
    { 0x037Eu, 0x037Fu, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_LU) |
                          SUCPM_F_ALPHABETIC | SUCPM_F_UPPERCASE |
                          SUCPM_M_BIDI(SUCPM_BIDI_LTR) |
                          SUCPM_M_SCR(SUCPM_SCRIPT_GREEK) },
    { 0x0384u, 0x0385u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_SK) |
                          SUCPM_M_SCR(SUCPM_SCRIPT_GREEK) },
    { 0x0386u, 0x0386u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_LU) |
                          SUCPM_F_ALPHABETIC | SUCPM_F_UPPERCASE |
                          SUCPM_M_BIDI(SUCPM_BIDI_LTR) |
                          SUCPM_M_SCR(SUCPM_SCRIPT_GREEK) },
    { 0x0389u, 0x038Bu, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_LU) |
                          SUCPM_F_ALPHABETIC | SUCPM_F_UPPERCASE |
                          SUCPM_M_BIDI(SUCPM_BIDI_LTR) |
                          SUCPM_M_SCR(SUCPM_SCRIPT_GREEK) },
    { 0x038Cu, 0x038Cu, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_LL) |
                          SUCPM_F_ALPHABETIC | SUCPM_F_LOWERCASE |
                          SUCPM_M_BIDI(SUCPM_BIDI_LTR) |
                          SUCPM_M_SCR(SUCPM_SCRIPT_GREEK) },
    { 0x038Eu, 0x03A1u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_LL) |
                          SUCPM_F_ALPHABETIC | SUCPM_F_LOWERCASE |
                          SUCPM_M_BIDI(SUCPM_BIDI_LTR) |
                          SUCPM_M_SCR(SUCPM_SCRIPT_GREEK) },
    { 0x03A3u, 0x03ABu, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_LU) |
                          SUCPM_F_ALPHABETIC | SUCPM_F_UPPERCASE |
                          SUCPM_M_BIDI(SUCPM_BIDI_LTR) |
                          SUCPM_M_SCR(SUCPM_SCRIPT_GREEK) },
    { 0x03ACu, 0x03CEu, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_LL) |
                          SUCPM_F_ALPHABETIC | SUCPM_F_LOWERCASE |
                          SUCPM_M_BIDI(SUCPM_BIDI_LTR) |
                          SUCPM_M_SCR(SUCPM_SCRIPT_GREEK) },
    { 0x03CFu, 0x03D3u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_LU) |
                          SUCPM_F_ALPHABETIC | SUCPM_F_UPPERCASE |
                          SUCPM_M_BIDI(SUCPM_BIDI_LTR) |
                          SUCPM_M_SCR(SUCPM_SCRIPT_GREEK) },
    { 0x03D5u, 0x03D6u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_LL) |
                          SUCPM_F_ALPHABETIC | SUCPM_F_LOWERCASE |
                          SUCPM_M_BIDI(SUCPM_BIDI_LTR) |
                          SUCPM_M_SCR(SUCPM_SCRIPT_GREEK) },
    { 0x03D7u, 0x03D7u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_LU) |
                          SUCPM_F_ALPHABETIC | SUCPM_F_UPPERCASE |
                          SUCPM_M_BIDI(SUCPM_BIDI_LTR) |
                          SUCPM_M_SCR(SUCPM_SCRIPT_GREEK) },
    { 0x03D8u, 0x03EFu, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_LL) |
                          SUCPM_F_ALPHABETIC | SUCPM_F_LOWERCASE |
                          SUCPM_M_BIDI(SUCPM_BIDI_LTR) |
                          SUCPM_M_SCR(SUCPM_SCRIPT_GREEK) },
    { 0x03F0u, 0x03FFu, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_LU) |
                          SUCPM_F_ALPHABETIC | SUCPM_F_UPPERCASE |
                          SUCPM_M_BIDI(SUCPM_BIDI_LTR) |
                          SUCPM_M_SCR(SUCPM_SCRIPT_GREEK) },

    /* ── Cyrillic ── */
    { 0x0400u, 0x042Fu, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_LU) |
                          SUCPM_F_ALPHABETIC | SUCPM_F_UPPERCASE |
                          SUCPM_M_BIDI(SUCPM_BIDI_LTR) |
                          SUCPM_M_SCR(SUCPM_SCRIPT_CYRILLIC) },
    { 0x0430u, 0x045Fu, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_LL) |
                          SUCPM_F_ALPHABETIC | SUCPM_F_LOWERCASE |
                          SUCPM_M_BIDI(SUCPM_BIDI_LTR) |
                          SUCPM_M_SCR(SUCPM_SCRIPT_CYRILLIC) },
    { 0x0460u, 0x0481u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_LU) |
                          SUCPM_F_ALPHABETIC | SUCPM_F_UPPERCASE |
                          SUCPM_M_BIDI(SUCPM_BIDI_LTR) |
                          SUCPM_M_SCR(SUCPM_SCRIPT_CYRILLIC) },
    { 0x048Au, 0x04FFu, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_LL) |
                          SUCPM_F_ALPHABETIC | SUCPM_F_LOWERCASE |
                          SUCPM_M_BIDI(SUCPM_BIDI_LTR) |
                          SUCPM_M_SCR(SUCPM_SCRIPT_CYRILLIC) },

    /* ── Hebrew and Arabic ── */
    { 0x05D0u, 0x05EAu, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_LO) |
                          SUCPM_F_ALPHABETIC | SUCPM_M_BIDI(SUCPM_BIDI_RTL) |
                          SUCPM_M_SCR(SUCPM_SCRIPT_HEBREW) },
    { 0x05EFu, 0x05F2u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_LO) |
                          SUCPM_F_ALPHABETIC | SUCPM_M_BIDI(SUCPM_BIDI_RTL) |
                          SUCPM_M_SCR(SUCPM_SCRIPT_HEBREW) },
    { 0x0621u, 0x063Au, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_LO) |
                          SUCPM_F_ALPHABETIC | SUCPM_M_BIDI(SUCPM_BIDI_ARABIC_AL) |
                          SUCPM_M_SCR(SUCPM_SCRIPT_ARABIC) },
    { 0x0641u, 0x064Au, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_LO) |
                          SUCPM_F_ALPHABETIC | SUCPM_M_BIDI(SUCPM_BIDI_ARABIC_AL) |
                          SUCPM_M_SCR(SUCPM_SCRIPT_ARABIC) },

    /* ── Devanagari ── */
    { 0x0904u, 0x0939u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_LO) |
                          SUCPM_F_ALPHABETIC | SUCPM_M_BIDI(SUCPM_BIDI_LTR) |
                          SUCPM_M_SCR(SUCPM_SCRIPT_DEVANAGARI) },

    /* ── General punctuation ── */
    { 0x2000u, 0x200Au, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_ZS) |
                          SUCPM_F_WHITE_SPACE | SUCPM_M_LB(SUCPM_LB_SP) |
                          SUCPM_M_BIDI(SUCPM_BIDI_WHITESPACE) |
                          SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x200Bu, 0x200Bu, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_CF) |
                          SUCPM_F_DEFAULT_IGNORABLE | SUCPM_M_LB(SUCPM_LB_WJ) |
                          SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x200Cu, 0x200Du, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_CF) |
                          SUCPM_F_DEFAULT_IGNORABLE | SUCPM_F_JOIN_CONTROL |
                          SUCPM_M_LB(SUCPM_LB_CM) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x200Eu, 0x200Eu, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_CF) |
                          SUCPM_F_DEFAULT_IGNORABLE | SUCPM_F_BIDI_CONTROL |
                          SUCPM_M_LB(SUCPM_LB_L) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x200Fu, 0x200Fu, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_CF) |
                          SUCPM_F_DEFAULT_IGNORABLE | SUCPM_F_BIDI_CONTROL |
                          SUCPM_M_LB(SUCPM_LB_R) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x2010u, 0x2027u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_PO) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x2028u, 0x2028u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_ZL) |
                          SUCPM_F_WHITE_SPACE | SUCPM_M_LB(SUCPM_LB_LF) |
                          SUCPM_M_BIDI(SUCPM_BIDI_WHITESPACE) |
                          SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x2029u, 0x2029u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_ZP) |
                          SUCPM_F_WHITE_SPACE | SUCPM_M_LB(SUCPM_LB_LF) |
                          SUCPM_M_BIDI(SUCPM_BIDI_WHITESPACE) |
                          SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x202Fu, 0x202Fu, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_ZS) |
                          SUCPM_F_WHITE_SPACE | SUCPM_M_LB(SUCPM_LB_SP) |
                          SUCPM_M_BIDI(SUCPM_BIDI_WHITESPACE) |
                          SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x2030u, 0x2034u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_PO) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x2035u, 0x2043u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_PO) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x2044u, 0x2044u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_SM) |
                          SUCPM_F_MATH | SUCPM_M_MC(SUCPM_MC_B) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x2045u, 0x2046u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_PS) |
                          SUCPM_M_LB(SUCPM_LB_OP) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x2047u, 0x2051u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_PO) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x2052u, 0x2052u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_PS) |
                          SUCPM_M_LB(SUCPM_LB_OP) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x2053u, 0x205Eu, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_PO) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x205Fu, 0x205Fu, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_PO) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x2060u, 0x2064u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_CF) |
                          SUCPM_F_DEFAULT_IGNORABLE | SUCPM_M_LB(SUCPM_LB_CM) |
                          SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x2066u, 0x2066u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_CF) |
                          SUCPM_F_DEFAULT_IGNORABLE | SUCPM_F_BIDI_CONTROL |
                          SUCPM_M_LB(SUCPM_LB_CM) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x2067u, 0x2067u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_CF) |
                          SUCPM_F_DEFAULT_IGNORABLE | SUCPM_F_BIDI_CONTROL |
                          SUCPM_M_LB(SUCPM_LB_CM) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x2068u, 0x2068u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_CF) |
                          SUCPM_F_DEFAULT_IGNORABLE | SUCPM_F_BIDI_CONTROL |
                          SUCPM_M_LB(SUCPM_LB_CM) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x2069u, 0x2069u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_CF) |
                          SUCPM_F_DEFAULT_IGNORABLE | SUCPM_F_BIDI_CONTROL |
                          SUCPM_M_LB(SUCPM_LB_CM) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x206Au, 0x206Fu, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_CF) |
                          SUCPM_F_DEFAULT_IGNORABLE | SUCPM_F_BIDI_CONTROL |
                          SUCPM_M_LB(SUCPM_LB_CM) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },

    /* ── Currency symbols ── */
    { 0x20A0u, 0x20BFu, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_SC) |
                          SUCPM_M_LB(SUCPM_LB_PR) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },

    /* ── Letterlike symbols: the letterlike mathematics of SUTR-9 §2.1 ── */
    { 0x2100u, 0x2101u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_SO) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x2102u, 0x2102u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_SO) |
                          SUCPM_F_MATH | SUCPM_M_MC(SUCPM_MC_N) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x2103u, 0x2106u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_SO) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x2107u, 0x2107u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_SO) |
                          SUCPM_F_MATH | SUCPM_M_MC(SUCPM_MC_N) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x2108u, 0x2109u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_SO) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x210Au, 0x210Bu, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_SO) |
                          SUCPM_F_MATH | SUCPM_M_MC(SUCPM_MC_N) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x210Cu, 0x210Cu, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_SO) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x210Du, 0x210Du, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_SO) |
                          SUCPM_F_MATH | SUCPM_M_MC(SUCPM_MC_N) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x210Eu, 0x210Eu, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_LM) |
                          SUCPM_F_MATH | SUCPM_F_ALPHABETIC |
                          SUCPM_M_MC(SUCPM_MC_N) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x210Fu, 0x210Fu, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_SO) |
                          SUCPM_F_MATH | SUCPM_M_MC(SUCPM_MC_N) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x2110u, 0x2111u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_SO) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x2112u, 0x2113u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_SO) |
                          SUCPM_F_MATH | SUCPM_M_MC(SUCPM_MC_N) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x2114u, 0x2114u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_SO) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x2115u, 0x2115u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_SO) |
                          SUCPM_F_MATH | SUCPM_M_MC(SUCPM_MC_N) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x2116u, 0x2118u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_NO) |
                          SUCPM_F_NUMBER | SUCPM_M_LB(SUCPM_LB_AL) |
                          SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x2119u, 0x211Au, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_SO) |
                          SUCPM_F_MATH | SUCPM_M_MC(SUCPM_MC_N) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x211Bu, 0x2123u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_SO) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x2124u, 0x2124u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_SO) |
                          SUCPM_F_MATH | SUCPM_M_MC(SUCPM_MC_N) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x2125u, 0x212Bu, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_SO) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x212Cu, 0x212Cu, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_SO) |
                          SUCPM_F_MATH | SUCPM_M_MC(SUCPM_MC_N) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x212Du, 0x212Eu, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_SO) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x212Fu, 0x2131u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_SO) |
                          SUCPM_F_MATH | SUCPM_M_MC(SUCPM_MC_N) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x2132u, 0x2132u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_SO) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x2133u, 0x2134u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_SO) |
                          SUCPM_F_MATH | SUCPM_M_MC(SUCPM_MC_N) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x2135u, 0x2135u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_SO) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x2136u, 0x2138u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_SO) |
                          SUCPM_F_MATH | SUCPM_M_MC(SUCPM_MC_N) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x2139u, 0x2140u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_SO) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x2141u, 0x2142u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_SO) |
                          SUCPM_F_MATH | SUCPM_M_MC(SUCPM_MC_N) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x2143u, 0x2149u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_SM) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x214Au, 0x214Fu, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_SO) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },

    /* ── Arrows ── */
    { 0x2190u, 0x2199u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_SM) |
                          SUCPM_F_MATH | SUCPM_M_MC(SUCPM_MC_R) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },

    /* ── Mathematical operators ── */
    { 0x2200u, 0x2204u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_SM) |
                          SUCPM_F_MATH | SUCPM_M_MC(SUCPM_MC_B) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x2205u, 0x2207u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_SM) |
                          SUCPM_F_MATH | SUCPM_M_MC(SUCPM_MC_S) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x2208u, 0x220Bu, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_SM) |
                          SUCPM_F_MATH | SUCPM_M_MC(SUCPM_MC_R) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x220Cu, 0x220Eu, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_SM) |
                          SUCPM_F_MATH | SUCPM_M_MC(SUCPM_MC_N) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x220Fu, 0x220Fu, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_SM) |
                          SUCPM_F_MATH | SUCPM_M_MC(SUCPM_MC_U) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x2210u, 0x2211u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_SM) |
                          SUCPM_F_MATH | SUCPM_M_MC(SUCPM_MC_U) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x2212u, 0x2219u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_SM) |
                          SUCPM_F_MATH | SUCPM_M_MC(SUCPM_MC_B) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x221Au, 0x221Cu, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_SM) |
                          SUCPM_F_MATH | SUCPM_M_MC(SUCPM_MC_U) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x221Du, 0x221Du, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_SM) |
                          SUCPM_F_MATH | SUCPM_M_MC(SUCPM_MC_B) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x221Eu, 0x221Eu, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_SM) |
                          SUCPM_F_MATH | SUCPM_M_MC(SUCPM_MC_N) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x221Fu, 0x221Fu, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_SM) |
                          SUCPM_F_MATH | SUCPM_M_MC(SUCPM_MC_R) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x2220u, 0x2227u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_SM) |
                          SUCPM_F_MATH | SUCPM_M_MC(SUCPM_MC_R) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x2228u, 0x222Au, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_SM) |
                          SUCPM_F_MATH | SUCPM_M_MC(SUCPM_MC_B) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x222Bu, 0x222Du, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_SM) |
                          SUCPM_F_MATH | SUCPM_M_MC(SUCPM_MC_U) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x222Eu, 0x222Eu, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_SM) |
                          SUCPM_F_MATH | SUCPM_M_MC(SUCPM_MC_R) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x222Fu, 0x222Fu, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_SM) |
                          SUCPM_F_MATH | SUCPM_M_MC(SUCPM_MC_R) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x2230u, 0x2233u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_SM) |
                          SUCPM_F_MATH | SUCPM_M_MC(SUCPM_MC_R) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x2234u, 0x2237u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_SM) |
                          SUCPM_F_MATH | SUCPM_M_MC(SUCPM_MC_R) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x223Cu, 0x223Cu, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_SM) |
                          SUCPM_F_MATH | SUCPM_M_MC(SUCPM_MC_R) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x223Du, 0x223Du, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_SM) |
                          SUCPM_F_MATH | SUCPM_M_MC(SUCPM_MC_R) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x2248u, 0x2248u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_SM) |
                          SUCPM_F_MATH | SUCPM_M_MC(SUCPM_MC_R) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x2260u, 0x2261u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_SM) |
                          SUCPM_F_MATH | SUCPM_M_MC(SUCPM_MC_R) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x2264u, 0x2265u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_SM) |
                          SUCPM_F_MATH | SUCPM_M_MC(SUCPM_MC_R) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x226Au, 0x226Bu, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_SM) |
                          SUCPM_F_MATH | SUCPM_M_MC(SUCPM_MC_R) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x2270u, 0x2271u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_SM) |
                          SUCPM_F_MATH | SUCPM_M_MC(SUCPM_MC_R) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x2282u, 0x2287u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_SM) |
                          SUCPM_F_MATH | SUCPM_M_MC(SUCPM_MC_R) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x2295u, 0x2297u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_SM) |
                          SUCPM_F_MATH | SUCPM_M_MC(SUCPM_MC_R) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x22C5u, 0x22C5u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_SM) |
                          SUCPM_F_MATH | SUCPM_M_MC(SUCPM_MC_B) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x2308u, 0x230Bu, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_SM) |
                          SUCPM_F_MATH | SUCPM_M_MC(SUCPM_MC_F) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x2329u, 0x232Au, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_PS) |
                          SUCPM_F_MATH | SUCPM_M_MC(SUCPM_MC_F) |
                          SUCPM_M_LB(SUCPM_LB_OP) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },

    /* ── Enclosed alphanumerics and CJK ── */
    { 0x2460u, 0x24FFu, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_NO) |
                          SUCPM_F_NUMBER | SUCPM_M_LB(SUCPM_LB_AL) |
                          SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },

    /* -- Miscellaneous Mathematical Symbols-A (SUTR-9 2.1) -- */
    { 0x27C0u, 0x27D3u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_SO) |
                          SUCPM_F_MATH | SUCPM_M_MC(SUCPM_MC_B) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x27D4u, 0x27DEu, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_SO) |
                          SUCPM_F_MATH | SUCPM_M_MC(SUCPM_MC_R) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x27DFu, 0x27DFu, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_SO) |
                          SUCPM_F_MATH | SUCPM_M_MC(SUCPM_MC_B) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x27E0u, 0x27E7u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_SO) |
                          SUCPM_F_MATH | SUCPM_M_MC(SUCPM_MC_R) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x27E8u, 0x27E9u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_SO) |
                          SUCPM_F_MATH | SUCPM_M_MC(SUCPM_MC_F) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x27EAu, 0x27EFu, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_SO) |
                          SUCPM_F_MATH | SUCPM_M_MC(SUCPM_MC_R) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },

    /* -- Miscellaneous Mathematical Symbols-B (SUTR-9 2.1) -- */
    { 0x2980u, 0x2983u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_SO) |
                          SUCPM_F_MATH | SUCPM_M_MC(SUCPM_MC_C) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x2984u, 0x29F9u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_SO) |
                          SUCPM_F_MATH | SUCPM_M_MC(SUCPM_MC_R) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x29FAu, 0x29FFu, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_SO) |
                          SUCPM_F_MATH | SUCPM_M_MC(SUCPM_MC_F) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },

    /* -- Supplemental Mathematical Operators (SUTR-9 2.1) -- */
    { 0x2A00u, 0x2A1Fu, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_SO) |
                          SUCPM_F_MATH | SUCPM_M_MC(SUCPM_MC_U) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x2A20u, 0x2A4Fu, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_SO) |
                          SUCPM_F_MATH | SUCPM_M_MC(SUCPM_MC_B) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x2A50u, 0x2AFFu, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_SO) |
                          SUCPM_F_MATH | SUCPM_M_MC(SUCPM_MC_R) |
                          SUCPM_M_LB(SUCPM_LB_AL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x3000u, 0x3000u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_ZS) |
                          SUCPM_F_WHITE_SPACE | SUCPM_M_LB(SUCPM_LB_SP) |
                          SUCPM_M_BIDI(SUCPM_BIDI_WHITESPACE) |
                          SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x3001u, 0x3004u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_PO) |
                          SUCPM_M_LB(SUCPM_LB_CL) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x3005u, 0x3007u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_LO) |
                          SUCPM_F_ALPHABETIC | SUCPM_M_BIDI(SUCPM_BIDI_LTR) |
                          SUCPM_M_SCR(SUCPM_SCRIPT_HAN) },
    { 0x3008u, 0x3020u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_PO) |
                          SUCPM_M_LB(SUCPM_LB_OP) | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x3021u, 0x3029u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_NO) |
                          SUCPM_F_NUMBER | SUCPM_M_LB(SUCPM_LB_CL) |
                          SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x3041u, 0x3096u, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_LO) |
                          SUCPM_F_ALPHABETIC | SUCPM_M_BIDI(SUCPM_BIDI_LTR) |
                          SUCPM_M_SCR(SUCPM_SCRIPT_HIRAGANA) },
    { 0x30A1u, 0x30FAu, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_LO) |
                          SUCPM_F_ALPHABETIC | SUCPM_M_BIDI(SUCPM_BIDI_LTR) |
                          SUCPM_M_SCR(SUCPM_SCRIPT_KATAKANA) },
    { 0x4E00u, 0x9FFFu, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_LO) |
                          SUCPM_F_ALPHABETIC | SUCPM_M_BIDI(SUCPM_BIDI_LTR) |
                          SUCPM_M_SCR(SUCPM_SCRIPT_HAN) },

    /* ── Private use, noncharacters, and the supplementary planes ── */
    { 0xD800u, 0xDFFFu, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_CO) |
                          SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0xE000u, 0xF8FFu, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_CO) |
                          SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },

    { 0xFDD0u, 0xFDEFu, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_CO) |
                          SUCPM_F_NONCHARACTER | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },

    /* -- Mathematical Alphanumeric Symbols (SUTR-9 2.1, 2.2) -- */
    { 0x1D400u, 0x1D7CBu, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_LU) |
                          SUCPM_F_MATH | SUCPM_F_ALPHABETIC |
                          SUCPM_M_MC(SUCPM_MC_N) |
                          SUCPM_M_BIDI(SUCPM_BIDI_LTR) |
                          SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x1D7CEu, 0x1D7FFu, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_ND) |
                          SUCPM_F_MATH | SUCPM_F_NUMBER |
                          SUCPM_M_NT(SUCPM_NT_DECIMAL) |
                          SUCPM_M_MC(SUCPM_MC_N) |
                          SUCPM_M_BIDI(SUCPM_BIDI_NEUTRAL) |
                          SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },

    /* -- Arabic Mathematical Alphabetic Symbols (SUTR-9 2.1) -- */
    { 0x1EE00u, 0x1EEFFu, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_LO) |
                          SUCPM_F_MATH | SUCPM_F_ALPHABETIC |
                          SUCPM_M_MC(SUCPM_MC_N) |
                          SUCPM_M_BIDI(SUCPM_BIDI_RTL) |
                          SUCPM_M_SCR(SUCPM_SCRIPT_ARABIC) },
    { 0xF0000u, 0xFFFFDu, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_CO) |
                          SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x100000u, 0x10FFFDu, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_CO) |
                          SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) },
    { 0x10FFFFu, 0x10FFFFu, SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_CN) |
                          SUCPM_F_NONCHARACTER | SUCPM_M_SCR(SUCPM_SCRIPT_COMMON) }
};

#define SUCPM_BRIDGE_COUNT (sizeof(g_sucpm_bridge) / sizeof(g_sucpm_bridge[0]))

/* ── The Native and plugin range registers (§4.3, §4.5) ─────────────── */

typedef struct {
    uint64_t       low;
    uint64_t       high;
    sucpm_flags64_t mask;
} sucpm_range_register_t;

static sucpm_range_register_t g_sucpm_native[SUCPM_NATIVE_REGISTER_CAPACITY];
static uint32_t g_sucpm_native_count = 0u;

static sucpm_range_register_t g_sucpm_plugin[SUCPM_PLUGIN_REGISTER_CAPACITY];
static uint32_t g_sucpm_plugin_count = 0u;

/* The Native Math ExtSUCS Space (SUTR-9 §3.2) is the one Native range the
 * base standard knows about without a plugin. */
#define SUCPM_NATIVE_MATH_MIN 0x00170000ULL
#define SUCPM_NATIVE_MATH_MAX 0x0017FFFFULL

/* ── Local helpers ─────────────────────────────────────────────────── */

static bool sucpm_streq(const char *a, const char *b)
{
    size_t i = 0u;

    if (a == NULL || b == NULL) {
        return false;
    }
    while (a[i] != '\0' && a[i] == b[i]) {
        i++;
    }
    return (a[i] == b[i]);
}

/* The Bridge lookup: a binary search over the sorted range list (§4.1). */
static sucpm_flags64_t sucpm_bridge_lookup(uint32_t codepoint)
{
    size_t lo = 0u;
    size_t hi = SUCPM_BRIDGE_COUNT;

    while (lo < hi) {
        size_t mid = lo + ((hi - lo) / 2u);
        const sucpm_bridge_range_t *r = &g_sucpm_bridge[mid];

        if (codepoint < r->low) {
            hi = mid;
        } else if (codepoint > r->high) {
            lo = mid + 1u;
        } else {
            return r->mask;
        }
    }
    /* An unassigned Bridge codepoint: assigned is false, GC is Cn. */
    return SUCPM_BRIDGE_BASE | SUCPM_M_GC(SUCPM_GC_CN);
}

uint32_t sutr_sucpm_bridge_range_count(void)
{
    return (uint32_t)SUCPM_BRIDGE_COUNT;
}

bool sutr_sucpm_bridge_range_at(uint32_t index, uint32_t *out_low,
                                uint32_t *out_high)
{
    if (index >= (uint32_t)SUCPM_BRIDGE_COUNT) {
        return false;
    }
    if (out_low != NULL) {
        *out_low = g_sucpm_bridge[index].low;
    }
    if (out_high != NULL) {
        *out_high = g_sucpm_bridge[index].high;
    }
    return true;
}

/* The Native range register lookup: a bounded linear scan over a
 * compile-time-constant number of registers. */
static bool sucpm_native_lookup(uint64_t codepoint, sucpm_flags64_t *out_mask)
{
    uint32_t i;

    for (i = 0u; i < g_sucpm_native_count; i++) {
        if (codepoint >= g_sucpm_native[i].low &&
            codepoint <= g_sucpm_native[i].high) {
            *out_mask = g_sucpm_native[i].mask;
            return true;
        }
    }
    return false;
}

/* The plugin manifest range register lookup (§4.5). */
static bool sucpm_plugin_lookup(uint64_t codepoint, sucpm_flags64_t *out_mask)
{
    uint32_t i;

    for (i = 0u; i < g_sucpm_plugin_count; i++) {
        if (codepoint >= g_sucpm_plugin[i].low &&
            codepoint <= g_sucpm_plugin[i].high) {
            *out_mask = g_sucpm_plugin[i].mask;
            return true;
        }
    }
    return false;
}

/* ── The dispatch (§4.6) ───────────────────────────────────────────── */

const char *sutr_sucpm_version_string(void)
{
    return "0.1.0";
}

uint32_t sutr_sucpm_tier_of(uint64_t codepoint)
{
    if (codepoint <= SUCPM_BRIDGE_MAX) {
        return SUCPM_TIER_BRIDGE;
    }
    if (codepoint <= SUCPM_SCP_MAX) {
        return SUCPM_TIER_SCP;
    }
    if (codepoint <= SUCPM_NATIVE_LOW_MAX) {
        return SUCPM_TIER_NATIVE;
    }
    if (codepoint <= SUCPM_RNUR_MAX) {
        return SUCPM_TIER_RNUR;
    }
    if (codepoint <= SUCPM_NATIVE_MAX) {
        return SUCPM_TIER_NATIVE;
    }
    if (codepoint <= SUCPM_SENTINEL) {
        return SUCPM_TIER_RESERVED;
    }
    return SUCPM_TIER_EXTSUCS;
}

sucpm_flags64_t sutr_sucpm_props(uint64_t codepoint)
{
    /* 1. Bridge — 0x000000-0x10FFFF (§4.1). */
    if (codepoint <= SUCPM_BRIDGE_MAX) {
        return sucpm_bridge_lookup((uint32_t)codepoint);
    }

    /* 2. System Control Plane — 0x110000-0x11FFFF (§4.2). */
    if (codepoint <= SUCPM_SCP_MAX) {
        uint8_t  argument = (uint8_t)(codepoint & 0xFFu);
        uint64_t flags    = SUCPM_M_TIER(SUCPM_TIER_SCP) |
                           SUCPM_F_DEFAULT_IGNORABLE;

        /* The class byte bears the cluster identity; the argument byte bears
         * the directional flag, the isolate/embed constraint, the break
         * constraint, and the LTR/RTL marker. The mask carries the tier and
         * the Bidi field; the constraints are tested on Y directly. */
        if ((argument & (uint8_t)SUCPM_SCP_ARG_RTL) != 0u) {
            flags |= SUCPM_M_BIDI(SUCPM_BIDI_RTL);
        } else if ((argument & (uint8_t)SUCPM_SCP_ARG_LTR) != 0u) {
            flags |= SUCPM_M_BIDI(SUCPM_BIDI_LTR);
        } else {
            flags |= SUCPM_M_BIDI(SUCPM_BIDI_NEUTRAL);
        }
        return flags;
    }

    /* 3. Native low — 0x120000-0x15FFFF (§4.3). */
    if (codepoint <= SUCPM_NATIVE_LOW_MAX) {
        sucpm_flags64_t mask;

        if (sucpm_native_lookup(codepoint, &mask) == true) {
            return mask;
        }
        return SUCPM_M_TIER(SUCPM_TIER_NATIVE);
    }

    /* 4. The RNUR registry — 0x160000-0x16FFFF (§4.4). */
    if (codepoint <= SUCPM_RNUR_MAX) {
        return SUCPM_M_TIER(SUCPM_TIER_RNUR) | SUCPM_F_RNUR;
    }

    /* 5. Native high — 0x170000-0x7FFFFFEF (§4.3). */
    if (codepoint <= SUCPM_NATIVE_MAX) {
        sucpm_flags64_t mask;

        if (sucpm_native_lookup(codepoint, &mask) == true) {
            return mask;
        }
        if (codepoint >= SUCPM_NATIVE_MATH_MIN &&
            codepoint <= SUCPM_NATIVE_MATH_MAX) {
            return SUCPM_M_TIER(SUCPM_TIER_NATIVE) | SUCPM_F_MATH |
                   SUCPM_M_MC(SUCPM_MC_N);
        }
        return SUCPM_M_TIER(SUCPM_TIER_NATIVE);
    }

    /* 6. The terminal 16 — the trap slots and the Sentinel (§4.6). */
    if (codepoint <= SUCPM_SENTINEL) {
        return (sucpm_flags64_t)SUCPM_MASK_RESERVED;
    }

    /* 7. The plugin manifest — above the Sentinel (§4.5). */
    {
        sucpm_flags64_t mask;

        if (sucpm_plugin_lookup(codepoint, &mask) == true) {
            return mask;
        }
        return SUCPM_M_TIER(SUCPM_TIER_EXTSUCS);
    }
}

/* ── The O(1) property tests (§5.3) ────────────────────────────────── */

bool sutr_sucpm_has(sucpm_flags64_t mask, uint64_t bits)
{
    return (mask & bits) == bits;
}

bool sutr_sucpm_is(sucpm_flags64_t mask, uint64_t field_shift,
                   uint64_t value)
{
    uint64_t width;

    /* The field width follows from the shift: the packed fields are not all
     * eight bits wide, so the width cannot be assumed. */
    switch (field_shift) {
    case SUCPM_GC_SHIFT:           width = 0x1Fu; break;
    case SUCPM_CCC_SHIFT:          width = 0xFFu; break;
    case SUCPM_SCRIPT_SHIFT:       width = 0x3FFu; break;
    case SUCPM_LINEBREAK_SHIFT:    width = 0x3Fu; break;
    case SUCPM_NUMERICTYPE_SHIFT:  width = 0x7u; break;
    case SUCPM_MATHCLASS_SHIFT:    width = 0xFu; break;
    default:                       width = 0xFFu; break;
    }
    return ((mask >> field_shift) & width) == value;
}

uint8_t sutr_sucpm_gc(sucpm_flags64_t mask)
{
    return (uint8_t)((mask & SUCPM_GC_MASK) >> SUCPM_GC_SHIFT);
}

uint8_t sutr_sucpm_ccc(sucpm_flags64_t mask)
{
    return (uint8_t)((mask & SUCPM_CCC_MASK) >> SUCPM_CCC_SHIFT);
}

uint16_t sutr_sucpm_script(sucpm_flags64_t mask)
{
    return (uint16_t)((mask & SUCPM_SCRIPT_MASK) >> SUCPM_SCRIPT_SHIFT);
}

uint8_t sutr_sucpm_linebreak(sucpm_flags64_t mask)
{
    return (uint8_t)((mask & SUCPM_LINEBREAK_MASK) >> SUCPM_LINEBREAK_SHIFT);
}

uint8_t sutr_sucpm_numeric_type(sucpm_flags64_t mask)
{
    return (uint8_t)((mask & SUCPM_NUMERICTYPE_MASK) >> SUCPM_NUMERICTYPE_SHIFT);
}

uint8_t sutr_sucpm_math_class(sucpm_flags64_t mask)
{
    return (uint8_t)((mask & SUCPM_MATHCLASS_MASK) >> SUCPM_MATHCLASS_SHIFT);
}

uint32_t sutr_sucpm_bidi(sucpm_flags64_t mask)
{
    return (uint32_t)(mask & SUCPM_BIDI_ALL);
}

bool sutr_sucpm_is_math(sucpm_flags64_t mask)
{
    return (mask & SUCPM_F_MATH) != 0u;
}

/* ── The System Control Plane decode (§4.2) ────────────────────────── */

uint8_t sutr_sucpm_scp_class_byte(uint64_t codepoint)
{
    return (uint8_t)((codepoint >> 8) & 0xFFu);
}

uint8_t sutr_sucpm_scp_argument_byte(uint64_t codepoint)
{
    return (uint8_t)(codepoint & 0xFFu);
}

sucpm_cluster_t sutr_sucpm_scp_cluster(uint64_t codepoint)
{
    uint8_t class_byte;

    if (codepoint < SUCPM_SCP_MIN || codepoint > SUCPM_SCP_MAX) {
        return SUCPM_CLUSTER_NONE;
    }
    class_byte = sutr_sucpm_scp_class_byte(codepoint);

    if (class_byte >= 0xA0u && class_byte <= 0xA7u) {
        return SUCPM_CLUSTER_B_PLUS;
    }
    if (class_byte >= 0xA8u && class_byte <= 0xABu) {
        return SUCPM_CLUSTER_W_PLUS;
    }
    if (class_byte >= 0xACu && class_byte <= 0xADu) {
        return SUCPM_CLUSTER_C_PLUS;
    }
    if (class_byte == 0xAEu) {
        return SUCPM_CLUSTER_S_PLUS;
    }
    return SUCPM_CLUSTER_NONE;
}

uint8_t sutr_sucpm_scp_argument_flags(uint64_t codepoint)
{
    if (codepoint < SUCPM_SCP_MIN || codepoint > SUCPM_SCP_MAX) {
        return 0u;
    }
    return (uint8_t)(sutr_sucpm_scp_argument_byte(codepoint) &
                     (uint8_t)SUCPM_SCP_ARG_ALL);
}

bool sutr_sucpm_scp_is_rtl(uint64_t codepoint)
{
    return (sutr_sucpm_scp_argument_flags(codepoint) &
            (uint8_t)SUCPM_SCP_ARG_RTL) != 0u;
}

bool sutr_sucpm_scp_is_ltr(uint64_t codepoint)
{
    return (sutr_sucpm_scp_argument_flags(codepoint) &
            (uint8_t)SUCPM_SCP_ARG_LTR) != 0u;
}

bool sutr_sucpm_scp_is_isolate(uint64_t codepoint)
{
    return (sutr_sucpm_scp_argument_flags(codepoint) &
            (uint8_t)SUCPM_SCP_ARG_ISOLATE) != 0u;
}

bool sutr_sucpm_scp_is_embed(uint64_t codepoint)
{
    return (sutr_sucpm_scp_argument_flags(codepoint) &
            (uint8_t)SUCPM_SCP_ARG_EMBED) != 0u;
}

bool sutr_sucpm_scp_forces_break(uint64_t codepoint)
{
    return (sutr_sucpm_scp_argument_flags(codepoint) &
            (uint8_t)SUCPM_SCP_ARG_BRK_MANDATORY) != 0u;
}

bool sutr_sucpm_scp_forbids_break(uint64_t codepoint)
{
    return (sutr_sucpm_scp_argument_flags(codepoint) &
            (uint8_t)SUCPM_SCP_ARG_BRK_PROHIBITED) != 0u;
}

/* ── Range register publication (§4.3, §4.5) ───────────────────────── */

bool sutr_sucpm_publish_native_range(uint64_t low, uint64_t high,
                                     sucpm_flags64_t mask)
{
    if (low > high) {
        return false;
    }
    if (g_sucpm_native_count >= SUCPM_NATIVE_REGISTER_CAPACITY) {
        return false;
    }
    g_sucpm_native[g_sucpm_native_count].low  = low;
    g_sucpm_native[g_sucpm_native_count].high = high;
    /* The tier is a property of the zone the register lives in, not of the
     * published data: the engine stamps it, and any tier bits the caller
     * supplied are discarded. */
    g_sucpm_native[g_sucpm_native_count].mask =
        (mask & ~(sucpm_flags64_t)SUCPM_TIER_MASK) |
        SUCPM_M_TIER(SUCPM_TIER_NATIVE);
    g_sucpm_native_count++;
    return true;
}

bool sutr_sucpm_publish_plugin_range(uint64_t low, uint64_t high,
                                     sucpm_flags64_t mask)
{
    if (low > high) {
        return false;
    }
    if (g_sucpm_plugin_count >= SUCPM_PLUGIN_REGISTER_CAPACITY) {
        return false;
    }
    g_sucpm_plugin[g_sucpm_plugin_count].low  = low;
    g_sucpm_plugin[g_sucpm_plugin_count].high = high;
    g_sucpm_plugin[g_sucpm_plugin_count].mask =
        (mask & ~(sucpm_flags64_t)SUCPM_TIER_MASK) |
        SUCPM_M_TIER(SUCPM_TIER_EXTSUCS);
    g_sucpm_plugin_count++;
    return true;
}

uint32_t sutr_sucpm_native_register_count(void)
{
    return g_sucpm_native_count;
}

uint32_t sutr_sucpm_plugin_register_count(void)
{
    return g_sucpm_plugin_count;
}

void sutr_sucpm_reset_registers(void)
{
    g_sucpm_native_count = 0u;
    g_sucpm_plugin_count = 0u;
}

/* ── The property registry (§3) ────────────────────────────────────── */

static const sucpm_property_record_t g_sucpm_properties[] = {
    { "General_Category", "General Category",
      SUCPM_TYPE_ENUMERATED, SUCPM_STATUS_NORMATIVE, SUCPM_CONTEXT_INDEPENDENT,
      SUCPM_STABILITY_STABLE, SUCPM_DERIVATION_SIMPLE,
      SUCPM_TIER_PROPERTY_NORMATIVE, 1u, 0u },
    { "Bidi_Class", "Bidi Class",
      SUCPM_TYPE_ENUMERATED, SUCPM_STATUS_NORMATIVE, SUCPM_CONTEXT_INDEPENDENT,
      SUCPM_STABILITY_STABLE, SUCPM_DERIVATION_SIMPLE,
      SUCPM_TIER_PROPERTY_NORMATIVE, 1u, 0u },
    { "Script_Identifier", "Script Identifier",
      SUCPM_TYPE_CATALOG, SUCPM_STATUS_NORMATIVE, SUCPM_CONTEXT_INDEPENDENT,
      SUCPM_STABILITY_STABLE, SUCPM_DERIVATION_SIMPLE,
      SUCPM_TIER_PROPERTY_NORMATIVE, 0u, 0u },
    { "Line_Break_Class", "Line Break Class",
      SUCPM_TYPE_CATALOG, SUCPM_STATUS_INFORMATIVE, SUCPM_CONTEXT_INDEPENDENT,
      SUCPM_STABILITY_STABLE, SUCPM_DERIVATION_DERIVED,
      SUCPM_TIER_PROPERTY_INFORMATIVE, 0u, 0u },
    { "Canonical_Combining_Class", "Canonical Combining Class",
      SUCPM_TYPE_NUMERIC, SUCPM_STATUS_NORMATIVE, SUCPM_CONTEXT_INDEPENDENT,
      SUCPM_STABILITY_STABLE, SUCPM_DERIVATION_SIMPLE,
      SUCPM_TIER_PROPERTY_NORMATIVE, 0u, 0u },
    { "Numeric_Type", "Numeric Type",
      SUCPM_TYPE_ENUMERATED, SUCPM_STATUS_INFORMATIVE, SUCPM_CONTEXT_INDEPENDENT,
      SUCPM_STABILITY_STABLE, SUCPM_DERIVATION_SIMPLE,
      SUCPM_TIER_PROPERTY_INFORMATIVE, 1u, 0u },
    { "Numeric_Value", "Numeric Value",
      SUCPM_TYPE_NUMERIC, SUCPM_STATUS_INFORMATIVE, SUCPM_CONTEXT_INDEPENDENT,
      SUCPM_STABILITY_STABLE, SUCPM_DERIVATION_DERIVED,
      SUCPM_TIER_PROPERTY_INFORMATIVE, 0u, 0u },
    { "Alphabetic", "Alphabetic",
      SUCPM_TYPE_BOOLEAN, SUCPM_STATUS_NORMATIVE, SUCPM_CONTEXT_INDEPENDENT,
      SUCPM_STABILITY_STABLE, SUCPM_DERIVATION_DERIVED,
      SUCPM_TIER_PROPERTY_NORMATIVE, 1u, 0u },
    { "Uppercase", "Uppercase",
      SUCPM_TYPE_BOOLEAN, SUCPM_STATUS_NORMATIVE, SUCPM_CONTEXT_INDEPENDENT,
      SUCPM_STABILITY_STABLE, SUCPM_DERIVATION_DERIVED,
      SUCPM_TIER_PROPERTY_NORMATIVE, 1u, 0u },
    { "Lowercase", "Lowercase",
      SUCPM_TYPE_BOOLEAN, SUCPM_STATUS_NORMATIVE, SUCPM_CONTEXT_INDEPENDENT,
      SUCPM_STABILITY_STABLE, SUCPM_DERIVATION_DERIVED,
      SUCPM_TIER_PROPERTY_NORMATIVE, 1u, 0u },
    { "White_Space", "White Space",
      SUCPM_TYPE_BOOLEAN, SUCPM_STATUS_NORMATIVE, SUCPM_CONTEXT_INDEPENDENT,
      SUCPM_STABILITY_STABLE, SUCPM_DERIVATION_SIMPLE,
      SUCPM_TIER_PROPERTY_NORMATIVE, 1u, 0u },
    { "Mark", "Mark",
      SUCPM_TYPE_BOOLEAN, SUCPM_STATUS_NORMATIVE, SUCPM_CONTEXT_INDEPENDENT,
      SUCPM_STABILITY_STABLE, SUCPM_DERIVATION_DERIVED,
      SUCPM_TIER_PROPERTY_NORMATIVE, 1u, 0u },
    { "Number", "Number",
      SUCPM_TYPE_BOOLEAN, SUCPM_STATUS_NORMATIVE, SUCPM_CONTEXT_INDEPENDENT,
      SUCPM_STABILITY_STABLE, SUCPM_DERIVATION_DERIVED,
      SUCPM_TIER_PROPERTY_NORMATIVE, 1u, 0u },
    { "Punctuation", "Punctuation",
      SUCPM_TYPE_BOOLEAN, SUCPM_STATUS_NORMATIVE, SUCPM_CONTEXT_INDEPENDENT,
      SUCPM_STABILITY_STABLE, SUCPM_DERIVATION_DERIVED,
      SUCPM_TIER_PROPERTY_NORMATIVE, 1u, 0u },
    { "Symbol", "Symbol",
      SUCPM_TYPE_BOOLEAN, SUCPM_STATUS_NORMATIVE, SUCPM_CONTEXT_INDEPENDENT,
      SUCPM_STABILITY_STABLE, SUCPM_DERIVATION_DERIVED,
      SUCPM_TIER_PROPERTY_NORMATIVE, 1u, 0u },
    { "Default_Ignorable_Code_Point", "Default Ignorable Code Point",
      SUCPM_TYPE_BOOLEAN, SUCPM_STATUS_NORMATIVE, SUCPM_CONTEXT_INDEPENDENT,
      SUCPM_STABILITY_STABLE, SUCPM_DERIVATION_SIMPLE,
      SUCPM_TIER_PROPERTY_NORMATIVE, 1u, 0u },
    { "Bidi_Control", "Bidi Control",
      SUCPM_TYPE_BOOLEAN, SUCPM_STATUS_NORMATIVE, SUCPM_CONTEXT_INDEPENDENT,
      SUCPM_STABILITY_STABLE, SUCPM_DERIVATION_SIMPLE,
      SUCPM_TIER_PROPERTY_NORMATIVE, 1u, 0u },
    { "Join_Control", "Join Control",
      SUCPM_TYPE_BOOLEAN, SUCPM_STATUS_NORMATIVE, SUCPM_CONTEXT_INDEPENDENT,
      SUCPM_STABILITY_STABLE, SUCPM_DERIVATION_SIMPLE,
      SUCPM_TIER_PROPERTY_NORMATIVE, 1u, 0u },
    { "Variation_Selector", "Variation Selector",
      SUCPM_TYPE_BOOLEAN, SUCPM_STATUS_NORMATIVE, SUCPM_CONTEXT_INDEPENDENT,
      SUCPM_STABILITY_STABLE, SUCPM_DERIVATION_SIMPLE,
      SUCPM_TIER_PROPERTY_NORMATIVE, 1u, 0u },
    { "Noncharacter_Code_Point", "Noncharacter Code Point",
      SUCPM_TYPE_BOOLEAN, SUCPM_STATUS_NORMATIVE, SUCPM_CONTEXT_INDEPENDENT,
      SUCPM_STABILITY_STABLE, SUCPM_DERIVATION_SIMPLE,
      SUCPM_TIER_PROPERTY_NORMATIVE, 1u, 0u },
    { "Assigned", "Assigned",
      SUCPM_TYPE_BOOLEAN, SUCPM_STATUS_INFORMATIVE, SUCPM_CONTEXT_INDEPENDENT,
      SUCPM_STABILITY_STABLE, SUCPM_DERIVATION_DERIVED,
      SUCPM_TIER_PROPERTY_INFORMATIVE, 1u, 0u },
    { "Canonical_Decomposition", "Canonical Decomposition",
      SUCPM_TYPE_STRING, SUCPM_STATUS_NORMATIVE, SUCPM_CONTEXT_INDEPENDENT,
      SUCPM_STABILITY_STABLE, SUCPM_DERIVATION_SIMPLE,
      SUCPM_TIER_PROPERTY_NORMATIVE, 0u, 0u },
    { "Full_Composition_Exclusion", "Full Composition Exclusion",
      SUCPM_TYPE_BOOLEAN, SUCPM_STATUS_NORMATIVE, SUCPM_CONTEXT_INDEPENDENT,
      SUCPM_STABILITY_STABLE, SUCPM_DERIVATION_SIMPLE,
      SUCPM_TIER_PROPERTY_NORMATIVE, 1u, 0u },
    { "Math", "Math",
      SUCPM_TYPE_BOOLEAN, SUCPM_STATUS_INFORMATIVE, SUCPM_CONTEXT_INDEPENDENT,
      SUCPM_STABILITY_STABLE, SUCPM_DERIVATION_SIMPLE,
      SUCPM_TIER_PROPERTY_INFORMATIVE, 1u, 0u },
    { "MathClass", "Math Class",
      SUCPM_TYPE_ENUMERATED, SUCPM_STATUS_INFORMATIVE, SUCPM_CONTEXT_INDEPENDENT,
      SUCPM_STABILITY_STABLE, SUCPM_DERIVATION_SIMPLE,
      SUCPM_TIER_PROPERTY_INFORMATIVE, 1u, 0u },
    { "RNUR", "RNUR Neography Registration",
      SUCPM_TYPE_BOOLEAN, SUCPM_STATUS_INFORMATIVE, SUCPM_CONTEXT_INDEPENDENT,
      SUCPM_STABILITY_STABLE, SUCPM_DERIVATION_SIMPLE,
      SUCPM_TIER_PROPERTY_CONTRIBUTED, 1u, 0u }
};

#define SUCPM_PROPERTY_COUNT (sizeof(g_sucpm_properties) / sizeof(g_sucpm_properties[0]))

uint32_t sutr_sucpm_property_count(void)
{
    return (uint32_t)SUCPM_PROPERTY_COUNT;
}

const sucpm_property_record_t *sutr_sucpm_property_at(uint32_t index)
{
    if (index >= (uint32_t)SUCPM_PROPERTY_COUNT) {
        return NULL;
    }
    return &g_sucpm_properties[index];
}

const sucpm_property_record_t *sutr_sucpm_property_by_alias(const char *alias)
{
    uint32_t i;

    if (alias == NULL) {
        return NULL;
    }
    for (i = 0u; i < (uint32_t)SUCPM_PROPERTY_COUNT; i++) {
        if (sucpm_streq(g_sucpm_properties[i].alias, alias)) {
            return &g_sucpm_properties[i];
        }
    }
    return NULL;
}

/* ── Taxonomy names ────────────────────────────────────────────────── */

const char *sutr_sucpm_property_type_name(sucpm_property_type_t type)
{
    switch (type) {
    case SUCPM_TYPE_ENUMERATED:  return "enumerated";
    case SUCPM_TYPE_BOOLEAN:     return "boolean";
    case SUCPM_TYPE_NUMERIC:     return "numeric";
    case SUCPM_TYPE_STRING:      return "string";
    case SUCPM_TYPE_IDENTIFIER:  return "identifier";
    case SUCPM_TYPE_CATALOG:     return "catalog";
    case SUCPM_TYPE_MISCELLANEOUS: return "miscellaneous";
    default:                     return "unknown";
    }
}

const char *sutr_sucpm_property_status_name(sucpm_property_status_t status)
{
    switch (status) {
    case SUCPM_STATUS_NORMATIVE:   return "normative";
    case SUCPM_STATUS_OVERRIDABLE: return "overridable";
    case SUCPM_STATUS_INFORMATIVE: return "informative";
    case SUCPM_STATUS_PROVISIONAL: return "provisional";
    default:                       return "unknown";
    }
}

const char *sutr_sucpm_context_name(sucpm_context_t context)
{
    switch (context) {
    case SUCPM_CONTEXT_DEPENDENT:   return "context-dependent";
    case SUCPM_CONTEXT_INDEPENDENT: return "context-independent";
    default:                        return "unknown";
    }
}

const char *sutr_sucpm_stability_name(sucpm_stability_t stability)
{
    switch (stability) {
    case SUCPM_STABILITY_STABLE:     return "stable";
    case SUCPM_STABILITY_FIXED:      return "fixed";
    case SUCPM_STABILITY_IMMUTABLE:  return "immutable";
    case SUCPM_STABILITY_STABILIZED: return "stabilized";
    case SUCPM_STABILITY_DEPRECATED: return "deprecated";
    default:                         return "unknown";
    }
}

const char *sutr_sucpm_derivation_name(sucpm_derivation_t derivation)
{
    switch (derivation) {
    case SUCPM_DERIVATION_SIMPLE: return "simple";
    case SUCPM_DERIVATION_DERIVED: return "derived";
    default:                     return "unknown";
    }
}

const char *sutr_sucpm_property_tier_name(sucpm_property_tier_t tier)
{
    switch (tier) {
    case SUCPM_TIER_PROPERTY_NORMATIVE:   return "normative";
    case SUCPM_TIER_PROPERTY_INFORMATIVE: return "informative";
    case SUCPM_TIER_PROPERTY_CONTRIBUTED: return "contributed";
    default:                              return "unknown";
    }
}

const char *sutr_sucpm_cluster_name(sucpm_cluster_t cluster)
{
    switch (cluster) {
    case SUCPM_CLUSTER_B_PLUS: return "b+";
    case SUCPM_CLUSTER_W_PLUS: return "w+";
    case SUCPM_CLUSTER_C_PLUS: return "c+";
    case SUCPM_CLUSTER_S_PLUS: return "s+";
    default:                  return "none";
    }
}

const char *sutr_sucpm_gc_name(sucpm_gc_t gc)
{
    switch (gc) {
    case SUCPM_GC_LU: return "Lu";
    case SUCPM_GC_LL: return "Ll";
    case SUCPM_GC_LT: return "Lt";
    case SUCPM_GC_LM: return "Lm";
    case SUCPM_GC_LO: return "Lo";
    case SUCPM_GC_MN: return "Mn";
    case SUCPM_GC_MC: return "Mc";
    case SUCPM_GC_ME: return "Me";
    case SUCPM_GC_ND: return "Nd";
    case SUCPM_GC_NL: return "Nl";
    case SUCPM_GC_NO: return "No";
    case SUCPM_GC_PC: return "Pc";
    case SUCPM_GC_PD: return "Pd";
    case SUCPM_GC_PS: return "Ps";
    case SUCPM_GC_PE: return "Pe";
    case SUCPM_GC_PI: return "Pi";
    case SUCPM_GC_PF: return "Pf";
    case SUCPM_GC_PO: return "Po";
    case SUCPM_GC_SM: return "Sm";
    case SUCPM_GC_SC: return "Sc";
    case SUCPM_GC_SK: return "Sk";
    case SUCPM_GC_SO: return "So";
    case SUCPM_GC_ZS: return "Zs";
    case SUCPM_GC_ZL: return "Zl";
    case SUCPM_GC_ZP: return "Zp";
    case SUCPM_GC_CC: return "Cc";
    case SUCPM_GC_CF: return "Cf";
    case SUCPM_GC_CS: return "Cs";
    case SUCPM_GC_CO: return "Co";
    case SUCPM_GC_CN: return "Cn";
    default:          return "Cn";
    }
}

const char *sutr_sucpm_math_class_name(sucpm_math_class_t math_class)
{
    switch (math_class) {
    case SUCPM_MC_N: return "N";
    case SUCPM_MC_B: return "B";
    case SUCPM_MC_U: return "U";
    case SUCPM_MC_R: return "R";
    case SUCPM_MC_O: return "O";
    case SUCPM_MC_C: return "C";
    case SUCPM_MC_F: return "F";
    case SUCPM_MC_P: return "P";
    case SUCPM_MC_I: return "I";
    case SUCPM_MC_S: return "S";
    default:         return "N";
    }
}

const char *sutr_sucpm_numeric_type_name(sucpm_numeric_type_t type)
{
    switch (type) {
    case SUCPM_NT_NONE:    return "none";
    case SUCPM_NT_DECIMAL: return "decimal";
    case SUCPM_NT_DIGIT:   return "digit";
    case SUCPM_NT_NUMERIC: return "numeric";
    default:               return "unknown";
    }
}
