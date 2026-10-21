/* SUAS-015 — SuperUnicode Arabic Mark Rendering (SUAMR)
 *
 * Reference implementation of docs/suas/SUAS-015-suamr.md.
 *
 * Freestanding C99, zero allocation. A static table of curated Arabic
 * combining-mark records is indexed once into a fixed-size open-addressing
 * array, so that a (base, mark) pair's stacked layout — the stacking flags,
 * the fixed-point 2D offset, and the stacking level — is resolved in O(1)
 * expected time. The Arabic Mark Transient Reordering Algorithm (AMTRA, §5)
 * runs in place over a caller-owned cluster, allocating nothing.
 *
 * The runtime layer is fed at build time from the offline reference layer (the
 * SuperUnicode Character Database, SUAS-012: the Arabic mark rendering rules,
 * the Modifier_Combining_Mark list and the positioning manifests); the table
 * below is the UAX #53 Arabic combining-mark set plus the Native/neography
 * declarations. Extending the table is a data update (a minor revision) and
 * never changes the API. A mark with no curated record is still answered, by
 * the algorithmic default of §6 — the property is total.
 */

#include "suas/suas_suamr.h"

/* ── Portable byte extraction ─────────────────────────────────────── */

static uint8_t suamr_byte_at(uint32_t cp, unsigned i)
{
    return (uint8_t)((cp >> (8u * i)) & 0xFFu);
}

/* ── Compile-time record geometry (§3.1) ──────────────────────────── */

#define SUAMR_STATIC_ASSERT(cond, name) \
    typedef char name[(cond) ? 1 : -1]

SUAMR_STATIC_ASSERT(sizeof(suamr_mark_layout_t) == 12u,
                    suamr_layout_is_12_bytes);
SUAMR_STATIC_ASSERT(offsetof(suamr_mark_layout_t, codepoint) == 0u,
                    suamr_layout_codepoint_at_0);
SUAMR_STATIC_ASSERT(offsetof(suamr_mark_layout_t, x_offset) == 4u,
                    suamr_layout_x_at_4);
SUAMR_STATIC_ASSERT(offsetof(suamr_mark_layout_t, y_offset) == 6u,
                    suamr_layout_y_at_6);
SUAMR_STATIC_ASSERT(offsetof(suamr_mark_layout_t, positioning_flags) == 8u,
                    suamr_layout_flags_at_8);
SUAMR_STATIC_ASSERT(offsetof(suamr_mark_layout_t, stacking_level) == 10u,
                    suamr_layout_level_at_10);
SUAMR_STATIC_ASSERT(offsetof(suamr_mark_layout_t, reserved) == 11u,
                    suamr_layout_reserved_at_11);

/* ── The curated static registry (§8) ─────────────────────────────── */

/* Strictly ascending codepoint order is a verification invariant, not a
 * convenience: the enumeration surface, the duplicate check and the compiler's
 * partition proof all depend on it. Every row carries the stacking flags
 * (STACK_ABOVE / STACK_BELOW / MCM / SHADDA / KASRA_LIKE / STAGGER /
 * TRANSPARENT / CUSTOM_CLASS), a signed 2D offset in 1/64 em, and a stacking
 * level (0 = base-most).
 *
 * The canonical combining class is kept in a *separate* parallel table below
 * (`suamr_ccc_table`), because the sketch's 12-byte record has no ccc field and
 * a 220/230 mark must keep its class without widening the record. */

static const suamr_mark_layout_t suamr_table[] = {
    /* codepoint     x_off  y_off  flags                                                     lvl rsv */
    /* ── Fathatan (ccc=27): above ─────────────────────────────────────────────── */
    { 0x0000064Bu,  0x0000, 0x0020, SUAMR_FLAG_STACK_ABOVE,                                    1u, 0u },
    /* ── Dammatan (ccc=28): above ─────────────────────────────────────────────── */
    { 0x0000064Cu,  0x0000, 0x0020, SUAMR_FLAG_STACK_ABOVE,                                    1u, 0u },
    /* ── Kasratan (ccc=29): below ─────────────────────────────────────────────── */
    { 0x0000064Du,  0x0000, 0xFFE0, SUAMR_FLAG_STACK_BELOW,                                    1u, 0u },
    /* ── Fatha (ccc=30): above ────────────────────────────────────────────────── */
    { 0x0000064Eu,  0x0000, 0x0018, SUAMR_FLAG_STACK_ABOVE,                                    1u, 0u },
    /* ── Damma (ccc=31): above ────────────────────────────────────────────────── */
    { 0x0000064Fu,  0x0000, 0x0018, SUAMR_FLAG_STACK_ABOVE,                                    1u, 0u },
    /* ── Kasra (ccc=32): below, kasra-like ────────────────────────────────────── */
    { 0x00000650u,  0x0000, 0xFFE8, SUAMR_FLAG_STACK_BELOW | SUAMR_FLAG_KASRA_LIKE,            1u, 0u },
    /* ── Shadda (ccc=33): above, base-most stacking level ─────────────────────── */
    { 0x00000651u,  0x0000, 0x0008, SUAMR_FLAG_STACK_ABOVE | SUAMR_FLAG_SHADDA,                0u, 0u },
    /* ── Sukun (ccc=34): above, transparent over the base ─────────────────────── */
    { 0x00000652u,  0x0000, 0x000C, SUAMR_FLAG_STACK_ABOVE | SUAMR_FLAG_TRANSPARENT,            0u, 0u },
    /* ── Maddah above (ccc=230): above ────────────────────────────────────────── */
    { 0x00000653u,  0x0000, 0x0028, SUAMR_FLAG_STACK_ABOVE,                                    1u, 0u },
    /* ── Hamza above (ccc=230): above, MCM (binds tight to the base) ──────────── */
    { 0x00000654u,  0x0000, 0x0004, SUAMR_FLAG_STACK_ABOVE | SUAMR_FLAG_MCM,                   0u, 0u },
    /* ── Hamza below (ccc=220): below, MCM ────────────────────────────────────── */
    { 0x00000655u,  0x0000, 0xFFF4, SUAMR_FLAG_STACK_BELOW | SUAMR_FLAG_MCM,                   0u, 0u },
    /* ── Subscript alef (ccc=220): below, transparent ─────────────────────────── */
    { 0x00000656u,  0x0000, 0xFFE0, SUAMR_FLAG_STACK_BELOW | SUAMR_FLAG_TRANSPARENT,            1u, 0u },
    /* ── Inverted damma (ccc=230): above ──────────────────────────────────────── */
    { 0x00000657u,  0x0000, 0x0018, SUAMR_FLAG_STACK_ABOVE,                                    1u, 0u },
    /* ── Noon ghunna (ccc=230): above, MCM ────────────────────────────────────── */
    { 0x00000658u,  0x0000, 0x0010, SUAMR_FLAG_STACK_ABOVE | SUAMR_FLAG_MCM,                   0u, 0u },
    /* ── Small high meem isolated (ccc=230): above ────────────────────────────── */
    { 0x00000661u,  0x0000, 0x0014, SUAMR_FLAG_STACK_ABOVE,                                    1u, 0u },
    /* ── Superscript alef (ccc=35): above ─────────────────────────────────────── */
    { 0x00000670u,  0x0000, 0x0028, SUAMR_FLAG_STACK_ABOVE,                                    1u, 0u },
    /* ── Small high seen (ccc=230): above, MCM ────────────────────────────────── */
    { 0x000006DCu,  0x0000, 0x0018, SUAMR_FLAG_STACK_ABOVE | SUAMR_FLAG_MCM,                   1u, 0u },
    /* ── Small high meem (ccc=230): above ─────────────────────────────────────── */
    { 0x000006DEu,  0x0000, 0x0014, SUAMR_FLAG_STACK_ABOVE,                                    1u, 0u },
    /* ── Small low seen (ccc=220): below, MCM ─────────────────────────────────── */
    { 0x000006E3u,  0x0000, 0xFFE8, SUAMR_FLAG_STACK_BELOW | SUAMR_FLAG_MCM,                   1u, 0u },
    /* ── Small high yeh (ccc=230): above, MCM ─────────────────────────────────── */
    { 0x000006E7u,  0x0000, 0x0018, SUAMR_FLAG_STACK_ABOVE | SUAMR_FLAG_MCM,                   1u, 0u },
    /* ── Small high noon (ccc=230): above, MCM ────────────────────────────────── */
    { 0x000006E8u,  0x0000, 0x0014, SUAMR_FLAG_STACK_ABOVE | SUAMR_FLAG_MCM,                   1u, 0u },
    /* ── Small low meem (ccc=220): below ──────────────────────────────────────── */
    { 0x000006EDu,  0x0000, 0xFFE8, SUAMR_FLAG_STACK_BELOW,                                    1u, 0u },
    /* ── Large round dot above (ccc=230): above, MCM ──────────────────────────── */
    { 0x000008CEu,  0x0000, 0x001C, SUAMR_FLAG_STACK_ABOVE | SUAMR_FLAG_MCM,                   1u, 0u },
    /* ── Large round dot below (ccc=220): below, MCM ──────────────────────────── */
    { 0x000008CFu,  0x0000, 0xFFE4, SUAMR_FLAG_STACK_BELOW | SUAMR_FLAG_MCM,                   1u, 0u },
    /* ── Small low waw (ccc=220): below, MCM ──────────────────────────────────── */
    { 0x000008D3u,  0x0000, 0xFFE8, SUAMR_FLAG_STACK_BELOW | SUAMR_FLAG_MCM,                   1u, 0u },
    /* ── Small low noon with kasra (ccc=230 misassigned, §5.8): below, MCM ────── */
    { 0x000008D9u,  0x0000, 0xFFE0, SUAMR_FLAG_STACK_BELOW | SUAMR_FLAG_MCM,                   1u, 0u },
    /* ── Small high waw (ccc=230): above, MCM ─────────────────────────────────── */
    { 0x000008F3u,  0x0000, 0x0018, SUAMR_FLAG_STACK_ABOVE | SUAMR_FLAG_MCM,                   1u, 0u },
    /* ── Small low noon with fatha (ccc=220, ancient): below, MCM ─────────────── */
    { 0x00010EF4u,  0x0000, 0xFFE0, SUAMR_FLAG_STACK_BELOW | SUAMR_FLAG_MCM,                   1u, 0u },
    /* ── Small low noon with damma (ccc=220, ancient): below, MCM ─────────────── */
    { 0x00010EF6u,  0x0000, 0xFFE0, SUAMR_FLAG_STACK_BELOW | SUAMR_FLAG_MCM,                   1u, 0u },
    /* ── Mark crown (ccc=230, ancient): above, MCM ────────────────────────────── */
    { 0x00010EF9u,  0x0000, 0x0020, SUAMR_FLAG_STACK_ABOVE | SUAMR_FLAG_MCM,                   1u, 0u },
    /* ── Native / RNUR neography declarations ─────────────────────────────────── */
    { 0x00160100u,  0x0000, 0x0018, SUAMR_FLAG_STACK_ABOVE | SUAMR_FLAG_CUSTOM_CLASS,          1u, 0u },
    { 0x00160101u,  0x0000, 0xFFE8, SUAMR_FLAG_STACK_BELOW | SUAMR_FLAG_CUSTOM_CLASS,          1u, 0u },
    { 0x00160102u,  0x0000, 0x0018, SUAMR_FLAG_STACK_ABOVE | SUAMR_FLAG_STAGGER
                                     | SUAMR_FLAG_CUSTOM_CLASS,                                2u, 0u },
};

#define SUAMR_TABLE_COUNT \
    ((uint32_t)(sizeof suamr_table / sizeof suamr_table[0]))

SUAMR_STATIC_ASSERT(SUAMR_TABLE_COUNT <= SUAS_SUAMR_REGISTRY_CAPACITY,
                    suamr_table_fits_capacity);

/* ── Canonical combining classes (§4) ─────────────────────────────── */

/* A parallel array to `suamr_table`, same order and same length. The sketch's
 * 12-byte record has no ccc field, so the class lives here; keeping it out of
 * the record preserves the byte layout the brief specified. */
static const uint8_t suamr_ccc_table[SUAMR_TABLE_COUNT] = {
    SUAMR_CCC_FATHATAN,        /* U+064B */
    SUAMR_CCC_DAMMATAN,        /* U+064C */
    SUAMR_CCC_KASRATAN,        /* U+064D */
    SUAMR_CCC_FATHA,           /* U+064E */
    SUAMR_CCC_DAMMA,           /* U+064F */
    SUAMR_CCC_KASRA,           /* U+0650 */
    SUAMR_CCC_SHADDA,          /* U+0651 */
    SUAMR_CCC_SUKUN,           /* U+0652 */
    SUAMR_CCC_ABOVE,           /* U+0653 */
    SUAMR_CCC_ABOVE,           /* U+0654 */
    SUAMR_CCC_BELOW,           /* U+0655 */
    SUAMR_CCC_BELOW,           /* U+0656 */
    SUAMR_CCC_ABOVE,           /* U+0657 */
    SUAMR_CCC_ABOVE,           /* U+0658 */
    SUAMR_CCC_ABOVE,           /* U+0661 */
    SUAMR_CCC_SUPERSCRIPT_ALEF,/* U+0670 */
    SUAMR_CCC_ABOVE,           /* U+06DC */
    SUAMR_CCC_ABOVE,           /* U+06DE */
    SUAMR_CCC_BELOW,           /* U+06E3 */
    SUAMR_CCC_ABOVE,           /* U+06E7 */
    SUAMR_CCC_ABOVE,           /* U+06E8 */
    SUAMR_CCC_BELOW,           /* U+06ED */
    SUAMR_CCC_ABOVE,           /* U+08CE */
    SUAMR_CCC_BELOW,           /* U+08CF */
    SUAMR_CCC_BELOW,           /* U+08D3 */
    SUAMR_CCC_ABOVE,           /* U+08D9 (misassigned 230, §5.8) */
    SUAMR_CCC_ABOVE,           /* U+08F3 */
    SUAMR_CCC_BELOW,           /* U+10EF4 */
    SUAMR_CCC_BELOW,           /* U+10EF6 */
    SUAMR_CCC_ABOVE,           /* U+10EF9 */
    SUAMR_CCC_BELOW,           /* 0x160100 */
    SUAMR_CCC_ABOVE,           /* 0x160101 */
    SUAMR_CCC_ABOVE            /* 0x160102 */
};

/* Compile-time proof that the two tables stay aligned. */
SUAMR_STATIC_ASSERT(sizeof suamr_ccc_table / sizeof suamr_ccc_table[0]
                    == SUAMR_TABLE_COUNT,
                    suamr_ccc_table_aligned);

/* ── The static lookup index (§3.3) ───────────────────────────────── */

typedef struct {
    uint16_t entry;
    uint8_t  used;
} suamr_index_slot_t;

static suamr_index_slot_t suamr_index[SUAS_SUAMR_INDEX_SIZE];
static bool suamr_ready = false;

/* ── FNV-1a primitives (§3.3) ─────────────────────────────────────── */

#define SUAMR_FNV_OFFSET 2166136261u
#define SUAMR_FNV_PRIME  16777619u

uint32_t suas_suamr_hash(uint32_t codepoint)
{
    uint32_t h = SUAMR_FNV_OFFSET;
    unsigned i;

    for (i = 0u; i < 4u; i++) {
        h ^= (uint32_t)suamr_byte_at(codepoint, i);
        h *= SUAMR_FNV_PRIME;
    }
    return h;
}

uint32_t suas_suamr_hash_pair(uint32_t base_codepoint, uint32_t mark_codepoint)
{
    uint32_t h = SUAMR_FNV_OFFSET;
    uint32_t cps[2];
    unsigned c;
    unsigned i;

    cps[0] = base_codepoint;
    cps[1] = mark_codepoint;
    for (c = 0u; c < 2u; c++) {
        for (i = 0u; i < 4u; i++) {
            h ^= (uint32_t)suamr_byte_at(cps[c], i);
            h *= SUAMR_FNV_PRIME;
        }
    }
    return h;
}

/* ── Zone dispatch (§2) ───────────────────────────────────────────── */

suas_suamr_zone_t suas_suamr_zone_of(uint32_t codepoint)
{
    if (codepoint > (uint32_t)SUAS_CODEPOINT_MAX)
        return SUAS_SUAMR_ZONE_EXTSUCS;
    if (codepoint <= SUAS_SUAMR_LIMIT_UNICODE_MAX)
        return SUAS_SUAMR_ZONE_BRIDGE;
    if (codepoint >= SUAS_SUAMR_LIMIT_SCP_MIN &&
        codepoint <= SUAS_SUAMR_LIMIT_SCP_MAX)
        return SUAS_SUAMR_ZONE_SCP;
    if (codepoint >= SUAS_SUAMR_LIMIT_TRAP_MIN &&
        codepoint <= SUAS_SUAMR_LIMIT_TRAP_MAX)
        return SUAS_SUAMR_ZONE_TRAP;
    if (codepoint == SUAS_SUAMR_LIMIT_SENTINEL)
        return SUAS_SUAMR_ZONE_SENTINEL;
    if (codepoint >= SUAS_SUAMR_RNUR_MIN &&
        codepoint <= SUAS_SUAMR_RNUR_MAX)
        return SUAS_SUAMR_ZONE_RNUR;
    return SUAS_SUAMR_ZONE_NATIVE;
}

bool suas_suamr_is_text_space(uint32_t codepoint)
{
    switch (suas_suamr_zone_of(codepoint)) {
    case SUAS_SUAMR_ZONE_BRIDGE:
    case SUAS_SUAMR_ZONE_RNUR:
    case SUAS_SUAMR_ZONE_NATIVE:
        return true;
    default:
        return false;
    }
}

bool suas_suamr_is_neography(uint32_t codepoint)
{
    return suas_suamr_zone_of(codepoint) == SUAS_SUAMR_ZONE_RNUR;
}

bool suas_suamr_neography_set(uint32_t codepoint, uint32_t *out_set)
{
    if (out_set == NULL || !suas_suamr_is_neography(codepoint))
        return false;
    *out_set = (codepoint - SUAS_SUAMR_RNUR_MIN) >> SUAS_SUAMR_RNUR_SET_SHIFT;
    return true;
}

/* ── Lazy index construction (§3.3) ───────────────────────────────── */

static void suamr_build(void)
{
    uint32_t i;
    uint32_t h;
    uint32_t start;

    for (i = 0u; i < SUAS_SUAMR_INDEX_SIZE; i++) {
        suamr_index[i].entry = 0u;
        suamr_index[i].used = 0u;
    }
    for (i = 0u; i < SUAMR_TABLE_COUNT; i++) {
        h = suas_suamr_hash(suamr_table[i].codepoint) &
            (SUAS_SUAMR_INDEX_SIZE - 1u);
        start = h;
        while (suamr_index[h].used) {
            h = (h + 1u) & (SUAS_SUAMR_INDEX_SIZE - 1u);
            if (h == start)
                break; /* unreachable: capacity < index size */
        }
        suamr_index[h].entry = (uint16_t)i;
        suamr_index[h].used = 1u;
    }

    suamr_ready = true;
}

void suas_suamr_init_engine(void)
{
    if (!suamr_ready)
        suamr_build();
}

/* ── O(1) probe (§3.3) ────────────────────────────────────────────── */

static const suamr_mark_layout_t* suamr_find(uint32_t codepoint)
{
    uint32_t h;
    uint32_t start;

    if (!suas_suamr_is_text_space(codepoint))
        return NULL;
    if (!suamr_ready)
        suamr_build();

    h = suas_suamr_hash(codepoint) & (SUAS_SUAMR_INDEX_SIZE - 1u);
    start = h;
    for (;;) {
        const suamr_mark_layout_t* cand;
        if (!suamr_index[h].used)
            return NULL;
        cand = &suamr_table[suamr_index[h].entry];
        if (cand->codepoint == codepoint)
            return cand;
        h = (h + 1u) & (SUAS_SUAMR_INDEX_SIZE - 1u);
        if (h == start)
            return NULL;
    }
}

/* The table index of a curated codepoint, or (uint32_t)-1. */
static uint32_t suamr_table_index(uint32_t codepoint)
{
    const suamr_mark_layout_t* found = suamr_find(codepoint);
    uint32_t i;

    if (found == NULL)
        return (uint32_t)-1;
    i = (uint32_t)(found - suamr_table);
    return i;
}

/* ── Algorithmic default (§6) ─────────────────────────────────────── */

/* True when an uncurated codepoint is a combining mark by its block. SUAMR
 * curates the Arabic combining inventory, but the property must stay total:
 * a mark the table does not carry — a later encoding, or a neography
 * declaration not yet built into the image — is still answered. These are the
 * Arabic-script combining ranges of the Unicode Standard; the RNUR carve-out
 * is included so a registry-declared mark resolves without a curated row. */
static bool suamr_is_default_mark(uint32_t cp)
{
    /* Arabic (0600-06FF) combining marks: 0610-061A, 064B-065F, 0670, and the
     * Quranic annotation marks 06D6-06ED. */
    if (cp >= 0x00000610u && cp <= 0x0000061Au) return true;
    if (cp >= 0x0000064Bu && cp <= 0x0000065Fu) return true;
    if (cp == 0x00000670u)                       return true;
    if (cp >= 0x000006D6u && cp <= 0x000006EDu) return true;
    /* Arabic Extended-A (08E3-08FF) combining marks 08D3-08FF. */
    if (cp >= 0x000008D3u && cp <= 0x000008FFu) return true;
    /* Arabic Extended-B (0870-08FF) marks 0898-08E1 were folded into the above
     * via the 08D3-08FF span only for Extended-A; keep the older 0898-089F. */
    if (cp >= 0x00000898u && cp <= 0x0000089Fu) return true;
    /* Arabic Extended-C (10EF0-10EFF) Quranic marks. */
    if (cp >= 0x000010EF0u && cp <= 0x000010EFFu) return true;
    /* The RNUR neography carve-out: a declared custom mark. */
    if (cp >= SUAS_SUAMR_RNUR_MIN && cp <= SUAS_SUAMR_RNUR_MAX) return true;
    return false;
}

/* A mark with no curated record is not unknown: its stacking follows from its
 * combining class, which is itself derived from the codepoint's block. The
 * synthesized record carries the derived flags and a modest offset. */
static suamr_mark_layout_t suamr_default_record(uint32_t mark_codepoint)
{
    suamr_mark_layout_t rec;
    uint16_t flags = 0u;
    int16_t y = 0x0018;

    /* Below marks sit under the base; above marks over it. The classes are the
     * UAX #53 generalized ones: the tashkil kasratan/kasra and the small low
     * seen/meem are below, everything else above. */
    if ((mark_codepoint >= 0x0000064Du && mark_codepoint <= 0x0000064Du) ||
        (mark_codepoint >= 0x00000650u && mark_codepoint <= 0x00000650u) ||
        (mark_codepoint >= 0x00000655u && mark_codepoint <= 0x00000656u) ||
        (mark_codepoint >= 0x000006E3u && mark_codepoint <= 0x000006E4u) ||
        (mark_codepoint >= 0x000008CFu && mark_codepoint <= 0x000008CFu)) {
        flags = SUAMR_FLAG_STACK_BELOW;
        y = (int16_t)0xFFE0;
    } else {
        flags = SUAMR_FLAG_STACK_ABOVE;
    }

    rec.codepoint = mark_codepoint;
    rec.x_offset = 0;
    rec.y_offset = y;
    rec.positioning_flags = flags;
    rec.stacking_level = 1u;
    rec.reserved = 0u;
    return rec;
}

/* ── Query surface (§3) ───────────────────────────────────────────── */

bool suas_suamr_get_mark_layout(uint32_t base_codepoint,
                                uint32_t mark_codepoint,
                                suamr_mark_layout_t *out_layout)
{
    const suamr_mark_layout_t* found;

    (void)base_codepoint; /* the 1.0 record is base-independent (§3.2) */

    if (out_layout == NULL)
        return false;
    if (!suas_suamr_is_text_space(mark_codepoint))
        return false;

    found = suamr_find(mark_codepoint);
    if (found != NULL) {
        *out_layout = *found;
        return true;
    }
    if (!suamr_is_default_mark(mark_codepoint))
        return false; /* an ordinary starter has no mark layout */
    *out_layout = suamr_default_record(mark_codepoint);
    return true;
}

bool suas_suamr_is_mark(uint32_t codepoint)
{
    return suamr_find(codepoint) != NULL ||
           (suas_suamr_is_text_space(codepoint) &&
            suamr_is_default_mark(codepoint));
}

bool suas_suamr_is_registered(uint32_t codepoint)
{
    return suamr_find(codepoint) != NULL;
}

bool suas_suamr_is_above(uint32_t codepoint)
{
    const suamr_mark_layout_t* found = suamr_find(codepoint);

    if (found != NULL)
        return (found->positioning_flags & SUAMR_FLAG_STACK_ABOVE) != 0u;
    if (!suas_suamr_is_text_space(codepoint) || !suamr_is_default_mark(codepoint))
        return false;
    return (suas_suamr_ccc(codepoint) != SUAMR_CCC_BELOW);
}

bool suas_suamr_is_below(uint32_t codepoint)
{
    const suamr_mark_layout_t* found = suamr_find(codepoint);

    if (found != NULL)
        return (found->positioning_flags & SUAMR_FLAG_STACK_BELOW) != 0u;
    if (!suas_suamr_is_text_space(codepoint) || !suamr_is_default_mark(codepoint))
        return false;
    return (suas_suamr_ccc(codepoint) == SUAMR_CCC_BELOW);
}

bool suas_suamr_is_mcm(uint32_t codepoint)
{
    const suamr_mark_layout_t* found = suamr_find(codepoint);

    return found != NULL &&
           (found->positioning_flags & SUAMR_FLAG_MCM) != 0u;
}

bool suas_suamr_is_shadda(uint32_t codepoint)
{
    return codepoint == SUAMR_SHADDA_CP;
}

uint8_t suas_suamr_ccc(uint32_t codepoint)
{
    uint32_t i = suamr_table_index(codepoint);
    suamr_mark_layout_t dflt;

    if (i != (uint32_t)-1)
        return suamr_ccc_table[i];

    /* A mark outside the curated set: the joiner and an ordinary starter are
     * ccc=0; a combining mark derives below/above from its block. */
    if (codepoint == SUAMR_CGJ || !suas_suamr_is_text_space(codepoint) ||
        !suamr_is_default_mark(codepoint))
        return SUAMR_CCC_STARTER;
    dflt = suamr_default_record(codepoint);
    if (dflt.positioning_flags & SUAMR_FLAG_STACK_BELOW)
        return SUAMR_CCC_BELOW;
    return SUAMR_CCC_ABOVE;
}

uint8_t suas_suamr_stacking_level(uint32_t codepoint)
{
    const suamr_mark_layout_t* found = suamr_find(codepoint);

    if (found != NULL)
        return found->stacking_level;
    return (suas_suamr_is_text_space(codepoint) &&
            suamr_is_default_mark(codepoint)) ? 1u : 0u;
}

int suas_suamr_x_offset(uint32_t base_codepoint, uint32_t mark_codepoint)
{
    suamr_mark_layout_t rec;

    if (!suas_suamr_get_mark_layout(base_codepoint, mark_codepoint, &rec))
        return 0;
    return (int)rec.x_offset;
}

int suas_suamr_y_offset(uint32_t base_codepoint, uint32_t mark_codepoint)
{
    suamr_mark_layout_t rec;

    if (!suas_suamr_get_mark_layout(base_codepoint, mark_codepoint, &rec))
        return 0;
    return (int)rec.y_offset;
}

/* ── Transient reordering (§5, AMTRA) ─────────────────────────────── */

/* True when the mark must not be moved across `index` because a CGJ immediately
 * precedes it: the CGJ is a barrier (§5.3). A CGJ at position 0 is a starter
 * and is handled by the caller. */
static bool suamr_cgj_barrier(const uint32_t *stream, size_t index)
{
    return index > 0u && stream[index - 1u] == SUAMR_CGJ;
}

static bool suamr_reorder_fail(suamr_reorder_status_t *out_status,
                               suamr_reorder_status_t status)
{
    if (out_status != NULL)
        *out_status = status;
    return false;
}

bool suas_suamr_reorder_cluster(uint32_t *codepoints,
                                size_t count,
                                suamr_reorder_status_t *out_status)
{
    size_t i;

    if (out_status != NULL)
        *out_status = SUAMR_REORDER_OK;
    if (codepoints == NULL)
        return suamr_reorder_fail(out_status, SUAMR_REORDER_NULL_ARGUMENT);
    if (count == 0u)
        return suamr_reorder_fail(out_status, SUAMR_REORDER_NULL_ARGUMENT);
    if (count > SUAS_SUAMR_MAX_CLUSTER)
        return suamr_reorder_fail(out_status, SUAMR_REORDER_TOO_LONG);

    /* Every codepoint must be admissible, and the first must be a starter. A
     * starter is ccc=0: the base, or a combining grapheme joiner. */
    for (i = 0u; i < count; i++) {
        if (!suas_suamr_is_text_space(codepoints[i]))
            return suamr_reorder_fail(out_status,
                                      SUAMR_REORDER_ILLEGAL_ZONE);
    }
    if (suas_suamr_ccc(codepoints[0]) != SUAMR_CCC_STARTER)
        return suamr_reorder_fail(out_status, SUAMR_REORDER_NO_BASE);

    if (count < 2u)
        return true;

    /* AMTRA (§3.2) reorders the marks that follow the base into the target
     * order (front to back):
     *
     *   (1) the leading run of ccc=220 MCM marks   — step 2c
     *   (2) the leading run of ccc=230 MCM marks   — step 2b
     *   (3) shadda (ccc=33)                        — step 2a
     *   (4) everything else, in original order     — stable default
     *
     * "Leading run" means: the maximal run of that class that starts at the
     * *first* mark of that class, and only if that first mark is an MCM. If the
     * first mark of the class is not an MCM, the class contributes nothing.
     *
     * Every mark is emitted by exactly one tier: the tiers partition the marks,
     * so no mark is duplicated and the relative order of same-tier marks is the
     * encoded order. A combining grapheme joiner (§4.2) pins the mark that
     * immediately follows it: such a mark is assigned to the last tier, so its
     * display order is exactly the encoded order. */
    {
        uint32_t out[SUAS_SUAMR_MAX_CLUSTER];
        size_t n_out = 0u;
        size_t tier;
        bool promote220 = false;
        bool promote230 = false;

        /* Whether the leading ccc=220 / ccc=230 run starts with an MCM: scan
         * the marks once, front to back, stopping at the first of each class. */
        for (i = 1u; i < count; i++) {
            uint8_t cc = suas_suamr_ccc(codepoints[i]);
            if (cc == SUAMR_CCC_BELOW) {
                promote220 = suas_suamr_is_mcm(codepoints[i]);
                break;
            }
        }
        for (i = 1u; i < count; i++) {
            uint8_t cc = suas_suamr_ccc(codepoints[i]);
            if (cc == SUAMR_CCC_ABOVE) {
                promote230 = suas_suamr_is_mcm(codepoints[i]);
                break;
            }
        }

        /* Now emit each mark into exactly one tier. A CGJ immediately before a
         * mark pins it: it is never promoted out of its slot, which the last
         * tier preserves. */
        for (tier = 0u; tier < 4u; tier++) {
            for (i = 1u; i < count; i++) {
                uint8_t cc = suas_suamr_ccc(codepoints[i]);
                bool pinned = suamr_cgj_barrier(codepoints, i);
                int slot;

                if (pinned) {
                    slot = 3; /* the pinned mark stays in the default tier */
                } else if (cc == SUAMR_CCC_BELOW && promote220 &&
                           suas_suamr_is_mcm(codepoints[i])) {
                    slot = 0;
                } else if (cc == SUAMR_CCC_ABOVE && promote230 &&
                           suas_suamr_is_mcm(codepoints[i])) {
                    slot = 1;
                } else if (cc == SUAMR_CCC_SHADDA) {
                    slot = 2;
                } else {
                    slot = 3;
                }

                if ((size_t)slot == tier)
                    out[n_out++] = codepoints[i];
            }
        }

        for (i = 0u; i < n_out; i++)
            codepoints[1u + i] = out[i];
    }

    return true;
}

/* ── Enumeration ──────────────────────────────────────────────────── */

uint32_t suas_suamr_entry_count(void)
{
    return SUAMR_TABLE_COUNT;
}

bool suas_suamr_entry_at(uint32_t index, suamr_mark_layout_t *out_layout)
{
    if (out_layout == NULL || index >= SUAMR_TABLE_COUNT)
        return false;
    *out_layout = suamr_table[index];
    return true;
}

/* ── Taxonomy names ───────────────────────────────────────────────── */

const char* suas_suamr_ccc_name(uint8_t ccc)
{
    switch (ccc) {
    case SUAMR_CCC_STARTER:          return "starter";
    case SUAMR_CCC_FATHATAN:         return "fathatan";
    case SUAMR_CCC_DAMMATAN:         return "dammatan";
    case SUAMR_CCC_KASRATAN:         return "kasratan";
    case SUAMR_CCC_FATHA:            return "fatha";
    case SUAMR_CCC_DAMMA:            return "damma";
    case SUAMR_CCC_KASRA:            return "kasra";
    case SUAMR_CCC_SHADDA:           return "shadda";
    case SUAMR_CCC_SUKUN:            return "sukun";
    case SUAMR_CCC_SUPERSCRIPT_ALEF: return "superscript-alef";
    case SUAMR_CCC_BELOW:            return "below";
    case SUAMR_CCC_ABOVE:            return "above";
    default:                         return "unknown";
    }
}

const char* suas_suamr_zone_name(suas_suamr_zone_t zone)
{
    switch (zone) {
    case SUAS_SUAMR_ZONE_BRIDGE:   return "bridge";
    case SUAS_SUAMR_ZONE_SCP:      return "scp";
    case SUAS_SUAMR_ZONE_RNUR:     return "rnur";
    case SUAS_SUAMR_ZONE_NATIVE:   return "native";
    case SUAS_SUAMR_ZONE_TRAP:     return "trap";
    case SUAS_SUAMR_ZONE_SENTINEL: return "sentinel";
    case SUAS_SUAMR_ZONE_EXTSUCS:  return "extsucs";
    default:                       return "unknown";
    }
}

const char* suas_suamr_registry_error_name(suamr_registry_error_t error)
{
    switch (error) {
    case SUAMR_REGISTRY_OK:                       return "OK";
    case SUAMR_REGISTRY_NULL_TABLE:               return "NULL_TABLE";
    case SUAMR_REGISTRY_COUNT_OVER:               return "COUNT_OVER";
    case SUAMR_REGISTRY_UNORDERED:                return "UNORDERED";
    case SUAMR_REGISTRY_DUPLICATE_CODEPOINT:      return "DUPLICATE_CODEPOINT";
    case SUAMR_REGISTRY_OUT_OF_SPACE:             return "OUT_OF_SPACE";
    case SUAMR_REGISTRY_INADMISSIBLE_CODEPOINT:   return "INADMISSIBLE_CODEPOINT";
    case SUAMR_REGISTRY_BAD_CCC:                  return "BAD_CCC";
    case SUAMR_REGISTRY_BAD_FLAGS:                return "BAD_FLAGS";
    case SUAMR_REGISTRY_BAD_RESERVED:             return "BAD_RESERVED";
    case SUAMR_REGISTRY_CUSTOM_CLASS_MISMATCH:    return "CUSTOM_CLASS_MISMATCH";
    case SUAMR_REGISTRY_STACK_CONTRADICTION:      return "STACK_CONTRADICTION";
    default:                                      return "UNKNOWN";
    }
}

const char* suas_suamr_reorder_status_name(suamr_reorder_status_t status)
{
    switch (status) {
    case SUAMR_REORDER_OK:            return "ok";
    case SUAMR_REORDER_NULL_ARGUMENT: return "null-argument";
    case SUAMR_REORDER_ILLEGAL_ZONE:  return "illegal-zone";
    case SUAMR_REORDER_TOO_LONG:      return "too-long";
    case SUAMR_REORDER_NO_BASE:       return "no-base";
    default:                          return "unknown";
    }
}

/* ── Verification (§9) ────────────────────────────────────────────── */

static bool suamr_fail(suamr_registry_error_t *out_error,
                       suamr_registry_error_t error)
{
    if (out_error != NULL)
        *out_error = error;
    return false;
}

bool suas_suamr_validate_table(const suamr_mark_layout_t *entries,
                               uint32_t count,
                               suamr_registry_error_t *out_error)
{
    uint32_t i;

    if (out_error != NULL)
        *out_error = SUAMR_REGISTRY_OK;
    if (count > SUAS_SUAMR_REGISTRY_CAPACITY)
        return suamr_fail(out_error, SUAMR_REGISTRY_COUNT_OVER);
    if (count == 0u)
        return true;
    if (entries == NULL)
        return suamr_fail(out_error, SUAMR_REGISTRY_NULL_TABLE);

    for (i = 0u; i < count; i++) {
        const suamr_mark_layout_t *e = &entries[i];
        uint16_t flags = e->positioning_flags;

        if (e->codepoint > (uint32_t)SUAS_CODEPOINT_MAX)
            return suamr_fail(out_error, SUAMR_REGISTRY_OUT_OF_SPACE);
        if (!suas_suamr_is_text_space(e->codepoint))
            return suamr_fail(out_error,
                              SUAMR_REGISTRY_INADMISSIBLE_CODEPOINT);
        /* No reserved bits may be set. */
        if ((flags & (uint16_t)~SUAMR_FLAG_ASSIGNED_MASK) != 0u)
            return suamr_fail(out_error, SUAMR_REGISTRY_BAD_FLAGS);
        /* A mark stacks above or below, never both. */
        if ((flags & SUAMR_FLAG_STACK_ABOVE) != 0u &&
            (flags & SUAMR_FLAG_STACK_BELOW) != 0u)
            return suamr_fail(out_error,
                              SUAMR_REGISTRY_STACK_CONTRADICTION);
        /* A mark must stack somewhere. */
        if ((flags & (SUAMR_FLAG_STACK_ABOVE | SUAMR_FLAG_STACK_BELOW)) == 0u)
            return suamr_fail(out_error, SUAMR_REGISTRY_BAD_FLAGS);
        /* A custom-class declaration only makes sense in the RNUR set. */
        if ((flags & SUAMR_FLAG_CUSTOM_CLASS) != 0u &&
            !suas_suamr_is_neography(e->codepoint))
            return suamr_fail(out_error,
                              SUAMR_REGISTRY_CUSTOM_CLASS_MISMATCH);
        if (e->reserved != 0u)
            return suamr_fail(out_error, SUAMR_REGISTRY_BAD_RESERVED);

        if (i > 0u) {
            if (e->codepoint == entries[i - 1u].codepoint)
                return suamr_fail(out_error,
                                  SUAMR_REGISTRY_DUPLICATE_CODEPOINT);
            if (e->codepoint < entries[i - 1u].codepoint)
                return suamr_fail(out_error, SUAMR_REGISTRY_UNORDERED);
        }
    }
    return true;
}

/* ── Version ──────────────────────────────────────────────────────── */

const char* suas_suamr_version_string(void)
{
    return "1.0";
}
