/* SUAS-013 — System Ideographic Source Registry (SISR)
 *
 * Reference implementation of docs/suas/SUAS-013-sisr.md.
 *
 * Freestanding C99, zero allocation. A static table of curated source records
 * and a static table of variant links are indexed once into fixed-size
 * open-addressing arrays, so that an external source identity, its registry
 * class, and its canonical fallback are all resolved in O(1) expected time.
 * No heap, no dynamic table growth, no text parsing.
 *
 * The runtime layer is fed at build time from the offline reference layer (the
 * SuperUnicode Character Database and the published /Public provenance
 * manifests); the tables below are a curated subset of that dataset. Extending
 * a table is a data update (a minor revision) and never changes the API.
 */

#include "suas/suas_sisr.h"

/* ── Portable byte extraction ─────────────────────────────────────── */

static uint8_t sisr_byte_at(uint32_t cp, unsigned i)
{
    return (uint8_t)((cp >> (8u * i)) & 0xFFu);
}

/* ── Compile-time record geometry (§3.1, §3.3) ────────────────────── */

#define SISR_STATIC_ASSERT(cond, name) \
    typedef char name[(cond) ? 1 : -1]

SISR_STATIC_ASSERT(sizeof(sisr_source_entry_t) == 16u,
                   sisr_source_entry_is_16_bytes);
SISR_STATIC_ASSERT(offsetof(sisr_source_entry_t, codepoint) == 0u,
                   sisr_entry_codepoint_at_0);
SISR_STATIC_ASSERT(offsetof(sisr_source_entry_t, source_serial) == 4u,
                   sisr_entry_serial_at_4);
SISR_STATIC_ASSERT(offsetof(sisr_source_entry_t, source_id_hash) == 8u,
                   sisr_entry_hash_at_8);
SISR_STATIC_ASSERT(offsetof(sisr_source_entry_t, registry_flags) == 12u,
                   sisr_entry_flags_at_12);
SISR_STATIC_ASSERT(offsetof(sisr_source_entry_t, source_type) == 14u,
                   sisr_entry_type_at_14);
SISR_STATIC_ASSERT(offsetof(sisr_source_entry_t, reserved) == 15u,
                   sisr_entry_reserved_at_15);

SISR_STATIC_ASSERT(sizeof(sisr_variant_link_t) == 12u,
                   sisr_link_is_12_bytes);
SISR_STATIC_ASSERT(offsetof(sisr_variant_link_t, codepoint) == 0u,
                   sisr_link_codepoint_at_0);
SISR_STATIC_ASSERT(offsetof(sisr_variant_link_t, fallback_codepoint) == 4u,
                   sisr_link_fallback_at_4);
SISR_STATIC_ASSERT(offsetof(sisr_variant_link_t, link_flags) == 8u,
                   sisr_link_flags_at_8);
SISR_STATIC_ASSERT(offsetof(sisr_variant_link_t, reserved) == 10u,
                   sisr_link_reserved_at_10);

/* ── The curated static registry (§8) ─────────────────────────────── */

/* Ascending codepoint order is a verification invariant, not a convenience:
 * the enumeration surface, the duplicate check and the compiler's partition
 * proof all depend on it. The `source_id_hash` of each row is FNV-1a over its
 * canonical token; `suas_sisr_validate_table` recomputes and rejects a stale
 * value. */
static const sisr_source_entry_t sisr_table[] = {
    /* codepoint   serial       token hash    registry_flags                         type                   rsv */
    { 0x00003402u, 0x00000937u, 0x3F08EB74u, SISR_SOURCE_HISTORICAL,                  SISR_SOURCE_TYPE_UCI,     0u },
    { 0x00009F8Du, 0x00000001u, 0x580B164Fu, SISR_SOURCE_HISTORICAL,                  SISR_SOURCE_TYPE_UK,      0u },
    { 0x00009F99u, 0x00000002u, 0x325AC826u, SISR_SOURCE_HISTORICAL,                  SISR_SOURCE_TYPE_UTC,     0u },
    { 0x0000FA26u, 0x000003A4u, 0x970E732Eu, (SISR_SOURCE_HISTORICAL |
                                             SISR_SOURCE_VERIFIED),                   SISR_SOURCE_TYPE_UTC,     0u },
    { 0x0002F800u, 0x00000003u, 0x335AC9B9u, SISR_SOURCE_HISTORICAL,                  SISR_SOURCE_TYPE_UTC,     0u },
    { 0x00140000u, 0x00000001u, 0x9197EED4u, (SISR_SOURCE_CONLANG |
                                             SISR_SOURCE_VERIFIED),                   SISR_SOURCE_TYPE_CUSTOM,  0u },
    { 0x00160100u, 0x00000100u, 0x0C6B4AFAu, (SISR_SOURCE_RNUR_SET1 |
                                             SISR_SOURCE_VERIFIED),                   SISR_SOURCE_TYPE_RNUR,    0u },
    { 0x00160101u, 0x00000101u, 0x0D6B4C8Du, (SISR_SOURCE_RNUR_SET1 |
                                             SISR_SOURCE_VERIFIED),                   SISR_SOURCE_TYPE_RNUR,    0u },
    { 0x00160102u, 0x00000102u, 0xFE6B34F0u, (SISR_SOURCE_RNUR_SET1 |
                                             SISR_SOURCE_DEPRECATED),                 SISR_SOURCE_TYPE_RNUR,    0u },
    { 0x00160200u, 0x00000200u, 0x102B036Bu, (SISR_SOURCE_RNUR_SET2 |
                                             SISR_SOURCE_VERIFIED),                   SISR_SOURCE_TYPE_RNUR,    0u },
    { 0x00160201u, 0x00000201u, 0x0F2B01D8u, (SISR_SOURCE_RNUR_SET2 |
                                             SISR_SOURCE_VERIFIED),                   SISR_SOURCE_TYPE_RNUR,    0u },
    { 0x00160202u, 0x00000202u, 0x162B0CDDu, (SISR_SOURCE_RNUR_SET2 |
                                             SISR_SOURCE_DEPRECATED),                 SISR_SOURCE_TYPE_RNUR,    0u },
    { 0x00160300u, 0x00000001u, 0x6AA18FCBu, (SISR_SOURCE_CONLANG |
                                             SISR_SOURCE_VERIFIED),                   SISR_SOURCE_TYPE_CONLANG, 0u },
    { 0x00160301u, 0x00000002u, 0x6BA1915Eu, (SISR_SOURCE_CONLANG |
                                             SISR_SOURCE_DEPRECATED),                 SISR_SOURCE_TYPE_CONLANG, 0u }
};

#define SISR_TABLE_COUNT ((uint32_t)(sizeof sisr_table / sizeof sisr_table[0]))

SISR_STATIC_ASSERT(SISR_TABLE_COUNT <= SUAS_SISR_REGISTRY_CAPACITY,
                   sisr_table_fits_capacity);

/* ── The curated static link table (§6) ───────────────────────────── */

/* Each row is the single fallback SISR publishes for a source form. Where SIP
 * also carries a `native_variant` for the same codepoint, the target must be
 * identical; tests/test_suas_sisr.c proves the overlap. */
static const sisr_variant_link_t sisr_links[] = {
    /* source       fallback     link_flags                                                    rsv */
    { 0x00003402u, 0x00160100u, (SISR_LINK_REGISTRY),                                           0u },
    { 0x00009F8Du, 0x00160200u, (SISR_LINK_REGISTRY | SISR_LINK_TRADITIONAL),                   0u },
    { 0x00009F99u, 0x00160201u, (SISR_LINK_REGISTRY | SISR_LINK_SIMPLIFIED),                    0u },
    { 0x0000FA26u, 0x000090FDu, (SISR_LINK_COMPATIBILITY),                                      0u },
    { 0x0002F800u, 0x00004E3Du, (SISR_LINK_COMPATIBILITY),                                      0u },
    { 0x00160101u, 0x00160102u, (SISR_LINK_REGISTRY),                                           0u },
    { 0x00160201u, 0x00160202u, (SISR_LINK_REGISTRY),                                           0u },
    { 0x00160300u, 0x00160301u, (SISR_LINK_REGISTRY | SISR_LINK_CUSTOM),                        0u }
};

#define SISR_LINK_TABLE_COUNT \
    ((uint32_t)(sizeof sisr_links / sizeof sisr_links[0]))

SISR_STATIC_ASSERT(SISR_LINK_TABLE_COUNT <= SUAS_SISR_LINK_CAPACITY,
                   sisr_links_fit_capacity);

/* ── The static lookup indexes (§3.5) ─────────────────────────────── */

typedef struct {
    uint16_t entry;
    uint8_t  used;
} sisr_index_slot_t;

typedef struct {
    uint8_t entry;
    uint8_t used;
} sisr_link_index_slot_t;

static sisr_index_slot_t sisr_index[SUAS_SISR_INDEX_SIZE];
static sisr_link_index_slot_t sisr_link_index[SUAS_SISR_LINK_INDEX_SIZE];
static bool sisr_ready = false;

/* ── FNV-1a primitives (§3.2) ─────────────────────────────────────── */

#define SISR_FNV_OFFSET 2166136261u
#define SISR_FNV_PRIME  16777619u

uint32_t suas_sisr_hash_codepoint(uint32_t codepoint)
{
    uint32_t h = SISR_FNV_OFFSET;
    unsigned i;

    for (i = 0u; i < 4u; i++) {
        h ^= (uint32_t)sisr_byte_at(codepoint, i);
        h *= SISR_FNV_PRIME;
    }
    return h;
}

uint32_t suas_sisr_hash_source_token(const char* token, size_t length)
{
    uint32_t h = SISR_FNV_OFFSET;
    size_t i;

    if (token == NULL || length == 0u)
        return 0u;

    for (i = 0u; i < length; i++) {
        h ^= (uint32_t)(unsigned char)token[i];
        h *= SISR_FNV_PRIME;
    }
    return h;
}

/* ── Zone dispatch (§5) ───────────────────────────────────────────── */

suas_sisr_zone_t suas_sisr_zone_of(uint32_t codepoint)
{
    if (codepoint > (uint32_t)SUAS_CODEPOINT_MAX)
        return SUAS_SISR_ZONE_EXTSUCS;
    if (codepoint <= SUAS_SISR_LIMIT_UNICODE_MAX)
        return SUAS_SISR_ZONE_BRIDGE;
    if (codepoint >= SUAS_SISR_LIMIT_SCP_MIN &&
        codepoint <= SUAS_SISR_LIMIT_SCP_MAX)
        return SUAS_SISR_ZONE_SCP;
    if (codepoint >= SUAS_SISR_LIMIT_TRAP_MIN &&
        codepoint <= SUAS_SISR_LIMIT_TRAP_MAX)
        return SUAS_SISR_ZONE_TRAP;
    if (codepoint == SUAS_SISR_LIMIT_SENTINEL)
        return SUAS_SISR_ZONE_SENTINEL;
    if (codepoint >= SUAS_SISR_RNUR_MIN &&
        codepoint <= SUAS_SISR_RNUR_MAX)
        return SUAS_SISR_ZONE_RNUR;
    return SUAS_SISR_ZONE_NATIVE;
}

bool suas_sisr_is_source_space(uint32_t codepoint)
{
    switch (suas_sisr_zone_of(codepoint)) {
    case SUAS_SISR_ZONE_BRIDGE:
    case SUAS_SISR_ZONE_RNUR:
    case SUAS_SISR_ZONE_NATIVE:
        return true;
    default:
        return false;
    }
}

bool suas_sisr_is_neography(uint32_t codepoint)
{
    return suas_sisr_zone_of(codepoint) == SUAS_SISR_ZONE_RNUR;
}

bool suas_sisr_neography_set(uint32_t codepoint, uint32_t* out_set)
{
    if (out_set == NULL || !suas_sisr_is_neography(codepoint))
        return false;
    *out_set = (codepoint - SUAS_SISR_RNUR_MIN) >> SUAS_SISR_RNUR_SET_SHIFT;
    return true;
}

/* ── Lazy index construction (§3.5) ───────────────────────────────── */

static void sisr_build(void)
{
    uint32_t i;
    uint32_t h;
    uint32_t start;

    for (i = 0u; i < SUAS_SISR_INDEX_SIZE; i++) {
        sisr_index[i].entry = 0u;
        sisr_index[i].used = 0u;
    }
    for (i = 0u; i < SISR_TABLE_COUNT; i++) {
        h = suas_sisr_hash_codepoint(sisr_table[i].codepoint) &
            (SUAS_SISR_INDEX_SIZE - 1u);
        start = h;
        while (sisr_index[h].used) {
            h = (h + 1u) & (SUAS_SISR_INDEX_SIZE - 1u);
            if (h == start)
                break; /* unreachable: capacity < index size */
        }
        sisr_index[h].entry = (uint16_t)i;
        sisr_index[h].used = 1u;
    }

    for (i = 0u; i < SUAS_SISR_LINK_INDEX_SIZE; i++) {
        sisr_link_index[i].entry = 0u;
        sisr_link_index[i].used = 0u;
    }
    for (i = 0u; i < SISR_LINK_TABLE_COUNT; i++) {
        h = suas_sisr_hash_codepoint(sisr_links[i].codepoint) &
            (SUAS_SISR_LINK_INDEX_SIZE - 1u);
        start = h;
        while (sisr_link_index[h].used) {
            h = (h + 1u) & (SUAS_SISR_LINK_INDEX_SIZE - 1u);
            if (h == start)
                break;
        }
        sisr_link_index[h].entry = (uint8_t)i;
        sisr_link_index[h].used = 1u;
    }

    sisr_ready = true;
}

void suas_sisr_init_registry(void)
{
    if (!sisr_ready)
        sisr_build();
}

/* ── O(1) probes (§3.4) ───────────────────────────────────────────── */

static const sisr_source_entry_t* sisr_find(uint32_t codepoint)
{
    uint32_t h;
    uint32_t start;

    if (!suas_sisr_is_source_space(codepoint))
        return NULL;
    if (!sisr_ready)
        sisr_build();

    h = suas_sisr_hash_codepoint(codepoint) & (SUAS_SISR_INDEX_SIZE - 1u);
    start = h;
    for (;;) {
        const sisr_source_entry_t* cand;
        if (!sisr_index[h].used)
            return NULL;
        cand = &sisr_table[sisr_index[h].entry];
        if (cand->codepoint == codepoint)
            return cand;
        h = (h + 1u) & (SUAS_SISR_INDEX_SIZE - 1u);
        if (h == start)
            return NULL;
    }
}

static const sisr_variant_link_t* sisr_find_link(uint32_t codepoint)
{
    uint32_t h;
    uint32_t start;

    if (!suas_sisr_is_source_space(codepoint))
        return NULL;
    if (!sisr_ready)
        sisr_build();

    h = suas_sisr_hash_codepoint(codepoint) &
        (SUAS_SISR_LINK_INDEX_SIZE - 1u);
    start = h;
    for (;;) {
        const sisr_variant_link_t* cand;
        if (!sisr_link_index[h].used)
            return NULL;
        cand = &sisr_links[sisr_link_index[h].entry];
        if (cand->codepoint == codepoint)
            return cand;
        h = (h + 1u) & (SUAS_SISR_LINK_INDEX_SIZE - 1u);
        if (h == start)
            return NULL;
    }
}

/* ── Query surface ────────────────────────────────────────────────── */

bool suas_sisr_get_source_entry(uint32_t codepoint,
                                sisr_source_entry_t* out_entry)
{
    const sisr_source_entry_t* found;

    if (out_entry == NULL)
        return false;
    found = sisr_find(codepoint);
    if (found == NULL)
        return false;
    *out_entry = *found;
    return true;
}

bool suas_sisr_is_registered(uint32_t codepoint)
{
    return sisr_find(codepoint) != NULL;
}

uint32_t suas_sisr_entry_count(void)
{
    return SISR_TABLE_COUNT;
}

bool suas_sisr_entry_at(uint32_t index, sisr_source_entry_t* out_entry)
{
    if (out_entry == NULL || index >= SISR_TABLE_COUNT)
        return false;
    *out_entry = sisr_table[index];
    return true;
}

bool suas_sisr_get_variant_link(uint32_t codepoint,
                                sisr_variant_link_t* out_link)
{
    const sisr_variant_link_t* found;

    if (out_link == NULL)
        return false;
    found = sisr_find_link(codepoint);
    if (found == NULL)
        return false;
    *out_link = *found;
    return true;
}

bool suas_sisr_fallback_target(uint32_t codepoint, uint32_t* out_codepoint)
{
    const sisr_variant_link_t* found;

    if (out_codepoint == NULL)
        return false;
    found = sisr_find_link(codepoint);
    if (found == NULL)
        return false;
    *out_codepoint = found->fallback_codepoint;
    return true;
}

uint32_t suas_sisr_link_count(void)
{
    return SISR_LINK_TABLE_COUNT;
}

bool suas_sisr_link_at(uint32_t index, sisr_variant_link_t* out_link)
{
    if (out_link == NULL || index >= SISR_LINK_TABLE_COUNT)
        return false;
    *out_link = sisr_links[index];
    return true;
}

/* ── Canonical tokens (§3.2) ──────────────────────────────────────── */

const char* suas_sisr_source_type_prefix(suas_sisr_source_type_t type)
{
    switch (type) {
    case SISR_SOURCE_TYPE_UTC:     return "UTC";
    case SISR_SOURCE_TYPE_UCI:     return "UCI";
    case SISR_SOURCE_TYPE_UK:      return "UK";
    case SISR_SOURCE_TYPE_RNUR:    return "RNUR";
    case SISR_SOURCE_TYPE_CONLANG: return "CONL";
    case SISR_SOURCE_TYPE_CUSTOM:  return "CUST";
    default:                       return NULL;
    }
}

const char* suas_sisr_source_type_name(suas_sisr_source_type_t type)
{
    switch (type) {
    case SISR_SOURCE_TYPE_UTC:     return "UTC";
    case SISR_SOURCE_TYPE_UCI:     return "UCI";
    case SISR_SOURCE_TYPE_UK:      return "UK";
    case SISR_SOURCE_TYPE_RNUR:    return "RNUR";
    case SISR_SOURCE_TYPE_CONLANG: return "CONLANG";
    case SISR_SOURCE_TYPE_CUSTOM:  return "CUSTOM";
    default:                       return "INVALID";
    }
}

static bool sisr_name_is(const char* name, size_t length, const char* text)
{
    size_t i;

    for (i = 0u; i < length; i++) {
        if (text[i] == '\0' || name[i] != text[i])
            return false;
    }
    return text[length] == '\0';
}

suas_sisr_source_type_t suas_sisr_source_type_by_name(const char* name,
                                                      size_t length)
{
    if (name == NULL || length == 0u)
        return SISR_SOURCE_TYPE_INVALID;
    if (sisr_name_is(name, length, "UTC"))
        return SISR_SOURCE_TYPE_UTC;
    if (sisr_name_is(name, length, "UCI"))
        return SISR_SOURCE_TYPE_UCI;
    if (sisr_name_is(name, length, "UK"))
        return SISR_SOURCE_TYPE_UK;
    if (sisr_name_is(name, length, "RNUR"))
        return SISR_SOURCE_TYPE_RNUR;
    if (sisr_name_is(name, length, "CONLANG") ||
        sisr_name_is(name, length, "CONL"))
        return SISR_SOURCE_TYPE_CONLANG;
    if (sisr_name_is(name, length, "CUSTOM") ||
        sisr_name_is(name, length, "CUST"))
        return SISR_SOURCE_TYPE_CUSTOM;
    return SISR_SOURCE_TYPE_INVALID;
}

/* Decimal digits of a 24-bit serial, least significant first. */
static unsigned sisr_serial_digits(uint32_t serial, uint8_t* digits)
{
    unsigned n = 0u;

    do {
        digits[n++] = (uint8_t)('0' + (serial % 10u));
        serial /= 10u;
    } while (serial != 0u);
    return n;
}

bool suas_sisr_format_source_id(const sisr_source_entry_t* entry,
                                char* buffer, size_t capacity)
{
    const char* prefix;
    uint8_t digits[10];
    unsigned digits_n;
    unsigned width;
    size_t prefix_n = 0u;
    size_t total;
    size_t i;

    if (entry == NULL || buffer == NULL)
        return false;
    if (entry->source_serial < SUAS_SISR_SERIAL_MIN ||
        entry->source_serial > SUAS_SISR_SERIAL_MAX)
        return false;

    prefix = suas_sisr_source_type_prefix(
        (suas_sisr_source_type_t)entry->source_type);
    if (prefix == NULL)
        return false;

    while (prefix[prefix_n] != '\0')
        prefix_n++;

    digits_n = sisr_serial_digits(entry->source_serial, digits);
    width = digits_n < 5u ? 5u : digits_n;
    total = prefix_n + 1u + (size_t)width;
    if (capacity < total + 1u)
        return false;

    for (i = 0u; i < prefix_n; i++)
        buffer[i] = prefix[i];
    buffer[prefix_n] = '-';
    for (i = 0u; i < (size_t)width; i++) {
        /* Pad on the left, then emit the digits most significant first. */
        size_t from_right = (size_t)width - i;
        char c = '0';
        if (from_right <= (size_t)digits_n)
            c = (char)digits[from_right - 1u];
        buffer[prefix_n + 1u + i] = c;
    }
    buffer[total] = '\0';
    return true;
}

bool suas_sisr_source_id_matches(const sisr_source_entry_t* entry,
                                 const char* token, size_t length)
{
    char formatted[SUAS_SISR_TOKEN_MAX];
    size_t i;

    if (entry == NULL || token == NULL)
        return false;
    if (!suas_sisr_format_source_id(entry, formatted,
                                    sizeof formatted))
        return false;

    for (i = 0u; i < length; i++) {
        if (formatted[i] == '\0' || formatted[i] != token[i])
            return false;
    }
    return formatted[length] == '\0';
}

/* ── Taxonomy names ───────────────────────────────────────────────── */

const char* suas_sisr_link_name(uint16_t link_flags)
{
    if (link_flags == 0u)
        return "NONE";
    if ((link_flags & (uint16_t)~SISR_LINK_ASSIGNED_MASK) != 0u)
        return "UNKNOWN";
    if (link_flags & SISR_LINK_SIMPLIFIED)    return "SIMPLIFIED";
    if (link_flags & SISR_LINK_TRADITIONAL)   return "TRADITIONAL";
    if (link_flags & SISR_LINK_Z_VARIANT)     return "Z_VARIANT";
    if (link_flags & SISR_LINK_COMPATIBILITY) return "COMPATIBILITY";
    if (link_flags & SISR_LINK_REORDERED)     return "REORDERED";
    if (link_flags & SISR_LINK_APPROXIMATE)   return "APPROXIMATE";
    if (link_flags & SISR_LINK_REGISTRY)      return "REGISTRY";
    return "CUSTOM";
}

const char* suas_sisr_registry_error_name(sisr_registry_error_t error)
{
    switch (error) {
    case SISR_REGISTRY_OK:                      return "OK";
    case SISR_REGISTRY_NULL_TABLE:              return "NULL_TABLE";
    case SISR_REGISTRY_COUNT_OVER:              return "COUNT_OVER";
    case SISR_REGISTRY_UNORDERED:               return "UNORDERED";
    case SISR_REGISTRY_DUPLICATE_CODEPOINT:     return "DUPLICATE_CODEPOINT";
    case SISR_REGISTRY_OUT_OF_SPACE:            return "OUT_OF_SPACE";
    case SISR_REGISTRY_INADMISSIBLE_CODEPOINT:  return "INADMISSIBLE_CODEPOINT";
    case SISR_REGISTRY_BAD_SOURCE_ID:           return "BAD_SOURCE_ID";
    case SISR_REGISTRY_BAD_SOURCE_TYPE:         return "BAD_SOURCE_TYPE";
    case SISR_REGISTRY_BAD_FLAGS:               return "BAD_FLAGS";
    case SISR_REGISTRY_BAD_RESERVED:            return "BAD_RESERVED";
    case SISR_REGISTRY_DUPLICATE_SOURCE_ID:     return "DUPLICATE_SOURCE_ID";
    case SISR_REGISTRY_HASH_MISMATCH:           return "HASH_MISMATCH";
    case SISR_REGISTRY_BAD_LINKAGE:             return "BAD_LINKAGE";
    default:                                    return "UNKNOWN";
    }
}

/* ── Verification (§9) ────────────────────────────────────────────── */

static bool sisr_fail(sisr_registry_error_t* out_error,
                      sisr_registry_error_t error)
{
    if (out_error != NULL)
        *out_error = error;
    return false;
}

static bool sisr_check_flags(uint16_t flags, bool is_link,
                             sisr_registry_error_t* out_error)
{
    uint16_t assigned = is_link ? (uint16_t)SISR_LINK_ASSIGNED_MASK
                                : (uint16_t)SISR_SOURCE_ASSIGNED_MASK;

    if ((flags & (uint16_t)~assigned) != 0u)
        return sisr_fail(out_error, SISR_REGISTRY_BAD_FLAGS);
    if (is_link) {
        if (flags == 0u)
            return sisr_fail(out_error, SISR_REGISTRY_BAD_LINKAGE);
    } else {
        if ((flags & SISR_SOURCE_SET_MASK) == SISR_SOURCE_SET_MASK)
            return sisr_fail(out_error, SISR_REGISTRY_BAD_FLAGS);
    }
    return true;
}

bool suas_sisr_validate_table(const sisr_source_entry_t* entries,
                              uint32_t count,
                              sisr_registry_error_t* out_error)
{
    uint32_t i;
    uint32_t j;
    char token[SUAS_SISR_TOKEN_MAX];

    if (out_error != NULL)
        *out_error = SISR_REGISTRY_OK;
    if (count > SUAS_SISR_REGISTRY_CAPACITY)
        return sisr_fail(out_error, SISR_REGISTRY_COUNT_OVER);
    if (count == 0u)
        return true;
    if (entries == NULL)
        return sisr_fail(out_error, SISR_REGISTRY_NULL_TABLE);

    for (i = 0u; i < count; i++) {
        const sisr_source_entry_t* e = &entries[i];

        if (e->codepoint > (uint32_t)SUAS_CODEPOINT_MAX)
            return sisr_fail(out_error, SISR_REGISTRY_OUT_OF_SPACE);
        if (!suas_sisr_is_source_space(e->codepoint))
            return sisr_fail(out_error, SISR_REGISTRY_INADMISSIBLE_CODEPOINT);
        if (e->source_type < (uint8_t)SISR_SOURCE_TYPE_UTC ||
            e->source_type > (uint8_t)SISR_SOURCE_TYPE_CUSTOM)
            return sisr_fail(out_error, SISR_REGISTRY_BAD_SOURCE_TYPE);
        if (e->source_serial < SUAS_SISR_SERIAL_MIN ||
            e->source_serial > SUAS_SISR_SERIAL_MAX)
            return sisr_fail(out_error, SISR_REGISTRY_BAD_SOURCE_ID);
        if (e->reserved != 0u)
            return sisr_fail(out_error, SISR_REGISTRY_BAD_RESERVED);
        if (!sisr_check_flags(e->registry_flags, false, out_error))
            return false;

        if (i > 0u) {
            if (e->codepoint == entries[i - 1u].codepoint)
                return sisr_fail(out_error,
                                 SISR_REGISTRY_DUPLICATE_CODEPOINT);
            if (e->codepoint < entries[i - 1u].codepoint)
                return sisr_fail(out_error, SISR_REGISTRY_UNORDERED);
        }

        for (j = 0u; j < i; j++) {
            if (entries[j].source_type == e->source_type &&
                entries[j].source_serial == e->source_serial)
                return sisr_fail(out_error,
                                 SISR_REGISTRY_DUPLICATE_SOURCE_ID);
        }

        if (!suas_sisr_format_source_id(e, token, sizeof token))
            return sisr_fail(out_error, SISR_REGISTRY_BAD_SOURCE_ID);
        {
            size_t n = 0u;
            while (token[n] != '\0')
                n++;
            if (suas_sisr_hash_source_token(token, n) != e->source_id_hash)
                return sisr_fail(out_error, SISR_REGISTRY_HASH_MISMATCH);
        }
    }
    return true;
}

bool suas_sisr_validate_links(const sisr_variant_link_t* links,
                              uint32_t count,
                              sisr_registry_error_t* out_error)
{
    uint32_t i;

    if (out_error != NULL)
        *out_error = SISR_REGISTRY_OK;
    if (count > SUAS_SISR_LINK_CAPACITY)
        return sisr_fail(out_error, SISR_REGISTRY_COUNT_OVER);
    if (count == 0u)
        return true;
    if (links == NULL)
        return sisr_fail(out_error, SISR_REGISTRY_NULL_TABLE);

    for (i = 0u; i < count; i++) {
        const sisr_variant_link_t* l = &links[i];

        if (l->codepoint > (uint32_t)SUAS_CODEPOINT_MAX ||
            l->fallback_codepoint > (uint32_t)SUAS_CODEPOINT_MAX)
            return sisr_fail(out_error, SISR_REGISTRY_OUT_OF_SPACE);
        if (!suas_sisr_is_source_space(l->codepoint) ||
            !suas_sisr_is_source_space(l->fallback_codepoint))
            return sisr_fail(out_error,
                             SISR_REGISTRY_INADMISSIBLE_CODEPOINT);
        if (l->reserved != 0u)
            return sisr_fail(out_error, SISR_REGISTRY_BAD_RESERVED);
        if (!sisr_check_flags(l->link_flags, true, out_error))
            return false;
        if (l->codepoint == l->fallback_codepoint)
            return sisr_fail(out_error, SISR_REGISTRY_BAD_LINKAGE);

        if (i > 0u) {
            if (l->codepoint == links[i - 1u].codepoint)
                return sisr_fail(out_error,
                                 SISR_REGISTRY_DUPLICATE_CODEPOINT);
            if (l->codepoint < links[i - 1u].codepoint)
                return sisr_fail(out_error, SISR_REGISTRY_UNORDERED);
        }
    }
    return true;
}

/* ── Version ──────────────────────────────────────────────────────── */

const char* suas_sisr_version_string(void)
{
    return "1.0";
}
