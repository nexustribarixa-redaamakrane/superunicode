/**
 * sustandard_map.h — SuperUnicode ↔ Unicode Standard Document Mapping Tables
 *
 * Shared by:
 *   suas_to_uax.c   — SUAS-N → UAX #N converter + compat tester
 *   suts_to_uts.c   — SUTS-N → UTS #N converter + compat tester
 *   sutr_to_utr.c   — SUTR-N → UTR #N converter + compat tester
 *
 * Coverage:
 *   SUAS-001 … SUAS-017   (analogous to all UAX annexes with direct mappings)
 *   SUTS-001 … SUTS-010   (analogous to all UTS standards; continuous recount)
 *   SUTR-7   … SUTR-013   (analogous to UTR #17 … UTR #59)
 *
 * Analogy disclaimer (mirrors SUTN-002):
 *   These mappings record purpose and lineage; they do NOT assert identical
 *   algorithms, data, scope, or compatibility.  The cited SuperUnicode document
 *   is the only source for its own requirements.
 */

#ifndef SUSTANDARD_MAP_H
#define SUSTANDARD_MAP_H

/* ------------------------------------------------------------------ types */

typedef struct {
  int su_number;        /* e.g. 1 for SUAS-001 */
  int unicode_number;   /* e.g. 9 for UAX #9   */
  const char *su_abbr;  /* short acronym, e.g. "SDF"  */
  const char *su_title; /* full SuperUnicode title     */
  const char *uc_title; /* Unicode document title      */
  /* Compatibility verdict returned by the compat tester.
   * 'Y' = fully analogous scope, 'P' = partial (tighter or stricter),
   * 'E' = extends beyond Unicode scope.                              */
  char compat_grade;
  const char *compat_note; /* one-line rationale          */
} suas_uax_entry_t;

typedef struct {
  int su_number;
  int unicode_number;
  const char *su_abbr;
  const char *su_title;
  const char *uc_title;
  char compat_grade;
  const char *compat_note;
} suts_uts_entry_t;

typedef struct {
  int su_number;
  int unicode_number;
  const char *su_abbr;
  const char *su_title;
  const char *uc_title;
  char compat_grade;
  const char *compat_note;
} sutr_utr_entry_t;

/* ----------------------------------------------------------------- tables */

/* SUAS-001…017 → UAX #9…60
 * Source evidence: SUAS document headers + SUTN-002 crosswalk.          */
static const suas_uax_entry_t SUAS_UAX_TABLE[] = {
    {1, 9, "SDF", "Structural Directional Framing",
     "Unicode Bidirectional Algorithm", 'P',
     "Single-pass zero-alloc state-machine replaces multi-pass buffer model; "
     "full UAX#9 P1-L4 semantics retained."},
    {2, 11, "SGW", "System Glyph Width & Monospace Grid", "East Asian Width",
     'Y',
     "Same F/H/W/Na/A/N six-class taxonomy; extends to full ExtSUCS 64-bit "
     "space."},
    {3, 14, "SBR", "System Boundary & Line Break Rules",
     "Line Breaking Algorithm", 'P',
     "O(1) pair-state machine replaces multi-pass buffer; 39 break classes, "
     "all UAX#14 rules encoded."},
    {4, 15, "SUCF", "SuperUnicode Canonical Forms",
     "Unicode Normalization Forms", 'P',
     "SUCF-C/SUCF-D analog of NFC/NFD; zero-alloc sliding-window reorder; "
     "Hangul algorithmic preserved."},
    {5, 24, "SSP", "System Script Property", "Unicode Script Property", 'Y',
     "Scripts.txt/ScriptExtensions.txt analog; zone-dispatched over ExtSUCS; "
     "O(1) range lookup."},
    {6, 29, "STS", "System Text Segmentation", "Unicode Text Segmentation", 'P',
     "Single-pass grapheme/word/sentence boundaries; replaces regex-rule model "
     "of UAX#29."},
    {7, 31, "SIS", "System Identifiers and Syntax",
     "Unicode Identifiers and Syntax", 'P',
     "ID_Start/ID_Continue via SUCD predicates; SCP identifiers are invariant "
     "(not in UAX#31)."},
    {8, 34, "SNCS", "System Named Character Sequences",
     "Unicode Named Character Sequences", 'P',
     "Static registry, O(1) name resolution; binary-packed vs. text "
     "NamedSequences.txt."},
    {9, 38, "SIP", "System Ideographic Properties",
     "Unicode Han Database (Unihan)", 'P',
     "Byte-packed 32-byte records vs. text flat files; O(1) keyed lookup; "
     "radical-stroke table."},
    {10, 41, "SSRE", "System Specification Registry & Cross-Reference Engine",
     "Common References for Unicode Standard Annexes", 'E',
     "Embeds dependency metadata and build-time linkage validation beyond "
     "UAX#41 reference scope."},
    {11, 42, "SCMS", "System Character Manifest & Schema Standard",
     "Unicode Character Database in XML", 'P',
     "Line-oriented binary-mappable manifest grammar replaces XML DOM; "
     "single-pass bounded parser."},
    {12, 44, "SUCD", "SuperUnicode Character Database",
     "Unicode Character Database", 'P',
     "Three-kind binary page directory; O(1) range lookup; partition proof; no "
     "text flat files."},
    {13, 45, "SISR", "System Ideographic Source Registry",
     "U-Source Ideographs", 'P',
     "Byte-packed source identity records; O(1) canonical variant fallback; "
     "extends to native SUCS plane."},
    {14, 50, "SUVTL", "SuperUnicode Vertical Text Layout",
     "Unicode Vertical Text Layout", 'E',
     "Extends to Native & Neography Plane where custom scripts declare own "
     "vertical flow."},
    {15, 53, "SUAMR", "SuperUnicode Arabic Mark Rendering",
     "Unicode Arabic Mark Rendering", 'P',
     "Byte-packed mark-layout records; parallel canonical-combining-class "
     "table; O(1) AMTRA reorder."},
    {16, 57, "SUKEMET", "SuperUnicode Egyptian Hieroglyph Database",
     "Unicode Egyptian Hieroglyph Database (Unikemet)", 'P',
     "Byte-packed 12-byte sign records; O(1) Gardiner/Core/function taxonomy "
     "lookup; zero alloc."},
    {17, 60, "SUEASD", "SuperUnicode East Asian Script Data",
     "Data for East Asian Scripts", 'P',
     "Byte-packed script records; classification range table; O(1) "
     "script/ancillary resolution."},
};

#define SUAS_UAX_COUNT                                                         \
  ((int)(sizeof(SUAS_UAX_TABLE) / sizeof(SUAS_UAX_TABLE[0])))

/* SUTS-001…010 → UTS #10…61
 * Continuous sequence SUTS-001 through SUTS-010 (no gaps).             */
static const suts_uts_entry_t SUTS_UTS_TABLE[] = {
    {1, 10, "SUCA", "SuperUnicode Collation Algorithm",
     "Unicode Collation Algorithm", 'P',
     "Multilevel UCA-equivalent; NFD pre-normalises via SUCF; zero-alloc "
     "weight tables."},
    {2, 18, "SRE", "SuperUnicode Regular Expressions",
     "Unicode Regular Expressions", 'P',
     "Same conformance vocabulary; stricter single-pass engine; no "
     "backtracking."},
    {3, 35, "SLDF", "System Locale Data & Formatting",
     "Unicode Locale Data Markup Language (LDML)", 'P',
     "Binary-compiled zero-alloc locale schema replaces XML CLDR/LDML tree."},
    {4, 37, "SIVD", "System Ideographic Variation Database",
     "Unicode Ideographic Variation Database", 'P',
     "Binary-compiled variation sequences; O(1) lookup; zero alloc; no "
     "external flat files."},
    {5, 39, "SCSS", "System Character Security Specification",
     "Unicode Security Mechanisms", 'P',
     "Kernel-primitive scope; compiled homograph/confusable tables; O(1) "
     "verdict; no heap."},
    {6, 46, "SIDN",
     "System Internationalized Domain & Identifier Network Architecture",
     "Unicode IDNA Compatibility Processing", 'P',
     "Single-pass zero-alloc label mapping; no Punycode (31-bit routing); no "
     "multi-step pipeline."},
    {7, 51, "SUEPS", "SuperUnicode Emoji & Presentation Specification",
     "Unicode Emoji", 'P',
     "O(1) emoji presentation; flattened ZWJ/modifier table; no string "
     "parser."},
    {8, 55, "SUSCH", "SuperUnicode Source Code Handling",
     "Unicode Source Code Handling", 'P',
     "Compiled binary tables; O(1) per-codepoint verdict; zero alloc vs. "
     "diagnostic report."},
    {9, 58, "SULDF", "SuperUnicode Link Detection & Formatting",
     "Unicode Link Detection and Formatting", 'P',
     "Compiled Link_Term/Link_Bracket/Link_Email tables; O(1) lookup; no "
     "string scanner."},
        {10, 61, "SUSN", "SuperUnicode Set Notation", "Unicode Set Notation",
         'P',
         "Compiled destination of UTS#61 parser output; range arrays; O(1) "
         "membership; zero alloc."},
    };

#define SUTS_UTS_COUNT                                                         \
  ((int)(sizeof(SUTS_UTS_TABLE) / sizeof(SUTS_UTS_TABLE[0])))

/* SUTR-7…13 → UTR #17…59
 * Source evidence: SUTR document §1.1 «Relation to UTR #N» sections.    */
static const sutr_utr_entry_t SUTR_UTR_TABLE[] = {
    {7, 17, "SUCEM", "SuperUnicode Character Encoding Model",
     "Character Encoding Model", 'P',
     "Four-level ACR→CCS→CEF→CES model; forbidden-form-name rules; SCP anchor; "
     "O(1) trap dispatch."},
    {8, 23, "SUCPM", "SuperUnicode Character Property Model",
     "Unicode Character Property Model", 'P',
     "Range-dispatched dual lookup engine; 32-bit core + 64-bit extended mask "
     "packing; O(1) tests."},
    {9, 25, "SUMNLS", "SuperUnicode Mathematical Notation and Layout Support",
     "Unicode Support for Mathematics", 'P',
     "Extends to 31-bit SUCS; packed-mask math classification; zero-alloc "
     "box-model layout engine."},
    {10, 33, "SUCM", "SuperUnicode Conformance Model",
     "Unicode Conformance Model", 'P',
     "Cumulative tiers; clause registry; claim identifier grammar; gate "
     "records; zero-alloc mandate."},
    {11, 54, "SUMS", "SuperUnicode Mongolian Shaping & Vertical Direction",
     "Unicode Mongolian 12.1 Snapshot", 'E',
     "Adds zero-alloc O(1) runtime shaping engine + native vertical flow on "
     "top of UTR#54 data."},
    {12, 56, "SUCSL", "SuperUnicode Cuneiform Sign Lists",
     "Unicode Cuneiform Sign Lists", 'E',
     "12-byte sign records; compound composition table; O(1) lookup; RNUR "
     "extension beyond UTR#56."},
    {13, 59, "SUEAS", "SuperUnicode East Asian Spacing", "East Asian Spacing",
     'E',
     "Compiled EAS/EAW tables; O(1) width categories; contextual auto-spacing; "
     "zero-alloc grid engine."},
};

#define SUTR_UTR_COUNT                                                         \
  ((int)(sizeof(SUTR_UTR_TABLE) / sizeof(SUTR_UTR_TABLE[0])))

/* ---------------------------------------------------------------- helpers */

static inline const suas_uax_entry_t *suas_find_by_su(int n) {
  int i;
  for (i = 0; i < SUAS_UAX_COUNT; ++i)
    if (SUAS_UAX_TABLE[i].su_number == n)
      return &SUAS_UAX_TABLE[i];
  return (const suas_uax_entry_t *)0;
}

static inline const suas_uax_entry_t *suas_find_by_uax(int n) {
  int i;
  for (i = 0; i < SUAS_UAX_COUNT; ++i)
    if (SUAS_UAX_TABLE[i].unicode_number == n)
      return &SUAS_UAX_TABLE[i];
  return (const suas_uax_entry_t *)0;
}

static inline const suts_uts_entry_t *suts_find_by_su(int n) {
  int i;
  for (i = 0; i < SUTS_UTS_COUNT; ++i)
    if (SUTS_UTS_TABLE[i].su_number == n)
      return &SUTS_UTS_TABLE[i];
  return (const suts_uts_entry_t *)0;
}

static inline const suts_uts_entry_t *suts_find_by_uts(int n) {
  int i;
  for (i = 0; i < SUTS_UTS_COUNT; ++i)
    if (SUTS_UTS_TABLE[i].unicode_number == n)
      return &SUTS_UTS_TABLE[i];
  return (const suts_uts_entry_t *)0;
}

static inline const sutr_utr_entry_t *sutr_find_by_su(int n) {
  int i;
  for (i = 0; i < SUTR_UTR_COUNT; ++i)
    if (SUTR_UTR_TABLE[i].su_number == n)
      return &SUTR_UTR_TABLE[i];
  return (const sutr_utr_entry_t *)0;
}

static inline const sutr_utr_entry_t *sutr_find_by_utr(int n) {
  int i;
  for (i = 0; i < SUTR_UTR_COUNT; ++i)
    if (SUTR_UTR_TABLE[i].unicode_number == n)
      return &SUTR_UTR_TABLE[i];
  return (const sutr_utr_entry_t *)0;
}

#endif /* SUSTANDARD_MAP_H */
