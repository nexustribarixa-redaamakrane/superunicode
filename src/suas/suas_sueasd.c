/* SUAS-017 - SuperUnicode East Asian Script Data (SUEASD)
 *
 * Freestanding C99, zero allocation. The engine is a static array of
 * sueasd_script_entry_t records, built at compile time from the Scripts.txt
 * classification and the four UAX #60 source files of Unicode 18.0.0, plus a
 * compact classification range table, a fixed open-addressing index over the
 * curated codepoints, and a parallel metadata table.
 *
 * The 12-byte sketch record carries identity and classification. The UAX #60
 * ancillary fields - source mask, radical-stroke counts, numeric value,
 * reading/source/aux hashes and the modern-CJK equivalence - live in the
 * parallel metadata table, aligned one-to-one with the main records.
 *
 * No part of this file reaches for malloc, stdio or <string.h>.
 */

#include "suas/suas_sueasd.h"

/* Freestanding primitives */

#define SUEASD_STATIC_ASSERT(cond, name) \
    typedef char name[(cond) ? 1 : -1]

SUEASD_STATIC_ASSERT(sizeof(sueasd_script_entry_t) == 12u,
                     sueasd_entry_is_12_bytes);
SUEASD_STATIC_ASSERT(offsetof(sueasd_script_entry_t, codepoint) == 0u,
                     sueasd_entry_codepoint_at_0);
SUEASD_STATIC_ASSERT(offsetof(sueasd_script_entry_t, script_mask) == 4u,
                     sueasd_entry_mask_at_4);
SUEASD_STATIC_ASSERT(offsetof(sueasd_script_entry_t, primary_script) == 8u,
                     sueasd_entry_script_at_8);
SUEASD_STATIC_ASSERT(offsetof(sueasd_script_entry_t, source_region) == 10u,
                     sueasd_entry_region_at_10);
SUEASD_STATIC_ASSERT(offsetof(sueasd_script_entry_t, reserved) == 11u,
                     sueasd_entry_reserved_at_11);
SUEASD_STATIC_ASSERT(sizeof(sueasd_metadata_t) == 24u,
                     sueasd_metadata_is_24_bytes);
SUEASD_STATIC_ASSERT(sizeof(sueasd_range_t) == 12u,
                     sueasd_range_is_12_bytes);

/* FNV-1a primitives */

#define SUEASD_FNV_OFFSET 2166136261u
#define SUEASD_FNV_PRIME  16777619u

static uint8_t sueasd_byte_at(uint32_t codepoint, unsigned index)
{
    return (uint8_t)((codepoint >> (index * 8u)) & 0xFFu);
}

uint32_t suas_sueasd_hash(uint32_t codepoint)
{
    uint32_t h = SUEASD_FNV_OFFSET;
    unsigned i;

    for (i = 0u; i < 4u; i++) {
        h ^= (uint32_t)sueasd_byte_at(codepoint, i);
        h *= SUEASD_FNV_PRIME;
    }
    return h;
}

uint32_t suas_sueasd_hash_bytes(const char *bytes, size_t length)
{
    uint32_t h = SUEASD_FNV_OFFSET;
    size_t i;

    if (bytes == NULL)
        return 0u;
    for (i = 0u; i < length; i++) {
        h ^= (uint32_t)(uint8_t)bytes[i];
        h *= SUEASD_FNV_PRIME;
    }
    return h;
}

/* Generated classification and ancillary data (see suas_sueasd_data.inc) */

#include "suas_sueasd_data.inc"

SUEASD_STATIC_ASSERT(SUEASD_TABLE_COUNT <= SUAS_SUEASD_REGISTRY_CAPACITY,
                     sueasd_table_fits_capacity);
SUEASD_STATIC_ASSERT(SUEASD_TABLE_COUNT <= 65534u,
                     sueasd_table_fits_index_slot);
SUEASD_STATIC_ASSERT(SUEASD_RANGE_COUNT <= SUAS_SUEASD_RANGE_CAPACITY,
                     sueasd_range_fits_capacity);
SUEASD_STATIC_ASSERT(SUAS_SUEASD_INDEX_SIZE > SUAS_SUEASD_REGISTRY_CAPACITY,
                     sueasd_index_has_slack);
SUEASD_STATIC_ASSERT((SUAS_SUEASD_INDEX_SIZE & (SUAS_SUEASD_INDEX_SIZE - 1u)) == 0u,
                     sueasd_index_is_power_of_two);
SUEASD_STATIC_ASSERT(sizeof sueasd_meta_table / sizeof sueasd_meta_table[0]
                     == SUEASD_TABLE_COUNT,
                     sueasd_meta_table_aligned);

/* Static lookup index over the curated codepoints */

typedef struct {
    uint16_t entry_plus_one; /* 0 = empty, otherwise table index + 1 */
} sueasd_index_slot_t;

static sueasd_index_slot_t sueasd_index[SUAS_SUEASD_INDEX_SIZE];
static bool sueasd_ready = false;

static void sueasd_build(void)
{
    uint32_t i;
    uint32_t h;
    uint32_t start;

    for (i = 0u; i < SUAS_SUEASD_INDEX_SIZE; i++)
        sueasd_index[i].entry_plus_one = 0u;

    for (i = 0u; i < SUEASD_TABLE_COUNT; i++) {
        h = suas_sueasd_hash(sueasd_table[i].codepoint) &
            (SUAS_SUEASD_INDEX_SIZE - 1u);
        start = h;
        while (sueasd_index[h].entry_plus_one != 0u) {
            h = (h + 1u) & (SUAS_SUEASD_INDEX_SIZE - 1u);
            if (h == start)
                break; /* unreachable: capacity < index size */
        }
        sueasd_index[h].entry_plus_one = (uint16_t)(i + 1u);
    }

    sueasd_ready = true;
}

void suas_sueasd_init_registry(void)
{
    if (!sueasd_ready)
        sueasd_build();
}

/* O(1) probe over the curated table. Returns the table index, or
 * (uint32_t)-1 when absent. */
static uint32_t sueasd_table_index(uint32_t codepoint)
{
    uint32_t h;
    uint32_t start;

    if (!sueasd_ready)
        sueasd_build();

    h = suas_sueasd_hash(codepoint) & (SUAS_SUEASD_INDEX_SIZE - 1u);
    start = h;
    for (;;) {
        uint32_t slot = sueasd_index[h].entry_plus_one;
        if (slot == 0u)
            return (uint32_t)-1;
        if (sueasd_table[slot - 1u].codepoint == codepoint)
            return slot - 1u;
        h = (h + 1u) & (SUAS_SUEASD_INDEX_SIZE - 1u);
        if (h == start)
            return (uint32_t)-1;
    }
}

/* Zone dispatch (section 2) */

suas_sueasd_zone_t suas_sueasd_zone_of(uint32_t codepoint)
{
    if (codepoint > (uint32_t)SUAS_CODEPOINT_MAX)
        return SUAS_SUEASD_ZONE_EXTSUCS;
    if (codepoint <= SUAS_SUEASD_LIMIT_UNICODE_MAX)
        return SUAS_SUEASD_ZONE_BRIDGE;
    if (codepoint >= SUAS_SUEASD_LIMIT_SCP_MIN &&
        codepoint <= SUAS_SUEASD_LIMIT_SCP_MAX)
        return SUAS_SUEASD_ZONE_SCP;
    if (codepoint >= SUAS_SUEASD_RNUR_MIN &&
        codepoint <= SUAS_SUEASD_RNUR_MAX)
        return SUAS_SUEASD_ZONE_RNUR;
    if (codepoint >= SUAS_SUEASD_LIMIT_TRAP_MIN &&
        codepoint <= SUAS_SUEASD_LIMIT_TRAP_MAX)
        return SUAS_SUEASD_ZONE_TRAP;
    if (codepoint == SUAS_SUEASD_LIMIT_SENTINEL)
        return SUAS_SUEASD_ZONE_SENTINEL;
    return SUAS_SUEASD_ZONE_NATIVE;
}

bool suas_sueasd_is_text_space(uint32_t codepoint)
{
    switch (suas_sueasd_zone_of(codepoint)) {
    case SUAS_SUEASD_ZONE_BRIDGE:
    case SUAS_SUEASD_ZONE_RNUR:
    case SUAS_SUEASD_ZONE_NATIVE:
        return true;
    default:
        return false;
    }
}

bool suas_sueasd_is_neography(uint32_t codepoint)
{
    return suas_sueasd_zone_of(codepoint) == SUAS_SUEASD_ZONE_RNUR;
}

bool suas_sueasd_neography_set(uint32_t codepoint, uint32_t *out_set)
{
    if (out_set == NULL || !suas_sueasd_is_neography(codepoint))
        return false;
    *out_set = (codepoint - SUAS_SUEASD_RNUR_MIN) >> SUAS_SUEASD_RNUR_SET_SHIFT;
    return true;
}

/* Classification range resolution (section 4.1) */

static const sueasd_range_t *sueasd_range_of(uint32_t codepoint)
{
    uint32_t i;

    for (i = 0u; i < SUEASD_RANGE_COUNT; i++) {
        if (codepoint < sueasd_range_table[i].first)
            return NULL;
        if (codepoint <= sueasd_range_table[i].last)
            return &sueasd_range_table[i];
    }
    return NULL;
}

bool suas_sueasd_is_east_asian(uint32_t codepoint)
{
    if (!suas_sueasd_is_text_space(codepoint))
        return false;
    if (sueasd_range_of(codepoint) != NULL)
        return true;
    return suas_sueasd_is_neography(codepoint);
}

/* Algorithmic default (section 5.2) */

static void sueasd_default(uint32_t codepoint, sueasd_script_entry_t *out_entry)
{
    out_entry->codepoint = codepoint;
    out_entry->script_mask = suas_sueasd_is_neography(codepoint)
                                 ? (uint32_t)SUEASD_FLAG_CUSTOM_SCRIPT
                                 : 0u;
    out_entry->primary_script = (uint16_t)SUEASD_SCRIPT_UNKNOWN;
    out_entry->source_region = (uint8_t)SUEASD_REGION_NONE;
    out_entry->reserved = 0u;
}

/* The region a script belongs to. */

static uint8_t sueasd_region_for_script(uint16_t script)
{
    switch (script) {
    case SUEASD_SCRIPT_HAN:
    case SUEASD_SCRIPT_BOPOMOFO:
        return (uint8_t)SUEASD_REGION_CHINA;
    case SUEASD_SCRIPT_HANGUL:
        return (uint8_t)SUEASD_REGION_KOREA;
    case SUEASD_SCRIPT_HIRAGANA:
    case SUEASD_SCRIPT_KATAKANA:
        return (uint8_t)SUEASD_REGION_JAPAN;
    case SUEASD_SCRIPT_JURCHEN:
        return (uint8_t)SUEASD_REGION_JURCHEN;
    case SUEASD_SCRIPT_NUSHU:
        return (uint8_t)SUEASD_REGION_NUSHU;
    case SUEASD_SCRIPT_SEAL:
        return (uint8_t)SUEASD_REGION_SEAL;
    case SUEASD_SCRIPT_TANGUT:
        return (uint8_t)SUEASD_REGION_TANGUT;
    case SUEASD_SCRIPT_KHITAN_SMALL:
        return (uint8_t)SUEASD_REGION_KHITAN;
    default:
        return (uint8_t)SUEASD_REGION_NONE;
    }
}

/* Core query (section 3.4) */

bool suas_sueasd_get_entry(uint32_t codepoint, sueasd_script_entry_t *out_entry)
{
    const sueasd_range_t *range;
    uint32_t index;

    if (out_entry == NULL || !suas_sueasd_is_text_space(codepoint))
        return false;

    range = sueasd_range_of(codepoint);
    if (range == NULL) {
        sueasd_default(codepoint, out_entry);
        return true;
    }

    index = sueasd_table_index(codepoint);
    if (index != (uint32_t)-1) {
        *out_entry = sueasd_table[index];
        return true;
    }

    /* Synthesize the base record from the classification range. */
    out_entry->codepoint = codepoint;
    out_entry->script_mask = (uint32_t)range->base_flags;
    out_entry->primary_script = range->primary_script;
    out_entry->source_region =
        (uint8_t)sueasd_region_for_script(range->primary_script);
    out_entry->reserved = 0u;
    return true;
}

bool suas_sueasd_get_metadata(uint32_t codepoint, sueasd_metadata_t *out_metadata)
{
    uint32_t index;

    if (out_metadata == NULL || !suas_sueasd_is_text_space(codepoint))
        return false;

    index = sueasd_table_index(codepoint);
    if (index == (uint32_t)-1) {
        uint32_t i;
        uint8_t *bytes = (uint8_t *)out_metadata;
        for (i = 0u; i < (uint32_t)sizeof *out_metadata; i++)
            bytes[i] = 0u;
        return true;
    }

    *out_metadata = sueasd_meta_table[index];
    return true;
}

bool suas_sueasd_is_curated(uint32_t codepoint)
{
    if (!suas_sueasd_is_text_space(codepoint))
        return false;
    return sueasd_table_index(codepoint) != (uint32_t)-1;
}

/* Classification predicates (section 6) */

static bool sueasd_flag(uint32_t codepoint, uint32_t flag)
{
    sueasd_script_entry_t entry;

    if (!suas_sueasd_get_entry(codepoint, &entry))
        return false;
    return (entry.script_mask & flag) != 0u;
}

bool suas_sueasd_is_ideographic(uint32_t codepoint)
{
    return sueasd_flag(codepoint, (uint32_t)SUEASD_FLAG_IDEOGRAPHIC);
}

bool suas_sueasd_is_radical(uint32_t codepoint)
{
    return sueasd_flag(codepoint, (uint32_t)SUEASD_FLAG_RADICAL);
}

bool suas_sueasd_is_compatibility(uint32_t codepoint)
{
    return sueasd_flag(codepoint, (uint32_t)SUEASD_FLAG_COMPATIBILITY);
}

bool suas_sueasd_is_custom_script(uint32_t codepoint)
{
    return sueasd_flag(codepoint, (uint32_t)SUEASD_FLAG_CUSTOM_SCRIPT);
}

bool suas_sueasd_is_kana(uint32_t codepoint)
{
    return sueasd_flag(codepoint, (uint32_t)SUEASD_FLAG_KANA);
}

bool suas_sueasd_is_jamo(uint32_t codepoint)
{
    return sueasd_flag(codepoint, (uint32_t)SUEASD_FLAG_JAMO);
}

bool suas_sueasd_is_syllable(uint32_t codepoint)
{
    return sueasd_flag(codepoint, (uint32_t)SUEASD_FLAG_SYLLABLE);
}

bool suas_sueasd_has_source_data(uint32_t codepoint)
{
    return sueasd_flag(codepoint, (uint32_t)SUEASD_FLAG_SOURCE_DATA);
}

/* Property queries (section 4) */

uint16_t suas_sueasd_primary_script(uint32_t codepoint)
{
    sueasd_script_entry_t entry;

    if (!suas_sueasd_get_entry(codepoint, &entry))
        return (uint16_t)SUEASD_SCRIPT_UNKNOWN;
    return entry.primary_script;
}

uint32_t suas_sueasd_script_mask(uint32_t codepoint)
{
    sueasd_script_entry_t entry;

    if (!suas_sueasd_get_entry(codepoint, &entry))
        return 0u;
    return entry.script_mask;
}

uint8_t suas_sueasd_source_region(uint32_t codepoint)
{
    sueasd_script_entry_t entry;

    if (!suas_sueasd_get_entry(codepoint, &entry))
        return (uint8_t)SUEASD_REGION_NONE;
    return entry.source_region;
}

uint16_t suas_sueasd_source_mask(uint32_t codepoint)
{
    sueasd_metadata_t meta;

    if (!suas_sueasd_get_metadata(codepoint, &meta))
        return 0u;
    return meta.source_mask;
}

uint16_t suas_sueasd_radical(uint32_t codepoint)
{
    sueasd_metadata_t meta;

    if (!suas_sueasd_get_metadata(codepoint, &meta))
        return 0u;
    return meta.radical;
}

uint8_t suas_sueasd_stroke_count(uint32_t codepoint)
{
    sueasd_metadata_t meta;

    if (!suas_sueasd_get_metadata(codepoint, &meta))
        return 0u;
    return meta.stroke_count;
}

uint16_t suas_sueasd_numeric_value(uint32_t codepoint)
{
    sueasd_metadata_t meta;

    if (!suas_sueasd_get_metadata(codepoint, &meta))
        return 0u;
    return meta.numeric_value;
}

uint8_t suas_sueasd_source_count(uint32_t codepoint)
{
    sueasd_metadata_t meta;

    if (!suas_sueasd_get_metadata(codepoint, &meta))
        return 0u;
    return meta.source_count;
}

uint32_t suas_sueasd_modern_cjk(uint32_t codepoint)
{
    sueasd_metadata_t meta;

    if (!suas_sueasd_get_metadata(codepoint, &meta))
        return 0u;
    return meta.modern_cjk;
}

uint32_t suas_sueasd_reading_hash(uint32_t codepoint)
{
    sueasd_metadata_t meta;

    if (!suas_sueasd_get_metadata(codepoint, &meta))
        return 0u;
    return meta.reading_hash;
}

uint32_t suas_sueasd_source_hash(uint32_t codepoint)
{
    sueasd_metadata_t meta;

    if (!suas_sueasd_get_metadata(codepoint, &meta))
        return 0u;
    return meta.source_hash;
}

uint32_t suas_sueasd_aux_hash(uint32_t codepoint)
{
    sueasd_metadata_t meta;

    if (!suas_sueasd_get_metadata(codepoint, &meta))
        return 0u;
    return meta.aux_hash;
}

/* Enumeration */

uint32_t suas_sueasd_entry_count(void)
{
    return SUEASD_TABLE_COUNT;
}

bool suas_sueasd_entry_at(uint32_t index, sueasd_script_entry_t *out_entry)
{
    if (out_entry == NULL || index >= SUEASD_TABLE_COUNT)
        return false;
    *out_entry = sueasd_table[index];
    return true;
}

bool suas_sueasd_metadata_at(uint32_t index, sueasd_metadata_t *out_metadata)
{
    if (out_metadata == NULL || index >= SUEASD_TABLE_COUNT)
        return false;
    *out_metadata = sueasd_meta_table[index];
    return true;
}

uint32_t suas_sueasd_range_count(void)
{
    return SUEASD_RANGE_COUNT;
}

bool suas_sueasd_range_at(uint32_t index, sueasd_range_t *out_range)
{
    if (out_range == NULL || index >= SUEASD_RANGE_COUNT)
        return false;
    *out_range = sueasd_range_table[index];
    return true;
}

/* Taxonomy names */

const char* suas_sueasd_script_name(uint16_t script)
{
    switch (script) {
    case SUEASD_SCRIPT_UNKNOWN: return "Unknown";
    case SUEASD_SCRIPT_HAN: return "Han";
    case SUEASD_SCRIPT_HANGUL: return "Hangul";
    case SUEASD_SCRIPT_HIRAGANA: return "Hiragana";
    case SUEASD_SCRIPT_KATAKANA: return "Katakana";
    case SUEASD_SCRIPT_BOPOMOFO: return "Bopomofo";
    case SUEASD_SCRIPT_JURCHEN: return "Jurchen";
    case SUEASD_SCRIPT_NUSHU: return "Nushu";
    case SUEASD_SCRIPT_SEAL: return "Small Seal";
    case SUEASD_SCRIPT_TANGUT: return "Tangut";
    case SUEASD_SCRIPT_KHITAN_SMALL: return "Khitan Small Script";
    default: return "Unknown";
    }
}

const char* suas_sueasd_script_short_name(uint16_t script)
{
    switch (script) {
    case SUEASD_SCRIPT_UNKNOWN: return "Zzzz";
    case SUEASD_SCRIPT_HAN: return "Hani";
    case SUEASD_SCRIPT_HANGUL: return "Hang";
    case SUEASD_SCRIPT_HIRAGANA: return "Hira";
    case SUEASD_SCRIPT_KATAKANA: return "Kana";
    case SUEASD_SCRIPT_BOPOMOFO: return "Bopo";
    case SUEASD_SCRIPT_JURCHEN: return "Jurc";
    case SUEASD_SCRIPT_NUSHU: return "Nshu";
    case SUEASD_SCRIPT_SEAL: return "Seal";
    case SUEASD_SCRIPT_TANGUT: return "Tang";
    case SUEASD_SCRIPT_KHITAN_SMALL: return "Kits";
    default: return "Zzzz";
    }
}

const char* suas_sueasd_region_name(uint8_t region)
{
    switch (region) {
    case SUEASD_REGION_NONE: return "none";
    case SUEASD_REGION_CHINA: return "China";
    case SUEASD_REGION_KOREA: return "Korea";
    case SUEASD_REGION_JAPAN: return "Japan";
    case SUEASD_REGION_JURCHEN: return "Jurchen";
    case SUEASD_REGION_NUSHU: return "Nushu";
    case SUEASD_REGION_SEAL: return "Seal";
    case SUEASD_REGION_TANGUT: return "Tangut";
    case SUEASD_REGION_KHITAN: return "Khitan";
    default: return "unknown";
    }
}

const char* suas_sueasd_source_name(uint16_t source_bit)
{
    switch (source_bit) {
    case SUEASD_SOURCE_JURC_NCREADING: return "jurc_ncreading";
    case SUEASD_SOURCE_JURC_NUMERIC: return "jurc_numeric";
    case SUEASD_SOURCE_JURC_RSUNICODE: return "jurc_rsunicode";
    case SUEASD_SOURCE_JURC_SRC: return "jurc_src";
    case SUEASD_SOURCE_NSHU_DUBENSRC: return "nshu_dubensrc";
    case SUEASD_SOURCE_NSHU_READING: return "nshu_reading";
    case SUEASD_SOURCE_SEAL_CCZSRC: return "seal_cczsrc";
    case SUEASD_SOURCE_SEAL_DYCSRC: return "seal_dycsrc";
    case SUEASD_SOURCE_SEAL_MCJK: return "seal_mcjk";
    case SUEASD_SOURCE_SEAL_QJZSRC: return "seal_qjzsrc";
    case SUEASD_SOURCE_SEAL_RAD: return "seal_rad";
    case SUEASD_SOURCE_SEAL_THXSRC: return "seal_thxsrc";
    case SUEASD_SOURCE_TGT_MERGEDSRC: return "tgt_mergedsrc";
    case SUEASD_SOURCE_TGT_NUMERIC: return "tgt_numeric";
    case SUEASD_SOURCE_TGT_RSUNICODE: return "tgt_rsunicode";
    case SUEASD_SOURCE_HAN: return "han";
    case 0u: return "none";
    default: return "combined";
    }
}

const char* suas_sueasd_zone_name(suas_sueasd_zone_t zone)
{
    switch (zone) {
    case SUAS_SUEASD_ZONE_BRIDGE: return "Bridge";
    case SUAS_SUEASD_ZONE_SCP: return "System Control Plane";
    case SUAS_SUEASD_ZONE_RNUR: return "Native & Neography Plane";
    case SUAS_SUEASD_ZONE_NATIVE: return "Native";
    case SUAS_SUEASD_ZONE_TRAP: return "Trap";
    case SUAS_SUEASD_ZONE_SENTINEL: return "Sentinel";
    case SUAS_SUEASD_ZONE_EXTSUCS: return "ExtSUCS";
    default: return "unknown";
    }
}

const char* suas_sueasd_registry_error_name(sueasd_registry_error_t error)
{
    switch (error) {
    case SUEASD_REGISTRY_OK: return "ok";
    case SUEASD_REGISTRY_NULL_TABLE: return "null table";
    case SUEASD_REGISTRY_COUNT_OVER: return "count over capacity";
    case SUEASD_REGISTRY_UNORDERED: return "unordered codepoints";
    case SUEASD_REGISTRY_DUPLICATE_CODEPOINT: return "duplicate codepoint";
    case SUEASD_REGISTRY_OUT_OF_SPACE: return "codepoint out of space";
    case SUEASD_REGISTRY_INADMISSIBLE_CODEPOINT: return "inadmissible codepoint";
    case SUEASD_REGISTRY_BAD_SCRIPT: return "bad script id";
    case SUEASD_REGISTRY_BAD_REGION: return "bad region id";
    case SUEASD_REGISTRY_BAD_RESERVED: return "reserved byte not zero";
    case SUEASD_REGISTRY_IDEOGRAPH_MISMATCH: return "ideographic flag mismatch";
    case SUEASD_REGISTRY_KANA_MISMATCH: return "kana flag mismatch";
    case SUEASD_REGISTRY_JAMO_MISMATCH: return "jamo/syllable mismatch";
    case SUEASD_REGISTRY_RANGE_UNORDERED: return "unordered ranges";
    case SUEASD_REGISTRY_RANGE_OUT_OF_SPACE: return "range out of space";
    default: return "unknown";
    }
}

/* Verification (section 8) */

static void sueasd_report(sueasd_registry_error_t *out_error,
                          sueasd_registry_error_t error)
{
    if (out_error != NULL)
        *out_error = error;
}

bool suas_sueasd_validate_table(const sueasd_script_entry_t *entries,
                                uint32_t count,
                                sueasd_registry_error_t *out_error)
{
    uint32_t i;

    if (out_error != NULL)
        *out_error = SUEASD_REGISTRY_OK;
    if (count == 0u)
        return true;
    if (entries == NULL) {
        sueasd_report(out_error, SUEASD_REGISTRY_NULL_TABLE);
        return false;
    }
    if (count > SUAS_SUEASD_REGISTRY_CAPACITY) {
        sueasd_report(out_error, SUEASD_REGISTRY_COUNT_OVER);
        return false;
    }

    for (i = 0u; i < count; i++) {
        const sueasd_script_entry_t *entry = &entries[i];
        uint32_t flags = entry->script_mask;
        bool ideographic;

        if (entry->codepoint > (uint32_t)SUAS_CODEPOINT_MAX) {
            sueasd_report(out_error, SUEASD_REGISTRY_OUT_OF_SPACE);
            return false;
        }
        if (!suas_sueasd_is_text_space(entry->codepoint) ||
            !suas_sueasd_is_east_asian(entry->codepoint)) {
            sueasd_report(out_error, SUEASD_REGISTRY_INADMISSIBLE_CODEPOINT);
            return false;
        }
        if (entry->primary_script >= (uint16_t)SUEASD_SCRIPT_COUNT) {
            sueasd_report(out_error, SUEASD_REGISTRY_BAD_SCRIPT);
            return false;
        }
        if (entry->source_region > (uint8_t)SUEASD_REGION_KHITAN) {
            sueasd_report(out_error, SUEASD_REGISTRY_BAD_REGION);
            return false;
        }
        if (entry->reserved != 0u) {
            sueasd_report(out_error, SUEASD_REGISTRY_BAD_RESERVED);
            return false;
        }
        ideographic = (flags & (uint32_t)SUEASD_FLAG_IDEOGRAPHIC) != 0u;
        if (ideographic &&
            entry->primary_script != (uint16_t)SUEASD_SCRIPT_HAN &&
            entry->primary_script != (uint16_t)SUEASD_SCRIPT_BOPOMOFO &&
            entry->primary_script != (uint16_t)SUEASD_SCRIPT_SEAL) {
            sueasd_report(out_error, SUEASD_REGISTRY_IDEOGRAPH_MISMATCH);
            return false;
        }
        if ((flags & (uint32_t)SUEASD_FLAG_KANA) != 0u &&
            entry->primary_script != (uint16_t)SUEASD_SCRIPT_HIRAGANA &&
            entry->primary_script != (uint16_t)SUEASD_SCRIPT_KATAKANA) {
            sueasd_report(out_error, SUEASD_REGISTRY_KANA_MISMATCH);
            return false;
        }
        if ((flags & (uint32_t)SUEASD_FLAG_JAMO) != 0u &&
            (flags & (uint32_t)SUEASD_FLAG_SYLLABLE) != 0u) {
            sueasd_report(out_error, SUEASD_REGISTRY_JAMO_MISMATCH);
            return false;
        }
        if (i > 0u) {
            if (entry->codepoint == entries[i - 1u].codepoint) {
                sueasd_report(out_error, SUEASD_REGISTRY_DUPLICATE_CODEPOINT);
                return false;
            }
            if (entry->codepoint < entries[i - 1u].codepoint) {
                sueasd_report(out_error, SUEASD_REGISTRY_UNORDERED);
                return false;
            }
        }
    }
    return true;
}

bool suas_sueasd_validate_ranges(const sueasd_range_t *ranges,
                                 uint32_t count,
                                 sueasd_registry_error_t *out_error)
{
    uint32_t i;

    if (out_error != NULL)
        *out_error = SUEASD_REGISTRY_OK;
    if (count == 0u)
        return true;
    if (ranges == NULL) {
        sueasd_report(out_error, SUEASD_REGISTRY_NULL_TABLE);
        return false;
    }
    if (count > SUAS_SUEASD_RANGE_CAPACITY) {
        sueasd_report(out_error, SUEASD_REGISTRY_COUNT_OVER);
        return false;
    }

    for (i = 0u; i < count; i++) {
        const sueasd_range_t *range = &ranges[i];

        if (range->last > (uint32_t)SUAS_CODEPOINT_MAX ||
            range->first > (uint32_t)SUAS_CODEPOINT_MAX) {
            sueasd_report(out_error, SUEASD_REGISTRY_RANGE_OUT_OF_SPACE);
            return false;
        }
        if (range->first > range->last) {
            sueasd_report(out_error, SUEASD_REGISTRY_RANGE_UNORDERED);
            return false;
        }
        if (range->primary_script >= (uint16_t)SUEASD_SCRIPT_COUNT) {
            sueasd_report(out_error, SUEASD_REGISTRY_BAD_SCRIPT);
            return false;
        }
        if (i > 0u && range->first <= ranges[i - 1u].last) {
            sueasd_report(out_error, SUEASD_REGISTRY_RANGE_UNORDERED);
            return false;
        }
    }
    return true;
}

/* Version */

const char* suas_sueasd_version_string(void)
{
    return "1.0";
}
