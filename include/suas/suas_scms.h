#ifndef SUAS_SUAS_SCMS_H
#define SUAS_SUAS_SCMS_H

/* SUAS-012 — System Character Manifest & Schema Standard (SCMS)
 *
 * The structural counterpart of Unicode's UAX #42 (the UCD in XML): the exact
 * grammar and field-mapping rules by which an offline character manifest is
 * written, validated, and compiled into the immutable binary lookup tables the
 * OpenWindows kernel embeds.
 *
 * UAX #42 describes the Unicode Character Database as an XML schema. That
 * choice is expensive. XML is hierarchical, so a record has no fixed size; the
 * parser must build a tree, so it must allocate; a schema validator is a
 * general-purpose engine, so its cost is unbounded; and no part of the result
 * is binary-mappable, so the data must be re-encoded again before a kernel can
 * use any of it. A character database compiled through UAX #42 therefore
 * costs a DOM, a heap, and two passes before the first lookup happens.
 *
 * SCMS rejects all of it. A manifest is a stream of lines; a record is exactly
 * six tab-delimited fields in a fixed order, each of a known and bounded type;
 * a directive is a name and a value. The grammar is line-oriented precisely so
 * that it is binary-mappable: one record line corresponds to one kernel table
 * entry, in order, with no re-encoding step and no intermediate tree. Build
 * tooling parses a manifest in a single linear pass over zero-allocation
 * streaming buffers, and a manifest that does not validate is never compiled.
 *
 * Dual-layer architecture, as with every other kernel primitive:
 *
 *   - the offline reference layer is the SuperUnicode Character Database
 *     (SUCD): the human-readable manifest files themselves, the sample
 *     datasets, and the tooling that reads them;
 *   - the runtime kernel layer is SCMS: the grammar constants, the bounded
 *     decoders, and the range and plane validation that a build-time compiler
 *     applies before a single byte reaches a kernel table.
 *
 * Validation is not advisory. Before a manifest is compiled, SCMS enforces
 * field types, field counts, range ordering, range span, and admissibility
 * across every SuperUnicode plane — the Unicode Compatibility Bridge, the
 * System Control Plane, the Native & Neography Plane and its RNUR carve-out —
 * and it refuses any line that would place a codepoint in the Control Plane,
 * a trap slot, or the Sentinel. The SCP invariance that SUAS-009 (SIP) and
 * SUTS-005 (SCSS) enforce on live codepoint queries is enforced here on the
 * data at rest, so a malformed manifest cannot smuggle an inadmissible value
 * into the image in the first place.
 *
 * Manifest integrity is also a linkage question, and SUAS-011 (SSRE) answers
 * it. A manifest exists to feed a target schema — SIP's property table, SCSS's
 * security table — and that schema is itself a registered specification with
 * version constraints. SCMS validates the manifest's structure; SSRE validates
 * that the schema the manifest is destined for is registered and satisfies the
 * declared floor. Both must hold before a blob is embedded.
 *
 * Freestanding C99, zero allocation: every token in a record is fixed-width or
 * strictly bounded, so every decoder is a bounded loop, every descriptor is a
 * fixed-size struct the caller owns, and no part of the engine reaches for
 * malloc, stdio, or libc string routines.
 *
 * See docs/suas/SUAS-012-scms.md for the normative specification.
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "suas/suas_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ── Specification identifiers ────────────────────────────────────── */
#define SUAS_SCMS_SPEC_ID        "SUAS-012"
#define SUAS_SCMS_SPEC_TITLE     "System Character Manifest & Schema Standard"
#define SUAS_SCMS_VERSION_MAJOR  1
#define SUAS_SCMS_VERSION_MINOR  0
#define SUAS_SCMS_VERSION_PATCH  0

/* ── Grammar limits (static, stack-allocatable parsing) ───────────── */
/* Longest accepted line, NUL included. Every record field is bounded, so the
 * record form is far shorter than this; the budget absorbs a long comment. */
#define SUAS_SCMS_MAX_LINE             128U
/* A record carries exactly this many tab-delimited fields. */
#define SUAS_SCMS_RECORD_FIELD_COUNT   6U
/* Maximum significant hex digits after "U+" — a 32-bit codepoint. The parsed
 * type is 64-bit so a future ExtSUCS manifest form can widen without
 * truncating; the grammar itself caps at 8 digits today. */
#define SUAS_SCMS_CODEPOINT_HEX_MAX    8U
/* Maximum significant hex digits after "0x" for the flag word. */
#define SUAS_SCMS_FLAGS_HEX_MAX        4U
/* Maximum significant decimal digits for any numeric field. */
#define SUAS_SCMS_DECIMAL_MAX          10U

/* ── Field value bounds ───────────────────────────────────────────── */
/* Kangxi radicals run 1…214; 0 means the record carries no radical. */
#define SUAS_SCMS_RADICAL_MAX          214U
#define SUAS_SCMS_STROKES_MAX          255U
#define SUAS_SCMS_FLAGS_MAX            0xFFFFU
#define SUAS_SCMS_WEIGHT_MAX           0xFFFFFFFFU
/* Widest range a single record line may describe. */
#define SUAS_SCMS_RANGE_MAX_SPAN       ((sucs_ex_char_t)0x0010FFFFULL)

/* ── 64-bit ExtSUCS codepoint type (guarded to coexist) ───────────── */
#ifndef EXTSUCS_CHAR_T_DEFINED
#define EXTSUCS_CHAR_T_DEFINED
typedef uint64_t sucs_ex_char_t;
#endif

/* ── Base SUCS zone boundaries (duplicated for freestanding SCMS) ─── */
#ifndef SUAS_SCMS_ZONE_UNICODE_MAX
#define SUAS_SCMS_ZONE_UNICODE_MAX  ((sucs_ex_char_t)0x0010FFFFULL)
#define SUAS_SCMS_ZONE_SCP_MIN      ((sucs_ex_char_t)0x00110000ULL)
#define SUAS_SCMS_ZONE_SCP_MAX      ((sucs_ex_char_t)0x0011FFFFULL)
#define SUAS_SCMS_ZONE_NATIVE_MIN   ((sucs_ex_char_t)0x00120000ULL)
#define SUAS_SCMS_ZONE_NATIVE_LAST  ((sucs_ex_char_t)0x7FFFFFEFULL)
#define SUAS_SCMS_ZONE_TRAP_MIN     ((sucs_ex_char_t)0x7FFFFFF0ULL)
#define SUAS_SCMS_ZONE_TRAP_MAX     ((sucs_ex_char_t)0x7FFFFFFEULL)
#define SUAS_SCMS_ZONE_SENTINEL     ((sucs_ex_char_t)0x7FFFFFFFULL)
#endif

/* RNUR / RCCR neography carve-out inside the Native plane (SUTR-8 §1.4). */
#define SUAS_SCMS_RNUR_MIN           ((sucs_ex_char_t)0x00160000ULL)
#define SUAS_SCMS_RNUR_MAX           ((sucs_ex_char_t)0x0016FFFFULL)

/* ── Plane mask bits (§4.3) ───────────────────────────────────────── */
/* A manifest header declares which planes its records touch. Note that RNUR
 * lives inside the Native plane, so a range wholly within RNUR reports both
 * NATIVE and RNUR. */
#define SUAS_SCMS_PLANE_BRIDGE   0x01u /* 0x000000–0x10FFFF             */
#define SUAS_SCMS_PLANE_SCP      0x02u /* 0x110000–0x11FFFF             */
#define SUAS_SCMS_PLANE_NATIVE   0x04u /* 0x120000–0x7FFFFFEF           */
#define SUAS_SCMS_PLANE_RNUR     0x08u /* 0x160000–0x16FFFF (inside NATIVE) */
#define SUAS_SCMS_PLANE_MASK_ALL 0x0Fu

/* ── Manifest line kinds (§3.1) ───────────────────────────────────── */
typedef enum {
    SCMS_LINE_BLANK     = 0, /* a zero-length line                        */
    SCMS_LINE_COMMENT   = 1, /* begins '#'                                */
    SCMS_LINE_DIRECTIVE = 2, /* begins '%'                                */
    SCMS_LINE_RECORD    = 3, /* begins a field                            */
    SCMS_LINE_INVALID   = 4  /* not dispatchable — over budget, or NULL   */
} scms_line_kind_t;

/* ── Validation outcomes (§5) ─────────────────────────────────────── */
typedef enum {
    SCMS_OK                        = 0,
    SCMS_ERR_NULL_INPUT            = 1,
    SCMS_ERR_LINE_TOO_LONG         = 2,
    SCMS_ERR_EMPTY_FIELD           = 3,
    SCMS_ERR_BAD_FIELD_COUNT       = 4,
    SCMS_ERR_BAD_CODEPOINT         = 5,
    SCMS_ERR_INADMISSIBLE_CODEPOINT= 6, /* SCP, trap slot, or the Sentinel */
    SCMS_ERR_RANGE_ORDER           = 7,
    SCMS_ERR_RANGE_TOO_WIDE        = 8,
    SCMS_ERR_BAD_RADICAL           = 9,
    SCMS_ERR_BAD_STROKES           = 10,
    SCMS_ERR_BAD_FLAGS             = 11,
    SCMS_ERR_BAD_WEIGHT            = 12,
    SCMS_ERR_BAD_DIRECTIVE         = 13,
    SCMS_ERR_MISSING_HEADER        = 14, /* no %manifest directive          */
    SCMS_ERR_RECORD_COUNT_MISMATCH = 15,
    SCMS_ERR_EMPTY_MANIFEST        = 16
} scms_error_t;

/* ── Record field taxonomy (§4.1) ─────────────────────────────────── */
/* The five kinds of value a manifest column may carry. A record has six
 * columns, not five: columns 0 and 1 are both SCMS_FIELD_CODEPOINT — the start
 * and the end of the described range — and columns 2…5 are radical, strokes,
 * flags, and weight in that order. */
typedef enum {
    SCMS_FIELD_CODEPOINT = 0,
    SCMS_FIELD_RADICAL   = 1,
    SCMS_FIELD_STROKES   = 2,
    SCMS_FIELD_FLAGS     = 3,
    SCMS_FIELD_WEIGHT    = 4
} scms_field_type_t;

/* ── Structural types (zero-allocation manifest descriptors) ──────── */

/* Populated from the directive block: %manifest, %records, %planes.
 * manifest_version borrows the caller's line buffer and is never copied; the
 * buffer must outlive every use of the descriptor. */
typedef struct {
    const char *manifest_version;
    uint32_t total_records;
    uint32_t plane_mask;         /* bitmask of active SuperUnicode planes covered */
} scms_manifest_header_t;

/* The parse result for one record line. Codepoints are widened to the 64-bit
 * ExtSUCS type so an RNUR neography logograph survives the round trip. */
typedef struct {
    sucs_ex_char_t start_codepoint;
    sucs_ex_char_t end_codepoint;      /* equal to start for a single codepoint */
    uint16_t field_count;              /* fields actually found, not expected   */
    bool     is_binary_aligned;        /* true iff the record is one codepoint  */
} scms_record_descriptor_t;

/* The streaming reader's scratch state. Caller-allocated — typically on the
 * stack — so that a whole manifest can be validated with no allocation. */
typedef struct {
    scms_manifest_header_t header;
    uint32_t lines;          /* every line fed, including blanks and comments */
    uint32_t records;
    uint32_t comments;
    uint32_t directives;
    uint32_t errors;         /* lines that failed validation                  */
    uint32_t first_error_line; /* 1-based; 0 if none                          */
    uint8_t  last_error;     /* scms_error_t of the first error               */
    uint8_t  observed_planes; /* SUAS_SCMS_PLANE_* bits the records touched  */
    bool     saw_manifest;   /* saw a %manifest directive                     */
    bool     saw_records;    /* saw a %records directive                      */
} scms_manifest_state_t;

/* ── A manifest's target schema, for the SSRE linkage gate (§6) ───── */
/* A manifest is meaningless on its own: it exists to feed a registered
 * schema. This names that schema and the floor the toolchain requires. */
typedef struct {
    const char *schema_spec_id;  /* e.g. "SUAS-009" for a SIP property table */
    uint8_t     schema_major;
    uint8_t     schema_minor;
    bool        schema_mandatory;
} scms_schema_link_t;

/* ── Engine setup ─────────────────────────────────────────────────── */

/* Version of this translation unit, e.g. "SUAS-012/1.0.0". Never NULL. */
const char* suas_scms_version_string(void);

/* Checks the grammar's static invariants and arms the engine. Idempotent and
 * safe to call from toolchain startup; the validators arm it lazily, so a
 * query is well-formed without setup. O(1), zero allocation. */
void suas_scms_init_engine(void);

/* ── Line classification (§3.1) ───────────────────────────────────── */

/* The kind of a manifest line. O(len), bounded by SUAS_SCMS_MAX_LINE. */
scms_line_kind_t suas_scms_line_kind(const char *manifest_line, size_t length);

/* ── Record validation (§4) ───────────────────────────────────────── */

/* Validates one line against the record grammar and, on success, fills the
 * caller's descriptor. O(len), bounded; zero allocation. Returns false on any
 * violation, with the reason available from suas_scms_last_error(). */
bool suas_scms_validate_record(const char *manifest_line, size_t length,
                               scms_record_descriptor_t *out_desc);

/* The value of the zero-based tab-delimited column, decoded from its U+,
 * 0x-, or decimal form. O(len), bounded. False for an out-of-range index, a
 * column that is not numeric, or an overflowing value. */
bool suas_scms_field_at(const char *manifest_line, size_t length,
                        uint16_t index, uint32_t *out_value);

/* The error reported by the most recent failing call on this thread. Read it
 * immediately after a false return. */
scms_error_t suas_scms_last_error(void);

/* Canonical name of an outcome — "SCMS_OK", "SCMS_ERR_RANGE_ORDER", … — or
 * "SCMS_ERR_UNKNOWN". Never NULL. */
const char* suas_scms_error_name(scms_error_t error);

/* ── Directive and header handling (§3.2) ─────────────────────────── */

/* Validates a single directive line, which must be well-formed regardless.
 * O(len), bounded. */
bool suas_scms_validate_directive(const char *manifest_line, size_t length);

/* Folds one line into the manifest state: directives update the header,
 * records are validated, counters accumulate. O(len), bounded. Returns false
 * if the line was rejected; the state records why. */
bool suas_scms_feed_line(scms_manifest_state_t *state,
                         const char *manifest_line, size_t length);

/* Resets a state and arms a fresh read. */
void suas_scms_begin(scms_manifest_state_t *state);

/* Closes a manifest: it must carry a %manifest directive and at least one
 * record, and where a %records directive declared a total, the counted total
 * must match. O(1). */
bool suas_scms_end(const scms_manifest_state_t *state);

/* True when every plane the manifest's records touched was declared in the
 * %planes directive — the cross-check that a manifest does not quietly write
 * outside the address space it claims to cover. A manifest with no %planes
 * directive can never satisfy it. O(1). */
bool suas_scms_planes_declared(const scms_manifest_state_t *state);

/* ── Range, plane, and alignment validation (§4.3, §4.4) ──────────── */

/* True if a codepoint may appear in a manifest: inside a permitted plane and
 * outside the Control Plane, the trap slots, and the Sentinel. O(1). */
bool suas_scms_codepoint_admissible(sucs_ex_char_t codepoint);

/* True if a range is ordered, admissible at both ends, and no wider than
 * SUAS_SCMS_RANGE_MAX_SPAN. O(1). */
bool suas_scms_range_admissible(sucs_ex_char_t start, sucs_ex_char_t end);

/* The SUAS_SCMS_PLANE_* bits every plane the range touches. O(1). */
uint8_t suas_scms_plane_mask(sucs_ex_char_t start, sucs_ex_char_t end);

/* True if a parsed descriptor describes exactly one codepoint and therefore
 * maps one line to one kernel table slot. O(1). */
bool suas_scms_is_binary_aligned(const scms_record_descriptor_t *desc);

/* ── SUAS-011 (SSRE) linkage gate (§6) ────────────────────────────── */

/* Confirms that the schema a manifest feeds is registered in the SSRE
 * cross-reference registry and satisfies the declared version floor and
 * mandatory flag. O(1) expected. False for a NULL link or an unregistered
 * schema — the build must not embed a blob whose schema is unknown. */
bool suas_scms_verify_schema_link(const scms_schema_link_t *link);

#ifdef __cplusplus
}
#endif

#endif /* SUAS_SUAS_SCMS_H */
