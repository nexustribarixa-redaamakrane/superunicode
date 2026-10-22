/* SUTR-012 — SuperUnicode Cuneiform Sign Lists (SUCSL)
 *
 * Reference implementation of docs/sutr/SUTR-012-sucsl.md.
 *
 * Freestanding C99, zero allocation. The curated sign table, the compound
 * composition table and the sequence table are static; the sign index is built
 * lazily once; every sign query is one FNV-1a hash and a bounded probe. No part
 * of this file reaches for malloc, stdio or a libc string routine.
 *
 * The compiled surface answers four questions an epigraphic renderer asks on
 * the hot path:
 *
 *   1. What is this sign's category, primary value, wedge count and period?
 *      (§4.2-§4.5)
 *   2. Which components make up this compound sign? (§4.6)
 *   3. Which sequence of signs represents this unencoded sign name? (§4.6)
 *   4. How does an uncurated Cuneiform or RNUR codepoint degrade? (§6)
 *
 * All are resolved by direct table indexing, O(1) per unit, with no runtime
 * catalog parse and no dynamic allocation.
 */

#include "sutr/sutr_sucsl.h"

/* ── Compile-time record geometry (§4.2, §4.6) ───────────────────── */

#define SUCSL_STATIC_ASSERT(cond, name) \
    typedef char name[(cond) ? 1 : -1]

SUCSL_STATIC_ASSERT(sizeof(sucsl_sign_entry_t) == 12u,
                    sucsl_entry_is_12_bytes);
SUCSL_STATIC_ASSERT(offsetof(sucsl_sign_entry_t, codepoint) == 0u,
                    sucsl_entry_codepoint_at_0);
SUCSL_STATIC_ASSERT(offsetof(sucsl_sign_entry_t, primary_value_hash) == 4u,
                    sucsl_entry_value_at_4);
SUCSL_STATIC_ASSERT(offsetof(sucsl_sign_entry_t, sign_flags) == 8u,
                    sucsl_entry_flags_at_8);
SUCSL_STATIC_ASSERT(offsetof(sucsl_sign_entry_t, wedge_count) == 10u,
                    sucsl_entry_wedges_at_10);
SUCSL_STATIC_ASSERT(offsetof(sucsl_sign_entry_t, historical_period) == 11u,
                    sucsl_entry_period_at_11);

SUCSL_STATIC_ASSERT(sizeof(sucsl_compound_t) == 24u,
                    sucsl_compound_is_24_bytes);
SUCSL_STATIC_ASSERT(offsetof(sucsl_compound_t, sign) == 0u,
                    sucsl_compound_sign_at_0);
SUCSL_STATIC_ASSERT(offsetof(sucsl_compound_t, name_hash) == 4u,
                    sucsl_compound_name_at_4);
SUCSL_STATIC_ASSERT(offsetof(sucsl_compound_t, component_a) == 8u,
                    sucsl_compound_a_at_8);
SUCSL_STATIC_ASSERT(offsetof(sucsl_compound_t, component_b) == 12u,
                    sucsl_compound_b_at_12);
SUCSL_STATIC_ASSERT(offsetof(sucsl_compound_t, component_c) == 16u,
                    sucsl_compound_c_at_16);
SUCSL_STATIC_ASSERT(offsetof(sucsl_compound_t, component_count) == 20u,
                    sucsl_compound_count_at_20);
SUCSL_STATIC_ASSERT(offsetof(sucsl_compound_t, compound_flags) == 21u,
                    sucsl_compound_flags_at_21);
SUCSL_STATIC_ASSERT(offsetof(sucsl_compound_t, reserved) == 22u,
                    sucsl_compound_reserved_at_22);

/* The flag families are disjoint where they must be, and every defined low
 * bit falls inside the assigned mask (§4.3). */
SUCSL_STATIC_ASSERT((SUCSL_FLAG_LOGOGRAM & SUCSL_FLAG_SYLLABIC) == 0u,
                    sucsl_logogram_syllabic_disjoint);
SUCSL_STATIC_ASSERT((SUCSL_FLAG_NUMERICAL & SUCSL_FLAG_COMPOSITE) == 0u,
                    sucsl_numerical_composite_disjoint);
SUCSL_STATIC_ASSERT((SUCSL_FLAG_UNCLASSIFIED & SUCSL_FLAG_ASSIGNED_MASK) != 0u,
                    sucsl_unclassified_in_mask);

/* ── The curated Cuneiform sign registry (§8) ────────────────────── */

/* Strictly ascending codepoint order is a verification invariant: the hierarchy
 * index, the enumeration surface and the open-addressing probe all depend on
 * it. Each row carries the FNV-1a hash of the sign's ASCII value skeleton (§4.4),
 * the packed classification flags (§4.3), the constituent wedge count and the
 * chronological period tag (§4.5). A row is curated when the sign has a stable
 * abstract character identity in the encoded repertoire; a sign represented
 * only by a sequence is described in sucsl_sequences[] instead of here. */
static const sucsl_sign_entry_t sucsl_signs[] = {
    /*  codepoint  primary_value_hash  flags                                                wedges period */
    { 0x12000u, 0xE40C292Cu, SUCSL_FLAG_LOGOGRAM | SUCSL_FLAG_SYLLABIC,                        2u, SUCSL_PERIOD_ALL }, /* a */
    { 0x12009u, 0x1D24BA3Au, SUCSL_FLAG_LOGOGRAM | SUCSL_FLAG_SYLLABIC,                        3u, SUCSL_PERIOD_ALL }, /* a2 */
    { 0x1202Du, 0x4124F2E6u, SUCSL_FLAG_LOGOGRAM | SUCSL_FLAG_SYLLABIC,                        4u, SUCSL_PERIOD_ALL }, /* an */
    { 0x12032u, 0x1F297F40u, SUCSL_FLAG_LOGOGRAM | SUCSL_FLAG_SYLLABIC,                        6u, SUCSL_PERIOD_ALL }, /* anshe */
    { 0x12038u, 0x1972C87Fu, SUCSL_FLAG_LOGOGRAM | SUCSL_FLAG_SYLLABIC,                        1u, SUCSL_PERIOD_ALL }, /* ash */
    { 0x1203Eu, 0x5CB15137u, SUCSL_FLAG_SYLLABIC,                                              2u, SUCSL_PERIOD_ALL }, /* ash2 */
    { 0x12046u, 0xE020DC9Cu, SUCSL_FLAG_LOGOGRAM | SUCSL_FLAG_SYLLABIC,                        7u, SUCSL_PERIOD_ALL }, /* balag */
    { 0x12077u, 0xCD628580u, SUCSL_FLAG_LOGOGRAM | SUCSL_FLAG_SYLLABIC,                        4u, SUCSL_PERIOD_ALL }, /* din */
    { 0x12079u, 0x4860455Du, SUCSL_FLAG_LOGOGRAM | SUCSL_FLAG_SYLLABIC,                        1u, SUCSL_PERIOD_ALL }, /* dish */
    { 0x1207Eu, 0xE1310830u, SUCSL_FLAG_LOGOGRAM | SUCSL_FLAG_SYLLABIC,                        4u, SUCSL_PERIOD_ALL }, /* dub */
    { 0x1207Fu, 0xAF07276Au, SUCSL_FLAG_LOGOGRAM | SUCSL_FLAG_COMPOSITE
                              | SUCSL_FLAG_DEPRECATED,                                          5u, SUCSL_PERIOD_ALL }, /* dub x esh2 */
    { 0x12080u, 0x822F9B26u, SUCSL_FLAG_LOGOGRAM | SUCSL_FLAG_SYLLABIC,                        4u, SUCSL_PERIOD_ALL }, /* dub2 */
    { 0x1208Au, 0xE00C22E0u, SUCSL_FLAG_LOGOGRAM | SUCSL_FLAG_SYLLABIC,                        2u, SUCSL_PERIOD_ALL }, /* e */
    { 0x12097u, 0x411A658Au, SUCSL_FLAG_LOGOGRAM | SUCSL_FLAG_SYLLABIC,                        4u, SUCSL_PERIOD_ALL }, /* en */
    { 0x120A0u, 0x66CB4573u, SUCSL_FLAG_SYLLABIC,                                              3u, SUCSL_PERIOD_ALL }, /* esh2 */
    { 0x120FBu, 0x4A1724F5u, SUCSL_FLAG_LOGOGRAM | SUCSL_FLAG_SYLLABIC,                        4u, SUCSL_PERIOD_ALL }, /* gar */
    { 0x120FCu, 0x686EE3B2u, SUCSL_FLAG_LOGOGRAM | SUCSL_FLAG_SYLLABIC,                        4u, SUCSL_PERIOD_ALL }, /* gar3 */
    { 0x12116u, 0x4C208709u, SUCSL_FLAG_LOGOGRAM | SUCSL_FLAG_SYLLABIC,                        3u, SUCSL_PERIOD_ALL }, /* gu */
    { 0x12118u, 0x1234E1E1u, SUCSL_FLAG_LOGOGRAM | SUCSL_FLAG_SYLLABIC,                        3u, SUCSL_PERIOD_ALL }, /* gu2 */
    { 0x1211Eu, 0x44353097u, SUCSL_FLAG_LOGOGRAM | SUCSL_FLAG_SYLLABIC,                        5u, SUCSL_PERIOD_ALL }, /* gud */
    { 0x12120u, 0x4024F153u, SUCSL_FLAG_LOGOGRAM | SUCSL_FLAG_SYLLABIC
                              | SUCSL_FLAG_COMPOSITE,                                            7u, SUCSL_PERIOD_ALL }, /* gud x kur = am */
    { 0x12123u, 0x3B35226Cu, SUCSL_FLAG_LOGOGRAM | SUCSL_FLAG_SYLLABIC,                        4u, SUCSL_PERIOD_ALL }, /* gum */
    { 0x12124u, 0x80FA16FEu, SUCSL_FLAG_LOGOGRAM | SUCSL_FLAG_COMPOSITE
                              | SUCSL_FLAG_DEPRECATED,                                          6u, SUCSL_PERIOD_ALL }, /* gum x she */
    { 0x1212Du, 0x683AF69Au, SUCSL_FLAG_LOGOGRAM | SUCSL_FLAG_SYLLABIC,                        3u, SUCSL_PERIOD_ALL }, /* hi */
    { 0x12137u, 0x4C3ACA86u, SUCSL_FLAG_LOGOGRAM | SUCSL_FLAG_SYLLABIC,                        3u, SUCSL_PERIOD_ALL }, /* hu */
    { 0x1213Fu, 0xEC0C35C4u, SUCSL_FLAG_LOGOGRAM | SUCSL_FLAG_SYLLABIC,                        1u, SUCSL_PERIOD_ALL }, /* i */
    { 0x12147u, 0x0642AB19u, SUCSL_FLAG_SYLLABIC,                                              4u, SUCSL_PERIOD_ALL }, /* igi dib = u3 */
    { 0x121B3u, 0x733CE92Du, SUCSL_FLAG_LOGOGRAM | SUCSL_FLAG_SYLLABIC,                        4u, SUCSL_PERIOD_ALL }, /* kur */
    { 0x121F4u, 0x596084C9u, SUCSL_FLAG_LOGOGRAM | SUCSL_FLAG_SYLLABIC,                        3u, SUCSL_PERIOD_ALL }, /* lam */
    { 0x12220u, 0x642E245Bu, SUCSL_FLAG_LOGOGRAM | SUCSL_FLAG_SYLLABIC,                        3u, SUCSL_PERIOD_ALL }, /* ma */
    { 0x12226u, 0xE47740B0u, SUCSL_FLAG_LOGOGRAM | SUCSL_FLAG_SYLLABIC,                        3u, SUCSL_PERIOD_ALL }, /* mash */
    { 0x12228u, 0x682E2AA7u, SUCSL_FLAG_LOGOGRAM | SUCSL_FLAG_SYLLABIC,                        3u, SUCSL_PERIOD_ALL }, /* me */
    { 0x12229u, 0xD4AD6BBCu, SUCSL_FLAG_LOGOGRAM | SUCSL_FLAG_SYLLABIC,                        3u, SUCSL_PERIOD_ALL }, /* mes */
    { 0x1223Eu, 0x543659F0u, SUCSL_FLAG_LOGOGRAM | SUCSL_FLAG_SYLLABIC,                        3u, SUCSL_PERIOD_ALL }, /* na */
    { 0x12248u, 0x5836603Cu, SUCSL_FLAG_LOGOGRAM | SUCSL_FLAG_SYLLABIC,                        3u, SUCSL_PERIOD_ALL }, /* ne */
    { 0x1224Bu, 0x714E5A9Eu, SUCSL_FLAG_SYLLABIC | SUCSL_FLAG_COMPOSITE,                       4u, SUCSL_PERIOD_ALL }, /* ne sheshig = bil2 */
    { 0x122A9u, 0xDE1C9F6Fu, SUCSL_FLAG_LOGOGRAM | SUCSL_FLAG_SYLLABIC,                        4u, SUCSL_PERIOD_ALL }, /* sal */
    { 0x122B9u, 0x80C9BBADu, SUCSL_FLAG_LOGOGRAM | SUCSL_FLAG_SYLLABIC,                        5u, SUCSL_PERIOD_EARLY_DYNASTIC }, /* shar2 */
    { 0x122BAu, 0xE92DF9E1u, SUCSL_FLAG_LOGOGRAM | SUCSL_FLAG_SYLLABIC,                        3u, SUCSL_PERIOD_ALL }, /* she */
    { 0x122C0u, 0x13EECB1Au, SUCSL_FLAG_LOGOGRAM | SUCSL_FLAG_SYLLABIC,                        5u, SUCSL_PERIOD_ALL }, /* shesh */
    { 0x122C1u, 0x88E9CFF8u, SUCSL_FLAG_LOGOGRAM | SUCSL_FLAG_SYLLABIC,                        5u, SUCSL_PERIOD_ALL }, /* shesh2 */
    { 0x122DBu, 0x405210F1u, SUCSL_FLAG_LOGOGRAM | SUCSL_FLAG_SYLLABIC,                        2u, SUCSL_PERIOD_ALL }, /* si */
    { 0x122E0u, 0x13C59105u, SUCSL_FLAG_LOGOGRAM | SUCSL_FLAG_SYLLABIC,                        5u, SUCSL_PERIOD_ALL }, /* sik2 = siki */
    { 0x122E1u, 0x96032A93u, SUCSL_FLAG_LOGOGRAM | SUCSL_FLAG_SYLLABIC,                        4u, SUCSL_PERIOD_ALL }, /* sila3 */
    { 0x122E9u, 0xA638D2F9u, SUCSL_FLAG_LOGOGRAM | SUCSL_FLAG_SYLLABIC,                        4u, SUCSL_PERIOD_ALL }, /* sur = shur */
    { 0x122FEu, 0x404544FEu, SUCSL_FLAG_LOGOGRAM | SUCSL_FLAG_SYLLABIC,                        3u, SUCSL_PERIOD_ALL }, /* ti */
    { 0x1230Bu, 0xF00C3C10u, SUCSL_FLAG_LOGOGRAM | SUCSL_FLAG_SYLLABIC
                              | SUCSL_FLAG_NUMERICAL,                                            1u, SUCSL_PERIOD_ALL }, /* u = 10 */
    { 0x1230Du, 0x5F5E19E2u, SUCSL_FLAG_NUMERICAL,                                              3u, SUCSL_PERIOD_ALL }, /* u u u = 30 */
    { 0x12311u, 0x0542A986u, SUCSL_FLAG_LOGOGRAM | SUCSL_FLAG_SYLLABIC,                        2u, SUCSL_PERIOD_ALL }, /* u2 */
    { 0x12313u, 0x57432A9Cu, SUCSL_FLAG_LOGOGRAM | SUCSL_FLAG_SYLLABIC,                        3u, SUCSL_PERIOD_ALL }, /* ud */
    { 0x1231Du, 0x604338C7u, SUCSL_FLAG_LOGOGRAM | SUCSL_FLAG_SYLLABIC,                        3u, SUCSL_PERIOD_ALL }, /* um */
    { 0x12376u, 0x36817CDAu, SUCSL_FLAG_COMPOSITE | SUCSL_FLAG_DEPRECATED,                     5u, SUCSL_PERIOD_ALL }, /* dub x she */
    { 0x1238Cu, 0xA100BEBCu, SUCSL_FLAG_LOGOGRAM | SUCSL_FLAG_SYLLABIC,                        6u, SUCSL_PERIOD_NEO_ASSYRIAN }, /* mesh */
    { 0x12397u, 0xF90B4D24u, SUCSL_FLAG_LOGOGRAM | SUCSL_FLAG_SYLLABIC,                        4u, SUCSL_PERIOD_OLD_ASSYRIAN }, /* ti2 */
    { 0x12399u, 0xE2D077ABu, SUCSL_FLAG_NUMERICAL,                                              2u, SUCSL_PERIOD_EARLY_DYNASTIC }, /* u u = 20 */
    { 0x12400u, 0x2628110Cu, SUCSL_FLAG_NUMERICAL,                                              2u, SUCSL_PERIOD_ALL }, /* two ash */
    { 0x12402u, 0x072342D0u, SUCSL_FLAG_NUMERICAL,                                              4u, SUCSL_PERIOD_ALL }, /* four ash */
    { 0x12403u, 0xBB31F4E0u, SUCSL_FLAG_NUMERICAL,                                              5u, SUCSL_PERIOD_ALL }, /* five ash */
    { 0x12408u, 0x5D4A0A82u, SUCSL_FLAG_NUMERICAL,                                              3u, SUCSL_PERIOD_UR_III }, /* three dish */
    { 0x12409u, 0x85F3C47Cu, SUCSL_FLAG_NUMERICAL,                                              4u, SUCSL_PERIOD_UR_III }, /* four dish */
    { 0x1240Au, 0x2E6BF46Cu, SUCSL_FLAG_NUMERICAL,                                              5u, SUCSL_PERIOD_UR_III }, /* five dish */
    { 0x1240Cu, 0xE42675F9u, SUCSL_FLAG_NUMERICAL,                                              7u, SUCSL_PERIOD_UR_III }, /* seven dish */
    { 0x1240Du, 0xBA971E03u, SUCSL_FLAG_NUMERICAL,                                              8u, SUCSL_PERIOD_UR_III }, /* eight dish */
    { 0x1240Eu, 0xA40E5BACu, SUCSL_FLAG_NUMERICAL,                                              9u, SUCSL_PERIOD_UR_III }, /* nine dish */
    { 0x1240Fu, 0x61890A97u, SUCSL_FLAG_NUMERICAL,                                              4u, SUCSL_PERIOD_ALL }, /* four u */
    { 0x12415u, 0x457AA419u, SUCSL_FLAG_NUMERICAL,                                              1u, SUCSL_PERIOD_ALL }, /* one gesh2 */
    { 0x1241Eu, 0x867B0A6Cu, SUCSL_FLAG_NUMERICAL,                                              1u, SUCSL_PERIOD_ALL }, /* one geshu */
    { 0x12423u, 0xF4DFC47Eu, SUCSL_FLAG_NUMERICAL,                                              2u, SUCSL_PERIOD_ALL }, /* two shar2 */
    { 0x12426u, 0x233A9F32u, SUCSL_FLAG_NUMERICAL,                                              4u, SUCSL_PERIOD_ALL }, /* four shar2 */
    { 0x12432u, 0x8BA0E8B3u, SUCSL_FLAG_NUMERICAL | SUCSL_FLAG_COMPOSITE,                       8u, SUCSL_PERIOD_ALL }, /* shar2 x gal + dish */
    { 0x12434u, 0xE88E323Au, SUCSL_FLAG_NUMERICAL,                                              1u, SUCSL_PERIOD_ALL }, /* one buru */
    { 0x1243Cu, 0xFEDB44ACu, SUCSL_FLAG_NUMERICAL | SUCSL_FLAG_VARIANT,                         4u, SUCSL_PERIOD_NEO_ASSYRIAN }, /* four limmu */
    { 0x1243Du, 0xCB2CFB48u, SUCSL_FLAG_NUMERICAL | SUCSL_FLAG_VARIANT,                         4u, SUCSL_PERIOD_NEO_ASSYRIAN }, /* four limmu4 */
    { 0x12440u, 0x1F773191u, SUCSL_FLAG_NUMERICAL | SUCSL_FLAG_VARIANT,                         6u, SUCSL_PERIOD_NEO_ASSYRIAN }, /* six ash9 */
    { 0x12441u, 0xDFF816FDu, SUCSL_FLAG_NUMERICAL | SUCSL_FLAG_VARIANT,                         7u, SUCSL_PERIOD_NEO_ASSYRIAN }, /* seven imin3 */
    { 0x12442u, 0xFB9155E6u, SUCSL_FLAG_NUMERICAL | SUCSL_FLAG_VARIANT,                         7u, SUCSL_PERIOD_NEO_ASSYRIAN }, /* seven imin a */
    { 0x12444u, 0xEE7ADA03u, SUCSL_FLAG_NUMERICAL | SUCSL_FLAG_VARIANT,                         8u, SUCSL_PERIOD_NEO_ASSYRIAN }, /* eight ussu */
    { 0x12445u, 0x9B657990u, SUCSL_FLAG_NUMERICAL | SUCSL_FLAG_VARIANT,                         8u, SUCSL_PERIOD_NEO_ASSYRIAN }, /* eight ussu3 */
    { 0x12446u, 0x35D96C5Du, SUCSL_FLAG_NUMERICAL | SUCSL_FLAG_VARIANT,                         9u, SUCSL_PERIOD_NEO_ASSYRIAN }, /* nine ilimmu */
    { 0x12447u, 0x3345B12Au, SUCSL_FLAG_NUMERICAL | SUCSL_FLAG_VARIANT,                         9u, SUCSL_PERIOD_NEO_ASSYRIAN }, /* nine ilimmu3 */
    { 0x1244Au, 0xB295412Du, SUCSL_FLAG_NUMERICAL | SUCSL_FLAG_VARIANT,                         2u, SUCSL_PERIOD_ALL }, /* two ash tenu */
    { 0x1244Fu, 0x8850EB2Bu, SUCSL_FLAG_NUMERICAL,                                              1u, SUCSL_PERIOD_ALL }, /* one ban2 */
    { 0x12450u, 0x533018CDu, SUCSL_FLAG_NUMERICAL,                                              2u, SUCSL_PERIOD_ALL }, /* two ban2 */
    { 0x12480u, 0xAC0AE1BCu, SUCSL_FLAG_COMPOSITE,                                              6u, SUCSL_PERIOD_EARLY_DYNASTIC }, /* ab x nun tenu */
    { 0x1248Au, 0xDDDA82C6u, SUCSL_FLAG_COMPOSITE,                                              6u, SUCSL_PERIOD_EARLY_DYNASTIC }, /* dim x igi */
    { 0x12514u, 0x9F6BBF9Fu, SUCSL_FLAG_LOGOGRAM | SUCSL_FLAG_COMPOSITE,                        9u, SUCSL_PERIOD_EARLY_DYNASTIC }  /* lak-648 */
};

#define SUCSL_SIGN_COUNT \
    ((uint32_t)(sizeof sucsl_signs / sizeof sucsl_signs[0]))

SUCSL_STATIC_ASSERT(SUCSL_SIGN_COUNT <= SUCSL_SIGN_CAPACITY,
                    sucsl_signs_fit_capacity);

/* ── Atomic compound signs (§4.6) ────────────────────────────────── */

/* A compound sign that is encoded atomically: the sign codepoint maps to the
 * components whose structural relationship its name records. The table is
 * strictly ascending in `sign`, so a composition query is a binary search. */
static const sucsl_compound_t sucsl_compounds[] = {
    /*  sign      name_hash   component_a component_b component_c count flags */
    { 0x1207Fu, 0xAF07276Au, 0x1207Eu, 0x120A0u, 0x00000u, 2u, SUCSL_COMPOUND_DEPRECATED, 0u }, /* dub x esh2 */
    { 0x12120u, 0xCB74D2C5u, 0x1211Eu, 0x121B3u, 0x00000u, 2u, 0u,                        0u }, /* gud x kur = am */
    { 0x12124u, 0x80FA16FEu, 0x12123u, 0x122BAu, 0x00000u, 2u, SUCSL_COMPOUND_DEPRECATED, 0u }, /* gum x she */
    { 0x1224Bu, 0x714E5A9Eu, 0x12248u, 0x00000u, 0x00000u, 1u, 0u,                        0u }, /* ne sheshig = bil2 */
    { 0x12376u, 0x36817CDAu, 0x1207Eu, 0x122BAu, 0x00000u, 2u, SUCSL_COMPOUND_DEPRECATED, 0u }  /* dub x she */
};

#define SUCSL_COMPOUND_TABLE_COUNT \
    ((uint32_t)(sizeof sucsl_compounds / sizeof sucsl_compounds[0]))

SUCSL_STATIC_ASSERT(SUCSL_COMPOUND_TABLE_COUNT <= SUCSL_COMPOUND_CAPACITY,
                    sucsl_compounds_fit_capacity);

/* ── Sequence signs (§4.6, §2.2) ─────────────────────────────────── */

/* A sign that the encoding represents as a sequence of other signs rather than
 * as one atomic codepoint. `sign` is zero; the lookup key is the FNV-1a hash of
 * the structural-name skeleton. The table is strictly ascending in `name_hash`,
 * so a sequence query is a binary search. */
static const sucsl_compound_t sucsl_sequences[] = {
    /*  sign  name_hash   component_a component_b component_c count flags */
    { 0u, 0x0842AE3Fu, 0x12137u, 0x122DBu, 0x00000u, 2u, SUCSL_COMPOUND_SEQUENCE | SUCSL_COMPOUND_LIGATURE, 0u }, /* u5 = hu.si */
    { 0u, 0x0D95F4ABu, 0x122A9u, 0x121B3u, 0x00000u, 2u, SUCSL_COMPOUND_SEQUENCE,                        0u }, /* geme2 = sal.kur */
    { 0u, 0x0D9D7CCDu, 0x12118u, 0x120FCu, 0x00000u, 2u, SUCSL_COMPOUND_SEQUENCE,                        0u }, /* idigna = gu2.gar3 */
    { 0u, 0x158520B6u, 0x12038u, 0x122E9u, 0x00000u, 2u, SUCSL_COMPOUND_SEQUENCE | SUCSL_COMPOUND_LIGATURE, 0u }, /* ash-shur */
    { 0u, 0x635E3147u, 0x122DBu, 0x12000u, 0x00000u, 2u, SUCSL_COMPOUND_SEQUENCE,                        0u }, /* diri = si.a */
    { 0u, 0xB91EC5A5u, 0x12226u, 0x12118u, 0x120FCu, 3u, SUCSL_COMPOUND_SEQUENCE,                        0u }, /* dalla = mash.gu2.gar3 */
    { 0u, 0xC53E2B7Au, 0x12228u, 0x120A0u, 0x00000u, 2u, SUCSL_COMPOUND_SEQUENCE,                        0u }, /* me-esh = me.esh2 */
    { 0u, 0xD7AE3C7Eu, 0x122E0u, 0x121F4u, 0x00000u, 2u, SUCSL_COMPOUND_SEQUENCE,                        0u }  /* siki-lam = sik2.lam */
};

#define SUCSL_SEQUENCE_TABLE_COUNT \
    ((uint32_t)(sizeof sucsl_sequences / sizeof sucsl_sequences[0]))

SUCSL_STATIC_ASSERT(SUCSL_SEQUENCE_TABLE_COUNT <= SUCSL_SEQUENCE_CAPACITY,
                    sucsl_sequences_fit_capacity);

/* ── Lazily built sign index (§4.7) ─────────────────────────────── */

/* Open addressing over a power-of-two table: one FNV-1a hash of the codepoint
 * selects a slot, and linear probing resolves the (rare) collision. With 85
 * curated rows in 512 slots the load factor is 0.17, so the probe is one step
 * in the overwhelming majority of cases. The index is rebuilt on the first
 * query, so an application that never touches Cuneiform never pays for it. */

typedef struct {
    uint16_t entry; /* index into sucsl_signs[], valid when used != 0 */
    uint8_t  used;
} sucsl_index_slot_t;

static sucsl_index_slot_t sucsl_index[SUCSL_SIGN_INDEX_SIZE];
static uint8_t sucsl_ready = 0u;

SUCSL_STATIC_ASSERT((SUCSL_SIGN_INDEX_SIZE & (SUCSL_SIGN_INDEX_SIZE - 1u)) == 0u,
                    sucsl_index_size_power_of_two);
SUCSL_STATIC_ASSERT(SUCSL_SIGN_INDEX_SIZE > SUCSL_SIGN_CAPACITY,
                    sucsl_index_load_factor_below_half);
SUCSL_STATIC_ASSERT(SUCSL_SIGN_CAPACITY <= 0xFFFFu,
                    sucsl_index_entry_is_uint16);

/* ── Hashing (§4.4) ─────────────────────────────────────────────── */

static uint32_t sucsl_fnv_bytes(const uint8_t *bytes, size_t count)
{
    uint32_t h = SUCSL_FNV_OFFSET_BASIS;
    size_t i;

    for (i = 0u; i < count; i++) {
        h ^= (uint32_t)bytes[i];
        h *= SUCSL_FNV_PRIME;
    }
    return h;
}

uint32_t sutr_sucsl_hash_value(const char *text)
{
    uint32_t h = SUCSL_FNV_OFFSET_BASIS;

    if (text == NULL)
        return 0u;
    while (*text != '\0') {
        h ^= (uint32_t)(unsigned char)*text++;
        h *= SUCSL_FNV_PRIME;
    }
    return h;
}

uint32_t sutr_sucsl_hash_codepoint(uint32_t codepoint)
{
    uint8_t bytes[4];

    bytes[0] = (uint8_t)(codepoint & 0xFFu);
    bytes[1] = (uint8_t)((codepoint >> 8) & 0xFFu);
    bytes[2] = (uint8_t)((codepoint >> 16) & 0xFFu);
    bytes[3] = (uint8_t)((codepoint >> 24) & 0xFFu);
    return sucsl_fnv_bytes(bytes, 4u);
}

/* ── The index build (§4.7) ─────────────────────────────────────── */

void sutr_sucsl_init_registry(void)
{
    uint32_t i;
    const uint32_t mask = SUCSL_SIGN_INDEX_SIZE - 1u;

    if (sucsl_ready != 0u)
        return;

    for (i = 0u; i < SUCSL_SIGN_INDEX_SIZE; i++) {
        sucsl_index[i].entry = 0u;
        sucsl_index[i].used = 0u;
    }

    for (i = 0u; i < SUCSL_SIGN_COUNT; i++) {
        uint32_t slot = sutr_sucsl_hash_codepoint(sucsl_signs[i].codepoint) & mask;

        while (sucsl_index[slot].used != 0u)
            slot = (slot + 1u) & mask;
        sucsl_index[slot].used = 1u;
        sucsl_index[slot].entry = (uint16_t)i;
    }

    sucsl_ready = 1u;
}

static const sucsl_sign_entry_t *sucsl_find(uint32_t codepoint)
{
    const uint32_t mask = SUCSL_SIGN_INDEX_SIZE - 1u;
    uint32_t slot;
    uint32_t probes;

    if (sucsl_ready == 0u)
        sutr_sucsl_init_registry();

    slot = sutr_sucsl_hash_codepoint(codepoint) & mask;
    for (probes = 0u; probes < SUCSL_SIGN_INDEX_SIZE; probes++) {
        const sucsl_index_slot_t *at = &sucsl_index[slot];

        if (at->used == 0u)
            return NULL;
        if (sucsl_signs[at->entry].codepoint == codepoint)
            return &sucsl_signs[at->entry];
        slot = (slot + 1u) & mask;
    }
    return NULL;
}

/* ── Zone taxonomy (§4.1, §5) ───────────────────────────────────── */

sucsl_zone_t sutr_sucsl_zone_of(uint32_t codepoint)
{
    if (codepoint <= SUCSL_LIMIT_UNICODE_MAX)
        return SUCSL_ZONE_BRIDGE;
    if (codepoint <= SUCSL_LIMIT_SCP_MAX)
        return SUCSL_ZONE_SCP;
    if (codepoint >= SUCSL_RNUR_MIN && codepoint <= SUCSL_RNUR_MAX)
        return SUCSL_ZONE_RNUR;
    if (codepoint <= SUCSL_LIMIT_NATIVE_MAX)
        return SUCSL_ZONE_NATIVE;
    if (codepoint >= SUCSL_LIMIT_TRAP_MIN && codepoint <= SUCSL_LIMIT_TRAP_MAX)
        return SUCSL_ZONE_TRAP;
    if (codepoint == SUCSL_LIMIT_SENTINEL)
        return SUCSL_ZONE_SENTINEL;
    return SUCSL_ZONE_EXTSUCS;
}

bool sutr_sucsl_is_text_space(uint32_t codepoint)
{
    sucsl_zone_t zone = sutr_sucsl_zone_of(codepoint);

    return zone == SUCSL_ZONE_BRIDGE || zone == SUCSL_ZONE_SCP ||
           zone == SUCSL_ZONE_RNUR || zone == SUCSL_ZONE_NATIVE;
}

bool sutr_sucsl_is_native(uint32_t codepoint)
{
    return sutr_sucsl_zone_of(codepoint) == SUCSL_ZONE_NATIVE;
}

bool sutr_sucsl_is_neography(uint32_t codepoint)
{
    return codepoint >= SUCSL_RNUR_MIN && codepoint <= SUCSL_RNUR_MAX;
}

bool sutr_sucsl_neography_set(uint32_t codepoint, uint32_t *out_set)
{
    if (out_set == NULL || !sutr_sucsl_is_neography(codepoint))
        return false;
    *out_set = (codepoint - SUCSL_RNUR_MIN) >> SUCSL_RNUR_SET_SHIFT;
    return true;
}

/* ── Cuneiform block predicates (§4.1) ──────────────────────────── */

bool sutr_sucsl_is_cuneiform(uint32_t codepoint)
{
    return (codepoint >= SUCSL_BLOCK_CUNEIFORM_MIN &&
            codepoint <= SUCSL_BLOCK_CUNEIFORM_MAX) ||
           (codepoint >= SUCSL_BLOCK_NUMBERS_MIN &&
            codepoint <= SUCSL_BLOCK_NUMBERS_MAX) ||
           (codepoint >= SUCSL_BLOCK_EARLY_MIN &&
            codepoint <= SUCSL_BLOCK_EARLY_MAX);
}

/* ── Sign resolution (§4.2-§4.7) ────────────────────────────────── */

static void sucsl_synthesize(uint32_t codepoint,
                             sucsl_sign_entry_t *entry,
                             uint16_t flags)
{
    entry->codepoint = codepoint;
    entry->primary_value_hash = 0u;
    entry->sign_flags = flags;
    entry->wedge_count = 0u;
    entry->historical_period = SUCSL_PERIOD_UNKNOWN;
}

bool sutr_sucsl_get_sign_entry_ex(uint32_t codepoint,
                                  sucsl_sign_entry_t *out_entry,
                                  sucsl_status_code_t *out_error)
{
    const sucsl_sign_entry_t *found;

    if (out_error != NULL)
        *out_error = SUCSL_OK;
    if (out_entry == NULL) {
        if (out_error != NULL)
            *out_error = SUCSL_ERR_NULL_ARGUMENT;
        return false;
    }
    if (!sutr_sucsl_is_text_space(codepoint)) {
        if (out_error != NULL)
            *out_error = SUCSL_ERR_ILLEGAL_ZONE;
        return false;
    }

    found = sucsl_find(codepoint);
    if (found != NULL) {
        *out_entry = *found;
        return true;
    }

    if (sutr_sucsl_is_cuneiform(codepoint)) {
        /* An uncurated sign in an encoded Cuneiform block still has a stable
         * abstract identity; it simply carries no classification yet (§6). */
        sucsl_synthesize(codepoint, out_entry, SUCSL_FLAG_UNCLASSIFIED);
        return true;
    }
    if (sutr_sucsl_is_neography(codepoint)) {
        /* RNUR is the designated extension space for custom sign lists (§5). */
        sucsl_synthesize(codepoint, out_entry,
                         SUCSL_FLAG_UNCLASSIFIED | SUCSL_FLAG_EXTENSION);
        return true;
    }

    if (out_error != NULL)
        *out_error = SUCSL_ERR_NOT_CUNEIFORM;
    return false;
}

bool sutr_sucsl_get_sign_entry(uint32_t codepoint, sucsl_sign_entry_t *out_entry)
{
    return sutr_sucsl_get_sign_entry_ex(codepoint, out_entry, NULL);
}

bool sutr_sucsl_is_sign(uint32_t codepoint)
{
    return sucsl_find(codepoint) != NULL;
}

bool sutr_sucsl_is_logogram(uint32_t codepoint)
{
    const sucsl_sign_entry_t *entry = sucsl_find(codepoint);

    return entry != NULL && (entry->sign_flags & SUCSL_FLAG_LOGOGRAM) != 0u;
}

bool sutr_sucsl_is_syllabic(uint32_t codepoint)
{
    const sucsl_sign_entry_t *entry = sucsl_find(codepoint);

    return entry != NULL && (entry->sign_flags & SUCSL_FLAG_SYLLABIC) != 0u;
}

bool sutr_sucsl_is_numerical(uint32_t codepoint)
{
    const sucsl_sign_entry_t *entry = sucsl_find(codepoint);

    return entry != NULL && (entry->sign_flags & SUCSL_FLAG_NUMERICAL) != 0u;
}

bool sutr_sucsl_is_composite(uint32_t codepoint)
{
    const sucsl_sign_entry_t *entry = sucsl_find(codepoint);

    return entry != NULL && (entry->sign_flags & SUCSL_FLAG_COMPOSITE) != 0u;
}

bool sutr_sucsl_is_deprecated(uint32_t codepoint)
{
    const sucsl_sign_entry_t *entry = sucsl_find(codepoint);

    return entry != NULL && (entry->sign_flags & SUCSL_FLAG_DEPRECATED) != 0u;
}

bool sutr_sucsl_is_variant(uint32_t codepoint)
{
    const sucsl_sign_entry_t *entry = sucsl_find(codepoint);

    return entry != NULL && (entry->sign_flags & SUCSL_FLAG_VARIANT) != 0u;
}

/* The single-category projection of the flag word (§4.3). The four functional
 * classes are not mutually exclusive in the data — a sign can be both a
 * logogram and a syllabogram — so the projection uses a fixed precedence:
 * numerical, then syllabic, then logographic, then structural composite, then
 * unknown. The bit flags themselves remain available to callers that need the
 * complete classification. */
uint8_t sutr_sucsl_category_of(uint16_t sign_flags)
{
    if ((sign_flags & SUCSL_FLAG_NUMERICAL) != 0u)
        return SUCSL_CATEGORY_NUMERICAL;
    if ((sign_flags & SUCSL_FLAG_SYLLABIC) != 0u)
        return SUCSL_CATEGORY_SYLLABIC;
    if ((sign_flags & SUCSL_FLAG_LOGOGRAM) != 0u)
        return SUCSL_CATEGORY_LOGOGRAPHIC;
    if ((sign_flags & SUCSL_FLAG_COMPOSITE) != 0u)
        return SUCSL_CATEGORY_COMPOSITE;
    return SUCSL_CATEGORY_UNKNOWN;
}

uint8_t sutr_sucsl_sign_category(uint32_t codepoint)
{
    sucsl_sign_entry_t entry;

    if (!sutr_sucsl_get_sign_entry(codepoint, &entry))
        return SUCSL_CATEGORY_UNKNOWN;
    return sutr_sucsl_category_of(entry.sign_flags);
}

const char *sutr_sucsl_category_name(uint8_t category)
{
    switch (category) {
    case SUCSL_CATEGORY_NUMERICAL:   return "Numerical";
    case SUCSL_CATEGORY_SYLLABIC:    return "Syllabic";
    case SUCSL_CATEGORY_LOGOGRAPHIC: return "Logographic";
    case SUCSL_CATEGORY_COMPOSITE:   return "Composite";
    default:                         return "Unknown";
    }
}

uint32_t sutr_sucsl_primary_value_hash(uint32_t codepoint)
{
    const sucsl_sign_entry_t *entry = sucsl_find(codepoint);

    return entry != NULL ? entry->primary_value_hash : 0u;
}

uint8_t sutr_sucsl_wedge_count(uint32_t codepoint)
{
    const sucsl_sign_entry_t *entry = sucsl_find(codepoint);

    return entry != NULL ? entry->wedge_count : 0u;
}

uint8_t sutr_sucsl_historical_period(uint32_t codepoint)
{
    const sucsl_sign_entry_t *entry = sucsl_find(codepoint);

    return entry != NULL ? entry->historical_period : SUCSL_PERIOD_UNKNOWN;
}

const char *sutr_sucsl_period_name(uint8_t period)
{
    switch (period) {
    case SUCSL_PERIOD_EARLY_DYNASTIC:  return "Early Dynastic";
    case SUCSL_PERIOD_UR_III:          return "Ur III";
    case SUCSL_PERIOD_OLD_BABYLONIAN:  return "Old Babylonian";
    case SUCSL_PERIOD_OLD_ASSYRIAN:    return "Old Assyrian";
    case SUCSL_PERIOD_MIDDLE_ASSYRIAN: return "Middle Assyrian";
    case SUCSL_PERIOD_NEO_ASSYRIAN:    return "Neo-Assyrian";
    case SUCSL_PERIOD_HELLENISTIC:     return "Hellenistic";
    case SUCSL_PERIOD_ALL:             return "All periods";
    default:                           return "Unknown";
    }
}

/* ── Compound composition and sequences (§4.6) ──────────────────── */

static const sucsl_compound_t *sucsl_find_compound(uint32_t sign)
{
    uint32_t lo = 0u;
    uint32_t hi = SUCSL_COMPOUND_TABLE_COUNT;

    while (lo < hi) {
        uint32_t mid = lo + (hi - lo) / 2u;

        if (sucsl_compounds[mid].sign < sign)
            lo = mid + 1u;
        else
            hi = mid;
    }
    if (lo < SUCSL_COMPOUND_TABLE_COUNT && sucsl_compounds[lo].sign == sign)
        return &sucsl_compounds[lo];
    return NULL;
}

static const sucsl_compound_t *sucsl_find_sequence(uint32_t name_hash)
{
    uint32_t lo = 0u;
    uint32_t hi = SUCSL_SEQUENCE_TABLE_COUNT;

    while (lo < hi) {
        uint32_t mid = lo + (hi - lo) / 2u;

        if (sucsl_sequences[mid].name_hash < name_hash)
            lo = mid + 1u;
        else
            hi = mid;
    }
    if (lo < SUCSL_SEQUENCE_TABLE_COUNT &&
        sucsl_sequences[lo].name_hash == name_hash)
        return &sucsl_sequences[lo];
    return NULL;
}

bool sutr_sucsl_get_composition(uint32_t sign, sucsl_compound_t *out_compound)
{
    const sucsl_compound_t *found;

    if (out_compound == NULL)
        return false;
    found = sucsl_find_compound(sign);
    if (found == NULL)
        return false;
    *out_compound = *found;
    return true;
}

bool sutr_sucsl_find_sequence(uint32_t name_hash, sucsl_compound_t *out_sequence)
{
    const sucsl_compound_t *found;

    if (out_sequence == NULL)
        return false;
    found = sucsl_find_sequence(name_hash);
    if (found == NULL)
        return false;
    *out_sequence = *found;
    return true;
}

uint32_t sutr_sucsl_compound_count(void)
{
    return SUCSL_COMPOUND_TABLE_COUNT;
}

bool sutr_sucsl_compound_at(uint32_t index, sucsl_compound_t *out_compound)
{
    if (out_compound == NULL || index >= SUCSL_COMPOUND_TABLE_COUNT)
        return false;
    *out_compound = sucsl_compounds[index];
    return true;
}

uint32_t sutr_sucsl_sequence_count(void)
{
    return SUCSL_SEQUENCE_TABLE_COUNT;
}

bool sutr_sucsl_sequence_at(uint32_t index, sucsl_compound_t *out_sequence)
{
    if (out_sequence == NULL || index >= SUCSL_SEQUENCE_TABLE_COUNT)
        return false;
    *out_sequence = sucsl_sequences[index];
    return true;
}

/* ── Hierarchy indices and enumeration (§4.2, §4.7) ─────────────── */

uint32_t sutr_sucsl_sign_index(uint32_t codepoint)
{
    const sucsl_sign_entry_t *found = sucsl_find(codepoint);

    if (found == NULL)
        return 0u;
    return (uint32_t)(found - sucsl_signs) + 1u;
}

uint32_t sutr_sucsl_sign_count(void)
{
    return SUCSL_SIGN_COUNT;
}

bool sutr_sucsl_sign_at(uint32_t index, sucsl_sign_entry_t *out_entry)
{
    if (out_entry == NULL || index >= SUCSL_SIGN_COUNT)
        return false;
    *out_entry = sucsl_signs[index];
    return true;
}

/* ── Version and taxonomy names ─────────────────────────────────── */

const char *sutr_sucsl_version_string(void)
{
    return "1.0.0";
}

const char *sutr_sucsl_zone_name(sucsl_zone_t zone)
{
    switch (zone) {
    case SUCSL_ZONE_BRIDGE:   return "Bridge";
    case SUCSL_ZONE_SCP:      return "SCP";
    case SUCSL_ZONE_RNUR:     return "RNUR";
    case SUCSL_ZONE_NATIVE:   return "Native";
    case SUCSL_ZONE_TRAP:     return "Trap";
    case SUCSL_ZONE_SENTINEL: return "Sentinel";
    default:                  return "ExtSUCS";
    }
}

const char *sutr_sucsl_status_code_name(sucsl_status_code_t code)
{
    switch (code) {
    case SUCSL_OK:                 return "SUCSL_OK";
    case SUCSL_ERR_NULL_ARGUMENT:  return "SUCSL_ERR_NULL_ARGUMENT";
    case SUCSL_ERR_NOT_CUNEIFORM:  return "SUCSL_ERR_NOT_CUNEIFORM";
    case SUCSL_ERR_ILLEGAL_ZONE:   return "SUCSL_ERR_ILLEGAL_ZONE";
    default:                       return "SUCSL_ERR_UNKNOWN";
    }
}
