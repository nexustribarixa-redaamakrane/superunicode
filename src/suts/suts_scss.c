/* SUTS-005 — System Character Security Specification (SCSS)
 *
 * Reference implementation of docs/suts/SUTS-005-scss.md.
 *
 * Freestanding C99, zero allocation. Character security data is compiled into
 * two immutable, statically-linked tables of fixed-size records:
 *
 *   - scss_security_attr_t    — a per-codepoint security attribute word;
 *   - scss_skeleton_entry_t   — a confusable -> prototype skeleton mapping.
 *
 * Each table is indexed once into a fixed-size open-addressing array keyed by
 * the FNV-1a hash of the codepoint, so the microkernel, the shell parser, and
 * the authentication path evaluate an identifier in O(1) expected time with no
 * heap, no flat-file parsing, and no dynamic table growth.
 *
 * The curated tables hold representative homographs across the Latin, Greek,
 * Cyrillic, and RNUR neography scripts, the invisible / control codepoints a
 * secure identifier must reject, and the script-neutral codepoints that are
 * always admissible. Extending them is a compile-time data update (a minor
 * revision) and never changes the API.
 */

#include "suts/suts_scss.h"

/* ── The curated attribute table (§2.1, Annex A) ──────────────────── */

static const scss_security_attr_t scss_attributes[] = {
    /* ── Cyrillic lowercase homoglyphs of Latin (Bridge) ──────────── */
    { .codepoint = 0x0430ULL, .flags = SUTS_SCSS_FLAG_CONFUSABLE }, /* a */
    { .codepoint = 0x0435ULL, .flags = SUTS_SCSS_FLAG_CONFUSABLE }, /* e */
    { .codepoint = 0x043EULL, .flags = SUTS_SCSS_FLAG_CONFUSABLE }, /* o */
    { .codepoint = 0x0440ULL, .flags = SUTS_SCSS_FLAG_CONFUSABLE }, /* p */
    { .codepoint = 0x0441ULL, .flags = SUTS_SCSS_FLAG_CONFUSABLE }, /* c */
    { .codepoint = 0x0445ULL, .flags = SUTS_SCSS_FLAG_CONFUSABLE }, /* x */
    { .codepoint = 0x0443ULL, .flags = SUTS_SCSS_FLAG_CONFUSABLE }, /* y */
    { .codepoint = 0x043AULL, .flags = SUTS_SCSS_FLAG_CONFUSABLE }, /* k */
    { .codepoint = 0x043CULL, .flags = SUTS_SCSS_FLAG_CONFUSABLE }, /* m */
    { .codepoint = 0x0442ULL, .flags = SUTS_SCSS_FLAG_CONFUSABLE }, /* t */

    /* ── Cyrillic uppercase homoglyphs of Latin (Bridge) ──────────── */
    { .codepoint = 0x0410ULL, .flags = SUTS_SCSS_FLAG_CONFUSABLE }, /* A */
    { .codepoint = 0x0415ULL, .flags = SUTS_SCSS_FLAG_CONFUSABLE }, /* E */
    { .codepoint = 0x041EULL, .flags = SUTS_SCSS_FLAG_CONFUSABLE }, /* O */
    { .codepoint = 0x0421ULL, .flags = SUTS_SCSS_FLAG_CONFUSABLE }, /* C */
    { .codepoint = 0x0420ULL, .flags = SUTS_SCSS_FLAG_CONFUSABLE }, /* P */
    { .codepoint = 0x0422ULL, .flags = SUTS_SCSS_FLAG_CONFUSABLE }, /* T */

    /* ── Greek homoglyphs of Latin (Bridge) ───────────────────────── */
    { .codepoint = 0x03BFULL, .flags = SUTS_SCSS_FLAG_CONFUSABLE }, /* o */
    { .codepoint = 0x03B1ULL, .flags = SUTS_SCSS_FLAG_CONFUSABLE }, /* a */
    { .codepoint = 0x03BDULL, .flags = SUTS_SCSS_FLAG_CONFUSABLE }, /* v */
    { .codepoint = 0x039FULL, .flags = SUTS_SCSS_FLAG_CONFUSABLE }, /* O */

    /* ── RNUR neography lookalikes (Native & Neography) ───────────── */
    { .codepoint = 0x160100ULL,
      .flags = SUTS_SCSS_FLAG_CONFUSABLE | SUTS_SCSS_FLAG_NEOGRAPHY_MIXED },
    { .codepoint = 0x160200ULL,
      .flags = SUTS_SCSS_FLAG_CONFUSABLE | SUTS_SCSS_FLAG_NEOGRAPHY_MIXED },

    /* ── Invisible / control codepoints: restricted ───────────────── */
    { .codepoint = 0x00ADULL, .flags = SUTS_SCSS_FLAG_RESTRICTED }, /* SOFT HYPHEN  */
    { .codepoint = 0x200BULL, .flags = SUTS_SCSS_FLAG_RESTRICTED }, /* ZWSP         */
    { .codepoint = 0x200CULL, .flags = SUTS_SCSS_FLAG_RESTRICTED }, /* ZWNJ         */
    { .codepoint = 0x200DULL, .flags = SUTS_SCSS_FLAG_RESTRICTED }, /* ZWJ          */
    { .codepoint = 0x202EULL, .flags = SUTS_SCSS_FLAG_RESTRICTED }, /* RLO          */
    { .codepoint = 0x2066ULL, .flags = SUTS_SCSS_FLAG_RESTRICTED }, /* LRI          */
    { .codepoint = 0xFEFFULL, .flags = SUTS_SCSS_FLAG_RESTRICTED }, /* ZWNBSP / BOM */

    /* ── Curated allowed ideographs (Bridge) ──────────────────────── */
    { .codepoint = 0x4E00ULL, .flags = SUTS_SCSS_FLAG_ALLOWED_IDENTIFIER },
    { .codepoint = 0x4E8CULL, .flags = SUTS_SCSS_FLAG_ALLOWED_IDENTIFIER },
};

#define SCSS_ATTR_COUNT \
    ((size_t)(sizeof(scss_attributes) / sizeof(scss_attributes[0])))

/* ── The curated skeleton table (§2.2, Annex A) ───────────────────── */

static const scss_skeleton_entry_t scss_skeletons[] = {
    /* Cyrillic lowercase -> Latin lowercase */
    { .codepoint = 0x0430ULL, .skeleton_codepoint = 0x0061ULL }, /* a */
    { .codepoint = 0x0435ULL, .skeleton_codepoint = 0x0065ULL }, /* e */
    { .codepoint = 0x043EULL, .skeleton_codepoint = 0x006FULL }, /* o */
    { .codepoint = 0x0440ULL, .skeleton_codepoint = 0x0070ULL }, /* p */
    { .codepoint = 0x0441ULL, .skeleton_codepoint = 0x0063ULL }, /* c */
    { .codepoint = 0x0445ULL, .skeleton_codepoint = 0x0078ULL }, /* x */
    { .codepoint = 0x0443ULL, .skeleton_codepoint = 0x0079ULL }, /* y */
    { .codepoint = 0x043AULL, .skeleton_codepoint = 0x006BULL }, /* k */
    { .codepoint = 0x043CULL, .skeleton_codepoint = 0x006DULL }, /* m */
    { .codepoint = 0x0442ULL, .skeleton_codepoint = 0x0074ULL }, /* t */

    /* Cyrillic uppercase -> Latin uppercase */
    { .codepoint = 0x0410ULL, .skeleton_codepoint = 0x0041ULL }, /* A */
    { .codepoint = 0x0415ULL, .skeleton_codepoint = 0x0045ULL }, /* E */
    { .codepoint = 0x041EULL, .skeleton_codepoint = 0x004FULL }, /* O */
    { .codepoint = 0x0421ULL, .skeleton_codepoint = 0x0043ULL }, /* C */
    { .codepoint = 0x0420ULL, .skeleton_codepoint = 0x0050ULL }, /* P */
    { .codepoint = 0x0422ULL, .skeleton_codepoint = 0x0054ULL }, /* T */

    /* Greek -> Latin */
    { .codepoint = 0x03BFULL, .skeleton_codepoint = 0x006FULL }, /* o */
    { .codepoint = 0x03B1ULL, .skeleton_codepoint = 0x0061ULL }, /* a */
    { .codepoint = 0x03BDULL, .skeleton_codepoint = 0x0076ULL }, /* v */
    { .codepoint = 0x039FULL, .skeleton_codepoint = 0x004FULL }, /* O */

    /* RNUR neography lookalikes -> Latin */
    { .codepoint = 0x160100ULL, .skeleton_codepoint = 0x0061ULL }, /* a */
    { .codepoint = 0x160200ULL, .skeleton_codepoint = 0x006FULL }, /* o */
};

#define SCSS_SKELETON_COUNT \
    ((size_t)(sizeof(scss_skeletons) / sizeof(scss_skeletons[0])))

/* ── The static lookup indexes (§4.2) ─────────────────────────────── */

typedef struct {
    uint8_t used; /* slot occupied                          */
    uint8_t loc;  /* index into its table (capacity < 256)  */
} scss_slot_t;

static scss_slot_t scss_attr_index[SUTS_SCSS_INDEX_SIZE];
static scss_slot_t scss_skel_index[SUTS_SCSS_INDEX_SIZE];
static bool        scss_ready = false;

static void scss_build(void)
{
    size_t i;

    for (i = 0; i < SUTS_SCSS_INDEX_SIZE; i++) {
        scss_attr_index[i].used = 0u;
        scss_attr_index[i].loc = 0u;
        scss_skel_index[i].used = 0u;
        scss_skel_index[i].loc = 0u;
    }
    for (i = 0; i < SCSS_ATTR_COUNT; i++) {
        uint32_t h = scss_hash(scss_attributes[i].codepoint) &
                     (SUTS_SCSS_INDEX_SIZE - 1u);
        while (scss_attr_index[h].used)
            h = (h + 1u) & (SUTS_SCSS_INDEX_SIZE - 1u);
        scss_attr_index[h].used = 1u;
        scss_attr_index[h].loc = (uint8_t)i;
    }
    for (i = 0; i < SCSS_SKELETON_COUNT; i++) {
        uint32_t h = scss_hash(scss_skeletons[i].codepoint) &
                     (SUTS_SCSS_INDEX_SIZE - 1u);
        while (scss_skel_index[h].used)
            h = (h + 1u) & (SUTS_SCSS_INDEX_SIZE - 1u);
        scss_skel_index[h].used = 1u;
        scss_skel_index[h].loc = (uint8_t)i;
    }
    scss_ready = true;
}

/* ── Setup ────────────────────────────────────────────────────────── */

const char* scss_version_string(void)
{
    return "1.0.0";
}

void scss_init_registry(void)
{
    scss_build();
}

/* ── Codepoint hash (§4.2) ────────────────────────────────────────── */

uint32_t scss_hash(sucs_ex_char_t codepoint)
{
    uint32_t h = SUTS_SCSS_FNV_OFFSET_BASIS;
    int      i;

    for (i = 0; i < 8; i++) {
        h ^= (uint8_t)((codepoint >> (8 * i)) & 0xFFu);
        h *= SUTS_SCSS_FNV_PRIME;
    }
    return h;
}

/* ── Zone and script classification (§2.3, §5) ────────────────────── */

static bool scss_is_control_zone(sucs_ex_char_t cp)
{
    if (cp >= SCSS_ZONE_SCP_MIN && cp <= SCSS_ZONE_SCP_MAX)
        return true; /* System Control Plane marker  */
    if (cp >= SCSS_ZONE_TRAP_MIN && cp <= SCSS_ZONE_TRAP_MAX)
        return true; /* trap slot                    */
    if (cp == SCSS_ZONE_SENTINEL)
        return true; /* Sentinel                     */
    return false;
}

bool scss_is_bridge_codepoint(sucs_ex_char_t cp)
{
    return cp <= SCSS_ZONE_UNICODE_MAX;
}

bool scss_is_native_codepoint(sucs_ex_char_t cp)
{
    return cp >= SCSS_ZONE_NATIVE_MIN && cp <= SCSS_ZONE_NATIVE_LAST;
}

static bool scss_is_ascii_identifier(sucs_ex_char_t cp)
{
    if (cp >= 0x41ULL && cp <= 0x5AULL)
        return true; /* A-Z */
    if (cp >= 0x61ULL && cp <= 0x7AULL)
        return true; /* a-z */
    if (cp >= 0x30ULL && cp <= 0x39ULL)
        return true; /* 0-9 */
    if (cp == 0x5FULL)
        return true; /* _   */
    if (cp == 0x2DULL)
        return true; /* -   */
    return false;
}

suts_scss_plane_t scss_plane_of(sucs_ex_char_t cp)
{
    if (cp <= SCSS_ZONE_UNICODE_MAX)
        return SUTS_SCSS_PLANE_BRIDGE;
    if (cp <= SCSS_ZONE_SCP_MAX)
        return SUTS_SCSS_PLANE_SCP;
    if (cp >= SCSS_ZONE_TRAP_MIN && cp <= SCSS_ZONE_TRAP_MAX)
        return SUTS_SCSS_PLANE_TRAP;
    if (cp == SCSS_ZONE_SENTINEL)
        return SUTS_SCSS_PLANE_SENTINEL;
    if (cp >= SCSS_RNUR_MIN && cp <= SCSS_RNUR_MAX)
        return SUTS_SCSS_PLANE_RNUR;
    if (cp >= SCSS_ZONE_NATIVE_MIN && cp <= SCSS_ZONE_NATIVE_LAST)
        return SUTS_SCSS_PLANE_NATIVE;
    return SUTS_SCSS_PLANE_EXTSUCS;
}

suts_scss_script_t scss_script_of(sucs_ex_char_t cp)
{
    if (scss_is_control_zone(cp))
        return SUTS_SCSS_SCRIPT_SCP;
    if (scss_is_native_codepoint(cp))
        return SUTS_SCSS_SCRIPT_NATIVE;
    if ((cp >= 0x0041ULL && cp <= 0x005AULL) ||
        (cp >= 0x0061ULL && cp <= 0x007AULL) ||
        (cp >= 0x00C0ULL && cp <= 0x024FULL) ||
        (cp >= 0x1E00ULL && cp <= 0x1EFFULL))
        return SUTS_SCSS_SCRIPT_LATIN;
    if ((cp >= 0x0370ULL && cp <= 0x03FFULL) ||
        (cp >= 0x1F00ULL && cp <= 0x1FFFULL))
        return SUTS_SCSS_SCRIPT_GREEK;
    if (cp >= 0x0400ULL && cp <= 0x052FULL)
        return SUTS_SCSS_SCRIPT_CYRILLIC;
    if ((cp >= 0x3400ULL && cp <= 0x9FFFULL) ||
        (cp >= 0xF900ULL && cp <= 0xFAFFULL) ||
        (cp >= 0x20000ULL && cp <= 0x3134FULL))
        return SUTS_SCSS_SCRIPT_HAN;
    if ((cp >= 0x0600ULL && cp <= 0x06FFULL) ||
        (cp >= 0x0750ULL && cp <= 0x077FULL) ||
        (cp >= 0xFB50ULL && cp <= 0xFDFFULL) ||
        (cp >= 0xFE70ULL && cp <= 0xFEFFULL))
        return SUTS_SCSS_SCRIPT_ARABIC;
    if (cp >= 0x0590ULL && cp <= 0x05FFULL)
        return SUTS_SCSS_SCRIPT_HEBREW;
    if (cp <= SCSS_ZONE_UNICODE_MAX)
        return SUTS_SCSS_SCRIPT_COMMON;
    return SUTS_SCSS_SCRIPT_UNKNOWN;
}

bool scss_script_compatible(sucs_ex_char_t a, sucs_ex_char_t b)
{
    suts_scss_script_t sa = scss_script_of(a);
    suts_scss_script_t sb = scss_script_of(b);

    if (sa == sb)
        return true;
    /* Script-neutral codepoints combine with any script. */
    if (sa == SUTS_SCSS_SCRIPT_COMMON || sb == SUTS_SCSS_SCRIPT_COMMON)
        return true;
    /* An unclassified (plugin) codepoint is not a boundary violation. */
    if (sa == SUTS_SCSS_SCRIPT_UNKNOWN || sb == SUTS_SCSS_SCRIPT_UNKNOWN)
        return true;
    return false;
}

bool scss_is_mixed_script(sucs_ex_char_t a, sucs_ex_char_t b)
{
    return !scss_script_compatible(a, b);
}

bool scss_crosses_bridge_boundary(sucs_ex_char_t a, sucs_ex_char_t b)
{
    return (scss_is_bridge_codepoint(a) && scss_is_native_codepoint(b)) ||
           (scss_is_bridge_codepoint(b) && scss_is_native_codepoint(a));
}

/* ── Lookup (§4.2) — O(1) expected ────────────────────────────────── */

static const scss_security_attr_t* scss_find_attr(sucs_ex_char_t cp)
{
    uint32_t h;
    uint32_t start;

    if (!scss_ready)
        scss_build(); /* lazy build, so lookup is safe without setup */

    h = scss_hash(cp) & (SUTS_SCSS_INDEX_SIZE - 1u);
    start = h;
    for (;;) {
        const scss_security_attr_t* cand;
        if (!scss_attr_index[h].used)
            return NULL;
        cand = &scss_attributes[scss_attr_index[h].loc];
        if (cand->codepoint == cp)
            return cand; /* stable pointer into read-only memory */
        h = (h + 1u) & (SUTS_SCSS_INDEX_SIZE - 1u);
        if (h == start)
            return NULL;
    }
}

static const scss_skeleton_entry_t* scss_find_skeleton(sucs_ex_char_t cp)
{
    uint32_t h;
    uint32_t start;

    if (!scss_ready)
        scss_build();

    h = scss_hash(cp) & (SUTS_SCSS_INDEX_SIZE - 1u);
    start = h;
    for (;;) {
        const scss_skeleton_entry_t* cand;
        if (!scss_skel_index[h].used)
            return NULL;
        cand = &scss_skeletons[scss_skel_index[h].loc];
        if (cand->codepoint == cp)
            return cand; /* stable pointer into read-only memory */
        h = (h + 1u) & (SUTS_SCSS_INDEX_SIZE - 1u);
        if (h == start)
            return NULL;
    }
}

const scss_security_attr_t* scss_lookup_attr(sucs_ex_char_t codepoint)
{
    return scss_find_attr(codepoint);
}

const scss_skeleton_entry_t* scss_lookup_skeleton(sucs_ex_char_t codepoint)
{
    return scss_find_skeleton(codepoint);
}

/* ── Security attributes (§2.1, §3) ───────────────────────────────── */

uint32_t scss_get_flags(sucs_ex_char_t codepoint)
{
    const scss_security_attr_t* a = scss_find_attr(codepoint);
    uint32_t flags = a ? a->flags : 0u;

    /* Structural default: SCP markers, trap slots, and the Sentinel are
     * never admissible. */
    if (scss_is_control_zone(codepoint))
        flags |= SUTS_SCSS_FLAG_RESTRICTED;

    /* Script-boundary default: a Native & Neography codepoint always
     * requires an isolated script context. */
    if (scss_is_native_codepoint(codepoint))
        flags |= SUTS_SCSS_FLAG_NEOGRAPHY_MIXED;

    /* Identifier default: the ASCII identifier class is allowed unless the
     * curated table overrides it. */
    if (flags == 0u && scss_is_ascii_identifier(codepoint))
        flags |= SUTS_SCSS_FLAG_ALLOWED_IDENTIFIER;

    return flags;
}

sucs_ex_char_t scss_resolve_skeleton(sucs_ex_char_t codepoint)
{
    const scss_skeleton_entry_t* e = scss_find_skeleton(codepoint);

    return e ? e->skeleton_codepoint : codepoint; /* identity if unmapped */
}

bool scss_is_allowed_identifier(sucs_ex_char_t codepoint)
{
    uint32_t f = scss_get_flags(codepoint);

    return (f & SUTS_SCSS_FLAG_ALLOWED_IDENTIFIER) != 0u &&
           (f & (SUTS_SCSS_FLAG_RESTRICTED |
                 SUTS_SCSS_FLAG_CONFUSABLE |
                 SUTS_SCSS_FLAG_NEOGRAPHY_MIXED)) == 0u;
}

bool scss_is_restricted(sucs_ex_char_t codepoint)
{
    return (scss_get_flags(codepoint) & SUTS_SCSS_FLAG_RESTRICTED) != 0u;
}

bool scss_is_confusable(sucs_ex_char_t codepoint)
{
    return (scss_get_flags(codepoint) & SUTS_SCSS_FLAG_CONFUSABLE) != 0u;
}

bool scss_is_neo_mixed(sucs_ex_char_t codepoint)
{
    return (scss_get_flags(codepoint) & SUTS_SCSS_FLAG_NEOGRAPHY_MIXED) != 0u;
}

bool scss_confusable_with(sucs_ex_char_t a, sucs_ex_char_t b)
{
    if (a == b)
        return false;
    return scss_resolve_skeleton(a) == scss_resolve_skeleton(b);
}

/* ── Enumeration (§4.1) ───────────────────────────────────────────── */

size_t scss_attr_count(void)
{
    return SCSS_ATTR_COUNT;
}

const scss_security_attr_t* scss_attr_at(size_t index)
{
    if (index >= SCSS_ATTR_COUNT)
        return NULL;
    return &scss_attributes[index];
}

size_t scss_skeleton_count(void)
{
    return SCSS_SKELETON_COUNT;
}

const scss_skeleton_entry_t* scss_skeleton_at(size_t index)
{
    if (index >= SCSS_SKELETON_COUNT)
        return NULL;
    return &scss_skeletons[index];
}
