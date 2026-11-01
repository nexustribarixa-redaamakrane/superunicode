/**
 * KT-1 host test: SKTO admission/publication at the OpenWindows name boundary.
 *
 * Exercises the compatibility adapter (compat/ow_skto_name.h) and the
 * non-truncating ow_sutf8_name_copy() replacement:
 *
 *   1. invalid input is refused;
 *   2. an over-ceiling name is refused, never truncated;
 *   3. copying is bounded and the caller's buffer is not retained;
 *   4. a valid name is published and resolved byte-exactly;
 *   5. publication stamps a new generation;
 *   6. a resolved snapshot survives one later publication, and its retired
 *      generation is rejected by resolve_at();
 *   7. a failed publication leaves the store unchanged.
 *
 * This is host-side evidence only. It does not exercise an OpenWindows kernel,
 * a real address-space containment callback, or concurrent publication.
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "ow_skto_name.h"
#include "ow_string.h"
#include "owfs_types.h"
#include "usfs_types.h"

static int g_fail = 0;

#define CHECK(cond) do { if (!(cond)) { \
    printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); g_fail = 1; \
} } while (0)

typedef struct {
    const void *base;
    size_t len;
    int allow;
    int calls;
} range_probe_t;

static bool probe_range(void *ctx, const void *base, size_t len)
{
    range_probe_t *p = (range_probe_t *)ctx;
    p->calls++;
    return p->allow && base == p->base && len == p->len;
}

static void test_store_geometry(void)
{
    ow_skto_name_store_t store;

    CHECK(OWFS_NAME_MAX_BYTES == 128);
    CHECK(USFS_NAME_MAX_BYTES == 128);
    CHECK(OW_SKTO_NAME_MAX_BYTES == (size_t)(OWFS_NAME_MAX_BYTES - 1));
    CHECK(SKTO_NAME_MAX_BYTES >= OWFS_NAME_MAX_BYTES);

    CHECK(ow_skto_name_store_init(NULL) == SKTO_E_NULL);
    CHECK(ow_skto_name_store_init(&store) == SKTO_OK);
    CHECK(store.table.live == 0);
    CHECK(store.limits.max_name_bytes == OW_SKTO_NAME_MAX_BYTES);
    CHECK(store.limits.max_admit_bytes == (size_t)SKTO_MAX_ADMIT_BYTES);

    /* Struct layout and ABI offsets */
    CHECK(sizeof(skto_name_snapshot_t) == 144u);
    CHECK(sizeof(skto_name_slot_t) == 580u);
    CHECK(offsetof(skto_name_slot_t, retired) == 578u);

    if (sizeof(void *) == 8u) {
        CHECK(sizeof(skto_text_t) == 40u);
        CHECK(sizeof(skto_name_table_t) == 9312u);
        CHECK(sizeof(ow_skto_name_store_t) == 9328u);
        CHECK(offsetof(ow_skto_name_store_t, table) == 0u);
        CHECK(offsetof(ow_skto_name_store_t, limits) == 9312u);
    } else if (sizeof(void *) == 4u) {
        CHECK(sizeof(skto_text_t) == 24u);
        CHECK(sizeof(skto_name_table_t) == 9296u);
        CHECK(sizeof(ow_skto_name_store_t) == 9304u);
        CHECK(offsetof(ow_skto_name_store_t, table) == 0u);
        CHECK(offsetof(ow_skto_name_store_t, limits) == 9296u);
    }

}

static void test_invalid_input_refused(void)
{
    ow_skto_name_store_t store;
    uint32_t gen = 99u;
    uint8_t overlong[2] = { 0xC0u, 0x80u };
    uint8_t truncated[2] = { 0xE2u, 0x82u };
    uint8_t scp[5] = { 0xF8u, 0x84u, 0x90u, 0x80u, 0x80u };
    uint8_t interior_nul[3] = { 'a', 0x00u, 'b' };

    CHECK(ow_skto_name_store_init(&store) == SKTO_OK);

    CHECK(ow_skto_name_publish(NULL, "x", 1, NULL, NULL, &gen) ==
          SKTO_E_NULL);
    CHECK(gen == 0u);

    gen = 99u;
    CHECK(ow_skto_name_publish(&store, NULL, 3, NULL, NULL, &gen) ==
          SKTO_E_NULL);
    CHECK(gen == 0u);

    CHECK(ow_skto_name_publish(&store, overlong, sizeof overlong,
                               NULL, NULL, &gen) == SKTO_E_ENCODING);
    CHECK(ow_skto_name_publish(&store, truncated, sizeof truncated,
                               NULL, NULL, &gen) == SKTO_E_ENCODING);
    CHECK(ow_skto_name_publish(&store, scp, sizeof scp,
                               NULL, NULL, &gen) == SKTO_E_INADMISSIBLE);
    CHECK(ow_skto_name_publish(&store, interior_nul, sizeof interior_nul,
                               NULL, NULL, &gen) == SKTO_E_POLICY);

    /* Nothing was admitted, and the failed generations report zero. */
    CHECK(store.table.live == 0u);
    CHECK(gen == 0u);
}

static void test_over_ceiling_refused(void)
{
    ow_skto_name_store_t store;
    uint8_t max_name[OWFS_NAME_MAX_BYTES];
    uint8_t too_long[OWFS_NAME_MAX_BYTES];
    uint32_t gen = 0u;

    memset(max_name, 'm', sizeof max_name - 1u);
    max_name[sizeof max_name - 1u] = 0u;
    memset(too_long, 't', sizeof too_long);

    CHECK(ow_skto_name_store_init(&store) == SKTO_OK);

    /* Exactly the namespace ceiling is accepted in full. */
    CHECK(ow_skto_name_publish(&store, max_name,
                               (size_t)(OWFS_NAME_MAX_BYTES - 1u),
                               NULL, NULL, &gen) == SKTO_OK);
    CHECK(gen == 0u);

    /* One byte more is refused as policy, not shortened. */
    gen = 99u;
    CHECK(ow_skto_name_publish(&store, too_long, sizeof too_long,
                               NULL, NULL, &gen) == SKTO_E_POLICY);
    CHECK(gen == 0u);
    CHECK(store.table.live == 1u);
}

static void test_bounded_copy_and_containment(void)
{
    ow_skto_name_store_t store;
    skto_text_t view;
    uint8_t mutable_name[16];
    uint8_t long_name[SKTO_NAME_MAX_BYTES];
    uint32_t gen = 0u;
    uint32_t view_gen = 0u;
    range_probe_t probe;

    CHECK(ow_skto_name_store_init(&store) == SKTO_OK);
    memcpy(mutable_name, "alpha", 5u);

    probe.base = mutable_name;
    probe.len = 5u;
    probe.allow = 1;
    probe.calls = 0;

    CHECK(ow_skto_name_publish(&store, mutable_name, 5u, probe_range, &probe,
                               &gen) == SKTO_OK);
    CHECK(probe.calls == 1);
    CHECK(gen == 0u);

    /* Publication copied the bytes; the caller may now reuse its buffer. */
    memset(mutable_name, 'z', sizeof mutable_name);

    memset(&view, 0xFF, sizeof view);
    CHECK(ow_skto_name_resolve(&store, "alpha", 5u, &view_gen, &view) ==
          SKTO_OK);
    CHECK(view_gen == 0u);
    CHECK(view.len == 5u);
    CHECK(memcmp(view.bytes, "alpha", 5u) == 0);
    CHECK(view.bytes[5] == 0u);
    CHECK(ow_skto_name_release(&store, &view) == SKTO_OK);

    /* A denied containment callback is refused before any admission. */
    probe.base = "beta";
    probe.len = 4u;
    probe.allow = 0;
    probe.calls = 0;
    CHECK(ow_skto_name_publish(&store, "beta", 4u, probe_range, &probe,
                               &gen) == SKTO_E_RANGE);
    CHECK(probe.calls == 1);
    CHECK(store.table.live == 1u);

    /* Maximum admitted length is copied whole, not clipped. */
    memset(long_name, 'L', sizeof long_name - 1u);
    long_name[sizeof long_name - 1u] = 0u;
    CHECK(ow_skto_name_publish(&store, long_name, sizeof long_name - 1u,
                               NULL, NULL, &gen) == SKTO_OK);
    CHECK(ow_skto_name_resolve(&store, long_name, sizeof long_name - 1u,
                               &view_gen, &view) == SKTO_OK);
    CHECK(view.len == sizeof long_name - 1u);
    CHECK(memcmp(view.bytes, long_name, view.len) == 0);
    CHECK(view.bytes[view.len] == 0u);
    CHECK(ow_skto_name_release(&store, &view) == SKTO_OK);
}

static void test_publication_and_generation(void)
{
    static const char *const k_names[] = {
        "vfs",
        "System Catalog",
        "\xE6\x97\xA5\xE6\x9C\xAC\xE8\xAA\x9E",      /* 3-byte */
        "\xF0\x9F\x92\xA1",                           /* 4-byte */
        "\xF8\x87\xBF\xBF\xBF",                       /* 5-byte */
        "\xFC\x84\xBF\xBF\xBF\xBF"                    /* 6-byte */
    };
    ow_skto_name_store_t store;
    skto_text_t view;
    uint32_t gen = 99u;
    uint32_t view_gen = 99u;
    size_t i;

    CHECK(ow_skto_name_store_init(&store) == SKTO_OK);

    for (i = 0u; i < sizeof k_names / sizeof k_names[0]; i++) {
        size_t len = strlen(k_names[i]);

        CHECK(len < OW_SKTO_NAME_MAX_BYTES);
        CHECK(ow_skto_name_publish(&store, k_names[i], len, NULL, NULL,
                                   &gen) == SKTO_OK);
        CHECK(gen == 0u);

        CHECK(ow_skto_name_resolve(&store, k_names[i], len, &view_gen,
                                   &view) == SKTO_OK);
        CHECK(view_gen == 0u);
        CHECK(view.owner == (uint16_t)SKTO_OWN_PUBLISHED);
        CHECK((view.flags & SKTO_F_NAME) != 0u);
        CHECK((view.flags & SKTO_F_CONST) != 0u);
        CHECK((view.flags & SKTO_F_NUL_SAFE) != 0u);
        CHECK(view.generation == 0u);
        CHECK(view.len == len);
        CHECK(memcmp(view.bytes, k_names[i], len) == 0);
        CHECK(view.bytes[len] == 0u);
        CHECK(ow_skto_name_release(&store, &view) == SKTO_OK);
    }
    CHECK(store.table.live == sizeof k_names / sizeof k_names[0]);

    /* Republishing an equal name increments its generation. */
    CHECK(ow_skto_name_publish(&store, "vfs", 3u, NULL, NULL, &gen) ==
          SKTO_OK);
    CHECK(gen == 1u);
    CHECK(store.table.live == sizeof k_names / sizeof k_names[0]);
    CHECK(ow_skto_name_resolve(&store, "vfs", 3u, &view_gen, &view) ==
          SKTO_OK);
    CHECK(view_gen == 1u);
    CHECK(view.generation == 1u);
    CHECK(ow_skto_name_release(&store, &view) == SKTO_OK);

    CHECK(ow_skto_name_resolve(&store, "missing", 7u, &gen, NULL) ==
          SKTO_E_NOTFOUND);
    CHECK(gen == 0u);
}

static void test_resolve_then_use_stability(void)
{
    ow_skto_name_store_t store;
    skto_text_t held;
    skto_text_t current;
    uint32_t gen = 99u;

    CHECK(ow_skto_name_store_init(&store) == SKTO_OK);
    CHECK(ow_skto_name_publish(&store, "vfs", 3u, NULL, NULL, &gen) ==
          SKTO_OK);
    CHECK(gen == 0u);
    CHECK(ow_skto_name_resolve(&store, "vfs", 3u, &gen, &held) == SKTO_OK);
    CHECK(gen == 0u);

    /* First later publication writes an inactive record. The held snapshot
     * remains the generation-0 object because the reader reference pins it. */
    CHECK(ow_skto_name_publish(&store, "vfs", 3u, NULL, NULL, &gen) ==
          SKTO_OK);
    CHECK(gen == 1u);
    CHECK(held.generation == 0u);
    CHECK(held.len == 3u);
    CHECK(memcmp(held.bytes, "vfs", 3u) == 0);
    CHECK(held.bytes[3] == 0u);

    /* The check-to-use binding rejects the retired generation. */
    {
        static const uint8_t sentinel = 0xA5u;
        current.bytes = &sentinel;
        current.len = 777u;
        CHECK(ow_skto_name_resolve_at(&store, "vfs", 3u, 0u, &current) ==
              SKTO_E_NOTFOUND);
        CHECK(current.bytes == &sentinel);
        CHECK(current.len == 777u);
    }
    CHECK(ow_skto_name_resolve_at(&store, "vfs", 3u, 1u, &current) ==
          SKTO_OK);
    CHECK(current.generation == 1u);
    CHECK(ow_skto_name_release(&store, &current) == SKTO_OK);

    /* Further publications reuse a record only when no reader holds it. The
     * generation-0 reference is still pinned, so its bytes are unchanged. */
    CHECK(ow_skto_name_publish(&store, "vfs", 3u, NULL, NULL, &gen) ==
          SKTO_OK);
    CHECK(gen == 2u);
    CHECK(held.generation == 0u);
    CHECK(held.len == 3u);
    CHECK(memcmp(held.bytes, "vfs", 3u) == 0);
    CHECK(ow_skto_name_resolve_at(&store, "vfs", 3u, 0u, &current) ==
          SKTO_E_NOTFOUND);
    CHECK(ow_skto_name_resolve_at(&store, "vfs", 3u, 1u, &current) ==
          SKTO_E_NOTFOUND);
    CHECK(ow_skto_name_resolve_at(&store, "vfs", 3u, 2u, &current) ==
          SKTO_OK);
    CHECK(current.generation == 2u);
    CHECK(ow_skto_name_release(&store, &current) == SKTO_OK);

    /* Releasing the final reference makes generation 0 reclaimable. */
    CHECK(ow_skto_name_release(&store, &held) == SKTO_OK);
    CHECK(ow_skto_name_release(&store, &held) == SKTO_E_NOTFOUND);
}

static void test_failure_atomicity(void)
{
    ow_skto_name_store_t store;
    skto_text_t view;
    uint8_t too_long[OWFS_NAME_MAX_BYTES];
    uint8_t bad[2] = { 0xC0u, 0x80u };
    range_probe_t probe;
    uint32_t gen = 99u;

    memset(too_long, 'x', sizeof too_long);
    CHECK(ow_skto_name_store_init(&store) == SKTO_OK);

    CHECK(ow_skto_name_publish(&store, "stable", 6u, NULL, NULL, &gen) ==
          SKTO_OK);
    CHECK(gen == 0u);
    CHECK(store.table.live == 1u);

    probe.base = too_long;
    probe.len = sizeof too_long;
    probe.allow = 0;
    probe.calls = 0;

    /* Each failure is refused and the prior publication is untouched. */
    CHECK(ow_skto_name_publish(&store, too_long, sizeof too_long,
                               NULL, NULL, &gen) == SKTO_E_POLICY);
    CHECK(gen == 0u);
    CHECK(ow_skto_name_publish(&store, bad, sizeof bad,
                               NULL, NULL, &gen) == SKTO_E_ENCODING);
    CHECK(gen == 0u);
    CHECK(ow_skto_name_publish(&store, too_long, sizeof too_long,
                               probe_range, &probe, &gen) == SKTO_E_POLICY);
    CHECK(gen == 0u);

    CHECK(store.table.live == 1u);
    CHECK(ow_skto_name_resolve_at(&store, "stable", 6u, 0u, &view) ==
          SKTO_OK);
    CHECK(view.generation == 0u);
    CHECK(view.len == 6u);
    CHECK(memcmp(view.bytes, "stable", 6u) == 0);
    CHECK(ow_skto_name_release(&store, &view) == SKTO_OK);

    /* The same holds after a valid replacement generation. */
    CHECK(ow_skto_name_publish(&store, "stable", 6u, NULL, NULL, &gen) ==
          SKTO_OK);
    CHECK(gen == 1u);
    CHECK(ow_skto_name_publish(&store, bad, sizeof bad,
                               NULL, NULL, &gen) == SKTO_E_ENCODING);
    CHECK(gen == 0u);
    CHECK(store.table.live == 1u);
    CHECK(ow_skto_name_resolve_at(&store, "stable", 6u, 1u, &view) ==
          SKTO_OK);
    CHECK(view.generation == 1u);
    CHECK(ow_skto_name_release(&store, &view) == SKTO_OK);
}

static void test_adapter_reader_lifetime(void)
{
    ow_skto_name_store_t store;
    skto_text_t held[4][3];
    skto_text_t view;
    uint32_t gen = 0u;
    uint32_t observed = 0u;
    size_t g;
    size_t r;

    CHECK(ow_skto_name_store_init(&store) == SKTO_OK);
    CHECK(ow_skto_name_publish(&store, "vfs", 3u, NULL, NULL, &gen) ==
          SKTO_OK);
    CHECK(gen == 0u);

    /* Hold three references to each of the four generations. */
    for (g = 0u; g < 4u; g++) {
        for (r = 0u; r < 3u; r++) {
            CHECK(ow_skto_name_resolve(&store, "vfs", 3u, NULL,
                                       &held[g][r]) == SKTO_OK);
            CHECK(held[g][r].generation == (uint32_t)g);
        }
        if (g < 3u) {
            CHECK(ow_skto_name_publish(&store, "vfs", 3u, NULL, NULL,
                                       &gen) == SKTO_OK);
            CHECK(gen == (uint32_t)g + 1u);
        }
    }

    /* No record is reclaimable, so replacement is refused without mutation. */
    gen = 99u;
    CHECK(ow_skto_name_publish(&store, "vfs", 3u, NULL, NULL, &gen) ==
          SKTO_E_BUSY);
    CHECK(gen == 0u);
    CHECK(ow_skto_name_resolve(&store, "vfs", 3u, &observed, NULL) ==
          SKTO_OK);
    CHECK(observed == 3u);
    CHECK(held[0][0].generation == 0u);
    CHECK(memcmp(held[0][0].bytes, "vfs", 3u) == 0);

    /* Releasing part of one record does not make it reusable. */
    CHECK(ow_skto_name_release(&store, &held[0][0]) == SKTO_OK);
    CHECK(ow_skto_name_release(&store, &held[0][1]) == SKTO_OK);
    CHECK(ow_skto_name_publish(&store, "vfs", 3u, NULL, NULL, &gen) ==
          SKTO_E_BUSY);
    CHECK(ow_skto_name_release(&store, &held[0][2]) == SKTO_OK);

    CHECK(ow_skto_name_publish(&store, "vfs", 3u, NULL, NULL, &gen) ==
          SKTO_OK);
    CHECK(gen == 4u);

    /* Older held generations remain byte-exact across reclamation. */
    for (g = 1u; g < 4u; g++) {
        for (r = 0u; r < 3u; r++) {
            CHECK(held[g][r].generation == (uint32_t)g);
            CHECK(held[g][r].len == 3u);
            CHECK(memcmp(held[g][r].bytes, "vfs", 3u) == 0);
            CHECK(held[g][r].bytes[3] == 0u);
            CHECK(ow_skto_name_release(&store, &held[g][r]) == SKTO_OK);
        }
    }

    CHECK(ow_skto_name_resolve(&store, "vfs", 3u, NULL, &view) == SKTO_OK);
    CHECK(view.generation == 4u);
    CHECK(ow_skto_name_release(NULL, &view) == SKTO_E_NULL);
    CHECK(ow_skto_name_release(&store, NULL) == SKTO_E_NULL);
    CHECK(ow_skto_name_release(&store, &view) == SKTO_OK);
    CHECK(ow_skto_name_release(&store, &view) == SKTO_E_NOTFOUND);
}

typedef struct {
    int acquires;
    int releases;
} adapter_lock_probe_t;

static void adapter_lock_acquire(void *ctx)
{
    ((adapter_lock_probe_t *)ctx)->acquires++;
}

static void adapter_lock_release(void *ctx)
{
    ((adapter_lock_probe_t *)ctx)->releases++;
}

static void test_adapter_lock_hooks(void)
{
    ow_skto_name_store_t store;
    skto_text_t view;
    adapter_lock_probe_t probe;
    uint32_t gen = 0u;

    memset(&probe, 0, sizeof probe);

    CHECK(ow_skto_name_store_init_locked(NULL, adapter_lock_acquire,
                                         adapter_lock_release, &probe) ==
          SKTO_E_NULL);

    /* A missing callback leaves an existing store untouched. */
    CHECK(ow_skto_name_store_init(&store) == SKTO_OK);
    store.table.live = 123u;
    CHECK(ow_skto_name_store_init_locked(&store, NULL,
                                         adapter_lock_release, &probe) ==
          SKTO_E_NULL);
    CHECK(store.table.live == 123u);
    CHECK(store.table.lock.acquire == NULL);

    CHECK(ow_skto_name_store_init_locked(&store, adapter_lock_acquire,
                                         adapter_lock_release, &probe) ==
          SKTO_OK);
    CHECK(store.table.lock.acquire == adapter_lock_acquire);
    CHECK(store.table.lock.release == adapter_lock_release);
    CHECK(store.table.lock.ctx == &probe);

    CHECK(ow_skto_name_publish(&store, "vfs", 3u, NULL, NULL, &gen) ==
          SKTO_OK);
    CHECK(probe.acquires == 1 && probe.releases == 1);

    CHECK(ow_skto_name_resolve(&store, "vfs", 3u, &gen, &view) == SKTO_OK);
    CHECK(probe.acquires == 2 && probe.releases == 2);

    CHECK(ow_skto_name_release(&store, &view) == SKTO_OK);
    CHECK(probe.acquires == 3 && probe.releases == 3);

    /* A destroy that finds a live reader must release the lock it took and
     * leave the table exactly as it was, so the store stays usable. */
    CHECK(ow_skto_name_resolve(&store, "vfs", 3u, &gen, &view) == SKTO_OK);
    CHECK(probe.acquires == 4 && probe.releases == 4);
    CHECK(ow_skto_name_store_destroy(&store) == SKTO_E_BUSY);
    CHECK(probe.acquires == 5 && probe.releases == 5);
    CHECK(store.table.shutting_down == 0u);
    CHECK(ow_skto_name_resolve(&store, "vfs", 3u, NULL, NULL) == SKTO_OK);

    /* Once drained, destroy clears the table and still releases the lock it
     * acquired. skto_names_clear() removes the callbacks, so a destroy that
     * cleared before unlocking would deadlock or corrupt the caller's lock. */
    CHECK(ow_skto_name_release(&store, &view) == SKTO_OK);
    CHECK(probe.acquires == 7 && probe.releases == 7);
    CHECK(ow_skto_name_store_destroy(&store) == SKTO_OK);
    CHECK(probe.acquires == 8 && probe.releases == 8);
    CHECK(store.table.live == 0u);
    CHECK(store.table.shutting_down == 0u);
    CHECK(store.table.lock.acquire == NULL);
    CHECK(store.table.lock.release == NULL);
    CHECK(store.table.lock.ctx == NULL);
}

static void test_retirement(void)
{
    ow_skto_name_store_t store;
    skto_text_t held;
    skto_text_t current;
    uint32_t gen = 0u;

    CHECK(ow_skto_name_store_init(&store) == SKTO_OK);

    CHECK(ow_skto_name_retire(NULL, "vfs", 3u) == SKTO_E_NULL);
    CHECK(ow_skto_name_retire(&store, NULL, 3u) == SKTO_E_NULL);
    CHECK(ow_skto_name_retire(&store, "vfs", 3u) == SKTO_E_NOTFOUND);

    CHECK(ow_skto_name_publish(&store, "vfs", 3u, NULL, NULL, &gen) ==
          SKTO_OK);
    CHECK(gen == 0u);
    CHECK(store.table.live == 1u);

    /* A held view survives retirement byte-exactly while lookup stops
     * matching, and the slot is not released until the reader does. */
    CHECK(ow_skto_name_resolve(&store, "vfs", 3u, &gen, &held) == SKTO_OK);
    CHECK(gen == 0u);
    CHECK(ow_skto_name_retire(&store, "vfs", 3u) == SKTO_OK);
    CHECK(store.table.live == 1u);
    CHECK(store.table.slots[0].live == 1u);
    CHECK(store.table.slots[0].retired == 1u);
    CHECK(ow_skto_name_resolve(&store, "vfs", 3u, NULL, NULL) ==
          SKTO_E_NOTFOUND);
    CHECK(held.generation == 0u);
    CHECK(held.len == 3u);
    CHECK(memcmp(held.bytes, "vfs", 3u) == 0);
    CHECK(held.bytes[3] == 0u);
    CHECK(ow_skto_name_retire(&store, "vfs", 3u) == SKTO_E_NOTFOUND);

    /* Republishing after retirement takes a fresh slot. */
    CHECK(ow_skto_name_publish(&store, "vfs", 3u, NULL, NULL, &gen) ==
          SKTO_OK);
    CHECK(gen == 0u);
    CHECK(store.table.live == 2u);
    CHECK(ow_skto_name_resolve(&store, "vfs", 3u, &gen, &current) == SKTO_OK);
    CHECK(gen == 0u);
    CHECK(current.bytes != held.bytes);
    CHECK(ow_skto_name_release(&store, &current) == SKTO_OK);

    CHECK(ow_skto_name_release(&store, &held) == SKTO_OK);
    CHECK(store.table.live == 1u);
    CHECK(store.table.slots[0].live == 0u);
    CHECK(store.table.slots[0].retired == 0u);
    CHECK(store.table.slots[0].snapshots[0].readers == 0u);

    /* With no reader the slot is reclaimed at once. */
    CHECK(ow_skto_name_retire(&store, "vfs", 3u) == SKTO_OK);
    CHECK(store.table.live == 0u);
    CHECK(ow_skto_name_resolve(&store, "vfs", 3u, NULL, NULL) ==
          SKTO_E_NOTFOUND);
}

static void test_retirement_is_teardown_safe(void)
{
    ow_skto_name_store_t store;
    uint32_t gen = 0u;

    CHECK(ow_skto_name_store_init(&store) == SKTO_OK);
    CHECK(ow_skto_name_publish(&store, "vfs", 3u, NULL, NULL, &gen) ==
          SKTO_OK);
    CHECK(ow_skto_name_store_shutdown(&store) == SKTO_OK);

    /* Admission is refused during shutdown, but withdrawal is part of
     * teardown and must remain available. */
    CHECK(ow_skto_name_publish(&store, "vfs2", 4u, NULL, NULL, &gen) ==
          SKTO_E_POLICY);
    CHECK(ow_skto_name_retire(&store, "vfs", 3u) == SKTO_OK);
    CHECK(store.table.live == 0u);
    CHECK(ow_skto_name_store_destroy(&store) == SKTO_OK);
}

static void test_legacy_copy_refuses_truncation(void)
{
    uint8_t dest[8];
    uint8_t exact[4] = { 'a', 'b', 'c', 'd' };

    /* Fits: copied whole with a terminator. */
    memset(dest, 0xAAu, sizeof dest);
    CHECK(ow_sutf8_name_copy(dest, sizeof dest, exact, 3u) == 3u);
    CHECK(memcmp(dest, "abc", 3u) == 0);
    CHECK(dest[3] == 0u);

    /* Exact capacity boundary: content bytes + terminator == dest_cap. */
    memset(dest, 0xAAu, sizeof dest);
    CHECK(ow_sutf8_name_copy(dest, 4u, exact, 3u) == 3u);
    CHECK(dest[3] == 0u);

    /* One byte too long: refuse and leave dest untouched. */
    memset(dest, 0x5Au, sizeof dest);
    CHECK(ow_sutf8_name_copy(dest, 4u, exact, 4u) ==
          OW_STRING_COPY_REFUSED);
    CHECK(dest[0] == 0x5Au);
    CHECK(dest[3] == 0x5Au);

    CHECK(ow_sutf8_name_copy(NULL, sizeof dest, exact, 3u) ==
          OW_STRING_COPY_REFUSED);
    CHECK(ow_sutf8_name_copy(dest, 0u, exact, 3u) ==
          OW_STRING_COPY_REFUSED);

    /* An empty name is still a successful copy, but not an absent pointer. */
    memset(dest, 0xAAu, sizeof dest);
    CHECK(ow_sutf8_name_copy(dest, sizeof dest, NULL, 0u) == 0u);
    CHECK(dest[0] == 0u);
    CHECK(ow_sutf8_name_copy(dest, sizeof dest, NULL, 1u) ==
          OW_STRING_COPY_REFUSED);
}

static void test_shutdown_and_drain(void)
{
    ow_skto_name_store_t store;
    skto_text_t view;
    uint32_t gen = 0u;
    uint32_t readers = 99u;

    CHECK(ow_skto_name_store_init(&store) == SKTO_OK);
    CHECK(ow_skto_name_publish(&store, "vfs", 3u, NULL, NULL, &gen) == SKTO_OK);
    CHECK(ow_skto_name_resolve(&store, "vfs", 3u, NULL, &view) == SKTO_OK);

    /* 1 reader held: drain reports BUSY and 1 outstanding reader */
    CHECK(ow_skto_name_store_drain(&store, &readers) == SKTO_E_BUSY);
    CHECK(readers == 1u);

    /* Destruction with outstanding readers must be refused */
    CHECK(ow_skto_name_store_destroy(&store) == SKTO_E_BUSY);

    /* Shutdown prevents new acquisitions and publications */
    CHECK(ow_skto_name_store_shutdown(&store) == SKTO_OK);
    CHECK(ow_skto_name_resolve(&store, "vfs", 3u, NULL, NULL) == SKTO_E_POLICY);
    CHECK(ow_skto_name_publish(&store, "vfs2", 4u, NULL, NULL, &gen) == SKTO_E_POLICY);

    /* Releasing held reference succeeds during shutdown */
    CHECK(ow_skto_name_release(&store, &view) == SKTO_OK);

    /* Draining after release succeeds */
    CHECK(ow_skto_name_store_drain(&store, &readers) == SKTO_OK);
    CHECK(readers == 0u);

    /* Clean destruction now succeeds */
    CHECK(ow_skto_name_store_destroy(&store) == SKTO_OK);
}

static void test_generation_exhaustion(void)
{
    ow_skto_name_store_t store;
    uint32_t gen = 0u;

    CHECK(ow_skto_name_store_init(&store) == SKTO_OK);
    CHECK(ow_skto_name_publish(&store, "exhaust", 7u, NULL, NULL, &gen) == SKTO_OK);
    CHECK(gen == 0u);

    /* Simulate maximum generation */
    store.table.slots[0].snapshots[store.table.slots[0].active].generation = 0xFFFFFFFFu;

    /* Next publication must be refused with OVERFLOW, preventing ABA wrap */
    CHECK(ow_skto_name_publish(&store, "exhaust", 7u, NULL, NULL, &gen) == SKTO_E_OVERFLOW);
}

int main(void)
{
    test_store_geometry();
    test_invalid_input_refused();
    test_over_ceiling_refused();
    test_bounded_copy_and_containment();
    test_publication_and_generation();
    test_resolve_then_use_stability();
    test_failure_atomicity();
    test_adapter_reader_lifetime();
    test_adapter_lock_hooks();
    test_shutdown_and_drain();
    test_generation_exhaustion();
    test_retirement();
    test_retirement_is_teardown_safe();
    test_legacy_copy_refuses_truncation();

    if (g_fail != 0) {
        printf("test_compat_skto_name: FAILED\n");
        return 1;
    }
    printf("test_compat_skto_name: all checks passed\n");
    return 0;
}
