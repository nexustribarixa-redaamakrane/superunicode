# SUTS-007 — SuperUnicode Emoji & Presentation Specification (SUEPS)

**Standard ID:** SUTS-007
**Status:** Draft — Authoring
**Scope:** Modular Technical Specification (emoji presentation, ZWJ & modifier sequence resolution, pictographic processing)
**Applies To:** Base SUCS (Unicode Compatibility Bridge, SCP, Native), ExtSUCS (unbounded arbitrary-precision plane), the RNUR neography registry, SUCD property tables, the OpenWindows microkernel, the graphical subsystem, the text-rendering path, the emoji palette and input path, SUTS-004 (SIVD) variation sequences, SUTS-005 (SCSS) character security, the character token stream
**Last Updated:** 2026-10-21

---

## 1. Introduction

Emoji are pictographs (pictorial symbols) that are typically presented in a
colorful cartoon form and used inline in text. They represent things such as
faces, weather, vehicles and buildings, food and drink, animals and plants, or
icons that represent emotions, feelings, or activities.

Emoji on smartphones and in chat and email applications have become extremely
popular worldwide. As of March 2015, for example, Instagram reported that
"nearly half of text [on Instagram] contained emoji." Individual emoji also
vary greatly in popularity (and even by country). Emoji are most often used in
quick, short social media messages, where they connect with the reader and add
flavor, color, and emotion. Emoji do not have the grammar or vocabulary to
substitute for written language. In social media, emoji make up for the lack of
gestures, facial expressions, and intonation that are found in speech. They
also add useful ambiguity to messages, allowing the writer to convey many
different possible concepts at the same time.

The word emoji comes from Japanese:

> 絵 (e ≅ picture) 文字 (moji ≅ written character).

Emoji may be represented internally as graphics or they may be represented by
normal glyphs encoded in fonts like other characters. These latter are called
*emoji characters* for clarity. Some Unicode characters are normally displayed
as emoji; some are normally displayed as ordinary text, and some can be
displayed both ways.

### 1.1 Relationship to UTS #51

SUEPS solves the same problem as UTS #51, *Unicode Emoji*: it says which
characters are emoji, which are displayed by default in emoji style versus text
style, which take a skin-tone modifier, and how an emoji sequence is composed.
It is deliberately **not** a UTS #51 analog, because a text emoji library and a
kernel presentation primitive differ in four structural ways:

1. **Surrogate pairs vs. atomic scalars.** In UTF-16 an emoji above the BMP is
   a *surrogate pair* — 😀 is `U+1F600`, encoded as the two code units
   `\uD83D\uDE00` — and every consumer must recombine the pair before it can
   name the character. SuperUnicode has **no surrogate mechanism at all**: per
   SUTR-1 (SUTF) invariant 2, `U+1F600` is one atomic 31-bit scalar, and the
   block `U+D800–U+DFFF` is ordinary valid PUA that is never paired, reserved,
   or rejected (§2). SUEPS therefore never recombines anything.
2. **String parser vs. flattened table.** UTS #51 leaves an emoji modifier
   sequence, a ZWJ sequence, a flag pair, a keycap sequence, and a tag sequence
   to a sequence-aware scanner over a variable-length string. SUEPS flattens
   every admitted chain into static binary transition tables at build time, so a
   renderer resolves a base + modifier pair by one integer hash and one bounded
   probe — O(1), no scan (§4).
3. **Heap allocation vs. zero allocation.** A user-space emoji layer loads the
   `emoji-test.txt` annotation set into hash maps and buffers each sequence
   stage on the heap. OpenWindows enforces a strict zero-allocation mandate for
   core subsystems. SUEPS resolves every property through a fixed-size, static,
   open-addressing index, and holds no per-query state (§3).
4. **Modular optional compliance.** Core operating system primitives, file
   paths, and kernel IPC channels require strict text safety rather than
   graphical emoticons. A lightweight server kernel or a headless environment
   can omit SUEPS entirely without impacting core system integrity (§1.6).

Where this document shares vocabulary with UTS #51 (emoji presentation, text
presentation, modifier base, ZWJ sequence, RGI), the requirements are normative
for SUEPS only.

### 1.2 Key architectural shifts

| Dimension | UTS #51 (user-space) | SUTS-007 SUEPS (kernel) |
|-----------|----------------------|--------------------------|
| Representation | UTF-16 code units; surrogate pairs above the BMP | One atomic 31-bit SUCS codepoint per emoji |
| Sequence handling | String parser, ZWJ / modifier scan | Flattened static binary transition tables |
| Allocation | Heap annotation maps and per-stage buffers | Zero; one static table + one fixed index |
| Lookup | Parse a data line, then a map lookup | codepoint → FNV-1a → fixed probe, O(1) |
| Modifiers | Re-scan `base + modifier` as a unit | `(base, modifier)` → hash → probe, O(1) |
| Surrogate block | The `0xD800–0xDFFF` pairing mechanism | Ordinary valid PUA; never paired (§2) |
| Dependence | A text-shaping / rendering stack | A kernel primitive, no user-space service |
| Compliance | A single conformance statement | Modular optional: omittable outright (§1.6) |

### 1.3 The dual-layer architecture

SUEPS is a **dual-layer** system, in the same relationship SUAS-009 (SIP),
SUAS-012 (SUCD), and SUTS-006 (SIDN) have with their Unicode counterparts:

* **Offline reference layer — SUCD.** The human-readable emoji annotation files,
  sequence charts, and presentation property manifests live in the SuperUnicode
  Character Database for user-space applications, font designers, and developer
  toolchains.
* **Runtime kernel layer — SUEPS.** Build-time tooling compiles those emoji
  datasets into immutable, byte-packed binary lookup tables embedded directly
  into graphical subsystem images. The kernel never reads a file at run time.

> The seeding brief for this specification named its offline reference layer
> "SUCA". That is stale: in this corpus **SUCA** is already the SuperUnicode
> Collation Algorithm (**SUTS-001**), and the SuperUnicode character database is
> **SUCD** (**SUAS-012**). SUEPS therefore names `SUCD` as its reference layer,
> matching SUAS-009 (SIP), SUAS-010 (SSRE), SUAS-011 (SCMS), SUAS-012 (SUCD),
> SUAS-013 (SISR), SUAS-014 (SUVTL), SUTS-005 (SCSS), and SUTS-006 (SIDN). The
> house prefix `SUEPS_*` and the sketched entry points `suts_sueps_init_registry`,
> `suts_sueps_get_entry`, and `suts_sueps_resolve_sequence` are preserved
> verbatim. The record type keeps the sketch's field layout, `sequence_hash`
> and `modifier_base` included.

### 1.4 Terminology and cross-references

SUEPS reuses the SuperUnicode architecture documents, numbered as they exist in
this repository:

| SUEPS concept | Authoritative document |
|---------------|------------------------|
| Codepoint space, SCP, traps, Sentinel | **SUTR-0 (SUCS Core)** — three-space layout |
| The "No Surrogates" rule, atomic scalars | **SUTR-1 (SUTF)** — invariant 2 |
| Bridge / Native / ExtSUCS planes | **SUTR-7 (SUCEM)**, **SUTR-6 (Bridge)** |
| RNUR neography registry, property tiers | **SUTR-8 (SUCPM)** — the character property model |
| Variation sequences and presentation selectors | **SUTS-004 (SIVD)** |
| Character security, homographs | **SUTS-005 (SCSS)** — UTS #39 analog |
| Emoji collation and search ordering | **SUTS-001 (SUCA)** — UCA-equivalent collation |
| Locale data, the `-em` extension and `Zsye`/`Zsym` | **SUTS-003 (SLDF)** |
| Conformance model and zero-allocation mandate | **SUTR-10 (SUCM)** |
| Offline reference data | **SUCD** — the SuperUnicode Character Database (SUAS-012) |

> The reference implementation ships as `include/suts/suts_sueps.h` +
> `src/suts/suts_sueps.c`. The library and function names (`suts_sueps.h`,
> `suts_sueps_*`) follow the house convention of the sibling SUTS modules. The
> record type is `sueps_emoji_entry_t`.

### 1.5 Conformance

A conforming SUEPS implementation MUST:

1. represent emoji presentation properties as immutable, fixed-size records
   reachable directly from a codepoint key, with **no heap allocation** on any
   query path;
2. resolve a codepoint to its presentation flags in **O(1) expected time** by a
   bounded probe of a fixed-size index, not by a linear scan of a table;
3. resolve a base + modifier pair from the flattened transition table in
   **O(1) expected time**, with no string re-scan and no dynamic buffer;
4. obey the **"No Surrogates" mandate**: never pair, reserve, or reject a
   codepoint in the block `U+D800–U+DFFF`, which SUTR-1 makes ordinary valid PUA
   (§2);
5. admit no System Control Plane marker, trap slot, the Sentinel, or an ExtSUCS
   plugin codepoint as emoji material (§2.2); and
6. treat a codepoint that carries no curated record by the **total algorithmic
   default** of §3.4, so every property query answers for every codepoint.

### 1.6 SUEPS conformance levels (modular optionality)

SUEPS is a **modular** specification. The architectural contract — the record
layout, the lookup, the flattened transition table, the "No Surrogates" rule —
is mandatory for an implementation that *claims* SUEPS support. But a
lightweight server kernel or a headless environment MAY omit the module
entirely: because core primitives, file paths, and kernel IPC channels require
strict text safety rather than graphical emoticons, an implementation with no
SUEPS ships no emoji records and remains conformant to the rest of the corpus.
Where SUEPS is claimed, the levels are:

| Level | Name | Requires |
|-------|------|----------|
| L0 | Kernel Core | the emoji property table; zero-allocation O(1) presentation resolution; the algorithmic default |
| L1 | Sequences | L0 plus the flattened modifier transition table and sequence-token hashes |
| L2 | Full repertoire | L1 plus tag sequences, flag pairs, hair and color components, and the RNUR neography emoji (§4) |

## 2. The "No Surrogates" mandate

### 2.1 The invariant

SUTR-1 (SUTF), §1.2, states the second non-negotiable invariant of the
SuperUnicode transformation formats:

> **No surrogates.** SuperUnicode has no surrogate mechanism. The range
> `0xD800`–`0xDFFF` is an ordinary block of valid PUA codepoints and MUST be
> encoded as such — never paired, never reserved, never rejected.

SUEPS inherits this rule directly. A Unicode emoji encoded above the BMP is one
**surrogate pair** in UTF-16 — `U+1F600` is `\uD83D\uDE00` — and every consumer
must recombine the pair before it can name the character. SUEPS has no such
step, because there is no such pair:

| | UTF-16 (UTS #51) | SuperUnicode (SUEPS) |
|---|---|---|
| 😀 `U+1F600` | two code units `D83D DE00`, recombined | one atomic 31-bit scalar `0x0001F600` |
| `U+D83D` alone | an unpaired surrogate: ill-formed | an ordinary valid PUA codepoint |
| `U+DC00` alone | an unpaired surrogate: ill-formed | an ordinary valid PUA codepoint |

The practical consequences for the engine are three:

1. **No recombination step.** `suts_sueps_get_entry` takes an already-atomic
   31-bit codepoint. There is nothing to combine and nothing to look ahead at.
2. **No defect for the block.** `U+D800–U+DFFF` is emoji-component material like
   any other PUA. `suts_sueps_is_pua_half` classifies the block; it is **not** a
   validity test, and no SUEPS entry point refuses a codepoint because it lies
   there.
3. **Total over the space.** Every query answers in one comparison chain or one
   bounded probe, for every 31-bit value — no branch for a half-character, and
   no reserved range.

### 2.2 Zones

SUEPS does not invent a second plane taxonomy. `suts_sueps_zone_of` classifies a
codepoint into the SUCD / SSP planes with the RNUR carve-out made explicit:

| Zone | Range | Renderable |
|------|-------|------------|
| `SUEPS_ZONE_BRIDGE` | `0x00000000–0x0010FFFF` | yes |
| `SUEPS_ZONE_SCP` | `0x00110000–0x0011FFFF` | no |
| `SUEPS_ZONE_RNUR` | `0x00160000–0x0016FFFF` | yes |
| `SUEPS_ZONE_NATIVE` | rest of `0x00120000–0x7FFFFFEF` | yes |
| `SUEPS_ZONE_TRAP` | `0x7FFFFFF0–0x7FFFFFFE` | no |
| `SUEPS_ZONE_SENTINEL` | `0x7FFFFFFF` | no |
| `SUEPS_ZONE_EXTSUCS` | `> 0x7FFFFFFF` | no |

`suts_sueps_is_renderable` admits exactly the Bridge, RNUR, and Native zones. A
codepoint in the Control Plane, a trap slot, the Sentinel, or the ExtSUCS plugin
space is never emoji material, and the resolution engine refuses it before
consulting the table. Note that the former-surrogate block `U+D800–U+DFFF` sits
squarely inside the Bridge zone: it is renderable, because SUTR-1 makes it
ordinary PUA.

## 3. The compiled emoji schema

### 3.1 Definitions (transcribed from UTS #51 §1.4)

The aspiration-level definitions the engine encodes:

| Label | Term | Meaning |
|-------|------|---------|
| ED-1 | emoji | a colorful pictograph usable inline in text: an image, a character, or a sequence |
| ED-3 | emoji character | a character with the Emoji property |
| ED-4 | extended pictographic character | a character with the Extended_Pictographic property (used for segmentation) |
| ED-5 | emoji component | a character used in emoji sequences that is not an independent palette choice |
| ED-6 | default emoji presentation character | Emoji_Presentation=Yes |
| ED-7 | default text presentation character | Emoji=Yes, Emoji_Presentation=No |
| ED-8 | text presentation selector | `U+FE0E` VS15 |
| ED-9 | emoji presentation selector | `U+FE0F` VS16 |
| ED-11 | emoji modifier | Emoji_Modifier=Yes |
| ED-12 | emoji modifier base | Emoji_Modifier_Base=Yes |
| ED-13 | emoji modifier sequence | `emoji_modifier_base emoji_modifier` |
| ED-14 | emoji flag sequence | two Regional_Indicator characters |
| ED-14a | emoji tag sequence | `tag_base tag_spec tag_end` |
| ED-14c | emoji keycap sequence | `[0-9#*] FE0F 20E3` |
| ED-16 | emoji ZWJ sequence | an emoji sequence with at least one joiner |
| ED-17 | emoji sequence | a core, tag, or ZWJ sequence |
| ED-18 | fully-qualified emoji | a qualified emoji whose every element is qualified |
| ED-19 | unqualified emoji | an emoji that is neither fully- nor minimally-qualified |
| ED-20 | basic emoji set | emoji characters and emoji presentation sequences for general input |
| ED-22…ED-25 | RGI modifier / flag / tag / ZWJ sets | the recommended-for-general-interchange subsets |
| ED-27 | RGI emoji set | the union of ED-20 through ED-25 |

Per ED-4, the Extended_Pictographic set contains all the Emoji characters
*except for some Emoji_Component characters*; SUEPS therefore answers
`suts_sueps_is_extended_pictographic` true for a curated emoji that is not a
component and false for a component such as a hair component or VS16.

### 3.2 The emoji record

A compiled per-codepoint emoji record is one `sueps_emoji_entry_t`:

```c
typedef struct {
    uint32_t codepoint;          /* Direct 31-bit codepoint (No Surrogates) */
    uint32_t sequence_hash;      /* Precomputed token hash for chains        */
    uint16_t presentation_flags; /* Packed attribute flags                   */
    uint8_t  modifier_base;      /* Associated base character reference      */
    uint8_t  reserved;           /* must be zero                             */
} sueps_emoji_entry_t;
```

The record is exactly **12 bytes** and byte-aligned, so an array of records is
directly indexable in read-only memory. The sketch's field layout is preserved
verbatim. `sequence_hash` is the precomputed FNV-1a token hash for a ZWJ /
modifier chain headed by this codepoint (zero when the code point heads no
curated chain), and `modifier_base` is the index of the associated base
character, or 0 when the codepoint is itself a base. `codepoint` is a 31-bit
SUCS value; the record never appears for an ExtSUCS codepoint above
`0x7FFFFFFF`.

### 3.3 The fixed index and the hash

An emoji query hashes the codepoint and probes a fixed, open-addressing index:

```c
static sueps_index_slot_t sueps_index[SUEPS_ENTRY_INDEX_SIZE]; /* 256 slots */
```

The index size (`256`) is a power of two larger than the table capacity (`128`),
so the load factor stays below one-half, the probe sequence stays short and
bounded, and a lookup is **O(1) expected time** with a small constant. The key
hash is FNV-1a 32-bit over the four little-endian bytes of the codepoint
(`SUEPS_FNV_OFFSET_BASIS = 2166136261`, `SUEPS_FNV_PRIME = 16777619`). The
sequence-token hash, `suts_sueps_hash_sequence`, is FNV-1a over the eight
little-endian bytes of the `(base, modifier)` pair, so it is order-sensitive:
`(👍, 🏽)` and `(🏽, 👍)` hash differently. The index is built lazily on the
first use and is idempotent; no query path allocates.

### 3.4 Totality by default

The reference emoji table is a **curated subset** of the offline SUCD dataset. A
codepoint with no record is not therefore *not* an emoji: the property function
is **total** over the space, answering by an algorithmic default. `is_emoji`,
`is_emoji_presentation`, and `is_extended_pictographic` return true for an
uncurated codepoint in the emoji planes the engine recognizes (the
`0x1F000–0x1FAFF` pictographic blocks and the `0x2190–0x27BF` symbols), while
`is_modifier`, `is_component`, `is_regional_indicator`, and `is_tag_character`
each have a total range rule. A codepoint outside every emoji plane resolves
false, and never as an error.

## 4. Sequence resolution

### 4.1 The flattened transition table

UTS #51 leaves an emoji modifier sequence — `emoji_modifier_base
emoji_modifier` (ED-13) — to a sequence-aware scanner: it recognizes a base, then
a following skin-tone modifier, and treats the pair as one unit. SUEPS flattens
every admitted pair into a static table at build time:

```c
typedef struct {
    uint32_t base;      /* 31-bit base codepoint     */
    uint32_t modifier;  /* 31-bit modifier codepoint */
    uint32_t token;     /* precomputed sequence token */
} sueps_modifier_pair_t;
```

A renderer resolves `👍 + 🏽` by one hash and one bounded probe, with no scan and
no dynamic buffer. The reference table is a representative slice of the RGI
emoji modifier sequence set (ED-22); the resolution contract is identical for
every additional row, and extending the set is a data update.

### 4.2 The resolution outcome

`suts_sueps_resolve_sequence` (the sketched entry point) and
`suts_sueps_resolve_sequence_ex` (the same resolution with a diagnostic channel)
resolve a base + modifier pair. `SUEPS_OK` is the only success value:

| Code | Meaning |
|------|---------|
| `SUEPS_ERR_NULL_ARGUMENT` | a NULL output descriptor |
| `SUEPS_ERR_NOT_EMOJI` | the base carries no emoji record |
| `SUEPS_ERR_BAD_BASE` | the base is not an emoji modifier base (ED-12) |
| `SUEPS_ERR_BAD_MODIFIER` | the second codepoint is not an emoji modifier (ED-11) |
| `SUEPS_ERR_ILLEGAL_ZONE` | a Control Plane / trap / Sentinel / ExtSUCS argument |
| `SUEPS_ERR_UNQUALIFIED` | the pair is not a qualified sequence |

`suts_sueps_status_code_name` renders any of them. On success the output record
is the base record with `SUEPS_FLAG_MODIFIER` and `SUEPS_FLAG_ZWJ_SEQUENCE` set,
`modifier_base` zeroed (the modifier was consumed), and `sequence_hash` set to
the precomputed pair hash — zero when the pair is well-formed but outside the
curated RGI subset.

### 4.3 Hair, color, direction, and gender components

The same flattening covers the component classes UTS #51 §2.8–§2.11 describes:

* **Hair components** (Emoji 11.0): `U+1F9B0` red, `U+1F9B1` curly, `U+1F9B2`
  bald, `U+1F9B3` white. Each is an emoji component, never a palette choice.
* **Color** (`U+2B1B`, `U+2B1C`, `U+1F7E5`–`U+1F7EB`): a ZWJ sequence asks for a
  base color.
* **Facing direction** (`U+2B05`, `U+27A1`): a ZWJ sequence asks for a direction.
* **Gender** (`U+2640`, `U+2642`): a sign-format ZWJ sequence adds explicit
  gender to a human-form emoji.

Each is a component record in the compiled table, so a renderer resolves a
combining chain by direct pointer indexing rather than by parsing a string.

## 5. Presentation style

### 5.1 Selectors

Every curated emoji character records whether it is default emoji presentation
(ED-6) or default text presentation (ED-7). Two components carry a
presentation-selector flag:

| Codepoint | Name | Effect |
|-----------|------|--------|
| `U+FE0E` | VARIATION SELECTOR-15 (VS15) | request a text presentation (ED-8) |
| `U+FE0F` | VARIATION SELECTOR-16 (VS16) | request an emoji presentation (ED-9) |

`suts_sueps_is_presentation_selector` is the O(1) predicate. A selector is a
component, never an emoji alone: `suts_sueps_is_emoji(0xFE0F)` is false. The
valid presentation *sequences* are the property of SUTS-004 (SIVD), which owns
the variation database; SUEPS owns the presentation *style* of the underlying
emoji character.

### 5.2 Default presentation

The engine distinguishes the three categories UTS #51 §4 names: emoji-default
(`is_emoji_presentation`), text-default (`is_text_default`), and text-only
(`is_emoji` false). A renderer picks its default from these flags and the
environment, exactly as UTS #51 §4 describes for formal, web, and chat
contexts.

## 6. Ordering and search

Emoji ordering is not code-point order. UTS #51 §5 and §7 delegate the ordering
and the annotations to CLDR-style data. In this corpus those are the property of
SUTS-001 (SUCA), the UCA-equivalent collation engine, and of SUTS-003 (SLDF),
the binary-compiled locale schema that carries the annotations and the CLDR
short names. SUEPS supplies the presentation flags those layers key on and does
not itself define a sort order, an annotation set, or a palette category.

## 7. Complexity

| Operation | Cost |
|-----------|------|
| `suts_sueps_get_entry`, `suts_sueps_is_emoji`, and the other property predicates | O(1) expected: one hash, one bounded probe |
| `suts_sueps_resolve_sequence` | O(1) expected: one pair hash, one probe |
| `suts_sueps_zone_of`, `is_renderable`, `is_neography` | O(1) comparison chain |
| `suts_sueps_is_modifier`, `is_regional_indicator`, `is_tag_character`, `is_pua_half` | O(1) range test |
| `suts_sueps_hash_codepoint`, `hash_sequence` | O(1), no allocation |

No path allocates, grows a table, or calls into libc. There is no recombination
step, because there is no surrogate pair to recombine.

## 8. The curated registry

The reference implementation ships a curated subset of the offline dataset, in
strictly ascending codepoint order (an ordering that is a **verification
invariant**, not a convenience: enumeration and the duplicate check both depend
on it). Extending the registry is a data update — a minor revision — and never
changes the API. The curated set exercises every presentation class §3 defines:
keycap bases, the enclosing keycap, VS15 and VS16, the ZWJ, default-text
symbols, Regional Indicators, the five modifiers, a tag base (black flag),
modifier bases and ZWJ heads, faces, hair components, and two RNUR neography
emoji.

## 9. Worked example

Consider a renderer that draws the emoji `👍🏽` ("thumbs up: medium skin tone"),
which UTS #51 writes `U+1F44D U+1F3FD`. In UTF-16 that is the four code units
`D83D DC4D D83C DFFD` and a parser must recombine two surrogate pairs. Under
SUEPS it is two atomic scalars, resolved in one call:

| Step | Input | Result |
|------|-------|--------|
| 1 | base `0x1F44D` | a modifier base, `SUEPS_FLAG_MODIFIER_BASE` set |
| 2 | modifier `0x1F3FD` | `suts_sueps_is_modifier` true (ED-11) |
| 3 | `resolve_sequence(0x1F44D, 0x1F3FD)` | the base record with `MODIFIER` and `ZWJ_SEQUENCE` set, `sequence_hash` = the pair token |

No pairing, no re-scan, no heap, and no branch for an ill-formed half.

---

## Annex A — Emoji properties and data files

The binary emoji properties SUEPS compiles from the SUCD manifests, transcribed
from UTS #51 Annex A:

| Property | Abbr | Predicate |
|----------|------|-----------|
| Emoji | Emoji | `suts_sueps_is_emoji` |
| Emoji_Presentation | EPres | `suts_sueps_is_emoji_presentation` |
| Emoji_Modifier | EMod | `suts_sueps_is_modifier` |
| Emoji_Modifier_Base | EBase | `suts_sueps_is_modifier_base` |
| Emoji_Component | EComp | `suts_sueps_is_component` |
| Extended_Pictographic | ExtPict | `suts_sueps_is_extended_pictographic` |

If `Emoji=No`, then `Emoji_Presentation=No`, `Emoji_Modifier=No`, and
`Emoji_Modifier_Base=No`. The offline source files are `emoji-data.txt`,
`emoji-variation-sequences.txt`, `emoji-zwj-sequences.txt`, `emoji-sequences.txt`,
and `emoji-test.txt`; they live in SUCD, and build-time tooling distils them into
the record table and the flattened transition table this module embeds.

## Annex B — Valid emoji flag sequences

While the syntax of a well-formed emoji flag sequence is defined in ED-14, only
valid sequences display as flags. `suts_sueps_is_regional_indicator` recognizes
the 26 Regional_Indicator codepoints `U+1F1E6–U+1F1FF`; a singleton Regional
Indicator is not a well-formed flag sequence. The set of valid region pairs,
their ordering, and the flag presentation rules are the property of SUTS-001
(SUCA) and SUTS-003 (SLDF), which key on the pair of indicators SUEPS
classifies.

## Annex C — Valid emoji tag sequences

A well-formed emoji tag sequence (ED-14a) is `tag_base tag_spec tag_end`, where
`tag_spec` is a run of `U+E0020–U+E007E` and `tag_end` is `U+E007F` CANCEL TAG.
The entire sequence must not exceed 32 code points. `suts_sueps_is_tag_character`
recognizes the tag block; `U+1F3F4` BLACK FLAG is the curated `tag_base`. The
validity criteria for a particular `tag_spec` — the CLDR subdivision and
region-subtag rules — are an offline SUCD dataset, and the display of an invalid
or unsupported tag sequence is a rendering concern outside the property engine.

---

## Annex D — Compliance matrix (reference implementation)

| Requirement | Reference status |
|-------------|------------------|
| Binary, fixed-size emoji records (12 bytes) | Yes |
| Zero heap allocation on any path | Yes |
| O(1) expected presentation resolution | Yes |
| O(1) expected modifier-sequence resolution | Yes |
| Flattened ZWJ / modifier transition table | Yes |
| "No Surrogates": `U+D800–U+DFFF` as ordinary PUA | Yes |
| Unique, ascending codepoint key in the table | Yes |
| Total property functions (algorithmic default) | Yes |
| Bridge + RNUR coverage | Yes |
| SCP invariance; traps and Sentinel refused | Yes |
| Modular optionality (module omittable) | Yes |
| 31-bit SUCS and unbounded ExtSUCS compatibility | Yes |

---

## References

- **UTS #51** — Unicode Emoji (the user-space spec this kernel primitive
  deliberately departs from)
- **SUTR-1 (SUTF)** — the "No Surrogates" invariant the mandate comes from
- **SUTS-004 (SIVD)** — the variation-sequence database that owns the
  presentation selectors and their valid sequences
- **SUTS-005 (SCSS)** — character security, the sibling `suts_*` kernel
  primitive SUEPS composes with
- **SUTS-001 (SUCA)** — emoji collation and ordering (UTS #51 §5, §7)
- **SUTS-003 (SLDF)** — locale data, the `-em` extension, and `Zsye`/`Zsym`
  script codes (UTS #51 §4.2, §4.3)
- **SUAS-005 (SSP)** — the script property engine
- **SUAS-012 (SUCD)** — SuperUnicode Character Database (the offline reference
  layer, and the repository's `SUCD`, *not* the `SUCA` of the seeding brief)
- **SUTR-0 (SUCS Core)** — the three-space layout, SCP, traps, and Sentinel
- **SUTR-6 (Unicode Compatibility Bridge)** — the permanent 1:1 guarantee
- **SUTR-7 (SUCEM)** — the four-level character encoding model
- **SUTR-8 (SUCPM)** — the RNUR registry space and property tiers
- **SUTR-10 (SUCM)** — the conformance model and the zero-allocation mandate
