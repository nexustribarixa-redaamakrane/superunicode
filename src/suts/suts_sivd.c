/* SUTS-004 — System Ideographic Variation Database (SIVD)
 *
 * Reference implementation of docs/suts/SUTS-004-sivd.md.
 *
 * Freestanding C99, zero allocation. Variation sequences are compiled into an
 * immutable, statically-linked table of fixed-size sivd_variation_entry_t
 * records, grouped into sivd_collection_t collections. The entry table is
 * indexed once into a fixed-size open-addressing array keyed by the FNV-1a
 * hash of the (base codepoint, variation selector) pair, so the suf font
 * pipeline resolves a variant glyph index in O(1) expected time with no heap,
 * no flat-file parsing, and no dynamic table growth.
 *
 * The curated table holds the Bridge Variation Space (standard Ideographic
 * Variation Sequences over Unicode variation selectors) plus a representative
 * RNUR neography collection (Native & Neography Variation Space). Extending the
 * table is a compile-time data update (a minor revision) and never changes the
 * API.
 */

#include "suts/suts_sivd.h"

/* ── The curated collections (§2.2, Annex A) ──────────────────────── */

static const sivd_collection_t sivd_collections[] = {
    { .id = "Adobe-Japan1",  .id_pattern = "[0-9]+",
      .first_entry = 0u,  .entry_count = 5u,
      .plane = SUTS_SIVD_PLANE_BRIDGE, .reserved = 0u },
    { .id = "Hanyo-Denshi",  .id_pattern = "[0-9]+",
      .first_entry = 5u,  .entry_count = 4u,
      .plane = SUTS_SIVD_PLANE_BRIDGE, .reserved = 0u },
    { .id = "Moji_Joho",     .id_pattern = "[A-Za-z0-9_]+",
      .first_entry = 9u,  .entry_count = 3u,
      .plane = SUTS_SIVD_PLANE_BRIDGE, .reserved = 0u },
    { .id = "KRName",        .id_pattern = "[0-9]+",
      .first_entry = 12u, .entry_count = 2u,
      .plane = SUTS_SIVD_PLANE_BRIDGE, .reserved = 0u },
    { .id = "Example_names", .id_pattern = "[0-9]+",
      .first_entry = 14u, .entry_count = 1u,
      .plane = SUTS_SIVD_PLANE_BRIDGE, .reserved = 0u },
    { .id = "Lumer_Variants", .id_pattern = "[a-z0-9_]+",
      .first_entry = 15u, .entry_count = 5u,
      .plane = SUTS_SIVD_PLANE_RNUR,   .reserved = 0u },
};

#define SIVD_COLLECTION_COUNT \
    ((size_t)(sizeof(sivd_collections) / sizeof(sivd_collections[0])))

/* ── The curated binary variation table (§2.1, §4.1) ──────────────── */

static const sivd_variation_entry_t sivd_table[] = {
    /* ── Adobe-Japan1 (Bridge) ────────────────────────────────────── */
    { .base_codepoint = 0x3402ULL, .variation_selector = 0xE0100ULL,
      .resolved_glyph_index = 16u, .collection_index = 0u,
      .plane = SUTS_SIVD_PLANE_BRIDGE, .flags = SUTS_SIVD_FLAG_NONE },
    { .base_codepoint = 0x4E00ULL, .variation_selector = 0xE0100ULL,
      .resolved_glyph_index = 17u, .collection_index = 0u,
      .plane = SUTS_SIVD_PLANE_BRIDGE, .flags = SUTS_SIVD_FLAG_NONE },
    { .base_codepoint = 0x8FBBULL, .variation_selector = 0xE0100ULL,
      .resolved_glyph_index = 18u, .collection_index = 0u,
      .plane = SUTS_SIVD_PLANE_BRIDGE, .flags = SUTS_SIVD_FLAG_NONE },
    { .base_codepoint = 0x845BULL, .variation_selector = 0xE0100ULL,
      .resolved_glyph_index = 19u, .collection_index = 0u,
      .plane = SUTS_SIVD_PLANE_BRIDGE, .flags = SUTS_SIVD_FLAG_NONE },
    { .base_codepoint = 0x5F66ULL, .variation_selector = 0xE0100ULL,
      .resolved_glyph_index = 20u, .collection_index = 0u,
      .plane = SUTS_SIVD_PLANE_BRIDGE, .flags = SUTS_SIVD_FLAG_NONE },

    /* ── Hanyo-Denshi (Bridge) ────────────────────────────────────── */
    { .base_codepoint = 0x3402ULL, .variation_selector = 0xE0101ULL,
      .resolved_glyph_index = 32u, .collection_index = 1u,
      .plane = SUTS_SIVD_PLANE_BRIDGE, .flags = SUTS_SIVD_FLAG_NONE },
    { .base_codepoint = 0x4E00ULL, .variation_selector = 0xE0101ULL,
      .resolved_glyph_index = 33u, .collection_index = 1u,
      .plane = SUTS_SIVD_PLANE_BRIDGE, .flags = SUTS_SIVD_FLAG_NONE },
    { .base_codepoint = 0x8FBBULL, .variation_selector = 0xE0101ULL,
      .resolved_glyph_index = 34u, .collection_index = 1u,
      .plane = SUTS_SIVD_PLANE_BRIDGE, .flags = SUTS_SIVD_FLAG_NONE },
    { .base_codepoint = 0x6F22ULL, .variation_selector = 0xE0101ULL,
      .resolved_glyph_index = 35u, .collection_index = 1u,
      .plane = SUTS_SIVD_PLANE_BRIDGE, .flags = SUTS_SIVD_FLAG_NONE },

    /* ── Moji_Joho (Bridge) ───────────────────────────────────────── */
    { .base_codepoint = 0x3402ULL, .variation_selector = 0xE0102ULL,
      .resolved_glyph_index = 48u, .collection_index = 2u,
      .plane = SUTS_SIVD_PLANE_BRIDGE, .flags = SUTS_SIVD_FLAG_NONE },
    { .base_codepoint = 0x4E00ULL, .variation_selector = 0xE0102ULL,
      .resolved_glyph_index = 49u, .collection_index = 2u,
      .plane = SUTS_SIVD_PLANE_BRIDGE, .flags = SUTS_SIVD_FLAG_NONE },
    { .base_codepoint = 0xFA0CULL, .variation_selector = 0xE0102ULL,
      .resolved_glyph_index = 50u, .collection_index = 2u,
      .plane = SUTS_SIVD_PLANE_BRIDGE, .flags = SUTS_SIVD_FLAG_NONE },

    /* ── KRName (Bridge) ──────────────────────────────────────────── */
    { .base_codepoint = 0x9FB4ULL, .variation_selector = 0xE0100ULL,
      .resolved_glyph_index = 64u, .collection_index = 3u,
      .plane = SUTS_SIVD_PLANE_BRIDGE, .flags = SUTS_SIVD_FLAG_NONE },
    { .base_codepoint = 0x9FB5ULL, .variation_selector = 0xE0100ULL,
      .resolved_glyph_index = 65u, .collection_index = 3u,
      .plane = SUTS_SIVD_PLANE_BRIDGE, .flags = SUTS_SIVD_FLAG_NONE },

    /* ── Example_names (Bridge, Annex B) ──────────────────────────── */
    { .base_codepoint = 0x82A6ULL, .variation_selector = 0xE0134ULL,
      .resolved_glyph_index = 80u, .collection_index = 4u,
      .plane = SUTS_SIVD_PLANE_BRIDGE, .flags = SUTS_SIVD_FLAG_NONE },

    /* ── Lumer_Variants (RNUR neography) ──────────────────────────── */
    { .base_codepoint = 0x160100ULL, .variation_selector = 0x16FF00ULL,
      .resolved_glyph_index = 96u, .collection_index = 5u,
      .plane = SUTS_SIVD_PLANE_RNUR, .flags = SUTS_SIVD_FLAG_NONE },
    { .base_codepoint = 0x160100ULL, .variation_selector = 0x16FF01ULL,
      .resolved_glyph_index = 97u, .collection_index = 5u,
      .plane = SUTS_SIVD_PLANE_RNUR, .flags = SUTS_SIVD_FLAG_STYLISTIC },
    { .base_codepoint = 0x160101ULL, .variation_selector = 0x16FF00ULL,
      .resolved_glyph_index = 98u, .collection_index = 5u,
      .plane = SUTS_SIVD_PLANE_RNUR, .flags = SUTS_SIVD_FLAG_NONE },
    { .base_codepoint = 0x160200ULL, .variation_selector = 0x16FF00ULL,
      .resolved_glyph_index = 99u, .collection_index = 5u,
      .plane = SUTS_SIVD_PLANE_RNUR, .flags = SUTS_SIVD_FLAG_REGIONAL },
    { .base_codepoint = 0x160200ULL, .variation_selector = 0x16FF01ULL,
      .resolved_glyph_index = 100u, .collection_index = 5u,
      .plane = SUTS_SIVD_PLANE_RNUR, .flags = SUTS_SIVD_FLAG_REGIONAL },
};

#define SIVD_ENTRY_COUNT \
    ((size_t)(sizeof(sivd_table) / sizeof(sivd_table[0])))

/* ── The static lookup index (§4.2) ───────────────────────────────── */

typedef struct {
    uint8_t used; /* slot occupied                                   */
    uint8_t loc;  /* index into sivd_table (capacity < 256)          */
} sivd_slot_t;

static sivd_slot_t sivd_index[SUTS_SIVD_INDEX_SIZE];
static bool        sivd_ready = false;

static void sivd_build(void)
{
    size_t i;

    for (i = 0; i < SUTS_SIVD_INDEX_SIZE; i++) {
        sivd_index[i].used = 0u;
        sivd_index[i].loc = 0u;
    }
    for (i = 0; i < SIVD_ENTRY_COUNT; i++) {
        uint32_t h = sivd_pair_hash(sivd_table[i].base_codepoint,
                                    sivd_table[i].variation_selector) &
                     (SUTS_SIVD_INDEX_SIZE - 1u);
        while (sivd_index[h].used)
            h = (h + 1u) & (SUTS_SIVD_INDEX_SIZE - 1u);
        sivd_index[h].used = 1u;
        sivd_index[h].loc = (uint8_t)i;
    }
    sivd_ready = true;
}

/* ── Setup ────────────────────────────────────────────────────────── */

const char* sivd_version_string(void)
{
    return "1.0.0";
}

void sivd_init_registry(void)
{
    sivd_build();
}

/* ── Pair hash (§4.2) ─────────────────────────────────────────────── */

uint32_t sivd_pair_hash(sucs_ex_char_t base_cp, sucs_ex_char_t var_sel)
{
    uint32_t h = SUTS_SIVD_FNV_OFFSET_BASIS;
    int      i;

    for (i = 0; i < 8; i++) {
        h ^= (uint8_t)((base_cp >> (8 * i)) & 0xFFu);
        h *= SUTS_SIVD_FNV_PRIME;
    }
    for (i = 0; i < 8; i++) {
        h ^= (uint8_t)((var_sel >> (8 * i)) & 0xFFu);
        h *= SUTS_SIVD_FNV_PRIME;
    }
    return h;
}

/* ── Selector and base classification (§2.3, §4.4) ────────────────── */

bool sivd_is_variation_selector(sucs_ex_char_t cp)
{
    if (cp >= SUTS_SIVD_VS1_MIN && cp <= SUTS_SIVD_VS1_MAX)
        return true;
    if (cp >= SUTS_SIVD_VS17_MIN && cp <= SUTS_SIVD_VS17_MAX)
        return true;
    if (cp >= SIVD_RNUR_VS_MIN && cp <= SIVD_RNUR_VS_MAX)
        return true;
    return false;
}

bool sivd_is_ivs_selector(sucs_ex_char_t cp)
{
    if (cp >= SUTS_SIVD_VS17_MIN && cp <= SUTS_SIVD_VS17_MAX)
        return true;
    if (cp >= SIVD_RNUR_VS_MIN && cp <= SIVD_RNUR_VS_MAX)
        return true;
    return false;
}

bool sivd_is_valid_base(sucs_ex_char_t cp)
{
    if (cp >= SIVD_ZONE_SCP_MIN && cp <= SIVD_ZONE_SCP_MAX)
        return false; /* System Control Plane marker */
    if (cp >= SIVD_ZONE_TRAP_MIN && cp <= SIVD_ZONE_TRAP_MAX)
        return false; /* trap slot                   */
    if (cp == SIVD_ZONE_SENTINEL)
        return false; /* Sentinel                    */
    return true;
}

suts_sivd_plane_t sivd_plane_of(sucs_ex_char_t cp)
{
    if (cp <= SIVD_ZONE_UNICODE_MAX)
        return SUTS_SIVD_PLANE_BRIDGE;
    if (cp <= SIVD_ZONE_SCP_MAX)
        return SUTS_SIVD_PLANE_SCP;
    if (cp >= SIVD_ZONE_TRAP_MIN && cp <= SIVD_ZONE_TRAP_MAX)
        return SUTS_SIVD_PLANE_TRAP;
    if (cp == SIVD_ZONE_SENTINEL)
        return SUTS_SIVD_PLANE_SENTINEL;
    if (cp >= SIVD_RNUR_MIN && cp <= SIVD_RNUR_MAX)
        return SUTS_SIVD_PLANE_RNUR;
    if (cp >= SIVD_ZONE_NATIVE_MIN && cp <= SIVD_ZONE_NATIVE_LAST)
        return SUTS_SIVD_PLANE_NATIVE;
    return SUTS_SIVD_PLANE_EXTSUCS;
}

/* ── Resolution (§4.2) — O(1) expected ────────────────────────────── */

static const sivd_variation_entry_t* sivd_find(sucs_ex_char_t base_cp,
                                               sucs_ex_char_t var_sel)
{
    uint32_t h;
    uint32_t start;

    if (!sivd_ready)
        sivd_build(); /* lazy build, so lookup is safe without setup */

    h = sivd_pair_hash(base_cp, var_sel) & (SUTS_SIVD_INDEX_SIZE - 1u);
    start = h;
    for (;;) {
        const sivd_variation_entry_t* cand;
        if (!sivd_index[h].used)
            return NULL;
        cand = &sivd_table[sivd_index[h].loc];
        if (cand->base_codepoint == base_cp &&
            cand->variation_selector == var_sel)
            return cand; /* stable pointer into read-only memory */
        h = (h + 1u) & (SUTS_SIVD_INDEX_SIZE - 1u);
        if (h == start)
            return NULL;
    }
}

const sivd_variation_entry_t* sivd_lookup(sucs_ex_char_t base_cp,
                                          sucs_ex_char_t var_sel)
{
    if (!sivd_is_valid_base(base_cp) || !sivd_is_variation_selector(var_sel))
        return NULL;
    return sivd_find(base_cp, var_sel);
}

bool sivd_resolve_variant(sucs_ex_char_t base_cp, sucs_ex_char_t var_sel,
                          uint32_t* out_glyph_index)
{
    const sivd_variation_entry_t* e;

    if (!sivd_is_valid_base(base_cp) || !sivd_is_variation_selector(var_sel))
        return false;
    e = sivd_find(base_cp, var_sel);
    if (!e)
        return false;
    if (out_glyph_index)
        *out_glyph_index = e->resolved_glyph_index;
    return true;
}

bool sivd_is_registered(sucs_ex_char_t base_cp, sucs_ex_char_t var_sel)
{
    return sivd_lookup(base_cp, var_sel) != NULL;
}

const sivd_collection_t* sivd_collection_of(sucs_ex_char_t base_cp,
                                            sucs_ex_char_t var_sel)
{
    const sivd_variation_entry_t* e = sivd_lookup(base_cp, var_sel);

    if (!e || (size_t)e->collection_index >= SIVD_COLLECTION_COUNT)
        return NULL;
    return &sivd_collections[e->collection_index];
}

/* ── Enumeration (§4.1) ───────────────────────────────────────────── */

size_t sivd_entry_count(void)
{
    return SIVD_ENTRY_COUNT;
}

const sivd_variation_entry_t* sivd_entry_at(size_t index)
{
    if (index >= SIVD_ENTRY_COUNT)
        return NULL;
    return &sivd_table[index];
}

size_t sivd_collection_count(void)
{
    return SIVD_COLLECTION_COUNT;
}

const sivd_collection_t* sivd_collection_at(size_t index)
{
    if (index >= SIVD_COLLECTION_COUNT)
        return NULL;
    return &sivd_collections[index];
}
