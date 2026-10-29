#ifndef SUAS_SUAS_STIM_H
#define SUAS_SUAS_STIM_H

/* SUAS-019 - System Text Input Model (STIM)
 *
 * Deterministic resolution of key events into bounded SUTF-8 text: an ordered
 * layout, exact modifier matching, one pending dead key, a bounded composition
 * table, and all-or-nothing emission. No allocation.
 *
 * See docs/suas/SUAS-019-stim.md for the normative specification.
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* -- Version ---------------------------------------------------------- */

#define SUAS_STIM_SPEC_ID      "SUAS-019"
#define SUAS_STIM_VERSION_MAJOR 1
#define SUAS_STIM_VERSION_MINOR 0
#define SUAS_STIM_VERSION_PATCH 0

/* -- Fixed capacities (section 9) ------------------------------------- */

#define STIM_MAX_SEQUENCE      8u
#define STIM_MAX_BINDINGS      128u
#define STIM_MAX_COMPOSITIONS  128u

/* -- Modifiers -------------------------------------------------------- */

#define STIM_MOD_SHIFT  0x0001u
#define STIM_MOD_CTRL   0x0002u
#define STIM_MOD_ALT    0x0004u
#define STIM_MOD_META   0x0008u
#define STIM_MOD_CAPS   0x0010u
#define STIM_MOD_NUM    0x0020u
#define STIM_MOD_ALTGR  0x0040u
#define STIM_MOD_ALL    0x007Fu

/* -- Types ------------------------------------------------------------ */

typedef enum {
    STIM_ACT_PRESS   = 0,
    STIM_ACT_RELEASE = 1,
    STIM_ACT_REPEAT  = 2
} stim_action_t;

typedef enum {
    STIM_BIND_TEXT = 0,
    STIM_BIND_DEAD = 1
} stim_bind_kind_t;

typedef enum {
    STIM_OK              = 0,
    STIM_E_NULL          = 1,
    STIM_E_ACTION        = 2,
    STIM_E_BINDING       = 3,
    STIM_E_FULL          = 4,
    STIM_E_CAPACITY      = 5,
    STIM_E_LAYOUT        = 6,
    STIM_E_INADMISSIBLE  = 7
} stim_status_t;

typedef struct {
    uint32_t keycode;    /* physical key identity, layout-defined */
    uint16_t modifiers;  /* STIM_MOD_*                            */
    uint8_t  action;     /* stim_action_t                         */
    uint8_t  reserved;   /* MUST be zero                          */
} stim_event_t;

typedef struct {
    uint32_t keycode;
    uint16_t required;       /* modifiers that must be present */
    uint16_t forbidden;      /* modifiers that must be absent  */
    uint8_t  kind;           /* stim_bind_kind_t               */
    uint8_t  count;          /* 1..STIM_MAX_SEQUENCE           */
    uint32_t cps[STIM_MAX_SEQUENCE];
} stim_binding_t;

typedef struct {
    uint32_t pending;  /* spacing form of the dead key       */
    uint32_t next;     /* first codepoint of the text binding */
    uint32_t result;   /* composed codepoint                  */
} stim_composition_t;

typedef struct {
    stim_binding_t     *bindings;
    size_t              binding_cap;
    size_t              binding_count;
    stim_composition_t *compositions;
    size_t              composition_cap;
    size_t              composition_count;
} stim_layout_t;

typedef struct {
    const stim_layout_t *layout;
    uint32_t             pending;
    bool                 has_pending;
    uint64_t             events;
    uint64_t             emitted;
} stim_state_t;

typedef struct {
    const uint8_t *bytes;    /* the caller buffer, or NULL when len == 0 */
    size_t         len;      /* bytes written                           */
    size_t         cap;      /* caller capacity                         */
    size_t         cp_count; /* codepoints represented                  */
    bool           flushed;  /* a dead key was flushed, not composed    */
} stim_output_t;

/* -- Lifecycle -------------------------------------------------------- */

const char   *suas_stim_version_string(void);
const char   *suas_stim_status_name(stim_status_t st);

/* Admissibility test of section 8. */
bool          suas_stim_is_admissible(uint32_t cp);

/* Initialise a layout over caller storage. Either array may be NULL when its
 * capacity is zero. Capacities above the header ceiling are clamped. */
stim_status_t suas_stim_layout_init(stim_layout_t *layout,
                                    stim_binding_t *bindings,
                                    size_t binding_cap,
                                    stim_composition_t *compositions,
                                    size_t composition_cap);

/* Add a binding. Duplicate (keycode, required, forbidden) is rejected. */
stim_status_t suas_stim_layout_bind(stim_layout_t *layout,
                                    uint32_t keycode,
                                    uint16_t required,
                                    uint16_t forbidden,
                                    stim_bind_kind_t kind,
                                    const uint32_t *cps,
                                    size_t count);

/* Add a composition. A duplicate (pending, next) pair is rejected. */
stim_status_t suas_stim_layout_compose(stim_layout_t *layout,
                                       uint32_t pending,
                                       uint32_t next,
                                       uint32_t result);

/* -- Input context ---------------------------------------------------- */

stim_status_t suas_stim_init(stim_state_t *state, const stim_layout_t *layout);

/* Resolve one event. On STIM_E_CAPACITY, out is zeroed and state is unchanged. */
stim_status_t suas_stim_feed(stim_state_t *state, const stim_event_t *event,
                             uint8_t *out, size_t cap, stim_output_t *out_desc);

/* Emit a pending dead key as its spacing form. Idle flush is a no-op. */
stim_status_t suas_stim_flush(stim_state_t *state,
                              uint8_t *out, size_t cap, stim_output_t *out_desc);

/* Discard pending state without emitting it. */
void          suas_stim_reset(stim_state_t *state);

#ifdef __cplusplus
}
#endif

#endif /* SUAS_SUAS_STIM_H */
