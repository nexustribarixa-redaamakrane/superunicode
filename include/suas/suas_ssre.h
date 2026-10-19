#ifndef SUAS_SUAS_SSRE_H
#define SUAS_SUAS_SSRE_H

/* SUAS-011 — System Specification Registry & Cross-Reference Engine (SSRE)
 *
 * The meta-standard of the OpenWindows microkernel: the normative registry and
 * deterministic linkage protocol governing how every SUAS standard, SUTS data
 * specification, and SUTR architecture report cross-references every other.
 *
 * Where a traditional Unicode environment carries its dependency story as a
 * document-level bibliography — a human-readable reference list in an annex
 * that no compiler ever reads and no linker ever checks — SSRE makes the
 * dependency tree a first-class, machine-verifiable artefact of the kernel
 * image itself.
 *
 * Dual-layer architecture, as with every other kernel primitive:
 *
 *   - the offline reference layer is the SuperUnicode Character Database
 *     (SUCD): the human-readable specification manifest files, normative
 *     reference lists, and standards indices that documentation, external
 *     compilers, and compliance tooling consult;
 *   - the runtime kernel layer is SSRE: at build time the SUCD manifests are
 *     compiled into an immutable, byte-packed binary registry of
 *     ssre_spec_record_t records, and the kernel validates subsystem
 *     integrity directly against those static tables at boot and at load.
 *
 * Two enforcement surfaces, both O(1) and both zero-allocation:
 *
 *   - SUAS_SSRE_VALIDATE_DEP / SUAS_SSRE_REQUIRE_DEP are preprocessor-level
 *     gates. Because a module's declared version constants are integer
 *     literals, the predicate evaluates inside #if, so a schema misalignment
 *     is a hard compile error before a single byte of the image is produced.
 *   - suas_ssre_verify_dependency() is the run-time mirror, for dependencies
 *     whose provider is only resolved when a module is actually loaded —
 *     plugins, optional data specifications, dynamically linked SUTS engines.
 *
 * A dependency is not merely a version floor. SSRE classifies every registered
 * specification into a cross-reference kind — MANDATORY (a SUAS invariant),
 * OPTIONAL (a SUTS data specification), or REFERENCE (a SUTR architecture
 * report) — and records the SUCM conformance tier it claims. A descriptor that
 * asserts is_mandatory against a registered OPTIONAL specification is an
 * architectural error, and SSRE rejects it: that is the whole point of a
 * non-negotiable standard.
 *
 * Freestanding C99, zero allocation: the registry is a static array plus a
 * fixed-size open-addressing index, neither of which grows at run time, and no
 * part of the engine reaches for libc string routines — identity and hashing
 * are performed over the NUL-terminated ASCII spec identifiers in bounded,
 * single-pass loops.
 *
 * See docs/suas/SUAS-011-ssre.md for the normative specification.
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "suas/suas_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ── Specification identifiers ────────────────────────────────────── */
#define SUAS_SSRE_SPEC_ID        "SUAS-011"
#define SUAS_SSRE_SPEC_TITLE     "System Specification Registry & Cross-Reference Engine"
#define SUAS_SSRE_VERSION_MAJOR  1
#define SUAS_SSRE_VERSION_MINOR  0
#define SUAS_SSRE_VERSION_PATCH  0

/* ── Fixed limits (static, stack-allocatable registry) ────────────── */
/* Number of registered specifications the static registry can hold. */
#define SUAS_SSRE_REGISTRY_CAPACITY 64U
/* Number of open-addressing slots (a power of two > capacity). */
#define SUAS_SSRE_INDEX_SIZE        256U
/* Longest registered identifier including its NUL, e.g. "SUAS-011". */
#define SUAS_SSRE_MAX_SPEC_ID       16U

/* ── SUCM compliance tiers (SUTR-10 §3) ───────────────────────────── */
#define SUAS_SSRE_TIER_L0     0U  /* Foundation & Encoding Layer          */
#define SUAS_SSRE_TIER_L1     1U  /* Rendering & Layout Layer             */
#define SUAS_SSRE_TIER_L2     2U  /* Semantic & Interaction Layer         */
#define SUAS_SSRE_TIER_LEVELS 3U

/* ── Cross-reference kinds (§4) ───────────────────────────────────── */
/* The tier of a specification in the standards hierarchy, not the tier of a
 * module's conformance claim. A descriptor's is_mandatory is only satisfiable
 * against SUAS_SSRE_KIND_MANDATORY. */
typedef enum {
    SUAS_SSRE_KIND_MANDATORY = 0, /* SUAS — non-negotiable architecture  */
    SUAS_SSRE_KIND_OPTIONAL  = 1, /* SUTS — optional data specification  */
    SUAS_SSRE_KIND_REFERENCE = 2  /* SUTR — architecture report         */
} suas_ssre_kind_t;

/* ── Structural types (zero-allocation metadata) ──────────────────── */

/* A dependency declaration supplied by the *requiring* module. The spec_id is
 * a NUL-terminated ASCII identifier pointing into read-only storage; it is
 * never copied, hashed state is never retained, and the descriptor itself is
 * caller-owned. */
typedef struct {
    const char *spec_id;          /* e.g., "SUTS-003", "SUAS-009"        */
    uint8_t min_version_major;
    uint8_t min_version_minor;
    bool is_mandatory;            /* true if SUAS, false if optional SUTS*/
} ssre_dependency_descriptor_t;

/* A registry record: one registered specification, compiled at build time
 * from the SUCD manifest and immutable for the life of the image. */
typedef struct {
    const char *spec_id;    /* canonical key, NUL-terminated, unique    */
    const char *short_name; /* family acronym, e.g. "SLDF"             */
    const char *module;     /* reference unit, e.g. "suts_sldf"; "" if
                             * the specification defines no C module    */
    uint8_t     major;      /* registered version                       */
    uint8_t     minor;
    uint8_t     kind;       /* suas_ssre_kind_t                         */
    uint8_t     tier;       /* SUCM tier the module claims (SUTR-10 §3) */
} ssre_spec_record_t;

/* ── Compile-time dependency gate (§7) ────────────────────────────── */

/* A constant predicate over four integer version literals. Every operator used
 * here is a preprocessor operator, so this macro is valid both in ordinary C
 * expressions and inside #if — which is what makes a compile-time gate
 * possible at all. A satisfied dependency folds to 1; an unsatisfied one folds
 * to 0 and, under SUAS_SSRE_REQUIRE_DEP, becomes a negative array bound. */
#define SUAS_SSRE_VALIDATE_DEP(req_major, req_minor, got_major, got_minor) \
    (((got_major) > (req_major)) ||                                        \
     (((got_major) == (req_major)) && ((got_minor) >= (req_minor))))

/* The classic C99 static-assertion idiom, sized off a caller-chosen symbol so
 * several gates may coexist in one translation unit. A failure reports as
 * "size of array is negative" naming the dependency being enforced. */
#define SUAS_SSRE_REQUIRE_DEP(sym, req_major, req_minor, got_major, got_minor) \
    typedef char sym[(SUAS_SSRE_VALIDATE_DEP((req_major), (req_minor),       \
                                             (got_major), (got_minor))) ? 1 : -1]

/* ── Registry setup ───────────────────────────────────────────────── */

/* Version of this translation unit, e.g. "SUAS-011/1.0.0". Never NULL. */
const char* suas_ssre_version_string(void);

/* Builds the cross-reference index over the static registry. Idempotent and
 * lazy: the first query builds it, every subsequent query reuses it. O(1)
 * amortised, zero allocation. Safe to call explicitly from kernel boot. */
void suas_ssre_init_registry(void);

/* ── Dependency validation (§3) ───────────────────────────────────── */

/* Validates one descriptor against the registry: the specification must be
 * registered, its registered version must satisfy the declared floor, and if
 * is_mandatory is set the specification must be registered as a SUAS
 * MANDATORY standard. O(1). Returns false on a NULL descriptor. */
bool suas_ssre_verify_dependency(const ssre_dependency_descriptor_t *dep);

/* Validates a whole dependency set. Every descriptor must verify; an empty or
 * NULL set with count 0 verifies vacuously. O(count). */
bool suas_ssre_verify_set(const ssre_dependency_descriptor_t *deps,
                          size_t count);

/* Returns the first unverifiable descriptor in the set, or NULL if the whole
 * set verifies — the diagnostic form of suas_ssre_verify_set(). A NULL set is
 * rejected by verify_set but has no descriptor to point at, so NULL is
 * returned there too; consult the count form to distinguish the two. */
const ssre_dependency_descriptor_t*
suas_ssre_first_missing(const ssre_dependency_descriptor_t *deps, size_t count);

/* Number of descriptors in the set that fail to verify. A NULL set with a
 * non-zero count fails entirely, so every member is counted. O(count). */
size_t suas_ssre_dependency_count(const ssre_dependency_descriptor_t *deps,
                                  size_t count);

/* ── Registry lookup (§2) ─────────────────────────────────────────── */

/* The record for a spec_id, or NULL if unregistered. O(1) expected. */
const ssre_spec_record_t* suas_ssre_lookup(const char *spec_id);

/* True if the spec_id is registered. O(1) expected. */
bool suas_ssre_is_registered(const char *spec_id);

/* Reads the registered version. O(1) expected. False if unregistered. Either
 * out-parameter may be NULL to read only one half of the version. */
bool suas_ssre_version_of(const char *spec_id,
                          uint8_t *out_major, uint8_t *out_minor);

/* True if the spec_id is registered as a SUAS MANDATORY standard. False for
 * unregistered identifiers and NULL. O(1) expected. */
bool suas_ssre_is_mandatory(const char *spec_id);

/* The SUCM tier the registered module claims, or SUAS_SSRE_TIER_LEVELS when
 * the specification is unregistered — an out-of-range sentinel, never a
 * plausible tier. O(1) expected. */
uint8_t suas_ssre_tier_of(const char *spec_id);

/* The reference unit for a spec_id, e.g. "suts_sldf", or NULL if
 * unregistered. The returned pointer is into immutable storage. */
const char* suas_ssre_module_of(const char *spec_id);

/* The inverse cross-reference: the spec_id declaring a given reference unit,
 * or NULL. "" matches no module. O(n) over the registry, by design — this is
 * the one-directional index and is intended for tooling, not the boot path. */
const char* suas_ssre_spec_of_module(const char *module);

/* True if the registered version already satisfies the given floor. False for
 * unregistered identifiers and NULL. O(1) expected. */
bool suas_ssre_satisfies(const char *spec_id,
                         uint8_t min_major, uint8_t min_minor);

/* ── Enumeration (§2.4) ───────────────────────────────────────────── */

/* Number of registered specifications. O(1). */
size_t suas_ssre_spec_count(void);

/* Record at a stable registration index, or NULL when out of range. O(1). */
const ssre_spec_record_t* suas_ssre_spec_at(size_t index);

/* Number of registered specifications of a given kind. O(1). */
size_t suas_ssre_kind_count(uint8_t kind);

/* Canonical name of a kind — "MANDATORY", "OPTIONAL", "REFERENCE" — or
 * "UNKNOWN". Never NULL. */
const char* suas_ssre_kind_name(uint8_t kind);

/* ── Primitives ───────────────────────────────────────────────────── */

/* FNV-1a 32-bit over the bytes of a NUL-terminated identifier. NULL hashes to
 * the offset basis. Bounded by SUAS_SSRE_MAX_SPEC_ID, hence O(1). */
uint32_t suas_ssre_hash(const char *spec_id);

/* Freestanding identifier equality: byte-wise equality over two
 * NUL-terminated strings, bounded by SUAS_SSRE_MAX_SPEC_ID. NULL equals only
 * NULL. No libc dependency. */
bool suas_ssre_id_equal(const char *a, const char *b);

#ifdef __cplusplus
}
#endif

#endif /* SUAS_SUAS_SSRE_H */
