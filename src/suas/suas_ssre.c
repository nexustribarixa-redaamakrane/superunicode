/* SUAS-010 — System Specification Registry & Cross-Reference Engine (SSRE)
 *
 * Freestanding C99, zero allocation. The engine is a static array of
 * ssre_spec_record_t records — one per registered specification, compiled at
 * build time from the SUCD manifest — plus a fixed-size open-addressing index
 * over their identifiers. Neither grows at run time.
 *
 * Identifier handling deliberately avoids libc: the registered identifiers are
 * short NUL-terminated ASCII, so hashing and equality are single bounded
 * passes and a freestanding kernel needs no <string.h> at all.
 */

#include "suas/suas_ssre.h"

/* ── Freestanding primitives ──────────────────────────────────────── */

bool suas_ssre_id_equal(const char *a, const char *b)
{
    unsigned i;

    if (a == b)
        return true;
    if (a == NULL || b == NULL)
        return false;
    for (i = 0u; i < SUAS_SSRE_MAX_SPEC_ID; i++) {
        if (a[i] != b[i])
            return false;
        if (a[i] == '\0')
            return true;
    }
    return true; /* both consumed the full budget without divergence */
}

/* ── Hash (§2.3): FNV-1a 32-bit over the identifier bytes ─────────── */

uint32_t suas_ssre_hash(const char *spec_id)
{
    uint32_t h = 2166136261u; /* SUAS_SSRE_FNV_OFFSET_BASIS */
    unsigned i;

    if (spec_id == NULL)
        return h;
    for (i = 0u; i < SUAS_SSRE_MAX_SPEC_ID && spec_id[i] != '\0'; i++) {
        h ^= (uint32_t)(unsigned char)spec_id[i];
        h *= 16777619u;       /* SUAS_SSRE_FNV_PRIME */
    }
    return h;
}

/* ── The curated static registry (§5, §8) ─────────────────────────── */

/* Every specification in the corpus. The registry is the single source of
 * truth for the dependency tree, so it must be complete to be worth
 * consulting: SUAS invariants, SUTS data specifications, and the SUTR
 * architecture reports that all of them cite. It grows only by a new build —
 * per SUAS-010 §11, adding a registration is a patch-level change, because no
 * descriptor that verified before can fail to verify afterwards. */
static const ssre_spec_record_t ssre_registry[] = {
    /* spec_id   short   module        maj min kind   tier */
    { "SUAS-001", "SDF",     "suas_sdf",     1u, 0u, 0u, 0u },
    { "SUAS-002", "SGW",     "suas_sgw",     1u, 0u, 0u, 0u },
    { "SUAS-003", "SBR",     "suas_sbr",     1u, 0u, 0u, 0u },
    { "SUAS-004", "SUCF",    "suas_sucf",    1u, 0u, 0u, 0u },
    { "SUAS-005", "SSP",     "suas_ssp",     1u, 0u, 0u, 0u },
    { "SUAS-006", "STS",     "suas_sts",     1u, 0u, 0u, 0u },
    { "SUAS-007", "SIS",     "suas_sis",     1u, 0u, 0u, 0u },
    { "SUAS-008", "SNCS",    "suas_sncs",    1u, 0u, 0u, 0u },
    { "SUAS-009", "SIP",     "suas_sip",     1u, 0u, 0u, 0u },
    { "SUAS-010", "SSRE",    "suas_ssre",    1u, 0u, 0u, 0u },
    { "SUAS-011", "SCMS",    "suas_scms",    1u, 0u, 0u, 0u },
    { "SUAS-012", "SUCD",    "suas_sucd",    1u, 0u, 0u, 0u },
    { "SUAS-013", "SISR",    "suas_sisr",    1u, 0u, 0u, 0u },
    { "SUTS-001", "SUCA",    "suts_suca",    1u, 0u, 1u, 0u },
    { "SUTS-002", "SRE",     "suts_sre",     1u, 0u, 1u, 0u },
    { "SUTS-003", "SLDF",    "suts_sldf",    1u, 0u, 1u, 0u },
    { "SUTS-004", "SIVD",    "suts_sivd",    1u, 0u, 1u, 0u },
    { "SUTS-005", "SCSS",    "suts_scss",    1u, 0u, 1u, 0u },
    { "SUTR-0",   "SUCS",    "",             0u, 1u, 2u, 0u },
    { "SUTR-1",   "SUTF",    "",             0u, 1u, 2u, 0u },
    { "SUTR-2",   "SUCA",    "",             0u, 1u, 2u, 0u },
    { "SUTR-3",   "STORAGE", "",             0u, 1u, 2u, 0u },
    { "SUTR-4",   "EXTSUCS", "",             0u, 1u, 2u, 0u },
    { "SUTR-5",   "PLUGIN",  "",             0u, 1u, 2u, 0u },
    { "SUTR-6",   "BRIDGE",  "",             0u, 1u, 2u, 0u },
    { "SUTR-7",   "SUCEM",   "",             0u, 1u, 2u, 0u },
    { "SUTR-8",   "SUCPM",   "",             0u, 1u, 2u, 0u },
    { "SUTR-9",   "SUMNLS",  "",             0u, 1u, 2u, 0u },
    { "SUTR-10",  "SUCM",    "",             0u, 1u, 2u, 0u }
};

#define SSRE_REGISTRY_COUNT \
    ((uint32_t)(sizeof ssre_registry / sizeof ssre_registry[0]))

/* A compile-time guarantee that the curated registry still fits the fixed
 * capacity the index was sized against. */
typedef char ssre_registry_fits_capacity[
    (SSRE_REGISTRY_COUNT <= SUAS_SSRE_REGISTRY_CAPACITY) ? 1 : -1];

/* ── The static lookup index (§2.2) ───────────────────────────────── */

typedef struct {
    uint8_t used;  /* slot occupied                                  */
    uint8_t entry; /* index into ssre_registry (capacity < 256)      */
} ssre_slot_t;

static ssre_slot_t ssre_index[SUAS_SSRE_INDEX_SIZE];
static bool        ssre_ready = false;

static void ssre_build(void)
{
    uint32_t i;

    for (i = 0u; i < SUAS_SSRE_INDEX_SIZE; i++) {
        ssre_index[i].used = 0u;
        ssre_index[i].entry = 0u;
    }
    for (i = 0u; i < SSRE_REGISTRY_COUNT; i++) {
        uint32_t h = suas_ssre_hash(ssre_registry[i].spec_id) &
                     (SUAS_SSRE_INDEX_SIZE - 1u);
        while (ssre_index[h].used)
            h = (h + 1u) & (SUAS_SSRE_INDEX_SIZE - 1u);
        ssre_index[h].used = 1u;
        ssre_index[h].entry = (uint8_t)i;
    }
    ssre_ready = true;
}

/* The O(1) lookup: returns the record for a spec_id, or NULL. A NULL or
 * overlong identifier resolves to nothing rather than probing. */
static const ssre_spec_record_t* ssre_find(const char *spec_id)
{
    uint32_t h;
    uint32_t start;
    uint32_t key;
    unsigned len = 0u;

    if (spec_id == NULL)
        return NULL;
    while (len < SUAS_SSRE_MAX_SPEC_ID && spec_id[len] != '\0')
        len++;
    if (len >= SUAS_SSRE_MAX_SPEC_ID)
        return NULL; /* not a registered identifier shape */
    if (!ssre_ready)
        ssre_build(); /* lazy build, so queries are safe without setup */

    key = suas_ssre_hash(spec_id);
    h = key & (SUAS_SSRE_INDEX_SIZE - 1u);
    start = h;
    for (;;) {
        const ssre_spec_record_t* cand;
        if (!ssre_index[h].used)
            return NULL;
        cand = &ssre_registry[ssre_index[h].entry];
        if (suas_ssre_id_equal(cand->spec_id, spec_id))
            return cand;
        h = (h + 1u) & (SUAS_SSRE_INDEX_SIZE - 1u);
        if (h == start)
            return NULL;
    }
}

/* ── Setup ────────────────────────────────────────────────────────── */

#define SSRE_STR2(x) #x
#define SSRE_STR(x)  SSRE_STR2(x)

const char* suas_ssre_version_string(void)
{
    return SUAS_SSRE_SPEC_ID "/"
           SSRE_STR(SUAS_SSRE_VERSION_MAJOR) "."
           SSRE_STR(SUAS_SSRE_VERSION_MINOR) "."
           SSRE_STR(SUAS_SSRE_VERSION_PATCH);
}

void suas_ssre_init_registry(void)
{
    ssre_build();
}

/* ── Dependency validation (§3) ───────────────────────────────────── */

bool suas_ssre_verify_dependency(const ssre_dependency_descriptor_t *dep)
{
    const ssre_spec_record_t* rec;

    if (dep == NULL || dep->spec_id == NULL)
        return false;
    rec = ssre_find(dep->spec_id);
    if (rec == NULL)
        return false; /* the specification is not registered at all */
    /* Version floor, evaluated with the same ordering the compile-time gate
     * uses so both enforcement surfaces agree exactly. */
    if (!suas_ssre_satisfies(dep->spec_id,
                             dep->min_version_major, dep->min_version_minor))
        return false;
    /* An architecturally mandatory dependency must actually be a SUAS
     * invariant. Claiming a SUTS data specification or a SUTR report as
     * mandatory is precisely the invalid linkage SSRE exists to reject. */
    if (dep->is_mandatory && rec->kind != (uint8_t)SUAS_SSRE_KIND_MANDATORY)
        return false;
    return true;
}

bool suas_ssre_verify_set(const ssre_dependency_descriptor_t *deps,
                          size_t count)
{
    size_t i;

    if (count == 0u)
        return true;
    if (deps == NULL)
        return false;
    for (i = 0u; i < count; i++) {
        if (!suas_ssre_verify_dependency(&deps[i]))
            return false;
    }
    return true;
}

const ssre_dependency_descriptor_t*
suas_ssre_first_missing(const ssre_dependency_descriptor_t *deps, size_t count)
{
    size_t i;

    if (count == 0u || deps == NULL)
        return NULL;
    for (i = 0u; i < count; i++) {
        if (!suas_ssre_verify_dependency(&deps[i]))
            return &deps[i];
    }
    return NULL;
}

size_t suas_ssre_dependency_count(const ssre_dependency_descriptor_t *deps,
                                  size_t count)
{
    size_t i;
    size_t bad = 0u;

    if (count == 0u)
        return 0u;
    if (deps == NULL)
        return count; /* every member of an absent set fails to verify */
    for (i = 0u; i < count; i++) {
        if (!suas_ssre_verify_dependency(&deps[i]))
            bad++;
    }
    return bad;
}

/* ── Registry lookup (§2) ─────────────────────────────────────────── */

const ssre_spec_record_t* suas_ssre_lookup(const char *spec_id)
{
    return ssre_find(spec_id);
}

bool suas_ssre_is_registered(const char *spec_id)
{
    return ssre_find(spec_id) != NULL;
}

bool suas_ssre_version_of(const char *spec_id,
                          uint8_t *out_major, uint8_t *out_minor)
{
    const ssre_spec_record_t* rec = ssre_find(spec_id);

    if (rec == NULL)
        return false;
    if (out_major != NULL)
        *out_major = rec->major;
    if (out_minor != NULL)
        *out_minor = rec->minor;
    return true;
}

bool suas_ssre_is_mandatory(const char *spec_id)
{
    const ssre_spec_record_t* rec = ssre_find(spec_id);

    return rec != NULL && rec->kind == (uint8_t)SUAS_SSRE_KIND_MANDATORY;
}

uint8_t suas_ssre_tier_of(const char *spec_id)
{
    const ssre_spec_record_t* rec = ssre_find(spec_id);

    /* Out-of-range on a miss, so a caller that forgets to check reads a tier
     * no module can legitimately claim. */
    return rec != NULL ? rec->tier : (uint8_t)SUAS_SSRE_TIER_LEVELS;
}

const char* suas_ssre_module_of(const char *spec_id)
{
    const ssre_spec_record_t* rec = ssre_find(spec_id);

    return rec != NULL ? rec->module : NULL;
}

const char* suas_ssre_spec_of_module(const char *module)
{
    uint32_t i;

    if (module == NULL || module[0] == '\0')
        return NULL;
    for (i = 0u; i < SSRE_REGISTRY_COUNT; i++) {
        if (suas_ssre_id_equal(ssre_registry[i].module, module))
            return ssre_registry[i].spec_id;
    }
    return NULL;
}

bool suas_ssre_satisfies(const char *spec_id,
                         uint8_t min_major, uint8_t min_minor)
{
    const ssre_spec_record_t* rec = ssre_find(spec_id);

    if (rec == NULL)
        return false;
    return rec->major > min_major ||
           (rec->major == min_major && rec->minor >= min_minor);
}

/* ── Enumeration (§2.4) ───────────────────────────────────────────── */

size_t suas_ssre_spec_count(void)
{
    return (size_t)SSRE_REGISTRY_COUNT;
}

const ssre_spec_record_t* suas_ssre_spec_at(size_t index)
{
    if (index >= (size_t)SSRE_REGISTRY_COUNT)
        return NULL;
    return &ssre_registry[index];
}

size_t suas_ssre_kind_count(uint8_t kind)
{
    uint32_t i;
    size_t   n = 0u;

    for (i = 0u; i < SSRE_REGISTRY_COUNT; i++) {
        if (ssre_registry[i].kind == kind)
            n++;
    }
    return n;
}

const char* suas_ssre_kind_name(uint8_t kind)
{
    switch (kind) {
    case (uint8_t)SUAS_SSRE_KIND_MANDATORY: return "MANDATORY";
    case (uint8_t)SUAS_SSRE_KIND_OPTIONAL:  return "OPTIONAL";
    case (uint8_t)SUAS_SSRE_KIND_REFERENCE: return "REFERENCE";
    default:                                return "UNKNOWN";
    }
}
