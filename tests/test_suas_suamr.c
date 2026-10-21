/**
 * SUAS-015 SUAMR — SuperUnicode Arabic Mark Rendering
 *
 * Unit tests for the zero-allocation, static-table, O(1) resolution of an
 * Arabic combining mark's stacking layout — its stacking flags, its fixed-point
 * 2D offset, and its stacking level — together with the Arabic Mark Transient
 * Reordering Algorithm (AMTRA) of §5 (a UAX #53 analog).
 *
 * Covers: version/meta, the compile-time 12-byte record geometry, the
 * canonical combining classes of §4 (including the mistaken U+08D9 assignment
 * of §5.8), zone dispatch and the RNUR carve-out, above/below/MCM/shadda
 * predicates, the algorithmic default, the O(1) layout query, offsets,
 * enumeration and table self-consistency, the transient reordering (shadda,
 * the ccc=230 MCM run, the ccc=220 MCM run, the CGJ override), every
 * verification outcome, and registration in SUAS-010 (SSRE).
 */

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <assert.h>

#include "suas/suas_suamr.h"
#include "suas/suas_ssre.h"

static int g_fail = 0;
#define CHECK(cond) do { if (!(cond)) { \
    printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); g_fail = 1; \
} } while (0)

/* ── Version, limits, and record geometry ─────────────────────────── */

static void test_version_and_meta(void)
{
    CHECK(suas_suamr_version_string() != NULL);
    CHECK(suas_suamr_version_string()[0] == '1');
    CHECK(suas_suamr_version_string()[2] == '0');
    CHECK(SUAS_SUAMR_SPEC_ID[0] == 'S');
    CHECK(SUAS_SUAMR_SPEC_TITLE[0] == 'S');
    CHECK(SUAS_SUAMR_VERSION_MAJOR == 1);
    CHECK(SUAS_SUAMR_VERSION_MINOR == 0);
    CHECK(SUAS_SUAMR_REGISTRY_CAPACITY == 256U);
    CHECK(SUAS_SUAMR_INDEX_SIZE == 512U);
    CHECK(SUAS_SUAMR_INDEX_SIZE > SUAS_SUAMR_REGISTRY_CAPACITY);
    CHECK(SUAS_SUAMR_MAX_CLUSTER == 32U);

    /* The record is fixed-size, byte-aligned and padding-free (§3.1): the
     * sketch's exact field order, 12 bytes with no padding. */
    CHECK(sizeof(suamr_mark_layout_t) == 12U);
    CHECK(offsetof(suamr_mark_layout_t, codepoint) == 0U);
    CHECK(offsetof(suamr_mark_layout_t, x_offset) == 4U);
    CHECK(offsetof(suamr_mark_layout_t, y_offset) == 6U);
    CHECK(offsetof(suamr_mark_layout_t, positioning_flags) == 8U);
    CHECK(offsetof(suamr_mark_layout_t, stacking_level) == 10U);
    CHECK(offsetof(suamr_mark_layout_t, reserved) == 11U);

    /* Flag bits are disjoint and the reserved mask is the upper eight bits. */
    CHECK((SUAMR_FLAG_STACK_ABOVE & SUAMR_FLAG_STACK_BELOW) == 0u);
    CHECK((SUAMR_FLAG_STAGGER & SUAMR_FLAG_ASSIGNED_MASK) != 0u);
    CHECK((SUAMR_FLAG_CUSTOM_CLASS & SUAMR_FLAG_ASSIGNED_MASK) != 0u);
    CHECK((SUAMR_FLAG_MCM & SUAMR_FLAG_ASSIGNED_MASK) != 0u);
    CHECK(SUAMR_FLAG_ASSIGNED_MASK == 0x00FFu);
    CHECK(SUAMR_OFFSET_SCALE == 64);

    /* The ccc constants carry the UAX #53 Table 1 values (§4). */
    CHECK(SUAMR_CCC_STARTER == 0u);
    CHECK(SUAMR_CCC_FATHATAN == 27u);
    CHECK(SUAMR_CCC_DAMMATAN == 28u);
    CHECK(SUAMR_CCC_KASRATAN == 29u);
    CHECK(SUAMR_CCC_FATHA == 30u);
    CHECK(SUAMR_CCC_DAMMA == 31u);
    CHECK(SUAMR_CCC_KASRA == 32u);
    CHECK(SUAMR_CCC_SHADDA == 33u);
    CHECK(SUAMR_CCC_SUKUN == 34u);
    CHECK(SUAMR_CCC_SUPERSCRIPT_ALEF == 35u);
    CHECK(SUAMR_CCC_BELOW == 220u);
    CHECK(SUAMR_CCC_ABOVE == 230u);
    CHECK(SUAMR_CGJ == 0x034Fu);
    CHECK(SUAMR_SHADDA_CP == 0x0651u);
}

/* ── Zone dispatch and the RNUR carve-out (§2) ────────────────────── */

static void test_zones(void)
{
    CHECK(suas_suamr_zone_of(0x064Eu) == SUAS_SUAMR_ZONE_BRIDGE);
    CHECK(suas_suamr_zone_of(0x0010FFFFu) == SUAS_SUAMR_ZONE_BRIDGE);
    CHECK(suas_suamr_zone_of(0x00110000u) == SUAS_SUAMR_ZONE_SCP);
    CHECK(suas_suamr_zone_of(0x0011FFFFu) == SUAS_SUAMR_ZONE_SCP);
    CHECK(suas_suamr_zone_of(0x00120000u) == SUAS_SUAMR_ZONE_NATIVE);
    CHECK(suas_suamr_zone_of(0x0015FFFFu) == SUAS_SUAMR_ZONE_NATIVE);
    CHECK(suas_suamr_zone_of(0x00160000u) == SUAS_SUAMR_ZONE_RNUR);
    CHECK(suas_suamr_zone_of(0x0016FFFFu) == SUAS_SUAMR_ZONE_RNUR);
    CHECK(suas_suamr_zone_of(0x00170000u) == SUAS_SUAMR_ZONE_NATIVE);
    CHECK(suas_suamr_zone_of(0x7FFFFFF0u) == SUAS_SUAMR_ZONE_TRAP);
    CHECK(suas_suamr_zone_of(0x7FFFFFFEu) == SUAS_SUAMR_ZONE_TRAP);
    CHECK(suas_suamr_zone_of(0x7FFFFFFFu) == SUAS_SUAMR_ZONE_SENTINEL);
    CHECK(suas_suamr_zone_of(0x80000000u) == SUAS_SUAMR_ZONE_EXTSUCS);
    CHECK(suas_suamr_zone_of(0xFFFFFFFFu) == SUAS_SUAMR_ZONE_EXTSUCS);

    /* A mark layout query may name Bridge, RNUR and Native only. */
    CHECK(suas_suamr_is_text_space(0x064Eu));
    CHECK(suas_suamr_is_text_space(0x00120000u));
    CHECK(suas_suamr_is_text_space(0x00160100u));
    CHECK(!suas_suamr_is_text_space(0x00110000u));
    CHECK(!suas_suamr_is_text_space(0x7FFFFFF0u));
    CHECK(!suas_suamr_is_text_space(0x7FFFFFFFu));
    CHECK(!suas_suamr_is_text_space(0x80000000u));

    CHECK(suas_suamr_is_neography(0x00160000u));
    CHECK(suas_suamr_is_neography(0x0016FFFFu));
    CHECK(!suas_suamr_is_neography(0x0015FFFFu));
}

static void test_neography_set(void)
{
    uint32_t set = 0xFFFFFFFFu;

    CHECK(suas_suamr_neography_set(0x00160100u, &set));
    CHECK(set == 1u);                    /* (0x100) >> 8 */
    CHECK(suas_suamr_neography_set(0x00160200u, &set));
    CHECK(set == 2u);
    CHECK(suas_suamr_neography_set(0x0016FFFFu, &set));
    CHECK(set == 0xFFu);

    CHECK(!suas_suamr_neography_set(0x00120000u, &set));
    CHECK(!suas_suamr_neography_set(0x00160100u, NULL));
}

/* ── Hash (§3.3) ──────────────────────────────────────────────────── */

static void test_hash(void)
{
    /* FNV-1a 32-bit over the four little-endian codepoint bytes. */
    CHECK(suas_suamr_hash(0x00160100u) == suas_suamr_hash(0x00160100u));
    CHECK(suas_suamr_hash(0x00160100u) != suas_suamr_hash(0x00160101u));
    CHECK(suas_suamr_hash(0u) != 0u);
    CHECK(suas_suamr_hash(0xFFFFFFFFu) != 0u);

    /* The pair hash is order-sensitive: (base, mark) != (mark, base). */
    CHECK(suas_suamr_hash_pair(0x0627u, 0x0651u) ==
          suas_suamr_hash_pair(0x0627u, 0x0651u));
    CHECK(suas_suamr_hash_pair(0x0627u, 0x0651u) !=
          suas_suamr_hash_pair(0x0651u, 0x0627u));
}

/* ── Canonical combining classes (§4) ─────────────────────────────── */

static void test_ccc(void)
{
    CHECK(suas_suamr_ccc(0x064Bu) == SUAMR_CCC_FATHATAN);
    CHECK(suas_suamr_ccc(0x064Cu) == SUAMR_CCC_DAMMATAN);
    CHECK(suas_suamr_ccc(0x064Du) == SUAMR_CCC_KASRATAN);
    CHECK(suas_suamr_ccc(0x064Eu) == SUAMR_CCC_FATHA);
    CHECK(suas_suamr_ccc(0x064Fu) == SUAMR_CCC_DAMMA);
    CHECK(suas_suamr_ccc(0x0650u) == SUAMR_CCC_KASRA);
    CHECK(suas_suamr_ccc(0x0651u) == SUAMR_CCC_SHADDA);
    CHECK(suas_suamr_ccc(0x0652u) == SUAMR_CCC_SUKUN);
    CHECK(suas_suamr_ccc(0x0670u) == SUAMR_CCC_SUPERSCRIPT_ALEF);

    /* The generalized classes. */
    CHECK(suas_suamr_ccc(0x0654u) == SUAMR_CCC_ABOVE);  /* hamza above */
    CHECK(suas_suamr_ccc(0x0655u) == SUAMR_CCC_BELOW);  /* hamza below */
    CHECK(suas_suamr_ccc(0x0653u) == SUAMR_CCC_ABOVE);  /* maddah above */

    /* §5.8: U+08D9 keeps its mistaken ccc=230 even though it renders below. */
    CHECK(suas_suamr_ccc(0x08D9u) == SUAMR_CCC_ABOVE);
    CHECK(suas_suamr_is_below(0x08D9u));

    /* A starter, the joiner, and an inadmissible zone are all ccc=0. */
    CHECK(suas_suamr_ccc(0x0627u) == SUAMR_CCC_STARTER);
    CHECK(suas_suamr_ccc(SUAMR_CGJ) == SUAMR_CCC_STARTER);
    CHECK(suas_suamr_ccc(0x00110000u) == SUAMR_CCC_STARTER);

    /* An uncurated above/below mark derives a generalized class. */
    CHECK(suas_suamr_ccc(0x0610u) == SUAMR_CCC_ABOVE);
}

/* ── Stacking predicates and MCM membership (§3, §4) ──────────────── */

static void test_predicates(void)
{
    /* Tashkil above / below. */
    CHECK(suas_suamr_is_mark(0x064Eu));
    CHECK(suas_suamr_is_above(0x064Eu));
    CHECK(!suas_suamr_is_below(0x064Eu));
    CHECK(suas_suamr_is_below(0x0650u));
    CHECK(!suas_suamr_is_above(0x0650u));

    /* Shadda is above, base-most, and not an MCM. */
    CHECK(suas_suamr_is_shadda(0x0651u));
    CHECK(suas_suamr_is_above(0x0651u));
    CHECK(!suas_suamr_is_mcm(0x0651u));
    CHECK(!suas_suamr_is_shadda(0x064Eu));

    /* The MCM set of §4.3: a curated sample. */
    CHECK(suas_suamr_is_mcm(0x0654u));  /* hamza above   */
    CHECK(suas_suamr_is_mcm(0x0655u));  /* hamza below   */
    CHECK(suas_suamr_is_mcm(0x0658u));  /* noon ghunna   */
    CHECK(suas_suamr_is_mcm(0x06DCu));  /* small high seen */
    CHECK(suas_suamr_is_mcm(0x06E3u));  /* small low seen  */
    CHECK(suas_suamr_is_mcm(0x06E7u));  /* small high yeh  */
    CHECK(suas_suamr_is_mcm(0x06E8u));  /* small high noon */
    CHECK(suas_suamr_is_mcm(0x08F3u));  /* small high waw  */
    CHECK(suas_suamr_is_mcm(0x08D3u));  /* small low waw   */
    CHECK(suas_suamr_is_mcm(0x08CEu));  /* large round dot above */
    CHECK(suas_suamr_is_mcm(0x08CFu));  /* large round dot below */
    CHECK(suas_suamr_is_mcm(0x10EF4u)); /* small low noon with fatha */
    CHECK(suas_suamr_is_mcm(0x10EF9u)); /* mark crown */

    /* §5.4: a mark excluded from the MCM so its natural order is kept. */
    CHECK(!suas_suamr_is_mcm(0x0653u)); /* maddah above */
    CHECK(!suas_suamr_is_mcm(0x0657u)); /* inverted damma */

    /* A starter is not a mark at all. */
    CHECK(!suas_suamr_is_mark(0x0627u));
    CHECK(!suas_suamr_is_above(0x0627u));
    CHECK(!suas_suamr_is_below(0x0627u));

    /* The shadda sits closer to the base than the vowels (§5.2). */
    CHECK(suas_suamr_stacking_level(0x0651u) < suas_suamr_stacking_level(0x064Fu));
    CHECK(suas_suamr_stacking_level(0x064Eu) == 1u);
    CHECK(suas_suamr_stacking_level(0x0627u) == 0u);
}

/* ── The O(1) layout query and offsets (§3) ───────────────────────── */

static void test_mark_layout(void)
{
    suamr_mark_layout_t lay;

    /* A curated mark: the query is total over the text space. */
    CHECK(suas_suamr_get_mark_layout(0x0627u, 0x0651u, &lay));
    CHECK(lay.codepoint == 0x0651u);
    CHECK((lay.positioning_flags & SUAMR_FLAG_SHADDA) != 0u);
    CHECK((lay.positioning_flags & SUAMR_FLAG_STACK_ABOVE) != 0u);
    CHECK(lay.reserved == 0u);

    /* The query is base-independent in 1.0. */
    {
        suamr_mark_layout_t a;
        suamr_mark_layout_t b;
        CHECK(suas_suamr_get_mark_layout(0x0627u, 0x064Eu, &a));
        CHECK(suas_suamr_get_mark_layout(0x0644u, 0x064Eu, &b));
        CHECK(a.codepoint == b.codepoint);
        CHECK(a.positioning_flags == b.positioning_flags);
        CHECK(a.y_offset == b.y_offset);
    }

    /* An uncurated but admissible mark is answered by the default. */
    CHECK(suas_suamr_get_mark_layout(0x0627u, 0x0610u, &lay));
    CHECK(lay.codepoint == 0x0610u);
    CHECK((lay.positioning_flags & SUAMR_FLAG_STACK_ABOVE) != 0u);

    /* Inadmissible zones and a NULL output are the only false. */
    CHECK(!suas_suamr_get_mark_layout(0x0627u, 0x00110000u, &lay));
    CHECK(!suas_suamr_get_mark_layout(0x0627u, 0x7FFFFFFFu, &lay));
    CHECK(!suas_suamr_get_mark_layout(0x0627u, 0x80000000u, &lay));
    CHECK(!suas_suamr_get_mark_layout(0x0627u, 0x0651u, NULL));

    /* Offsets: an above mark raises y, a below mark lowers it. */
    CHECK(suas_suamr_y_offset(0x0627u, 0x064Eu) > 0);
    CHECK(suas_suamr_y_offset(0x0627u, 0x0650u) < 0);
    /* An inadmissible *mark* zone is the only zero. */
    CHECK(suas_suamr_y_offset(0x0627u, 0x00110000u) == 0);
    CHECK(suas_suamr_x_offset(0x0627u, 0x064Eu) == 0);
}

/* ── Enumeration and table self-consistency ───────────────────────── */

static void test_enumeration(void)
{
    uint32_t n = suas_suamr_entry_count();
    uint32_t i;
    uint32_t mcm = 0u;
    uint32_t custom = 0u;
    suamr_mark_layout_t e;
    suamr_mark_layout_t prev;

    CHECK(n > 0u);
    CHECK(n <= SUAS_SUAMR_REGISTRY_CAPACITY);

    for (i = 0u; i < n; i++) {
        CHECK(suas_suamr_entry_at(i, &e));
        CHECK(suas_suamr_is_registered(e.codepoint));
        CHECK(suas_suamr_is_text_space(e.codepoint));
        CHECK(e.reserved == 0u);
        CHECK((e.positioning_flags & (uint16_t)~SUAMR_FLAG_ASSIGNED_MASK) == 0u);
        /* A record stacks above or below, never both, and never neither. */
        CHECK((e.positioning_flags & SUAMR_FLAG_STACK_ABOVE) != 0u ||
              (e.positioning_flags & SUAMR_FLAG_STACK_BELOW) != 0u);
        CHECK((e.positioning_flags & SUAMR_FLAG_STACK_ABOVE) == 0u ||
              (e.positioning_flags & SUAMR_FLAG_STACK_BELOW) == 0u);

        /* Ascending, unique codepoints. */
        if (i > 0u) {
            CHECK(suas_suamr_entry_at(i - 1u, &prev));
            CHECK(prev.codepoint < e.codepoint);
        }

        if ((e.positioning_flags & SUAMR_FLAG_MCM) != 0u)
            mcm++;
        if ((e.positioning_flags & SUAMR_FLAG_CUSTOM_CLASS) != 0u)
            custom++;
    }
    CHECK(mcm > 0u);
    CHECK(custom > 0u);

    CHECK(!suas_suamr_entry_at(n, &e));
    CHECK(!suas_suamr_entry_at(0u, NULL));
}

static void test_init_idempotent(void)
{
    suamr_mark_layout_t a;
    suamr_mark_layout_t b;

    suas_suamr_init_engine();
    suas_suamr_init_engine();
    CHECK(suas_suamr_get_mark_layout(0x0627u, 0x0651u, &a));
    suas_suamr_init_engine();
    CHECK(suas_suamr_get_mark_layout(0x0627u, 0x0651u, &b));
    CHECK(a.codepoint == b.codepoint &&
          a.positioning_flags == b.positioning_flags);
}

/* ── Transient reordering (§5, AMTRA) ─────────────────────────────── */

/* <base, damma, shadda>: AMTRA moves the shadda before the damma. */
static void test_reorder_shadda_first(void)
{
    uint32_t cl[3];
    suamr_reorder_status_t st = (suamr_reorder_status_t)-1;

    cl[0] = 0x0627u; /* alef */
    cl[1] = 0x064Fu; /* damma  (ccc=31) */
    cl[2] = 0x0651u; /* shadda (ccc=33) */

    CHECK(suas_suamr_reorder_cluster(cl, 3u, &st));
    CHECK(st == SUAMR_REORDER_OK);
    CHECK(cl[0] == 0x0627u);
    CHECK(cl[1] == 0x0651u);
    CHECK(cl[2] == 0x064Fu);

    /* A canonical-equivalent input renders identically. */
    {
        uint32_t other[3];
        other[0] = 0x0627u;
        other[1] = 0x0651u;
        other[2] = 0x064Fu;
        CHECK(suas_suamr_reorder_cluster(other, 3u, NULL));
        CHECK(other[1] == cl[1] && other[2] == cl[2]);
    }
}

/* <base, fatha, hamza above>: the ccc=230 MCM run is promoted (§4.3). */
static void test_reorder_230_mcm(void)
{
    uint32_t cl[3];

    cl[0] = 0x0627u; /* alef */
    cl[1] = 0x064Eu; /* fatha (ccc=30)       */
    cl[2] = 0x0654u; /* hamza above (230, MCM) */

    CHECK(suas_suamr_reorder_cluster(cl, 3u, NULL));
    CHECK(cl[0] == 0x0627u);
    CHECK(cl[1] == 0x0654u); /* hamza above moves closer to the base */
    CHECK(cl[2] == 0x064Eu);
}

/* <base, kasra, hamza below>: the ccc=220 MCM run is promoted. */
static void test_reorder_220_mcm(void)
{
    uint32_t cl[3];

    cl[0] = 0x0627u; /* alef */
    cl[1] = 0x0650u; /* kasra (ccc=32)        */
    cl[2] = 0x0655u; /* hamza below (220, MCM) */

    CHECK(suas_suamr_reorder_cluster(cl, 3u, NULL));
    CHECK(cl[0] == 0x0627u);
    CHECK(cl[1] == 0x0655u);
    CHECK(cl[2] == 0x0650u);
}

/* The three tiers in one cluster: 220 MCM, then 230 MCM, then shadda. */
static void test_reorder_tiers(void)
{
    uint32_t cl[4];

    cl[0] = 0x0627u; /* alef */
    cl[1] = 0x0651u; /* shadda (33)           */
    cl[2] = 0x064Eu; /* fatha (30)            */
    cl[3] = 0x0655u; /* hamza below (220, MCM) */

    CHECK(suas_suamr_reorder_cluster(cl, 4u, NULL));
    CHECK(cl[0] == 0x0627u);
    CHECK(cl[1] == 0x0655u); /* 220 MCM */
    CHECK(cl[2] == 0x0651u); /* shadda  */
    CHECK(cl[3] == 0x064Eu); /* fatha   */
}

/* The CGJ is a barrier: it pins the mark that follows it (§4.2). */
static void test_reorder_cgj_override(void)
{
    uint32_t cl[4];

    cl[0] = 0x0627u;  /* alef */
    cl[1] = 0x064Eu;  /* fatha (30) */
    cl[2] = SUAMR_CGJ;/* joining barrier */
    cl[3] = 0x0654u;  /* hamza above (230, MCM), pinned after the CGJ */

    CHECK(suas_suamr_reorder_cluster(cl, 4u, NULL));
    /* The hamza above may not jump the CGJ, so the order is unchanged. */
    CHECK(cl[1] == 0x064Eu);
    CHECK(cl[2] == SUAMR_CGJ);
    CHECK(cl[3] == 0x0654u);
}

/* A single mark, and a bare base, are no-ops. */
static void test_reorder_trivial(void)
{
    uint32_t cl[2];

    cl[0] = 0x0627u;
    cl[1] = 0x064Eu;
    CHECK(suas_suamr_reorder_cluster(cl, 2u, NULL));
    CHECK(cl[1] == 0x064Eu);

    cl[0] = 0x0627u;
    CHECK(suas_suamr_reorder_cluster(cl, 1u, NULL));
    CHECK(cl[0] == 0x0627u);
}

/* The guards name every failure mode. */
static void test_reorder_guards(void)
{
    uint32_t cl[3];
    suamr_reorder_status_t st = (suamr_reorder_status_t)-1;

    CHECK(!suas_suamr_reorder_cluster(NULL, 3u, &st));
    CHECK(st == SUAMR_REORDER_NULL_ARGUMENT);

    cl[0] = 0x0627u;
    CHECK(!suas_suamr_reorder_cluster(cl, 0u, &st));
    CHECK(st == SUAMR_REORDER_NULL_ARGUMENT);

    /* More than MAX_CLUSTER is refused, not truncated. */
    {
        uint32_t big[SUAS_SUAMR_MAX_CLUSTER + 1u];
        size_t i;
        for (i = 0u; i < SUAS_SUAMR_MAX_CLUSTER + 1u; i++)
            big[i] = 0x064Eu;
        big[0] = 0x0627u;
        CHECK(!suas_suamr_reorder_cluster(big, SUAS_SUAMR_MAX_CLUSTER + 1u, &st));
        CHECK(st == SUAMR_REORDER_TOO_LONG);
    }

    /* An inadmissible zone anywhere in the cluster. */
    cl[0] = 0x0627u;
    cl[1] = 0x064Eu;
    cl[2] = 0x00110000u;
    CHECK(!suas_suamr_reorder_cluster(cl, 3u, &st));
    CHECK(st == SUAMR_REORDER_ILLEGAL_ZONE);

    /* The first codepoint must be a starter. */
    cl[0] = 0x064Eu;
    cl[1] = 0x0651u;
    CHECK(!suas_suamr_reorder_cluster(cl, 2u, &st));
    CHECK(st == SUAMR_REORDER_NO_BASE);

    /* A CGJ may lead the cluster: it is a starter. */
    cl[0] = SUAMR_CGJ;
    cl[1] = 0x064Eu;
    CHECK(suas_suamr_reorder_cluster(cl, 2u, &st));
    CHECK(st == SUAMR_REORDER_OK);
}

/* ── Verification (§9) ────────────────────────────────────────────── */

static void test_validate_table(void)
{
    suamr_mark_layout_t good[3];
    suamr_mark_layout_t bad[3];
    suamr_registry_error_t err = (suamr_registry_error_t)-1;

    /* An empty registry is conformant, with or without a pointer. */
    CHECK(suas_suamr_validate_table(NULL, 0u, &err));
    CHECK(err == SUAMR_REGISTRY_OK);
    CHECK(suas_suamr_validate_table(good, 0u, NULL));

    /* A well-formed pair. */
    good[0].codepoint = 0x0000064Eu;
    good[0].x_offset = 0;
    good[0].y_offset = 0x18;
    good[0].positioning_flags = SUAMR_FLAG_STACK_ABOVE;
    good[0].stacking_level = 1u;
    good[0].reserved = 0u;
    good[1].codepoint = 0x00000651u;
    good[1].x_offset = 0;
    good[1].y_offset = 0x08;
    good[1].positioning_flags =
        (uint16_t)(SUAMR_FLAG_STACK_ABOVE | SUAMR_FLAG_SHADDA);
    good[1].stacking_level = 0u;
    good[1].reserved = 0u;
    CHECK(suas_suamr_validate_table(good, 2u, &err));
    CHECK(err == SUAMR_REGISTRY_OK);

    /* NULL_TABLE: a count with no pointer. */
    CHECK(!suas_suamr_validate_table(NULL, 1u, &err));
    CHECK(err == SUAMR_REGISTRY_NULL_TABLE);

    /* COUNT_OVER. */
    CHECK(!suas_suamr_validate_table(good, SUAS_SUAMR_REGISTRY_CAPACITY + 1u,
                                     &err));
    CHECK(err == SUAMR_REGISTRY_COUNT_OVER);

    /* OUT_OF_SPACE: above the 31-bit Base space. */
    bad[0] = good[0];
    bad[0].codepoint = 0x80000000u;
    CHECK(!suas_suamr_validate_table(bad, 1u, &err));
    CHECK(err == SUAMR_REGISTRY_OUT_OF_SPACE);

    /* INADMISSIBLE_CODEPOINT: the Control Plane. */
    bad[0] = good[0];
    bad[0].codepoint = 0x00110000u;
    CHECK(!suas_suamr_validate_table(bad, 1u, &err));
    CHECK(err == SUAMR_REGISTRY_INADMISSIBLE_CODEPOINT);

    /* BAD_FLAGS: a reserved bit. */
    bad[0] = good[0];
    bad[0].positioning_flags = (uint16_t)0x8000u;
    CHECK(!suas_suamr_validate_table(bad, 1u, &err));
    CHECK(err == SUAMR_REGISTRY_BAD_FLAGS);

    /* STACK_CONTRADICTION: both above and below. */
    bad[0] = good[0];
    bad[0].positioning_flags =
        (uint16_t)(SUAMR_FLAG_STACK_ABOVE | SUAMR_FLAG_STACK_BELOW);
    CHECK(!suas_suamr_validate_table(bad, 1u, &err));
    CHECK(err == SUAMR_REGISTRY_STACK_CONTRADICTION);

    /* BAD_FLAGS: a mark that stacks nowhere. */
    bad[0] = good[0];
    bad[0].positioning_flags = 0u;
    CHECK(!suas_suamr_validate_table(bad, 1u, &err));
    CHECK(err == SUAMR_REGISTRY_BAD_FLAGS);

    /* BAD_RESERVED. */
    bad[0] = good[0];
    bad[0].reserved = 1u;
    CHECK(!suas_suamr_validate_table(bad, 1u, &err));
    CHECK(err == SUAMR_REGISTRY_BAD_RESERVED);

    /* CUSTOM_CLASS_MISMATCH: a custom-class mark outside the RNUR set. */
    bad[0] = good[0];
    bad[0].positioning_flags =
        (uint16_t)(good[0].positioning_flags | SUAMR_FLAG_CUSTOM_CLASS);
    CHECK(!suas_suamr_validate_table(bad, 1u, &err));
    CHECK(err == SUAMR_REGISTRY_CUSTOM_CLASS_MISMATCH);
    /* ...but it is fine inside the RNUR set. */
    bad[0] = good[0];
    bad[0].codepoint = 0x00160100u;
    bad[0].positioning_flags =
        (uint16_t)(SUAMR_FLAG_STACK_ABOVE | SUAMR_FLAG_CUSTOM_CLASS);
    CHECK(suas_suamr_validate_table(bad, 1u, &err));
    CHECK(err == SUAMR_REGISTRY_OK);

    /* UNORDERED and DUPLICATE_CODEPOINT. */
    bad[0] = good[1];
    bad[1] = good[0];
    CHECK(!suas_suamr_validate_table(bad, 2u, &err));
    CHECK(err == SUAMR_REGISTRY_UNORDERED);
    bad[0] = good[0];
    bad[1] = good[0];
    CHECK(!suas_suamr_validate_table(bad, 2u, &err));
    CHECK(err == SUAMR_REGISTRY_DUPLICATE_CODEPOINT);

    /* The shipped table passes its own verifier. */
    {
        uint32_t i;
        suamr_mark_layout_t all[SUAS_SUAMR_REGISTRY_CAPACITY];
        for (i = 0u; i < suas_suamr_entry_count(); i++)
            CHECK(suas_suamr_entry_at(i, &all[i]));
        CHECK(suas_suamr_validate_table(all, suas_suamr_entry_count(), &err));
        CHECK(err == SUAMR_REGISTRY_OK);
    }
}

/* ── Taxonomy names ───────────────────────────────────────────────── */

static void test_taxonomy(void)
{
    CHECK(suas_suamr_ccc_name(SUAMR_CCC_SHADDA)[0] == 's');
    CHECK(suas_suamr_ccc_name(SUAMR_CCC_BELOW)[0] == 'b');
    CHECK(suas_suamr_ccc_name(SUAMR_CCC_ABOVE)[0] == 'a');
    CHECK(suas_suamr_ccc_name(SUAMR_CCC_STARTER)[0] == 's');
    CHECK(suas_suamr_ccc_name((uint8_t)99)[0] == 'u');

    CHECK(suas_suamr_zone_name(SUAS_SUAMR_ZONE_BRIDGE)[0] == 'b');
    CHECK(suas_suamr_zone_name(SUAS_SUAMR_ZONE_RNUR)[0] == 'r');
    CHECK(suas_suamr_zone_name((suas_suamr_zone_t)99)[0] == 'u');

    CHECK(suas_suamr_registry_error_name(SUAMR_REGISTRY_OK)[0] == 'O');
    CHECK(suas_suamr_registry_error_name(SUAMR_REGISTRY_UNORDERED)[0] == 'U');
    CHECK(suas_suamr_registry_error_name((suamr_registry_error_t)99)[0] == 'U');

    CHECK(suas_suamr_reorder_status_name(SUAMR_REORDER_OK)[0] == 'o');
    CHECK(suas_suamr_reorder_status_name(SUAMR_REORDER_NO_BASE)[0] == 'n');
    CHECK(suas_suamr_reorder_status_name((suamr_reorder_status_t)99)[0] == 'u');
}

/* ── SSRE registration (§10) ──────────────────────────────────────── */

static void test_ssre_registration(void)
{
    uint8_t major = 0u;
    uint8_t minor = 0u;

    CHECK(suas_ssre_is_registered(SUAS_SUAMR_SPEC_ID));
    CHECK(suas_ssre_is_mandatory(SUAS_SUAMR_SPEC_ID));
    CHECK(suas_ssre_version_of(SUAS_SUAMR_SPEC_ID, &major, &minor));
    CHECK(major == 1u && minor == 0u);
    CHECK(suas_ssre_satisfies(SUAS_SUAMR_SPEC_ID, 1u, 0u));
    CHECK(suas_ssre_module_of(SUAS_SUAMR_SPEC_ID) != NULL);
    CHECK(suas_ssre_module_of(SUAS_SUAMR_SPEC_ID)[0] == 's');
}

int main(void)
{
    test_version_and_meta();
    test_zones();
    test_neography_set();
    test_hash();
    test_ccc();
    test_predicates();
    test_mark_layout();
    test_enumeration();
    test_init_idempotent();
    test_reorder_shadda_first();
    test_reorder_230_mcm();
    test_reorder_220_mcm();
    test_reorder_tiers();
    test_reorder_cgj_override();
    test_reorder_trivial();
    test_reorder_guards();
    test_validate_table();
    test_taxonomy();
    test_ssre_registration();

    if (g_fail) {
        printf("SUAS-015 SUAMR tests FAILED\n");
        return 1;
    }
    printf("SUAS-015 SUAMR tests passed\n");
    return 0;
}
