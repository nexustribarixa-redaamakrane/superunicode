/* SUTS-006 — System Internationalized Domain & Identifier Network Architecture
 * (SIDN)
 *
 * Reference implementation of docs/suts/SUTS-006-sidn.md.
 *
 * Freestanding C99, zero allocation. A static table of compiled IDNA mapping
 * records is indexed once into a fixed-size open-addressing array, so that a
 * codepoint's Status and mapping value are resolved in O(1) expected time. A
 * label is validated and canonicalized in a single linear pass over the input
 * stream: no heap, no dynamic growth, no Punycode, no libc.
 *
 * The runtime layer is fed at build time from the offline reference layer (the
 * SuperUnicode Character Database and the published IDNA mapping manifests);
 * the tables below are a curated subset of that dataset. Extending a table is a
 * data update (a minor revision) and never changes the API.
 */

#include "suts/suts_sidn.h"

/* ── Portable byte extraction ─────────────────────────────────────── */

static uint8_t sidn_byte_at(uint32_t cp, unsigned i)
{
    return (uint8_t)((cp >> (8u * i)) & 0xFFu);
}

/* Record the first defect and report failure. A NULL `out_error` is allowed. */
static bool sidn_fail(sidn_status_code_t* out_error, sidn_status_code_t code)
{
    if (out_error != NULL)
        *out_error = code;
    return false;
}

/* ── Compile-time record geometry (§2.2) ──────────────────────────── */

#define SIDN_STATIC_ASSERT(cond, name) \
    typedef char name[(cond) ? 1 : -1]

SIDN_STATIC_ASSERT(sizeof(sidn_mapping_entry_t) == 16u,
                   sidn_mapping_entry_is_16_bytes);
SIDN_STATIC_ASSERT(offsetof(sidn_mapping_entry_t, codepoint) == 0u,
                   sidn_entry_codepoint_at_0);
SIDN_STATIC_ASSERT(offsetof(sidn_mapping_entry_t, mapping) == 4u,
                   sidn_entry_mapping_at_4);
SIDN_STATIC_ASSERT(offsetof(sidn_mapping_entry_t, status) == 12u,
                   sidn_entry_status_at_12);
SIDN_STATIC_ASSERT(offsetof(sidn_mapping_entry_t, mapping_length) == 13u,
                   sidn_entry_length_at_13);
SIDN_STATIC_ASSERT(offsetof(sidn_mapping_entry_t, reserved) == 14u,
                   sidn_entry_reserved_at_14);

/* ── The curated static mapping table (§6) ────────────────────────── */

/* Ascending codepoint order is a verification invariant, not a convenience:
 * the enumeration surface and the duplicate check both depend on it. Only
 * codepoints whose Status is ignored, mapped or deviation (or a `valid` record
 * worth publishing explicitly) appear here; every other codepoint resolves to
 * SIDN_STATUS_DISALLOWED through the table's absence, which is why the mapping
 * function is total without the table being dense. */
static const sidn_mapping_entry_t sidn_table[] = {
    /* codepoint   mapping[2]                   status                ml  rsv */
    /* ASCII punctuation and digits that are valid (kept). */
    { 0x0000002Du, { 0u, 0u }, SIDN_STATUS_VALID,          0u, 0u }, /* - */
    { 0x0000002Eu, { 0u, 0u }, SIDN_STATUS_VALID,          0u, 0u }, /* . */
    { 0x00000030u, { 0u, 0u }, SIDN_STATUS_VALID,          0u, 0u }, /* 0 */
    { 0x00000031u, { 0u, 0u }, SIDN_STATUS_VALID,          0u, 0u }, /* 1 */
    { 0x00000032u, { 0u, 0u }, SIDN_STATUS_VALID,          0u, 0u }, /* 2 */
    { 0x00000033u, { 0u, 0u }, SIDN_STATUS_VALID,          0u, 0u }, /* 3 */
    { 0x00000034u, { 0u, 0u }, SIDN_STATUS_VALID,          0u, 0u }, /* 4 */
    { 0x00000035u, { 0u, 0u }, SIDN_STATUS_VALID,          0u, 0u }, /* 5 */
    { 0x00000036u, { 0u, 0u }, SIDN_STATUS_VALID,          0u, 0u }, /* 6 */
    { 0x00000037u, { 0u, 0u }, SIDN_STATUS_VALID,          0u, 0u }, /* 7 */
    { 0x00000038u, { 0u, 0u }, SIDN_STATUS_VALID,          0u, 0u }, /* 8 */
    { 0x00000039u, { 0u, 0u }, SIDN_STATUS_VALID,          0u, 0u }, /* 9 */

    /* ASCII: every uppercase letter maps to lower case. */
    { 0x00000041u, { 0x00000061u, 0u }, SIDN_STATUS_MAPPED,     1u, 0u }, /* A.. */
    { 0x00000042u, { 0x00000062u, 0u }, SIDN_STATUS_MAPPED,     1u, 0u },
    { 0x00000043u, { 0x00000063u, 0u }, SIDN_STATUS_MAPPED,     1u, 0u },
    { 0x00000044u, { 0x00000064u, 0u }, SIDN_STATUS_MAPPED,     1u, 0u },
    { 0x00000045u, { 0x00000065u, 0u }, SIDN_STATUS_MAPPED,     1u, 0u },
    { 0x00000046u, { 0x00000066u, 0u }, SIDN_STATUS_MAPPED,     1u, 0u },
    { 0x00000047u, { 0x00000067u, 0u }, SIDN_STATUS_MAPPED,     1u, 0u },
    { 0x00000048u, { 0x00000068u, 0u }, SIDN_STATUS_MAPPED,     1u, 0u },
    { 0x00000049u, { 0x00000069u, 0u }, SIDN_STATUS_MAPPED,     1u, 0u },
    { 0x0000004Au, { 0x0000006Au, 0u }, SIDN_STATUS_MAPPED,     1u, 0u },
    { 0x0000004Bu, { 0x0000006Bu, 0u }, SIDN_STATUS_MAPPED,     1u, 0u },
    { 0x0000004Cu, { 0x0000006Cu, 0u }, SIDN_STATUS_MAPPED,     1u, 0u },
    { 0x0000004Du, { 0x0000006Du, 0u }, SIDN_STATUS_MAPPED,     1u, 0u },
    { 0x0000004Eu, { 0x0000006Eu, 0u }, SIDN_STATUS_MAPPED,     1u, 0u },
    { 0x0000004Fu, { 0x0000006Fu, 0u }, SIDN_STATUS_MAPPED,     1u, 0u },
    { 0x00000050u, { 0x00000070u, 0u }, SIDN_STATUS_MAPPED,     1u, 0u },
    { 0x00000051u, { 0x00000071u, 0u }, SIDN_STATUS_MAPPED,     1u, 0u },
    { 0x00000052u, { 0x00000072u, 0u }, SIDN_STATUS_MAPPED,     1u, 0u },
    { 0x00000053u, { 0x00000073u, 0u }, SIDN_STATUS_MAPPED,     1u, 0u },
    { 0x00000054u, { 0x00000074u, 0u }, SIDN_STATUS_MAPPED,     1u, 0u },
    { 0x00000055u, { 0x00000075u, 0u }, SIDN_STATUS_MAPPED,     1u, 0u },
    { 0x00000056u, { 0x00000076u, 0u }, SIDN_STATUS_MAPPED,     1u, 0u },
    { 0x00000057u, { 0x00000077u, 0u }, SIDN_STATUS_MAPPED,     1u, 0u },
    { 0x00000058u, { 0x00000078u, 0u }, SIDN_STATUS_MAPPED,     1u, 0u },
    { 0x00000059u, { 0x00000079u, 0u }, SIDN_STATUS_MAPPED,     1u, 0u },
    { 0x0000005Au, { 0x0000007Au, 0u }, SIDN_STATUS_MAPPED,     1u, 0u },

    /* Lowercase letters that the curated subset publishes explicitly, and the
     * underbar (valid as a raw symbol; the STD3 criterion rejects it alone). */
    { 0x0000005Fu, { 0u, 0u }, SIDN_STATUS_VALID,          0u, 0u }, /* _ */
    { 0x00000061u, { 0u, 0u }, SIDN_STATUS_VALID,          0u, 0u }, /* a */
    { 0x00000065u, { 0u, 0u }, SIDN_STATUS_VALID,          0u, 0u }, /* e */
    { 0x0000006Fu, { 0u, 0u }, SIDN_STATUS_VALID,          0u, 0u }, /* o */
    { 0x00000073u, { 0u, 0u }, SIDN_STATUS_VALID,          0u, 0u }, /* s */
    { 0x0000007Au, { 0u, 0u }, SIDN_STATUS_VALID,          0u, 0u }, /* z */

    /* Ignored: soft hyphen. */
    { 0x000000ADu, { 0u, 0u }, SIDN_STATUS_IGNORED,        0u, 0u }, /* SOFT HYPHEN */

    /* The deviation characters (§3.2). In nontransitional processing they are
     * valid and kept; the mapping value is the transitional behaviour. */
    { 0x000000DFu, { 0x00000073u, 0x00000073u }, SIDN_STATUS_DEVIATION, 2u, 0u }, /* ß */
    { 0x000000FCu, { 0u, 0u }, SIDN_STATUS_VALID,          0u, 0u }, /* ü */
    { 0x000003C2u, { 0x000003C3u, 0u }, SIDN_STATUS_DEVIATION, 1u, 0u }, /* ς */

    { 0x00001E9Eu, { 0x000000DFu, 0u }, SIDN_STATUS_MAPPED,     1u, 0u }, /* ẞ */
    { 0x0000200Cu, { 0u, 0u }, SIDN_STATUS_DEVIATION,      1u, 0u }, /* ZWNJ */
    { 0x0000200Du, { 0u, 0u }, SIDN_STATUS_DEVIATION,      1u, 0u }, /* ZWJ */

    /* Disallowed: the digit-one-full-stop (its decomposition contains a dot)
     * and a symbol, both of which UTS #46 rejects. */
    { 0x00002488u, { 0u, 0u }, SIDN_STATUS_DISALLOWED,     0u, 0u }, /* ⒈ */
    { 0x00002665u, { 0u, 0u }, SIDN_STATUS_DISALLOWED,     0u, 0u }, /* ♥ */

    /* The four label separators fold to U+002E (§3.1, Mapping step 1.1.a). */
    { 0x00003002u, { 0x0000002Eu, 0u }, SIDN_STATUS_MAPPED,     1u, 0u }, /* 。 */
    { 0x0000FF0Eu, { 0x0000002Eu, 0u }, SIDN_STATUS_MAPPED,     1u, 0u }, /* ． */
    { 0x0000FF21u, { 0x00000061u, 0u }, SIDN_STATUS_MAPPED,     1u, 0u }, /* Ａ */
    { 0x0000FF61u, { 0x0000002Eu, 0u }, SIDN_STATUS_MAPPED,     1u, 0u }, /* ｡ */

    /* The RNUR neography carve-out: two representative valid codepoints. */
    { 0x00160100u, { 0u, 0u }, SIDN_STATUS_VALID,          0u, 0u }, /* RNUR */
    { 0x00160200u, { 0u, 0u }, SIDN_STATUS_VALID,          0u, 0u }  /* RNUR */
};

#define SIDN_TABLE_COUNT \
    ((uint32_t)(sizeof sidn_table / sizeof sidn_table[0]))

SIDN_STATIC_ASSERT(SIDN_TABLE_COUNT <= SIDN_MAPPING_CAPACITY,
                   sidn_table_fits_capacity);

/* ── The static lookup index (§2.4) ───────────────────────────────── */

typedef struct {
    uint16_t entry;
    uint8_t  used;
} sidn_index_slot_t;

static sidn_index_slot_t sidn_index[SIDN_MAPPING_INDEX_SIZE];
static bool sidn_ready = false;

/* ── FNV-1a primitives (§2.4) ─────────────────────────────────────── */

uint32_t suts_sidn_hash_codepoint(uint32_t codepoint)
{
    uint32_t h = SIDN_FNV_OFFSET_BASIS;
    unsigned i;

    for (i = 0u; i < 4u; i++) {
        h ^= (uint32_t)sidn_byte_at(codepoint, i);
        h *= SIDN_FNV_PRIME;
    }
    return h;
}

uint32_t suts_sidn_hash_label(const uint32_t* label, size_t length)
{
    uint32_t h = SIDN_FNV_OFFSET_BASIS;
    size_t i;
    unsigned b;

    if (label == NULL || length == 0u)
        return 0u;

    for (i = 0u; i < length; i++) {
        for (b = 0u; b < 4u; b++) {
            h ^= (uint32_t)sidn_byte_at(label[i], b);
            h *= SIDN_FNV_PRIME;
        }
    }
    return h;
}

/* ── Zone dispatch (§5) ───────────────────────────────────────────── */

sidn_zone_t suts_sidn_zone_of(uint32_t codepoint)
{
    if (codepoint > SIDN_LIMIT_SENTINEL)
        return SIDN_ZONE_EXTSUCS;
    if (codepoint <= SIDN_LIMIT_UNICODE_MAX)
        return SIDN_ZONE_BRIDGE;
    if (codepoint >= SIDN_LIMIT_SCP_MIN && codepoint <= SIDN_LIMIT_SCP_MAX)
        return SIDN_ZONE_SCP;
    if (codepoint >= SIDN_LIMIT_TRAP_MIN && codepoint <= SIDN_LIMIT_TRAP_MAX)
        return SIDN_ZONE_TRAP;
    if (codepoint == SIDN_LIMIT_SENTINEL)
        return SIDN_ZONE_SENTINEL;
    if (codepoint >= SIDN_RNUR_MIN && codepoint <= SIDN_RNUR_MAX)
        return SIDN_ZONE_RNUR;
    return SIDN_ZONE_NATIVE;
}

bool suts_sidn_is_routable(uint32_t codepoint)
{
    switch (suts_sidn_zone_of(codepoint)) {
    case SIDN_ZONE_BRIDGE:
    case SIDN_ZONE_RNUR:
    case SIDN_ZONE_NATIVE:
        return true;
    default:
        return false;
    }
}

bool suts_sidn_is_neography(uint32_t codepoint)
{
    return suts_sidn_zone_of(codepoint) == SIDN_ZONE_RNUR;
}

bool suts_sidn_neography_set(uint32_t codepoint, uint32_t* out_set)
{
    if (out_set == NULL || !suts_sidn_is_neography(codepoint))
        return false;
    *out_set = (codepoint - SIDN_RNUR_MIN) >> SIDN_RNUR_SET_SHIFT;
    return true;
}

/* ── Lazy index construction (§2.4) ───────────────────────────────── */

static void sidn_build(void)
{
    uint32_t i;

    for (i = 0u; i < SIDN_MAPPING_INDEX_SIZE; i++) {
        sidn_index[i].entry = 0u;
        sidn_index[i].used = 0u;
    }
    for (i = 0u; i < SIDN_TABLE_COUNT; i++) {
        uint32_t h = suts_sidn_hash_codepoint(sidn_table[i].codepoint) &
                     (SIDN_MAPPING_INDEX_SIZE - 1u);
        uint32_t start = h;
        while (sidn_index[h].used) {
            h = (h + 1u) & (SIDN_MAPPING_INDEX_SIZE - 1u);
            if (h == start)
                break; /* unreachable: capacity < index size */
        }
        sidn_index[h].entry = (uint16_t)i;
        sidn_index[h].used = 1u;
    }
    sidn_ready = true;
}

void suts_sidn_init_engine(void)
{
    if (!sidn_ready)
        sidn_build();
}

/* ── O(1) probe (§2.4) ────────────────────────────────────────────── */

static const sidn_mapping_entry_t* sidn_find(uint32_t codepoint)
{
    uint32_t h;
    uint32_t start;

    if (!suts_sidn_is_routable(codepoint))
        return NULL;
    if (!sidn_ready)
        sidn_build();

    h = suts_sidn_hash_codepoint(codepoint) & (SIDN_MAPPING_INDEX_SIZE - 1u);
    start = h;
    for (;;) {
        const sidn_mapping_entry_t* cand;
        if (!sidn_index[h].used)
            return NULL;
        cand = &sidn_table[sidn_index[h].entry];
        if (cand->codepoint == codepoint)
            return cand;
        h = (h + 1u) & (SIDN_MAPPING_INDEX_SIZE - 1u);
        if (h == start)
            return NULL;
    }
}

bool suts_sidn_mapping_of(uint32_t codepoint, sidn_mapping_entry_t* out_entry)
{
    const sidn_mapping_entry_t* found;

    if (out_entry == NULL)
        return false;
    found = sidn_find(codepoint);
    if (found == NULL)
        return false;
    *out_entry = *found;
    return true;
}

sidn_status_t suts_sidn_status_of(uint32_t codepoint)
{
    const sidn_mapping_entry_t* found = sidn_find(codepoint);

    if (found == NULL)
        return SIDN_STATUS_DISALLOWED; /* closed by default */
    return (sidn_status_t)found->status;
}

bool suts_sidn_is_admissible(uint32_t codepoint)
{
    sidn_status_t s = suts_sidn_status_of(codepoint);
    return s == SIDN_STATUS_VALID || s == SIDN_STATUS_DEVIATION;
}

uint32_t suts_sidn_mapping_count(void)
{
    return SIDN_TABLE_COUNT;
}

bool suts_sidn_mapping_at(uint32_t index, sidn_mapping_entry_t* out_entry)
{
    if (out_entry == NULL || index >= SIDN_TABLE_COUNT)
        return false;
    *out_entry = sidn_table[index];
    return true;
}

/* ── Default validity for uncurated codepoints (§6) ───────────────── */
/* The reference mapping table is a curated subset of the offline dataset, so a
 * codepoint with no record is not automatically disallowed. It is valid when it
 * is a routable codepoint that is neither a control/format codepoint in the low
 * ASCII range nor an explicitly disallowed ASCII symbol. This makes a plain
 * letter such as 'b' or a Han ideograph valid without a per-codepoint record,
 * while the digit-one-full-stop and a black heart suit carry explicit records. */
static bool sidn_default_valid(uint32_t c)
{
    if (!suts_sidn_is_routable(c))
        return false;
    if (c < 0x0021u)
        return false;              /* controls and space           */
    if (c == 0x0021u || (c >= 0x0022u && c <= 0x002Cu))
        return false;              /* " ! # $ % & ' ( ) * + ,      */
    if (c == 0x002Fu || c == 0x003Au || c == 0x003Bu)
        return false;              /* / : ;                        */
    if (c >= 0x003Cu && c <= 0x0040u)
        return false;              /* < = > ? @                    */
    if (c >= 0x005Bu && c <= 0x0060u)
        return false;              /* [ \ ] ^ _ ` ( _ curated)     */
    if (c >= 0x007Bu && c <= 0x007Fu)
        return false;              /* { | } ~ DEL                  */
    if (c >= 0x0080u && c <= 0x009Fu)
        return false;              /* C1 controls                  */
    return true;
}

/* ── Label separators (§3.1) ──────────────────────────────────────── */
bool suts_sidn_is_label_separator(uint32_t codepoint)
{
    return codepoint == 0x002Eu || codepoint == 0xFF0Eu ||
           codepoint == 0x3002u || codepoint == 0xFF61u;
}

bool suts_sidn_has_reserved_prefix(const uint32_t* label, size_t length)
{
    if (label == NULL || length < 3u)
        return false;
    return label[0] == SIDN_RESERVED_PREFIX_0 &&
           label[1] == SIDN_RESERVED_PREFIX_1 &&
           label[2] == SIDN_RESERVED_PREFIX_2;
}

/* ── Validity criteria (§4.2) ─────────────────────────────────────── */

bool suts_sidn_check_hyphens(const uint32_t* label, size_t length)
{
    if (label == NULL || length == 0u)
        return false;
    if (label[0] == 0x002Du || label[length - 1u] == 0x002Du)
        return false;
    if (length >= 4u && label[2] == 0x002Du && label[3] == 0x002Du)
        return false;
    return true;
}

bool suts_sidn_check_std3(const uint32_t* label, size_t length)
{
    size_t i;

    if (label == NULL)
        return false;
    for (i = 0u; i < length; i++) {
        uint32_t c = label[i];
        if (c > 0x007Fu)
            continue; /* STD3 constrains ASCII only */
        if ((c >= 0x0061u && c <= 0x007Au) ||
            (c >= 0x0030u && c <= 0x0039u) ||
            c == 0x002Du)
            continue;
        return false;
    }
    return true;
}

/* ContextJ (§4.2): a joiner is admissible only inside a Devanagari / Indic or
 * Arabic shaping context. This revision recognizes the two canonical contexts
 * the standard names: ZWNJ between two Arabic letters, and ZWNJ immediately
 * after a virama (a Devanagari conjunct joiner); ZWJ after a virama. */
bool suts_sidn_check_joiners(const uint32_t* label, size_t length)
{
    size_t i;

    if (label == NULL)
        return true;
    for (i = 0u; i < length; i++) {
        uint32_t c = label[i];
        if (c != 0x200Cu && c != 0x200Du)
            continue;
        /* ZWNJ between two Arabic letters. */
        if (c == 0x200Cu && i > 0u && i + 1u < length) {
            uint32_t prev = label[i - 1u];
            uint32_t next = label[i + 1u];
            if (prev >= 0x0620u && prev <= 0x064Fu &&
                next >= 0x0620u && next <= 0x064Fu)
                continue;
        }
        /* A joiner after a Devanagari virama (U+094D). */
        if (i > 0u && label[i - 1u] == 0x094Du)
            continue;
        return false;
    }
    return true;
}

/* The Bidi classes SIDN models: enough to recognize a Bidi domain and to check
 * the two structural rules this revision enforces (a label may not begin with a
 * combining mark, and an RTL label may not mix LTR letters without an enclosing
 * mark). A full RFC 5893 evaluator is out of scope for the reference engine. */
static bool sidn_is_rtl_letter(uint32_t c)
{
    return (c >= 0x0590u && c <= 0x05FFu) ||   /* Hebrew */
           (c >= 0x0600u && c <= 0x06FFu) ||   /* Arabic */
           (c >= 0x0750u && c <= 0x077Fu);     /* Arabic Supplement */
}

static bool sidn_is_ltr_letter(uint32_t c)
{
    return (c >= 0x0041u && c <= 0x005Au) ||
           (c >= 0x0061u && c <= 0x007Au) ||
           (c >= 0x00C0u && c <= 0x024Fu);
}

static bool sidn_is_combining_mark(uint32_t c)
{
    return (c >= 0x0300u && c <= 0x036Fu) ||   /* Combining Diacritical Marks */
           (c >= 0x0591u && c <= 0x05BDu) ||   /* Hebrew points */
           (c >= 0x0610u && c <= 0x061Au) ||   /* Arabic marks  */
           (c >= 0x0900u && c <= 0x0903u) ||   /* Devanagari marks */
           (c >= 0x093Au && c <= 0x094Fu);
}

bool suts_sidn_is_bidi_domain(const uint32_t* label, size_t length)
{
    size_t i;

    if (label == NULL)
        return false;
    for (i = 0u; i < length; i++) {
        if (sidn_is_rtl_letter(label[i]))
            return true;
    }
    return false;
}

bool suts_sidn_check_bidi(const uint32_t* label, size_t length)
{
    size_t i;
    bool has_rtl = false;
    bool has_ltr = false;

    if (label == NULL || length == 0u)
        return false;

    /* Criterion 6: a label may not begin with a combining mark. */
    if (sidn_is_combining_mark(label[0]))
        return false;

    for (i = 0u; i < length; i++) {
        if (sidn_is_rtl_letter(label[i]))
            has_rtl = true;
        if (sidn_is_ltr_letter(label[i]))
            has_ltr = true;
    }
    /* A Bidi label that mixes RTL and LTR letters must end in an RTL letter
     * (the RFC 5893 rule 2 / 5 shape this revision models). */
    if (has_rtl && has_ltr) {
        if (!sidn_is_rtl_letter(label[length - 1u]))
            return false;
    }
    return true;
}

/* ── Label validation (§4) ────────────────────────────────────────── */

/* The processing core: one linear pass over the input. Folds a separator into
 * the canonical boundary, applies the mapping Status of every codepoint, and
 * validates. When `out` is not NULL the mapped codepoints are appended to it
 * and `*out_len` is updated. On any outcome the descriptor is filled in: on
 * failure it carries SIDN_FLAG_DISALLOWED and the count of codepoints processed
 * so far. */
static bool sidn_process(const uint32_t* input, size_t length,
                         sidn_mode_t mode,
                         uint32_t* out, size_t out_cap, size_t* out_len,
                         sidn_label_descriptor_t* desc,
                         sidn_status_code_t* out_error)
{
    size_t i;
    size_t written = 0u;
    bool saw_deviation = false;
    bool saw_neography = false;
    bool empty = true;
    bool ok = true;
    sidn_status_code_t code = SIDN_OK;

    if (input == NULL || desc == NULL)
        return sidn_fail(out_error, SIDN_ERR_NULL_ARGUMENT);

    desc->label_hash = SIDN_FNV_OFFSET_BASIS;
    desc->length = 0u;
    desc->flags = 0u;

    for (i = 0u; i < length; i++) {
        uint32_t c = input[i];
        const sidn_mapping_entry_t* rec;
        sidn_status_t status;
        unsigned ml = 0u;

        if (suts_sidn_is_label_separator(c))
            break; /* the label ends at the first separator */

        /* Refuse a control-plane, trap, sentinel or plugin codepoint before
         * consulting the table: those zones are never label material. */
        if (!suts_sidn_is_routable(c)) {
            code = SIDN_ERR_DISALLOWED;
            goto done;
        }

        rec = sidn_find(c);
        if (rec == NULL) {
            /* The reference table is a curated subset, so an uncurated
             * routable codepoint is valid unless it is in a range the standard
             * disallows outright. */
            if (!sidn_default_valid(c)) {
                code = SIDN_ERR_DISALLOWED;
                goto done;
            }
            status = SIDN_STATUS_VALID;
        } else {
            status = (sidn_status_t)rec->status;
            ml = rec->mapping_length;
        }

        switch (status) {
        case SIDN_STATUS_IGNORED:
            continue; /* removed; does not make a label non-empty */
        case SIDN_STATUS_MAPPED:
            if (out != NULL) {
                unsigned k;
                if (written + ml > out_cap) {
                    code = SIDN_ERR_OUTPUT_TOO_SMALL;
                    goto done;
                }
                for (k = 0u; k < ml; k++)
                    out[written++] = rec->mapping[k];
            } else {
                written += ml;
            }
            empty = false;
            continue;
        case SIDN_STATUS_DEVIATION:
            saw_deviation = true;
            if (mode == SIDN_MODE_TRANSITIONAL) {
                if (out != NULL) {
                    unsigned k;
                    if (written + ml > out_cap) {
                        code = SIDN_ERR_OUTPUT_TOO_SMALL;
                        goto done;
                    }
                    for (k = 0u; k < ml; k++)
                        out[written++] = rec->mapping[k];
                } else {
                    written += ml;
                }
                empty = false;
                continue;
            }
            /* Nontransitional: a deviation is kept as-is (valid). */
            break;
        case SIDN_STATUS_DISALLOWED:
            code = SIDN_ERR_DISALLOWED;
            goto done;
        case SIDN_STATUS_VALID:
        default:
            break;
        }

        /* Status valid (or a kept deviation): emit the codepoint itself. */
        if (out != NULL) {
            if (written + 1u > out_cap) {
                code = SIDN_ERR_OUTPUT_TOO_SMALL;
                goto done;
            }
            out[written] = c;
        }
        written++;
        empty = false;

        if (suts_sidn_is_neography(c))
            saw_neography = true;
    }

    if (empty) {
        code = SIDN_ERR_LABEL_EMPTY;
        goto done;
    }

    /* Validity criteria (§4.2), applied to the emitted stream. */
    if (out != NULL) {
        if (out[0] == SIDN_LABEL_SEPARATOR) {
            code = SIDN_ERR_INTERNAL_STOP;
            goto done;
        }
        if (suts_sidn_has_reserved_prefix(out, written)) {
            code = SIDN_ERR_RESERVED_PREFIX;
            goto done;
        }
        if (!suts_sidn_check_hyphens(out, written)) {
            code = SIDN_ERR_HYPHEN_POSITION;
            goto done;
        }
        if (!suts_sidn_check_std3(out, written)) {
            code = SIDN_ERR_STD3_ASCII;
            goto done;
        }
        if (!suts_sidn_check_joiners(out, written)) {
            code = SIDN_ERR_CONTEXTJ;
            goto done;
        }
        if (!suts_sidn_check_bidi(out, written)) {
            code = SIDN_ERR_BIDI;
            goto done;
        }
        if (written > SIDN_LABEL_MAX_DNS) {
            code = SIDN_ERR_LABEL_TOO_LONG;
            goto done;
        }
    }

    if (out_len != NULL)
        *out_len = written;

    desc->length = (uint16_t)(written > 0xFFFFu ? 0xFFFFu : written);
    desc->label_hash = (out != NULL)
        ? suts_sidn_hash_label(out, written)
        : (uint32_t)written;
    desc->flags = SIDN_FLAG_VALID_LABEL;
    if (saw_deviation)
        desc->flags |= SIDN_FLAG_TRANSITIONAL;
    if (saw_neography)
        desc->flags |= SIDN_FLAG_NEOGRAPHY_DOMAIN;
    if (out_error != NULL)
        *out_error = SIDN_OK;
    return true;

done:
    if (out_len != NULL)
        *out_len = written;
    desc->length = (uint16_t)(written > 0xFFFFu ? 0xFFFFu : written);
    desc->label_hash = (out != NULL) ? suts_sidn_hash_label(out, written)
                                     : (uint32_t)written;
    desc->flags = SIDN_FLAG_DISALLOWED;
    (void)ok;
    return sidn_fail(out_error, code);
}

bool suts_sidn_validate_label(const uint32_t* label_stream, size_t length,
                              sidn_label_descriptor_t* out_desc,
                              sidn_status_code_t* out_error)
{
    /* Validation still materializes the mapped stream into a bounded stack
     * buffer: the validity criteria (§4.2) are defined over the *mapped* label,
     * not the raw input, so a validator that skipped the emitted form would
     * miss a defect such as a non-STD3 ASCII codepoint that survived mapping.
     * The buffer is SIDN_LABEL_MAX codepoints — the largest label the routing
     * table admits — so no allocation is ever needed and an over-long label is
     * caught by the SIDN_ERR_LABEL_TOO_LONG criterion. */
    uint32_t scratch[SIDN_LABEL_MAX];

    return sidn_process(label_stream, length, SIDN_MODE_NONTRANSITIONAL,
                        scratch, SIDN_LABEL_MAX, NULL, out_desc, out_error);
}

bool suts_sidn_to_canonical(const uint32_t* input_stream, size_t input_len,
                            uint32_t* output_stream, size_t* output_len,
                            sidn_status_code_t* out_error)
{
    sidn_label_descriptor_t desc;

    if (output_stream == NULL)
        return sidn_fail(out_error, SIDN_ERR_NULL_ARGUMENT);
    return sidn_process(input_stream, input_len, SIDN_MODE_NONTRANSITIONAL,
                        output_stream, SIDN_LABEL_MAX, output_len, &desc,
                        out_error);
}

/* ── Taxonomy names ───────────────────────────────────────────────── */

const char* suts_sidn_status_name(sidn_status_t status)
{
    switch (status) {
    case SIDN_STATUS_VALID:      return "valid";
    case SIDN_STATUS_IGNORED:    return "ignored";
    case SIDN_STATUS_MAPPED:     return "mapped";
    case SIDN_STATUS_DEVIATION:  return "deviation";
    case SIDN_STATUS_DISALLOWED: return "disallowed";
    default:                     return "unknown";
    }
}

const char* suts_sidn_zone_name(sidn_zone_t zone)
{
    switch (zone) {
    case SIDN_ZONE_BRIDGE:   return "BRIDGE";
    case SIDN_ZONE_SCP:      return "SCP";
    case SIDN_ZONE_RNUR:     return "RNUR";
    case SIDN_ZONE_NATIVE:   return "NATIVE";
    case SIDN_ZONE_TRAP:     return "TRAP";
    case SIDN_ZONE_SENTINEL: return "SENTINEL";
    case SIDN_ZONE_EXTSUCS:  return "EXTSUCS";
    default:                 return "UNKNOWN";
    }
}

const char* suts_sidn_status_code_name(sidn_status_code_t code)
{
    switch (code) {
    case SIDN_OK:                    return "OK";
    case SIDN_ERR_NULL_ARGUMENT:     return "NULL_ARGUMENT";
    case SIDN_ERR_OUTPUT_TOO_SMALL:  return "OUTPUT_TOO_SMALL";
    case SIDN_ERR_LABEL_EMPTY:       return "LABEL_EMPTY";
    case SIDN_ERR_DISALLOWED:        return "DISALLOWED";
    case SIDN_ERR_TRANSITIONAL_ONLY: return "TRANSITIONAL_ONLY";
    case SIDN_ERR_LEADING_COMBINING: return "LEADING_COMBINING";
    case SIDN_ERR_INTERNAL_STOP:     return "INTERNAL_STOP";
    case SIDN_ERR_HYPHEN_POSITION:   return "HYPHEN_POSITION";
    case SIDN_ERR_EDGE_HYPHEN:       return "EDGE_HYPHEN";
    case SIDN_ERR_RESERVED_PREFIX:   return "RESERVED_PREFIX";
    case SIDN_ERR_STD3_ASCII:        return "STD3_ASCII";
    case SIDN_ERR_CONTEXTJ:          return "CONTEXTJ";
    case SIDN_ERR_BIDI:              return "BIDI";
    case SIDN_ERR_LABEL_TOO_LONG:    return "LABEL_TOO_LONG";
    case SIDN_ERR_NO_MAPPING:        return "NO_MAPPING";
    default:                         return "UNKNOWN";
    }
}

/* ── Version ──────────────────────────────────────────────────────── */

const char* suts_sidn_version_string(void)
{
    return "1.0";
}
