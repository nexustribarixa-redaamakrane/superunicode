/* SUAS-016 - SuperUnicode Egyptian Hieroglyph Database (SUKEMET)
 *
 * Freestanding C99, zero allocation. The engine is a static array of
 * sukemet_hieroglyph_entry_t records, built at compile time from the
 * Unikemet 18.0.0 catalogue and the Egyptian Hieroglyph Format Controls
 * block, plus a fixed-size open-addressing index over their codepoints.
 * Neither the table nor the index grows at run time.
 *
 * The 12-byte sketch record carries identity and classification. The
 * informative Unikemet fields - catalogue index, description hash, UniK hash,
 * source mask, Core class, function mask, modifier role and alternate
 * sequence - live in a parallel metadata table, aligned one-to-one with the
 * main records.
 *
 * No part of this file reaches for malloc, stdio or <string.h>.
 */

#include "suas/suas_sukemet.h"

/* Freestanding primitives */

#define SUKEMET_STATIC_ASSERT(cond, name) \
    typedef char name[(cond) ? 1 : -1]

SUKEMET_STATIC_ASSERT(sizeof(sukemet_hieroglyph_entry_t) == 12u,
                      sukemet_entry_is_12_bytes);
SUKEMET_STATIC_ASSERT(offsetof(sukemet_hieroglyph_entry_t, codepoint) == 0u,
                      sukemet_entry_codepoint_at_0);
SUKEMET_STATIC_ASSERT(offsetof(sukemet_hieroglyph_entry_t, phonetic_hash) == 4u,
                      sukemet_entry_phonetic_hash_at_4);
SUKEMET_STATIC_ASSERT(offsetof(sukemet_hieroglyph_entry_t, sukemet_flags) == 8u,
                      sukemet_entry_flags_at_8);
SUKEMET_STATIC_ASSERT(offsetof(sukemet_hieroglyph_entry_t, gardiner_category) == 10u,
                      sukemet_entry_category_at_10);
SUKEMET_STATIC_ASSERT(offsetof(sukemet_hieroglyph_entry_t, sub_index) == 11u,
                      sukemet_entry_sub_index_at_11);
SUKEMET_STATIC_ASSERT(sizeof(sukemet_metadata_t) == 24u,
                      sukemet_metadata_is_24_bytes);

/* FNV-1a primitives */

#define SUKEMET_FNV_OFFSET 2166136261u
#define SUKEMET_FNV_PRIME  16777619u

static uint8_t sukemet_byte_at(uint32_t codepoint, unsigned index)
{
    return (uint8_t)((codepoint >> (index * 8u)) & 0xFFu);
}

uint32_t suas_sukemet_hash(uint32_t codepoint)
{
    uint32_t h = SUKEMET_FNV_OFFSET;
    unsigned i;

    for (i = 0u; i < 4u; i++) {
        h ^= (uint32_t)sukemet_byte_at(codepoint, i);
        h *= SUKEMET_FNV_PRIME;
    }
    return h;
}

uint32_t suas_sukemet_hash_bytes(const char *bytes, size_t length)
{
    uint32_t h = SUKEMET_FNV_OFFSET;
    size_t i;

    if (bytes == NULL)
        return 0u;
    for (i = 0u; i < length; i++) {
        h ^= (uint32_t)(uint8_t)bytes[i];
        h *= SUKEMET_FNV_PRIME;
    }
    return h;
}

/* Generated catalogue (see suas_sukemet_data.inc) */

#include "suas_sukemet_data.inc"

SUKEMET_STATIC_ASSERT(SUKEMET_TABLE_COUNT <= SUAS_SUKEMET_REGISTRY_CAPACITY,
                      sukemet_table_fits_capacity);
SUKEMET_STATIC_ASSERT(SUKEMET_TABLE_COUNT <= 65534u,
                      sukemet_table_fits_index_slot);
SUKEMET_STATIC_ASSERT(SUAS_SUKEMET_INDEX_SIZE > SUAS_SUKEMET_REGISTRY_CAPACITY,
                      sukemet_index_has_slack);
SUKEMET_STATIC_ASSERT((SUAS_SUKEMET_INDEX_SIZE & (SUAS_SUKEMET_INDEX_SIZE - 1u)) == 0u,
                      sukemet_index_is_power_of_two);
SUKEMET_STATIC_ASSERT(SUKEMET_ALT_DATA_COUNT <= SUAS_SUKEMET_ALT_CAPACITY,
                      sukemet_alt_data_fits_capacity);
SUKEMET_STATIC_ASSERT(sizeof sukemet_meta_table / sizeof sukemet_meta_table[0]
                      == SUKEMET_TABLE_COUNT,
                      sukemet_meta_table_aligned);
SUKEMET_STATIC_ASSERT(sizeof sukemet_alt_offset / sizeof sukemet_alt_offset[0]
                      == SUKEMET_TABLE_COUNT,
                      sukemet_alt_offset_aligned);

/* Static lookup index */

typedef struct {
    uint16_t entry_plus_one; /* 0 = empty, otherwise table index + 1 */
} sukemet_index_slot_t;

static sukemet_index_slot_t sukemet_index[SUAS_SUKEMET_INDEX_SIZE];
static bool sukemet_ready = false;

static void sukemet_build(void)
{
    uint32_t i;
    uint32_t h;
    uint32_t start;

    for (i = 0u; i < SUAS_SUKEMET_INDEX_SIZE; i++)
        sukemet_index[i].entry_plus_one = 0u;

    for (i = 0u; i < SUKEMET_TABLE_COUNT; i++) {
        h = suas_sukemet_hash(sukemet_table[i].codepoint) &
            (SUAS_SUKEMET_INDEX_SIZE - 1u);
        start = h;
        while (sukemet_index[h].entry_plus_one != 0u) {
            h = (h + 1u) & (SUAS_SUKEMET_INDEX_SIZE - 1u);
            if (h == start)
                break; /* unreachable: capacity < index size */
        }
        sukemet_index[h].entry_plus_one = (uint16_t)(i + 1u);
    }

    sukemet_ready = true;
}

void suas_sukemet_init_registry(void)
{
    if (!sukemet_ready)
        sukemet_build();
}

/* O(1) probe. Returns the table index, or (uint32_t)-1 when absent. */

static uint32_t sukemet_table_index(uint32_t codepoint)
{
    uint32_t h;
    uint32_t start;

    if (!sukemet_ready)
        sukemet_build();

    h = suas_sukemet_hash(codepoint) & (SUAS_SUKEMET_INDEX_SIZE - 1u);
    start = h;
    for (;;) {
        uint32_t slot = sukemet_index[h].entry_plus_one;
        if (slot == 0u)
            return (uint32_t)-1;
        if (sukemet_table[slot - 1u].codepoint == codepoint)
            return slot - 1u;
        h = (h + 1u) & (SUAS_SUKEMET_INDEX_SIZE - 1u);
        if (h == start)
            return (uint32_t)-1;
    }
}

/* Zone dispatch (section 2) */

suas_sukemet_zone_t suas_sukemet_zone_of(uint32_t codepoint)
{
    if (codepoint > (uint32_t)SUAS_CODEPOINT_MAX)
        return SUAS_SUKEMET_ZONE_EXTSUCS;
    if (codepoint <= SUAS_SUKEMET_LIMIT_UNICODE_MAX)
        return SUAS_SUKEMET_ZONE_BRIDGE;
    if (codepoint >= SUAS_SUKEMET_LIMIT_SCP_MIN &&
        codepoint <= SUAS_SUKEMET_LIMIT_SCP_MAX)
        return SUAS_SUKEMET_ZONE_SCP;
    if (codepoint >= SUAS_SUKEMET_RNUR_MIN &&
        codepoint <= SUAS_SUKEMET_RNUR_MAX)
        return SUAS_SUKEMET_ZONE_RNUR;
    if (codepoint >= SUAS_SUKEMET_LIMIT_TRAP_MIN &&
        codepoint <= SUAS_SUKEMET_LIMIT_TRAP_MAX)
        return SUAS_SUKEMET_ZONE_TRAP;
    if (codepoint == SUAS_SUKEMET_LIMIT_SENTINEL)
        return SUAS_SUKEMET_ZONE_SENTINEL;
    return SUAS_SUKEMET_ZONE_NATIVE;
}

bool suas_sukemet_is_text_space(uint32_t codepoint)
{
    switch (suas_sukemet_zone_of(codepoint)) {
    case SUAS_SUKEMET_ZONE_BRIDGE:
    case SUAS_SUKEMET_ZONE_RNUR:
    case SUAS_SUKEMET_ZONE_NATIVE:
        return true;
    default:
        return false;
    }
}

sukemet_block_t suas_sukemet_block_of(uint32_t codepoint)
{
    if (codepoint >= SUKEMET_BLOCK_HIEROGLYPHS_MIN &&
        codepoint <= SUKEMET_BLOCK_HIEROGLYPHS_MAX)
        return SUKEMET_BLOCK_EGYPTIAN;
    if (codepoint >= SUKEMET_BLOCK_CONTROLS_MIN &&
        codepoint <= SUKEMET_BLOCK_CONTROLS_MAX)
        return SUKEMET_BLOCK_FORMAT_CONTROLS;
    if (codepoint >= SUKEMET_BLOCK_EXTENDED_A_MIN &&
        codepoint <= SUKEMET_BLOCK_EXTENDED_A_MAX)
        return SUKEMET_BLOCK_EXTENDED_A;
    if (codepoint >= SUAS_SUKEMET_RNUR_MIN &&
        codepoint <= SUAS_SUKEMET_RNUR_MAX)
        return SUKEMET_BLOCK_NEOGRAPHY;
    return SUKEMET_BLOCK_NONE;
}

bool suas_sukemet_is_neography(uint32_t codepoint)
{
    return suas_sukemet_zone_of(codepoint) == SUAS_SUKEMET_ZONE_RNUR;
}

bool suas_sukemet_neography_set(uint32_t codepoint, uint32_t *out_set)
{
    if (out_set == NULL || !suas_sukemet_is_neography(codepoint))
        return false;
    *out_set = (codepoint - SUAS_SUKEMET_RNUR_MIN) >>
               SUAS_SUKEMET_RNUR_SET_SHIFT;
    return true;
}

bool suas_sukemet_is_hieroglyph(uint32_t codepoint)
{
    if (!suas_sukemet_is_text_space(codepoint))
        return false;
    return suas_sukemet_block_of(codepoint) != SUKEMET_BLOCK_NONE;
}

/* Algorithmic default (section 6) */

static void sukemet_default(uint32_t codepoint,
                            sukemet_hieroglyph_entry_t *out_entry)
{
    out_entry->codepoint = codepoint;
    out_entry->phonetic_hash = 0u;
    out_entry->sukemet_flags = suas_sukemet_is_neography(codepoint)
                                   ? (uint16_t)SUKEMET_FLAG_NEOGRAPHY
                                   : (uint16_t)0u;
    out_entry->gardiner_category = (uint8_t)SUKEMET_GARDINER_NONE;
    out_entry->sub_index = 0u;
}

/* Core query (section 3.4) */

bool suas_sukemet_get_entry(uint32_t codepoint,
                            sukemet_hieroglyph_entry_t *out_entry)
{
    uint32_t index;

    if (out_entry == NULL || !suas_sukemet_is_hieroglyph(codepoint))
        return false;

    index = sukemet_table_index(codepoint);
    if (index == (uint32_t)-1) {
        sukemet_default(codepoint, out_entry);
        return true;
    }

    *out_entry = sukemet_table[index];
    return true;
}

bool suas_sukemet_get_metadata(uint32_t codepoint,
                               sukemet_metadata_t *out_metadata)
{
    uint32_t index;

    if (out_metadata == NULL || !suas_sukemet_is_hieroglyph(codepoint))
        return false;

    index = sukemet_table_index(codepoint);
    if (index == (uint32_t)-1) {
        uint32_t i;
        uint8_t *bytes = (uint8_t *)out_metadata;
        for (i = 0u; i < (uint32_t)sizeof *out_metadata; i++)
            bytes[i] = 0u;
        return true;
    }

    *out_metadata = sukemet_meta_table[index];
    return true;
}

bool suas_sukemet_is_registered(uint32_t codepoint)
{
    if (!suas_sukemet_is_hieroglyph(codepoint))
        return false;
    return sukemet_table_index(codepoint) != (uint32_t)-1;
}

/* Classification predicates (section 3) */

static bool sukemet_flag(uint32_t codepoint, uint16_t flag)
{
    sukemet_hieroglyph_entry_t entry;

    if (!suas_sukemet_get_entry(codepoint, &entry))
        return false;
    return (entry.sukemet_flags & flag) != 0u;
}

bool suas_sukemet_is_gardiner(uint32_t codepoint)
{
    return sukemet_flag(codepoint, (uint16_t)SUKEMET_FLAG_GARDINER);
}

bool suas_sukemet_is_modifier(uint32_t codepoint)
{
    return sukemet_flag(codepoint, (uint16_t)SUKEMET_FLAG_MODIFIER);
}

bool suas_sukemet_is_enclosure(uint32_t codepoint)
{
    return sukemet_flag(codepoint, (uint16_t)SUKEMET_FLAG_ENCLOSURE);
}

bool suas_sukemet_is_composite(uint32_t codepoint)
{
    return sukemet_flag(codepoint, (uint16_t)SUKEMET_FLAG_COMPOSITE);
}

bool suas_sukemet_is_core(uint32_t codepoint)
{
    return sukemet_flag(codepoint, (uint16_t)SUKEMET_FLAG_CORE);
}

bool suas_sukemet_is_legacy(uint32_t codepoint)
{
    return sukemet_flag(codepoint, (uint16_t)SUKEMET_FLAG_LEGACY);
}

bool suas_sukemet_is_format_control(uint32_t codepoint)
{
    return sukemet_flag(codepoint, (uint16_t)SUKEMET_FLAG_FORMAT);
}

bool suas_sukemet_is_joiner(uint32_t codepoint)
{
    return sukemet_flag(codepoint, (uint16_t)SUKEMET_FLAG_STACKING);
}

bool suas_sukemet_is_insertion(uint32_t codepoint)
{
    return sukemet_flag(codepoint, (uint16_t)SUKEMET_FLAG_INSERTION);
}

bool suas_sukemet_is_overlay(uint32_t codepoint)
{
    return sukemet_flag(codepoint, (uint16_t)SUKEMET_FLAG_OVERLAY);
}

bool suas_sukemet_is_segment(uint32_t codepoint)
{
    return sukemet_flag(codepoint, (uint16_t)SUKEMET_FLAG_SEGMENT);
}

bool suas_sukemet_is_mirror_control(uint32_t codepoint)
{
    return sukemet_flag(codepoint, (uint16_t)SUKEMET_FLAG_MIRROR);
}

bool suas_sukemet_is_mirrorable(uint32_t codepoint)
{
    sukemet_hieroglyph_entry_t entry;

    if (!suas_sukemet_get_entry(codepoint, &entry))
        return false;
    if ((entry.sukemet_flags & (uint16_t)SUKEMET_FLAG_FORMAT) != 0u)
        return false;
    return (entry.sukemet_flags & (uint16_t)SUKEMET_FLAG_NO_MIRROR) == 0u;
}

bool suas_sukemet_is_rotatable(uint32_t codepoint)
{
    sukemet_hieroglyph_entry_t entry;

    if (!suas_sukemet_get_entry(codepoint, &entry))
        return false;
    if ((entry.sukemet_flags & (uint16_t)SUKEMET_FLAG_FORMAT) != 0u)
        return false;
    return (entry.sukemet_flags & (uint16_t)SUKEMET_FLAG_NO_ROTATE) == 0u;
}

static bool sukemet_class_is(uint32_t codepoint, uint8_t modifier_class)
{
    sukemet_metadata_t meta;

    if (!suas_sukemet_get_metadata(codepoint, &meta))
        return false;
    return meta.modifier_class == modifier_class;
}

bool suas_sukemet_is_shading(uint32_t codepoint)
{
    return sukemet_class_is(codepoint, (uint8_t)SUKEMET_MODIFIER_SHADING);
}

bool suas_sukemet_is_damage(uint32_t codepoint)
{
    return sukemet_class_is(codepoint, (uint8_t)SUKEMET_MODIFIER_DAMAGE);
}

/* Property queries (section 4) */

uint8_t suas_sukemet_gardiner_category(uint32_t codepoint)
{
    sukemet_hieroglyph_entry_t entry;

    if (!suas_sukemet_get_entry(codepoint, &entry))
        return (uint8_t)SUKEMET_GARDINER_NONE;
    return entry.gardiner_category;
}

uint8_t suas_sukemet_core_class(uint32_t codepoint)
{
    sukemet_metadata_t meta;

    if (!suas_sukemet_get_metadata(codepoint, &meta))
        return (uint8_t)SUKEMET_CORE_N;
    return meta.core_class;
}

uint8_t suas_sukemet_function_types(uint32_t codepoint)
{
    sukemet_metadata_t meta;

    if (!suas_sukemet_get_metadata(codepoint, &meta))
        return (uint8_t)SUKEMET_FUNC_NONE;
    return meta.function_types & (uint8_t)SUKEMET_FUNC_ASSIGNED_MASK;
}

uint8_t suas_sukemet_modifier_class(uint32_t codepoint)
{
    sukemet_metadata_t meta;

    if (!suas_sukemet_get_metadata(codepoint, &meta))
        return (uint8_t)SUKEMET_MODIFIER_NONE;
    return meta.modifier_class;
}

uint8_t suas_sukemet_enclosure_kind(uint32_t codepoint)
{
    sukemet_metadata_t meta;

    if (!suas_sukemet_get_metadata(codepoint, &meta))
        return (uint8_t)SUKEMET_ENCLOSURE_NONE;
    return meta.enclosure_kind;
}

bool suas_sukemet_catalog_index(uint32_t codepoint, uint32_t *out_index)
{
    sukemet_metadata_t meta;

    if (out_index == NULL || !suas_sukemet_get_metadata(codepoint, &meta))
        return false;
    if (meta.catalog_index == 0u)
        return false;
    *out_index = meta.catalog_index;
    return true;
}

bool suas_sukemet_catalog_of(uint32_t codepoint,
                             uint8_t *out_category,
                             uint8_t *out_subgroup,
                             uint16_t *out_item)
{
    sukemet_metadata_t meta;

    if (!suas_sukemet_get_metadata(codepoint, &meta))
        return false;
    if (meta.catalog_index == 0u)
        return false;
    if (out_category != NULL)
        *out_category = (uint8_t)((meta.catalog_index >> 24) & 0xFFu);
    if (out_subgroup != NULL)
        *out_subgroup = (uint8_t)((meta.catalog_index >> 16) & 0xFFu);
    if (out_item != NULL)
        *out_item = (uint16_t)(meta.catalog_index & 0xFFFFu);
    return true;
}

uint16_t suas_sukemet_source_mask(uint32_t codepoint)
{
    sukemet_metadata_t meta;

    if (!suas_sukemet_get_metadata(codepoint, &meta))
        return 0u;
    return meta.source_mask;
}

uint32_t suas_sukemet_description_hash(uint32_t codepoint)
{
    sukemet_metadata_t meta;

    if (!suas_sukemet_get_metadata(codepoint, &meta))
        return 0u;
    return meta.description_hash;
}

uint32_t suas_sukemet_unik_hash(uint32_t codepoint)
{
    sukemet_metadata_t meta;

    if (!suas_sukemet_get_metadata(codepoint, &meta))
        return 0u;
    return meta.unik_hash;
}

uint8_t suas_sukemet_alt_seq_length(uint32_t codepoint)
{
    sukemet_metadata_t meta;

    if (!suas_sukemet_get_metadata(codepoint, &meta))
        return 0u;
    return meta.alternate_length;
}

bool suas_sukemet_alt_seq_at(uint32_t codepoint,
                             uint8_t index,
                             uint32_t *out_codepoint)
{
    uint32_t table_index;
    sukemet_metadata_t meta;
    uint32_t offset;

    if (out_codepoint == NULL || !suas_sukemet_is_hieroglyph(codepoint))
        return false;
    table_index = sukemet_table_index(codepoint);
    if (table_index == (uint32_t)-1)
        return false;
    meta = sukemet_meta_table[table_index];
    if (index >= meta.alternate_length)
        return false;
    offset = sukemet_alt_offset[table_index];
    if (offset == 0xFFFFFFFFu || offset + (uint32_t)index >= SUKEMET_ALT_DATA_COUNT)
        return false;
    *out_codepoint = sukemet_alt_data[offset + (uint32_t)index];
    return true;
}

/* Enumeration */

uint32_t suas_sukemet_entry_count(void)
{
    return SUKEMET_TABLE_COUNT;
}

bool suas_sukemet_entry_at(uint32_t index,
                           sukemet_hieroglyph_entry_t *out_entry)
{
    if (out_entry == NULL || index >= SUKEMET_TABLE_COUNT)
        return false;
    *out_entry = sukemet_table[index];
    return true;
}

bool suas_sukemet_metadata_at(uint32_t index,
                              sukemet_metadata_t *out_metadata)
{
    if (out_metadata == NULL || index >= SUKEMET_TABLE_COUNT)
        return false;
    *out_metadata = sukemet_meta_table[index];
    return true;
}

/* Taxonomy names */

static const char *const sukemet_gardiner_letters[SUKEMET_GARDINER_COUNT] = {
    "A", "B", "C", "D", "E", "F", "G", "H", "I", "K",
    "L", "M", "N", "O", "P", "Q", "R", "S", "T", "U",
    "V", "W", "X", "Y", "Z", "AA"
};

static const char *const sukemet_gardiner_names[SUKEMET_GARDINER_COUNT] = {
    "Man and his occupations",
    "Woman and her occupations",
    "Anthropomorphic deities",
    "Parts of the human body",
    "Mammals",
    "Parts of mammals",
    "Birds",
    "Parts of birds",
    "Amphibious animals, reptiles, etc.",
    "Fishes and parts of fishes",
    "Invertebrates and lesser animals",
    "Trees and plants",
    "Sky, earth, water",
    "Buildings, parts of buildings, etc.",
    "Ships and parts of ships",
    "Domestic and funerary furniture",
    "Temple furniture and sacred emblems",
    "Crowns, dress, staves, etc.",
    "Warfare, hunting, butchery",
    "Agriculture, crafts and professions",
    "Rope, fibre, baskets, bags, etc.",
    "Vessels of stone and earthenware",
    "Loaves and cakes",
    "Writings, games, music",
    "Strokes, signs derived from hieratic, geometrical figures",
    "Unclassified"
};

const char* suas_sukemet_gardiner_letter(uint8_t category)
{
    if (category >= (uint8_t)SUKEMET_GARDINER_COUNT)
        return "?";
    return sukemet_gardiner_letters[category];
}

const char* suas_sukemet_gardiner_name(uint8_t category)
{
    if (category >= (uint8_t)SUKEMET_GARDINER_COUNT)
        return "Unknown";
    return sukemet_gardiner_names[category];
}

const char* suas_sukemet_core_name(uint8_t core_class)
{
    switch (core_class) {
    case SUKEMET_CORE_N: return "N";
    case SUKEMET_CORE_C: return "C";
    case SUKEMET_CORE_L: return "L";
    default: return "Unknown";
    }
}

const char* suas_sukemet_function_name(uint8_t function_type)
{
    switch (function_type) {
    case SUKEMET_FUNC_NONE: return "none";
    case SUKEMET_FUNC_PICTOGRAM: return "pictogram";
    case SUKEMET_FUNC_LOGOGRAM: return "logogram";
    case SUKEMET_FUNC_PHONOGRAM: return "phonogram";
    case SUKEMET_FUNC_CLASSIFIER: return "classifier";
    case SUKEMET_FUNC_PHONO_REPEATER: return "phono-repeater";
    case SUKEMET_FUNC_RADICOGRAM: return "radicogram";
    case SUKEMET_FUNC_INTERPRETANT: return "interpretant";
    case SUKEMET_FUNC_CONTROL: return "control";
    default: return "multiple";
    }
}

const char* suas_sukemet_modifier_class_name(uint8_t modifier_class)
{
    switch (modifier_class) {
    case SUKEMET_MODIFIER_NONE: return "none";
    case SUKEMET_MODIFIER_JOINER: return "joiner";
    case SUKEMET_MODIFIER_INSERTION: return "insertion";
    case SUKEMET_MODIFIER_OVERLAY: return "overlay";
    case SUKEMET_MODIFIER_SEGMENT: return "segment";
    case SUKEMET_MODIFIER_ENCLOSURE: return "enclosure";
    case SUKEMET_MODIFIER_MIRROR: return "mirror";
    case SUKEMET_MODIFIER_DAMAGE: return "damage";
    case SUKEMET_MODIFIER_SHADING: return "shading";
    default: return "unknown";
    }
}

const char* suas_sukemet_enclosure_name(uint8_t enclosure_kind)
{
    switch (enclosure_kind) {
    case SUKEMET_ENCLOSURE_NONE: return "none";
    case SUKEMET_ENCLOSURE_CARTOUCHE: return "cartouche";
    case SUKEMET_ENCLOSURE_SEREKH: return "serekh";
    case SUKEMET_ENCLOSURE_CUSTOM: return "custom";
    default: return "unknown";
    }
}

const char* suas_sukemet_source_name(uint16_t source_bit)
{
    switch (source_bit) {
    case SUKEMET_SOURCE_HG: return "hg";
    case SUKEMET_SOURCE_JSESH: return "jsesh";
    case SUKEMET_SOURCE_IFAO: return "ifao";
    case SUKEMET_SOURCE_UNIK: return "unik";
    case SUKEMET_SOURCE_CAT: return "cat";
    case SUKEMET_SOURCE_DESC: return "desc";
    case SUKEMET_SOURCE_FUNC: return "func";
    case SUKEMET_SOURCE_FVAL: return "fval";
    case SUKEMET_SOURCE_ALTSEQ: return "altseq";
    case SUKEMET_SOURCE_MIRROR: return "mirror";
    case SUKEMET_SOURCE_ROTATE: return "rotate";
    case 0u: return "none";
    default: return "combined";
    }
}

const char* suas_sukemet_block_name(sukemet_block_t block)
{
    switch (block) {
    case SUKEMET_BLOCK_NONE: return "none";
    case SUKEMET_BLOCK_EGYPTIAN: return "Egyptian Hieroglyphs";
    case SUKEMET_BLOCK_FORMAT_CONTROLS: return "Egyptian Hieroglyph Format Controls";
    case SUKEMET_BLOCK_EXTENDED_A: return "Egyptian Hieroglyphs Extended-A";
    case SUKEMET_BLOCK_NEOGRAPHY: return "Native & Neography Plane";
    default: return "unknown";
    }
}

const char* suas_sukemet_zone_name(suas_sukemet_zone_t zone)
{
    switch (zone) {
    case SUAS_SUKEMET_ZONE_BRIDGE: return "Bridge";
    case SUAS_SUKEMET_ZONE_SCP: return "System Control Plane";
    case SUAS_SUKEMET_ZONE_RNUR: return "Native & Neography Plane";
    case SUAS_SUKEMET_ZONE_NATIVE: return "Native";
    case SUAS_SUKEMET_ZONE_TRAP: return "Trap";
    case SUAS_SUKEMET_ZONE_SENTINEL: return "Sentinel";
    case SUAS_SUKEMET_ZONE_EXTSUCS: return "ExtSUCS";
    default: return "unknown";
    }
}

const char* suas_sukemet_registry_error_name(sukemet_registry_error_t error)
{
    switch (error) {
    case SUKEMET_REGISTRY_OK: return "ok";
    case SUKEMET_REGISTRY_NULL_TABLE: return "null table";
    case SUKEMET_REGISTRY_COUNT_OVER: return "count over capacity";
    case SUKEMET_REGISTRY_UNORDERED: return "unordered codepoints";
    case SUKEMET_REGISTRY_DUPLICATE_CODEPOINT: return "duplicate codepoint";
    case SUKEMET_REGISTRY_OUT_OF_SPACE: return "codepoint out of space";
    case SUKEMET_REGISTRY_INADMISSIBLE_CODEPOINT: return "inadmissible codepoint";
    case SUKEMET_REGISTRY_BAD_FLAGS: return "bad flags";
    case SUKEMET_REGISTRY_BAD_CATEGORY: return "bad Gardiner category";
    case SUKEMET_REGISTRY_GARDINER_MISMATCH: return "Gardiner flag mismatch";
    case SUKEMET_REGISTRY_CONTROL_MISMATCH: return "control flag mismatch";
    case SUKEMET_REGISTRY_NEOGRAPHY_MISMATCH: return "neography flag mismatch";
    case SUKEMET_REGISTRY_CORE_CONTRADICTION: return "core/legacy contradiction";
    default: return "unknown";
    }
}

/* Verification (section 9) */

static void sukemet_report(sukemet_registry_error_t *out_error,
                           sukemet_registry_error_t error)
{
    if (out_error != NULL)
        *out_error = error;
}

bool suas_sukemet_validate_table(const sukemet_hieroglyph_entry_t *entries,
                                 uint32_t count,
                                 sukemet_registry_error_t *out_error)
{
    uint32_t i;

    if (out_error != NULL)
        *out_error = SUKEMET_REGISTRY_OK;
    if (count == 0u)
        return true;
    if (entries == NULL) {
        sukemet_report(out_error, SUKEMET_REGISTRY_NULL_TABLE);
        return false;
    }
    if (count > SUAS_SUKEMET_REGISTRY_CAPACITY) {
        sukemet_report(out_error, SUKEMET_REGISTRY_COUNT_OVER);
        return false;
    }

    for (i = 0u; i < count; i++) {
        const sukemet_hieroglyph_entry_t *entry = &entries[i];
        uint16_t flags = entry->sukemet_flags;
        sukemet_block_t block;
        bool has_gardiner;

        if (entry->codepoint > (uint32_t)SUAS_CODEPOINT_MAX) {
            sukemet_report(out_error, SUKEMET_REGISTRY_OUT_OF_SPACE);
            return false;
        }
        block = suas_sukemet_block_of(entry->codepoint);
        if (block == SUKEMET_BLOCK_NONE ||
            !suas_sukemet_is_text_space(entry->codepoint)) {
            sukemet_report(out_error, SUKEMET_REGISTRY_INADMISSIBLE_CODEPOINT);
            return false;
        }
        if ((flags & (uint16_t)SUKEMET_FLAG_RESERVED_MASK) != 0u) {
            sukemet_report(out_error, SUKEMET_REGISTRY_BAD_FLAGS);
            return false;
        }
        if ((flags & (uint16_t)SUKEMET_FLAG_CORE) != 0u &&
            (flags & (uint16_t)SUKEMET_FLAG_LEGACY) != 0u) {
            sukemet_report(out_error, SUKEMET_REGISTRY_CORE_CONTRADICTION);
            return false;
        }
        if (entry->gardiner_category != (uint8_t)SUKEMET_GARDINER_NONE &&
            entry->gardiner_category >= (uint8_t)SUKEMET_GARDINER_COUNT) {
            sukemet_report(out_error, SUKEMET_REGISTRY_BAD_CATEGORY);
            return false;
        }
        has_gardiner = entry->gardiner_category != (uint8_t)SUKEMET_GARDINER_NONE;
        if (((flags & (uint16_t)SUKEMET_FLAG_GARDINER) != 0u) != has_gardiner) {
            sukemet_report(out_error, SUKEMET_REGISTRY_GARDINER_MISMATCH);
            return false;
        }
        if ((flags & (uint16_t)SUKEMET_FLAG_FORMAT) != 0u) {
            if (block != SUKEMET_BLOCK_FORMAT_CONTROLS) {
                sukemet_report(out_error, SUKEMET_REGISTRY_CONTROL_MISMATCH);
                return false;
            }
            if ((flags & (uint16_t)SUKEMET_FLAG_MODIFIER) == 0u) {
                sukemet_report(out_error, SUKEMET_REGISTRY_BAD_FLAGS);
                return false;
            }
        }
        if (((flags & (uint16_t)SUKEMET_FLAG_NEOGRAPHY) != 0u) !=
            (block == SUKEMET_BLOCK_NEOGRAPHY)) {
            sukemet_report(out_error, SUKEMET_REGISTRY_NEOGRAPHY_MISMATCH);
            return false;
        }
        if (i > 0u) {
            if (entry->codepoint == entries[i - 1u].codepoint) {
                sukemet_report(out_error, SUKEMET_REGISTRY_DUPLICATE_CODEPOINT);
                return false;
            }
            if (entry->codepoint < entries[i - 1u].codepoint) {
                sukemet_report(out_error, SUKEMET_REGISTRY_UNORDERED);
                return false;
            }
        }
    }
    return true;
}

/* Version */

const char* suas_sukemet_version_string(void)
{
    return "1.0";
}
