/**
 * sutr_to_utr.c — SUTR-to-UTR Converter & Compatibility Tester
 *
 * Converts between SuperUnicode Technical Report (SUTR) document numbers
 * and their Unicode Technical Report (UTR) analogs, and reports a detailed
 * compatibility grade for each pair.
 *
 * Coverage: SUTR-7 … SUTR-013 (the subset that maps to UTR #17 … UTR #59).
 *
 * Usage:
 *   sutr_to_utr                     — print full SUTR↔UTR mapping table
 *   sutr_to_utr sutr <N>            — convert SUTR-N → UTR #?
 *   sutr_to_utr utr  <N>            — convert UTR #N → SUTR-?
 *   sutr_to_utr compat sutr <N>     — compat grade for SUTR-N
 *   sutr_to_utr compat utr  <N>     — compat grade for UTR #N
 *   sutr_to_utr test                — run all self-checks (exit 0 on pass)
 *
 * Build (standalone, no extra deps):
 *   gcc -std=c99 -Wall -Wextra -o sutr_to_utr sutr_to_utr.c
 *
 * Build via CMake (see compat/CMakeLists.txt):
 *   add_executable(sutr_to_utr sutr_to_utr.c)
 *   target_include_directories(sutr_to_utr PRIVATE .)
 *   set_target_properties(sutr_to_utr PROPERTIES
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

static void print_sutr_entry(const sutr_utr_entry_t *e) {
    const char *gname, *gsym;
    grade_describe(e->compat_grade, &gname, &gsym);
    printf("  SUTR-%-3d (%s)  <->  UTR #%d\n", e->su_number, e->su_abbr, e->unicode_number);
    printf("    SU title : %s\n", e->su_title);
    printf("    UC title : %s\n", e->uc_title);
    printf("    Compat   : %s %s\n", gsym, gname);
    printf("    Note     : %s\n", e->compat_note);
}

static void print_full_table(void) {
    int i;
    printf("=================================================================\n");
    printf("  SUTR ↔ UTR Mapping Table  (SUTR-7 … SUTR-013)\n");
    printf("  Grades: [Y]=Full analog  [P]=Partial  [E]=Extends beyond UTR\n");
    printf("=================================================================\n");
    printf("%-10s %-9s %-8s %s\n", "SUTR", "UTR", "Grade", "Abbr / Short title");
    printf("-----------------------------------------------------------------\n");
    for (i = 0; i < SUTR_UTR_COUNT; ++i) {
        const sutr_utr_entry_t *e = &SUTR_UTR_TABLE[i];
        const char *gname, *gsym;
        grade_describe(e->compat_grade, &gname, &gsym);
        printf("SUTR-%-3d    UTR #%-4d  %s  %s — %s\n",
               e->su_number, e->unicode_number, gsym, e->su_abbr, e->su_title);
    }
    printf("=================================================================\n");
    printf("Note: SUTR-0..6 have no direct UTR analog (foundational/original work).\n");
}

/* ------------------------------------------------------------------- test */

static int run_tests(void) {
    int failed = 0;

#define T_ASSERT(cond, msg) \
    do { if (!(cond)) { fprintf(stderr, "FAIL: %s\n", msg); ++failed; } } while(0)

    /* Must have exactly 7 entries (SUTR-7 through SUTR-013). */
    T_ASSERT(SUTR_UTR_COUNT == 7, "SUTR_UTR_COUNT must be 7");

    /* No duplicate SUTR or UTR numbers. */
    {
        int i, j;
        for (i = 0; i < SUTR_UTR_COUNT; ++i) {
            for (j = i + 1; j < SUTR_UTR_COUNT; ++j) {
                T_ASSERT(SUTR_UTR_TABLE[i].su_number != SUTR_UTR_TABLE[j].su_number,
                         "Duplicate SUTR number");
                T_ASSERT(SUTR_UTR_TABLE[i].unicode_number != SUTR_UTR_TABLE[j].unicode_number,
                         "Duplicate UTR number");
            }
        }
    }

    /* Spot-check canonical pairs (from SUTR §1.1 «Relation to UTR #N» sections). */
    {
        const sutr_utr_entry_t *e;

        e = sutr_find_by_su(7);
        T_ASSERT(e != 0, "SUTR-7 must exist");
        T_ASSERT(e->unicode_number == 17, "SUTR-7 -> UTR #17 (Character Encoding Model)");
        T_ASSERT(strcmp(e->su_abbr, "SUCEM") == 0, "SUTR-7 abbr must be SUCEM");

        e = sutr_find_by_su(8);
        T_ASSERT(e && e->unicode_number == 23, "SUTR-8 -> UTR #23 (Property Model)");
        T_ASSERT(e && strcmp(e->su_abbr, "SUCPM") == 0, "SUTR-8 abbr must be SUCPM");

        e = sutr_find_by_su(9);
        T_ASSERT(e && e->unicode_number == 25, "SUTR-9 -> UTR #25 (Math Support)");
        T_ASSERT(e && strcmp(e->su_abbr, "SUMNLS") == 0, "SUTR-9 abbr must be SUMNLS");

        e = sutr_find_by_su(10);
        T_ASSERT(e && e->unicode_number == 33, "SUTR-10 -> UTR #33 (Conformance Model)");
        T_ASSERT(e && strcmp(e->su_abbr, "SUCM") == 0, "SUTR-10 abbr must be SUCM");

        e = sutr_find_by_su(11);
        T_ASSERT(e && e->unicode_number == 54, "SUTR-11 -> UTR #54 (Mongolian)");
        T_ASSERT(e && strcmp(e->su_abbr, "SUMS") == 0, "SUTR-11 abbr must be SUMS");

        e = sutr_find_by_su(12);
        T_ASSERT(e && e->unicode_number == 56, "SUTR-012 -> UTR #56 (Cuneiform)");
        T_ASSERT(e && strcmp(e->su_abbr, "SUCSL") == 0, "SUTR-012 abbr must be SUCSL");

        e = sutr_find_by_su(13);
        T_ASSERT(e && e->unicode_number == 59, "SUTR-013 -> UTR #59 (East Asian Spacing)");
        T_ASSERT(e && strcmp(e->su_abbr, "SUEAS") == 0, "SUTR-013 abbr must be SUEAS");
    }

    /* Reverse lookup spot-checks. */
    {
        const sutr_utr_entry_t *e;
        e = sutr_find_by_utr(17);
        T_ASSERT(e && e->su_number == 7,  "UTR #17 -> SUTR-7");
        e = sutr_find_by_utr(33);
        T_ASSERT(e && e->su_number == 10, "UTR #33 -> SUTR-10");
        e = sutr_find_by_utr(59);
        T_ASSERT(e && e->su_number == 13, "UTR #59 -> SUTR-013");
    }

    /* Grades are only Y, P, or E. */
    {
        int i;
        for (i = 0; i < SUTR_UTR_COUNT; ++i) {
            char g = SUTR_UTR_TABLE[i].compat_grade;
            T_ASSERT(g == 'Y' || g == 'P' || g == 'E', "Invalid compat grade");
        }
    }

    /* SUTR-0..6 have no UTR analog in the covered range. */
    T_ASSERT(sutr_find_by_su(0)  == 0, "SUTR-0 outside covered range");
    T_ASSERT(sutr_find_by_su(6)  == 0, "SUTR-6 outside covered range");
    T_ASSERT(sutr_find_by_su(14) == 0, "SUTR-014 outside covered range");
    T_ASSERT(sutr_find_by_utr(1) == 0, "UTR #1 has no SUTR analog");

#undef T_ASSERT

    if (failed == 0) {
        printf("sutr_to_utr: all tests passed.\n");
    } else {
        printf("sutr_to_utr: %d test(s) FAILED.\n", failed);
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

    if (strcmp(argv[1], "sutr") == 0 && argc == 3) {
        int n = atoi(argv[2]);
        const sutr_utr_entry_t *e = sutr_find_by_su(n);
        if (!e) {
            if (n >= 0 && n <= 6)
                fprintf(stderr, "sutr_to_utr: SUTR-%d has no UTR analog (foundational/original work).\n", n);
            else
                fprintf(stderr, "sutr_to_utr: SUTR-%d is not in the covered range (7..13).\n", n);
            return 1;
        }
        printf("SUTR-%d (%s) is analogous to UTR #%d \"%s\"\n",
               e->su_number, e->su_abbr, e->unicode_number, e->uc_title);
        return 0;
    }

    if (strcmp(argv[1], "utr") == 0 && argc == 3) {
        int n = atoi(argv[2]);
        const sutr_utr_entry_t *e = sutr_find_by_utr(n);
        if (!e) {
            fprintf(stderr, "sutr_to_utr: UTR #%d has no SUTR analog in the covered range.\n", n);
            return 1;
        }
        printf("UTR #%d \"%s\" is analogous to SUTR-%d (%s) \"%s\"\n",
               e->unicode_number, e->uc_title,
               e->su_number, e->su_abbr, e->su_title);
        return 0;
    }

    if (strcmp(argv[1], "compat") == 0 && argc == 4) {
        const sutr_utr_entry_t *e = 0;
        if (strcmp(argv[2], "sutr") == 0)      e = sutr_find_by_su(atoi(argv[3]));
        else if (strcmp(argv[2], "utr") == 0)  e = sutr_find_by_utr(atoi(argv[3]));
        if (!e) {
            fprintf(stderr, "sutr_to_utr: document not found in covered range.\n");
            return 1;
        }
        print_sutr_entry(e);
        return 0;
    }

    fprintf(stderr,
        "sutr_to_utr — SUTR ↔ UTR converter & compat tester\n"
        "Usage:\n"
        "  sutr_to_utr                      print full mapping table\n"
        "  sutr_to_utr sutr <N>             SUTR-N -> UTR #?\n"
        "  sutr_to_utr utr  <N>             UTR #N -> SUTR-?\n"
        "  sutr_to_utr compat sutr <N>      compat grade for SUTR-N\n"
        "  sutr_to_utr compat utr  <N>      compat grade for UTR #N\n"
        "  sutr_to_utr test                 run self-checks\n"
        "Coverage: SUTR-7..13 / UTR #17,23,25,33,54,56,59\n");
    return 1;
}
