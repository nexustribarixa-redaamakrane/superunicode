#ifndef SUTR_SUTR_SUCEM_H
#define SUTR_SUTR_SUCEM_H

/* SUTR-7 — SuperUnicode Character Encoding Model (SUCEM)
 *
 * A modular technical-report specification within the OpenWindows microkernel
 * ecosystem. SUCEM is the SuperUnicode analog of ISO/IEC 10646's four-level
 * character encoding model — the Abstract Character Repertoire (ACR), the
 * Coded Character Set (CCS), the Character Encoding Form (CEF), and the
 * Character Encoding Scheme (CES) — restated over the 31-bit Base SUCS space
 * and the unbounded 64-bit ExtSUCS space.
 *
 * The model is architectural, so this module makes the architecture
 * *checkable* rather than merely describing it. Every load-bearing claim of
 * SUTR-7 becomes an O(1), allocation-free predicate:
 *
 *   1. THE FOUR LEVELS. A form declares which level it belongs to, and the
 *      byte-order test of §7 is the discriminator: if the processor byte
 *      order changed, would the format change? No means Level 3 (CEF); yes
 *      means Level 4 (CES).
 *   2. THE THREE §7 CONSEQUENCES. There is no SUTF-32 (a fixed 32-bit form is
 *      a CES framing, not an SUTF form), there is no SUTF-16-LE (the byte
 *      order is chosen by SUST-16, not by the transformation), and there is no
 *      SUST-8 (SUTF-8 is byte-oriented, so its CES is the identity).
 *      `sutr_sucem_name_status()` reports which of these a candidate name
 *      violates.
 *   3. THE REPERTOIRE SOURCES. The ACR is open and has three sources —
 *      Unicode, RNUR, and SCC — layered over the Native allocation space.
 *   4. THE SCP FOUR-LEVEL ANCHORING. A System Control Plane codepoint is
 *      simultaneously an abstract directive, an address, a transformed
 *      integer, and a framed packet. SUCEM separates those four roles, and
 *      decodes the codepoint into its directive class byte and its argument
 *      byte.
 *   5. THE TRAP DISPATCH GEOMETRY. `sutr_sucem_bancode_to_trap()` and
 *      `sutr_sucem_trap_to_bancode_range()` are one subtract, one divide by a
 *      compile-time constant, and one add — O(1), and totally invertible for
 *      every trap slot that exists.
 *
 * Freestanding C99, zero allocation: every table is static, every descriptor is
 * a fixed-size struct the caller owns, and no part of the engine reaches for
 * malloc, stdio or libc string routines.
 *
 * See docs/sutr/SUTR-7-sucem.md for the normative specification.
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Specification identifiers ────────────────────────────────────── */

#define SUCEM_SPEC_ID            "SUTR-7"
#define SUCEM_SPEC_TITLE         "SuperUnicode Character Encoding Model"
#define SUCEM_VERSION_MAJOR      0
#define SUCEM_VERSION_MINOR      1
#define SUCEM_VERSION_PATCH      0
#define SUCEM_MODULE             "sutr_sucem"

/* ── The four levels of the model (§1) ─────────────────────────────── */

typedef enum {
    SUCEM_LEVEL_NONE = 0, /* outside the four levels (CM / TES)         */
    SUCEM_LEVEL_ACR  = 1, /* Level 1 — Abstract Character Repertoire     */
    SUCEM_LEVEL_CCS  = 2, /* Level 2 — Coded Character Set (SUCS)        */
    SUCEM_LEVEL_CEF  = 3, /* Level 3 — Character Encoding Form (SUTF)    */
    SUCEM_LEVEL_CES  = 4  /* Level 4 — Character Encoding Scheme (SUST)  */
} sucem_level_t;

#define SUCEM_LEVEL_COUNT 4u

/* ── The form registry (SUTF = CEF, SUST = CES) ────────────────────── */

typedef enum {
    SUCEM_FORM_NONE = 0,
    SUCEM_FORM_SUTF8,   /* CEF — byte-oriented, self-delimiting      */
    SUCEM_FORM_SUTF16,  /* CEF — 16-bit word, endian-neutral         */
    SUCEM_FORM_SUTF4,   /* CEF — 4-bit nibble, fixed 8 nibbles        */
    SUCEM_FORM_SUTF2,   /* CEF — 2-bit frame, fixed 16 frames         */
    SUCEM_FORM_VSUTF,   /* CEF — variable multi-byte, 64-bit ExtSUCS  */
    SUCEM_FORM_SUST16,  /* CES — explicit byte order, 16-bit word     */
    SUCEM_FORM_SUST32,  /* CES — fixed-width 32-bit framing           */
    SUCEM_FORM_SUST64,  /* CES — fixed-width 64-bit framing           */
    SUCEM_FORM_SUST128, /* CES — fixed-width 128-bit framing          */
    SUCEM_FORM_SUST256, /* CES — fixed-width 256-bit framing          */
    SUCEM_FORM_SUST512, /* CES — fixed-width 512-bit framing          */
    SUCEM_FORM_SUSTN,   /* CES — N-bit framing, width is a parameter  */
    SUCEM_FORM_ESUST    /* CES — e-SUST, the Transfer Encoding Syntax  */
} sucem_form_t;

#define SUCEM_FORM_COUNT 13u

/* ── The forbidden names of §7, stated as rules rather than preferences ── */

#define SUCEM_FORBIDDEN_CEF_FIXED32  "SUTF-32"    /* §7.1 */
#define SUCEM_FORBIDDEN_CEF_LITTLE   "SUTF-16-LE" /* §7.2 */
#define SUCEM_FORBIDDEN_CES_BYTE     "SUST-8"     /* §7.3 */

/* ── The ACR repertoire sources (§3) ───────────────────────────────── */

typedef enum {
    SUCEM_REPERTOIRE_NONE = 0,
    SUCEM_REPERTOIRE_UNICODE, /* 0x000000-0x10FFFF — the Unicode repertoire  */
    SUCEM_REPERTOIRE_RNUR,    /* 0x160000-0x16FFFF — neography registry       */
    SUCEM_REPERTOIRE_SCC,     /* 0x110000-0x11FFFF — System Control Plane     */
    SUCEM_REPERTOIRE_NATIVE,  /* the Native allocation space                  */
    SUCEM_REPERTOIRE_TRAP,    /* the Kernel Security Trap slots               */
    SUCEM_REPERTOIRE_SENTINEL /* 0x7FFFFFFF — the in-band terminator          */
} sucem_repertoire_t;

/* ── The SCP severity clusters (SUTR-0 §5) ─────────────────────────── */

typedef enum {
    SUCEM_SCP_CLUSTER_NONE = 0,
    SUCEM_SCP_CLUSTER_B_PLUS, /* B+ BANcode   — fatal, unrecoverable  */
    SUCEM_SCP_CLUSTER_W_PLUS, /* W+ WARNcode  — non-fatal telemetry   */
    SUCEM_SCP_CLUSTER_C_PLUS, /* C+ COMcode   — IPC / orchestration   */
    SUCEM_SCP_CLUSTER_S_PLUS  /* S+ SOFTcode  — soft recovery          */
} sucem_scp_cluster_t;

/* ── The four roles a System Control Plane codepoint plays (§8) ────── */

typedef enum {
    SUCEM_SCP_ROLE_NONE = 0,
    SUCEM_SCP_ROLE_DIRECTIVE,  /* Level 1 — a non-printable abstract directive */
    SUCEM_SCP_ROLE_ADDRESS,    /* Level 2 — a number in the codespace         */
    SUCEM_SCP_ROLE_TRANSFORMED,/* Level 3 — an SUTF-transformed integer       */
    SUCEM_SCP_ROLE_FRAMED      /* Level 4 — a CES-framed packet               */
} sucem_scp_role_t;

/* ── The decoded SCP anchor (§8) ───────────────────────────────────── */

typedef struct {
    sucem_scp_role_t    role;          /* what the codepoint is at the level */
    sucem_scp_cluster_t cluster;       /* severity cluster, or NONE          */
    uint8_t             class_byte;    /* X — bits 8-15, directive class     */
    uint8_t             argument_byte; /* Y — bits 0-7, argument byte        */
    uint32_t            index;         /* offset within the cluster          */
} sucem_scp_anchor_t;

/* ── Name validation outcomes (§7) ─────────────────────────────────── */

typedef enum {
    SUCEM_NAME_OK = 0,
    SUCEM_NAME_UNKNOWN,       /* not a SuperUnicode form name at all       */
    SUCEM_NAME_FORBIDDEN_CEF, /* §7.1 SUTF-32 or §7.2 SUTF-16-LE           */
    SUCEM_NAME_FORBIDDEN_CES, /* §7.3 SUST-8                                */
    SUCEM_NAME_WRONG_LEVEL    /* a valid name, but at the other level      */
} sucem_name_status_t;

/* ── Base SUCS zone boundaries (duplicated for freestanding SUCEM) ─── */

#ifndef SUCEM_LIMIT_UNICODE_MAX
#define SUCEM_LIMIT_UNICODE_MAX  0x0010FFFFUL
#define SUCEM_LIMIT_SCP_MIN      0x00110000UL
#define SUCEM_LIMIT_SCP_MAX      0x0011FFFFUL
#define SUCEM_LIMIT_NATIVE_MIN   0x00120000UL
#define SUCEM_LIMIT_NATIVE_MAX   0x7FFFFFEFUL
#define SUCEM_LIMIT_RNUR_MIN     0x00160000UL
#define SUCEM_LIMIT_RNUR_MAX     0x0016FFFFUL
#define SUCEM_LIMIT_TRAP_MIN     0x7FFFFFF0UL
#define SUCEM_LIMIT_TRAP_MAX     0x7FFFFFFEUL
#define SUCEM_LIMIT_SENTINEL     0x7FFFFFFFUL
#endif

/* ── The BANcode Registry (SUTR-0 §5) ──────────────────────────────── */

#define SUCEM_BANCODE_REGISTRY_MIN 0x0011A000UL
#define SUCEM_BANCODE_REGISTRY_MAX 0x0011AEFFUL

#define SUCEM_BANCODE_BPLUS_MIN    0x0011A000UL /* 2,048 codepoints */
#define SUCEM_BANCODE_BPLUS_MAX    0x0011A7FFUL
#define SUCEM_BANCODE_WPLUS_MIN    0x0011A800UL /* 1,024 codepoints */
#define SUCEM_BANCODE_WPLUS_MAX    0x0011ABFFUL
#define SUCEM_BANCODE_CPLUS_MIN    0x0011AC00UL /*   512 codepoints */
#define SUCEM_BANCODE_CPLUS_MAX    0x0011ADFFUL
#define SUCEM_BANCODE_SPLUS_MIN    0x0011AE00UL /*   256 codepoints */
#define SUCEM_BANCODE_SPLUS_MAX    0x0011AEFFUL

/* ── Trap dispatch geometry (SUTR-0 §5.1) ──────────────────────────── */

#define SUCEM_TRAP_SLOT_COUNT     15u
#define SUCEM_BANCODES_PER_TRAP   128u

/* ── The identity Character Map covers the Bridge (§9) ─────────────── */

#define SUCEM_IDENTITY_CM_MIN     0x000000UL
#define SUCEM_IDENTITY_CM_MAX     0x0010FFFFUL

/* ── e-SUST frames a BANcode as 6 bytes (§8, Level 4) ──────────────── */

#define SUCEM_ESUST_BANCODE_FRAME_BYTES 6u

/* ── Setup ─────────────────────────────────────────────────────────── */

/**
 * Returns the "MAJOR.MINOR.PATCH" version string of this module.
 * Freestanding: the returned pointer is to a static string, never NULL.
 */
const char* sutr_sucem_version_string(void);

/* ── The level model (§1, §5, §6, §7) ─────────────────────────────── */

/**
 * The level a form belongs to. A CEF is an SUTF; a CES is a SUST. The
 * byte-order test is the discriminator.
 */
sucem_level_t sutr_sucem_level_of(sucem_form_t form);

/**
 * The canonical name of a level ("ACR", "CCS", "CEF", "CES", or "none").
 */
const char *sutr_sucem_level_name(sucem_level_t level);

/**
 * The canonical name of a form ("SUTF-8", "SUST-N", "e-SUST", ...), or
 * "unknown" for a value that names no form.
 */
const char *sutr_sucem_form_name(sucem_form_t form);

/**
 * The form a name denotes, or SUCEM_FORM_NONE. The comparison is exact:
 * the canonical spellings are the only accepted ones.
 */
sucem_form_t sutr_sucem_form_by_name(const char *name);

/**
 * Reports whether a candidate form name is admissible, and if not, which of
 * the three §7 rules it violates.
 *
 * When `level` is SUCEM_LEVEL_CEF or SUCEM_LEVEL_CES, a name that exists at
 * the *other* level reports SUCEM_NAME_WRONG_LEVEL. When `level` is
 * SUCEM_LEVEL_NONE, the level of the name is not constrained and any valid
 * form name reports SUCEM_NAME_OK.
 */
sucem_name_status_t sutr_sucem_name_status(const char *name, sucem_level_t level);

/**
 * True when the name denotes an existing Level 3 (CEF) form. "SUTF-32" and
 * "SUTF-16-LE" are therefore false.
 */
bool sutr_sucem_is_cef_name(const char *name);

/**
 * True when the name denotes an existing Level 4 (CES) form. "SUST-8" is
 * therefore false.
 */
bool sutr_sucem_is_ces_name(const char *name);

/**
 * The byte-order test of §7, as a predicate over a form: a CEF would not
 * change if the processor byte order changed; a CES would.
 */
bool sutr_sucem_form_is_byte_order_sensitive(sucem_form_t form);

/**
 * True for a Transfer Encoding Syntax. e-SUST is the only one defined.
 */
bool sutr_sucem_is_tes(sucem_form_t form);

/* ── Code units and framed widths (§5, §6) ─────────────────────────── */

/**
 * The width in bits of one code unit of a CEF. Zero for a CES, whose
 * transport operates on framed bytes rather than on code units.
 */
uint16_t sutr_sucem_form_unit_bits(sucem_form_t form);

/**
 * The number of code units a codepoint occupies in a CEF, or 0 when the form
 * is not a CEF or the count is not a function of the codepoint alone
 * (vSUTF) or the codepoint is not a valid Base SUCS codepoint.
 */
uint16_t sutr_sucem_form_unit_count(uint32_t codepoint, sucem_form_t form);

/**
 * The fixed word width in bits of a CES, or 0 for a variable-width transport
 * (SUST-N) and for the e-SUST syntax.
 */
uint16_t sutr_sucem_ces_word_bits(sucem_form_t form);

/**
 * The size in bytes of an e-SUST frame carrying a BANcode (§8, Level 4).
 */
uint8_t sutr_sucem_esust_bancode_frame_bytes(void);

/* ── The repertoire sources (§3) ───────────────────────────────────── */

sucem_repertoire_t sutr_sucem_repertoire_of(uint32_t codepoint);

/**
 * The canonical name of a repertoire source ("unicode", "rnur", "scc",
 * "native", "trap", "sentinel", or "none").
 */
const char *sutr_sucem_repertoire_name(sucem_repertoire_t repertoire);

/**
 * True when the codepoint falls in the Unicode-sourced repertoire.
 */
bool sutr_sucem_is_unicode_repertoire(uint32_t codepoint);

/**
 * True when the codepoint falls in the RNUR neography carve-out.
 */
bool sutr_sucem_is_rnur_repertoire(uint32_t codepoint);

/**
 * True when the codepoint falls in the System Control Plane.
 */
bool sutr_sucem_is_scc_repertoire(uint32_t codepoint);

/**
 * True when the codepoint is a Kernel Security Trap slot.
 */
bool sutr_sucem_is_trap(uint32_t codepoint);

/**
 * True when the codepoint is the Sentinel.
 */
bool sutr_sucem_is_sentinel(uint32_t codepoint);

/**
 * True when the codepoint is covered by the identity Character Map, i.e.
 * lies in the Unicode bridge range.
 */
bool sutr_sucem_identity_cm_covers(uint32_t codepoint);

/* ── The SCP four-level anchor (§8) ───────────────────────────────── */

/**
 * The directive class byte X of a System Control Plane codepoint: bits 8-15.
 */
uint8_t sutr_sucem_scp_class_byte(uint32_t codepoint);

/**
 * The argument byte Y of a System Control Plane codepoint: bits 0-7.
 */
uint8_t sutr_sucem_scp_argument_byte(uint32_t codepoint);

/**
 * The severity cluster a System Control Plane codepoint belongs to, or
 * SUCEM_SCP_CLUSTER_NONE when it is not in the BANcode Registry.
 */
sucem_scp_cluster_t sutr_sucem_scp_cluster_of(uint32_t codepoint);

/**
 * The canonical name of a severity cluster ("b+", "w+", "c+", "s+", or
 * "none").
 */
const char *sutr_sucem_scp_cluster_name(sucem_scp_cluster_t cluster);

bool sutr_sucem_is_bancode(uint32_t codepoint);
bool sutr_sucem_is_warncode(uint32_t codepoint);
bool sutr_sucem_is_comcode(uint32_t codepoint);
bool sutr_sucem_is_softcode(uint32_t codepoint);

/**
 * Decodes a System Control Plane codepoint into its four-level anchor: the
 * role it plays at `level`, its severity cluster, its directive class byte,
 * its argument byte, and its index within the cluster.
 *
 * Returns false for a codepoint outside the SCP, for a level outside the
 * four levels, or for a NULL output.
 */
bool sutr_sucem_scp_anchor(uint32_t codepoint, uint8_t level,
                           sucem_scp_anchor_t *out_anchor);

/* ── Trap dispatch geometry (§8.1) ─────────────────────────────────── */

/**
 * Maps a B+ BANcode to its Kernel Security Trap address, in O(1): one
 * subtract, one divide by a compile-time constant, one add.
 *
 * Returns false for a NULL output, for a codepoint outside the B+ cluster,
 * and for the topmost 128 B+ codepoints, which the 15-slot registry does not
 * cover.
 */
bool sutr_sucem_bancode_to_trap(uint32_t bancode, uint32_t *out_trap);

/**
 * Inverts the trap mapping: the 128-codepoint B+ range a trap governs.
 *
 * Returns false for a NULL output, for a value outside the trap range, and
 * for the Sentinel.
 */
bool sutr_sucem_trap_to_bancode_range(uint32_t trap, uint32_t *out_low,
                                      uint32_t *out_high);

/* ── Taxonomy names ───────────────────────────────────────────────── */

const char *sutr_sucem_name_status_name(sucem_name_status_t status);

#ifdef __cplusplus
}
#endif

#endif /* SUTR_SUTR_SUCEM_H */
