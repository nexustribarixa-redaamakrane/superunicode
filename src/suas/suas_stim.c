/* SUAS-019 - System Text Input Model (STIM)
 *
 * Reference implementation: an ordered layout, exact modifier matching, one
 * pending dead key, a bounded composition table, and all-or-nothing SUTF-8
 * emission. Freestanding C99, zero allocation.
 *
 * See docs/suas/SUAS-019-stim.md for the normative specification.
 */

#include "suas/suas_stim.h"

/* -- Helpers ---------------------------------------------------------- */

static size_t stim_utf8_len(uint32_t cp)
{
    if (cp <= 0x7Fu)      return 1u;
    if (cp <= 0x7FFu)     return 2u;
    if (cp <= 0xFFFFu)    return 3u;
    if (cp <= 0x1FFFFFu)  return 4u;
    if (cp <= 0x3FFFFFFu) return 5u;
    return 6u;
}

static size_t stim_utf8_put(uint32_t cp, uint8_t *out)
{
    if (cp <= 0x7Fu) {
        out[0] = (uint8_t)cp;
        return 1u;
    }
    if (cp <= 0x7FFu) {
        out[0] = (uint8_t)(0xC0u | (cp >> 6));
        out[1] = (uint8_t)(0x80u | (cp & 0x3Fu));
        return 2u;
    }
    if (cp <= 0xFFFFu) {
        out[0] = (uint8_t)(0xE0u | (cp >> 12));
        out[1] = (uint8_t)(0x80u | ((cp >> 6) & 0x3Fu));
        out[2] = (uint8_t)(0x80u | (cp & 0x3Fu));
        return 3u;
    }
    if (cp <= 0x1FFFFFu) {
        out[0] = (uint8_t)(0xF0u | (cp >> 18));
        out[1] = (uint8_t)(0x80u | ((cp >> 12) & 0x3Fu));
        out[2] = (uint8_t)(0x80u | ((cp >> 6) & 0x3Fu));
        out[3] = (uint8_t)(0x80u | (cp & 0x3Fu));
        return 4u;
    }
    if (cp <= 0x3FFFFFFu) {
        out[0] = (uint8_t)(0xF8u | (cp >> 24));
        out[1] = (uint8_t)(0x80u | ((cp >> 18) & 0x3Fu));
        out[2] = (uint8_t)(0x80u | ((cp >> 12) & 0x3Fu));
        out[3] = (uint8_t)(0x80u | ((cp >> 6) & 0x3Fu));
        out[4] = (uint8_t)(0x80u | (cp & 0x3Fu));
        return 5u;
    }
    out[0] = (uint8_t)(0xFCu | (cp >> 30));
    out[1] = (uint8_t)(0x80u | ((cp >> 24) & 0x3Fu));
    out[2] = (uint8_t)(0x80u | ((cp >> 18) & 0x3Fu));
    out[3] = (uint8_t)(0x80u | ((cp >> 12) & 0x3Fu));
    out[4] = (uint8_t)(0x80u | ((cp >> 6) & 0x3Fu));
    out[5] = (uint8_t)(0x80u | (cp & 0x3Fu));
    return 6u;
}

static size_t stim_capacity(size_t requested, size_t ceiling)
{
    return requested < ceiling ? requested : ceiling;
}

static const stim_binding_t *stim_find_binding(const stim_layout_t *layout,
                                               const stim_event_t *event)
{
    size_t i;

    if (layout == NULL || event == NULL) {
        return NULL;
    }
    for (i = 0u; i < layout->binding_count; i++) {
        const stim_binding_t *b = &layout->bindings[i];

        if (b->keycode != event->keycode) {
            continue;
        }
        if ((event->modifiers & b->required) != b->required) {
            continue;
        }
        if ((event->modifiers & b->forbidden) != 0u) {
            continue;
        }
        return b;
    }
    return NULL;
}

static bool stim_find_composition(const stim_layout_t *layout,
                                  uint32_t pending, uint32_t next,
                                  uint32_t *result)
{
    size_t i;

    if (layout == NULL) {
        return false;
    }
    for (i = 0u; i < layout->composition_count; i++) {
        const stim_composition_t *c = &layout->compositions[i];

        if (c->pending == pending && c->next == next) {
            if (result != NULL) {
                *result = c->result;
            }
            return true;
        }
    }
    return false;
}

static void stim_zero_output(stim_output_t *out_desc, uint8_t *out, size_t cap)
{
    if (out_desc != NULL) {
        out_desc->bytes = (cap == 0u) ? NULL : out;
        out_desc->len = 0u;
        out_desc->cap = cap;
        out_desc->cp_count = 0u;
        out_desc->flushed = false;
    }
}

static stim_status_t stim_emit(const uint32_t *cps, size_t count, bool flushed,
                               uint8_t *out, size_t cap, stim_output_t *out_desc)
{
    size_t i;
    size_t total = 0u;
    size_t pos = 0u;

    for (i = 0u; i < count; i++) {
        total += stim_utf8_len(cps[i]);
        if (total > cap) {
            stim_zero_output(out_desc, out, cap);
            return STIM_E_CAPACITY;
        }
    }

    for (i = 0u; i < count; i++) {
        pos += stim_utf8_put(cps[i], out + pos);
    }

    if (out_desc != NULL) {
        out_desc->bytes = out;
        out_desc->len = pos;
        out_desc->cap = cap;
        out_desc->cp_count = count;
        out_desc->flushed = flushed;
    }
    return STIM_OK;
}

/* -- Lifecycle -------------------------------------------------------- */

const char *suas_stim_version_string(void)
{
    return "SUAS-019 STIM 1.0.0";
}

const char *suas_stim_status_name(stim_status_t st)
{
    switch (st) {
    case STIM_OK:             return "STIM_OK";
    case STIM_E_NULL:         return "STIM_E_NULL";
    case STIM_E_ACTION:       return "STIM_E_ACTION";
    case STIM_E_BINDING:      return "STIM_E_BINDING";
    case STIM_E_FULL:         return "STIM_E_FULL";
    case STIM_E_CAPACITY:     return "STIM_E_CAPACITY";
    case STIM_E_LAYOUT:       return "STIM_E_LAYOUT";
    case STIM_E_INADMISSIBLE: return "STIM_E_INADMISSIBLE";
    default:                  return "STIM_E_UNKNOWN";
    }
}

bool suas_stim_is_admissible(uint32_t cp)
{
    if (cp >= 0x00110000u && cp <= 0x0011FFFFu) return false;
    if (cp >= 0x0000D800u && cp <= 0x0000DFFFu) return false;
    if (cp >= 0x7FFFFFF0u)                       return false;
    if (cp > 0x7FFFFFEFu)                        return false;
    return true;
}

stim_status_t suas_stim_layout_init(stim_layout_t *layout,
                                    stim_binding_t *bindings,
                                    size_t binding_cap,
                                    stim_composition_t *compositions,
                                    size_t composition_cap)
{
    if (layout == NULL) {
        return STIM_E_NULL;
    }
    if (bindings == NULL && binding_cap != 0u) {
        return STIM_E_NULL;
    }
    if (compositions == NULL && composition_cap != 0u) {
        return STIM_E_NULL;
    }

    layout->bindings = bindings;
    layout->binding_cap = stim_capacity(binding_cap, STIM_MAX_BINDINGS);
    layout->binding_count = 0u;
    layout->compositions = compositions;
    layout->composition_cap = stim_capacity(composition_cap,
                                            STIM_MAX_COMPOSITIONS);
    layout->composition_count = 0u;
    return STIM_OK;
}

stim_status_t suas_stim_layout_bind(stim_layout_t *layout,
                                    uint32_t keycode,
                                    uint16_t required,
                                    uint16_t forbidden,
                                    stim_bind_kind_t kind,
                                    const uint32_t *cps,
                                    size_t count)
{
    size_t i;

    if (layout == NULL || cps == NULL) {
        return STIM_E_NULL;
    }
    if (layout->bindings == NULL && layout->binding_cap == 0u) {
        return STIM_E_FULL;
    }
    if ((required & forbidden) != 0u) {
        return STIM_E_BINDING;
    }
    if ((required | forbidden) & (uint16_t)~STIM_MOD_ALL) {
        return STIM_E_BINDING;
    }
    if (kind != STIM_BIND_TEXT && kind != STIM_BIND_DEAD) {
        return STIM_E_BINDING;
    }
    if (count == 0u || count > STIM_MAX_SEQUENCE) {
        return STIM_E_BINDING;
    }
    if (kind == STIM_BIND_DEAD && count != 1u) {
        return STIM_E_BINDING;
    }
    for (i = 0u; i < count; i++) {
        if (!suas_stim_is_admissible(cps[i])) {
            return STIM_E_INADMISSIBLE;
        }
    }
    if (layout->binding_count >= layout->binding_cap) {
        return STIM_E_FULL;
    }

    for (i = 0u; i < layout->binding_count; i++) {
        const stim_binding_t *b = &layout->bindings[i];

        if (b->keycode == keycode &&
            b->required == required &&
            b->forbidden == forbidden) {
            return STIM_E_BINDING;
        }
    }

    {
        stim_binding_t *b = &layout->bindings[layout->binding_count];

        b->keycode = keycode;
        b->required = required;
        b->forbidden = forbidden;
        b->kind = (uint8_t)kind;
        b->count = (uint8_t)count;
        for (i = 0u; i < count; i++) {
            b->cps[i] = cps[i];
        }
        for (; i < STIM_MAX_SEQUENCE; i++) {
            b->cps[i] = 0u;
        }
    }
    layout->binding_count++;
    return STIM_OK;
}

stim_status_t suas_stim_layout_compose(stim_layout_t *layout,
                                       uint32_t pending,
                                       uint32_t next,
                                       uint32_t result)
{
    size_t i;

    if (layout == NULL) {
        return STIM_E_NULL;
    }
    if (!suas_stim_is_admissible(pending) ||
        !suas_stim_is_admissible(next) ||
        !suas_stim_is_admissible(result)) {
        return STIM_E_INADMISSIBLE;
    }
    if (layout->compositions == NULL && layout->composition_cap == 0u) {
        return STIM_E_FULL;
    }
    if (layout->composition_count >= layout->composition_cap) {
        return STIM_E_FULL;
    }
    for (i = 0u; i < layout->composition_count; i++) {
        const stim_composition_t *c = &layout->compositions[i];

        if (c->pending == pending && c->next == next) {
            return STIM_E_BINDING;
        }
    }

    layout->compositions[layout->composition_count].pending = pending;
    layout->compositions[layout->composition_count].next = next;
    layout->compositions[layout->composition_count].result = result;
    layout->composition_count++;
    return STIM_OK;
}

/* -- Input context ---------------------------------------------------- */

stim_status_t suas_stim_init(stim_state_t *state, const stim_layout_t *layout)
{
    if (state == NULL || layout == NULL) {
        return STIM_E_NULL;
    }
    state->layout = layout;
    state->pending = 0u;
    state->has_pending = false;
    state->events = 0u;
    state->emitted = 0u;
    return STIM_OK;
}

stim_status_t suas_stim_feed(stim_state_t *state, const stim_event_t *event,
                             uint8_t *out, size_t cap, stim_output_t *out_desc)
{
    const stim_binding_t *b;
    uint32_t seq[STIM_MAX_SEQUENCE + 1u];
    size_t seq_count = 0u;
    size_t start = 0u;
    bool flushed = false;
    stim_status_t st;

    if (state == NULL || state->layout == NULL || event == NULL) {
        return STIM_E_NULL;
    }
    if (out == NULL && cap != 0u) {
        return STIM_E_NULL;
    }
    if (event->action != (uint8_t)STIM_ACT_PRESS &&
        event->action != (uint8_t)STIM_ACT_RELEASE &&
        event->action != (uint8_t)STIM_ACT_REPEAT) {
        return STIM_E_ACTION;
    }

    state->events++;

    if (event->action == (uint8_t)STIM_ACT_RELEASE) {
        stim_zero_output(out_desc, out, cap);
        return STIM_OK;
    }

    b = stim_find_binding(state->layout, event);
    if (b == NULL) {
        /* An unmapped key emits nothing and changes no pending state. */
        stim_zero_output(out_desc, out, cap);
        return STIM_OK;
    }

    if (b->kind == (uint8_t)STIM_BIND_DEAD) {
        if (state->has_pending) {
            seq[seq_count++] = state->pending;
            flushed = true;
        }
        st = stim_emit(seq, seq_count, flushed, out, cap, out_desc);
        if (st != STIM_OK) {
            return st;
        }
        state->pending = b->cps[0];
        state->has_pending = true;
        if (seq_count != 0u) {
            state->emitted++;
        }
        return STIM_OK;
    }

    if (b->kind != (uint8_t)STIM_BIND_TEXT) {
        return STIM_E_BINDING;
    }

    if (state->has_pending) {
        uint32_t composed;

        if (stim_find_composition(state->layout, state->pending, b->cps[0],
                                  &composed)) {
            seq[seq_count++] = composed;
            start = 1u;
        } else {
            seq[seq_count++] = state->pending;
            flushed = true;
            start = 0u;
        }
    }

    {
        size_t i;

        for (i = start; i < (size_t)b->count; i++) {
            seq[seq_count++] = b->cps[i];
        }
    }

    st = stim_emit(seq, seq_count, flushed, out, cap, out_desc);
    if (st != STIM_OK) {
        return st;
    }

    state->pending = 0u;
    state->has_pending = false;
    if (seq_count != 0u) {
        state->emitted++;
    }
    return STIM_OK;
}

stim_status_t suas_stim_flush(stim_state_t *state,
                              uint8_t *out, size_t cap, stim_output_t *out_desc)
{
    stim_status_t st;

    if (state == NULL || state->layout == NULL) {
        return STIM_E_NULL;
    }
    if (out == NULL && cap != 0u) {
        return STIM_E_NULL;
    }
    if (!state->has_pending) {
        stim_zero_output(out_desc, out, cap);
        return STIM_OK;
    }

    st = stim_emit(&state->pending, 1u, true, out, cap, out_desc);
    if (st != STIM_OK) {
        return st;
    }

    state->pending = 0u;
    state->has_pending = false;
    state->emitted++;
    return STIM_OK;
}

void suas_stim_reset(stim_state_t *state)
{
    if (state != NULL) {
        state->pending = 0u;
        state->has_pending = false;
    }
}
