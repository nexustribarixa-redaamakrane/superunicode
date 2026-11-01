/**
 * SUAS-018 SKTO - System Kernel Text Objects
 *
 * Unit tests for the mandatory kernel text contract: bounded reads, capacity
 * refusal, ownership classes, admission of untrusted text, publication
 * immutability, and byte-exact comparison.
 *
 * The suite deliberately reads only through the descriptor. Guard bytes after
 * every test buffer prove that no operation inspects bytes[len].
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "suas/suas_skto.h"

#ifdef _WIN32
#include <windows.h>
#endif

static int g_fail = 0;
#define CHECK(cond) do { if (!(cond)) { \
    printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); g_fail = 1; \
} } while (0)

static void test_version_and_defaults(void)
{
    skto_limits_t lim;

    CHECK(suas_skto_version_string() != NULL);
    CHECK(strstr(suas_skto_version_string(), "SUAS-018") != NULL);

    memset(&lim, 0xFF, sizeof lim);
    suas_skto_limits_default(&lim);
    CHECK(lim.max_admit_bytes == (size_t)SKTO_MAX_ADMIT_BYTES);
    CHECK(lim.max_name_bytes == (size_t)SKTO_NAME_MAX_BYTES);

    /* A NULL limits pointer is accepted everywhere. */
    suas_skto_limits_default(NULL);
}

static void test_structural_rejection(void)
{
    skto_text_t t;
    uint8_t b[4] = { 'a', 'b', 'c', 'd' };

    CHECK(suas_skto_check(NULL) == SKTO_E_NULL);

    CHECK(suas_skto_init_borrowed(&t, b, 2) == SKTO_OK);
    CHECK(suas_skto_check(&t) == SKTO_OK);

    t.len = 5;                      /* V2: len > cap */
    CHECK(suas_skto_check(&t) == SKTO_E_STATE);

    t.len = 1;
    t.bytes = NULL;                 /* V1: NULL with len */
    CHECK(suas_skto_check(&t) == SKTO_E_NULL);

    t.bytes = b;
    t.owner = 99;                   /* V3: unknown owner */
    CHECK(suas_skto_check(&t) == SKTO_E_OWNER);

    t.owner = (uint16_t)SKTO_OWN_BORROWED;
    t.encoding = 7;                 /* V4: unknown encoding */
    CHECK(suas_skto_check(&t) == SKTO_E_STATE);

    CHECK(suas_skto_init_borrowed(&t, NULL, 3) == SKTO_E_NULL);
    CHECK(suas_skto_init_static(&t, NULL, 3) == SKTO_E_NULL);
    CHECK(suas_skto_init_owned(&t, NULL, 3) == SKTO_E_NULL);
    CHECK(suas_skto_init_static(&t, NULL, 0) == SKTO_OK);
    CHECK(suas_skto_check(&t) == SKTO_OK);
}

static void test_no_terminator_dependence(void)
{
    /* Guard bytes beyond len are non-zero and must never be read as text. */
    uint8_t b[8] = { 'a', 'b', 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF };
    skto_text_t t;
    size_t pos = 0;
    size_t count = 0;
    uint32_t cp = 0;

    CHECK(suas_skto_init_borrowed(&t, b, 2) == SKTO_OK);
    CHECK(suas_skto_validate(&t) == SKTO_OK);  /* bytes[2] is not 0x00 */

    CHECK(suas_skto_next(&t, &pos, &cp) == SKTO_OK);
    CHECK(cp == 'a' && pos == 1);
    CHECK(suas_skto_next(&t, &pos, &cp) == SKTO_OK);
    CHECK(cp == 'b' && pos == 2);
    CHECK(suas_skto_next(&t, &pos, &cp) == SKTO_E_RANGE);

    CHECK(suas_skto_count(&t, &count) == SKTO_OK);
    CHECK(count == 2);

    /* Canaries are untouched. */
    CHECK(b[2] == 0xAA && b[3] == 0xBB);

    /* Empty is valid and distinct from absent. */
    CHECK(suas_skto_init_borrowed(&t, NULL, 0) == SKTO_OK);
    CHECK(suas_skto_validate(&t) == SKTO_OK);
    CHECK(suas_skto_count(&t, &count) == SKTO_OK && count == 0);
    CHECK(suas_skto_copy(&t, NULL, 0, &count) == SKTO_OK && count == 0);
}

static void test_capacity_refusal(void)
{
    uint8_t src[4] = { 'a', 'b', 'c', 'd' };
    uint8_t dst[8];
    skto_text_t t;
    size_t n = 999;

    CHECK(suas_skto_init_borrowed(&t, src, 4) == SKTO_OK);

    memset(dst, 0x5A, sizeof dst);
    CHECK(suas_skto_copy(&t, dst, 3, &n) == SKTO_E_CAPACITY);
    CHECK(n == 0);
    CHECK(dst[0] == 0x5A);                 /* no partial write on failure */

    CHECK(suas_skto_copy(&t, dst, 4, &n) == SKTO_OK);
    CHECK(n == 4);
    CHECK(memcmp(dst, src, 4) == 0);
    CHECK(dst[4] == 0x5A);                 /* writes exactly len bytes */

    CHECK(suas_skto_copy(&t, NULL, 4, &n) == SKTO_E_NULL);
    CHECK(n == 0);
}

static void test_borrowed_lifetime(void)
{
    uint8_t caller[8] = { 'o', 'w', 'n', 'e', 'd', 0, 0, 0 };
    skto_text_t t;

    CHECK(suas_skto_init_borrowed(&t, caller, 5) == SKTO_OK);
    CHECK(t.owner == (uint16_t)SKTO_OWN_BORROWED);
    CHECK(t.bytes == caller);              /* no copy was made */

    /* The borrowed view observes caller storage for the length of the call.
     * It must not be retained: there is no release operation, and the
     * descriptor carries no owned storage. */
    caller[0] = 'O';
    CHECK(t.bytes[0] == 'O');
    CHECK(t.owner != (uint16_t)SKTO_OWN_OWNED);
}

static void test_owned_object(void)
{
    uint8_t buf[8];
    skto_text_t t;

    CHECK(suas_skto_init_owned(&t, buf, sizeof buf) == SKTO_OK);
    CHECK(t.len == 0);
    CHECK(t.cap == sizeof buf);
    CHECK(t.owner == (uint16_t)SKTO_OWN_OWNED);
    CHECK(suas_skto_validate(&t) == SKTO_OK);
    CHECK(suas_skto_mark_nul_safe(&t) == SKTO_E_STATE); /* no content yet */
}

static void test_legacy_cstr_and_assign(void)
{
    uint8_t buf[8];
    uint8_t src[8];
    skto_text_t t;
    skto_text_t read_only;

    /* Bounded legacy constructor: the bound includes the terminator. */
    CHECK(suas_skto_init_legacy_cstr(NULL, "x", 2) == SKTO_E_NULL);
    CHECK(suas_skto_init_legacy_cstr(&t, NULL, 2) == SKTO_E_NULL);
    CHECK(suas_skto_init_legacy_cstr(&t, "abc", 4) == SKTO_OK);
    CHECK(t.len == 3 && t.cap == 3);
    CHECK(t.owner == (uint16_t)SKTO_OWN_BORROWED);
    CHECK((t.flags & SKTO_F_NUL_SAFE) != 0);
    CHECK(suas_skto_validate(&t) == SKTO_OK);
    CHECK(suas_skto_init_legacy_cstr(&t, "abc", 3) == SKTO_E_TERMINATOR);
    CHECK(suas_skto_init_legacy_cstr(&t, "", 1) == SKTO_OK);
    CHECK(t.len == 0 && t.bytes != NULL && t.bytes[0] == 0);

    /* Owned assignment is bounded, validated, and terminational. */
    CHECK(suas_skto_init_owned(&t, buf, sizeof buf) == SKTO_OK);
    CHECK(suas_skto_assign(&t, "hey", 3) == SKTO_OK);
    CHECK(t.len == 3);
    CHECK((t.flags & SKTO_F_ADMITTED) != 0);
    CHECK((t.flags & SKTO_F_NUL_SAFE) != 0);
    CHECK(buf[3] == 0);
    CHECK(suas_skto_validate(&t) == SKTO_OK);

    /* Capacity refusal leaves the descriptor and bytes unchanged. */
    memcpy(src, "abcdefgh", sizeof src);
    CHECK(suas_skto_assign(&t, src, 9) == SKTO_E_CAPACITY);
    CHECK(t.len == 3 && memcmp(buf, "hey", 3) == 0);

    /* Inadmissible content is rejected before the destination is written. */
    CHECK(suas_skto_assign(&t, "\xF8\x84\x90\x80\x80", 5) ==
          SKTO_E_INADMISSIBLE);
    CHECK(t.len == 3 && memcmp(buf, "hey", 3) == 0);
    CHECK(suas_skto_assign(&t, "\xE2\x82", 2) == SKTO_E_ENCODING);
    CHECK(t.len == 3 && memcmp(buf, "hey", 3) == 0);

    /* Only an owned object accepts assignment. */
    CHECK(suas_skto_init_borrowed(&read_only, "abc", 3) == SKTO_OK);
    CHECK(suas_skto_assign(&read_only, "z", 1) == SKTO_E_OWNER);

    /* Overlapping source and destination are copied in the safe direction. */
    CHECK(suas_skto_assign(&t, "abcdef", 6) == SKTO_OK);
    CHECK(suas_skto_assign(&t, buf + 1, 3) == SKTO_OK);
    CHECK(t.len == 3 && memcmp(buf, "bcd", 3) == 0);
    CHECK(buf[3] == 0);

    /* An interior NUL clears NUL_SAFE even though spare capacity exists. */
    CHECK(suas_skto_assign(&t, "a\0b", 3) == SKTO_OK);
    CHECK(t.len == 3);
    CHECK((t.flags & SKTO_F_NUL_SAFE) == 0);
    CHECK(suas_skto_validate(&t) == SKTO_OK);
}

/* -- Admission --------------------------------------------------------- */

static bool range_ok(void *ctx, const void *base, size_t len)
{
    (void)base;
    (void)len;
    return *(int *)ctx != 0;
}

static void test_admission(void)
{
    uint8_t dst[16];
    skto_text_t t;
    size_t written = 999;
    int allow = 1;
    int deny = 0;
    skto_limits_t lim;

    /* Success, with a trailing NUL for free. */
    CHECK(suas_skto_admit("hello", 5, range_ok, &allow, NULL,
                          dst, sizeof dst, &t, &written) == SKTO_OK);
    CHECK(written == 5);
    CHECK(t.len == 5 && t.cap == sizeof dst);
    CHECK(t.owner == (uint16_t)SKTO_OWN_OWNED);
    CHECK((t.flags & SKTO_F_ADMITTED) != 0);
    CHECK((t.flags & SKTO_F_NUL_SAFE) != 0);
    CHECK(dst[5] == 0);
    CHECK(suas_skto_validate(&t) == SKTO_OK);

    /* NUL-terminated admission: containment covers the scan window, and the
     * terminator must occur inside the admission ceiling. */
    CHECK(suas_skto_admit_cstr("hello", 6, range_ok, &allow, NULL,
                               dst, sizeof dst, &t, &written) == SKTO_OK);
    CHECK(written == 5 && t.len == 5 && dst[5] == 0);
    CHECK(suas_skto_admit_cstr("hello", 5, NULL, NULL, NULL,
                               dst, sizeof dst, &t, &written) ==
          SKTO_E_TERMINATOR);
    CHECK(written == 0);
    CHECK(suas_skto_admit_cstr(NULL, 1, NULL, NULL, NULL,
                               dst, sizeof dst, &t, &written) == SKTO_E_NULL);
    CHECK(suas_skto_admit_cstr("hello", 6, range_ok, &deny, NULL,
                               dst, sizeof dst, &t, &written) == SKTO_E_RANGE);
    {
        skto_limits_t scan_lim;
        suas_skto_limits_default(&scan_lim);
        scan_lim.max_admit_bytes = 4;
        CHECK(suas_skto_admit_cstr("hello", 6, NULL, NULL, &scan_lim,
                                   dst, sizeof dst, &t, &written) ==
              SKTO_E_TERMINATOR);
    }
    /* The first NUL ends the candidate; bytes behind it are never read. */
    CHECK(suas_skto_admit_cstr("a\0XY", 4, NULL, NULL, NULL,
                               dst, sizeof dst, &t, &written) == SKTO_OK);
    CHECK(written == 1 && t.len == 1 && dst[0] == 'a');

    /* Step 1: absent pointer with length. */
    CHECK(suas_skto_admit(NULL, 3, NULL, NULL, NULL,
                          dst, sizeof dst, &t, &written) == SKTO_E_NULL);
    CHECK(written == 0);

    /* Step 2: arithmetic overflow. */
    CHECK(suas_skto_admit((const void *)(uintptr_t)~(uintptr_t)0, 4,
                          NULL, NULL, NULL,
                          dst, sizeof dst, &t, &written) == SKTO_E_OVERFLOW);

    /* Step 2: the admission ceiling is a hard bound. */
    suas_skto_limits_default(&lim);
    lim.max_admit_bytes = 4;
    CHECK(suas_skto_admit("hello", 5, NULL, NULL, &lim,
                          dst, sizeof dst, &t, &written) == SKTO_E_OVERFLOW);

    /* Step 3: containment failure. */
    CHECK(suas_skto_admit("hello", 5, range_ok, &deny, NULL,
                          dst, sizeof dst, &t, &written) == SKTO_E_RANGE);

    /* Step 5: overlong C0 80. */
    CHECK(suas_skto_admit("\xC0\x80", 2, NULL, NULL, NULL,
                          dst, sizeof dst, &t, &written) == SKTO_E_ENCODING);
    /* Step 5: truncated E2 82. */
    CHECK(suas_skto_admit("\xE2\x82", 2, NULL, NULL, NULL,
                          dst, sizeof dst, &t, &written) == SKTO_E_ENCODING);
    /* Step 5: surrogate ED A0 80. */
    CHECK(suas_skto_admit("\xED\xA0\x80", 3, NULL, NULL, NULL,
                          dst, sizeof dst, &t, &written) == SKTO_E_ENCODING);
    /* Step 5: bare continuation. */
    CHECK(suas_skto_admit("\x80", 1, NULL, NULL, NULL,
                          dst, sizeof dst, &t, &written) == SKTO_E_ENCODING);

    /* Step 6: SCP U+110000 encodes as F8 84 90 80 80. */
    CHECK(suas_skto_admit("\xF8\x84\x90\x80\x80", 5, NULL, NULL, NULL,
                          dst, sizeof dst, &t, &written) ==
          SKTO_E_INADMISSIBLE);
    /* Step 6: Sentinel U+7FFFFFFF encodes as FD BF BF BF BF BF. */
    CHECK(suas_skto_admit("\xFD\xBF\xBF\xBF\xBF\xBF", 6, NULL, NULL, NULL,
                          dst, sizeof dst, &t, &written) ==
          SKTO_E_INADMISSIBLE);

    /* Step 8: caller storage too small. */
    CHECK(suas_skto_admit("hello", 5, NULL, NULL, NULL,
                          dst, 4, &t, &written) == SKTO_E_CAPACITY);
    CHECK(written == 0);

    /* Interior NUL is legal text but never NUL-safe. */
    CHECK(suas_skto_admit("a\0b", 3, NULL, NULL, NULL,
                          dst, sizeof dst, &t, &written) == SKTO_OK);
    CHECK((t.flags & SKTO_F_ADMITTED) != 0);
    CHECK((t.flags & SKTO_F_NUL_SAFE) == 0);
    CHECK(suas_skto_validate(&t) == SKTO_OK);

    /* Failure leaves *out untouched when the argument is absent. */
    CHECK(suas_skto_admit("x", 1, NULL, NULL, NULL,
                          dst, sizeof dst, NULL, &written) == SKTO_E_NULL);
    CHECK(written == 0);
}

/* -- Comparison and hash ----------------------------------------------- */

static void test_compare_and_hash(void)
{
    skto_text_t a, b, c, empty;

    CHECK(suas_skto_init_borrowed(&a, "abc", 3) == SKTO_OK);
    CHECK(suas_skto_init_borrowed(&b, "abc", 3) == SKTO_OK);
    CHECK(suas_skto_init_borrowed(&c, "abd", 3) == SKTO_OK);
    CHECK(suas_skto_init_borrowed(&empty, "", 0) == SKTO_OK);

    CHECK(suas_skto_equal(&a, &b));
    CHECK(suas_skto_equal(&a, &a));
    CHECK(!suas_skto_equal(&a, &c));
    CHECK(!suas_skto_equal(&a, &empty));
    CHECK(!suas_skto_equal(&a, NULL));

    CHECK(suas_skto_compare(&a, &b) == 0);
    CHECK(suas_skto_compare(&a, &c) < 0);
    CHECK(suas_skto_compare(&c, &a) > 0);
    CHECK(suas_skto_compare(&empty, &a) < 0);
    CHECK(suas_skto_compare(&a, &empty) > 0);

    CHECK(suas_skto_hash(&a) == suas_skto_hash(&b));
    CHECK(suas_skto_hash(&a) != suas_skto_hash(&c));
    CHECK(suas_skto_hash(&empty) == 2166136261u);
    CHECK(suas_skto_hash(NULL) == 0u);

    /* A length mismatch is decided without reading the shorter buffer past
     * its end; a shared prefix does not imply equality. */
    {
        skto_text_t p, q;
        CHECK(suas_skto_init_borrowed(&p, "ab", 2) == SKTO_OK);
        CHECK(suas_skto_init_borrowed(&q, "abc", 3) == SKTO_OK);
        CHECK(!suas_skto_equal(&p, &q));
        CHECK(suas_skto_compare(&p, &q) < 0);
    }
}

/* -- Names ------------------------------------------------------------- */

static void test_publication(void)
{
    skto_name_table_t table;
    skto_text_t name, view;
    uint8_t buf[16];
    uint32_t gen = 99;

    suas_skto_names_init(&table);
    CHECK(table.live == 0);

    /* "vfs" as a NUL-safe borrowed view. */
    memcpy(buf, "vfs\0", 4);
    CHECK(suas_skto_init_borrowed(&name, buf, 3) == SKTO_OK);
    CHECK(suas_skto_mark_nul_safe(&name) == SKTO_OK);
    CHECK((name.flags & SKTO_F_NUL_SAFE) != 0);
    CHECK(suas_skto_name_check(&name, NULL) == SKTO_OK);

    CHECK(suas_skto_publish(&table, &name, NULL, &gen) == SKTO_OK);
    CHECK(gen == 0);
    CHECK(table.live == 1);

    /* Resolve binds the verdict to the same immutable object. */
    CHECK(suas_skto_resolve(&table, &name, &gen, &view) == SKTO_OK);
    CHECK(gen == 0);
    CHECK(view.owner == (uint16_t)SKTO_OWN_PUBLISHED);
    CHECK((view.flags & SKTO_F_NAME) != 0);
    CHECK((view.flags & SKTO_F_CONST) != 0);
    CHECK((view.flags & SKTO_F_NUL_SAFE) != 0);
    CHECK(view.generation == 0);
    CHECK(view.len == 3 && memcmp(view.bytes, "vfs", 3) == 0);
    CHECK(view.bytes[3] == 0);
    CHECK(suas_skto_validate(&view) == SKTO_OK);

    /* Republishing an equal name is a new generation, not an edit. */
    CHECK(suas_skto_publish(&table, &name, NULL, &gen) == SKTO_OK);
    CHECK(gen == 1);
    CHECK(table.live == 1);

    /* The snapshot resolved before the republish is untouched: publication
     * wrote an inactive record and flipped the active index. */
    CHECK(view.generation == 0);
    CHECK(view.len == 3 && memcmp(view.bytes, "vfs", 3) == 0);
    CHECK(view.bytes[3] == 0);
    CHECK(suas_skto_name_release(&table, &view) == SKTO_OK);

    CHECK(suas_skto_resolve(&table, &name, &gen, &view) == SKTO_OK);
    CHECK(gen == 1 && view.generation == 1);
    CHECK(suas_skto_name_release(&table, &view) == SKTO_OK);

    /* A different name starts at generation zero. */
    CHECK(suas_skto_init_borrowed(&name, "alpc", 4) == SKTO_OK);
    {
        uint8_t nbuf[8];
        memcpy(nbuf, "alpc\0", 5);
        CHECK(suas_skto_init_borrowed(&name, nbuf, 4) == SKTO_OK);
        CHECK(suas_skto_mark_nul_safe(&name) == SKTO_OK);
        CHECK(suas_skto_publish(&table, &name, NULL, &gen) == SKTO_OK);
        CHECK(gen == 0);
        CHECK(table.live == 2);
    }

    /* Unknown names are distinguishable from empty text. */
    {
        skto_text_t missing;
        uint8_t mbuf[8];
        memcpy(mbuf, "nope\0", 5);
        CHECK(suas_skto_init_borrowed(&missing, mbuf, 4) == SKTO_OK);
        CHECK(suas_skto_mark_nul_safe(&missing) == SKTO_OK);
        CHECK(suas_skto_resolve(&table, &missing, &gen, &view) ==
              SKTO_E_NOTFOUND);
        CHECK(gen == 0);
    }

    /* A name that is not NUL-safe is refused. */
    CHECK(suas_skto_init_borrowed(&name, "raw", 3) == SKTO_OK);
    CHECK(suas_skto_name_check(&name, NULL) == SKTO_E_POLICY);
    CHECK(suas_skto_publish(&table, &name, NULL, &gen) == SKTO_E_POLICY);

    /* An interior NUL cannot become NUL-safe. */
    {
        uint8_t ibuf[8] = { 'a', 0, 'b', 0, 0, 0, 0, 0 };
        CHECK(suas_skto_init_borrowed(&name, ibuf, 3) == SKTO_OK);
        CHECK(suas_skto_mark_nul_safe(&name) == SKTO_E_STATE);
    }

    /* A namespace ceiling below the storage ceiling is a policy refusal. */
    {
        skto_limits_t lim;
        uint8_t longbuf[16];
        memcpy(longbuf, "abcdefgh\0", 9);
        CHECK(suas_skto_init_borrowed(&name, longbuf, 8) == SKTO_OK);
        CHECK(suas_skto_mark_nul_safe(&name) == SKTO_OK);
        suas_skto_limits_default(&lim);
        lim.max_name_bytes = 4;
        CHECK(suas_skto_name_check(&name, &lim) == SKTO_E_POLICY);
        CHECK(suas_skto_publish(&table, &name, &lim, &gen) == SKTO_E_POLICY);
    }
}

static void test_table_full(void)
{
    skto_name_table_t table;
    uint32_t gen = 0;
    uint8_t one[4] = { 'a', 0, 0, 0 };
    unsigned i;

    suas_skto_names_init(&table);

    /* Fill the table with distinct one-byte names. U+0061 .. U+0070. */
    for (i = 0; i < (unsigned)SKTO_NAME_CAPACITY; i++) {
        uint8_t b[2];
        skto_text_t n;
        b[0] = (uint8_t)('a' + i);
        b[1] = 0;
        CHECK(suas_skto_init_borrowed(&n, b, 1) == SKTO_OK);
        CHECK(suas_skto_mark_nul_safe(&n) == SKTO_OK);
        CHECK(suas_skto_publish(&table, &n, NULL, &gen) == SKTO_OK);
        CHECK(gen == 0);
    }
    CHECK(table.live == SKTO_NAME_CAPACITY);

    {
        skto_text_t extra;
        CHECK(suas_skto_init_borrowed(&extra, one, 1) == SKTO_OK);
        CHECK(suas_skto_mark_nul_safe(&extra) == SKTO_OK);
        /* 'x' is already present, so this is a replacement, not a new slot. */
        CHECK(suas_skto_publish(&table, &extra, NULL, &gen) == SKTO_OK);
        CHECK(gen == 1);
    }

    {
        skto_text_t overflow;
        uint8_t z[2] = { 'z', 0 };
        CHECK(suas_skto_init_borrowed(&overflow, z, 1) == SKTO_OK);
        CHECK(suas_skto_mark_nul_safe(&overflow) == SKTO_OK);
        CHECK(suas_skto_publish(&table, &overflow, NULL, &gen) ==
              SKTO_E_FULL);
    }
}

static void make_test_name(skto_text_t *name, uint8_t *buf, const char *text)
{
    size_t n = strlen(text);
    memcpy(buf, text, n + 1u);
    CHECK(suas_skto_init_borrowed(name, buf, n) == SKTO_OK);
    CHECK(suas_skto_mark_nul_safe(name) == SKTO_OK);
}

/* A held reference pins its record across many later publications. This is
 * stronger than the old two-buffer promise: the record is not reused until the
 * reader releases it. */
static void test_reader_lifetime(void)
{
    skto_name_table_t table;
    skto_text_t name;
    skto_text_t held;
    skto_text_t current;
    uint8_t nbuf[8];
    uint32_t gen = 0u;
    unsigned i;
    size_t s;
    size_t j;

    suas_skto_names_init(&table);
    make_test_name(&name, nbuf, "vfs");

    CHECK(suas_skto_publish(&table, &name, NULL, &gen) == SKTO_OK);
    CHECK(gen == 0u);
    CHECK(suas_skto_resolve(&table, &name, &gen, &held) == SKTO_OK);
    CHECK(gen == 0u && held.generation == 0u);

    for (i = 1u; i <= 6u; i++) {
        CHECK(suas_skto_publish(&table, &name, NULL, &gen) == SKTO_OK);
        CHECK(gen == i);
        CHECK(held.generation == 0u);
        CHECK(held.len == 3u);
        CHECK(memcmp(held.bytes, "vfs", 3u) == 0);
        CHECK(held.bytes[3] == 0u);
    }

    CHECK(suas_skto_resolve(&table, &name, &gen, &current) == SKTO_OK);
    CHECK(gen == 6u && current.generation == 6u);
    CHECK(current.bytes != held.bytes);
    CHECK(suas_skto_name_release(&table, &current) == SKTO_OK);
    CHECK(suas_skto_name_release(&table, &held) == SKTO_OK);
    CHECK(suas_skto_name_release(&table, &held) == SKTO_E_NOTFOUND);

    for (s = 0u; s < (size_t)SKTO_NAME_CAPACITY; s++) {
        for (j = 0u; j < (size_t)SKTO_NAME_SNAPSHOTS; j++) {
            CHECK(table.slots[s].snapshots[j].readers == 0u);
        }
    }
}

/* Exhaust every reusable record, prove publication refuses without mutation,
 * then prove that releasing only the final reference to a record makes it
 * reclaimable. Several readers hold each generation. */
static void test_capacity_and_reclamation(void)
{
    skto_name_table_t table;
    skto_text_t name;
    skto_text_t held[4][3];
    uint8_t nbuf[8];
    uint32_t gen = 0u;
    uint32_t observed = 0u;
    size_t g;
    size_t r;

    suas_skto_names_init(&table);
    make_test_name(&name, nbuf, "vfs");

    CHECK(suas_skto_publish(&table, &name, NULL, &gen) == SKTO_OK);
    CHECK(gen == 0u);

    for (g = 0u; g < 4u; g++) {
        for (r = 0u; r < 3u; r++) {
            CHECK(suas_skto_resolve(&table, &name, NULL, &held[g][r]) ==
                  SKTO_OK);
            CHECK(held[g][r].generation == (uint32_t)g);
            CHECK(held[g][r].len == 3u);
            CHECK(memcmp(held[g][r].bytes, "vfs", 3u) == 0);
        }
        if (g < 3u) {
            CHECK(suas_skto_publish(&table, &name, NULL, &gen) == SKTO_OK);
            CHECK(gen == (uint32_t)g + 1u);
        }
    }

    gen = 77u;
    CHECK(suas_skto_publish(&table, &name, NULL, &gen) == SKTO_E_BUSY);
    CHECK(gen == 0u);                 /* failed publication clears the out */
    CHECK(suas_skto_resolve(&table, &name, &observed, NULL) == SKTO_OK);
    CHECK(observed == 3u);            /* the visible generation is unchanged */

    for (g = 0u; g < 4u; g++) {
        for (r = 0u; r < 3u; r++) {
            CHECK(held[g][r].generation == (uint32_t)g);
            CHECK(memcmp(held[g][r].bytes, "vfs", 3u) == 0);
        }
    }

    /* One released reference is not enough while the record still has
     * readers. */
    CHECK(suas_skto_name_release(&table, &held[0][0]) == SKTO_OK);
    CHECK(suas_skto_publish(&table, &name, NULL, &gen) == SKTO_E_BUSY);

    CHECK(suas_skto_name_release(&table, &held[0][1]) == SKTO_OK);
    CHECK(suas_skto_name_release(&table, &held[0][2]) == SKTO_OK);

    /* The generation-0 record is now reclaimable and is the next target. */
    CHECK(suas_skto_publish(&table, &name, NULL, &gen) == SKTO_OK);
    CHECK(gen == 4u);

    /* The still-held older generations were not disturbed by the reuse. */
    for (g = 1u; g < 4u; g++) {
        for (r = 0u; r < 3u; r++) {
            CHECK(held[g][r].generation == (uint32_t)g);
            CHECK(held[g][r].len == 3u);
            CHECK(memcmp(held[g][r].bytes, "vfs", 3u) == 0);
            CHECK(suas_skto_name_release(&table, &held[g][r]) == SKTO_OK);
        }
    }

    CHECK(suas_skto_resolve(&table, &name, &observed, NULL) == SKTO_OK);
    CHECK(observed == 4u);
}

static void test_release_validation(void)
{
    skto_name_table_t table;
    skto_text_t name;
    skto_text_t a;
    skto_text_t b;
    skto_text_t bad;
    uint8_t nbuf[8];
    uint32_t gen = 0u;

    suas_skto_names_init(&table);
    make_test_name(&name, nbuf, "vfs");
    CHECK(suas_skto_publish(&table, &name, NULL, &gen) == SKTO_OK);
    CHECK(suas_skto_resolve(&table, &name, NULL, &a) == SKTO_OK);
    CHECK(suas_skto_resolve(&table, &name, NULL, &b) == SKTO_OK);

    CHECK(suas_skto_name_release(NULL, &a) == SKTO_E_NULL);
    CHECK(suas_skto_name_release(&table, NULL) == SKTO_E_NULL);

    bad = a;
    bad.generation++;
    CHECK(suas_skto_name_release(&table, &bad) == SKTO_E_NOTFOUND);
    bad = a;
    bad.len++;
    CHECK(suas_skto_name_release(&table, &bad) == SKTO_E_NOTFOUND);
    bad = a;
    bad.bytes++;
    CHECK(suas_skto_name_release(&table, &bad) == SKTO_E_NOTFOUND);
    CHECK(suas_skto_name_release(&table, &bad) == SKTO_E_NOTFOUND);

    /* The failed releases did not consume either real reference. */
    CHECK(suas_skto_name_release(&table, &a) == SKTO_OK);
    CHECK(suas_skto_name_release(&table, &b) == SKTO_OK);
    CHECK(suas_skto_name_release(&table, &a) == SKTO_E_NOTFOUND);
}

typedef struct {
    int acquires;
    int releases;
} skto_lock_probe_t;

static void probe_lock_acquire(void *ctx)
{
    ((skto_lock_probe_t *)ctx)->acquires++;
}

static void probe_lock_release(void *ctx)
{
    ((skto_lock_probe_t *)ctx)->releases++;
}

/* Retirement withdraws a name from lookup without invalidating a view a reader
 * already holds. A slot with no readers is reclaimed immediately; one with a
 * reader stays allocated and is reclaimed on that reader's final release. */
static void test_retirement(void)
{
    skto_name_table_t table;
    skto_text_t name;
    skto_text_t other;
    skto_text_t held;
    skto_text_t after;
    skto_text_t invalid;
    uint8_t nbuf[8];
    uint8_t obuf[8];
    uint32_t gen = 0u;

    suas_skto_names_init(&table);
    make_test_name(&name, nbuf, "vfs");
    make_test_name(&other, obuf, "tmp");

    CHECK(suas_skto_retire(NULL, &name) == SKTO_E_NULL);
    CHECK(suas_skto_retire(&table, NULL) == SKTO_E_NULL);

    /* Nothing published: retirement reports the name as absent. */
    CHECK(suas_skto_retire(&table, &name) == SKTO_E_NOTFOUND);

    /* A structurally invalid query is a caller error, not a miss. */
    invalid = name;
    invalid.len = invalid.cap + 1u;
    CHECK(suas_skto_retire(&table, &invalid) == SKTO_E_STATE);

    /* No outstanding reader: the slot is reclaimed at once. */
    CHECK(suas_skto_publish(&table, &name, NULL, &gen) == SKTO_OK);
    CHECK(gen == 0u);
    CHECK(table.live == 1u);
    CHECK(suas_skto_retire(&table, &name) == SKTO_OK);
    CHECK(table.live == 0u);
    CHECK(table.slots[0].live == 0u);
    CHECK(table.slots[0].retired == 0u);
    CHECK(suas_skto_resolve(&table, &name, NULL, NULL) == SKTO_E_NOTFOUND);

    /* A second retirement of the same name is a no-op miss. */
    CHECK(suas_skto_retire(&table, &name) == SKTO_E_NOTFOUND);

    /* Publishing the name again starts a fresh generation-0 slot. */
    CHECK(suas_skto_publish(&table, &name, NULL, &gen) == SKTO_OK);
    CHECK(gen == 0u);
    CHECK(table.live == 1u);

    /* Hold a reader across retirement. Lookup stops matching immediately, but
     * the outstanding view stays byte-exact and its slot stays allocated. */
    CHECK(suas_skto_resolve(&table, &name, &gen, &held) == SKTO_OK);
    CHECK(gen == 0u && held.generation == 0u);
    CHECK(suas_skto_retire(&table, &name) == SKTO_OK);
    CHECK(table.live == 1u);
    CHECK(table.slots[0].live == 1u);
    CHECK(table.slots[0].retired == 1u);
    CHECK(suas_skto_resolve(&table, &name, NULL, NULL) == SKTO_E_NOTFOUND);
    CHECK(held.len == 3u);
    CHECK(memcmp(held.bytes, "vfs", 3u) == 0);
    CHECK(held.bytes[3] == 0u);

    /* A retired slot is not a republish match, so the same name takes a new
     * slot while the reader still pins the old one. */
    CHECK(suas_skto_publish(&table, &name, NULL, &gen) == SKTO_OK);
    CHECK(gen == 0u);
    CHECK(table.live == 2u);
    CHECK(table.slots[1].live == 1u);
    CHECK(table.slots[1].retired == 0u);
    CHECK(suas_skto_resolve(&table, &name, &gen, &after) == SKTO_OK);
    CHECK(after.generation == 0u);
    CHECK(after.bytes != held.bytes);
    CHECK(suas_skto_name_release(&table, &after) == SKTO_OK);

    /* Other names are unaffected by a retirement. */
    CHECK(suas_skto_publish(&table, &other, NULL, &gen) == SKTO_OK);
    CHECK(suas_skto_resolve(&table, &other, NULL, NULL) == SKTO_OK);
    CHECK(suas_skto_retire(&table, &other) == SKTO_OK);
    CHECK(suas_skto_resolve(&table, &other, NULL, NULL) == SKTO_E_NOTFOUND);

    /* The final release of the pinned view reclaims the withdrawn slot. */
    CHECK(suas_skto_name_release(&table, &held) == SKTO_OK);
    CHECK(table.live == 1u);
    CHECK(table.slots[0].live == 0u);
    CHECK(table.slots[0].retired == 0u);
    CHECK(table.slots[0].snapshots[0].readers == 0u);
    CHECK(table.slots[0].snapshots[0].bytes[0] == 0u);

    /* A stale duplicate release cannot reclaim or corrupt anything. */
    CHECK(suas_skto_name_release(&table, &held) == SKTO_E_NOTFOUND);
    CHECK(table.live == 1u);
}

static void test_retirement_lock_hooks(void)
{
    skto_name_table_t table;
    skto_text_t name;
    skto_lock_probe_t probe;
    uint8_t nbuf[8];
    uint32_t gen = 0u;

    memset(&probe, 0, sizeof probe);
    CHECK(suas_skto_names_init_locked(&table, probe_lock_acquire,
                                      probe_lock_release, &probe) == SKTO_OK);
    make_test_name(&name, nbuf, "vfs");
    CHECK(suas_skto_publish(&table, &name, NULL, &gen) == SKTO_OK);
    CHECK(probe.acquires == 1 && probe.releases == 1);

    CHECK(suas_skto_retire(&table, &name) == SKTO_OK);
    CHECK(probe.acquires == 2 && probe.releases == 2);
    CHECK(suas_skto_retire(&table, &name) == SKTO_E_NOTFOUND);
    CHECK(probe.acquires == 3 && probe.releases == 3);
}

static void test_lock_hooks(void)
{
    skto_name_table_t table;
    skto_text_t name;
    skto_text_t view;
    skto_lock_probe_t probe;
    uint8_t nbuf[8];
    uint32_t gen = 0u;

    memset(&probe, 0, sizeof probe);
    CHECK(suas_skto_names_init_locked(NULL, probe_lock_acquire,
                                      probe_lock_release, &probe) ==
          SKTO_E_NULL);

    /* A missing callback leaves an existing table untouched. */
    suas_skto_names_init(&table);
    table.live = 123u;
    CHECK(suas_skto_names_init_locked(&table, NULL, probe_lock_release,
                                      &probe) == SKTO_E_NULL);
    CHECK(table.live == 123u);
    CHECK(table.lock.acquire == NULL);
    CHECK(table.lock.release == NULL);

    CHECK(suas_skto_names_init_locked(&table, probe_lock_acquire,
                                      probe_lock_release, &probe) == SKTO_OK);
    CHECK(table.lock.acquire == probe_lock_acquire);
    CHECK(table.lock.release == probe_lock_release);
    CHECK(table.lock.ctx == &probe);

    make_test_name(&name, nbuf, "vfs");
    CHECK(suas_skto_publish(&table, &name, NULL, &gen) == SKTO_OK);
    CHECK(probe.acquires == 1 && probe.releases == 1);

    CHECK(suas_skto_resolve(&table, &name, &gen, &view) == SKTO_OK);
    CHECK(probe.acquires == 2 && probe.releases == 2);

    /* A non-acquiring resolve still takes the lock but leaves readers alone. */
    CHECK(suas_skto_resolve(&table, &name, NULL, NULL) == SKTO_OK);
    CHECK(probe.acquires == 3 && probe.releases == 3);

    CHECK(suas_skto_name_release(&table, &view) == SKTO_OK);
    CHECK(probe.acquires == 4 && probe.releases == 4);
}

#ifdef _WIN32
#define SKTO_CONCURRENT_CYCLES 16

typedef struct {
    skto_name_table_t table;
    skto_text_t       query;
    uint8_t           query_bytes[4];
    CRITICAL_SECTION  lock;
    volatile LONG     failure;
    HANDLE            start;
    HANDLE            held[2];
    HANDLE            release[2];
    HANDLE            done[2];
} skto_thread_ctx_t;

typedef struct {
    skto_thread_ctx_t *ctx;
    unsigned           id;
} skto_reader_arg_t;

static void thread_lock_acquire(void *ctx)
{
    EnterCriticalSection(&((skto_thread_ctx_t *)ctx)->lock);
}

static void thread_lock_release(void *ctx)
{
    LeaveCriticalSection(&((skto_thread_ctx_t *)ctx)->lock);
}

static int thread_view_ok(const skto_text_t *view)
{
    return view->len == 3u &&
           memcmp(view->bytes, "vfs", 3u) == 0 &&
           view->bytes[3] == 0u;
}

static DWORD WINAPI skto_reader_thread(LPVOID arg)
{
    skto_reader_arg_t *self = (skto_reader_arg_t *)arg;
    skto_thread_ctx_t *ctx = self->ctx;
    unsigned i;

    for (i = 0u; i < SKTO_CONCURRENT_CYCLES; i++) {
        skto_text_t view;
        uint32_t gen = 0u;
        skto_status_t st;

        (void)WaitForSingleObject(ctx->start, INFINITE);
        st = suas_skto_resolve(&ctx->table, &ctx->query, &gen, &view);
        if (st != SKTO_OK || !thread_view_ok(&view)) {
            InterlockedExchange(&ctx->failure, 1);
        }

        /* Tell the writer that this reader now holds a reference, then wait
         * for the writer to publish while the reference is still held. */
        (void)SetEvent(ctx->held[self->id]);
        (void)WaitForSingleObject(ctx->release[self->id], INFINITE);

        if (st == SKTO_OK) {
            if (!thread_view_ok(&view)) {
                InterlockedExchange(&ctx->failure, 1);
            }
            if (suas_skto_name_release(&ctx->table, &view) != SKTO_OK) {
                InterlockedExchange(&ctx->failure, 1);
            }
        }
        (void)SetEvent(ctx->done[self->id]);
    }
    return 0u;
}

/* Deterministic host concurrency: both readers acquire and hold their views
 * before the writer publishes; the writer publishes before either reader is
 * allowed to verify and release. Repeated failures under a naive two-buffer
 * scheme would corrupt a held view. */
static void test_concurrent_readers_and_writer(void)
{
    skto_thread_ctx_t ctx;
    skto_reader_arg_t args[2];
    HANDLE threads[2];
    skto_text_t name;
    uint8_t nbuf[8];
    uint32_t gen = 0u;
    unsigned i;

    memset(&ctx, 0, sizeof ctx);
    memcpy(ctx.query_bytes, "vfs\0", 4);
    CHECK(suas_skto_init_borrowed(&ctx.query, ctx.query_bytes, 3u) == SKTO_OK);
    InitializeCriticalSection(&ctx.lock);

    ctx.start = CreateEvent(NULL, TRUE, FALSE, NULL);
    CHECK(ctx.start != NULL);
    for (i = 0u; i < 2u; i++) {
        ctx.held[i] = CreateEvent(NULL, FALSE, FALSE, NULL);
        ctx.release[i] = CreateEvent(NULL, FALSE, FALSE, NULL);
        ctx.done[i] = CreateEvent(NULL, FALSE, FALSE, NULL);
        CHECK(ctx.held[i] != NULL);
        CHECK(ctx.release[i] != NULL);
        CHECK(ctx.done[i] != NULL);
    }

    CHECK(suas_skto_names_init_locked(&ctx.table, thread_lock_acquire,
                                      thread_lock_release, &ctx) == SKTO_OK);
    make_test_name(&name, nbuf, "vfs");
    CHECK(suas_skto_publish(&ctx.table, &name, NULL, &gen) == SKTO_OK);
    CHECK(gen == 0u);

    for (i = 0u; i < 2u; i++) {
        args[i].ctx = &ctx;
        args[i].id = i;
        threads[i] = CreateThread(NULL, 0, skto_reader_thread, &args[i], 0,
                                  NULL);
        CHECK(threads[i] != NULL);
    }

    (void)SetEvent(ctx.start);
    for (i = 0u; i < SKTO_CONCURRENT_CYCLES; i++) {
        CHECK(WaitForMultipleObjects(2, ctx.held, TRUE, 10000) ==
              WAIT_OBJECT_0);
        CHECK(suas_skto_publish(&ctx.table, &name, NULL, &gen) == SKTO_OK);
        CHECK(gen == (uint32_t)i + 1u);
        (void)SetEvent(ctx.release[0]);
        (void)SetEvent(ctx.release[1]);
        CHECK(WaitForMultipleObjects(2, ctx.done, TRUE, 10000) ==
              WAIT_OBJECT_0);
    }

    CHECK(WaitForMultipleObjects(2, threads, TRUE, 10000) == WAIT_OBJECT_0);
    CHECK(ctx.failure == 0);

    for (i = 0u; i < 2u; i++) {
        (void)CloseHandle(ctx.done[i]);
        (void)CloseHandle(ctx.release[i]);
        (void)CloseHandle(ctx.held[i]);
        (void)CloseHandle(threads[i]);
    }
    (void)CloseHandle(ctx.start);
    DeleteCriticalSection(&ctx.lock);
}
#endif


static void test_abi_layout(void)
{
    /* Pointer-width independent checks */
    CHECK(sizeof(uint8_t) == 1u);
    CHECK(sizeof(uint16_t) == 2u);
    CHECK(sizeof(uint32_t) == 4u);
    CHECK(sizeof(uint64_t) == 8u);

    /* skto_name_snapshot_t is fixed-width regardless of pointer size */
    CHECK(sizeof(skto_name_snapshot_t) == 144u);
    CHECK(offsetof(skto_name_snapshot_t, bytes) == 0u);
    CHECK(offsetof(skto_name_snapshot_t, generation) == 132u);
    CHECK(offsetof(skto_name_snapshot_t, readers) == 136u);
    CHECK(offsetof(skto_name_snapshot_t, len) == 140u);
    CHECK(offsetof(skto_name_snapshot_t, live) == 142u);

    /* skto_name_slot_t */
    CHECK(sizeof(skto_name_slot_t) == 580u);
    CHECK(offsetof(skto_name_slot_t, snapshots) == 0u);
    CHECK(offsetof(skto_name_slot_t, active) == 576u);
    CHECK(offsetof(skto_name_slot_t, live) == 577u);
    CHECK(offsetof(skto_name_slot_t, retired) == 578u);

    /* Pointer-dependent sizes */
    if (sizeof(void *) == 8u) {
        CHECK(sizeof(skto_text_t) == 40u);
        CHECK(offsetof(skto_text_t, bytes) == 0u);
        CHECK(offsetof(skto_text_t, len) == 8u);
        CHECK(offsetof(skto_text_t, cap) == 16u);
        CHECK(offsetof(skto_text_t, owner) == 24u);
        CHECK(offsetof(skto_text_t, encoding) == 26u);
        CHECK(offsetof(skto_text_t, flags) == 28u);
        CHECK(offsetof(skto_text_t, generation) == 32u);

        CHECK(sizeof(skto_limits_t) == 16u);
        CHECK(offsetof(skto_limits_t, max_admit_bytes) == 0u);
        CHECK(offsetof(skto_limits_t, max_name_bytes) == 8u);

        CHECK(sizeof(skto_lock_ops_t) == 24u);
        CHECK(offsetof(skto_lock_ops_t, acquire) == 0u);
        CHECK(offsetof(skto_lock_ops_t, release) == 8u);
        CHECK(offsetof(skto_lock_ops_t, ctx) == 16u);

        CHECK(sizeof(skto_name_table_t) == 9312u);
        CHECK(offsetof(skto_name_table_t, slots) == 0u);
        CHECK(offsetof(skto_name_table_t, live) == 9280u);
        CHECK(offsetof(skto_name_table_t, shutting_down) == 9282u);
        CHECK(offsetof(skto_name_table_t, lock) == 9288u);
    } else if (sizeof(void *) == 4u) {
        CHECK(sizeof(skto_text_t) == 24u);
        CHECK(offsetof(skto_text_t, bytes) == 0u);
        CHECK(offsetof(skto_text_t, len) == 4u);
        CHECK(offsetof(skto_text_t, cap) == 8u);
        CHECK(offsetof(skto_text_t, owner) == 12u);
        CHECK(offsetof(skto_text_t, encoding) == 14u);
        CHECK(offsetof(skto_text_t, flags) == 16u);
        CHECK(offsetof(skto_text_t, generation) == 20u);

        CHECK(sizeof(skto_limits_t) == 8u);
        CHECK(offsetof(skto_limits_t, max_admit_bytes) == 0u);
        CHECK(offsetof(skto_limits_t, max_name_bytes) == 4u);

        CHECK(sizeof(skto_lock_ops_t) == 12u);
        CHECK(offsetof(skto_lock_ops_t, acquire) == 0u);
        CHECK(offsetof(skto_lock_ops_t, release) == 4u);
        CHECK(offsetof(skto_lock_ops_t, ctx) == 8u);

        CHECK(sizeof(skto_name_table_t) == 9296u);
        CHECK(offsetof(skto_name_table_t, slots) == 0u);
        CHECK(offsetof(skto_name_table_t, live) == 9280u);
        CHECK(offsetof(skto_name_table_t, shutting_down) == 9282u);
        CHECK(offsetof(skto_name_table_t, lock) == 9284u);
    }
}

int main(void)
{
    test_version_and_defaults();
    test_structural_rejection();
    test_no_terminator_dependence();
    test_capacity_refusal();
    test_borrowed_lifetime();
    test_owned_object();
    test_legacy_cstr_and_assign();
    test_admission();
    test_compare_and_hash();
    test_publication();
    test_table_full();
    test_reader_lifetime();
    test_capacity_and_reclamation();
    test_release_validation();
    test_retirement();
    test_retirement_lock_hooks();
    test_lock_hooks();
    test_abi_layout();
#ifdef _WIN32
    test_concurrent_readers_and_writer();
#endif

    if (g_fail) {
        printf("SKTO tests FAILED\n");
        return 1;
    }
    printf("SKTO tests passed\n");
    return 0;
}
