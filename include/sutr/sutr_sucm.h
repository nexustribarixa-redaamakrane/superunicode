#ifndef SUTR_SUTR_SUCM_H
#define SUTR_SUTR_SUCM_H

/* SUTR-10 — SuperUnicode Conformance Model (SUCM)
 *
 * A modular technical-report specification within the OpenWindows microkernel
 * ecosystem. SUCM establishes the compliance surface of SuperUnicode: the three
 * cumulative conformance tiers, the zero-allocation and O(1) execution
 * mandate, the conformance claim clauses and their naming rules, and the
 * automated verification gates that every kernel subsystem, driver, or
 * ExtSUCS plugin must satisfy before a system image may be compiled.
 *
 * Where UTR #33, "Unicode Conformance Model", defines what it means for a
 * process to claim conformance to the Unicode Standard — conformance clauses,
 * implementation levels, and the interoperability guarantees that follow —
 * SUCM is the SuperUnicode analog, with one decisive difference: the resource
 * model. UTR #33 is silent on how a conforming process spends memory or
 * time; SUCM grades conformance by tier *and* by whether the unit holds the
 * zero-allocation and O(1) mandate of §4. On SuperUnicode the kernel runs on
 * a freestanding target with no general allocator and no bounded-time budget
 * for a backtracking pass, so an algorithm that allocates on the hot path or
 * scans in variable time does not conform, however correct its output.
 *
 * The model binds the individual algorithm contracts of the SUAS and SUTS
 * families into a single checkable surface. It names the tiers, fixes the
 * resource ceilings, defines how a conformance claim is written and named,
 * and specifies the pipeline that proves a claim is true:
 *
 *     build → ctest (test gate) → verify-docs (documentation gate)
 *           → image compile                                          (§7.4)
 *
 * A claim that is not backed by a passing gate record is void (§5.1 C6,
 * §6 R15). The gates are the contract; without them the clauses are prose.
 *
 * This module is the reference implementation of that surface. It provides:
 *
 *   - the tier model of §3 and its cumulative inclusion relation (§3.4);
 *   - the clause registry of §5 — the SUCM-D, SUCM-C and SUCM-R identifiers
 *     with their kinds, numbers, and titles;
 *   - the claim identifier grammar of §2.3 and the naming rules R1–R4 of §5.4,
 *     including the R3 equivalence relation;
 *   - the claim row of §8 (`tier;unit;version;clauses;gate-record`) and its
 *     validation against the tier's clause set;
 *   - the gate record of §7 — the test gate, the documentation gate, and the
 *     build gate;
 *   - the core engine registry of §8 (module, owning spec, memory class,
 *     time class) and the published claim set;
 *   - the three resource gates a Level-2 unit must hold: the native-plane
 *     range gate (R10), the integrity gate (R11), and the neography
 *     carve-out (R12).
 *
 * Freestanding C99, zero allocation: every table is static, every descriptor
 * is a fixed-size struct the caller owns, and no part of the module reaches
 * for malloc, stdio, or libc string routines. The registry lookups are
 * bounded scans over a fixed table, so the per-query cost is O(1) with
 * respect to the input and independent of the size of the corpus.
 *
 * See docs/sutr/SUTR-10-sucm.md for the normative specification.
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Specification identifiers ────────────────────────────────────── */

#define SUCM_SPEC_ID            "SUTR-10"
#define SUCM_SPEC_TITLE         "SuperUnicode Conformance Model"
#define SUCM_VERSION_MAJOR      0
#define SUCM_VERSION_MINOR      1
#define SUCM_VERSION_PATCH      0
#define SUCM_MODULE             "sutr_sucm"

/* ── Fixed limits (static, stack-allocatable tables) ──────────────── */

#define SUCM_MAX_UNIT           32U   /* the unit component of a claim id   */
#define SUCM_MAX_VERSION        16U   /* the version component              */
#define SUCM_MAX_CLAUSE_LIST    160U  /* the clause column of a claim row   */
#define SUCM_MAX_GATE           64U   /* the gate-record column              */
#define SUCM_MAX_HARNESS        32U   /* the harness name                    */
#define SUCM_MAX_SUITE          64U   /* one suite or document name          */
#define SUCM_MAX_SUITES         32U   /* the suites a gate record exercises  */
#define SUCM_CLAUSE_SET_BITS    32U   /* one bit per registered clause       */
#define SUCM_CLAUSE_COUNT       29U   /* D1-D8, C1-C6, R1-R15                */
#define SUCM_ENGINE_COUNT       32U   /* the conformance units of §1.3.1     */
#define SUCM_CLAIM_COUNT        32U   /* the published claims of §8          */

/* The resource ceilings of §4.2. */
#define SUCM_HEAP_CEILING_BYTES 0U
#define SUCM_TRAP_SLOT_LOW      0x7FFFFFF0ULL  /* unallocatable (§3.3, R10)   */
#define SUCM_TRAP_SLOT_HIGH     0x7FFFFFFEULL  /* unallocatable (§3.3, R10)   */
#define SUCM_SENTINEL           0x7FFFFFFFULL  /* unallocatable (§3.3, R10)   */

/* The plane boundaries of §3.2, §3.3, and SUTR-5 §5. */
#define SUCM_BRIDGE_MAX         0x00010FFFFULL  /* the Bridge range (§3.2)    */
#define SUCM_NATIVE_MIN         0x00120000ULL  /* the Native plane (§3.3)     */
#define SUCM_NATIVE_MAX         0x7FFFFFEFULL  /* the Native plane (§3.3)     */
#define SUCM_NEOGRAPHY_MIN      0x00160000ULL  /* the carve-out (R12)        */
#define SUCM_NEOGRAPHY_MAX      0x0016FFFFULL  /* the carve-out (R12)        */

/* ── The tiers of §3 ──────────────────────────────────────────────── */

typedef enum {
    SUCM_TIER_L0 = 0,   /* Kernel Core Conformance                    */
    SUCM_TIER_L1 = 1,   /* Unicode Bridge Conformance                 */
    SUCM_TIER_L2 = 2,   /* Extended & Plugin Conformance              */
    SUCM_TIER_COUNT = 3
} sucm_tier_t;

/* ── The clause registry of §5.2 ──────────────────────────────────── */

typedef enum {
    SUCM_CLAUSE_DEFINITION  = 0,  /* SUCM-Dn */
    SUCM_CLAUSE_CONFORMANCE = 1,  /* SUCM-Cn */
    SUCM_CLAUSE_REQUIREMENT = 2,  /* SUCM-Rn */
    SUCM_CLAUSE_KIND_COUNT  = 3
} sucm_clause_kind_t;

/* The three kinds, in the order the registry sorts them. */
#define SUCM_CLAUSE_KIND_DEFINITION  SUCM_CLAUSE_DEFINITION
#define SUCM_CLAUSE_KIND_CONFORMANCE SUCM_CLAUSE_CONFORMANCE
#define SUCM_CLAUSE_KIND_REQUIREMENT SUCM_CLAUSE_REQUIREMENT

/* One clause of the registry. */
typedef struct {
    const char       *id;        /* "SUCM-C1"                              */
    sucm_clause_kind_t kind;
    uint32_t          number;    /* the n of SUCM-<K>n                     */
    uint8_t           withdrawn; /* 1 when the clause has been withdrawn  */
    const char       *title;     /* a short title                          */
} sucm_clause_t;

/* A set of clauses, one bit per registry entry (§5.2). */
typedef struct {
    uint32_t bits;
} sucm_clause_set_t;

/* ── The claim identifier of §2.3 ─────────────────────────────────── */

typedef struct {
    sucm_tier_t tier;
    char        unit[SUCM_MAX_UNIT];
    char        version[SUCM_MAX_VERSION];
    uint8_t     valid;
} sucm_claim_id_t;

/* ── The gate records of §7 ───────────────────────────────────────── */

typedef enum {
    SUCM_GATE_TEST  = 0,   /* the kernel test harness (§7.1)          */
    SUCM_GATE_DOCS  = 1,   /* the documentation gate (§7.2)            */
    SUCM_GATE_BUILD = 2,   /* the build gate (§7.3)                   */
    SUCM_GATE_COUNT = 3
} sucm_gate_t;

/* The result of a gate. */
typedef enum {
    SUCM_RESULT_UNKNOWN = 0,
    SUCM_RESULT_GREEN   = 1,
    SUCM_RESULT_RED     = 2
} sucm_result_t;

/*
 * A gate record (§2.1 D7): the harness that ran, its version, the suites or
 * documents it exercised, and the result. The record is caller-owned; the
 * suite list is a fixed-capacity array of fixed-size names.
 */
typedef struct {
    sucm_gate_t gate;
    char        harness[SUCM_MAX_HARNESS];
    char        version[SUCM_MAX_VERSION];
    char        suites[SUCM_MAX_SUITES][SUCM_MAX_SUITE];
    uint32_t    suite_count;
    sucm_result_t result;
} sucm_gate_record_t;

/* ── The claim row of §8 ──────────────────────────────────────────── */

/*
 * One row of SUCMClaims.txt: `tier;unit;version;clauses;gate-record`.
 * The row is the unit of the build gate (§7.3); a claim absent from the
 * registry does not exist for the purposes of image compilation.
 */
typedef struct {
    sucm_tier_t tier;
    char        unit[SUCM_MAX_UNIT];
    char        version[SUCM_MAX_VERSION];
    char        clauses[SUCM_MAX_CLAUSE_LIST];
    char        gate[SUCM_MAX_GATE];
} sucm_claim_t;

/* ── The core engine registry of §8 ───────────────────────────────── */

typedef struct {
    const char *module;        /* the engine's module name              */
    const char *unit;          /* the unit component of its claim id    */
    const char *spec;          /* the owning specification              */
    const char *memory_class;  /* the memory class of §4.2              */
    const char *time_class;    /* the time class of §4.2                */
    sucm_tier_t tier;          /* the tier the engine is claimed at     */
} sucm_engine_t;

/* ── The published identifiers and the registry API ───────────────── */

const char *sutr_sucm_version_string(void);

/* The tiers of §3. */
const char *sutr_sucm_tier_name(sucm_tier_t tier);
sucm_tier_t sutr_sucm_tier_from_name(const char *name);
bool        sutr_sucm_tier_includes(sucm_tier_t higher, sucm_tier_t lower);
bool        sutr_sucm_tier_is_valid(sucm_tier_t tier);

/* The clause registry of §5.2. */
uint32_t          sutr_sucm_clause_count(void);
bool              sutr_sucm_clause_at(uint32_t index, sucm_clause_t *out);
bool              sutr_sucm_clause_by_id(const char *id, sucm_clause_t *out);
const char       *sutr_sucm_clause_kind_name(sucm_clause_kind_t kind);
sucm_clause_kind_t sutr_sucm_clause_kind_of(const char *id);
uint32_t          sutr_sucm_clause_number_of(const char *id);
bool              sutr_sucm_clause_is_withdrawn(const char *id);

/* The clause sets of §5.1 and §8. */
sucm_clause_set_t sutr_sucm_tier_clause_set(sucm_tier_t tier);
bool              sutr_sucm_clause_set_has(sucm_clause_set_t set,
                                           const char *id);
bool              sutr_sucm_clause_set_contains(sucm_clause_set_t set,
                                                sucm_clause_set_t other);
bool              sutr_sucm_clause_list_parse(const char *list,
                                               sucm_clause_set_t *out);
uint32_t          sutr_sucm_clause_set_count(sucm_clause_set_t set);

/* The claim identifier of §2.3 and the naming rules of §5.4. */
bool   sutr_sucm_claim_id_parse(const char *text, sucm_claim_id_t *out);
bool   sutr_sucm_claim_id_build(sucm_tier_t tier, const char *unit,
                                const char *version, char *out, size_t capacity);
bool   sutr_sucm_claim_id_is_valid(const char *text);
bool   sutr_sucm_claim_id_normalize(const char *text, char *out, size_t capacity);
bool   sutr_sucm_claim_id_equal(const char *left, const char *right);
bool   sutr_sucm_claim_id_unit_is_valid(const char *unit);
bool   sutr_sucm_claim_id_version_is_valid(const char *version);

/* The gate records of §7. */
const char   *sutr_sucm_gate_name(sucm_gate_t gate);
sucm_gate_t  sutr_sucm_gate_from_name(const char *name);
const char   *sutr_sucm_result_name(sucm_result_t result);
sucm_result_t sutr_sucm_result_from_name(const char *name);
bool          sutr_sucm_gate_record_parse(const char *text,
                                          sucm_gate_record_t *out);
bool          sutr_sucm_gate_record_is_valid(const sucm_gate_record_t *record);
bool          sutr_sucm_gate_record_passes(const sucm_gate_record_t *record);
bool          sutr_sucm_gate_record_add_suite(sucm_gate_record_t *record,
                                              const char *suite);
uint32_t      sutr_sucm_gate_record_suite_count(
                  const sucm_gate_record_t *record);
const char   *sutr_sucm_gate_record_suite_at(const sucm_gate_record_t *record,
                                             uint32_t index);

/* The claim rows of §8. */
uint32_t      sutr_sucm_claim_count(void);
bool          sutr_sucm_claim_at(uint32_t index, sucm_claim_t *out);
bool          sutr_sucm_claim_find(sucm_tier_t tier, const char *unit,
                                   const char *version, sucm_claim_t *out);
bool          sutr_sucm_claim_row_parse(const char *row, sucm_claim_t *out);
bool          sutr_sucm_claim_row_format(const sucm_claim_t *claim, char *out,
                                          size_t capacity);
bool          sutr_sucm_claim_is_valid(const sucm_claim_t *claim);

/* The core engine registry of §8. */
uint32_t      sutr_sucm_engine_count(void);
bool          sutr_sucm_engine_at(uint32_t index, sucm_engine_t *out);
bool          sutr_sucm_engine_by_module(const char *module,
                                         sucm_engine_t *out);
bool          sutr_sucm_engine_by_unit(const char *unit, sucm_engine_t *out);

/* The resource gates of §3.2, §3.3, and §6. */
bool sutr_sucm_bridge_gate_allows(uint64_t codepoint);
bool sutr_sucm_native_gate_allows(uint64_t codepoint);
bool sutr_sucm_neography_gate_allows(uint64_t codepoint);
bool sutr_sucm_is_trap(uint64_t codepoint);
bool sutr_sucm_is_sentinel(uint64_t codepoint);

#ifdef __cplusplus
}
#endif

#endif /* SUTR_SUTR_SUCM_H */
