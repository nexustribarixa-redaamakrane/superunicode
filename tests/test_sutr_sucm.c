/*
 * SUTR-10 SUCM - SuperUnicode Conformance Model
 *
 * Unit tests for the compliance surface of SUTR-10: the cumulative tiers
 * of §3, the clause registry of §5.2, the clause sets of the tiers, the
 * claim identifier grammar of §2.3, the naming rules R1-R4 of §5.4, the
 * gate records of §7, the claim rows and the published claim set of §8,
 * the core engine registry of §8, and the resource gates of §3.2, §3.3
 * and §6.
 */

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>

#include "sutr/sutr_sucm.h"

static int g_fail = 0;
#define CHECK(cond) do { if (!(cond)) { \
    printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); g_fail = 1; \
} } while (0)

/* A local string comparison: the host test does not reach for libc. */
static bool same_text(const char *left, const char *right)
{
    size_t i;

    if (left == NULL || right == NULL) {
        return false;
    }
    for (i = 0u;; i++) {
        if (left[i] != right[i]) {
            return false;
        }
        if (left[i] == '\0') {
            return true;
        }
    }
}

/* The claim row the report itself publishes in §8. */
#define DOC_CLAIM_ROW \
    "L0;sis;0.1.0;SUCM-C1,SUCM-C2,SUCM-C3,R5-R8,R13-R14;ctest:green"

/* ── Version, limits, and identifiers ─────────────────────────────── */

static void test_version_and_meta(void)
{
    CHECK(sutr_sucm_version_string() != NULL);
    CHECK(sutr_sucm_version_string()[0] == '0');
    CHECK(sutr_sucm_version_string()[2] == '1');

    CHECK(SUCM_SPEC_ID[0] == 'S');
    CHECK(SUCM_SPEC_TITLE[0] == 'S');
    CHECK(SUCM_MODULE[0] == 's');
    CHECK(SUCM_VERSION_MAJOR == 0);
    CHECK(SUCM_VERSION_MINOR == 1);
    CHECK(SUCM_VERSION_PATCH == 0);

    /* The resource ceilings of §4.2 and the plane boundaries of §3. */
    CHECK(SUCM_HEAP_CEILING_BYTES == 0u);
    CHECK(SUCM_BRIDGE_MAX == 0x00010FFFFULL);
    CHECK(SUCM_NATIVE_MIN == 0x00120000ULL);
    CHECK(SUCM_NATIVE_MAX == 0x7FFFFFEFULL);
    CHECK(SUCM_NEOGRAPHY_MIN == 0x00160000ULL);
    CHECK(SUCM_NEOGRAPHY_MAX == 0x0016FFFFULL);
    CHECK(SUCM_TRAP_SLOT_LOW == 0x7FFFFFF0ULL);
    CHECK(SUCM_TRAP_SLOT_HIGH == 0x7FFFFFFEULL);
    CHECK(SUCM_SENTINEL == 0x7FFFFFFFULL);

    /* The registry sizes. */
    CHECK(SUCM_CLAUSE_COUNT == 29u);
    CHECK(SUCM_ENGINE_COUNT == 34u);
    CHECK(SUCM_CLAIM_COUNT == 34u);
}

/* ── The tiers of §3 ─────────────────────────────────────────────── */

static void test_tiers(void)
{
    CHECK(sutr_sucm_tier_name(SUCM_TIER_L0)[0] == 'L');
    CHECK(sutr_sucm_tier_name(SUCM_TIER_L0)[1] == '0');
    CHECK(sutr_sucm_tier_name(SUCM_TIER_L1)[1] == '1');
    CHECK(sutr_sucm_tier_name(SUCM_TIER_L2)[1] == '2');
    CHECK(sutr_sucm_tier_name(SUCM_TIER_COUNT)[0] == 'u');

    CHECK(sutr_sucm_tier_from_name("L0") == SUCM_TIER_L0);
    CHECK(sutr_sucm_tier_from_name("L1") == SUCM_TIER_L1);
    CHECK(sutr_sucm_tier_from_name("L2") == SUCM_TIER_L2);
    CHECK(sutr_sucm_tier_from_name("L3") == SUCM_TIER_COUNT);
    CHECK(sutr_sucm_tier_from_name("") == SUCM_TIER_COUNT);
    CHECK(sutr_sucm_tier_from_name(NULL) == SUCM_TIER_COUNT);

    CHECK(sutr_sucm_tier_is_valid(SUCM_TIER_L0) == true);
    CHECK(sutr_sucm_tier_is_valid(SUCM_TIER_L2) == true);
    CHECK(sutr_sucm_tier_is_valid(SUCM_TIER_COUNT) == false);

    /* The tier inclusion of §3.4: L2 ⊃ L1 ⊃ L0. */
    CHECK(sutr_sucm_tier_includes(SUCM_TIER_L0, SUCM_TIER_L0) == true);
    CHECK(sutr_sucm_tier_includes(SUCM_TIER_L1, SUCM_TIER_L0) == true);
    CHECK(sutr_sucm_tier_includes(SUCM_TIER_L2, SUCM_TIER_L0) == true);
    CHECK(sutr_sucm_tier_includes(SUCM_TIER_L2, SUCM_TIER_L1) == true);
    CHECK(sutr_sucm_tier_includes(SUCM_TIER_L0, SUCM_TIER_L1) == false);
    CHECK(sutr_sucm_tier_includes(SUCM_TIER_L1, SUCM_TIER_L2) == false);
    CHECK(sutr_sucm_tier_includes(SUCM_TIER_COUNT, SUCM_TIER_L0) == false);
}

/* ── The clause registry of §5.2 ─────────────────────────────────── */

static void test_clause_registry(void)
{
    sucm_clause_t clause;
    uint32_t i;
    uint32_t definitions = 0u;
    uint32_t conformances = 0u;
    uint32_t requirements = 0u;

    CHECK(sutr_sucm_clause_count() == 29u);

    /* Every clause of §5 and §6 is registered, with a stable identifier,
     * a kind, and a number. */
    CHECK(sutr_sucm_clause_by_id("SUCM-D1", &clause) == true);
    CHECK(clause.kind == SUCM_CLAUSE_DEFINITION);
    CHECK(clause.number == 1u);
    CHECK(clause.withdrawn == 0u);
    CHECK(clause.title[0] != '\0');

    CHECK(sutr_sucm_clause_by_id("SUCM-C6", &clause) == true);
    CHECK(clause.kind == SUCM_CLAUSE_CONFORMANCE);
    CHECK(clause.number == 6u);

    CHECK(sutr_sucm_clause_by_id("SUCM-R15", &clause) == true);
    CHECK(clause.kind == SUCM_CLAUSE_REQUIREMENT);
    CHECK(clause.number == 15u);

    /* An unknown identifier resolves to nothing. */
    CHECK(sutr_sucm_clause_by_id("SUCM-C7", &clause) == false);
    CHECK(sutr_sucm_clause_by_id("SUCM-R16", &clause) == false);
    CHECK(sutr_sucm_clause_by_id("SUCM-X1", &clause) == false);
    CHECK(sutr_sucm_clause_by_id("", &clause) == false);
    CHECK(sutr_sucm_clause_by_id(NULL, &clause) == false);

    CHECK(sutr_sucm_clause_kind_of("SUCM-D3") == SUCM_CLAUSE_DEFINITION);
    CHECK(sutr_sucm_clause_kind_of("SUCM-C2") == SUCM_CLAUSE_CONFORMANCE);
    CHECK(sutr_sucm_clause_kind_of("SUCM-R8") == SUCM_CLAUSE_REQUIREMENT);
    CHECK(sutr_sucm_clause_kind_of("SUCM-Z9") == SUCM_CLAUSE_KIND_COUNT);

    CHECK(sutr_sucm_clause_number_of("SUCM-R15") == 15u);
    CHECK(sutr_sucm_clause_number_of("SUCM-R16") == 0u);

    CHECK(sutr_sucm_clause_is_withdrawn("SUCM-C1") == false);
    CHECK(sutr_sucm_clause_is_withdrawn("SUCM-Z1") == false);

    CHECK(sutr_sucm_clause_kind_name(SUCM_CLAUSE_DEFINITION)[0] == 'd');
    CHECK(sutr_sucm_clause_kind_name(SUCM_CLAUSE_CONFORMANCE)[0] == 'c');
    CHECK(sutr_sucm_clause_kind_name(SUCM_CLAUSE_REQUIREMENT)[0] == 'r');
    CHECK(sutr_sucm_clause_kind_name(SUCM_CLAUSE_KIND_COUNT)[0] == 'u');

    /* The registry holds every clause exactly once. */
    for (i = 0u; i < sutr_sucm_clause_count(); i++) {
        CHECK(sutr_sucm_clause_at(i, &clause) == true);
        if (clause.kind == SUCM_CLAUSE_DEFINITION) {
            definitions++;
        } else if (clause.kind == SUCM_CLAUSE_CONFORMANCE) {
            conformances++;
        } else {
            requirements++;
        }
    }
    CHECK(definitions == 8u);
    CHECK(conformances == 6u);
    CHECK(requirements == 15u);

    /* The accessor is bounded. */
    CHECK(sutr_sucm_clause_at(sutr_sucm_clause_count(), &clause) == false);
    CHECK(sutr_sucm_clause_at(0xFFFFFFFFu, &clause) == false);
    CHECK(sutr_sucm_clause_at(0u, NULL) == false);
}

/* ── The clause sets of the tiers ────────────────────────────────── */

static void test_clause_sets(void)
{
    sucm_clause_set_t l0 = sutr_sucm_tier_clause_set(SUCM_TIER_L0);
    sucm_clause_set_t l1 = sutr_sucm_tier_clause_set(SUCM_TIER_L1);
    sucm_clause_set_t l2 = sutr_sucm_tier_clause_set(SUCM_TIER_L2);

    /* The tier sets are cumulative (§3.4). */
    CHECK(sutr_sucm_clause_set_contains(l1, l0) == true);
    CHECK(sutr_sucm_clause_set_contains(l2, l0) == true);
    CHECK(sutr_sucm_clause_set_contains(l2, l1) == true);
    CHECK(sutr_sucm_clause_set_contains(l0, l1) == false);

    /* The clause columns of the report's own example. */
    CHECK(sutr_sucm_clause_set_has(l0, "SUCM-C1") == true);
    CHECK(sutr_sucm_clause_set_has(l0, "SUCM-C3") == true);
    CHECK(sutr_sucm_clause_set_has(l0, "SUCM-C4") == false);
    CHECK(sutr_sucm_clause_set_has(l0, "SUCM-R5") == true);
    CHECK(sutr_sucm_clause_set_has(l0, "SUCM-R9") == false);
    CHECK(sutr_sucm_clause_set_has(l0, "SUCM-R13") == true);
    CHECK(sutr_sucm_clause_set_has(l0, "SUCM-R15") == false);

    CHECK(sutr_sucm_clause_set_has(l1, "SUCM-C4") == true);
    CHECK(sutr_sucm_clause_set_has(l1, "SUCM-R9") == true);
    CHECK(sutr_sucm_clause_set_has(l1, "SUCM-C5") == false);

    CHECK(sutr_sucm_clause_set_has(l2, "SUCM-C5") == true);
    CHECK(sutr_sucm_clause_set_has(l2, "SUCM-R10") == true);
    CHECK(sutr_sucm_clause_set_has(l2, "SUCM-R11") == true);
    CHECK(sutr_sucm_clause_set_has(l2, "SUCM-R12") == true);

    /* The sizes of the three tier sets. */
    CHECK(sutr_sucm_clause_set_count(l0) == 9u);
    CHECK(sutr_sucm_clause_set_count(l1) == 11u);
    CHECK(sutr_sucm_clause_set_count(l2) == 15u);

    /* An unknown identifier is never in a set. */
    CHECK(sutr_sucm_clause_set_has(l2, "SUCM-R16") == false);
}

/* ── The clause list grammar of §8 ───────────────────────────────── */

static void test_clause_list_parse(void)
{
    sucm_clause_set_t set;

    /* The exact column the report publishes in §8. */
    CHECK(sutr_sucm_clause_list_parse(
              "SUCM-C1,SUCM-C2,SUCM-C3,R5-R8,R13-R14", &set) == true);
    CHECK(sutr_sucm_clause_set_count(set) == 9u);
    CHECK(sutr_sucm_clause_set_has(set, "SUCM-C1") == true);
    CHECK(sutr_sucm_clause_set_has(set, "SUCM-R5") == true);
    CHECK(sutr_sucm_clause_set_has(set, "SUCM-R8") == true);
    CHECK(sutr_sucm_clause_set_has(set, "SUCM-R13") == true);
    CHECK(sutr_sucm_clause_set_has(set, "SUCM-R14") == true);
    CHECK(sutr_sucm_clause_set_has(set, "SUCM-R9") == false);

    /* The level-1 and level-2 columns. */
    CHECK(sutr_sucm_clause_list_parse(
              "SUCM-C1,SUCM-C2,SUCM-C3,SUCM-C4,R5-R8,R9,R13-R14", &set) == true);
    CHECK(sutr_sucm_clause_set_count(set) == 11u);
    CHECK(sutr_sucm_clause_set_contains(set,
              sutr_sucm_tier_clause_set(SUCM_TIER_L1)) == true);

    CHECK(sutr_sucm_clause_list_parse(
              "SUCM-C1,SUCM-C2,SUCM-C3,SUCM-C4,SUCM-C5,R5-R8,R9-R12,R13-R14",
              &set) == true);
    CHECK(sutr_sucm_clause_set_count(set) == 15u);
    CHECK(sutr_sucm_clause_set_contains(set,
              sutr_sucm_tier_clause_set(SUCM_TIER_L2)) == true);

    /* The kind-qualified shorthand and a bare range. */
    CHECK(sutr_sucm_clause_list_parse("C1,C2,C3,R5-R8,R13-R14", &set) == true);
    CHECK(sutr_sucm_clause_set_count(set) == 9u);

    /* A single clause and an empty list. */
    CHECK(sutr_sucm_clause_list_parse("SUCM-C1", &set) == true);
    CHECK(sutr_sucm_clause_set_count(set) == 1u);
    CHECK(sutr_sucm_clause_list_parse("", &set) == true);
    CHECK(sutr_sucm_clause_set_count(set) == 0u);

    /* A malformed list is rejected rather than partially accepted. */
    CHECK(sutr_sucm_clause_list_parse("SUCM-C99", &set) == false);
    CHECK(sutr_sucm_clause_list_parse("R5-R99", &set) == false);
    CHECK(sutr_sucm_clause_list_parse("X1", &set) == false);
    CHECK(sutr_sucm_clause_list_parse("SUCM-", &set) == false);
    CHECK(sutr_sucm_clause_list_parse("C", &set) == false);
    CHECK(sutr_sucm_clause_list_parse("R5-", &set) == false);
    CHECK(sutr_sucm_clause_list_parse("R8-R5", &set) == false);
    CHECK(sutr_sucm_clause_list_parse("C1!", &set) == false);
    CHECK(sutr_sucm_clause_list_parse(NULL, &set) == false);
    CHECK(sutr_sucm_clause_list_parse("C1", NULL) == false);
}

/* ── The claim identifier of §2.3 ────────────────────────────────── */

static void test_claim_id(void)
{
    sucm_claim_id_t id;
    char built[64];

    /* The example of §2.3. */
    CHECK(sutr_sucm_claim_id_parse("L1/sucf/0.1.0", &id) == true);
    CHECK(id.valid != 0u);
    CHECK(id.tier == SUCM_TIER_L1);
    CHECK(id.unit[0] == 's');
    CHECK(id.unit[1] == 'u');
    CHECK(id.unit[2] == 'c');
    CHECK(id.unit[3] == 'f');
    CHECK(id.unit[4] == '\0');
    CHECK(id.version[0] == '0');
    CHECK(id.version[5] == '\0');

    /* The example of §5.4. */
    CHECK(sutr_sucm_claim_id_parse("L0/sis/0.1.0", &id) == true);
    CHECK(id.tier == SUCM_TIER_L0);

    /* A malformed identifier is rejected. */
    CHECK(sutr_sucm_claim_id_parse("L0/sis", &id) == false);
    CHECK(sutr_sucm_claim_id_parse("L0//0.1.0", &id) == false);
    CHECK(sutr_sucm_claim_id_parse("/sis/0.1.0", &id) == false);
    CHECK(sutr_sucm_claim_id_parse("L3/sis/0.1.0", &id) == false);
    CHECK(sutr_sucm_claim_id_parse("L0/sis/", &id) == false);
    CHECK(sutr_sucm_claim_id_parse("", &id) == false);
    CHECK(sutr_sucm_claim_id_parse(NULL, &id) == false);
    CHECK(sutr_sucm_claim_id_parse("L0/sis/0.1.0", NULL) == false);

    CHECK(sutr_sucm_claim_id_is_valid("L0/sis/0.1.0") == true);
    CHECK(sutr_sucm_claim_id_is_valid("L0/sis/0.1.0/extra") == false);

    /* The builder round-trips the parser. */
    CHECK(sutr_sucm_claim_id_build(SUCM_TIER_L0, "sis", "0.1.0", built,
                                   sizeof(built)) == true);
    CHECK(built[0] == 'L');
    CHECK(built[1] == '0');
    CHECK(built[2] == '/');
    CHECK(sutr_sucm_claim_id_is_valid(built) == true);

    /* The builder refuses what the parser would refuse. */
    CHECK(sutr_sucm_claim_id_build(SUCM_TIER_L0, "sis", "0.1.0", built, 4u) ==
          false);
    CHECK(sutr_sucm_claim_id_build(SUCM_TIER_COUNT, "sis", "0.1.0", built,
                                   sizeof(built)) == false);
    CHECK(sutr_sucm_claim_id_build(SUCM_TIER_L0, "3sis", "0.1.0", built,
                                   sizeof(built)) == false);
    CHECK(sutr_sucm_claim_id_build(SUCM_TIER_L0, "sis", "x", built,
                                   sizeof(built)) == false);
    CHECK(sutr_sucm_claim_id_build(SUCM_TIER_L0, "sis", "0.1.0", NULL, 10u) ==
          false);
}

/* ── The naming rules of §5.4 ────────────────────────────────────── */

static void test_naming_rules(void)
{
    /* SUCM-R1: the unit's alphabet and the leading-digit rule. */
    CHECK(sutr_sucm_claim_id_unit_is_valid("sis") == true);
    CHECK(sutr_sucm_claim_id_unit_is_valid("SIS") == true);
    CHECK(sutr_sucm_claim_id_unit_is_valid("sis-3") == true);
    CHECK(sutr_sucm_claim_id_unit_is_valid("system id") == true);
    CHECK(sutr_sucm_claim_id_unit_is_valid("3sis") == false);
    CHECK(sutr_sucm_claim_id_unit_is_valid("sis 3id") == false);
    CHECK(sutr_sucm_claim_id_unit_is_valid("") == false);
    CHECK(sutr_sucm_claim_id_unit_is_valid(NULL) == false);
    CHECK(sutr_sucm_claim_id_unit_is_valid("sis!") == false);

    /* The version is the dotted-numeric form the report exhibits. */
    CHECK(sutr_sucm_claim_id_version_is_valid("0.1.0") == true);
    CHECK(sutr_sucm_claim_id_version_is_valid("1") == true);
    CHECK(sutr_sucm_claim_id_version_is_valid("0.1") == true);
    CHECK(sutr_sucm_claim_id_version_is_valid("") == false);
    CHECK(sutr_sucm_claim_id_version_is_valid("0..1") == false);
    CHECK(sutr_sucm_claim_id_version_is_valid(".1") == false);
    CHECK(sutr_sucm_claim_id_version_is_valid("v0.1") == false);
    CHECK(sutr_sucm_claim_id_version_is_valid(NULL) == false);
}

/* ── The SUCM-R3 equivalence relation ────────────────────────────── */

static void test_claim_id_equivalence(void)
{
    char key[160];

    /* The key of an ordinary identifier is the identifier itself. */
    CHECK(sutr_sucm_claim_id_normalize("L0/sis/0.1.0", key, sizeof(key)) ==
          true);
    CHECK(key[0] == 'L');
    CHECK(key[1] == '0');
    CHECK(key[2] == '/');
    CHECK(key[3] == 's');
    CHECK(key[4] == 'i');
    CHECK(key[5] == 's');

    /* A medial hyphen is ignored. */
    CHECK(sutr_sucm_claim_id_equal("L0/sis/0.1.0", "L0/s-i-s/0.1.0") == true);
    CHECK(sutr_sucm_claim_id_equal("L0/sis/0-1-0", "L0/sis/010") == true);

    /* SPACE is ignored. */
    CHECK(sutr_sucm_claim_id_equal("L0/sis/0.1.0", "L0/sis/0.1.0 ") == true);
    CHECK(sutr_sucm_claim_id_equal("L0/sis/0.1.0", "L0/ sis /0.1.0") == true);

    /* The words LEVEL, TIER, and CORE are ignored. */
    CHECK(sutr_sucm_claim_id_equal("L0/sis/0.1.0", "L0/sis-core/0.1.0") ==
          true);
    CHECK(sutr_sucm_claim_id_equal("L0/sis/0.1.0", "L0/core-sis/0.1.0") ==
          true);
    CHECK(sutr_sucm_claim_id_equal("L0/sis/0.1.0", "L0/sis-level/0.1.0") ==
          true);
    CHECK(sutr_sucm_claim_id_equal("L0/sis/0.1.0", "L0/sis-tier/0.1.0") ==
          true);

    /* A word that merely contains a word is not the word. */
    CHECK(sutr_sucm_claim_id_equal("L0/sis/0.1.0", "L0/score/0.1.0") == false);
    CHECK(sutr_sucm_claim_id_equal("L0/sis/0.1.0", "L0/cores/0.1.0") == false);

    /* A leading or a trailing hyphen is not medial. */
    CHECK(sutr_sucm_claim_id_equal("L0/sis/0.1.0", "L0/-sis/0.1.0") == false);
    CHECK(sutr_sucm_claim_id_equal("L0/sis/0.1.0", "L0/sis-/0.1.0") == false);

    /* A different tier, unit, or version is a different claim. */
    CHECK(sutr_sucm_claim_id_equal("L0/sis/0.1.0", "L1/sis/0.1.0") == false);
    CHECK(sutr_sucm_claim_id_equal("L0/sis/0.1.0", "L0/sip/0.1.0") == false);
    CHECK(sutr_sucm_claim_id_equal("L0/sis/0.1.0", "L0/sis/0.1.1") == false);

    /* The arguments are bounded. */
    CHECK(sutr_sucm_claim_id_normalize(NULL, key, sizeof(key)) == false);
    CHECK(sutr_sucm_claim_id_normalize("L0/sis/0.1.0", NULL, 10u) == false);
}

/* ── The gate records of §7 ──────────────────────────────────────── */

static void test_gate_records(void)
{
    sucm_gate_record_t record;

    CHECK(sutr_sucm_gate_name(SUCM_GATE_TEST)[0] == 'c');
    CHECK(sutr_sucm_gate_name(SUCM_GATE_DOCS)[0] == 'v');
    CHECK(sutr_sucm_gate_name(SUCM_GATE_BUILD)[0] == 'b');
    CHECK(sutr_sucm_gate_name(SUCM_GATE_COUNT)[0] == 'u');

    CHECK(sutr_sucm_gate_from_name("ctest") == SUCM_GATE_TEST);
    CHECK(sutr_sucm_gate_from_name("verify-docs") == SUCM_GATE_DOCS);
    CHECK(sutr_sucm_gate_from_name("build") == SUCM_GATE_BUILD);
    CHECK(sutr_sucm_gate_from_name("nope") == SUCM_GATE_COUNT);

    CHECK(sutr_sucm_result_name(SUCM_RESULT_GREEN)[0] == 'g');
    CHECK(sutr_sucm_result_name(SUCM_RESULT_RED)[0] == 'r');
    CHECK(sutr_sucm_result_name(SUCM_RESULT_UNKNOWN)[0] == 'u');
    CHECK(sutr_sucm_result_from_name("green") == SUCM_RESULT_GREEN);
    CHECK(sutr_sucm_result_from_name("red") == SUCM_RESULT_RED);
    CHECK(sutr_sucm_result_from_name("amber") == SUCM_RESULT_UNKNOWN);

    /* The compact record of §8. */
    CHECK(sutr_sucm_gate_record_parse("ctest:green", &record) == true);
    CHECK(record.gate == SUCM_GATE_TEST);
    CHECK(record.result == SUCM_RESULT_GREEN);
    CHECK(record.harness[0] == 'c');
    CHECK(record.suite_count == 0u);
    CHECK(sutr_sucm_gate_record_is_valid(&record) == true);
    CHECK(sutr_sucm_gate_record_passes(&record) == true);

    CHECK(sutr_sucm_gate_record_parse("verify-docs:green", &record) == true);
    CHECK(record.gate == SUCM_GATE_DOCS);

    /* A red gate is a valid record and an invalid claim. */
    CHECK(sutr_sucm_gate_record_parse("ctest:red", &record) == true);
    CHECK(sutr_sucm_gate_record_is_valid(&record) == true);
    CHECK(sutr_sucm_gate_record_passes(&record) == false);

    /* An unknown harness is rejected: the gate set is closed. */
    CHECK(sutr_sucm_gate_record_parse("make:green", &record) == false);
    CHECK(sutr_sucm_gate_record_parse("ctest", &record) == false);
    CHECK(sutr_sucm_gate_record_parse(":green", &record) == false);
    CHECK(sutr_sucm_gate_record_parse("ctest:", &record) == false);
    CHECK(sutr_sucm_gate_record_parse("", &record) == false);
    CHECK(sutr_sucm_gate_record_parse(NULL, &record) == false);
    CHECK(sutr_sucm_gate_record_parse("ctest:green", NULL) == false);

    /* The suite list of a gate record. */
    CHECK(sutr_sucm_gate_record_parse("ctest:green", &record) == true);
    CHECK(sutr_sucm_gate_record_add_suite(&record, "test_sutr_sucm") == true);
    CHECK(sutr_sucm_gate_record_add_suite(&record, "test_su_as_ssre") == true);
    CHECK(sutr_sucm_gate_record_suite_count(&record) == 2u);
    CHECK(sutr_sucm_gate_record_suite_at(&record, 0u)[0] == 't');
    CHECK(sutr_sucm_gate_record_suite_at(&record, 1u)[0] == 't');
    CHECK(sutr_sucm_gate_record_suite_at(&record, 2u) == NULL);
    CHECK(sutr_sucm_gate_record_suite_at(NULL, 0u) == NULL);
    CHECK(sutr_sucm_gate_record_add_suite(&record, "") == false);
    CHECK(sutr_sucm_gate_record_add_suite(NULL, "x") == false);
    CHECK(sutr_sucm_gate_record_suite_count(NULL) == 0u);
}

/* ── The claim rows and the published claims of §8 ────────────────── */

static void test_claims(void)
{
    sucm_claim_t claim;
    sucm_claim_t found;
    char row[256];
    uint32_t i;
    uint32_t j;

    CHECK(sutr_sucm_claim_count() == 34u);

    /* The row the report itself publishes. */
    CHECK(sutr_sucm_claim_row_parse(DOC_CLAIM_ROW, &claim) == true);
    CHECK(claim.tier == SUCM_TIER_L0);
    CHECK(claim.unit[0] == 's');
    CHECK(claim.unit[2] == 's');
    CHECK(claim.unit[3] == '\0');
    CHECK(claim.version[0] == '0');
    CHECK(claim.clauses[0] == 'S');
    CHECK(claim.gate[0] == 'c');
    CHECK(sutr_sucm_claim_is_valid(&claim) == true);

    /* The row round-trips through the formatter. */
    CHECK(sutr_sucm_claim_row_format(&claim, row, sizeof(row)) == true);
    CHECK(sutr_sucm_claim_row_parse(row, &found) == true);
    CHECK(found.tier == claim.tier);
    CHECK(sutr_sucm_claim_is_valid(&found) == true);

    /* A row that is missing a field is rejected. */
    CHECK(sutr_sucm_claim_row_parse("L0;sis;0.1.0;SUCM-C1", &claim) == false);
    CHECK(sutr_sucm_claim_row_parse("L0;sis;0.1.0;SUCM-C1;ctest:green;x",
                                    &claim) == false);
    CHECK(sutr_sucm_claim_row_parse("", &claim) == false);
    CHECK(sutr_sucm_claim_row_parse(NULL, &claim) == false);
    CHECK(sutr_sucm_claim_row_parse(DOC_CLAIM_ROW, NULL) == false);

    /* Every published claim holds its tier. */
    for (i = 0u; i < sutr_sucm_claim_count(); i++) {
        CHECK(sutr_sucm_claim_at(i, &claim) == true);
        CHECK(sutr_sucm_claim_is_valid(&claim) == true);

        /* SUCM-R2: only one claim per (tier, unit, version). */
        for (j = 0u; j < i; j++) {
            sucm_claim_t other;

            CHECK(sutr_sucm_claim_at(j, &other) == true);
            CHECK(!(other.tier == claim.tier &&
                   same_text(other.unit, claim.unit) &&
                   same_text(other.version, claim.version)));
        }
    }

    /* The lookup by the claim's tuple. */
    CHECK(sutr_sucm_claim_find(SUCM_TIER_L0, "sis", "0.1.0", &found) == true);
    CHECK(found.tier == SUCM_TIER_L0);
    CHECK(sutr_sucm_claim_find(SUCM_TIER_L0, "sucsl", "0.1.0", &found) == true);
    CHECK(found.tier == SUCM_TIER_L0);
    CHECK(sutr_sucm_claim_find(SUCM_TIER_L1, "sucf", "0.1.0", &found) == true);
    CHECK(found.tier == SUCM_TIER_L1);
    CHECK(sutr_sucm_claim_find(SUCM_TIER_L2, "plugin", "0.1.0", &found) ==
          true);
    CHECK(found.tier == SUCM_TIER_L2);
    CHECK(sutr_sucm_claim_find(SUCM_TIER_L0, "nosuch", "0.1.0", &found) ==
          false);
    CHECK(sutr_sucm_claim_find(SUCM_TIER_L2, "sis", "0.1.0", &found) == false);
    CHECK(sutr_sucm_claim_find(SUCM_TIER_L0, NULL, "0.1.0", &found) == false);
    CHECK(sutr_sucm_claim_at(sutr_sucm_claim_count(), &found) == false);
}

/* ── The rejection rules of §3.4, §5.1 and §6 ────────────────────── */

static void test_claim_rejection(void)
{
    sucm_claim_t claim;

    /* A claim that omits a clause of its tier is void. */
    CHECK(sutr_sucm_claim_row_parse(
              "L1;sucf;0.1.0;SUCM-C1,SUCM-C2,SUCM-C3,R5-R8,R13-R14;ctest:green",
              &claim) == true);
    CHECK(sutr_sucm_claim_is_valid(&claim) == false);

    /* A claim whose gate record is red is void. */
    CHECK(sutr_sucm_claim_row_parse(
              "L0;sis;0.1.0;SUCM-C1,SUCM-C2,SUCM-C3,R5-R8,R13-R14;ctest:red",
              &claim) == true);
    CHECK(sutr_sucm_claim_is_valid(&claim) == false);

    /* A claim with no gate record at all is void. */
    CHECK(sutr_sucm_claim_row_parse(
              "L0;sis;0.1.0;SUCM-C1,SUCM-C2,SUCM-C3,R5-R8,R13-R14;", &claim) ==
          true);
    CHECK(sutr_sucm_claim_is_valid(&claim) == false);

    /* A claim that names a clause that does not exist is void. */
    CHECK(sutr_sucm_claim_row_parse(
              "L0;sis;0.1.0;SUCM-C1,SUCM-C99,R5-R8,R13-R14;ctest:green",
              &claim) == true);
    CHECK(sutr_sucm_claim_is_valid(&claim) == false);

    /* A claim with a malformed unit is void. */
    CHECK(sutr_sucm_claim_row_parse(
              "L0;3sis;0.1.0;SUCM-C1,SUCM-C2,SUCM-C3,R5-R8,R13-R14;ctest:green",
              &claim) == true);
    CHECK(sutr_sucm_claim_is_valid(&claim) == false);

    CHECK(sutr_sucm_claim_is_valid(NULL) == false);
}

/* ── The core engine registry of §8 ──────────────────────────────── */

static void test_engine_registry(void)
{
    sucm_engine_t engine;
    uint32_t i;

    CHECK(sutr_sucm_engine_count() == 34u);

    /* The engine the report names in §1.3.1. */
    CHECK(sutr_sucm_engine_by_module("suas_sis", &engine) == true);
    CHECK(engine.unit[0] == 's');
    CHECK(engine.spec[0] == 'S');
    CHECK(engine.spec[4] == '-');
    CHECK(engine.memory_class[0] == 'O');
    CHECK(engine.time_class[0] == 'O');
    CHECK(engine.tier == SUCM_TIER_L0);

    /* The SUTS engines of §1.3.1. */
    CHECK(sutr_sucm_engine_by_module("suts_sueps", &engine) == true);
    CHECK(engine.spec[0] == 'S');

    /* The reference implementations of the SUTR reports. */
    CHECK(sutr_sucm_engine_by_module("sutr_sucm", &engine) == true);
    CHECK(engine.spec[0] == 'S');
    CHECK(sutr_sucm_engine_by_module("sutr_sucsl", &engine) == true);
    CHECK(engine.spec[0] == 'S' && engine.spec[5] == '0');
    CHECK(engine.tier == SUCM_TIER_L0);
    CHECK(sutr_sucm_engine_by_module("sutr_sucem", &engine) == true);
    CHECK(sutr_sucm_engine_by_module("sutr_sucpm", &engine) == true);
    CHECK(sutr_sucm_engine_by_module("sutr_sumnls", &engine) == true);
    CHECK(sutr_sucm_engine_by_module("sutr_sums", &engine) == true);

    /* The plugin lifecycle of SUTR-5 is the Level-2 unit. */
    CHECK(sutr_sucm_engine_by_module("sucs_plugin", &engine) == true);
    CHECK(engine.tier == SUCM_TIER_L2);

    /* The lookup by the unit name of a claim identifier. */
    CHECK(sutr_sucm_engine_by_unit("sis", &engine) == true);
    CHECK(engine.module[0] == 's');
    CHECK(sutr_sucm_engine_by_unit("nosuch", &engine) == false);
    CHECK(sutr_sucm_engine_by_module("nosuch", &engine) == false);
    CHECK(sutr_sucm_engine_by_module(NULL, &engine) == false);
    CHECK(sutr_sucm_engine_by_unit(NULL, &engine) == false);

    /* Every engine names a module and a specification, and the registry
     * holds no duplicate module names. */
    for (i = 0u; i < sutr_sucm_engine_count(); i++) {
        uint32_t j;

        CHECK(sutr_sucm_engine_at(i, &engine) == true);
        CHECK(engine.module[0] != '\0');
        CHECK(engine.unit[0] != '\0');
        CHECK(engine.spec[0] != '\0');
        CHECK(sutr_sucm_tier_is_valid(engine.tier) == true);
        for (j = 0u; j < i; j++) {
            sucm_engine_t other;

            CHECK(sutr_sucm_engine_at(j, &other) == true);
            CHECK(same_text(other.module, engine.module) == false);
        }
    }
    CHECK(sutr_sucm_engine_at(sutr_sucm_engine_count(), &engine) == false);
    CHECK(sutr_sucm_engine_at(0u, NULL) == false);
}

/* ── The resource gates of §3.2, §3.3 and §6 ─────────────────────── */

static void test_resource_gates(void)
{
    /* The Level-1 bridge gate: 0x000000-0x10FFFF. */
    CHECK(sutr_sucm_bridge_gate_allows(0x00000000ULL) == true);
    CHECK(sutr_sucm_bridge_gate_allows(0x00010FFFFULL) == true);
    CHECK(sutr_sucm_bridge_gate_allows(0x000110000ULL) == false);
    CHECK(sutr_sucm_bridge_gate_allows(0x00120000ULL) == false);

    /* The Level-2 native-plane gate: 0x120000-0x7FFFFFEF, or strictly
     * above the Sentinel. */
    CHECK(sutr_sucm_native_gate_allows(0x00120000ULL) == true);
    CHECK(sutr_sucm_native_gate_allows(0x0016FFFFULL) == true);
    CHECK(sutr_sucm_native_gate_allows(0x7FFFFFEFULL) == true);
    CHECK(sutr_sucm_native_gate_allows(0x80000000ULL) == true);
    CHECK(sutr_sucm_native_gate_allows(0xFFFFFFFFULL) == true);

    /* The bridge range is not the native plane. */
    CHECK(sutr_sucm_native_gate_allows(0x00000000ULL) == false);
    CHECK(sutr_sucm_native_gate_allows(0x00010FFFFULL) == false);
    CHECK(sutr_sucm_native_gate_allows(0x0011FFFFULL) == false);

    /* The trap slots and the Sentinel are unallocatable. */
    CHECK(sutr_sucm_native_gate_allows(0x7FFFFFF0ULL) == false);
    CHECK(sutr_sucm_native_gate_allows(0x7FFFFFFEULL) == false);
    CHECK(sutr_sucm_native_gate_allows(0x7FFFFFFFULL) == false);

    CHECK(sutr_sucm_is_trap(0x7FFFFFF0ULL) == true);
    CHECK(sutr_sucm_is_trap(0x7FFFFFFEULL) == true);
    CHECK(sutr_sucm_is_trap(0x7FFFFFFFULL) == false);
    CHECK(sutr_sucm_is_trap(0x7FFFFFEFULL) == false);
    CHECK(sutr_sucm_is_sentinel(0x7FFFFFFFULL) == true);
    CHECK(sutr_sucm_is_sentinel(0x7FFFFFFEULL) == false);

    /* The neography carve-out: 0x160000-0x16FFFF. */
    CHECK(sutr_sucm_neography_gate_allows(0x00160000ULL) == true);
    CHECK(sutr_sucm_neography_gate_allows(0x0016FFFFULL) == true);
    CHECK(sutr_sucm_neography_gate_allows(0x0015FFFFULL) == false);
    CHECK(sutr_sucm_neography_gate_allows(0x00170000ULL) == false);
}

int main(void)
{
    test_version_and_meta();
    test_tiers();
    test_clause_registry();
    test_clause_sets();
    test_clause_list_parse();
    test_claim_id();
    test_naming_rules();
    test_claim_id_equivalence();
    test_gate_records();
    test_claims();
    test_claim_rejection();
    test_engine_registry();
    test_resource_gates();

    if (g_fail != 0) {
        printf("FAIL: SUTR-10 SUCM tests failed (%d)\n", g_fail);
        return 1;
    }
    printf("[PASS] SUTR-10 SUCM tests passed\n");
    return 0;
}
