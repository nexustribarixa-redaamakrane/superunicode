#ifndef SUAS_SUAS_SNCS_H
#define SUAS_SUAS_SNCS_H

/* SUAS-008 — System Named Character Sequences (SNCS)
 *
 * The core kernel architecture governing the deterministic parsing, static
 * packing, and O(1) resolution of multi-codepoint symbol names across the
 * SuperUnicode (SUCS) codepoint space (a UAX #34 analog).
 *
 * Where UAX #34 defines named character sequences for standard multi-character
 * symbols and emoji flag sequences through dynamic string tables, SNCS binds
 * the same concept to a zero-allocation, static lookup table, optimized for
 * the OpenWindows shell and the font rendering engine (suf). A named sequence
 * is a name paired with a fixed, stack-allocatable token array of codepoints;
 * the name resolves to the token in O(1) with no heap, no dynamic table
 * growth, and no string parsing.
 *
 * The engine keeps a dual-registry index:
 *   - Bridge sequences (0x000000-0x10FFFF): standard compatibility sequences
 *     mapped from the Unicode named sequence registry;
 *   - Native & neography sequences (0x160000-0x16FFFF): custom compound glyphs,
 *     ligatures, and multi-codepoint logographs defined by the RNUR codepoint
 *     registry, so a neography script can expose complex diacritic clusters and
 *     signs under unique string handles.
 * (RCCR functions separately as an out-of-band metadata/subtag registry for
 * language identification and allocates no codepoints.)
 *
 * Freestanding C99, zero allocation: the registry is a static table and the
 * lookup index is a fixed-size open-addressing array, neither of which grows.
 *
 * See docs/suas/SUAS-008-sncs.md for the normative specification.
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "suas/suas_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ── Specification identifiers ────────────────────────────────────── */
#define SUAS_SNCS_SPEC_ID       "SUAS-008"
#define SUAS_SNCS_SPEC_TITLE    "System Named Character Sequences"
#define SUAS_SNCS_VERSION_MAJOR 1
#define SUAS_SNCS_VERSION_MINOR 0
#define SUAS_SNCS_VERSION_PATCH 0

/* ── Fixed limits (static, stack-allocatable packing) ─────────────── */
/* Maximum codepoints in a single named sequence token. */
#define SUAS_SNCS_MAX_SEQUENCE_LEN 8U
/* Maximum bytes of a sequence name, including the terminating NUL. */
#define SUAS_SNCS_MAX_NAME_LEN     64U
/* Number of curated sequences the static registry can hold. */
#define SUAS_SNCS_REGISTRY_CAPACITY 64U
/* Number of open-addressing slots (a power of two > capacity). */
#define SUAS_SNCS_INDEX_SIZE        128U

/* ── 64-bit ExtSUCS codepoint type (guarded to coexist) ───────────── */
#ifndef EXTSUCS_CHAR_T_DEFINED
#define EXTSUCS_CHAR_T_DEFINED
typedef uint64_t sucs_ex_char_t;
#endif

/* ── Base SUCS zone boundaries (duplicated for freestanding SNCS) ─── */
#ifndef SUAS_SNCS_ZONE_UNICODE_MAX
#define SUAS_SNCS_ZONE_UNICODE_MAX  ((sucs_ex_char_t)0x0010FFFFULL)
#define SUAS_SNCS_ZONE_SCP_MIN      ((sucs_ex_char_t)0x00110000ULL)
#define SUAS_SNCS_ZONE_SCP_MAX      ((sucs_ex_char_t)0x0011FFFFULL)
#define SUAS_SNCS_ZONE_NATIVE_MIN   ((sucs_ex_char_t)0x00120000ULL)
#define SUAS_SNCS_ZONE_NATIVE_LAST  ((sucs_ex_char_t)0x7FFFFFEFULL)
#define SUAS_SNCS_ZONE_TRAP_MIN     ((sucs_ex_char_t)0x7FFFFFF0ULL)
#define SUAS_SNCS_ZONE_TRAP_MAX     ((sucs_ex_char_t)0x7FFFFFFEULL)
#define SUAS_SNCS_ZONE_SENTINEL     ((sucs_ex_char_t)0x7FFFFFFFULL)
#endif

/* RNUR / RCCR neography carve-out inside the Native plane (SUTR-8 §1.4).
 * RCCR allocates no codepoints; the RNUR registry supplies the compound
 * glyphs and logographs named here. */
#define SUAS_SNCS_RNUR_MIN          ((sucs_ex_char_t)0x00160000ULL)
#define SUAS_SNCS_RNUR_MAX          ((sucs_ex_char_t)0x0016FFFFULL)

/* ── Structural type: the static multi-codepoint token (§6.1) ─────── */
/* A named sequence packs its name and its codepoint sequence into a fixed
 * size, stack-allocatable token. `length` is the number of meaningful
 * codepoints in `codepoints`; a named sequence is always two or more. */
typedef struct {
    char           name[SUAS_SNCS_MAX_NAME_LEN];
    sucs_ex_char_t codepoints[SUAS_SNCS_MAX_SEQUENCE_LEN];
    uint8_t        length;
} suas_named_sequence_t;

/* ── Dual-registry classification (§3) ───────────────────────────── */
typedef enum {
    SUAS_SNCS_REGISTRY_BRIDGE = 0, /* Unicode named sequences, 0x000000-0x10FFFF */
    SUAS_SNCS_REGISTRY_NATIVE = 1  /* RNUR neography,        0x160000-0x16FFFF */
} suas_sncs_registry_t;

/* ── Setup and names ──────────────────────────────────────────────── */
const char* suas_sncs_version_string(void);
const char* suas_sncs_registry_name(suas_sncs_registry_t registry);

/* Build (or rebuild) the static registry lookup index. Idempotent; the
 * resolve entry point builds it lazily on first use if this is not
 * called first. Performs no allocation. */
void suas_sncs_init_registry(void);

/* Resolve a named character sequence by its string identifier in O(1).
 * On success, `*out_seq` receives a copy of the packed token and the
 * function returns true; on failure it returns false and leaves the
 * output untouched. `name` is matched exactly, byte for byte, against
 * the canonical (upper-case) registered name. */
bool suas_sncs_resolve_sequence(const char* name,
                                suas_named_sequence_t* out_seq);

/* ── Registry enumeration and classification (§3, §6) ─────────────── */
/* Number of curated sequences in the static registry. */
uint32_t suas_sncs_registry_count(void);

/* Copy the i-th curated sequence (0-based). Returns false out of range. */
bool suas_sncs_sequence_at(uint32_t index, suas_named_sequence_t* out_seq);

/* The registry a sequence belongs to: NATIVE if any of its codepoints
 * lies in the Native / neography plane, else BRIDGE. */
suas_sncs_registry_t suas_sncs_registry_of(
    const suas_named_sequence_t* seq);

/* ── Zone predicates ──────────────────────────────────────────────── */
/* True for a Unicode Bridge codepoint (0x000000-0x0010FFFF). */
bool suas_sncs_is_bridge_codepoint(sucs_ex_char_t cp);
/* True for a Native-plane codepoint (0x00120000-0x7FFFFFEF) or an
 * unbounded extSUCS plugin codepoint (above 0x7FFFFFFF). */
bool suas_sncs_is_native_codepoint(sucs_ex_char_t cp);

/* ── Name and sequence well-formedness (§4) ───────────────────────── */
/* SNCS-R1: only LATIN CAPITAL LETTER A-Z, digits 0-9 (not first in a
 * word), SPACE, and HYPHEN-MINUS; non-empty and within the name limit. */
bool suas_sncs_is_valid_name(const char* name);

/* SNCS-R3: two names are equal in the namespace for names when they are
 * identical after ignoring SPACE and medial HYPHEN-MINUS, and after
 * ignoring the substrings "LETTER", "CHARACTER", and "DIGIT". */
bool suas_sncs_names_equal(const char* a, const char* b);

/* True when a sequence is a well-formed named character sequence: a valid
 * name, a length in [2, SUAS_SNCS_MAX_SEQUENCE_LEN], and codepoints that
 * are neither System Control Plane markers nor trap / Sentinel slots. */
bool suas_sncs_is_well_formed(const suas_named_sequence_t* seq);

/* ── Sequence comparison ──────────────────────────────────────────── */
/* True when two sequences carry the same codepoint list (ignoring name). */
bool suas_sncs_sequence_equals(const suas_named_sequence_t* a,
                               const suas_named_sequence_t* b);

#ifdef __cplusplus
}
#endif

#endif /* SUAS_SUAS_SNCS_H */
