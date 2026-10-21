# SUTR-11 — SuperUnicode Mongolian Shaping & Vertical Direction (SUMS)

**Standard ID:** SUTR-11
**Status:** 0.1.0 — Published
**Scope:** The binary-mapped Mongolian shaping schema, the flattened Free Variation Selector (FVS) sequence table, the zero-allocation O(1) contextual positional-form engine, and the native vertical-flow metrics of the Mongolian scripts (Hudum, Todo, Sibe, Manchu, Ali Gali) across the Bridge and Supplement blocks and the Native & Neography Plane
**Applies To:** SUCD (SUAS-012), SUAS-014 (SUVTL), SUTR-0 (SUCS Core), SUTR-1 (SUTF), SUTR-6 (Bridge), the SUF vector font engine, and every cursive or vertical-script consumer
**Last Updated:** 2026-10-21

---

## 1. Overview

Where UTR #54 *Unicode Mongolian 12.1 Snapshot* preserves the state of the
Mongolian code charts as of Unicode 12.1 — after the Unicode Technical Committee
decided to split the documentation of glyph variation in Mongolian from the
documentation of the code points, names, and basic reference glyphs —
**SUTR-11 (SUMS)** establishes the equivalent contract for SuperUnicode: the
**binary-mapped schemas**, the **contextual shaping rules**, and the
**zero-allocation lookup engines** required to render and process vertical script
flows and cursive Mongolian text streams in `O(1)` constant time.

The report answers four questions that a renderer must share:

1. **What is the contextual positional form** of a letter — initial, medial,
   final, or isolated — given its neighbours?
2. **Which standardized variant glyph** does a letter plus a Free Variation
   Selector (VS1–VS3) sequence select?
3. **How much vertical space** does a character occupy, and how does that flow
   integrate with SUVTL?
4. **How do all of the above stay reachable** in `O(1)`, with no heap and no
   runtime string scanning?

### 1.1 Relation to UTR #54

SUMS is the SuperUnicode analog of UTR #54. It adopts that report's framing: the
positional variants and the standardized variation sequences are *separated* from
the basic code-chart information, and the glyph-variation documentation is
maintained in a document that can be updated on its own schedule rather than on
the Unicode release cycle. Where this report and UTR #54 differ, **this report
governs for SuperUnicode**.

The decisive difference is the execution model. UTR #54 is a documentation
vehicle: it preserves the Unicode 12.1 code chart, and it is silent on how a
shaping engine resolves a variant sequence at run time. SUMS is an *engine*
specification. In a traditional implementation, Mongolian shaping requires
dynamic Free Variation Selector sequence parsing, a runtime state machine for the
positional forms, and a separate vertical-layout pass drawn from CSS or OpenType
rules. SUMS compiles all of that into an immutable, byte-packed, static table:
Mongolian is not a string-parsed hack but a clean, deterministic, 31-bit
binary-mapped writing system backed by the SuperUnicode Character Database
(SUCD — SUAS-012). The kernel never reads a manifest at run time.

### 1.2 Scope

This report specifies the address-space zones and their renderability (§2), the
binary shaping record and its flags (§3), the flattened FVS sequence table and
its resolution contract (§4), the native vertical metrics and their SUVTL
integration (§5), the algorithmic default for uncurated characters (§6), the
normative requirements (§7), and the reference implementation and its data
curation (§8). It does **not** define a glyph rasterizer, a font format, or a
layout composer; those remain with the SUF vector font engine and SUVTL. It also
does not define the Mongolian *contents*: the shaping records and variation
pairs are version data, curated from the SUCD manifests, and may be revised
independently of the engine that reads them.

### 1.3 Key architectural concepts

**1.3.1 No runtime variant-selector parsing**

An FVS sequence — a Mongolian letter followed by one of the Mongolian Free
Variation Selectors — is flattened at build time into a static binary token
table. A renderer resolves a `(base, selector)` pair by direct pointer indexing,
`O(1)`, with no `malloc`/`free`, no libc, and no sequence scan. The pair carries
a **precompiled token** into which the variant glyph is folded, so a downstream
renderer treats a base letter and a resolved variant uniformly.

**1.3.2 Native vertical flow**

Mongolian flows top-to-bottom and left-to-right. SUMS does not require a CSS or
OpenType vertical-layout pass: the **vertical advance**, the baseline rotation,
and the word-boundary spacing are stored as fixed-point byte offsets directly in
the character's binary record, and integrate natively with SUVTL (SUAS-014).

**1.3.3 Zero-allocation contextual shaping in O(1)**

SUMS replaces runtime state-machine parsers with static, byte-aligned transition
tables. Given a stream of 31-bit SuperUnicode Mongolian codepoints, the kernel
computes the positional shaping (initial, medial, final, isolated) via direct
pointer arithmetic in `O(1)` constant time per glyph, without a single heap
allocation.

**1.3.4 Expansion to neography and conscript variations**

Because SUMS decouples Mongolian shaping from the legacy Unicode blocks, a
user-space toolchain can adapt the engine to a vertical, connected, or cursive
conlang script within the Native & Neography Plane (`0x160000–0x16FFFF`) using
the exact same structural rules.

### 1.4 Shaping specification matrix

| Concern | UTR #54 | SUMS |
|---------|---------|------|
| Glyph-variation documentation | Code chart, Unicode release cycle | Separate report, revision-independent |
| Positional forms | Described per letter in the chart | Precomputed bitmask in the binary record |
| Variation sequences | Letter + FVS noted in the chart | Flattened `(base, selector) → token` table |
| Resolution cost | Runtime sequence parse, state machine | `O(1)` hash and bounded probe |
| Vertical flow | External layout rules | Fixed-point offset in the record |
| Memory | Not specified | Zero allocation, caller-owned descriptors |
| Repertoire split | One chart, all scripts | Flags distinguish Hudum / Todo / Sibe / Manchu / Ali Gali |

### 1.5 Cross-references

- **SUAS-012 (SUCD)** — the offline reference layer: the human-readable
  Mongolian shaping manifests, variation-sequence charts, and vertical-metric
  tables from which SUMS's binary tables are compiled.
- **SUAS-014 (SUVTL)** — vertical text layout: SUMS supplies the per-character
  vertical metrics that SUVTL's line and column passes consume.
- **SUTR-10 (SUCM)** — the conformance model: SUMS is a REFERENCE-kind report
  that nevertheless ships a reference unit, and holds the zero-allocation and
  `O(1)` mandate for every query on its hot path.

### 1.6 Omitting SUMS

Because core operating system primitives, file paths, and kernel IPC channels
require strict text safety rather than cursive display, SUMS is categorized as a
modular technical report: a lightweight server kernel or a headless environment
may omit it entirely without impacting core system integrity. The engine, the
record layouts, and the resolution contract are architectural; the Mongolian
*contents* are version data (§8).

## 2. Background

There are a number of strong reasons to separate the documentation of glyph
variation for Mongolian from the basic code charts, and SUMS preserves the
structure of those reasons while replacing their delivery mechanism.

There are two major classes of glyph variation impacting the rendering and
display of Mongolian text:

- **Positional Variants**
- **Standardized Variation Sequences**

The **positional variants** are changes in the shapes of Mongolian letters,
depending on their relative position with a cursively connected sequence of
letters. In particular, letter forms may change shape in initial, medial, and/or
final positions. The **standardized variation sequences** involve specific
sequences of a letter plus one of several dedicated Mongolian Free Variation
Selector (FVS) characters. Each defined combination of letter plus FVS has a
particular expected shape outcome. For both positional variants and standardized
variation sequences, in many instances the shape may be influenced by further
details of the morphological context for the word.

The Mongolian code charts gradually became more complex over the years, in an
attempt to convey all this shaping information. As of Version 12.1, all of the
positional variants and all of the standardized variation sequences were
displayed together in the Mongolian code chart. The net effect turned out not to
be particularly clear or helpful — instead the result was to overwhelm the user
of the code chart with a mass of glyph-variant detail, without sufficient
information or structure to make sense of all of it. SUMS separates this
information into a structured alternative to the basic code chart.

A second major concern is that the Mongolian script encoding encompasses more
than just the writing system for modern Mongolian (Hudum). It also includes
repertoire to cover Todo, Sibe, Manchu, and Ali Gali (Sanskrit). The code chart
necessarily includes all of the repertoire for all of these writing systems
together in one chart, which makes it very difficult to distinguish the relevant
glyph variation that applies to each writing system. SUMS keeps the one registry
but tags each record so the scripts remain distinguishable.

Third, the tooling used to format the PDF file for the Unicode code charts is
very complex software that was not optimized specifically for any one script, and
certainly not for the details of Mongolian glyph variants. It can be very
frustrating for people who wish to improve the documentation of the Mongolian
text model, or to improve that model, when they run into the tooling barriers
which make it difficult to easily adjust the display of glyph variants.
Separation of the glyph-variant information into documents that can be maintained
with normal editorial tools, instead of complicated, proprietary code-chart
formatting software, makes it possible to be much more responsive to reports of
problems in the documentation and in the Mongolian text model. In SUMS the
editorial artifact is the SUCD manifest, a line-oriented text file — not a PDF.

Finally, the Unicode code charts are only updated once with each new release of
the Unicode Standard, and each update is therefore closely tied to the Unicode
release cycle. Separating the glyph-variant information and the documentation of
the associated contextual rules and their interaction with the Mongolian text
model from the production of versioned code charts makes it possible to update
this information much more quickly. Documentation types such as Unicode Technical
Reports and Unicode Technical Notes are not tied to the Unicode release cycle,
and thus are more appropriate vehicles for quick turnaround of documentation and
model improvements.

### 2.1 Address-space zones

SUMS operates over the 31-bit SUCS codepoint space and recognizes the same
address-space zones as the rest of the standard. A shaping query is admissible
only against a **renderable** zone.

| Zone | Range | Renderable |
|------|-------|------------|
| Bridge | `0x00000000–0x0010FFFF` | Yes |
| SCP | `0x00110000–0x0011FFFF` | No |
| RNUR | `0x00160000–0x0016FFFF` | Yes |
| Native | rest of `0x00120000–0x7FFFFFEF` | Yes |
| Trap | `0x7FFFFFF0–0x7FFFFFFE` | No |
| Sentinel | `0x7FFFFFFF` | No |
| ExtSUCS | `> 0x7FFFFFFF` | No |

The Mongolian script spans two Bridge blocks plus the Native & Neography Plane:

| Block | Range | Role |
|-------|-------|------|
| Mongolian | `0x1800–0x18AF` | Hudum letters, FVS selectors, MVS, punctuation |
| Mongolian Supplement | `0x116600–0x1167FF` | Todo, Sibe, Manchu, Ali Gali repertoire |
| RNUR carve-out | `0x00160000–0x0016FFFF` | Conscript and conlang cursive scripts |

The Mongolian Supplement is numerically inside the SCP range but is ordinary
renderable Mongolian text, so it is carved out before the SCP test and reports
the Bridge zone.

### 2.2 The RNUR neography carve-out

A toolchain may adapt the SUMS engine to a vertical, connected, or cursive
conlang script within the Native & Neography Plane. The **neography set** of a
codepoint is `(codepoint − 0x00160000) >> 8`, so an RNUR codepoint maps to a
set index in `0x00–0xFF`.

## 3. The Mongolian shaping schema

### 3.1 The binary shaping record

Every curated Mongolian character is a fixed-size, byte-aligned, padding-free
record of **12 bytes**, so a consumer may map an array of them directly from a
compiled image with no decode step.

| Field | Type | Offset | Size | Meaning |
|-------|------|--------|------|---------|
| `codepoint` | `uint32_t` | 0 | 4 | The direct 31-bit codepoint |
| `vertical_advance` | `int16_t` | 4 | 2 | Vertical advance, 1/64 em |
| `shaping_flags` | `uint16_t` | 6 | 2 | Packed shaping flags (§3.2) |
| `positional_forms` | `uint8_t` | 8 | 1 | Bit `i` set: form `i` is distinct (§3.3) |
| `reserved` | `uint8_t` | 9 | 1 | Must be zero |

The exact field order is a compile-time invariant, proven with
`offsetof`-based negative-array-bound assertions, so the layout cannot drift.

### 3.2 Shaping flags

| Flag | Bit | Meaning |
|------|-----|---------|
| `SUMS_FLAG_CURSIVE` | 0 | Joins on at least one side |
| `SUMS_FLAG_VERTICAL` | 1 | Has a native vertical form |
| `SUMS_FLAG_HAS_INITIAL` | 2 | Has an initial positional form |
| `SUMS_FLAG_HAS_MEDIAL` | 3 | Has a medial positional form |
| `SUMS_FLAG_HAS_FINAL` | 4 | Has a final positional form |
| `SUMS_FLAG_VARIANT_SOURCE` | 5 | Subject to a standardized variation sequence |
| `SUMS_FLAG_FVS` | 6 | Is itself a variation selector |
| `SUMS_FLAG_FEMININE` | 7 | A feminine or alternative stem form |

All defined flag bits fall within `SUMS_FLAG_ASSIGNED_MASK` (`0x00FF`); the
upper byte is reserved and must read as zero in a compiled record.

### 3.3 Contextual positional shaping

A Mongolian letter takes one of four canonical shapes:

| Form | Constant | Meaning |
|------|----------|---------|
| Isolated | `SUMS_FORM_ISOLATED` = 0 | No join on either side |
| Initial | `SUMS_FORM_INITIAL` = 1 | Joins forward only |
| Medial | `SUMS_FORM_MEDIAL` = 2 | Joins both sides |
| Final | `SUMS_FORM_FINAL` = 3 | Joins backward only |

The `positional_forms` bitmask records which forms are *distinct* for a letter:
bit `i` set means form `i` is a distinct shape, not merely a ligature fallback.
For example, the letter *ang* (`U+1829`) has no distinct initial form, so its
mask is `0x0D` (isolated, medial, final) rather than `0x0F`.

The engine resolves a form from the neighbours of a letter in `O(1)`:

- a letter **joins backward** when the preceding character can precede it in a
  cursive run — that is, the predecessor has an initial or medial form, or is an
  FVS selector;
- a letter **joins forward** when the following character can follow it in a
  cursive run — that is, the follower has a final or medial form, or is an FVS
  selector.

A neighbour that is absent (`0`), in an inadmissible zone, or non-joining
reduces to isolated. A variation selector never takes a positional form of its
own and always resolves to isolated.

### 3.4 O(1) resolution

The curated records form a strictly ascending array, indexed once into a fixed
array of 256 open-addressing slots with an FNV-1a hash over the four
little-endian codepoint bytes and linear probing. A lookup is therefore a single
hash and a bounded probe — `O(1)` expected, with no allocation and no libc
`<string.h>` dependency.

## 4. Standardized variation sequences

### 4.1 The Free Variation Selectors

Mongolian defines three dedicated Free Variation Selectors:

| Constant | Codepoint | Character |
|----------|-----------|-----------|
| `SUMS_FVS1` | `0x180B` | MONGOLIAN FREE VARIATION SELECTOR ONE |
| `SUMS_FVS2` | `0x180C` | MONGOLIAN FREE VARIATION SELECTOR TWO |
| `SUMS_FVS3` | `0x180D` | MONGOLIAN FREE VARIATION SELECTOR THREE |

Each defined combination of letter plus FVS has a particular expected shape
outcome.

### 4.2 The flattened pair table

Each standardized sequence is a fixed **12-byte** record, and the whole set is a
static array — there is no runtime sequence scan; a renderer resolves the pair by
direct pointer indexing.

| Field | Type | Offset | Meaning |
|-------|------|--------|---------|
| `base` | `uint32_t` | 0 | The Mongolian letter |
| `selector` | `uint32_t` | 4 | `FVS1`, `FVS2`, or `FVS3` |
| `token` | `uint32_t` | 8 | The precompiled variant glyph token |

The token is non-zero, so a token of zero always means "no standardized
variant".

### 4.3 Resolution contract

Given a `(base, selector)` pair, the engine returns the base's shaping record
with the variant token folded into the `codepoint` field, so a downstream
renderer treats a base letter and a resolved variant uniformly. The failure
order is fixed and deterministic:

| Outcome | Condition |
|---------|-----------|
| `SUMS_ERR_NULL_ARGUMENT` | The output descriptor is `NULL` |
| `SUMS_ERR_ILLEGAL_ZONE` | The base or selector is not renderable |
| `SUMS_ERR_NOT_MONGOLIAN` | The base carries no curated record |
| `SUMS_ERR_BAD_SELECTOR` | The second codepoint is not a Free Variation Selector |
| `SUMS_ERR_NO_VARIANT` | The pair has no standardized variant |
| `SUMS_OK` | The pair resolved; the record was written |

The first defect encountered is reported, so a caller may branch on a single
outcome value.

## 5. Native vertical flow

Mongolian flows top-to-bottom and left-to-right. SUMS stores the vertical
advance as a **fixed-point value in 1/64 em** directly in the shaping record, so
a layout pass needs no external metric table.

| Constant | Codepoint | Role | Default advance |
|----------|-----------|------|-----------------|
| `SUMS_MVS` | `0x180E` | Mongolian vowel separator; carries width but never joins | 0 |
| `SUMS_NNBSP` | `0x202F` | Narrow no-break space; carries the stem, never joins | 0 |

An ordinary cursive letter carries the **default Mongolian stem** advance
(`0x0040`, i.e. one em) and a Supplement letter carries the Supplement stem
(`0x0010`). A character in an inadmissible zone reports a zero advance.

The metrics integrate natively with **SUVTL (SUAS-014)**: SUVTL's line and
column passes consume the per-character advance and baseline directly from the
shaping record, so no separate vertical-layout table is materialized.

## 6. The algorithmic default

A Mongolian codepoint without a curated record is not treated as unknown. If it
falls within the Mongolian block, the Mongolian Supplement, or the RNUR
carve-out, the engine synthesizes a record that is **vertical** but
**non-joining**, with the default stem advance and no distinct positional forms.
A codepoint outside those ranges — an ordinary starter — has no Mongolian
shaping at all, and both the record query and the vertical-advance query report
absence.

This rule is what makes an uncurated addition safe: a newer Mongolian letter
that the version data has not yet characterized still renders, vertically and in
isolation, without a code change to the engine.

## 7. Normative requirements

1. Every shaping query shall complete in `O(1)` expected time and shall not
   allocate.
2. Every descriptor written by the engine shall be a fixed-size, caller-owned
   struct; the engine shall never return a pointer into a heap.
3. The shaping record shall be 12 bytes with the exact field order of §3.1.
4. A variation sequence shall be resolved from the flattened pair table with no
   runtime scan of the input stream.
5. The positional-form resolution of §3.3 shall be a pure function of the
   previous, current, and next codepoints.
6. A shaping query against an inadmissible zone shall report absence rather than
   a synthesized record.
7. The curated records shall be strictly ascending in codepoint.

## 8. Reference implementation

SUMS is implemented by `include/sutr/sutr_sums.h` and `src/sutr/sutr_sums.c`
as a freestanding C99 static library (`sutr_static`), with the host unit tests
in `tests/test_sutr_sums.c`. It registers with the SuperUnicode Specification
Registry & Cross-Reference Engine (SSRE, SUAS-010) as specification `SUTR-11`
with reference unit `sutr_sums`, kind `REFERENCE`, version `0.1.0`.

### 8.1 The shaping registry

The curated shaping registry holds the Hudum letters, the Mongolian Supplement
repertoire, the Free Variation Selectors, the vowel separator, the narrow
no-break space, and the cursive-joining punctuation. The records are version
data: a revision tracks a SUCD manifest revision, not an engine revision.

### 8.2 The variation registry

The flattened variation registry holds the standardized `(letter, FVS)` pairs and
their precompiled tokens. The tokens are non-zero and distinct.

### 8.3 Verification

The unit test suite covers the version and metadata, the compile-time record
geometry, zone dispatch and the RNUR carve-out, the FNV-1a hash, the curated
records and the algorithmic default, the classification predicates, the
contextual positional forms across cursive runs, the vertical metrics, the
flattened variation resolution and every error outcome, enumeration and table
self-consistency, the taxonomy names, and the SSRE registration.

**END OF SUTR-11**
