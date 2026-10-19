/* SUAS-009 — System Ideographic Properties (SIP)
 *
 * Reference implementation of docs/suas/SUAS-009-sip.md.
 *
 * Freestanding C99, zero allocation. A static table of curated ideographic
 * property records is indexed once into a fixed-size open-addressing array, so
 * that radical-stroke data, classification flags, a collation seed, and a
 * variant linkage are all extracted in O(1). No heap, no dynamic table growth,
 * no text parsing.
 *
 * The runtime layer is fed at build time from the offline reference layer (the
 * SuperUnicode Character Database, SUCD); the table below is a curated subset
 * of that dataset. Extending the table is a data update (a minor revision) and
 * never changes the API.
 */

#include "suas/suas_sip.h"

/* ── Portable wide shifts ─────────────────────────────────────────── */

/* A uint32_t shifted by an int, masked to 0xFF; avoids any 64-bit
 * dependency in the hash while remaining exact. */
static uint8_t sip_byte_at(sucs_ex_char_t cp, unsigned i)
{
    return (uint8_t)((cp >> (8u * i)) & (sucs_ex_char_t)0xFFULL);
}

/* ── Hash (§6.3): FNV-1a 32-bit over the little-endian encoding ───── */

uint32_t suas_sip_hash(sucs_ex_char_t codepoint)
{
    uint32_t h = 2166136261u;
    unsigned i;

    for (i = 0u; i < 8u; i++) {
        h ^= (uint32_t)sip_byte_at(codepoint, i);
        h *= 16777619u;
    }
    return h;
}

/* ── Zone helpers ─────────────────────────────────────────────────── */

static bool sip_in(sucs_ex_char_t cp, sucs_ex_char_t lo, sucs_ex_char_t hi)
{
    return cp >= lo && cp <= hi;
}

/* The System Control Plane, the trap slots, and the Sentinel are never
 * admissible; a record can never name them and a query for them always
 * misses. */
static bool sip_admissible(sucs_ex_char_t cp)
{
    if (sip_in(cp, SUAS_SIP_ZONE_SCP_MIN, SUAS_SIP_ZONE_SCP_MAX))
        return false;
    if (sip_in(cp, SUAS_SIP_ZONE_TRAP_MIN, SUAS_SIP_ZONE_SENTINEL))
        return false;
    return true;
}

/* ── The curated static property table (§5, §8) ───────────────────── */

#define SIP_U   SUAS_SIP_FLAG_UNIFIED
#define SIP_R   SUAS_SIP_FLAG_RADICAL
#define SIP_V   SUAS_SIP_FLAG_VARIANT
#define SIP_N   SUAS_SIP_FLAG_NUMERIC
#define SIP_NAT SUAS_SIP_FLAG_NATIVE
#define SIP_C   SUAS_SIP_FLAG_COMPAT
#define SIP_T   SUAS_SIP_FLAG_TRADITIONAL

static const suas_glyph_prop_t sip_table[] = {
    /* ── Bridge: CJK Unified Ideographs (URO), 0x4E00-0x9FFF (§5.1) ── */
    { 0x00004E00ULL,   1,  1, (SIP_U | SIP_R),         0x00004E00u, 0ULL },
    { 0x00004E8CULL,   7,  2, (SIP_U | SIP_R),         0x00004E8Cu, 0ULL },
    { 0x00004E09ULL,   1,  3, (SIP_U | SIP_R),         0x00004E09u, 0ULL },
    { 0x000056DBULL,  31,  5, (SIP_U | SIP_R | SIP_N), 0x000056DBu, 0ULL },
    { 0x00004E94ULL,   7,  4, (SIP_U | SIP_R | SIP_N), 0x00004E94u, 0ULL },
    { 0x00005341ULL,  24,  2, (SIP_U | SIP_R | SIP_N), 0x00005341u, 0ULL },
    { 0x00004EBAULL,   9,  2, (SIP_U | SIP_R),         0x00004EBAu, 0ULL },
    { 0x00005927ULL,  37,  3, (SIP_U | SIP_R),         0x00005927u, 0ULL },
    { 0x00005C0FULL,  42,  3, (SIP_U | SIP_R),         0x00005C0Fu, 0ULL },
    { 0x00005C71ULL,  46,  3, (SIP_U | SIP_R),         0x00005C71u, 0ULL },
    { 0x00005DDDULL,  47,  3, (SIP_U | SIP_R),         0x00005DDDu, 0ULL },
    { 0x000065E5ULL,  72,  4, (SIP_U | SIP_R),         0x000065E5u, 0ULL },
    { 0x00006708ULL,  74,  4, (SIP_U | SIP_R),         0x00006708u, 0ULL },
    { 0x00006728ULL,  75,  4, (SIP_U | SIP_R),         0x00006728u, 0ULL },
    { 0x00006C34ULL,  85,  4, (SIP_U | SIP_R),         0x00006C34u, 0ULL },
    { 0x0000706BULL,  86,  4, (SIP_U | SIP_R),         0x0000706Bu, 0ULL },
    { 0x0000571FULL,  32,  3, (SIP_U | SIP_R),         0x0000571Fu, 0ULL },
    { 0x000091D1ULL, 167,  8, (SIP_U | SIP_R),         0x000091D1u, 0ULL },
    /* Traditional 龍 and simplified 龙 link into the Native plane. */
    { 0x00009F8DULL, 212, 16, (SIP_U | SIP_R | SIP_V | SIP_T), 0x00009F8Du, 0x00160200ULL },
    { 0x00009F99ULL, 212,  5, (SIP_U | SIP_R | SIP_V),         0x00009F99u, 0x00160201ULL },

    /* ── Bridge: CJK Extension A, 0x3400-0x4DBF (§5.1) ─────────────── */
    { 0x00003402ULL,   1,  6, (SIP_U | SIP_R | SIP_V), 0x00003402u, 0x00160100ULL },

    /* ── Bridge: CJK Extension B, 0x20000-0x2A6DF (§5.1) ───────────── */
    { 0x00020000ULL,   1,  2, (SIP_U | SIP_R),         0x00020000u, 0ULL },

    /* ── Bridge: compatibility ideographs, 0xF900-0xFAFF and supplement.
     * The sort weight folds each compatibility form onto its canonical
     * unified counterpart, so the collation order interleaves them (§5.3).
     * ─────────────────────────────────────────────────────────────── */
    { 0x0000F900ULL, 151, 10, (SIP_C | SIP_V),         0x00008C48u, 0ULL },
    { 0x0002F800ULL,   1,  7, (SIP_C | SIP_V),         0x00004E3Du, 0ULL },

    /* ── Native & neography logographs, RNUR 0x160000-0x16FFFF (§5.2) ─ */
    { 0x00160100ULL,   1,  4, (SIP_NAT),               0x00160100u, 0ULL },
    { 0x00160101ULL,   1,  5, (SIP_NAT | SIP_V),       0x00160101u, 0x00160102ULL },
    { 0x00160200ULL,   2,  6, (SIP_NAT),               0x00160200u, 0ULL },
    { 0x00160201ULL,   2,  7, (SIP_NAT | SIP_V),       0x00160201u, 0x00160202ULL },
};

#define SIP_TABLE_COUNT ((uint32_t)(sizeof(sip_table) / sizeof(sip_table[0])))

/* ── The static lookup index (§6.2) ───────────────────────────────── */

typedef struct {
    uint8_t used;  /* slot occupied                          */
    uint8_t entry; /* index into sip_table (capacity < 256)  */
} sip_slot_t;

static sip_slot_t sip_index[SUAS_SIP_INDEX_SIZE];
static bool       sip_ready = false;

static void sip_build(void)
{
    uint32_t i;

    for (i = 0u; i < SUAS_SIP_INDEX_SIZE; i++) {
        sip_index[i].used = 0u;
        sip_index[i].entry = 0u;
    }
    for (i = 0u; i < SIP_TABLE_COUNT; i++) {
        uint32_t h = suas_sip_hash(sip_table[i].codepoint) &
                     (SUAS_SIP_INDEX_SIZE - 1u);
        while (sip_index[h].used)
            h = (h + 1u) & (SUAS_SIP_INDEX_SIZE - 1u);
        sip_index[h].used = 1u;
        sip_index[h].entry = (uint8_t)i;
    }
    sip_ready = true;
}

/* The O(1) lookup: returns the record for a codepoint, or NULL. Rejects the
 * inadmissible zones before probing. */
static const suas_glyph_prop_t* sip_find(sucs_ex_char_t cp)
{
    uint32_t h;
    uint32_t start;
    uint32_t key;

    if (!sip_admissible(cp))
        return NULL;
    if (!sip_ready)
        sip_build(); /* lazy build, so queries are safe without setup */

    key = suas_sip_hash(cp);
    h = key & (SUAS_SIP_INDEX_SIZE - 1u);
    start = h;
    for (;;) {
        const suas_glyph_prop_t* cand;
        if (!sip_index[h].used)
            return NULL;
        cand = &sip_table[sip_index[h].entry];
        if (cand->codepoint == cp)
            return cand;
        h = (h + 1u) & (SUAS_SIP_INDEX_SIZE - 1u);
        if (h == start)
            return NULL;
    }
}

/* ── Setup ────────────────────────────────────────────────────────── */

const char* suas_sip_version_string(void)
{
    return "1.0.0";
}

void suas_sip_init_engine(void)
{
    sip_build();
}

/* ── Core query (§3.2) — O(1) ─────────────────────────────────────── */

bool suas_sip_get_properties(sucs_ex_char_t codepoint,
                             suas_glyph_prop_t* out_prop)
{
    const suas_glyph_prop_t* e;

    if (!out_prop)
        return false;
    e = sip_find(codepoint);
    if (!e)
        return false;
    *out_prop = *e; /* fixed-size struct copy: no allocation */
    return true;
}

/* ── Enumeration ──────────────────────────────────────────────────── */

uint32_t suas_sip_entry_count(void)
{
    return SIP_TABLE_COUNT;
}

bool suas_sip_entry_at(uint32_t index, suas_glyph_prop_t* out_prop)
{
    if (!out_prop || index >= SIP_TABLE_COUNT)
        return false;
    *out_prop = sip_table[index];
    return true;
}

/* ── Algorithmic property extraction (§2) ─────────────────────────── */

bool suas_sip_get_radical_stroke(sucs_ex_char_t codepoint,
                                 uint16_t* out_radical, uint8_t* out_strokes)
{
    const suas_glyph_prop_t* e = sip_find(codepoint);

    if (!e)
        return false;
    if (out_radical)
        *out_radical = e->radical;
    if (out_strokes)
        *out_strokes = e->strokes;
    return true;
}

bool suas_sip_get_sort_weight(sucs_ex_char_t codepoint, uint32_t* out_weight)
{
    const suas_glyph_prop_t* e;

    if (!out_weight)
        return false;
    e = sip_find(codepoint);
    if (e) {
        *out_weight = e->sort_weight;
        return true;
    }
    if (!suas_sip_is_ideograph(codepoint))
        return false;
    /* Algorithmic native-order weight for a valid, unregistered ideograph:
     * the low 31 bits of the codepoint, so the order is total and stable. */
    *out_weight = (uint32_t)(codepoint & (sucs_ex_char_t)0x7FFFFFFFULL);
    return true;
}

uint8_t suas_sip_flags(sucs_ex_char_t codepoint)
{
    const suas_glyph_prop_t* e = sip_find(codepoint);

    return e ? e->flags : 0u;
}

bool suas_sip_has_variant(sucs_ex_char_t codepoint)
{
    const suas_glyph_prop_t* e = sip_find(codepoint);

    return e && (e->flags & SUAS_SIP_FLAG_VARIANT) != 0u;
}

bool suas_sip_variant_target(sucs_ex_char_t codepoint,
                             sucs_ex_char_t* out_native)
{
    const suas_glyph_prop_t* e;

    if (!out_native)
        return false;
    e = sip_find(codepoint);
    if (!e || e->native_variant == 0ULL)
        return false;
    *out_native = e->native_variant;
    return true;
}

/* ── Classification predicates (§2.1, §2.4) ───────────────────────── */

bool suas_sip_is_registered(sucs_ex_char_t codepoint)
{
    return sip_find(codepoint) != NULL;
}

bool suas_sip_is_ideograph(sucs_ex_char_t codepoint)
{
    if (suas_sip_is_registered(codepoint))
        return true;
    if (sip_in(codepoint, SUAS_SIP_BLOCK_EXT_A_MIN, SUAS_SIP_BLOCK_EXT_A_MAX))
        return true;
    if (sip_in(codepoint, SUAS_SIP_BLOCK_URO_MIN, SUAS_SIP_BLOCK_URO_MAX))
        return true;
    if (sip_in(codepoint, SUAS_SIP_BLOCK_COMPAT_MIN, SUAS_SIP_BLOCK_COMPAT_MAX))
        return true;
    if (sip_in(codepoint, SUAS_SIP_BLOCK_EXT_B_MIN, SUAS_SIP_BLOCK_EXT_B_MAX))
        return true;
    if (sip_in(codepoint, SUAS_SIP_BLOCK_EXT_C_MIN, SUAS_SIP_BLOCK_EXT_C_MAX))
        return true;
    if (sip_in(codepoint, SUAS_SIP_BLOCK_COMPAT2_MIN,
               SUAS_SIP_BLOCK_COMPAT2_MAX))
        return true;
    if (sip_in(codepoint, SUAS_SIP_BLOCK_EXT_G_MIN, SUAS_SIP_BLOCK_EXT_G_MAX))
        return true;
    if (sip_in(codepoint, SUAS_SIP_RNUR_MIN, SUAS_SIP_RNUR_MAX))
        return true;
    return false;
}

suas_sip_plane_t suas_sip_plane_of(sucs_ex_char_t codepoint)
{
    if (codepoint <= SUAS_SIP_ZONE_UNICODE_MAX)
        return SUAS_SIP_PLANE_BRIDGE;
    if (codepoint <= SUAS_SIP_ZONE_SCP_MAX)
        return SUAS_SIP_PLANE_SCP;
    if (sip_in(codepoint, SUAS_SIP_RNUR_MIN, SUAS_SIP_RNUR_MAX))
        return SUAS_SIP_PLANE_RNUR;
    if (codepoint <= SUAS_SIP_ZONE_NATIVE_LAST)
        return SUAS_SIP_PLANE_NATIVE;
    if (codepoint <= SUAS_SIP_ZONE_TRAP_MAX)
        return SUAS_SIP_PLANE_TRAP;
    if (codepoint == SUAS_SIP_ZONE_SENTINEL)
        return SUAS_SIP_PLANE_SENTINEL;
    return SUAS_SIP_PLANE_EXTSUCS;
}
