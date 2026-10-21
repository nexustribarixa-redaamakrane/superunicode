# SUAS-015 — SuperUnicode Arabic Mark Rendering (SUAMR)

**Standard ID:** SUAS-015
**Status:** Draft — Ratified
**Scope:** Core Kernel Invariant
**Applies To:** SUCD, UAX #53 (Unicode Arabic Mark Rendering analog), SUAS-005 (SSP), SUAS-006 (STS), SUAS-009 (SIP), SUAS-011 (SCMS), SUAS-012 (SUCD), SUAS-013 (SISR), SUAS-014 (SUVTL), SUTS-001 (SUCA), SUTS-003 (SRE), SUTS-005 (SCSS), the SUF font engine, the OpenWindows shell, the System Control Plane
**Last Updated:** 2026-10-21

---

## 1. Overview

SuperUnicode Arabic Mark Rendering (SUAMR) is the core algorithmic subsystem
governing the **zero-allocation, O(1) resolution of an Arabic combining mark's
stacked layout** — where the mark sits relative to its base, how deep it stacks,
and the deterministic transient order in which a cluster's marks are displayed.

Where a traditional Unicode environment applies UAX #53, "Unicode Arabic Mark
Rendering", through an **OpenType shaping engine**, dynamically parsed font
metrics and runtime heuristics, SUAMR compiles the same result into an
**immutable, byte-packed, static table**. A terminal emulator, a microkernel
text renderer or a GUI pipeline can lay out complex right-to-left text without a
single heap allocation.

SUAMR is the runtime counterpart of UAX #53 in the same **dual-layer**
relationship that SUAS-009 (SIP) has with UAX #38, SUAS-013 (SISR) has with
UAX #45, and SUAS-014 (SUVTL) has with UAX #50:

- the **offline reference layer** is the SuperUnicode Character Database
  (SUCD, SUAS-012): the Arabic mark rendering rules, the
  **Modifier_Combining_Mark (MCM)** list, the stacking precedence classes, and
  the human-readable positioning manifests for font engineers, shaping engines
  and developer toolchains;
- the **runtime kernel layer** is this specification: a byte-packed
  mark-layout record, a fixed open-addressing index, and a bounded query surface.

The brief named the offline layer **"SUCA"**. In this corpus **SUCA is already
the SuperUnicode Collation Algorithm (SUTS-001)**, so SUAMR names **SUCD
(SUAS-012)** as its reference layer — exactly as SUAS-009 (SIP), SUTS-005
(SCSS), SUAS-013 (SISR) and SUAS-014 (SUVTL) already do.

### 1.1 Architectural shifts from UAX #53

| UAX #53 / shaping-engine world | SUAMR world |
|--------------------------------|-------------|
| Glyph positioning resolved by an OpenType/GSUB/GPOS shaper | A 12-byte static record read from a fixed table |
| Font metrics parsed at run time | Pre-computed fixed-point offsets, compiled at build time |
| Heap-allocated shaping buffers | Zero allocation; the caller owns every buffer |
| Runtime heuristics per engine | One deterministic algorithm (§5), byte-identical everywhere |
| "Reorder to NFD, then adjust" left to the shaper | A named, bounded, in-place cluster pass with a CGJ override |

### 1.2 Goals

1. **A single record.** A mark's layout is a fixed 12-byte descriptor, not a
   metric parsed from a font table.
2. **A bounded lookup.** A query hashes the codepoint, probes a fixed index, and
   returns the record — O(1) expected, a small constant number of probes, no
   allocation, no libc.
3. **A deterministic order.** A mark cluster is reordered by the AMTRA of §5:
   NFD-stable, canonically-equivalent sequences render identically, and a
   combining grapheme joiner overrides the default.
4. **A total property.** Every mark in a text space resolves — curated or not —
   by the algorithmic default of §6.
5. **A general engine.** The same stacking and mark-positioning engine extends
   to complex conscript abugidas through the Native & Neography Plane
   (`0x160000–0x16FFFF`).

### 1.3 Non-negotiable invariants

- **SUAMR-I1 — Zero allocation.** No entry point calls `malloc`, `free`, or any
  libc allocator. Every buffer is caller-owned or static.
- **SUAMR-I2 — O(1) lookup.** `suas_suamr_get_mark_layout` and the mark
  predicates run in O(1) expected time, independent of the table size.
- **SUAMR-I3 — Fixed record.** `suamr_mark_layout_t` is exactly 12 bytes, with
  no padding, on every supported ABI (§3.1).
- **SUAMR-I4 — Reserved bits are zero.** The upper eight bits of
  `positioning_flags` and the `reserved` byte are zero
  (`SUAMR_REGISTRY_BAD_FLAGS`, `SUAMR_REGISTRY_BAD_RESERVED`).
- **SUAMR-I5 — One stacking side.** A mark stacks above or below, never both and
  never neither (`SUAMR_REGISTRY_STACK_CONTRADICTION`).
- **SUAMR-I6 — Ordered and unique.** Records are ascending by codepoint and
  unique by codepoint (`SUAMR_REGISTRY_UNORDERED`,
  `SUAMR_REGISTRY_DUPLICATE_CODEPOINT`).
- **SUAMR-I7 — Custom class is a neography right.** `SUAMR_FLAG_CUSTOM_CLASS`
  appears only on an RNUR record (`SUAMR_REGISTRY_CUSTOM_CLASS_MISMATCH`).
- **SUAMR-I8 — Totality.** `suas_suamr_get_mark_layout` returns `true` for every
  mark in a text space, curated or not (§6).
- **SUAMR-I9 — Deterministic reordering.** The AMTRA pass is stable, reorders
  only in place, names every failure, and never crosses a combining grapheme
  joiner (§5).

### 1.4 Deviations from the reference sketch

1. **Classification.** The seeding sketch labelled SUAMR **SUAS-015**, and the
   series owner ratified it as a **SUAS** core invariant. It is registered as
   **SUAS-015**, the number immediately after SUAS-014 (SUVTL), keeping the SUAS
   series contiguous with no reserved gaps (SUAS-010 §11.1). The registry
   *contents* are optional version data; the engine, record layout and
   reordering rules are mandatory architecture.
2. **A stale numbering reference.** The sketch named the offline layer "SUCA".
   In this corpus SUCA is the SuperUnicode Collation Algorithm (SUTS-001), so
   SUAMR names **SUCD (SUAS-012)** (§1, §7).
3. **Macro prefix.** Spec-level constants carry the `SUAS_SUAMR_*` prefix
   (`SUAS_SUAMR_SPEC_ID`, `SUAS_SUAMR_REGISTRY_CAPACITY`, …); the sketch's
   flag and type names (`SUAMR_FLAG_*`, `suamr_mark_layout_t`,
   `suas_suamr_get_mark_layout`) are preserved verbatim.
4. **The canonical combining class lives beside the record.** The sketch's
   12-byte record has no ccc field. Rather than widen it, SUAMR keeps the class
   in a **parallel table** aligned with the layout table (§3.2). This preserves
   the exact byte layout the brief specified while carrying the 220/230 classes
   AMTRA needs.
5. **A base-independent record in 1.0.** The sketch's signature
   `suas_suamr_get_mark_layout(base, mark, *out)` is preserved verbatim, but the
   1.0 layout is a property of the *mark alone*; `base_codepoint` is accepted
   and ignored. The pair hash (§3.4) is nonetheless order-sensitive, so a future
   base-dependent revision can key on the pair without an API change.
6. **An added orthogonal flag set.** The sketch's four flags (STACK_ABOVE,
   STACK_BELOW, STAGGER, CUSTOM_CLASS) are kept; SUAMR adds MCM, SHADDA,
   KASRA_LIKE and TRANSPARENT so a renderer can distinguish roles the AMTRA
   depends on (§3.3).
7. **A total property.** UAX #53 leaves an absent mark undefined. SUAMR requires
   a **total** answer for a well-formed mark and supplies the algorithmic default
   of §6.

---

## 2. Definitions and Notation

### 2.1 Definitions

- **Combining Character Sequence (D56).** A starter followed by one or more
  combining marks. SUAMR does not compute the boundary; it consumes one cluster.
- **Canonical_Combining_Class (ccc).** The Unicode property of Table 1 (§4).
  ccc=0 is a starter; non-zero classes order marks.
- **Starter.** A codepoint with ccc=0: a base letter, or the combining grapheme
  joiner.
- **Modifier Combining Mark (MCM).** A combining mark that binds tightly to the
  base and should render *closer to the base* than the tashkil (§4). The set is
  defined by `PropList.txt` (UAX #53 §3.1) and curated here as
  `SUAMR_FLAG_MCM`.
- **Tashkil.** The supplementary diacritics (the short vowels and their
  variants). Rendered outward from an MCM.
- **AMTRA.** The Arabic Mark Transient Reordering Algorithm (§5): NFD, then move
  certain marks closer to the base.
- **Combining grapheme joiner (CGJ, U+034F).** A starter used to pin an
  exceptional display order; a barrier the algorithm never crosses (§5.3).
- **Stacking level.** The depth of a mark from its base: 0 is the base-most mark
  (the shadda), increasing outward.
- **Algorithmic default.** The layout synthesized for an uncurated mark from its
  block (§6).
- **RNUR set number.** For a neography coordinate,
  `set = (cp − 0x00160000) >> 8` (SUAS-005 §5.4) — an O(1) decode.

### 2.2 Notational conventions

`U+` names a Bridge-plane Unicode codepoint; a bare hexadecimal value names a
Base SUCS codepoint. `cp` is a 31-bit Base SUCS value (`0x00000000–0x7FFFFFFF`).
A value above `SUAS_CODEPOINT_MAX` is ExtSUCS and is outside the registry. ccc
values are UAX #53's own; the macros `SUAMR_CCC_*` are their binary encodings.

### 2.3 The address-space zones

| Zone | Range | Text space? |
|------|-------|-------------|
| `SUAS_SUAMR_ZONE_BRIDGE` | `0x00000000–0x0010FFFF` | yes |
| `SUAS_SUAMR_ZONE_SCP` | `0x00110000–0x0011FFFF` | no — Control Plane |
| `SUAS_SUAMR_ZONE_RNUR` | `0x00160000–0x0016FFFF` | yes — neography |
| `SUAS_SUAMR_ZONE_NATIVE` | rest of `0x00120000–0x7FFFFFEF` | yes — custom conscript space |
| `SUAS_SUAMR_ZONE_TRAP` | `0x7FFFFFF0–0x7FFFFFFE` | no |
| `SUAS_SUAMR_ZONE_SENTINEL` | `0x7FFFFFFF` | no |
| `SUAS_SUAMR_ZONE_EXTSUCS` | `> 0x7FFFFFFF` | no — plugin space |

`suas_suamr_is_text_space` is true for Bridge, RNUR and Native only.

---

## 3. The Record and the Index

### 3.1 The mark-layout record

```c
typedef struct {
    uint32_t codepoint;         /* Combining mark codepoint                 */
    int16_t  x_offset;          /* Fixed-point horizontal displacement       */
    int16_t  y_offset;          /* Fixed-point vertical displacement         */
    uint16_t positioning_flags; /* Packed stacking and rendering flags       */
    uint8_t  stacking_level;    /* Stacking depth index for multi-mark runs  */
    uint8_t  reserved;          /* must be zero                              */
} suamr_mark_layout_t;
```

The record is exactly **12 bytes** on every supported ABI, with no padding. Its
field offsets are `0, 4, 6, 8, 10, 11`, proven by compile-time assertion.

### 3.2 The canonical combining class table

The record carries no ccc field. The class of a curated mark is the byte at the
same index in a **parallel table** `suamr_ccc_table[]`, whose length is proven
equal to the layout table's at compile time. This keeps the record at the
sketch's exact 12 bytes while carrying the 220/230 generalized classes that
AMTRA depends on.

For an **uncurated** mark the class is derived from its block by the
algorithmic default of §6.

### 3.3 The positioning flags

| Flag | Bit | Meaning |
|------|-----|---------|
| `SUAMR_FLAG_STACK_ABOVE` | `1<<0` | Stack the mark vertically above the base |
| `SUAMR_FLAG_STACK_BELOW` | `1<<1` | Stack the mark vertically below the base |
| `SUAMR_FLAG_STAGGER` | `1<<2` | Apply a horizontal/vertical stagger adjustment |
| `SUAMR_FLAG_CUSTOM_CLASS` | `1<<3` | Uses extended neography positioning |
| `SUAMR_FLAG_MCM` | `1<<4` | A Modifier Combining Mark (§4) |
| `SUAMR_FLAG_SHADDA` | `1<<5` | The shadda (ccc=33) |
| `SUAMR_FLAG_KASRA_LIKE` | `1<<6` | A kasra / kasra-like ligating mark |
| `SUAMR_FLAG_TRANSPARENT` | `1<<7` | Renders directly over the base |

`SUAMR_FLAG_ASSIGNED_MASK == 0x00FF`. The upper eight bits are reserved and must
be zero.

### 3.4 The offsets

`x_offset` and `y_offset` are signed fixed-point displacements in **1/64 em**
(`SUAMR_OFFSET_SCALE == 64`). Zero is the default; a positive `y_offset` moves
the mark toward the top of the cell, a positive `x_offset` toward the end of the
line.

### 3.5 The hash

SUAMR uses **FNV-1a 32-bit** (offset `2166136261`, prime `16777619`) over the
four little-endian bytes of the codepoint. `suas_suamr_hash_pair` hashes the
`(base, mark)` pair in that order, so it is order-sensitive: `(base, mark)` and
`(mark, base)` hash differently.

### 3.6 The index

A fixed open-addressing array of `SUAS_SUAMR_INDEX_SIZE == 512` slots, holding at
most `SUAS_SUAMR_REGISTRY_CAPACITY == 256` curated records, is built lazily on
first use. The load factor stays below one-half, so a probe is a small bounded
sequence. Construction is idempotent.

---

## 4. The Canonical_Combining_Class Values of Table 1

UAX #53 Table 1 assigns eleven different non-zero ccc values to Arabic script
combining marks, plus two generalized classes:

| ccc | Combining marks in this class |
|-----|-------------------------------|
| 0 | Combining grapheme joiner, combining alef overlay |
| 27 | Fathatan, open fathatan |
| 28 | Dammatan, open dammatan |
| 29 | Kasratan, open kasratan |
| 30 | Fatha, small fatha |
| 31 | Damma, small damma |
| 32 | Kasra, small kasra |
| 33 | Shadda |
| 34 | Sukun |
| 35 | Superscript alef |
| 220 | All other below combining marks except small low noon with kasra |
| 230 | All other above combining marks, small low noon with kasra |

The macros `SUAMR_CCC_STARTER`, `SUAMR_CCC_FATHATAN`, … `SUAMR_CCC_ABOVE` carry
these values.

### 4.1 The Modifier Combining Marks (MCM)

The MCM set is the UAX #53 §3.1 group: marks that modify the base and should
render closer to it than the tashkil. SUAMR curates the set with
`SUAMR_FLAG_MCM`; the reference roster is §13. The set is **stable** — adding an
existing character would change the rendering of data that assumed AMTRA;
additional characters may be added only when they are encoded.

---

## 5. The Arabic Mark Transient Reordering Algorithm

AMTRA reorders one Combining Character Sequence so that the **inside-out**
rendering rule displays the marks in the expected visual order, so that
canonically equivalent sequences render identically, and so that a caller can pin
an exceptional order with a combining grapheme joiner.

`suas_suamr_reorder_cluster(codepoints, count, *out_status)` applies it **in
place**, allocating nothing.

### 5.1 Input and output

- **Input:** a Combining Character Sequence containing one or more Arabic
  combining marks.
- **Output:** a canonically equivalent sequence reordered for inside-out
  stacking.

The first codepoint must be a **starter** (ccc=0): a base letter or the CGJ
(`SUAMR_REORDER_NO_BASE` otherwise). Every codepoint must be in a text space
(`SUAMR_REORDER_ILLEGAL_ZONE` otherwise). A cluster longer than
`SUAS_SUAMR_MAX_CLUSTER == 32` is refused, not truncated
(`SUAMR_REORDER_TOO_LONG`).

### 5.2 The steps

AMTRA normalizes the input to **NFD**, then, within each maximal run of
non-starters, reorders the marks into four tiers, front to back:

1. **Step 2c — the ccc=220 MCM run.** If the first ccc=220 mark is an MCM, the
   leading run of ccc=220 MCM marks moves to the front.
2. **Step 2b — the ccc=230 MCM run.** If the first ccc=230 mark is an MCM, the
   leading run of ccc=230 MCM marks moves ahead of any shadda.
3. **Step 2a — the shadda.** Any shadda (ccc=33) moves ahead of the remaining
   marks.
4. **The rest.** Every other mark keeps its relative encoded order.

Because the record carries a stacking **level** (§3.3), a renderer can also
express the same result as a depth: the shadda is level 0 (base-most), the
promoted MCMs next, the tashkil outermost.

### 5.3 The combining grapheme joiner

The default order is correct for most uses. Where a different order is desired,
the CGJ (U+034F) pins the mark that immediately follows it: AMTRA **never moves a
mark across a CGJ**. A caller encodes `<damma, CGJ, hamza above>` to place the
hamza above over the damma; `<kasra, CGJ, hamza below>` places the hamza below
under the kasra.

### 5.4 Why NFD and not NFC

NFD assures that `<superscript alef, madda>` orders the same way regardless of
the base. Under NFC, `<alef, superscript alef, madda>` would order differently
from `<lam, superscript alef, madda>` because NFC composes `<alef, madda>`.

### 5.5 Precomposed recombination (informative)

A renderer may, *after* AMTRA, replace a base and a following combining mark with
a precomposed character where one exists (for example `<yeh, hamza above>` →
`<yeh with hamza above>`). It may not *skip* a combining mark when doing so: the
output `<alef, superscript alef, maddah above>` must not become
`<alef with madda above, superscript alef>`.

### 5.6 Marks deliberately excluded from the MCM

- **Meem above / meem below.** Their behavior already meets expectations;
  including them would wrongly move them ahead of fatha, kasra or damma.
- **Maddah.** Neither `U+0653` nor `U+06E4` is an MCM: they normally display
  above vowel marks.
- **Combining alef overlay** (`U+10EFC`). It has ccc=0 and is a starter, so
  AMTRA never moves it.
- **Sukun alternate forms** (`U+06DF`–`U+06E1`). Their ccc is 230, so AMTRA does
  not affect their ordering; AMTRA does not make them equivalent to `U+0652`.

### 5.7 Dotted circles (informative)

A renderer that inserts a dotted circle for a sequence it believes invalid is a
problem in Arabic script: text that looks invalid may be a lesser-known
orthography or the Quran. A conforming renderer applies AMTRA **first** and only
then considers a dotted circle; the process is best kept at a higher level than
the mark engine.

---

## 6. Zone Dispatch and the Algorithmic Default

### 6.1 Zone dispatch

`suas_suamr_zone_of` classifies a codepoint into the zones of §2.3 in O(1),
testing the Control Plane, the trap slots, the Sentinel and ExtSUCS before the
Native plane, with the RNUR carve-out noted explicitly.

### 6.2 The algorithmic default

An uncurated mark is not unknown. Its stacking follows from its block:

- a below mark sits under the base with a negative `y_offset`;
- an above mark sits over it with a positive `y_offset`.

The default mark ranges are the Arabic combining inventory (Arabic, Arabic
Extended-A/B/C) and the RNUR carve-out. A codepoint that is **not** a default
mark and has no curated record is an ordinary starter: it resolves to ccc=0, is
not a mark, and has no layout.

### 6.3 The mistaken assignment of U+08D9

`U+08D9 ARABIC SMALL LOW NOON WITH KASRA` (Unicode 9.0) appears **below** the
text but was mistakenly given **ccc=230**. That cannot now be changed. SUAMR
records the mark as it is: ccc=230, but stacking **below**
(`SUAMR_FLAG_STACK_BELOW`). Consequences a conforming implementation must
respect:

- Used with any ccc=220 mark, AMTRA places the ccc=220 mark between the base and
  `U+08D9`; a CGJ prevents that.
- A sequence containing both `U+08D9` and another ccc=230 mark may display the
  same but not be canonically equivalent; `U+08D9` should be encoded at the end
  of a combining-mark sequence.
- In Quranic orthographies where `U+08D9` appears between a base and a kasra,
  the input must be `<base, low noon with kasra, CGJ, kasra>`.
- A font should render `U+08D9` as a mark below and give it no effect on marks
  above.

---

## 7. UAX #53 / SUCD Field Mapping

| UAX #53 / UCD notion | SUAMR artifact |
|----------------------|----------------|
| Canonical_Combining_Class | `suamr_ccc_table[]` / `suas_suamr_ccc` |
| Modifier_Combining_Mark (PropList.txt) | `SUAMR_FLAG_MCM` |
| Shadda (ccc=33) | `SUAMR_FLAG_SHADDA`, `SUAMR_SHADDA_CP` |
| the AMTRA reorder | `suas_suamr_reorder_cluster` |
| the CGJ override | `SUAMR_CGJ` barrier in §5.3 |
| the positioning manifest (SUCD) | `suamr_table[]` (compiled, byte-packed) |
| a below / above generalized class | `SUAMR_CCC_BELOW` / `SUAMR_CCC_ABOVE` |

---

## 8. Data Files and the Curated Table

The curated table `suamr_table[]` is fed from the SUCD manifests at build time.
It is the UAX #53 Arabic combining-mark set plus the Native/neography
declarations; the reference roster is §13. Extending it is a **data update** (a
minor revision) and never changes the API. A mark with no curated record is
answered by §6 — the property is total.

---

## 9. Verification

`suas_suamr_validate_table(entries, count, *out_error)` reports the first defect
in a compiled table. An empty registry (count 0) is conformant. The outcomes:

| Outcome | Meaning |
|---------|---------|
| `SUAMR_REGISTRY_OK` | The table is conformant |
| `SUAMR_REGISTRY_NULL_TABLE` | count > 0 with a NULL pointer |
| `SUAMR_REGISTRY_COUNT_OVER` | count > `SUAS_SUAMR_REGISTRY_CAPACITY` |
| `SUAMR_REGISTRY_UNORDERED` | Codepoints are not ascending |
| `SUAMR_REGISTRY_DUPLICATE_CODEPOINT` | A codepoint appears twice |
| `SUAMR_REGISTRY_OUT_OF_SPACE` | A codepoint exceeds `0x7FFFFFFF` |
| `SUAMR_REGISTRY_INADMISSIBLE_CODEPOINT` | A codepoint is SCP, trap or Sentinel |
| `SUAMR_REGISTRY_BAD_CCC` | A canonical combining class is invalid |
| `SUAMR_REGISTRY_BAD_FLAGS` | Reserved or contradictory bits are set |
| `SUAMR_REGISTRY_BAD_RESERVED` | The reserved byte is not zero |
| `SUAMR_REGISTRY_CUSTOM_CLASS_MISMATCH` | CUSTOM_CLASS outside the RNUR set |
| `SUAMR_REGISTRY_STACK_CONTRADICTION` | Both STACK_ABOVE and STACK_BELOW set |

---

## 10. The SUAS-010 (SSRE) Linkage Gate

SUAMR registers as a **mandatory core invariant** in SUAS-010 (SSRE):

| spec_id | short | module | maj | min | kind | tier |
|---------|-------|--------|-----|-----|------|------|
| `SUAS-015` | `SUAMR` | `suas_suamr` | 1 | 0 | mandatory | L0 |

A dependency may now name `SUAS-015`; `suas_ssre_verify_dependency` succeeds at
version `1.0` and above.

---

## 11. Stability and Versioning

- A **data update** (extending `suamr_table[]` within the fixed limits) is a
  minor revision; no descriptor that verified before can fail afterwards.
- Adding a **registration** is a patch-level change (SUAS-010 §11).
- The record layout, the flag assignments, the zone taxonomy and the AMTRA steps
  are architectural and change only by a major revision.

---

## 12. Conformance Requirements

A conforming SUAMR implementation MUST:

1. Expose `suamr_mark_layout_t` as exactly 12 bytes (§3.1).
2. Resolve a mark's layout in O(1) expected time with no allocation (§1.2).
3. Return `true` for every mark in a text space and `false` only for an
   inadmissible zone or a NULL output (§6).
4. Preserve the AMTRA tiers and the CGJ barrier exactly (§5).
5. Report every defect of §9 from its verifier.
6. Register as `SUAS-015` (SUAMR), mandatory, at version `1.0` (§10).

### 12.1 Compliance matrix (reference implementation)

| Requirement | Where |
|-------------|-------|
| 12-byte record | `src/suas/suas_suamr.c` static asserts |
| O(1) lookup | `suamr_build`, `suamr_find` |
| Totality | `suas_suamr_get_mark_layout`, `suamr_default_record` |
| AMTRA | `suas_suamr_reorder_cluster` |
| Verifier | `suas_suamr_validate_table` |
| SSRE registration | `src/suas/suas_ssre.c` |

---

## 13. Annex — Curated Roster

The reference roster includes the tashkil (U+064B–U+0658), the Quranic marks
(U+0661, U+06DC, U+06DE, U+06E3, U+06E7, U+06E8, U+06ED), the superscript alef
(U+0670), the Arabic Extended-A marks (U+08CE, U+08CF, U+08D3, U+08D9, U+08F3),
the Arabic Extended-C Quranic marks (U+10EF4, U+10EF6, U+10EF9), and the Native /
RNUR neography declarations (`0x160100`–`0x160102`).

---

## 14. Terms and Conventions

- **Base.** The starter a mark attaches to; ignored by the 1.0 layout query.
- **Cluster.** One Combining Character Sequence, bounded by SUAS-006 (STS).
- **Neography.** A conlang or custom writing system; its marks declare
  `SUAMR_FLAG_CUSTOM_CLASS` inside the RNUR carve-out.
- **SUCS.** A 31-bit SuperUnicode codepoint (`0x00000000–0x7FFFFFFF`).

---

## 15. Testing

`tests/test_suas_suamr.c` exercises the version/meta constants, the 12-byte
record geometry, the ccc values of §4 (including the `U+08D9` case of §6.3),
zone dispatch and the RNUR carve-out, the above/below/MCM/shadda predicates, the
algorithmic default, the O(1) layout query and offsets, enumeration, the AMTRA
tiers and the CGJ barrier, every verification outcome, and the SSRE
registration. `unified/test_unified.c` proves the header coexists with every
other module header in one translation unit.
