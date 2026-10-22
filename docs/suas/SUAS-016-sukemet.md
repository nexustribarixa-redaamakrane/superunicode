# SUAS-016 - SuperUnicode Egyptian Hieroglyph Database (SUKEMET)

**Standard ID:** SUAS-016
**Status:** Draft - Ratified
**Scope:** Core Kernel Invariant
**Applies To:** SUCD, UAX #57 (Unicode Egyptian Hieroglyph Database analogue), SUAS-005 (SSP), SUAS-006 (STS), SUAS-010 (SSRE), SUAS-011 (SCMS), SUAS-012 (SUCD), SUAS-013 (SISR), SUAS-014 (SUVTL), SUAS-015 (SUAMR), SUTS-001 (SUCA), SUTS-004 (SIVD), SUTS-005 (SCSS), the SUF font engine, the OpenWindows shell, the System Control Plane
**Last Updated:** 2026-10-22

---

## 1. Overview

SuperUnicode Egyptian Hieroglyph Database (SUKEMET) is the core data subsystem
governing the **zero-allocation, O(1) classification of Egyptian hieroglyphs**
and of the format controls that compose them. It answers five questions about a
codepoint without parsing a text manifest:

1. Is it a Gardiner-classified sign, a format control, an enclosure, or a
   neography declaration?
2. Which Gardiner group does it belong to, and what is its index within that
   group?
3. Is it in the Core set, the Legacy set, or neither?
4. What function does Unikemet assign to it, and what is its catalogue index?
5. May it be mirrored or rotated, and what alternate sequence represents it?

UAX #57, "Unicode Egyptian Hieroglyph Database (Unikemet)", publishes the
corpus as `Unikemet.txt`: a three-field, tab-separated UTF-8 manifest whose
rows must be read, joined and interpreted at run time. SUKEMET compiles the
same information into an **immutable, byte-packed, static table**. An
epigraphic renderer, a terminal emulator, a microkernel text pipeline or a
font toolchain can classify a sign without a single heap allocation.

SUKEMET is the runtime counterpart of UAX #57 in the same **dual-layer**
relationship that SUAS-009 (SIP) has with UAX #38, SUAS-013 (SISR) has with
UAX #45, SUAS-014 (SUVTL) has with UAX #50 and SUAS-015 (SUAMR) has with
UAX #53:

- the **offline reference layer** is the SuperUnicode Character Database
  (SUCD, SUAS-012): the Unikemet catalogue, the Gardiner taxonomy, the Core
  set, the mirror/rotate properties, alternate sequences, and the
  human-readable descriptions for Egyptologists and font engineers;
- the **runtime kernel layer** is this specification: a byte-packed 12-byte
  sign record, a parallel metadata table, a fixed open-addressing index, and a
  bounded query surface.

### 1.1 Architectural shifts from UAX #57

| UAX #57 / text-manifest world | SUKEMET world |
|-------------------------------|---------------|
| `Unikemet.txt` parsed and joined per query | A 12-byte static record read from a fixed table |
| Gardiner group inferred from a catalogue string | A byte-indexed `SUKEMET_GARDINER_*` value |
| Core/function/source fields carried as strings | Packed flags, masks and hashes |
| Alternate sequences re-parsed as text | A flattened `uint32_t` store with offset/length |
| Format controls handled by ad-hoc renderer rules | Named flags plus a one-byte modifier role |
| Heap-allocated property maps | Zero allocation; the caller owns every record |
| Absent row undefined | A total, synthesized default over the repertoire |

### 1.2 Goals

1. **A single record.** A sign's identity is a fixed 12-byte descriptor, not a
   manifest row parsed on demand.
2. **A bounded lookup.** A query hashes the codepoint, probes a fixed index, and
   returns the record - O(1) expected, a small constant number of probes, no
   allocation, no libc.
3. **One taxonomy.** Every UAX #57 Gardiner group A-Z plus AA, the Core/Legacy
   classes, the function classes, the mirror/rotate properties, and the
   format-control roles have a named binary encoding.
4. **A total repertoire.** Every codepoint in the Egyptian Hieroglyphs,
   Egyptian Hieroglyph Format Controls and Egyptian Hieroglyphs Extended-A
   blocks, plus the Native & Neography Plane carve-out, resolves - curated or
   synthesized - by the default of section 6.
5. **A neography extension.** The same engine classifies custom signs,
   enclosures, shading modifiers and compounds in the RNUR carve-out
   (`0x160000-0x16FFFF`).

### 1.3 Non-negotiable invariants

- **SUKEMET-I1 - Zero allocation.** No entry point calls `malloc`, `free`, or
  any libc allocator. Every buffer is caller-owned or static.
- **SUKEMET-I2 - O(1) lookup.** `suas_sukemet_get_entry` and the predicates run
  in O(1) expected time, independent of the table size.
- **SUKEMET-I3 - Fixed record.** `sukemet_hieroglyph_entry_t` is exactly 12
  bytes, with no padding, on every supported ABI (section 3.1).
- **SUKEMET-I4 - Reserved bit is zero.** Bit 15 of `sukemet_flags` is zero
  (`SUKEMET_REGISTRY_BAD_FLAGS`).
- **SUKEMET-I5 - Gardiner agreement.** `SUKEMET_FLAG_GARDINER` is set if and
  only if `gardiner_category != SUKEMET_GARDINER_NONE`
  (`SUKEMET_REGISTRY_GARDINER_MISMATCH`).
- **SUKEMET-I6 - Ordered and unique.** Records are ascending by codepoint and
  unique by codepoint (`SUKEMET_REGISTRY_UNORDERED`,
  `SUKEMET_REGISTRY_DUPLICATE_CODEPOINT`).
- **SUKEMET-I7 - Format controls stay in their block.**
  `SUKEMET_FLAG_FORMAT` appears only inside `U+13430-U+1345F`, and always with
  `SUKEMET_FLAG_MODIFIER` (`SUKEMET_REGISTRY_CONTROL_MISMATCH`).
- **SUKEMET-I8 - Neography is an RNUR right.** `SUKEMET_FLAG_NEOGRAPHY` appears
  if and only if the codepoint is in `0x00160000-0x0016FFFF`
  (`SUKEMET_REGISTRY_NEOGRAPHY_MISMATCH`).
- **SUKEMET-I9 - Core and Legacy are exclusive.** Both
  `SUKEMET_FLAG_CORE` and `SUKEMET_FLAG_LEGACY` may not be set
  (`SUKEMET_REGISTRY_CORE_CONTRADICTION`).
- **SUKEMET-I10 - Totality.** `suas_sukemet_get_entry` returns `true` for every
  codepoint in a SUKEMET repertoire space, curated or not (section 6).

### 1.4 Deviations from the reference sketch

1. **Classification.** The sketch labelled the module **SUAS-016** and named it
   the SuperUnicode Egyptian Hieroglyph Database. It is registered as
   **SUAS-016**, immediately after SUAS-015 (SUAMR), keeping the SUAS series
   contiguous with no reserved gaps (SUAS-010 section 11.1). The registry
   *contents* are version data; the engine, record layout, taxonomy and
   verification rules are mandatory architecture.
2. **Macro prefix.** Spec-level constants carry both the sketch names
   (`SUKEMET_SPEC_ID`, `SUKEMET_SPEC_TITLE`, `SUKEMET_VERSION_MAJOR`,
   `SUKEMET_VERSION_MINOR`, `SUKEMET_FLAG_*`) and the house aliases
   (`SUAS_SUKEMET_*`). The sketch's record and entry points are preserved
   verbatim.
3. **A parallel metadata record.** The sketch's 12-byte record has no room for
   the catalogue index, description hash, UniK hash, source mask, Core class,
   function mask, modifier role, enclosure kind or alternate-sequence length.
   Rather than widen the record, SUKEMET keeps them in a **parallel table**
   aligned one-to-one with the main table (section 3.2).
4. **An added orthogonal flag set.** The sketch's four flags
   (`GARDINER`, `MODIFIER`, `ENCLOSURE`, `COMPOSITE`) are kept. SUKEMET adds
   CORE, LEGACY, NO_MIRROR, NO_ROTATE, FORMAT, STACKING, INSERTION, OVERLAY,
   SEGMENT, MIRROR and NEOGRAPHY (section 3.3). The modifier *role* (joiner,
   insertion, overlay, segment, enclosure, mirror, damage, shading) lives in a
   separate byte, not in the flag word.
5. **A total property.** UAX #57 leaves an absent row undefined. SUKEMET
   requires a total answer for the repertoire and supplies the algorithmic
   default of section 6. `suas_sukemet_is_registered` distinguishes a curated
   row from a synthesized default.
6. **Cartouche and serekh.** `BEGIN/END ENCLOSURE` are the rounded cartouche
   frame; `BEGIN/END WALLED ENCLOSURE` are the rectangular serekh-style frame.
   The mapping is exposed as `SUKEMET_ENCLOSURE_CARTOUCHE` and
   `SUKEMET_ENCLOSURE_SEREKH`; a neography may declare
   `SUKEMET_ENCLOSURE_CUSTOM`.
7. **Shading.** Unicode 18.0.0 encodes no standard Egyptian shading control.
   The modifier taxonomy nevertheless names `SUKEMET_MODIFIER_SHADING`; a
   shading modifier is therefore always an RNUR declaration, never a
   mislabelled standard codepoint.
8. **The `is_damage` signature.** The sketch's typo `codepoint_t` is corrected
   to `uint32_t`.

---

## 2. Definitions and Notation

### 2.1 Definitions

- **Unikemet.** The Unicode Egyptian Hieroglyph Database (UAX #57), published
  as `Unikemet-18.0.0.txt`.
- **Gardiner group.** The alphabetic classification A-Z plus AA. Gardiner has
  no group J; this specification preserves that gap. AA is the unclassified
  catch-all.
- **`kEH_Cat`.** The IFAO catalogue index of a sign, shaped
  `GROUP-SUBGROUP-ITEM` (for example `A-01-001`).
- **`kEH_UniK`.** The original catalogue identifier (for example `A001`). Its
  numeric part supplies the sketch's `sub_index` when it fits the byte.
- **`kEH_Core`.** `C` for a Core sign, `L` for a Legacy sign, and the default
  `N` otherwise.
- **`kEH_Func`.** The function classification: pictogram, logogram, phonogram,
  classifier, phono-repeater, radicogram or interpretant. A sign may carry
  several.
- **`kEH_FVal`.** The phonetic or semantic value. The first alternative before
  `|` is the primary reading hashed into `phonetic_hash`.
- **`kEH_AltSeq`.** An alternate sequence of codepoints, including format
  controls, that can represent a compound or mirrored sign.
- **`kEH_NoMirror` / `kEH_NoRotate`.** `Y` when the sign must not be mirrored
  or rotated.
- **Format control.** A member of `U+13430-U+1345F` that joins, inserts,
  overlays, segments or encloses signs. `U+13440` is the mirror control;
  `U+13441-U+13446` are blank/lost signs; `U+13447-U+13455` are damage
  modifiers.
- **Cartouche.** The rounded royal-name enclosure built by
  `U+1343C/U+1343D`.
- **Serekh.** The rectangular, palace-facade enclosure built by
  `U+1343E/U+1343F`.
- **Native & Neography Plane carve-out (RNUR).** `0x00160000-0x0016FFFF`; a
  declared neography extension space (SUAS-005 section 5.4).
- **RNUR set number.** For a neography coordinate,
  `set = (cp - 0x00160000) >> 8` - an O(1) decode.
- **Algorithmic default.** The record synthesized for an uncurated codepoint in
  a SUKEMET repertoire space (section 6).

### 2.2 Notational conventions

`U+` names a Bridge-plane Unicode codepoint; a bare hexadecimal value names a
Base SUCS codepoint. `cp` is a 31-bit Base SUCS value
(`0x00000000-0x7FFFFFFF`). A value above `SUAS_CODEPOINT_MAX` is ExtSUCS and is
outside the registry. Catalogue indexes are packed as
`(category << 24) | (subgroup << 16) | item`.

### 2.3 The address-space zones

| Zone | Range | SUKEMET repertoire? |
|------|-------|---------------------|
| `SUAS_SUKEMET_ZONE_BRIDGE` | `0x00000000-0x0010FFFF` | yes - the Egyptian blocks |
| `SUAS_SUKEMET_ZONE_SCP` | `0x00110000-0x0011FFFF` | no - Control Plane |
| `SUAS_SUKEMET_ZONE_RNUR` | `0x00160000-0x0016FFFF` | yes - neography |
| `SUAS_SUKEMET_ZONE_NATIVE` | rest of `0x00120000-0x7FFFFFEF` | no curated rows |
| `SUAS_SUKEMET_ZONE_TRAP` | `0x7FFFFFF0-0x7FFFFFFE` | no |
| `SUAS_SUKEMET_ZONE_SENTINEL` | `0x7FFFFFFF` | no |
| `SUAS_SUKEMET_ZONE_EXTSUCS` | `> 0x7FFFFFFF` | no - plugin space |

`suas_sukemet_is_text_space` is true for Bridge, RNUR and Native only.
`suas_sukemet_is_hieroglyph` is narrower: it is true only for the three
Egyptian blocks and the RNUR carve-out.

### 2.4 The Egyptian repertoire blocks

| Block | Range | Contents |
|-------|-------|----------|
| Egyptian Hieroglyphs | `U+13000-U+1342F` | 1,071 assigned Gardiner signs |
| Egyptian Hieroglyph Format Controls | `U+13430-U+1345F` | joiners, insertions, overlays, segments, enclosures, mirror, blanks, damage marks |
| Egyptian Hieroglyphs Extended-A | `U+13460-U+143FF` | the extended Gardiner repertoire through AA |
| RNUR carve-out | `0x00160000-0x0016FFFF` | neography declarations |

---

## 3. The Record, Metadata and Index

### 3.1 The hieroglyph record

```c
typedef struct {
    uint32_t codepoint;          /* Raw 31-bit SUCS codepoint               */
    uint32_t phonetic_hash;      /* FNV-1a of the primary kEH_FVal reading  */
    uint16_t sukemet_flags;      /* Packed classification/control flags     */
    uint8_t  gardiner_category;  /* A-Z, AA, or SUKEMET_GARDINER_NONE       */
    uint8_t  sub_index;          /* Sequential index within the group       */
} sukemet_hieroglyph_entry_t;
```

The record is exactly **12 bytes** on every supported ABI, with no padding. Its
field offsets are `0, 4, 8, 10, 11`, proven by compile-time assertion.

`phonetic_hash == 0` means the sign has no `kEH_FVal`. Otherwise it is the
FNV-1a 32-bit hash of the UTF-8 bytes of the first alternative before `|`.
Hashing preserves a stable, bounded identity without storing a variable-length
transliteration in the kernel.

### 3.2 The parallel metadata record

```c
typedef struct {
    uint32_t catalog_index;     /* (cat << 24) | (subgroup << 16) | item   */
    uint32_t description_hash;  /* FNV-1a of kEH_Desc, 0 if absent         */
    uint32_t unik_hash;         /* FNV-1a of kEH_UniK, 0 if absent         */
    uint16_t source_mask;       /* SUKEMET_SOURCE_*                        */
    uint8_t  core_class;        /* N, C, or L                              */
    uint8_t  function_types;    /* SUKEMET_FUNC_* mask                     */
    uint8_t  modifier_class;    /* SUKEMET_MODIFIER_*                      */
    uint8_t  enclosure_kind;    /* SUKEMET_ENCLOSURE_*                     */
    uint8_t  alternate_length;  /* count of alternate codepoints           */
    uint8_t  reserved[3];       /* must be zero                            */
} sukemet_metadata_t;
```

The metadata table is aligned one-to-one with the main table. For a synthesized
record every field is zero. The alternate-sequence *offset* is an internal
parallel array; callers see only the length and the indexed query.

### 3.3 The classification flags

| Flag | Bit | Meaning |
|------|-----|---------|
| `SUKEMET_FLAG_GARDINER` | `1<<0` | Standard Gardiner classification |
| `SUKEMET_FLAG_MODIFIER` | `1<<1` | Layout control or stacking modifier |
| `SUKEMET_FLAG_ENCLOSURE` | `1<<2` | Cartouche, serekh or walled frame |
| `SUKEMET_FLAG_COMPOSITE` | `1<<3` | Multi-sign ligature cluster |
| `SUKEMET_FLAG_CORE` | `1<<4` | `kEH_Core = C` |
| `SUKEMET_FLAG_LEGACY` | `1<<5` | `kEH_Core = L` |
| `SUKEMET_FLAG_NO_MIRROR` | `1<<6` | `kEH_NoMirror = Y` |
| `SUKEMET_FLAG_NO_ROTATE` | `1<<7` | `kEH_NoRotate = Y` |
| `SUKEMET_FLAG_FORMAT` | `1<<8` | Egyptian format control |
| `SUKEMET_FLAG_STACKING` | `1<<9` | Vertical/horizontal joiner |
| `SUKEMET_FLAG_INSERTION` | `1<<10` | Insert-at-* control |
| `SUKEMET_FLAG_OVERLAY` | `1<<11` | Overlay-middle control |
| `SUKEMET_FLAG_SEGMENT` | `1<<12` | Begin/end segment control |
| `SUKEMET_FLAG_MIRROR` | `1<<13` | Mirror-horizontally control |
| `SUKEMET_FLAG_NEOGRAPHY` | `1<<14` | Native & Neography Plane |

`SUKEMET_FLAG_ASSIGNED_MASK == 0x7FFF`. Bit 15 is reserved and must be zero.
`COMPOSITE` is set for a multi-codepoint alternate sequence and for every
compositional format control, but not for a single-sign alternate mapping.

### 3.4 The Gardiner taxonomy

| Value | Letter | Group name |
|-------|--------|------------|
| 0 | A | Man and his occupations |
| 1 | B | Woman and her occupations |
| 2 | C | Anthropomorphic deities |
| 3 | D | Parts of the human body |
| 4 | E | Mammals |
| 5 | F | Parts of mammals |
| 6 | G | Birds |
| 7 | H | Parts of birds |
| 8 | I | Amphibious animals, reptiles, etc. |
| 9 | K | Fishes and parts of fishes |
| 10 | L | Invertebrates and lesser animals |
| 11 | M | Trees and plants |
| 12 | N | Sky, earth, water |
| 13 | O | Buildings, parts of buildings, etc. |
| 14 | P | Ships and parts of ships |
| 15 | Q | Domestic and funerary furniture |
| 16 | R | Temple furniture and sacred emblems |
| 17 | S | Crowns, dress, staves, etc. |
| 18 | T | Warfare, hunting, butchery |
| 19 | U | Agriculture, crafts and professions |
| 20 | V | Rope, fibre, baskets, bags, etc. |
| 21 | W | Vessels of stone and earthenware |
| 22 | X | Loaves and cakes |
| 23 | Y | Writings, games, music |
| 24 | Z | Strokes, signs derived from hieratic, geometrical figures |
| 25 | AA | Unclassified |
| 255 | - | `SUKEMET_GARDINER_NONE` |

### 3.5 The function and modifier taxonomy

`function_types` is a bitmask, because Unikemet describes a sign with one or
more functions:

| Bit | Name |
|-----|------|
| `1<<0` | Pictogram |
| `1<<1` | Logogram |
| `1<<2` | Phonogram / phonemogram |
| `1<<3` | Classifier / determinative |
| `1<<4` | Phono-repeater |
| `1<<5` | Radicogram |
| `1<<6` | Interpretant |
| `1<<7` | Control |

`modifier_class` is one byte: none, joiner, insertion, overlay, segment,
enclosure, mirror, damage, or shading. The enum keeps the layout role separate
from the flag word, so a control can be a joiner and a modifier without
inventing extra bits.

### 3.6 The hash

`suas_sukemet_hash` is FNV-1a over the four little-endian bytes of the
codepoint. `suas_sukemet_hash_bytes` is FNV-1a over a caller-supplied byte
string and is the function used to precompute the metadata hashes. FNV-1a is
used as an index key, not as an identity.

### 3.7 The index

The table is compiled in ascending codepoint order. On first query,
`suas_sukemet_init_registry` (or the lazy build inside
`suas_sukemet_table_index`) inserts every record into a fixed
`SUAS_SUKEMET_INDEX_SIZE` open-addressing array. A slot stores
`table_index + 1`; zero means empty. The index is never resized, never
allocated, and never mutated after the build. Initialization is idempotent and
must be serialized by the boot path if two threads may query before the first
call completes.

---

## 4. UAX #57 / SUCD Field Mapping

| Unikemet field | SUKEMET representation |
|----------------|------------------------|
| `kEH_Cat` | `gardiner_category`, `sub_index`, packed `catalog_index` |
| `kEH_Core` | CORE/LEGACY flags, `core_class` |
| `kEH_Desc` | `description_hash` |
| `kEH_Func` | `function_types` bitmask |
| `kEH_FVal` | `phonetic_hash` (primary alternative) |
| `kEH_UniK` | `unik_hash`, `sub_index` when the numeric part fits |
| `kEH_JSesh` | `SUKEMET_SOURCE_JSESH` |
| `kEH_HG` | `SUKEMET_SOURCE_HG` |
| `kEH_IFAO` | `SUKEMET_SOURCE_IFAO` |
| `kEH_AltSeq` | `alternate_length`, `suas_sukemet_alt_seq_at` |
| `kEH_NoMirror` | NO_MIRROR flag, `SUKEMET_SOURCE_MIRROR` |
| `kEH_NoRotate` | NO_ROTATE flag, `SUKEMET_SOURCE_ROTATE` |

The source mask records *which* fields were present, so a caller can tell an
absent `kEH_FVal` from a value whose hash happens to be zero.

---

## 5. Format Controls and Enclosures

The Egyptian Hieroglyph Format Controls block is curated from Unicode 18.0.0
and the format-control roles of the core specification, not from
`Unikemet.txt` (which does not carry those codepoints).

| Codepoints | Role | Flags / modifier class |
|------------|------|------------------------|
| `U+13430`, `U+13431` | Vertical / horizontal joiner | FORMAT, STACKING, COMPOSITE / JOINER |
| `U+13432-U+13435`, `U+13439-U+1343B` | Insert-at-* positions | FORMAT, INSERTION, COMPOSITE / INSERTION |
| `U+13436` | Overlay middle | FORMAT, OVERLAY, COMPOSITE / OVERLAY |
| `U+13437`, `U+13438` | Begin / end segment | FORMAT, SEGMENT, COMPOSITE / SEGMENT |
| `U+1343C`, `U+1343D` | Begin / end enclosure | FORMAT, ENCLOSURE, COMPOSITE / ENCLOSURE, CARTOUCHE |
| `U+1343E`, `U+1343F` | Begin / end walled enclosure | FORMAT, ENCLOSURE, COMPOSITE / ENCLOSURE, SEREKH |
| `U+13440` | Mirror horizontally | FORMAT, MIRROR / MIRROR |
| `U+13441-U+13446` | Full blank and lost signs | no control flags |
| `U+13447-U+13455` | Damage modifiers | MODIFIER / DAMAGE (not Cf) |

Every format control also carries `SUKEMET_FLAG_MODIFIER`; the validator
enforces that pairing (SUKEMET-I7). The mirror control is a combining mark, not
a `Cf` character, but it is still a format-control role here, so it carries
`SUKEMET_FLAG_FORMAT`.

A cartouche is the rounded royal enclosure; a serekh is the rectangular
palace-facade enclosure. UAX #57 leaves the glyph choice to fonts, so SUKEMET
names the *frame kind* and leaves the actual outline to the renderer. The RNUR
declaration `0x160302` exercises the custom enclosure path.

Shading has no standard Unicode 18.0.0 codepoint. SUKEMET names the role
`SUKEMET_MODIFIER_SHADING` and demonstrates it with the RNUR declaration
`0x160301`; it never tags a standard damage modifier as shading.

---

## 6. Zone Dispatch and the Algorithmic Default

### 6.1 Zone dispatch

`suas_sukemet_zone_of` classifies a 31-bit value into Bridge, SCP, RNUR, Native,
Trap, Sentinel or ExtSUCS. `suas_sukemet_block_of` then narrows a Bridge or
RNUR value to one of the three Egyptian blocks or the neography block.

`suas_sukemet_get_entry` returns `false` for a NULL output, an inadmissible
zone, or a codepoint outside the Egyptian repertoire. It leaves the output
untouched on failure.

### 6.2 The algorithmic default

For a codepoint inside an Egyptian block or the RNUR carve-out with no curated
row, `suas_sukemet_get_entry` returns `true` and synthesizes:

```c
codepoint        = cp;
phonetic_hash    = 0;
sukemet_flags    = (is RNUR) ? SUKEMET_FLAG_NEOGRAPHY : 0;
gardiner_category = SUKEMET_GARDINER_NONE;
sub_index        = 0;
```

`suas_sukemet_get_metadata` returns an all-zero metadata record for the same
codepoint. `suas_sukemet_is_registered` remains `false`: the caller can always
distinguish a curated row from a synthesized default. This is why a renderer
can answer for an unassigned Extended-A slot or a newly declared RNUR
coordinate without special-casing a missing lookup.

---

## 7. Data Files and the Curated Table

The shipped table is generated from:

- `Unikemet-18.0.0.txt` (2026-05-19): 5,067 catalogue signs;
- `UnicodeData-18.0.0.txt`: the 38 format-control, blank/lost and damage
  codepoints of `U+13430-U+13455`;
- SUAS-005 section 5.4 RNUR declarations: four neography records
  (`0x160300-0x160303`).

The result is **5,109 curated records**, 98 alternate sequences containing 265
codepoints, and a metadata row for every record. The capacity
`SUAS_SUKEMET_REGISTRY_CAPACITY` is 8,192 and the index is 16,384 slots, leaving
room for a later Unikemet revision without changing the index geometry.

The data is a generated `.inc` file included by `suas_sukemet.c`. The arrays
`sukemet_table[]`, `sukemet_meta_table[]`, `sukemet_alt_offset[]` and
`sukemet_alt_data[]` are aligned and their lengths are checked by compile-time
assertion. Regenerate through the SUCD build-time extractor when the Unicode
data version changes; do not edit the catalogue by hand.

---

## 8. Verification

`suas_sukemet_validate_table` checks a compiled table in one left-to-right
pass. It accepts an empty table (`entries == NULL, count == 0`) as conformant;
an implementation with an empty table still answers every repertoire codepoint
by the default of section 6. The outcomes are:

| Error | Condition |
|-------|-----------|
| `SUKEMET_REGISTRY_NULL_TABLE` | `count > 0` and `entries == NULL` |
| `SUKEMET_REGISTRY_COUNT_OVER` | `count > SUAS_SUKEMET_REGISTRY_CAPACITY` |
| `SUKEMET_REGISTRY_OUT_OF_SPACE` | `codepoint > 0x7FFFFFFF` |
| `SUKEMET_REGISTRY_INADMISSIBLE_CODEPOINT` | outside the repertoire |
| `SUKEMET_REGISTRY_BAD_FLAGS` | reserved bit set, or FORMAT without MODIFIER |
| `SUKEMET_REGISTRY_CORE_CONTRADICTION` | CORE and LEGACY both set |
| `SUKEMET_REGISTRY_BAD_CATEGORY` | category > AA and not NONE |
| `SUKEMET_REGISTRY_GARDINER_MISMATCH` | flag and category disagree |
| `SUKEMET_REGISTRY_CONTROL_MISMATCH` | FORMAT outside `U+13430-U+1345F` |
| `SUKEMET_REGISTRY_NEOGRAPHY_MISMATCH` | NEOGRAPHY outside RNUR |
| `SUKEMET_REGISTRY_UNORDERED` | codepoints not ascending |
| `SUKEMET_REGISTRY_DUPLICATE_CODEPOINT` | same codepoint twice |

The shipped table is validated in the unit suite; the full 5,109-row roster
passes every check.

---

## 9. The SUAS-010 (SSRE) Linkage Gate

SUAS-016 is registered in SSRE as a **mandatory** architecture specification
with module `suas_sukemet`, version `1.0.0` and tier `L0`. The registration
changes no existing dependency: every descriptor that verified before still
verifies. The SUCM claim `sukemet` and engine `suas_sukemet` mirror the SSRE
row, so a conformance image can name the engine and the owning specification
in one lookup.

---

## 10. Stability and Versioning

The record layout, flag meanings, Gardiner enum values, taxonomy names and
verification outcomes are architectural. The **contents** of the catalogue are
version data and may change with a Unicode release. A table update that only
adds, removes or corrects rows is a patch-level data change; a change to the
12-byte layout, a flag meaning, or a verification outcome is a major change.

The engine is freestanding C99. No entry point reaches for libc, and the static
tables are immutable after the lazy index build.

---

## 11. Conformance Requirements

An implementation conforms to SUAS-016 when it:

1. compiles the 12-byte record with the exact field order and size;
2. answers `suas_sukemet_get_entry` in O(1) expected time for every codepoint
   in the Egyptian blocks and RNUR carve-out, curated or synthesized;
3. returns `false`, without touching the output, for an inadmissible zone or a
   NULL output;
4. distinguishes a curated record from a synthesized default through
   `suas_sukemet_is_registered`;
5. preserves the Gardiner group numbering, including the missing J and the AA
   catch-all;
6. reports Core and Legacy exclusively, and never sets the reserved flag bit;
7. carries the format-control, enclosure, mirror, damage and RNUR roles through
   the taxonomy of sections 3 and 5;
8. passes `suas_sukemet_validate_table` on its compiled table; and
9. registers itself in SSRE as mandatory and publishes the matching SUCM claim.

### 11.1 Compliance matrix

| Requirement | Reference implementation | Test surface |
|-------------|--------------------------|--------------|
| 12-byte record | `sukemet_hieroglyph_entry_t` | `sizeof`, `offsetof` |
| O(1) lookup | open-addressing index | 5,109-row enumeration and query identity |
| Totality | section 6 default | unassigned Extended-A, RNUR, SCP/trap misses |
| Gardiner taxonomy | `sukemet_gardiner_t` | A, K, AA, invalid |
| Core/Legacy | flags plus `core_class` | U+13000 Core, U+1305D Legacy |
| Catalogue | packed `catalog_index` | A-01-001 and A-01-027 |
| Function mask | `function_types` | classifier, phonogram, multi-class |
| Mirror/rotate | NO_MIRROR / NO_ROTATE | U+13081, U+1305D |
| Alternate sequences | flattened store | U+1305D, U+13096, U+130B9 |
| Controls | format flags and modifier roles | joiners, insertions, overlays, segments |
| Enclosures | CARTOUCHE / SEREKH / CUSTOM | U+1343C, U+1343E, U+160302 |
| Damage/shading | DAMAGE / SHADING | U+13447, U+160301 |
| Verification | all twelve outcomes | `test_validate_table` |
| SSRE/SUCM linkage | registered rows | `SUAS-016` lookup |

---

## 12. Terms and Conventions

- **SUCD** is the SuperUnicode Character Database (SUAS-012).
- **SSRE** is the System Specification Registry and Cross-Reference Engine
  (SUAS-010).
- **SUCM** is the SuperUnicode Conformance Model (SUTR-10).
- **RNUR** is the Native & Neography Plane carve-out of SUAS-005.
- **Unikemet** is UAX #57's Egyptian Hieroglyph Database.
- **Cf** and **Mn** are the Unicode general categories for format controls and
  non-spacing marks; the damage modifiers are `Mn`, not `Cf`.
- A **curated** record is one present in the compiled table; a **synthesized**
  record is the default of section 6.

---

## 13. Testing

`tests/test_suas_sukemet.c` exercises the version and record geometry, zone
dispatch, the RNUR set decode, FNV-1a, the Gardiner and Core taxonomy, the
catalogue and metadata queries, alternate sequences, every format-control and
enclosure role, the neography declarations, the totality domain, all 5,109
curated rows, every verification outcome, the taxonomy names and the SSRE
registration. `unified/test_unified.c` includes the SUKEMET header alongside
every other SUAS/SUTS/SUTR header and proves the interfaces coexist in one
translation unit.
