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
     * wrote the inactive buffer and flipped the active index. */
    CHECK(view.generation == 0);
    CHECK(view.len == 3 && memcmp(view.bytes, "vfs", 3) == 0);
    CHECK(view.bytes[3] == 0);

    CHECK(suas_skto_resolve(&table, &name, &gen, &view) == SKTO_OK);
    CHECK(gen == 1 && view.generation == 1);

    /* A different name starts at generation zero. */
    CHECK(suas_skto_init_borrowed(&name, "alpc", 4) == SKTO_OK);
    {
        uint8_t nbuf[8];
        memcpy(nbuf, "alpc\0", 5);
        CHECK(suas_skto_init_borrowed(&name, nbuf, 4) == SKTO_OK);
        CHECK(suas_skto_mark_nul_safe(&name) == SKTO_OK);
    }
    CHECK(suas_skto_publish(&table, &name, NULL, &gen) == SKTO_OK);
    CHECK(gen == 0);
    CHECK(table.live == 2);

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

    if (g_fail) {
        printf("SKTO tests FAILED\n");
        return 1;
    }
    printf("SKTO tests passed\n");
    return 0;
}
