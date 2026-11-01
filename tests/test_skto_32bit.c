/* 32-bit ABI and behaviour evidence for SUAS-018 SKTO.
 *
 * The host suites are built by the default toolchain, which on this machine is
 * an x86_64 MinGW GCC: every sizeof/offsetof assertion they make describes
 * the LP64 layout.  A layout assertion that is only ever evaluated on one
 * pointer width says nothing about the other, so this file is a standalone
 * program intended to be compiled by a real 32-bit compiler (MSVC x86
 * `cl.exe`) and linked against src/suas/suas_skto.c.  It fails if it is
 * accidentally built for a 64-bit target, because then it would be re-testing
 * the layout the main suite already covers.
 *
 * Build (from the repository root, in a VS x86 developer prompt):
 *
 *   cl /nologo /W4 /WX /Iinclude /Fed:test_skto_32bit.exe ^
 *      tests\test_skto_32bit.c src\suas\suas_skto.c
 *
 * Expected: "SKTO 32-bit ABI: all checks passed", exit code 0.
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>

#include "suas/suas_skto.h"

static int g_fail = 0;

#define CHECK(cond)                                                      \
    do {                                                                 \
        if (!(cond)) {                                                   \
            printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);       \
            g_fail++;                                                    \
        }                                                                \
    } while (0)

static skto_status_t make_name(skto_text_t *t, uint8_t *buf, const char *s)
{
    size_t n = strlen(s);
    skto_status_t st;

    memcpy(buf, s, n + 1u);
    st = suas_skto_init_borrowed(t, buf, n);
    if (st != SKTO_OK) return st;
    return suas_skto_mark_nul_safe(t);
}

int main(void)
{
    /* This program exists for the 32-bit layout only. */
    if (sizeof(void *) != 4u) {
        printf("FAIL: not a 32-bit build (sizeof(void*)=%u)\n",
               (unsigned)sizeof(void *));
        return 2;
    }

    /* -- Widths ------------------------------------------------------- */
    CHECK(sizeof(uint8_t) == 1u);
    CHECK(sizeof(uint16_t) == 2u);
    CHECK(sizeof(uint32_t) == 4u);
    CHECK(sizeof(uint64_t) == 8u);
    CHECK(sizeof(size_t) == 4u);

    /* -- skto_text_t: ILP32 layout ------------------------------------ */
    CHECK(sizeof(skto_text_t) == 24u);
    CHECK(offsetof(skto_text_t, bytes) == 0u);
    CHECK(offsetof(skto_text_t, len) == 4u);
    CHECK(offsetof(skto_text_t, cap) == 8u);
    CHECK(offsetof(skto_text_t, owner) == 12u);
    CHECK(offsetof(skto_text_t, encoding) == 14u);
    CHECK(offsetof(skto_text_t, flags) == 16u);
    CHECK(offsetof(skto_text_t, generation) == 20u);

    /* -- Pointer-width independent records ---------------------------- */
    CHECK(sizeof(skto_name_snapshot_t) == 144u);
    CHECK(offsetof(skto_name_snapshot_t, bytes) == 0u);
    CHECK(offsetof(skto_name_snapshot_t, generation) == 132u);
    CHECK(offsetof(skto_name_snapshot_t, readers) == 136u);
    CHECK(offsetof(skto_name_snapshot_t, len) == 140u);
    CHECK(offsetof(skto_name_snapshot_t, live) == 142u);

    CHECK(sizeof(skto_name_slot_t) == 580u);
    CHECK(offsetof(skto_name_slot_t, snapshots) == 0u);
    CHECK(offsetof(skto_name_slot_t, active) == 576u);
    CHECK(offsetof(skto_name_slot_t, live) == 577u);
    CHECK(offsetof(skto_name_slot_t, retired) == 578u);

    /* All three pointers in lock_ops_t are 4 bytes on this target. */
    CHECK(sizeof(skto_lock_ops_t) == 12u);

    CHECK(sizeof(skto_name_table_t) == 9296u);
    CHECK(offsetof(skto_name_table_t, slots) == 0u);
    CHECK(offsetof(skto_name_table_t, live) == 9280u);
    CHECK(offsetof(skto_name_table_t, lock) == 9284u);

    /* -- Behaviour on the 32-bit representation ----------------------- */
    {
        skto_name_table_t table;
        skto_text_t name;
        skto_text_t view;
        skto_text_t second;
        uint8_t nbuf[16];
        uint8_t other[16];
        uint32_t gen = 0u;

        suas_skto_names_init(&table);
        CHECK(make_name(&name, nbuf, "vfs") == SKTO_OK);
        CHECK(make_name(&second, other, "tmp") == SKTO_OK);

        CHECK(suas_skto_publish(&table, &name, NULL, &gen) == SKTO_OK);
        CHECK(gen == 0u);
        CHECK(table.live == 1u);

        CHECK(suas_skto_resolve(&table, &name, &gen, &view) == SKTO_OK);
        CHECK(gen == 0u);
        CHECK(view.len == 3u);
        CHECK(view.generation == 0u);
        CHECK(memcmp(view.bytes, "vfs", 3u) == 0);

        /* An equal name is replaced with generation + 1; the held view keeps
         * the old generation's bytes. */
        CHECK(suas_skto_publish(&table, &name, NULL, &gen) == SKTO_OK);
        CHECK(gen == 1u);
        CHECK(view.generation == 0u);
        CHECK(suas_skto_resolve(&table, &name, &gen, &second) == SKTO_OK);
        CHECK(gen == 1u);
        CHECK(second.bytes != view.bytes);
        CHECK(suas_skto_name_release(&table, &second) == SKTO_OK);

        /* Retirement removes the name from lookup but leaves the held view
         * valid until its own release. */
        CHECK(suas_skto_retire(&table, &name) == SKTO_OK);
        CHECK(suas_skto_resolve(&table, &name, NULL, NULL) == SKTO_E_NOTFOUND);
        CHECK(memcmp(view.bytes, "vfs", 3u) == 0);
        CHECK(suas_skto_name_release(&table, &view) == SKTO_OK);
        CHECK(suas_skto_name_release(&table, &view) == SKTO_E_NOTFOUND);
        CHECK(table.live == 0u);
    }

    /* -- Bounded capacity, counted on the 32-bit table ---------------- */
    {
        skto_name_table_t table;
        skto_text_t name;
        uint8_t nbuf[8];
        uint32_t i;
        skto_status_t st = SKTO_OK;

        suas_skto_names_init(&table);
        for (i = 0u; i < SKTO_NAME_CAPACITY; ++i) {
            char s[4];
            s[0] = (char)('a' + (i % 26u));
            s[1] = (char)('a' + ((i / 26u) % 26u));
            s[2] = '\0';
            if (make_name(&name, nbuf, s) != SKTO_OK) {
                CHECK(0 && "make_name");
                break;
            }
            st = suas_skto_publish(&table, &name, NULL, NULL);
            if (st != SKTO_OK) break;
        }
        CHECK(st == SKTO_OK);
        CHECK(table.live == (uint16_t)SKTO_NAME_CAPACITY);

        /* One more distinct name is refused, not truncated. */
        {
            uint8_t over[8];
            skto_text_t extra;
            CHECK(make_name(&extra, over, "zz") == SKTO_OK);
            CHECK(suas_skto_publish(&table, &extra, NULL, NULL) == SKTO_E_FULL);
        }
        CHECK(table.live == (uint16_t)SKTO_NAME_CAPACITY);
    }

    if (g_fail != 0) {
        printf("SKTO 32-bit ABI: %d check(s) FAILED\n", g_fail);
        return 1;
    }
    printf("SKTO 32-bit ABI: all checks passed\n");
    return 0;
}
