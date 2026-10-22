/**
 * SUTR-012 SUCSL — SuperUnicode Cuneiform Sign Lists
 *
 * Unit tests for the zero-allocation, static-table, O(1) resolution of a
 * Cuneiform sign's category, primary value hash, wedge count, chronological
 * period, compound composition and sequence spelling — together with the zone
 * taxonomy of §4.1, the RNUR extension carve-out of §5 and the taxonomy names
 * of §8 (a UTR #56 analog).
 *
 * Covers: version/meta, the compile-time 12-byte sign geometry and 24-byte
 * compound geometry, zone dispatch and the RNUR neography carve-out, FNV-1a
 * value/codepoint hashing, the curated sign registry and its synthesized
 * fallback, the classification predicates and category projection, compound
 * composition and sequence lookup, hierarchy indices, enumeration and table
 * self-consistency, idempotent initialization, taxonomy names, and
 * registration in SUAS-010 (SSRE).
 */

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>

#include "sutr/sutr_sucsl.h"
#include "suas/suas_ssre.h"

static int g_fail = 0;
#define CHECK(cond) do { if (!(cond)) { \
    printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); g_fail = 1; \
} } while (0)

static int streq(const char *a, const char *b)
{
    if (a == NULL || b == NULL)
        return 0;
    while (*a != '\0' && *a == *b) {
        a++;
        b++;
    }
    return *a == *b;
}

/* ── Version, limits, and record geometry (§4.2, §4.6) ───────────── */

static void test_version_and_meta(void)
{
    CHECK(sutr_sucsl_version_string() != NULL);
    CHECK(streq(sutr_sucsl_version_string(), "1.0.0"));

    CHECK(streq(SUCSL_SPEC_ID, "SUTR-012"));
    CHECK(streq(SUCSL_SPEC_TITLE, "SuperUnicode Cuneiform Sign Lists"));
    CHECK(streq(SUCSL_MODULE, "sutr_sucsl"));
    CHECK(SUCSL_VERSION_MAJOR == 1);
    CHECK(SUCSL_VERSION_MINOR == 0);
    CHECK(SUCSL_VERSION_PATCH == 0);

    /* The sign index is a power of two and strictly larger than the capacity,
     * so the open-addressing probe always finds a free slot. */
    CHECK(SUCSL_SIGN_INDEX_SIZE > SUCSL_SIGN_CAPACITY);
    CHECK((SUCSL_SIGN_INDEX_SIZE & (SUCSL_SIGN_INDEX_SIZE - 1u)) == 0u);
    CHECK(SUCSL_COMPOUND_CAPACITY >= 5u);
    CHECK(SUCSL_SEQUENCE_CAPACITY >= 8u);

    CHECK(SUCSL_FNV_OFFSET_BASIS == 2166136261u);
    CHECK(SUCSL_FNV_PRIME == 16777619u);

    /* The sign record is fixed-size and padding-free (§4.2): 12 bytes. */
    CHECK(sizeof(sucsl_sign_entry_t) == 12U);
    CHECK(offsetof(sucsl_sign_entry_t, codepoint) == 0U);
    CHECK(offsetof(sucsl_sign_entry_t, primary_value_hash) == 4U);
    CHECK(offsetof(sucsl_sign_entry_t, sign_flags) == 8U);
    CHECK(offsetof(sucsl_sign_entry_t, wedge_count) == 10U);
    CHECK(offsetof(sucsl_sign_entry_t, historical_period) == 11U);

    /* The compound/sequence descriptor is 24 bytes (§4.6). */
    CHECK(sizeof(sucsl_compound_t) == 24U);
    CHECK(offsetof(sucsl_compound_t, sign) == 0U);
    CHECK(offsetof(sucsl_compound_t, name_hash) == 4U);
    CHECK(offsetof(sucsl_compound_t, component_a) == 8U);
    CHECK(offsetof(sucsl_compound_t, component_b) == 12U);
    CHECK(offsetof(sucsl_compound_t, component_c) == 16U);
    CHECK(offsetof(sucsl_compound_t, component_count) == 20U);
    CHECK(offsetof(sucsl_compound_t, compound_flags) == 21U);
    CHECK(offsetof(sucsl_compound_t, reserved) == 22U);

    /* The four sketch flags are disjoint and the assigned mask is the low
     * byte. */
    CHECK(SUCSL_FLAG_LOGOGRAM == (1U << 0));
    CHECK(SUCSL_FLAG_SYLLABIC == (1U << 1));
    CHECK(SUCSL_FLAG_NUMERICAL == (1U << 2));
    CHECK(SUCSL_FLAG_COMPOSITE == (1U << 3));
    CHECK((SUCSL_FLAG_LOGOGRAM & SUCSL_FLAG_SYLLABIC) == 0u);
    CHECK((SUCSL_FLAG_NUMERICAL & SUCSL_FLAG_COMPOSITE) == 0u);
    CHECK((SUCSL_FLAG_ASSIGNED_MASK & 0xFF00u) == 0u);

    /* Block bounds are ordered and the RNUR carve-out sits in Native. */
    CHECK(SUCSL_BLOCK_CUNEIFORM_MIN < SUCSL_BLOCK_CUNEIFORM_MAX);
    CHECK(SUCSL_BLOCK_CUNEIFORM_MAX + 1u == SUCSL_BLOCK_NUMBERS_MIN);
    CHECK(SUCSL_BLOCK_NUMBERS_MAX + 1u == SUCSL_BLOCK_EARLY_MIN);
    CHECK(SUCSL_RNUR_MIN > SUCSL_LIMIT_SCP_MAX);
    CHECK(SUCSL_RNUR_MAX < SUCSL_LIMIT_NATIVE_MAX);
}

/* ── Zone dispatch (§4.1, §5) ───────────────────────────────────── */

static void test_zones(void)
{
    CHECK(sutr_sucsl_zone_of(0x41u) == SUCSL_ZONE_BRIDGE);
    CHECK(sutr_sucsl_zone_of(0x12000u) == SUCSL_ZONE_BRIDGE);
    CHECK(sutr_sucsl_zone_of(0x10FFFFu) == SUCSL_ZONE_BRIDGE);
    CHECK(sutr_sucsl_zone_of(0x110000u) == SUCSL_ZONE_SCP);
    CHECK(sutr_sucsl_zone_of(0x11FFFFu) == SUCSL_ZONE_SCP);
    CHECK(sutr_sucsl_zone_of(0x120000u) == SUCSL_ZONE_NATIVE);
    CHECK(sutr_sucsl_zone_of(0x15FFFFu) == SUCSL_ZONE_NATIVE);
    CHECK(sutr_sucsl_zone_of(0x160000u) == SUCSL_ZONE_RNUR);
    CHECK(sutr_sucsl_zone_of(0x16FFFFu) == SUCSL_ZONE_RNUR);
    CHECK(sutr_sucsl_zone_of(0x170000u) == SUCSL_ZONE_NATIVE);
    CHECK(sutr_sucsl_zone_of(0x7FFFFFEFu) == SUCSL_ZONE_NATIVE);
    CHECK(sutr_sucsl_zone_of(0x7FFFFFF0u) == SUCSL_ZONE_TRAP);
    CHECK(sutr_sucsl_zone_of(0x7FFFFFFEu) == SUCSL_ZONE_TRAP);
    CHECK(sutr_sucsl_zone_of(0x7FFFFFFFu) == SUCSL_ZONE_SENTINEL);
    CHECK(sutr_sucsl_zone_of(0x80000000u) == SUCSL_ZONE_EXTSUCS);
    CHECK(sutr_sucsl_zone_of(0xFFFFFFFFu) == SUCSL_ZONE_EXTSUCS);

    CHECK(sutr_sucsl_is_text_space(0x12000u));
    CHECK(sutr_sucsl_is_text_space(0x110000u));
    CHECK(sutr_sucsl_is_text_space(0x120000u));
    CHECK(sutr_sucsl_is_text_space(0x160000u));
    CHECK(!sutr_sucsl_is_text_space(0x7FFFFFF0u));
    CHECK(!sutr_sucsl_is_text_space(0x7FFFFFFFu));
    CHECK(!sutr_sucsl_is_text_space(0x80000000u));

    CHECK(sutr_sucsl_is_native(0x120000u));
    CHECK(sutr_sucsl_is_native(0x170000u));
    CHECK(!sutr_sucsl_is_native(0x160000u));

    CHECK(sutr_sucsl_is_neography(0x160000u));
    CHECK(sutr_sucsl_is_neography(0x16FFFFu));
    CHECK(!sutr_sucsl_is_neography(0x15FFFFu));
    CHECK(!sutr_sucsl_is_neography(0x170000u));

    CHECK(sutr_sucsl_zone_name(SUCSL_ZONE_BRIDGE)[0] == 'B');
    CHECK(streq(sutr_sucsl_zone_name(SUCSL_ZONE_SCP), "SCP"));
    CHECK(streq(sutr_sucsl_zone_name(SUCSL_ZONE_RNUR), "RNUR"));
    CHECK(streq(sutr_sucsl_zone_name(SUCSL_ZONE_NATIVE), "Native"));
    CHECK(streq(sutr_sucsl_zone_name(SUCSL_ZONE_TRAP), "Trap"));
    CHECK(streq(sutr_sucsl_zone_name(SUCSL_ZONE_SENTINEL), "Sentinel"));
    CHECK(streq(sutr_sucsl_zone_name(SUCSL_ZONE_EXTSUCS), "ExtSUCS"));
}

static void test_neography_set(void)
{
    uint32_t set = 0u;

    CHECK(sutr_sucsl_neography_set(0x160000u, &set));
    CHECK(set == 0u);
    CHECK(sutr_sucsl_neography_set(0x1600FFu, &set));
    CHECK(set == 0u);
    CHECK(sutr_sucsl_neography_set(0x160100u, &set));
    CHECK(set == 1u);
    CHECK(sutr_sucsl_neography_set(0x16FFFFu, &set));
    CHECK(set == 0xFFu);
    CHECK(!sutr_sucsl_neography_set(0x12000u, &set));
    CHECK(!sutr_sucsl_neography_set(0x160000u, NULL));
}

/* ── Hashing (§4.4) ─────────────────────────────────────────────── */

static void test_hash(void)
{
    CHECK(sutr_sucsl_hash_value("am") == 0x4024F153u);
    CHECK(sutr_sucsl_hash_value("geme2") == 0x0D95F4ABu);
    CHECK(sutr_sucsl_hash_value("diri") == 0x635E3147u);
    CHECK(sutr_sucsl_hash_value("idigna") == 0x0D9D7CCDu);
    CHECK(sutr_sucsl_hash_value("dalla") == 0xB91EC5A5u);
    CHECK(sutr_sucsl_hash_value("u5") == 0x0842AE3Fu);
    CHECK(sutr_sucsl_hash_value("ash-shur") == 0x158520B6u);
    CHECK(sutr_sucsl_hash_value("siki-lam") == 0xD7AE3C7Eu);
    CHECK(sutr_sucsl_hash_value("me-esh") == 0xC53E2B7Au);
    CHECK(sutr_sucsl_hash_value("gudxkur") == 0xCB74D2C5u);
    CHECK(sutr_sucsl_hash_value("dubxesh2") == 0xAF07276Au);
    CHECK(sutr_sucsl_hash_value("gumxshe") == 0x80FA16FEu);
    CHECK(sutr_sucsl_hash_value("dubxshe") == 0x36817CDAu);

    CHECK(sutr_sucsl_hash_value(NULL) == 0u);
    CHECK(sutr_sucsl_hash_value("") == SUCSL_FNV_OFFSET_BASIS);

    CHECK(sutr_sucsl_hash_codepoint(0x12000u) == 0x0269E40Cu);
    CHECK(sutr_sucsl_hash_codepoint(0x12000u) ==
          sutr_sucsl_hash_codepoint(0x12000u));
    CHECK(sutr_sucsl_hash_codepoint(0x12000u) !=
          sutr_sucsl_hash_codepoint(0x12001u));

    CHECK(sutr_sucsl_primary_value_hash(0x12120u) ==
          sutr_sucsl_hash_value("am"));
    CHECK(sutr_sucsl_primary_value_hash(0x12228u) ==
          sutr_sucsl_hash_value("me"));
    CHECK(sutr_sucsl_primary_value_hash(0x12001u) == 0u);
    CHECK(sutr_sucsl_primary_value_hash(0x41u) == 0u);
}

/* ── Curated sign resolution (§4.2-§4.5) ────────────────────────── */

static void test_get_entry(void)
{
    sucsl_sign_entry_t entry;
    sucsl_status_code_t err = SUCSL_OK;

    /* AM = GUD×KUR: a composite that is also a logogram and a syllabogram. */
    entry.codepoint = 0u;
    CHECK(sutr_sucsl_get_sign_entry(0x12120u, &entry));
    CHECK(entry.codepoint == 0x12120u);
    CHECK((entry.sign_flags & SUCSL_FLAG_LOGOGRAM) != 0u);
    CHECK((entry.sign_flags & SUCSL_FLAG_SYLLABIC) != 0u);
    CHECK((entry.sign_flags & SUCSL_FLAG_COMPOSITE) != 0u);
    CHECK((entry.sign_flags & SUCSL_FLAG_NUMERICAL) == 0u);
    CHECK((entry.sign_flags & SUCSL_FLAG_DEPRECATED) == 0u);
    CHECK(entry.wedge_count == 7u);
    CHECK(entry.historical_period == SUCSL_PERIOD_ALL);
    CHECK(entry.primary_value_hash == sutr_sucsl_hash_value("am"));

    /* Numerals: the plain four-diš and the Neo-Assyrian four-limmu variant. */
    CHECK(sutr_sucsl_get_sign_entry(0x12409u, &entry));
    CHECK(entry.codepoint == 0x12409u);
    CHECK(entry.sign_flags == SUCSL_FLAG_NUMERICAL);
    CHECK(entry.historical_period == SUCSL_PERIOD_UR_III);

    CHECK(sutr_sucsl_get_sign_entry(0x1243Cu, &entry));
    CHECK(entry.codepoint == 0x1243Cu);
    CHECK((entry.sign_flags & SUCSL_FLAG_NUMERICAL) != 0u);
    CHECK((entry.sign_flags & SUCSL_FLAG_VARIANT) != 0u);
    CHECK(entry.historical_period == SUCSL_PERIOD_NEO_ASSYRIAN);

    /* The 2(u) twenty-graphic is Early Dynastic. */
    CHECK(sutr_sucsl_get_sign_entry(0x12399u, &entry));
    CHECK(entry.codepoint == 0x12399u);
    CHECK(entry.sign_flags == SUCSL_FLAG_NUMERICAL);
    CHECK(entry.historical_period == SUCSL_PERIOD_EARLY_DYNASTIC);

    /* MEŠ is atomic; TI₂ and ŠAR₂ carry period tags. */
    CHECK(sutr_sucsl_get_sign_entry(0x1238Cu, &entry));
    CHECK(entry.codepoint == 0x1238Cu);
    CHECK((entry.sign_flags & SUCSL_FLAG_LOGOGRAM) != 0u);
    CHECK((entry.sign_flags & SUCSL_FLAG_SYLLABIC) != 0u);
    CHECK(entry.historical_period == SUCSL_PERIOD_NEO_ASSYRIAN);

    CHECK(sutr_sucsl_get_sign_entry(0x12397u, &entry));
    CHECK(entry.historical_period == SUCSL_PERIOD_OLD_ASSYRIAN);

    CHECK(sutr_sucsl_get_sign_entry(0x122B9u, &entry));
    CHECK(entry.historical_period == SUCSL_PERIOD_EARLY_DYNASTIC);

    /* The three withdrawn signs of §3 still resolve, flagged deprecated. */
    CHECK(sutr_sucsl_get_sign_entry(0x1207Fu, &entry));
    CHECK((entry.sign_flags & SUCSL_FLAG_DEPRECATED) != 0u);
    CHECK((entry.sign_flags & SUCSL_FLAG_COMPOSITE) != 0u);

    CHECK(sutr_sucsl_get_sign_entry(0x12376u, &entry));
    CHECK((entry.sign_flags & SUCSL_FLAG_DEPRECATED) != 0u);
    CHECK((entry.sign_flags & SUCSL_FLAG_COMPOSITE) != 0u);

    CHECK(sutr_sucsl_get_sign_entry(0x12124u, &entry));
    CHECK((entry.sign_flags & SUCSL_FLAG_DEPRECATED) != 0u);

    /* A basic sign and a syllabic-only sign. */
    CHECK(sutr_sucsl_get_sign_entry(0x12000u, &entry));
    CHECK(entry.codepoint == 0x12000u);
    CHECK(entry.wedge_count == 2u);
    CHECK(entry.historical_period == SUCSL_PERIOD_ALL);

    CHECK(sutr_sucsl_get_sign_entry(0x12147u, &entry));
    CHECK(entry.codepoint == 0x12147u);
    CHECK((entry.sign_flags & SUCSL_FLAG_SYLLABIC) != 0u);
    CHECK((entry.sign_flags & SUCSL_FLAG_LOGOGRAM) == 0u);

    /* An uncurated sign inside an encoded block is synthesized, not rejected. */
    CHECK(sutr_sucsl_get_sign_entry(0x12001u, &entry));
    CHECK(entry.codepoint == 0x12001u);
    CHECK(entry.sign_flags == SUCSL_FLAG_UNCLASSIFIED);
    CHECK(entry.wedge_count == 0u);
    CHECK(entry.historical_period == SUCSL_PERIOD_UNKNOWN);
    CHECK(entry.primary_value_hash == 0u);

    /* RNUR is the designated extension space. */
    CHECK(sutr_sucsl_get_sign_entry(0x160000u, &entry));
    CHECK(entry.codepoint == 0x160000u);
    CHECK(entry.sign_flags ==
          (SUCSL_FLAG_UNCLASSIFIED | SUCSL_FLAG_EXTENSION));
    CHECK(entry.historical_period == SUCSL_PERIOD_UNKNOWN);

    /* Ordinary characters, native objects and illegal zones are rejected. */
    err = SUCSL_OK;
    CHECK(!sutr_sucsl_get_sign_entry_ex(0x41u, &entry, &err));
    CHECK(err == SUCSL_ERR_NOT_CUNEIFORM);
    CHECK(!sutr_sucsl_get_sign_entry(0x120000u, &entry));
    CHECK(!sutr_sucsl_get_sign_entry(0x110000u, &entry));
    CHECK(!sutr_sucsl_get_sign_entry(0x7FFFFFF0u, &entry));
    CHECK(!sutr_sucsl_get_sign_entry(0x7FFFFFFFu, &entry));

    err = SUCSL_OK;
    CHECK(!sutr_sucsl_get_sign_entry_ex(0x110000u, &entry, &err));
    CHECK(err == SUCSL_ERR_NOT_CUNEIFORM);
    CHECK(!sutr_sucsl_get_sign_entry_ex(0x7FFFFFFFu, &entry, &err));
    CHECK(err == SUCSL_ERR_ILLEGAL_ZONE);

    err = SUCSL_OK;
    CHECK(!sutr_sucsl_get_sign_entry_ex(0x12000u, NULL, &err));
    CHECK(err == SUCSL_ERR_NULL_ARGUMENT);
    CHECK(!sutr_sucsl_get_sign_entry(0x12000u, NULL));
    CHECK(!sutr_sucsl_get_sign_entry_ex(0x12000u, NULL, NULL));

    /* A successful extended query reports SUCSL_OK. */
    err = SUCSL_ERR_ILLEGAL_ZONE;
    CHECK(sutr_sucsl_get_sign_entry_ex(0x12000u, &entry, &err));
    CHECK(err == SUCSL_OK);
}

/* ── Classification predicates and category projection (§2, §4.3) ─ */

static void test_predicates(void)
{
    CHECK(sutr_sucsl_is_cuneiform(0x12000u));
    CHECK(sutr_sucsl_is_cuneiform(0x123FFu));
    CHECK(sutr_sucsl_is_cuneiform(0x12409u));
    CHECK(sutr_sucsl_is_cuneiform(0x1254Fu));
    CHECK(!sutr_sucsl_is_cuneiform(0x12550u));
    CHECK(!sutr_sucsl_is_cuneiform(0x41u));
    CHECK(!sutr_sucsl_is_cuneiform(0x160000u));

    CHECK(sutr_sucsl_is_sign(0x12000u));
    CHECK(sutr_sucsl_is_sign(0x12120u));
    CHECK(!sutr_sucsl_is_sign(0x12001u));
    CHECK(!sutr_sucsl_is_sign(0x160000u));
    CHECK(!sutr_sucsl_is_sign(0x41u));

    CHECK(sutr_sucsl_is_logogram(0x12120u));
    CHECK(sutr_sucsl_is_syllabic(0x12120u));
    CHECK(!sutr_sucsl_is_numerical(0x12120u));
    CHECK(sutr_sucsl_is_composite(0x12120u));

    CHECK(sutr_sucsl_is_numerical(0x12409u));
    CHECK(!sutr_sucsl_is_logogram(0x12409u));
    CHECK(!sutr_sucsl_is_syllabic(0x12409u));
    CHECK(!sutr_sucsl_is_composite(0x12409u));

    CHECK(sutr_sucsl_is_deprecated(0x1207Fu));
    CHECK(!sutr_sucsl_is_deprecated(0x1207Eu));
    CHECK(sutr_sucsl_is_deprecated(0x12376u));
    CHECK(sutr_sucsl_is_deprecated(0x12124u));

    CHECK(sutr_sucsl_is_variant(0x1243Cu));
    CHECK(sutr_sucsl_is_variant(0x12441u));
    CHECK(!sutr_sucsl_is_variant(0x12409u));

    /* The projection precedence is numerical, syllabic, logographic,
     * composite, unknown. */
    CHECK(sutr_sucsl_category_of(SUCSL_FLAG_NUMERICAL) ==
          SUCSL_CATEGORY_NUMERICAL);
    CHECK(sutr_sucsl_category_of(SUCSL_FLAG_NUMERICAL | SUCSL_FLAG_VARIANT) ==
          SUCSL_CATEGORY_NUMERICAL);
    CHECK(sutr_sucsl_category_of(SUCSL_FLAG_SYLLABIC | SUCSL_FLAG_COMPOSITE) ==
          SUCSL_CATEGORY_SYLLABIC);
    CHECK(sutr_sucsl_category_of(SUCSL_FLAG_LOGOGRAM | SUCSL_FLAG_COMPOSITE) ==
          SUCSL_CATEGORY_LOGOGRAPHIC);
    CHECK(sutr_sucsl_category_of(SUCSL_FLAG_COMPOSITE) ==
          SUCSL_CATEGORY_COMPOSITE);
    CHECK(sutr_sucsl_category_of(0u) == SUCSL_CATEGORY_UNKNOWN);

    CHECK(sutr_sucsl_sign_category(0x12409u) == SUCSL_CATEGORY_NUMERICAL);
    CHECK(sutr_sucsl_sign_category(0x12120u) == SUCSL_CATEGORY_SYLLABIC);
    CHECK(sutr_sucsl_sign_category(0x12376u) == SUCSL_CATEGORY_COMPOSITE);
    CHECK(sutr_sucsl_sign_category(0x12001u) == SUCSL_CATEGORY_UNKNOWN);
    CHECK(sutr_sucsl_sign_category(0x41u) == SUCSL_CATEGORY_UNKNOWN);

    CHECK(streq(sutr_sucsl_category_name(SUCSL_CATEGORY_NUMERICAL),
                "Numerical"));
    CHECK(streq(sutr_sucsl_category_name(SUCSL_CATEGORY_SYLLABIC),
                "Syllabic"));
    CHECK(streq(sutr_sucsl_category_name(SUCSL_CATEGORY_LOGOGRAPHIC),
                "Logographic"));
    CHECK(streq(sutr_sucsl_category_name(SUCSL_CATEGORY_COMPOSITE),
                "Composite"));
    CHECK(streq(sutr_sucsl_category_name(SUCSL_CATEGORY_UNKNOWN), "Unknown"));
}

/* ── Period tags (§4.5) ─────────────────────────────────────────── */

static void test_periods(void)
{
    CHECK(streq(sutr_sucsl_period_name(SUCSL_PERIOD_UNKNOWN), "Unknown"));
    CHECK(streq(sutr_sucsl_period_name(SUCSL_PERIOD_EARLY_DYNASTIC),
                "Early Dynastic"));
    CHECK(streq(sutr_sucsl_period_name(SUCSL_PERIOD_UR_III), "Ur III"));
    CHECK(streq(sutr_sucsl_period_name(SUCSL_PERIOD_OLD_BABYLONIAN),
                "Old Babylonian"));
    CHECK(streq(sutr_sucsl_period_name(SUCSL_PERIOD_OLD_ASSYRIAN),
                "Old Assyrian"));
    CHECK(streq(sutr_sucsl_period_name(SUCSL_PERIOD_MIDDLE_ASSYRIAN),
                "Middle Assyrian"));
    CHECK(streq(sutr_sucsl_period_name(SUCSL_PERIOD_NEO_ASSYRIAN),
                "Neo-Assyrian"));
    CHECK(streq(sutr_sucsl_period_name(SUCSL_PERIOD_HELLENISTIC),
                "Hellenistic"));
    CHECK(streq(sutr_sucsl_period_name(SUCSL_PERIOD_ALL), "All periods"));
    CHECK(streq(sutr_sucsl_period_name(9u), "Unknown"));
    CHECK(streq(sutr_sucsl_period_name(0xFFu), "Unknown"));

    CHECK(sutr_sucsl_historical_period(0x12120u) == SUCSL_PERIOD_ALL);
    CHECK(sutr_sucsl_historical_period(0x122B9u) ==
          SUCSL_PERIOD_EARLY_DYNASTIC);
    CHECK(sutr_sucsl_historical_period(0x12399u) ==
          SUCSL_PERIOD_EARLY_DYNASTIC);
    CHECK(sutr_sucsl_historical_period(0x12409u) == SUCSL_PERIOD_UR_III);
    CHECK(sutr_sucsl_historical_period(0x12397u) ==
          SUCSL_PERIOD_OLD_ASSYRIAN);
    CHECK(sutr_sucsl_historical_period(0x1238Cu) ==
          SUCSL_PERIOD_NEO_ASSYRIAN);
    CHECK(sutr_sucsl_historical_period(0x12001u) == SUCSL_PERIOD_UNKNOWN);
    CHECK(sutr_sucsl_historical_period(0x41u) == SUCSL_PERIOD_UNKNOWN);
}

/* ── Atomic compound composition (§4.6) ─────────────────────────── */

static void test_composition(void)
{
    sucsl_compound_t c;

    CHECK(sutr_sucsl_compound_count() == 5u);

    /* GUD×KUR = AM, the bull-of-the-mountain compound. */
    CHECK(sutr_sucsl_get_composition(0x12120u, &c));
    CHECK(c.sign == 0x12120u);
    CHECK(c.name_hash == sutr_sucsl_hash_value("gudxkur"));
    CHECK(c.component_a == 0x1211Eu);
    CHECK(c.component_b == 0x121B3u);
    CHECK(c.component_c == 0u);
    CHECK(c.component_count == 2u);
    CHECK(c.compound_flags == 0u);
    CHECK(c.reserved == 0u);

    CHECK(sutr_sucsl_get_composition(0x1207Fu, &c));
    CHECK(c.component_a == 0x1207Eu);
    CHECK(c.component_b == 0x120A0u);
    CHECK(c.component_count == 2u);
    CHECK((c.compound_flags & SUCSL_COMPOUND_DEPRECATED) != 0u);

    CHECK(sutr_sucsl_get_composition(0x12124u, &c));
    CHECK(c.component_a == 0x12123u);
    CHECK(c.component_b == 0x122BAu);
    CHECK((c.compound_flags & SUCSL_COMPOUND_DEPRECATED) != 0u);

    /* BIL₂ = NE ŠEŠIG: one base component plus a wedge modification. */
    CHECK(sutr_sucsl_get_composition(0x1224Bu, &c));
    CHECK(c.component_a == 0x12248u);
    CHECK(c.component_b == 0u);
    CHECK(c.component_count == 1u);

    CHECK(sutr_sucsl_get_composition(0x12376u, &c));
    CHECK(c.component_a == 0x1207Eu);
    CHECK(c.component_b == 0x122BAu);
    CHECK((c.compound_flags & SUCSL_COMPOUND_DEPRECATED) != 0u);

    /* A base sign, an uncurated sign and RNUR have no composition. */
    CHECK(!sutr_sucsl_get_composition(0x12000u, &c));
    CHECK(!sutr_sucsl_get_composition(0x12001u, &c));
    CHECK(!sutr_sucsl_get_composition(0x160000u, &c));
    CHECK(!sutr_sucsl_get_composition(0x12120u, NULL));

    /* The table is ascending in `sign`, and each row round-trips. */
    {
        uint32_t i;
        uint32_t previous = 0u;

        for (i = 0u; i < sutr_sucsl_compound_count(); i++) {
            sucsl_compound_t row;

            CHECK(sutr_sucsl_compound_at(i, &row));
            CHECK(row.sign > previous);
            previous = row.sign;
            CHECK(sutr_sucsl_is_sign(row.sign));
            CHECK(sutr_sucsl_is_composite(row.sign));
            CHECK(row.component_count >= 1u && row.component_count <= 3u);
            CHECK(row.component_a != 0u);
            CHECK(sutr_sucsl_is_sign(row.component_a));
            if (row.component_count >= 2u) {
                CHECK(row.component_b != 0u);
                CHECK(sutr_sucsl_is_sign(row.component_b));
            }
            if (row.component_count == 3u)
                CHECK(row.component_c != 0u);
            CHECK(sutr_sucsl_get_composition(row.sign, &c));
            CHECK(c.sign == row.sign);
            CHECK(c.name_hash == row.name_hash);
        }
        CHECK(!sutr_sucsl_compound_at(5u, &c));
        CHECK(!sutr_sucsl_compound_at(0u, NULL));
    }
}

/* ── Sequence spelling and mergers/splits (§2.2, §2.3, §4.6) ────── */

static void test_sequences(void)
{
    sucsl_compound_t c;

    CHECK(sutr_sucsl_sequence_count() == 8u);

    CHECK(sutr_sucsl_find_sequence(sutr_sucsl_hash_value("geme2"), &c));
    CHECK(c.sign == 0u);
    CHECK(c.component_a == 0x122A9u); /* SAL */
    CHECK(c.component_b == 0x121B3u); /* KUR */
    CHECK(c.component_count == 2u);
    CHECK((c.compound_flags & SUCSL_COMPOUND_SEQUENCE) != 0u);
    CHECK((c.compound_flags & SUCSL_COMPOUND_LIGATURE) == 0u);

    CHECK(sutr_sucsl_find_sequence(sutr_sucsl_hash_value("diri"), &c));
    CHECK(c.component_a == 0x122DBu); /* SI */
    CHECK(c.component_b == 0x12000u); /* A */

    CHECK(sutr_sucsl_find_sequence(sutr_sucsl_hash_value("idigna"), &c));
    CHECK(c.component_a == 0x12118u); /* GU₂ */
    CHECK(c.component_b == 0x120FCu); /* GAR₃ */

    CHECK(sutr_sucsl_find_sequence(sutr_sucsl_hash_value("dalla"), &c));
    CHECK(c.component_a == 0x12226u); /* MAŠ */
    CHECK(c.component_b == 0x12118u); /* GU₂ */
    CHECK(c.component_c == 0x120FCu); /* GAR₃ */
    CHECK(c.component_count == 3u);

    /* U₅ is the discretionary ḪU.SI ligature; Aššur is AŠ+ŠUR. */
    CHECK(sutr_sucsl_find_sequence(sutr_sucsl_hash_value("u5"), &c));
    CHECK(c.component_a == 0x12137u); /* ḪU */
    CHECK(c.component_b == 0x122DBu); /* SI */
    CHECK((c.compound_flags & SUCSL_COMPOUND_LIGATURE) != 0u);

    CHECK(sutr_sucsl_find_sequence(sutr_sucsl_hash_value("ash-shur"), &c));
    CHECK(c.component_a == 0x12038u); /* AŠ */
    CHECK(c.component_b == 0x122E9u); /* ŠUR */
    CHECK((c.compound_flags & SUCSL_COMPOUND_LIGATURE) != 0u);

    CHECK(sutr_sucsl_find_sequence(sutr_sucsl_hash_value("siki-lam"), &c));
    CHECK(c.component_a == 0x122E0u); /* SIK₂ */
    CHECK(c.component_b == 0x121F4u); /* LAM */

    /* MEŠ is atomic and is not the sequence me-eš (§2.3.1). */
    CHECK(sutr_sucsl_is_sign(0x1238Cu));
    CHECK(!sutr_sucsl_find_sequence(sutr_sucsl_hash_value("mesh"), &c));
    CHECK(sutr_sucsl_find_sequence(sutr_sucsl_hash_value("me-esh"), &c));
    CHECK(c.component_a == 0x12228u); /* ME */
    CHECK(c.component_b == 0x120A0u); /* EŠ₂ */

    CHECK(!sutr_sucsl_find_sequence(0u, &c));
    CHECK(!sutr_sucsl_find_sequence(sutr_sucsl_hash_value("nope"), &c));
    CHECK(!sutr_sucsl_find_sequence(sutr_sucsl_hash_value("geme2"), NULL));

    /* The table is ascending in `name_hash`, and each row round-trips. */
    {
        uint32_t i;
        uint32_t previous = 0u;

        for (i = 0u; i < sutr_sucsl_sequence_count(); i++) {
            sucsl_compound_t row;

            CHECK(sutr_sucsl_sequence_at(i, &row));
            CHECK(row.sign == 0u);
            CHECK(row.name_hash > previous);
            previous = row.name_hash;
            CHECK((row.compound_flags & SUCSL_COMPOUND_SEQUENCE) != 0u);
            CHECK(row.component_count >= 2u && row.component_count <= 3u);
            CHECK(row.component_a != 0u && row.component_b != 0u);
            CHECK(sutr_sucsl_is_sign(row.component_a));
            CHECK(sutr_sucsl_is_sign(row.component_b));
            if (row.component_count == 3u)
                CHECK(sutr_sucsl_is_sign(row.component_c));
            CHECK(sutr_sucsl_find_sequence(row.name_hash, &c));
            CHECK(c.name_hash == row.name_hash);
        }
        CHECK(!sutr_sucsl_sequence_at(8u, &c));
        CHECK(!sutr_sucsl_sequence_at(0u, NULL));
    }
}

/* ── Hierarchy indices and enumeration (§4.2, §4.7) ─────────────── */

static void test_enumeration(void)
{
    uint32_t count = sutr_sucsl_sign_count();
    uint32_t i;
    uint32_t previous = 0u;
    uint32_t numerical = 0u, syllabic = 0u, logographic = 0u;
    uint32_t composite = 0u, deprecated = 0u, variant = 0u;

    CHECK(count == 86u);
    CHECK(count <= SUCSL_SIGN_CAPACITY);

    {
        sucsl_sign_entry_t first, last;

        CHECK(sutr_sucsl_sign_at(0u, &first));
        CHECK(first.codepoint == 0x12000u);
        CHECK(sutr_sucsl_sign_at(count - 1u, &last));
        CHECK(last.codepoint == 0x12514u);
        CHECK(sutr_sucsl_sign_index(0x12000u) == 1u);
        CHECK(sutr_sucsl_sign_index(0x12120u) == 21u);
        CHECK(sutr_sucsl_sign_index(0x12514u) == count);
        CHECK(sutr_sucsl_sign_index(0x12001u) == 0u);
        CHECK(sutr_sucsl_sign_index(0x41u) == 0u);
        CHECK(!sutr_sucsl_sign_at(count, &first));
        CHECK(!sutr_sucsl_sign_at(0u, NULL));
    }

    for (i = 0u; i < count; i++) {
        sucsl_sign_entry_t row;
        sucsl_sign_entry_t looked_up;

        CHECK(sutr_sucsl_sign_at(i, &row));
        CHECK(row.codepoint > previous);
        previous = row.codepoint;
        CHECK(sutr_sucsl_is_cuneiform(row.codepoint));
        CHECK(sutr_sucsl_is_sign(row.codepoint));
        CHECK(row.wedge_count > 0u);
        CHECK(row.historical_period <= SUCSL_PERIOD_ALL);
        CHECK(row.primary_value_hash != 0u);
        CHECK((row.sign_flags & (uint16_t)~SUCSL_FLAG_ASSIGNED_MASK) == 0u);
        CHECK(sutr_sucsl_sign_index(row.codepoint) == i + 1u);
        CHECK(sutr_sucsl_get_sign_entry(row.codepoint, &looked_up));
        CHECK(looked_up.codepoint == row.codepoint);
        CHECK(looked_up.primary_value_hash == row.primary_value_hash);
        CHECK(looked_up.sign_flags == row.sign_flags);
        CHECK(looked_up.wedge_count == row.wedge_count);
        CHECK(looked_up.historical_period == row.historical_period);

        if ((row.sign_flags & SUCSL_FLAG_NUMERICAL) != 0u)
            numerical++;
        if ((row.sign_flags & SUCSL_FLAG_SYLLABIC) != 0u)
            syllabic++;
        if ((row.sign_flags & SUCSL_FLAG_LOGOGRAM) != 0u)
            logographic++;
        if ((row.sign_flags & SUCSL_FLAG_COMPOSITE) != 0u)
            composite++;
        if ((row.sign_flags & SUCSL_FLAG_DEPRECATED) != 0u)
            deprecated++;
        if ((row.sign_flags & SUCSL_FLAG_VARIANT) != 0u)
            variant++;
    }

    /* Every functional class and the two derived classes are populated. */
    CHECK(numerical >= 30u);
    CHECK(syllabic >= 40u);
    CHECK(logographic >= 40u);
    CHECK(composite >= 8u);
    CHECK(deprecated == 3u);
    CHECK(variant == 10u);
}

/* ── Initialization is idempotent (§4.7) ────────────────────────── */

static void test_init_idempotent(void)
{
    sucsl_sign_entry_t entry;
    uint32_t count = sutr_sucsl_sign_count();

    sutr_sucsl_init_registry();
    sutr_sucsl_init_registry();
    sutr_sucsl_init_registry();

    CHECK(sutr_sucsl_sign_count() == count);
    CHECK(sutr_sucsl_get_sign_entry(0x12120u, &entry));
    CHECK(entry.codepoint == 0x12120u);
    CHECK(sutr_sucsl_is_sign(0x12000u));
    CHECK(sutr_sucsl_sign_index(0x12514u) == count);
}

/* ── Taxonomy names ─────────────────────────────────────────────── */

static void test_taxonomy(void)
{
    CHECK(streq(sutr_sucsl_status_code_name(SUCSL_OK), "SUCSL_OK"));
    CHECK(streq(sutr_sucsl_status_code_name(SUCSL_ERR_NULL_ARGUMENT),
                "SUCSL_ERR_NULL_ARGUMENT"));
    CHECK(streq(sutr_sucsl_status_code_name(SUCSL_ERR_NOT_CUNEIFORM),
                "SUCSL_ERR_NOT_CUNEIFORM"));
    CHECK(streq(sutr_sucsl_status_code_name(SUCSL_ERR_ILLEGAL_ZONE),
                "SUCSL_ERR_ILLEGAL_ZONE"));
    CHECK(streq(sutr_sucsl_status_code_name((sucsl_status_code_t)99),
                "SUCSL_ERR_UNKNOWN"));

    CHECK(streq(sutr_sucsl_version_string(), "1.0.0"));
}

/* ── Registration in SSRE (SUAS-010) ────────────────────────────── */

static void test_ssre_registration(void)
{
    uint8_t major = 0u;
    uint8_t minor = 0u;

    CHECK(suas_ssre_is_registered(SUCSL_SPEC_ID));
    CHECK(suas_ssre_version_of(SUCSL_SPEC_ID, &major, &minor));
    CHECK(major == 1u && minor == 0u);
    CHECK(suas_ssre_satisfies(SUCSL_SPEC_ID, 1u, 0u));
    CHECK(!suas_ssre_satisfies(SUCSL_SPEC_ID, 1u, 1u));
    CHECK(!suas_ssre_satisfies(SUCSL_SPEC_ID, 2u, 0u));

    /* SUCSL is a SUTR reference report that ships a real reference unit, so
     * its SSRE row names one. */
    CHECK(suas_ssre_spec_count() == 37u);
    CHECK(suas_ssre_kind_count(SUAS_SSRE_KIND_REFERENCE) == 13u);
    CHECK(suas_ssre_module_of(SUCSL_SPEC_ID) != NULL);
    CHECK(streq(suas_ssre_module_of(SUCSL_SPEC_ID), SUCSL_MODULE));
    CHECK(suas_ssre_spec_of_module(SUCSL_MODULE) != NULL);
    CHECK(streq(suas_ssre_spec_of_module(SUCSL_MODULE), SUCSL_SPEC_ID));
}

int main(void)
{
    test_version_and_meta();
    test_zones();
    test_neography_set();
    test_hash();
    test_get_entry();
    test_predicates();
    test_periods();
    test_composition();
    test_sequences();
    test_enumeration();
    test_init_idempotent();
    test_taxonomy();
    test_ssre_registration();

    if (g_fail) {
        printf("FAIL: SUTR-012 SUCSL tests failed (%d)\n", g_fail);
        return 1;
    }
    printf("[PASS] SUTR-012 SUCSL tests passed\n");
    return 0;
}
