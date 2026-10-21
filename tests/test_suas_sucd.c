/**
 * SUAS-012 SUCD — SuperUnicode Character Database
 *
 * Unit tests for the canonical architecture and zero-allocation binary memory
 * representation of the character database itself: the UAX #44 analog.
 *
 * Covers: version/meta, the geometry's static invariants, the property and
 * general-category taxonomies, plane resolution (including the RNUR carve-out
 * and the Sentinel), every image-verification outcome, all three directory
 * slot kinds, O(1) record lookup through a uniform page, a dense page and a
 * dense-page gap, the default record, the two data-file range syntaxes the
 * published tree uses, the partition and overlap predicates, and the SUAS-010
 * (SSRE) linkage gate.
 */

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>

#include "suas/suas_sucd.h"
#include "suas/suas_ssp.h"
#include "suas/suas_ssre.h"

static int g_fail = 0;
#define CHECK(cond) do { if (!(cond)) { \
    printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); g_fail = 1; \
} } while (0)

/* ── Test image geometry ─────────────────────────────────────────── */
/* Two dense pages (0 and 32767), every other populated page uniform. That is
 * exactly the shape a compiler emits for the published tree: the ASCII page
 * carries a dozen distinct classifications, the Native plane carries one, and
 * the top page carries the traps and the Sentinel. */

#define T_PAGE_SIZE    65536u
#define T_PAGE_MASK    0xFFFFu
#define T_DIR_SIZE     32768u
#define T_DENSE_PAGES  2u
#define T_NREC         11u

#define T_DIR_OFF      (sizeof(sucd_header_t))
#define T_DENSE_OFF    (T_DIR_OFF + T_DIR_SIZE * 4u)
#define T_RECS_OFF     (T_DENSE_OFF + T_DENSE_PAGES * T_PAGE_SIZE * 2u)
#define T_IMAGE_SIZE   (T_RECS_OFF + T_NREC * sizeof(sucd_record_t))

/* 8-byte aligned so the image is mapped the way a kernel would map it. */
static union {
    uint64_t align;
    uint8_t  bytes[T_IMAGE_SIZE];
} g_img;

/* A second, dense-free image: one uniform page and 32767 unassigned ones. */
#define T_MIN_NREC   1u
#define T_MIN_SIZE   (sizeof(sucd_header_t) + T_DIR_SIZE * 4u \
                      + T_MIN_NREC * sizeof(sucd_record_t))
static union {
    uint64_t align;
    uint8_t  bytes[T_MIN_SIZE];
} g_min;

/* A mutable copy the corruption tests scribble on. */
static union {
    uint64_t align;
    uint8_t  bytes[T_IMAGE_SIZE];
} g_scratch;

/* ── Image construction ──────────────────────────────────────────── */

static sucd_header_t *hdr_of(uint8_t *b)   { return (sucd_header_t *)(void *)b; }
static uint32_t *dir_of(uint8_t *b)        { return (uint32_t *)(void *)(b + T_DIR_OFF); }
static uint16_t *dense_of(uint8_t *b)      { return (uint16_t *)(void *)(b + T_DENSE_OFF); }
static sucd_record_t *recs_of(uint8_t *b)  { return (sucd_record_t *)(void *)(b + T_RECS_OFF); }

static sucd_record_t mkrec(uint32_t first, uint32_t last, uint32_t props,
                           uint16_t gc, uint8_t flags)
{
    sucd_record_t r;

    r.first = first;
    r.last = last;
    r.property_mask = props;
    r.general_category = gc;
    r.script_id = SUAS_SCRIPT_UNKNOWN;
    r.flags = flags;
    return r;
}

static void build_image(uint8_t *b)
{
    sucd_record_t *r = recs_of(b);
    uint16_t *d = dense_of(b);
    uint32_t *dir = dir_of(b);
    sucd_header_t *h = hdr_of(b);
    uint32_t i;

    memset(b, 0, T_IMAGE_SIZE);

    h->magic = SUAS_SUCD_MAGIC;
    h->version = ((uint32_t)SUAS_SUCD_VERSION_MAJOR << 16)
               | ((uint32_t)SUAS_SUCD_VERSION_MINOR << 8);
    h->total_records = T_NREC;
    h->plane_mask = SUCD_PLANE_BRIDGE | SUCD_PLANE_SCP | SUCD_PLANE_NATIVE
                  | SUCD_PLANE_TRAP | SUCD_PLANE_SENTINEL;
    h->directory_count = T_DIR_SIZE;
    h->flags = SUCD_IMAGE_PARTITION;

    /* An ASCII page with a genuine unassigned gap at 0x0009..0x000F, so the
     * dense "no record here" path is reachable without inventing a page. */
    r[0]  = mkrec(0x00000000u, 0x00000008u,
                  SUCD_PROP_UNICODE_BRIDGED | SUCD_PROP_CONTROL,
                  (uint16_t)SUCD_GC_CC, SUCD_RECORD_CONTROL);
    r[1]  = mkrec(0x00000010u, 0x0000001Fu,
                  SUCD_PROP_UNICODE_BRIDGED | SUCD_PROP_CONTROL,
                  (uint16_t)SUCD_GC_CC, SUCD_RECORD_CONTROL);
    r[2]  = mkrec(0x00000020u, 0x00000020u,
                  SUCD_PROP_UNICODE_BRIDGED | SUCD_PROP_BIDI_WHITESPACE,
                  (uint16_t)SUCD_GC_ZS, SUCD_RECORD_PRINTABLE);
    r[3]  = mkrec(0x00000021u, 0x00000027u,
                  SUCD_PROP_UNICODE_BRIDGED | SUCD_PROP_BIDI_LTR,
                  (uint16_t)SUCD_GC_PO, SUCD_RECORD_PRINTABLE);
    r[4]  = mkrec(0x00000028u, 0x00000029u,
                  SUCD_PROP_UNICODE_BRIDGED | SUCD_PROP_BIDI_MIRRORED,
                  (uint16_t)SUCD_GC_PO, SUCD_RECORD_PRINTABLE);
    r[5]  = mkrec(0x0000002Au, 0x0000FFFFu,
                  SUCD_PROP_UNICODE_BRIDGED | SUCD_PROP_ALPHABETIC
                      | SUCD_PROP_BIDI_LTR,
                  (uint16_t)SUCD_GC_LO, SUCD_RECORD_PRINTABLE);
    r[6]  = mkrec(0x00010000u, 0x0010FFFFu,
                  SUCD_PROP_UNICODE_BRIDGED | SUCD_PROP_ALPHABETIC,
                  (uint16_t)SUCD_GC_LO, SUCD_RECORD_PRINTABLE);
    r[7]  = mkrec(0x00110000u, 0x0011FFFFu,
                  SUCD_PROP_CONTROL_PLANE | SUCD_PROP_CONTROL,
                  (uint16_t)SUCD_GC_CC, SUCD_RECORD_CONTROL);
    r[8]  = mkrec(0x00120000u, 0x7FFFFFEFu,
                  SUCD_PROP_PRINTABLE | SUCD_PROP_ALPHABETIC,
                  (uint16_t)SUCD_GC_LO, SUCD_RECORD_PRINTABLE);
    r[9]  = mkrec(0x7FFFFFF0u, 0x7FFFFFFEu,
                  SUCD_PROP_TRAP_INSTRUCTION | SUCD_PROP_CONTROL,
                  (uint16_t)SUCD_GC_CC, SUCD_RECORD_CONTROL);
    r[10] = mkrec(0x7FFFFFFFu, 0x7FFFFFFFu,
                  SUCD_PROP_SENTINEL_VALUE,
                  (uint16_t)SUCD_GC_CN, SUCD_RECORD_CONTROL);

    /* Directory: everything defaults to "no page" first. */
    for (i = 0u; i < T_DIR_SIZE; i++)
        dir[i] = SUAS_SUCD_NO_PAGE;

    dir[0] = 0u;                                   /* dense page 0        */
    for (i = 1u; i <= 16u; i++)
        dir[i] = SUAS_SUCD_UNIFORM_TAG | 6u;       /* the Bridge tail     */
    dir[17] = SUAS_SUCD_UNIFORM_TAG | 7u;          /* the control plane   */
    for (i = 18u; i <= 32766u; i++)
        dir[i] = SUAS_SUCD_UNIFORM_TAG | 8u;       /* the Native plane    */
    dir[32767] = T_PAGE_SIZE;                      /* dense top page      */

    /* Dense page 0. */
    for (i = 0x0000u; i <= 0x0008u; i++) d[i] = 0u;
    for (i = 0x0009u; i <= 0x000Fu; i++) d[i] = SUAS_SUCD_NO_RECORD;
    for (i = 0x0010u; i <= 0x001Fu; i++) d[i] = 1u;
    d[0x0020u] = 2u;
    for (i = 0x0021u; i <= 0x0027u; i++) d[i] = 3u;
    for (i = 0x0028u; i <= 0x0029u; i++) d[i] = 4u;
    for (i = 0x002Au; i <= 0xFFFFu; i++) d[i] = 5u;

    /* Dense page 32767: the Native tail, the traps, the Sentinel. */
    for (i = 0x0000u; i <= 0xFFEFu; i++) d[T_PAGE_SIZE + i] = 8u;
    for (i = 0xFFF0u; i <= 0xFFFEu; i++) d[T_PAGE_SIZE + i] = 9u;
    d[T_PAGE_SIZE + 0xFFFFu] = 10u;
}

static void build_minimal_image(uint8_t *b)
{
    sucd_header_t *h = (sucd_header_t *)(void *)b;
    uint32_t *dir = (uint32_t *)(void *)(b + sizeof(sucd_header_t));
    sucd_record_t *r = (sucd_record_t *)(void *)(b + sizeof(sucd_header_t)
                       + T_DIR_SIZE * 4u);
    uint32_t i;

    memset(b, 0, T_MIN_SIZE);
    h->magic = SUAS_SUCD_MAGIC;
    h->version = ((uint32_t)SUAS_SUCD_VERSION_MAJOR << 16);
    h->total_records = T_MIN_NREC;
    h->plane_mask = SUCD_PLANE_BRIDGE;
    h->directory_count = T_DIR_SIZE;
    h->flags = SUCD_IMAGE_PARTITION;

    /* Page 0 is uniform, so the record must cover it whole. */
    r[0] = mkrec(0x00000000u, 0x0000FFFFu, SUCD_PROP_UNICODE_BRIDGED,
                 (uint16_t)SUCD_GC_LO, SUCD_RECORD_PRINTABLE);
    for (i = 0u; i < T_DIR_SIZE; i++)
        dir[i] = SUAS_SUCD_NO_PAGE;
    dir[0] = SUAS_SUCD_UNIFORM_TAG | 0u;
}

/* ── Version and metadata ────────────────────────────────────────── */

static void test_version_and_meta(void)
{
    const char *vs = suas_sucd_version_string();

    CHECK(vs != NULL);
    CHECK(vs[0] == 'S' && vs[1] == 'U' && vs[2] == 'A' && vs[3] == 'S');
    CHECK(vs[4] == '-' && vs[5] == '0' && vs[6] == '1' && vs[7] == '2');
    CHECK(vs[8] == '/');
    CHECK(strcmp(vs, "SUAS-012/1.0.0") == 0);

    CHECK(SUAS_SUCD_PAGE_SIZE == 65536u);
    CHECK(SUAS_SUCD_PAGE_MASK == 0xFFFFu);
    CHECK(SUAS_SUCD_DIRECTORY_SIZE == 32768u);
    CHECK(SUAS_SUCD_DIRECTORY_BYTES == 131072u);
    CHECK(SUAS_SUCD_MAX_CODEPOINT == 0x7FFFFFFFu);
    CHECK(SUAS_SUCD_NO_RECORD == 0xFFFFu);
    CHECK(SUAS_SUCD_RECORD_CAPACITY == 65535u);
    CHECK(SUAS_SUCD_MAGIC == 0x53554344u);

    /* The page split is exhaustive: the top page index is the last slot. */
    CHECK((SUAS_SUCD_MAX_CODEPOINT >> SUAS_SUCD_PAGE_SHIFT) ==
          SUAS_SUCD_DIRECTORY_SIZE - 1u);
}

/* ── Taxonomy ────────────────────────────────────────────────────── */

static void test_general_categories(void)
{
    CHECK(strcmp(suas_sucd_gc_name(SUCD_GC_CN), "Cn") == 0);
    CHECK(strcmp(suas_sucd_gc_name(SUCD_GC_LU), "Lu") == 0);
    CHECK(strcmp(suas_sucd_gc_name(SUCD_GC_LO), "Lo") == 0);
    CHECK(strcmp(suas_sucd_gc_name(SUCD_GC_ZS), "Zs") == 0);
    CHECK(strcmp(suas_sucd_gc_name(SUCD_GC_CC), "Cc") == 0);
    CHECK(strcmp(suas_sucd_gc_name(SUCD_GC_CO), "Co") == 0);
    CHECK(strcmp(suas_sucd_gc_name((sucd_general_category_t)SUCD_GC_COUNT),
                 "??") == 0);
    CHECK(strcmp(suas_sucd_gc_name((sucd_general_category_t)999), "??") == 0);

    CHECK(suas_sucd_gc_by_name("Lu", 2u) == SUCD_GC_LU);
    CHECK(suas_sucd_gc_by_name("Cn", 2u) == SUCD_GC_CN);
    CHECK(suas_sucd_gc_by_name("Co", 2u) == SUCD_GC_CO);
    /* Exact and case-sensitive: an alias that is nearly right is not right. */
    CHECK(suas_sucd_gc_by_name("lu", 2u) == SUCD_GC_CN);
    CHECK(suas_sucd_gc_by_name("LU", 2u) == SUCD_GC_CN);
    CHECK(suas_sucd_gc_by_name("L", 1u) == SUCD_GC_CN);
    CHECK(suas_sucd_gc_by_name("Luu", 3u) == SUCD_GC_CN);
    CHECK(suas_sucd_gc_by_name(NULL, 2u) == SUCD_GC_CN);
    CHECK(suas_sucd_gc_by_name("Lu", 0u) == SUCD_GC_CN);
}

static void test_properties(void)
{
    /* Every published spelling resolves, and to its own bit. */
    CHECK(suas_sucd_property_by_name("Unicode_Bridged", 15u) ==
          SUCD_PROP_UNICODE_BRIDGED);
    CHECK(suas_sucd_property_by_name("Control_Plane", 13u) ==
          SUCD_PROP_CONTROL_PLANE);
    CHECK(suas_sucd_property_by_name("Printable", 9u) == SUCD_PROP_PRINTABLE);
    CHECK(suas_sucd_property_by_name("Diagnostic_BAN", 14u) ==
          SUCD_PROP_DIAGNOSTIC_BAN);
    CHECK(suas_sucd_property_by_name("Diagnostic_WARN", 15u) ==
          SUCD_PROP_DIAGNOSTIC_WARN);
    CHECK(suas_sucd_property_by_name("Diagnostic_COM", 14u) ==
          SUCD_PROP_DIAGNOSTIC_COM);
    CHECK(suas_sucd_property_by_name("Diagnostic_SOFT", 15u) ==
          SUCD_PROP_DIAGNOSTIC_SOFT);
    CHECK(suas_sucd_property_by_name("Trap_Instruction", 16u) ==
          SUCD_PROP_TRAP_INSTRUCTION);
    CHECK(suas_sucd_property_by_name("Sentinel_Value", 14u) ==
          SUCD_PROP_SENTINEL_VALUE);
    CHECK(suas_sucd_property_by_name("Alphabetic", 10u) ==
          SUCD_PROP_ALPHABETIC);
    CHECK(suas_sucd_property_by_name("Control", 7u) == SUCD_PROP_CONTROL);
    CHECK(suas_sucd_property_by_name("LTR", 3u) == SUCD_PROP_BIDI_LTR);
    CHECK(suas_sucd_property_by_name("RTL", 3u) == SUCD_PROP_BIDI_RTL);
    CHECK(suas_sucd_property_by_name("ARABIC_AL", 9u) ==
          SUCD_PROP_BIDI_ARABIC_AL);
    CHECK(suas_sucd_property_by_name("NEUTRAL", 7u) ==
          SUCD_PROP_BIDI_NEUTRAL);
    CHECK(suas_sucd_property_by_name("MIRRORED", 8u) ==
          SUCD_PROP_BIDI_MIRRORED);
    CHECK(suas_sucd_property_by_name("WHITESPACE", 10u) ==
          SUCD_PROP_BIDI_WHITESPACE);

    CHECK(suas_sucd_property_by_name("Bridged", 7u) == 0u);
    CHECK(suas_sucd_property_by_name("printable", 9u) == 0u);
    CHECK(suas_sucd_property_by_name(NULL, 3u) == 0u);
    CHECK(suas_sucd_property_by_name("LTR", 0u) == 0u);

    CHECK(strcmp(suas_sucd_property_name(SUCD_PROP_UNICODE_BRIDGED),
                 "Unicode_Bridged") == 0);
    CHECK(strcmp(suas_sucd_property_name(SUCD_PROP_TRAP_INSTRUCTION),
                 "Trap_Instruction") == 0);
    CHECK(strcmp(suas_sucd_property_name(SUCD_PROP_BIDI_WHITESPACE),
                 "WHITESPACE") == 0);

    /* Zero, a multi-bit mask, and a reserved bit are all "no single name". */
    CHECK(suas_sucd_property_name(0u) == NULL);
    CHECK(suas_sucd_property_name(SUCD_PROP_PRINTABLE |
                                  SUCD_PROP_ALPHABETIC) == NULL);
    CHECK(suas_sucd_property_name(0x00020000u) == NULL);
}

static void test_planes(void)
{
    CHECK(suas_sucd_plane_of(0x00000000u) == SUCD_PLANE_BRIDGE);
    CHECK(suas_sucd_plane_of(0x0010FFFFu) == SUCD_PLANE_BRIDGE);
    CHECK(suas_sucd_plane_of(0x00110000u) == SUCD_PLANE_SCP);
    CHECK(suas_sucd_plane_of(0x0011FFFFu) == SUCD_PLANE_SCP);
    CHECK(suas_sucd_plane_of(0x00120000u) == SUCD_PLANE_NATIVE);
    CHECK(suas_sucd_plane_of(0x0015FFFFu) == SUCD_PLANE_NATIVE);
    /* RNUR is a carve-out inside Native, and must stay addressable. */
    CHECK(suas_sucd_plane_of(0x00160000u) == SUCD_PLANE_RNUR);
    CHECK(suas_sucd_plane_of(0x0016FFFFu) == SUCD_PLANE_RNUR);
    CHECK(suas_sucd_plane_of(0x00170000u) == SUCD_PLANE_NATIVE);
    CHECK(suas_sucd_plane_of(0x7FFFFFEFu) == SUCD_PLANE_NATIVE);
    CHECK(suas_sucd_plane_of(0x7FFFFFF0u) == SUCD_PLANE_TRAP);
    CHECK(suas_sucd_plane_of(0x7FFFFFFEu) == SUCD_PLANE_TRAP);
    CHECK(suas_sucd_plane_of(0x7FFFFFFFu) == SUCD_PLANE_SENTINEL);
    CHECK(suas_sucd_plane_of(0x80000000u) == 0u);

    CHECK(strcmp(suas_sucd_plane_name(SUCD_PLANE_BRIDGE),
                 "SUCD_PLANE_BRIDGE") == 0);
    CHECK(strcmp(suas_sucd_plane_name(SUCD_PLANE_SCP), "SUCD_PLANE_SCP") == 0);
    CHECK(strcmp(suas_sucd_plane_name(SUCD_PLANE_NATIVE),
                 "SUCD_PLANE_NATIVE") == 0);
    CHECK(strcmp(suas_sucd_plane_name(SUCD_PLANE_RNUR),
                 "SUCD_PLANE_RNUR") == 0);
    CHECK(strcmp(suas_sucd_plane_name(SUCD_PLANE_TRAP),
                 "SUCD_PLANE_TRAP") == 0);
    CHECK(strcmp(suas_sucd_plane_name(SUCD_PLANE_SENTINEL),
                 "SUCD_PLANE_SENTINEL") == 0);
    CHECK(suas_sucd_plane_name(0u) == NULL);
    CHECK(suas_sucd_plane_name(SUCD_PLANE_BRIDGE | SUCD_PLANE_SCP) == NULL);
}

/* ── Data-file range decoding (§6.1) ─────────────────────────────── */

static void test_range_forms(void)
{
    uint32_t first = 0u, last = 0u;

    /* The UCD double-dot form, six hex digits, as Props/ and Derived/ write. */
    CHECK(suas_sucd_range_form("0000..0010FFFF;Unicode_Bridged", 29u) ==
          SUCD_RANGE_FORM_UCD);
    CHECK(suas_sucd_parse_range("0000..0010FFFF;Unicode_Bridged", 29u,
                                &first, &last));
    CHECK(first == 0x0000u && last == 0x0010FFFFu);

    /* The eight-digit hyphen form, as SUCSData.txt and sudat/ write. */
    CHECK(suas_sucd_range_form("00000000-0010FFFF;UNICODE BRIDGE", 32u) ==
          SUCD_RANGE_FORM_HYPHEN);
    CHECK(suas_sucd_parse_range("00000000-0010FFFF;UNICODE BRIDGE", 32u,
                                &first, &last));
    CHECK(first == 0x00000000u && last == 0x0010FFFFu);

    /* The four-digit hyphen form plus a bare singleton, as BidiProps.txt does. */
    CHECK(suas_sucd_range_form("0021-0027;LTR", 11u) ==
          SUCD_RANGE_FORM_HYPHEN);
    CHECK(suas_sucd_parse_range("0021-0027;LTR", 11u, &first, &last));
    CHECK(first == 0x0021u && last == 0x0027u);

    CHECK(suas_sucd_range_form("0028", 4u) == SUCD_RANGE_FORM_POINT);
    CHECK(suas_sucd_parse_range("0028", 4u, &first, &last));
    CHECK(first == 0x0028u && last == 0x0028u);

    /* Leading whitespace and an inline comment are both tolerated. */
    CHECK(suas_sucd_range_form("  008E", 6u) == SUCD_RANGE_FORM_POINT);
    CHECK(suas_sucd_parse_range("  008E", 6u, &first, &last));
    CHECK(first == 0x008Eu && last == 0x008Eu);

    CHECK(suas_sucd_parse_range("0041-005A;LTR  # basic Latin", 27u,
                                &first, &last));
    CHECK(first == 0x0041u && last == 0x005Au);

    /* The top of the space is inclusive and must decode. */
    CHECK(suas_sucd_parse_range("7FFFFFFF", 8u, &first, &last));
    CHECK(first == 0x7FFFFFFFu && last == 0x7FFFFFFFu);

    /* Malformed: a comment, a blank line, a reversed pair, a value past the
     * space, and a ninth hex digit. */
    CHECK(suas_sucd_range_form("# PropList.txt", 15u) == SUCD_RANGE_FORM_NONE);
    CHECK(suas_sucd_range_form("", 0u) == SUCD_RANGE_FORM_NONE);
    CHECK(suas_sucd_range_form("   \t", 4u) == SUCD_RANGE_FORM_NONE);
    CHECK(suas_sucd_range_form("005A-0041", 9u) == SUCD_RANGE_FORM_NONE);
    CHECK(suas_sucd_parse_range("005A-0041", 9u, &first, &last) == false);
    CHECK(suas_sucd_range_form("80000000", 8u) == SUCD_RANGE_FORM_NONE);
    CHECK(suas_sucd_parse_range("80000000", 8u, &first, &last) == false);
    CHECK(suas_sucd_parse_range("123456789", 9u, &first, &last) == false);
    CHECK(suas_sucd_parse_range("0021-", 5u, &first, &last) == false);
    CHECK(suas_sucd_parse_range(NULL, 4u, &first, &last) == false);

    /* NULL out-parameters are allowed: the caller may want only the verdict. */
    CHECK(suas_sucd_parse_range("0041-005A", 9u, NULL, NULL));
}

/* ── Record predicates (§4.4) ────────────────────────────────────── */

static void test_record_predicates(void)
{
    sucd_record_t a = mkrec(0x0010u, 0x0020u, 0u, (uint16_t)SUCD_GC_CN,
                            SUCD_RECORD_PRINTABLE);
    sucd_record_t b = mkrec(0x0021u, 0x0030u, 0u, (uint16_t)SUCD_GC_CN,
                            SUCD_RECORD_PRINTABLE);
    sucd_record_t c = mkrec(0x0018u, 0x001Cu, 0u, (uint16_t)SUCD_GC_CN,
                            SUCD_RECORD_PRINTABLE);

    /* Inclusive ranges: 0x0020 and 0x0021 are adjacent, not overlapping. */
    CHECK(suas_sucd_records_overlap(&a, &b) == false);
    CHECK(suas_sucd_records_overlap(&b, &a) == false);
    /* A record entirely inside another overlaps it. */
    CHECK(suas_sucd_records_overlap(&a, &c));
    CHECK(suas_sucd_records_overlap(&c, &a));
    CHECK(suas_sucd_records_overlap(&a, &a));
    CHECK(suas_sucd_records_overlap(NULL, &a) == false);
    CHECK(suas_sucd_records_overlap(&a, NULL) == false);

    {
        sucd_record_t set[3];

        set[0] = a;
        set[1] = b;
        set[2] = mkrec(0x0031u, 0x0040u, 0u, (uint16_t)SUCD_GC_CN,
                       SUCD_RECORD_PRINTABLE);
        CHECK(suas_sucd_is_partition(set, 3u));
        /* Gaps are allowed; overlap and disorder are not. */
        set[1].first = 0x0020u;
        CHECK(suas_sucd_is_partition(set, 3u) == false);
        set[1].first = 0x0005u;
        CHECK(suas_sucd_is_partition(set, 3u) == false);
        set[1].first = 0x0022u;
        CHECK(suas_sucd_is_partition(set, 3u));
    }

    {
        sucd_record_t bad = mkrec(0x0030u, 0x0020u, 0u,
                                  (uint16_t)SUCD_GC_CN,
                                  SUCD_RECORD_PRINTABLE);
        sucd_record_t over = mkrec(0x7FFFFFF0u, 0x80000000u, 0u,
                                   (uint16_t)SUCD_GC_CN,
                                   SUCD_RECORD_PRINTABLE);

        CHECK(suas_sucd_is_partition(&bad, 1u) == false);
        CHECK(suas_sucd_is_partition(&over, 1u) == false);
    }

    CHECK(suas_sucd_is_partition(NULL, 0u));   /* the empty partition */
    CHECK(suas_sucd_is_partition(NULL, 1u) == false);
}

/* ── Image verification (§5.3) ───────────────────────────────────── */

/* Each case corrupts exactly one thing in a fresh copy and demands the verdict
 * that names it. A verifier that reports "bad image" without saying what is
 * bad is a verifier a compiler cannot act on. */
static sucd_image_error_t corrupt_and_verify(void (*mutate)(uint8_t *))
{
    sucd_image_error_t why = SUCD_IMAGE_OK;

    memcpy(g_scratch.bytes, g_img.bytes, T_IMAGE_SIZE);
    mutate(g_scratch.bytes);
    CHECK(suas_sucd_verify_image(g_scratch.bytes, T_IMAGE_SIZE, &why) == false);
    return why;
}

static void m_bad_magic(uint8_t *b) { hdr_of(b)->magic = 0xDEADBEEFu; }
static void m_bad_dir_count(uint8_t *b) { hdr_of(b)->directory_count = 4096u; }
static void m_count_over(uint8_t *b) { hdr_of(b)->total_records = 65536u; }
static void m_bad_flag(uint8_t *b)  { recs_of(b)[0].flags = 0u; }
static void m_both_flags(uint8_t *b)
{
    recs_of(b)[0].flags = SUCD_RECORD_CONTROL | SUCD_RECORD_PRINTABLE;
}
static void m_range_order(uint8_t *b) { recs_of(b)[0].first = 0x00000010u; }
static void m_out_of_space(uint8_t *b)
{
    recs_of(b)[10].last = 0x80000000u;
}
static void m_bad_gc(uint8_t *b)
{
    recs_of(b)[0].general_category = (uint16_t)SUCD_GC_COUNT;
}
static void m_reserved_prop(uint8_t *b)
{
    recs_of(b)[0].property_mask = 0x00020000u;
}
static void m_unordered(uint8_t *b) { recs_of(b)[1].first = 0x00000000u; }
static void m_overlapping(uint8_t *b) { recs_of(b)[1].first = 0x00000005u; }
static void m_dir_oob(uint8_t *b)
{
    dir_of(b)[1] = SUAS_SUCD_UNIFORM_TAG | 99u;
}
static void m_dense_over(uint8_t *b)
{
    dir_of(b)[0] = 0x7FFFFFF0u;
}
static void m_page_not_covered(uint8_t *b)
{
    dir_of(b)[1] = SUAS_SUCD_UNIFORM_TAG | 0u;
}
static void m_dense_wrong_record(uint8_t *b)
{
    /* Codepoint 0x0020 is R2; point it at R0, which does not cover it. */
    dense_of(b)[0x0020u] = 0u;
}

static void test_image_verification(void)
{
    sucd_image_error_t why = SUCD_IMAGE_UNKNOWN;
    uint8_t *img;

    build_image(g_img.bytes);
    img = g_img.bytes;

    CHECK(suas_sucd_verify_image(img, T_IMAGE_SIZE, &why));
    CHECK(why == SUCD_IMAGE_OK);
    CHECK(strcmp(suas_sucd_image_error_name(SUCD_IMAGE_OK),
                 "SUCD_IMAGE_OK") == 0);

    /* A NULL reason must not crash the verifier. */
    CHECK(suas_sucd_verify_image(img, T_IMAGE_SIZE, NULL));

    CHECK(suas_sucd_verify_image(NULL, T_IMAGE_SIZE, &why) == false);
    CHECK(why == SUCD_IMAGE_NULL_IMAGE);
    CHECK(suas_sucd_verify_image(img, 0u, &why) == false);
    CHECK(why == SUCD_IMAGE_TRUNCATED);
    CHECK(suas_sucd_verify_image(img, sizeof(sucd_header_t) - 1u, &why) ==
          false);
    CHECK(why == SUCD_IMAGE_TRUNCATED);
    /* One byte short of the declared layout is not a layout. */
    CHECK(suas_sucd_verify_image(img, T_IMAGE_SIZE - 1u, &why) == false);
    CHECK(why == SUCD_IMAGE_SIZE_MISMATCH);

    CHECK(corrupt_and_verify(m_bad_magic) == SUCD_IMAGE_BAD_MAGIC);
    CHECK(corrupt_and_verify(m_bad_dir_count) ==
          SUCD_IMAGE_BAD_DIRECTORY_COUNT);
    CHECK(corrupt_and_verify(m_count_over) == SUCD_IMAGE_RECORD_COUNT_OVER);
    CHECK(corrupt_and_verify(m_bad_flag) == SUCD_IMAGE_BAD_RECORD_FLAG);
    CHECK(corrupt_and_verify(m_both_flags) == SUCD_IMAGE_BAD_RECORD_FLAG);
    CHECK(corrupt_and_verify(m_range_order) == SUCD_IMAGE_RANGE_ORDER);
    CHECK(corrupt_and_verify(m_out_of_space) == SUCD_IMAGE_OUT_OF_SPACE);
    CHECK(corrupt_and_verify(m_bad_gc) == SUCD_IMAGE_GC_OUT_OF_RANGE);
    CHECK(corrupt_and_verify(m_reserved_prop) ==
          SUCD_IMAGE_RESERVED_PROPERTY_BIT);
    CHECK(corrupt_and_verify(m_unordered) == SUCD_IMAGE_UNORDERED_RECORDS);
    CHECK(corrupt_and_verify(m_overlapping) ==
          SUCD_IMAGE_OVERLAPPING_RECORDS);
    CHECK(corrupt_and_verify(m_dir_oob) ==
          SUCD_IMAGE_DIRECTORY_OUT_OF_RANGE);
    CHECK(corrupt_and_verify(m_dense_over) == SUCD_IMAGE_DENSE_OFFSET_OVER);
    CHECK(corrupt_and_verify(m_page_not_covered) ==
          SUCD_IMAGE_PAGE_NOT_COVERED);
    CHECK(corrupt_and_verify(m_dense_wrong_record) ==
          SUCD_IMAGE_PAGE_NOT_COVERED);

    /* The outcome names round-trip, and the tail is a fixed fallback. */
    CHECK(strcmp(suas_sucd_image_error_name(SUCD_IMAGE_BAD_MAGIC),
                 "SUCD_IMAGE_BAD_MAGIC") == 0);
    CHECK(strcmp(suas_sucd_image_error_name(SUCD_IMAGE_TRUNCATED),
                 "SUCD_IMAGE_TRUNCATED") == 0);
    CHECK(strcmp(suas_sucd_image_error_name((sucd_image_error_t)999),
                 "SUCD_IMAGE_UNKNOWN") == 0);
}

/* ── Binding and queries (§5.2) ─────────────────────────────────── */

static void expect_record(uint32_t cp, uint16_t gc, uint8_t flags,
                          uint32_t props, uint32_t first, uint32_t last)
{
    sucd_record_t r;

    memset(&r, 0xAA, sizeof r);
    CHECK(suas_sucd_get_record(cp, &r));
    CHECK(r.general_category == gc);
    CHECK(r.flags == flags);
    CHECK(r.property_mask == props);
    CHECK(r.first == first);
    CHECK(r.last == last);
}

static void test_default_record(void)
{
    sucd_record_t r;

    memset(&r, 0xAA, sizeof r);
    CHECK(suas_sucd_default_record(&r));
    CHECK(r.first == 0u && r.last == 0u);
    CHECK(r.property_mask == 0u);
    CHECK(r.general_category == (uint16_t)SUCD_GC_CN);
    CHECK(r.script_id == SUAS_SCRIPT_UNKNOWN);
    CHECK(r.flags == SUCD_RECORD_PRINTABLE);
    CHECK(suas_sucd_default_record(NULL) == false);
}

static void test_binding_and_queries(void)
{
    sucd_record_t r;

    build_image(g_img.bytes);

    CHECK(!suas_sucd_is_loaded());
    CHECK(suas_sucd_header() == NULL);
    /* Unbound, the engine answers with the default rather than failing. */
    CHECK(suas_sucd_get_record(0x0041u, &r));
    CHECK(r.general_category == (uint16_t)SUCD_GC_CN);

    CHECK(suas_sucd_init(g_img.bytes, T_IMAGE_SIZE));
    CHECK(suas_sucd_is_loaded());
    CHECK(suas_sucd_header() != NULL);
    CHECK(suas_sucd_header()->total_records == T_NREC);
    CHECK(suas_sucd_header()->flags == SUCD_IMAGE_PARTITION);
    /* Binding is idempotent. */
    CHECK(suas_sucd_init(g_img.bytes, T_IMAGE_SIZE));

    /* Dense page 0: three distinct classifications and a gap. */
    expect_record(0x0000u, (uint16_t)SUCD_GC_CC, SUCD_RECORD_CONTROL,
                  SUCD_PROP_UNICODE_BRIDGED | SUCD_PROP_CONTROL,
                  0x00000000u, 0x00000008u);
    expect_record(0x0008u, (uint16_t)SUCD_GC_CC, SUCD_RECORD_CONTROL,
                  SUCD_PROP_UNICODE_BRIDGED | SUCD_PROP_CONTROL,
                  0x00000000u, 0x00000008u);
    /* The gap falls to the default record. */
    memset(&r, 0xAA, sizeof r);
    CHECK(suas_sucd_get_record(0x000Au, &r));
    CHECK(r.general_category == (uint16_t)SUCD_GC_CN);
    CHECK(r.property_mask == 0u);

    expect_record(0x0010u, (uint16_t)SUCD_GC_CC, SUCD_RECORD_CONTROL,
                  SUCD_PROP_UNICODE_BRIDGED | SUCD_PROP_CONTROL,
                  0x00000010u, 0x0000001Fu);
    expect_record(0x0020u, (uint16_t)SUCD_GC_ZS, SUCD_RECORD_PRINTABLE,
                  SUCD_PROP_UNICODE_BRIDGED | SUCD_PROP_BIDI_WHITESPACE,
                  0x00000020u, 0x00000020u);
    expect_record(0x0025u, (uint16_t)SUCD_GC_PO, SUCD_RECORD_PRINTABLE,
                  SUCD_PROP_UNICODE_BRIDGED | SUCD_PROP_BIDI_LTR,
                  0x00000021u, 0x00000027u);
    expect_record(0x0028u, (uint16_t)SUCD_GC_PO, SUCD_RECORD_PRINTABLE,
                  SUCD_PROP_UNICODE_BRIDGED | SUCD_PROP_BIDI_MIRRORED,
                  0x00000028u, 0x00000029u);
    expect_record(0x0041u, (uint16_t)SUCD_GC_LO, SUCD_RECORD_PRINTABLE,
                  SUCD_PROP_UNICODE_BRIDGED | SUCD_PROP_ALPHABETIC
                      | SUCD_PROP_BIDI_LTR,
                  0x0000002Au, 0x0000FFFFu);

    /* Uniform pages: the whole Bridge tail, the control plane, Native. */
    expect_record(0x00010000u, (uint16_t)SUCD_GC_LO, SUCD_RECORD_PRINTABLE,
                  SUCD_PROP_UNICODE_BRIDGED | SUCD_PROP_ALPHABETIC,
                  0x00010000u, 0x0010FFFFu);
    expect_record(0x0010FFFFu, (uint16_t)SUCD_GC_LO, SUCD_RECORD_PRINTABLE,
                  SUCD_PROP_UNICODE_BRIDGED | SUCD_PROP_ALPHABETIC,
                  0x00010000u, 0x0010FFFFu);
    expect_record(0x00110000u, (uint16_t)SUCD_GC_CC, SUCD_RECORD_CONTROL,
                  SUCD_PROP_CONTROL_PLANE | SUCD_PROP_CONTROL,
                  0x00110000u, 0x0011FFFFu);
    expect_record(0x00123456u, (uint16_t)SUCD_GC_LO, SUCD_RECORD_PRINTABLE,
                  SUCD_PROP_PRINTABLE | SUCD_PROP_ALPHABETIC,
                  0x00120000u, 0x7FFFFFEFu);
    expect_record(0x7FFFFFEFu, (uint16_t)SUCD_GC_LO, SUCD_RECORD_PRINTABLE,
                  SUCD_PROP_PRINTABLE | SUCD_PROP_ALPHABETIC,
                  0x00120000u, 0x7FFFFFEFu);

    /* The dense top page: the Native tail, the traps, the Sentinel. */
    expect_record(0x7FFFFFF0u, (uint16_t)SUCD_GC_CC, SUCD_RECORD_CONTROL,
                  SUCD_PROP_TRAP_INSTRUCTION | SUCD_PROP_CONTROL,
                  0x7FFFFFF0u, 0x7FFFFFFEu);
    expect_record(0x7FFFFFFEu, (uint16_t)SUCD_GC_CC, SUCD_RECORD_CONTROL,
                  SUCD_PROP_TRAP_INSTRUCTION | SUCD_PROP_CONTROL,
                  0x7FFFFFF0u, 0x7FFFFFFEu);
    expect_record(0x7FFFFFFFu, (uint16_t)SUCD_GC_CN, SUCD_RECORD_CONTROL,
                  SUCD_PROP_SENTINEL_VALUE,
                  0x7FFFFFFFu, 0x7FFFFFFFu);

    /* Out of space and NULL output. */
    memset(&r, 0xAA, sizeof r);
    CHECK(suas_sucd_get_record(0x80000000u, &r));
    CHECK(r.general_category == (uint16_t)SUCD_GC_CN);
    CHECK(suas_sucd_get_record(0x0041u, NULL) == false);

    CHECK(suas_sucd_property_mask(0x0041u) ==
          (SUCD_PROP_UNICODE_BRIDGED | SUCD_PROP_ALPHABETIC
               | SUCD_PROP_BIDI_LTR));
    CHECK(suas_sucd_property_mask(0x80000000u) == 0u);

    suas_sucd_shutdown();
    CHECK(!suas_sucd_is_loaded());
    CHECK(suas_sucd_header() == NULL);
    CHECK(suas_sucd_get_record(0x0041u, &r));
    CHECK(r.general_category == (uint16_t)SUCD_GC_CN);
}

static void test_no_page_default(void)
{
    sucd_record_t r;

    build_minimal_image(g_min.bytes);
    suas_sucd_shutdown();
    CHECK(suas_sucd_init(g_min.bytes, T_MIN_SIZE));
    CHECK(suas_sucd_header()->total_records == T_MIN_NREC);

    /* Page 0 is uniform and covered. */
    CHECK(suas_sucd_get_record(0x0010u, &r));
    CHECK(r.general_category == (uint16_t)SUCD_GC_LO);
    CHECK(r.first == 0x00000000u && r.last == 0x0000FFFFu);

    /* Every other page is unassigned and answers with the default. */
    CHECK(suas_sucd_get_record(0x00010000u, &r));
    CHECK(r.general_category == (uint16_t)SUCD_GC_CN);
    CHECK(suas_sucd_get_record(0x40000000u, &r));
    CHECK(r.general_category == (uint16_t)SUCD_GC_CN);
    CHECK(suas_sucd_get_record(0x7FFFFFFFu, &r));
    CHECK(r.general_category == (uint16_t)SUCD_GC_CN);

    suas_sucd_shutdown();
}

/* ── SUAS-010 (SSRE) linkage gate (§7) ──────────────────────────── */

static void test_ssre_link(void)
{
    sucd_schema_link_t link;

    link.schema_spec_id = "SUAS-011";   /* SCMS, the manifest grammar */
    link.schema_major = 1u;
    link.schema_minor = 0u;
    link.schema_mandatory = true;
    CHECK(suas_sucd_verify_schema_link(&link));

    /* A mandatory claim against an optional data spec is a linkage error. */
    link.schema_spec_id = "SUTS-005";
    CHECK(suas_sucd_verify_schema_link(&link) == false);
    link.schema_mandatory = false;
    CHECK(suas_sucd_verify_schema_link(&link));

    /* An unregistered schema, and an over-strict floor. */
    link.schema_spec_id = "SUAS-014";
    CHECK(suas_sucd_verify_schema_link(&link) == false);
    link.schema_spec_id = "SUAS-011";
    link.schema_major = 2u;
    CHECK(suas_sucd_verify_schema_link(&link) == false);

    CHECK(suas_sucd_verify_schema_link(NULL) == false);
    link.schema_spec_id = NULL;
    CHECK(suas_sucd_verify_schema_link(&link) == false);
}

/* SUCD and SSRE must agree that SUCD is part of the architecture. */
static void test_sucd_is_registered(void)
{
    CHECK(suas_ssre_is_registered(SUAS_SUCD_SPEC_ID));
    CHECK(suas_ssre_is_mandatory(SUAS_SUCD_SPEC_ID));
    CHECK(suas_ssre_module_of(SUAS_SUCD_SPEC_ID) != NULL);
}

int main(void)
{
    test_version_and_meta();
    test_general_categories();
    test_properties();
    test_planes();
    test_range_forms();
    test_record_predicates();
    test_image_verification();
    test_default_record();
    test_binding_and_queries();
    test_no_page_default();
    test_ssre_link();
    test_sucd_is_registered();

    if (g_fail) {
        printf("SUAS-012 SUCD tests FAILED\n");
        return 1;
    }
    printf("SUAS-012 SUCD tests passed\n");
    return 0;
}
