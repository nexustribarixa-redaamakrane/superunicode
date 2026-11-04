/**
 * suts_to_uts.c — SUTS-to-UTS Converter & Compatibility Tester
 *
 * Converts between SuperUnicode Technical Specification (SUTS) document
 * numbers and their Unicode Technical Standard (UTS) analogs, and reports
 * a detailed compatibility grade for each pair.
 *
 * Coverage: SUTS-001 … SUTS-010 (the full set analogous to Unicode UTSes).
 *           Continuous numbering — SUTS-010 is SUSN (UTS #61).
 *
 * Usage:
 *   suts_to_uts                     — print full SUTS↔UTS mapping table
 *   suts_to_uts suts <N>            — convert SUTS-N → UTS #?
 *   suts_to_uts uts  <N>            — convert UTS #N → SUTS-?
 *   suts_to_uts compat suts <N>     — compat grade for SUTS-N
 *   suts_to_uts compat uts  <N>     — compat grade for UTS #N
 *   suts_to_uts test                — run all self-checks (exit 0 on pass)
 *
 * Build (standalone, no extra deps):
 *   gcc -std=c99 -Wall -Wextra -o suts_to_uts suts_to_uts.c
 *
 * Build via CMake (see compat/CMakeLists.txt):
 *   add_executable(suts_to_uts suts_to_uts.c)
 *   target_include_directories(suts_to_uts PRIVATE .)
 *   set_target_properties(suts_to_uts PROPERTIES
 *       RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)
 */

#include "sustandard_map.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---------------------------------------------------------------- helpers */

static void grade_describe(char g, const char **long_name,
                           const char **symbol) {
  switch (g) {
  case 'Y':
    *long_name = "Full analog";
    *symbol = "[Y]";
    break;
  case 'P':
    *long_name = "Partial analog";
    *symbol = "[P]";
    break;
  case 'E':
    *long_name = "Extends beyond";
    *symbol = "[E]";
    break;
  default:
    *long_name = "Unknown";
    *symbol = "[?]";
    break;
  }
}

static void print_suts_entry(const suts_uts_entry_t *e) {
  const char *gname, *gsym;
  grade_describe(e->compat_grade, &gname, &gsym);
  printf("  SUTS-%03d (%s)  <->  UTS #%d\n", e->su_number, e->su_abbr,
         e->unicode_number);
  printf("    SU title : %s\n", e->su_title);
  printf("    UC title : %s\n", e->uc_title);
  printf("    Compat   : %s %s\n", gsym, gname);
  printf("    Note     : %s\n", e->compat_note);
}

static void print_full_table(void) {
  int i;
  printf("=================================================================\n");
  printf("  SUTS ↔ UTS Mapping Table  (SUTS-001 … SUTS-010)\n");
  printf("  Grades: [Y]=Full analog  [P]=Partial  [E]=Extends beyond UTS\n");
  printf("=================================================================\n");
  printf("%-10s %-9s %-8s %s\n", "SUTS", "UTS", "Grade", "Abbr / Short title");
  printf("-----------------------------------------------------------------\n");
  for (i = 0; i < SUTS_UTS_COUNT; ++i) {
    const suts_uts_entry_t *e = &SUTS_UTS_TABLE[i];
    const char *gname, *gsym;
    grade_describe(e->compat_grade, &gname, &gsym);
    printf("SUTS-%03d    UTS #%-4d  %s  %s — %s\n", e->su_number,
           e->unicode_number, gsym, e->su_abbr, e->su_title);
  }
  printf("=================================================================\n");
}

/* ------------------------------------------------------------------- test */

static int run_tests(void) {
  int failed = 0;

#define T_ASSERT(cond, msg)                                                    \
  do {                                                                         \
    if (!(cond)) {                                                             \
      fprintf(stderr, "FAIL: %s\n", msg);                                      \
      ++failed;                                                                \
    }                                                                          \
  } while (0)

  /* Table must have exactly 10 entries (SUTS-001 through SUTS-010). */
  T_ASSERT(SUTS_UTS_COUNT == 10, "SUTS_UTS_COUNT must be 10");

  /* No duplicate SUTS or UTS numbers. */
  {
    int i, j;
    for (i = 0; i < SUTS_UTS_COUNT; ++i) {
      for (j = i + 1; j < SUTS_UTS_COUNT; ++j) {
        T_ASSERT(SUTS_UTS_TABLE[i].su_number != SUTS_UTS_TABLE[j].su_number,
                 "Duplicate SUTS number");
        T_ASSERT(SUTS_UTS_TABLE[i].unicode_number !=
                     SUTS_UTS_TABLE[j].unicode_number,
                 "Duplicate UTS number");
      }
    }
  }

  /* Spot-check canonical pairs (from SUTS document §1.1 sections). */
  {
    const suts_uts_entry_t *e;

    e = suts_find_by_su(1);
    T_ASSERT(e != 0, "SUTS-001 must exist");
    T_ASSERT(e->unicode_number == 10, "SUTS-001 -> UTS #10 (Collation)");
    T_ASSERT(strcmp(e->su_abbr, "SUCA") == 0, "SUTS-001 abbr must be SUCA");

    e = suts_find_by_su(2);
    T_ASSERT(e && e->unicode_number == 18, "SUTS-002 -> UTS #18 (Regex)");

    e = suts_find_by_su(3);
    T_ASSERT(e && e->unicode_number == 35, "SUTS-003 -> UTS #35 (LDML)");

    e = suts_find_by_su(4);
    T_ASSERT(e && e->unicode_number == 37, "SUTS-004 -> UTS #37 (IVD)");

    e = suts_find_by_su(5);
    T_ASSERT(e && e->unicode_number == 39, "SUTS-005 -> UTS #39 (Security)");

    e = suts_find_by_su(6);
    T_ASSERT(e && e->unicode_number == 46, "SUTS-006 -> UTS #46 (IDNA)");

    e = suts_find_by_su(7);
    T_ASSERT(e && e->unicode_number == 51, "SUTS-007 -> UTS #51 (Emoji)");

    e = suts_find_by_su(8);
    T_ASSERT(e && e->unicode_number == 55, "SUTS-008 -> UTS #55 (Source Code)");

    e = suts_find_by_su(9);
    T_ASSERT(e && e->unicode_number == 58,
             "SUTS-009 -> UTS #58 (Link Detection)");

    e = suts_find_by_su(10);
    T_ASSERT(e && e->unicode_number == 61,
             "SUTS-010 -> UTS #61 (Set Notation)");
    T_ASSERT(strcmp(e->su_abbr, "SUSN") == 0, "SUTS-010 abbr must be SUSN");
  }

  /* Reverse lookup spot-checks. */
  {
    const suts_uts_entry_t *e;
    e = suts_find_by_uts(10);
    T_ASSERT(e && e->su_number == 1, "UTS #10 -> SUTS-001");
    e = suts_find_by_uts(51);
    T_ASSERT(e && e->su_number == 7, "UTS #51 -> SUTS-007");
    e = suts_find_by_uts(61);
    T_ASSERT(e && e->su_number == 10, "UTS #61 -> SUTS-010");
  }

  /* Grades are only Y, P, or E. */
  {
    int i;
    for (i = 0; i < SUTS_UTS_COUNT; ++i) {
      char g = SUTS_UTS_TABLE[i].compat_grade;
      T_ASSERT(g == 'Y' || g == 'P' || g == 'E', "Invalid compat grade");
    }
  }

  /* Out-of-range. */
  T_ASSERT(suts_find_by_su(0) == 0, "SUTS-0 must not exist");
  T_ASSERT(suts_find_by_su(11) == 0, "SUTS-011 outside covered range");
  T_ASSERT(suts_find_by_uts(1) == 0, "UTS #1 has no SUTS analog");

#undef T_ASSERT

  if (failed == 0) {
    printf("suts_to_uts: all tests passed.\n");
  } else {
    printf("suts_to_uts: %d test(s) FAILED.\n", failed);
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

  if (strcmp(argv[1], "suts") == 0 && argc == 3) {
    int n = atoi(argv[2]);
    const suts_uts_entry_t *e = suts_find_by_su(n);
    if (!e) {
      fprintf(stderr,
              "suts_to_uts: SUTS-%d is not in the covered range (1..10).\n", n);
      return 1;
    }
    printf("SUTS-%03d (%s) is analogous to UTS #%d \"%s\"\n", e->su_number,
           e->su_abbr, e->unicode_number, e->uc_title);
    return 0;
  }

  if (strcmp(argv[1], "uts") == 0 && argc == 3) {
    int n = atoi(argv[2]);
    const suts_uts_entry_t *e = suts_find_by_uts(n);
    if (!e) {
      fprintf(stderr,
              "suts_to_uts: UTS #%d has no SUTS analog in the covered range.\n",
              n);
      return 1;
    }
    printf("UTS #%d \"%s\" is analogous to SUTS-%03d (%s) \"%s\"\n",
           e->unicode_number, e->uc_title, e->su_number, e->su_abbr,
           e->su_title);
    return 0;
  }

  if (strcmp(argv[1], "compat") == 0 && argc == 4) {
    const suts_uts_entry_t *e = 0;
    if (strcmp(argv[2], "suts") == 0)
      e = suts_find_by_su(atoi(argv[3]));
    else if (strcmp(argv[2], "uts") == 0)
      e = suts_find_by_uts(atoi(argv[3]));
    if (!e) {
      fprintf(stderr, "suts_to_uts: document not found in covered range.\n");
      return 1;
    }
    print_suts_entry(e);
    return 0;
  }

  fprintf(stderr,
          "suts_to_uts — SUTS ↔ UTS converter & compat tester\n"
          "Usage:\n"
          "  suts_to_uts                      print full mapping table\n"
          "  suts_to_uts suts <N>             SUTS-N -> UTS #?\n"
          "  suts_to_uts uts  <N>             UTS #N -> SUTS-?\n"
          "  suts_to_uts compat suts <N>      compat grade for SUTS-N\n"
          "  suts_to_uts compat uts  <N>      compat grade for UTS #N\n"
          "  suts_to_uts test                 run self-checks\n"
        "Coverage: SUTS-001..010 / UTS #10,18,35,37,39,46,51,55,58,61\n");
        return 1;
}
