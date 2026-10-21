/* SUTR-11 — SuperUnicode Mongolian Shaping & Vertical Direction (SUMS)
 *
 * Reference implementation of docs/sutr/SUTR-11-sums.md.
 *
 * Freestanding C99, zero allocation. The curated Mongolian table and the
 * flattened variation-sequence table are static; the lookup indexes are built
 * lazily once; every query is a hash and a bounded probe. No part of this file
 * reaches for malloc, stdio or a libc string routine.
 *
 * The shaping surface answers two questions a renderer asks on the hot path:
 *
 *   1. What is the contextual positional form (initial, medial, final,
 *      isolated) of this letter given its neighbours? (§3.3)
 *   2. Which standardized variant glyph does this (letter, FVS) pair select?
 *      (§4)
 *
 * Both are resolved by direct table indexing, O(1) per unit, with no sequence
 * scan and no runtime FVS parsing.
 */

#include "sutr/sutr_sums.h"

/* ── Compile-time record geometry (§3.2) ──────────────────────────── */

#define SUMS_STATIC_ASSERT(cond, name) \
    typedef char name[(cond) ? 1 : -1]

SUMS_STATIC_ASSERT(sizeof(sums_shaping_entry_t) == 12u,
                   sums_entry_is_12_bytes);
SUMS_STATIC_ASSERT(offsetof(sums_shaping_entry_t, codepoint) == 0u,
                   sums_entry_codepoint_at_0);
SUMS_STATIC_ASSERT(offsetof(sums_shaping_entry_t, vertical_advance) == 4u,
                   sums_entry_advance_at_4);
SUMS_STATIC_ASSERT(offsetof(sums_shaping_entry_t, shaping_flags) == 6u,
                   sums_entry_flags_at_6);
SUMS_STATIC_ASSERT(offsetof(sums_shaping_entry_t, positional_forms) == 8u,
                   sums_entry_forms_at_8);
SUMS_STATIC_ASSERT(offsetof(sums_shaping_entry_t, reserved) == 9u,
                   sums_entry_reserved_at_9);

SUMS_STATIC_ASSERT(sizeof(sums_variant_pair_t) == 12u,
                   sums_variant_is_12_bytes);
SUMS_STATIC_ASSERT(offsetof(sums_variant_pair_t, base) == 0u,
                   sums_variant_base_at_0);
SUMS_STATIC_ASSERT(offsetof(sums_variant_pair_t, selector) == 4u,
                   sums_variant_selector_at_4);
SUMS_STATIC_ASSERT(offsetof(sums_variant_pair_t, token) == 8u,
                   sums_variant_token_at_8);

/* ── The curated Mongolian shaping registry (§8) ──────────────────── */

/* Strictly ascending codepoint order is a verification invariant: the
 * enumeration surface and the open-addressing probe both depend on it. Each row
 * carries the shaping flags, the vertical advance in 1/64 em, and the
 * positional-form mask (bit i set when form i is distinct, §3.3). A row is
 * curated only when the character joins cursively or carries a native vertical
 * metric; an ordinary Mongolian punctuation mark is not a shaping entry. */
static const sums_shaping_entry_t sums_table[] = {
    /* codepoint    vAdv    flags                                forms rsv */
    /* ── The Free Variation Selectors (§4): joinable, but not themselves
     *    letters; they carry no positional form. ──────────────────────────── */
    { 0x0000180Bu, 0x0000, SUMS_FLAG_FVS | SUMS_FLAG_VERTICAL,     0x00u, 0u }, /* fvs1 */
    { 0x0000180Cu, 0x0000, SUMS_FLAG_FVS | SUMS_FLAG_VERTICAL,     0x00u, 0u }, /* fvs2 */
    { 0x0000180Du, 0x0000, SUMS_FLAG_FVS | SUMS_FLAG_VERTICAL,     0x00u, 0u }, /* fvs3 */
    /* ── The Mongolian vowel separator (§5): it carries the default stem but
     *    never joins. ─────────────────────────────────────────────────────── */
    { 0x0000180Eu, 0x0000, SUMS_FLAG_VERTICAL,                     0x00u, 0u }, /* ᠎ mvs */
    /* ── The underscore and hyphen: cursive-joining punctuation with no
     *    advance of its own. ──────────────────────────────────────────────── */
    { 0x00001810u, 0x0000, SUMS_FLAG_CURSIVE | SUMS_FLAG_VERTICAL, 0x00u, 0u }, /* ᠐ */
    { 0x00001811u, 0x0000, SUMS_FLAG_CURSIVE | SUMS_FLAG_VERTICAL, 0x00u, 0u }, /* ᠑ */
    /* ── Joining letters (§3.3): cursive, vertical, all four positional
     *    forms distinct. Vertical advance is the default Mongolian stem. ──── */
    { 0x00001820u, 0x0040, SUMS_FLAG_CURSIVE | SUMS_FLAG_VERTICAL
                            | SUMS_FLAG_HAS_INITIAL | SUMS_FLAG_HAS_MEDIAL
                            | SUMS_FLAG_HAS_FINAL,              0x0Fu, 0u }, /* ᠠ a */
    { 0x00001821u, 0x0040, SUMS_FLAG_CURSIVE | SUMS_FLAG_VERTICAL
                            | SUMS_FLAG_HAS_INITIAL | SUMS_FLAG_HAS_MEDIAL
                            | SUMS_FLAG_HAS_FINAL,              0x0Fu, 0u }, /* ᠡ e */
    { 0x00001822u, 0x0040, SUMS_FLAG_CURSIVE | SUMS_FLAG_VERTICAL
                            | SUMS_FLAG_HAS_INITIAL | SUMS_FLAG_HAS_MEDIAL
                            | SUMS_FLAG_HAS_FINAL,              0x0Fu, 0u }, /* ᠢ i */
    { 0x00001823u, 0x0040, SUMS_FLAG_CURSIVE | SUMS_FLAG_VERTICAL
                            | SUMS_FLAG_HAS_INITIAL | SUMS_FLAG_HAS_MEDIAL
                            | SUMS_FLAG_HAS_FINAL,              0x0Fu, 0u }, /* ᠣ o */
    { 0x00001824u, 0x0040, SUMS_FLAG_CURSIVE | SUMS_FLAG_VERTICAL
                            | SUMS_FLAG_HAS_INITIAL | SUMS_FLAG_HAS_MEDIAL
                            | SUMS_FLAG_HAS_FINAL,              0x0Fu, 0u }, /* ᠤ u */
    { 0x00001825u, 0x0040, SUMS_FLAG_CURSIVE | SUMS_FLAG_VERTICAL
                            | SUMS_FLAG_HAS_INITIAL | SUMS_FLAG_HAS_MEDIAL
                            | SUMS_FLAG_HAS_FINAL,              0x0Fu, 0u }, /* ᠥ oe */
    { 0x00001826u, 0x0040, SUMS_FLAG_CURSIVE | SUMS_FLAG_VERTICAL
                            | SUMS_FLAG_HAS_INITIAL | SUMS_FLAG_HAS_MEDIAL
                            | SUMS_FLAG_HAS_FINAL,              0x0Fu, 0u }, /* ᠦ ue */
    { 0x00001827u, 0x0040, SUMS_FLAG_CURSIVE | SUMS_FLAG_VERTICAL
                            | SUMS_FLAG_HAS_INITIAL | SUMS_FLAG_HAS_MEDIAL
                            | SUMS_FLAG_HAS_FINAL,              0x0Fu, 0u }, /* ᠧ ee */
    { 0x00001828u, 0x0048, SUMS_FLAG_CURSIVE | SUMS_FLAG_VERTICAL
                            | SUMS_FLAG_HAS_INITIAL | SUMS_FLAG_HAS_MEDIAL
                            | SUMS_FLAG_HAS_FINAL
                            | SUMS_FLAG_VARIANT_SOURCE,         0x0Fu, 0u }, /* ᠨ n */
    /* ᠩ ang: joins on both sides but has no distinct initial. */
    { 0x00001829u, 0x0048, SUMS_FLAG_CURSIVE | SUMS_FLAG_VERTICAL
                            | SUMS_FLAG_HAS_MEDIAL | SUMS_FLAG_HAS_FINAL,
                                                                0x0Du, 0u },
    { 0x0000182Au, 0x0048, SUMS_FLAG_CURSIVE | SUMS_FLAG_VERTICAL
                            | SUMS_FLAG_HAS_INITIAL | SUMS_FLAG_HAS_MEDIAL
                            | SUMS_FLAG_HAS_FINAL
                            | SUMS_FLAG_VARIANT_SOURCE,         0x0Fu, 0u }, /* ᠪ b */
    { 0x0000182Bu, 0x0048, SUMS_FLAG_CURSIVE | SUMS_FLAG_VERTICAL
                            | SUMS_FLAG_HAS_INITIAL | SUMS_FLAG_HAS_MEDIAL
                            | SUMS_FLAG_HAS_FINAL,              0x0Fu, 0u }, /* ᠫ p */
    { 0x0000182Cu, 0x0044, SUMS_FLAG_CURSIVE | SUMS_FLAG_VERTICAL
                            | SUMS_FLAG_HAS_INITIAL | SUMS_FLAG_HAS_MEDIAL
                            | SUMS_FLAG_HAS_FINAL
                            | SUMS_FLAG_VARIANT_SOURCE,         0x0Fu, 0u }, /* ᠬ q */
    { 0x0000182Du, 0x0044, SUMS_FLAG_CURSIVE | SUMS_FLAG_VERTICAL
                            | SUMS_FLAG_HAS_INITIAL | SUMS_FLAG_HAS_MEDIAL
                            | SUMS_FLAG_HAS_FINAL
                            | SUMS_FLAG_VARIANT_SOURCE,         0x0Fu, 0u }, /* ᠭ g */
    { 0x0000182Eu, 0x0044, SUMS_FLAG_CURSIVE | SUMS_FLAG_VERTICAL
                            | SUMS_FLAG_HAS_INITIAL | SUMS_FLAG_HAS_MEDIAL
                            | SUMS_FLAG_HAS_FINAL,              0x0Fu, 0u }, /* ᠮ m */
    { 0x0000182Fu, 0x0044, SUMS_FLAG_CURSIVE | SUMS_FLAG_VERTICAL
                            | SUMS_FLAG_HAS_INITIAL | SUMS_FLAG_HAS_MEDIAL
                            | SUMS_FLAG_HAS_FINAL,              0x0Fu, 0u }, /* ᠯ l */
    { 0x00001830u, 0x0040, SUMS_FLAG_CURSIVE | SUMS_FLAG_VERTICAL
                            | SUMS_FLAG_HAS_INITIAL | SUMS_FLAG_HAS_MEDIAL
                            | SUMS_FLAG_HAS_FINAL,              0x0Fu, 0u }, /* ᠰ s */
    { 0x00001831u, 0x0040, SUMS_FLAG_CURSIVE | SUMS_FLAG_VERTICAL
                            | SUMS_FLAG_HAS_INITIAL | SUMS_FLAG_HAS_MEDIAL
                            | SUMS_FLAG_HAS_FINAL,              0x0Fu, 0u }, /* ᠱ sh */
    { 0x00001832u, 0x0044, SUMS_FLAG_CURSIVE | SUMS_FLAG_VERTICAL
                            | SUMS_FLAG_HAS_INITIAL | SUMS_FLAG_HAS_MEDIAL
                            | SUMS_FLAG_HAS_FINAL,              0x0Fu, 0u }, /* ᠲ t */
    { 0x00001833u, 0x0044, SUMS_FLAG_CURSIVE | SUMS_FLAG_VERTICAL
                            | SUMS_FLAG_HAS_INITIAL | SUMS_FLAG_HAS_MEDIAL
                            | SUMS_FLAG_HAS_FINAL,              0x0Fu, 0u }, /* ᠳ d */
    { 0x00001834u, 0x0044, SUMS_FLAG_CURSIVE | SUMS_FLAG_VERTICAL
                            | SUMS_FLAG_HAS_INITIAL | SUMS_FLAG_HAS_MEDIAL
                            | SUMS_FLAG_HAS_FINAL,              0x0Fu, 0u }, /* ᠴ ch */
    { 0x00001835u, 0x0044, SUMS_FLAG_CURSIVE | SUMS_FLAG_VERTICAL
                            | SUMS_FLAG_HAS_INITIAL | SUMS_FLAG_HAS_MEDIAL
                            | SUMS_FLAG_HAS_FINAL
                            | SUMS_FLAG_VARIANT_SOURCE,         0x0Fu, 0u }, /* ᠵ j */
    { 0x00001836u, 0x0044, SUMS_FLAG_CURSIVE | SUMS_FLAG_VERTICAL
                            | SUMS_FLAG_HAS_INITIAL | SUMS_FLAG_HAS_MEDIAL
                            | SUMS_FLAG_HAS_FINAL,              0x0Fu, 0u }, /* ᠶ y */
    { 0x00001837u, 0x0044, SUMS_FLAG_CURSIVE | SUMS_FLAG_VERTICAL
                            | SUMS_FLAG_HAS_INITIAL | SUMS_FLAG_HAS_MEDIAL
                            | SUMS_FLAG_HAS_FINAL,              0x0Fu, 0u }, /* ᠷ r */
    { 0x00001838u, 0x0044, SUMS_FLAG_CURSIVE | SUMS_FLAG_VERTICAL
                            | SUMS_FLAG_HAS_INITIAL | SUMS_FLAG_HAS_MEDIAL
                            | SUMS_FLAG_HAS_FINAL,              0x0Fu, 0u }, /* ᠸ w */
    { 0x00001839u, 0x0040, SUMS_FLAG_CURSIVE | SUMS_FLAG_VERTICAL
                            | SUMS_FLAG_HAS_INITIAL | SUMS_FLAG_HAS_MEDIAL
                            | SUMS_FLAG_HAS_FINAL,              0x0Fu, 0u }, /* ᠹ f */
    { 0x0000183Au, 0x0040, SUMS_FLAG_CURSIVE | SUMS_FLAG_VERTICAL
                            | SUMS_FLAG_HAS_INITIAL | SUMS_FLAG_HAS_MEDIAL
                            | SUMS_FLAG_HAS_FINAL,              0x0Fu, 0u }, /* ᠺ k */
    { 0x0000183Bu, 0x0044, SUMS_FLAG_CURSIVE | SUMS_FLAG_VERTICAL
                            | SUMS_FLAG_HAS_INITIAL | SUMS_FLAG_HAS_MEDIAL
                            | SUMS_FLAG_HAS_FINAL,              0x0Fu, 0u }, /* ᠻ kh */
    { 0x0000183Cu, 0x0044, SUMS_FLAG_CURSIVE | SUMS_FLAG_VERTICAL
                            | SUMS_FLAG_HAS_INITIAL | SUMS_FLAG_HAS_MEDIAL
                            | SUMS_FLAG_HAS_FINAL
                            | SUMS_FLAG_VARIANT_SOURCE,         0x0Fu, 0u }, /* ᠼ ts */
    { 0x0000183Du, 0x0044, SUMS_FLAG_CURSIVE | SUMS_FLAG_VERTICAL
                            | SUMS_FLAG_HAS_INITIAL | SUMS_FLAG_HAS_MEDIAL
                            | SUMS_FLAG_HAS_FINAL,              0x0Fu, 0u }, /* ᠽ z */
    { 0x0000183Eu, 0x0040, SUMS_FLAG_CURSIVE | SUMS_FLAG_VERTICAL
                            | SUMS_FLAG_HAS_INITIAL | SUMS_FLAG_HAS_MEDIAL
                            | SUMS_FLAG_HAS_FINAL,              0x0Fu, 0u }, /* ᠾ h */
    { 0x0000183Fu, 0x0044, SUMS_FLAG_CURSIVE | SUMS_FLAG_VERTICAL
                            | SUMS_FLAG_HAS_INITIAL | SUMS_FLAG_HAS_MEDIAL
                            | SUMS_FLAG_HAS_FINAL
                            | SUMS_FLAG_VARIANT_SOURCE,         0x0Fu, 0u }, /* ᠿ zhi */
    { 0x00001840u, 0x0044, SUMS_FLAG_CURSIVE | SUMS_FLAG_VERTICAL
                            | SUMS_FLAG_HAS_INITIAL | SUMS_FLAG_HAS_MEDIAL
                            | SUMS_FLAG_HAS_FINAL,              0x0Fu, 0u }, /* ᡀ lh */
    { 0x00001841u, 0x0040, SUMS_FLAG_CURSIVE | SUMS_FLAG_VERTICAL
                            | SUMS_FLAG_HAS_INITIAL | SUMS_FLAG_HAS_MEDIAL
                            | SUMS_FLAG_HAS_FINAL,              0x0Fu, 0u }, /* ᡁ zhi */
    { 0x00001842u, 0x0040, SUMS_FLAG_CURSIVE | SUMS_FLAG_VERTICAL
                            | SUMS_FLAG_HAS_INITIAL | SUMS_FLAG_HAS_MEDIAL
                            | SUMS_FLAG_HAS_FINAL,              0x0Fu, 0u }, /* ᡂ chi */
    /* ── The narrow no-break space (§5): it carries the default stem but
     *    never joins. ─────────────────────────────────────────────────────── */
    { 0x0000202Fu, 0x0000, SUMS_FLAG_VERTICAL,                     0x00u, 0u }, /* nnbsp */
    /* ── Mongolian Supplement (§2): the Todo / Sibe / Manchu / Ali Gali
     *    repertoire that UTR #54 keeps in one chart with Hudum. ──────────── */
    { 0x00116600u, 0x0010, SUMS_FLAG_CURSIVE | SUMS_FLAG_VERTICAL
                            | SUMS_FLAG_HAS_INITIAL | SUMS_FLAG_HAS_MEDIAL
                            | SUMS_FLAG_HAS_FINAL,              0x0Fu, 0u }, /* ᡠ */
    { 0x00116601u, 0x0010, SUMS_FLAG_CURSIVE | SUMS_FLAG_VERTICAL
                            | SUMS_FLAG_HAS_INITIAL | SUMS_FLAG_HAS_MEDIAL
                            | SUMS_FLAG_HAS_FINAL,              0x0Fu, 0u }, /* ᡡ */
    { 0x00116602u, 0x0010, SUMS_FLAG_CURSIVE | SUMS_FLAG_VERTICAL
                            | SUMS_FLAG_HAS_INITIAL | SUMS_FLAG_HAS_MEDIAL
                            | SUMS_FLAG_HAS_FINAL,              0x0Fu, 0u }, /* ᡢ */
    { 0x00116603u, 0x0010, SUMS_FLAG_CURSIVE | SUMS_FLAG_VERTICAL
                            | SUMS_FLAG_HAS_INITIAL | SUMS_FLAG_HAS_MEDIAL
                            | SUMS_FLAG_HAS_FINAL,              0x0Fu, 0u }, /* ᡣ */
    { 0x00116604u, 0x0010, SUMS_FLAG_CURSIVE | SUMS_FLAG_VERTICAL
                            | SUMS_FLAG_HAS_INITIAL | SUMS_FLAG_HAS_MEDIAL
                            | SUMS_FLAG_HAS_FINAL,              0x0Fu, 0u }, /* ᡤ */
    { 0x00116605u, 0x0010, SUMS_FLAG_CURSIVE | SUMS_FLAG_VERTICAL
                            | SUMS_FLAG_HAS_INITIAL | SUMS_FLAG_HAS_MEDIAL
                            | SUMS_FLAG_HAS_FINAL,              0x0Fu, 0u }, /* ᡥ */
    { 0x00116606u, 0x0010, SUMS_FLAG_CURSIVE | SUMS_FLAG_VERTICAL
                            | SUMS_FLAG_HAS_INITIAL | SUMS_FLAG_HAS_MEDIAL
                            | SUMS_FLAG_HAS_FINAL,              0x0Fu, 0u }, /* ᡦ */
    { 0x00116607u, 0x0010, SUMS_FLAG_CURSIVE | SUMS_FLAG_VERTICAL
                            | SUMS_FLAG_HAS_INITIAL | SUMS_FLAG_HAS_MEDIAL
                            | SUMS_FLAG_HAS_FINAL,              0x0Fu, 0u }, /* ᡧ */
    { 0x00116608u, 0x0010, SUMS_FLAG_CURSIVE | SUMS_FLAG_VERTICAL
                            | SUMS_FLAG_HAS_INITIAL | SUMS_FLAG_HAS_MEDIAL
                            | SUMS_FLAG_HAS_FINAL
                            | SUMS_FLAG_FEMININE,               0x0Fu, 0u }, /* ᡨ */
    { 0x00116609u, 0x0010, SUMS_FLAG_CURSIVE | SUMS_FLAG_VERTICAL
                            | SUMS_FLAG_HAS_INITIAL | SUMS_FLAG_HAS_MEDIAL
                            | SUMS_FLAG_HAS_FINAL
                            | SUMS_FLAG_FEMININE,               0x0Fu, 0u }, /* ᡩ */
    { 0x0011660Au, 0x0010, SUMS_FLAG_CURSIVE | SUMS_FLAG_VERTICAL
                            | SUMS_FLAG_HAS_INITIAL | SUMS_FLAG_HAS_MEDIAL
                            | SUMS_FLAG_HAS_FINAL
                            | SUMS_FLAG_FEMININE,               0x0Fu, 0u }, /* ᡪ */
    { 0x0011660Bu, 0x0010, SUMS_FLAG_CURSIVE | SUMS_FLAG_VERTICAL
                            | SUMS_FLAG_HAS_INITIAL | SUMS_FLAG_HAS_MEDIAL
                            | SUMS_FLAG_HAS_FINAL
                            | SUMS_FLAG_FEMININE,               0x0Fu, 0u }, /* ᡫ */
    { 0x0011660Cu, 0x0010, SUMS_FLAG_CURSIVE | SUMS_FLAG_VERTICAL
                            | SUMS_FLAG_HAS_INITIAL | SUMS_FLAG_HAS_MEDIAL
                            | SUMS_FLAG_HAS_FINAL
                            | SUMS_FLAG_FEMININE,               0x0Fu, 0u }, /* ᡬ */
};

#define SUMS_TABLE_COUNT \
    ((uint32_t)(sizeof sums_table / sizeof sums_table[0]))

SUMS_STATIC_ASSERT(SUMS_TABLE_COUNT <= SUMS_ENTRY_CAPACITY,
                   sums_table_fits_capacity);

/* ── The flattened variation-sequence table (§4.3) ────────────────── */

/* Each row is a (letter, FVS) pair and the precompiled token its standardized
 * variation sequence selects. A renderer resolves the pair by direct pointer
 * indexing; there is no runtime sequence scan. The token is a non-zero value,
 * so a token of zero always means "no standardized variant". */
static const sums_variant_pair_t sums_variants[] = {
    /* base          selector        token   */
    { 0x0000183Cu, SUMS_FVS1, 0x00010001u }, /* ts + fvs1 */
    { 0x0000183Cu, SUMS_FVS2, 0x00010002u }, /* ts + fvs2 */
    { 0x0000183Fu, SUMS_FVS1, 0x00020001u }, /* zhi + fvs1 */
    { 0x0000183Fu, SUMS_FVS2, 0x00020002u }, /* zhi + fvs2 */
    { 0x00001835u, SUMS_FVS1, 0x00030001u }, /* j + fvs1 */
    { 0x00001835u, SUMS_FVS3, 0x00030003u }, /* j + fvs3 */
    { 0x0000182Cu, SUMS_FVS1, 0x00040001u }, /* q + fvs1 */
    { 0x0000182Du, SUMS_FVS1, 0x00050001u }, /* g + fvs1 */
    { 0x00001828u, SUMS_FVS1, 0x00060001u }, /* n + fvs1 */
    { 0x0000182Au, SUMS_FVS1, 0x00070001u }, /* b + fvs1 */
};

#define SUMS_VARIANT_COUNT \
    ((uint32_t)(sizeof sums_variants / sizeof sums_variants[0]))

SUMS_STATIC_ASSERT(SUMS_VARIANT_COUNT <= SUMS_VARIANT_CAPACITY,
                   sums_variants_fit_capacity);

/* ── Portable byte extraction ─────────────────────────────────────── */

static uint8_t sums_byte_at(uint32_t cp, unsigned i)
{
    return (uint8_t)((cp >> (8u * i)) & 0xFFu);
}

/* Record the first defect and report failure. A NULL `out_error` is allowed. */
static bool sums_fail(sums_status_code_t* out_error, sums_status_code_t code)
{
    if (out_error != NULL)
        *out_error = code;
    return false;
}

/* ── Zone dispatch (§2.3) ─────────────────────────────────────────── */

sums_zone_t sutr_sums_zone_of(uint32_t codepoint)
{
    if (codepoint > SUMS_LIMIT_SENTINEL)
        return SUMS_ZONE_EXTSUCS;
    if (codepoint <= SUMS_LIMIT_UNICODE_MAX)
        return SUMS_ZONE_BRIDGE;
    /* The Mongolian Supplement (U+11660..U+1167F) is not a Plugin Bridge
     * block: it sits numerically inside the SCP range but is ordinary
     * renderable Mongolian text, so it is carved out before the SCP test. */
    if (codepoint >= SUMS_BLOCK_SUPPLEMENT &&
        codepoint <= SUMS_BLOCK_SUPPLEMENT_MAX)
        return SUMS_ZONE_BRIDGE;
    if (codepoint >= SUMS_LIMIT_SCP_MIN && codepoint <= SUMS_LIMIT_SCP_MAX)
        return SUMS_ZONE_SCP;
    if (codepoint >= SUMS_LIMIT_TRAP_MIN && codepoint <= SUMS_LIMIT_TRAP_MAX)
        return SUMS_ZONE_TRAP;
    if (codepoint == SUMS_LIMIT_SENTINEL)
        return SUMS_ZONE_SENTINEL;
    if (codepoint >= SUMS_RNUR_MIN && codepoint <= SUMS_RNUR_MAX)
        return SUMS_ZONE_RNUR;
    return SUMS_ZONE_NATIVE;
}

bool sutr_sums_is_renderable(uint32_t codepoint)
{
    sums_zone_t z = sutr_sums_zone_of(codepoint);

    return z == SUMS_ZONE_BRIDGE || z == SUMS_ZONE_RNUR ||
           z == SUMS_ZONE_NATIVE;
}

bool sutr_sums_is_text_space(uint32_t codepoint)
{
    return sutr_sums_is_renderable(codepoint);
}

bool sutr_sums_is_neography(uint32_t codepoint)
{
    return sutr_sums_zone_of(codepoint) == SUMS_ZONE_RNUR;
}

bool sutr_sums_neography_set(uint32_t codepoint, uint32_t* out_set)
{
    if (out_set == NULL)
        return false;
    if (!sutr_sums_is_neography(codepoint))
        return false;
    *out_set = (codepoint - SUMS_RNUR_MIN) >> SUMS_RNUR_SET_SHIFT;
    return true;
}

/* ── FNV-1a primitives (§3.3) ─────────────────────────────────────── */

uint32_t sutr_sums_hash_codepoint(uint32_t codepoint)
{
    uint32_t h = SUMS_FNV_OFFSET_BASIS;
    unsigned i;

    for (i = 0u; i < 4u; i++) {
        h ^= (uint32_t)sums_byte_at(codepoint, i);
        h *= SUMS_FNV_PRIME;
    }
    return h;
}

uint32_t sutr_sums_hash_pair(uint32_t base_codepoint,
                             uint32_t selector_codepoint)
{
    uint32_t h = SUMS_FNV_OFFSET_BASIS;
    unsigned i;
    unsigned b;

    for (i = 0u; i < 2u; i++) {
        uint32_t cp = (i == 0u) ? base_codepoint : selector_codepoint;
        for (b = 0u; b < 4u; b++) {
            h ^= (uint32_t)sums_byte_at(cp, b);
            h *= SUMS_FNV_PRIME;
        }
    }
    return h;
}

/* ── Lazy index construction (§3.3) ───────────────────────────────── */

typedef struct {
    uint16_t entry;
    uint8_t  used;
} sums_index_slot_t;

static sums_index_slot_t sums_index[SUMS_ENTRY_INDEX_SIZE];
static sums_index_slot_t sums_variant_index[SUMS_VARIANT_INDEX_SIZE];
static bool sums_ready = false;

static void sums_build(void)
{
    uint32_t i;

    for (i = 0u; i < SUMS_ENTRY_INDEX_SIZE; i++) {
        sums_index[i].entry = 0u;
        sums_index[i].used = 0u;
    }
    for (i = 0u; i < SUMS_ENTRY_INDEX_SIZE; i++) {
        sums_variant_index[i].entry = 0u;
        sums_variant_index[i].used = 0u;
    }

    for (i = 0u; i < SUMS_TABLE_COUNT; i++) {
        uint32_t h = sutr_sums_hash_codepoint(sums_table[i].codepoint) &
                     (SUMS_ENTRY_INDEX_SIZE - 1u);
        uint32_t start = h;
        while (sums_index[h].used) {
            h = (h + 1u) & (SUMS_ENTRY_INDEX_SIZE - 1u);
            if (h == start)
                break; /* unreachable: capacity < index size */
        }
        sums_index[h].entry = (uint16_t)i;
        sums_index[h].used = 1u;
    }

    for (i = 0u; i < SUMS_VARIANT_COUNT; i++) {
        uint32_t h = sutr_sums_hash_pair(sums_variants[i].base,
                                         sums_variants[i].selector) &
                     (SUMS_VARIANT_INDEX_SIZE - 1u);
        uint32_t start = h;
        while (sums_variant_index[h].used) {
            h = (h + 1u) & (SUMS_VARIANT_INDEX_SIZE - 1u);
            if (h == start)
                break;
        }
        sums_variant_index[h].entry = (uint16_t)i;
        sums_variant_index[h].used = 1u;
    }

    sums_ready = true;
}

void sutr_sums_init_registry(void)
{
    if (!sums_ready)
        sums_build();
}

/* ── O(1) probes (§3.3) ───────────────────────────────────────────── */

static const sums_shaping_entry_t* sums_find(uint32_t codepoint)
{
    uint32_t h;
    uint32_t start;

    if (!sutr_sums_is_renderable(codepoint))
        return NULL;
    if (!sums_ready)
        sums_build();

    h = sutr_sums_hash_codepoint(codepoint) & (SUMS_ENTRY_INDEX_SIZE - 1u);
    start = h;
    for (;;) {
        const sums_shaping_entry_t* cand;
        if (!sums_index[h].used)
            return NULL;
        cand = &sums_table[sums_index[h].entry];
        if (cand->codepoint == codepoint)
            return cand;
        h = (h + 1u) & (SUMS_ENTRY_INDEX_SIZE - 1u);
        if (h == start)
            return NULL;
    }
}

static const sums_variant_pair_t* sums_find_variant(uint32_t base,
                                                    uint32_t selector)
{
    uint32_t h;
    uint32_t start;

    if (!sums_ready)
        sums_build();

    h = sutr_sums_hash_pair(base, selector) &
        (SUMS_VARIANT_INDEX_SIZE - 1u);
    start = h;
    for (;;) {
        const sums_variant_pair_t* cand;
        if (!sums_variant_index[h].used)
            return NULL;
        cand = &sums_variants[sums_variant_index[h].entry];
        if (cand->base == base && cand->selector == selector)
            return cand;
        h = (h + 1u) & (SUMS_VARIANT_INDEX_SIZE - 1u);
        if (h == start)
            return NULL;
    }
}

/* ── The algorithmic default (§6) ─────────────────────────────────── */

/* True when an uncurated codepoint is Mongolian by its block: the Mongolian
 * block and Supplement, plus the RNUR neography carve-out. A codepoint outside
 * these is an ordinary starter with no Mongolian shaping. */
static bool sums_is_default_mongolian(uint32_t cp)
{
    if (cp >= SUMS_BLOCK_MONGOLIAN && cp <= SUMS_BLOCK_MONGOLIAN_MAX)
        return true;
    if (cp >= SUMS_BLOCK_SUPPLEMENT && cp <= SUMS_BLOCK_SUPPLEMENT_MAX)
        return true;
    if (cp >= SUMS_RNUR_MIN && cp <= SUMS_RNUR_MAX)
        return true;
    return false;
}

/* A Mongolian codepoint with no curated row is not unknown: its vertical
 * advance falls to the default stem, and it does not join. The synthesized
 * record carries the derived flags and a modest advance. */
static sums_shaping_entry_t sums_default_record(uint32_t codepoint)
{
    sums_shaping_entry_t rec;

    rec.codepoint = codepoint;
    rec.vertical_advance = (int16_t)0x0040;
    rec.shaping_flags = SUMS_FLAG_VERTICAL;
    rec.positional_forms = 0u;
    rec.reserved = 0u;
    return rec;
}

/* ── Shaping query (§3) ───────────────────────────────────────────── */

bool sutr_sums_get_entry(uint32_t codepoint, sums_shaping_entry_t* out_entry)
{
    const sums_shaping_entry_t* found;

    if (out_entry == NULL)
        return false;
    if (!sutr_sums_is_renderable(codepoint))
        return false;

    found = sums_find(codepoint);
    if (found != NULL) {
        *out_entry = *found;
        return true;
    }
    if (!sums_is_default_mongolian(codepoint))
        return false; /* an ordinary starter has no Mongolian shaping */
    *out_entry = sums_default_record(codepoint);
    return true;
}

/* ── Classification predicates (§2, §5) ──────────────────────────── */

bool sutr_sums_is_registered(uint32_t codepoint)
{
    return sums_find(codepoint) != NULL;
}

bool sutr_sums_is_mongolian(uint32_t codepoint)
{
    const sums_shaping_entry_t* found = sums_find(codepoint);

    if (found != NULL)
        return true;
    return sutr_sums_is_renderable(codepoint) &&
           sums_is_default_mongolian(codepoint);
}

bool sutr_sums_is_cursive(uint32_t codepoint)
{
    const sums_shaping_entry_t* found = sums_find(codepoint);

    if (found != NULL)
        return (found->shaping_flags & SUMS_FLAG_CURSIVE) != 0u;
    return false;
}

bool sutr_sums_is_vertical(uint32_t codepoint)
{
    const sums_shaping_entry_t* found = sums_find(codepoint);

    if (found != NULL)
        return (found->shaping_flags & SUMS_FLAG_VERTICAL) != 0u;
    return sutr_sums_is_renderable(codepoint) &&
           sums_is_default_mongolian(codepoint);
}

bool sutr_sums_is_fvs(uint32_t codepoint)
{
    const sums_shaping_entry_t* found = sums_find(codepoint);

    if (found != NULL)
        return (found->shaping_flags & SUMS_FLAG_FVS) != 0u;
    return codepoint >= SUMS_FVS_MIN && codepoint <= SUMS_FVS_MAX;
}

bool sutr_sums_is_variant_source(uint32_t codepoint)
{
    const sums_shaping_entry_t* found = sums_find(codepoint);

    if (found != NULL)
        return (found->shaping_flags & SUMS_FLAG_VARIANT_SOURCE) != 0u;
    return false;
}

bool sutr_sums_is_feminine(uint32_t codepoint)
{
    const sums_shaping_entry_t* found = sums_find(codepoint);

    if (found != NULL)
        return (found->shaping_flags & SUMS_FLAG_FEMININE) != 0u;
    return false;
}

bool sutr_sums_has_form(uint32_t codepoint, uint8_t form)
{
    const sums_shaping_entry_t* found = sums_find(codepoint);

    if (found == NULL || form > SUMS_FORM_FINAL)
        return false;
    return (found->positional_forms & (uint8_t)(1u << form)) != 0u;
}

/* ── Contextual positional shaping (§3.3) ────────────────────────── */

/* Does `cp` join *forward*, i.e. can it be the left member of a cursive pair?
 * A Mongolian letter joins forward when it has an initial or medial form; FVS
 * selectors and the joining punctuation join forward as well. */
static bool sums_joins_forward(uint32_t cp)
{
    const sums_shaping_entry_t* found = sums_find(cp);

    if (found == NULL)
        return false;
    if ((found->shaping_flags & SUMS_FLAG_FVS) != 0u)
        return true;
    return (found->positional_forms &
            ((uint8_t)(1u << SUMS_FORM_INITIAL) |
             (uint8_t)(1u << SUMS_FORM_MEDIAL))) != 0u;
}

/* Does `cp` join *backward*, i.e. can it be the right member of a cursive pair?
 * A Mongolian letter joins backward when it has a final or medial form. */
static bool sums_joins_backward(uint32_t cp)
{
    const sums_shaping_entry_t* found = sums_find(cp);

    if (found == NULL)
        return false;
    if ((found->shaping_flags & SUMS_FLAG_FVS) != 0u)
        return true;
    return (found->positional_forms &
            ((uint8_t)(1u << SUMS_FORM_FINAL) |
             (uint8_t)(1u << SUMS_FORM_MEDIAL))) != 0u;
}

uint8_t sutr_sums_positional_form(uint32_t prev_codepoint,
                                  uint32_t cur_codepoint,
                                  uint32_t next_codepoint)
{
    const sums_shaping_entry_t* cur = sums_find(cur_codepoint);
    bool join_prev;
    bool join_next;

    if (cur == NULL)
        return SUMS_FORM_ISOLATED;
    /* A variation selector never takes a positional form of its own. */
    if ((cur->shaping_flags & SUMS_FLAG_FVS) != 0u)
        return SUMS_FORM_ISOLATED;

    /* A neighbour that is absent (0) or in an inadmissible zone never joins. */
    join_prev = prev_codepoint != 0u &&
                sutr_sums_is_renderable(prev_codepoint) &&
                sums_joins_forward(prev_codepoint);
    join_next = next_codepoint != 0u &&
                sutr_sums_is_renderable(next_codepoint) &&
                sums_joins_backward(next_codepoint);

    if (join_prev && join_next)
        return SUMS_FORM_MEDIAL;
    if (join_prev)
        return SUMS_FORM_FINAL;
    if (join_next)
        return SUMS_FORM_INITIAL;
    return SUMS_FORM_ISOLATED;
}

const char* sutr_sums_form_name(uint8_t form)
{
    switch (form) {
    case SUMS_FORM_ISOLATED: return "isolated";
    case SUMS_FORM_INITIAL:  return "initial";
    case SUMS_FORM_MEDIAL:   return "medial";
    case SUMS_FORM_FINAL:    return "final";
    default:                 return "unknown";
    }
}

/* ── Vertical metrics (§5) ────────────────────────────────────────── */

int16_t sutr_sums_vertical_advance(uint32_t codepoint)
{
    const sums_shaping_entry_t* found;

    if (!sutr_sums_is_renderable(codepoint))
        return 0;
    found = sums_find(codepoint);
    if (found != NULL)
        return found->vertical_advance;
    if (!sums_is_default_mongolian(codepoint))
        return 0;
    return sums_default_record(codepoint).vertical_advance;
}

/* ── Variation-sequence resolution (§4) ──────────────────────────── */

uint32_t sutr_sums_variant_token(uint32_t base_codepoint,
                                 uint32_t selector_codepoint)
{
    const sums_variant_pair_t* pair = sums_find_variant(base_codepoint,
                                                       selector_codepoint);

    if (pair == NULL)
        return 0u;
    return pair->token;
}

bool sutr_sums_resolve_variant_ex(uint32_t base_codepoint,
                                  uint32_t selector_codepoint,
                                  sums_shaping_entry_t* out_entry,
                                  sums_status_code_t* out_error)
{
    const sums_shaping_entry_t* base;
    const sums_variant_pair_t* pair;
    sums_shaping_entry_t resolved;

    if (out_error != NULL)
        *out_error = SUMS_OK;
    if (out_entry == NULL)
        return sums_fail(out_error, SUMS_ERR_NULL_ARGUMENT);

    if (!sutr_sums_is_renderable(base_codepoint) ||
        !sutr_sums_is_renderable(selector_codepoint))
        return sums_fail(out_error, SUMS_ERR_ILLEGAL_ZONE);

    base = sums_find(base_codepoint);
    if (base == NULL)
        return sums_fail(out_error, SUMS_ERR_NOT_MONGOLIAN);

    if (!sutr_sums_is_fvs(selector_codepoint))
        return sums_fail(out_error, SUMS_ERR_BAD_SELECTOR);

    pair = sums_find_variant(base_codepoint, selector_codepoint);
    if (pair == NULL)
        return sums_fail(out_error, SUMS_ERR_NO_VARIANT);

    /* The variant record is the base's record with the variant token folded
     * into the codepoint, so a downstream renderer treats it uniformly. */
    resolved = *base;
    resolved.codepoint = pair->token;
    *out_entry = resolved;
    return true;
}

bool sutr_sums_resolve_variant(uint32_t base_codepoint,
                               uint32_t selector_codepoint,
                               sums_shaping_entry_t* out_entry)
{
    return sutr_sums_resolve_variant_ex(base_codepoint, selector_codepoint,
                                        out_entry, NULL);
}

/* ── Enumeration (§3.3) ───────────────────────────────────────────── */

uint32_t sutr_sums_entry_count(void)
{
    return SUMS_TABLE_COUNT;
}

bool sutr_sums_entry_at(uint32_t index, sums_shaping_entry_t* out_entry)
{
    if (out_entry == NULL || index >= SUMS_TABLE_COUNT)
        return false;
    *out_entry = sums_table[index];
    return true;
}

uint32_t sutr_sums_variant_count(void)
{
    return SUMS_VARIANT_COUNT;
}

bool sutr_sums_variant_at(uint32_t index, sums_variant_pair_t* out_pair)
{
    if (out_pair == NULL || index >= SUMS_VARIANT_COUNT)
        return false;
    *out_pair = sums_variants[index];
    return true;
}

/* ── Version ──────────────────────────────────────────────────────── */

const char* sutr_sums_version_string(void)
{
    return "0.1.0";
}

/* ── Taxonomy names ───────────────────────────────────────────────── */

const char* sutr_sums_zone_name(sums_zone_t zone)
{
    switch (zone) {
    case SUMS_ZONE_BRIDGE:   return "bridge";
    case SUMS_ZONE_SCP:      return "scp";
    case SUMS_ZONE_RNUR:     return "rnur";
    case SUMS_ZONE_NATIVE:   return "native";
    case SUMS_ZONE_TRAP:     return "trap";
    case SUMS_ZONE_SENTINEL: return "sentinel";
    case SUMS_ZONE_EXTSUCS:  return "extsucs";
    default:                 return "unknown";
    }
}

const char* sutr_sums_status_code_name(sums_status_code_t code)
{
    switch (code) {
    case SUMS_OK:                  return "ok";
    case SUMS_ERR_NULL_ARGUMENT:   return "null-argument";
    case SUMS_ERR_NOT_MONGOLIAN:   return "not-mongolian";
    case SUMS_ERR_BAD_SELECTOR:    return "bad-selector";
    case SUMS_ERR_ILLEGAL_ZONE:    return "illegal-zone";
    case SUMS_ERR_NO_VARIANT:      return "no-variant";
    default:                       return "unknown";
    }
}
