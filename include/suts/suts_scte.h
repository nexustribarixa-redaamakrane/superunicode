#ifndef SUTS_SUTS_SCTE_H
#define SUTS_SUTS_SCTE_H

/* SUTS-013 - System Console Terminal Emulation (SCTE)
 *
 * The optional terminal contract over an SCTS plane: a bounded SUTF-8 parser,
 * the C0/ESC/CSI/OSC subsets, partial-sequence carry, and SCP directive
 * routing. Placement, width, breaks, and clusters remain SCTS and SUAS
 * decisions; SCTE only decides which control the bytes request.
 *
 * Zero allocation, freestanding C99.
 *
 * See docs/suts/SUTS-013-scte.md for the normative specification.
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "suts/suts_scts.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -- Version ---------------------------------------------------------- */

#define SUTS_SCTE_SPEC_ID      "SUTS-013"
#define SUTS_SCTE_VERSION_MAJOR 1
#define SUTS_SCTE_VERSION_MINOR 0
#define SUTS_SCTE_VERSION_PATCH 0

/* -- Fixed capacities (section 8) ------------------------------------- */

#define SCTE_MAX_PARAMS       16u
#define SCTE_MAX_OSC          128u
#define SCTE_MAX_PARAM_VALUE  65535u
#define SCTE_PARAM_MISSING    0xFFFFFFFFu
#define SCTE_CARRY_BYTES      8u

/* -- Status (section 7) ----------------------------------------------- */

typedef enum {
    SCTE_OK             = 0,
    SCTE_E_NULL         = 1,
    SCTE_E_PLANE        = 2,
    SCTE_E_ENCODING     = 3,
    SCTE_E_INCOMPLETE   = 4,
    SCTE_E_INTERNAL     = 5
} scte_status_t;

/* -- Parser states (section 4.2) -------------------------------------- */

typedef enum {
    SCTE_GROUND           = 0,
    SCTE_ESC              = 1,
    SCTE_CSI_ENTRY        = 2,
    SCTE_CSI_PARAM        = 3,
    SCTE_CSI_INTERMEDIATE = 4,
    SCTE_OSC              = 5,
    SCTE_OSC_ESC          = 6
} scte_state_t;

/* -- Callbacks -------------------------------------------------------- */

typedef void (*scte_scp_fn)(uintptr_t token, uint32_t cp);
typedef void (*scte_bell_fn)(uintptr_t token);

/* -- Terminal (section 4.1) ------------------------------------------- */

typedef struct {
    scts_plane_t     *plane;        /* borrowed; never owned              */
    scte_state_t      state;        /* parser state                       */
    uint8_t           carry[SCTE_CARRY_BYTES];
    uint8_t           carry_len;
    uint32_t          params[SCTE_MAX_PARAMS];
    uint8_t           param_count;
    uint8_t           prefix;       /* '?' / '>' / '!' or 0               */
    uint8_t           intermediate; /* one 0x20..0x2F byte or 0           */
    bool              have_param;    /* current parameter has digits      */
    bool              overflow;      /* too many params/intermediates     */
    uint16_t          saved_row;
    uint16_t          saved_col;
    bool              cursor_visible;
    bool              origin_mode;
    uint16_t          osc_len;       /* OSC bytes seen, saturated         */
    bool              osc_overflow;
    uint64_t          controls;      /* C0 and ESC executions             */
    uint64_t          sequences;     /* executed CSI sequences            */
    uint64_t          unsupported;   /* consumed but not executed         */
    uint64_t          directives;    /* SCP codepoints delivered/dropped  */
    uint64_t          encoding_errors;
    scte_scp_fn       scp;
    scte_bell_fn      bell;
    uintptr_t         token;
} scte_term_t;

/* -- Lifecycle -------------------------------------------------------- */

const char   *suts_scte_version_string(void);
const char   *suts_scte_status_name(scte_status_t st);

/* Construct a terminal over a borrowed plane. The plane must already be
 * initialised. Callbacks may be NULL. */
scte_status_t suts_scte_init(scte_term_t *term, scts_plane_t *plane,
                             scte_scp_fn scp, scte_bell_fn bell,
                             uintptr_t token);

/* Consume a byte stream. Returns SCTE_OK when every complete sequence was
 * executed or deliberately ignored. SCTE_E_ENCODING is recoverable. */
scte_status_t suts_scte_feed(scte_term_t *term, const uint8_t *bytes,
                             size_t len);

/* End-of-stream check. Returns SCTE_E_INCOMPLETE when the UTF-8 carry or the
 * parser state is not empty. */
scte_status_t suts_scte_finish(scte_term_t *term);

/* Discard parser state and the carry without emitting anything. */
void          suts_scte_reset(scte_term_t *term);

#ifdef __cplusplus
}
#endif

#endif /* SUTS_SUTS_SCTE_H */
