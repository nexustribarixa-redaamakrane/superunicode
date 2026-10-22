/* SUTR-7 — SuperUnicode Character Encoding Model (SUCEM)
 *
 * Reference implementation. See include/sutr/sutr_sucem.h for the API and
 * docs/sutr/SUTR-7-sucem.md for the normative specification.
 *
 * Freestanding C99: static tables only, no libc, no allocation.
 */

#include "sutr/sutr_sucem.h"

/* ── The form registry ─────────────────────────────────────────────── */

typedef struct {
    sucem_form_t  form;
    const char   *name;
    sucem_level_t level;
    uint16_t      unit_bits; /* CEF code-unit width; CES word width        */
    uint8_t       ces;       /* 1 when the form is a CES (framed bytes)     */
} sucem_form_record_t;

static const sucem_form_record_t g_sucem_forms[SUCEM_FORM_COUNT] = {
    { SUCEM_FORM_SUTF8,   "SUTF-8",   SUCEM_LEVEL_CEF,  8u, 0u },
    { SUCEM_FORM_SUTF16,  "SUTF-16",  SUCEM_LEVEL_CEF, 16u, 0u },
    { SUCEM_FORM_SUTF4,   "SUTF-4",   SUCEM_LEVEL_CEF,  4u, 0u },
    { SUCEM_FORM_SUTF2,   "SUTF-2",   SUCEM_LEVEL_CEF,  2u, 0u },
    { SUCEM_FORM_VSUTF,   "vSUTF",    SUCEM_LEVEL_CEF,  8u, 0u },
    { SUCEM_FORM_SUST16,  "SUST-16",  SUCEM_LEVEL_CES, 16u, 1u },
    { SUCEM_FORM_SUST32,  "SUST-32",  SUCEM_LEVEL_CES, 32u, 1u },
    { SUCEM_FORM_SUST64,  "SUST-64",  SUCEM_LEVEL_CES, 64u, 1u },
    { SUCEM_FORM_SUST128, "SUST-128", SUCEM_LEVEL_CES, 128u, 1u },
    { SUCEM_FORM_SUST256, "SUST-256", SUCEM_LEVEL_CES, 256u, 1u },
    { SUCEM_FORM_SUST512, "SUST-512", SUCEM_LEVEL_CES, 512u, 1u },
    { SUCEM_FORM_SUSTN,   "SUST-N",   SUCEM_LEVEL_CES, 0u,  1u },
    { SUCEM_FORM_ESUST,   "e-SUST",   SUCEM_LEVEL_CES, 0u,  1u }
};

/* ── Local helpers ─────────────────────────────────────────────────── */

/* Exact string compare over a bounded length; no libc. */
static bool sucem_streq(const char *a, const char *b)
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

/* A codepoint that may be encoded as Base SUCS data: not a trap, not the
 * Sentinel, and inside the 31-bit codespace. */
static bool sucem_is_valid_codepoint(uint32_t cp)
{
    if (cp > SUCEM_LIMIT_NATIVE_MAX) {
        return false;
    }
    if (cp >= SUCEM_LIMIT_TRAP_MIN) {
        return false;
    }
    return true;
}

/* The severity cluster a directive class byte belongs to. */
static sucem_scp_cluster_t sucem_cluster_of_class_byte(uint8_t class_byte)
{
    if (class_byte >= 0xA0u && class_byte <= 0xA7u) {
        return SUCEM_SCP_CLUSTER_B_PLUS;
    }
    if (class_byte >= 0xA8u && class_byte <= 0xABu) {
        return SUCEM_SCP_CLUSTER_W_PLUS;
    }
    if (class_byte >= 0xACu && class_byte <= 0xADu) {
        return SUCEM_SCP_CLUSTER_C_PLUS;
    }
    if (class_byte == 0xAEu) {
        return SUCEM_SCP_CLUSTER_S_PLUS;
    }
    return SUCEM_SCP_CLUSTER_NONE;
}

/* The first codepoint of a severity cluster. */
static uint32_t sucem_cluster_base(sucem_scp_cluster_t cluster)
{
    switch (cluster) {
    case SUCEM_SCP_CLUSTER_B_PLUS: return SUCEM_BANCODE_BPLUS_MIN;
    case SUCEM_SCP_CLUSTER_W_PLUS: return SUCEM_BANCODE_WPLUS_MIN;
    case SUCEM_SCP_CLUSTER_C_PLUS: return SUCEM_BANCODE_CPLUS_MIN;
    case SUCEM_SCP_CLUSTER_S_PLUS: return SUCEM_BANCODE_SPLUS_MIN;
    default:                       return 0u;
    }
}

/* ── Setup ─────────────────────────────────────────────────────────── */

const char* sutr_sucem_version_string(void)
{
    return "0.1.0";
}

/* ── The level model ───────────────────────────────────────────────── */

sucem_level_t sutr_sucem_level_of(sucem_form_t form)
{
    size_t i;

    for (i = 0u; i < SUCEM_FORM_COUNT; i++) {
        if (g_sucem_forms[i].form == form) {
            return g_sucem_forms[i].level;
        }
    }
    return SUCEM_LEVEL_NONE;
}

const char *sutr_sucem_level_name(sucem_level_t level)
{
    switch (level) {
    case SUCEM_LEVEL_ACR: return "ACR";
    case SUCEM_LEVEL_CCS: return "CCS";
    case SUCEM_LEVEL_CEF: return "CEF";
    case SUCEM_LEVEL_CES: return "CES";
    default:              return "none";
    }
}

const char *sutr_sucem_form_name(sucem_form_t form)
{
    size_t i;

    for (i = 0u; i < SUCEM_FORM_COUNT; i++) {
        if (g_sucem_forms[i].form == form) {
            return g_sucem_forms[i].name;
        }
    }
    return "unknown";
}

sucem_form_t sutr_sucem_form_by_name(const char *name)
{
    size_t i;

    if (name == NULL) {
        return SUCEM_FORM_NONE;
    }
    for (i = 0u; i < SUCEM_FORM_COUNT; i++) {
        if (sucem_streq(g_sucem_forms[i].name, name)) {
            return g_sucem_forms[i].form;
        }
    }
    return SUCEM_FORM_NONE;
}

sucem_name_status_t sutr_sucem_name_status(const char *name, sucem_level_t level)
{
    sucem_form_t form;

    if (name == NULL || name[0] == '\0') {
        return SUCEM_NAME_UNKNOWN;
    }

    /* §7.1 — a fixed 32-bit form is not an SUTF form. */
    if (sucem_streq(name, SUCEM_FORBIDDEN_CEF_FIXED32)) {
        return SUCEM_NAME_FORBIDDEN_CEF;
    }
    /* §7.2 — little-endian is not an SUTF form. */
    if (sucem_streq(name, SUCEM_FORBIDDEN_CEF_LITTLE)) {
        return SUCEM_NAME_FORBIDDEN_CEF;
    }
    /* §7.3 — SUTF-8 is byte-oriented, so its CES is the identity. */
    if (sucem_streq(name, SUCEM_FORBIDDEN_CES_BYTE)) {
        return SUCEM_NAME_FORBIDDEN_CES;
    }

    form = sutr_sucem_form_by_name(name);
    if (form == SUCEM_FORM_NONE) {
        return SUCEM_NAME_UNKNOWN;
    }
    if (level == SUCEM_LEVEL_NONE ||
        sutr_sucem_level_of(form) == level) {
        return SUCEM_NAME_OK;
    }
    return SUCEM_NAME_WRONG_LEVEL;
}

bool sutr_sucem_is_cef_name(const char *name)
{
    return sutr_sucem_name_status(name, SUCEM_LEVEL_CEF) == SUCEM_NAME_OK;
}

bool sutr_sucem_is_ces_name(const char *name)
{
    return sutr_sucem_name_status(name, SUCEM_LEVEL_CES) == SUCEM_NAME_OK;
}

bool sutr_sucem_form_is_byte_order_sensitive(sucem_form_t form)
{
    return sutr_sucem_level_of(form) == SUCEM_LEVEL_CES;
}

bool sutr_sucem_is_tes(sucem_form_t form)
{
    return form == SUCEM_FORM_ESUST;
}

/* ── Code units and framed widths ──────────────────────────────────── */

uint16_t sutr_sucem_form_unit_bits(sucem_form_t form)
{
    size_t i;

    for (i = 0u; i < SUCEM_FORM_COUNT; i++) {
        if (g_sucem_forms[i].form == form) {
            if (g_sucem_forms[i].ces != 0u) {
                return 0u; /* a CES frames bytes, not code units */
            }
            return g_sucem_forms[i].unit_bits;
        }
    }
    return 0u;
}

uint16_t sutr_sucem_form_unit_count(uint32_t codepoint, sucem_form_t form)
{
    if (!sucem_is_valid_codepoint(codepoint)) {
        return 0u;
    }

    switch (form) {
    case SUCEM_FORM_SUTF8:
        /* SUTF-8 spans 1 to 6 bytes over the 31-bit space. */
        if (codepoint <= 0x0000007FUL) { return 1u; }
        if (codepoint <= 0x000007FFUL) { return 2u; }
        if (codepoint <= 0x0000FFFFUL) { return 3u; }
        if (codepoint <= 0x0010FFFFUL) { return 4u; }
        if (codepoint <= 0x03FFFFFFUL) { return 5u; }
        return 6u;

    case SUCEM_FORM_SUTF16:
        /* One word up to 0x7FFF; two words above. */
        if (codepoint <= 0x00007FFFUL) { return 1u; }
        return 2u;

    case SUCEM_FORM_SUTF4:
        return 8u;  /* fixed 8 nibbles (4 bytes) */

    case SUCEM_FORM_SUTF2:
        return 16u; /* fixed 16 frames (4 bytes) */

    case SUCEM_FORM_VSUTF:
        /* Variable across the 64-bit ExtSUCS space; not a function of the
         * codepoint alone. */
        return 0u;

    default:
        return 0u; /* not a CEF */
    }
}

uint16_t sutr_sucem_ces_word_bits(sucem_form_t form)
{
    size_t i;

    for (i = 0u; i < SUCEM_FORM_COUNT; i++) {
        if (g_sucem_forms[i].form == form) {
            if (g_sucem_forms[i].ces == 0u) {
                return 0u; /* not a CES */
            }
            return g_sucem_forms[i].unit_bits;
        }
    }
    return 0u;
}

uint8_t sutr_sucem_esust_bancode_frame_bytes(void)
{
    return (uint8_t)SUCEM_ESUST_BANCODE_FRAME_BYTES;
}

/* ── The repertoire sources ────────────────────────────────────────── */

sucem_repertoire_t sutr_sucem_repertoire_of(uint32_t codepoint)
{
    if (codepoint > SUCEM_LIMIT_SENTINEL) {
        return SUCEM_REPERTOIRE_NONE; /* ExtSUCS, above the 31-bit space */
    }
    if (codepoint == SUCEM_LIMIT_SENTINEL) {
        return SUCEM_REPERTOIRE_SENTINEL;
    }
    if (codepoint >= SUCEM_LIMIT_TRAP_MIN) {
        return SUCEM_REPERTOIRE_TRAP;
    }
    if (codepoint >= SUCEM_LIMIT_NATIVE_MIN) {
        if (codepoint >= SUCEM_LIMIT_RNUR_MIN && codepoint <= SUCEM_LIMIT_RNUR_MAX) {
            return SUCEM_REPERTOIRE_RNUR;
        }
        return SUCEM_REPERTOIRE_NATIVE;
    }
    if (codepoint >= SUCEM_LIMIT_SCP_MIN) {
        return SUCEM_REPERTOIRE_SCC;
    }
    return SUCEM_REPERTOIRE_UNICODE;
}

const char *sutr_sucem_repertoire_name(sucem_repertoire_t repertoire)
{
    switch (repertoire) {
    case SUCEM_REPERTOIRE_UNICODE: return "unicode";
    case SUCEM_REPERTOIRE_RNUR:    return "rnur";
    case SUCEM_REPERTOIRE_SCC:     return "scc";
    case SUCEM_REPERTOIRE_NATIVE:  return "native";
    case SUCEM_REPERTOIRE_TRAP:    return "trap";
    case SUCEM_REPERTOIRE_SENTINEL: return "sentinel";
    default:                       return "none";
    }
}

bool sutr_sucem_is_unicode_repertoire(uint32_t codepoint)
{
    return sutr_sucem_repertoire_of(codepoint) == SUCEM_REPERTOIRE_UNICODE;
}

bool sutr_sucem_is_rnur_repertoire(uint32_t codepoint)
{
    return sutr_sucem_repertoire_of(codepoint) == SUCEM_REPERTOIRE_RNUR;
}

bool sutr_sucem_is_scc_repertoire(uint32_t codepoint)
{
    return sutr_sucem_repertoire_of(codepoint) == SUCEM_REPERTOIRE_SCC;
}

bool sutr_sucem_is_trap(uint32_t codepoint)
{
    return (codepoint >= SUCEM_LIMIT_TRAP_MIN) &&
           (codepoint <= SUCEM_LIMIT_TRAP_MAX);
}

bool sutr_sucem_is_sentinel(uint32_t codepoint)
{
    return codepoint == SUCEM_LIMIT_SENTINEL;
}

bool sutr_sucem_identity_cm_covers(uint32_t codepoint)
{
    return (codepoint >= SUCEM_IDENTITY_CM_MIN) &&
           (codepoint <= SUCEM_IDENTITY_CM_MAX);
}

/* ── The SCP four-level anchor ─────────────────────────────────────── */

uint8_t sutr_sucem_scp_class_byte(uint32_t codepoint)
{
    return (uint8_t)((codepoint >> 8) & 0xFFu);
}

uint8_t sutr_sucem_scp_argument_byte(uint32_t codepoint)
{
    return (uint8_t)(codepoint & 0xFFu);
}

sucem_scp_cluster_t sutr_sucem_scp_cluster_of(uint32_t codepoint)
{
    if (codepoint < SUCEM_LIMIT_SCP_MIN || codepoint > SUCEM_LIMIT_SCP_MAX) {
        return SUCEM_SCP_CLUSTER_NONE;
    }
    return sucem_cluster_of_class_byte(sutr_sucem_scp_class_byte(codepoint));
}

const char *sutr_sucem_scp_cluster_name(sucem_scp_cluster_t cluster)
{
    switch (cluster) {
    case SUCEM_SCP_CLUSTER_B_PLUS: return "b+";
    case SUCEM_SCP_CLUSTER_W_PLUS: return "w+";
    case SUCEM_SCP_CLUSTER_C_PLUS: return "c+";
    case SUCEM_SCP_CLUSTER_S_PLUS: return "s+";
    default:                      return "none";
    }
}

bool sutr_sucem_is_bancode(uint32_t codepoint)
{
    return sutr_sucem_scp_cluster_of(codepoint) == SUCEM_SCP_CLUSTER_B_PLUS;
}

bool sutr_sucem_is_warncode(uint32_t codepoint)
{
    return sutr_sucem_scp_cluster_of(codepoint) == SUCEM_SCP_CLUSTER_W_PLUS;
}

bool sutr_sucem_is_comcode(uint32_t codepoint)
{
    return sutr_sucem_scp_cluster_of(codepoint) == SUCEM_SCP_CLUSTER_C_PLUS;
}

bool sutr_sucem_is_softcode(uint32_t codepoint)
{
    return sutr_sucem_scp_cluster_of(codepoint) == SUCEM_SCP_CLUSTER_S_PLUS;
}

bool sutr_sucem_scp_anchor(uint32_t codepoint, uint8_t level,
                           sucem_scp_anchor_t *out_anchor)
{
    if (out_anchor == NULL) {
        return false;
    }
    if (codepoint < SUCEM_LIMIT_SCP_MIN || codepoint > SUCEM_LIMIT_SCP_MAX) {
        return false;
    }
    if (level < (uint8_t)SUCEM_LEVEL_ACR || level > (uint8_t)SUCEM_LEVEL_CES) {
        return false;
    }

    out_anchor->class_byte    = sutr_sucem_scp_class_byte(codepoint);
    out_anchor->argument_byte = sutr_sucem_scp_argument_byte(codepoint);
    out_anchor->cluster       = sucem_cluster_of_class_byte(out_anchor->class_byte);
    out_anchor->index         = 0u;

    if (out_anchor->cluster != SUCEM_SCP_CLUSTER_NONE) {
        out_anchor->index = codepoint - sucem_cluster_base(out_anchor->cluster);
    }

    switch (level) {
    case (uint8_t)SUCEM_LEVEL_ACR: out_anchor->role = SUCEM_SCP_ROLE_DIRECTIVE;   break;
    case (uint8_t)SUCEM_LEVEL_CCS: out_anchor->role = SUCEM_SCP_ROLE_ADDRESS;     break;
    case (uint8_t)SUCEM_LEVEL_CEF: out_anchor->role = SUCEM_SCP_ROLE_TRANSFORMED; break;
    default:                      out_anchor->role = SUCEM_SCP_ROLE_FRAMED;       break;
    }
    return true;
}

/* ── Trap dispatch geometry ────────────────────────────────────────── */

bool sutr_sucem_bancode_to_trap(uint32_t bancode, uint32_t *out_trap)
{
    uint32_t slot;

    if (out_trap == NULL) {
        return false;
    }
    /* B+ only: W+, C+ and S+ codepoints are never dispatch targets. */
    if (bancode < SUCEM_BANCODE_BPLUS_MIN || bancode > SUCEM_BANCODE_BPLUS_MAX) {
        return false;
    }

    slot = (bancode - SUCEM_BANCODE_BPLUS_MIN) / SUCEM_BANCODES_PER_TRAP;
    /* Partial coverage: 15 slots x 128 = 1,920 of 2,048 B+ codepoints. */
    if (slot >= SUCEM_TRAP_SLOT_COUNT) {
        return false;
    }

    *out_trap = SUCEM_LIMIT_TRAP_MIN + slot;
    return true;
}

bool sutr_sucem_trap_to_bancode_range(uint32_t trap, uint32_t *out_low,
                                      uint32_t *out_high)
{
    uint32_t slot;
    uint32_t low;

    if (out_low == NULL || out_high == NULL) {
        return false;
    }
    /* The Sentinel and every value outside the trap range are rejected. */
    if (trap < SUCEM_LIMIT_TRAP_MIN || trap > SUCEM_LIMIT_TRAP_MAX) {
        return false;
    }

    slot  = trap - SUCEM_LIMIT_TRAP_MIN;
    low   = SUCEM_BANCODE_BPLUS_MIN + (slot * SUCEM_BANCODES_PER_TRAP);
    *out_low  = low;
    *out_high = low + (SUCEM_BANCODES_PER_TRAP - 1u);
    return true;
}

/* ── Taxonomy names ────────────────────────────────────────────────── */

const char *sutr_sucem_name_status_name(sucem_name_status_t status)
{
    switch (status) {
    case SUCEM_NAME_OK:             return "ok";
    case SUCEM_NAME_UNKNOWN:        return "unknown";
    case SUCEM_NAME_FORBIDDEN_CEF:  return "forbidden-cef";
    case SUCEM_NAME_FORBIDDEN_CES:  return "forbidden-ces";
    case SUCEM_NAME_WRONG_LEVEL:    return "wrong-level";
    default:                        return "unknown";
    }
}
