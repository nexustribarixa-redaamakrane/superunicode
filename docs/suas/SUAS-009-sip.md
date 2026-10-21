# SUAS-009 — System Ideographic Properties (SIP)

**Standard ID:** SUAS-009
**Status:** Draft — Ratified
**Scope:** Core Kernel Invariant
**Applies To:** SUCD, UAX #38 (Unihan analog), SUAS-013 (SISR), SUTR-8 (SUCPM), SUAS-004 (SUCF), SUAS-005 (SSP), SUTS-001 (SUCA), SUTS-004 (SIVD), the SUF font engine, the OpenWindows shell, the System Control Plane
**Last Updated:** 2026-10-19

---

## 1. Overview

System Ideographic Properties (SIP) is the core algorithmic subsystem governing
the **zero-allocation, O(1) querying and bitfield extraction of ideographic
character attributes**: the Kangxi radical, the stroke count, a classification
bitfield, a collation seed, and the linkage to a regional or historical variant.

Where a traditional Unicode environment consults the Unihan Database (UAX #38) —
a multi-megabyte flat-file collection laden with historical narrative notes and
string properties, read and parsed at run time — SIP is a **dual-layer**
architecture. The offline reference layer is the SuperUnicode Character
Database (SUCD), which carries the full text manifests for developer tooling,
user-space applications, and offline documentation. At build time the SUCD
datasets are **compiled into immutable, byte-packed binary tables**; the runtime
kernel layer then executes the SIP algorithms directly against those static
bitfields, with no text parsing and no memory allocation.

A SIP query discards the human-readable text keys (`kTotalStrokes`, `kMandarin`,
`kDefinition`, …) in favour of **direct bitfield masking and a hash-indexed
static array**, so a radical-stroke count, a sorting weight, or a variant
linkage is evaluated in constant time.

SIP is not an application feature. It is a kernel invariant: the `suf` vector
font engine calls SIP during single-pass text layout to resolve fallback
rendering metrics and spatial layout bounding for complex CJK and neography
logographs, and every query must return the same answer, everywhere, in O(1).

SIP operates over the SuperUnicode space (`0x000000`–`0x7FFFFFFF`, plus the
unbounded extSUCS plugin plane above it):

| Zone | Range | Ideographic treatment |
|------|-------|-----------------------|
| CJK Extension A | `0x00003400`–`0x00004DBF` | Admitted (ideographic block) |
| CJK Unified Ideographs (URO) | `0x00004E00`–`0x00009FFF` | Admitted (ideographic block) |
| CJK Compatibility Ideographs | `0x0000F900`–`0x0000FAFF` | Admitted (compatibility) |
| CJK Extension B | `0x00020000`–`0x0002A6DF` | Admitted (ideographic block) |
| CJK Extensions C–F | `0x0002A700`–`0x0002EBEF` | Admitted (ideographic block) |
| CJK Compatibility Supplement | `0x0002F800`–`0x0002FA1F` | Admitted (compatibility) |
| CJK Extension G | `0x00030000`–`0x0003134F` | Admitted (ideographic block) |
| System Control Plane | `0x00110000`–`0x0011FFFF` | **Strictly inadmissible** |
| RNUR neography carve-out | `0x00160000`–`0x0016FFFF` | Admitted (neography logographs) |
| Trap slots | `0x7FFFFFF0`–`0x7FFFFFFE` | Inadmissible |
| Sentinel | `0x7FFFFFFF` | Inadmissible |
| extSUCS plugin plane | above `0x7FFFFFFF` | Not ideographic (opaque) |

### 1.1 Architectural shifts from UAX #38

| Dimension | UAX #38 | SUAS-009 SIP |
|-----------|---------|--------------|
| Store | Multi-MB text flat files (`Unihan_*.txt`) | Immutable, byte-packed binary table |
| Keys | Human-readable string keys (`kTotalStrokes`) | Direct bitfield mask / array field |
| Lookup | Line scan / map build at run time | O(1) hash-indexed static array |
| Memory | Implementation-defined; grows with data | Zero allocation; fixed-size stack record |
| Scope | 21-bit Unicode scalar values | Full SUCS space; Native plane for neography |
| Numeric/stroke | Separate string properties | Packed numeric fields / flags |
| Variants | `k*Variant` string lists | `native_variant` linkage + SIVD (SUTS-004) |
| Notes / readings | Narrative and phonetic strings | **Not carried** (kept in offline SUCD) |
| Consumers | Cross-mapping to other standards | OpenWindows shell, SUF font engine |

SIP is a **property engine**, not a shaping engine: it answers "what are the
ideographic attributes of this codepoint?" and nothing more. It never rasterizes
or shapes; font fallback and glyph choice are the caller's responsibility (see
SUAS-005, SSP, and SUTS-004, SIVD).

The following UAX #38 property keys are mapped onto the SIP record; the text-only
properties remain in the SUCD reference layer and are deliberately not carried
on the kernel path:

| UAX #38 key | SIP representation |
|-------------|--------------------|
| `kRSUnicode` | `radical` (+ `SUAS_SIP_FLAG_RADICAL`) |
| `kTotalStrokes` | `strokes` |
| `kPrimaryNumeric` / `kAccountingNumeric` | `SUAS_SIP_FLAG_NUMERIC` |
| `kCompatibilityVariant` | `SUAS_SIP_FLAG_COMPAT` + sort-weight fold |
| `kSimplifiedVariant` / `kTraditionalVariant` | `SUAS_SIP_FLAG_TRADITIONAL` / `native_variant` |
| `kUnified_Ideograph` | `SUAS_SIP_FLAG_UNIFIED` |
| `kMandarin`, `kCantonese`, `kDefinition`, `kIRG_*`, … | **Not carried**; offline SUCD only |

> **Curation note.** The seeding brief named the offline reference layer
> "SUCA". In this corpus **SUCA** is already the *SuperUnicode Collation
> Algorithm* (SUTS-001), and the character database is **SUCD**. SIP therefore
> names SUCD as its offline reference layer; the collision is resolved in favour
> of the established identifier.

### 1.2 Goals

- **Determinism.** The same codepoint always yields the same property record,
  independent of platform, locale, or threading.
- **Constant time.** A query performs a bounded number of hash, probe, and
  mask steps regardless of the dataset size.
- **Zero allocation.** The table is static and the index is a fixed-size array;
  a queried record is a stack struct.
- **Freestanding.** No C library, no floating point, no heap; C99 freestanding
  subset only.
- **Total.** Every codepoint produces a defined answer (a record, an algorithmic
  weight, or a defined miss).

### 1.3 Non-negotiable invariants

1. **SCP invariance.** No record names a codepoint in `0x00110000`–`0x0011FFFF`,
   and no query for an SCP marker, a trap slot, or the Sentinel ever succeeds
   (§2.4).
2. **Uniqueness.** At most one record exists per codepoint; a query resolves to
   exactly one record or to a defined miss (`SIP-C1`).
3. **Immutability.** The compiled table is read-only at run time; it never grows
   and is never rewritten.
4. **Idempotent setup.** `suas_sip_init_engine` may be called any number of
   times; the index is always the same afterwards.
5. **Static capacity.** The number of records never exceeds
   `SUAS_SIP_REGISTRY_CAPACITY`; the index never grows at run time.

---

## 2. Definitions and Notation

### 2.1 Definitions

```
SIP-D1. Ideographic property record: the fixed-size, stack-allocatable
        description of a single codepoint's ideographic attributes:
        its radical, stroke count, classification flags, collation seed, and
        optional variant linkage.

SIP-D2. Radical-stroke: the Kangxi radical number and the total stroke count of
        an ideograph, carried in the `radical` and `strokes` fields.

SIP-D3. Collation seed: the `sort_weight` of a record, a 31-bit primary-order
        weight consistent with the SUCA implicit ordering (SUTS-001). A record
        with no curated weight uses the algorithmic native-order weight.

SIP-D4. Variant linkage: the `native_variant` field, naming a codepoint in the
        Native & Neography Plane that is the regional or historical counterpart
        of the record's codepoint. A zero linkage means "none".

SIP-D5. Ideographic repertoire: the union of the registered table and the
        codepoint ranges of §5; membership is decided by O(1) range dispatch.
```

> **Curation note.** As with UAX #38, it is neither possible nor desirable to
> carry every property of every ideograph on the kernel path. SIP admits a
> deliberately selective, curated table: the properties the shell and the font
> engine need at layout time. The full text dataset remains in SUCD.

### 2.2 Notational conventions

A record is written as a bracketed **property tuple** — the codepoint, then the
radical, the stroke count, the flags, and the collation seed:

```
U+4E00  一   radical 1   strokes 1   UNIFIED|RADICAL   weight 0x00004E00
U+9F8D  龍   radical 212 strokes 16  UNIFIED|RADICAL|VARIANT|TRADITIONAL
                                   native variant U+160200 (RNUR)
```

In data tables the codepoint is written in bare hexadecimal with a semicolon
before the attributes (mirroring the Unihan one-line-per-property convention,
folded onto one line per codepoint so the build compiles it into a record):

```
4E00; rad=1 str=1 flags=UNIFIED,RADICAL weight=4E00
9F8D; rad=212 str=16 flags=UNIFIED,RADICAL,VARIANT,TRADITIONAL variant=160200
```

### 2.3 The static record

A queried property set is a fixed, stack-allocatable record
(`suas_glyph_prop_t`, §3.1). It carries the codepoint, the radical, the stroke
count, the flag bitfield, the collation seed, and the optional variant linkage.
There is no pointer into shared mutable storage and no heap.

### 2.4 Address-space planes and SCP invariance

`suas_sip_plane_of` classifies a codepoint into exactly one plane. The System
Control Plane block `0x00110000`–`0x0011FFFF` is strictly inadmissible, as are
the trap slots (`0x7FFFFFF0`–`0x7FFFFFFE`) and the Sentinel (`0x7FFFFFFF`).
This keeps ideographic data disjoint from the in-band control instructions of
SBR, the math and segmentation directives, the SDF directional block, the SSP
script directives, and the BANcodes.

The Native & Neography Plane carve-out `0x00160000`–`0x0016FFFF` is the RNUR
neography range (`SUAS_SIP_RNUR_MIN`–`SUAS_SIP_RNUR_MAX`). RNUR supplies the
ideograph-like logographs whose properties SIP reports; RCCR supplies
language-identification subtags out of band and **allocates no codepoints**.

---

## 3. The Dual-Layer Architecture

### 3.1 Offline reference layer (SUCD)

The SuperUnicode Character Database (SUCD) is the offline, human-readable
reference layer. It carries the full text manifests analogous to the Unihan
files — readings, definitions, IRG source data, and historical notes — for
developer tooling, user-space applications, and offline documentation. This
layer is **not** consulted at run time.

### 3.2 Runtime kernel layer (SIP)

At build time the SUCD attribute dataset is compiled into the immutable binary
table `sip_table[]` (§8). The kernel executes the SIP algorithms directly
against the packed records: bitfield masking for the flags, direct field reads
for the radical, stroke count, and weight, and an array lookup for the variant
linkage. No text is parsed and no memory is allocated on the query path.

---

## 4. Properties and Algorithms

### 4.1 Radical-stroke (`SIP-D2`)

`suas_sip_get_radical_stroke` returns the Kangxi `radical` and the total
`strokes` in one O(1) query. Either output pointer may be `NULL`. A record whose
flags lack `SUAS_SIP_FLAG_RADICAL` carries no radical-stroke data; the query
still succeeds and reports whatever the record holds (typically `0`). A radical
of `0` denotes "none" and is used for native logographs whose structure is not
Kangxi-based.

### 4.2 Collation seed (`SIP-D3`)

`suas_sip_get_sort_weight` returns the record's `sort_weight` when the codepoint
is registered. Otherwise, if the codepoint is a valid ideograph that is not in
the table, it returns the **algorithmic native-order weight** — the low 31 bits
of the codepoint — so ordering is total and stable across the whole repertoire.
For a non-ideograph, an SCP marker, a trap slot, or the Sentinel, the query
returns `false`.

Curated weights let a record deviate from native order. In particular, a
compatibility ideograph folds onto its canonical unified counterpart: the sort
weight of `U+F900` (豈) is `0x00008C48` (the weight of `U+8C48`, 豈), so the two
interleave in collation order (§5.3). This is the SIP counterpart of SUCA's
implicit-weight interleaving (SUTS-001).

### 4.3 Classification flags

`suas_sip_flags` returns the record's bitfield, or `0` when the codepoint is
unregistered. Each flag is a single, disjoint bit:

| Flag | Meaning |
|------|---------|
| `SUAS_SIP_FLAG_UNIFIED` | Unified_Ideograph |
| `SUAS_SIP_FLAG_RADICAL` | radical-stroke data present |
| `SUAS_SIP_FLAG_VARIANT` | carries a variant linkage |
| `SUAS_SIP_FLAG_OBSOLETE` | obsolete / deprecated form |
| `SUAS_SIP_FLAG_NUMERIC` | carries a numeric value |
| `SUAS_SIP_FLAG_NATIVE` | Native / RNUR-plane logograph |
| `SUAS_SIP_FLAG_COMPAT` | compatibility ideograph |
| `SUAS_SIP_FLAG_TRADITIONAL` | traditional (vs. simplified) form |

### 4.4 Variant linkage (`SIP-D4`)

`suas_sip_has_variant` reports whether a registered record carries the
`SUAS_SIP_FLAG_VARIANT` bit. `suas_sip_variant_target` resolves the linkage:
it returns the `native_variant` codepoint in O(1), or `false` when the record
carries no linkage.

The linkage routes a query **directly into the Native & Neography Plane and the
SIVD variation database (SUTS-004)**. An ideograph that needs a distinct
regional or historical representation beyond Unicode Han unification — a
traditional form, a regional glyph, or a neography logograph — is registered
here with a native target, and the caller follows the linkage into SIVD for the
CSS/variation-selector-level glyph.

---

## 5. Classification and Range Dispatch

### 5.1 The ideographic repertoire (`SIP-D5`)

`suas_sip_is_ideograph` decides membership in O(1). A codepoint is ideographic
if it has a record, if it falls inside one of the CJK ideographic blocks of
§1, or if it lies in the RNUR neography carve-out. The registration table is a
curated subset; the range dispatch makes the predicate **total**, so an
unregistered but valid ideograph is still recognized.

### 5.2 Bridge and Native ideographs

- **Bridge ideographs** draw their codepoints from `0x00000000`–`0x0010FFFF`:
  the CJK unified, extension, and compatibility blocks.
- **Native & neography ideographs** draw their codepoints from the RNUR
  carve-out `0x00160000`–`0x0016FFFF` (inside the Native plane), letting a
  conlang expose logographs with radical, stroke, and variant data. A record
  whose flags carry `SUAS_SIP_FLAG_NATIVE` always has
  `suas_sip_plane_of(codepoint) == SUAS_SIP_PLANE_RNUR`.

### 5.3 Compatibility folding

A compatibility ideograph is a codepoint whose canonical form is a unified
ideograph. SIP marks it with `SUAS_SIP_FLAG_COMPAT` and gives it a curated
`sort_weight` equal to that of its canonical unified counterpart, so the two
interleave in collation order rather than sorting by their own codepoint values
(§4.2).

---

## 6. Static Packing and O(1) Resolution

### 6.1 The record

```c
typedef struct {
    sucs_ex_char_t codepoint;      /* the ideograph, e.g. U+4E00             */
    uint16_t       radical;        /* Kangxi radical (0 = none / native)     */
    uint8_t        strokes;        /* total stroke count (0 = unknown)       */
    uint8_t        flags;          /* SUAS_SIP_FLAG_* bitfield               */
    uint32_t       sort_weight;    /* SUCA collation seed (§4.2)             */
    sucs_ex_char_t native_variant; /* Native/RNUR variant target, or 0       */
} suas_glyph_prop_t;
```

The record is fixed-size and stack-allocatable; it is 24 bytes on both 32- and
64-bit ABIs. It keeps the seeding sketch's five core fields — `codepoint`,
`radical`, `strokes`, `flags`, `sort_weight` — **verbatim and in order**, with
two documented deviations:

- `codepoint` is widened from `uint32_t` to `sucs_ex_char_t` (64-bit), so an
  RNUR neography logograph in `0x00160000`–`0x0016FFFF` survives; and
- one additive field, `native_variant`, carries the variant linkage (§4.4).

### 6.2 The index

The table is a static array `sip_table[]` of at most
`SUAS_SIP_REGISTRY_CAPACITY` (128) records. A second fixed array of
`SUAS_SIP_INDEX_SIZE` (256) slots is an open-addressing hash index:

1. `suas_sip_init_engine` hashes each registered codepoint (FNV-1a, 32-bit,
   over the eight little-endian bytes of the 64-bit value) and places its table
   offset in the first free slot, resolving collisions by linear probing. The
   build is O(n) once and touches no heap.
2. `suas_sip_get_properties` (and every other query) hashes the query codepoint
   and probes the same sequence of slots until it matches or hits an empty
   slot.

Because the load factor is at most one-half over a bounded, static table, the
number of probes is bounded by a small constant. Resolution is O(1) in the table
size. Queries build the index lazily on first use, so they are correct even if
`suas_sip_init_engine` was not called first.

### 6.3 Complexity

| Operation | Cost |
|-----------|------|
| `suas_sip_init_engine` | O(n) once, n ≤ 128, no allocation |
| `suas_sip_get_properties` | O(1) expected (bounded probes) |
| `suas_sip_get_radical_stroke` | O(1) expected |
| `suas_sip_get_sort_weight` | O(1) expected |
| `suas_sip_flags` / `has_variant` / `variant_target` | O(1) expected |
| `suas_sip_is_ideograph` | O(1) range dispatch |
| `suas_sip_is_registered` | O(1) expected (bounded probes) |
| `suas_sip_plane_of` | O(1) |
| `suas_sip_entry_count` / `entry_at` | O(1) |

---

## 7. Kernel Shell and Font Integration

SIP provides fast ideographic metrics for the OpenWindows shell and for the
`suf` vector font engine:

- **Single-pass layout.** During a single pass over the text, the font engine
  calls `suas_sip_get_properties` to obtain the radical, stroke count, flags,
  and collation seed of each ideograph without breaking the pass or allocating.
- **Fallback rendering metrics.** When a glyph is missing from the active font,
  the engine consults `suas_sip_flags` / `suas_sip_is_ideograph` to decide
  whether a fallback is warranted, and reads the radical-stroke data to select a
  size- and density-appropriate fallback face.
- **Spatial layout bounding.** The stroke count and radical give a cheap density
  estimate for advanced-width and box metrics, so the engine can reserve the
  correct cell without rasterizing first (see also SUAS-002, SGW).
- **Variant routing.** For an ideograph that needs a regional or historical
  form, the engine follows `suas_sip_variant_target` into the Native &
  Neography Plane and the SIVD variation database (SUTS-004), resolving the
  variation selector to a glyph index.

The shell is expected to delimit its query; SIP matches the codepoint exactly.

---

## 8. Data Files

The normative table is the compiled static array `sip_table[]` in
`src/suas/suas_sip.c`. It is the single source of truth for the runtime
properties. The analogous offline form, materialized in SUCD, uses one line per
codepoint with a semicolon-delimited attribute list:

```
4E00; rad=1 str=1 flags=UNIFIED,RADICAL weight=4E00
9F8D; rad=212 str=16 flags=UNIFIED,RADICAL,VARIANT,TRADITIONAL variant=160200
F900; rad=151 str=10 flags=COMPAT,VARIANT weight=8C48
160100; rad=1 str=4 flags=NATIVE weight=160100
```

A provisional attribute set may be maintained in SUCD before promotion into the
compiled table (§11); it carries no normative status until compiled.
Implementations may use other formats for this data, as appropriate; the key
information is the codepoint and its attributes.

---

## 9. Implementation Notes

### 9.1 Querying

```c
suas_glyph_prop_t p;
if (suas_sip_get_properties(0x9F8DULL, &p)) {
    /* p.radical == 212, p.strokes == 16,
       p.flags & SUAS_SIP_FLAG_TRADITIONAL,
       p.native_variant == 0x00160200ULL */
}
```

`suas_sip_get_properties` is O(1): it performs a bounded hash and probe
sequence and no allocation.

### 9.2 Extracting a single attribute

```c
uint8_t strokes = 0;
if (suas_sip_get_radical_stroke(0x4E00ULL, NULL, &strokes)) {
    /* strokes == 1 */
}
```

### 9.3 Enumerating

```c
uint32_t i, n = suas_sip_entry_count();
suas_glyph_prop_t p;
for (i = 0; i < n; i++) {
    suas_sip_entry_at(i, &p);
    /* p.codepoint, p.radical, p.strokes, p.flags, p.sort_weight, ... */
}
```

### 9.4 Routing a variant

```c
sucs_ex_char_t native;
if (suas_sip_variant_target(0x9F8DULL, &native)) {
    /* native == 0x00160200ULL; follow into SIVD for the glyph */
}
```

---

## 10. Testing

Conformance is demonstrated by a host test that exercises the table and the
algorithms. The reference suite covers, at minimum:

- **Core query:** known URO, compatibility, and RNUR records; field values and
  flags; the lazy build path; the idempotent setup.
- **Radical-stroke:** a registered record; `NULL` output guards; a miss; a
  non-ideograph.
- **Collation seed:** the stored weight; the compatibility fold; the algorithmic
  native-order fallback for an unregistered ideograph; a non-ideograph miss.
- **Variant linkage:** a traditional/simplified link; an RNUR chain; a
  `VARIANT`-flagged compatibility form with no native target; `NULL` guards.
- **Flags:** `NUMERIC`, `COMPAT`, `UNIFIED`; unregistered and SCP returns `0`.
- **Classification:** registered and unregistered ideographs across every block
  and the RNUR carve-out; rejection of Latin, Hiragana, the SCP, trap slots, and
  the Sentinel.
- **Planes:** every plane, including the RNUR carve-out and the extSUCS plane.
- **Miss and guards:** unregistered, non-ideograph, SCP, trap, Sentinel, and
  `NULL` outputs.
- **Enumeration:** the count, per-record self-consistency, SCP invariance, the
  native-plane rule, and the presence of Native and compatibility records.
- **Hash:** determinism, distinctness, and the FNV-1a offset basis.

---

## 11. Stability and Versioning

- The record layout, the table/index sizes, and the query contracts are
  **stable** within a major version.
- New ideographs may be **added** to the table (§8); existing records and their
  attributes never change (`SIP-C1`).
- The curated table tracks SUCD; a dataset update is a **minor** revision and
  does not change the API.

Current version: **1.0.0** (`SUAS_SIP_SPEC_ID` = `"SUAS-009"`).

### 11.1 Version-domain introduction

At the time of writing, `SUAS-009` (SIP) is introduced alongside the ratified
SUAS family (SUAS-001 … SUAS-008) as a **1.0** core invariant. The table it
ships is the initial curated Bridge + RNUR set; additions are minor revisions.

---

## 12. Conformance Requirements

An implementation claims conformance by identifying the SIP version it
implements (`SIP-C1`) and by declaring which of the following it observes:

| Requirement | Subject |
|-------------|---------|
| `SIP-C1` | Query only the normative table; one record per codepoint |
| `SIP-D1` | The static property record |
| `SIP-D2` | Radical-stroke extraction |
| `SIP-D3` | Collation seed, stored and algorithmic |
| `SIP-D4` | Variant linkage into the Native & Neography Plane |
| `SIP-D5` | Total classification via O(1) range dispatch |
| §2.4 | SCP / trap / Sentinel invariance |
| §6 | Static packing and O(1) resolution |

### 12.1 Compliance matrix (reference implementation)

| Requirement | Reference | Test |
|-------------|-----------|------|
| Version identification (`SIP-C1`) | §11 | `test_version_and_meta` |
| Core query | §3.2, §6.2 | `test_get_properties` |
| Lazy / idempotent index build | §6.2 | `test_init_idempotent` |
| Radical-stroke (`SIP-D2`) | §4.1 | `test_radical_stroke` |
| Collation seed (`SIP-D3`) | §4.2, §5.3 | `test_sort_weight` |
| Variant linkage (`SIP-D4`) | §4.4, §7 | `test_variant_linkage` |
| Classification flags | §4.3 | `test_flags` |
| Repertoire dispatch (`SIP-D5`) | §5.1 | `test_ideograph_dispatch` |
| Plane classification and SCP invariance | §2.4 | `test_plane_of` |
| Miss and guard behaviour | §6.2 | `test_get_miss_and_guards` |
| Enumeration and self-consistency | §6.2, §8 | `test_enumeration` |
| Hash | §6.2 | `test_hash` |

---

## 13. Terms and Conventions

- **Ideographic property record** — the fixed-size `suas_glyph_prop_t`
  (`SIP-D1`).
- **Radical-stroke** — the Kangxi radical number and total stroke count
  (`SIP-D2`).
- **Collation seed** — the record's `sort_weight`, or the algorithmic
  native-order weight for an unregistered ideograph (`SIP-D3`).
- **Variant linkage** — the `native_variant` target in the Native & Neography
  Plane (`SIP-D4`).
- **Ideographic repertoire** — the registered table plus the CJK and RNUR
  ranges (`SIP-D5`).
- **Compatibility fold** — the curated sort weight of a compatibility ideograph,
  equal to its canonical unified counterpart (§5.3).
- **SUCD** — the SuperUnicode Character Database; the offline text reference
  layer (distinct from SUCA, the SuperUnicode Collation Algorithm).
- **SCP marker** — a codepoint in `0x00110000`–`0x0011FFFF`; always inadmissible.
- **RNUR carve-out** — `0x00160000`–`0x0016FFFF`, the neography range.

**END OF SUAS-009**
