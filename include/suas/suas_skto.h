#ifndef SUAS_SUAS_SKTO_H
#define SUAS_SUAS_SKTO_H

/* SUAS-018 - System Kernel Text Objects (SKTO)
 *
 * The mandatory kernel text contract: bounded, explicitly measured views over
 * SUTF-8 bytes with an ownership class, a capacity, and immutable published
 * names. No terminator dependence, no allocation, no SCP as content.
 *
 * See docs/suas/SUAS-018-skto.md for the normative specification.
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* -- Version ---------------------------------------------------------- */

#define SUAS_SKTO_SPEC_ID      "SUAS-018"
#define SUAS_SKTO_VERSION_MAJOR 1
#define SUAS_SKTO_VERSION_MINOR 0
#define SUAS_SKTO_VERSION_PATCH 0

/* Admission ceiling: the largest candidate a single call will accept.
 * A caller may lower it with skto_limits_t; it may not raise it. */
#define SKTO_MAX_ADMIT_BYTES   4096u

/* Name storage ceiling. SKTO does not impose a namespace's limit; it only
 * refuses to publish a name it cannot store here. A namespace with a larger
 * limit keeps its own storage (section 9.1 N5).
 *
 * The ceiling is set to the OWFS/USFS name field (128 bytes including the
 * terminator) so the compatibility adapter can store a complete admitted
 * name. It remains an implementation storage bound, not a namespace policy. */
#define SKTO_NAME_MAX_BYTES    128u
#define SKTO_NAME_CAPACITY     16u

/* Snapshot records per live name. One record is visible; the others can keep
 * older generations immutable while readers still reference them. A publisher
 * that cannot find an unreferenced inactive record fails with SKTO_E_BUSY.
 * The pool is caller-owned and fixed; there is no allocation or background
 * reclamation. */
#define SKTO_NAME_SNAPSHOTS    4u

/* -- Ownership and encoding ------------------------------------------ */

typedef enum {
    SKTO_OWN_STATIC    = 0, /* immutable image data          */
    SKTO_OWN_BORROWED  = 1, /* caller-owned, this call only  */
    SKTO_OWN_OWNED     = 2, /* caller storage filled here    */
    SKTO_OWN_PUBLISHED = 3  /* registered, immutable         */
} skto_owner_t;

typedef enum {
    SKTO_ENC_SUTF8 = 0      /* the only interior form in 1.0 */
} skto_encoding_t;

/* -- Flags ------------------------------------------------------------ */

#define SKTO_F_NUL_SAFE   0x01u
#define SKTO_F_CONST      0x02u
#define SKTO_F_NAME       0x04u
#define SKTO_F_ADMITTED   0x08u

/* -- Status ----------------------------------------------------------- */

typedef enum {
    SKTO_OK            = 0,
    SKTO_E_NULL        = 1,  /* absent argument                    */
    SKTO_E_OVERFLOW    = 2,  /* length arithmetic overflows        */
    SKTO_E_RANGE       = 3,  /* candidate escapes its validated range */
    SKTO_E_ENCODING    = 4,  /* malformed SUTF-8                   */
    SKTO_E_INADMISSIBLE= 5,  /* SCP, surrogate, trap, Sentinel     */
    SKTO_E_CAPACITY    = 6,  /* destination too small              */
    SKTO_E_POLICY      = 7,  /* name policy rejection              */
    SKTO_E_OWNER       = 8,  /* operation illegal for the owner    */
    SKTO_E_STATE       = 9,  /* descriptor is not valid            */
    SKTO_E_BOUNDARY    = 10, /* slice is not on a codepoint edge   */
    SKTO_E_FULL        = 11, /* name table has no free slot        */
    SKTO_E_NOTFOUND    = 12, /* no such name or generation         */
    SKTO_E_TERMINATOR  = 13, /* no NUL found inside the scan bound */
    SKTO_E_BUSY        = 14  /* every reusable snapshot is referenced */
} skto_status_t;

/* -- The text object (section 4.1) ------------------------------------ */

typedef struct {
    const uint8_t *bytes;    /* text bytes; NULL only when len == 0 */
    size_t         len;      /* length in BYTES, not codepoints     */
    size_t         cap;      /* capacity in bytes; >= len           */
    uint16_t       owner;    /* skto_owner_t                        */
    uint16_t       encoding; /* skto_encoding_t                     */
    uint32_t       flags;    /* SKTO_F_*                            */
    uint32_t       generation; /* meaningful only for PUBLISHED     */
} skto_text_t;

/* Per-call limits. A zero field means "use the default". */
typedef struct {
    size_t max_admit_bytes;
    size_t max_name_bytes;
} skto_limits_t;

/* -- Construction (section 4.4) --------------------------------------- */

const char *suas_skto_version_string(void);
void        suas_skto_limits_default(skto_limits_t *limits);

/* Construct over image data. len must be the true length; cap == len. */
skto_status_t suas_skto_init_static(skto_text_t *t, const void *bytes,
                                    size_t len);

/* Construct a borrowed view. Structural validation only (V1-V4). */
skto_status_t suas_skto_init_borrowed(skto_text_t *t, const void *bytes,
                                      size_t len);

/* Construct an owned object over caller storage. len starts at 0. */
skto_status_t suas_skto_init_owned(skto_text_t *t, void *buffer, size_t cap);

/* Construct the empty static object. */
void suas_skto_init_empty(skto_text_t *t);

/* Bounded compatibility constructor for legacy C callers (section 4.4).
 * Scans at most cap bytes for the terminator, never past it. On success the
 * object is a BORROWED view of cstr with NUL_SAFE and CONST set. Returns
 * SKTO_E_TERMINATOR when no NUL is found inside cap, leaving *t untouched.
 * New code must use the explicit-length constructors. */
skto_status_t suas_skto_init_legacy_cstr(skto_text_t *t, const char *cstr,
                                         size_t cap);

/* -- Validation (section 4.3) ----------------------------------------- */

/* O(1) structural check: V1-V4. */
skto_status_t suas_skto_check(const skto_text_t *t);

/* Full check: V1-V8, including a single SUTF-8 scan. */
skto_status_t suas_skto_validate(const skto_text_t *t);

/* Assert the NUL-safe view. Verifies the byte and rejects interior NUL. */
skto_status_t suas_skto_mark_nul_safe(skto_text_t *t);

/* -- Reading ---------------------------------------------------------- */

/* Replace the contents of an OWNED object with an explicitly measured,
 * validated byte sequence. Copies nothing when cap is too small. On success
 * len is the byte count, ADMITTED is set, and a terminator is written when
 * cap > len. NUL_SAFE is set only when cap > len and the bytes contain no
 * interior 0x00. On failure the object is unchanged. The source may overlap
 * the destination storage. */
skto_status_t suas_skto_assign(skto_text_t *t, const void *bytes, size_t len);

/* Decode the codepoint starting at *pos. Advances *pos past it. */
skto_status_t suas_skto_next(const skto_text_t *t, size_t *pos,
                             uint32_t *out_cp);

/* Count codepoints. O(len), no allocation. */
skto_status_t suas_skto_count(const skto_text_t *t, size_t *out_count);

/* Copy the object into caller storage. Fails rather than truncates. */
skto_status_t suas_skto_copy(const skto_text_t *t, uint8_t *dst, size_t cap,
                             size_t *out_len);

/* -- Comparison and hash (section 10) --------------------------------- */

bool   suas_skto_equal(const skto_text_t *a, const skto_text_t *b);
int    suas_skto_compare(const skto_text_t *a, const skto_text_t *b);
uint32_t suas_skto_hash(const skto_text_t *t);

/* -- Admission (section 7) -------------------------------------------- */

/* Optional containment callback. Returns true when [base, base+len) is
 * entirely inside memory the caller owns. A NULL callback skips step 3,
 * which is correct only for a trusted same-domain buffer. */
typedef bool (*skto_range_fn)(void *ctx, const void *base, size_t len);

/* Admit an untrusted candidate into caller storage. On failure *out is
 * untouched and *written is 0. */
skto_status_t suas_skto_admit(const void *src, size_t len,
                              skto_range_fn range, void *range_ctx,
                              const skto_limits_t *limits,
                              uint8_t *dst, size_t cap,
                              skto_text_t *out, size_t *written);

/* Admit a NUL-terminated candidate (section 7.1 step 4). Scans at most
 * scan_cap bytes and never uses an unbounded strlen. The terminator is not
 * part of the admitted text. Returns SKTO_E_TERMINATOR when no terminator
 * occurs inside scan_cap; otherwise behaves exactly like suas_skto_admit. */
skto_status_t suas_skto_admit_cstr(const char *src, size_t scan_cap,
                                   skto_range_fn range, void *range_ctx,
                                   const skto_limits_t *limits,
                                   uint8_t *dst, size_t cap,
                                   skto_text_t *out, size_t *written);

/* -- Names (section 9) ------------------------------------------------ */

typedef struct {
    uint8_t  bytes[SKTO_NAME_MAX_BYTES + 1]; /* text + NUL              */
    uint32_t generation;
    uint32_t readers;   /* outstanding references; 0 means reclaimable   */
    uint16_t len;
    uint8_t  live;      /* the record holds a published generation       */
} skto_name_snapshot_t;

typedef struct {
    /* One active snapshot plus records retained for readers of older
     * generations. Publication writes only to an inactive record whose
     * readers count is zero; it never rewrites a record a reader holds. */
    skto_name_snapshot_t snapshots[SKTO_NAME_SNAPSHOTS];
    uint8_t active;     /* index of the visible snapshot                 */
    uint8_t live;       /* the slot holds a published name               */
    uint8_t retired;    /* withdrawn from lookup; readers may still hold  */
                        /* snapshots. Occupies existing tail padding, so */
                        /* the slot and table layouts are unchanged.      */
} skto_name_slot_t;

/* Optional caller-supplied table lock. SKTO performs no atomic operations and
 * implements no lock of its own. When both callbacks are installed, every
 * table read or write in publish, resolve, and release is bracketed by
 * acquire/release. The callbacks MUST provide mutual exclusion and at least
 * acquire/release ordering; ctx MUST outlive the table. They MUST NOT call
 * back into SKTO. With no callbacks installed the caller MUST serialize every
 * table operation externally. */
typedef void (*skto_lock_fn)(void *ctx);

typedef struct {
    skto_lock_fn acquire;
    skto_lock_fn release;
    void         *ctx;
} skto_lock_ops_t;

typedef struct {
    skto_name_slot_t slots[SKTO_NAME_CAPACITY];
    uint16_t live;
    uint8_t  shutting_down;
    skto_lock_ops_t lock;
} skto_name_table_t;

/* Initialise an empty namespace with no internal lock. The caller must
 * serialize all later table operations. */
void suas_skto_names_init(skto_name_table_t *table);

/* Initialise an empty namespace with the supplied lock callbacks. Both
 * callbacks are required; a missing callback returns SKTO_E_NULL and leaves
 * the table unchanged. Initialisation itself is not locked. */
skto_status_t suas_skto_names_init_locked(skto_name_table_t *table,
                                          skto_lock_fn acquire,
                                          skto_lock_fn release,
                                          void *ctx);

/* Mark the namespace as shutting down. Further publications and new reader
 * acquisitions are rejected with SKTO_E_POLICY. Existing readers may continue
 * to release their snapshots. */
skto_status_t suas_skto_names_shutdown(skto_name_table_t *table);

/* Check whether all readers across the table have drained. If out_readers
 * is non-NULL, writes the count of outstanding reader references. Returns
 * SKTO_OK if 0 readers remain, or SKTO_E_BUSY if readers are outstanding. */
skto_status_t suas_skto_names_drain(skto_name_table_t *table, uint32_t *out_readers);

/* Destroy a table cleanly. If any reader references are still outstanding,
 * the table is not destroyed and returns SKTO_E_BUSY. Once drained, marks the
 * table dead and zeroes memory. */
skto_status_t suas_skto_names_destroy(skto_name_table_t *table);

/* Publish a name, or replace an equal one with generation + 1.
 * The bytes are copied; the caller's buffer is never retained.
 *
 * A replacement writes an inactive snapshot record whose readers count is
 * zero. If every inactive record is still referenced, it returns
 * SKTO_E_BUSY and leaves the table, active generation, and snapshots
 * unchanged. A fresh name returns SKTO_E_FULL when the table has no free
 * slot. */
skto_status_t suas_skto_publish(skto_name_table_t *table,
                                const skto_text_t *name,
                                const skto_limits_t *limits,
                                uint32_t *out_generation);

/* Resolve an equal name and, when out_view is non-NULL, acquire one reader
 * reference to the visible snapshot. The returned view is immutable and MUST
 * be passed to suas_skto_name_release() exactly once. It remains valid until
 * that release, table re-initialisation, or table destruction; publications
 * cannot overwrite it while the reference is held.
 *
 * When out_view is NULL no reference is acquired; *out_generation is then a
 * point-in-time observation only and confers no lifetime protection.
 * Returns SKTO_E_BUSY if the snapshot's reader count is exhausted. */
skto_status_t suas_skto_resolve(skto_name_table_t *table,
                                const skto_text_t *name,
                                uint32_t *out_generation,
                                skto_text_t *out_view);

/* Release one reader reference acquired by suas_skto_resolve().
 * The reference is matched by buffer address, generation, and length, so a
 * stale or double release does not decrement a different reader. Returns
 * SKTO_E_NOTFOUND when the view no longer identifies a held reference.
 * A record becomes reclaimable only after its final reference is released;
 * it is reused lazily by a later publication, never in the background. */
skto_status_t suas_skto_name_release(skto_name_table_t *table,
                                     const skto_text_t *view);

/* Withdraw an equal name from lookup. The slot stops matching immediately, so
 * no later resolve can observe the retired name. A snapshot already acquired
 * by a reader is NOT rewritten: the slot is reclaimed only after its final
 * reader calls suas_skto_name_release(), at which point its storage becomes
 * reusable by a later publication.
 *
 * Retirement is permitted while the table is shutting down, because it is part
 * of teardown rather than admission. It is idempotent in the sense that a
 * missing or already-retired name returns SKTO_E_NOTFOUND and changes nothing.
 * Returns SKTO_E_STATE when the query is not a structurally valid SKTO object.
 *
 * This is an implementation extension used by a bounded advisory namespace
 * index; SUAS-018 does not require a retirement operation. */
skto_status_t suas_skto_retire(skto_name_table_t *table,
                               const skto_text_t *name);

/* Namespace name rules N2, N3, N5 (N4 and N6 belong to SUAS-007/SUAS-004). */
skto_status_t suas_skto_name_check(const skto_text_t *name,
                                   const skto_limits_t *limits);

#ifdef __cplusplus
}
#endif

#endif /* SUAS_SUAS_SKTO_H */
