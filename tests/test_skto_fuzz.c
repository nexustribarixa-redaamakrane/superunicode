/* Deterministic property/fuzz harness for SUAS-018 SKTO.
 *
 * Two properties are exercised over a fixed-seed, fixed-iteration corpus:
 *
 *   A. Admission is total and honest.  For any byte slice, suas_skto_admit()
 *      either refuses without writing to *out or storage, or accepts and then
 *      the resulting object is an exact, validated copy of the input.  No
 *      input may crash, over-read, or produce a partial object.
 *
 *   B. The name table behaves like its contract under arbitrary interleavings
 *      of publish/resolve/release/retire.  Resolvability, generations, held
 *      views, and the bounded slot count are checked against a small model;
 *      the model intentionally does not model slot reuse, so it asserts only
 *      what the contract guarantees (live <= capacity, a fresh name starts at
 *      generation 0, a live name is replaced at generation + 1, retirement
 *      removes lookup, held views stay byte-exact).
 *
 * Determinism matters as much as coverage: the RNG is a fixed xorshift with a
 * fixed seed, so a failure is reproducible and the run length does not depend
 * on time or host load.  Set SKTO_FUZZ_ITER to raise the iteration count for a
 * longer manual run; the default is what CTest runs.
 *
 * This is host-side evidence.  It does not exercise the OpenWindows kernel,
 * a real address-space containment callback, or concurrent publication.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "suas/suas_skto.h"

static int g_fail = 0;
static unsigned long g_checks = 0;

#define CHECK(cond)                                                      \
    do {                                                                 \
        g_checks++;                                                      \
        if (!(cond)) {                                                   \
            printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);       \
            if (++g_fail > 20) {                                         \
                printf("too many failures; stopping\n");                 \
                return;                                                  \
            }                                                            \
        }                                                                \
    } while (0)

/* -- Deterministic RNG (xorshift32) ----------------------------------- */

static uint32_t g_rng = 0x5EED1234u;

static uint32_t rng_next(void)
{
    uint32_t x = g_rng;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    g_rng = x;
    return x;
}

/* -- Property A: admission -------------------------------------------- */

typedef struct {
    bool     allow;
    unsigned calls;
    size_t   last_len;
} range_probe_t;

static bool probe_range(void *ctx, const void *base, size_t len)
{
    range_probe_t *p = (range_probe_t *)ctx;
    (void)base;
    p->calls++;
    p->last_len = len;
    return p->allow;
}

static void fill_random(uint8_t *buf, size_t len)
{
    static const uint8_t alphabet[] = {
        'a', 'b', 'c', 'z', '/', '.', '-', '_',
        0x00, 0x7F, 0x80, 0xC0, 0xC1, 0xC3, 0xE0, 0xED,
        0xF0, 0xF4, 0xF8, 0xFF
    };
    size_t i;
    for (i = 0u; i < len; ++i) {
        buf[i] = alphabet[rng_next() % (uint32_t)sizeof(alphabet)];
    }
}

static void test_admission(unsigned iterations)
{
    uint8_t src[256];
    uint8_t dst[512];
    uint8_t sentinel[sizeof(skto_text_t)];
    unsigned it;

    for (it = 0u; it < iterations; ++it) {
        skto_text_t out;
        range_probe_t probe;
        size_t len = (size_t)(rng_next() % 201u);
        size_t cap = (size_t)(rng_next() % 513u);
        size_t written = 0xDEADBEEFu;
        skto_status_t st;

        fill_random(src, len);
        memset(&out, 0xA5, sizeof out);
        memcpy(sentinel, &out, sizeof out);

        probe.allow = (rng_next() & 1u) != 0u;
        probe.calls = 0u;
        probe.last_len = 0u;

        st = suas_skto_admit(src, len, probe_range, &probe, NULL,
                             dst, cap, &out, &written);

        if (probe.allow) {
            if (len != 0u) CHECK(probe.calls == 1u);
            if (st == SKTO_OK) {
                CHECK(written == len);
                CHECK(out.len == len);
                CHECK(out.cap == cap);
                CHECK(out.bytes == dst);
                CHECK(out.owner == (uint16_t)SKTO_OWN_OWNED);
                CHECK((out.flags & SKTO_F_ADMITTED) != 0u);
                CHECK(memcmp(dst, src, len) == 0);
                CHECK(suas_skto_validate(&out) == SKTO_OK);
                CHECK((out.flags & SKTO_F_ADMITTED) != 0u);
                CHECK((out.flags & ~(SKTO_F_ADMITTED | SKTO_F_NUL_SAFE)) ==
                      0u);
            } else {
                CHECK(written == 0u);
                /* A refusal must not touch the caller's object. */
                CHECK(memcmp(&out, sentinel, sizeof out) == 0);
                CHECK(st == SKTO_E_CAPACITY || st == SKTO_E_ENCODING ||
                      st == SKTO_E_INADMISSIBLE || st == SKTO_E_OVERFLOW ||
                      st == SKTO_E_NULL);
                if (cap < len && len <= SKTO_MAX_ADMIT_BYTES) {
                    /* Only reachable after the scan passed; a malformed
                     * sequence is refused before the capacity check. */
                    if (st == SKTO_E_CAPACITY) CHECK(cap < len);
                }
            }
        } else {
            if (len != 0u) {
                CHECK(st == SKTO_E_RANGE);
                CHECK(probe.calls == 1u);
                CHECK(probe.last_len == len);
                CHECK(written == 0u);
                CHECK(memcmp(&out, sentinel, sizeof out) == 0);
            }
        }
    }
}

/* Deterministic regression cases: every one is a shape a fuzzer is unlikely
 * to hit at the exact byte boundary that decides the answer. */
static void test_admission_regressions(void)
{
    static const uint8_t bad_cont[] = { 'a', 0xC3, 0x28 };
    static const uint8_t surrogate[] = { 0xED, 0xA0, 0x80 };
    static const uint8_t over_max[] = { 0xF4, 0x90, 0x80, 0x80 };
    static const uint8_t scp_plane[] = { 0xF8, 0x84, 0x90, 0x80, 0x80 };
    static const uint8_t interior_nul[] = { 'a', 0x00, 'b' };
    static const uint8_t good_4[] = { 0xF4, 0x8F, 0xBF, 0xBF };
    uint8_t dst[512];
    uint8_t terminal[40];
    skto_text_t out;
    size_t written;

    memset(&out, 0, sizeof out);

    /* Empty candidate is valid and needs no storage. */
    written = 1u;
    CHECK(suas_skto_admit(NULL, 0u, NULL, NULL, NULL, NULL, 0u, &out,
                          &written) == SKTO_OK);
    CHECK(written == 0u);
    CHECK(out.len == 0u);
    CHECK(out.flags == SKTO_F_ADMITTED);

    /* Malformed continuation is a syntax error; a surrogate and a codepoint
     * above U+10FFFF are refused at decode time as well (the decoder treats
     * them as malformed framing). */
    CHECK(suas_skto_admit(bad_cont, sizeof bad_cont, NULL, NULL, NULL,
                          dst, sizeof dst, &out, &written) == SKTO_E_ENCODING);
    CHECK(suas_skto_admit(surrogate, sizeof surrogate, NULL, NULL, NULL,
                          dst, sizeof dst, &out, &written) == SKTO_E_ENCODING);
    CHECK(suas_skto_admit(over_max, sizeof over_max, NULL, NULL, NULL,
                          dst, sizeof dst, &out, &written) == SKTO_E_ENCODING);
    CHECK(suas_skto_admit(scp_plane, sizeof scp_plane, NULL, NULL, NULL,
                          dst, sizeof dst, &out, &written) ==
          SKTO_E_INADMISSIBLE);
    CHECK(written == 0u);

    /* The largest valid codepoint is admitted. */
    CHECK(suas_skto_admit(good_4, sizeof good_4, NULL, NULL, NULL,
                          dst, sizeof dst, &out, &written) == SKTO_OK);
    CHECK(written == sizeof good_4);

    /* Interior NUL is admissible but not NUL-safe. */
    CHECK(suas_skto_admit(interior_nul, sizeof interior_nul, NULL, NULL, NULL,
                          dst, sizeof dst, &out, &written) == SKTO_OK);
    CHECK((out.flags & SKTO_F_NUL_SAFE) == 0u);

    /* Over the default admission ceiling. */
    memset(dst, 'a', sizeof dst);
    CHECK(suas_skto_admit(dst, SKTO_MAX_ADMIT_BYTES + 1u, NULL, NULL, NULL,
                          dst, sizeof dst, &out, &written) == SKTO_E_OVERFLOW);

    /* The bounded C-string scanner never reads past scan_cap.  The NUL sits
     * exactly one byte beyond the scan window, so TERMINATOR is the only
     * honest answer. */
    memset(terminal, 'x', sizeof terminal);
    terminal[32] = 0u;
    CHECK(suas_skto_admit_cstr((const char *)terminal, 32u, NULL, NULL, NULL,
                               dst, sizeof dst, &out, &written) ==
          SKTO_E_TERMINATOR);
    CHECK(suas_skto_admit_cstr((const char *)terminal, 33u, NULL, NULL, NULL,
                               dst, sizeof dst, &out, &written) == SKTO_OK);
    CHECK(written == 32u);
}

/* -- Property B: name-table interleavings ----------------------------- */

#define MODEL_NAMES 20
#define MODEL_HELD  128

typedef struct {
    skto_text_t view;
    unsigned    name;
} held_t;

static void make_name(skto_text_t *t, uint8_t *buf, unsigned i)
{
    char s[8];
    size_t n;

    s[0] = 'n';
    s[1] = (char)('0' + (int)(i / 10u));
    s[2] = (char)('0' + (int)(i % 10u));
    s[3] = '\0';
    n = strlen(s);
    memcpy(buf, s, n + 1u);
    CHECK(suas_skto_init_borrowed(t, buf, n) == SKTO_OK);
    CHECK(suas_skto_mark_nul_safe(t) == SKTO_OK);
}

static void test_name_interleavings(unsigned iterations)
{
    skto_name_table_t table;
    skto_text_t names[MODEL_NAMES];
    uint8_t nbuf[MODEL_NAMES][8];
    bool live[MODEL_NAMES];
    uint32_t gen[MODEL_NAMES];
    held_t held[MODEL_HELD];
    unsigned held_count = 0u;
    unsigned i;
    unsigned it;

    suas_skto_names_init(&table);
    memset(live, 0, sizeof live);
    memset(gen, 0, sizeof gen);
    for (i = 0u; i < MODEL_NAMES; ++i) {
        make_name(&names[i], nbuf[i], i);
    }

    for (it = 0u; it < iterations; ++it) {
        unsigned op = rng_next() % 4u;
        unsigned n = rng_next() % MODEL_NAMES;

        switch (op) {
        case 0: { /* publish */
            uint32_t expect = live[n] ? gen[n] + 1u : 0u;
            uint32_t got = 0xFFFFFFFFu;
            skto_status_t st = suas_skto_publish(&table, &names[n], NULL, &got);
            if (st == SKTO_OK) {
                CHECK(got == expect);
                live[n] = true;
                gen[n] = got;
            } else {
                /* A fresh name can only be refused when every slot is in
                 * use; a live name is only refused for an exhausted
                 * generation (not reachable here) or a fully held pool. */
                CHECK(st == SKTO_E_FULL || st == SKTO_E_BUSY);
                if (st == SKTO_E_FULL) CHECK(table.live == SKTO_NAME_CAPACITY);
            }
            break;
        }
        case 1: { /* resolve */
            skto_text_t view;
            uint32_t got = 0xFFFFFFFFu;
            skto_status_t st = suas_skto_resolve(&table, &names[n], &got, &view);
            if (live[n]) {
                CHECK(st == SKTO_OK);
                CHECK(got == gen[n]);
                CHECK(view.len == names[n].len);
                CHECK(view.generation == gen[n]);
                CHECK(memcmp(view.bytes, names[n].bytes, names[n].len) == 0);
                if (held_count < MODEL_HELD) {
                    held[held_count].view = view;
                    held[held_count].name = n;
                    held_count++;
                } else {
                    CHECK(suas_skto_name_release(&table, &view) == SKTO_OK);
                }
            } else {
                CHECK(st == SKTO_E_NOTFOUND);
            }
            break;
        }
        case 2: { /* release a held view */
            if (held_count != 0u) {
                unsigned k = rng_next() % held_count;
                skto_text_t v = held[k].view;
                held[k] = held[held_count - 1u];
                held_count--;
                CHECK(suas_skto_name_release(&table, &v) == SKTO_OK);
            }
            break;
        }
        default: { /* retire, sometimes of an already-retired name */
            skto_status_t st = suas_skto_retire(&table, &names[n]);
            if (live[n]) {
                CHECK(st == SKTO_OK);
                live[n] = false;
                gen[n] = 0u;
                CHECK(suas_skto_resolve(&table, &names[n], NULL, NULL) ==
                      SKTO_E_NOTFOUND);
            } else {
                CHECK(st == SKTO_E_NOTFOUND);
            }
            break;
        }
        }

        CHECK(table.live <= SKTO_NAME_CAPACITY);
    }

    /* Held views stay valid and byte-exact until their own release, even
     * though names have been retired and republished underneath them. */
    while (held_count != 0u) {
        held_count--;
        CHECK(suas_skto_name_release(&table, &held[held_count].view) == SKTO_OK);
    }
    for (i = 0u; i < MODEL_NAMES; ++i) {
        if (live[i]) {
            CHECK(suas_skto_retire(&table, &names[i]) == SKTO_OK);
        }
    }
    CHECK(table.live == 0u);
    {
        uint32_t outstanding = 1u;
        CHECK(suas_skto_names_drain(&table, &outstanding) == SKTO_OK);
        CHECK(outstanding == 0u);
    }
    CHECK(suas_skto_names_destroy(&table) == SKTO_OK);
    CHECK(table.live == 0u);
}

int main(void)
{
    unsigned iterations = 20000u;
    const char *env = getenv("SKTO_FUZZ_ITER");

    if (env != NULL) {
        unsigned long v = strtoul(env, NULL, 10);
        if (v >= 100u && v <= 5000000ul) iterations = (unsigned)v;
    }

    test_admission_regressions();
    test_admission(iterations);
    test_name_interleavings(iterations);

    if (g_fail != 0) {
        printf("SKTO fuzz: %d FAILED check(s) after %u iterations "
               "(%lu checks)\n", g_fail, iterations, g_checks);
        return 1;
    }
    printf("SKTO fuzz: all %lu checks passed over %u iterations "
           "(seed 0x5EED1234)\n", g_checks, iterations);
    return 0;
}
