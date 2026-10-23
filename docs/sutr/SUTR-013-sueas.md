# SUTR-013 — SuperUnicode East Asian Spacing (SUEAS)

**Standard ID:** SUTR-013
**Status:** 0.1.0 — Published
**Scope:** The binary-mapped East Asian width schema, the compiled East_Asian_Spacing and East_Asian_Width range tables, the zero-allocation O(1) width and column-span engine, the contextual inter-character spacing resolver, and the single-pass cell-grid engine across the Bridge and Supplement blocks and the Native & Neography Plane
**Applies To:** SUCD (SUAS-012), SUAS-014 (SUVTL), SUTR-0 (SUCS Core), SUTR-1 (SUTF), SUTR-6 (Bridge), the SUF vector font engine, terminal emulators, GUI text renderers, and every grid or proportional layout consumer
**Last Updated:** 2026-10-22

---

## 1. Overview

Where UTR #59 *East Asian Spacing* describes — as an informative draft — the
algorithm and the data by which a small amount of visible space is inserted
between East Asian scripts and other scripts to improve readability,
**SUTR-013 (SUEAS)** establishes the equivalent contract for SuperUnicode: the
**binary-mapped width metrics**, the **contextual spacing rules**, and the
**zero-allocation layout engines** required to resolve East Asian character
widths (Fullwidth, Halfwidth, Ambiguous, and Neutral) and proportional
inter-character spacing across 31-bit SuperUnicode text streams in `O(1)`
constant time.

The report answers four questions that a renderer must share:

1. **What is the grid width** of a codepoint — one column or two — and what
   width category (Fullwidth, Halfwidth, Narrow, Wide, Ambiguous) does it carry?
2. **Does a visible auto-space belong** between two adjacent clusters?
3. **How wide is a whole stream** in terminal cells, and how many auto-spaces
   would it receive?
4. **How do all of the above stay reachable** in `O(1)` per glyph, with no heap
   and no runtime property-file parse?

### 1.1 Relation to UTR #59

SUEAS is the SuperUnicode analog of UTR #59. It adopts that report's framing:
the *spacing* classification and the *width* classification are derived
properties of the Unicode Character Database, maintained separately from the
basic code-chart information, and the algorithm that inserts the visible space
is documented on its own schedule rather than on the Unicode release cycle.
Where this report and UTR #59 differ, **this report governs for SuperUnicode**.

The decisive difference is the execution model. UTR #59 is an informative
document: it describes a runtime `East_Asian_Spacing` property query, a dynamic
`East_Asian_Width` (EAW) parse, and a heuristic grid-alignment algorithm that
adds the visible space in a post-pass. SUEAS is an *engine* specification. Both
properties are compiled at build time into immutable, byte-aligned, static
range tables; a codepoint resolves its spacing value, its width value, and its
column span with a bounded binary search — no property-file parse, no libc, no
heap. The kernel never reads a data file at run time.

### 1.2 Scope

This report specifies the address-space zones and their renderability (§2), the
compiled binary range schema and its descriptor geometry (§3), the contextual
spacing-resolution contract (§4), the zero-allocation grid and space-count
engines (§5), the algorithmic default for unlisted characters (§6), the
normative requirements (§7), and the reference implementation and its data
curation (§8). It does **not** define a glyph rasterizer, a font format, or a
line-breaking algorithm; those remain with the SUF vector font engine and
SUVTL. It also does not define the East Asian *contents*: the compiled range
tables are version data, curated from the Unicode 18.0.0 UCD and the UTR #59
derived data file, and may be revised independently of the engine that reads
them.

### 1.3 Key architectural concepts

**1.3.1 Pre-compiled binary width classifications**

SUEAS replaces runtime EAW property parsing with static, byte-packed width
descriptors (`sueas_width_entry_t`). Each 31-bit SuperUnicode codepoint maps
directly to an explicit width category (Fullwidth, Halfwidth, Narrow, Wide, or
Ambiguous) and proportional-scaling flags, resolved from two merged, sorted,
non-overlapping range tables — one for `East_Asian_Spacing`, one for
`East_Asian_Width`.

**1.3.2 Zero-allocation grid and spacing resolution in O(1)**

During text rendering and terminal cell-grid allocation, the microkernel
evaluates character widths and conditional spacing adjustments — including
punctuation adjustment and CJK-to-Latin spacing — via direct memory indexing in
constant time per glyph, with a fixed 12-byte previous-cluster record on the
frame and not a single heap allocation (`malloc`/`free`).

**1.3.3 Modular technical report compliance**

Because specialized East Asian grid formatting and proportional spacing
adjustments are modular display features rather than core microkernel
bootstrapping requirements, SUEAS is categorized as a modular SUTR technical
specification: a headless server kernel may omit it entirely without impacting
core system integrity (§1.6).

**1.3.4 Neography and conscript spacing integration**

Because SUEAS decouples width calculations from the legacy Unicode blocks, a
user-space toolchain can extend its binary lookup tables to define custom grid
widths, fixed-pitch metrics, and proportional spacing rules for logographic and
mixed-script conlangs within the Native & Neography Plane
(`0x160000–0x16FFFF`) using the exact same structural rules.

### 1.4 Spacing specification matrix

| Concern | UTR #59 | SUEAS |
|---------|---------|-------|
| Spacing classification | Runtime derived-property query | Compiled `O(log R)` range table |
| Width classification | Dynamic EAW parse | Compiled `O(log R)` range table |
| Column span | Heuristic, per consumer | `column_units` in the binary descriptor |
| Spacing adoption | Post-pass grid alignment | Single-pass boundary rule (§4.3) |
| Stream measurement | Per-glyph, with allocation | One pass, zero allocation |
| Memory | Not specified | Zero allocation, caller-owned descriptors |
| Repertoire split | One data file, all scripts | Two merged range tables, tagged by zone |

### 1.5 Cross-references

- **SUAS-012 (SUCD)** — the offline reference layer: the human-readable
  `East_Asian_Spacing` and `East_Asian_Width` manifests from which SUEAS's
  binary tables are compiled.
- **SUAS-014 (SUVTL)** — vertical text layout: SUEAS supplies the per-character
  column spans that SUVTL's line and column passes consume.
- **SUTR-10 (SUCM)** — the conformance model: SUEAS is a REFERENCE-kind report
  that nevertheless ships a reference unit (`sutr_sueas`), and holds the
  zero-allocation and `O(1)` mandate for every query on its hot path.

### 1.6 Omitting SUEAS

Because core operating system primitives, file paths, and kernel IPC channels
require strict text safety rather than East Asian grid display, SUEAS is
categorized as a modular technical report: a lightweight server kernel or a
headless environment may omit it entirely without impacting core system
integrity. The engine, the record layout, and the resolution contract are
architectural; the East Asian *contents* are version data (§8).

## 2. Background

East Asian established typography defines that a small amount of visible space
between East Asian scripts and other scripts improves readability. This visible
space is *not* a typographic space character; it is a rendering adjustment
applied at the boundary between two adjacent clusters. SUEAS preserves the
structure of that idea while replacing its delivery mechanism.

There are two derived properties involved:

- **`East_Asian_Spacing` (EAS)** — the classification that drives the spacing
  algorithm, with the values **`O`** (Other), **`N`** (Narrow), **`W`** (Wide),
  and **`C`** (Conditional). The value `W` marks an East Asian script character
  that receives visible space against a Narrow neighbour; `N` marks a
  non-East-Asian character that receives visible space against a Wide
  neighbour; `C` marks a character whose treatment depends on the language
  context (a punctuation such as the full stop or the comma).
- **`East_Asian_Width` (EAW)** — the UAX #11 width classification, with the
  values **`N`** (Neutral), **`A`** (Ambiguous), **`H`** (Halfwidth), **`W`**
  (Wide), **`F`** (Fullwidth), and **`Na`** (Narrow).

A traditional implementation resolves both at run time: it parses a properties
file or builds an in-memory index, queries `East_Asian_Spacing` for each
character, dereferences the EAW value for each character, and then applies a
grid-alignment heuristic in a post-pass over the laid-out line. Every one of
those steps may allocate, and none of them is bounded by the microkernel's
`O(1)` guarantee.

There are several reasons to prefer the compiled form:

First, the two properties change only with a Unicode release (or with a UTR #59
data revision), so a runtime parse re-derives a constant on every boot. SUEAS
compiles the constant once.

Second, the merged range form is exact and self-verifying: the ranges are
strictly ascending, non-overlapping, and gap-separated, so a bounded binary
search is a total function, and the invariant is checked by the unit test
rather than merely asserted.

Third, a renderer's hot path asks a small number of questions — the column span,
the category, the two spacing values, and the boundary decision. SUEAS answers
all of them from the same two tables, so a terminal row, a wrapped paragraph,
and a proportional-spacing pass share one source of truth.

Finally, separating the spacing and width *contents* from the engine that reads
them makes the data maintainable with normal editorial tools. In SUEAS the
editorial artifact is the SUCD manifest, a line-oriented text file — not a
runtime parse.

### 2.1 Address-space zones

SUEAS operates over the 31-bit SUCS codepoint space and recognizes the same
address-space zones as the rest of the standard. A width query is admissible
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

The East Asian repertoire spans the Bridge Plane:

| Block | Range | Role |
|-------|-------|------|
| CJK Symbols and Punctuation, Kana, Hangul | `0x3000–0x33FF`, `0x3040–0x30FF`, `0xAC00–0xD7A3`, … | Wide script characters |
| Halfwidth and Fullwidth Forms | `0xFF00–0xFFEF` | Fullwidth and Halfwidth forms |
| CJK Unified Ideographs | `0x4E00–0x9FFF` | Wide ideographs |
| RNUR carve-out | `0x00160000–0x0016FFFF` | Conscript and logographic conlangs |

### 2.2 The RNUR neography carve-out

A toolchain may adapt the SUEAS engine to a logographic or mixed-script conlang
within the Native & Neography Plane. The **neography set** of a codepoint is
`(codepoint − 0x00160000) >> 8`, so an RNUR codepoint maps to a set index in
`0x00–0xFF`. A renderer that defines custom grid widths or fixed-pitch metrics
for such a script tags its records with the Native plane flag and grid-snap
flag; the resolver treats them exactly as it treats Bridge characters.

## 3. The compiled East Asian width schema

### 3.1 The binary range record

Both compiled tables are arrays of fixed-size, byte-aligned, padding-free
records of **12 bytes**, so a consumer may map an array of them directly from a
compiled image with no decode step.

| Field | Type | Offset | Size | Meaning |
|-------|------|--------|------|---------|
| `first` | `uint32_t` | 0 | 4 | First codepoint of the range |
| `last` | `uint32_t` | 4 | 4 | Last codepoint of the range |
| `value` | `uint32_t` | 8 | 4 | The `sueas_spacing_t` or `sueas_eaw_t` value |

The exact field order is a compile-time invariant, proven with
`offsetof`-based negative-array-bound assertions, so the layout cannot drift.

### 3.2 The width descriptor

The per-codepoint result of a query is the fixed-size `sueas_width_entry_t` of
**8 bytes**, padding-free.

| Field | Type | Offset | Size | Meaning |
|-------|------|--------|------|---------|
| `codepoint` | `uint32_t` | 0 | 4 | The raw 31-bit SuperUnicode codepoint |
| `spacing_flags` | `uint16_t` | 4 | 2 | Packed spacing and adjustment flags (§3.3) |
| `width_category` | `uint8_t` | 6 | 1 | The `sueas_width_category_t` (§3.2.1) |
| `column_units` | `uint8_t` | 7 | 1 | Grid column span: 1 ordinary, 2 wide |

**3.2.1 The width category**

The width category named by the seeding sketch is a projection of the two
compiled properties:

| Category | Value | Projected from |
|----------|-------|----------------|
| `SUEAS_WIDTH_NEUTRAL` | 0 | Neither East Asian nor an ordinary starter |
| `SUEAS_WIDTH_NAR` | 1 | `East_Asian_Spacing = N` (one column, ordinary) |
| `SUEAS_WIDTH_WID` | 2 | `East_Asian_Spacing = W` (two columns, East Asian) |
| `SUEAS_WIDTH_FULL` | 3 | `East_Asian_Width = F` |
| `SUEAS_WIDTH_HALF` | 4 | `East_Asian_Width = H` |
| `SUEAS_WIDTH_AMB` | 5 | `East_Asian_Width = A`, not Wide spacing |

The `East_Asian_Spacing` value wins for a Wide or Narrow character, because
that is the value the spacing algorithm acts on.

### 3.3 Spacing and adjustment flags

| Flag | Bit | Meaning |
|------|-----|---------|
| `SUEAS_FLAG_PROPORTIONAL` | 0 | Allow proportional micro-spacing |
| `SUEAS_FLAG_GRID_SNAP` | 1 | Force alignment to the character grid |
| `SUEAS_FLAG_PUNCT_ADJUST` | 2 | Apply halfwidth punctuation centering |
| `SUEAS_FLAG_WIDE` | 3 | Has a Wide `East_Asian_Spacing` |
| `SUEAS_FLAG_NARROW` | 4 | Has a Narrow `East_Asian_Spacing` |
| `SUEAS_FLAG_CONDITIONAL` | 5 | Has a Conditional spacing value |
| `SUEAS_FLAG_EAST_ASIAN` | 6 | Belongs to an East Asian script |
| `SUEAS_FLAG_NATIVE` | 7 | Lies in the Native / neography plane |

All defined flag bits fall within `SUEAS_FLAG_ASSIGNED_MASK` (`0x00FF`); the
bits are disjoint and the mask is the low byte.

### 3.4 Grid column span

The `column_units` of a codepoint is **2** for a Wide, Fullwidth, or Ambiguous
glyph, and **1** otherwise. Ambiguous resolves to two columns by default,
because the default context is an East Asian grid; a consumer that resolves
Ambiguous to one column does so through its own policy before measuring. A
codepoint in an inadmissible zone has no column span and reports `0`.

### 3.5 The compiled range tables and the bounded search

Two static tables hold the merged, sorted, non-overlapping, gap-separated
ranges:

- **`East_Asian_Spacing`** — `O` = 0, `N` = 1, `W` = 2, `C` = 3.
- **`East_Asian_Width`** — `N` = 0, `A` = 1, `H` = 2, `W` = 3, `F` = 4,
  `Na` = 5.

A lookup is a standard upper-bound binary search over the sorted table followed
by an endpoint test; a codepoint that precedes the first range, or that falls
in a gap between ranges, is answered by the algorithmic default of §6. The
search is bounded by the table size, hence `O(log R)`, and the table is
enumerated for verification through `sutr_sueas_spacing_at` /
`sutr_sueas_eaw_at`.

The **FNV-1a** hash of a codepoint (`sutr_sueas_hash_codepoint`), used by
consumers that build their own caches, folds the four little-endian bytes of the
codepoint with the standard `2166136261` offset basis and the `16777619` prime.

## 4. Contextual spacing resolution

### 4.1 The resolved spacing value

The compiled `East_Asian_Spacing` value is a property of the character alone.
The *resolved* value that drives the boundary rule depends on two context bits:

1. **Language context.** A `Conditional` (`C`) value resolves to `Narrow` when
   the language context is Chinese, and to `Other` otherwise. This is how the
   full stop and the comma behave: they space against East Asian characters in
   Chinese text and do not elsewhere.
2. **Vertical upright.** A `Narrow` value displayed upright in vertical layout
   resolves to `Other`, because the visible space is a horizontal-typography
   adjustment.

`sutr_sueas_resolve_spacing` applies both rules and returns the effective
value; `Wide` and `Other` are unchanged by either context bit.

### 4.2 The boundary rule

A visible auto-space is inserted **exactly** at a `Wide`/`Narrow` or
`Narrow`/`Wide` boundary of the *resolved* values, and nowhere else:

| Left | Right | Space |
|------|-------|-------|
| `Wide` | `Narrow` | Yes |
| `Narrow` | `Wide` | Yes |
| `Wide` | `Wide` | No |
| `Narrow` | `Narrow` | No |
| `Other` | any | No |
| any | `Other` | No |

`sutr_sueas_needs_space` implements the boundary rule alone; it is a pure
function of the two resolved values and costs `O(1)`.

### 4.3 The per-pair decision

`sutr_sueas_spacing_decision` composes the two: it resolves both codepoints
through §4.1 and then applies §4.2, writing a fixed-size
`sueas_spacing_decision_t` of **8 bytes**:

| Field | Type | Offset | Meaning |
|-------|------|--------|---------|
| `space` | `uint8_t` | 0 | 1 when a visible space is inserted |
| `tight` | `uint8_t` | 1 | 1 when the pair must not break (no space) |
| `reserved` | `uint16_t` | 2 | Must be zero |
| `reserved2` | `uint32_t` | 4 | Must be zero |

Either codepoint in an inadmissible zone, or a `NULL` descriptor, reports
absence.

## 5. The zero-allocation grid engine

### 5.1 Stream measurement

`sutr_sueas_measure_columns` walks a codepoint stream once and writes the
number of grid columns each codepoint occupies into a caller-owned array; the
return value is the total column span of the whole stream in terminal cells. The
caller may pass a `NULL` array to compute only the total, and a `capacity`
smaller than the stream to write only the prefix — the running total is always
complete, following the `snprintf` convention. The walk is `O(n)` with a single
`size_t` accumulator and no allocation.

`sutr_sueas_measure_columns_ex` is the same walk with the language-context and
vertical-upright bits of §4.1 threaded through, so a `Conditional` that resolves
to `Narrow` takes one column and an upright `Narrow` takes one column.

### 5.2 Space counting

`sutr_sueas_count_spaces` counts the auto-spaces the stream would receive: the
number of adjacent cluster pairs at a `Wide`/`Narrow` or `Narrow`/`Wide`
boundary of the resolved values, holding only a fixed 12-byte previous-cluster
record on the frame. It is `O(n)` with no allocation, and a non-renderable
control character is skipped without breaking adjacency.

### 5.3 Integration

A terminal emulator uses §5.1 to size a row; a GUI text renderer uses it to
decide wrap points; and a proportional-spacing pass uses §5.2 to know how many
visible spaces to distribute. All three share the same two tables and the same
resolution rules, so a measured width and a rendered width cannot disagree.

## 6. The algorithmic default

Only codepoints listed in the compiled tables have an explicit value. Every
other renderable codepoint falls to the algorithmic default:

- `East_Asian_Spacing` = **`Other`** (code 0),
- `East_Asian_Width` = **`Neutral`** (code 0),
- width category = **`Neutral`**,
- `column_units` = **1**.

This matches the documented defaults of the source data files: the UTR #59
derived data file gives `@missing: 0000..10FFFF; O`, and the UCD
`EastAsianWidth.txt` gives `N` to every unlisted codepoint. The default is
total: a query against a renderable-but-unlisted codepoint succeeds and returns
the default rather than a lookup failure. A query against an inadmissible zone
reports absence.

## 7. Normative requirements

1. The engine shall be freestanding C99 and shall not call `malloc`, `free`,
   `stdio`, or a libc string routine on any query path.
2. A width query shall be `O(log R)` in the size `R` of the compiled tables and
   shall never return a pointer into a heap.
3. The width descriptor shall be 8 bytes and the range record 12 bytes, with the
   exact field order of §3.1 and §3.2.
4. The compiled range tables shall be strictly ascending, non-overlapping, and
   gap-separated, so the bounded search of §3.5 is exact.
5. The resolved spacing value shall be a pure function of the compiled value and
   the two context bits of §4.1.
6. The boundary rule of §4.2 shall insert a visible space exactly at a
   `Wide`/`Narrow` or `Narrow`/`Wide` boundary of the resolved values.
7. The stream-measurement and space-count engines of §5 shall be `O(n)` with no
   allocation.
8. A query against an inadmissible zone shall report absence rather than a
   synthesized descriptor.

## 8. Reference implementation

SUEAS is implemented by `include/sutr/sutr_sueas.h` and `src/sutr/sutr_sueas.c`
as a freestanding C99 static library (`sutr_static`), with the host unit tests
in `tests/test_sutr_sueas.c`. It registers with the SuperUnicode Specification
Registry & Cross-Reference Engine (SSRE, SUAS-010) as specification `SUTR-013`
with reference unit `sutr_sueas`, kind `REFERENCE`, version `0.1.0`, and records
the `sueas` claim in the SuperUnicode Conformance Model (SUCM, SUTR-10) at tier
`L0`.

### 8.1 The compiled data

The compiled tables are generated from the Unicode Character Database:

| Table | Source | Rows | Distribution |
|-------|--------|------|--------------|
| `East_Asian_Spacing` | UTR #59 `east-asian-spacing.txt` | 1170 | `W`: 52, `N`: 736, `C`: 170, `O`: 212 |
| `East_Asian_Width` | UCD `EastAsianWidth.txt` (18.0.0) | 1204 | `N`: 883, `A`: 179, `H`: 7, `W`: 125, `F`: 3, `Na`: 7 |

The records are version data: a revision tracks a Unicode release or a UTR #59
data revision, not an engine revision.

### 8.2 Verification

The unit test suite covers the version and metadata, the compile-time record
geometry, zone dispatch and the RNUR carve-out, the FNV-1a hash, the compiled
range tables and the algorithmic default, the width query and every failure
outcome, the classification predicates, the spacing resolver across the
Conditional and vertical rules, the `O(n)` grid and space-count engines,
enumeration and table self-consistency, the taxonomy names, and the SSRE
registration.

## Annex A. References

- **UTR #59**, *East Asian Spacing*, Revision 1, 2024-12-16.
- **UAX #11**, *East Asian Width*.
- **UCD** `EastAsianWidth.txt`, Unicode 18.0.0, 2026-06-29.
- **UTR #59** `east-asian-spacing.txt`, 2024-12-16.

**END OF SUTR-013**
