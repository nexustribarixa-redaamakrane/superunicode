/* Tests for SUAS-019 - System Text Input Model (STIM). */

#include <stdio.h>
#include <string.h>

#include "suas/suas_stim.h"

static int g_failures;

#define CHECK(cond) do {                                                     \
    if (!(cond)) {                                                           \
        printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);               \
        g_failures++;                                                        \
    }                                                                        \
} while (0)

static stim_event_t ev(uint32_t keycode, uint16_t mods, stim_action_t action)
{
    stim_event_t e;

    e.keycode = keycode;
    e.modifiers = mods;
    e.action = (uint8_t)action;
    e.reserved = 0u;
    return e;
}

static void test_version_and_admissibility(void)
{
    CHECK(strstr(suas_stim_version_string(), "SUAS-019") != NULL);
    CHECK(strcmp(suas_stim_status_name(STIM_OK), "STIM_OK") == 0);
    CHECK(strcmp(suas_stim_status_name(STIM_E_CAPACITY),
                 "STIM_E_CAPACITY") == 0);
    CHECK(strcmp(suas_stim_status_name((stim_status_t)99),
                 "STIM_E_UNKNOWN") == 0);

    CHECK(suas_stim_is_admissible(0x41u));
    CHECK(suas_stim_is_admissible(0x10FFFFu));
    CHECK(suas_stim_is_admissible(0x7FFFFFEFu));
    CHECK(!suas_stim_is_admissible(0xD800u));
    CHECK(!suas_stim_is_admissible(0xDFFFu));
    CHECK(!suas_stim_is_admissible(0x110000u));
    CHECK(!suas_stim_is_admissible(0x11FFFFu));
    CHECK(!suas_stim_is_admissible(0x7FFFFFF0u));
    CHECK(!suas_stim_is_admissible(0x7FFFFFFFu));
}

static void test_layout_validation(void)
{
    stim_binding_t binds[4];
    stim_composition_t comps[4];
    stim_layout_t layout;
    const uint32_t a[1] = { 0x61u };
    const uint32_t nine[9] = { 0x61u, 0x62u, 0x63u, 0x64u, 0x65u,
                               0x66u, 0x67u, 0x68u, 0x69u };
    const uint32_t bad[1] = { 0x110000u };

    CHECK(suas_stim_layout_init(NULL, binds, 4u, comps, 4u) == STIM_E_NULL);
    CHECK(suas_stim_layout_init(&layout, NULL, 1u, comps, 4u) == STIM_E_NULL);
    CHECK(suas_stim_layout_init(&layout, binds, 4u, NULL, 1u) == STIM_E_NULL);
    CHECK(suas_stim_layout_init(&layout, binds, 4u, comps, 4u) == STIM_OK);
    CHECK(layout.binding_count == 0u && layout.composition_count == 0u);

    CHECK(suas_stim_layout_bind(&layout, 1u, 0u, 0u, STIM_BIND_TEXT,
                                a, 1u) == STIM_OK);
    CHECK(suas_stim_layout_bind(&layout, 1u, 0u, 0u, STIM_BIND_TEXT,
                                a, 1u) == STIM_E_BINDING);
    CHECK(suas_stim_layout_bind(&layout, 2u, STIM_MOD_SHIFT, STIM_MOD_SHIFT,
                                STIM_BIND_TEXT, a, 1u) == STIM_E_BINDING);
    CHECK(suas_stim_layout_bind(&layout, 3u, 0u, 0u, STIM_BIND_TEXT,
                                nine, 9u) == STIM_E_BINDING);
    CHECK(suas_stim_layout_bind(&layout, 4u, 0u, 0u, STIM_BIND_TEXT,
                                NULL, 1u) == STIM_E_NULL);
    CHECK(suas_stim_layout_bind(&layout, 5u, 0u, 0u, STIM_BIND_DEAD,
                                nine, 2u) == STIM_E_BINDING);
    CHECK(suas_stim_layout_bind(&layout, 6u, 0u, 0u, (stim_bind_kind_t)9,
                                a, 1u) == STIM_E_BINDING);
    CHECK(suas_stim_layout_bind(&layout, 7u, 0u, 0u, STIM_BIND_TEXT,
                                bad, 1u) == STIM_E_INADMISSIBLE);

    CHECK(suas_stim_layout_compose(&layout, 0x60u, 0x61u, 0xE0u) == STIM_OK);
    CHECK(suas_stim_layout_compose(&layout, 0x60u, 0x61u, 0xE1u)
          == STIM_E_BINDING);
    CHECK(suas_stim_layout_compose(&layout, 0x60u, 0x61u, 0x110000u)
          == STIM_E_INADMISSIBLE);
}

static void test_layout_capacity(void)
{
    stim_binding_t binds[1];
    stim_composition_t comps[1];
    stim_layout_t layout;
    const uint32_t a[1] = { 0x61u };
    const uint32_t b[1] = { 0x62u };

    CHECK(suas_stim_layout_init(&layout, binds, 1u, comps, 1u) == STIM_OK);
    CHECK(suas_stim_layout_bind(&layout, 1u, 0u, 0u, STIM_BIND_TEXT,
                                a, 1u) == STIM_OK);
    CHECK(suas_stim_layout_bind(&layout, 2u, 0u, 0u, STIM_BIND_TEXT,
                                b, 1u) == STIM_E_FULL);
    CHECK(suas_stim_layout_compose(&layout, 0x60u, 0x61u, 0xE0u) == STIM_OK);
    CHECK(suas_stim_layout_compose(&layout, 0x60u, 0x62u, 0xE1u) == STIM_E_FULL);
}

static void test_basic_events(void)
{
    stim_binding_t binds[6];
    stim_composition_t comps[2];
    stim_layout_t layout;
    stim_state_t state;
    stim_output_t out;
    uint8_t buf[16];
    const uint32_t a[1] = { 0x61u };
    const uint32_t b[1] = { 0x62u };
    const uint32_t ab[2] = { 0x61u, 0x62u };
    const uint32_t upper[1] = { 0x41u };
    const uint32_t x[1] = { 0x78u };
    stim_event_t e;

    CHECK(suas_stim_layout_init(&layout, binds, 6u, comps, 2u) == STIM_OK);
    CHECK(suas_stim_layout_bind(&layout, 1u, 0u, 0u, STIM_BIND_TEXT,
                                a, 1u) == STIM_OK);
    CHECK(suas_stim_layout_bind(&layout, 2u, 0u, 0u, STIM_BIND_TEXT,
                                b, 1u) == STIM_OK);
    CHECK(suas_stim_layout_bind(&layout, 3u, 0u, 0u, STIM_BIND_TEXT,
                                ab, 2u) == STIM_OK);
    CHECK(suas_stim_layout_bind(&layout, 4u, STIM_MOD_SHIFT, 0u,
                                STIM_BIND_TEXT, upper, 1u) == STIM_OK);
    CHECK(suas_stim_layout_bind(&layout, 5u, 0u, STIM_MOD_CAPS,
                                STIM_BIND_TEXT, x, 1u) == STIM_OK);

    CHECK(suas_stim_init(NULL, &layout) == STIM_E_NULL);
    CHECK(suas_stim_init(&state, NULL) == STIM_E_NULL);
    CHECK(suas_stim_init(&state, &layout) == STIM_OK);

    e = ev(1u, 0u, STIM_ACT_PRESS);
    CHECK(suas_stim_feed(&state, &e, buf, sizeof buf, &out) == STIM_OK);
    CHECK(out.len == 1u && buf[0] == 'a' && out.cp_count == 1u);
    CHECK(!out.flushed);

    e = ev(3u, 0u, STIM_ACT_PRESS);
    CHECK(suas_stim_feed(&state, &e, buf, sizeof buf, &out) == STIM_OK);
    CHECK(out.len == 2u && buf[0] == 'a' && buf[1] == 'b');
    CHECK(out.cp_count == 2u);

    e = ev(99u, 0u, STIM_ACT_PRESS);
    CHECK(suas_stim_feed(&state, &e, buf, sizeof buf, &out) == STIM_OK);
    CHECK(out.len == 0u && out.cp_count == 0u);

    e = ev(1u, 0u, STIM_ACT_RELEASE);
    CHECK(suas_stim_feed(&state, &e, buf, sizeof buf, &out) == STIM_OK);
    CHECK(out.len == 0u);

    e = ev(1u, 0u, STIM_ACT_REPEAT);
    CHECK(suas_stim_feed(&state, &e, buf, sizeof buf, &out) == STIM_OK);
    CHECK(out.len == 1u && buf[0] == 'a');

    e = ev(4u, STIM_MOD_SHIFT, STIM_ACT_PRESS);
    CHECK(suas_stim_feed(&state, &e, buf, sizeof buf, &out) == STIM_OK);
    CHECK(out.len == 1u && buf[0] == 'A');

    e = ev(4u, STIM_MOD_SHIFT | STIM_MOD_CTRL, STIM_ACT_PRESS);
    CHECK(suas_stim_feed(&state, &e, buf, sizeof buf, &out) == STIM_OK);
    CHECK(out.len == 1u && buf[0] == 'A');

    e = ev(4u, 0u, STIM_ACT_PRESS);
    CHECK(suas_stim_feed(&state, &e, buf, sizeof buf, &out) == STIM_OK);
    CHECK(out.len == 0u);

    e = ev(5u, 0u, STIM_ACT_PRESS);
    CHECK(suas_stim_feed(&state, &e, buf, sizeof buf, &out) == STIM_OK);
    CHECK(out.len == 1u && buf[0] == 'x');

    e = ev(5u, STIM_MOD_CAPS, STIM_ACT_PRESS);
    CHECK(suas_stim_feed(&state, &e, buf, sizeof buf, &out) == STIM_OK);
    CHECK(out.len == 0u);

    e = ev(1u, 0u, (stim_action_t)9);
    CHECK(suas_stim_feed(&state, &e, buf, sizeof buf, &out) == STIM_E_ACTION);
}

static void test_dead_keys(void)
{
    stim_binding_t binds[5];
    stim_composition_t comps[2];
    stim_layout_t layout;
    stim_state_t state;
    stim_output_t out;
    uint8_t buf[16];
    const uint32_t grave[1] = { 0x60u };
    const uint32_t a[1] = { 0x61u };
    const uint32_t b[1] = { 0x62u };
    const uint32_t ab[2] = { 0x61u, 0x62u };
    stim_event_t e;

    CHECK(suas_stim_layout_init(&layout, binds, 5u, comps, 2u) == STIM_OK);
    CHECK(suas_stim_layout_bind(&layout, 10u, 0u, 0u, STIM_BIND_DEAD,
                                grave, 1u) == STIM_OK);
    CHECK(suas_stim_layout_bind(&layout, 11u, 0u, 0u, STIM_BIND_TEXT,
                                a, 1u) == STIM_OK);
    CHECK(suas_stim_layout_bind(&layout, 12u, 0u, 0u, STIM_BIND_TEXT,
                                b, 1u) == STIM_OK);
    CHECK(suas_stim_layout_bind(&layout, 13u, 0u, 0u, STIM_BIND_TEXT,
                                ab, 2u) == STIM_OK);
    CHECK(suas_stim_layout_compose(&layout, 0x60u, 0x61u, 0xE0u) == STIM_OK);
    CHECK(suas_stim_init(&state, &layout) == STIM_OK);

    /* Dead key alone: nothing emitted, pending set. */
    e = ev(10u, 0u, STIM_ACT_PRESS);
    CHECK(suas_stim_feed(&state, &e, buf, sizeof buf, &out) == STIM_OK);
    CHECK(out.len == 0u && state.has_pending && state.pending == 0x60u);

    /* Composes with the next base. */
    e = ev(11u, 0u, STIM_ACT_PRESS);
    CHECK(suas_stim_feed(&state, &e, buf, sizeof buf, &out) == STIM_OK);
    CHECK(out.len == 2u && buf[0] == 0xC3u && buf[1] == 0xA0u);
    CHECK(out.cp_count == 1u && !out.flushed && !state.has_pending);

    /* No composition: the pending spacing form flushes, then the base. */
    e = ev(10u, 0u, STIM_ACT_PRESS);
    CHECK(suas_stim_feed(&state, &e, buf, sizeof buf, &out) == STIM_OK);
    e = ev(12u, 0u, STIM_ACT_PRESS);
    CHECK(suas_stim_feed(&state, &e, buf, sizeof buf, &out) == STIM_OK);
    CHECK(out.len == 2u && buf[0] == 0x60u && buf[1] == 'b');
    CHECK(out.cp_count == 2u && out.flushed && !state.has_pending);

    /* A composed sequence keeps its tail: the head composes, the rest follows. */
    e = ev(10u, 0u, STIM_ACT_PRESS);
    CHECK(suas_stim_feed(&state, &e, buf, sizeof buf, &out) == STIM_OK);
    e = ev(13u, 0u, STIM_ACT_PRESS);
    CHECK(suas_stim_feed(&state, &e, buf, sizeof buf, &out) == STIM_OK);
    CHECK(out.len == 3u && buf[0] == 0xC3u && buf[1] == 0xA0u && buf[2] == 'b');
    CHECK(out.cp_count == 2u && !out.flushed && !state.has_pending);

    /* Two dead keys: the first flushes. */
    e = ev(10u, 0u, STIM_ACT_PRESS);
    CHECK(suas_stim_feed(&state, &e, buf, sizeof buf, &out) == STIM_OK);
    CHECK(out.len == 0u);
    CHECK(suas_stim_feed(&state, &e, buf, sizeof buf, &out) == STIM_OK);
    CHECK(out.len == 1u && buf[0] == 0x60u && out.flushed);
    CHECK(state.has_pending && state.pending == 0x60u);
}

static void test_flush_reset_and_capacity(void)
{
    stim_binding_t binds[2];
    stim_composition_t comps[2];
    stim_layout_t layout;
    stim_state_t state;
    stim_output_t out;
    uint8_t buf[8];
    const uint32_t grave[1] = { 0x60u };
    const uint32_t a[1] = { 0x61u };
    stim_event_t e;

    CHECK(suas_stim_layout_init(&layout, binds, 2u, comps, 2u) == STIM_OK);
    CHECK(suas_stim_layout_bind(&layout, 10u, 0u, 0u, STIM_BIND_DEAD,
                                grave, 1u) == STIM_OK);
    CHECK(suas_stim_layout_bind(&layout, 11u, 0u, 0u, STIM_BIND_TEXT,
                                a, 1u) == STIM_OK);
    CHECK(suas_stim_layout_compose(&layout, 0x60u, 0x61u, 0xE0u) == STIM_OK);
    CHECK(suas_stim_init(&state, &layout) == STIM_OK);

    /* Idle flush is a no-op. */
    CHECK(suas_stim_flush(&state, buf, sizeof buf, &out) == STIM_OK);
    CHECK(out.len == 0u && !out.flushed);

    e = ev(10u, 0u, STIM_ACT_PRESS);
    CHECK(suas_stim_feed(&state, &e, buf, sizeof buf, &out) == STIM_OK);
    CHECK(suas_stim_flush(&state, buf, sizeof buf, &out) == STIM_OK);
    CHECK(out.len == 1u && buf[0] == 0x60u && out.flushed);
    CHECK(!state.has_pending);
    CHECK(suas_stim_flush(&state, buf, sizeof buf, &out) == STIM_OK);
    CHECK(out.len == 0u);

    /* Reset discards without emitting. */
    e = ev(10u, 0u, STIM_ACT_PRESS);
    CHECK(suas_stim_feed(&state, &e, buf, sizeof buf, &out) == STIM_OK);
    suas_stim_reset(&state);
    CHECK(!state.has_pending);
    e = ev(11u, 0u, STIM_ACT_PRESS);
    CHECK(suas_stim_feed(&state, &e, buf, sizeof buf, &out) == STIM_OK);
    CHECK(out.len == 1u && buf[0] == 'a');

    /* Capacity failure leaves pending state untouched. */
    e = ev(10u, 0u, STIM_ACT_PRESS);
    CHECK(suas_stim_feed(&state, &e, buf, sizeof buf, &out) == STIM_OK);
    e = ev(11u, 0u, STIM_ACT_PRESS);
    CHECK(suas_stim_feed(&state, &e, buf, 1u, &out) == STIM_E_CAPACITY);
    CHECK(out.len == 0u && out.cp_count == 0u);
    CHECK(state.has_pending && state.pending == 0x60u);
    CHECK(suas_stim_feed(&state, &e, buf, 2u, &out) == STIM_OK);
    CHECK(out.len == 2u && buf[0] == 0xC3u && buf[1] == 0xA0u);
    CHECK(!state.has_pending);
}

static void test_determinism(void)
{
    stim_binding_t binds[4];
    stim_composition_t comps[2];
    stim_layout_t layout;
    stim_state_t a, b;
    stim_output_t oa, ob;
    uint8_t ba[32], bb[32];
    const uint32_t grave[1] = { 0x60u };
    const uint32_t x[2] = { 0x61u, 0x62u };
    const uint32_t y[1] = { 0x63u };
    stim_event_t e;
    size_t i;

    CHECK(suas_stim_layout_init(&layout, binds, 4u, comps, 2u) == STIM_OK);
    CHECK(suas_stim_layout_bind(&layout, 1u, 0u, 0u, STIM_BIND_DEAD,
                                grave, 1u) == STIM_OK);
    CHECK(suas_stim_layout_bind(&layout, 2u, 0u, 0u, STIM_BIND_TEXT,
                                x, 2u) == STIM_OK);
    CHECK(suas_stim_layout_bind(&layout, 3u, 0u, 0u, STIM_BIND_TEXT,
                                y, 1u) == STIM_OK);
    CHECK(suas_stim_layout_compose(&layout, 0x60u, 0x61u, 0xE0u) == STIM_OK);
    CHECK(suas_stim_init(&a, &layout) == STIM_OK);
    CHECK(suas_stim_init(&b, &layout) == STIM_OK);

    for (i = 0u; i < 3u; i++) {
        e = ev(1u, 0u, STIM_ACT_PRESS);
        CHECK(suas_stim_feed(&a, &e, ba, sizeof ba, &oa) == STIM_OK);
        CHECK(suas_stim_feed(&b, &e, bb, sizeof bb, &ob) == STIM_OK);
        CHECK(oa.len == ob.len && memcmp(ba, bb, oa.len) == 0);

        e = ev(2u, 0u, STIM_ACT_PRESS);
        CHECK(suas_stim_feed(&a, &e, ba, sizeof ba, &oa) == STIM_OK);
        CHECK(suas_stim_feed(&b, &e, bb, sizeof bb, &ob) == STIM_OK);
        CHECK(oa.len == ob.len && memcmp(ba, bb, oa.len) == 0);

        e = ev(3u, 0u, STIM_ACT_PRESS);
        CHECK(suas_stim_feed(&a, &e, ba, sizeof ba, &oa) == STIM_OK);
        CHECK(suas_stim_feed(&b, &e, bb, sizeof bb, &ob) == STIM_OK);
        CHECK(oa.len == ob.len && memcmp(ba, bb, oa.len) == 0);
    }
    CHECK(a.emitted == b.emitted);
    CHECK(a.events == b.events);
}

int main(void)
{
    test_version_and_admissibility();
    test_layout_validation();
    test_layout_capacity();
    test_basic_events();
    test_dead_keys();
    test_flush_reset_and_capacity();
    test_determinism();

    if (g_failures != 0) {
        printf("%d STIM test(s) failed\n", g_failures);
        return 1;
    }
    printf("[PASS] SUAS-019 STIM tests\n");
    return 0;
}
