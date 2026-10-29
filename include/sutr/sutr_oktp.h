#ifndef SUTR_SUTR_OKTP_H
#define SUTR_SUTR_OKTP_H

/* SUTR-014 - OpenWindows Kernel Text Integration Profile (OKTP)
 *
 * An informative, machine-readable profile: the adoption matrix, the three
 * cumulative profile levels, the observed gaps, and the gates that can be run
 * to decide whether a claim is true. Every citation is checked against the
 * SUAS-010 registry; this report creates no requirement of its own.
 *
 * Zero allocation, freestanding C99.
 *
 * See docs/sutr/SUTR-014-oktp.md for the informative report.
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* -- Version ---------------------------------------------------------- */

#define SUTR_OKTP_SPEC_ID      "SUTR-014"
#define SUTR_OKTP_VERSION_MAJOR 1
#define SUTR_OKTP_VERSION_MINOR 0
#define SUTR_OKTP_VERSION_PATCH 0

#define OKTP_MAX_DOCS_PER_ROW  8u

/* -- Status ----------------------------------------------------------- */

typedef enum {
    OKTP_OK          = 0,
    OKTP_E_NULL      = 1,
    OKTP_E_RANGE     = 2,
    OKTP_E_CITATION  = 3, /* a cited document is not in the registry */
    OKTP_E_KIND      = 4, /* a cited document is registered, but its
                           * mandatory/optional/reference class does not
                           * match the family prefix                  */
    OKTP_E_PROFILE   = 5  /* profile levels are not cumulative       */
} oktp_status_t;

/* -- Adoption status (report section 2) ------------------------------- */

typedef enum {
    OKTP_ADOPT_LINKED    = 0,
    OKTP_ADOPT_PRESENT   = 1,
    OKTP_ADOPT_SPECIFIED = 2,
    OKTP_ADOPT_ABSENT    = 3
} oktp_adoption_t;

/* -- Adoption tier (report section 2) --------------------------------- */

typedef enum {
    OKTP_TIER_LINKED = 0, /* already on the build path            */
    OKTP_TIER_T0     = 1, /* adopt now                            */
    OKTP_TIER_T1     = 2, /* adopt with the feature               */
    OKTP_TIER_T2     = 3, /* deferred                             */
    OKTP_TIER_NA     = 4  /* out of scope for this kernel         */
} oktp_tier_t;

/* -- Matrix (report section 4) ---------------------------------------- */

typedef struct {
    const char *subsystem; /* file or component the row describes */
    const char *docs[OKTP_MAX_DOCS_PER_ROW]; /* cited spec ids      */
    uint8_t     doc_count;
    uint8_t     tier;      /* oktp_tier_t                         */
    const char *why;
} oktp_matrix_entry_t;

/* -- Profile levels (report section 5) -------------------------------- */

typedef enum {
    OKTP_LEVEL_KT0 = 0, /* encoding only                    */
    OKTP_LEVEL_KT1 = 1, /* bounded text (SUAS-018)          */
    OKTP_LEVEL_KT2 = 2  /* conformant console (SUTS-012)    */
} oktp_level_id_t;

#define OKTP_LEVEL_COUNT 3u

typedef struct {
    const char *id;        /* "KT-0"                            */
    const char *title;
    const char *docs[OKTP_MAX_DOCS_PER_ROW];
    uint8_t     doc_count;
    bool        met;       /* evidence at the time of writing   */
    const char *evidence;
} oktp_level_t;

/* -- Gaps (report section 6) ------------------------------------------ */

typedef struct {
    const char *id;        /* "G-1"                             */
    const char *summary;
    const char *closed_by; /* a registered spec id              */
    const char *evidence;
} oktp_gap_t;

/* -- Gates (report section 7) ----------------------------------------- */

typedef struct {
    const char *id;        /* "D-1"                             */
    const char *command;
    const char *proves;
} oktp_gate_t;

/* -- Introspection ---------------------------------------------------- */

const char *sutr_oktp_version_string(void);

const char *sutr_oktp_status_name(oktp_status_t st);
const char *sutr_oktp_adoption_name(oktp_adoption_t a);
const char *sutr_oktp_adoption_meaning(oktp_adoption_t a);
const char *sutr_oktp_tier_name(oktp_tier_t t);
const char *sutr_oktp_tier_meaning(oktp_tier_t t);
const char *sutr_oktp_level_name(oktp_level_id_t l);

/* -- Matrix ----------------------------------------------------------- */

size_t                      sutr_oktp_matrix_count(void);
const oktp_matrix_entry_t  *sutr_oktp_matrix_at(size_t index);
const oktp_matrix_entry_t  *sutr_oktp_matrix_find(const char *subsystem);

/* True when the row cites doc_id. A NULL row or id is not a citation. */
bool sutr_oktp_entry_cites(const oktp_matrix_entry_t *entry,
                           const char *doc_id);

/* How many matrix rows cite doc_id. O(n), n is small. */
size_t sutr_oktp_matrix_doc_count(const char *doc_id);

/* -- Levels, gaps, gates ---------------------------------------------- */

const oktp_level_t *sutr_oktp_level(oktp_level_id_t level);
const oktp_level_t *sutr_oktp_level_find(const char *id);
bool                sutr_oktp_level_met(oktp_level_id_t level);

size_t             sutr_oktp_gap_count(void);
const oktp_gap_t  *sutr_oktp_gap_at(size_t index);
const oktp_gap_t  *sutr_oktp_gap_find(const char *id);

size_t              sutr_oktp_gate_count(void);
const oktp_gate_t  *sutr_oktp_gate_at(size_t index);
const oktp_gate_t  *sutr_oktp_gate_find(const char *id);

/* -- Verification ----------------------------------------------------- */

/* Checks one citation against SUAS-010: it must be registered, and its
 * registered kind must match the family prefix (SUAS -> MANDATORY,
 * SUTS -> OPTIONAL, SUTR -> REFERENCE). */
oktp_status_t sutr_oktp_check_citation(const char *doc_id);

/* Checks every citation in the profile and the cumulative ordering of the
 * levels. On failure *out_doc (when non-NULL) receives the offending id or
 * NULL for a structural failure, and *out_index (when non-NULL) receives the
 * matrix/level/gap position, or 0 when not applicable. */
oktp_status_t sutr_oktp_verify(const char **out_doc, size_t *out_index);

#ifdef __cplusplus
}
#endif

#endif /* SUTR_SUTR_OKTP_H */
