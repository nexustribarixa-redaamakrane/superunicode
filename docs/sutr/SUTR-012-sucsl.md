# SUTR-012 — SuperUnicode Cuneiform Sign Lists (SUCSL)

**Standard ID:** SUTR-012
**Status:** 1.0.0 — Published
**Scope:** The binary-mapped Cuneiform sign schema, the compiled sign hierarchy, the zero-allocation O(1) sign, compound and sequence lookup engine, and the native extension model for custom epigraphic sign lists across the Cuneiform, Cuneiform Numbers and Punctuation, and Early Dynastic Cuneiform blocks of the Bridge plane
**Applies To:** SUAS-012 (SUCD), SUAS-005 (SSP), SUTR-0 (SUCS Core), SUTR-1 (SUTF), SUTR-6 (Bridge), SUTR-10 (SUCM), SUTR-11 (SUMS), the SUF vector font engine, and every cuneiform text, corpus and rendering consumer
**Last Updated:** 2026-10-22

---

## 1. Overview

Where UTR #56 *Unicode Cuneiform Sign Lists* documents how the Cuneiform
blocks of the Unicode Standard were assembled — the sign lists, the values,
the compound signs, the numerical notation and the ligature conventions — and
leaves the mapping of a particular sign to a particular value to prose, a
text-based catalog parser or an application-specific table, **SUTR-012
(SUCSL)** establishes the equivalent contract for SuperUnicode: the
**binary-mapped Cuneiform sign schema**, the **packed sign record**, the
**compound composition pointer** and the **zero-allocation O(1) resolution
engine** that every SuperUnicode consumer uses instead of a runtime catalog.

SUCSL is a modular technical-report specification, a peer of SUTR-11 (SUMS)
and SUTS-008 (SUSCH). It is the cuneiform member of the same family:

| Report | Counterpart | Domain | Compiled surface |
|--------|-------------|--------|------------------|
| SUTR-11 (SUMS) | UTR #54 | Mongolian shaping | 12-byte shaping record, FVS table |
| SUTS-008 (SUSCH) | UTS #39 | Source-code safety | Identifier profile, confusable table |
| **SUTR-012 (SUCSL)** | **UTR #56** | **Cuneiform sign lists** | **12-byte sign record, compound and sequence tables** |

The report is normative for the record layout, the flag vocabulary, the hash
function, the lookup contract and the degradation rules. It is informative for
the *contents* of the curated registry: the signs, values, wedge counts and
period tags are version data compiled from the SUCD manifests and may grow
without changing the engine.

### 1.1 Relation to UTR #56

UTR #56 is a **Unicode Technical Report**, not a UAX. It does not define new
characters and it does not add normative properties to the code charts. It
records how the Cuneiform repertoire was selected and how the sign lists were
built, so that implementations can recover the intent behind a code chart whose
glyphs are only representative. It covers three areas:

1. The principles of Cuneiform encoding: signs, values, sequences, mergers and
   splits, representative glyphs, sign names and ligatures.
2. The sign lists themselves: the Oracc Sign List ([OSL]) and the older
   Assyrian Sign List ([ASL]), their machine-readable form, and the signs that
   were withdrawn.
3. References to the epigraphic literature from which the repertoire derives.

SUCSL keeps those three areas but changes their implementation status. The
principles become the normative schema of §2 and §4. The sign lists become the
compiled registry of §8. The references become the provenance notes of §3.
Nothing in the report depends on a runtime text file, a locale, a regular
expression or a heap allocation.

### 1.2 Scope and terminology

The key words **MUST**, **MUST NOT**, **SHOULD**, **SHOULD NOT** and **MAY** in
this report are to be interpreted as described in RFC 2119.

A **sign** is an abstract Cuneiform character with a stable codepoint. A
**value** is a reading or meaning conventionally assigned to that sign: a
Sumerian logogram, an Akkadian syllable, a Hittite reading, a numeral. A sign
is **polyvalent** when it has more than one value. A **compound sign** is a
sign whose structure is written as one sign built from other signs, such as
GUD×KUR. A **sequence** is a sign that the encoding represents as an ordered
list of other signs rather than as one atomic codepoint, such as GEME₂ =
SAL.KUR. A **merger** is a historical process in which two signs become one;
a **split** is the reverse.

The unit of the engine is one 31-bit SuperUnicode codepoint. SUTR-0 (SUCS Core)
defines the three-space layout and the Sentinel. SUTR-1 (SUTF) defines the
transport forms. SUCSL defines only what a Cuneiform codepoint means to a sign
consumer.

### 1.3 Key architectural concepts

#### 1.3.1 Pre-compiled binary sign hierarchies

A traditional implementation parses a sign-list file, builds a dictionary of
sign names to codepoints, and resolves a value by searching strings at run time.
SUCSL replaces the dictionary with a **static array of fixed-size records**
sorted by codepoint, plus an open-addressing index built once. A codepoint is
the key; the record is the value. There is no string comparison on the hot
path, and there is no per-query allocation.

The hierarchies are:

- **Codepoint → record.** The 12-byte `sucsl_sign_entry_t` of §4.2.
- **Compound codepoint → components.** The 24-byte `sucsl_compound_t` of §4.6.
- **Sequence name hash → components.** The same descriptor with `sign == 0`.
- **Registry ordinal → record.** The 1-based hierarchy index of §4.8.

#### 1.3.2 The four functional categories

Every curated sign is classified into one or more of four functional classes:
**Numerical**, **Syllabic**, **Logographic** and **Composite**. The classes are
packed into the `sign_flags` word of §4.3. They are not mutually exclusive in
the data, because a sign such as AM is at once a logogram, a syllabogram and a
structural compound. A single-category projection is provided for consumers
that need one label; its precedence is normative and is stated in §4.3.

#### 1.3.3 Values are hashes, not strings

The engine stores a 32-bit FNV-1a hash of the sign's primary value skeleton,
not the value string. This keeps the record at 12 bytes and keeps the module
freestanding. The hash is an index and a hint, not an identity: two different
values may collide, and the engine never treats a hash match as proof that two
values are equal. Identity is carried by the codepoint. A consumer that needs
the spelling reads it from the SUCD manifest, not from SUCSL. §4.4 states the
normalization rules that make the hash reproducible.

#### 1.3.4 Compounds and sequences are separate tables

An atomic compound sign has a codepoint and a decomposition. A pure sequence
has no atomic codepoint; it is a spelling convention for a sign that the
encoding represents with two or three existing signs. Mixing the two in one
namespace would make `sign == 0` ambiguous and would force a linear scan.
SUCSL therefore keeps `sucsl_compounds[]` (non-zero key, binary search by
codepoint) and `sucsl_sequences[]` (zero key, binary search by name hash)
separate. §4.6 specifies both.

#### 1.3.5 The RNUR extension

The Native & Neography Plane of SUAS-005 (SSP) reserves
`0x00160000–0x0016FFFF` for user-defined writing systems. SUCSL treats that
range as a first-class extension space: an uncurated RNUR codepoint receives a
synthesized record with the `SUCSL_FLAG_EXTENSION` bit, and a user-space
toolchain may overlay its own sign table using the same binary layout. No kernel
patch and no new codepoint assignment are required. §5 specifies the range and
the set numbering.

### 1.4 The Cuneiform specification matrix

| Concern | UTR #56 treatment | SUCSL treatment | Section |
|---------|-------------------|-----------------|---------|
| Sign identity | Code chart + prose | Curated record keyed by codepoint | §4.2 |
| Functional class | Prose description | Four packed flags + projection | §4.3 |
| Value | Value list in prose | FNV-1a hash of an ASCII skeleton | §4.4 |
| Compound sign | "X times Y" name | Component pointer table | §4.6 |
| Sequence | "written as A.B" | Name-hash sequence table | §4.6 |
| Numeral | Numerical sign charts | `NUMERICAL` + `VARIANT` flags | §2.1.2 |
| Period | Glyph commentary | 8-bit period tag | §4.5 |
| Rendering hint | Representative glyph | Wedge count byte | §4.5 |
| Sign names | Unicode name | Structural-name skeleton | §2.5 |
| Ligature | Prose recommendation | `LIGATURE` compound flag | §2.6 |
| Deprecated sign | Sign-list note | `DEPRECATED` flag, retained | §3.3 |
| Custom sign list | Not defined | RNUR extension records | §5 |

### 1.5 Cross-references

| Specification | Relationship |
|---------------|--------------|
| SUTR-0 (SUCS Core) | Defines the 31-bit codepoint, the three spaces and the Sentinel. |
| SUTR-1 (SUTF) | Defines the transport forms that carry a Cuneiform codepoint. |
| SUTR-6 (Bridge) | Allocates the Cuneiform blocks inside the Bridge plane. |
| SUAS-005 (SSP) | Defines the RNUR carve-out used by §5. |
| SUAS-012 (SUCD) | The offline character database and the OSL/ASL manifests that feed the curated table. |
| SUTR-10 (SUCM) | The conformance model; SUCSL publishes claim `sucsl` at tier L0. |
| SUTR-11 (SUMS) | The sibling modular report for Mongolian shaping; SUCSL reuses its zone model and hash discipline. |
| SUTS-008 (SUSCH) | Source-code safety; a Cuneiform identifier is screened by SUSCH, not by SUCSL. |

### 1.6 Why SUCSL is a modular SUTR

Ancient-script support is not a bootstrapping requirement of a microkernel. An
image that never renders or indexes cuneiform text should not carry 3 KiB of
sign tables. SUCSL is therefore a **modular** report: its engine is
self-contained, its tables are static, and its omission does not affect any
other specification. A kernel that includes it gets the O(1) contract; a kernel
that omits it remains conformant to SUCS Core, SUTF and the Bridge, and simply
does not resolve Cuneiform signs.

The module boundary is the header `sutr/sutr_sucsl.h`. It includes only
`<stdint.h>`, `<stdbool.h>` and `<stddef.h>`, and it compiles under
`-ffreestanding -nostdlib`.

---

## 2. Principles of Cuneiform Encoding

### 2.1 Cuneiform signs

A Cuneiform sign is a conventional arrangement of wedges impressed into clay.
The encoding does not preserve the impression; it preserves the **abstract
sign**, exactly as a Latin encoding preserves the letter and not the typeface.
The code chart shows one **representative glyph** per sign, usually a
Neo-Assyrian or Old Babylonian form chosen for legibility. That glyph is not
normative and is not the only correct rendering (§2.4).

A sign acquires its values from the scribal tradition. The same sign can be a
Sumerian word, an Akkadian syllable, a Hittite gloss and a numeral, sometimes in
the same document. SUCSL records one **primary value** per curated sign and
marks the functional classes; it does not attempt to enumerate every reading.
The complete reading list belongs to the SUCD manifest, which is a text
resource and is not compiled into the kernel.

#### 2.1.1 Transliteration

Transliteration is the ASCII representation of a sign sequence used by
assyriologists and by the OSL. SUCSL defines a **value skeleton** so that a
32-bit hash is reproducible across tools and releases. The normalization rules
are:

| Input convention | Skeleton rule | Example |
|------------------|---------------|---------|
| Lowercase or uppercase | Fold to lowercase ASCII | `AM` → `am` |
| Diacritics (`š`, `ḫ`, `ṭ`, `ṣ`, `ū`, `ā`) | Fold to the ASCII digraph or base letter | `meš` → `mesh`, `ḫu` → `hu` |
| Homophone subscript (`₂`, `₃`, …) | Append the ASCII digit | `u₂` → `u2` |
| Variant marker `@v` | Drop the marker and its argument | `a@v` → `a` |
| Compound multiplication (`×`) | Replace with ASCII `x` | `GUD×KUR` → `gudxkur` |
| Sequence separator (`.`) | Replace with ASCII `-` | `SAL.KUR` → `sal-kur` |
| Numeric subscript in a name | Preserve the digit | `GAR₃` → `gar3` |
| Whitespace and punctuation | Remove, except the separator | `NE ŠEŠIG` → `nesheshig` |

The skeleton is a **hash key**, not a display form. A user interface continues
to show `GUD×KUR`, `meš` and `u₂`; only the hash sees `gudxkur`, `mes` and `u2`.
The `@v` rule deserves emphasis: when a transliteration offers an alternative
reading, the alternative does not create a second sign. It selects the same
primary value and is recorded as a variant through `SUCSL_FLAG_VARIANT` or as a
sequence descriptor. This is why the variant forms of the numerals in §2.1.2
share a value skeleton with their plain counterparts but carry a different
codepoint and the `VARIANT` bit.

The "diš-less" notation follows from the same discipline. The numeral 1 is
written `diš`, but a lone vertical wedge is frequently omitted in
transliteration when the context makes the quantity clear. The skeleton of the
bare numeral is therefore `dish`, not `1dish`; a consumer that wants the
quantity reads the codepoint, not the spelling.

#### 2.1.2 Numerals and stacking patterns

Cuneiform numerals are a separate repertoire. The **Cuneiform Numbers and
Punctuation** block (`U+12400–U+1247F`) contains precomposed numeric signs for
the common quantities, while the main block contains the wedge signs from which
larger numbers are stacked. The important cases are:

| Concept | Plain form | Variant form | Encoding note |
|---------|-----------|--------------|---------------|
| One | DIŠ `U+12079` | — | A lone wedge; often omitted in transliteration |
| One (AŠ) | AŠ `U+12038` | — | Also a syllabic sign; `dil` in some lists |
| Ten | U `U+1230B` | — | One wedge, ten-value |
| Twenty | U U `U+12399` | — | Two stacked tens; Early Dynastic graphic |
| Thirty | U U U `U+1230D` | — | Three stacked tens |
| Three | 3(diš) `U+12408` | — | Three vertical wedges |
| Four | 4(diš) `U+12409` | 4(limmu) `U+1243C` | Ur III vs Neo-Assyrian graphic |
| Five | 5(diš) `U+1240A` | 5(aš) `U+12403` | Different wedge families |
| Seven | 7(diš) `U+1240C` | 7(imin) `U+12441` / `U+12442` | Two variant traditions |
| Eight | 8(diš) `U+1240D` | 8(ussu) `U+12444` / `U+12445` | Two variant traditions |
| Nine | 9(diš) `U+1240E` | 9(ilimmu) `U+12446` / `U+12447` | Two variant traditions |

The **stacking pattern** is the rule that a numeral is built by repeating a
wedge a fixed number of times. It is not a font feature: the precomposed
codepoint already names the quantity. `U+12409` (four-diš) and `U+1243C`
(four-limmu) are not interchangeable glyph variants of one abstract character;
they are two encoded characters with different histories. SUCSL preserves the
distinction through two different records, both `NUMERICAL`, one additionally
`VARIANT`, and both tagged with their period. A consumer that normalizes them
would destroy exactly the evidence an epigrapher needs.

`U+12399` (U U) is the twenty-graphic; `U+12409` is four-diš. They are in
different blocks and were encoded for different reasons. The report calls out
the pair because a naive "repeat the wedge" renderer can mistake one for the
other when it ignores the block boundary. SUCSL's block predicate (§4.1)
prevents that mistake at the API boundary.

### 2.2 Sequences

Some signs have no atomic codepoint. The encoding represents them as an ordered
sequence of existing signs, and the sequence is the identity. The canonical
examples are:

| Sign | Sequence | Codepoints | Meaning |
|------|----------|------------|---------|
| GEME₂ | SAL.KUR | `U+122A9 U+121B3` | female servant |
| DIRI | SI.A | `U+122DB U+12000` | to be more, to exceed |
| IDIGNA | GU₂.GAR₃ | `U+12118 U+120FC` | the Tigris |
| DALLA | MAŠ.GU₂.GAR₃ | `U+12226 U+12118 U+120FC` | a type of sprout |
| U₅ | ḪU.SI | `U+12137 U+122DB` | a bird; a ligature |
| Aššur | AŠ.ŠUR | `U+12038 U+122E9` | the god and the city |
| SIKI.LAM | SIK₂.LAM | `U+122E0 U+121F4` | wool |
| me-eš | ME.EŠ₂ | `U+12228 U+120A0` | the sequence spelling of `meš` |

A sequence is not a canonical decomposition of an atomic sign. GEME₂ is not
"the character SAL followed by the character KUR" in the way a Latin base plus
combining mark is a decomposition; it is a spelling convention whose referent
is a single lexical item. SUCSL therefore does not apply normalization to it.
The sequence is emitted as written, and the `sucsl_sequences[]` row tells a
consumer that the two signs form the named item.

### 2.3 Mergers and splits

The cuneiform repertoire is the product of three thousand years of scribal
change. Signs merge, split, and change value. The encoding freezes one moment of
that process, but the sign list must remember the others. SUCSL captures the
history in three places: the primary value hash, the period tag, and the
`VARIANT` flag.

The signs **ŠAR₂** (`U+122B9`), **ḪI** (`U+1212D`), **TI₂** (`U+12397`) and
**DIN** (`U+12077`) are standard examples. Each has a reference glyph that is
characteristic of a particular period — ŠAR₂ of the Early Dynastic corpus,
TI₂ of the Old Assyrian tradition, DIN of the later tradition — while its value
is attested across a wider span. A consumer that renders an Old Babylonian
document with a Neo-Assyrian glyph is not making an encoding error, but it is
making a historical one. The `historical_period` byte exists so that a font
engine can select a style-appropriate glyph without re-parsing the document.

MEŠ (`U+1238C`) is an atomic sign. The spelling `me-eš` is a sequence of ME
(`U+12228`) and EŠ₂ (`U+120A0`). They are not two spellings of one character:
MEŠ is a single abstract sign, and `me-eš` is two signs whose combination a
reader interprets as the same word. SUCSL stores MEŠ in `sucsl_signs[]` and
`me-eš` in `sucsl_sequences[]`, under different keys, so neither can be
mistaken for the other. A text that uses the wrong one is a different text.

#### 2.3.1 Mergers and splits of sequences

The same distinction applies to **ŠEŠ₂** (`U+122C1`) and **SIKI.LAM**
(`U+122E0 U+121F4`). ŠEŠ₂ is an atomic sign; SIKI.LAM is a sequence. Their
reference glyphs can resemble one another in some styles, and a careless
transliteration can collapse them. SUCSL keeps the atomic sign in the sign
table and the sequence in the sequence table; `sutr_sucsl_is_sign()` is true for
the first and false for the second, and `sutr_sucsl_find_sequence()` is true for
the second and false for the first.

| Case | Atomic codepoint | Sequence key | Distinguishing predicate |
|------|------------------|--------------|--------------------------|
| MEŠ vs me-eš | `U+1238C` | `hash("me-esh")` | `is_sign(0x1238C)`, no sequence `hash("mesh")` |
| ŠEŠ₂ vs SIKI.LAM | `U+122C1` | `hash("siki-lam")` | `is_sign(0x122C1)`, sequence has two components |
| DIRI vs SI.A | none | `hash("diri")` | `is_sign(0x122DB) && is_sign(0x12000)` |
| GEME₂ vs SAL.KUR | none | `hash("geme2")` | `is_sign(0x122A9) && is_sign(0x121B3)` |

The general rule is: **if the encoding gives the sign an atomic codepoint, it
is a sign; if it does not, it is a sequence.** SUCSL never invents an atomic
codepoint for a sequence and never decomposes an atomic sign into a sequence.
This is the single most important invariant of the module, and §8.3 tests it
directly.

### 2.4 Representative glyphs

The glyph in a code chart is a representative. It is usually chosen for
legibility and for continuity with the sign list, not because it is the only
attested form. The same sign may be written horizontally or vertically, with
more or fewer wedges, and with different proportions in different periods. The
encoding is not the glyph.

SUCSL exposes two rendering hints and no glyph data:

- `wedge_count` — the number of constituent wedges in the reference form. It
  is a hint for a font engine that selects among pre-designed glyphs; it is not
  a constraint on the rendering. A value of zero means "unknown".
- `historical_period` — the chronological tag of the reference form. A font
  engine may use it to choose an Early Dynastic, Ur III, Old Babylonian, Old
  Assyrian, Middle Assyrian, Neo-Assyrian or Hellenistic style.

The actual outlines belong to the SUF vector font engine. SUCSL does not embed
an outline, a bitmap or a variation axis. This separation is deliberate: a
kernel image may need to index signs without carrying a font, and a font may be
updated without rebuilding the sign registry.

### 2.5 Sign names

A Unicode character name is an algorithmic identifier, not a value. The name
`CUNEIFORM SIGN GUD TIMES KUR` records the structure of the sign: GUD combined
with KUR. It does not say that the sign is read `am`, and it does not say that
`am` is the only reading. The name is stable across releases; the value is a
scholarly interpretation.

SUCSL preserves the structural name as a **name hash** in the compound and
sequence descriptors. The skeleton rule is the same as §2.1.1, with one
addition: the word `TIMES` becomes `x`. A sign whose name records a wedge
modification rather than a multiplication, such as `NE SHESHIG`, has no `x`
component; its conventional value `bil2` is the key. Thus:

| Unicode name | Structural skeleton | Hash |
|--------------|---------------------|------|
| `CUNEIFORM SIGN GUD TIMES KUR` | `gudxkur` | `0xCB74D2C5` |
| `CUNEIFORM SIGN DUB TIMES ESH2` | `dubxesh2` | `0xAF07276A` |
| `CUNEIFORM SIGN GUM TIMES SHE` | `gumxshe` | `0x80FA16FE` |
| `CUNEIFORM SIGN DUB TIMES SHE` | `dubxshe` | `0x36817CDA` |
| `CUNEIFORM SIGN NE SHESHIG` | `bil2` | `0x714E5A9E` |

The last row shows the exception: a sign whose name records a modification
rather than a multiplication has no `x` component, and its conventional value
`bil2` is the key. The table is explicit, so the exception never has to be
guessed at run time.

### 2.6 Ligatures

A ligature is a single glyph that represents a sequence of signs. It is a
rendering choice, not an encoding. The sequence remains the encoded
representation. The canonical cases are:

| Ligature | Sequence | Codepoints | Notes |
|----------|----------|------------|-------|
| U₅ | ḪU.SI | `U+12137 U+122DB` | bird; discretionary in most styles |
| DIRI | SI.A | `U+122DB U+12000` | the sequence is the identity |
| Aššur | AŠ.ŠUR | `U+12038 U+122E9` | theophoric and toponymic |
| um-me | UM.ME | `U+1231D U+12228` | scholarly convention |
| nenni | NE.NE | `U+12248 U+12248` | conventional ligature |

A sequence row carries `SUCSL_COMPOUND_LIGATURE` when the conventional
rendering joins the components. The flag is a hint to the font engine; it is
not a requirement. A renderer that emits two separate glyphs is still correct.

#### 2.6.1 Discretionary ligatures

A discretionary ligature is one that a font MAY apply and a text MAY forbid.
SuperUnicode has no separate control codepoint for cuneiform ligatures. The
format controls **ZWNJ** (`U+200C`) and **ZWJ** (`U+200D`) retain their general
meaning: ZWNJ requests that the adjacent signs not be joined, and ZWJ requests
that they be treated as a unit. Neither control is part of the Cuneiform
blocks, and neither is stored in a `sucsl_sign_entry_t`.

The recommended model is:

1. The encoded text contains the component signs in order.
2. A sequence descriptor with the `LIGATURE` flag announces that a conventional
   joined glyph exists.
3. The font engine applies the ligature unless a ZWNJ intervenes.
4. A ZWJ may force the joined form where the font would otherwise break it.

Because the ligature is discretionary, two conformant renderers may produce
different glyphs for the same sequence. They do not disagree about the text.
SUCSL's job is to tell them that the sequence is a known one, not to force a
particular outline.

### 2.7 Early Dynastic Cuneiform

The **Early Dynastic Cuneiform** block (`U+12480–U+1254F`) contains signs from
the third millennium BCE that are not simply older drawings of the main-block
signs. Some are distinct abstract characters with their own codepoints. SUCSL
curates a representative set — `AB×NUN TENU` (`U+12480`), `DIM×IGI`
(`U+1248A`) and `LAK-648×SHESH+KI` (`U+12514`) — and tags each with the Early
Dynastic period. The block predicate of §4.1 includes it, so an uncurated Early
Dynastic sign still receives a synthesized record rather than a rejection.

The Early Dynastic block is also where the `LAK` sign numbering of the sign
lists is most visible. A `LAK-648` name is a sign-list index, not a value;
SUCSL stores it as the structural skeleton `lak648` and leaves the value hash
to the manifest. This is the same treatment as any other structural name.

---

## 3. The Oracc Sign List

### 3.1 [OSL] and [ASL]

The **Oracc Sign List** ([OSL]) is the modern machine-readable sign list
maintained by the Open Richly Annotated Cuneiform Corpus. It assigns each sign
a name, a Unicode encoding, a list of values and a set of notes. The older
**Assyrian Sign List** ([ASL]) remains useful as a concordance: its numbering
appears in the epigraphic literature long before Unicode, and many publications
cite it. The two lists do not always agree on the boundaries of a sign, which
is precisely why an implementation needs a compiled, versioned snapshot rather
than a runtime guess.

SUCSL does not replace either list. It defines the binary form into which a
conforming toolchain compiles one. The lists remain the scholarly authority for
*which* signs exist and *what* they mean; SUCSL is the authority for how a
compiled sign is represented and resolved.

### 3.2 The compiled manifest

The build pipeline is:

1. **Import.** SUCD reads the OSL and ASL manifests and records every sign it
   can map to a SuperUnicode codepoint.
2. **Normalize.** Each value and structural name is reduced to the skeleton of
   §2.1.1, and the FNV-1a hash is computed.
3. **Curate.** A human reviewer resolves conflicts, assigns the primary value,
   the functional flags, the wedge count and the period tag, and marks
   withdrawn signs.
4. **Emit.** The compiler writes `sucsl_signs[]`, `sucsl_compounds[]` and
   `sucsl_sequences[]` in strictly ascending order, together with the
   compile-time geometry asserts.
5. **Verify.** A reproducibility check recomputes every hash and every sort
   order before the image is linked.

The kernel never performs steps 1–5. It links the result. A release that adds
a sign is a data change; the engine and the ABI are unchanged. This is the same
division of labour that SUTR-11 uses for Mongolian and SUTS-008 uses for
source-code confusables.

The following table maps an OSL record onto the compiled fields:

| OSL field | SUCSL field | Transformation |
|-----------|-------------|----------------|
| Unicode codepoint | `codepoint` | Copied verbatim; must be in a Cuneiform block |
| Primary value | `primary_value_hash` | Skeleton, then FNV-1a |
| Sign class | `sign_flags` | Mapped to the four functional bits |
| Sign name | `sucsl_compound_t.name_hash` | Structural skeleton, then FNV-1a |
| Wedge count | `wedge_count` | Copied, clamped to 0–255 |
| Period note | `historical_period` | Mapped to the eight period tags |
| Deprecation note | `SUCSL_FLAG_DEPRECATED` | Set when the OSL withdraws the sign |
| Variant note | `SUCSL_FLAG_VARIANT` | Set for an alternate stacking tradition |

### 3.3 Deprecated signs

A **deprecated sign** is one that the standardized list has withdrawn. It is
not deleted from the encoding, because legacy corpora contain it and a text
that used it is still a text. SUCSL retains the record and sets
`SUCSL_FLAG_DEPRECATED`. A conforming producer MUST NOT emit a deprecated sign
in new text; a conforming consumer MUST still resolve one that it receives.

The three signs called out by UTR #56 are curated. The explanatory notes in the
sign lists differ; SUCSL preserves the flag and leaves the prose to the SUCD
manifest.

| Sign | Codepoint | Status | Reason |
|------|-----------|--------|--------|
| DUB×EŠ₂ | `U+1207F` | Deprecated | Withdrawn; retained for legacy text |
| DUB×ŠE | `U+12376` | Deprecated | Withdrawn; retained for legacy text |
| GUM×ŠE | `U+12124` | Deprecated | Withdrawn; retained for legacy text |

The records remain in `sucsl_signs[]` with their flags, and their compound
decompositions remain in `sucsl_compounds[]` with
`SUCSL_COMPOUND_DEPRECATED`. This is important for corpus migration: a tool can
detect a deprecated sign, report its codepoint, and propose the replacement
without a string search and without a separate exception table.

### 3.4 Correspondence to SUCSL records

| Sign-list question | SUCSL answer |
|--------------------|--------------|
| Does this codepoint have a curated record? | `sutr_sucsl_is_sign(cp)` |
| What is its function? | `sutr_sucsl_sign_category(cp)` and the flag predicates |
| What is its primary value? | `sutr_sucsl_primary_value_hash(cp)` |
| How many wedges in the reference form? | `sutr_sucsl_wedge_count(cp)` |
| Which period is the reference form from? | `sutr_sucsl_historical_period(cp)` |
| What is its sign-list ordinal? | `sutr_sucsl_sign_index(cp)` |
| What are its components? | `sutr_sucsl_get_composition(cp, &c)` |
| Is this spelling a sequence? | `sutr_sucsl_find_sequence(hash, &c)` |
| Is the sign withdrawn? | `sutr_sucsl_is_deprecated(cp)` |

---

## 4. The SUCSL Schema

### 4.1 Zones and blocks

SUCSL reuses the zone taxonomy of SUTR-11 (SUMS) and SUAS-005 (SSP). A
Cuneiform sign lives in the **Bridge** plane; the **SCP** is text space but
carries no sign; the **RNUR** is the extension space; the **Native** plane
carries native objects; the **Trap**, **Sentinel** and **ExtSUCS** zones are not
text space.

| Zone | Range | SUCSL behavior |
|------|-------|----------------|
| Bridge | `0x00000000–0x0010FFFF` | Cuneiform blocks are sign space; other values are not |
| SCP | `0x00110000–0x0011FFFF` | Text space, no sign; `NOT_CUNEIFORM` |
| RNUR | `0x00160000–0x0016FFFF` | Extension sign space; synthesized record |
| Native | rest of `0x00120000–0x7FFFFFEF` | Native object, no sign; `NOT_CUNEIFORM` |
| Trap | `0x7FFFFFF0–0x7FFFFFFE` | `ILLEGAL_ZONE` |
| Sentinel | `0x7FFFFFFF` | `ILLEGAL_ZONE` |
| ExtSUCS | `0x80000000–0xFFFFFFFF` | Plugin space; `ILLEGAL_ZONE` |

The three Cuneiform blocks are:

| Block | Range | Size | Curated examples |
|-------|-------|------|------------------|
| Cuneiform | `U+12000–U+123FF` | 922 codepoints | A, AN, DIŠ, ME, GUD×KUR, DUB×EŠ₂ |
| Cuneiform Numbers and Punctuation | `U+12400–U+1247F` | 128 codepoints | 4(diš), 4(limmu), 7(imin), 1(ban₂) |
| Early Dynastic Cuneiform | `U+12480–U+1254F` | 208 codepoints | AB×NUN TENU, DIM×IGI, LAK-648 |

The ranges are inclusive. `U+123FF` is a Cuneiform codepoint; `U+12400` is a
Numbers codepoint; the boundary is not a gap. `U+12550` is outside all three
blocks and is not a sign for SUCSL, even though it lies in the Bridge plane.

### 4.2 The sign record

The sign record is fixed-size, padding-free and byte-aligned. It is the
structure the user sketch specifies, and it is the ABI:

| Offset | Field | Type | Size | Meaning |
|--------|-------|------|------|---------|
| 0 | `codepoint` | `uint32_t` | 4 | Raw 31-bit SuperUnicode Cuneiform codepoint |
| 4 | `primary_value_hash` | `uint32_t` | 4 | FNV-1a of the primary value skeleton |
| 8 | `sign_flags` | `uint16_t` | 2 | Packed classification flags (§4.3) |
| 10 | `wedge_count` | `uint8_t` | 1 | Constituent wedges in the reference form; 0 = unknown |
| 11 | `historical_period` | `uint8_t` | 1 | Chronological tag (§4.5) |

`sizeof(sucsl_sign_entry_t)` is exactly 12 bytes. The header publishes the
structure; the implementation contains compile-time asserts for the size and
for every offset, so a compiler that inserts padding fails the build rather
than silently changing the ABI.

The curated table obeys four invariants:

1. Every `codepoint` is in a Cuneiform block.
2. The table is **strictly ascending** in `codepoint`.
3. Every record has a non-zero `primary_value_hash` and a non-zero
   `wedge_count`.
4. Every `sign_flags` value is a subset of `SUCSL_FLAG_ASSIGNED_MASK`.

The first invariant keeps `is_sign()` and `is_cuneiform()` consistent. The
second makes the hierarchy index stable and the enumeration deterministic. The
third prevents an uncurated row from masquerading as a curated one. The fourth
keeps the flag word forward-compatible: bits 8–15 are reserved and MUST read
as zero in a curated record.

### 4.3 Sign flags and categories

The four flags of the sketch occupy bits 0–3:

| Flag | Value | Meaning |
|------|-------|---------|
| `SUCSL_FLAG_LOGOGRAM` | `1U << 0` | Functions as an independent word sign |
| `SUCSL_FLAG_SYLLABIC` | `1U << 1` | Functions as a phonetic syllable value |
| `SUCSL_FLAG_NUMERICAL` | `1U << 2` | Part of a numerical notation system |
| `SUCSL_FLAG_COMPOSITE` | `1U << 3` | Built from multiple wedge components |

Four derived flags occupy bits 4–7:

| Flag | Value | Meaning |
|------|-------|---------|
| `SUCSL_FLAG_DEPRECATED` | `1U << 4` | Withdrawn or erroneous sign (§3.3) |
| `SUCSL_FLAG_SEQUENCE` | `1U << 5` | Encoded as a sequence of signs (§2.2) |
| `SUCSL_FLAG_VARIANT` | `1U << 6` | Variant stacking or glyph family (§2.1.2) |
| `SUCSL_FLAG_UNCLASSIFIED` | `1U << 7` | Synthesized record with no curated flags |

`SUCSL_FLAG_ASSIGNED_MASK` is `0x00FF`. A synthesized RNUR record additionally
carries `SUCSL_FLAG_EXTENSION` (`1U << 8`), which distinguishes a custom
epigraphic sign from a Bridge sign. Bits 9–15 are reserved.

The flags **overlap by design**. AM (`U+12120`) is a logogram, a syllabogram
and a composite; GUD (`U+1211E`) is a logogram and a syllabogram but not a
composite. A consumer that needs the full classification reads the flags. A
consumer that needs exactly one label uses the projection
`sutr_sucsl_category_of(flags)`, whose precedence is normative:

| Precedence | Condition | Category |
|------------|-----------|----------|
| 1 | `NUMERICAL` set | `SUCSL_CATEGORY_NUMERICAL` |
| 2 | `SYLLABIC` set | `SUCSL_CATEGORY_SYLLABIC` |
| 3 | `LOGOGRAM` set | `SUCSL_CATEGORY_LOGOGRAPHIC` |
| 4 | `COMPOSITE` set | `SUCSL_CATEGORY_COMPOSITE` |
| 5 | otherwise | `SUCSL_CATEGORY_UNKNOWN` |

The order is chosen so that the *function* wins over the *structure*. AM is
classified as Syllabic, not Composite, because a reader who asks "what is this
sign doing in this text?" is usually asking about its reading. A sign whose only
flag is `COMPOSITE`, such as DUB×ŠE (`U+12376`), projects to Composite because
it has no functional class of its own.

### 4.4 Primary-value hashing

The primary value is hashed with **FNV-1a 32-bit**:

```c
h = 2166136261u;
for each byte b of the ASCII skeleton:
    h ^= b;
    h *= 16777619u;
```

The constants are `SUCSL_FNV_OFFSET_BASIS` and `SUCSL_FNV_PRIME`. The function
`sutr_sucsl_hash_value(const char *skeleton)` implements the loop without a
libc dependency. A NULL pointer returns 0; the empty string returns the offset
basis. Both behaviours are tested.

The skeleton rules are the table of §2.1.1. The important consequences are:

- `am` and `AM` both hash to `0x4024F153`.
- `a@v` normalizes to `a` and hashes to `0xE40C292C`, the same as `a`; the
  marker selects a variant reading without changing the primary key.
- `meš` normalizes to `mesh` and hashes to `0xA100BEBC`. The sequence
  `me-eš` normalizes to `me-esh` and hashes to `0xC53E2B7A`. The two hashes
  are different because MEŠ (`U+1238C`) is atomic and `me-eš` is a sequence.
- `GUD×KUR` and `gudxkur` both hash to `0xCB74D2C5`.
- `GEME₂` and `geme2` both hash to `0x0D95F4AB`.
- `GUD×KUR` and `gudxkur` both hash to `0xCB74D2C5`.
- `GEME₂` and `geme2` both hash to `0x0D95F4AB`.

The hash is **not a checksum of the complete reading list**. It is a stable
index of the primary value only. A sign with five readings has one primary
value and four alternates that live in SUCD. A consumer MUST NOT infer the
absence of a reading from a hash mismatch.

A 32-bit hash can collide. SUCSL therefore treats a hash as a *filter*, never
as an identity. The codepoint is the identity; the hash is a fast pre-check for
a dictionary keyed by value. If two curated signs share a value hash (a
homophone), both records are correct; the engine does not merge them, and the
hierarchy index distinguishes them. The curated table is checked for duplicate
codepoints, not for duplicate hashes.

### 4.5 Wedge count and historical period

`wedge_count` is a **rendering hint**, not a spelling. It counts the
constituent wedges in the reference form, from 1 to 9 in the curated set. A
consumer MAY use it to choose among pre-designed glyphs, to estimate advance
width, or to decide whether a ligature is plausible. A consumer MUST NOT reject
a sign because its count is unexpected; the count is version data and may be
revised when the reference glyph changes.

`historical_period` is one of nine tags:

| Tag | Value | Period |
|-----|-------|--------|
| `SUCSL_PERIOD_UNKNOWN` | 0 | No chronological classification |
| `SUCSL_PERIOD_EARLY_DYNASTIC` | 1 | Early Dynastic IIIa–IIIb |
| `SUCSL_PERIOD_UR_III` | 2 | Ur III |
| `SUCSL_PERIOD_OLD_BABYLONIAN` | 3 | Old Babylonian |
| `SUCSL_PERIOD_OLD_ASSYRIAN` | 4 | Old Assyrian |
| `SUCSL_PERIOD_MIDDLE_ASSYRIAN` | 5 | Middle Assyrian |
| `SUCSL_PERIOD_NEO_ASSYRIAN` | 6 | Neo-Assyrian |
| `SUCSL_PERIOD_HELLENISTIC` | 7 | Hellenistic |
| `SUCSL_PERIOD_ALL` | 8 | Attested across the whole record |

The tag describes the **reference form**, not the sign's entire history. ŠAR₂
is tagged Early Dynastic because its reference glyph is Early Dynastic; its
value is attested later as well. A synthesized record is always
`SUCSL_PERIOD_UNKNOWN`; an unknown period is not evidence of an unknown sign.

### 4.6 Compound composition and sequences

The compound descriptor is 24 bytes:

| Offset | Field | Type | Size | Meaning |
|--------|-------|------|------|---------|
| 0 | `sign` | `uint32_t` | 4 | Atomic compound codepoint, or 0 for a sequence |
| 4 | `name_hash` | `uint32_t` | 4 | FNV-1a of the structural-name skeleton |
| 8 | `component_a` | `uint32_t` | 4 | First component codepoint |
| 12 | `component_b` | `uint32_t` | 4 | Second component, or 0 |
| 16 | `component_c` | `uint32_t` | 4 | Third component, or 0 |
| 20 | `component_count` | `uint8_t` | 1 | 1–3 |
| 21 | `compound_flags` | `uint8_t` | 1 | `SUCSL_COMPOUND_*` bits |
| 22 | `reserved` | `uint16_t` | 2 | Must be zero |

The flags are:

| Flag | Value | Meaning |
|------|-------|---------|
| `SUCSL_COMPOUND_SEQUENCE` | `1U << 0` | Text is the component sequence |
| `SUCSL_COMPOUND_LIGATURE` | `1U << 1` | A style-dependent font ligature exists |
| `SUCSL_COMPOUND_DEPRECATED` | `1U << 2` | Withdrawn or erroneous sign |
| `SUCSL_COMPOUND_EARLY` | `1U << 3` | Third-millennium-only analysis |

`SUCSL_COMPOUND_FLAG_MASK` is `0x00FF`.

The **atomic compound table** is keyed by `sign` and sorted strictly ascending.
It contains only records whose `sign` is non-zero. The **sequence table** is
keyed by `name_hash` and sorted strictly ascending; every row has `sign == 0`.
The two tables are separate because their keys have different types and because
a linear scan over a mixed table would make the `sign == 0` case slow and
ambiguous.

The invariants are:

1. In `sucsl_compounds[]`, every `sign` is a curated sign and carries
   `SUCSL_FLAG_COMPOSITE`.
2. Every `component_a` is a curated sign; `component_b` is non-zero when
   `component_count >= 2`; `component_c` is non-zero when `component_count == 3`.
3. In `sucsl_sequences[]`, every row has `sign == 0`, a non-zero `name_hash`,
   the `SEQUENCE` flag, and two or three curated components.
4. Both tables are strictly ascending in their key.
5. `reserved` is zero in every row.

The lookup functions are:

- `sutr_sucsl_get_composition(sign, &c)` — binary search in the compound table.
- `sutr_sucsl_find_sequence(name_hash, &c)` — binary search in the sequence
  table.
- `sutr_sucsl_compound_at(i, &c)` and `sutr_sucsl_sequence_at(i, &c)` —
  enumeration in table order.

A failed lookup leaves the output untouched and returns `false`; it does not
zero the caller's structure. This matches the rest of the corpus and lets a
caller distinguish "not found" from "found a zero record".

### 4.7 O(1) resolution

The sign lookup is an open-addressing hash table over the curated array. The
index has `SUCSL_SIGN_INDEX_SIZE` (512) slots of `{ uint16_t entry; uint8_t
used; }`, and the table has at most `SUCSL_SIGN_CAPACITY` (160) records. The
load factor is therefore below one half, and the index is strictly larger than
the capacity, so a free slot always exists.

The algorithm is:

```text
init:
    clear the 512 slots
    for i in 0 .. count-1:
        slot = hash_codepoint(signs[i].codepoint) & 511
        while slot is used: slot = (slot + 1) & 511
        slot.entry = i; slot.used = 1
    ready = 1

lookup(cp):
    if not ready: init
    slot = hash_codepoint(cp) & 511
    repeat at most 512 times:
        if slot is empty: return not found
        if signs[slot.entry].codepoint == cp: return &signs[slot.entry]
        slot = (slot + 1) & 511
    return not found
```

`hash_codepoint` is FNV-1a over the four little-endian bytes of the codepoint.
It is a different function from `hash_value`: one hashes a codepoint key, the
other hashes an ASCII skeleton. Keeping them separate prevents a value hash
from accidentally being used as a table slot.

The probe is bounded by the table size, not by the number of records, so the
worst case is 512 comparisons and the expected case is one. The index is built
lazily on the first query and is idempotent; a second call to
`sutr_sucsl_init_registry()` returns immediately. The build is allocation-free
and touches only static storage. A multithreaded application SHOULD call
`sutr_sucsl_init_registry()` once during single-threaded start-up; after that,
lookups are read-only and may run concurrently.

### 4.8 Hierarchy indices and enumeration

`sutr_sucsl_sign_index(cp)` returns the **1-based** position of a curated sign
in `sucsl_signs[]`, or 0 when the codepoint is not curated. The index is the
SUCSL analog of a sign-list number: it is stable for a given compiled table and
is useful for compact references and for cross-checking a manifest. It is not a
Unicode property and it is not stable across releases that insert a sign.

`sutr_sucsl_sign_at(i, &e)` enumerates the table in **0-based** order.
`sutr_sucsl_sign_count()` returns the number of curated records. The pair is
consistent: for every `i` in `[0, count)`, `sign_at(i)` succeeds and
`sign_index(sign_at(i).codepoint) == i + 1`. The enumeration is deterministic
because the table is sorted, and it is the supported way to walk the registry
without knowing a codepoint.

---

## 5. The RNUR Extension

The **Native & Neography User Range (RNUR)** is the SuperUnicode carve-out for
writing systems that are not yet part of the standard repertoire. SUAS-005
(SSP) places it at `0x00160000–0x0016FFFF`, inside the Native plane but treated
as text space. SUCSL uses the same range as its **custom sign-list space**.

The range is exactly 65,536 codepoints, divided into **256 sets of 256**:

| Property | Value |
|----------|-------|
| Minimum | `0x00160000` (`SUCSL_RNUR_MIN`) |
| Maximum | `0x0016FFFF` (`SUCSL_RNUR_MAX`) |
| Size | 65,536 codepoints |
| Set size | 256 codepoints (`SUCSL_RNUR_SET_SIZE`) |
| Set count | 256 |
| Set number | `(cp - SUCSL_RNUR_MIN) >> 8` |

`sutr_sucsl_is_neography(cp)` is true exactly inside the range.
`sutr_sucsl_neography_set(cp, &set)` returns the set number, from 0 for
`0x160000–0x1600FF` to 255 for `0x16FF00–0x16FFFF`. The function is O(1) and
allocation-free. It returns `false` for a codepoint outside the range and for a
NULL output pointer.

A sign in the RNUR has no standard value, no standard wedge count and no
standard period. The engine therefore returns a **synthesized record**:

```c
entry.codepoint          = cp;
entry.primary_value_hash = 0u;
entry.sign_flags         = SUCSL_FLAG_UNCLASSIFIED | SUCSL_FLAG_EXTENSION;
entry.wedge_count        = 0u;
entry.historical_period  = SUCSL_PERIOD_UNKNOWN;
```

The `SUCSL_FLAG_EXTENSION` bit is the marker that distinguishes a custom sign
from an uncurated Bridge sign. A consumer that wants to reject custom signs can
test it; a consumer that wants to accept them can treat the record as a stable
identity and read the value from its own overlay.

The extension model is deliberately **table-shaped, not parser-shaped**. A
user-space toolchain that defines a custom sign list compiles its own
`sucsl_signs[]` with the same 12-byte record and the same hash discipline, and
resolves its signs before consulting the kernel table. The kernel table remains
the fallback. Because the layout is public and the index is keyed by codepoint,
an overlay can be a second static array with the same lookup procedure; no
kernel patch, no new system call and no codepoint assignment are required.

Constraints on an overlay:

1. Every custom codepoint MUST lie in the RNUR.
2. The overlay MUST NOT shadow a Bridge cuneiform codepoint.
3. The overlay MUST use the same FNV-1a functions if it wants its hashes to be
   comparable with the kernel table.
4. The overlay MUST preserve the 12-byte record layout if it is exposed across
   the module boundary.
5. A custom sign MUST NOT be emitted into text that claims to be standard
   SuperUnicode text; it is an application-private extension.

The RNUR is not a dumping ground for unencoded signs. It is a **versioned
extension namespace** with a fixed geometry, so two tools that agree on a set
number can exchange records without a negotiation protocol.

---

## 6. Degradation and Synthesized Records

SUCSL never returns a partially filled record on failure and never writes to an
output structure it does not own. The behavior is summarized below.

| Input | Example | `get_sign_entry` | `out_error` | Record |
|-------|---------|------------------|-------------|--------|
| Curated Cuneiform | `U+12120` | `true` | `SUCSL_OK` | Curated |
| Uncurated Cuneiform | `U+12001` | `true` | `SUCSL_OK` | Synthesized `UNCLASSIFIED` |
| Uncurated RNUR | `U+160000` | `true` | `SUCSL_OK` | Synthesized `UNCLASSIFIED + EXTENSION` |
| SCP | `U+110000` | `false` | `SUCSL_ERR_NOT_CUNEIFORM` | Untouched |
| Native | `U+120000` | `false` | `SUCSL_ERR_NOT_CUNEIFORM` | Untouched |
| Trap | `U+7FFFFFF0` | `false` | `SUCSL_ERR_ILLEGAL_ZONE` | Untouched |
| Sentinel | `U+7FFFFFFF` | `false` | `SUCSL_ERR_ILLEGAL_ZONE` | Untouched |
| ExtSUCS | `U+80000000` | `false` | `SUCSL_ERR_ILLEGAL_ZONE` | Untouched |
| NULL record | `NULL` | `false` | `SUCSL_ERR_NULL_ARGUMENT` | Not applicable |
| NULL error pointer | any | normal result | not written | Per input |

Three rules follow from the table:

1. **Uncurated is not unknown.** A codepoint in an encoded Cuneiform block has
   a stable abstract identity even if the curated table has not reached it. The
   synthesized record lets a consumer carry it through a pipeline, report it,
   and round-trip it without inventing a value. `is_sign()` remains false, so
   a consumer can distinguish "encoded but not curated" from "curated".
2. **A zone error is a type error.** The Trap, Sentinel and ExtSUCS zones are
   not text. Returning a synthesized record for them would make an illegal
   codepoint look like a rare sign. SUCSL rejects them with `ILLEGAL_ZONE`.
3. **A non-cuneiform character is not a sign.** ASCII, Latin, Arabic, emoji
   and the SCP are text space but carry no cuneiform sign. They are rejected
   with `NOT_CUNEIFORM`, which is a weaker failure than `ILLEGAL_ZONE` and
   lets a caller distinguish "wrong character" from "corrupt codepoint".

The extended entry point `sutr_sucsl_get_sign_entry_ex()` reports the reason.
The convenience wrapper `sutr_sucsl_get_sign_entry()` discards it. Both share
one implementation, so they cannot drift. If `out_error` is NULL the function
still works; if `out_entry` is NULL it returns false without dereferencing it.

A failed call leaves `*out_entry` exactly as the caller left it. This is
normative: a caller may initialize a record once and reuse it across a loop,
and a failure will not silently zero a field that the caller was about to
inspect. The tests assert the post-condition for the NULL, zone and
non-cuneiform cases.

---

## 7. Complexity, Memory and Conformance

### 7.1 Time and space

| Operation | Expected | Worst case | Allocation |
|-----------|----------|------------|------------|
| `sutr_sucsl_init_registry()` | O(n + 512), once | O(n + 512) | None |
| `sutr_sucsl_get_sign_entry()` | O(1) | O(512) probes | None |
| `sutr_sucsl_is_sign()` and predicates | O(1) | O(512) probes | None |
| `sutr_sucsl_sign_index()` | O(1) | O(512) probes | None |
| `sutr_sucsl_get_composition()` | O(log 5) | O(log 5) | None |
| `sutr_sucsl_find_sequence()` | O(log 8) | O(log 8) | None |
| `sutr_sucsl_sign_at()` | O(1) | O(1) | None |
| `sutr_sucsl_hash_value()` | O(len) | O(len) | None |
| `sutr_sucsl_zone_of()` | O(1) | O(1) | None |

`n` is `SUCSL_SIGN_COUNT`, at most `SUCSL_SIGN_CAPACITY` (160). The probe bound
is the index size (512), not `n`, so the worst case is a constant. The expected
probe count at the current load factor (86/512 = 0.17) is close to one.

The static footprint is:

| Object | Size | Notes |
|--------|------|-------|
| `sucsl_signs[]` | 86 × 12 = 1,032 B | Curated records |
| `sucsl_compounds[]` | 5 × 24 = 120 B | Atomic compounds |
| `sucsl_sequences[]` | 8 × 24 = 192 B | Sequence descriptors |
| `sucsl_index[]` | 512 × 4 = 2,048 B | `{ uint16_t; uint8_t; }` with padding |
| Strings and helpers | < 1 KiB | Taxonomy names, version |
| **Total** | **≈ 3.4 KiB** | Compile-time constant |

The index slot is 4 bytes because the compiler pads the two-byte `entry` field
and the one-byte `used` field to the alignment of `uint16_t`. The header does
not expose the slot type, so the padding is an implementation detail; the
compile-time assert `SUCSL_SIGN_INDEX_SIZE > SUCSL_SIGN_CAPACITY` is what
guarantees that a free slot exists.

The module has **no heap allocation, no `malloc`, no `free`, no `realloc`, no
variable-length array, no recursion and no libc call**. It includes only
`<stdint.h>`, `<stdbool.h>` and `<stddef.h>`. It compiles under
`-ffreestanding -nostdlib` and under `-std=c99 -Wall -Wextra -Wpedantic`
without a diagnostic.

### 7.2 Normative requirements

A conforming implementation of SUCSL:

- **R1.** MUST represent a codepoint as a 31-bit unsigned value and MUST reject
  the Sentinel and the ExtSUCS range with `SUCSL_ERR_ILLEGAL_ZONE`.
- **R2.** MUST expose the 12-byte `sucsl_sign_entry_t` with the field offsets of
  §4.2, and MUST fail the build if the layout changes.
- **R3.** MUST assign the four sketch flags the values `1, 2, 4, 8` and MUST
  keep bits 8–15 reserved except for `SUCSL_FLAG_EXTENSION` on RNUR records.
- **R4.** MUST compute `primary_value_hash` with FNV-1a 32-bit over the ASCII
  skeleton of §2.1.1.
- **R5.** MUST resolve a curated sign in expected O(1) time and MUST NOT scan
  the table linearly on the lookup path.
- **R6.** MUST NOT allocate on the heap, use a variable-length array, or call a
  libc function on any code path.
- **R7.** MUST synthesize a record for an uncurated Cuneiform or RNUR codepoint
  and MUST mark it `UNCLASSIFIED` (and `EXTENSION` for RNUR).
- **R8.** MUST reject SCP, Native and non-cuneiform Bridge values with
  `NOT_CUNEIFORM`, and Trap/Sentinel/ExtSUCS with `ILLEGAL_ZONE`.
- **R9.** MUST retain deprecated signs and mark them `DEPRECATED`; MUST NOT
  emit them in new text.
- **R10.** MUST keep `sucsl_signs[]` strictly ascending in `codepoint` and the
  compound and sequence tables strictly ascending in their keys.
- **R11.** MUST make `sutr_sucsl_init_registry()` idempotent and MUST NOT
  require a second call after the first query.
- **R12.** MUST leave the output record untouched on every failure path.

A conforming **producer** additionally MUST NOT create an atomic codepoint for
a sequence, MUST NOT decompose an atomic sign into a sequence, and MUST NOT use
a deprecated sign in new text. A conforming **consumer** MUST accept a
synthesized record, MUST preserve an unknown codepoint across a round trip, and
MUST NOT infer a value from a hash collision.

### 7.3 Conformance tiers

SUCSL publishes the claim `sucsl` at **SUCM tier L0** (see SUTR-10). L0 covers
the deterministic, stateless subset: the record layout, the flags, the hash,
the zone rules, the synthesized records, the deprecated records and the O(1)
lookup. The higher tiers add corpus-scale behavior that is out of scope for a
static table, such as a full OSL import, a font-driven glyph selection and a
multi-table overlay. The claim is recorded in `sutr_sucm` with version `1.0.0`
and the clause set `SUCM_CLAUSES_L0`.

The claim is a statement about **this engine**, not about the completeness of
the curated data. A release that adds a sign changes the data version in SUCD,
not the claim. A release that changes the record layout, the flag values, the
hash or the degradation rules requires a new SUCSL minor or major version and a
new claim.

---

## 8. Reference Implementation and Verification

The reference implementation lives in `src/sutr/sutr_sucsl.c`, the public
surface in `include/sutr/sutr_sucsl.h`, and the unit tests in
`tests/test_sutr_sucsl.c`. It is linked into `sutr_static` and is exercised by
the `test_sutr_sucsl` CTest target, the unified smoke test and the SSRE
registry test.

### 8.1 API summary

| Function | Purpose |
|----------|---------|
| `sutr_sucsl_version_string()` | `"1.0.0"` |
| `sutr_sucsl_init_registry()` | Build the index once; idempotent |
| `sutr_sucsl_get_sign_entry(cp, &e)` | Resolve a sign; convenience wrapper |
| `sutr_sucsl_get_sign_entry_ex(cp, &e, &err)` | Resolve with a status code |
| `sutr_sucsl_is_cuneiform(cp)` | In one of the three Cuneiform blocks |
| `sutr_sucsl_is_sign(cp)` | Has a curated record |
| `sutr_sucsl_is_logogram(cp)` | `LOGOGRAM` flag |
| `sutr_sucsl_is_syllabic(cp)` | `SYLLABIC` flag |
| `sutr_sucsl_is_numerical(cp)` | `NUMERICAL` flag |
| `sutr_sucsl_is_composite(cp)` | `COMPOSITE` flag |
| `sutr_sucsl_is_deprecated(cp)` | `DEPRECATED` flag |
| `sutr_sucsl_is_variant(cp)` | `VARIANT` flag |
| `sutr_sucsl_category_of(flags)` | Project a flag word to one category |
| `sutr_sucsl_sign_category(cp)` | Category of a resolved sign |
| `sutr_sucsl_category_name(cat)` | `"Numerical"`, … |
| `sutr_sucsl_hash_value(skeleton)` | FNV-1a over a value skeleton |
| `sutr_sucsl_primary_value_hash(cp)` | Stored hash, or 0 |
| `sutr_sucsl_wedge_count(cp)` | Stored wedge count, or 0 |
| `sutr_sucsl_historical_period(cp)` | Stored period tag, or `UNKNOWN` |
| `sutr_sucsl_period_name(period)` | `"Ur III"`, … |
| `sutr_sucsl_get_composition(sign, &c)` | Atomic compound lookup |
| `sutr_sucsl_find_sequence(hash, &c)` | Sequence lookup by name hash |
| `sutr_sucsl_compound_count()` | Number of atomic compounds |
| `sutr_sucsl_compound_at(i, &c)` | Enumerate atomic compounds |
| `sutr_sucsl_sequence_count()` | Number of sequences |
| `sutr_sucsl_sequence_at(i, &c)` | Enumerate sequences |
| `sutr_sucsl_sign_index(cp)` | 1-based hierarchy index, or 0 |
| `sutr_sucsl_sign_count()` | Number of curated records |
| `sutr_sucsl_sign_at(i, &e)` | Enumerate curated records |
| `sutr_sucsl_zone_of(cp)` | Zone taxonomy |
| `sutr_sucsl_is_text_space(cp)` | Bridge, SCP, RNUR or Native |
| `sutr_sucsl_is_native(cp)` | Native plane object |
| `sutr_sucsl_is_neography(cp)` | In the RNUR |
| `sutr_sucsl_neography_set(cp, &set)` | RNUR set number, or false |
| `sutr_sucsl_hash_codepoint(cp)` | FNV-1a over the four key bytes |
| `sutr_sucsl_zone_name(zone)` | `"Bridge"`, … |
| `sutr_sucsl_status_code_name(code)` | `"SUCSL_OK"`, … |

Every pointer output is optional only where the documentation says so. A NULL
output pointer is a `false` return with `SUCSL_ERR_NULL_ARGUMENT` for the entry
point, and a `false` return for the table accessors; it is never a crash.

### 8.2 Curated registry snapshot

| Quantity | Value |
|----------|-------|
| Curated signs | 86 |
| Logogram flag set | 49 |
| Syllabic flag set | 50 |
| Numerical flag set | 31 |
| Composite flag set | 9 |
| Deprecated flag set | 3 |
| Variant flag set | 10 |
| Atomic compounds | 5 |
| Sequences | 8 |
| First curated codepoint | `U+12000` (A) |
| Last curated codepoint | `U+12514` (LAK-648×SHESH+KI) |
| Index of `U+12120` (GUD×KUR) | 21 |

The flag counts overlap, so they do not sum to 86. A sign may be a logogram,
a syllabogram and a composite at once. The category projection assigns exactly
one of the five category values to each sign, and the sum of the five projected
counts is 86.

The canonical value hashes are:

| Skeleton | Hash | Identity |
|----------|------|----------|
| `am` | `0x4024F153` | GUD×KUR, `U+12120` |
| `mes` | `0xD4AD6BBC` | MES, `U+12229` |
| `mesh` | `0xA100BEBC` | MEŠ, `U+1238C` |
| `me-esh` | `0xC53E2B7A` | ME.EŠ₂ sequence |
| `gudxkur` | `0xCB74D2C5` | GUD×KUR name |
| `geme2` | `0x0D95F4AB` | SAL.KUR sequence |
| `diri` | `0x635E3147` | SI.A sequence |
| `idigna` | `0x0D9D7CCD` | GU₂.GAR₃ sequence |
| `dalla` | `0xB91EC5A5` | MAŠ.GU₂.GAR₃ sequence |
| `u5` | `0x0842AE3F` | ḪU.SI ligature |
| `ash-shur` | `0x158520B6` | AŠ.ŠUR ligature |
| `siki-lam` | `0xD7AE3C7E` | SIK₂.LAM sequence |
| `dubxesh2` | `0xAF07276A` | DUB×EŠ₂, `U+1207F` |
| `gumxshe` | `0x80FA16FE` | GUM×ŠE, `U+12124` |
| `dubxshe` | `0x36817CDA` | DUB×ŠE, `U+12376` |
| `bil2` | `0x714E5A9E` | NE ŠEŠIG, `U+1224B` |
| `she` | `0xE92DF9E1` | SHE, `U+122BA` |

`sutr_sucsl_hash_codepoint(0x12000)` is `0x0269E40C`; it is a key hash, not a
value hash, and it is never compared with the table above.

### 8.3 Verification gates

A release of SUCSL passes:

1. **Standalone compile.** `cc -std=c99 -Wall -Wextra -Wpedantic -Iinclude
   -c src/sutr/sutr_sucsl.c` with no diagnostic.
2. **Unit tests.** `test_sutr_sucsl` covers 13 groups: version and geometry,
   zones, RNUR sets, hashing, curated resolution, predicates, periods,
   composition, sequences, enumeration, idempotent initialization, taxonomy
   and SSRE registration.
3. **Registry self-consistency.** Every curated row is ascending, in a
   Cuneiform block, has a non-zero hash and wedge count, and round-trips
   through `get_sign_entry` and `sign_index`.
4. **Table self-consistency.** Every compound row is a curated composite with
   curated components; every sequence row has `sign == 0`, two or three
   curated components, and a non-zero name hash; both tables are ascending.
5. **Unified smoke test.** `unified/test_unified.c` includes the header and
   resolves AM, GUD×KUR, GEME₂, a numeral, a deprecated composite and an
   illegal zone alongside every other module.
6. **Registry tests.** `test_suas_ssre` sees 37 specifications, 16 mandatory,
   8 optional and 13 reference; `test_sutr_sucm` sees 31 engines and 31 claims
   and resolves the `sucsl` claim at L0.
7. **Documentation check.** `verify-docs` accepts the report's field block,
   headings and cross-references, and the website generator emits the
   SUTR-012 page from this file.

The hash table is also checked by an independent generator that recomputes
every FNV-1a value from the source skeletons and compares it with the compiled
table; a mismatch fails the build before the image is linked.

---

## Annex A — Constants

| Constant | Value |
|----------|-------|
| `SUCSL_SPEC_ID` | `"SUTR-012"` |
| `SUCSL_SPEC_TITLE` | `"SuperUnicode Cuneiform Sign Lists"` |
| `SUCSL_MODULE` | `"sutr_sucsl"` |
| `SUCSL_VERSION_MAJOR` | `1` |
| `SUCSL_VERSION_MINOR` | `0` |
| `SUCSL_VERSION_PATCH` | `0` |
| `SUCSL_SIGN_CAPACITY` | `160` |
| `SUCSL_SIGN_INDEX_SIZE` | `512` |
| `SUCSL_COMPOUND_CAPACITY` | `64` |
| `SUCSL_SEQUENCE_CAPACITY` | `64` |
| `SUCSL_FNV_OFFSET_BASIS` | `2166136261` |
| `SUCSL_FNV_PRIME` | `16777619` |
| `SUCSL_FLAG_LOGOGRAM` | `1` |
| `SUCSL_FLAG_SYLLABIC` | `2` |
| `SUCSL_FLAG_NUMERICAL` | `4` |
| `SUCSL_FLAG_COMPOSITE` | `8` |
| `SUCSL_FLAG_DEPRECATED` | `16` |
| `SUCSL_FLAG_SEQUENCE` | `32` |
| `SUCSL_FLAG_VARIANT` | `64` |
| `SUCSL_FLAG_UNCLASSIFIED` | `128` |
| `SUCSL_FLAG_EXTENSION` | `256` |
| `SUCSL_FLAG_ASSIGNED_MASK` | `0x00FF` |
| `SUCSL_CATEGORY_NUMERICAL` | `0` |
| `SUCSL_CATEGORY_SYLLABIC` | `1` |
| `SUCSL_CATEGORY_LOGOGRAPHIC` | `2` |
| `SUCSL_CATEGORY_COMPOSITE` | `3` |
| `SUCSL_CATEGORY_UNKNOWN` | `4` |
| `SUCSL_PERIOD_UNKNOWN` | `0` |
| `SUCSL_PERIOD_EARLY_DYNASTIC` | `1` |
| `SUCSL_PERIOD_UR_III` | `2` |
| `SUCSL_PERIOD_OLD_BABYLONIAN` | `3` |
| `SUCSL_PERIOD_OLD_ASSYRIAN` | `4` |
| `SUCSL_PERIOD_MIDDLE_ASSYRIAN` | `5` |
| `SUCSL_PERIOD_NEO_ASSYRIAN` | `6` |
| `SUCSL_PERIOD_HELLENISTIC` | `7` |
| `SUCSL_PERIOD_ALL` | `8` |
| `SUCSL_BLOCK_CUNEIFORM_MIN/MAX` | `0x12000` / `0x123FF` |
| `SUCSL_BLOCK_NUMBERS_MIN/MAX` | `0x12400` / `0x1247F` |
| `SUCSL_BLOCK_EARLY_MIN/MAX` | `0x12480` / `0x1254F` |
| `SUCSL_RNUR_MIN/MAX` | `0x160000` / `0x16FFFF` |
| `SUCSL_RNUR_SET_SHIFT` | `8` |
| `SUCSL_RNUR_SET_SIZE` | `256` |
| `SUCSL_LIMIT_UNICODE_MAX` | `0x10FFFF` |
| `SUCSL_LIMIT_SCP_MAX` | `0x11FFFF` |
| `SUCSL_LIMIT_NATIVE_MAX` | `0x7FFFFFEF` |
| `SUCSL_LIMIT_TRAP_MIN` | `0x7FFFFFF0` |
| `SUCSL_LIMIT_TRAP_MAX` | `0x7FFFFFFE` |
| `SUCSL_LIMIT_SENTINEL` | `0x7FFFFFFF` |
| `SUCSL_ZONE_BRIDGE` | `0` |
| `SUCSL_ZONE_SCP` | `1` |
| `SUCSL_ZONE_RNUR` | `2` |
| `SUCSL_ZONE_NATIVE` | `3` |
| `SUCSL_ZONE_TRAP` | `4` |
| `SUCSL_ZONE_SENTINEL` | `5` |
| `SUCSL_ZONE_EXTSUCS` | `6` |
| `SUCSL_OK` | `0` |
| `SUCSL_ERR_NULL_ARGUMENT` | `1` |
| `SUCSL_ERR_NOT_CUNEIFORM` | `2` |
| `SUCSL_ERR_ILLEGAL_ZONE` | `3` |
| `SUCSL_ERR_UNKNOWN` | `4` |

---

## Annex B — Worked Examples

### B.1 Resolve a composite sign

```c
#include "sutr/sutr_sucsl.h"

sucsl_sign_entry_t e;
sucsl_compound_t c;

sutr_sucsl_init_registry();
if (sutr_sucsl_get_sign_entry(0x12120u, &e)) {
    /* AM = GUD x KUR */
    /* e.sign_flags          == LOGOGRAM | SYLLABIC | COMPOSITE */
    /* e.primary_value_hash  == 0x4024F153 */
    /* e.wedge_count         == 7 */
    /* e.historical_period   == SUCSL_PERIOD_ALL */
}
if (sutr_sucsl_get_composition(0x12120u, &c)) {
    /* c.component_a == 0x1211E (GUD), c.component_b == 0x121B3 (KUR) */
    /* c.component_count == 2, c.name_hash == 0xCB74D2C5 */
}
```

### B.2 Distinguish four-diš from four-limmu

```c
sucsl_sign_entry_t plain, variant;

sutr_sucsl_get_sign_entry(0x12409u, &plain);   /* FOUR DISH    */
sutr_sucsl_get_sign_entry(0x1243Cu, &variant); /* FOUR LIMMU   */

/* plain.sign_flags   == SUCSL_FLAG_NUMERICAL */
/* variant.sign_flags == SUCSL_FLAG_NUMERICAL | SUCSL_FLAG_VARIANT */
/* plain.historical_period   == SUCSL_PERIOD_UR_III */
/* variant.historical_period == SUCSL_PERIOD_NEO_ASSYRIAN */
```

The two records are not normalizable into one another. A consumer that wants
the quantity reads `NUMERICAL`; a consumer that wants the period reads
`historical_period`.

### B.3 Distinguish MEŠ from me-eš

```c
sucsl_compound_t seq;

/* MEŠ is atomic. */
(void)sutr_sucsl_is_sign(0x1238Cu);           /* true  */

/* "me-eš" is a sequence. */
(void)sutr_sucsl_find_sequence(sutr_sucsl_hash_value("me-esh"), &seq);
/* seq.component_a == 0x12228 (ME), seq.component_b == 0x120A0 (EŠ₂) */

/* There is no sequence named "mesh"; the atomic sign owns that skeleton. */
(void)sutr_sucsl_find_sequence(sutr_sucsl_hash_value("mesh"), &seq); /* false */
```

### B.4 Detect a deprecated sign

```c
sucsl_sign_entry_t e;
sucsl_compound_t c;

sutr_sucsl_get_sign_entry(0x1207Fu, &e);       /* DUB x EŠ₂ */
/* e.sign_flags & SUCSL_FLAG_DEPRECATED != 0 */

sutr_sucsl_get_composition(0x1207Fu, &c);
/* c.component_a == 0x1207E (DUB), c.component_b == 0x120A0 (EŠ₂) */
/* c.compound_flags & SUCSL_COMPOUND_DEPRECATED != 0 */
```

A producer should not emit `U+1207F` in new text. A consumer should still
resolve it, report it, and offer the replacement from the manifest.

### B.5 Accept a custom RNUR sign

```c
sucsl_sign_entry_t e;
uint32_t set;

sutr_sucsl_get_sign_entry(0x160000u, &e);
/* e.sign_flags == SUCSL_FLAG_UNCLASSIFIED | SUCSL_FLAG_EXTENSION */
/* e.primary_value_hash == 0, e.historical_period == SUCSL_PERIOD_UNKNOWN */

sutr_sucsl_neography_set(0x160000u, &set);     /* true, set == 0   */
sutr_sucsl_neography_set(0x160100u, &set);     /* true, set == 1   */
sutr_sucsl_neography_set(0x12000u, &set);      /* false            */
```

The application overlay supplies the value, the wedge count and the period; the
engine supplies the stable identity and the set arithmetic.

### B.6 Reject a non-sign

```c
sucsl_sign_entry_t e;
sucsl_status_code_t err;

/* Ordinary character: text space, no sign. */
(void)sutr_sucsl_get_sign_entry_ex(0x41u, &e, &err);
/* false, err == SUCSL_ERR_NOT_CUNEIFORM */

/* SCP: text space, no sign. */
(void)sutr_sucsl_get_sign_entry_ex(0x110000u, &e, &err);
/* false, err == SUCSL_ERR_NOT_CUNEIFORM */

/* Sentinel: not text space. */
(void)sutr_sucsl_get_sign_entry_ex(0x7FFFFFFFu, &e, &err);
/* false, err == SUCSL_ERR_ILLEGAL_ZONE */
```

A failure leaves `e` untouched. The test suite initializes the record to a
sentinel value and asserts that the sentinel survives.

---

## Annex C — Change Log

| Version | Date | Change |
|---------|------|--------|
| 1.0.0 | 2026-10-22 | Initial publication. Publishes the 12-byte sign record, the four sketch flags and four derived flags, the FNV-1a value and key hashes, the compound and sequence descriptor, the O(1) open-addressing index, the zone and block taxonomy, the RNUR extension model, and the degradation rules. Ships 86 curated signs, 5 atomic compounds and 8 sequences. Registers `sucsl` in SUCM at tier L0 and adds SUTR-012 to SSRE. |

The identifier `SUTR-012` is the published module identifier and is
intentionally zero-padded; it is the twelfth reference report. The zero-padded
form is part of the ABI (`SUCSL_SPEC_ID`), so a consumer that compares the
literal string must use the padded form.

**END OF SUTR-012**



