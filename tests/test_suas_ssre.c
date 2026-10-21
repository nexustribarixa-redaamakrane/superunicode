/**
 * SUAS-010 SSRE — System Specification Registry & Cross-Reference Engine
 *
 * Unit tests for the meta-standard: the embedded cross-reference registry and
 * the deterministic linkage protocol that governs how every SUAS, SUTS, and
 * SUTR document declares and satisfies its architectural dependencies.
 *
 * Covers: version/meta, the compile-time dependency gate (both the preprocessor
 * and the assertion-idiom forms), registry self-consistency and uniqueness,
 * O(1) lookup and miss behaviour, version and kind queries, the module
 * cross-reference in both directions, whole-set validation with first-failure
 * diagnostics, and the freestanding hash and identifier-equality primitives.
 */

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <assert.h>

#include "suas/suas_ssre.h"

static int g_fail = 0;
#define CHECK(cond) do { if (!(cond)) { \
    printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); g_fail = 1; \
} } while (0)

/* ── The compile-time gate, exercised in the preprocessor itself ──── */
/* These two directives are the whole point of §7: a schema misalignment has
 * to be a build failure, so the gate has to be provably usable inside #if. */
#if !SUAS_SSRE_VALIDATE_DEP(1, 0, 1, 0)
#error "SSRE gate should accept a provider of 1.0 against a floor of 1.0"
#endif
#if SUAS_SSRE_VALIDATE_DEP(2, 0, 1, 0)
#error "SSRE gate should reject a provider of 1.0 against a floor of 2.0"
#endif

/* The assertion-idiom form, instantiated once so a failure would be a
 * negative-array-bound diagnostic naming the dependency. */
SUAS_SSRE_REQUIRE_DEP(g_ssre_dep_gate_satisfied, 1, 0, 1, 0);

static void test_version_and_meta(void)
{
    const char* vs = suas_ssre_version_string();

    CHECK(vs != NULL);
    CHECK(vs[0] == 'S');
    CHECK(SUAS_SSRE_SPEC_ID[0] == 'S');
    CHECK(SUAS_SSRE_SPEC_TITLE[0] == 'S');
    CHECK(SUAS_SSRE_VERSION_MAJOR == 1);
    CHECK(SUAS_SSRE_VERSION_MINOR == 0);

    CHECK(SUAS_SSRE_REGISTRY_CAPACITY == 64U);
    CHECK(SUAS_SSRE_INDEX_SIZE == 256U);
    CHECK(SUAS_SSRE_MAX_SPEC_ID == 16U);
    CHECK(SUAS_SSRE_TIER_L0 == 0U);
    CHECK(SUAS_SSRE_TIER_LEVELS == 3U);

    /* The descriptor keeps the four-field shape the standard mandates. */
    CHECK(sizeof(ssre_dependency_descriptor_t) == 16U);
    CHECK(offsetof(ssre_dependency_descriptor_t, spec_id) == 0U);
    CHECK(offsetof(ssre_dependency_descriptor_t, min_version_major) == 8U);
    CHECK(offsetof(ssre_dependency_descriptor_t, min_version_minor) == 9U);
    CHECK(offsetof(ssre_dependency_descriptor_t, is_mandatory) == 10U);

    /* The registry record is three read-only pointers plus four octets (§2.3). */
    CHECK(sizeof(ssre_spec_record_t) == 32U);
    CHECK(offsetof(ssre_spec_record_t, spec_id) == 0U);
    CHECK(offsetof(ssre_spec_record_t, short_name) == 8U);
    CHECK(offsetof(ssre_spec_record_t, module) == 16U);
    CHECK(offsetof(ssre_spec_record_t, major) == 24U);
    CHECK(offsetof(ssre_spec_record_t, tier) == 27U);
}

/* "SUAS-010/1.0.0" */
static void test_version_string_shape(void)
{
    const char* vs = suas_ssre_version_string();

    CHECK(vs[8] == '/');
    CHECK(vs[9] == '1');
    CHECK(vs[10] == '.');
    CHECK(vs[11] == '0');
    CHECK(vs[12] == '.');
    CHECK(vs[13] == '0');
    CHECK(vs[14] == '\0');
}

/* A constant fold of the gate, so the ordering is pinned as an expression and
 * not only as a preprocessor directive. */
static void test_compile_time_gate(void)
{
    CHECK(SUAS_SSRE_VALIDATE_DEP(1, 0, 1, 0) == 1);  /* equal versions      */
    CHECK(SUAS_SSRE_VALIDATE_DEP(1, 0, 1, 7) == 1);  /* newer minor         */
    CHECK(SUAS_SSRE_VALIDATE_DEP(1, 3, 2, 0) == 1);  /* newer major         */
    CHECK(SUAS_SSRE_VALIDATE_DEP(1, 4, 1, 3) == 0);  /* older minor         */
    CHECK(SUAS_SSRE_VALIDATE_DEP(2, 0, 1, 9) == 0);  /* older major         */
    CHECK(!SUAS_SSRE_VALIDATE_DEP(0, 1, 0, 0));      /* SUTR 0.1 vs 0.0     */
    CHECK(SUAS_SSRE_VALIDATE_DEP(0, 1, 0, 1) == 1);
}

/* The registry is the single source of truth, so it has to be internally
 * consistent before anything may be looked up in it. */
static void test_registry_self_consistency(void)
{
    size_t n = suas_ssre_spec_count();
    size_t i;
    size_t mandatory = 0u, optional = 0u, reference = 0u;

    /* 13 SUAS + 5 SUTS + 11 SUTR as of SUAS-013's registration. */
    CHECK(n == 29U);
    CHECK(n <= SUAS_SSRE_REGISTRY_CAPACITY);

    for (i = 0u; i < n; i++) {
        const ssre_spec_record_t* rec = suas_ssre_spec_at(i);
        size_t j;

        CHECK(rec != NULL);
        if (rec == NULL)
            continue;
        CHECK(rec->spec_id != NULL && rec->spec_id[0] != '\0');
        CHECK(rec->short_name != NULL && rec->short_name[0] != '\0');
        CHECK(rec->module != NULL);
        CHECK(rec->kind <= (uint8_t)SUAS_SSRE_KIND_REFERENCE);
        CHECK(rec->tier < SUAS_SSRE_TIER_LEVELS);

        /* Identifiers must fit the budget the lookup rejects on. */
        {
            unsigned len = 0u;
            while (len < SUAS_SSRE_MAX_SPEC_ID && rec->spec_id[len] != '\0')
                len++;
            CHECK(len < SUAS_SSRE_MAX_SPEC_ID);
        }

        /* Every record must be reachable through its own index. */
        CHECK(suas_ssre_lookup(rec->spec_id) == rec);
        CHECK(suas_ssre_is_registered(rec->spec_id));

        /* And no identifier may appear twice. */
        for (j = 0u; j < i; j++)
            CHECK(!suas_ssre_id_equal(rec->spec_id,
                                      suas_ssre_spec_at(j)->spec_id));

        if (rec->kind == (uint8_t)SUAS_SSRE_KIND_MANDATORY) mandatory++;
        else if (rec->kind == (uint8_t)SUAS_SSRE_KIND_OPTIONAL) optional++;
        else reference++;
    }

    CHECK(mandatory == 13u);
    CHECK(optional == 5u);
    CHECK(reference == 11u);
    CHECK(suas_ssre_kind_count((uint8_t)SUAS_SSRE_KIND_MANDATORY) == mandatory);
    CHECK(suas_ssre_kind_count((uint8_t)SUAS_SSRE_KIND_OPTIONAL) == optional);
    CHECK(suas_ssre_kind_count((uint8_t)SUAS_SSRE_KIND_REFERENCE) == reference);
    CHECK(mandatory + optional + reference == n);
}

/* SUAS is mandatory, SUTS is optional, SUTR is reference — and the boundary
 * between the three families is exactly what a dependency may not blur. */
static void test_family_classification(void)
{
    CHECK(suas_ssre_is_mandatory("SUAS-009"));
    CHECK(suas_ssre_is_mandatory("SUAS-010"));
    CHECK(suas_ssre_is_registered("SUTS-005"));
    CHECK(!suas_ssre_is_mandatory("SUTS-005"));
    CHECK(!suas_ssre_is_mandatory("SUTR-10"));

    CHECK(suas_ssre_tier_of("SUAS-009") == SUAS_SSRE_TIER_L0);
    CHECK(suas_ssre_tier_of("SUTS-003") == SUAS_SSRE_TIER_L0);
    /* A miss reports an out-of-range tier, never a plausible one. */
    CHECK(suas_ssre_tier_of("SUAS-014") == SUAS_SSRE_TIER_LEVELS);
    CHECK(suas_ssre_tier_of(NULL) == SUAS_SSRE_TIER_LEVELS);

    CHECK(suas_ssre_module_of("SUAS-009") != NULL);
    CHECK(suas_ssre_module_of("SUTR-10") != NULL);
    CHECK(suas_ssre_module_of("SUTR-10")[0] == '\0'); /* a report, no unit */
}

static void test_lookup_and_miss(void)
{
    uint8_t major = 0u, minor = 0u;

    CHECK(suas_ssre_lookup("SUAS-010") != NULL);
    CHECK(suas_ssre_lookup("SUTS-003") != NULL);
    CHECK(suas_ssre_lookup("SUTR-0") != NULL);

    /* SUAS-014 is past the end of the contiguous series; an identifier
     * outside the corpus is not a registered specification. */
    CHECK(suas_ssre_lookup("SUAS-014") == NULL);
    CHECK(!suas_ssre_is_registered("SUAS-014"));
    CHECK(suas_ssre_lookup("SUTS-999") == NULL);
    CHECK(suas_ssre_lookup("suas-009") == NULL); /* identifiers are exact */
    CHECK(suas_ssre_lookup("") == NULL);
    CHECK(suas_ssre_lookup(NULL) == NULL);
    /* Overlong identifiers are not a registered shape and are rejected
     * before any probing occurs. */
    CHECK(suas_ssre_lookup("SUAS-00123456789012") == NULL);

    CHECK(suas_ssre_version_of("SUTS-004", &major, &minor));
    CHECK(major == 1u && minor == 0u);
    CHECK(suas_ssre_version_of("SUTR-8", &major, &minor));
    CHECK(major == 0u && minor == 1u);
    CHECK(!suas_ssre_version_of("SUAS-014", &major, &minor));
    CHECK(suas_ssre_version_of("SUAS-009", NULL, NULL));
}

static void test_satisfies(void)
{
    CHECK(suas_ssre_satisfies("SUTS-003", 1u, 0u));
    CHECK(suas_ssre_satisfies("SUTS-003", 0u, 9u));
    CHECK(!suas_ssre_satisfies("SUTS-003", 1u, 1u));
    CHECK(!suas_ssre_satisfies("SUTS-003", 2u, 0u));
    CHECK(suas_ssre_satisfies("SUTR-10", 0u, 1u));
    CHECK(!suas_ssre_satisfies("SUTR-10", 0u, 2u));
    CHECK(!suas_ssre_satisfies("SUAS-014", 0u, 0u));
    CHECK(!suas_ssre_satisfies(NULL, 0u, 0u));
}

/* The module cross-reference, which is what ties the spec tree to the code. */
static void test_module_cross_reference(void)
{
    CHECK(suas_ssre_module_of("SUTS-003") != NULL);
    CHECK(suas_ssre_module_of("SUTS-003")[0] == 's');
    CHECK(suas_ssre_spec_of_module("suts_sldf") != NULL);
    CHECK(suas_ssre_spec_of_module("suas_ssre") != NULL);
    CHECK(suas_ssre_spec_of_module("suts_scss") != NULL);
    CHECK(suas_ssre_spec_of_module("no_such_unit") == NULL);
    CHECK(suas_ssre_spec_of_module("") == NULL);
    CHECK(suas_ssre_spec_of_module(NULL) == NULL);

    /* The two directions must agree for every registered reference unit. */
    {
        size_t n = suas_ssre_spec_count();
        size_t i;

        for (i = 0u; i < n; i++) {
            const ssre_spec_record_t* rec = suas_ssre_spec_at(i);
            if (rec->module[0] == '\0')
                continue;
            CHECK(suas_ssre_spec_of_module(rec->module) != NULL);
            CHECK(suas_ssre_id_equal(suas_ssre_spec_of_module(rec->module),
                                     rec->spec_id));
        }
    }
}

/* The whole linkage protocol: a module declares what it needs, and SSRE says
 * whether the image actually provides it. */
static void test_verify_dependency(void)
{
    ssre_dependency_descriptor_t dep;

    /* A satisfied optional data-spec dependency. */
    dep.spec_id = "SUTS-003";
    dep.min_version_major = 1u;
    dep.min_version_minor = 0u;
    dep.is_mandatory = false;
    CHECK(suas_ssre_verify_dependency(&dep));

    /* The same dependency declared architecturally mandatory: rejected,
     * because a SUTS data specification is not a SUAS invariant. */
    dep.is_mandatory = true;
    CHECK(!suas_ssre_verify_dependency(&dep));

    /* A SUAS invariant, on the other hand, may be mandatory. */
    dep.spec_id = "SUAS-005";
    dep.is_mandatory = true;
    CHECK(suas_ssre_verify_dependency(&dep));

    /* A reference report may never be mandatory. */
    dep.spec_id = "SUTR-10";
    dep.min_version_major = 0u;
    dep.min_version_minor = 1u;
    CHECK(!suas_ssre_verify_dependency(&dep));
    dep.is_mandatory = false;
    CHECK(suas_ssre_verify_dependency(&dep));

    /* Version floors. */
    dep.spec_id = "SUTR-10";
    dep.min_version_minor = 2u;
    CHECK(!suas_ssre_verify_dependency(&dep));

    /* An unregistered specification is never satisfied, at any version. */
    dep.spec_id = "SUAS-014";
    dep.min_version_major = 0u;
    dep.min_version_minor = 0u;
    CHECK(!suas_ssre_verify_dependency(&dep));

    CHECK(!suas_ssre_verify_dependency(NULL));
    dep.spec_id = NULL;
    CHECK(!suas_ssre_verify_dependency(&dep));
}

static void test_verify_set(void)
{
    ssre_dependency_descriptor_t set[4];

    set[0].spec_id = "SUAS-009";
    set[0].min_version_major = 1u;
    set[0].min_version_minor = 0u;
    set[0].is_mandatory = true;

    set[1].spec_id = "SUTS-004";
    set[1].min_version_major = 1u;
    set[1].min_version_minor = 0u;
    set[1].is_mandatory = false;

    set[2].spec_id = "SUTR-0";
    set[2].min_version_major = 0u;
    set[2].min_version_minor = 1u;
    set[2].is_mandatory = false;

    set[3].spec_id = "SUAS-009";
    set[3].min_version_major = 1u;
    set[3].min_version_minor = 9u; /* SLDF-style over-strict floor */
    set[3].is_mandatory = false;

    CHECK(suas_ssre_verify_set(set, 3u));
    CHECK(suas_ssre_first_missing(set, 3u) == NULL);
    CHECK(suas_ssre_dependency_count(set, 3u) == 0u);

    /* The fourth descriptor is the single failure, and it is reported at its
     * own position so a build gate can name it. */
    CHECK(!suas_ssre_verify_set(set, 4u));
    CHECK(suas_ssre_first_missing(set, 4u) == &set[3]);
    CHECK(suas_ssre_dependency_count(set, 4u) == 1u);

    /* Vacuous success on an empty set; a NULL set is not a set. */
    CHECK(suas_ssre_verify_set(NULL, 0u));
    CHECK(suas_ssre_first_missing(NULL, 0u) == NULL);
    CHECK(suas_ssre_dependency_count(NULL, 0u) == 0u);
    CHECK(!suas_ssre_verify_set(NULL, 4u));
    CHECK(suas_ssre_dependency_count(NULL, 4u) == 4u);
}

static void test_enumeration_bounds(void)
{
    size_t n = suas_ssre_spec_count();

    CHECK(suas_ssre_spec_at(0u) != NULL);
    CHECK(suas_ssre_spec_at(n - 1u) != NULL);
    CHECK(suas_ssre_spec_at(n) == NULL);
    CHECK(suas_ssre_spec_at((size_t)-1) == NULL);

    CHECK(suas_ssre_kind_count((uint8_t)SUAS_SSRE_KIND_MANDATORY) > 0u);
    CHECK(suas_ssre_kind_count(99u) == 0u);

    CHECK(suas_ssre_kind_name((uint8_t)SUAS_SSRE_KIND_MANDATORY) != NULL);
    CHECK(suas_ssre_kind_name((uint8_t)SUAS_SSRE_KIND_OPTIONAL)[0] == 'O');
    CHECK(suas_ssre_kind_name((uint8_t)SUAS_SSRE_KIND_REFERENCE)[0] == 'R');
    CHECK(suas_ssre_kind_name(99u)[0] == 'U');
}

static void test_hash_and_identity(void)
{
    uint32_t a = suas_ssre_hash("SUAS-009");
    uint32_t b = suas_ssre_hash("SUAS-009");
    uint32_t c = suas_ssre_hash("SUAS-007");

    CHECK(a == b);          /* deterministic                                */
    CHECK(a != c);          /* distinct identifiers hash distinctly          */
    CHECK(a != 0u);
    /* NULL folds to the FNV offset basis, not to an arbitrary value. */
    CHECK(suas_ssre_hash(NULL) == 2166136261u);

    CHECK(suas_ssre_id_equal("SUAS-009", "SUAS-009"));
    CHECK(!suas_ssre_id_equal("SUAS-009", "SUAS-007"));
    CHECK(!suas_ssre_id_equal("SUAS-0", "SUAS-009"));
    CHECK(suas_ssre_id_equal("", ""));
    CHECK(!suas_ssre_id_equal("", "SUAS-009"));
    CHECK(suas_ssre_id_equal(NULL, NULL));
    CHECK(!suas_ssre_id_equal(NULL, "SUAS-009"));
    CHECK(!suas_ssre_id_equal("SUAS-009", NULL));
}

/* Re-initialisation must not corrupt or duplicate the index. */
static void test_idempotent_init(void)
{
    const ssre_spec_record_t* before = suas_ssre_lookup("SUTS-005");
    size_t n = suas_ssre_spec_count();
    size_t i;

    suas_ssre_init_registry();
    suas_ssre_init_registry();

    CHECK(suas_ssre_lookup("SUTS-005") == before);
    CHECK(suas_ssre_spec_count() == n);
    for (i = 0u; i < n; i++)
        CHECK(suas_ssre_lookup(suas_ssre_spec_at(i)->spec_id) ==
              suas_ssre_spec_at(i));
}

int main(void)
{
    test_version_and_meta();
    test_version_string_shape();
    test_compile_time_gate();
    test_registry_self_consistency();
    test_family_classification();
    test_lookup_and_miss();
    test_satisfies();
    test_module_cross_reference();
    test_verify_dependency();
    test_verify_set();
    test_enumeration_bounds();
    test_hash_and_identity();
    test_idempotent_init();

    if (g_fail) {
        printf("SUAS-010 SSRE tests FAILED\n");
        return 1;
    }
    printf("SUAS-010 SSRE tests passed\n");
    return 0;
}
