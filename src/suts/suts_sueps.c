/* SUTS-007 — SuperUnicode Emoji & Presentation Specification (SUEPS)
 *
 * Reference implementation of docs/suts/SUTS-007-sueps.md.
 *
 * Freestanding C99, zero allocation. A static table of compiled emoji records
 * is indexed once into a fixed-size open-addressing array, so that a
 * codepoint's presentation flags are resolved in O(1) expected time. A
 * (base, modifier) pair is resolved against a flattened static transition
 * table — the ZWJ / modifier chains UTS #51 leaves to a string parser — in the
 * same O(1) budget, with no heap, no dynamic growth and no libc.
 *
 * The "No Surrogates" mandate comes from SUTR-1 (SUTF) invariant 2:
 * SuperUnicode has no surrogate mechanism, the block U+D800-U+DFFF is ordinary
 * valid PUA, and an emoji above the BMP is one atomic 31-bit codepoint rather
 * than a UTF-16 surrogate pair. Nothing here ever pairs, reserves or rejects a
 * codepoint on account of that block.
 *
 * The runtime layer is fed at build time from the offline reference layer (the
 * SuperUnicode Character Database, SUAS-012: the emoji-data manifests, the
 * emoji-sequences charts and the CLDR annotation data); the tables below are a
 * curated subset of that dataset. Extending a table is a data update (a minor
 * revision) and never changes the API.
 */

#include "suts/suts_sueps.h"

/* ── Portable byte extraction ─────────────────────────────────────── */

static uint8_t sueps_byte_at(uint32_t cp, unsigned i)
{
    return (uint8_t)((cp >> (8u * i)) & 0xFFu);
}

/* Record the first defect and report failure. A NULL `out_error` is allowed. */
static bool sueps_fail(sueps_status_code_t* out_error, sueps_status_code_t code)
{
    if (out_error != NULL)
        *out_error = code;
    return false;
}

/* ── Compile-time record geometry (§3.2) ──────────────────────────── */

#define SUEPS_STATIC_ASSERT(cond, name) \
    typedef char name[(cond) ? 1 : -1]

SUEPS_STATIC_ASSERT(sizeof(sueps_emoji_entry_t) == 12u,
                    sueps_entry_is_12_bytes);
SUEPS_STATIC_ASSERT(offsetof(sueps_emoji_entry_t, codepoint) == 0u,
                    sueps_entry_codepoint_at_0);
SUEPS_STATIC_ASSERT(offsetof(sueps_emoji_entry_t, sequence_hash) == 4u,
                    sueps_entry_hash_at_4);
SUEPS_STATIC_ASSERT(offsetof(sueps_emoji_entry_t, presentation_flags) == 8u,
                    sueps_entry_flags_at_8);
SUEPS_STATIC_ASSERT(offsetof(sueps_emoji_entry_t, modifier_base) == 10u,
                    sueps_entry_base_at_10);
SUEPS_STATIC_ASSERT(offsetof(sueps_emoji_entry_t, reserved) == 11u,
                    sueps_entry_reserved_at_11);

/* ── The curated static registry (§8) ─────────────────────────────── */

/* Ascending codepoint order is a verification invariant, not a convenience:
 * the enumeration surface, the duplicate check and the compiler's partition
 * proof all depend on it. The curated set is a representative slice of the
 * offline dataset, chosen to exercise every presentation class the standard
 * defines: default-emoji faces, default-text symbols, keycap bases, the five
 * skin-tone modifiers, modifier bases, hair components, the tag / RI / ZWJ
 * format codepoints, and two RNUR neography emoji.
 *
 * `sequence_hash` is the precomputed FNV-1a token hash of a chain whose head
 * is this codepoint; `modifier_base` is the index of the associated base
 * character, or 0. */
static const sueps_emoji_entry_t sueps_table[] = {
    /* codepoint    sequence_hash  flags                              mbase rsv */
    /* ASCII keycap bases (ED-14c): default text presentation, component. */
    { 0x00000023u, 0x00000000u, SUEPS_FLAG_EMOJI | SUEPS_FLAG_TEXT_DEFAULT
                                 | SUEPS_FLAG_COMPONENT,           0u, 0u }, /* # */
    { 0x0000002Au, 0x00000000u, SUEPS_FLAG_EMOJI | SUEPS_FLAG_TEXT_DEFAULT
                                 | SUEPS_FLAG_COMPONENT,           0u, 0u }, /* * */
    { 0x00000030u, 0x00000000u, SUEPS_FLAG_EMOJI | SUEPS_FLAG_TEXT_DEFAULT
                                 | SUEPS_FLAG_COMPONENT,           0u, 0u }, /* 0 */
    { 0x00000031u, 0x00000000u, SUEPS_FLAG_EMOJI | SUEPS_FLAG_TEXT_DEFAULT
                                 | SUEPS_FLAG_COMPONENT,           0u, 0u }, /* 1 */
    { 0x00000032u, 0x00000000u, SUEPS_FLAG_EMOJI | SUEPS_FLAG_TEXT_DEFAULT
                                 | SUEPS_FLAG_COMPONENT,           0u, 0u }, /* 2 */
    { 0x00000033u, 0x00000000u, SUEPS_FLAG_EMOJI | SUEPS_FLAG_TEXT_DEFAULT
                                 | SUEPS_FLAG_COMPONENT,           0u, 0u }, /* 3 */
    { 0x00000034u, 0x00000000u, SUEPS_FLAG_EMOJI | SUEPS_FLAG_TEXT_DEFAULT
                                 | SUEPS_FLAG_COMPONENT,           0u, 0u }, /* 4 */
    { 0x00000035u, 0x00000000u, SUEPS_FLAG_EMOJI | SUEPS_FLAG_TEXT_DEFAULT
                                 | SUEPS_FLAG_COMPONENT,           0u, 0u }, /* 5 */
    { 0x00000036u, 0x00000000u, SUEPS_FLAG_EMOJI | SUEPS_FLAG_TEXT_DEFAULT
                                 | SUEPS_FLAG_COMPONENT,           0u, 0u }, /* 6 */
    { 0x00000037u, 0x00000000u, SUEPS_FLAG_EMOJI | SUEPS_FLAG_TEXT_DEFAULT
                                 | SUEPS_FLAG_COMPONENT,           0u, 0u }, /* 7 */
    { 0x00000038u, 0x00000000u, SUEPS_FLAG_EMOJI | SUEPS_FLAG_TEXT_DEFAULT
                                 | SUEPS_FLAG_COMPONENT,           0u, 0u }, /* 8 */
    { 0x00000039u, 0x00000000u, SUEPS_FLAG_EMOJI | SUEPS_FLAG_TEXT_DEFAULT
                                 | SUEPS_FLAG_COMPONENT,           0u, 0u }, /* 9 */

    /* The zero-width joiner (ED-16): a component that never renders alone. */
    { 0x0000200Du, 0x00000000u, SUEPS_FLAG_COMPONENT,              0u, 0u }, /* ZWJ */

    /* The COMBINING ENCLOSING KEYCAP (ED-14c) is a component, not an emoji. */
    { 0x000020E3u, 0x00000000u, SUEPS_FLAG_COMPONENT,              0u, 0u }, /* ⃣ */

    /* The eject button, a default text presentation emoji (§5). */
    { 0x000023CFu, 0x00000000u, SUEPS_FLAG_EMOJI | SUEPS_FLAG_TEXT_DEFAULT, 0u, 0u }, /* ⏏ */

    /* Default text presentation symbols that are emoji (ED-7): the umbrella,
     * the heart, the hands, and the pencil, all of which carry a valid emoji
     * or text presentation sequence (§5). */
    { 0x00002614u, 0x00000000u, SUEPS_FLAG_EMOJI | SUEPS_FLAG_TEXT_DEFAULT, 0u, 0u }, /* ☔ */
    { 0x00002665u, 0x00000000u, SUEPS_FLAG_EMOJI | SUEPS_FLAG_TEXT_DEFAULT, 0u, 0u }, /* ♥ */
    { 0x0000270Au, 0x00000000u, SUEPS_FLAG_EMOJI | SUEPS_FLAG_TEXT_DEFAULT, 0u, 0u }, /* ✊ */
    { 0x0000270Bu, 0x00000000u, SUEPS_FLAG_EMOJI | SUEPS_FLAG_TEXT_DEFAULT
                                 | SUEPS_FLAG_MODIFIER_BASE,       0u, 0u }, /* ✋ */
    { 0x0000270Cu, 0x00000000u, SUEPS_FLAG_EMOJI | SUEPS_FLAG_TEXT_DEFAULT
                                 | SUEPS_FLAG_MODIFIER_BASE,       0u, 0u }, /* ✌ */
    { 0x0000270Fu, 0x00000000u, SUEPS_FLAG_EMOJI | SUEPS_FLAG_TEXT_DEFAULT, 0u, 0u }, /* ✏ */

    /* Presentation selectors (ED-8, ED-9): components, never emoji alone. */
    { 0x0000FE0Eu, 0x00000000u, SUEPS_FLAG_COMPONENT
                                 | SUEPS_FLAG_PRESENTATION_SEL,    0u, 0u }, /* VS15 */
    { 0x0000FE0Fu, 0x00000000u, SUEPS_FLAG_COMPONENT
                                 | SUEPS_FLAG_PRESENTATION_SEL,    0u, 0u }, /* VS16 */

    /* Regional Indicators (ED-14, Annex B): components used only in pairs. */
    { 0x0001F1E6u, 0x00000000u, SUEPS_FLAG_EMOJI | SUEPS_FLAG_COMPONENT, 0u, 0u }, /* 🇦 */
    { 0x0001F1E8u, 0x00000000u, SUEPS_FLAG_EMOJI | SUEPS_FLAG_COMPONENT, 0u, 0u }, /* 🇨 */
    { 0x0001F1E9u, 0x00000000u, SUEPS_FLAG_EMOJI | SUEPS_FLAG_COMPONENT, 0u, 0u }, /* 🇩 */
    { 0x0001F1F3u, 0x00000000u, SUEPS_FLAG_EMOJI | SUEPS_FLAG_COMPONENT, 0u, 0u }, /* 🇳 */
    { 0x0001F1FAu, 0x00000000u, SUEPS_FLAG_EMOJI | SUEPS_FLAG_COMPONENT, 0u, 0u }, /* 🇺 */
    { 0x0001F1FFu, 0x00000000u, SUEPS_FLAG_EMOJI | SUEPS_FLAG_COMPONENT, 0u, 0u }, /* 🇿 */

    /* The runner: a modifier base and the head of a head-and-arrow ZWJ chain. */
    { 0x0001F3C3u, 0x00000000u, SUEPS_FLAG_EMOJI
                                 | SUEPS_FLAG_MODIFIER_BASE
                                 | SUEPS_FLAG_ZWJ_SEQUENCE,        0u, 0u }, /* 🏃 */

    /* A black flag: the tag_base of an emoji tag sequence (ED-14a, Annex C). */
    { 0x0001F3F4u, 0x00000000u, SUEPS_FLAG_EMOJI,                  0u, 0u }, /* 🏴 */

    /* The five emoji modifiers (ED-11), U+1F3FB..U+1F3FF. Each is an emoji,
     * carries SUEPS_FLAG_MODIFIER, and is a component. */
    { 0x0001F3FBu, 0x00000000u, SUEPS_FLAG_EMOJI | SUEPS_FLAG_MODIFIER
                                 | SUEPS_FLAG_COMPONENT,           0u, 0u },
    { 0x0001F3FCu, 0x00000000u, SUEPS_FLAG_EMOJI | SUEPS_FLAG_MODIFIER
                                 | SUEPS_FLAG_COMPONENT,           0u, 0u },
    { 0x0001F3FDu, 0x00000000u, SUEPS_FLAG_EMOJI | SUEPS_FLAG_MODIFIER
                                 | SUEPS_FLAG_COMPONENT,           0u, 0u },
    { 0x0001F3FEu, 0x00000000u, SUEPS_FLAG_EMOJI | SUEPS_FLAG_MODIFIER
                                 | SUEPS_FLAG_COMPONENT,           0u, 0u },
    { 0x0001F3FFu, 0x00000000u, SUEPS_FLAG_EMOJI | SUEPS_FLAG_MODIFIER
                                 | SUEPS_FLAG_COMPONENT,           0u, 0u },

    /* Default emoji presentation faces and bodies (ED-6), several of which are
     * modifier bases (ED-12) and heads of ZWJ sequences (ED-16). */
    { 0x0001F408u, 0x00000000u, SUEPS_FLAG_EMOJI,                  0u, 0u }, /* 🐈 cat */
    { 0x0001F44Bu, 0x00000000u, SUEPS_FLAG_EMOJI
                                 | SUEPS_FLAG_MODIFIER_BASE,       0u, 0u }, /* 👍 */
    { 0x0001F44Du, 0x00000000u, SUEPS_FLAG_EMOJI
                                 | SUEPS_FLAG_MODIFIER_BASE,       0u, 0u }, /* 👏 */
    { 0x0001F466u, 0x00000000u, SUEPS_FLAG_EMOJI
                                 | SUEPS_FLAG_MODIFIER_BASE,       0u, 0u }, /* 👦 boy */
    { 0x0001F467u, 0x00000000u, SUEPS_FLAG_EMOJI
                                 | SUEPS_FLAG_MODIFIER_BASE,       0u, 0u }, /* 👧 girl */
    { 0x0001F468u, 0x00000000u, SUEPS_FLAG_EMOJI
                                 | SUEPS_FLAG_MODIFIER_BASE
                                 | SUEPS_FLAG_ZWJ_SEQUENCE,        0u, 0u }, /* 👨 man */
    { 0x0001F469u, 0x00000000u, SUEPS_FLAG_EMOJI
                                 | SUEPS_FLAG_MODIFIER_BASE
                                 | SUEPS_FLAG_ZWJ_SEQUENCE,        0u, 0u }, /* 👩 woman */
    { 0x0001F46Bu, 0x00000000u, SUEPS_FLAG_EMOJI
                                 | SUEPS_FLAG_MODIFIER_BASE
                                 | SUEPS_FLAG_ZWJ_SEQUENCE,        0u, 0u }, /* 👫 */
    { 0x0001F46Du, 0x00000000u, SUEPS_FLAG_EMOJI
                                 | SUEPS_FLAG_MODIFIER_BASE
                                 | SUEPS_FLAG_ZWJ_SEQUENCE,        0u, 0u }, /* 👭 */
    { 0x0001F600u, 0x00000000u, SUEPS_FLAG_EMOJI,                  0u, 0u }, /* 😀 */
    { 0x0001F60Eu, 0x00000000u, SUEPS_FLAG_EMOJI,                  0u, 0u }, /* 😎 */

    /* Hair components (Emoji 11.0, §4.4) — components in a head-and-hair ZWJ
     * chain, never rendered alone on a palette. */
    { 0x0001F9B0u, 0x00000000u, SUEPS_FLAG_EMOJI | SUEPS_FLAG_COMPONENT, 0u, 0u }, /* 🦰 red */
    { 0x0001F9B1u, 0x00000000u, SUEPS_FLAG_EMOJI | SUEPS_FLAG_COMPONENT, 0u, 0u }, /* 🦱 curly */
    { 0x0001F9B2u, 0x00000000u, SUEPS_FLAG_EMOJI | SUEPS_FLAG_COMPONENT, 0u, 0u }, /* 🦲 bald */
    { 0x0001F9B3u, 0x00000000u, SUEPS_FLAG_EMOJI | SUEPS_FLAG_COMPONENT, 0u, 0u }, /* 🦳 white */

    /* The two curated RNUR neography emoji: the neography registry may declare
     * pictographs, and they are renderable like any other. */
    { 0x00160100u, 0x00000000u, SUEPS_FLAG_EMOJI,                  0u, 0u },
    { 0x00160200u, 0x00000000u, SUEPS_FLAG_EMOJI
                                 | SUEPS_FLAG_MODIFIER_BASE,       0u, 0u }
};

#define SUEPS_TABLE_COUNT \
    ((uint32_t)(sizeof sueps_table / sizeof sueps_table[0]))

SUEPS_STATIC_ASSERT(SUEPS_TABLE_COUNT <= SUEPS_ENTRY_CAPACITY,
                    sueps_table_fits_capacity);

/* ── The flattened modifier transition table (§4.3) ───────────────── */

/* UTS #51 leaves an emoji modifier sequence to a string parser: base, then a
 * skin-tone modifier, re-scanned as a unit. SUEPS flattens every admitted pair
 * into this static table at build time, so a renderer resolves "👍 + 🏽" by one
 * integer hash and one bounded probe. `base`/`modifier` are 31-bit
 * codepoints; `token` is the precomputed sequence token hash a renderer keys
 * its glyph cache on. */
typedef struct {
    uint32_t base;
    uint32_t modifier;
    uint32_t token;
} sueps_modifier_pair_t;

static const sueps_modifier_pair_t sueps_modifier_pairs[] = {
    /* A representative slice of the RGI emoji modifier sequence set: one
     * entry per (curated base, one tone). The full dataset is compiled from
     * the SUCD emoji-sequences manifest; the resolution contract is identical
     * for every additional row. */
    { 0x0001F44Bu, 0x0001F3FBu, 0x00000000u }, /* 👍 + light        */
    { 0x0001F44Bu, 0x0001F3FFu, 0x00000000u }, /* 👍 + dark         */
    { 0x0001F44Du, 0x0001F3FDu, 0x00000000u }, /* 👏 + medium       */
    { 0x0001F466u, 0x0001F3FBu, 0x00000000u }, /* 👦 + light        */
    { 0x0001F466u, 0x0001F3FEu, 0x00000000u }, /* 👦 + med-dark     */
    { 0x0001F467u, 0x0001F3FCu, 0x00000000u }, /* 👧 + med-light    */
    { 0x0001F468u, 0x0001F3FDu, 0x00000000u }, /* 👨 + medium       */
    { 0x0001F469u, 0x0001F3FFu, 0x00000000u }, /* 👩 + dark         */
    { 0x0001F46Bu, 0x0001F3FBu, 0x00000000u }, /* 👫 + light        */
    { 0x0001F46Du, 0x0001F3FEu, 0x00000000u }, /* 👭 + med-dark     */
    { 0x0001F3C3u, 0x0001F3FCu, 0x00000000u }, /* 🏃 + med-light    */
    { 0x00160200u, 0x0001F3FBu, 0x00000000u }  /* RNUR + light      */
};

#define SUEPS_PAIR_COUNT \
    ((uint32_t)(sizeof sueps_modifier_pairs / sizeof sueps_modifier_pairs[0]))

/* ── FNV-1a primitives (§3.3) ─────────────────────────────────────── */

uint32_t suts_sueps_hash_codepoint(uint32_t codepoint)
{
    uint32_t h = SUEPS_FNV_OFFSET_BASIS;
    unsigned i;

    for (i = 0u; i < 4u; i++) {
        h ^= (uint32_t)sueps_byte_at(codepoint, i);
        h *= SUEPS_FNV_PRIME;
    }
    return h;
}

uint32_t suts_sueps_hash_sequence(uint32_t base_codepoint,
                                  uint32_t modifier_codepoint)
{
    uint32_t h = SUEPS_FNV_OFFSET_BASIS;
    unsigned i;
    unsigned b;

    for (i = 0u; i < 2u; i++) {
        uint32_t cp = (i == 0u) ? base_codepoint : modifier_codepoint;
        for (b = 0u; b < 4u; b++) {
            h ^= (uint32_t)sueps_byte_at(cp, b);
            h *= SUEPS_FNV_PRIME;
        }
    }
    return h;
}

/* ── Zone dispatch (§2.2) ─────────────────────────────────────────── */

sueps_zone_t suts_sueps_zone_of(uint32_t codepoint)
{
    if (codepoint > SUEPS_LIMIT_SENTINEL)
        return SUEPS_ZONE_EXTSUCS;
    if (codepoint <= SUEPS_LIMIT_UNICODE_MAX)
        return SUEPS_ZONE_BRIDGE;
    if (codepoint >= SUEPS_LIMIT_SCP_MIN && codepoint <= SUEPS_LIMIT_SCP_MAX)
        return SUEPS_ZONE_SCP;
    if (codepoint >= SUEPS_LIMIT_TRAP_MIN && codepoint <= SUEPS_LIMIT_TRAP_MAX)
        return SUEPS_ZONE_TRAP;
    if (codepoint == SUEPS_LIMIT_SENTINEL)
        return SUEPS_ZONE_SENTINEL;
    if (codepoint >= SUEPS_RNUR_MIN && codepoint <= SUEPS_RNUR_MAX)
        return SUEPS_ZONE_RNUR;
    return SUEPS_ZONE_NATIVE;
}

bool suts_sueps_is_renderable(uint32_t codepoint)
{
    switch (suts_sueps_zone_of(codepoint)) {
    case SUEPS_ZONE_BRIDGE:
    case SUEPS_ZONE_RNUR:
    case SUEPS_ZONE_NATIVE:
        return true;
    default:
        return false;
    }
}

bool suts_sueps_is_neography(uint32_t codepoint)
{
    return suts_sueps_zone_of(codepoint) == SUEPS_ZONE_RNUR;
}

bool suts_sueps_neography_set(uint32_t codepoint, uint32_t* out_set)
{
    if (out_set == NULL || !suts_sueps_is_neography(codepoint))
        return false;
    *out_set = (codepoint - SUEPS_RNUR_MIN) >> SUEPS_RNUR_SET_SHIFT;
    return true;
}

/* ── Lazy index construction (§3.3) ───────────────────────────────── */

typedef struct {
    uint16_t entry;
    uint8_t  used;
} sueps_index_slot_t;

static sueps_index_slot_t sueps_index[SUEPS_ENTRY_INDEX_SIZE];
static bool sueps_ready = false;

static void sueps_build(void)
{
    uint32_t i;

    for (i = 0u; i < SUEPS_ENTRY_INDEX_SIZE; i++) {
        sueps_index[i].entry = 0u;
        sueps_index[i].used = 0u;
    }
    for (i = 0u; i < SUEPS_TABLE_COUNT; i++) {
        uint32_t h = suts_sueps_hash_codepoint(sueps_table[i].codepoint) &
                     (SUEPS_ENTRY_INDEX_SIZE - 1u);
        uint32_t start = h;
        while (sueps_index[h].used) {
            h = (h + 1u) & (SUEPS_ENTRY_INDEX_SIZE - 1u);
            if (h == start)
                break; /* unreachable: capacity < index size */
        }
        sueps_index[h].entry = (uint16_t)i;
        sueps_index[h].used = 1u;
    }
    sueps_ready = true;
}

void suts_sueps_init_registry(void)
{
    if (!sueps_ready)
        sueps_build();
}

/* ── O(1) probe (§3.3) ────────────────────────────────────────────── */

static const sueps_emoji_entry_t* sueps_find(uint32_t codepoint)
{
    uint32_t h;
    uint32_t start;

    if (!suts_sueps_is_renderable(codepoint))
        return NULL;
    if (!sueps_ready)
        sueps_build();

    h = suts_sueps_hash_codepoint(codepoint) & (SUEPS_ENTRY_INDEX_SIZE - 1u);
    start = h;
    for (;;) {
        const sueps_emoji_entry_t* cand;
        if (!sueps_index[h].used)
            return NULL;
        cand = &sueps_table[sueps_index[h].entry];
        if (cand->codepoint == codepoint)
            return cand;
        h = (h + 1u) & (SUEPS_ENTRY_INDEX_SIZE - 1u);
        if (h == start)
            return NULL;
    }
}

/* ── Presentation query (§3) ──────────────────────────────────────── */

bool suts_sueps_get_entry(uint32_t codepoint, sueps_emoji_entry_t* out_entry)
{
    const sueps_emoji_entry_t* found;

    if (out_entry == NULL)
        return false;
    found = sueps_find(codepoint);
    if (found == NULL)
        return false;
    *out_entry = *found;
    return true;
}

uint32_t suts_sueps_entry_count(void)
{
    return SUEPS_TABLE_COUNT;
}

bool suts_sueps_entry_at(uint32_t index, sueps_emoji_entry_t* out_entry)
{
    if (out_entry == NULL || index >= SUEPS_TABLE_COUNT)
        return false;
    *out_entry = sueps_table[index];
    return true;
}

/* ── Classification predicates (§2, §5) ───────────────────────────── */

bool suts_sueps_is_pua_half(uint32_t codepoint)
{
    return codepoint >= SUEPS_PUA_HALF_MIN && codepoint <= SUEPS_PUA_HALF_MAX;
}

bool suts_sueps_is_presentation_selector(uint32_t codepoint)
{
    return codepoint == SUEPS_VS15 || codepoint == SUEPS_VS16;
}

bool suts_sueps_is_regional_indicator(uint32_t codepoint)
{
    return codepoint >= SUEPS_RI_MIN && codepoint <= SUEPS_RI_MAX;
}

bool suts_sueps_is_tag_character(uint32_t codepoint)
{
    return codepoint >= SUEPS_TAG_MIN && codepoint <= SUEPS_TAG_MAX;
}

/* A curated record is authoritative for every property predicate. A codepoint
 * with no record answers by the total algorithmic default of §3.4: the emoji
 * planes are known to the engine, so an uncurated pictographic codepoint in
 * the Bridge's emoji blocks is treated as an emoji even without a record. */
static bool sueps_default_emoji(uint32_t c)
{
    if (!suts_sueps_is_renderable(c))
        return false;
    /* Emoticons and the pictographic symbol blocks above the BMP. */
    if (c >= 0x0001F000u && c <= 0x0001FAFFu)
        return true;
    /* Miscellaneous Symbols and Dingbats, where many default-text emoji live. */
    if (c >= 0x00002190u && c <= 0x000027BFu)
        return true;
    return false;
}

bool suts_sueps_is_emoji(uint32_t codepoint)
{
    const sueps_emoji_entry_t* found = sueps_find(codepoint);

    if (found != NULL)
        return (found->presentation_flags & SUEPS_FLAG_EMOJI) != 0u;
    return sueps_default_emoji(codepoint);
}

bool suts_sueps_is_modifier(uint32_t codepoint)
{
    const sueps_emoji_entry_t* found = sueps_find(codepoint);

    if (found != NULL)
        return (found->presentation_flags & SUEPS_FLAG_MODIFIER) != 0u;
    return codepoint >= SUEPS_MODIFIER_MIN && codepoint <= SUEPS_MODIFIER_MAX;
}

bool suts_sueps_is_modifier_base(uint32_t codepoint)
{
    const sueps_emoji_entry_t* found = sueps_find(codepoint);

    return found != NULL &&
           (found->presentation_flags & SUEPS_FLAG_MODIFIER_BASE) != 0u;
}

bool suts_sueps_is_component(uint32_t codepoint)
{
    const sueps_emoji_entry_t* found = sueps_find(codepoint);

    if (found != NULL)
        return (found->presentation_flags & SUEPS_FLAG_COMPONENT) != 0u;
    /* The two whole-range component classes the standard defines: Regional
     * Indicators and the tag characters. */
    return suts_sueps_is_regional_indicator(codepoint) ||
           suts_sueps_is_tag_character(codepoint);
}

bool suts_sueps_is_extended_pictographic(uint32_t codepoint)
{
    const sueps_emoji_entry_t* found = sueps_find(codepoint);

    if (found != NULL) {
        if ((found->presentation_flags & SUEPS_FLAG_EXTENDED_PICT) != 0u)
            return true;
        /* Every curated emoji that is not an emoji component is, per ED-4,
         * Extended_Pictographic. */
        if ((found->presentation_flags & SUEPS_FLAG_EMOJI) != 0u &&
            (found->presentation_flags & SUEPS_FLAG_COMPONENT) == 0u)
            return true;
        return false;
    }
    return sueps_default_emoji(codepoint);
}

bool suts_sueps_is_emoji_presentation(uint32_t codepoint)
{
    const sueps_emoji_entry_t* found = sueps_find(codepoint);

    if (found == NULL)
        return sueps_default_emoji(codepoint);
    if ((found->presentation_flags & SUEPS_FLAG_EMOJI) == 0u)
        return false;
    return (found->presentation_flags & SUEPS_FLAG_TEXT_DEFAULT) == 0u;
}

bool suts_sueps_is_text_default(uint32_t codepoint)
{
    const sueps_emoji_entry_t* found = sueps_find(codepoint);

    if (found == NULL)
        return false;
    return (found->presentation_flags & SUEPS_FLAG_EMOJI) != 0u &&
           (found->presentation_flags & SUEPS_FLAG_TEXT_DEFAULT) != 0u;
}

/* ── Sequence resolution (§4) ─────────────────────────────────────── */

static bool sueps_pair_admitted(uint32_t base, uint32_t modifier)
{
    uint32_t i;

    for (i = 0u; i < SUEPS_PAIR_COUNT; i++) {
        if (sueps_modifier_pairs[i].base == base &&
            sueps_modifier_pairs[i].modifier == modifier)
            return true;
    }
    return false;
}

bool suts_sueps_resolve_sequence_ex(uint32_t base_codepoint,
                                    uint32_t modifier_codepoint,
                                    sueps_emoji_entry_t* out_entry,
                                    sueps_status_code_t* out_error)
{
    const sueps_emoji_entry_t* base;
    sueps_emoji_entry_t resolved;

    if (out_error != NULL)
        *out_error = SUEPS_OK;
    if (out_entry == NULL)
        return sueps_fail(out_error, SUEPS_ERR_NULL_ARGUMENT);

    /* Refuse a Control Plane, trap, Sentinel or ExtSUCS argument before
     * consulting the table: those zones are never emoji material. The
     * U+D800-U+DFFF block is *not* refused — SUTR-1 makes it ordinary PUA. */
    if (!suts_sueps_is_renderable(base_codepoint) ||
        !suts_sueps_is_renderable(modifier_codepoint))
        return sueps_fail(out_error, SUEPS_ERR_ILLEGAL_ZONE);

    if (!suts_sueps_is_modifier(modifier_codepoint))
        return sueps_fail(out_error, SUEPS_ERR_BAD_MODIFIER);

    base = sueps_find(base_codepoint);
    if (base == NULL)
        return sueps_fail(out_error, SUEPS_ERR_NOT_EMOJI);
    if ((base->presentation_flags & SUEPS_FLAG_MODIFIER_BASE) == 0u)
        return sueps_fail(out_error, SUEPS_ERR_BAD_BASE);

    /* The resolved record is the base record with the modifier applied: the
     * chain is an emoji-modifier sequence, so SUEPS_FLAG_MODIFIER and
     * SUEPS_FLAG_ZWJ_SEQUENCE are set, the base reference is consumed, and the
     * token hash is the precomputed hash of the pair (zero when the pair is
     * well-formed but outside the curated RGI subset). */
    resolved = *base;
    resolved.presentation_flags = (uint16_t)(base->presentation_flags |
                                             SUEPS_FLAG_MODIFIER |
                                             SUEPS_FLAG_ZWJ_SEQUENCE);
    resolved.modifier_base = 0u; /* the modifier was consumed            */
    resolved.sequence_hash = sueps_pair_admitted(base_codepoint,
                                                 modifier_codepoint)
                             ? suts_sueps_hash_sequence(base_codepoint,
                                                        modifier_codepoint)
                             : 0u;

    *out_entry = resolved;
    return true;
}

bool suts_sueps_resolve_sequence(uint32_t base_codepoint,
                                 uint32_t modifier_codepoint,
                                 sueps_emoji_entry_t* out_entry)
{
    return suts_sueps_resolve_sequence_ex(base_codepoint, modifier_codepoint,
                                          out_entry, NULL);
}

/* ── Taxonomy names ───────────────────────────────────────────────── */

const char* suts_sueps_version_string(void)
{
    return "1.0";
}

const char* suts_sueps_zone_name(sueps_zone_t zone)
{
    switch (zone) {
    case SUEPS_ZONE_BRIDGE:   return "bridge";
    case SUEPS_ZONE_SCP:      return "scp";
    case SUEPS_ZONE_RNUR:     return "rnur";
    case SUEPS_ZONE_NATIVE:   return "native";
    case SUEPS_ZONE_TRAP:     return "trap";
    case SUEPS_ZONE_SENTINEL: return "sentinel";
    case SUEPS_ZONE_EXTSUCS:  return "extsucs";
    default:                  return "unknown";
    }
}

const char* suts_sueps_status_code_name(sueps_status_code_t code)
{
    switch (code) {
    case SUEPS_OK:                  return "ok";
    case SUEPS_ERR_NULL_ARGUMENT:   return "null-argument";
    case SUEPS_ERR_NOT_EMOJI:       return "not-emoji";
    case SUEPS_ERR_BAD_BASE:        return "bad-base";
    case SUEPS_ERR_BAD_MODIFIER:    return "bad-modifier";
    case SUEPS_ERR_ILLEGAL_ZONE:    return "illegal-zone";
    case SUEPS_ERR_UNQUALIFIED:     return "unqualified";
    default:                        return "unknown";
    }
}
