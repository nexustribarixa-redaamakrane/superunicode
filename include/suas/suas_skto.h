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

/* Name ceiling. SKTO does not impose a namespace's limit; it only refuses to
 * publish a name it cannot store here. A namespace with a larger limit keeps
 * its own storage (section 9.1 N5). */
#define SKTO_NAME_MAX_BYTES    64u
#define SKTO_NAME_CAPACITY     16u

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
    SKTO_E_NOTFOUND    = 12  /* no such name or generation         */
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

/* -- Validation (section 4.3) ----------------------------------------- */

/* O(1) structural check: V1-V4. */
skto_status_t suas_skto_check(const skto_text_t *t);

/* Full check: V1-V8, including a single SUTF-8 scan. */
skto_status_t suas_skto_validate(const skto_text_t *t);

/* Assert the NUL-safe view. Verifies the byte and rejects interior NUL. */
skto_status_t suas_skto_mark_nul_safe(skto_text_t *t);

/* -- Reading ---------------------------------------------------------- */

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

/* -- Names (section 9) ------------------------------------------------ */

typedef struct {
    uint8_t  bytes[SKTO_NAME_MAX_BYTES + 1]; /* text + NUL */
    uint16_t len;
    uint32_t generation;
    uint8_t  live;
} skto_name_slot_t;

typedef struct {
    skto_name_slot_t slots[SKTO_NAME_CAPACITY];
    uint16_t live;
} skto_name_table_t;

void          suas_skto_names_init(skto_name_table_t *table);

/* Publish a name, or replace an equal one with generation + 1.
 * The bytes are copied; the caller's buffer is never retained. */
skto_status_t suas_skto_publish(skto_name_table_t *table,
                                const skto_text_t *name,
                                const skto_limits_t *limits,
                                uint32_t *out_generation);

/* Resolve an equal name. On success *out_view (when non-NULL) receives an
 * immutable PUBLISHED view bound to the same generation that is reported
 * through *out_generation. The view remains valid until the slot is
 * republished or the table is re-initialised. */
skto_status_t suas_skto_resolve(const skto_name_table_t *table,
                                const skto_text_t *name,
                                uint32_t *out_generation,
                                skto_text_t *out_view);

/* Namespace name rules N2, N3, N5 (N4 and N6 belong to SUAS-007/SUAS-004). */
skto_status_t suas_skto_name_check(const skto_text_t *name,
                                   const skto_limits_t *limits);

#ifdef __cplusplus
}
#endif

#endif /* SUAS_SUAS_SKTO_H */
