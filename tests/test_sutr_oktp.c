/**
 * SUTR-014 OKTP - OpenWindows Kernel Text Integration Profile
 *
 * Unit tests for the informative profile data: the matrix shape, level
 * cumulativity, gap closure, gate commands, and the registry cross-check that
 * every citation resolves with the right family class.
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "sutr/sutr_oktp.h"

static int g_fail = 0;
#define CHECK(cond) do { if (!(cond)) { \
    printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); g_fail = 1; \
} } while (0)

static void test_version_and_names(void)
{
    CHECK(sutr_oktp_version_string() != NULL);
    CHECK(strstr(sutr_oktp_version_string(), "SUTR-014") != NULL);

    CHECK(strcmp(sutr_oktp_status_name(OKTP_OK), "OKTP_OK") == 0);
    CHECK(strcmp(sutr_oktp_status_name(OKTP_E_CITATION),
                 "OKTP_E_CITATION") == 0);
    CHECK(strcmp(sutr_oktp_status_name((oktp_status_t)99),
                 "OKTP_E_UNKNOWN") == 0);

    CHECK(strcmp(sutr_oktp_adoption_name(OKTP_ADOPT_LINKED), "Linked") == 0);
    CHECK(strcmp(sutr_oktp_adoption_name(OKTP_ADOPT_SPECIFIED),
                 "Specified") == 0);
    CHECK(strcmp(sutr_oktp_adoption_name((oktp_adoption_t)99),
                 "Unknown") == 0);
    CHECK(sutr_oktp_adoption_meaning(OKTP_ADOPT_ABSENT) != NULL);

    CHECK(strcmp(sutr_oktp_tier_name(OKTP_TIER_T0), "T0") == 0);
    CHECK(strcmp(sutr_oktp_tier_name(OKTP_TIER_NA), "N/A") == 0);
    CHECK(strcmp(sutr_oktp_tier_name((oktp_tier_t)99), "Unknown") == 0);
    CHECK(sutr_oktp_tier_meaning(OKTP_TIER_LINKED) != NULL);

    CHECK(strcmp(sutr_oktp_level_name(OKTP_LEVEL_KT0), "KT-0") == 0);
    CHECK(strcmp(sutr_oktp_level_name(OKTP_LEVEL_KT2), "KT-2") == 0);
    CHECK(strcmp(sutr_oktp_level_name((oktp_level_id_t)99), "Unknown") == 0);
}

static void test_matrix(void)
{
    const oktp_matrix_entry_t *e;
    size_t i;

    CHECK(sutr_oktp_matrix_count() == 40u);
    CHECK(sutr_oktp_matrix_at(40u) == NULL);
    CHECK(sutr_oktp_matrix_at(0u) != NULL);

    for (i = 0u; i < sutr_oktp_matrix_count(); i++) {
        e = sutr_oktp_matrix_at(i);
        CHECK(e != NULL);
        if (e == NULL) {
            continue;
        }
        CHECK(e->subsystem != NULL && e->subsystem[0] != '\0');
        CHECK(e->why != NULL && e->why[0] != '\0');
        CHECK(e->doc_count > 0u);
        CHECK((size_t)e->doc_count <= (size_t)OKTP_MAX_DOCS_PER_ROW);
        CHECK(e->tier <= (uint8_t)OKTP_TIER_NA);
        CHECK(sutr_oktp_tier_name((oktp_tier_t)e->tier) != NULL);
    }

    e = sutr_oktp_matrix_find("lib/kstring.c");
    CHECK(e != NULL);
    CHECK(e != NULL && e->tier == (uint8_t)OKTP_TIER_T0);
    CHECK(sutr_oktp_entry_cites(e, "SUAS-018"));
    CHECK(!sutr_oktp_entry_cites(e, "SUTS-012"));

    e = sutr_oktp_matrix_find("DLL/suct plane");
    CHECK(e != NULL);
    CHECK(sutr_oktp_entry_cites(e, "SUTS-012"));
    CHECK(sutr_oktp_entry_cites(e, "SUAS-002"));

    e = sutr_oktp_matrix_find("Software/owterm");
    CHECK(e != NULL);
    CHECK(e != NULL && e->doc_count == 4u);

    CHECK(sutr_oktp_matrix_find("no such subsystem") == NULL);
    CHECK(sutr_oktp_matrix_find(NULL) == NULL);

    CHECK(!sutr_oktp_entry_cites(NULL, "SUAS-018"));
    CHECK(!sutr_oktp_entry_cites(sutr_oktp_matrix_find("lib/kstring.c"),
                                 NULL));

    /* SUAS-018 is the most-cited standard: every text-owning kernel path. */
    CHECK(sutr_oktp_matrix_doc_count("SUAS-018") == 9u);
    CHECK(sutr_oktp_matrix_doc_count("SUTS-012") == 8u);
    CHECK(sutr_oktp_matrix_doc_count("SUAS-019") == 1u);
    CHECK(sutr_oktp_matrix_doc_count("SUTS-013") == 1u);
    CHECK(sutr_oktp_matrix_doc_count("SUTR-014") == 0u);
    CHECK(sutr_oktp_matrix_doc_count("SUAS-099") == 0u);
    CHECK(sutr_oktp_matrix_doc_count(NULL) == 0u);
}

static void test_levels(void)
{
    const oktp_level_t *l;

    CHECK(sutr_oktp_level(OKTP_LEVEL_KT0) != NULL);
    CHECK(sutr_oktp_level(OKTP_LEVEL_KT1) != NULL);
    CHECK(sutr_oktp_level(OKTP_LEVEL_KT2) != NULL);
    CHECK(sutr_oktp_level((oktp_level_id_t)OKTP_LEVEL_COUNT) == NULL);

    l = sutr_oktp_level_find("KT-0");
    CHECK(l != NULL && l->doc_count == 2u);
    CHECK(l != NULL && l->met);

    l = sutr_oktp_level_find("KT-1");
    CHECK(l != NULL);
    CHECK(!sutr_oktp_level_met(OKTP_LEVEL_KT1));
    CHECK(l != NULL && l->evidence != NULL);

    l = sutr_oktp_level_find("KT-2");
    CHECK(l != NULL);
    CHECK(!sutr_oktp_level_met(OKTP_LEVEL_KT2));
    CHECK(l != NULL && l->doc_count <= OKTP_MAX_DOCS_PER_ROW);

    CHECK(sutr_oktp_level_met(OKTP_LEVEL_KT0));
    CHECK(!sutr_oktp_level_met((oktp_level_id_t)99));
    CHECK(sutr_oktp_level_find("KT-9") == NULL);
    CHECK(sutr_oktp_level_find(NULL) == NULL);

    /* Cumulative: each level restates the previous level's documents. */
    {
        const oktp_level_t *a = sutr_oktp_level(OKTP_LEVEL_KT0);
        const oktp_level_t *b = sutr_oktp_level(OKTP_LEVEL_KT1);
        const oktp_level_t *c = sutr_oktp_level(OKTP_LEVEL_KT2);
        uint8_t i;
        CHECK(a != NULL && b != NULL && c != NULL);
        for (i = 0u; i < a->doc_count; i++) {
            bool found = false;
            uint8_t j;
            for (j = 0u; j < b->doc_count; j++) {
                if (strcmp(a->docs[i], b->docs[j]) == 0) found = true;
            }
            CHECK(found);
        }
        for (i = 0u; i < b->doc_count; i++) {
            bool found = false;
            uint8_t j;
            for (j = 0u; j < c->doc_count; j++) {
                if (strcmp(b->docs[i], c->docs[j]) == 0) found = true;
            }
            CHECK(found);
        }
    }
}

static void test_gaps_and_gates(void)
{
    const oktp_gap_t *g;
    const oktp_gate_t *gate;
    size_t i;

    CHECK(sutr_oktp_gap_count() == 11u);
    CHECK(sutr_oktp_gap_at(11u) == NULL);

    for (i = 0u; i < sutr_oktp_gap_count(); i++) {
        g = sutr_oktp_gap_at(i);
        CHECK(g != NULL);
        if (g == NULL) {
            continue;
        }
        CHECK(g->id != NULL && g->summary != NULL);
        CHECK(g->closed_by != NULL);
        CHECK(sutr_oktp_check_citation(g->closed_by) == OKTP_OK);
    }

    /* The pilot added two adoption gaps; both are closed by the new
     * standards and are therefore carried as open gaps in this profile. */
    CHECK(sutr_oktp_gap_find("G-10") != NULL);
    CHECK(sutr_oktp_gap_find("G-11") != NULL);
    CHECK(sutr_oktp_gap_at(11u) == NULL);

    g = sutr_oktp_gap_find("G-9");
    CHECK(g != NULL);
    CHECK(g != NULL && strcmp(g->closed_by, "SUAS-018") == 0);
    g = sutr_oktp_gap_find("G-10");
    CHECK(g != NULL && strcmp(g->closed_by, "SUAS-019") == 0);
    g = sutr_oktp_gap_find("G-11");
    CHECK(g != NULL && strcmp(g->closed_by, "SUTS-013") == 0);
    CHECK(sutr_oktp_gap_find("G-99") == NULL);
    CHECK(sutr_oktp_gap_find(NULL) == NULL);

    CHECK(sutr_oktp_gate_count() == 6u);
    CHECK(sutr_oktp_gate_at(6u) == NULL);

    gate = sutr_oktp_gate_find("D-1");
    CHECK(gate != NULL);
    CHECK(gate != NULL && strstr(gate->command, "verify-docs") != NULL);
    CHECK(gate != NULL && gate->proves != NULL);

    gate = sutr_oktp_gate_find("K-2");
    CHECK(gate != NULL);
    CHECK(gate != NULL && strstr(gate->command, "qemu") != NULL);

    CHECK(sutr_oktp_gate_find("D-9") == NULL);
    CHECK(sutr_oktp_gate_find(NULL) == NULL);
}

static void test_citations(void)
{
    CHECK(sutr_oktp_check_citation("SUAS-018") == OKTP_OK);
    CHECK(sutr_oktp_check_citation("SUAS-002") == OKTP_OK);
    CHECK(sutr_oktp_check_citation("SUTS-012") == OKTP_OK);
    CHECK(sutr_oktp_check_citation("SUTR-0") == OKTP_OK);
    CHECK(sutr_oktp_check_citation("SUTR-014") == OKTP_OK);

    CHECK(sutr_oktp_check_citation(NULL) == OKTP_E_NULL);
    CHECK(sutr_oktp_check_citation("SUAS-099") == OKTP_E_CITATION);
    CHECK(sutr_oktp_check_citation("SUTS-099") == OKTP_E_CITATION);
    CHECK(sutr_oktp_check_citation("SUTR-099") == OKTP_E_CITATION);
    CHECK(sutr_oktp_check_citation("NOT-A-SPEC") == OKTP_E_CITATION);
}

static void test_verify(void)
{
    const char *doc = (const char *)"sentinel";
    size_t index = 99u;

    CHECK(sutr_oktp_verify(&doc, &index) == OKTP_OK);
    CHECK(doc == NULL);
    CHECK(index == 0u);

    /* The same check with optional out-parameters absent. */
    CHECK(sutr_oktp_verify(NULL, NULL) == OKTP_OK);
}

int main(void)
{
    test_version_and_names();
    test_matrix();
    test_levels();
    test_gaps_and_gates();
    test_citations();
    test_verify();

    if (g_fail) {
        printf("OKTP tests FAILED\n");
        return 1;
    }
    printf("OKTP tests passed\n");
    return 0;
}
