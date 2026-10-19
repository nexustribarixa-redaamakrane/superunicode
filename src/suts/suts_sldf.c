/* SUTS-003 — System Locale Data & Formatting (SLDF)
 *
 * Reference implementation of docs/suts/SUTS-003-sldf.md.
 *
 * Freestanding C99, zero allocation. Locale definitions are compiled into an
 * immutable, statically-linked table of fixed-size sldf_locale_t blocks. The
 * table is indexed once into a fixed-size open-addressing array keyed by the
 * FNV-1a hash of the normalized RCCR subtag, so a subsystem resolves a locale
 * in O(1) expected time with no heap, no markup parsing, and no dynamic table
 * growth.
 *
 * The curated table holds natural-language locales (Bridge plane) plus a
 * representative set of registered conlang locales (RNUR neography plane, e.g.
 * `lumer` and `kupon`). Extending the table is a data update (a minor revision)
 * and never changes the API.
 */

#include "suts/suts_sldf.h"

/* ── Small string helpers (no <string.h>: freestanding subset only) ─ */

/* Normalize an RCCR subtag into `out`: fold A-Z to a-z, keep [a-z0-9-], and
 * reject any other byte or an over-long key. Returns the normalized length,
 * or 0 when the tag is empty, malformed, or too long. */
static size_t sldf_normalize(const char* tag, char* out, size_t cap)
{
    size_t i = 0;
    size_t o = 0;

    if (!tag || !tag[0])
        return 0u;
    for (; tag[i]; i++) {
        char c = tag[i];
        if (c >= 'A' && c <= 'Z')
            c = (char)(c - 'A' + 'a');
        else if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-'))
            return 0u;
        if (o + 1u >= cap)
            return 0u;
        out[o++] = c;
    }
    out[o] = '\0';
    return o;
}

static int sldf_buf_equal(const char* a, const char* b, size_t n)
{
    size_t i;
    for (i = 0; i < n; i++) {
        if (a[i] != b[i])
            return 0;
    }
    return 1;
}

/* ── The curated static registry (§2.1, §4.1) ─────────────────────── */

#define SLDF_SYM(...) { __VA_ARGS__ }

static const sldf_locale_t sldf_table[] = {
    /* ── Bridge plane: natural languages ─────────────────────────── */
    { .tag_key = "en-US",
      .numeric = { .decimal_separator = 0x002EULL, .grouping_separator = 0x002CULL,
                   .grouping_primary = 3u, .grouping_secondary = 3u,
                   .currency_symbol = SLDF_SYM(0x0024ULL), .currency_symbol_length = 1u,
                   .currency_position = SUTS_SLDF_AFFIX_PREFIX, .currency_space = 0u,
                   .minus_sign = 0x002DULL, .minus_position = SUTS_SLDF_AFFIX_PREFIX,
                   .negative_parenthesized = 0u },
      .temporal = { .date_order = SUTS_SLDF_DATE_MDY, .date_separator = 0x002FULL,
                    .time_separator = 0x003AULL, .use_24_hour = 0u,
                    .first_day_of_week = 0u, .weekend_start = 0u, .weekend_end = 6u,
                    .long_month_name = 0u },
      .collation_base_offset = 0x0100u, .collation_plane = SUTS_SLDF_PLANE_BRIDGE,
      .is_rtl = 0u, .script = 1u, .reserved = 0u },

    { .tag_key = "en-GB",
      .numeric = { .decimal_separator = 0x002EULL, .grouping_separator = 0x002CULL,
                   .grouping_primary = 3u, .grouping_secondary = 3u,
                   .currency_symbol = SLDF_SYM(0x00A3ULL), .currency_symbol_length = 1u,
                   .currency_position = SUTS_SLDF_AFFIX_PREFIX, .currency_space = 0u,
                   .minus_sign = 0x002DULL, .minus_position = SUTS_SLDF_AFFIX_PREFIX,
                   .negative_parenthesized = 0u },
      .temporal = { .date_order = SUTS_SLDF_DATE_DMY, .date_separator = 0x002FULL,
                    .time_separator = 0x003AULL, .use_24_hour = 1u,
                    .first_day_of_week = 1u, .weekend_start = 6u, .weekend_end = 0u,
                    .long_month_name = 0u },
      .collation_base_offset = 0x0101u, .collation_plane = SUTS_SLDF_PLANE_BRIDGE,
      .is_rtl = 0u, .script = 1u, .reserved = 0u },

    { .tag_key = "de-DE",
      .numeric = { .decimal_separator = 0x002CULL, .grouping_separator = 0x002EULL,
                   .grouping_primary = 3u, .grouping_secondary = 3u,
                   .currency_symbol = SLDF_SYM(0x20ACULL), .currency_symbol_length = 1u,
                   .currency_position = SUTS_SLDF_AFFIX_SUFFIX, .currency_space = 1u,
                   .minus_sign = 0x002DULL, .minus_position = SUTS_SLDF_AFFIX_PREFIX,
                   .negative_parenthesized = 0u },
      .temporal = { .date_order = SUTS_SLDF_DATE_DMY, .date_separator = 0x002EULL,
                    .time_separator = 0x003AULL, .use_24_hour = 1u,
                    .first_day_of_week = 1u, .weekend_start = 6u, .weekend_end = 0u,
                    .long_month_name = 0u },
      .collation_base_offset = 0x0102u, .collation_plane = SUTS_SLDF_PLANE_BRIDGE,
      .is_rtl = 0u, .script = 1u, .reserved = 0u },

    { .tag_key = "fr-FR",
      .numeric = { .decimal_separator = 0x002CULL, .grouping_separator = 0x202FULL,
                   .grouping_primary = 3u, .grouping_secondary = 3u,
                   .currency_symbol = SLDF_SYM(0x20ACULL), .currency_symbol_length = 1u,
                   .currency_position = SUTS_SLDF_AFFIX_SUFFIX, .currency_space = 1u,
                   .minus_sign = 0x002DULL, .minus_position = SUTS_SLDF_AFFIX_PREFIX,
                   .negative_parenthesized = 0u },
      .temporal = { .date_order = SUTS_SLDF_DATE_DMY, .date_separator = 0x002FULL,
                    .time_separator = 0x003AULL, .use_24_hour = 1u,
                    .first_day_of_week = 1u, .weekend_start = 6u, .weekend_end = 0u,
                    .long_month_name = 0u },
      .collation_base_offset = 0x0103u, .collation_plane = SUTS_SLDF_PLANE_BRIDGE,
      .is_rtl = 0u, .script = 1u, .reserved = 0u },

    { .tag_key = "es-ES",
      .numeric = { .decimal_separator = 0x002CULL, .grouping_separator = 0x002EULL,
                   .grouping_primary = 3u, .grouping_secondary = 3u,
                   .currency_symbol = SLDF_SYM(0x20ACULL), .currency_symbol_length = 1u,
                   .currency_position = SUTS_SLDF_AFFIX_SUFFIX, .currency_space = 1u,
                   .minus_sign = 0x002DULL, .minus_position = SUTS_SLDF_AFFIX_PREFIX,
                   .negative_parenthesized = 0u },
      .temporal = { .date_order = SUTS_SLDF_DATE_DMY, .date_separator = 0x002FULL,
                    .time_separator = 0x003AULL, .use_24_hour = 1u,
                    .first_day_of_week = 1u, .weekend_start = 6u, .weekend_end = 0u,
                    .long_month_name = 0u },
      .collation_base_offset = 0x0104u, .collation_plane = SUTS_SLDF_PLANE_BRIDGE,
      .is_rtl = 0u, .script = 1u, .reserved = 0u },

    { .tag_key = "it-IT",
      .numeric = { .decimal_separator = 0x002CULL, .grouping_separator = 0x002EULL,
                   .grouping_primary = 3u, .grouping_secondary = 3u,
                   .currency_symbol = SLDF_SYM(0x20ACULL), .currency_symbol_length = 1u,
                   .currency_position = SUTS_SLDF_AFFIX_SUFFIX, .currency_space = 1u,
                   .minus_sign = 0x002DULL, .minus_position = SUTS_SLDF_AFFIX_PREFIX,
                   .negative_parenthesized = 0u },
      .temporal = { .date_order = SUTS_SLDF_DATE_DMY, .date_separator = 0x002FULL,
                    .time_separator = 0x003AULL, .use_24_hour = 1u,
                    .first_day_of_week = 1u, .weekend_start = 6u, .weekend_end = 0u,
                    .long_month_name = 0u },
      .collation_base_offset = 0x0105u, .collation_plane = SUTS_SLDF_PLANE_BRIDGE,
      .is_rtl = 0u, .script = 1u, .reserved = 0u },

    { .tag_key = "ru-RU",
      .numeric = { .decimal_separator = 0x002CULL, .grouping_separator = 0x00A0ULL,
                   .grouping_primary = 3u, .grouping_secondary = 3u,
                   .currency_symbol = SLDF_SYM(0x20BDULL), .currency_symbol_length = 1u,
                   .currency_position = SUTS_SLDF_AFFIX_SUFFIX, .currency_space = 1u,
                   .minus_sign = 0x2212ULL, .minus_position = SUTS_SLDF_AFFIX_PREFIX,
                   .negative_parenthesized = 0u },
      .temporal = { .date_order = SUTS_SLDF_DATE_DMY, .date_separator = 0x002EULL,
                    .time_separator = 0x003AULL, .use_24_hour = 1u,
                    .first_day_of_week = 1u, .weekend_start = 6u, .weekend_end = 0u,
                    .long_month_name = 0u },
      .collation_base_offset = 0x0106u, .collation_plane = SUTS_SLDF_PLANE_BRIDGE,
      .is_rtl = 0u, .script = 8u, .reserved = 0u },

    { .tag_key = "zh-CN",
      .numeric = { .decimal_separator = 0x002EULL, .grouping_separator = 0x002CULL,
                   .grouping_primary = 3u, .grouping_secondary = 3u,
                   .currency_symbol = SLDF_SYM(0x00A5ULL), .currency_symbol_length = 1u,
                   .currency_position = SUTS_SLDF_AFFIX_PREFIX, .currency_space = 0u,
                   .minus_sign = 0x002DULL, .minus_position = SUTS_SLDF_AFFIX_PREFIX,
                   .negative_parenthesized = 0u },
      .temporal = { .date_order = SUTS_SLDF_DATE_YMD, .date_separator = 0x002FULL,
                    .time_separator = 0x003AULL, .use_24_hour = 1u,
                    .first_day_of_week = 0u, .weekend_start = 0u, .weekend_end = 6u,
                    .long_month_name = 0u },
      .collation_base_offset = 0x0107u, .collation_plane = SUTS_SLDF_PLANE_BRIDGE,
      .is_rtl = 0u, .script = 4u, .reserved = 0u },

    { .tag_key = "ja-JP",
      .numeric = { .decimal_separator = 0x002EULL, .grouping_separator = 0x002CULL,
                   .grouping_primary = 3u, .grouping_secondary = 3u,
                   .currency_symbol = SLDF_SYM(0x00A5ULL), .currency_symbol_length = 1u,
                   .currency_position = SUTS_SLDF_AFFIX_PREFIX, .currency_space = 0u,
                   .minus_sign = 0x002DULL, .minus_position = SUTS_SLDF_AFFIX_PREFIX,
                   .negative_parenthesized = 0u },
      .temporal = { .date_order = SUTS_SLDF_DATE_YMD, .date_separator = 0x002FULL,
                    .time_separator = 0x003AULL, .use_24_hour = 1u,
                    .first_day_of_week = 0u, .weekend_start = 0u, .weekend_end = 6u,
                    .long_month_name = 0u },
      .collation_base_offset = 0x0108u, .collation_plane = SUTS_SLDF_PLANE_BRIDGE,
      .is_rtl = 0u, .script = 5u, .reserved = 0u },

    { .tag_key = "hi-IN",
      .numeric = { .decimal_separator = 0x002EULL, .grouping_separator = 0x002CULL,
                   .grouping_primary = 3u, .grouping_secondary = 2u,
                   .currency_symbol = SLDF_SYM(0x20B9ULL), .currency_symbol_length = 1u,
                   .currency_position = SUTS_SLDF_AFFIX_PREFIX, .currency_space = 0u,
                   .minus_sign = 0x002DULL, .minus_position = SUTS_SLDF_AFFIX_PREFIX,
                   .negative_parenthesized = 0u },
      .temporal = { .date_order = SUTS_SLDF_DATE_DMY, .date_separator = 0x002FULL,
                    .time_separator = 0x003AULL, .use_24_hour = 0u,
                    .first_day_of_week = 0u, .weekend_start = 0u, .weekend_end = 6u,
                    .long_month_name = 0u },
      .collation_base_offset = 0x0109u, .collation_plane = SUTS_SLDF_PLANE_BRIDGE,
      .is_rtl = 0u, .script = 6u, .reserved = 0u },

    { .tag_key = "ar-EG",
      .numeric = { .decimal_separator = 0x066BULL, .grouping_separator = 0x066CULL,
                   .grouping_primary = 3u, .grouping_secondary = 3u,
                   .currency_symbol = SLDF_SYM(0x062CULL, 0x002EULL, 0x0645ULL),
                   .currency_symbol_length = 3u,
                   .currency_position = SUTS_SLDF_AFFIX_SUFFIX, .currency_space = 1u,
                   .minus_sign = 0x002DULL, .minus_position = SUTS_SLDF_AFFIX_PREFIX,
                   .negative_parenthesized = 0u },
      .temporal = { .date_order = SUTS_SLDF_DATE_DMY, .date_separator = 0x002FULL,
                    .time_separator = 0x003AULL, .use_24_hour = 0u,
                    .first_day_of_week = 6u, .weekend_start = 5u, .weekend_end = 6u,
                    .long_month_name = 1u },
      .collation_base_offset = 0x010Au, .collation_plane = SUTS_SLDF_PLANE_BRIDGE,
      .is_rtl = 1u, .script = 2u, .reserved = 0u },

    { .tag_key = "he-IL",
      .numeric = { .decimal_separator = 0x002EULL, .grouping_separator = 0x002CULL,
                   .grouping_primary = 3u, .grouping_secondary = 3u,
                   .currency_symbol = SLDF_SYM(0x20AAULL), .currency_symbol_length = 1u,
                   .currency_position = SUTS_SLDF_AFFIX_PREFIX, .currency_space = 0u,
                   .minus_sign = 0x002DULL, .minus_position = SUTS_SLDF_AFFIX_PREFIX,
                   .negative_parenthesized = 0u },
      .temporal = { .date_order = SUTS_SLDF_DATE_DMY, .date_separator = 0x002EULL,
                    .time_separator = 0x003AULL, .use_24_hour = 1u,
                    .first_day_of_week = 0u, .weekend_start = 5u, .weekend_end = 6u,
                    .long_month_name = 0u },
      .collation_base_offset = 0x010Bu, .collation_plane = SUTS_SLDF_PLANE_BRIDGE,
      .is_rtl = 1u, .script = 3u, .reserved = 0u },

    { .tag_key = "th-TH",
      .numeric = { .decimal_separator = 0x002EULL, .grouping_separator = 0x002CULL,
                   .grouping_primary = 3u, .grouping_secondary = 3u,
                   .currency_symbol = SLDF_SYM(0x0E3FULL), .currency_symbol_length = 1u,
                   .currency_position = SUTS_SLDF_AFFIX_PREFIX, .currency_space = 0u,
                   .minus_sign = 0x002DULL, .minus_position = SUTS_SLDF_AFFIX_PREFIX,
                   .negative_parenthesized = 0u },
      .temporal = { .date_order = SUTS_SLDF_DATE_DMY, .date_separator = 0x002FULL,
                    .time_separator = 0x003AULL, .use_24_hour = 1u,
                    .first_day_of_week = 0u, .weekend_start = 0u, .weekend_end = 6u,
                    .long_month_name = 0u },
      .collation_base_offset = 0x010Cu, .collation_plane = SUTS_SLDF_PLANE_BRIDGE,
      .is_rtl = 0u, .script = 7u, .reserved = 0u },

    /* ── RNUR neography plane: registered conlang locales (§3.3) ─── */
    { .tag_key = "lumer",
      .numeric = { .decimal_separator = 0x160010ULL, .grouping_separator = 0x160011ULL,
                   .grouping_primary = 3u, .grouping_secondary = 0u,
                   .currency_symbol = SLDF_SYM(0x160020ULL), .currency_symbol_length = 1u,
                   .currency_position = SUTS_SLDF_AFFIX_PREFIX, .currency_space = 0u,
                   .minus_sign = 0x160030ULL, .minus_position = SUTS_SLDF_AFFIX_PREFIX,
                   .negative_parenthesized = 0u },
      .temporal = { .date_order = SUTS_SLDF_DATE_YMD, .date_separator = 0x160040ULL,
                    .time_separator = 0x160041ULL, .use_24_hour = 1u,
                    .first_day_of_week = 1u, .weekend_start = 6u, .weekend_end = 0u,
                    .long_month_name = 0u },
      .collation_base_offset = 0x0100u, .collation_plane = SUTS_SLDF_PLANE_RNUR,
      .is_rtl = 0u, .script = 9u, .reserved = 0u },

    { .tag_key = "kupon",
      .numeric = { .decimal_separator = 0x160012ULL, .grouping_separator = 0x160013ULL,
                   .grouping_primary = 3u, .grouping_secondary = 2u,
                   .currency_symbol = SLDF_SYM(0x160021ULL, 0x160022ULL),
                   .currency_symbol_length = 2u,
                   .currency_position = SUTS_SLDF_AFFIX_SUFFIX, .currency_space = 1u,
                   .minus_sign = 0x160030ULL, .minus_position = SUTS_SLDF_AFFIX_PREFIX,
                   .negative_parenthesized = 0u },
      .temporal = { .date_order = SUTS_SLDF_DATE_DMY, .date_separator = 0x160042ULL,
                    .time_separator = 0x160041ULL, .use_24_hour = 0u,
                    .first_day_of_week = 0u, .weekend_start = 0u, .weekend_end = 6u,
                    .long_month_name = 0u },
      .collation_base_offset = 0x0200u, .collation_plane = SUTS_SLDF_PLANE_RNUR,
      .is_rtl = 0u, .script = 9u, .reserved = 0u },
};

#define SLDF_TABLE_COUNT ((size_t)(sizeof(sldf_table) / sizeof(sldf_table[0])))

/* ── The static lookup index (§4.2) ───────────────────────────────── */

typedef struct {
    uint8_t used; /* slot occupied                            */
    uint8_t loc;  /* index into sldf_table (capacity < 256)   */
} sldf_slot_t;

static sldf_slot_t sldf_index[SUTS_SLDF_INDEX_SIZE];
static bool        sldf_ready = false;

static void sldf_build(void)
{
    size_t i;

    for (i = 0; i < SUTS_SLDF_INDEX_SIZE; i++) {
        sldf_index[i].used = 0u;
        sldf_index[i].loc = 0u;
    }
    for (i = 0; i < SLDF_TABLE_COUNT; i++) {
        uint32_t h = sldf_tag_hash(sldf_table[i].tag_key) &
                     (SUTS_SLDF_INDEX_SIZE - 1u);
        while (sldf_index[h].used)
            h = (h + 1u) & (SUTS_SLDF_INDEX_SIZE - 1u);
        sldf_index[h].used = 1u;
        sldf_index[h].loc = (uint8_t)i;
    }
    sldf_ready = true;
}

/* ── Setup ────────────────────────────────────────────────────────── */

const char* sldf_version_string(void)
{
    return "1.0.0";
}

void sldf_init_registry(void)
{
    sldf_build();
}

/* ── RCCR tag helpers (§3) ────────────────────────────────────────── */

uint32_t sldf_tag_hash(const char* tag)
{
    char     buf[SUTS_SLDF_MAX_TAG_LEN];
    size_t   n;
    size_t   i;
    uint32_t h = SUTS_SLDF_FNV_OFFSET_BASIS;

    n = sldf_normalize(tag, buf, sizeof(buf));
    for (i = 0; i < n; i++) {
        h ^= (uint8_t)buf[i];
        h *= SUTS_SLDF_FNV_PRIME;
    }
    return h;
}

bool sldf_is_valid_tag(const char* tag)
{
    char   buf[SUTS_SLDF_MAX_TAG_LEN];
    size_t n;
    size_t i;
    int    part_len = 0;

    n = sldf_normalize(tag, buf, sizeof(buf));
    if (n == 0u)
        return false;
    if (!(buf[0] >= 'a' && buf[0] <= 'z'))
        return false; /* the language subtag begins with a letter */
    for (i = 0; i < n; i++) {
        char c = buf[i];
        if (c == '-') {
            if (part_len == 0)
                return false; /* leading, trailing, or doubled hyphen */
            part_len = 0;
        } else if (++part_len > 8) {
            return false; /* a subtag is at most 8 bytes */
        }
    }
    return part_len != 0;
}

bool sldf_tags_equal(const char* a, const char* b)
{
    char   ba[SUTS_SLDF_MAX_TAG_LEN];
    char   bb[SUTS_SLDF_MAX_TAG_LEN];
    size_t na = sldf_normalize(a, ba, sizeof(ba));
    size_t nb = sldf_normalize(b, bb, sizeof(bb));

    if (na == 0u || na != nb)
        return false;
    return sldf_buf_equal(ba, bb, na) != 0;
}

/* ── Resolution (§4.2) — O(1) expected ────────────────────────────── */

const sldf_locale_t* sldf_resolve_locale(const char* rccr_tag)
{
    char     norm[SUTS_SLDF_MAX_TAG_LEN];
    uint32_t h;
    uint32_t start;

    if (sldf_normalize(rccr_tag, norm, sizeof(norm)) == 0u)
        return NULL;
    if (!sldf_ready)
        sldf_build(); /* lazy build, so resolve is safe without setup */

    h = sldf_tag_hash(norm) & (SUTS_SLDF_INDEX_SIZE - 1u);
    start = h;
    for (;;) {
        const sldf_locale_t* cand;
        if (!sldf_index[h].used)
            return NULL;
        cand = &sldf_table[sldf_index[h].loc];
        if (sldf_tags_equal(cand->tag_key, norm))
            return cand; /* stable pointer into read-only memory */
        h = (h + 1u) & (SUTS_SLDF_INDEX_SIZE - 1u);
        if (h == start)
            return NULL;
    }
}

/* ── Enumeration (§4.1) ───────────────────────────────────────────── */

size_t sldf_locale_count(void)
{
    return SLDF_TABLE_COUNT;
}

const sldf_locale_t* sldf_locale_at(size_t index)
{
    if (index >= SLDF_TABLE_COUNT)
        return NULL;
    return &sldf_table[index];
}

/* ── Plane mapping and collation seed (§2.4, §5) ──────────────────── */

suts_sldf_plane_t sldf_plane_of(sucs_ex_char_t cp)
{
    if (cp <= SLDF_ZONE_UNICODE_MAX)
        return SUTS_SLDF_PLANE_BRIDGE;
    if (cp <= SLDF_ZONE_SCP_MAX)
        return SUTS_SLDF_PLANE_SCP;
    if (cp >= SLDF_ZONE_TRAP_MIN && cp <= SLDF_ZONE_TRAP_MAX)
        return SUTS_SLDF_PLANE_TRAP;
    if (cp == SLDF_ZONE_SENTINEL)
        return SUTS_SLDF_PLANE_SENTINEL;
    if (cp >= SLDF_RNUR_MIN && cp <= SLDF_RNUR_MAX)
        return SUTS_SLDF_PLANE_RNUR;
    if (cp >= SLDF_ZONE_NATIVE_MIN && cp <= SLDF_ZONE_NATIVE_LAST)
        return SUTS_SLDF_PLANE_NATIVE;
    return SUTS_SLDF_PLANE_EXTSUCS;
}

uint32_t sldf_collation_base(const sldf_locale_t* locale)
{
    uint32_t seed;

    if (!locale)
        return 0u;
    switch (locale->collation_plane) {
    case SUTS_SLDF_PLANE_SCP:
        seed = SLDF_COLLATION_SCP_SEED;
        break;
    case SUTS_SLDF_PLANE_NATIVE:
        seed = SLDF_COLLATION_NATIVE_SEED;
        break;
    case SUTS_SLDF_PLANE_RNUR:
        seed = SLDF_COLLATION_RNUR_SEED;
        break;
    default:
        seed = SLDF_COLLATION_BRIDGE_SEED;
        break;
    }
    return seed + locale->collation_base_offset;
}
