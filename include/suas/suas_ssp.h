#ifndef SUAS_SUAS_SSP_H
#define SUAS_SUAS_SSP_H

/* SUAS-005 — System Script Property (SSP)
 *
 * The core kernel architecture governing script identification, script-
 * extension boundaries, and font-fallback routing across the 31-bit
 * SuperUnicode (SUCS) address space.
 *
 * Where Unicode's UAX #24 maps characters to writing systems through flat
 * Script / Script_Extensions tables, SSP provides a unified O(1)
 * script-tagging mechanism: a compact 16-bit suas_script_t id is resolved
 * for every codepoint by a single range-dispatched engine spanning the
 * Unicode Bridge (SUCD tables), the System Control Plane (explicit
 * script-context directives), the Native plane, and the RNUR/RCCR neography
 * carve-out at 0x160000-0x16FFFF. The 16-bit id is small enough to pack
 * directly into a character token header, letting a freestanding font engine
 * (suf) jump straight to the correct registered typeface block with no
 * multi-font fallback search.
 *
 * Freestanding C99, zero allocation: all tables are immutable static data and
 * every lookup is a bounded binary search or an O(1) range test.
 *
 * See docs/suas/SUAS-005-ssp.md for the normative specification.
 */

#include <stdint.h>
#include <stdbool.h>

#include "suas/suas_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ── Specification identifiers ────────────────────────────────────── */
#define SUAS_SSP_SPEC_ID       "SUAS-005"
#define SUAS_SSP_SPEC_TITLE    "System Script Property"
#define SUAS_SSP_VERSION_MAJOR 1
#define SUAS_SSP_VERSION_MINOR 0
#define SUAS_SSP_VERSION_PATCH 0

/* ── 64-bit ExtSUCS codepoint type (guarded to coexist) ───────────── */
#ifndef EXTSUCS_CHAR_T_DEFINED
#define EXTSUCS_CHAR_T_DEFINED
typedef uint64_t sucs_ex_char_t;
#endif

/* ── Zone boundaries (SUTR-0 / SUTR-8; duplicated for freestanding) ─ */
#define SUAS_SSP_BRIDGE_MAX   ((sucs_ex_char_t)0x0010FFFFULL)
#define SUAS_SSP_SCP_MIN      ((sucs_ex_char_t)0x00110000ULL)
#define SUAS_SSP_SCP_MAX      ((sucs_ex_char_t)0x0011FFFFULL)
#define SUAS_SSP_NATIVE_MIN   ((sucs_ex_char_t)0x00120000ULL)
#define SUAS_SSP_NATIVE_LAST  ((sucs_ex_char_t)0x7FFFFFEFULL) /* last allocatable */
#define SUAS_SSP_TRAP_MIN     ((sucs_ex_char_t)0x7FFFFFF0ULL)
#define SUAS_SSP_TRAP_MAX     ((sucs_ex_char_t)0x7FFFFFFEULL)
#define SUAS_SSP_SENTINEL     ((sucs_ex_char_t)0x7FFFFFFFULL)

/* RNUR / RCCR neography carve-out inside the Native plane (SUTR-8 §1.4). */
#define SUAS_SSP_RNUR_MIN     ((sucs_ex_char_t)0x00160000ULL)
#define SUAS_SSP_RNUR_MAX     ((sucs_ex_char_t)0x0016FFFFULL)

/* ── Compact 16-bit script identifiers ────────────────────────────── */
/* The namespace is a full partition of the codespace: every codepoint has
 * exactly one suas_script_t value. The two implicit values (UNKNOWN, COMMON)
 * are followed by explicit script values. COMMON is the merged implicit value
 * for Unicode's Common and Inherited scripts: combining marks and shared
 * symbols both resolve to COMMON, and inheritance from a base character is
 * applied by suas_ssp_resolve() (§5.2). Values 0x0000-0x03FF are the packed
 * "Bridge" range; 0x1000 and above are the Native/registry extension range,
 * reserved for scripts registered by a mounted plugin (none yet). */
typedef enum {
    SUAS_SCRIPT_UNKNOWN   = 0x0000, /* unassigned, private-use, noncharacter */
    SUAS_SCRIPT_COMMON    = 0x0001, /* Common / Inherited (shared + marks)   */

    /* Bridge scripts (ISO 15924-aligned explicit values, curated subset) */
    SUAS_SCRIPT_LATIN     = 0x0002,
    SUAS_SCRIPT_ARABIC    = 0x0003,
    SUAS_SCRIPT_CYRILLIC  = 0x0004,
    SUAS_SCRIPT_GREEK     = 0x0005,
    SUAS_SCRIPT_HAN       = 0x0006,
    SUAS_SCRIPT_HEBREW    = 0x0007,
    SUAS_SCRIPT_DEVANAGARI = 0x0008,
    SUAS_SCRIPT_HIRAGANA  = 0x0009,
    SUAS_SCRIPT_KATAKANA  = 0x000A,
    SUAS_SCRIPT_HANGUL    = 0x000B,
    SUAS_SCRIPT_THAI      = 0x000C,
    SUAS_SCRIPT_LAO       = 0x000D,
    SUAS_SCRIPT_TIBETAN   = 0x000E,
    SUAS_SCRIPT_ARMENIAN  = 0x000F,
    SUAS_SCRIPT_GEORGIAN  = 0x0010,
    SUAS_SCRIPT_BRAILLE   = 0x0011,
    SUAS_SCRIPT_SYRIAC    = 0x0012,
    SUAS_SCRIPT_COPTIC    = 0x0013,

    /* Native / registry scripts are assigned at 0x1000 and above. None are
     * registered yet, so the range is reserved: a mounted plugin (the RNUR
     * registry) defines its values. See §3.1 and §5.4. */

    SUAS_SCRIPT_CUSTOM_MAX = 0xFFFF
} suas_script_t;

/* Inclusive Bridge-script bound; values above this are the Native range. */
#define SUAS_SCRIPT_BRIDGE_MAX 0x03FF
#define SUAS_SCRIPT_NATIVE_MIN 0x1000

/* ── Script_Extensions (scx) set ───────────────────────────────────── */
/* An scx set is a small, unordered, deduplicated set of Script values.
 * The empty set is never legal: a codepoint with no extension maps to the
 * singleton { Script(cp) } (§4.1). */
#define SUAS_SSP_SCX_MAX 4

typedef struct {
    uint8_t       n;
    suas_script_t scripts[SUAS_SSP_SCX_MAX];
} suas_ssp_scx_t;

/* ── SCP script-context override directives ──────────────────────── */
/* The System Control Plane can carry an explicit script context. The
 * directive codepoint encodes the selected 15-bit script id in its low bits:
 *   codepoint = SUAS_SSP_SCP_SCRIPT_BASE + (script_id & 0x7FFF)
 * The sub-range 0x00112000-0x00119FFF is reserved for script directives;
 * BANcodes (0x0011A000+) and the SDF/SBR directives remain disjoint. */
#define SUAS_SSP_SCP_SCRIPT_BASE ((sucs_ex_char_t)0x00112000ULL)
#define SUAS_SSP_SCP_SCRIPT_LAST ((sucs_ex_char_t)0x00119FFFULL)
#define SUAS_SSP_SCP_SCRIPT(id) \
    ((sucs_ex_char_t)(SUAS_SSP_SCP_SCRIPT_BASE + ((sucs_ex_char_t)(id) & 0x7FFFULL)))

/* ── Script-tagged character token (font-engine fast path, §5.5) ──── */
/*   bits  0-15 : 16-bit script id
 *   bits 16-46 : 31-bit SUCS codepoint
 *   bits 47-63 : reserved (must be zero) */
#define SUAS_SSP_TOKEN_SCRIPT_MASK ((uint64_t)0x000000000000FFFFULL)
#define SUAS_SSP_TOKEN_CP_MASK     ((uint64_t)0x00007FFFFFFF0000ULL)

/* ── Setup ───────────────────────────────────────────────────────── */
const char* suas_ssp_version_string(void);

/* ── The Script property (§3) ─────────────────────────────────────── */
/* Resolve the 16-bit script property for any SUCS / ExtSUCS codepoint in
 * O(log R) time (bounded range search) or O(1) for range-dispatched zones.
 * Never allocates. */
suas_script_t suas_ssp_get_script(sucs_ex_char_t cp);

/* True when the codepoint belongs to (or extends into) the target script. */
bool suas_ssp_matches_script(sucs_ex_char_t cp, suas_script_t target_script);

/* True for the two implicit values (UNKNOWN, COMMON). */
bool suas_ssp_is_implicit(suas_script_t script);

/* ── The Script_Extensions property (§4) ─────────────────────────── */
/* Resolve the scx set for a codepoint. When the codepoint has no curated
 * extension the result is the singleton { Script(cp) }. */
suas_ssp_scx_t suas_ssp_get_script_extensions(sucs_ex_char_t cp);

/* Set membership test for an scx set. */
bool suas_ssp_scx_contains(const suas_ssp_scx_t* set, suas_script_t script);

/* ── Context resolution (§5.2) ────────────────────────────────────── */
/* Resolve a codepoint's effective script against an explicit context script
 * (typically the script of the preceding base character). Implicit values
 * (COMMON / UNKNOWN) inherit the context when the context is explicit. */
suas_script_t suas_ssp_resolve(sucs_ex_char_t cp, suas_script_t context_script);

/* ── SCP directives (§5.3) ────────────────────────────────────────── */
/* True when cp is an SCP script-context directive; writes the selected
 * script to *out when out is non-NULL. */
bool suas_ssp_is_script_directive(sucs_ex_char_t cp, suas_script_t* out);

/* ── Names and aliases (§3.4, §3.5) ───────────────────────────────── */
/* Long (identifier) and short (ISO 15924-style) names. Never NULL. */
const char* suas_ssp_script_long_name(suas_script_t script);
const char* suas_ssp_script_short_name(suas_script_t script);

/* Resolve a loose script tag (case-insensitive; SPACE, HYPHEN, UNDERSCORE
 * ignored) against the long and short names. Returns true on a match. */
bool suas_ssp_script_from_tag(const char* tag, suas_script_t* out);

/* ── Font-engine token packing (§5.5) ─────────────────────────────── */
uint64_t      suas_ssp_tag_codepoint(sucs_ex_char_t cp, suas_script_t script);
suas_script_t suas_ssp_token_script(uint64_t token);
sucs_ex_char_t suas_ssp_token_codepoint(uint64_t token);

#ifdef __cplusplus
}
#endif

#endif /* SUAS_SUAS_SSP_H */
