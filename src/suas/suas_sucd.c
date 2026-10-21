/* SUAS-012 — SuperUnicode Character Database (SUCD)
 *
 * Freestanding C99, zero allocation. The engine is a set of bounded decoders
 * over a caller-mapped, read-only binary image plus four static pointers into
 * it. Nothing is copied out of the image, nothing is retained beyond a call,
 * and no query may exceed the page directory plus one index load.
 *
 * The image is deliberately a *mapping target* rather than a data structure the
 * engine owns. A kernel maps SUCD at boot, passes the base and the length, and
 * the engine verifies the layout before it will read a single field of it —
 * because the whole value of an O(1) lookup is that the caller may trust it
 * without re-deriving anything, and a verifier that trusts the header it is
 * about to use has already lost that argument.
 */

#include "suas/suas_sucd.h"
#include "suas/suas_ssp.h"   /* SUAS_SCRIPT_UNKNOWN for the default record   */
#include "suas/suas_ssre.h"  /* the §7 schema-linkage gate                  */

/* ── Geometry self-checks ────────────────────────────────────────── */

typedef char sucd_assert_directory[
    (SUAS_SUCD_DIRECTORY_SIZE ==
     ((SUAS_SUCD_MAX_CODEPOINT >> SUAS_SUCD_PAGE_SHIFT) + 1u)) ? 1 : -1];
typedef char sucd_assert_page[
    (SUAS_SUCD_PAGE_SIZE == 65536u) ? 1 : -1];
typedef char sucd_assert_capacity[
    (SUAS_SUCD_RECORD_CAPACITY <= (uint32_t)SUAS_SUCD_NO_RECORD) ? 1 : -1];
typedef char sucd_assert_record[
    (sizeof(sucd_record_t) == 16u) ? 1 : -1];
typedef char sucd_assert_header[
    (sizeof(sucd_header_t) == 24u) ? 1 : -1];
typedef char sucd_assert_planes[
    (SUCD_PLANE_MASK_ALL == 0x3Fu) ? 1 : -1];
typedef char sucd_assert_properties[
    (SUCD_PROP_ASSIGNED_MASK == 0x0001FFFFu) ? 1 : -1];

/* ── Engine state ────────────────────────────────────────────────── */

/* Four pointers into the bound image, plus its extent. Every one of them is
 * either NULL or points at a region the verifier has already proven is inside
 * the image and internally consistent, which is what lets the query path do
 * exactly two indexed reads and nothing defensive at all. */
static const sucd_header_t   *sucd_hdr    = NULL;
static const uint32_t        *sucd_dir    = NULL;
static const uint16_t        *sucd_dense  = NULL;
static const sucd_record_t   *sucd_recs   = NULL;
static uint32_t               sucd_nrecs  = 0u;
static size_t                 sucd_dense_units = 0u;
static bool                   sucd_ready  = false;

void suas_sucd_init_engine(void)
{
    /* The geometry's consistency is already enforced at compile time by the
     * assertions above; arming is the run-time acknowledgement that a tool is
     * about to consume the geometry, and is idempotent. */
    sucd_ready = true;
}

void suas_sucd_shutdown(void)
{
    sucd_hdr   = NULL;
    sucd_dir   = NULL;
    sucd_dense = NULL;
    sucd_recs  = NULL;
    sucd_nrecs = 0u;
    sucd_dense_units = 0u;
    sucd_ready = false;
}

bool suas_sucd_is_loaded(void)
{
    return sucd_hdr != NULL && sucd_ready;
}

#define SUAS_SUCD_STR2(x) #x
#define SUAS_SUCD_STR(x)  SUAS_SUCD_STR2(x)

const char* suas_sucd_version_string(void)
{
    return SUAS_SUCD_SPEC_ID "/"
           SUAS_SUCD_STR(SUAS_SUCD_VERSION_MAJOR) "."
           SUAS_SUCD_STR(SUAS_SUCD_VERSION_MINOR) "."
           SUAS_SUCD_STR(SUAS_SUCD_VERSION_PATCH);
}

const sucd_header_t* suas_sucd_header(void)
{
    return sucd_hdr;
}

/* ── Taxonomy tables (§4.3, §4.5) ────────────────────────────────── */

static const char *const sucd_gc_names[SUCD_GC_COUNT] = {
    "Cn", "Lu", "Ll", "Lt", "Lm", "Lo", "Mn", "Mc", "Me", "Nd",
    "Nl", "No", "Pc", "Pd", "Ps", "Pe", "Pi", "Pf", "Po", "Sm",
    "Sc", "Sk", "So", "Zs", "Zl", "Zp", "Cc", "Cf", "Cs", "Co"
};

const char* suas_sucd_gc_name(sucd_general_category_t gc)
{
    if ((unsigned)gc >= (unsigned)SUCD_GC_COUNT)
        return "??";
    return sucd_gc_names[(unsigned)gc];
}

/* Bounded, exact, case-sensitive: an alias is a name, and a database that
 * accepts "lu" or "LU " is a database whose lookups cannot be compared
 * between two implementations. */
static bool sucd_name_eq(const char *name, size_t length, const char *lit)
{
    size_t i;

    for (i = 0u; i < length; i++) {
        if (lit[i] == '\0' || name[i] != lit[i])
            return false;
    }
    return lit[length] == '\0';
}

sucd_general_category_t suas_sucd_gc_by_name(const char *name, size_t length)
{
    unsigned i;

    if (name == NULL || length == 0u)
        return SUCD_GC_CN;
    for (i = 0u; i < (unsigned)SUCD_GC_COUNT; i++) {
        if (sucd_name_eq(name, length, sucd_gc_names[i]))
            return (sucd_general_category_t)i;
    }
    return SUCD_GC_CN;
}

typedef struct {
    const char *name;
    uint32_t    bit;
} sucd_prop_entry_t;

/* The published spellings, in mask order. A compiler matching PropList.txt
 * matches "Unicode_Bridged", and an API returning it says "Unicode_Bridged";
 * neither side invents a third form. */
static const sucd_prop_entry_t sucd_props[] = {
    { "Unicode_Bridged",  SUCD_PROP_UNICODE_BRIDGED  },
    { "Control_Plane",    SUCD_PROP_CONTROL_PLANE    },
    { "Printable",        SUCD_PROP_PRINTABLE        },
    { "Diagnostic_BAN",   SUCD_PROP_DIAGNOSTIC_BAN   },
    { "Diagnostic_WARN",  SUCD_PROP_DIAGNOSTIC_WARN  },
    { "Diagnostic_COM",   SUCD_PROP_DIAGNOSTIC_COM   },
    { "Diagnostic_SOFT",  SUCD_PROP_DIAGNOSTIC_SOFT  },
    { "Trap_Instruction", SUCD_PROP_TRAP_INSTRUCTION },
    { "Sentinel_Value",   SUCD_PROP_SENTINEL_VALUE   },
    { "Alphabetic",       SUCD_PROP_ALPHABETIC       },
    { "Control",          SUCD_PROP_CONTROL          },
    { "LTR",              SUCD_PROP_BIDI_LTR         },
    { "RTL",              SUCD_PROP_BIDI_RTL         },
    { "ARABIC_AL",        SUCD_PROP_BIDI_ARABIC_AL   },
    { "NEUTRAL",          SUCD_PROP_BIDI_NEUTRAL     },
    { "MIRRORED",         SUCD_PROP_BIDI_MIRRORED    },
    { "WHITESPACE",       SUCD_PROP_BIDI_WHITESPACE  }
};

#define SUCD_PROP_COUNT \
    ((uint32_t)(sizeof sucd_props / sizeof sucd_props[0]))

uint32_t suas_sucd_property_by_name(const char *name, size_t length)
{
    uint32_t i;

    if (name == NULL || length == 0u)
        return 0u;
    for (i = 0u; i < SUCD_PROP_COUNT; i++) {
        if (sucd_name_eq(name, length, sucd_props[i].name))
            return sucd_props[i].bit;
    }
    return 0u;
}

const char* suas_sucd_property_name(uint32_t property_bit)
{
    uint32_t i;

    if (property_bit == 0u || (property_bit & (property_bit - 1u)) != 0u)
        return NULL; /* 0, or more than one bit set */
    for (i = 0u; i < SUCD_PROP_COUNT; i++) {
        if (sucd_props[i].bit == property_bit)
            return sucd_props[i].name;
    }
    return NULL;
}

/* ── Plane resolution (§4.2) ─────────────────────────────────────── */
/* Most specific first: the Sentinel is a single codepoint sitting at the top
 * of the trap range's neighbourhood, the trap range sits at the top of the
 * Native plane, and RNUR is a carve-out inside Native. Resolving in the other
 * order would answer every one of those with SUCD_PLANE_NATIVE, which is true
 * and useless. */
uint8_t suas_sucd_plane_of(uint32_t codepoint)
{
    if (codepoint > SUAS_SUCD_MAX_CODEPOINT)
        return 0u;
    if (codepoint == SUAS_SUCD_MAX_CODEPOINT)
        return SUCD_PLANE_SENTINEL;
    if (codepoint >= 0x7FFFFFF0u)
        return SUCD_PLANE_TRAP;
    if (codepoint >= 0x00160000u && codepoint <= 0x0016FFFFu)
        return SUCD_PLANE_RNUR;
    if (codepoint <= 0x0010FFFFu)
        return SUCD_PLANE_BRIDGE;
    if (codepoint <= 0x0011FFFFu)
        return SUCD_PLANE_SCP;
    return SUCD_PLANE_NATIVE;
}

const char* suas_sucd_plane_name(uint8_t plane_bit)
{
    switch (plane_bit) {
    case SUCD_PLANE_BRIDGE:   return "SUCD_PLANE_BRIDGE";
    case SUCD_PLANE_SCP:      return "SUCD_PLANE_SCP";
    case SUCD_PLANE_NATIVE:   return "SUCD_PLANE_NATIVE";
    case SUCD_PLANE_RNUR:     return "SUCD_PLANE_RNUR";
    case SUCD_PLANE_TRAP:     return "SUCD_PLANE_TRAP";
    case SUCD_PLANE_SENTINEL: return "SUCD_PLANE_SENTINEL";
    default:                  return NULL;
    }
}

const char* suas_sucd_image_error_name(sucd_image_error_t error)
{
    switch (error) {
    case SUCD_IMAGE_OK:                     return "SUCD_IMAGE_OK";
    case SUCD_IMAGE_NULL_IMAGE:             return "SUCD_IMAGE_NULL_IMAGE";
    case SUCD_IMAGE_TRUNCATED:              return "SUCD_IMAGE_TRUNCATED";
    case SUCD_IMAGE_BAD_MAGIC:              return "SUCD_IMAGE_BAD_MAGIC";
    case SUCD_IMAGE_BAD_DIRECTORY_COUNT:    return "SUCD_IMAGE_BAD_DIRECTORY_COUNT";
    case SUCD_IMAGE_RECORD_COUNT_OVER:      return "SUCD_IMAGE_RECORD_COUNT_OVER";
    case SUCD_IMAGE_SIZE_MISMATCH:          return "SUCD_IMAGE_SIZE_MISMATCH";
    case SUCD_IMAGE_BAD_RECORD_FLAG:        return "SUCD_IMAGE_BAD_RECORD_FLAG";
    case SUCD_IMAGE_RANGE_ORDER:            return "SUCD_IMAGE_RANGE_ORDER";
    case SUCD_IMAGE_OUT_OF_SPACE:           return "SUCD_IMAGE_OUT_OF_SPACE";
    case SUCD_IMAGE_GC_OUT_OF_RANGE:        return "SUCD_IMAGE_GC_OUT_OF_RANGE";
    case SUCD_IMAGE_RESERVED_PROPERTY_BIT:  return "SUCD_IMAGE_RESERVED_PROPERTY_BIT";
    case SUCD_IMAGE_UNORDERED_RECORDS:      return "SUCD_IMAGE_UNORDERED_RECORDS";
    case SUCD_IMAGE_OVERLAPPING_RECORDS:    return "SUCD_IMAGE_OVERLAPPING_RECORDS";
    case SUCD_IMAGE_DIRECTORY_OUT_OF_RANGE: return "SUCD_IMAGE_DIRECTORY_OUT_OF_RANGE";
    case SUCD_IMAGE_DENSE_OFFSET_OVER:      return "SUCD_IMAGE_DENSE_OFFSET_OVER";
    case SUCD_IMAGE_PAGE_NOT_COVERED:       return "SUCD_IMAGE_PAGE_NOT_COVERED";
    default:                                return "SUCD_IMAGE_UNKNOWN";
    }
}

/* ── Record predicates (§4.4) ────────────────────────────────────── */

bool suas_sucd_records_overlap(const sucd_record_t *a, const sucd_record_t *b)
{
    if (a == NULL || b == NULL)
        return false;
    if (a->first > a->last || b->first > b->last)
        return false; /* a malformed record covers nothing */
    return a->first <= b->last && b->first <= a->last;
}

bool suas_sucd_is_partition(const sucd_record_t *records, uint32_t count)
{
    uint32_t i;

    if (records == NULL && count != 0u)
        return false;
    for (i = 0u; i < count; i++) {
        if (records[i].first > records[i].last)
            return false;
        if (records[i].last > SUAS_SUCD_MAX_CODEPOINT)
            return false;
        if (i > 0u && records[i].first <= records[i - 1u].last)
            return false;
    }
    return true;
}

/* ── Image verification (§5.3) ───────────────────────────────────── */

static void sucd_fail(sucd_image_error_t *out, sucd_image_error_t why)
{
    if (out != NULL)
        *out = why;
}

/* Places the three regions the query path reads. Returns the dense extent in
 * index units, or 0 when the header's own arithmetic cannot be satisfied. */
static bool sucd_layout(const uint8_t *base, size_t size,
                        const sucd_header_t **out_hdr,
                        const uint32_t **out_dir,
                        const uint16_t **out_dense,
                        const sucd_record_t **out_recs,
                        uint32_t *out_nrecs, size_t *out_dense_units)
{
    const sucd_header_t *h;
    size_t need, dense_units;
    uint32_t n;

    h = (const sucd_header_t *)(const void *)base;
    n = h->total_records;

    /* header + directory, before anything else may be read. */
    need = sizeof(sucd_header_t) + (size_t)SUAS_SUCD_DIRECTORY_BYTES;
    if (size < need)
        return false;

    /* Everything past the directory splits into a dense index of an even
     * number of bytes and a trailing record array. The record count is the
     * header's; the dense extent is therefore whatever remains, and requiring
     * it to divide exactly is what stops a truncated image from passing. */
    if ((size - need) < (size_t)n * sizeof(sucd_record_t))
        return false;
    /* A single trailing byte is not a dense entry, so it is not a layout. */
    if (((size - need) - (size_t)n * sizeof(sucd_record_t))
        % sizeof(uint16_t) != 0u)
        return false;
    dense_units = (size - need - (size_t)n * sizeof(sucd_record_t))
                  / sizeof(uint16_t);
    if (dense_units > (size_t)0x7FFFFFFF)
        return false; /* a slot cannot address it */

    *out_hdr = h;
    *out_dir = (const uint32_t *)(const void *)(base + sizeof(sucd_header_t));
    *out_dense = (const uint16_t *)(const void *)(base + need);
    *out_recs = (const sucd_record_t *)(const void *)(base + need
                 + dense_units * sizeof(uint16_t));
    *out_nrecs = n;
    *out_dense_units = dense_units;
    return true;
}

/* Validates the record array, returning the specific defect. Records must be
 * individually well-formed, and — because the image's whole promise is that a
 * page resolves without a search — ordered by `first` and pairwise disjoint. */
static sucd_image_error_t sucd_verify_records(const sucd_record_t *recs,
                                              uint32_t n)
{
    uint32_t i;

    for (i = 0u; i < n; i++) {
        uint32_t f = recs[i].first;
        uint32_t l = recs[i].last;
        uint8_t  fl = recs[i].flags;

        if (f > l)
            return SUCD_IMAGE_RANGE_ORDER;
        if (l > SUAS_SUCD_MAX_CODEPOINT)
            return SUCD_IMAGE_OUT_OF_SPACE;
        if (recs[i].general_category >= (uint16_t)SUCD_GC_COUNT)
            return SUCD_IMAGE_GC_OUT_OF_RANGE;
        if ((recs[i].property_mask & ~SUCD_PROP_ASSIGNED_MASK) != 0u)
            return SUCD_IMAGE_RESERVED_PROPERTY_BIT;
        /* Exactly one cursor contract, and no bits outside it. */
        if (fl != SUCD_RECORD_CONTROL && fl != SUCD_RECORD_PRINTABLE)
            return SUCD_IMAGE_BAD_RECORD_FLAG;
        if (i > 0u) {
            if (f <= recs[i - 1u].first)
                return SUCD_IMAGE_UNORDERED_RECORDS;
            if (f <= recs[i - 1u].last)
                return SUCD_IMAGE_OVERLAPPING_RECORDS;
        }
    }
    return SUCD_IMAGE_OK;
}

bool suas_sucd_verify_image(const void *binary_image, size_t image_size,
                            sucd_image_error_t *out_reason)
{
    const uint8_t *base = (const uint8_t *)binary_image;
    const sucd_header_t *h;
    const uint32_t *dir;
    const uint16_t *dense;
    const sucd_record_t *recs;
    uint32_t n, p;
    size_t dense_units;
    sucd_image_error_t why;

    sucd_fail(out_reason, SUCD_IMAGE_OK);

    if (base == NULL) {
        sucd_fail(out_reason, SUCD_IMAGE_NULL_IMAGE);
        return false;
    }
    if (image_size < sizeof(sucd_header_t)) {
        sucd_fail(out_reason, SUCD_IMAGE_TRUNCATED);
        return false;
    }
    h = (const sucd_header_t *)(const void *)base;
    if (h->magic != SUAS_SUCD_MAGIC) {
        sucd_fail(out_reason, SUCD_IMAGE_BAD_MAGIC);
        return false;
    }
    if (h->directory_count != SUAS_SUCD_DIRECTORY_SIZE) {
        sucd_fail(out_reason, SUCD_IMAGE_BAD_DIRECTORY_COUNT);
        return false;
    }
    if (h->total_records > SUAS_SUCD_RECORD_CAPACITY) {
        sucd_fail(out_reason, SUCD_IMAGE_RECORD_COUNT_OVER);
        return false;
    }
    if (!sucd_layout(base, image_size, &h, &dir, &dense, &recs, &n,
                     &dense_units)) {
        sucd_fail(out_reason, SUCD_IMAGE_SIZE_MISMATCH);
        return false;
    }
    why = sucd_verify_records(recs, n);
    if (why != SUCD_IMAGE_OK) {
        sucd_fail(out_reason, why);
        return false;
    }

    /* Every slot must name something the image actually contains, and a dense
     * slot must not run off the end of its own index. */
    for (p = 0u; p < SUAS_SUCD_DIRECTORY_SIZE; p++) {
        uint32_t slot = dir[p];

        if (slot == SUAS_SUCD_NO_PAGE)
            continue;
        if ((slot & SUAS_SUCD_UNIFORM_TAG) != 0u) {
            uint32_t idx = SUAS_SUCD_UNIFORM_INDEX(slot);
            uint32_t lo = p << SUAS_SUCD_PAGE_SHIFT;
            uint32_t hi = lo + SUAS_SUCD_PAGE_MASK;

            if (idx >= n) {
                sucd_fail(out_reason, SUCD_IMAGE_DIRECTORY_OUT_OF_RANGE);
                return false;
            }
            if (recs[idx].first > lo || recs[idx].last < hi) {
                sucd_fail(out_reason, SUCD_IMAGE_PAGE_NOT_COVERED);
                return false;
            }
        } else {
            if ((size_t)slot + SUAS_SUCD_PAGE_SIZE > dense_units) {
                sucd_fail(out_reason, SUCD_IMAGE_DENSE_OFFSET_OVER);
                return false;
            }
        }
    }

    /* Dense entries must name a record that actually covers the codepoint they
     * stand for; a stale index is the one defect a bounds compare at query time
     * would silently turn into a wrong answer rather than a miss. */
    for (p = 0u; p < SUAS_SUCD_DIRECTORY_SIZE; p++) {
        uint32_t slot = dir[p];
        uint32_t off, k;

        if (slot == SUAS_SUCD_NO_PAGE ||
            (slot & SUAS_SUCD_UNIFORM_TAG) != 0u)
            continue;
        off = slot;
        for (k = 0u; k < SUAS_SUCD_PAGE_SIZE; k++) {
            uint16_t idx = dense[off + k];
            uint32_t cp;

            if (idx == SUAS_SUCD_NO_RECORD)
                continue;
            if ((uint32_t)idx >= n) {
                sucd_fail(out_reason, SUCD_IMAGE_DIRECTORY_OUT_OF_RANGE);
                return false;
            }
            cp = (p << SUAS_SUCD_PAGE_SHIFT) | k;
            if (cp < recs[idx].first || cp > recs[idx].last) {
                sucd_fail(out_reason, SUCD_IMAGE_PAGE_NOT_COVERED);
                return false;
            }
        }
    }
    return true;
}

bool suas_sucd_init(const void *binary_image, size_t image_size)
{
    const uint8_t *base = (const uint8_t *)binary_image;
    const sucd_header_t *h;
    const uint32_t *dir;
    const uint16_t *dense;
    const sucd_record_t *recs;
    uint32_t n;
    size_t dense_units;

    if (sucd_hdr != NULL)
        return true; /* already bound; binding is idempotent */

    if (!suas_sucd_verify_image(binary_image, image_size, NULL))
        return false;
    if (!sucd_layout(base, image_size, &h, &dir, &dense, &recs, &n,
                     &dense_units))
        return false;

    sucd_hdr = h;
    sucd_dir = dir;
    sucd_dense = dense;
    sucd_recs = recs;
    sucd_nrecs = n;
    sucd_dense_units = dense_units;
    sucd_ready = true;
    return true;
}

/* ── Queries (§5.2) ──────────────────────────────────────────────── */

bool suas_sucd_default_record(sucd_record_t *out_record)
{
    if (out_record == NULL)
        return false;
    out_record->first = 0u;
    out_record->last = 0u;
    out_record->property_mask = 0u;
    out_record->general_category = (uint16_t)SUCD_GC_CN;
    out_record->script_id = SUAS_SCRIPT_UNKNOWN;
    out_record->flags = SUCD_RECORD_PRINTABLE;
    return true;
}

bool suas_sucd_get_record(uint32_t codepoint, sucd_record_t *out_record)
{
    uint32_t page, slot, idx;
    const sucd_record_t *r;

    if (out_record == NULL)
        return false;
    if (sucd_hdr == NULL || codepoint > SUAS_SUCD_MAX_CODEPOINT)
        return suas_sucd_default_record(out_record);

    page = codepoint >> SUAS_SUCD_PAGE_SHIFT;
    slot = sucd_dir[page];

    if (slot == SUAS_SUCD_NO_PAGE)
        return suas_sucd_default_record(out_record);

    if ((slot & SUAS_SUCD_UNIFORM_TAG) != 0u) {
        idx = SUAS_SUCD_UNIFORM_INDEX(slot);
    } else {
        uint32_t off = slot + (codepoint & SUAS_SUCD_PAGE_MASK);
        uint16_t leaf;

        if ((size_t)off >= sucd_dense_units)
            return suas_sucd_default_record(out_record);
        leaf = sucd_dense[off];
        if (leaf == SUAS_SUCD_NO_RECORD)
            return suas_sucd_default_record(out_record);
        idx = leaf;
    }

    if (idx >= sucd_nrecs)
        return suas_sucd_default_record(out_record);
    r = &sucd_recs[idx];
    if (codepoint < r->first || codepoint > r->last)
        return suas_sucd_default_record(out_record);

    *out_record = *r;
    return true;
}

uint32_t suas_sucd_property_mask(uint32_t codepoint)
{
    sucd_record_t rec;

    if (!suas_sucd_get_record(codepoint, &rec))
        return 0u;
    return rec.property_mask;
}

/* ── Data-file decoding (§6.1) ───────────────────────────────────── */

static bool sucd_is_hex(char c)
{
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
           (c >= 'A' && c <= 'F');
}

static int sucd_hex(char c)
{
    if (c >= '0' && c <= '9') return (int)(c - '0');
    if (c >= 'a' && c <= 'f') return (int)(c - 'a') + 10;
    return (int)(c - 'A') + 10;
}

/* Runs of 1..8 hex digits, so that the four-, six- and eight-digit files all
 * decode through one path. */
static bool sucd_scan_hex(const char *s, size_t len, size_t *io,
                          uint32_t *out)
{
    uint64_t v = 0u;
    size_t i = *io, digits = 0u;

    while (i < len && sucd_is_hex(s[i]) && digits < 8u) {
        v = (v << 4) | (uint64_t)sucd_hex(s[i]);
        i++;
        digits++;
    }
    if (digits == 0u || v > (uint64_t)SUAS_SUCD_MAX_CODEPOINT)
        return false;
    /* A ninth hex digit means the value does not fit the space, not that the
     * parser should quietly take the low bits of it. */
    if (i < len && sucd_is_hex(s[i]))
        return false;
    *io = i;
    *out = (uint32_t)v;
    return true;
}

static size_t sucd_skip_space(const char *s, size_t len, size_t i)
{
    while (i < len && (s[i] == ' ' || s[i] == '\t'))
        i++;
    return i;
}

sucd_range_form_t suas_sucd_range_form(const char *line, size_t length)
{
    size_t i;
    uint32_t a, b;

    if (line == NULL || length == 0u)
        return SUCD_RANGE_FORM_NONE;
    i = sucd_skip_space(line, length, 0u);
    if (i >= length || line[i] == '#')
        return SUCD_RANGE_FORM_NONE;
    if (!sucd_is_hex(line[i]))
        return SUCD_RANGE_FORM_NONE;

    a = 0u;
    if (!sucd_scan_hex(line, length, &i, &a))
        return SUCD_RANGE_FORM_NONE;

    if (i + 1u < length && line[i] == '.' && line[i + 1u] == '.') {
        i += 2u;
        i = sucd_skip_space(line, length, i);
        if (i >= length || !sucd_is_hex(line[i]))
            return SUCD_RANGE_FORM_NONE;
        b = 0u;
        if (!sucd_scan_hex(line, length, &i, &b))
            return SUCD_RANGE_FORM_NONE;
        if (a > b)
            return SUCD_RANGE_FORM_NONE;
        return SUCD_RANGE_FORM_UCD;
    }
    if (i < length && line[i] == '-') {
        i++;
        i = sucd_skip_space(line, length, i);
        if (i >= length || !sucd_is_hex(line[i]))
            return SUCD_RANGE_FORM_NONE;
        b = 0u;
        if (!sucd_scan_hex(line, length, &i, &b))
            return SUCD_RANGE_FORM_NONE;
        if (a > b)
            return SUCD_RANGE_FORM_NONE;
        return SUCD_RANGE_FORM_HYPHEN;
    }
    return SUCD_RANGE_FORM_POINT;
}

bool suas_sucd_parse_range(const char *line, size_t length,
                           uint32_t *out_first, uint32_t *out_last)
{
    size_t i;
    uint32_t a, b;

    if (line == NULL || length == 0u)
        return false;
    i = sucd_skip_space(line, length, 0u);
    if (i >= length || !sucd_is_hex(line[i]))
        return false;

    a = 0u;
    if (!sucd_scan_hex(line, length, &i, &a))
        return false;

    if (i + 1u < length && line[i] == '.' && line[i + 1u] == '.') {
        i += 2u;
        i = sucd_skip_space(line, length, i);
        if (i >= length || !sucd_is_hex(line[i]))
            return false;
        b = 0u;
        if (!sucd_scan_hex(line, length, &i, &b))
            return false;
        if (a > b)
            return false;
        if (out_first != NULL) *out_first = a;
        if (out_last  != NULL) *out_last  = b;
        return true;
    }
    if (i < length && line[i] == '-') {
        i++;
        i = sucd_skip_space(line, length, i);
        if (i >= length || !sucd_is_hex(line[i]))
            return false;
        b = 0u;
        if (!sucd_scan_hex(line, length, &i, &b))
            return false;
        if (a > b)
            return false;
        if (out_first != NULL) *out_first = a;
        if (out_last  != NULL) *out_last  = b;
        return true;
    }
    /* A lone codepoint, as BidiProps.txt writes its singletons. */
    if (out_first != NULL) *out_first = a;
    if (out_last  != NULL) *out_last  = a;
    return true;
}

/* ── SUAS-010 (SSRE) linkage gate (§7) ───────────────────────────── */

bool suas_sucd_verify_schema_link(const sucd_schema_link_t *link)
{
    const ssre_spec_record_t *rec;

    if (link == NULL || link->schema_spec_id == NULL)
        return false;
    rec = suas_ssre_lookup(link->schema_spec_id);
    if (rec == NULL)
        return false;
    if (link->schema_mandatory && rec->kind != SUAS_SSRE_KIND_MANDATORY)
        return false;
    if (rec->major < link->schema_major)
        return false;
    if (rec->major == link->schema_major && rec->minor < link->schema_minor)
        return false;
    return true;
}
