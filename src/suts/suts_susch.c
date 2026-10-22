/* SUTS-008 — SuperUnicode Source Code Handling (SUSCH)
 *
 * Reference implementation of docs/suts/SUTS-008-susch.md.
 *
 * Freestanding C99, zero allocation. Every source-code safety property of a
 * codepoint is answered by a bounded binary search over one of four static,
 * sorted, non-overlapping range tables, and the identifier validator walks a
 * token stream exactly once. No path allocates, grows a table, or calls into
 * libc.
 *
 * The runtime layer is fed at build time from the offline reference layer (the
 * SuperUnicode Character Database, SUAS-012: the identifier-status manifests,
 * the confusables table, the Default_Ignorable_Code_Point inventory and the
 * directional-formatting-character inventory). The tables below are a curated
 * subset of that dataset. Extending a table is a data update (a minor
 * revision) and never changes the API.
 *
 * UTS #55 is the model, not the target: UTS #55 describes diagnostics for a
 * user-space toolchain, while SUSCH compiles the same evidence into immutable
 * binary tables a kernel-hosted script engine or a build-time linter can
 * consult in O(1) per codepoint.
 */

#include "suts/suts_susch.h"

/* ── Compile-time geometry checks (§3.1) ──────────────────────────── */

#define SUSCH_STATIC_ASSERT(cond, name) \
    typedef char name[(cond) ? 1 : -1]

SUSCH_STATIC_ASSERT(sizeof(susch_source_attr_t) == 12u,
                    susch_attr_is_12_bytes);
SUSCH_STATIC_ASSERT(offsetof(susch_source_attr_t, codepoint) == 0u,
                    susch_attr_codepoint_at_0);
SUSCH_STATIC_ASSERT(offsetof(susch_source_attr_t, security_mask) == 4u,
                    susch_attr_mask_at_4);
SUSCH_STATIC_ASSERT(offsetof(susch_source_attr_t, script_category) == 8u,
                    susch_attr_script_at_8);
SUSCH_STATIC_ASSERT(offsetof(susch_source_attr_t, identifier_status) == 10u,
                    susch_attr_status_at_10);
SUSCH_STATIC_ASSERT(offsetof(susch_source_attr_t, reserved) == 11u,
                    susch_attr_reserved_at_11);

/* ── Internal row shapes ──────────────────────────────────────────── */

/* One row of a compiled range table: an inclusive codepoint span and the
 * packed properties of that span. */
typedef struct {
    uint32_t first;
    uint32_t last;
    uint16_t flags;
} susch_range_row_t;

/* One row of the skeleton table: a confusable codepoint and the canonical
 * prototype a reader's eye substitutes for it (§3.5). */
typedef struct {
    uint32_t codepoint;
    uint32_t prototype;
} susch_skeleton_row_t;

/* One row of the identifier table: an inclusive codepoint span and the
 * identifier class of that span (§3.2). */
typedef struct {
    uint32_t first;
    uint32_t last;
    uint8_t  status;
} susch_ident_row_t;

/* One row of the script table: an inclusive codepoint span and its script
 * classification (§4.3). */
typedef struct {
    uint32_t first;
    uint32_t last;
    uint16_t script;
} susch_script_row_t;

/* One row of the case table: an inclusive codepoint span and its case class
 * (§4.1). */
typedef struct {
    uint32_t first;
    uint32_t last;
    uint8_t  case_class;
} susch_case_row_t;

/* ── Internal class flags (§3.3) ──────────────────────────────────── */
/* These live in the `flags` word of a class-table row. They are internal;
 * the public attribute word carries the SUSCH_FLAG_* bits. */
#define SUSCH_CLS_WHITESPACE   (1u << 0)
#define SUSCH_CLS_DIGIT        (1u << 1)
#define SUSCH_CLS_COMBINING    (1u << 2)
#define SUSCH_CLS_PUNCT        (1u << 3)
#define SUSCH_CLS_PUNCT_OTHER  (1u << 4)  /* Po: no chunk boundary */
#define SUSCH_CLS_QUOTATION    (1u << 5)
#define SUSCH_CLS_BIDI         (1u << 6)
#define SUSCH_CLS_INVISIBLE    (1u << 7)
#define SUSCH_CLS_RISK         (1u << 8)
#define SUSCH_CLS_IGNORABLE    (1u << 9)
#define SUSCH_CLS_LINEBREAK    (1u << 10)
#define SUSCH_CLS_JOINING      (1u << 11)

/* ── The compiled class table (§3.3) ──────────────────────────────── */
/* Ascending, non-overlapping order is a verification invariant, not a
 * convenience: the lookup is a binary search, and an unsorted table would
 * silently answer wrongly rather than fail. */
static const susch_range_row_t susch_class_table[] = {
    /* first     last      flags */
    { 0x00000009u, 0x00000009u, SUSCH_CLS_WHITESPACE },
    { 0x0000000Au, 0x0000000Du, SUSCH_CLS_WHITESPACE | SUSCH_CLS_LINEBREAK },
    { 0x00000020u, 0x00000020u, SUSCH_CLS_WHITESPACE },
    { 0x00000021u, 0x00000027u, SUSCH_CLS_PUNCT | SUSCH_CLS_PUNCT_OTHER },
    { 0x00000028u, 0x00000029u, SUSCH_CLS_PUNCT },
    { 0x0000002Au, 0x0000002Cu, SUSCH_CLS_PUNCT | SUSCH_CLS_PUNCT_OTHER },
    { 0x0000002Du, 0x0000002Du, SUSCH_CLS_PUNCT },
    { 0x0000002Eu, 0x0000002Fu, SUSCH_CLS_PUNCT | SUSCH_CLS_PUNCT_OTHER },
    { 0x00000030u, 0x00000039u, SUSCH_CLS_DIGIT },
    { 0x0000003Au, 0x0000003Bu, SUSCH_CLS_PUNCT | SUSCH_CLS_PUNCT_OTHER },
    { 0x0000003Fu, 0x00000040u, SUSCH_CLS_PUNCT | SUSCH_CLS_PUNCT_OTHER },
    { 0x0000005Bu, 0x0000005Bu, SUSCH_CLS_PUNCT },
    { 0x0000005Cu, 0x0000005Cu, SUSCH_CLS_PUNCT | SUSCH_CLS_PUNCT_OTHER },
    { 0x0000005Du, 0x0000005Du, SUSCH_CLS_PUNCT },
    { 0x0000005Fu, 0x0000005Fu, SUSCH_CLS_PUNCT },
    { 0x0000007Bu, 0x0000007Bu, SUSCH_CLS_PUNCT },
    { 0x0000007Du, 0x0000007Du, SUSCH_CLS_PUNCT },
    { 0x00000085u, 0x00000085u, SUSCH_CLS_WHITESPACE | SUSCH_CLS_LINEBREAK },
    { 0x000000A0u, 0x000000A0u, SUSCH_CLS_WHITESPACE | SUSCH_CLS_RISK },
    { 0x000000A1u, 0x000000A1u, SUSCH_CLS_PUNCT | SUSCH_CLS_PUNCT_OTHER },
    { 0x000000A7u, 0x000000A7u, SUSCH_CLS_PUNCT | SUSCH_CLS_PUNCT_OTHER },
    { 0x000000A9u, 0x000000A9u, SUSCH_CLS_PUNCT | SUSCH_CLS_PUNCT_OTHER },
    { 0x000000ABu, 0x000000ABu, SUSCH_CLS_PUNCT | SUSCH_CLS_QUOTATION },
    { 0x000000ADu, 0x000000ADu, SUSCH_CLS_INVISIBLE | SUSCH_CLS_IGNORABLE },
    { 0x000000AFu, 0x000000AFu, SUSCH_CLS_PUNCT | SUSCH_CLS_PUNCT_OTHER },
    { 0x000000B6u, 0x000000B7u, SUSCH_CLS_PUNCT | SUSCH_CLS_PUNCT_OTHER },
    { 0x000000B9u, 0x000000B9u, SUSCH_CLS_PUNCT | SUSCH_CLS_PUNCT_OTHER },
    { 0x000000BBu, 0x000000BBu, SUSCH_CLS_PUNCT | SUSCH_CLS_QUOTATION },
    { 0x000000BFu, 0x000000BFu, SUSCH_CLS_PUNCT | SUSCH_CLS_PUNCT_OTHER },
    { 0x00000300u, 0x0000034Eu, SUSCH_CLS_COMBINING },
    { 0x0000034Fu, 0x0000034Fu, SUSCH_CLS_COMBINING | SUSCH_CLS_INVISIBLE
                                        | SUSCH_CLS_IGNORABLE },
    { 0x00000350u, 0x0000036Fu, SUSCH_CLS_COMBINING },
    { 0x00000483u, 0x00000489u, SUSCH_CLS_COMBINING },
    { 0x00000591u, 0x000005BDu, SUSCH_CLS_COMBINING },
    { 0x000005BFu, 0x000005C7u, SUSCH_CLS_COMBINING },
    { 0x00000610u, 0x0000061Au, SUSCH_CLS_COMBINING },
    { 0x0000061Cu, 0x0000061Cu, SUSCH_CLS_BIDI | SUSCH_CLS_INVISIBLE
                                        | SUSCH_CLS_IGNORABLE },
    { 0x0000064Bu, 0x0000065Fu, SUSCH_CLS_COMBINING },
    { 0x00000660u, 0x00000669u, SUSCH_CLS_DIGIT },
    { 0x00000670u, 0x00000670u, SUSCH_CLS_COMBINING },
    { 0x000006D6u, 0x000006DCu, SUSCH_CLS_COMBINING },
    { 0x000006DFu, 0x000006E4u, SUSCH_CLS_COMBINING },
    { 0x000006E7u, 0x000006E8u, SUSCH_CLS_COMBINING },
    { 0x000006EEu, 0x000006EFu, SUSCH_CLS_COMBINING },
    { 0x000006F0u, 0x000006F9u, SUSCH_CLS_DIGIT },
    { 0x00000730u, 0x0000074Au, SUSCH_CLS_COMBINING },
    { 0x00000900u, 0x00000903u, SUSCH_CLS_COMBINING },
    { 0x0000093Au, 0x0000094Fu, SUSCH_CLS_COMBINING },
    { 0x00000951u, 0x00000957u, SUSCH_CLS_COMBINING },
    { 0x00000962u, 0x00000963u, SUSCH_CLS_COMBINING },
    { 0x00000966u, 0x0000096Fu, SUSCH_CLS_DIGIT },
    { 0x000009BCu, 0x000009BCu, SUSCH_CLS_COMBINING },
    { 0x000009BEu, 0x000009C4u, SUSCH_CLS_COMBINING },
    { 0x000009C7u, 0x000009C8u, SUSCH_CLS_COMBINING },
    { 0x000009CBu, 0x000009CDu, SUSCH_CLS_COMBINING },
    { 0x000009E2u, 0x000009E3u, SUSCH_CLS_COMBINING },
    { 0x000009F0u, 0x000009F1u, SUSCH_CLS_DIGIT },
    { 0x00000E31u, 0x00000E3Au, SUSCH_CLS_COMBINING },
    { 0x00000E40u, 0x00000E54u, SUSCH_CLS_COMBINING },
    { 0x0000115Fu, 0x00001160u, SUSCH_CLS_INVISIBLE | SUSCH_CLS_IGNORABLE },
    { 0x000017B4u, 0x000017B5u, SUSCH_CLS_INVISIBLE | SUSCH_CLS_IGNORABLE },
    { 0x0000180Bu, 0x0000180Fu, SUSCH_CLS_INVISIBLE | SUSCH_CLS_IGNORABLE },
    { 0x00002000u, 0x0000200Au, SUSCH_CLS_WHITESPACE },
    { 0x0000200Bu, 0x0000200Bu, SUSCH_CLS_INVISIBLE | SUSCH_CLS_IGNORABLE },
    { 0x0000200Cu, 0x0000200Du, SUSCH_CLS_INVISIBLE | SUSCH_CLS_IGNORABLE
                                        | SUSCH_CLS_JOINING },
    { 0x0000200Eu, 0x0000200Fu, SUSCH_CLS_BIDI | SUSCH_CLS_INVISIBLE
                                        | SUSCH_CLS_IGNORABLE },
    { 0x00002010u, 0x00002015u, SUSCH_CLS_PUNCT | SUSCH_CLS_RISK },
    { 0x00002016u, 0x00002017u, SUSCH_CLS_PUNCT | SUSCH_CLS_PUNCT_OTHER
                                        | SUSCH_CLS_RISK },
    { 0x00002018u, 0x00002019u, SUSCH_CLS_PUNCT | SUSCH_CLS_QUOTATION
                                        | SUSCH_CLS_RISK },
    { 0x0000201Au, 0x0000201Cu, SUSCH_CLS_PUNCT | SUSCH_CLS_PUNCT_OTHER
                                        | SUSCH_CLS_RISK },
    { 0x0000201Du, 0x0000201Du, SUSCH_CLS_PUNCT | SUSCH_CLS_QUOTATION
                                        | SUSCH_CLS_RISK },
    { 0x0000201Eu, 0x00002027u, SUSCH_CLS_PUNCT | SUSCH_CLS_PUNCT_OTHER
                                        | SUSCH_CLS_RISK },
    { 0x00002028u, 0x00002029u, SUSCH_CLS_WHITESPACE | SUSCH_CLS_LINEBREAK },
    { 0x0000202Au, 0x0000202Eu, SUSCH_CLS_BIDI | SUSCH_CLS_INVISIBLE
                                        | SUSCH_CLS_IGNORABLE },
    { 0x0000202Fu, 0x0000202Fu, SUSCH_CLS_WHITESPACE | SUSCH_CLS_RISK },
    { 0x00002030u, 0x00002038u, SUSCH_CLS_PUNCT | SUSCH_CLS_PUNCT_OTHER
                                        | SUSCH_CLS_RISK },
    { 0x00002039u, 0x0000203Au, SUSCH_CLS_PUNCT | SUSCH_CLS_QUOTATION },
    { 0x0000203Bu, 0x00002044u, SUSCH_CLS_PUNCT | SUSCH_CLS_PUNCT_OTHER
                                        | SUSCH_CLS_RISK },
    { 0x00002045u, 0x00002046u, SUSCH_CLS_PUNCT },
    { 0x00002047u, 0x0000205Eu, SUSCH_CLS_PUNCT | SUSCH_CLS_PUNCT_OTHER
                                        | SUSCH_CLS_RISK },
    { 0x0000205Fu, 0x0000205Fu, SUSCH_CLS_WHITESPACE },
    { 0x00002060u, 0x00002065u, SUSCH_CLS_INVISIBLE | SUSCH_CLS_IGNORABLE },
    { 0x00002066u, 0x00002069u, SUSCH_CLS_BIDI | SUSCH_CLS_INVISIBLE
                                        | SUSCH_CLS_IGNORABLE },
    { 0x0000206Au, 0x0000206Fu, SUSCH_CLS_INVISIBLE | SUSCH_CLS_IGNORABLE },
    { 0x00002100u, 0x0000214Fu, SUSCH_CLS_RISK },
    { 0x00002153u, 0x0000215Fu, SUSCH_CLS_RISK },
    { 0x00002212u, 0x00002215u, SUSCH_CLS_RISK },
    { 0x00002329u, 0x0000232Au, SUSCH_CLS_PUNCT },
    { 0x00003000u, 0x00003000u, SUSCH_CLS_WHITESPACE | SUSCH_CLS_RISK },
    { 0x00003001u, 0x00003003u, SUSCH_CLS_PUNCT | SUSCH_CLS_PUNCT_OTHER
                                        | SUSCH_CLS_RISK },
    { 0x00003008u, 0x0000300Au, SUSCH_CLS_PUNCT | SUSCH_CLS_PUNCT_OTHER },
    { 0x0000300Cu, 0x0000300Fu, SUSCH_CLS_PUNCT | SUSCH_CLS_PUNCT_OTHER
                                        | SUSCH_CLS_QUOTATION },
    { 0x00003010u, 0x00003011u, SUSCH_CLS_PUNCT },
    { 0x00003014u, 0x00003015u, SUSCH_CLS_PUNCT },
    { 0x00003016u, 0x00003017u, SUSCH_CLS_PUNCT | SUSCH_CLS_PUNCT_OTHER },
    { 0x00003018u, 0x00003018u, SUSCH_CLS_PUNCT },
    { 0x00003019u, 0x00003019u, SUSCH_CLS_PUNCT },
    { 0x0000301Au, 0x0000301Fu, SUSCH_CLS_PUNCT | SUSCH_CLS_PUNCT_OTHER },
    { 0x0000302Eu, 0x0000302Fu, SUSCH_CLS_COMBINING },
    { 0x00003030u, 0x00003030u, SUSCH_CLS_PUNCT | SUSCH_CLS_PUNCT_OTHER },
    { 0x00003099u, 0x0000309Au, SUSCH_CLS_COMBINING },
    { 0x00003164u, 0x00003164u, SUSCH_CLS_INVISIBLE | SUSCH_CLS_IGNORABLE },
    { 0x0000FE00u, 0x0000FE0Fu, SUSCH_CLS_INVISIBLE | SUSCH_CLS_COMBINING
                                        | SUSCH_CLS_IGNORABLE },
    { 0x0000FE10u, 0x0000FE19u, SUSCH_CLS_COMBINING },
    { 0x0000FE20u, 0x0000FE2Fu, SUSCH_CLS_COMBINING },
    { 0x0000FEFFu, 0x0000FEFFu, SUSCH_CLS_INVISIBLE | SUSCH_CLS_IGNORABLE },
    { 0x0000FF01u, 0x0000FF03u, SUSCH_CLS_PUNCT | SUSCH_CLS_PUNCT_OTHER
                                        | SUSCH_CLS_RISK },
    { 0x0000FF06u, 0x0000FF07u, SUSCH_CLS_PUNCT | SUSCH_CLS_PUNCT_OTHER
                                        | SUSCH_CLS_RISK },
    { 0x0000FF08u, 0x0000FF08u, SUSCH_CLS_PUNCT | SUSCH_CLS_RISK },
    { 0x0000FF09u, 0x0000FF09u, SUSCH_CLS_PUNCT | SUSCH_CLS_RISK },
    { 0x0000FF0Au, 0x0000FF0Cu, SUSCH_CLS_PUNCT | SUSCH_CLS_PUNCT_OTHER
                                        | SUSCH_CLS_RISK },
    { 0x0000FF0Du, 0x0000FF0Du, SUSCH_CLS_PUNCT | SUSCH_CLS_RISK },
    { 0x0000FF0Eu, 0x0000FF0Fu, SUSCH_CLS_PUNCT | SUSCH_CLS_PUNCT_OTHER
                                        | SUSCH_CLS_RISK },
    { 0x0000FF10u, 0x0000FF19u, SUSCH_CLS_DIGIT | SUSCH_CLS_RISK },
    { 0x0000FF1Au, 0x0000FF1Bu, SUSCH_CLS_PUNCT | SUSCH_CLS_PUNCT_OTHER
                                        | SUSCH_CLS_RISK },
    { 0x0000FF1Fu, 0x0000FF1Fu, SUSCH_CLS_PUNCT | SUSCH_CLS_PUNCT_OTHER
                                        | SUSCH_CLS_RISK },
    { 0x0000FF21u, 0x0000FF3Au, SUSCH_CLS_RISK },
    { 0x0000FF3Bu, 0x0000FF3Bu, SUSCH_CLS_PUNCT | SUSCH_CLS_RISK },
    { 0x0000FF3Cu, 0x0000FF3Cu, SUSCH_CLS_PUNCT | SUSCH_CLS_PUNCT_OTHER
                                        | SUSCH_CLS_RISK },
    { 0x0000FF3Du, 0x0000FF3Du, SUSCH_CLS_PUNCT | SUSCH_CLS_RISK },
    { 0x0000FF3Fu, 0x0000FF3Fu, SUSCH_CLS_PUNCT | SUSCH_CLS_PUNCT_OTHER
                                        | SUSCH_CLS_RISK },
    { 0x0000FF41u, 0x0000FF5Au, SUSCH_CLS_RISK },
    { 0x0000FF5Bu, 0x0000FF5Bu, SUSCH_CLS_PUNCT | SUSCH_CLS_RISK },
    { 0x0000FF5Du, 0x0000FF5Du, SUSCH_CLS_PUNCT | SUSCH_CLS_RISK },
    { 0x0000FF5Fu, 0x0000FF5Fu, SUSCH_CLS_PUNCT | SUSCH_CLS_PUNCT_OTHER
                                        | SUSCH_CLS_RISK },
    { 0x0000FF61u, 0x0000FF61u, SUSCH_CLS_PUNCT | SUSCH_CLS_PUNCT_OTHER
                                        | SUSCH_CLS_RISK },
    { 0x0000FF64u, 0x0000FF65u, SUSCH_CLS_PUNCT | SUSCH_CLS_PUNCT_OTHER
                                        | SUSCH_CLS_RISK },
    { 0x0000FFA0u, 0x0000FFA0u, SUSCH_CLS_INVISIBLE | SUSCH_CLS_IGNORABLE },
    { 0x0000FFF9u, 0x0000FFFBu, SUSCH_CLS_INVISIBLE | SUSCH_CLS_IGNORABLE },
    { 0x0001BCA0u, 0x0001BCA3u, SUSCH_CLS_INVISIBLE | SUSCH_CLS_IGNORABLE },
    { 0x0001D173u, 0x0001D17Au, SUSCH_CLS_INVISIBLE | SUSCH_CLS_IGNORABLE },
    { 0x0001D400u, 0x0001D7FFu, SUSCH_CLS_RISK },
    { 0x000E0000u, 0x000E0FFFu, SUSCH_CLS_INVISIBLE | SUSCH_CLS_IGNORABLE }
};

#define SUSCH_CLASS_COUNT \
    ((uint32_t)(sizeof susch_class_table / sizeof susch_class_table[0]))

SUSCH_STATIC_ASSERT(SUSCH_CLASS_COUNT <= SUSCH_RANGE_CAPACITY,
                    susch_class_table_fits_capacity);

/* ── The compiled skeleton table (§3.5) ───────────────────────────── */
/* A codepoint and the canonical prototype a reader's eye substitutes for
 * it. The table is what turns "this character looks like a Latin letter"
 * into a machine-checkable fact: two codepoints that share a prototype are
 * homographs of one another, and the presence of any prototype that differs
 * from the codepoint itself is the homoglyph risk signal. */
static const susch_skeleton_row_t susch_skeleton_table[] = {
    /* Latin Extended and IPA forms that read as Latin letters. */
    { 0x0000012Fu, 'I' }, { 0x00000131u, 'i' }, { 0x00000138u, 'k' },
    { 0x0000017Fu, 's' }, { 0x00000180u, 'b' }, { 0x00000184u, 'B' },
    { 0x00000186u, 'O' }, { 0x0000018Cu, 'D' }, { 0x00000191u, 'F' },
    { 0x00000192u, 'f' }, { 0x0000019Au, 'l' }, { 0x000001A2u, 'O' },
    { 0x000001A3u, 'a' }, { 0x000001A4u, 'P' }, { 0x000001A5u, 'p' },
    { 0x000001B5u, 'Z' }, { 0x000001B6u, 'z' }, { 0x000001DDu, 'e' },
    { 0x000001E0u, 'e' }, { 0x000001E3u, 'g' }, { 0x000001E4u, 'o' },
    { 0x000001E5u, 's' }, { 0x000001E6u, 'S' }, { 0x00000261u, 'g' },
    { 0x0000026Au, 'i' }, { 0x00000272u, 'n' }, { 0x0000027Cu, 'r' },
    { 0x00000287u, 'g' }, { 0x00000292u, 'z' }, { 0x0000029Eu, 'k' },

    /* Greek capitals and lowercase that read as Latin letters. */
    { 0x00000391u, 'A' }, { 0x00000392u, 'B' }, { 0x00000395u, 'E' },
    { 0x00000396u, 'Z' }, { 0x00000397u, 'H' }, { 0x00000399u, 'I' },
    { 0x0000039Au, 'K' }, { 0x0000039Cu, 'M' }, { 0x0000039Du, 'N' },
    { 0x0000039Fu, 'O' }, { 0x000003A1u, 'P' }, { 0x000003A4u, 'T' },
    { 0x000003A5u, 'Y' }, { 0x000003A7u, 'X' }, { 0x000003AAu, 'I' },
    { 0x000003ABu, 'u' }, { 0x000003ACu, 'a' }, { 0x000003ADu, 'i' },
    { 0x000003AFu, 'i' }, { 0x000003B1u, 'a' }, { 0x000003B2u, 'b' },
    { 0x000003B5u, 'e' }, { 0x000003B9u, 'i' }, { 0x000003BAu, 'k' },
    { 0x000003BCu, 'u' }, { 0x000003BDu, 'v' }, { 0x000003BFu, 'o' },
    { 0x000003C1u, 'p' },
    { 0x000003C5u, 'u' }, { 0x000003C7u, 'x' }, { 0x000003D0u, 'b' },
    { 0x000003D1u, 'o' }, { 0x000003D5u, 'f' }, { 0x000003D6u, 'p' },
    { 0x000003F2u, 'c' }, { 0x000003F9u, 'C' },

    /* Cyrillic capitals and lowercase that read as Latin letters. */
    { 0x00000405u, 's' }, { 0x00000406u, 'i' }, { 0x00000407u, 'j' },
    { 0x00000408u, 'J' }, { 0x00000409u, 'L' }, { 0x0000040Au, 'N' },
    { 0x0000040Bu, 'N' }, { 0x0000040Cu, 'h' }, { 0x0000040Du, 'H' },
    { 0x0000040Eu, 'U' }, { 0x0000040Fu, 'Y' }, { 0x00000410u, 'A' },
    { 0x00000412u, 'B' }, { 0x00000415u, 'E' }, { 0x00000418u, 'K' },
    { 0x00000419u, 'K' }, { 0x0000041Au, 'K' }, { 0x0000041Bu, 'K' },
    { 0x0000041Cu, 'M' }, { 0x0000041Du, 'H' }, { 0x0000041Eu, 'O' },
    { 0x00000420u, 'P' }, { 0x00000421u, 'C' }, { 0x00000422u, 'T' },
    { 0x00000423u, 'Y' }, { 0x00000425u, 'X' }, { 0x00000428u, 'Z' },
    { 0x0000042Cu, 'I' }, { 0x0000042Fu, 'I' }, { 0x00000430u, 'a' },
    { 0x00000431u, 'c' }, { 0x00000433u, 'e' }, { 0x00000435u, 'e' },
    { 0x00000438u, 'b' }, { 0x00000439u, 'n' }, { 0x0000043Au, 'k' },
    { 0x0000043Cu, 'c' }, { 0x0000043Eu, 'o' }, { 0x00000440u, 'p' },
    { 0x00000441u, 'c' }, { 0x00000442u, 't' }, { 0x00000443u, 'y' },
    { 0x00000445u, 'x' }, { 0x00000447u, 'r' }, { 0x00000448u, 'r' },
    { 0x00000449u, 'r' }, { 0x0000044Au, 'r' }, { 0x0000044Cu, 'l' },
    { 0x0000044Du, 'n' }, { 0x0000044Eu, 'n' }, { 0x00000450u, 'e' },
    { 0x00000451u, 'e' }, { 0x00000453u, 'r' }, { 0x00000454u, 'r' },
    { 0x00000455u, 's' }, { 0x00000456u, 'i' }, { 0x00000457u, 'j' },
    { 0x00000458u, 's' }, { 0x00000459u, 's' }, { 0x0000045Au, 's' },
    { 0x0000045Bu, 's' }, { 0x0000045Cu, 'k' }, { 0x0000045Du, 'i' },
    { 0x0000045Eu, 'u' }, { 0x0000045Fu, 'n' }, { 0x00000461u, 'r' },
    { 0x00000463u, 'r' }, { 0x00000465u, 'i' }, { 0x00000466u, 'i' },
    { 0x00000467u, 'i' }, { 0x00000468u, 'i' }, { 0x00000469u, 'i' },
    { 0x0000046Au, 'i' }, { 0x0000046Bu, 'i' }, { 0x0000046Cu, 'i' },
    { 0x0000046Du, 'n' }, { 0x00000473u, 'n' }, { 0x00000475u, 'n' },
    { 0x00000476u, 'n' }, { 0x00000477u, 'n' }, { 0x00000478u, 'o' },
    { 0x00000479u, 'n' }, { 0x0000047Au, 'n' }, { 0x0000047Bu, 'n' },
    { 0x0000047Cu, 'n' }, { 0x0000047Du, 'n' }, { 0x0000047Eu, 'n' },
    { 0x0000047Fu, 'n' }, { 0x00000481u, 'r' }, { 0x0000048Au, 'r' },
    { 0x0000048Bu, 'r' }, { 0x0000048Cu, 'r' }, { 0x0000048Du, 'r' },
    { 0x0000048Eu, 'r' }, { 0x0000048Fu, 'r' }, { 0x00000490u, 'r' },
    { 0x00000491u, 'r' }, { 0x00000493u, 'r' }, { 0x00000494u, 'r' },
    { 0x00000495u, 'r' }, { 0x00000496u, 'r' }, { 0x00000497u, 'r' },
    { 0x00000498u, 'r' }, { 0x00000499u, 'r' }, { 0x0000049Au, 'r' },
    { 0x0000049Bu, 'r' }, { 0x0000049Cu, 'r' }, { 0x0000049Du, 'r' },
    { 0x0000049Eu, 'r' }, { 0x0000049Fu, 'r' }, { 0x000004A0u, 'r' },
    { 0x000004A1u, 'r' }, { 0x000004A3u, 'r' }, { 0x000004A4u, 'r' },
    { 0x000004A7u, 'r' }, { 0x000004A8u, 'r' }, { 0x000004AEu, 'y' },
    { 0x000004AFu, 'i' }, { 0x000004B0u, 'y' }, { 0x000004B1u, 'y' },
    { 0x000004B2u, 'y' }, { 0x000004B3u, 'y' }, { 0x000004B4u, 'y' },
    { 0x000004B5u, 'y' }, { 0x000004B6u, 'y' }, { 0x000004B7u, 'y' },
    { 0x000004B8u, 'y' }, { 0x000004B9u, 'y' }, { 0x000004BAu, 'y' },
    { 0x000004BBu, 'h' }, { 0x000004BCu, 'y' }, { 0x000004BDu, 'y' },
    { 0x000004BEu, 'y' }, { 0x000004BFu, 'y' }, { 0x000004C0u, 'e' },
    { 0x000004C1u, 'y' }, { 0x000004C2u, 'y' }, { 0x000004C3u, 'y' },
    { 0x000004C4u, 'e' }, { 0x000004C5u, 'e' }, { 0x000004C6u, 'a' },
    { 0x000004C7u, 'a' }, { 0x000004C8u, 'a' }, { 0x000004C9u, 'a' },
    { 0x000004CAu, 'a' }, { 0x000004CBu, 'a' }, { 0x000004CCu, 'a' },
    { 0x000004CDu, 'a' }, { 0x000004CEu, 'a' }, { 0x000004CFu, 'a' },
    { 0x000004D0u, 'a' }, { 0x000004D1u, 'a' }, { 0x000004D2u, 'a' },
    { 0x000004D3u, 'a' }, { 0x000004D4u, 'a' }, { 0x000004D5u, 'a' },
    { 0x000004D6u, 'a' }, { 0x000004D7u, 'a' }, { 0x000004D8u, 'a' },
    { 0x000004D9u, 'a' }, { 0x000004DAu, 'a' }, { 0x000004DBu, 'a' },
    { 0x000004DDu, 'y' }, { 0x000004DEu, 'y' }, { 0x000004DFu, 'y' },
    { 0x000004E0u, 'I' }, { 0x000004E1u, 'i' }, { 0x000004E2u, 'U' },
    { 0x000004E3u, 'u' }, { 0x000004E4u, 'U' }, { 0x000004E5u, 'u' },
    { 0x000004E6u, 'U' }, { 0x000004E7u, 'u' }, { 0x000004E8u, 'Y' },
    { 0x000004E9u, 'y' }, { 0x000004EAu, 'A' }, { 0x000004EBu, 'a' },
    { 0x000004ECu, 'E' }, { 0x000004EDu, 'e' }, { 0x000004EEu, 'I' },
    { 0x000004EFu, 'i' }, { 0x000004F0u, 'U' }, { 0x000004F1u, 'u' },
    { 0x000004F2u, 'U' }, { 0x000004F3u, 'u' }, { 0x000004F4u, 'E' },
    { 0x000004F5u, 'e' }, { 0x000004F6u, 'I' }, { 0x000004F7u, 'i' },
    { 0x000004F8u, 'Z' }, { 0x000004F9u, 'z' }, { 0x000004FAu, 'Z' },
    { 0x000004FBu, 'z' }, { 0x000004FCu, 'L' }, { 0x000004FDu, 'l' },
    { 0x000004FEu, 'H' }, { 0x000004FFu, 'h' },

    /* Latin phonetic extensions and small-capital modifier forms. */
    { 0x0001D0Fu, 'o' }, { 0x0001D1Bu, 'T' }, { 0x0001D33u, 'P' },
    { 0x0001D44u, 'A' }, { 0x0001E9Eu, 'S' },

    /* Letterlike symbols that read as Latin letters. */
    { 0x00002126u, 'O' }, { 0x0000212Au, 'K' }, { 0x0000212Bu, 'A' },
    { 0x00002130u, 'E' }, { 0x00002131u, 'F' }, { 0x00002133u, 'M' },
    { 0x00002134u, 'o' }, { 0x00002135u, 'a' }, { 0x00002C6Cu, 'l' },
    { 0x00002C70u, 't' }, { 0x00002C92u, 'l' }, { 0x00002CA0u, 'C' },
    { 0x00002D27u, 'o' }, { 0x00002D2Du, 'o' },

    /* Latin Extended-D modifier letters. */
    { 0x0000A7B0u, 'o' }, { 0x0000A7B1u, 'l' }
};

#define SUSCH_SKELETON_COUNT \
    ((uint32_t)(sizeof susch_skeleton_table / sizeof susch_skeleton_table[0]))

SUSCH_STATIC_ASSERT(SUSCH_SKELETON_COUNT <= SUSCH_SKELETON_CAPACITY,
                    susch_skeleton_table_fits_capacity);

/* ── Identifier classes (§3.2) ────────────────────────────────────── */
/* The identifier table records only the *positive* rows: a codepoint that
 * is absent from the table is not an identifier character at all. The
 * default is therefore DISALLOWED, and the table stays small. */
#define SUSCH_IDENT_CLASS_NONE     0u /* never an identifier character      */
#define SUSCH_IDENT_CLASS_START    1u /* may begin an identifier            */
#define SUSCH_IDENT_CLASS_CONTINUE 2u /* may only continue an identifier    */
#define SUSCH_IDENT_CLASS_BOTH     3u /* may begin and continue             */

static const susch_ident_row_t susch_ident_table[] = {
    /* first      last       class */
    { 0x00000030u, 0x00000039u, SUSCH_IDENT_CLASS_CONTINUE },
    { 0x00000041u, 0x0000005Au, SUSCH_IDENT_CLASS_BOTH },
    { 0x0000005Fu, 0x0000005Fu, SUSCH_IDENT_CLASS_BOTH },
    { 0x00000061u, 0x0000007Au, SUSCH_IDENT_CLASS_BOTH },
    { 0x000000AAu, 0x000000AAu, SUSCH_IDENT_CLASS_BOTH },
    { 0x000000B5u, 0x000000B5u, SUSCH_IDENT_CLASS_BOTH },
    { 0x000000BAu, 0x000000BAu, SUSCH_IDENT_CLASS_BOTH },
    { 0x000000C0u, 0x000000D6u, SUSCH_IDENT_CLASS_BOTH },
    { 0x000000D8u, 0x000000F6u, SUSCH_IDENT_CLASS_BOTH },
    { 0x000000F8u, 0x000002AFu, SUSCH_IDENT_CLASS_BOTH },
    { 0x000002B0u, 0x000002FFu, SUSCH_IDENT_CLASS_BOTH },
    { 0x00000300u, 0x0000036Fu, SUSCH_IDENT_CLASS_CONTINUE },
    { 0x00000370u, 0x00000373u, SUSCH_IDENT_CLASS_BOTH },
    { 0x00000376u, 0x00000377u, SUSCH_IDENT_CLASS_BOTH },
    { 0x0000037Cu, 0x0000037Du, SUSCH_IDENT_CLASS_BOTH },
    { 0x0000037Fu, 0x0000037Fu, SUSCH_IDENT_CLASS_BOTH },
    { 0x00000386u, 0x00000386u, SUSCH_IDENT_CLASS_BOTH },
    { 0x00000388u, 0x000003ABu, SUSCH_IDENT_CLASS_BOTH },
    { 0x000003ACu, 0x000003ACu, SUSCH_IDENT_CLASS_CONTINUE },
    { 0x000003ADu, 0x000003FAu, SUSCH_IDENT_CLASS_BOTH },
    { 0x00000400u, 0x00000481u, SUSCH_IDENT_CLASS_BOTH },
    { 0x00000483u, 0x00000489u, SUSCH_IDENT_CLASS_CONTINUE },
    { 0x0000048Au, 0x0000052Fu, SUSCH_IDENT_CLASS_BOTH },
    { 0x00000531u, 0x00000556u, SUSCH_IDENT_CLASS_BOTH },
    { 0x00000561u, 0x00000587u, SUSCH_IDENT_CLASS_BOTH },
    { 0x000005D0u, 0x000005EAu, SUSCH_IDENT_CLASS_BOTH },
    { 0x00000610u, 0x0000061Au, SUSCH_IDENT_CLASS_CONTINUE },
    { 0x00000620u, 0x0000063Fu, SUSCH_IDENT_CLASS_BOTH },
    { 0x00000640u, 0x00000640u, SUSCH_IDENT_CLASS_CONTINUE },
    { 0x00000641u, 0x0000064Au, SUSCH_IDENT_CLASS_BOTH },
    { 0x0000064Bu, 0x0000065Fu, SUSCH_IDENT_CLASS_CONTINUE },
    { 0x00000660u, 0x00000669u, SUSCH_IDENT_CLASS_CONTINUE },
    { 0x00000670u, 0x00000670u, SUSCH_IDENT_CLASS_CONTINUE },
    { 0x00000671u, 0x000006D3u, SUSCH_IDENT_CLASS_BOTH },
    { 0x000006D5u, 0x000006DCu, SUSCH_IDENT_CLASS_CONTINUE },
    { 0x000006DFu, 0x000006E4u, SUSCH_IDENT_CLASS_CONTINUE },
    { 0x000006E7u, 0x000006E8u, SUSCH_IDENT_CLASS_CONTINUE },
    { 0x000006EEu, 0x000006EFu, SUSCH_IDENT_CLASS_CONTINUE },
    { 0x000006F0u, 0x000006F9u, SUSCH_IDENT_CLASS_CONTINUE },
    { 0x00000710u, 0x0000072Fu, SUSCH_IDENT_CLASS_BOTH },
    { 0x0000074Du, 0x0000074Fu, SUSCH_IDENT_CLASS_CONTINUE },
    { 0x00000750u, 0x0000077Fu, SUSCH_IDENT_CLASS_BOTH },
    { 0x000008A0u, 0x000008B4u, SUSCH_IDENT_CLASS_BOTH },
    { 0x00000900u, 0x00000903u, SUSCH_IDENT_CLASS_CONTINUE },
    { 0x00000904u, 0x00000939u, SUSCH_IDENT_CLASS_BOTH },
    { 0x0000093Au, 0x0000094Fu, SUSCH_IDENT_CLASS_CONTINUE },
    { 0x00000951u, 0x00000957u, SUSCH_IDENT_CLASS_CONTINUE },
    { 0x00000958u, 0x00000961u, SUSCH_IDENT_CLASS_BOTH },
    { 0x00000962u, 0x00000963u, SUSCH_IDENT_CLASS_CONTINUE },
    { 0x00000985u, 0x000009B9u, SUSCH_IDENT_CLASS_BOTH },
    { 0x000009BCu, 0x000009CDu, SUSCH_IDENT_CLASS_CONTINUE },
    { 0x000009CEu, 0x000009CEu, SUSCH_IDENT_CLASS_BOTH },
    { 0x000009D7u, 0x000009D7u, SUSCH_IDENT_CLASS_BOTH },
    { 0x000009DCu, 0x000009DDu, SUSCH_IDENT_CLASS_BOTH },
    { 0x000009DFu, 0x000009E1u, SUSCH_IDENT_CLASS_CONTINUE },
    { 0x000009F0u, 0x000009F1u, SUSCH_IDENT_CLASS_CONTINUE },
    { 0x00000E01u, 0x00000E30u, SUSCH_IDENT_CLASS_BOTH },
    { 0x00000E31u, 0x00000E3Au, SUSCH_IDENT_CLASS_CONTINUE },
    { 0x00000E40u, 0x00000E54u, SUSCH_IDENT_CLASS_CONTINUE },
    { 0x000010A0u, 0x000010C5u, SUSCH_IDENT_CLASS_BOTH },
    { 0x000010C7u, 0x000010C7u, SUSCH_IDENT_CLASS_BOTH },
    { 0x000010D0u, 0x000010FAu, SUSCH_IDENT_CLASS_BOTH },
    { 0x00001100u, 0x000011FFu, SUSCH_IDENT_CLASS_BOTH },
    { 0x00001E00u, 0x00001EFFu, SUSCH_IDENT_CLASS_BOTH },
    { 0x00001F00u, 0x00001FFFu, SUSCH_IDENT_CLASS_BOTH },
    { 0x0000200Cu, 0x0000200Du, SUSCH_IDENT_CLASS_CONTINUE },
    { 0x00002071u, 0x00002071u, SUSCH_IDENT_CLASS_BOTH },
    { 0x0000207Fu, 0x0000207Fu, SUSCH_IDENT_CLASS_BOTH },
    { 0x00002090u, 0x0000209Cu, SUSCH_IDENT_CLASS_BOTH },
    { 0x000020D0u, 0x000020DCu, SUSCH_IDENT_CLASS_CONTINUE },
    { 0x00002102u, 0x00002102u, SUSCH_IDENT_CLASS_BOTH },
    { 0x00002107u, 0x00002107u, SUSCH_IDENT_CLASS_BOTH },
    { 0x0000210Au, 0x00002113u, SUSCH_IDENT_CLASS_BOTH },
    { 0x00002115u, 0x00002115u, SUSCH_IDENT_CLASS_BOTH },
    { 0x00002118u, 0x0000211Du, SUSCH_IDENT_CLASS_BOTH },
    { 0x00002124u, 0x00002124u, SUSCH_IDENT_CLASS_BOTH },
    { 0x00002126u, 0x00002126u, SUSCH_IDENT_CLASS_BOTH },
    { 0x0000212Au, 0x0000212Du, SUSCH_IDENT_CLASS_BOTH },
    { 0x0000212Fu, 0x00002139u, SUSCH_IDENT_CLASS_BOTH },
    { 0x0000213Cu, 0x0000213Fu, SUSCH_IDENT_CLASS_BOTH },
    { 0x00002145u, 0x00002149u, SUSCH_IDENT_CLASS_BOTH },
    { 0x00002160u, 0x00002188u, SUSCH_IDENT_CLASS_BOTH },
    { 0x00002C00u, 0x00002C5Eu, SUSCH_IDENT_CLASS_BOTH },
    { 0x00002C60u, 0x00002C7Fu, SUSCH_IDENT_CLASS_BOTH },
    { 0x00003005u, 0x00003005u, SUSCH_IDENT_CLASS_BOTH },
    { 0x00003007u, 0x00003007u, SUSCH_IDENT_CLASS_BOTH },
    { 0x00003021u, 0x00003029u, SUSCH_IDENT_CLASS_BOTH },
    { 0x0000302Eu, 0x0000302Fu, SUSCH_IDENT_CLASS_CONTINUE },
    { 0x00003031u, 0x00003035u, SUSCH_IDENT_CLASS_BOTH },
    { 0x00003038u, 0x0000303Cu, SUSCH_IDENT_CLASS_CONTINUE },
    { 0x00003041u, 0x00003096u, SUSCH_IDENT_CLASS_BOTH },
    { 0x0000309Du, 0x0000309Fu, SUSCH_IDENT_CLASS_BOTH },
    { 0x000030A1u, 0x000030FAu, SUSCH_IDENT_CLASS_BOTH },
    { 0x000030FCu, 0x000030FFu, SUSCH_IDENT_CLASS_BOTH },
    { 0x00003105u, 0x0000312Du, SUSCH_IDENT_CLASS_BOTH },
    { 0x00003131u, 0x0000318Eu, SUSCH_IDENT_CLASS_BOTH },
    { 0x000031A0u, 0x000031BFu, SUSCH_IDENT_CLASS_BOTH },
    { 0x000031F0u, 0x000031FFu, SUSCH_IDENT_CLASS_BOTH },
    { 0x00003400u, 0x00004DB5u, SUSCH_IDENT_CLASS_BOTH },
    { 0x00004E00u, 0x00009FFCu, SUSCH_IDENT_CLASS_BOTH },
    { 0x0000A000u, 0x0000A48Cu, SUSCH_IDENT_CLASS_BOTH },
    { 0x0000AC00u, 0x0000D7A3u, SUSCH_IDENT_CLASS_BOTH },
    { 0x0000F900u, 0x0000FA6Du, SUSCH_IDENT_CLASS_BOTH },
    { 0x0000FB13u, 0x0000FB17u, SUSCH_IDENT_CLASS_BOTH },
    { 0x0000FB1Du, 0x0000FB4Fu, SUSCH_IDENT_CLASS_BOTH },
    { 0x0000FE10u, 0x0000FE19u, SUSCH_IDENT_CLASS_CONTINUE },
    { 0x0000FE20u, 0x0000FE2Fu, SUSCH_IDENT_CLASS_CONTINUE },
    { 0x0000FE70u, 0x0000FE74u, SUSCH_IDENT_CLASS_CONTINUE },
    { 0x0000FE75u, 0x0000FE77u, SUSCH_IDENT_CLASS_BOTH },
    { 0x0000FF10u, 0x0000FF19u, SUSCH_IDENT_CLASS_CONTINUE },
    { 0x0000FF21u, 0x0000FF3Au, SUSCH_IDENT_CLASS_BOTH },
    { 0x0000FF41u, 0x0000FF5Au, SUSCH_IDENT_CLASS_BOTH },
    { 0x0000FF66u, 0x0000FFDCu, SUSCH_IDENT_CLASS_BOTH }
};

#define SUSCH_IDENT_COUNT \
    ((uint32_t)(sizeof susch_ident_table / sizeof susch_ident_table[0]))

SUSCH_STATIC_ASSERT(SUSCH_IDENT_COUNT <= SUSCH_IDENT_CAPACITY,
                    susch_ident_table_fits_capacity);

/* ── The compiled script table (§4.3) ─────────────────────────────── */
static const susch_script_row_t susch_script_table[] = {
    /* first      last        script */
    { 0x00000000u, 0x00000040u, SUSCH_SCRIPT_COMMON },
    { 0x00000041u, 0x0000005Au, SUSCH_SCRIPT_LATIN },
    { 0x0000005Bu, 0x00000060u, SUSCH_SCRIPT_COMMON },
    { 0x00000061u, 0x0000007Au, SUSCH_SCRIPT_LATIN },
    { 0x0000007Bu, 0x000000BFu, SUSCH_SCRIPT_COMMON },
    { 0x000000C0u, 0x000000D6u, SUSCH_SCRIPT_LATIN },
    { 0x000000D7u, 0x000000D7u, SUSCH_SCRIPT_COMMON },
    { 0x000000D8u, 0x000000F6u, SUSCH_SCRIPT_LATIN },
    { 0x000000F7u, 0x000000F7u, SUSCH_SCRIPT_COMMON },
    { 0x000000F8u, 0x000002AFu, SUSCH_SCRIPT_LATIN },
    { 0x000002B0u, 0x000002FFu, SUSCH_SCRIPT_COMMON },
    { 0x00000300u, 0x0000036Fu, SUSCH_SCRIPT_INHERITED },
    { 0x00000370u, 0x000003FFu, SUSCH_SCRIPT_GREEK },
    { 0x00000400u, 0x0000052Fu, SUSCH_SCRIPT_CYRILLIC },
    { 0x00000530u, 0x0000058Fu, SUSCH_SCRIPT_ARMENIAN },
    { 0x00000590u, 0x000005FFu, SUSCH_SCRIPT_HEBREW },
    { 0x00000600u, 0x000006FFu, SUSCH_SCRIPT_ARABIC },
    { 0x00000700u, 0x0000074Fu, SUSCH_SCRIPT_SYRIAC },
    { 0x00000750u, 0x0000077Fu, SUSCH_SCRIPT_ARABIC },
    { 0x00000900u, 0x0000097Fu, SUSCH_SCRIPT_DEVANAGARI },
    { 0x00000980u, 0x000009FFu, SUSCH_SCRIPT_BENGALI },
    { 0x00000E00u, 0x00000E7Fu, SUSCH_SCRIPT_THAI },
    { 0x000010A0u, 0x000010FFu, SUSCH_SCRIPT_GEORGIAN },
    { 0x00001100u, 0x000011FFu, SUSCH_SCRIPT_HANGUL },
    { 0x00001E00u, 0x00001EFFu, SUSCH_SCRIPT_LATIN },
    { 0x00001F00u, 0x00001FFFu, SUSCH_SCRIPT_GREEK },
    { 0x00002000u, 0x00002FFFu, SUSCH_SCRIPT_COMMON },
    { 0x00003000u, 0x00003037u, SUSCH_SCRIPT_COMMON },
    { 0x00003038u, 0x0000303Fu, SUSCH_SCRIPT_INHERITED },
    { 0x00003040u, 0x0000309Fu, SUSCH_SCRIPT_HIRAGANA },
    { 0x000030A0u, 0x000030FFu, SUSCH_SCRIPT_KATAKANA },
    { 0x00003130u, 0x0000318Fu, SUSCH_SCRIPT_HANGUL },
    { 0x000031A0u, 0x000031FFu, SUSCH_SCRIPT_KATAKANA },
    { 0x00003200u, 0x000033FFu, SUSCH_SCRIPT_COMMON },
    { 0x00003400u, 0x00004DBFu, SUSCH_SCRIPT_HAN },
    { 0x00004E00u, 0x00009FFFu, SUSCH_SCRIPT_HAN },
    { 0x0000AC00u, 0x0000D7A3u, SUSCH_SCRIPT_HANGUL },
    { 0x0000F900u, 0x0000FAFFu, SUSCH_SCRIPT_HAN },
    { 0x0000FB00u, 0x0000FDFFu, SUSCH_SCRIPT_HEBREW },
    { 0x0000FE00u, 0x0000FE2Fu, SUSCH_SCRIPT_INHERITED },
    { 0x0000FF00u, 0x0000FF20u, SUSCH_SCRIPT_COMMON },
    { 0x0000FF21u, 0x0000FF3Au, SUSCH_SCRIPT_LATIN },
    { 0x0000FF3Bu, 0x0000FF40u, SUSCH_SCRIPT_COMMON },
    { 0x0000FF41u, 0x0000FF5Au, SUSCH_SCRIPT_LATIN },
    { 0x0000FF5Bu, 0x0000FF65u, SUSCH_SCRIPT_COMMON },
    { 0x0000FF66u, 0x0000FF9Fu, SUSCH_SCRIPT_KATAKANA },
    { 0x0000FFA0u, 0x0000FFDCu, SUSCH_SCRIPT_HANGUL },
    { 0x0000FFDDu, 0x0000FFFFu, SUSCH_SCRIPT_COMMON },
    { 0x00020000u, 0x0002FA1Fu, SUSCH_SCRIPT_HAN },
    { 0x00120000u, 0x0015FFFFu, SUSCH_SCRIPT_NATIVE },
    { 0x00160000u, 0x0016FFFFu, SUSCH_SCRIPT_NEOGRAPHY },
    { 0x00170000u, 0x7FFFFFEFu, SUSCH_SCRIPT_NATIVE }
};

#define SUSCH_SCRIPT_COUNT \
    ((uint32_t)(sizeof susch_script_table / sizeof susch_script_table[0]))

SUSCH_STATIC_ASSERT(SUSCH_SCRIPT_COUNT <= SUSCH_SCRIPT_CAPACITY,
                    susch_script_table_fits_capacity);

/* ── The compiled case table (§4.1) ───────────────────────────────── */
/* Case classes drive the camel and hat chunk boundaries of UTS #55
 * §5.1.2.1. Scripts that are unicameral (Arabic, Hebrew, Devanagari,
 * Thai, Han, Hiragana, Katakana, Hangul, Armenian, Georgian, Bengali,
 * Syriac) are deliberately absent: for them the case class is NONE, so no
 * case-driven boundary is ever invented. */
#define SUSCH_CASE_NONE        0u
#define SUSCH_CASE_LOWER       1u
#define SUSCH_CASE_UPPER       2u
#define SUSCH_CASE_TITLE       3u /* Lt outside the Greek script         */
#define SUSCH_CASE_TITLE_GREEK 4u /* Lt of the Greek script: no hat edge */

static const susch_case_row_t susch_case_table[] = {
    /* first      last       class */
    { 0x00000041u, 0x0000005Au, SUSCH_CASE_UPPER },
    { 0x00000061u, 0x0000007Au, SUSCH_CASE_LOWER },
    { 0x000000C0u, 0x000000D6u, SUSCH_CASE_UPPER },
    { 0x000000D8u, 0x000000DEu, SUSCH_CASE_UPPER },
    { 0x000000DFu, 0x000000F6u, SUSCH_CASE_LOWER },
    { 0x000000F8u, 0x000000FFu, SUSCH_CASE_LOWER },
    { 0x000001C5u, 0x000001C5u, SUSCH_CASE_TITLE_GREEK },
    { 0x000001C7u, 0x000001C7u, SUSCH_CASE_TITLE_GREEK },
    { 0x000001C9u, 0x000001C9u, SUSCH_CASE_TITLE_GREEK },
    { 0x000001CBu, 0x000001CBu, SUSCH_CASE_TITLE_GREEK },
    { 0x000001F2u, 0x000001F3u, SUSCH_CASE_TITLE },
    { 0x00000391u, 0x000003A1u, SUSCH_CASE_UPPER },
    { 0x000003A3u, 0x000003ABu, SUSCH_CASE_UPPER },
    { 0x000003B1u, 0x000003C1u, SUSCH_CASE_LOWER },
    { 0x000003C2u, 0x000003C2u, SUSCH_CASE_UPPER },
    { 0x000003C3u, 0x000003CEu, SUSCH_CASE_LOWER },
    { 0x00000400u, 0x0000042Fu, SUSCH_CASE_UPPER },
    { 0x00000430u, 0x0000045Fu, SUSCH_CASE_LOWER },
    { 0x00001F88u, 0x00001F8Fu, SUSCH_CASE_TITLE_GREEK },
    { 0x00001FBCu, 0x00001FBCu, SUSCH_CASE_TITLE_GREEK },
    { 0x00001FCCu, 0x00001FCCu, SUSCH_CASE_TITLE_GREEK },
    { 0x00001FFCu, 0x00001FFCu, SUSCH_CASE_TITLE_GREEK }
};

#define SUSCH_CASE_COUNT \
    ((uint32_t)(sizeof susch_case_table / sizeof susch_case_table[0]))

SUSCH_STATIC_ASSERT(SUSCH_CASE_COUNT <= SUSCH_CASE_CAPACITY,
                    susch_case_table_fits_capacity);

/* ── Binary-search lookups (§3.1) ────────────────────────────────── */
/* Every lookup is a bounded probe: the tables have a compile-time-fixed
 * size, so the search depth is a constant of the build, not a function of
 * the address space. The cost per codepoint is O(1). */

static const susch_range_row_t* susch_class_lookup(uint32_t cp)
{
    uint32_t lo = 0u;
    uint32_t hi = SUSCH_CLASS_COUNT;
    while (lo < hi) {
        const susch_range_row_t *row;
        uint32_t mid = lo + ((hi - lo) / 2u);
        row = &susch_class_table[mid];
        if (cp < row->first) {
            hi = mid;
        } else if (cp > row->last) {
            lo = mid + 1u;
        } else {
            return row;
        }
    }
    return NULL;
}

static uint16_t susch_class_flags(uint32_t cp)
{
    const susch_range_row_t *row = susch_class_lookup(cp);
    return (row != NULL) ? row->flags : 0u;
}

static uint8_t susch_ident_class_lookup(uint32_t cp)
{
    uint32_t lo;
    uint32_t hi;

    /* The RNUR carve-out follows the fixed positional convention of §3.6:
     * every 0x100-aligned block is one neography set, positions 0x00-0x5F
     * are letter positions, 0x60-0x7F are continuation-only positions, and
     * 0x80-0xFF are symbol positions that may never appear in an
     * identifier. */
    if (cp >= SUSCH_RNUR_MIN && cp <= SUSCH_RNUR_MAX) {
        uint32_t offset = cp & 0xFFu;
        if (offset <= 0x5Fu) {
            return SUSCH_IDENT_CLASS_BOTH;
        }
        if (offset <= 0x7Fu) {
            return SUSCH_IDENT_CLASS_CONTINUE;
        }
        return SUSCH_IDENT_CLASS_NONE;
    }

    lo = 0u;
    hi = SUSCH_IDENT_COUNT;
    while (lo < hi) {
        const susch_ident_row_t *row;
        uint32_t mid = lo + ((hi - lo) / 2u);
        row = &susch_ident_table[mid];
        if (cp < row->first) {
            hi = mid;
        } else if (cp > row->last) {
            lo = mid + 1u;
        } else {
            return row->status;
        }
    }
    return SUSCH_IDENT_CLASS_NONE;
}

static susch_script_t susch_script_lookup(uint32_t cp)
{
    uint32_t lo = 0u;
    uint32_t hi = SUSCH_SCRIPT_COUNT;
    while (lo < hi) {
        const susch_script_row_t *row;
        uint32_t mid = lo + ((hi - lo) / 2u);
        row = &susch_script_table[mid];
        if (cp < row->first) {
            hi = mid;
        } else if (cp > row->last) {
            lo = mid + 1u;
        } else {
            return (susch_script_t)row->script;
        }
    }
    return SUSCH_SCRIPT_UNKNOWN;
}

static uint8_t susch_case_lookup(uint32_t cp)
{
    uint32_t lo;
    uint32_t hi;

    /* Latin Extended-A alternates upper and lower case by parity; that
     * single rule covers all 56 letters of the block. */
    if (cp >= 0x00000100u && cp <= 0x00000137u) {
        return ((cp & 1u) == 0u) ? SUSCH_CASE_UPPER : SUSCH_CASE_LOWER;
    }

    lo = 0u;
    hi = SUSCH_CASE_COUNT;
    while (lo < hi) {
        const susch_case_row_t *row;
        uint32_t mid = lo + ((hi - lo) / 2u);
        row = &susch_case_table[mid];
        if (cp < row->first) {
            hi = mid;
        } else if (cp > row->last) {
            lo = mid + 1u;
        } else {
            return row->case_class;
        }
    }
    return SUSCH_CASE_NONE;
}

static const susch_skeleton_row_t* susch_skeleton_lookup(uint32_t cp)
{
    uint32_t lo = 0u;
    uint32_t hi = SUSCH_SKELETON_COUNT;
    while (lo < hi) {
        const susch_skeleton_row_t *row;
        uint32_t mid = lo + ((hi - lo) / 2u);
        row = &susch_skeleton_table[mid];
        if (cp < row->codepoint) {
            hi = mid;
        } else if (cp > row->codepoint) {
            lo = mid + 1u;
        } else {
            return row;
        }
    }
    return NULL;
}

/* ── Zone classification (§5) ─────────────────────────────────────── */

susch_zone_t suts_susch_zone_of(uint32_t codepoint)
{
    if (codepoint > SUSCH_LIMIT_SENTINEL) {
        return SUSCH_ZONE_EXTSUCS;
    }
    if (codepoint == SUSCH_LIMIT_SENTINEL) {
        return SUSCH_ZONE_SENTINEL;
    }
    if (codepoint >= SUSCH_LIMIT_TRAP_MIN) {
        return SUSCH_ZONE_TRAP;
    }
    if (codepoint >= SUSCH_RNUR_MIN && codepoint <= SUSCH_RNUR_MAX) {
        return SUSCH_ZONE_RNUR;
    }
    if (codepoint >= SUSCH_LIMIT_NATIVE_MIN) {
        return SUSCH_ZONE_NATIVE;
    }
    if (codepoint >= SUSCH_LIMIT_SCP_MIN) {
        return SUSCH_ZONE_SCP;
    }
    return SUSCH_ZONE_BRIDGE;
}

bool suts_susch_is_neography(uint32_t codepoint)
{
    return suts_susch_zone_of(codepoint) == SUSCH_ZONE_RNUR;
}

bool suts_susch_neography_set(uint32_t codepoint, uint32_t *out_set)
{
    if (out_set == NULL) {
        return false;
    }
    if (codepoint < SUSCH_RNUR_MIN || codepoint > SUSCH_RNUR_MAX) {
        return false;
    }
    *out_set = (codepoint - SUSCH_RNUR_MIN) >> SUSCH_RNUR_SET_SHIFT;
    return true;
}

bool suts_susch_crosses_bridge_boundary(uint32_t a, uint32_t b)
{
    susch_zone_t za = suts_susch_zone_of(a);
    susch_zone_t zb = suts_susch_zone_of(b);
    bool a_bridge = (za == SUSCH_ZONE_BRIDGE);
    bool b_bridge = (zb == SUSCH_ZONE_BRIDGE);
    bool a_native = (za == SUSCH_ZONE_NATIVE) || (za == SUSCH_ZONE_RNUR);
    bool b_native = (zb == SUSCH_ZONE_NATIVE) || (zb == SUSCH_ZONE_RNUR);
    return (a_bridge && b_native) || (b_bridge && a_native);
}

/* ── Skeleton and confusability (§3.5) ────────────────────────────── */

uint32_t suts_susch_skeleton_of(uint32_t codepoint)
{
    const susch_skeleton_row_t *row = susch_skeleton_lookup(codepoint);
    if (row != NULL) {
        return row->prototype;
    }
    /* Fullwidth Latin folds to ASCII: FF21-FF3A to A-Z, FF41-FF5A to
     * a-z. The fold is arithmetic, so it costs no table space. */
    if (codepoint >= 0x0000FF21u && codepoint <= 0x0000FF3Au) {
        return 0x00000041u + (codepoint - 0x0000FF21u);
    }
    if (codepoint >= 0x0000FF41u && codepoint <= 0x0000FF5Au) {
        return 0x00000061u + (codepoint - 0x0000FF41u);
    }
    return codepoint;
}

bool suts_susch_confusable_with(uint32_t a, uint32_t b)
{
    if (a == b) {
        return false;
    }
    return suts_susch_skeleton_of(a) == suts_susch_skeleton_of(b);
}

/* ── The compiled attribute (§3.4) ────────────────────────────────── */

static susch_identifier_status_t susch_identifier_status_of(uint32_t cp,
                                                             uint16_t clsflags)
{
    uint8_t ident_class = susch_ident_class_lookup(cp);
    susch_zone_t zone = suts_susch_zone_of(cp);

    if (ident_class == SUSCH_IDENT_CLASS_NONE) {
        return SUSCH_IDENT_DISALLOWED;
    }
    /* Zones that never carry source text. */
    if (zone == SUSCH_ZONE_SCP || zone == SUSCH_ZONE_TRAP ||
        zone == SUSCH_ZONE_SENTINEL || zone == SUSCH_ZONE_EXTSUCS) {
        return SUSCH_IDENT_DISALLOWED;
    }
    if (zone == SUSCH_ZONE_NATIVE) {
        /* The Native plane outside the RNUR carve-out is a project-private
         * allocation space; SUSCH admits it only through the RNUR. */
        return SUSCH_IDENT_DISALLOWED;
    }
    /* A directional control is never an identifier character. */
    if ((clsflags & SUSCH_CLS_BIDI) != 0u) {
        return SUSCH_IDENT_DISALLOWED;
    }
    /* An invisible character is never an identifier character, with the
     * single exception UTS #55 §5.1.3 grants to the two joining controls
     * U+200C and U+200D, which stay admissible under contextual
     * restriction and are therefore RESTRICTED rather than DISALLOWED. */
    if ((clsflags & SUSCH_CLS_INVISIBLE) != 0u) {
        if ((clsflags & SUSCH_CLS_JOINING) == 0u) {
            return SUSCH_IDENT_DISALLOWED;
        }
        return SUSCH_IDENT_RESTRICTED;
    }
    /* A character with a confusable prototype stays admissible but is
     * flagged: it is the homoglyph risk of §3.5. */
    if (suts_susch_skeleton_of(cp) != cp) {
        return SUSCH_IDENT_RESTRICTED;
    }
    /* A neography character is admissible, but its identifier profile is
     * declared by the project at build time rather than compiled here, so
     * the shipped table cannot vouch for it. */
    if (zone == SUSCH_ZONE_RNUR) {
        return SUSCH_IDENT_RESTRICTED;
    }
    return SUSCH_IDENT_ALLOWED;
}

bool suts_susch_get_attributes(uint32_t codepoint, susch_source_attr_t *out_attr)
{
    uint16_t clsflags;
    uint32_t mask = 0u;
    uint32_t skeleton;
    susch_identifier_status_t status;
    susch_script_t script;
    susch_zone_t zone;

    if (out_attr == NULL) {
        return false;
    }
    /* Anything above the Sentinel is ExtSUCS plugin space: it is not
     * source code, and SUSCH has no compiled evidence about it. */
    if (codepoint > SUSCH_LIMIT_SENTINEL) {
        return false;
    }

    clsflags = susch_class_flags(codepoint);
    skeleton = suts_susch_skeleton_of(codepoint);
    status = susch_identifier_status_of(codepoint, clsflags);
    script = susch_script_lookup(codepoint);
    zone = suts_susch_zone_of(codepoint);

    if ((clsflags & SUSCH_CLS_BIDI) != 0u) {
        mask |= SUSCH_FLAG_BIDI_CONTROL;
    }
    if ((clsflags & SUSCH_CLS_INVISIBLE) != 0u) {
        mask |= SUSCH_FLAG_INVISIBLE;
    }
    if ((clsflags & SUSCH_CLS_IGNORABLE) != 0u) {
        mask |= SUSCH_FLAG_IGNORABLE;
    }
    if ((clsflags & SUSCH_CLS_WHITESPACE) != 0u) {
        mask |= SUSCH_FLAG_WHITESPACE;
    }
    if ((clsflags & SUSCH_CLS_PUNCT) != 0u) {
        mask |= SUSCH_FLAG_PUNCTUATION;
    }
    if ((clsflags & SUSCH_CLS_DIGIT) != 0u) {
        mask |= SUSCH_FLAG_DIGIT;
    }
    if ((clsflags & SUSCH_CLS_COMBINING) != 0u) {
        mask |= SUSCH_FLAG_COMBINING;
    }
    if ((clsflags & SUSCH_CLS_LINEBREAK) != 0u) {
        mask |= SUSCH_FLAG_LINE_BREAK;
    }
    if ((clsflags & SUSCH_CLS_JOINING) != 0u) {
        mask |= SUSCH_FLAG_JOINING_CONTROL;
    }
    if ((clsflags & SUSCH_CLS_QUOTATION) != 0u) {
        mask |= SUSCH_FLAG_QUOTATION;
    }
    if ((clsflags & SUSCH_CLS_RISK) != 0u) {
        mask |= SUSCH_FLAG_HOMOGLYPH_RISK;
    }
    if (skeleton != codepoint) {
        mask |= SUSCH_FLAG_HOMOGLYPH_RISK;
    }
    if (status != SUSCH_IDENT_DISALLOWED) {
        mask |= SUSCH_FLAG_VALID_IDENTIFIER;
    }
    if (zone == SUSCH_ZONE_NATIVE || zone == SUSCH_ZONE_RNUR) {
        mask |= SUSCH_FLAG_NATIVE_PLANE;
    }

    out_attr->codepoint = codepoint;
    out_attr->security_mask = mask;
    out_attr->script_category = (uint16_t)script;
    out_attr->identifier_status = (uint8_t)status;
    out_attr->reserved = 0u;
    return true;
}

/* ── Classification predicates (§3, §4) ────────────────────────────── */

bool suts_susch_is_bidi_control(uint32_t codepoint)
{
    return (susch_class_flags(codepoint) & SUSCH_CLS_BIDI) != 0u;
}

bool suts_susch_is_invisible(uint32_t codepoint)
{
    return (susch_class_flags(codepoint) & SUSCH_CLS_INVISIBLE) != 0u;
}

bool suts_susch_is_homoglyph_risk(uint32_t codepoint)
{
    uint16_t clsflags = susch_class_flags(codepoint);
    if ((clsflags & SUSCH_CLS_RISK) != 0u) {
        return true;
    }
    return suts_susch_skeleton_of(codepoint) != codepoint;
}

susch_script_t suts_susch_script_of(uint32_t codepoint)
{
    return susch_script_lookup(codepoint);
}

susch_identifier_status_t suts_susch_identifier_status(uint32_t codepoint)
{
    return susch_identifier_status_of(codepoint, susch_class_flags(codepoint));
}

bool suts_susch_is_identifier_start(uint32_t codepoint)
{
    uint8_t ident_class;
    if (suts_susch_identifier_status(codepoint) == SUSCH_IDENT_DISALLOWED) {
        return false;
    }
    ident_class = susch_ident_class_lookup(codepoint);
    return (ident_class == SUSCH_IDENT_CLASS_START) ||
           (ident_class == SUSCH_IDENT_CLASS_BOTH);
}

bool suts_susch_is_identifier_continue(uint32_t codepoint)
{
    uint8_t ident_class;
    if (suts_susch_identifier_status(codepoint) == SUSCH_IDENT_DISALLOWED) {
        return false;
    }
    ident_class = susch_ident_class_lookup(codepoint);
    return (ident_class != SUSCH_IDENT_CLASS_NONE);
}

/* ── Identifier chunk analysis (§4.2) ─────────────────────────────── */
/* The chunk boundaries are the camel, hat and snake boundaries of UTS #55
 * §5.1.2.1:
 *
 *   CAMEL   ([\p{Ll}] | [\p{Lt} & -\p{Grek}]) ÷ ([\p{Lu}\p{Lt}])
 *   HAT     ÷ ([\p{Lu}\p{Lt}] × [\p{Ll}])
 *           ÷ [\p{Lt} & -\p{Grek}]
 *   SNAKE   ÷ [\p{P} & -\p{Po}]   and   [\p{P} & -\p{Po}] ÷
 *
 * A combining mark never forms a chunk of its own: it belongs to the
 * letter it follows. */

/* The properties of one chunk-significant codepoint. */
typedef struct {
    uint8_t  case_class;     /* SUSCH_CASE_*                          */
    bool     punct_boundary; /* punctuation that is not Po           */
    uint16_t script;         /* susch_script_t                       */
} susch_sig_t;

static bool susch_sig_of(uint32_t cp, susch_sig_t *out)
{
    uint16_t clsflags = susch_class_flags(cp);

    if ((clsflags & SUSCH_CLS_COMBINING) != 0u) {
        return false; /* a combining mark attaches to its base letter */
    }
    out->case_class = susch_case_lookup(cp);
    out->punct_boundary = ((clsflags & SUSCH_CLS_PUNCT) != 0u) &&
                          ((clsflags & SUSCH_CLS_PUNCT_OTHER) == 0u);
    out->script = (uint16_t)susch_script_lookup(cp);
    return true;
}

static bool susch_case_is_upper_or_title(uint8_t case_class)
{
    return (case_class == SUSCH_CASE_UPPER) ||
           (case_class == SUSCH_CASE_TITLE) ||
           (case_class == SUSCH_CASE_TITLE_GREEK);
}

static bool susch_case_is_camel_tail(uint8_t case_class)
{
    /* The left side of a camel boundary: a lowercase letter, or a
     * titlecase letter that is not of the Greek script. */
    return (case_class == SUSCH_CASE_LOWER) || (case_class == SUSCH_CASE_TITLE);
}

static uint32_t susch_script_bit(uint16_t script)
{
    return (uint32_t)1u << (unsigned)script;
}

static bool susch_mask_is_mixed(uint32_t mask)
{
    uint32_t significant = mask &
        ~(susch_script_bit((uint16_t)SUSCH_SCRIPT_COMMON) |
          susch_script_bit((uint16_t)SUSCH_SCRIPT_INHERITED) |
          susch_script_bit((uint16_t)SUSCH_SCRIPT_UNKNOWN));
    unsigned count = 0u;
    while (significant != 0u) {
        significant &= (significant - 1u);
        count++;
        if (count > 1u) {
            return true;
        }
    }
    return false;
}

/* The outcome of one linear pass over a token stream. */
typedef struct {
    susch_token_status_t status;
    uint32_t warnings;
    size_t   index;        /* the offending index, or the stream length */
    size_t   chunks;       /* the number of identifier chunks         */
    bool     mixed_script; /* at least one chunk spans two scripts    */
} susch_scan_t;

/* The script evidence accumulated by one identifier chunk. */
typedef struct {
    uint32_t scripts; /* a bitmap of the scripts seen in the chunk */
    bool     bridge;  /* the chunk holds a Bridge-script letter    */
    bool     native;  /* the chunk holds a Native or neography one */
} susch_chunk_t;

static void susch_chunk_add(susch_chunk_t *chunk, const susch_sig_t *sig)
{
    chunk->scripts |= susch_script_bit(sig->script);
    if (sig->script == (uint16_t)SUSCH_SCRIPT_NATIVE ||
        sig->script == (uint16_t)SUSCH_SCRIPT_NEOGRAPHY) {
        chunk->native = true;
    } else if (sig->script != (uint16_t)SUSCH_SCRIPT_COMMON &&
               sig->script != (uint16_t)SUSCH_SCRIPT_INHERITED &&
               sig->script != (uint16_t)SUSCH_SCRIPT_UNKNOWN) {
        chunk->bridge = true;
    }
}

/* The boundary decision of UTS #55 §5.1.2.1 for the edge that precedes
 * `pend`. `has_successor` is false for the last significant codepoint of
 * a stream, where the hat rule cannot fire because it needs a following
 * lowercase letter. */
static bool susch_has_boundary(bool prev_valid, const susch_sig_t *prev,
                                const susch_sig_t *pend, bool has_successor,
                                const susch_sig_t *succ)
{
    if (!prev_valid) {
        /* The very first significant codepoint of a stream opens the first
         * chunk; no boundary can precede it. */
        return false;
    }
    /* CAMEL: a lowercase letter, or a non-Greek titlecase letter, followed
     * by an uppercase or titlecase letter. */
    if (susch_case_is_camel_tail(prev->case_class) &&
        susch_case_is_upper_or_title(pend->case_class)) {
        return true;
    }
    /* HAT: before an uppercase or titlecase letter that is followed by a
     * lowercase letter. */
    if (has_successor && susch_case_is_upper_or_title(pend->case_class) &&
        succ->case_class == SUSCH_CASE_LOWER) {
        return true;
    }
    /* HAT: before a titlecase letter outside the Greek script. */
    if (pend->case_class == SUSCH_CASE_TITLE) {
        return true;
    }
    /* SNAKE: on either side of a punctuation character that is not Po. */
    if (prev->punct_boundary || pend->punct_boundary) {
        return true;
    }
    return false;
}

/* The single linear pass (§6). The validator, the chunk counter and the
 * mixed-script predicate all share it, so screening a token costs one
 * traversal, not three. */
static void susch_scan_token_stream(const uint32_t *stream, size_t length,
                                    susch_scan_t *out)
{
    susch_sig_t prev;
    susch_sig_t pend;
    susch_chunk_t chunk;
    bool prev_valid = false;
    bool pend_valid = false;
    size_t i;

    out->status = SUSCH_TOKEN_OK;
    out->warnings = SUSCH_WARN_NONE;
    out->index = length;
    out->chunks = 0u;
    out->mixed_script = false;

    if (stream == NULL) {
        if (length > 0u) {
            out->status = SUSCH_TOKEN_ERR_NULL;
        } else {
            out->status = SUSCH_TOKEN_ERR_EMPTY;
        }
        return;
    }
    if (length == 0u) {
        out->status = SUSCH_TOKEN_ERR_EMPTY;
        return;
    }

    chunk.scripts = 0u;
    chunk.bridge = false;
    chunk.native = false;

    for (i = 0u; i < length; i++) {
        uint32_t cp = stream[i];
        uint16_t clsflags = susch_class_flags(cp);
        susch_zone_t zone = suts_susch_zone_of(cp);
        uint8_t ident_class;
        susch_sig_t sig;

        /* ── Defects (a token is invalid when one is present) ────── */
        if (zone == SUSCH_ZONE_SCP || zone == SUSCH_ZONE_TRAP ||
            zone == SUSCH_ZONE_SENTINEL || zone == SUSCH_ZONE_EXTSUCS) {
            out->status = SUSCH_TOKEN_ERR_ILLEGAL_ZONE;
            out->index = i;
            return;
        }
        if ((clsflags & SUSCH_CLS_BIDI) != 0u) {
            out->status = SUSCH_TOKEN_ERR_BIDI_CONTROL;
            out->index = i;
            return;
        }
        /* An invisible character is a defect, with the single exception
         * UTS #55 §5.1.3 grants to the two joining controls U+200C and
         * U+200D: those stay admissible under contextual restriction and
         * are reported through the RESTRICTED warning instead. */
        if ((clsflags & SUSCH_CLS_INVISIBLE) != 0u &&
            (clsflags & SUSCH_CLS_JOINING) == 0u) {
            out->status = SUSCH_TOKEN_ERR_INVISIBLE;
            out->index = i;
            return;
        }
        if ((clsflags & SUSCH_CLS_LINEBREAK) != 0u) {
            out->status = SUSCH_TOKEN_ERR_LINE_BREAK;
            out->index = i;
            return;
        }

        /* ── Identifier admission ─────────────────────────────────── */
        ident_class = susch_ident_class_lookup(cp);
        if (i == 0u) {
            if (ident_class != SUSCH_IDENT_CLASS_START &&
                ident_class != SUSCH_IDENT_CLASS_BOTH) {
                out->status = SUSCH_TOKEN_ERR_INVALID_START;
                out->index = i;
                return;
            }
        } else if (ident_class == SUSCH_IDENT_CLASS_NONE) {
            out->status = SUSCH_TOKEN_ERR_INVALID_CONTINUE;
            out->index = i;
            return;
        }

        /* ── Advisory findings ────────────────────────────────────── */
        if (suts_susch_skeleton_of(cp) != cp ||
            (clsflags & SUSCH_CLS_RISK) != 0u) {
            out->warnings |= SUSCH_WARN_HOMOGLYPH;
        }
        if (susch_identifier_status_of(cp, clsflags) ==
            SUSCH_IDENT_RESTRICTED) {
            out->warnings |= SUSCH_WARN_RESTRICTED;
        }

        /* ── Chunk analysis ───────────────────────────────────────── */
        if (!susch_sig_of(cp, &sig)) {
            continue; /* a combining mark belongs to its base letter */
        }
        if (!pend_valid) {
            pend = sig;
            pend_valid = true;
            prev_valid = false;
            continue;
        }
        if (susch_has_boundary(prev_valid, &prev, &pend, true, &sig)) {
            /* The chunk that just ended is complete. */
            if (susch_mask_is_mixed(chunk.scripts)) {
                out->mixed_script = true;
            }
            if (chunk.bridge && chunk.native) {
                out->warnings |= SUSCH_WARN_CROSS_ZONE;
            }
            out->chunks++;
            chunk.scripts = 0u;
            chunk.bridge = false;
            chunk.native = false;
        }
        susch_chunk_add(&chunk, &pend);
        prev = pend;
        prev_valid = true;
        pend = sig;
    }

    if (pend_valid) {
        if (susch_has_boundary(prev_valid, &prev, &pend, false, &pend)) {
            if (susch_mask_is_mixed(chunk.scripts)) {
                out->mixed_script = true;
            }
            if (chunk.bridge && chunk.native) {
                out->warnings |= SUSCH_WARN_CROSS_ZONE;
            }
            out->chunks++;
            chunk.scripts = 0u;
            chunk.bridge = false;
            chunk.native = false;
        }
        susch_chunk_add(&chunk, &pend);
        if (susch_mask_is_mixed(chunk.scripts)) {
            out->mixed_script = true;
        }
        if (chunk.bridge && chunk.native) {
            out->warnings |= SUSCH_WARN_CROSS_ZONE;
        }
        out->chunks++;
    }
}

/* ── Public validation entry points (§6) ──────────────────────────── */

bool suts_susch_validate_token_stream_ex(const uint32_t *token_stream,
                                         size_t length,
                                         susch_token_status_t *out_status,
                                         uint32_t *out_warnings,
                                         size_t *out_index)
{
    susch_scan_t scan;
    susch_scan_token_stream(token_stream, length, &scan);

    if (scan.mixed_script) {
        scan.warnings |= SUSCH_WARN_MIXED_SCRIPT;
    }
    if (out_status != NULL) {
        *out_status = scan.status;
    }
    if (out_warnings != NULL) {
        *out_warnings = scan.warnings;
    }
    if (out_index != NULL) {
        *out_index = scan.index;
    }
    return (scan.status == SUSCH_TOKEN_OK);
}

bool suts_susch_validate_token_stream(const uint32_t *token_stream, size_t length)
{
    return suts_susch_validate_token_stream_ex(token_stream, length, NULL, NULL,
                                               NULL);
}

bool suts_susch_is_mixed_script(const uint32_t *token_stream, size_t length)
{
    susch_scan_t scan;
    susch_scan_token_stream(token_stream, length, &scan);
    return scan.mixed_script;
}

bool suts_susch_chunk_count(const uint32_t *token_stream, size_t length,
                            size_t *out_chunks)
{
    susch_scan_t scan;
    if (out_chunks == NULL) {
        return false;
    }
    susch_scan_token_stream(token_stream, length, &scan);
    *out_chunks = scan.chunks;
    return true;
}

/* ── Engine setup and version ─────────────────────────────────────── */

/* The compiled tables are static and read-only, so the engine is ready the
 * moment the image is mapped. The entry point exists so a toolchain can
 * force initialization at a deterministic point and so that a future
 * revision with build-time generated tables keeps a stable ABI. */
void suts_susch_init_engine(void)
{
    /* Nothing to build: every table is static const. */
}

SUSCH_STATIC_ASSERT(SUSCH_VERSION_MAJOR == 1 && SUSCH_VERSION_MINOR == 0,
                    susch_version_string_is_1_0);

const char* suts_susch_version_string(void)
{
    return "1.0";
}

/* ── Taxonomy names ───────────────────────────────────────────────── */

const char* suts_susch_zone_name(susch_zone_t zone)
{
    switch (zone) {
    case SUSCH_ZONE_BRIDGE:   return "Bridge";
    case SUSCH_ZONE_SCP:      return "SCP";
    case SUSCH_ZONE_RNUR:     return "RNUR";
    case SUSCH_ZONE_NATIVE:   return "Native";
    case SUSCH_ZONE_TRAP:     return "Trap";
    case SUSCH_ZONE_SENTINEL: return "Sentinel";
    case SUSCH_ZONE_EXTSUCS:  return "ExtSUCS";
    default:                  return "Unknown";
    }
}

const char* suts_susch_script_name(susch_script_t script)
{
    switch (script) {
    case SUSCH_SCRIPT_COMMON:     return "Common";
    case SUSCH_SCRIPT_LATIN:      return "Latin";
    case SUSCH_SCRIPT_GREEK:      return "Greek";
    case SUSCH_SCRIPT_CYRILLIC:   return "Cyrillic";
    case SUSCH_SCRIPT_ARMENIAN:   return "Armenian";
    case SUSCH_SCRIPT_HEBREW:     return "Hebrew";
    case SUSCH_SCRIPT_ARABIC:     return "Arabic";
    case SUSCH_SCRIPT_SYRIAC:     return "Syriac";
    case SUSCH_SCRIPT_DEVANAGARI: return "Devanagari";
    case SUSCH_SCRIPT_BENGALI:    return "Bengali";
    case SUSCH_SCRIPT_THAI:       return "Thai";
    case SUSCH_SCRIPT_GEORGIAN:   return "Georgian";
    case SUSCH_SCRIPT_HAN:        return "Han";
    case SUSCH_SCRIPT_HIRAGANA:   return "Hiragana";
    case SUSCH_SCRIPT_KATAKANA:   return "Katakana";
    case SUSCH_SCRIPT_HANGUL:     return "Hangul";
    case SUSCH_SCRIPT_NATIVE:     return "Native";
    case SUSCH_SCRIPT_NEOGRAPHY:  return "Neography";
    case SUSCH_SCRIPT_INHERITED:  return "Inherited";
    default:                      return "Unknown";
    }
}

const char* suts_susch_status_name(susch_token_status_t status)
{
    switch (status) {
    case SUSCH_TOKEN_OK:                 return "OK";
    case SUSCH_TOKEN_ERR_NULL:           return "NullStream";
    case SUSCH_TOKEN_ERR_EMPTY:          return "EmptyIdentifier";
    case SUSCH_TOKEN_ERR_ILLEGAL_ZONE:   return "IllegalZone";
    case SUSCH_TOKEN_ERR_BIDI_CONTROL:   return "BidiControl";
    case SUSCH_TOKEN_ERR_INVISIBLE:      return "InvisibleCharacter";
    case SUSCH_TOKEN_ERR_LINE_BREAK:     return "LineBreak";
    case SUSCH_TOKEN_ERR_INVALID_START:  return "InvalidStart";
    case SUSCH_TOKEN_ERR_INVALID_CONTINUE: return "InvalidContinue";
    default:                             return "Unknown";
    }
}

const char* suts_susch_identifier_status_name(susch_identifier_status_t status)
{
    switch (status) {
    case SUSCH_IDENT_DISALLOWED: return "Disallowed";
    case SUSCH_IDENT_RESTRICTED: return "Restricted";
    case SUSCH_IDENT_ALLOWED:    return "Allowed";
    default:                     return "Unknown";
    }
}
