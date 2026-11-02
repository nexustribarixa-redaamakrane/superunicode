#include "ow_skto_name.h"

/* Stage size is the SKTO storage ceiling, which the header checks is at least
 * one byte larger than the OWFS/USFS field. The stage is stack storage: no
 * allocation, and its bound is a compile-time constant. */
typedef char ow_skto_stage_fits[
    (SKTO_NAME_MAX_BYTES >= OWFS_NAME_MAX_BYTES) ? 1 : -1];

static void ow_skto_name_apply_limits(ow_skto_name_store_t *store)
{
    suas_skto_limits_default(&store->limits);
    if (store->limits.max_name_bytes > OW_SKTO_NAME_MAX_BYTES) {
        store->limits.max_name_bytes = OW_SKTO_NAME_MAX_BYTES;
    }
}

skto_status_t ow_skto_name_store_init(ow_skto_name_store_t *store)
{
    if (store == NULL) {
        return SKTO_E_NULL;
    }

    suas_skto_names_init(&store->table);
    ow_skto_name_apply_limits(store);
    return SKTO_OK;
}

skto_status_t ow_skto_name_store_init_locked(ow_skto_name_store_t *store,
                                             skto_lock_fn acquire,
                                             skto_lock_fn release,
                                             void *ctx)
{
    skto_status_t st;

    if (store == NULL || acquire == NULL || release == NULL) {
        return SKTO_E_NULL;
    }

    st = suas_skto_names_init_locked(&store->table, acquire, release, ctx);
    if (st != SKTO_OK) {
        return st;
    }
    ow_skto_name_apply_limits(store);
    return SKTO_OK;
}

skto_status_t ow_skto_name_publish(ow_skto_name_store_t *store,
                                   const void *src, size_t len,
                                   skto_range_fn range, void *range_ctx,
                                   uint32_t *out_generation)
{
    uint8_t stage[SKTO_NAME_MAX_BYTES];
    skto_text_t admitted;
    size_t written = 0u;
    skto_status_t st;

    if (out_generation != NULL) {
        *out_generation = 0u;
    }
    if (store == NULL) {
        return SKTO_E_NULL;
    }

    /* Refuse the namespace ceiling before touching the candidate. This is a
     * policy rejection, not a capacity fallback: the adapter never copies a
     * prefix and never truncates. */
    if (len > OW_SKTO_NAME_MAX_BYTES) {
        return SKTO_E_POLICY;
    }

    st = suas_skto_admit(src, len, range, range_ctx, &store->limits,
                         stage, sizeof stage, &admitted, &written);
    if (st != SKTO_OK) {
        return st;
    }

    /* suas_skto_publish validates N1-N3 and N5 again before it writes, and
     * copies into namespace-owned storage. A failure leaves the table intact. */
    return suas_skto_publish(&store->table, &admitted, &store->limits,
                             out_generation);
}

skto_status_t ow_skto_name_resolve(ow_skto_name_store_t *store,
                                   const void *src, size_t len,
                                   uint32_t *out_generation,
                                   skto_text_t *out_view)
{
    skto_text_t query;
    skto_status_t st;

    if (out_generation != NULL) {
        *out_generation = 0u;
    }
    if (store == NULL) {
        return SKTO_E_NULL;
    }

    st = suas_skto_init_borrowed(&query, src, len);
    if (st != SKTO_OK) {
        return st;
    }
    return suas_skto_resolve(&store->table, &query, out_generation, out_view);
}

skto_status_t ow_skto_name_release(ow_skto_name_store_t *store,
                                   const skto_text_t *view)
{
    if (store == NULL || view == NULL) {
        return SKTO_E_NULL;
    }
    return suas_skto_name_release(&store->table, view);
}

skto_status_t ow_skto_name_retire(ow_skto_name_store_t *store,
                                  const void *src, size_t len)
{
    skto_text_t query;
    skto_status_t st;

    if (store == NULL || src == NULL) {
        return SKTO_E_NULL;
    }

    st = suas_skto_init_borrowed(&query, src, len);
    if (st != SKTO_OK) {
        return st;
    }
    return suas_skto_retire(&store->table, &query);
}

skto_status_t ow_skto_name_resolve_at(ow_skto_name_store_t *store,
                                      const void *src, size_t len,
                                      uint32_t expected_generation,
                                      skto_text_t *out_view)
{
    skto_text_t view = {0};
    uint32_t generation = 0u;
    skto_status_t st;

    st = ow_skto_name_resolve(store, src, len, &generation,
                              out_view != NULL ? &view : NULL);
    if (st != SKTO_OK) {
        return st;
    }
    if (generation != expected_generation) {
        /* Release the reference acquired by the check; a NULL out_view never
         * acquired one. */
        if (out_view != NULL) {
            (void)ow_skto_name_release(store, &view);
        }
        return SKTO_E_NOTFOUND; /* the generation has been retired */
    }
    if (out_view != NULL) {
        *out_view = view;
    }
    return SKTO_OK;
}

skto_status_t ow_skto_name_store_shutdown(ow_skto_name_store_t *store)
{
    if (store == NULL) {
        return SKTO_E_NULL;
    }
    return suas_skto_names_shutdown(&store->table);
}

skto_status_t ow_skto_name_store_drain(ow_skto_name_store_t *store,
                                       uint32_t *out_readers)
{
    if (store == NULL) {
        return SKTO_E_NULL;
    }
    return suas_skto_names_drain(&store->table, out_readers);
}

skto_status_t ow_skto_name_store_destroy(ow_skto_name_store_t *store)
{
    if (store == NULL) {
        return SKTO_E_NULL;
    }
    return suas_skto_names_destroy(&store->table);
}
