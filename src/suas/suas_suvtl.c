/* SUAS-014 — SuperUnicode Vertical Text Layout (SUVTL)
 *
 * Reference implementation of docs/suas/SUAS-014-suvtl.md.
 *
 * Freestanding C99, zero allocation. A static table of curated orientation
 * records is indexed once into a fixed-size open-addressing array, so that the
 * vertical orientation, the derived upright/rotated/transformed predicates and
 * the baseline adjustment of a 31-bit SuperUnicode codepoint are all resolved
 * in O(1) expected time. No heap, no dynamic table growth, no text parsing.
 *
 * The runtime layer is fed at build time from the offline reference layer (the
 * SuperUnicode Character Database, SUAS-012: the `VerticalOrientation.txt`
 * manifests and the per-script vertical-flow definitions); the table below is
 * the full UAX #50 Table 3 Tu/Tr set plus the Native/neography declarations.
 * Extending the table is a data update (a minor revision) and never changes the
 * API. A codepoint with no curated record is still answered, by the algorithmic
 * default of §5 — the property is total.
 */

#include "suas/suas_suvtl.h"

/* ── Portable byte extraction ─────────────────────────────────────── */

static uint8_t suvtl_byte_at(uint32_t cp, unsigned i)
{
    return (uint8_t)((cp >> (8u * i)) & 0xFFu);
}

/* ── Compile-time record geometry (§3.1, §3.3) ────────────────────── */

#define SUVTL_STATIC_ASSERT(cond, name) \
    typedef char name[(cond) ? 1 : -1]

SUVTL_STATIC_ASSERT(sizeof(suvtl_glyph_layout_t) == 8u,
                    suvtl_layout_is_8_bytes);
SUVTL_STATIC_ASSERT(offsetof(suvtl_glyph_layout_t, codepoint) == 0u,
                    suvtl_layout_codepoint_at_0);
SUVTL_STATIC_ASSERT(offsetof(suvtl_glyph_layout_t, orientation_flags) == 4u,
                    suvtl_layout_flags_at_4);
SUVTL_STATIC_ASSERT(offsetof(suvtl_glyph_layout_t, baseline_offset) == 6u,
                    suvtl_layout_baseline_at_6);
SUVTL_STATIC_ASSERT(offsetof(suvtl_glyph_layout_t, reserved) == 7u,
                    suvtl_layout_reserved_at_7);

/* ── The curated static registry (§8) ─────────────────────────────── */

/* Strictly ascending codepoint order is a verification invariant, not a
 * convenience: the enumeration surface, the duplicate check and the compiler's
 * partition proof all depend on it. Every row carries the orientation value in
 * bits 0-2 of `orientation_flags` (UPRIGHT / ROTATED / TRANSFORM), the
 * orthogonal SQUARED / QUOTATION / CUSTOM_FLOW flags above it, and a signed
 * `baseline_offset` in 1/16 em that is zero except for the curated neography
 * examples that exercise the field. */
static const suvtl_glyph_layout_t suvtl_table[] = {
    { 0x00002018u, 0x0026u, 0x00u, 0u },
    { 0x00002019u, 0x0026u, 0x00u, 0u },
    { 0x0000201Cu, 0x0026u, 0x00u, 0u },
    { 0x0000201Du, 0x0026u, 0x00u, 0u },
    { 0x00002329u, 0x0006u, 0x00u, 0u },
    { 0x0000232Au, 0x0006u, 0x00u, 0u },
    { 0x00003001u, 0x0006u, 0x00u, 0u },
    { 0x00003002u, 0x0006u, 0x00u, 0u },
    { 0x00003008u, 0x0006u, 0x00u, 0u },
    { 0x00003009u, 0x0006u, 0x00u, 0u },
    { 0x0000300Au, 0x0006u, 0x00u, 0u },
    { 0x0000300Bu, 0x0006u, 0x00u, 0u },
    { 0x0000300Cu, 0x0006u, 0x00u, 0u },
    { 0x0000300Du, 0x0006u, 0x00u, 0u },
    { 0x0000300Eu, 0x0006u, 0x00u, 0u },
    { 0x0000300Fu, 0x0006u, 0x00u, 0u },
    { 0x00003010u, 0x0006u, 0x00u, 0u },
    { 0x00003011u, 0x0006u, 0x00u, 0u },
    { 0x00003014u, 0x0006u, 0x00u, 0u },
    { 0x00003015u, 0x0006u, 0x00u, 0u },
    { 0x00003016u, 0x0006u, 0x00u, 0u },
    { 0x00003017u, 0x0006u, 0x00u, 0u },
    { 0x00003018u, 0x0006u, 0x00u, 0u },
    { 0x00003019u, 0x0006u, 0x00u, 0u },
    { 0x0000301Au, 0x0006u, 0x00u, 0u },
    { 0x0000301Bu, 0x0006u, 0x00u, 0u },
    { 0x0000301Cu, 0x0006u, 0x00u, 0u },
    { 0x0000301Du, 0x0006u, 0x00u, 0u },
    { 0x0000301Eu, 0x0006u, 0x00u, 0u },
    { 0x0000301Fu, 0x0006u, 0x00u, 0u },
    { 0x00003030u, 0x0006u, 0x00u, 0u },
    { 0x00003041u, 0x0006u, 0x00u, 0u },
    { 0x00003043u, 0x0006u, 0x00u, 0u },
    { 0x00003045u, 0x0006u, 0x00u, 0u },
    { 0x00003047u, 0x0006u, 0x00u, 0u },
    { 0x00003049u, 0x0006u, 0x00u, 0u },
    { 0x00003063u, 0x0006u, 0x00u, 0u },
    { 0x00003083u, 0x0006u, 0x00u, 0u },
    { 0x00003085u, 0x0006u, 0x00u, 0u },
    { 0x00003087u, 0x0006u, 0x00u, 0u },
    { 0x0000308Eu, 0x0006u, 0x00u, 0u },
    { 0x00003095u, 0x0006u, 0x00u, 0u },
    { 0x00003096u, 0x0006u, 0x00u, 0u },
    { 0x0000309Bu, 0x0006u, 0x00u, 0u },
    { 0x0000309Cu, 0x0006u, 0x00u, 0u },
    { 0x000030A0u, 0x0006u, 0x00u, 0u },
    { 0x000030A1u, 0x0006u, 0x00u, 0u },
    { 0x000030A3u, 0x0006u, 0x00u, 0u },
    { 0x000030A5u, 0x0006u, 0x00u, 0u },
    { 0x000030A7u, 0x0006u, 0x00u, 0u },
    { 0x000030A9u, 0x0006u, 0x00u, 0u },
    { 0x000030C3u, 0x0006u, 0x00u, 0u },
    { 0x000030E3u, 0x0006u, 0x00u, 0u },
    { 0x000030E5u, 0x0006u, 0x00u, 0u },
    { 0x000030E7u, 0x0006u, 0x00u, 0u },
    { 0x000030EEu, 0x0006u, 0x00u, 0u },
    { 0x000030F5u, 0x0006u, 0x00u, 0u },
    { 0x000030F6u, 0x0006u, 0x00u, 0u },
    { 0x000030FCu, 0x0006u, 0x00u, 0u },
    { 0x00003127u, 0x0006u, 0x00u, 0u },
    { 0x000031B4u, 0x0006u, 0x00u, 0u },
    { 0x000031B5u, 0x0006u, 0x00u, 0u },
    { 0x000031B6u, 0x0006u, 0x00u, 0u },
    { 0x000031B7u, 0x0006u, 0x00u, 0u },
    { 0x000031BBu, 0x0006u, 0x00u, 0u },
    { 0x000031F0u, 0x0006u, 0x00u, 0u },
    { 0x000031F1u, 0x0006u, 0x00u, 0u },
    { 0x000031F2u, 0x0006u, 0x00u, 0u },
    { 0x000031F3u, 0x0006u, 0x00u, 0u },
    { 0x000031F4u, 0x0006u, 0x00u, 0u },
    { 0x000031F5u, 0x0006u, 0x00u, 0u },
    { 0x000031F6u, 0x0006u, 0x00u, 0u },
    { 0x000031F7u, 0x0006u, 0x00u, 0u },
    { 0x000031F8u, 0x0006u, 0x00u, 0u },
    { 0x000031F9u, 0x0006u, 0x00u, 0u },
    { 0x000031FAu, 0x0006u, 0x00u, 0u },
    { 0x000031FBu, 0x0006u, 0x00u, 0u },
    { 0x000031FCu, 0x0006u, 0x00u, 0u },
    { 0x000031FDu, 0x0006u, 0x00u, 0u },
    { 0x000031FEu, 0x0006u, 0x00u, 0u },
    { 0x000031FFu, 0x0006u, 0x00u, 0u },
    { 0x000032FFu, 0x0015u, 0x00u, 0u },
    { 0x00003300u, 0x0015u, 0x00u, 0u },
    { 0x00003301u, 0x0015u, 0x00u, 0u },
    { 0x00003302u, 0x0015u, 0x00u, 0u },
    { 0x00003303u, 0x0015u, 0x00u, 0u },
    { 0x00003304u, 0x0015u, 0x00u, 0u },
    { 0x00003305u, 0x0015u, 0x00u, 0u },
    { 0x00003306u, 0x0015u, 0x00u, 0u },
    { 0x00003307u, 0x0015u, 0x00u, 0u },
    { 0x00003308u, 0x0015u, 0x00u, 0u },
    { 0x00003309u, 0x0015u, 0x00u, 0u },
    { 0x0000330Au, 0x0015u, 0x00u, 0u },
    { 0x0000330Bu, 0x0015u, 0x00u, 0u },
    { 0x0000330Cu, 0x0015u, 0x00u, 0u },
    { 0x0000330Du, 0x0015u, 0x00u, 0u },
    { 0x0000330Eu, 0x0015u, 0x00u, 0u },
    { 0x0000330Fu, 0x0015u, 0x00u, 0u },
    { 0x00003310u, 0x0015u, 0x00u, 0u },
    { 0x00003311u, 0x0015u, 0x00u, 0u },
    { 0x00003312u, 0x0015u, 0x00u, 0u },
    { 0x00003313u, 0x0015u, 0x00u, 0u },
    { 0x00003314u, 0x0015u, 0x00u, 0u },
    { 0x00003315u, 0x0015u, 0x00u, 0u },
    { 0x00003316u, 0x0015u, 0x00u, 0u },
    { 0x00003317u, 0x0015u, 0x00u, 0u },
    { 0x00003318u, 0x0015u, 0x00u, 0u },
    { 0x00003319u, 0x0015u, 0x00u, 0u },
    { 0x0000331Au, 0x0015u, 0x00u, 0u },
    { 0x0000331Bu, 0x0015u, 0x00u, 0u },
    { 0x0000331Cu, 0x0015u, 0x00u, 0u },
    { 0x0000331Du, 0x0015u, 0x00u, 0u },
    { 0x0000331Eu, 0x0015u, 0x00u, 0u },
    { 0x0000331Fu, 0x0015u, 0x00u, 0u },
    { 0x00003320u, 0x0015u, 0x00u, 0u },
    { 0x00003321u, 0x0015u, 0x00u, 0u },
    { 0x00003322u, 0x0015u, 0x00u, 0u },
    { 0x00003323u, 0x0015u, 0x00u, 0u },
    { 0x00003324u, 0x0015u, 0x00u, 0u },
    { 0x00003325u, 0x0015u, 0x00u, 0u },
    { 0x00003326u, 0x0015u, 0x00u, 0u },
    { 0x00003327u, 0x0015u, 0x00u, 0u },
    { 0x00003328u, 0x0015u, 0x00u, 0u },
    { 0x00003329u, 0x0015u, 0x00u, 0u },
    { 0x0000332Au, 0x0015u, 0x00u, 0u },
    { 0x0000332Bu, 0x0015u, 0x00u, 0u },
    { 0x0000332Cu, 0x0015u, 0x00u, 0u },
    { 0x0000332Du, 0x0015u, 0x00u, 0u },
    { 0x0000332Eu, 0x0015u, 0x00u, 0u },
    { 0x0000332Fu, 0x0015u, 0x00u, 0u },
    { 0x00003330u, 0x0015u, 0x00u, 0u },
    { 0x00003331u, 0x0015u, 0x00u, 0u },
    { 0x00003332u, 0x0015u, 0x00u, 0u },
    { 0x00003333u, 0x0015u, 0x00u, 0u },
    { 0x00003334u, 0x0015u, 0x00u, 0u },
    { 0x00003335u, 0x0015u, 0x00u, 0u },
    { 0x00003336u, 0x0015u, 0x00u, 0u },
    { 0x00003337u, 0x0015u, 0x00u, 0u },
    { 0x00003338u, 0x0015u, 0x00u, 0u },
    { 0x00003339u, 0x0015u, 0x00u, 0u },
    { 0x0000333Au, 0x0015u, 0x00u, 0u },
    { 0x0000333Bu, 0x0015u, 0x00u, 0u },
    { 0x0000333Cu, 0x0015u, 0x00u, 0u },
    { 0x0000333Du, 0x0015u, 0x00u, 0u },
    { 0x0000333Eu, 0x0015u, 0x00u, 0u },
    { 0x0000333Fu, 0x0015u, 0x00u, 0u },
    { 0x00003340u, 0x0015u, 0x00u, 0u },
    { 0x00003341u, 0x0015u, 0x00u, 0u },
    { 0x00003342u, 0x0015u, 0x00u, 0u },
    { 0x00003343u, 0x0015u, 0x00u, 0u },
    { 0x00003344u, 0x0015u, 0x00u, 0u },
    { 0x00003345u, 0x0015u, 0x00u, 0u },
    { 0x00003346u, 0x0015u, 0x00u, 0u },
    { 0x00003347u, 0x0015u, 0x00u, 0u },
    { 0x00003348u, 0x0015u, 0x00u, 0u },
    { 0x00003349u, 0x0015u, 0x00u, 0u },
    { 0x0000334Au, 0x0015u, 0x00u, 0u },
    { 0x0000334Bu, 0x0015u, 0x00u, 0u },
    { 0x0000334Cu, 0x0015u, 0x00u, 0u },
    { 0x0000334Du, 0x0015u, 0x00u, 0u },
    { 0x0000334Eu, 0x0015u, 0x00u, 0u },
    { 0x0000334Fu, 0x0015u, 0x00u, 0u },
    { 0x00003350u, 0x0015u, 0x00u, 0u },
    { 0x00003351u, 0x0015u, 0x00u, 0u },
    { 0x00003352u, 0x0015u, 0x00u, 0u },
    { 0x00003353u, 0x0015u, 0x00u, 0u },
    { 0x00003354u, 0x0015u, 0x00u, 0u },
    { 0x00003355u, 0x0015u, 0x00u, 0u },
    { 0x00003356u, 0x0015u, 0x00u, 0u },
    { 0x00003357u, 0x0015u, 0x00u, 0u },
    { 0x0000337Bu, 0x0015u, 0x00u, 0u },
    { 0x0000337Cu, 0x0015u, 0x00u, 0u },
    { 0x0000337Du, 0x0015u, 0x00u, 0u },
    { 0x0000337Eu, 0x0015u, 0x00u, 0u },
    { 0x0000337Fu, 0x0015u, 0x00u, 0u },
    { 0x0000FE50u, 0x0006u, 0x00u, 0u },
    { 0x0000FE51u, 0x0006u, 0x00u, 0u },
    { 0x0000FE52u, 0x0006u, 0x00u, 0u },
    { 0x0000FE59u, 0x0006u, 0x00u, 0u },
    { 0x0000FE5Au, 0x0006u, 0x00u, 0u },
    { 0x0000FE5Bu, 0x0006u, 0x00u, 0u },
    { 0x0000FE5Cu, 0x0006u, 0x00u, 0u },
    { 0x0000FE5Du, 0x0006u, 0x00u, 0u },
    { 0x0000FE5Eu, 0x0006u, 0x00u, 0u },
    { 0x0000FF01u, 0x0006u, 0x00u, 0u },
    { 0x0000FF08u, 0x0006u, 0x00u, 0u },
    { 0x0000FF09u, 0x0006u, 0x00u, 0u },
    { 0x0000FF0Cu, 0x0006u, 0x00u, 0u },
    { 0x0000FF0Eu, 0x0006u, 0x00u, 0u },
    { 0x0000FF1Au, 0x0006u, 0x00u, 0u },
    { 0x0000FF1Bu, 0x0006u, 0x00u, 0u },
    { 0x0000FF1Fu, 0x0006u, 0x00u, 0u },
    { 0x0000FF3Bu, 0x0006u, 0x00u, 0u },
    { 0x0000FF3Du, 0x0006u, 0x00u, 0u },
    { 0x0000FF3Fu, 0x0006u, 0x00u, 0u },
    { 0x0000FF5Bu, 0x0006u, 0x00u, 0u },
    { 0x0000FF5Cu, 0x0006u, 0x00u, 0u },
    { 0x0000FF5Du, 0x0006u, 0x00u, 0u },
    { 0x0000FF5Eu, 0x0006u, 0x00u, 0u },
    { 0x0000FF5Fu, 0x0006u, 0x00u, 0u },
    { 0x0000FF60u, 0x0006u, 0x00u, 0u },
    { 0x0000FFE3u, 0x0006u, 0x00u, 0u },
    { 0x0001B132u, 0x0006u, 0x00u, 0u },
    { 0x0001B150u, 0x0006u, 0x00u, 0u },
    { 0x0001B151u, 0x0006u, 0x00u, 0u },
    { 0x0001B152u, 0x0006u, 0x00u, 0u },
    { 0x0001B155u, 0x0006u, 0x00u, 0u },
    { 0x0001B164u, 0x0006u, 0x00u, 0u },
    { 0x0001B165u, 0x0006u, 0x00u, 0u },
    { 0x0001B166u, 0x0006u, 0x00u, 0u },
    { 0x0001B167u, 0x0006u, 0x00u, 0u },
    { 0x0001B168u, 0x0006u, 0x00u, 0u },
    { 0x0001F200u, 0x0015u, 0x00u, 0u },
    { 0x0001F201u, 0x0015u, 0x00u, 0u },
    { 0x00160000u, 0x0009u, 0x00u, 0u },
    { 0x00160100u, 0x0009u, 0x04u, 0u },
    { 0x00160200u, 0x0009u, 0xFFu, 0u },
};

#define SUVTL_TABLE_COUNT \
    ((uint32_t)(sizeof suvtl_table / sizeof suvtl_table[0]))

SUVTL_STATIC_ASSERT(SUVTL_TABLE_COUNT <= SUAS_SUVTL_REGISTRY_CAPACITY,
                    suvtl_table_fits_capacity);

/* ── The static lookup index (§3.5) ───────────────────────────────── */

typedef struct {
    uint16_t entry;
    uint8_t  used;
} suvtl_index_slot_t;

static suvtl_index_slot_t suvtl_index[SUAS_SUVTL_INDEX_SIZE];
static bool suvtl_ready = false;

/* ── FNV-1a primitives (§3.4) ─────────────────────────────────────── */

#define SUVTL_FNV_OFFSET 2166136261u
#define SUVTL_FNV_PRIME  16777619u

uint32_t suas_suvtl_hash(uint32_t codepoint)
{
    uint32_t h = SUVTL_FNV_OFFSET;
    unsigned i;

    for (i = 0u; i < 4u; i++) {
        h ^= (uint32_t)suvtl_byte_at(codepoint, i);
        h *= SUVTL_FNV_PRIME;
    }
    return h;
}

/* ── Orientation value helpers (§3.1) ─────────────────────────────── */

/* The value is read directly off the bits, so nothing has to be derived. A
 * well-formed record has exactly one of UPRIGHT / ROTATED set; the value's low
 * three bits are then simply the orientation, with TRANSFORM selecting the
 * Tu/Tr forms. */
static uint16_t suvtl_value_bits(uint16_t flags)
{
    return (uint16_t)(flags & SUVTL_ORIENT_MASK);
}

/* True when the value bits name exactly one of the four UAX #50 values. */
static bool suvtl_value_well_formed(uint16_t value)
{
    return value == SUVTL_ORIENT_U ||
           value == SUVTL_ORIENT_R ||
           value == SUVTL_ORIENT_TU ||
           value == SUVTL_ORIENT_TR;
}

/* ── Zone dispatch (§5) ───────────────────────────────────────────── */

suas_suvtl_zone_t suas_suvtl_zone_of(uint32_t codepoint)
{
    if (codepoint > (uint32_t)SUAS_CODEPOINT_MAX)
        return SUAS_SUVTL_ZONE_EXTSUCS;
    if (codepoint <= SUAS_SUVTL_LIMIT_UNICODE_MAX)
        return SUAS_SUVTL_ZONE_BRIDGE;
    if (codepoint >= SUAS_SUVTL_LIMIT_SCP_MIN &&
        codepoint <= SUAS_SUVTL_LIMIT_SCP_MAX)
        return SUAS_SUVTL_ZONE_SCP;
    if (codepoint >= SUAS_SUVTL_LIMIT_TRAP_MIN &&
        codepoint <= SUAS_SUVTL_LIMIT_TRAP_MAX)
        return SUAS_SUVTL_ZONE_TRAP;
    if (codepoint == SUAS_SUVTL_LIMIT_SENTINEL)
        return SUAS_SUVTL_ZONE_SENTINEL;
    if (codepoint >= SUAS_SUVTL_RNUR_MIN &&
        codepoint <= SUAS_SUVTL_RNUR_MAX)
        return SUAS_SUVTL_ZONE_RNUR;
    return SUAS_SUVTL_ZONE_NATIVE;
}

bool suas_suvtl_is_text_space(uint32_t codepoint)
{
    switch (suas_suvtl_zone_of(codepoint)) {
    case SUAS_SUVTL_ZONE_BRIDGE:
    case SUAS_SUVTL_ZONE_RNUR:
    case SUAS_SUVTL_ZONE_NATIVE:
        return true;
    default:
        return false;
    }
}

bool suas_suvtl_is_neography(uint32_t codepoint)
{
    return suas_suvtl_zone_of(codepoint) == SUAS_SUVTL_ZONE_RNUR;
}

bool suas_suvtl_neography_set(uint32_t codepoint, uint32_t* out_set)
{
    if (out_set == NULL || !suas_suvtl_is_neography(codepoint))
        return false;
    *out_set = (codepoint - SUAS_SUVTL_RNUR_MIN) >> SUAS_SUVTL_RNUR_SET_SHIFT;
    return true;
}

/* ── Vertical-behaviour groups (§5.2) ─────────────────────────────── */

/* The group is a coarse classification of the codepoint's script for the
 * algorithmic default. The ranges below are the East Asian blocks that UAX #50
 * treats as upright in vertical text (Han, Hiragana, Katakana, Hangul,
 * Bopomofo, and the CJK compatibility/enclosed forms), the presentation forms
 * that rotate, and the remainder. */
suas_suvtl_group_t suas_suvtl_group_of(uint32_t codepoint)
{
    if (suas_suvtl_zone_of(codepoint) == SUAS_SUVTL_ZONE_RNUR)
        return SUAS_SUVTL_GROUP_NATIVE;

    /* East Asian ideographic and syllabic blocks: upright by default. */
    if ((codepoint >= 0x00002E80u && codepoint <= 0x00009FFFu) ||  /* CJK/Kana */
        (codepoint >= 0x0000AC00u && codepoint <= 0x0000D7AFu) ||  /* Hangul  */
        (codepoint >= 0x0000F900u && codepoint <= 0x0000FAFFu) ||  /* CJK comp*/
        (codepoint >= 0x0000FE30u && codepoint <= 0x0000FE4Fu) ||  /* CJK form*/
        (codepoint >= 0x0000FF00u && codepoint <= 0x0000FFEFu) ||  /* Fullwidth*/
        (codepoint >= 0x00020000u && codepoint <= 0x0003FFFFu))    /* Ext B+  */
        return SUAS_SUVTL_GROUP_CJK;

    /* Latin, Greek, Cyrillic and the other horizontally-written scripts that
     * rotate ninety degrees clockwise in vertical columns. */
    if ((codepoint >= 0x00000041u && codepoint <= 0x0000007Au) ||  /* Latin */
        (codepoint >= 0x00000391u && codepoint <= 0x000003C9u) ||  /* Greek */
        (codepoint >= 0x00000410u && codepoint <= 0x000004FFu))    /* Cyrillic */
        return SUAS_SUVTL_GROUP_ROTATING;

    return SUAS_SUVTL_GROUP_OTHER;
}

/* ── Algorithmic default (§5.3) ───────────────────────────────────── */

/* A codepoint with no curated record is not unknown: its orientation follows
 * from its script group. CJK stands upright; rotating scripts rotate; the
 * remainder — punctuation, symbols, the Latin of an acronym and the like — is
 * upright, which is UAX #50's own default for unassigned and common characters.
 * The synthesized record carries only the derived value bits. */
static suvtl_glyph_layout_t suvtl_default_record(uint32_t codepoint)
{
    suvtl_glyph_layout_t rec;
    uint16_t value = SUVTL_ORIENT_U;

    if (suas_suvtl_group_of(codepoint) == SUAS_SUVTL_GROUP_ROTATING)
        value = SUVTL_ORIENT_R;

    rec.codepoint = codepoint;
    rec.orientation_flags = value;
    rec.baseline_offset = 0u;
    rec.reserved = 0u;
    return rec;
}

/* ── Lazy index construction (§3.5) ───────────────────────────────── */

static void suvtl_build(void)
{
    uint32_t i;
    uint32_t h;
    uint32_t start;

    for (i = 0u; i < SUAS_SUVTL_INDEX_SIZE; i++) {
        suvtl_index[i].entry = 0u;
        suvtl_index[i].used = 0u;
    }
    for (i = 0u; i < SUVTL_TABLE_COUNT; i++) {
        h = suas_suvtl_hash(suvtl_table[i].codepoint) &
            (SUAS_SUVTL_INDEX_SIZE - 1u);
        start = h;
        while (suvtl_index[h].used) {
            h = (h + 1u) & (SUAS_SUVTL_INDEX_SIZE - 1u);
            if (h == start)
                break; /* unreachable: capacity < index size */
        }
        suvtl_index[h].entry = (uint16_t)i;
        suvtl_index[h].used = 1u;
    }

    suvtl_ready = true;
}

void suas_suvtl_init_engine(void)
{
    if (!suvtl_ready)
        suvtl_build();
}

/* ── O(1) probe (§3.5) ────────────────────────────────────────────── */

static const suvtl_glyph_layout_t* suvtl_find(uint32_t codepoint)
{
    uint32_t h;
    uint32_t start;

    if (!suas_suvtl_is_text_space(codepoint))
        return NULL;
    if (!suvtl_ready)
        suvtl_build();

    h = suas_suvtl_hash(codepoint) & (SUAS_SUVTL_INDEX_SIZE - 1u);
    start = h;
    for (;;) {
        const suvtl_glyph_layout_t* cand;
        if (!suvtl_index[h].used)
            return NULL;
        cand = &suvtl_table[suvtl_index[h].entry];
        if (cand->codepoint == codepoint)
            return cand;
        h = (h + 1u) & (SUAS_SUVTL_INDEX_SIZE - 1u);
        if (h == start)
            return NULL;
    }
}

/* ── Query surface ────────────────────────────────────────────────── */

bool suas_suvtl_get_layout_properties(uint32_t codepoint,
                                      suvtl_glyph_layout_t* out_layout)
{
    const suvtl_glyph_layout_t* found;

    if (out_layout == NULL)
        return false;
    if (!suas_suvtl_is_text_space(codepoint))
        return false;

    found = suvtl_find(codepoint);
    if (found != NULL) {
        *out_layout = *found;
        return true;
    }
    *out_layout = suvtl_default_record(codepoint);
    return true;
}

uint16_t suas_suvtl_orientation_of(uint32_t codepoint)
{
    suvtl_glyph_layout_t rec;

    if (!suas_suvtl_get_layout_properties(codepoint, &rec))
        return SUVTL_ORIENT_U;
    return (uint16_t)(rec.orientation_flags & SUVTL_ORIENT_MASK);
}

bool suas_suvtl_is_upright(uint32_t codepoint)
{
    uint16_t value = suas_suvtl_orientation_of(codepoint);

    return value == SUVTL_ORIENT_U || value == SUVTL_ORIENT_TU;
}

bool suas_suvtl_is_rotated(uint32_t codepoint)
{
    uint16_t value = suas_suvtl_orientation_of(codepoint);

    return value == SUVTL_ORIENT_R || value == SUVTL_ORIENT_TR;
}

bool suas_suvtl_is_transformed(uint32_t codepoint)
{
    uint16_t value = suas_suvtl_orientation_of(codepoint);

    return value == SUVTL_ORIENT_TU || value == SUVTL_ORIENT_TR;
}

int suas_suvtl_baseline_offset(uint32_t codepoint)
{
    suvtl_glyph_layout_t rec;

    if (!suas_suvtl_get_layout_properties(codepoint, &rec))
        return 0;
    return (int)(int8_t)rec.baseline_offset;
}

bool suas_suvtl_is_registered(uint32_t codepoint)
{
    return suvtl_find(codepoint) != NULL;
}

/* ── Orientation and cluster helpers (§4) ─────────────────────────── */

/* The enclosing combining marks (general category Me) that UAX #50 §3.2.1
 * names as the exception to the first-character rule. The ranges are the Me
 * blocks of the Unicode Standard, curated here so the engine stays freestanding
 * and needs no SUCD image loaded. */
static const struct {
    uint32_t first;
    uint32_t last;
} suvtl_me_ranges[] = {
    { 0x00000488u, 0x00000489u }, /* Cyrillic combining enclosing          */
    { 0x00001ABEu, 0x00001AFFu }, /* combining diacritical marks extended  */
    { 0x000020DDu, 0x000020E0u }, /* combining enclosing marks for symbols */
    { 0x000020E2u, 0x000020E4u },
    { 0x0000A670u, 0x0000A672u }, /* Cyrillic combining enclosing (ext)    */
    { 0x0000FE20u, 0x0000FE2Fu }  /* combining half marks                  */
};

bool suas_suvtl_is_enclosing_mark(uint32_t codepoint)
{
    uint32_t i;

    for (i = 0u; i < (uint32_t)(sizeof suvtl_me_ranges /
                                sizeof suvtl_me_ranges[0]); i++) {
        if (codepoint >= suvtl_me_ranges[i].first &&
            codepoint <= suvtl_me_ranges[i].last)
            return true;
    }
    return false;
}

uint16_t suas_suvtl_cluster_orientation(const uint32_t* codepoints,
                                        size_t count)
{
    size_t i;

    if (codepoints == NULL || count == 0u)
        return SUVTL_ORIENT_U;

    /* The enclosing-mark exception scans the whole cluster first, so a mark
     * after the base still forces upright. Detecting Me is done by the engine's
     * curated Me ranges (suas_suvtl_is_enclosing_mark); the orientation of the
     * first character supplies the value otherwise. */
    for (i = 0u; i < count; i++) {
        if (suas_suvtl_is_enclosing_mark(codepoints[i]))
            return SUVTL_ORIENT_U;
    }
    return suas_suvtl_orientation_of(codepoints[0]);
}

/* ── Enumeration ──────────────────────────────────────────────────── */

uint32_t suas_suvtl_entry_count(void)
{
    return SUVTL_TABLE_COUNT;
}

bool suas_suvtl_entry_at(uint32_t index, suvtl_glyph_layout_t* out_layout)
{
    if (out_layout == NULL || index >= SUVTL_TABLE_COUNT)
        return false;
    *out_layout = suvtl_table[index];
    return true;
}

/* ── Taxonomy names ───────────────────────────────────────────────── */

const char* suas_suvtl_orientation_name(uint16_t orientation)
{
    switch (orientation & SUVTL_ORIENT_MASK) {
    case SUVTL_ORIENT_U:  return "U";
    case SUVTL_ORIENT_R:  return "R";
    case SUVTL_ORIENT_TU: return "Tu";
    case SUVTL_ORIENT_TR: return "Tr";
    default:              return NULL;
    }
}

const char* suas_suvtl_orientation_long_name(uint16_t orientation)
{
    switch (orientation & SUVTL_ORIENT_MASK) {
    case SUVTL_ORIENT_U:  return "Upright";
    case SUVTL_ORIENT_R:  return "Rotated";
    case SUVTL_ORIENT_TU: return "Transformed Upright";
    case SUVTL_ORIENT_TR: return "Transformed Rotated";
    default:              return NULL;
    }
}

const char* suas_suvtl_group_name(suas_suvtl_group_t group)
{
    switch (group) {
    case SUAS_SUVTL_GROUP_OTHER:    return "OTHER";
    case SUAS_SUVTL_GROUP_CJK:      return "CJK";
    case SUAS_SUVTL_GROUP_ROTATING: return "ROTATING";
    case SUAS_SUVTL_GROUP_NATIVE:   return "NATIVE";
    default:                        return "INVALID";
    }
}

const char* suas_suvtl_registry_error_name(suvtl_registry_error_t error)
{
    switch (error) {
    case SUVTL_REGISTRY_OK:                      return "OK";
    case SUVTL_REGISTRY_NULL_TABLE:              return "NULL_TABLE";
    case SUVTL_REGISTRY_COUNT_OVER:              return "COUNT_OVER";
    case SUVTL_REGISTRY_UNORDERED:               return "UNORDERED";
    case SUVTL_REGISTRY_DUPLICATE_CODEPOINT:     return "DUPLICATE_CODEPOINT";
    case SUVTL_REGISTRY_OUT_OF_SPACE:            return "OUT_OF_SPACE";
    case SUVTL_REGISTRY_INADMISSIBLE_CODEPOINT:  return "INADMISSIBLE_CODEPOINT";
    case SUVTL_REGISTRY_BAD_ORIENTATION:         return "BAD_ORIENTATION";
    case SUVTL_REGISTRY_BAD_FLAGS:               return "BAD_FLAGS";
    case SUVTL_REGISTRY_BAD_RESERVED:            return "BAD_RESERVED";
    case SUVTL_REGISTRY_CUSTOM_FLOW_MISMATCH:    return "CUSTOM_FLOW_MISMATCH";
    default:                                     return "UNKNOWN";
    }
}

/* ── Verification (§9) ────────────────────────────────────────────── */

static bool suvtl_fail(suvtl_registry_error_t* out_error,
                       suvtl_registry_error_t error)
{
    if (out_error != NULL)
        *out_error = error;
    return false;
}

bool suas_suvtl_validate_table(const suvtl_glyph_layout_t* entries,
                               uint32_t count,
                               suvtl_registry_error_t* out_error)
{
    uint32_t i;

    if (out_error != NULL)
        *out_error = SUVTL_REGISTRY_OK;
    if (count > SUAS_SUVTL_REGISTRY_CAPACITY)
        return suvtl_fail(out_error, SUVTL_REGISTRY_COUNT_OVER);
    if (count == 0u)
        return true;
    if (entries == NULL)
        return suvtl_fail(out_error, SUVTL_REGISTRY_NULL_TABLE);

    for (i = 0u; i < count; i++) {
        const suvtl_glyph_layout_t* e = &entries[i];
        uint16_t value = suvtl_value_bits(e->orientation_flags);

        if (e->codepoint > (uint32_t)SUAS_CODEPOINT_MAX)
            return suvtl_fail(out_error, SUVTL_REGISTRY_OUT_OF_SPACE);
        if (!suas_suvtl_is_text_space(e->codepoint))
            return suvtl_fail(out_error,
                              SUVTL_REGISTRY_INADMISSIBLE_CODEPOINT);
        /* No reserved bits may be set. */
        if ((e->orientation_flags & (uint16_t)~SUVTL_FLAG_ASSIGNED_MASK) != 0u)
            return suvtl_fail(out_error, SUVTL_REGISTRY_BAD_FLAGS);
        /* The value bits must name exactly one of U/R/Tu/Tr, which also
         * enforces that exactly one of UPRIGHT / ROTATED is set. */
        if (!suvtl_value_well_formed(value))
            return suvtl_fail(out_error, SUVTL_REGISTRY_BAD_ORIENTATION);
        if (e->reserved != 0u)
            return suvtl_fail(out_error, SUVTL_REGISTRY_BAD_RESERVED);
        /* A custom-flow declaration only makes sense inside the RNUR set. */
        if ((e->orientation_flags & SUVTL_FLAG_CUSTOM_FLOW) != 0u &&
            !suas_suvtl_is_neography(e->codepoint))
            return suvtl_fail(out_error,
                              SUVTL_REGISTRY_CUSTOM_FLOW_MISMATCH);

        if (i > 0u) {
            if (e->codepoint == entries[i - 1u].codepoint)
                return suvtl_fail(out_error,
                                  SUVTL_REGISTRY_DUPLICATE_CODEPOINT);
            if (e->codepoint < entries[i - 1u].codepoint)
                return suvtl_fail(out_error, SUVTL_REGISTRY_UNORDERED);
        }
    }
    return true;
}

/* ── Version ──────────────────────────────────────────────────────── */

const char* suas_suvtl_version_string(void)
{
    return "1.0";
}
