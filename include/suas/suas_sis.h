#ifndef SUAS_SUAS_SIS_H
#define SUAS_SUAS_SIS_H

/* SUAS-007 — System Identifiers and Syntax (SIS)
 *
 * The core kernel architecture governing identifier parsing and lexical
 * syntax over the SuperUnicode (SUCS) codepoint space (a UAX #31 analog).
 *
 * Where UAX #31 defines pattern-matching rules for programming-language
 * identifiers, lexical tokens, and variable names over the 21-bit Unicode
 * planes, SIS is a zero-allocation, single-pass lexical filter that resolves
 * the class of every codepoint in O(1). It governs symbol naming for the
 * OpenWindows shell, configuration parsers, and the nVAI scripting runtime
 * without dynamic memory or regular-expression parsing.
 *
 * The engine defines two structural classes — ID_Start and ID_Continue
 * analogs — over the full SUCS space, treats the System Control Plane
 * (0x00110000-0x0011FFFF) as strictly invalid inside identifiers, admits
 * RNUR / RCCR neography codepoints as identifier characters, and exposes
 * the pattern whitespace / pattern syntax split of UAX #31 §4 so that
 * identifiers, whitespace, and operators stay disjoint.
 *
 * Freestanding C99, zero allocation: every class is a bounded range test.
 *
 * See docs/suas/SUAS-007-sis.md for the normative specification.
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "suas/suas_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ── Specification identifiers ────────────────────────────────────── */
#define SUAS_SIS_SPEC_ID       "SUAS-007"
#define SUAS_SIS_SPEC_TITLE    "System Identifiers and Syntax"
#define SUAS_SIS_VERSION_MAJOR 1
#define SUAS_SIS_VERSION_MINOR 0
#define SUAS_SIS_VERSION_PATCH 0

/* ── 64-bit ExtSUCS codepoint type (guarded to coexist) ───────────── */
#ifndef EXTSUCS_CHAR_T_DEFINED
#define EXTSUCS_CHAR_T_DEFINED
typedef uint64_t sucs_ex_char_t;
#endif

/* ── Base SUCS zone boundaries (duplicated for freestanding SIS) ──── */
#ifndef SUAS_SIS_ZONE_UNICODE_MAX
#define SUAS_SIS_ZONE_UNICODE_MAX  ((sucs_ex_char_t)0x0010FFFFULL)
#define SUAS_SIS_ZONE_SCP_MIN      ((sucs_ex_char_t)0x00110000ULL)
#define SUAS_SIS_ZONE_SCP_MAX      ((sucs_ex_char_t)0x0011FFFFULL)
#define SUAS_SIS_ZONE_NATIVE_MIN   ((sucs_ex_char_t)0x00120000ULL)
#define SUAS_SIS_ZONE_NATIVE_LAST  ((sucs_ex_char_t)0x7FFFFFEFULL)
#define SUAS_SIS_ZONE_TRAP_MIN     ((sucs_ex_char_t)0x7FFFFFF0ULL)
#define SUAS_SIS_ZONE_TRAP_MAX     ((sucs_ex_char_t)0x7FFFFFFEULL)
#define SUAS_SIS_ZONE_SENTINEL     ((sucs_ex_char_t)0x7FFFFFFFULL)
#endif

/* RNUR / RCCR neography carve-out inside the Native plane (SUTR-8 §1.4). */
#define SUAS_SIS_RNUR_MIN          ((sucs_ex_char_t)0x00160000ULL)
#define SUAS_SIS_RNUR_MAX          ((sucs_ex_char_t)0x0016FFFFULL)

/* ── Identifier classification (§2) ──────────────────────────────── */
/* The two structural identifier classes. Every codepoint resolves to
 * exactly one value. START is a superset-free class (a start character may
 * or may not also be a continue character); CONTINUE denotes a codepoint
 * that is valid only in a non-initial position. */
typedef enum {
    SUAS_SIS_CLASS_INVALID  = 0x0, /* cannot be part of an identifier */
    SUAS_SIS_CLASS_START    = 0x1, /* valid initial codepoint         */
    SUAS_SIS_CLASS_CONTINUE = 0x2  /* valid subsequent codepoint      */
} suas_sis_class_t;

/* ── Lexical syntax categories (§4) ──────────────────────────────── */
/* The disjoint split of UAX #31 §4: identifier characters (LITERAL, when
 * not classed as START/CONTINUE), whitespace, syntactic characters, and
 * ignorable format controls; CONTROL denotes in-band system codepoints. */
typedef enum {
    SUAS_SIS_SYNTAX_LITERAL    = 0, /* not whitespace/syntax/control      */
    SUAS_SIS_SYNTAX_WHITESPACE = 1, /* horizontal space (Pattern_White_Space) */
    SUAS_SIS_SYNTAX_LINETERM   = 2, /* line terminator (Pattern_White_Space)  */
    SUAS_SIS_SYNTAX_IGNORABLE  = 3, /* ignorable format control (LRM/RLM)  */
    SUAS_SIS_SYNTAX_OPERATOR   = 4, /* syntactic use (Pattern_Syntax)      */
    SUAS_SIS_SYNTAX_CONTROL    = 5  /* SCP / trap / in-band system control  */
} suas_sis_syntax_t;

/* ── Identifier profiles (§2, §3, §6) ────────────────────────────── */
typedef enum {
    SUAS_SIS_PROFILE_DEFAULT   = 0, /* R1: XID_Start / XID_Continue       */
    SUAS_SIS_PROFILE_IMMUTABLE = 1, /* R2: anything not PWS/PS/Co/Cs/Cc   */
    SUAS_SIS_PROFILE_HASHTAG   = 2  /* R8: '#' start; emoji + [-+_] cont. */
} suas_sis_profile_t;

/* ── Zero-allocation lexical state machine (§8) ──────────────────── */
typedef struct {
    bool     is_started; /* a valid identifier is currently in progress */
    uint32_t length;     /* accepted codepoints in the current identifier */
} suas_sis_state_t;

/* ── Setup and names ──────────────────────────────────────────────── */
const char* suas_sis_version_string(void);
const char* suas_sis_class_name(suas_sis_class_t klass);
const char* suas_sis_syntax_name(suas_sis_syntax_t syntax);
const char* suas_sis_profile_name(suas_sis_profile_t profile);

/* ── Class queries (§2) — O(1) for any 64-bit codepoint ──────────── */
suas_sis_class_t suas_sis_get_class(sucs_ex_char_t cp);
suas_sis_class_t suas_sis_get_class_ex(sucs_ex_char_t cp,
                                       suas_sis_profile_t profile);

/* Structural predicates. is_continue() is the authoritative test for a
 * non-initial position: a profile may admit a START character that is not
 * a legal CONTINUE character (the hashtag '#'). */
bool suas_sis_is_start(sucs_ex_char_t cp);
bool suas_sis_is_start_ex(sucs_ex_char_t cp, suas_sis_profile_t profile);
bool suas_sis_is_continue(sucs_ex_char_t cp);
bool suas_sis_is_continue_ex(sucs_ex_char_t cp, suas_sis_profile_t profile);

/* ── Lexical syntax (§4) ──────────────────────────────────────────── */
suas_sis_syntax_t suas_sis_get_syntax(sucs_ex_char_t cp);

/* True when the codepoint is inside the System Control Plane block
 * (0x00110000-0x0011FFFF); SCP markers are strictly invalid in identifiers
 * and are never pushed onto the state machine (§2.4). */
bool suas_sis_is_scp(sucs_ex_char_t cp);

/* ── State machine (§8) ───────────────────────────────────────────── */
void     suas_sis_init(suas_sis_state_t* state);
bool     suas_sis_is_started(const suas_sis_state_t* state);
uint32_t suas_sis_length(const suas_sis_state_t* state);

/* Feed one codepoint. Returns true iff the codepoint was validly appended
 * to the current identifier. A codepoint that cannot be appended and that
 * arrives while an identifier is in progress terminates that identifier and
 * resets the machine. The DEFAULT profile is used. */
bool suas_sis_feed(suas_sis_state_t* state, sucs_ex_char_t cp);
bool suas_sis_feed_ex(suas_sis_state_t* state, sucs_ex_char_t cp,
                      suas_sis_profile_t profile);

#ifdef __cplusplus
}
#endif

#endif /* SUAS_SUAS_SIS_H */
