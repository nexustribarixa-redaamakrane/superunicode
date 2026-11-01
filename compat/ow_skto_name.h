#ifndef OW_SKTO_NAME_H
#define OW_SKTO_NAME_H

/* SKTO admission/publication adapter for the OpenWindows name field.
 *
 * This is host-side compatibility glue between the vendored OWFS/USFS name
 * representation and SUAS-018 (SKTO). It is deliberately kept out of the
 * vendored storage sources: the storage field remains a plain byte array, and
 * the adapter is the only code that turns candidate bytes into a published
 * SKTO name.
 *
 * The adapter:
 *   - refuses an absent argument, a malformed or inadmissible sequence, an
 *     interior NUL, and a name above the namespace ceiling;
 *   - runs the optional caller containment callback over the candidate before
 *     reading it;
 *   - admits into a bounded stack stage and publishes into namespace-owned
 *     storage, so no caller buffer is retained and no name is truncated;
 *   - returns the publication generation.
 *
 * Concurrency: the default initialiser installs no lock, so the caller must
 * serialise every table operation. With ow_skto_name_store_init_locked() the
 * adapter brackets each publish, resolve, and release with the caller's lock
 * callbacks. The adapter performs no atomic operations and makes no lock-free
 * claim; the callbacks must provide mutual exclusion and acquire/release
 * ordering.
 *
 * Snapshot lifetime: a view returned by ow_skto_name_resolve() or
 * ow_skto_name_resolve_at() with a non-NULL out_view acquires one reader
 * reference. It remains valid and byte-exact until it is passed to
 * ow_skto_name_release(), the store is re-initialised, or the store is
 * destroyed. Publication never overwrites a referenced record. A publisher
 * that cannot find an unreferenced inactive record returns SKTO_E_BUSY and
 * changes nothing. Reclamation is lazy: the record becomes reusable only
 * after its final release.
 */

#include <stddef.h>
#include <stdint.h>

#include "owfs_types.h"
#include "suas/suas_skto.h"

#ifdef __cplusplus
extern "C" {
#endif

/* OWFS and USFS both reserve OWFS_NAME_MAX_BYTES for the encoded field,
 * including its terminator. The adapter accepts at most one byte less. */
#define OW_SKTO_NAME_MAX_BYTES ((size_t)(OWFS_NAME_MAX_BYTES - 1u))

#if (SKTO_NAME_MAX_BYTES + 1u) < OWFS_NAME_MAX_BYTES
#error "SKTO name storage cannot hold a complete OWFS/USFS name field"
#endif

/* Caller-allocated namespace. No allocation is performed by this module. */
typedef struct {
    skto_name_table_t table;  /* immutable published generations */
    skto_limits_t     limits; /* namespace ceiling, lowered to the adapter max */
} ow_skto_name_store_t;

/* Initialise an empty namespace. Returns SKTO_E_NULL for a NULL store. The
 * default initialiser installs no internal lock: the caller must serialise
 * later table operations. */
skto_status_t ow_skto_name_store_init(ow_skto_name_store_t *store);

/* Initialise an empty namespace with caller-supplied lock callbacks. Both
 * callbacks are required; a NULL callback returns SKTO_E_NULL and leaves the
 * store unchanged. Initialisation itself is not locked. */
skto_status_t ow_skto_name_store_init_locked(ow_skto_name_store_t *store,
                                             skto_lock_fn acquire,
                                             skto_lock_fn release,
                                             void *ctx);

/* Admit and publish a name of exactly len bytes.
 *
 * `range` is the optional SKTO containment callback. When non-NULL it must
 * accept [src, src + len) before any byte is read. On any failure the store is
 * unchanged and *out_generation is set to 0. On success *out_generation is the
 * assigned generation (0 for the first publication of the name, otherwise the
 * previous generation plus one). Returns SKTO_E_BUSY when replacing the name
 * would require overwriting a snapshot that a reader still holds. */
skto_status_t ow_skto_name_publish(ow_skto_name_store_t *store,
                                   const void *src, size_t len,
                                   skto_range_fn range, void *range_ctx,
                                   uint32_t *out_generation);

/* Resolve a name. On success *out_view is an immutable PUBLISHED snapshot and
 * *out_generation is its generation. The query descriptor is built locally;
 * the caller's bytes are not retained.
 *
 * When out_view is non-NULL this acquires one reader reference and the view
 * MUST be released with ow_skto_name_release() exactly once. When out_view is
 * NULL no reference is acquired and *out_generation is a point-in-time
 * observation only. */
skto_status_t ow_skto_name_resolve(ow_skto_name_store_t *store,
                                   const void *src, size_t len,
                                   uint32_t *out_generation,
                                   skto_text_t *out_view);

/* Release one reference acquired by ow_skto_name_resolve() or
 * ow_skto_name_resolve_at(). Returns SKTO_E_NOTFOUND for a stale or duplicate
 * view. */
skto_status_t ow_skto_name_release(ow_skto_name_store_t *store,
                                   const skto_text_t *view);

/* Withdraw a name from lookup. Outstanding snapshots are not rewritten: the
 * store reclaims the slot after their final release. Returns SKTO_E_NOTFOUND
 * when the store does not hold that name. */
skto_status_t ow_skto_name_retire(ow_skto_name_store_t *store,
                                  const void *src, size_t len);

/* Resolve a name and accept it only when its current generation equals
 * expected_generation. On success with a non-NULL out_view this acquires a
 * reader reference, which the caller must release. A stale generation returns
 * SKTO_E_NOTFOUND, releases any reference acquired during the check, and
 * leaves *out_view untouched. With out_view == NULL the call performs a
 * non-acquiring generation check. */
skto_status_t ow_skto_name_resolve_at(ow_skto_name_store_t *store,
                                      const void *src, size_t len,
                                      uint32_t expected_generation,
                                      skto_text_t *out_view);

/* Initiate shutdown on the store, rejecting subsequent publications and
 * acquisitions with SKTO_E_POLICY. */
skto_status_t ow_skto_name_store_shutdown(ow_skto_name_store_t *store);

/* Report the number of outstanding reader references. Returns SKTO_OK if 0,
 * or SKTO_E_BUSY if references are still held. */
skto_status_t ow_skto_name_store_drain(ow_skto_name_store_t *store,
                                       uint32_t *out_readers);

/* Destroy the store if and only if all readers have drained. */
skto_status_t ow_skto_name_store_destroy(ow_skto_name_store_t *store);

#ifdef __cplusplus
}
#endif

#endif /* OW_SKTO_NAME_H */
