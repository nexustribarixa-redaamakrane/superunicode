#ifndef SUTS_SUTS_SCSS_H
#define SUTS_SUTS_SCSS_H

/* SUTS-005 — System Character Security Specification (SCSS)
 *
 * The core OpenWindows kernel primitive governing character-level security:
 * visual homograph (confusable) detection, identifier permissibility, and
 * script-boundary isolation across SuperUnicode string streams. SCSS maps a
 * codepoint to a compiled security attribute word and to a canonical prototype
 * skeleton, so a kernel object path, an IPC channel name, a symbol table entry,
 * or a user handle can be evaluated without heap allocation and without a
 * text-based security database.
 *
 * Where a user-space i18n security layer such as UTS #39 (Unicode Security
 * Mechanisms — Confusables.txt, IdentifierStatus.txt, IdentifierType.txt) is a
 * set of flat text files parsed at run time into heap-resident lookup trees,
 * SCSS:
 *
 *   - is a compiled binary schema — fixed-size, byte-aligned C structures
 *     (scss_security_attr_t, scss_skeleton_entry_t) that sit directly in
 *     read-only memory at known offsets, with no parser;
 *   - obeys the zero-allocation mandate — every query is an O(1) probe of a
 *     fixed-size open-addressing index built once over a static table, and
 *     never touches a heap;
 *   - is a kernel primitive — the microkernel, the shell parser, and the
 *     authentication path decide identifier admissibility in-band, with no
 *     external file system, user process, or service dependency.
 *
 * SCSS is a dual-layer system. The offline reference layer is the SuperUnicode
 * Character Database (SUCD): the full text manifests — confusable mappings,
 * identifier profiles, and human-readable security tables — live there for
 * compilers, developer toolchains, and user-space web applications. Build-time
 * tooling distils those datasets into the immutable binary tables this module
 * embeds in the kernel memory image. (The seeding brief named the offline layer
 * "SUCA"; in this corpus SUCA is already the SuperUnicode Collation Algorithm,
 * SUTS-001, so SCSS names SUCD, consistent with SUAS-009 SIP.)
 *
 * Freestanding C99, zero allocation: the registry is a static array of
 * scss_security_attr_t records plus a static array of scss_skeleton_entry_t
 * records, each indexed into a fixed-size array of slots, none of which grows
 * at run time.
 *
 * See docs/suts/SUTS-005-scss.md for the normative specification.
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Specification identifiers ────────────────────────────────────── */
#define SUTS_SCSS_SPEC_ID        "SUTS-005"
#define SUTS_SCSS_SPEC_TITLE     "System Character Security Specification"
#define SUTS_SCSS_VERSION_MAJOR  1
#define SUTS_SCSS_VERSION_MINOR  0
#define SUTS_SCSS_VERSION_PATCH  0

/* ── Fixed limits (static, stack-allocatable blocks) ──────────────── */
/* Number of curated per-codepoint attribute records the registry holds. */
#define SUTS_SCSS_ATTR_CAPACITY     128U
/* Number of curated confusable skeleton mappings the registry holds.     */
#define SUTS_SCSS_SKELETON_CAPACITY 128U
/* Number of open-addressing slots per index (a power of two > capacity). */
#define SUTS_SCSS_INDEX_SIZE        256U

/* ── 64-bit ExtSUCS codepoint type (guarded to coexist) ───────────── */
#ifndef EXTSUCS_CHAR_T_DEFINED
#define EXTSUCS_CHAR_T_DEFINED
typedef uint64_t sucs_ex_char_t;
#endif

/* ── Base SUCS zone boundaries (duplicated for freestanding SCSS) ─── */
#ifndef SUTS_SCSS_ZONE_UNICODE_MAX
#define SCSS_ZONE_UNICODE_MAX  ((sucs_ex_char_t)0x0010FFFFULL) /* UCB, 1:1 with Unicode */
#define SCSS_ZONE_SCP_MIN      ((sucs_ex_char_t)0x00110000ULL)
#define SCSS_ZONE_SCP_MAX      ((sucs_ex_char_t)0x0011FFFFULL)
#define SCSS_ZONE_NATIVE_MIN   ((sucs_ex_char_t)0x00120000ULL)
#define SCSS_ZONE_NATIVE_LAST  ((sucs_ex_char_t)0x7FFFFFEFULL)
#define SCSS_ZONE_TRAP_MIN     ((sucs_ex_char_t)0x7FFFFFF0ULL)
#define SCSS_ZONE_TRAP_MAX     ((sucs_ex_char_t)0x7FFFFFFEULL)
#define SCSS_ZONE_SENTINEL     ((sucs_ex_char_t)0x7FFFFFFFULL)
#endif

/* RNUR / RCCR neography carve-out inside the Native plane (SUTR-8 §1.4). */
#define SCSS_RNUR_MIN          ((sucs_ex_char_t)0x00160000ULL)
#define SCSS_RNUR_MAX          ((sucs_ex_char_t)0x0016FFFFULL)

/* ── FNV-1a constants for the codepoint hash (§4.2) ───────────────── */
#define SUTS_SCSS_FNV_OFFSET_BASIS ((uint32_t)2166136261u)
#define SUTS_SCSS_FNV_PRIME        ((uint32_t)16777619u)

/* ── Compiled security metadata flags (§2.1) ──────────────────────── */
/* Safe for kernel/system paths and secure identifiers. */
#define SUTS_SCSS_FLAG_ALLOWED_IDENTIFIER (1u << 0)
/* Invisible, unassigned, or control: never part of an identifier. */
#define SUTS_SCSS_FLAG_RESTRICTED         (1u << 1)
/* A known visual homograph; carries a skeleton mapping (§2.2). */
#define SUTS_SCSS_FLAG_CONFUSABLE         (1u << 2)
/* Requires an isolated script context (Native & Neography); see §2.3. */
#define SUTS_SCSS_FLAG_NEOGRAPHY_MIXED    (1u << 3)

/* ── Address-space planes (§5) ────────────────────────────────────── */
typedef enum {
    SUTS_SCSS_PLANE_BRIDGE   = 0, /* 0x00000000-0x0010FFFF           */
    SUTS_SCSS_PLANE_SCP      = 1, /* 0x00110000-0x0011FFFF           */
    SUTS_SCSS_PLANE_NATIVE   = 2, /* 0x00120000-0x7FFFFFEF           */
    SUTS_SCSS_PLANE_RNUR     = 3, /* 0x00160000-0x0016FFFF           */
    SUTS_SCSS_PLANE_TRAP     = 4, /* 0x7FFFFFF0-0x7FFFFFFE           */
    SUTS_SCSS_PLANE_SENTINEL = 5, /* 0x7FFFFFFF                      */
    SUTS_SCSS_PLANE_EXTSUCS  = 6  /* > 0x7FFFFFFF (plugin space)     */
} suts_scss_plane_t;

/* ── Script groups for boundary isolation (§2.3) ──────────────────── */
/* A coarse script classification, sufficient to tell a same-script
 * identifier from a mixed-script spoof. COMMON covers script-neutral
 * codepoints (ASCII digits, punctuation, symbols) that combine with any
 * script; NATIVE covers the Native & Neography plane. */
typedef enum {
    SUTS_SCSS_SCRIPT_COMMON   = 0, /* script-neutral digits, punctuation */
    SUTS_SCSS_SCRIPT_LATIN    = 1,
    SUTS_SCSS_SCRIPT_GREEK    = 2,
    SUTS_SCSS_SCRIPT_CYRILLIC = 3,
    SUTS_SCSS_SCRIPT_HAN      = 4,
    SUTS_SCSS_SCRIPT_ARABIC   = 5,
    SUTS_SCSS_SCRIPT_HEBREW   = 6,
    SUTS_SCSS_SCRIPT_NATIVE   = 7, /* Native / RNUR neography plane      */
    SUTS_SCSS_SCRIPT_SCP      = 8, /* SCP markers, traps, the Sentinel   */
    SUTS_SCSS_SCRIPT_UNKNOWN  = 9  /* unclassified (ExtSUCS plugins)     */
} suts_scss_script_t;

/* ── Status codes ─────────────────────────────────────────────────── */
typedef enum {
    SUTS_SCSS_OK              =  0,
    SUTS_SCSS_NOT_FOUND       =  1,
    SUTS_SCSS_ERR_INVALID_ARG = -1
} suts_scss_status_t;

/* ── Binary-compiled security structures (zero-allocation) ────────── */

/* A security skeleton mapping (§2.2). The codepoint fields are
 * `sucs_ex_char_t` (64-bit), never `uint32_t`, so an RNUR neography
 * lookalike in 0x00160000-0x0016FFFF and an ExtSUCS plugin codepoint
 * above 0x7FFFFFFF survive intact. `skeleton_codepoint` is the prototype
 * canonical skeleton a confusable variant maps onto; two codepoints are
 * confusable exactly when their skeletons agree (§2.2). */
typedef struct {
    sucs_ex_char_t codepoint;          /* the lookalike, e.g. U+0430      */
    sucs_ex_char_t skeleton_codepoint; /* its prototype, e.g. U+0061 'a'  */
} scss_skeleton_entry_t;

/* A compiled per-codepoint security attribute (§2.1). `flags` is a
 * bitmask of SUTS_SCSS_FLAG_*. */
typedef struct {
    sucs_ex_char_t codepoint; /* the codepoint, e.g. U+200B             */
    uint32_t       flags;     /* SUTS_SCSS_FLAG_*                       */
} scss_security_attr_t;

/* ── Function declarations ────────────────────────────────────────── */

/* Version string, e.g. "1.0.0". */
const char* scss_version_string(void);

/* Builds (or rebuilds) the SCSS kernel security registry indexes. Idempotent
 * and allocation-free; the query entry points build them lazily on first use
 * if this is not called first. */
void scss_init_registry(void);

/* Resolves the compiled security attribute word of a codepoint in O(1)
 * expected time. Returns the bitmask of SUTS_SCSS_FLAG_* (0 when the codepoint
 * carries no attribute and no structural default applies). A Native &
 * Neography codepoint always carries SUTS_SCSS_FLAG_NEOGRAPHY_MIXED; an SCP
 * marker, trap slot, or the Sentinel always carries
 * SUTS_SCSS_FLAG_RESTRICTED. */
uint32_t scss_get_flags(sucs_ex_char_t codepoint);

/* Looks up the binary skeleton prototype for a codepoint. Returns the mapped
 * skeleton codepoint, or the original codepoint when no mapping exists. */
sucs_ex_char_t scss_resolve_skeleton(sucs_ex_char_t codepoint);

/* ── Security predicates (§3) ─────────────────────────────────────── */
/* True when a codepoint is permissible in a secure identifier: it carries
 * SUTS_SCSS_FLAG_ALLOWED_IDENTIFIER and none of RESTRICTED, CONFUSABLE, or
 * NEOGRAPHY_MIXED (O(1)). */
bool scss_is_allowed_identifier(sucs_ex_char_t codepoint);

/* True when a codepoint carries SUTS_SCSS_FLAG_RESTRICTED (O(1)). */
bool scss_is_restricted(sucs_ex_char_t codepoint);

/* True when a codepoint is a known visual homograph (O(1)). */
bool scss_is_confusable(sucs_ex_char_t codepoint);

/* True when a codepoint requires an isolated script context (O(1)). */
bool scss_is_neo_mixed(sucs_ex_char_t codepoint);

/* True when two distinct codepoints share a skeleton, i.e. are visual
 * homographs of one another (O(1)). */
bool scss_confusable_with(sucs_ex_char_t a, sucs_ex_char_t b);

/* ── Attribute and skeleton lookup ────────────────────────────────── */
/* Stable pointer to the immutable attribute record for a codepoint, or NULL
 * when the codepoint is not explicitly curated (O(1) expected). The pointer
 * is stable for the lifetime of the image and must never be freed. */
const scss_security_attr_t* scss_lookup_attr(sucs_ex_char_t codepoint);

/* Stable pointer to the immutable skeleton record for a codepoint, or NULL
 * when the codepoint has no explicit mapping (O(1) expected). */
const scss_skeleton_entry_t* scss_lookup_skeleton(sucs_ex_char_t codepoint);

/* ── Enumeration ──────────────────────────────────────────────────── */
/* Number of records in the static attribute table (O(1)). */
size_t scss_attr_count(void);

/* Direct, stable pointer to attribute record `index`, or NULL when out of
 * range (O(1)). The order is the static table order and is stable. */
const scss_security_attr_t* scss_attr_at(size_t index);

/* Number of records in the static skeleton table (O(1)). */
size_t scss_skeleton_count(void);

/* Direct, stable pointer to skeleton record `index`, or NULL (O(1)). */
const scss_skeleton_entry_t* scss_skeleton_at(size_t index);

/* ── Script boundary isolation (§2.3) ─────────────────────────────── */
/* Classifies a codepoint into a coarse script group (O(1)). */
suts_scss_script_t scss_script_of(sucs_ex_char_t codepoint);

/* True when two codepoints may appear together in a single identifier:
 * they share a script group, or either is script-neutral COMMON (O(1)). */
bool scss_script_compatible(sucs_ex_char_t a, sucs_ex_char_t b);

/* True when mixing two codepoints crosses a script boundary — the classic
 * cross-script injection vector (O(1)). */
bool scss_is_mixed_script(sucs_ex_char_t a, sucs_ex_char_t b);

/* True specifically when one codepoint is in the Unicode Compatibility
 * Bridge (0x000000-0x10FFFF) and the other in the Native & Neography plane:
 * the primary boundary SCSS protects a system path from (O(1)). */
bool scss_crosses_bridge_boundary(sucs_ex_char_t a, sucs_ex_char_t b);

/* ── Zone classification (§5) ─────────────────────────────────────── */
/* True for a codepoint in the Unicode Compatibility Bridge. */
bool scss_is_bridge_codepoint(sucs_ex_char_t codepoint);

/* True for a codepoint in the Native plane, 0x00120000-0x7FFFFFEF (the RNUR
 * neography carve-out included). ExtSUCS plugin codepoints above the Sentinel
 * are not Native; they classify as SUTS_SCSS_SCRIPT_UNKNOWN. */
bool scss_is_native_codepoint(sucs_ex_char_t codepoint);

/* Classifies a codepoint into its address-space plane (O(1)). */
suts_scss_plane_t scss_plane_of(sucs_ex_char_t codepoint);

/* ── Codepoint hash ───────────────────────────────────────────────── */
/* FNV-1a 32-bit hash over the little-endian encoding of a codepoint
 * (O(1), §4.2). */
uint32_t scss_hash(sucs_ex_char_t codepoint);

#ifdef __cplusplus
}
#endif

#endif /* SUTS_SUTS_SCSS_H */
