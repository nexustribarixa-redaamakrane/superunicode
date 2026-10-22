# SUTS-004 — System Ideographic Variation Database (SIVD)

**Standard ID:** SUTS-004
**Status:** Draft — Authoring
**Scope:** Modular Technical Specification (ideographic and stylistic character variation)
**Applies To:** Base SUCS (Unicode Bridge, SCP, Native), ExtSUCS (unbounded arbitrary-precision plane), the RNUR neography registry, the RCCR language and script subtags, SUCD property tables, the OpenWindows vector font engine (`suf`), the character token stream, the OpenWindows microkernel and shell
**Last Updated:** 2026-10-19

---

## 1. Introduction

Characters in the Unicode Standard can be represented by a wide variety of
glyphs. Occasionally the need arises in text processing to restrict or change
the set of glyphs that are used to display a character. In special
circumstances this restriction must be expressed in plain text rather than by
font selection or some other rich-text mechanism. A **variation sequence** — a
graphic character followed by a variation selector — identifies such a
restriction. The combination of an ideograph and a selector is an **Ideographic
Variation Sequence** (IVS).

In the case of Han and other ideographs, it is impossible to build a single
collection of variation sequences that satisfies every user. The requirements
of scholars, governments, and publishers differ too much to be met by one
collection; instead they are met by multiple independent **collections**. A
database ensures that there is a single definition of a given variation
sequence, so that interchange of text using such sequences is reliable.

**SUTS-004 (SIVD) is the System Ideographic Variation Database specification.**
It is a core kernel primitive of the OpenWindows microkernel. SIVD establishes
a **binary-compiled, zero-allocation schema** for managing ideographic and
stylistic character variant sequences — bypassing traditional text-heavy,
multi-megabyte external flat files (such as Unicode's **UTS #37** Ideographic
Variation Database) in favour of fixed-size, byte-aligned memory layouts
optimized for O(1) kernel execution and font-pipeline lookup.

A subsystem requests a variant by the **(base codepoint, variation selector)**
pair. The kernel hashes the pair and probes a fixed open-addressing index over
an immutable static table, resolving the pair to a font-bundle glyph index in
O(1) expected time.

### 1.1 OpenWindows kernel principles

In keeping with OpenWindows kernel principles, SIVD ditches text-registry
parsers, dynamic glyph caches, and heap-resident lookup trees. Instead, it
defines a **compiled binary schema**: variation definitions are compiled to an
immutable, statically-linked table of fixed-size C structures
(`sivd_variation_entry_t`, `sivd_collection_t`). The OpenWindows vector font
engine (`suf`) reads the resolved glyph index directly from a fixed memory
offset during single-pass text layout. Lookup is a single probe of a fixed-size
open-addressing index built once over a static array, so a resolve performs a
bounded number of integer operations with no call to a general allocator.

### 1.2 Relationship to UTS #37 IVD

SIVD solves the same problem as UTS #37 *Unicode Ideographic Variation
Database*: associating an ideograph-plus-selector sequence with a unique
glyphic subset, and keeping that association stable for interchange. It is
deliberately **not** a UTS #37 analog, because the two systems differ in three
structural ways:

1. **Text registries vs. compiled binary structures.** The IVD is two UTF-8
   flat files — `IVD_Collections.txt` and `IVD_Sequences.txt` — that a
   conforming implementation reads, tokenizes on `;`, and interprets at run
   time. Interpreting a rule requires file I/O, a text parser, and dynamic
   string manipulation. A kernel primitive cannot afford a flat-file parser on
   the rendering path. SIVD discards text registries entirely in favour of
   byte-aligned, statically-compiled C structures that sit directly in
   read-only memory.
2. **Heap allocation vs. zero allocation.** A user-space IVD implementation
   loads the registries into dynamic structures — hash maps of sequence to
   collection, per-collection identifier tables, glyph-subset blobs — built on
   the heap at start-up. OpenWindows enforces a strict zero-allocation mandate
   for core kernel subsystems. SIVD achieves O(1) lookup through fixed-size,
   static tables without ever touching a heap.
3. **User-space vs. kernel-primitive scope.** The IVD is designed for
   user-space text layout engines, publishing tools, and font handlers that
   read the files from disk. SIVD is a low-level kernel primitive: it gives the
   OpenWindows microkernel, the `suf` font pipeline, and the terminal drivers
   immediate access to deterministic variant resolution without depending on an
   external file system, a user-space service, or a per-process cache.

Where this document shares vocabulary with UTS #37 (variation sequence,
collection, glyphic subset, registration), the requirements are normative for
SIVD only.

### 1.3 Key architectural shifts

| Dimension | UTS #37 IVD (user-space) | SUTS-004 SIVD (kernel) |
|-----------|--------------------------|------------------------|
| Representation | Two UTF-8 flat files parsed at run time | Fixed-size C structs in read-only memory |
| Allocation | Registries loaded into heap hash maps and tables | Zero; static table + fixed index, no heap |
| Scope | Layout engines, publishing tools, font handlers | Kernel primitive: microkernel, `suf` pipeline, terminal drivers |
| Lookup | Parse, tokenize, then a map lookup | (base, selector) → FNV-1a → fixed open-addressing probe, O(1) |
| Selector domain | Unicode selectors only (U+E0100–U+E01EF) | Dual-plane: Unicode selectors plus RNUR neography selectors |
| Registration | Flat-file merge by a registrar, read at run time | Compile-time table merge; the kernel never reads a file |

### 1.4 The dual-plane variation index

SIVD maintains a **single dual-plane index**:

* **Bridge Variation Space (`0x000000–0x10FFFF`).** Maps standard Ideographic
  Variation Sequences and supplementary variation selectors directly to
  standard CJK and historical glyph variants. This is the SIVD analogue of the
  UTS #37 IVD proper.
* **Native & Neography Variation Space (`0x160000–0x16FFFF`).** Extends
  variation-selector mechanics to the RNUR neography scripts and conlangs. A
  conlang can trigger custom contextual alternates, regional glyph forms, and
  stylistic flourishes deterministically, without heap allocation.

Both planes share one entry type, one collection type, and one lookup index;
only the codepoint ranges differ.

### 1.5 Terminology and cross-references

SIVD reuses the SuperUnicode architecture documents, numbered as they exist in
this repository:

| SIVD concept | Authoritative document |
|--------------|------------------------|
| Variation base / ideograph (SUCS) | **SUTR-0 (SUCS Core)** — three-space layout |
| Glyph index and font pipeline | **suf** — the SuperUnicode Font format and engine |
| RNUR neography registry, property tiers | **SUTR-8 (SUCPM)** — the character property model |
| Neography carve-out and plugins | **SUTR-5 (Plugin Lifecycle)** |
| Script identification | **SUAS-005 (SSP)** — System Script Property |
| Canonical forms (unification) | **SUAS-004 (SUCF)** — SuperUnicode Canonical Forms |
| Conformance model and zero-allocation mandate | **SUTR-10 (SUCM)** |

> The reference implementation ships as `include/suts/suts_sivd.h` +
> `src/suts/suts_sivd.c`. The sketched identifiers `SIVD_SPEC_ID` and
> `SIVD_SPEC_TITLE` become `SUTS_SIVD_SPEC_ID` and `SUTS_SIVD_SPEC_TITLE`; the
> library and function names (`suts_sivd.h`, `sivd_*`) are unchanged. The sketch
> typed the codepoint fields as `uint32_t`; the reference types them as
> `sucs_ex_char_t` (64-bit) so an RNUR neography base in `0x00160000–0x0016FFFF`
> and an ExtSUCS plugin base above `0x7FFFFFFF` survive intact.

### 1.6 Conformance

A conforming SIVD implementation MUST:

1. represent variation data as immutable, fixed-size structures reachable
   directly from a (base codepoint, variation selector) key, with **no heap
   allocation** on any lookup path;
2. resolve a pair to its glyph index in **O(1) expected time** by a bounded
   probe of a fixed-size index, not by a linear scan of the table;
3. admit only well-formed variation selectors of §2.3 and only SCP-invariant
   bases of §4.4; and
4. keep the **(base codepoint, variation selector)** key unique under the
   registry of §4.1, so that one key selects at most one glyphic subset.

### 1.7 SIVD conformance levels

| Level | Name | Requires |
|-------|------|----------|
| L0 | Kernel Core | the Bridge Variation Space; zero-allocation O(1) resolution of `(base, selector)` pairs |
| L1 | Bridge & Neography | L0 plus the RNUR neography variation space and the plane mapping of §5 |
| L2 | Plugin & ExtSUCS | L1 plus entries sourced from a mounted plugin over the ExtSUCS space |

## 2. The binary variation schema

### 2.1 The variation entry

A registered variation sequence is one `sivd_variation_entry_t`:

```c
typedef struct {
    sucs_ex_char_t base_codepoint;       /* the ideograph, e.g. U+4E00      */
    sucs_ex_char_t variation_selector;   /* U+E0100-U+E01EF, or an RNUR VS  */
    uint32_t       resolved_glyph_index; /* suf glyph id (opaque, compiled) */
    uint16_t       collection_index;     /* index into the collection table */
    uint8_t        plane;                /* suts_sivd_plane_t               */
    uint8_t        flags;                /* SUTS_SIVD_FLAG_*                */
} sivd_variation_entry_t;
```

The first three fields are the sketched core of the schema: the base, the
selector, and the glyph index the font pipeline renders. The remaining fields
are registry metadata — the owning collection, the address-space plane, and the
variation class.

All codepoint fields are `sucs_ex_char_t` (64-bit), never `uint32_t` or `char`.
This is the central SIVD deviation from the sketched structure: an RNUR
neography base in `0x00160000–0x0016FFFF`, an ExtSUCS plugin base above
`0x7FFFFFFF`, and the supplementary selectors U+E0100–U+E01EF all survive
intact, whereas a narrower field would truncate or misinterpret them.

The `flags` byte carries the variation class:

| Flag | Meaning |
|------|---------|
| `SUTS_SIVD_FLAG_NONE` | an ordinary glyphic subset |
| `SUTS_SIVD_FLAG_STYLISTIC` | a stylistic alternate |
| `SUTS_SIVD_FLAG_REGIONAL` | a regional glyph form |
| `SUTS_SIVD_FLAG_CONTEXTUAL` | a contextual alternate |

### 2.2 The collection

Registered sequences are grouped into collections, so that the glyphic subsets
of one user community are tracked together. A collection is one
`sivd_collection_t`:

```c
typedef struct {
    char     id[SUTS_SIVD_MAX_COLLECTION_ID];     /* e.g. "Adobe-Japan1"      */
    char     id_pattern[SUTS_SIVD_MAX_ID_PATTERN];/* opaque, e.g. "[0-9]+"    */
    uint16_t first_entry;                         /* first owned entry        */
    uint16_t entry_count;                         /* number of owned entries  */
    uint8_t  plane;                               /* suts_sivd_plane_t        */
    uint8_t  reserved;                            /* zero; readers ignore it  */
} sivd_collection_t;
```

`id` is at most `SUTS_SIVD_MAX_COLLECTION_ID` (32) bytes including the
terminator. `id_pattern` is the collection's identifier pattern, stored as an
**opaque descriptor**: SIVD never evaluates it, because there is no regular
expression engine on the lookup path. The registrar validates identifiers at
compile time (§7.3). Each collection owns a **contiguous run** of entries,
`[first_entry, first_entry + entry_count)`, in the static entry table, so that
enumerating a collection's glyphic subsets is an O(1) range walk.

### 2.3 Variation selectors

A variation selector is one of:

| Range | Name | Use |
|-------|------|-----|
| `U+FE00–U+FE0F` | VS1–VS16 | base-plane selectors; not admissible in an IVS |
| `U+E0100–U+E01EF` | VS17–VS256 | the Ideographic Variation Selectors |
| `0x0016FF00–0x0016FFFF` | RNUR VS | neography selectors (top of the RNUR carve-out) |

`sivd_is_variation_selector` is true for all three ranges.
`sivd_is_ivs_selector` is true for the supplementary range and the RNUR range —
that is, exactly the selectors admissible in an Ideographic Variation Sequence.
The RNUR selectors occupy the **top** 256 slots of the RNUR carve-out, disjoint
from the separators and symbols SLDF allocates at the bottom of the same
carve-out (`0x00160010–0x00160042`).

UTS #37 admits only VS17–VS256 for an IVS. SIVD keeps that range as the
Bridge-plane ideographic selectors and **extends** the same mechanics to the
RNUR plane; a user-space IVD consumer sees the Bridge range as an ordinary IVD
and is unaffected by the RNUR extension.

Unlike UTS #37, the number of admitted selectors is not a fixed budget of 240
that triggers encoding more selectors when exhausted: the RNUR plane provides
its own 256 selectors, and the ExtSUCS plugin space is unbounded. A collection
that would exceed the Bridge budget on a single ideograph spills to a native
allocation rather than waiting for a future Unicode revision.

### 2.4 The resolved glyph index and `suf` integration

`resolved_glyph_index` is a **font-bundle-relative** glyph id, exactly the
`uint32_t glyph_id` that `suf` addresses (`suf_lookup_glyph_id`,
`suf_lookup_glyph_id_ext`, `suf_metric_t`). SIVD does not itself store
outlines or metrics; it maps a registered variation sequence to the glyph that
the active font bundle supplies for that subset. The mapping is compiled in
(the build step of §7 resolves each registered sequence against the font
bundle's character map) and is immutable at run time.

## 3. Variation sequences and glyphic subsets

### 3.1 The Ideographic Variation Sequence

An **Ideographic Variation Sequence** (IVS) is a sequence of two coded
characters: the first a character with the Ideographic property that is not
canonically or compatibly decomposable, the second a variation selector in
`U+E0100–U+E01EF`. SIVD generalizes this to the **variation sequence**: a valid
base (§4.4) followed by any selector of §2.3.

### 3.2 Registered and unregistered IVS

A **glyphic subset** for a character is a subset of the glyphs appropriate for
displaying that character. SIVD associates a registered sequence with a unique
glyphic subset. A sequence present in the registry is **registered**; its
intent is determined reliably by consulting SIVD. A sequence not present is
**unregistered** and MUST NOT be used in text interchange.

SIVD applies the usual rules for variation sequences. An unregistered sequence
resolves to nothing (`SUTS_SIVD_NOT_FOUND`: `sivd_lookup` returns `NULL` and
`sivd_resolve_variant` returns `false`); a registered sequence is used only to
restrict the rendering of its base to the associated subset. Because variation
selectors are default-ignorable, a registrant is expected to ensure the subset
is a subset of the glyphs acceptable for the base alone — the shapes in the
subset must be unifiable with the base.

### 3.3 Permanence and stability

To guarantee the stability of texts using registered sequences, the association
between a sequence and a glyphic subset is **permanent**: a sequence is never
reassigned to another glyphic subset. This is the SIVD §10 stability rule.

SIVD guarantees that a registered sequence corresponds to a single glyphic
subset and that the association is permanent. It does **not** guarantee that two
sequences on the same ideograph have non-overlapping or even distinct subsets,
and it does **not** guarantee that two sequences using the same selector on
different ideographs have any relationship. A selector is not designated for a
purpose independently of a base.

### 3.4 Unification rules

For characters with the `Unified_Ideograph` property, the SIVD unification rules
— matching those of UTS #37 — admit two expanded cases beyond strict shape
identity. Both are illustrated with Ideographic Description Sequences (IDSes),
because one character in each pair is unencoded or not-yet-encoded:

* **Different structure, insignificantly different.** Characters whose
  difference is not significant enough to encode them separately, for which
  strong evidence of variation can be provided. Examples:
  `⿱椎十` versus `U+69AB 榫`; `⿱汨皿` versus `U+6E29 温`.
* **Same structure, different second-level components.** Characters with
  different components at the second (or subsequent) level that may not be
  generally unifiable, for which strong evidence of variation can be provided.
  Examples: `⿰月㲋` versus `U+818D 膍`; `⿺𠃊西` versus `U+8FFA 迺`. The
  character SHOULD be rarely used and not in general circulation, and the
  registrant is expected to demonstrate similarity of glyph shape and general
  acceptance as a variant.

### 3.5 Collections and sequence sharing

Sequences are grouped in collections. If two or more sequences in a submission
correspond to the **same glyphic subset**, implementers carry a redundant
burden, and a collection becomes less likely to be implemented. Registrants are
therefore strongly encouraged (but not required) to share sequences where a new
submission is similar to an existing collection. As part of registration, the
registrar shall alert the registrant to the potential for sharing; sharing
across collections may occur with the mutual agreement of the affected
registrants.

Registration of a collection does not imply suitability for any purpose. An
implementation is free to support any combination of registered sequences,
including those from multiple collections or partial subsets of collections;
SIVD exposes exactly that granularity through `sivd_collection_at` and
`sivd_entry_at`.

## 4. The registry and O(1) resolution

### 4.1 The static table

The registry is a static array of `sivd_variation_entry_t` records, immutable
and read-only, with capacity `SUTS_SIVD_REGISTRY_CAPACITY` (128), together with
a static array of `sivd_collection_t` collections, capacity
`SUTS_SIVD_COLLECTION_CAPACITY` (16). The curated reference table holds 20
entries in 6 collections (Annex A). Extending the table is a bridge-data update
— a minor revision — and never changes the API or the lookup complexity.

### 4.2 The pair hash and the open-addressing index

The table is indexed once into a fixed array of `SUTS_SIVD_INDEX_SIZE` (256)
slots. Each slot records whether it is occupied and, if so, the table index of
its entry. The index is built by hashing each entry's **(base, selector)** pair
and placing it at `hash & (256 - 1)` with linear probing.

The key is the **FNV-1a 32-bit** hash over the little-endian encodings of the
two codepoints, with offset basis `2166136261` and prime `16777619`:

```c
h = 2166136261u;
for i in 0..7: h ^= (base_cp >> (8 * i)) & 0xFF; h *= 16777619u;
for i in 0..7: h ^= (var_sel >> (8 * i)) & 0xFF; h *= 16777619u;
```

A resolve hashes the query pair and probes until it finds a matching entry or an
empty slot. The index is built lazily on the first lookup if `sivd_init_registry`
has not been called, and it is **idempotent**. It never grows at run time.
Because `SUTS_SIVD_REGISTRY_CAPACITY` (128) is half of `SUTS_SIVD_INDEX_SIZE`
(256), the load factor stays below 1/2 and a probe terminates.

### 4.3 Complexity

| Operation | Cost |
|-----------|------|
| `sivd_init_registry` | O(n) once, n ≤ 128; no allocation |
| `sivd_resolve_variant` / `sivd_lookup` / `sivd_is_registered` | O(1) expected (bounded probes); no allocation |
| `sivd_collection_of` | O(1) expected |
| `sivd_entry_count` / `sivd_entry_at` | O(1) |
| `sivd_collection_count` / `sivd_collection_at` | O(1) |
| `sivd_is_variation_selector` / `sivd_is_ivs_selector` / `sivd_is_valid_base` | O(1) |
| `sivd_plane_of` / `sivd_pair_hash` | O(1) |

### 4.4 SCP invariance and trap exclusion

A codepoint admitted as a variation base MUST NOT be:

* a System Control Plane marker in `0x00110000–0x0011FFFF`, which is out-of-band
  control data and never part of rendered text;
* a trap slot in `0x7FFFFFF0–0x7FFFFFFE`; or
* the Sentinel `0x7FFFFFFF`.

This is the SIVD form of the SuperUnicode SCP invariance: the control plane
carries directives, not glyphs, so it can never be a variation base.
`sivd_is_valid_base` is the predicate; `sivd_lookup` and `sivd_resolve_variant`
return "not found" for a base that fails it.

A selector is constrained by the ranges of §2.3 and can never fall in the SCP,
trap, or Sentinel ranges. The reference table is validated against both rules
by the test suite (§9).

## 5. Zone and plane mapping

`sivd_plane_of` classifies a codepoint into its address-space plane in O(1),
using the boundaries of SUTR-0:

| Plane | Range | Notes |
|-------|-------|-------|
| Bridge | `0x00000000–0x0010FFFF` | permanently 1:1 with Unicode |
| SCP | `0x00110000–0x0011FFFF` | system directives |
| Native | `0x00120000–0x7FFFFFEF` | native allocations |
| RNUR | `0x00160000–0x0016FFFF` | neography carve-out (inside Native) |
| Trap | `0x7FFFFFF0–0x7FFFFFFE` | 15 unallocatable slots |
| Sentinel | `0x7FFFFFFF` | end-of-space marker |
| ExtSUCS | `> 0x7FFFFFFF` | unbounded plugin space |

The RNUR range is tested before the Native range, because it is carved out of
it. The Bridge plane and the RNUR plane are the two planes of §1.4; all other
planes are classified for completeness (an SCP, trap, Sentinel, or ExtSUCS base
is never an admissible variation base except for ExtSUCS, which is the L2
plugin space).

## 6. Font pipeline integration (`suf`)

SIVD integrates with the OpenWindows character token stream and the `suf` vector
font engine:

1. The layout pass emits codepoints one at a time. When it reads a valid base
   immediately followed by a variation selector, it forms the pair
   `(base, selector)`.
2. `sivd_resolve_variant(base, selector, &glyph)` resolves the pair in O(1) and
   returns the font-relative glyph index. On `false` the selector is
   default-ignorable and the layout pass renders the base's default glyph.
3. The glyph index feeds the `suf` outline and metric lookup
   (`suf_lookup_glyph_id_ext` / `suf_get_glyph_metric`).

The whole path is single-pass and allocation-free: one probe per variation
selector, a fixed amount of stack, and no character-map rebuild. The optional
variation class (`flags`) lets the pipeline forward a stylistic, regional, or
contextual hint to the renderer without a second lookup.

## 7. The compile-time registration model

SIVD replaces the run-time read of the IVD flat files with a **compile-time
registration model**. The registration authority still accepts submissions in
the interchange formats below and still assigns identifiers and selectors; the
difference is that the approved registrations are *compiled* into the binary
table of §4.1, and the kernel never reads a file at run time.

### 7.1 Collections interchange format

The collections interchange file, `IVD_Collections.txt`, records the registered
collections. Lines beginning with `#` and empty lines are comments; other lines
are fields separated by `;`, with insignificant leading and trailing white
space. The file is UTF-8, uses `U+000A` as the line separator, and ends with the
comment line `# EOF`. Each line has three fields:

1. the identifier of the collection;
2. a regular expression for the identifiers within the collection; all such
   identifiers match it (the expression may be extended over time); and
3. the URL of a website describing the collection.

At compile time, field 1 becomes `sivd_collection_t::id` and field 2 becomes
`sivd_collection_t::id_pattern` (the opaque descriptor of §2.2). Field 3 is
documentation and is not carried into the kernel image.

### 7.2 Sequences interchange format

The sequences interchange file, `IVD_Sequences.txt`, records the registered
sequences in the same framing. Each line has three fields:

1. the code points of the base character and the variation selector, separated
   by a space;
2. the identifier of the collection under which the sequence is registered; and
3. the identifier of the sequence, provided by the registrant, which matches the
   collection's pattern.

At compile time, field 1 becomes `base_codepoint` + `variation_selector`, field
2 becomes `collection_index`, and field 3 becomes the sequence identifier the
collection owns. The build resolves the pair against the font bundle to produce
`resolved_glyph_index` (§2.4).

### 7.3 Identifiers

Collection and sequence identifiers are character strings that begin with one of
`A`–`Z`, `a`–`z`, and continue with `A`–`Z`, `a`–`z`, `0`–`9`, `_`, `-`, `+`.
`-` and `+` are permitted for backwards compatibility with earlier
registrations. A unique programmatic identifier can be produced by folding `-`
and `+` into `_`, or by removing them altogether. The patterns conform to Perl
5.8 regular expressions in the interchange file; SIVD stores them opaquely
(§2.2) and never evaluates them.

### 7.4 Registration procedure

Registration proceeds in two stages.

1. **Register a collection.** The registrant supplies the intent of the
   collection, its principles, and any useful data. The registrar publishes a
   page summarizing the proposed collection and announces the intent publicly;
   a review period of at least **90 days** follows, during which comments and
   questions are submitted to the registrant. At the end of the period the
   registrant submits the collection registration form (Annex C) together with a
   signed statement that the registrant will make reasonable efforts to maintain
   the stability of referenced websites, that the sequences in the collection may
   be used freely without limitation, fee, or other requirement, and that all
   comments were addressed. The registrar then assigns a collection identifier
   and adds the collection.
2. **Register sequences in the collection.** The registrant supplies the
   proposed sequences, each identified by its base character only, ensuring that
   sequence identifiers are unique for each base character within the collection
   and match the collection's pattern. After the same review period, the
   registrant submits one or more representative glyphs for each sequence, and
   the registrar assigns a variation selector to each and adds the sequences.

Once a submission is approved, the registrar **compiles** the collection and its
sequences into the SIVD table (a minor revision, §10). The registration of
sequences may begin concurrently with the registration of the collection itself.

### 7.5 Registration authority

The SuperUnicode consortium acts as the registration authority and appoints a
registrar. Collections for which the authority is itself the registrant have no
special status; they undergo the same vetting as any other submission.

## 8. Reference implementation

The reference implementation ships as a freestanding, zero-allocation C99
module, `include/suts/suts_sivd.h` + `src/suts/suts_sivd.c`, registered in the
`suts_static` library.

### 8.1 Header `suts_sivd.h`

```c
#define SUTS_SIVD_SPEC_ID       "SUTS-004"
#define SUTS_SIVD_SPEC_TITLE    "System Ideographic Variation Database"
#define SUTS_SIVD_VERSION_MAJOR 1
#define SUTS_SIVD_VERSION_MINOR 0
#define SUTS_SIVD_VERSION_PATCH 0

#define SUTS_SIVD_MAX_COLLECTION_ID   32U
#define SUTS_SIVD_MAX_ENTRY_ID        16U
#define SUTS_SIVD_MAX_ID_PATTERN      32U
#define SUTS_SIVD_REGISTRY_CAPACITY   128U
#define SUTS_SIVD_COLLECTION_CAPACITY 16U
#define SUTS_SIVD_INDEX_SIZE          256U
```

### 8.2 API

| Function | Purpose |
|----------|---------|
| `sivd_version_string()` | version string |
| `sivd_init_registry()` | build the index once; idempotent, allocation-free |
| `sivd_resolve_variant(base, sel, &glyph)` | resolve a pair to a glyph index in O(1) |
| `sivd_lookup(base, sel)` | stable pointer to the entry, or `NULL` |
| `sivd_is_registered(base, sel)` | whether a mapping exists |
| `sivd_collection_of(base, sel)` | the owning collection, or `NULL` |
| `sivd_entry_count()` / `sivd_entry_at(i)` | enumerate entries |
| `sivd_collection_count()` / `sivd_collection_at(i)` | enumerate collections |
| `sivd_is_variation_selector(cp)` | any admitted selector |
| `sivd_is_ivs_selector(cp)` | an IVS-admissible selector |
| `sivd_is_valid_base(cp)` | SCP-invariant variation base |
| `sivd_plane_of(cp)` | address-space plane of a codepoint |
| `sivd_pair_hash(base, sel)` | FNV-1a hash of the pair |

### 8.3 Worked examples

| Subtask | Result |
|---------|--------|
| `sivd_resolve_variant(0x82A6, 0xE0134, &g)` | `true`, `g == 80` (collection `Example_names`, id 23) |
| `sivd_resolve_variant(0x3402, 0xE0100, &g)` | `true`, `g == 16` (collection `Adobe-Japan1`) |
| `sivd_lookup(0x4E00, 0xE0102)` | entry with glyph index 49 (collection `Moji_Joho`) |
| `sivd_collection_of(0x3402, 0xE0101)->id` | `"Hanyo-Denshi"` |
| `sivd_resolve_variant(0x160200, 0x16FF01, &g)` | `true`, `g == 100` (RNUR, `Lumer_Variants`) |
| `sivd_resolve_variant(0x4E01, 0xE0100, &g)` | `false` (unregistered) |
| `sivd_lookup(0x82A6, 0x0041)` | `NULL` (not a selector) |
| `sivd_lookup(0x110000, 0xE0100)` | `NULL` (SCP base) |
| `sivd_is_ivs_selector(0xFE00)` | `false` (VS1 is not IVS-admissible) |

## 9. Testing

The reference module is covered by `tests/test_suts_sivd.c`, which exercises the
version, selector classification, base validation and SCP invariance, the
registry and enumeration (including uniqueness of the `(base, selector)` key and
the collection runs), O(1) resolution (including miss, malformed-selector, and
SCP-invading paths), collection attribution, the plane mapping, and the pair
hash. The test is also included in the unified header-coexistence harness
(`unified/test_unified.c`), which compiles every module header in one
translation unit.

## 10. Stability and versioning

* The `SIVD` identifiers, the `sivd_variation_entry_t` layout, and the API are
  stable.
* Additive fields are appended before `reserved` or replace it in a minor
  revision; a reader ignores a field it does not know.
* The curated table may grow in a minor revision; the order of existing entries
  is stable, and **no `(base, selector)` key is ever reused for a different
  glyphic subset** (§3.3).
* Removing or re-pointing an entry is a major revision and is avoided.

## 11. Conformance requirements

An implementation claims SIVD L0 when it provides the Bridge Variation Space
with zero-allocation O(1) resolution of `(base, selector)` pairs. L1 adds the
RNUR neography variation space and the plane mapping of §5. L2 adds
plugin-sourced entries over ExtSUCS. In every level, `sivd_lookup` MUST return a
stable pointer to read-only memory, and no lookup path may allocate.

### 11.1 Compliance matrix (reference implementation)

| Requirement | Reference status |
|-------------|------------------|
| Binary, fixed-size variation entries | Yes |
| Zero heap allocation on any path | Yes |
| O(1) expected `(base, selector)` resolution | Yes |
| Unique `(base, selector)` key | Yes |
| Bridge Variation Space (IVS + supplementary selectors) | Yes |
| RNUR neography variation space | Yes |
| Collections with contiguous entry runs | Yes |
| SCP-invariant bases; selectors excluded from SCP/traps | Yes |
| ExtSUCS compatibility (64-bit fast path) | Yes |

---

## Annex A — Curated registry contents

The reference table holds 20 entries in 6 collections. The collection plane is
`Bridge` unless marked `RNUR`. Glyph indices are font-bundle-relative (§2.4).

| Collection (plane) | Pattern | Entries |
|--------------------|---------|---------|
| `Adobe-Japan1` (Bridge) | `[0-9]+` | `3402 E0100`→16, `4E00 E0100`→17, `8FBB E0100`→18, `845B E0100`→19, `5F66 E0100`→20 |
| `Hanyo-Denshi` (Bridge) | `[0-9]+` | `3402 E0101`→32, `4E00 E0101`→33, `8FBB E0101`→34, `6F22 E0101`→35 |
| `Moji_Joho` (Bridge) | `[A-Za-z0-9_]+` | `3402 E0102`→48, `4E00 E0102`→49, `FA0C E0102`→50 |
| `KRName` (Bridge) | `[0-9]+` | `9FB4 E0100`→64, `9FB5 E0100`→65 |
| `Example_names` (Bridge) | `[0-9]+` | `82A6 E0134`→80 |
| `Lumer_Variants` (RNUR) | `[a-z0-9_]+` | `160100 16FF00`→96, `160100 16FF01`→97 (stylistic), `160101 16FF00`→98, `160200 16FF00`→99 (regional), `160200 16FF01`→100 (regional) |

---

## Annex B — Worked example

This annex adapts the hypothetical example of UTS #37 Appendix B to SIVD.

Consider the ideograph `U+82A6` *ashi*, which can be rendered by several
different glyphs with different stroke shapes. In the following Japanese
sentence, the character occurs twice; the first occurrence may be displayed with
any glyph, so no restriction is expressed in plain text, while the second
occurrence is in the name of the town *Ashiya*, customarily displayed with an
older glyph form:

```
芦田さんは芦屋のお嬢様だ
```

A collection `Example_names` is registered for the representation of person and
place names, with the pattern `[0-9]+`. A single glyphic subset — identifier
`23` for base `U+82A6` — is registered against the sequence `<82A6 E0134>`. In
the SIVD table this is one entry:

```c
{ .base_codepoint = 0x82A6ULL, .variation_selector = 0xE0134ULL,
  .resolved_glyph_index = 80u, .collection_index = 4u /* Example_names */,
  .plane = SUTS_SIVD_PLANE_BRIDGE, .flags = SUTS_SIVD_FLAG_NONE }
```

The sentence is then represented by the character sequence `<82A6, 7530, …,
82A6, E0134, …>`. The first `U+82A6` has no selector; the second carries the
registered sequence, and the `suf` pipeline resolves it to glyph `80` in O(1)
without reading a registry file.

---

## Annex C — Collection registration form

Application for registration of an Ideographic Variation Sequence Collection:

* Name and address of the registrant:
* Name and email address of the representative:
* Suggested identifier for the collection:
* Pattern for the sequence identifiers:

---

## References

- **UTS #37** — Unicode Ideographic Variation Database (the user-space flat-file
  system this kernel primitive deliberately departs from)
- **SUAS-004 (SUCF)** — SuperUnicode Canonical Forms (unification basis)
- **SUAS-005 (SSP)** — System Script Property (script routing)
- **SUTS-003 (SLDF)** — System Locale Data & Formatting (the sibling
  binary-compiled kernel primitive, and the origin of the RNUR carve-out layout)
- **SUTR-0 (SUCS Core)** — the three-space layout, SCP, traps, and Sentinel
- **SUTR-5 (Plugin Lifecycle)** — the neography carve-out and plugin mounting
- **SUTR-8 (SUCPM)** — the RNUR registry space and property tiers
- **SUTR-10 (SUCM)** — the conformance model and the zero-allocation mandate
- **suf** — the SuperUnicode Font format and vector font engine (glyph indices)
