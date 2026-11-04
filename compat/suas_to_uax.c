/**
 * suas_to_uax.c — SUAS-to-UAX Converter & Compatibility Tester
 *
 * Converts between SuperUnicode Architecture Standard (SUAS) document
 * numbers and their Unicode Standard Annex (UAX) analogs, and reports
 * a detailed compatibility grade for each pair.
 *
 * Coverage: SUAS-001 … SUAS-017 (the full set analogous to Unicode UAXes).
 *
 * Usage:
 *   suas_to_uax                     — print full SUAS↔UAX mapping table
 *   suas_to_uax suas <N>            — convert SUAS-N → UAX #?
 *   suas_to_uax uax  <N>            — convert UAX #N → SUAS-?
 *   suas_to_uax compat suas <N>     — compat grade for SUAS-N
 *   suas_to_uax compat uax  <N>     — compat grade for UAX #N
 *   suas_to_uax test                — run all self-checks (exit 0 on pass)
 *
 * Build (standalone, no extra deps):
 *   gcc -std=c99 -Wall -Wextra -o suas_to_uax suas_to_uax.c
 *
 * Build via CMake (see compat/CMakeLists.txt):
 *   add_executable(suas_to_uax suas_to_uax.c)
 *   target_include_directories(suas_to_uax PRIVATE .)
 *   set_target_properties(suas_to_uax PROPERTIES
 *       RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sustandard_map.h"

/* ---------------------------------------------------------------- helpers */

static void grade_describe(char g, const char **long_name, const char **symbol) {
    switch (g) {
    case 'Y': *long_name = "Full analog";    *symbol = "[Y]"; break;
    case 'P': *long_name = "Partial analog"; *symbol = "[P]"; break;
    case 'E': *long_name = "Extends beyond"; *symbol = "[E]"; break;
    default:  *long_name = "Unknown";        *symbol = "[?]"; break;
    }
}

static void print_suas_entry(const suas_uax_entry_t *e) {
    const char *gname, *gsym;
    grade_describe(e->compat_grade, &gname, &gsym);
    printf("  SUAS-%03d (%s)  <->  UAX #%d\n", e->su_number, e->su_abbr, e->unicode_number);
    printf("    SU title : %s\n", e->su_title);
    printf("    UC title : %s\n", e->uc_title);
    printf("    Compat   : %s %s\n", gsym, gname);
    printf("    Note     : %s\n", e->compat_note);
}

static void print_full_table(void) {
    int i;
    printf("=================================================================\n");
    printf("  SUAS ↔ UAX Mapping Table  (SUAS-001 … SUAS-017)\n");
    printf("  Grades: [Y]=Full analog  [P]=Partial  [E]=Extends beyond UAX\n");
    printf("=================================================================\n");
    printf("%-10s %-9s %-8s %s\n", "SUAS", "UAX", "Grade", "Abbr / Short title");
    printf("-----------------------------------------------------------------\n");
    for (i = 0; i < SUAS_UAX_COUNT; ++i) {
        const suas_uax_entry_t *e = &SUAS_UAX_TABLE[i];
        const char *gname, *gsym;
        grade_describe(e->compat_grade, &gname, &gsym);
        printf("SUAS-%03d    UAX #%-4d  %s  %s — %s\n",
               e->su_number, e->unicode_number, gsym, e->su_abbr, e->su_title);
    }
    printf("=================================================================\n");
}

/* ------------------------------------------------------------------- test */

static int run_tests(void) {
    int failed = 0;

#define T_ASSERT(cond, msg) \
    do { if (!(cond)) { fprintf(stderr, "FAIL: %s\n", msg); ++failed; } } while(0)

    /* Every SUAS-N (1..17) must map to a unique UAX number. */
    {
        int i, j;
        for (i = 0; i < SUAS_UAX_COUNT; ++i) {
            for (j = i + 1; j < SUAS_UAX_COUNT; ++j) {
                T_ASSERT(SUAS_UAX_TABLE[i].su_number != SUAS_UAX_TABLE[j].su_number,
                         "Duplicate SUAS number in table");
                T_ASSERT(SUAS_UAX_TABLE[i].unicode_number != SUAS_UAX_TABLE[j].unicode_number,
                         "Duplicate UAX number in table");
            }
        }
    }

    /* Table count. */
    T_ASSERT(SUAS_UAX_COUNT == 17, "SUAS_UAX_COUNT must be 17");

    /* Spot-check canonical pairs (from SUAS document headers). */
    {
        const suas_uax_entry_t *e;

        e = suas_find_by_su(1);
        T_ASSERT(e != 0, "SUAS-001 must exist");
        T_ASSERT(e->unicode_number == 9, "SUAS-001 must map to UAX #9 (BiDi)");
        T_ASSERT(strcmp(e->su_abbr, "SDF") == 0, "SUAS-001 abbr must be SDF");

        e = suas_find_by_su(2);
        T_ASSERT(e != 0, "SUAS-002 must exist");
        T_ASSERT(e->unicode_number == 11, "SUAS-002 must map to UAX #11");

        e = suas_find_by_su(3);
        T_ASSERT(e && e->unicode_number == 14, "SUAS-003 -> UAX #14");

        e = suas_find_by_su(4);
        T_ASSERT(e && e->unicode_number == 15, "SUAS-004 -> UAX #15");

        e = suas_find_by_su(5);
        T_ASSERT(e && e->unicode_number == 24, "SUAS-005 -> UAX #24");

        e = suas_find_by_su(6);
        T_ASSERT(e && e->unicode_number == 29, "SUAS-006 -> UAX #29");

        e = suas_find_by_su(7);
        T_ASSERT(e && e->unicode_number == 31, "SUAS-007 -> UAX #31");

        e = suas_find_by_su(8);
        T_ASSERT(e && e->unicode_number == 34, "SUAS-008 -> UAX #34");

        e = suas_find_by_su(9);
        T_ASSERT(e && e->unicode_number == 38, "SUAS-009 -> UAX #38");

        e = suas_find_by_su(10);
        T_ASSERT(e && e->unicode_number == 41, "SUAS-010 -> UAX #41");

        e = suas_find_by_su(11);
        T_ASSERT(e && e->unicode_number == 42, "SUAS-011 -> UAX #42");

        e = suas_find_by_su(12);
        T_ASSERT(e && e->unicode_number == 44, "SUAS-012 -> UAX #44");

        e = suas_find_by_su(13);
        T_ASSERT(e && e->unicode_number == 45, "SUAS-013 -> UAX #45");

        e = suas_find_by_su(14);
        T_ASSERT(e && e->unicode_number == 50, "SUAS-014 -> UAX #50");

        e = suas_find_by_su(15);
        T_ASSERT(e && e->unicode_number == 53, "SUAS-015 -> UAX #53");

        e = suas_find_by_su(16);
        T_ASSERT(e && e->unicode_number == 57, "SUAS-016 -> UAX #57");

        e = suas_find_by_su(17);
        T_ASSERT(e && e->unicode_number == 60, "SUAS-017 -> UAX #60");
    }

    /* Reverse lookup spot-checks. */
    {
        const suas_uax_entry_t *e;
        e = suas_find_by_uax(9);
        T_ASSERT(e && e->su_number == 1, "UAX #9 -> SUAS-001");
        e = suas_find_by_uax(44);
        T_ASSERT(e && e->su_number == 12, "UAX #44 -> SUAS-012");
        e = suas_find_by_uax(60);
        T_ASSERT(e && e->su_number == 17, "UAX #60 -> SUAS-017");
    }

    /* Compat grades are only Y, P, or E. */
    {
        int i;
        for (i = 0; i < SUAS_UAX_COUNT; ++i) {
            char g = SUAS_UAX_TABLE[i].compat_grade;
            T_ASSERT(g == 'Y' || g == 'P' || g == 'E', "Invalid compat grade");
        }
    }

    /* Out-of-range lookups must return NULL. */
    T_ASSERT(suas_find_by_su(0)   == 0, "SUAS-0 must not exist");
    T_ASSERT(suas_find_by_su(18)  == 0, "SUAS-018 is outside covered range");
    T_ASSERT(suas_find_by_uax(1)  == 0, "UAX #1 has no SUAS analog");

#undef T_ASSERT

    if (failed == 0) {
        printf("suas_to_uax: all tests passed.\n");
    } else {
        printf("suas_to_uax: %d test(s) FAILED.\n", failed);
    }
    return failed == 0 ? 0 : 1;
}

/* -------------------------------------------------------------------- main */

int main(int argc, char *argv[]) {
    if (argc == 1) {
        print_full_table();
        return 0;
    }

    if (strcmp(argv[1], "test") == 0) {
        return run_tests();
    }

    if (strcmp(argv[1], "suas") == 0 && argc == 3) {
        int n = atoi(argv[2]);
        const suas_uax_entry_t *e = suas_find_by_su(n);
        if (!e) {
            fprintf(stderr, "suas_to_uax: SUAS-%d is not in the covered range (1..17).\n", n);
            return 1;
        }
        printf("SUAS-%03d (%s) is analogous to UAX #%d \"%s\"\n",
               e->su_number, e->su_abbr, e->unicode_number, e->uc_title);
        return 0;
    }

    if (strcmp(argv[1], "uax") == 0 && argc == 3) {
        int n = atoi(argv[2]);
        const suas_uax_entry_t *e = suas_find_by_uax(n);
        if (!e) {
            fprintf(stderr, "suas_to_uax: UAX #%d has no SUAS analog in the covered range.\n", n);
            return 1;
        }
        printf("UAX #%d \"%s\" is analogous to SUAS-%03d (%s) \"%s\"\n",
               e->unicode_number, e->uc_title,
               e->su_number, e->su_abbr, e->su_title);
        return 0;
    }

    if (strcmp(argv[1], "compat") == 0 && argc == 4) {
        const suas_uax_entry_t *e = 0;
        if (strcmp(argv[2], "suas") == 0)      e = suas_find_by_su(atoi(argv[3]));
        else if (strcmp(argv[2], "uax") == 0)  e = suas_find_by_uax(atoi(argv[3]));
        if (!e) {
            fprintf(stderr, "suas_to_uax: document not found in covered range.\n");
            return 1;
        }
        print_suas_entry(e);
        return 0;
    }

    fprintf(stderr,
        "suas_to_uax — SUAS ↔ UAX converter & compat tester\n"
        "Usage:\n"
        "  suas_to_uax                      print full mapping table\n"
        "  suas_to_uax suas <N>             SUAS-N -> UAX #?\n"
        "  suas_to_uax uax  <N>             UAX #N -> SUAS-?\n"
        "  suas_to_uax compat suas <N>      compat grade for SUAS-N\n"
        "  suas_to_uax compat uax  <N>      compat grade for UAX #N\n"
        "  suas_to_uax test                 run self-checks\n"
        "Coverage: SUAS-001..017 / UAX #9,11,14,15,24,29,31,34,38,41,42,44,45,50,53,57,60\n");
    return 1;
}
