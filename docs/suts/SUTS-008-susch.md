# SUTS-008 — SuperUnicode Source Code Handling (SUSCH)

**Standard ID:** SUTS-008
**Status:** Draft — Authoring
**Scope:** Modular Technical Specification (source code safety attributes, identifier validation, bidi and invisible elimination, homoglyph risk, identifier chunk analysis)
**Applies To:** Base SUCS (Unicode Compatibility Bridge, SCP, Native), the RNUR neography registry, SUCD property tables, the OpenWindows microkernel, SUTS-005 (SCSS), SUTS-006 (SIDN), SUTS-007 (SUEPS), the character token stream
**Last Updated:** 2026-10-22

---

## 1. Introduction

A source file is text that a machine reads literally and a human reads
visually. When those two readings disagree, the disagreement is an attack.
A file may *display* as `payment_total`, compile as something else entirely,
and reach a reviewer as a clean diff. The classic vectors are the
bidirectional override, the zero-width joiner, the homoglyph, and the
character that looks like a letter but is a symbol.

UTS #55, *Unicode Source Code Handling*, describes how a user-space toolchain
finds and reports these problems. It does so with a text scanner: a
maintainer-mode parser for the literal text of a source file, a set of
regular-expression-style filters over the identifier characters, and a
per-file diagnostic report.

SUSCH compiles the same evidence into immutable binary tables. Where UTS #55
runs a filter over a string, SUSCH runs a bounded binary search over a static
range table and answers in O(1) per codepoint. Where UTS #55 reports on a
file, SUSCH validates a *token stream* — the codepoint sequence a compiler
has already lexed into an identifier — in a single linear pass.

### 1.1 Relationship to UTS #55

SUSCH is deliberately **not** a UTS #55 implementation, because a user-space
linting report and a kernel-hosted screening primitive differ in four
structural ways:

1. **Diagnostic report vs. compiled verdict.** UTS #55 emits a
   human-readable report with line numbers, ranges, and prose. SUSCH emits a
   boolean verdict plus a closed outcome code and a packed advisory bitmap,
   because the consumer is a compiler front end, a kernel script engine, or a
   CI gate, not a person.
2. **String scanner vs. token-stream validator.** UTS #55 must decide where
   an identifier starts and ends inside arbitrary text, so it carries a
   maintainer-mode lexer. SUSCH is handed the token: the codepoints of one
   identifier, already delimited. It never re-lexes, never looks at
   whitespace, and never needs a source line.
3. **Heap allocation vs. zero allocation.** A user-space linter allocates
   per-line buffers, per-match records, and a report tree. SUSCH allocates
   nothing at all: every table is `static const`, every descriptor is a
   fixed-size struct the caller owns, and no path reaches for `malloc`,
   `stdio`, or a libc string routine.
4. **Modular optional compliance.** Core operating system primitives, file
   paths, and kernel IPC channels need strict text safety, not a lint
   report. A minimal kernel can omit SUSCH outright; a multilingual compiler
   opts in to get it (§1.6).

UTS #55 remains the model. Where this document shares vocabulary with it —
identifier chunks, camel/hat/snake boundaries, the General Security Profile,
directional formatting characters — the requirements are normative for SUSCH
only, and they are re-expressed in terms of compiled tables.

### 1.2 Key architectural shifts

| Dimension | UTS #55 (user-space) | SUTS-008 SUSCH (kernel) |
|-----------|----------------------|-------------------------|
| Unit of work | A source file, scanned as text | An identifier token, as a codepoint stream |
| Detection | Regular-expression-style filters over text | Bounded binary search over static range tables |
| Chunk boundaries | Derived from Unicode properties at run time | Compiled case and script tables, applied in one pass |
| Confusables | A data file loaded into memory | A compiled skeleton table plus a fullwidth fold |
| Output | A diagnostic report for a human | A verdict, an outcome code, and an advisory bitmap |
| Allocation | Per-line and per-match buffers | None, on any path |
| Complexity | O(n) in file size, with per-file setup | O(1) per codepoint, O(n) in token length, no setup |
| Dependence | A text-scanning toolchain and a properties library | A kernel primitive, no user-space service |
| Compliance | A single conformance statement | Modular optional: omittable outright (§1.6) |

### 1.3 The dual-layer architecture

SUSCH is a **dual-layer** system, in the same relationship SUAS-009 (SIP),
SUAS-012 (SUCD), and SUTS-005 (SCSS) have with their Unicode counterparts:

* **Offline reference layer — SUCD.** The human-readable confusables
  manifests, identifier-status tables, Default_Ignorable_Code_Point
  inventories, and directional-formatting-character inventories live in the
  SuperUnicode Character Database for user-space compilers, linters, IDEs,
  and developer toolchains.
* **Runtime kernel layer — SUSCH.** Build-time tooling compiles those
  datasets into immutable, byte-packed binary lookup tables embedded directly
  into kernel and toolchain images. The kernel never reads a file at run
  time.

> The seeding brief for this specification named its offline reference layer
> "SUCD - SUAS-014". That is stale: in this corpus **SUCD** is the SuperUnicode
> Character Database (**SUAS-012**), while SUAS-014 is **SUVTL**, the
> SuperUnicode Version and Licensing Table, which has nothing to do with
> character data. SUSCH therefore names `SUCD` (SUAS-012) as its reference
> layer, matching SUAS-009 (SIP), SUAS-010 (SSRE), SUAS-011 (SCMS),
> SUAS-012 (SUCD), SUAS-013 (SISR), SUAS-014 (SUVTL), SUTS-005 (SCSS),
> SUTS-006 (SIDN), and SUTS-007 (SUEPS). The house prefix `SUSCH_*` and the
> sketched entry points `suts_susch_init_engine`,
> `suts_susch_get_attributes`, and `suts_susch_validate_token_stream` are
> preserved verbatim, and the attribute type keeps the sketch's field layout
> (`codepoint`, `security_mask`, `script_category`, `identifier_status`).

### 1.4 Terminology and cross-references

SUSCH reuses the SuperUnicode architecture documents, numbered as they exist
in this repository:

| SUSCH concept | Authoritative document |
|---------------|------------------------|
| Codepoint space, SCP, traps, Sentinel | **SUTR-0 (SUCS Core)** — three-space layout |
| The "No Surrogates" rule, atomic scalars | **SUTR-1 (SUTF)** — invariant 2 |
| Bridge / Native / ExtSUCS planes | **SUTR-7 (SUCEM)**, **SUTR-6 (Bridge)** |
| RNUR neography registry, property tiers | **SUTR-8 (SUCPM)** — the character property model |
| Conformance model and zero-allocation mandate | **SUTR-10 (SUCM)** |
| Vertical direction, Mongolian shaping | **SUTR-11 (SUMS)** |
| Character security, homographs | **SUTS-005 (SCSS)** — UTS #39 analog |
| Identifier and label handling | **SUTS-006 (SIDN)** — UTS #46 analog |
| Offline reference data | **SUCD** — the SuperUnicode Character Database (SUAS-012) |

> **Relationship to SUTS-005 (SCSS).** SCSS answers *"is this character
> dangerous?"* at the level of a single codepoint: it detects homographs and
> script mixing for display and logging contexts. SUSCH answers *"is this
> token safe to compile?"* at the level of a whole identifier, and it adds the
> things a security filter cannot: the bidi override vector, the invisible
> character, the identifier-admissibility rule, and the chunk boundary
> analysis that makes a mixed-script finding meaningful. A toolchain that
> already links SCSS gains nothing by also linking SUSCH except the token
> grammar; a toolchain that must validate identifiers cannot do it with SCSS
> alone.

> The reference implementation ships as `include/suts/suts_susch.h` +
> `src/suts/suts_susch.c`. The library and function names (`suts_susch.h`,
> `suts_susch.c`, `suts_susch_get_attributes`,
> `suts_susch_validate_token_stream`) are stable API.

### 1.5 What SUSCH decides, and what it does not

SUSCH answers three questions about a token:

1. **Is this token an admissible identifier?** (§3.2, §6)
2. **Does this token contain a character that attacks the reader rather than
   the compiler?** (§3.6) — a directional control, an invisible character.
3. **Does this token *look* like something it is not?** (§3.5, §4.2) — a
   confusable, a mixed-script chunk, a chunk that crosses the Bridge
   boundary.

It does **not** decide whether a program is correct, whether an identifier is
well-named, or whether a script is the *right* one for a project. A
confusable is admissible with a caveat, not a rejection: SUSCH reports what
it found and lets the consuming policy decide.

### 1.6 Conformance and optionality

SUSCH is a **modular optional** specification. A conforming implementation
shall provide, for every codepoint in the 31-bit SuperUnicode address space:

* a total attribute query whose answer is never a lookup failure (§3.4);
* a token-stream validator that distinguishes defects from advisories
  (§6.2, §6.3);
* a zero-allocation path — no `malloc`, no `free`, no libc string
  routine, no static mutable state that grows.

An implementation that omits SUSCH is still conforming to the SuperUnicode
architecture; it simply declines to screen source text, and its source
tooling must then obtain the equivalent guarantees from the user-space
reference layer (SUCD) directly.

## 2. Scope of source code safety

### 2.1 The threat model

SUSCH assumes an adversary who can place arbitrary codepoints into a source
file, a string literal, a comment, or an identifier, and who wants one of
the following:

| Vector | What the reader sees | What the compiler reads | SUSCH's finding |
|--------|----------------------|-------------------------|-----------------|
| Right-to-left override (RLO) | `txt.exe` | `exe.txt` | `SUSCH_FLAG_BIDI_CONTROL`, defect |
| Zero-width space | `pay ment` | `pay ment` | `SUSCH_FLAG_INVISIBLE`, defect |
| Cyrillic homoglyph | `pауment` (Latin p, Cyrillic a) | a different identifier | `SUSCH_FLAG_HOMOGLYPH_RISK`, advisory |
| Mixed-script chunk | `pаyment` | one identifier, two scripts | `SUSCH_WARN_MIXED_SCRIPT`, advisory |
| Neography substitution | an unknown glyph | a native-plane identifier | `SUSCH_WARN_CROSS_ZONE`, advisory |
| Tag or format character | nothing at all | a hidden codepoint | `SUSCH_FLAG_INVISIBLE`, defect |

The first two are *defects*: a token that contains them is not a token a
compiler should accept. The rest are *advisories*: the token is well-formed,
and the finding tells the consuming toolchain what to look at.

### 2.2 Address-space zones

SUSCH classifies every codepoint into one of seven zones, mirroring the
three-space layout of SUTR-0 and the zone taxonomy of SUEPS (§5).

| Zone | Range | Meaning |
|------|-------|---------|
| BRIDGE | `0x00000000`–`0x0010FFFF` | The Unicode Compatibility Bridge: ordinary text. |
| SCP | `0x00110000`–`0x0011FFFF` | The System Control Plane: control markers, never source text. |
| RNUR | `0x00160000`–`0x0016FFFF` | The neography carve-out, taken out of the Native plane. |
| NATIVE | `0x00120000`–`0x7FFFFFEF` | The Native plane, less the RNUR carve-out. |
| TRAP | `0x7FFFFFF0`–`0x7FFFFFFE` | Trap slots: never valid. |
| SENTINEL | `0x7FFFFFFF` | The Sentinel: the last codepoint of the address space. |
| EXTSUCS | above `0x7FFFFFFF` | ExtSUCS plugin space: unbounded arbitrary precision, not source code. |

Only BRIDGE and RNUR carry source text. A token that contains a codepoint
from SCP, TRAP, SENTINEL, or EXTSUCS is rejected as an illegal zone.

## 3. The compiled source code safety schema

### 3.1 Table geometry and the lookup discipline

SUSCH answers every per-codepoint question from one of five static tables:

| Table | Row shape | Lookup | Rows shipped |
|-------|-----------|--------|--------------|
| Class table | `(first, last, flags)` | binary search over an inclusive span | 133 |
| Skeleton table | `(codepoint, prototype)` | binary search over a point key | 284 |
| Identifier table | `(first, last, class)` | binary search over an inclusive span | 113 |
| Script table | `(first, last, script)` | binary search over an inclusive span | 51 |
| Case table | `(first, last, case)` | binary search over an inclusive span | 22 |

Every table is `static const`, sorted in ascending order by its first key,
and non-overlapping. **Sortedness is a verification invariant, not a
convenience**: the lookup is a binary search, and an unsorted table would
silently answer wrongly rather than fail.

Each table has a compile-time capacity declared in the public header
(`SUSCH_RANGE_CAPACITY`, `SUSCH_SKELETON_CAPACITY`, `SUSCH_IDENT_CAPACITY`,
`SUSCH_SCRIPT_CAPACITY`, `SUSCH_CASE_CAPACITY`) and a compile-time assertion
that the shipped table fits. A data update that overflows a table fails the
build rather than the kernel.

The lookup depth is a constant of the build — `ceil(log2(rows))` probes — and
not a function of the address space. A codepoint query is therefore O(1) in
the size of the space, with a small fixed factor.

### 3.2 Identifier admission

Each codepoint carries a three-state admission verdict:

| Verdict | Meaning |
|---------|---------|
| `SUSCH_IDENT_ALLOWED` | Admissible in an identifier, with no caveat. |
| `SUSCH_IDENT_RESTRICTED` | Admissible in an identifier, but carries a caveat the caller should see. |
| `SUSCH_IDENT_DISALLOWED` | May never appear in an identifier. |

The verdict is computed from the compiled tables in this order:

1. **Not an identifier character.** A codepoint with no identifier class is
   `DISALLOWED`. The table records only positive rows, so the default is the
   safe answer. The one exception is the RNUR carve-out, whose class is
   derived positionally from the neography convention of §5.1 rather than
   from the table.
2. **Illegal zone.** A codepoint in SCP, TRAP, SENTINEL, or EXTSUCS is
   `DISALLOWED`.
3. **Outside the source zones.** A codepoint in the Native plane outside the
   RNUR is `DISALLOWED`: the Native plane is a project-private allocation
   space, and SUSCH admits it only through the neography carve-out (§5).
4. **Directional control.** A bidi control is `DISALLOWED`.
5. **Invisible.** An invisible character is `DISALLOWED`, with exactly one
   exception: the two joining controls U+200C and U+200D, which UTS #55
   §5.1.3 permits in identifiers under contextual restriction, and which are
   therefore `RESTRICTED`.
6. **Confusable prototype.** A codepoint whose skeleton prototype differs
   from itself is `RESTRICTED`: it is admissible, and it is the classic
   homoglyph risk (§3.5).
7. **Neography.** A codepoint in the RNUR is `RESTRICTED`: the identifier
   profile of a neography set is declared by the project at build time, not
   by the shipped table, so the table cannot vouch for it (§5).
8. Otherwise, `ALLOWED`.

`ALLOWED` means *admissible at some position in an identifier*, not *usable
as the first character*. The positional rule is separate, and is exposed
through `suts_susch_is_identifier_start` and
`suts_susch_is_identifier_continue`:

* **Start** — the codepoint's identifier class is start-capable.
* **Continue** — the codepoint's identifier class is continue-capable.

A decimal digit is `ALLOWED` and continue-capable but not start-capable.
An underscore is both. A combining mark is continue-capable only.

### 3.3 The character class table

The class table assigns each codepoint span a packed set of internal
properties. These are the facts the engine needs but that are not part of
the public attribute word:

| Property | Meaning |
|----------|---------|
| whitespace | a space of any kind |
| digit | a decimal digit |
| combining | a combining mark; attaches to the letter it follows |
| punctuation | any punctuation |
| punctuation (other) | punctuation that is not `Po`; bounds a snake chunk |
| quotation | a quotation mark |
| bidi | a directional formatting character |
| invisible | zero-width or otherwise invisible |
| ignorable | a default-ignorable codepoint |
| line break | a line terminator candidate |
| joining | a joining control (U+200C, U+200D) |
| risk | a curated high-risk visual form |

The last property deserves a note. The skeleton table (§3.5) answers
"does this codepoint fold onto a different prototype?". The risk property
answers a broader question: "is this codepoint drawn in a way that has been
used to deceive?". It covers the mathematical alphanumeric symbols
(`0x1D400`–`0x1D7FF`), the letterlike symbols (`0x2100`–`0x214F`), the
fullwidth Latin forms, the general punctuation blocks, and the CJK
ideographic iteration marks — the shapes that carry no prototype of their own
but are visually adjacent to one.

### 3.4 The source attribute

The public answer to "what are the source code safety properties of this
codepoint?" is a fixed-size record the caller owns:

```c
typedef struct {
    uint32_t codepoint;         /* Raw 31-bit SuperUnicode codepoint */
    uint32_t security_mask;     /* Packed source code safety attributes */
    uint16_t script_category;   /* Associated script classification */
    uint8_t  identifier_status; /* Allowed, Restricted, or Disallowed */
    uint8_t  reserved;          /* must be zero */
} susch_source_attr_t;
```

The layout is exactly 12 bytes, byte-aligned, and is asserted at compile
time. The four sketch-named flags occupy bits 0–3; the derived flags occupy
bits 4–12; bit 13 is reserved for future use; the mask of *defined* bits is
`SUSCH_FLAG_MASK_DEFINED`.

| Flag | Bit | Meaning |
|------|-----|---------|
| `SUSCH_FLAG_VALID_IDENTIFIER` | 0 | Admissible in an identifier |
| `SUSCH_FLAG_BIDI_CONTROL` | 1 | A directional formatting character |
| `SUSCH_FLAG_INVISIBLE` | 2 | Zero-width or otherwise invisible |
| `SUSCH_FLAG_HOMOGLYPH_RISK` | 3 | A high-risk confusable profile |
| `SUSCH_FLAG_LINE_BREAK` | 4 | A line terminator candidate |
| `SUSCH_FLAG_JOINING_CONTROL` | 5 | U+200C or U+200D |
| `SUSCH_FLAG_WHITESPACE` | 6 | Horizontal or vertical space |
| `SUSCH_FLAG_PUNCTUATION` | 7 | Any punctuation |
| `SUSCH_FLAG_DIGIT` | 8 | A decimal digit |
| `SUSCH_FLAG_COMBINING` | 9 | A combining mark |
| `SUSCH_FLAG_IGNORABLE` | 10 | A default-ignorable codepoint |
| `SUSCH_FLAG_NATIVE_PLANE` | 11 | The Native or neography plane |
| `SUSCH_FLAG_QUOTATION` | 12 | A quotation mark |

The query is **total**: every codepoint from `0x00000000` through the
Sentinel `0x7FFFFFFF` has an answer, and an unclassified codepoint is
answered by the algorithmic default rather than by a lookup failure. The
query returns `false` — leaving the output untouched — in exactly two cases:
a NULL output pointer, or a codepoint above the Sentinel, which is ExtSUCS
plugin space and is not source code.

### 3.5 The skeleton table

The skeleton table is what turns "this character looks like a Latin letter"
into a machine-checkable fact. Each row is a confusable codepoint and the
canonical prototype a reader's eye substitutes for it:

```
U+0430 CYRILLIC SMALL LETTER A   →  U+0061 LATIN SMALL LETTER A
U+03B1 GREEK SMALL LETTER ALPHA   →  U+0061 LATIN SMALL LETTER A
U+0435 CYRILLIC SMALL LETTER IE   →  U+0065 LATIN SMALL LETTER E
U+FF21 FULLWIDTH LATIN CAPITAL A  →  U+0041 LATIN CAPITAL LETTER A
```

Two consequences follow:

* `suts_susch_skeleton_of(cp)` returns the prototype, or `cp` itself when
  the codepoint has none.
* `suts_susch_confusable_with(a, b)` is true when two distinct codepoints
  share a prototype — that is, when they are visual homographs of one
  another.

The fullwidth Latin fold (U+FF21–U+FF3A to A–Z, U+FF41–U+FF5A to a–z) is
computed arithmetically rather than stored, so it costs no table space.

A codepoint whose prototype differs from itself carries
`SUSCH_FLAG_HOMOGLYPH_RISK` and the `RESTRICTED` identifier verdict. It is
*not* rejected: a legitimate Greek or Cyrillic identifier must still
compile. The finding is what a linter or a CI gate acts on.

### 3.6 Bidi controls and invisibles

The bidi control inventory is the explicit controls (U+202A LRE, U+202B RLE,
U+202C PDF, U+202D LRO, U+202E RLO), the implicit marks (U+200E LRM,
U+200F RLM, U+061C ALM), and the isolate initiators and terminator (U+2066
LRI, U+2067 RLI, U+2068 FSI, U+2069 PDI). Every one of them is invisible,
and every one of them is a defect.

The invisible inventory covers the soft hyphen (U+00AD), the combining
grapheme joiner (U+034F), the Hangul fillers (U+115F–U+1160), the zero-width
space (U+200B), the word joiner (U+2060), the invisible operators
(U+206A–U+206F), the Hangul fixed filler (U+3164), the variation selectors
(U+FE00–U+FE0F), the zero-width no-break space (U+FEFF), the halfwidth
Hangul filler (U+FFA0), the interlinear annotation separators
(U+FFF9–U+FFFB), the shorthand format letters (U+1BCA0–U+1BCA3), the musical
formatting characters (U+1D173–U+1D17A), and the language tags
(U+E0000–U+E0FFF).

Both classes are detected in O(1) by a probe of the class table, and both
are disqualifying inside a token. The exception is the pair of joining
controls (§3.2, rule 5).

## 4. Identifier chunk analysis

### 4.1 Case classes

Chunk boundaries are driven by case. SUSCH classifies each codepoint into
one of five case classes:

| Class | Meaning |
|-------|---------|
| none | Unicameral, or a letter with no case distinction |
| lower | A lowercase letter |
| upper | An uppercase letter |
| title | A titlecase letter outside the Greek script |
| title (Greek) | A titlecase letter of the Greek script |

The distinction between the last two matters: UTS #55 §5.1.2.1 excludes the
Greek titlecase letters from the camel rule, because a Greek capital is a
titlecase letter whose capitalization is not a word boundary in the same
sense.

Scripts that are unicameral — Arabic, Hebrew, Devanagari, Bengali, Thai,
Han, Hiragana, Katakana, Hangul, Armenian, Georgian, Syriac — are
deliberately absent from the case table. For them the case class is *none*,
so no case-driven boundary is ever invented. Latin Extended-A alternates
upper and lower case by parity; that single rule covers all fifty-six
letters of the block, and is applied before the table is consulted.

### 4.2 Chunk boundaries

The chunk boundaries are the camel, hat, and snake boundaries of UTS #55
§5.1.2.1, expressed in terms of the compiled case classes:

| Rule | Condition | Boundary |
|------|-----------|----------|
| CAMEL | `([Ll] \| [Lt & −Greek])` ÷ `([Lu] \| [Lt])` | between a lowercase (or non-Greek titlecase) letter and an uppercase or titlecase letter |
| HAT | ÷ `([Lu] \| [Lt] × [Ll])` | before an uppercase or titlecase letter that is followed by a lowercase letter |
| HAT | ÷ `[Lt & −Greek]` | before a non-Greek titlecase letter |
| SNAKE | ÷ `[P & −Po]` and `[P & −Po]` ÷ | on either side of a punctuation character that is not `Po` |

A combining mark never forms a chunk of its own: it belongs to the letter
it follows.

Worked examples:

| Token | Chunks | Rule |
|-------|--------|------|
| `HTTPRequest` | `HTTP` \| `Request` | HAT before `R` |
| `TypeII` | `Type` \| `II` | CAMEL before `I` |
| `OCaml` | `O` \| `Caml` | HAT before `C` |
| `aB` | `a` \| `B` | CAMEL before `B` |
| `snakeELEPHANTSnake` | `snake` \| `ELEPHANT` \| `Snake` | CAMEL then HAT |
| `my_variable` | `my` \| `_` \| `variable` | SNAKE on both sides |
| `abc` | `abc` | no rule fires |

The hat rule needs to see the codepoint *after* the one it applies to, so
the boundary decision is taken with a one-codepoint lookahead. The engine
holds two chunk-significant codepoints in registers and never re-reads the
stream: the pass stays linear (§6.1).

### 4.3 Script classification

Each codepoint carries a coarse script classification. The set is
deliberately small — it exists to tell a single-script chunk from a visibly
mixed one, not to reproduce the full Unicode `Script` property:

`COMMON`, `LATIN`, `GREEK`, `CYRILLIC`, `ARMENIAN`, `HEBREW`, `ARABIC`,
`SYRIAC`, `DEVANAGARI`, `BENGALI`, `THAI`, `GEORGIAN`, `HAN`, `HIRAGANA`,
`KATAKANA`, `HANGUL`, `NATIVE`, `NEOGRAPHY`, `INHERITED`, `UNKNOWN`.

`COMMON` covers the script-neutral codepoints — digits, punctuation,
symbols. `INHERITED` covers combining marks, which take the script of their
base. `NATIVE` and `NEOGRAPHY` cover the Native plane and the RNUR
carve-out respectively. A codepoint the curated table does not classify is
`UNKNOWN`.

A chunk is **mixed-script** when its distinct scripts, ignoring `COMMON`,
`INHERITED`, and `UNKNOWN`, number more than one. The chunk evidence is
accumulated in a 32-bit bitmap, so the test is a popcount on a register
value.

Note that the camel boundary can *separate* two scripts — `μB` (Greek mu,
Latin B) is two single-script chunks, and is therefore not a mixed-script
finding. That is the correct UTS #55 behaviour, and it is exactly why the
homoglyph warning is a separate mechanism: the confusable pair is still
reported, by the skeleton table (§3.5), even when the chunk analysis has
already split it.

## 5. Zones and the neography carve-out

The zone taxonomy of §2.2 is exposed through `suts_susch_zone_of`, which
answers in O(1) by a sequence of range comparisons.

### 5.1 The RNUR convention

The RNUR neography carve-out (`0x00160000`–`0x0016FFFF`) is where a
project's own conlang scripts live. SUSCH admits those characters as
identifier material, but the *profile* of a neography set is declared by the
project at build time, not by the shipped table — so SUSCH applies a fixed
positional convention instead of inventing per-set data:

> **RNUR identifier convention.** Every `0x100`-aligned block of the RNUR is
> one neography set. Within a set, the codepoint offsets `0x00`–`0x5F` are
> letter positions (start-capable and continue-capable), the offsets
> `0x60`–`0x7F` are continuation-only positions, and the offsets `0x80`–`0xFF`
> are symbol positions, which may never appear in an identifier.

The convention is positional, so it needs no table and no build-time data:
every neography set gets the same shape, and a project that needs a
different shape declares it in its own layer. The set number of a neography
coordinate is `(cp - 0x00160000) >> 8`, exposed through
`suts_susch_neography_set`.

A neography character is `RESTRICTED` rather than `ALLOWED` (§3.2, rule 7):
the shipped table cannot vouch for a profile it does not carry.

### 5.2 The Bridge boundary

The boundary SUSCH exists to protect a source file from is the one between
the Unicode Compatibility Bridge and the Native plane.
`suts_susch_crosses_bridge_boundary(a, b)` answers whether a pair of
codepoints straddles it, in O(1).

Within an identifier chunk, the crossing is reported as
`SUSCH_WARN_CROSS_ZONE`. A chunk that mixes a Bridge-script letter with a
neography letter is *also* a mixed-script chunk, so both advisories fire;
they are complementary, and a consumer that cares only about the zone
boundary reads only the first.

## 6. The token-stream validator

### 6.1 The single linear pass

`suts_susch_validate_token_stream(stream, length)` walks the codepoint
stream exactly once. During that walk it accumulates, for the whole token:

* the outcome code (§6.2),
* the index of the offending codepoint,
* the advisory bitmap (§6.3),
* the identifier chunk count, and
* the per-chunk script evidence.

There is no second pass, no re-lex, and no scratch buffer. The only state
the pass holds is two chunk-significant codepoints, one chunk script
bitmap, and two boolean chunk flags — a fixed number of registers, whatever
the token's length.

The same pass backs `suts_susch_chunk_count` and
`suts_susch_is_mixed_script`, so screening a token for a verdict, for its
chunk count, and for its script evidence costs one traversal, not three.

The complexity is O(n) in the length of the token and O(1) in the size of
the address space. There is no per-file setup, because there is nothing to
load: the tables are part of the image.

### 6.2 Outcome codes

The validator distinguishes defects from advisories. A defect makes the
token invalid; an advisory never does.

| Outcome | Meaning |
|---------|---------|
| `SUSCH_TOKEN_OK` | The stream is a valid identifier. |
| `SUSCH_TOKEN_ERR_NULL` | A NULL stream with a non-zero length. |
| `SUSCH_TOKEN_ERR_EMPTY` | A zero-length identifier. |
| `SUSCH_TOKEN_ERR_ILLEGAL_ZONE` | A trap slot, the Sentinel, the SCP, or ExtSUCS. |
| `SUSCH_TOKEN_ERR_BIDI_CONTROL` | A directional formatting character. |
| `SUSCH_TOKEN_ERR_INVISIBLE` | A zero-width or invisible character. |
| `SUSCH_TOKEN_ERR_LINE_BREAK` | A line terminator inside the token. |
| `SUSCH_TOKEN_ERR_INVALID_START` | The first codepoint may not start an identifier. |
| `SUSCH_TOKEN_ERR_INVALID_CONTINUE` | A later codepoint may not continue one. |

The defect checks run in a fixed order, and the first one that fires is the
outcome — so the outcome is the *most specific* finding available:

1. illegal zone,
2. bidi control,
3. invisible character,
4. line break,
5. invalid start,
6. invalid continuation.

The two joining controls are exempt from step 3 (§3.2, rule 5) and fall
through to the identifier rules, where they are admissible as continuations
and are reported through the advisory channel instead.

### 6.3 Advisory findings

The advisory bitmap never changes the verdict. It carries four bits:

| Advisory | Bit | Meaning |
|----------|-----|---------|
| `SUSCH_WARN_HOMOGLYPH` | 0 | A confusable prototype is present in the token. |
| `SUSCH_WARN_MIXED_SCRIPT` | 1 | A chunk spans more than one script. |
| `SUSCH_WARN_CROSS_ZONE` | 2 | A chunk mixes a Bridge and a Native character. |
| `SUSCH_WARN_RESTRICTED` | 3 | A `RESTRICTED` character is present in the token. |

The mask of defined bits is `SUSCH_WARN_MASK_DEFINED`. The division of
labour follows UTS #55: a homoglyph or a mixed-script chunk is a warning,
while a directional control or an invisible character is a defect.

## 7. Complexity

| Operation | Cost | Allocation |
|-----------|------|------------|
| `suts_susch_get_attributes` | O(1): ≤ 8 probes per table | none |
| `suts_susch_skeleton_of` | O(1): ≤ 9 probes | none |
| `suts_susch_confusable_with` | O(1): two skeleton folds | none |
| `suts_susch_zone_of` | O(1): a comparison chain | none |
| `suts_susch_is_identifier_start` / `_continue` | O(1): a table probe | none |
| `suts_susch_validate_token_stream` | O(n) in the token length | none |
| `suts_susch_chunk_count` | O(n) in the token length | none |
| `suts_susch_is_mixed_script` | O(n) in the token length | none |
| `suts_susch_init_engine` | O(1) | none |

No operation in the module allocates, grows a table, mutates static state,
or calls into libc. The engine is ready the moment the image is mapped;
`suts_susch_init_engine` exists so a toolchain can force initialization at
a deterministic point, and so that a future revision with build-time
generated tables keeps a stable entry point.

## 8. The curated dataset

The tables shipped with the reference implementation are a curated subset of
the SUCD reference layer. The curation is deliberate: the shipped tables
must be small enough to embed in a kernel image, sorted, non-overlapping,
and reviewable.

| Dataset | Ships as | SUCD source |
|---------|----------|-------------|
| Character classes | 133 range rows | The general-category and property manifests |
| Confusables | 284 point rows + an arithmetic fullwidth fold | The skeleton/confusables manifest |
| Identifier classes | 113 range rows | The identifier-status manifests |
| Script classification | 51 range rows | The script property manifest |
| Case classes | 22 range rows | The case property manifest |

Extending a table is a data update — a minor revision of this document —
and never changes the API.

## 9. Worked example

Screening the token `pаyment`, where the second character is Cyrillic
U+0430 rather than Latin U+0061:

```
stream   = [ 'p', U+0430, 'y', 'm', 'e', 'n', 't' ]
length   = 7
```

The single pass produces:

| Step | Observation |
|------|-------------|
| Per-codepoint attribute of U+0430 | `VALID_IDENTIFIER \| HOMOGLYPH_RISK`, script CYRILLIC, status `RESTRICTED` |
| Skeleton of U+0430 | U+0061 — a different prototype, so the homoglyph risk fires |
| Zone of every codepoint | BRIDGE — no cross-zone finding |
| Identifier admission | Every codepoint is start-capable or continue-capable |
| Chunk boundaries | No camel, hat, or snake rule fires: the chunk is one chunk |
| Chunk scripts | LATIN and CYRILLIC in the same chunk — the mixed-script finding fires |
| Outcome | `SUSCH_TOKEN_OK` — the token is a valid identifier |
| Advisories | `HOMOGLYPH \| MIXED_SCRIPT \| RESTRICTED` |

The token compiles. The advisories say why a reviewer should look at it.

For contrast, the token `'p', U+0430, U+202E, 'y', 'm', 'e', 'n', 't'` — the
same substitution with a right-to-left override inserted after the
substituted character — is rejected: the outcome is
`SUSCH_TOKEN_ERR_BIDI_CONTROL`, and the offending index is reported.

## Annex A - Compliance matrix (reference implementation)

| Requirement | Reference status |
|-------------|------------------|
| Fixed-size source attribute (12 bytes) | Yes |
| Zero heap allocation on any path | Yes |
| O(1) attribute query per codepoint | Yes |
| Total attribute query over the 31-bit space | Yes |
| Sorted, non-overlapping compiled tables | Yes |
| Compile-time capacity assertions | Yes |
| Bidi control detection (explicit, implicit, isolate) | Yes |
| Invisible / zero-width detection | Yes |
| Homoglyph risk and confusable pairing | Yes |
| Three-state identifier admission | Yes |
| Start and continue admission rules | Yes |
| UTS #55 §5.1.2.1 chunk boundaries | Yes |
| Mixed-script chunk detection | Yes |
| Bridge / Native cross-zone detection | Yes |
| RNUR neography carve-out with a positional convention | Yes |
| Single linear pass over the token stream | Yes |
| Defects distinguished from advisories | Yes |
| Modular optionality (module omittable) | Yes |
| 31-bit SUCS and unbounded ExtSUCS compatibility | Yes |

## Annex B - Flag and warning bit assignments

| Bit | Attribute flag | Warning flag |
|-----|----------------|--------------|
| 0 | `SUSCH_FLAG_VALID_IDENTIFIER` | `SUSCH_WARN_HOMOGLYPH` |
| 1 | `SUSCH_FLAG_BIDI_CONTROL` | `SUSCH_WARN_MIXED_SCRIPT` |
| 2 | `SUSCH_FLAG_INVISIBLE` | `SUSCH_WARN_CROSS_ZONE` |
| 3 | `SUSCH_FLAG_HOMOGLYPH_RISK` | `SUSCH_WARN_RESTRICTED` |
| 4 | `SUSCH_FLAG_LINE_BREAK` | — |
| 5 | `SUSCH_FLAG_JOINING_CONTROL` | — |
| 6 | `SUSCH_FLAG_WHITESPACE` | — |
| 7 | `SUSCH_FLAG_PUNCTUATION` | — |
| 8 | `SUSCH_FLAG_DIGIT` | — |
| 9 | `SUSCH_FLAG_COMBINING` | — |
| 10 | `SUSCH_FLAG_IGNORABLE` | — |
| 11 | `SUSCH_FLAG_NATIVE_PLANE` | — |
| 12 | `SUSCH_FLAG_QUOTATION` | — |

The attribute word is 32 bits wide and the mask of defined bits is
`SUSCH_FLAG_MASK_DEFINED` (`0x1FFF`). The warning bitmap is 32 bits wide
and the mask of defined bits is `SUSCH_WARN_MASK_DEFINED` (`0x000F`).

## References

- **UTS #55** - Unicode Source Code Handling (the user-space spec this kernel
  primitive deliberately departs from)
- **UTS #39** - Unicode Security Mechanisms (the General Security Profile and
  the confusable skeleton model)
- **SUTR-0 (SUCS Core)** - the three-space layout, SCP, traps, and Sentinel
- **SUTR-1 (SUTF)** - the "No Surrogates" invariant
- **SUTR-6 (Unicode Compatibility Bridge)** - the permanent 1:1 guarantee
- **SUTR-7 (SUCEM)** - the four-level character encoding model
- **SUTR-8 (SUCPM)** - the RNUR registry space and property tiers
- **SUTR-10 (SUCM)** - the conformance model and the zero-allocation mandate
- **SUTR-11 (SUMS)** - Mongolian shaping and vertical direction
- **SUTS-005 (SCSS)** - character security, the sibling kernel primitive
  SUSCH composes with
- **SUTS-006 (SIDN)** - identifier and label handling
- **SUTS-007 (SUEPS)** - emoji presentation, the sibling kernel primitive
  SUSCH composes with
- **SUAS-005 (SSP)** - the script property engine
- **SUAS-012 (SUCD)** - SuperUnicode Character Database (the offline reference
  layer, and the repository's `SUCD`, *not* the SUAS-014 of the seeding brief)

---

**END OF SUTS-008**


