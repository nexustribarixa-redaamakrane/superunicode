/* SUAS-018 - System Kernel Text Objects (SKTO)
 *
 * Reference implementation of the mandatory kernel text contract.
 * Freestanding C99, zero allocation, no libc string functions.
 *
 * See docs/suas/SUAS-018-skto.md for the normative specification.
 */

#include "suas/suas_skto.h"

/* -- Internal helpers ------------------------------------------------- */

static int skto_is_cont(uint8_t b)
{
    return (b & 0xC0u) == 0x80u;
}

/* Decode one SUTF-8 sequence. Returns SKTO_OK and the consumed length, or
 * SKTO_E_ENCODING for a malformed sequence. Values that are structurally
 * invalid for a text object (SCP, traps, Sentinel, surrogates) are reported
 * as SKTO_E_INADMISSIBLE once the sequence decodes cleanly. */
static skto_status_t skto_decode(const uint8_t *p, size_t n,
                                 size_t *used, uint32_t *out_cp)
{
    uint8_t b0;
    uint32_t cp;
    size_t need;
    size_t i;

    if (n == 0u) {
        return SKTO_E_RANGE;
    }

    b0 = p[0];
    if (b0 < 0x80u) {
        cp = (uint32_t)b0;
        *used = 1u;
        *out_cp = cp;
    } else if ((b0 & 0xE0u) == 0xC0u) {
        need = 2u;
        if (n < need) return SKTO_E_ENCODING;
        if (!skto_is_cont(p[1])) return SKTO_E_ENCODING;
        cp = ((uint32_t)(b0 & 0x1Fu) << 6) | (uint32_t)(p[1] & 0x3Fu);
        if (cp < 0x80u) return SKTO_E_ENCODING; /* overlong */
        *used = need;
        *out_cp = cp;
    } else if ((b0 & 0xF0u) == 0xE0u) {
        need = 3u;
        if (n < need) return SKTO_E_ENCODING;
        for (i = 1u; i < need; i++) {
            if (!skto_is_cont(p[i])) return SKTO_E_ENCODING;
        }
        cp = ((uint32_t)(b0 & 0x0Fu) << 12) |
             ((uint32_t)(p[1] & 0x3Fu) << 6) |
             (uint32_t)(p[2] & 0x3Fu);
        if (cp < 0x800u) return SKTO_E_ENCODING; /* overlong */
        if (cp >= 0xD800u && cp <= 0xDFFFu) return SKTO_E_ENCODING;
        *used = need;
        *out_cp = cp;
    } else if ((b0 & 0xF8u) == 0xF0u) {
        need = 4u;
        if (n < need) return SKTO_E_ENCODING;
        for (i = 1u; i < need; i++) {
            if (!skto_is_cont(p[i])) return SKTO_E_ENCODING;
        }
        cp = ((uint32_t)(b0 & 0x07u) << 18) |
             ((uint32_t)(p[1] & 0x3Fu) << 12) |
             ((uint32_t)(p[2] & 0x3Fu) << 6) |
             (uint32_t)(p[3] & 0x3Fu);
        if (cp < 0x10000u) return SKTO_E_ENCODING;    /* overlong        */
        if (cp > 0x0010FFFFu) return SKTO_E_ENCODING; /* 5-byte framing  */
        *used = need;
        *out_cp = cp;
    } else if ((b0 & 0xFCu) == 0xF8u) {
        need = 5u;
        if (n < need) return SKTO_E_ENCODING;
        for (i = 1u; i < need; i++) {
            if (!skto_is_cont(p[i])) return SKTO_E_ENCODING;
        }
        cp = ((uint32_t)(b0 & 0x03u) << 24) |
             ((uint32_t)(p[1] & 0x3Fu) << 18) |
             ((uint32_t)(p[2] & 0x3Fu) << 12) |
             ((uint32_t)(p[3] & 0x3Fu) << 6) |
             (uint32_t)(p[4] & 0x3Fu);
        if (cp < 0x00110000u) return SKTO_E_ENCODING; /* overlong       */
        if (cp > 0x03FFFFFFu) return SKTO_E_ENCODING; /* 6-byte framing */
        *used = need;
        *out_cp = cp;
    } else if ((b0 & 0xFEu) == 0xFCu) {
        need = 6u;
        if (n < need) return SKTO_E_ENCODING;
        for (i = 1u; i < need; i++) {
            if (!skto_is_cont(p[i])) return SKTO_E_ENCODING;
        }
        cp = ((uint32_t)(b0 & 0x01u) << 30) |
             ((uint32_t)(p[1] & 0x3Fu) << 24) |
             ((uint32_t)(p[2] & 0x3Fu) << 18) |
             ((uint32_t)(p[3] & 0x3Fu) << 12) |
             ((uint32_t)(p[4] & 0x3Fu) << 6) |
             (uint32_t)(p[5] & 0x3Fu);
        if (cp < 0x04000000u) return SKTO_E_ENCODING; /* overlong */
        *used = need;
        *out_cp = cp;
    } else {
        return SKTO_E_ENCODING; /* bare continuation or 0xFE/0xFF */
    }

    /* V6 and V7: the System Control Plane and the sixteen terminal values are
     * not text. Note that 0x7FFFFFFF is the Sentinel and is unallocatable. */
    if (cp >= 0x00110000u && cp <= 0x0011FFFFu) {
        return SKTO_E_INADMISSIBLE;
    }
    if (cp >= 0x7FFFFFF0u) {
        return SKTO_E_INADMISSIBLE;
    }

    return SKTO_OK;
}

static int skto_bytes_equal(const uint8_t *a, const uint8_t *b, size_t n)
{
    size_t i;
    for (i = 0u; i < n; i++) {
        if (a[i] != b[i]) {
            return 0;
        }
    }
    return 1;
}

static size_t skto_bytes_copy(uint8_t *dst, const uint8_t *src, size_t n)
{
    size_t i;
    for (i = 0u; i < n; i++) {
        dst[i] = src[i];
    }
    return n;
}

static skto_status_t skto_scan(const uint8_t *bytes, size_t len,
                               int *out_has_nul)
{
    size_t pos = 0u;
    size_t used = 0u;
    uint32_t cp = 0u;
    skto_status_t st;

    if (out_has_nul != NULL) {
        *out_has_nul = 0;
    }

    while (pos < len) {
        st = skto_decode(bytes + pos, len - pos, &used, &cp);
        if (st != SKTO_OK) {
            return st;
        }
        if (cp == 0u && out_has_nul != NULL) {
            *out_has_nul = 1;
        }
        pos += used;
    }
    return SKTO_OK;
}

static size_t skto_limits_admit(const skto_limits_t *limits)
{
    if (limits == NULL || limits->max_admit_bytes == 0u) {
        return (size_t)SKTO_MAX_ADMIT_BYTES;
    }
    if (limits->max_admit_bytes < (size_t)SKTO_MAX_ADMIT_BYTES) {
        return limits->max_admit_bytes;
    }
    return (size_t)SKTO_MAX_ADMIT_BYTES; /* a caller may lower, not raise */
}

static size_t skto_limits_name(const skto_limits_t *limits)
{
    if (limits == NULL || limits->max_name_bytes == 0u) {
        return (size_t)SKTO_NAME_MAX_BYTES;
    }
    if (limits->max_name_bytes < (size_t)SKTO_NAME_MAX_BYTES) {
        return limits->max_name_bytes;
    }
    return (size_t)SKTO_NAME_MAX_BYTES;
}

/* -- Version and defaults --------------------------------------------- */

const char *suas_skto_version_string(void)
{
    return "SUAS-018 SKTO 1.0.0";
}

void suas_skto_limits_default(skto_limits_t *limits)
{
    if (limits == NULL) {
        return;
    }
    limits->max_admit_bytes = (size_t)SKTO_MAX_ADMIT_BYTES;
    limits->max_name_bytes = (size_t)SKTO_NAME_MAX_BYTES;
}

/* -- Construction ------------------------------------------------------ */

skto_status_t suas_skto_init_static(skto_text_t *t, const void *bytes,
                                    size_t len)
{
    if (t == NULL) {
        return SKTO_E_NULL;
    }
    if (bytes == NULL && len != 0u) {
        return SKTO_E_NULL;
    }
    t->bytes = (const uint8_t *)bytes;
    t->len = len;
    t->cap = len;
    t->owner = (uint16_t)SKTO_OWN_STATIC;
    t->encoding = (uint16_t)SKTO_ENC_SUTF8;
    t->flags = SKTO_F_CONST;
    t->generation = 0u;
    return SKTO_OK;
}

skto_status_t suas_skto_init_borrowed(skto_text_t *t, const void *bytes,
                                      size_t len)
{
    if (t == NULL) {
        return SKTO_E_NULL;
    }
    if (bytes == NULL && len != 0u) {
        return SKTO_E_NULL;
    }
    t->bytes = (const uint8_t *)bytes;
    t->len = len;
    t->cap = len;
    t->owner = (uint16_t)SKTO_OWN_BORROWED;
    t->encoding = (uint16_t)SKTO_ENC_SUTF8;
    t->flags = SKTO_F_CONST;
    t->generation = 0u;
    return SKTO_OK;
}

skto_status_t suas_skto_init_owned(skto_text_t *t, void *buffer, size_t cap)
{
    if (t == NULL) {
        return SKTO_E_NULL;
    }
    if (buffer == NULL && cap != 0u) {
        return SKTO_E_NULL;
    }
    t->bytes = (const uint8_t *)buffer;
    t->len = 0u;
    t->cap = cap;
    t->owner = (uint16_t)SKTO_OWN_OWNED;
    t->encoding = (uint16_t)SKTO_ENC_SUTF8;
    t->flags = 0u;
    t->generation = 0u;
    return SKTO_OK;
}

void suas_skto_init_empty(skto_text_t *t)
{
    if (t == NULL) {
        return;
    }
    t->bytes = NULL;
    t->len = 0u;
    t->cap = 0u;
    t->owner = (uint16_t)SKTO_OWN_STATIC;
    t->encoding = (uint16_t)SKTO_ENC_SUTF8;
    t->flags = 0u;
    t->generation = 0u;
}

/* -- Validation -------------------------------------------------------- */

skto_status_t suas_skto_check(const skto_text_t *t)
{
    if (t == NULL) {
        return SKTO_E_NULL;
    }
    if (t->bytes == NULL && t->len != 0u) {
        return SKTO_E_NULL;              /* V1 */
    }
    if (t->len > t->cap) {
        return SKTO_E_STATE;             /* V2 */
    }
    if (t->owner > (uint16_t)SKTO_OWN_PUBLISHED) {
        return SKTO_E_OWNER;             /* V3 */
    }
    if (t->encoding != (uint16_t)SKTO_ENC_SUTF8) {
        return SKTO_E_STATE;             /* V4 */
    }
    return SKTO_OK;
}

skto_status_t suas_skto_validate(const skto_text_t *t)
{
    skto_status_t st;
    int has_nul = 0;

    st = suas_skto_check(t);
    if (st != SKTO_OK) {
        return st;
    }

    if (t->len == 0u) {
        if ((t->flags & SKTO_F_NUL_SAFE) != 0u) {
            /* An empty NUL-safe view still needs a readable terminator. */
            if (t->bytes == NULL || t->bytes[0] != 0u) {
                return SKTO_E_STATE;
            }
        }
        return SKTO_OK;
    }

    st = skto_scan(t->bytes, t->len, &has_nul);
    if (st != SKTO_OK) {
        return st;
    }

    if ((t->flags & SKTO_F_NUL_SAFE) != 0u) {
        if (has_nul) {
            return SKTO_E_STATE;          /* V8: no interior NUL */
        }
        if (t->bytes[t->len] != 0u) {
            return SKTO_E_STATE;          /* V8: terminator      */
        }
    }

    return SKTO_OK;
}

skto_status_t suas_skto_mark_nul_safe(skto_text_t *t)
{
    size_t i;

    if (t == NULL) {
        return SKTO_E_NULL;
    }
    if (suas_skto_check(t) != SKTO_OK) {
        return SKTO_E_STATE;
    }
    if (t->owner == (uint16_t)SKTO_OWN_STATIC ||
        t->owner == (uint16_t)SKTO_OWN_BORROWED ||
        t->owner == (uint16_t)SKTO_OWN_PUBLISHED) {
        /* Read-only views cannot be given a terminator they do not already
         * have; the producer must have arranged it. */
        if (t->bytes == NULL) {
            return SKTO_E_STATE;
        }
        if (t->bytes[t->len] != 0u) {
            return SKTO_E_STATE;
        }
    }

    if (t->bytes == NULL) {
        return SKTO_E_STATE;
    }
    for (i = 0u; i < t->len; i++) {
        if (t->bytes[i] == 0u) {
            return SKTO_E_STATE;
        }
    }
    if (t->bytes[t->len] != 0u) {
        return SKTO_E_STATE;
    }

    t->flags |= SKTO_F_NUL_SAFE;
    return SKTO_OK;
}

/* -- Reading ----------------------------------------------------------- */

skto_status_t suas_skto_next(const skto_text_t *t, size_t *pos,
                             uint32_t *out_cp)
{
    skto_status_t st;
    size_t used = 0u;
    uint32_t cp = 0u;

    if (t == NULL || pos == NULL || out_cp == NULL) {
        return SKTO_E_NULL;
    }
    if (suas_skto_check(t) != SKTO_OK) {
        return SKTO_E_STATE;
    }
    if (*pos >= t->len) {
        return SKTO_E_RANGE;
    }

    st = skto_decode(t->bytes + *pos, t->len - *pos, &used, &cp);
    if (st != SKTO_OK) {
        return st;
    }
    *pos += used;
    *out_cp = cp;
    return SKTO_OK;
}

skto_status_t suas_skto_count(const skto_text_t *t, size_t *out_count)
{
    size_t pos = 0u;
    size_t used = 0u;
    size_t count = 0u;
    uint32_t cp = 0u;
    skto_status_t st;

    if (out_count == NULL) {
        return SKTO_E_NULL;
    }
    *out_count = 0u;
    if (t == NULL) {
        return SKTO_E_NULL;
    }
    if (suas_skto_check(t) != SKTO_OK) {
        return SKTO_E_STATE;
    }

    while (pos < t->len) {
        st = skto_decode(t->bytes + pos, t->len - pos, &used, &cp);
        if (st != SKTO_OK) {
            return st;
        }
        pos += used;
        count++;
    }
    *out_count = count;
    return SKTO_OK;
}

skto_status_t suas_skto_copy(const skto_text_t *t, uint8_t *dst, size_t cap,
                             size_t *out_len)
{
    if (out_len != NULL) {
        *out_len = 0u;
    }
    if (t == NULL) {
        return SKTO_E_NULL;
    }
    if (suas_skto_check(t) != SKTO_OK) {
        return SKTO_E_STATE;
    }
    if (dst == NULL && t->len != 0u) {
        return SKTO_E_NULL;
    }
    if (cap < t->len) {
        return SKTO_E_CAPACITY;
    }
    if (t->len != 0u) {
        (void)skto_bytes_copy(dst, t->bytes, t->len);
    }
    if (out_len != NULL) {
        *out_len = t->len;
    }
    return SKTO_OK;
}

/* -- Comparison and hash ---------------------------------------------- */

bool suas_skto_equal(const skto_text_t *a, const skto_text_t *b)
{
    if (a == NULL || b == NULL) {
        return false;
    }
    if (a->len != b->len) {
        return false;
    }
    if (a->len == 0u) {
        return true;
    }
    if (a->bytes == NULL || b->bytes == NULL) {
        return false;
    }
    return skto_bytes_equal(a->bytes, b->bytes, a->len) != 0;
}

int suas_skto_compare(const skto_text_t *a, const skto_text_t *b)
{
    size_t i;
    size_t n;

    if (a == NULL || b == NULL) {
        return (a == b) ? 0 : (a == NULL ? -1 : 1);
    }
    n = (a->len < b->len) ? a->len : b->len;
    for (i = 0u; i < n; i++) {
        uint8_t ca = a->bytes[i];
        uint8_t cb = b->bytes[i];
        if (ca != cb) {
            return (ca < cb) ? -1 : 1;
        }
    }
    if (a->len == b->len) {
        return 0;
    }
    return (a->len < b->len) ? -1 : 1;
}

uint32_t suas_skto_hash(const skto_text_t *t)
{
    uint32_t h = 2166136261u; /* FNV-1a 32-bit, the SUAS-010 function */
    size_t i;

    if (t == NULL) {
        return 0u;
    }
    if (t->bytes == NULL) {
        return h;
    }
    for (i = 0u; i < t->len; i++) {
        h ^= (uint32_t)t->bytes[i];
        h *= 16777619u;
    }
    return h;
}

/* -- Admission --------------------------------------------------------- */

skto_status_t suas_skto_admit(const void *src, size_t len,
                              skto_range_fn range, void *range_ctx,
                              const skto_limits_t *limits,
                              uint8_t *dst, size_t cap,
                              skto_text_t *out, size_t *written)
{
    uintptr_t base;
    size_t max;
    size_t pos = 0u;
    size_t used = 0u;
    uint32_t cp = 0u;
    skto_status_t st;
    int has_nul = 0;

    if (written != NULL) {
        *written = 0u;
    }
    if (out == NULL) {
        return SKTO_E_NULL;
    }
    if (src == NULL && len != 0u) {
        return SKTO_E_NULL;                       /* step 1 */
    }

    max = skto_limits_admit(limits);
    if (len > max) {
        return SKTO_E_OVERFLOW;                   /* step 2: ceiling */
    }

    base = (uintptr_t)src;
    if (base > (uintptr_t)(~(uintptr_t)0) - (uintptr_t)len) {
        return SKTO_E_OVERFLOW;                   /* step 2: arithmetic */
    }

    if (len != 0u && range != NULL) {
        if (!range(range_ctx, src, len)) {
            return SKTO_E_RANGE;                  /* step 3 */
        }
    }

    /* Steps 5 and 6: encoding and admissibility, one pass, no copy. */
    while (pos < len) {
        st = skto_decode((const uint8_t *)src + pos, len - pos, &used, &cp);
        if (st != SKTO_OK) {
            return st;
        }
        if (cp == 0u) {
            has_nul = 1;
        }
        pos += used;
    }

    if (dst == NULL && len != 0u) {
        return SKTO_E_NULL;                       /* step 8: storage */
    }
    if (cap < len) {
        return SKTO_E_CAPACITY;                   /* step 8: capacity */
    }

    if (len != 0u) {
        (void)skto_bytes_copy(dst, (const uint8_t *)src, len);
    }

    out->bytes = dst;
    out->len = len;
    out->cap = cap;
    out->owner = (uint16_t)SKTO_OWN_OWNED;
    out->encoding = (uint16_t)SKTO_ENC_SUTF8;
    out->flags = SKTO_F_ADMITTED;
    out->generation = 0u;

    if (cap > len && !has_nul) {
        dst[len] = 0u;
        out->flags |= SKTO_F_NUL_SAFE;
    }

    if (written != NULL) {
        *written = len;
    }
    return SKTO_OK;
}

/* -- Names ------------------------------------------------------------- */

void suas_skto_names_init(skto_name_table_t *table)
{
    size_t i;

    if (table == NULL) {
        return;
    }
    for (i = 0u; i < (size_t)SKTO_NAME_CAPACITY; i++) {
        table->slots[i].len = 0u;
        table->slots[i].generation = 0u;
        table->slots[i].live = 0u;
    }
    table->live = 0u;
}

skto_status_t suas_skto_name_check(const skto_text_t *name,
                                   const skto_limits_t *limits)
{
    size_t i;
    size_t max;

    if (name == NULL) {
        return SKTO_E_NULL;
    }
    if (suas_skto_validate(name) != SKTO_OK) {
        return SKTO_E_STATE;                   /* N1 */
    }
    if ((name->flags & SKTO_F_NUL_SAFE) == 0u) {
        return SKTO_E_POLICY;                  /* N3 */
    }
    for (i = 0u; i < name->len; i++) {
        if (name->bytes[i] == 0u) {
            return SKTO_E_POLICY;              /* N2 */
        }
    }

    max = skto_limits_name(limits);
    if (name->len > max) {
        return SKTO_E_POLICY;                  /* N5 */
    }
    return SKTO_OK;
}

static int skto_name_matches(const skto_name_slot_t *slot,
                             const skto_text_t *name)
{
    if (slot->live == 0u || (size_t)slot->len != name->len) {
        return 0;
    }
    return skto_bytes_equal(slot->bytes, name->bytes, name->len) != 0;
}

skto_status_t suas_skto_publish(skto_name_table_t *table,
                                const skto_text_t *name,
                                const skto_limits_t *limits,
                                uint32_t *out_generation)
{
    skto_status_t st;
    size_t i;
    int free_slot = -1;

    if (out_generation != NULL) {
        *out_generation = 0u;
    }
    if (table == NULL || name == NULL) {
        return SKTO_E_NULL;
    }

    st = suas_skto_name_check(name, limits);
    if (st != SKTO_OK) {
        return st;
    }

    for (i = 0u; i < (size_t)SKTO_NAME_CAPACITY; i++) {
        if (skto_name_matches(&table->slots[i], name)) {
            uint32_t next = table->slots[i].generation + 1u;
            (void)skto_bytes_copy(table->slots[i].bytes, name->bytes,
                                  name->len);
            table->slots[i].len = (uint16_t)name->len;
            table->slots[i].bytes[name->len] = 0u;
            table->slots[i].generation = next;
            if (out_generation != NULL) {
                *out_generation = next;
            }
            return SKTO_OK;
        }
        if (table->slots[i].live == 0u && free_slot < 0) {
            free_slot = (int)i;
        }
    }

    if (free_slot < 0) {
        return SKTO_E_FULL;
    }

    i = (size_t)free_slot;
    (void)skto_bytes_copy(table->slots[i].bytes, name->bytes, name->len);
    table->slots[i].len = (uint16_t)name->len;
    table->slots[i].bytes[name->len] = 0u;
    table->slots[i].generation = 0u;
    table->slots[i].live = 1u;
    table->live++;

    if (out_generation != NULL) {
        *out_generation = 0u;
    }
    return SKTO_OK;
}

skto_status_t suas_skto_resolve(const skto_name_table_t *table,
                                const skto_text_t *name,
                                uint32_t *out_generation,
                                skto_text_t *out_view)
{
    size_t i;

    if (out_generation != NULL) {
        *out_generation = 0u;
    }
    if (table == NULL || name == NULL) {
        return SKTO_E_NULL;
    }
    if (suas_skto_check(name) != SKTO_OK) {
        return SKTO_E_STATE;
    }

    for (i = 0u; i < (size_t)SKTO_NAME_CAPACITY; i++) {
        const skto_name_slot_t *slot = &table->slots[i];
        if (!skto_name_matches(slot, name)) {
            continue;
        }
        if (out_generation != NULL) {
            *out_generation = slot->generation;
        }
        if (out_view != NULL) {
            out_view->bytes = slot->bytes;
            out_view->len = (size_t)slot->len;
            out_view->cap = (size_t)slot->len;
            out_view->owner = (uint16_t)SKTO_OWN_PUBLISHED;
            out_view->encoding = (uint16_t)SKTO_ENC_SUTF8;
            out_view->flags = SKTO_F_CONST | SKTO_F_NAME | SKTO_F_NUL_SAFE;
            out_view->generation = slot->generation;
        }
        return SKTO_OK;
    }
    return SKTO_E_NOTFOUND;
}
