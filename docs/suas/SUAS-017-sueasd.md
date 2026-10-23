# SUAS-017 - SuperUnicode East Asian Script Data (SUEASD)

**Standard ID:** SUAS-017
**Status:** Draft - Ratified
**Scope:** Core Kernel Invariant
**Applies To:** SUCD, UAX #60 (Unicode East Asian Script Data analogue), SUAS-005 (SSP), SUAS-006 (STS), SUAS-009 (SIP), SUAS-010 (SSRE), SUAS-011 (SCMS), SUAS-012 (SUCD), SUAS-013 (SISR), SUAS-016 (SUKEMET), SUTS-001 (SUCA), SUTS-004 (SIVD), SUTS-005 (SCSS), the SUF font engine, the OpenWindows shell, the System Control Plane
**Last Updated:** 2026-10-23

---

## 1. Overview

SuperUnicode East Asian Script Data (SUEASD) is the core data subsystem
governing the **zero-allocation, O(1) classification of East Asian scripts**
and of the ancillary source data that UAX #60 publishes for them. It answers
four questions about a codepoint without parsing a text manifest:

1. Which East Asian script does it belong to - Han, Hangul, Hiragana, Katakana,
   Bopomofo, Jurchen, Nushu, Small Seal, Tangut or Khitan Small Script?
2. Is it ideographic, a radical, a compatibility ideograph, kana, jamo, a
   precomposed syllable, or a neography/conscript declaration?
3. What regional source class does its script belong to?
4. What ancillary UAX #60 data does it carry - a source identifier, a
   radical-stroke index, a reading, a numeric value, a modern-CJK equivalent -
   and in how many distinct source rows?

UAX #60, "Data for East Asian Scripts" (Rev 3, Unicode 18.0.0), publishes the
ancillary corpus as four three-field, tab-separated UTF-8 manifests -
`JurchenSources.txt`, `NushuSources.txt`, `SealSources.txt` and
`TangutSources.txt` - whose rows must be read, joined and interpreted at run
time, while `Scripts.txt` carries the primary script assignment. SUEASD
compiles the same information into an **immutable, byte-packed, static
table**. A font engine, an input method, a terminal emulator, a microkernel
text pipeline or a CJK justification pass can classify a codepoint without a
single heap allocation.

SUEASD is the runtime counterpart of UAX #60 in the same **dual-layer**
relationship that SUAS-009 (SIP) has with UAX #38, SUAS-013 (SISR) has with
UAX #45, SUAS-014 (SUVTL) has with UAX #50, SUAS-015 (SUAMR) has with UAX #53
and SUAS-016 (SUKEMET) has with UAX #57:

- the **offline reference layer** is the SuperUnicode Character Database
  (SUCD, SUAS-012): the `Scripts.txt` classification, the four UAX #60 source
  files, and the human-readable descriptions for font engineers;
- the **runtime kernel layer** is this specification: a byte-packed 12-byte
  script record, a compact classification range table, a parallel 24-byte
  metadata table, a fixed open-addressing index, and a bounded query surface.

### 1.1 Architectural shifts from UAX #60

| UAX #60 / text-manifest world | SUEASD world |
|-------------------------------|--------------|
| `Scripts.txt` scanned per query | A 12-byte static record read from a fixed range table |
| Four source files parsed and joined per query | One curated record plus a parallel metadata row |
| Radical-stroke carried as `"12.4"` text | A `uint16_t` radical and a `uint8_t` stroke count |
| Readings and source identifiers carried as strings | FNV-1a hashes of the primary fields |
| Numeric values parsed from text | A `uint16_t` integer value |
| Small Seal modern-CJK lookup by string join | A `uint32_t` codepoint equivalence |
| Heap-allocated property maps | Zero allocation; the caller owns every record |
| Absent row undefined | A total, synthesized default over the repertoire |

### 1.2 Goals

1. **A single record.** A codepoint's identity is a fixed 12-byte descriptor,
   not a manifest row parsed on demand.
2. **A bounded lookup.** A query hashes the codepoint, probes a fixed index,
   and returns the record - O(1) expected, a small constant number of probes,
   no allocation, no libc.
3. **One taxonomy.** Every East Asian script, regional source class, UAX #60
   source tag, and the derived role flags have a named binary encoding.
4. **A total repertoire.** Every codepoint in an East Asian `Scripts.txt`
   range, plus the Native & Neography Plane carve-out, resolves - curated or
   synthesized - by the default of section 6.
5. **A neography extension.** The same engine classifies conscript
   declarations in the RNUR carve-out (`0x160000-0x16FFFF`).

### 1.3 Non-negotiable invariants

- **SUEASD-I1 - Zero allocation.** No entry point calls `malloc`, `free`, or
  any libc allocator. Every buffer is caller-owned or static.
- **SUEASD-I2 - O(1) lookup.** `suas_sueasd_get_entry` and the predicates run
  in O(1) time, independent of the table size.
- **SUEASD-I3 - Fixed record.** `sueasd_script_entry_t` is exactly 12 bytes,
  with no padding, on every supported ABI (section 3.1).
- **SUEASD-I4 - Reserved byte is zero.** `reserved` is zero
  (`SUEASD_REGISTRY_BAD_RESERVED`).
- **SUEASD-I5 - Script and region agree.** A record's `source_region` is the
  regional class of its `primary_script` (section 3.4).
- **SUEASD-I6 - Ordered and unique.** Curated records are ascending by
  codepoint and unique by codepoint (`SUEASD_REGISTRY_UNORDERED`,
  `SUEASD_REGISTRY_DUPLICATE_CODEPOINT`).
- **SUEASD-I7 - Ideographic agreement.** `SUEASD_FLAG_IDEOGRAPHIC` appears only
  on the Han, Bopomofo and Small Seal scripts (`SUEASD_REGISTRY_IDEOGRAPH_MISMATCH`).
- **SUEASD-I8 - Kana agreement.** `SUEASD_FLAG_KANA` appears only on Hiragana
  and Katakana (`SUEASD_REGISTRY_KANA_MISMATCH`).
- **SUEASD-I9 - Jamo and syllable are exclusive.** Both `SUEASD_FLAG_JAMO` and
  `SUEASD_FLAG_SYLLABLE` may not be set (`SUEASD_REGISTRY_JAMO_MISMATCH`).
- **SUEASD-I10 - Ordered classification ranges.** Classification ranges are
  ascending and disjoint (`SUEASD_REGISTRY_RANGE_UNORDERED`).
- **SUEASD-I11 - Totality.** `suas_sueasd_get_entry` returns `true` for every
  codepoint in a SUEASD text space, curated or not (section 6).

### 1.4 Deviations from the reference sketch

1. **Classification.** The sketch labelled the module **SUAS-017** and named it
   the SuperUnicode East Asian Script Data standard. It is registered as
   **SUAS-017**, immediately after SUAS-016 (SUKEMET), keeping the SUAS series
   contiguous with no reserved gaps (SUAS-010 section 11.1). The registry
   *contents* are version data; the engine, record layout, taxonomy and
   verification rules are mandatory architecture.
2. **Macro prefix.** Spec-level constants carry both the sketch names
   (`SUEASD_SPEC_ID`, `SUEASD_SPEC_TITLE`, `SUEASD_VERSION_MAJOR`,
   `SUEASD_VERSION_MINOR`, `SUEASD_FLAG_*`) and the house aliases
   (`SUAS_SUEASD_*`). The sketch's record and entry points are preserved
   verbatim.
3. **Two repaired macros.** The sketch printed
   `SUEASD_FLAG_COMPATIBILITY (1U << 2)` and
   `SUEASD_FLAG_CUSTOM_SCRIPT (1U << 3)` without the `=`. Both are defined here
   with the intended `((1U << n))` values.
4. **Extended script enum.** The sketch named the five scripts of the CJK
   core plus Bopomofo. UAX #60 covers four further scripts (Jurchen, Nushu,
   Small Seal, Tangut) and Unicode 18.0.0 encodes one more East Asian script
   (Khitan Small Script), so `sueasd_script_id_t` extends the sketch's values
   in place and appends them:
   `JURCHEN = 6`, `NUSHU = 7`, `SEAL = 8`, `TANGUT = 9`,
   `KHITAN_SMALL = 10`, `COUNT = 11`. The sketch's five values keep their
   numeric assignments.
5. **A parallel metadata record.** The sketch's 12-byte record has no room for
   the source mask, the radical-stroke counts, the numeric value, the raw-field
   hashes or the modern-CJK equivalence. Rather than widen the record, SUEASD
   keeps them in a **parallel table** aligned index-for-index with the main
   table, exactly as SUKEMET keeps the Unikemet catalogue beside its sign
   record. The metadata record is exactly 24 bytes (section 3.2).
6. **A classification range layer.** Enumerating every East Asian codepoint
   would exceed the 65,534 index-slot ceiling. SUEASD therefore resolves a
   base record from a compact table of **78 disjoint, coalesced ranges** and
   consults the curated table only for the ancillary layer (section 3.3).

---

## 2. Definitions and Notation

### 2.1 Definitions

- **East Asian repertoire** - the union of the codepoint ranges that
  `Scripts.txt` assigns to Han, Hangul, Hiragana, Katakana, Bopomofo, Jurchen,
  Nushu, Small Seal, Tangut and Khitan Small Script.
- **Classification range** - a maximal contiguous run of codepoints that share
  one primary script and one base flag set.
- **Curated record** - a record present in the compiled ancillary table, drawn
  from one or more UAX #60 source rows.
- **Synthesized record** - the algorithmic default of section 6, returned for
  an in-repertoire codepoint with no curated row.
- **Covered script** - a script for which UAX #60 publishes ancillary source
  data: Jurchen, Nushu, Small Seal and Tangut.
- **Native & Neography Plane carve-out (RNUR)** - `0x00160000-0x0016FFFF`,
  the neography region of SUAS-005 section 5.4.
- **Source row** - one line of a UAX #60 source manifest, a triple of
  codepoint, property tag and value.

### 2.2 Notational conventions

- Codepoints are written `U+XXXX` or `0xXXXX`; SuperUnicode coordinates above
  `U+10FFFF` are written as bare 31-bit hex values.
- `SUEASD_FLAG_*` names bits of `script_mask`; `SUEASD_SOURCE_*` names bits of
  `source_mask`.
- "O(1)" means a fixed, data-independent bound. The range resolver scans at
  most 78 entries, a constant.

### 2.3 The address-space zones

| Zone | Range | SUEASD answer |
|------|-------|---------------|
| Bridge | `0x00000000-0x0010FFFF` | Classification ranges; Unicode repertoire |
| SCP | `0x00110000-0x0011FFFF` | Not a text space (dark matter) |
| RNUR | `0x00160000-0x0016FFFF` | Neography; synthesized custom-script default |
| Native | rest of `0x00120000-0x7FFFFFEF` | Text space; synthesized default |
| Trap | `0x7FFFFFF0-0x7FFFFFFE` | Not a text space |
| Sentinel | `0x7FFFFFFF` | Not a text space |
| ExtSUCS | `> 0x7FFFFFFF` | Plugin space; not a SUEASD text space |

### 2.4 The East Asian repertoire

The 78 classification ranges cover the following block groups:

| Script | Representative ranges | Count |
|--------|-----------------------|-------|
| Han | `U+2E80-2FD5`, `U+3400-4DBF`, `U+4E00-9FFF`, `U+F900-FAD9`, `U+20000-33479`, `U+2F800-2FA1D` | 28 |
| Hangul | `U+1100-11FF`, `U+3131-318E`, `U+AC00-D7A3`, `U+FFA0-FFDC` | 14 |
| Hiragana | `U+3041-309F`, `U+1B001-1B11F`, `U+1B123`, `U+1F200` | 8 |
| Katakana | `U+30A1-30FF`, `U+31F0-31FF`, `U+32D0-3357`, `U+FF66-FF9D`, `U+1AFF0-1B16A` | 16 |
| Bopomofo | `U+02EA-02EB`, `U+3105-312F`, `U+31A0-31BF` | 3 |
| Jurchen | `U+18E00-191D2` | 2 |
| Nushu | `U+16FE1`, `U+1B170-1B2FB` | 2 |
| Small Seal | `U+3D000-3FC3F` | 1 |
| Tangut | `U+16FE0`, `U+17000-18DF2` | 4 |
| Khitan Small Script | `U+16FE4`, `U+18B00-18CFF` | 3 |

(The quoted totals differ slightly from the coalesced 78 because a range is
split whenever the derived flag set changes within a `Scripts.txt` range.)

---

## 3. The Record, Metadata and Index

### 3.1 The script record

The seeding sketch's fields, verbatim and in order. Exactly 12 bytes.

| Offset | Field | Type | Meaning |
|--------|-------|------|---------|
| 0 | `codepoint` | `uint32_t` | Raw 31-bit SuperUnicode codepoint |
| 4 | `script_mask` | `uint32_t` | Packed property and behavior flags |
| 8 | `primary_script` | `uint16_t` | `sueasd_script_id_t` |
| 10 | `source_region` | `uint8_t` | `sueasd_region_t` |
| 11 | `reserved` | `uint8_t` | Must be zero |

```c
typedef struct {
    uint32_t codepoint;
    uint32_t script_mask;
    uint16_t primary_script;
    uint8_t  source_region;
    uint8_t  reserved;
} sueasd_script_entry_t;
```

### 3.2 The parallel metadata record

Exactly 24 bytes, aligned index-for-index with the curated table.

| Offset | Field | Type | Meaning |
|--------|-------|------|---------|
| 0 | `source_mask` | `uint16_t` | `SUEASD_SOURCE_*` bits present |
| 2 | `radical` | `uint16_t` | Radical/component number, 0 if absent |
| 4 | `numeric_value` | `uint16_t` | Integer numeric value, 0 if absent |
| 6 | `stroke_count` | `uint8_t` | Total strokes, 0 if absent |
| 7 | `source_count` | `uint8_t` | Distinct UAX #60 rows |
| 8 | `reading_hash` | `uint32_t` | FNV-1a of the primary reading, 0 if absent |
| 12 | `source_hash` | `uint32_t` | FNV-1a of the primary source id, 0 if absent |
| 16 | `aux_hash` | `uint32_t` | FNV-1a of the raw RS/Rad value, 0 if absent |
| 20 | `modern_cjk` | `uint32_t` | `kSEAL_MCJK` equivalence, 0 if absent |

The radical is 16 bits rather than 8 because the Small Seal and Tangut
radical sets are numbered past 255 (the shipped maximum is 783). The stroke
count, by contrast, is bounded by the repertoire (the shipped maximum is 22).

### 3.3 The classification range

```c
typedef struct {
    uint32_t first;
    uint32_t last;
    uint16_t primary_script;
    uint16_t base_flags;
} sueasd_range_t;
```

Exactly 12 bytes. A range names a maximal run of codepoints that share a
script and a base flag set. Resolution is a fixed linear scan over at most 78
entries; the record for a codepoint is the range's base record unless the
curated table has an entry for that codepoint, in which case the curated
record (which repeats the same base flags) wins.

### 3.4 The regional source classes

`source_region` is the regional class of `primary_script`, not a per-row
provenance tag:

| Value | Region | Scripts |
|-------|--------|---------|
| 0 | `NONE` | Unknown / neography |
| 1 | `CHINA` | Han, Bopomofo |
| 2 | `KOREA` | Hangul |
| 3 | `JAPAN` | Hiragana, Katakana |
| 4 | `JURCHEN` | Jurchen |
| 5 | `NUSHU` | Nushu |
| 6 | `SEAL` | Small Seal |
| 7 | `TANGUT` | Tangut |
| 8 | `KHITAN` | Khitan Small Script |

### 3.5 The classification flags

| Bit | Flag | Meaning |
|-----|------|---------|
| 0 | `SUEASD_FLAG_IDEOGRAPHIC` | Han-family grid and justification role |
| 1 | `SUEASD_FLAG_RADICAL` | Serves as a primary CJK radical component |
| 2 | `SUEASD_FLAG_COMPATIBILITY` | Compatibility ideograph variant |
| 3 | `SUEASD_FLAG_CUSTOM_SCRIPT` | Neography / conscript declaration |
| 4 | `SUEASD_FLAG_HAN_CORE` | Unified ideograph (UAX #38 core) |
| 5 | `SUEASD_FLAG_HAN_RADICAL` | CJK / Kangxi Radical block |
| 6 | `SUEASD_FLAG_HAN_COMPAT` | Compatibility ideograph |
| 7 | `SUEASD_FLAG_KANA` | Hiragana or Katakana |
| 8 | `SUEASD_FLAG_JAMO` | Hangul Jamo |
| 9 | `SUEASD_FLAG_SYLLABLE` | Precomposed Hangul syllable |
| 10 | `SUEASD_FLAG_SOURCE_DATA` | Has UAX #60 ancillary source data |
| 11 | `SUEASD_FLAG_READING` | Has a reading (Jurchen / Nushu) |
| 12 | `SUEASD_FLAG_RS` | Has radical-stroke data |
| 13 | `SUEASD_FLAG_NUMERIC` | Has a numeric value |
| 14 | `SUEASD_FLAG_MODERN_CJK` | Small Seal: `kSEAL_MCJK` present |
| 15 | `SUEASD_FLAG_NEOGRAPHY` | Native & Neography Plane |

`SUEASD_FLAG_ASSIGNED_MASK` is `0xFFFF`: every bit is assigned.

`IDEOGRAPHIC` is deliberately narrower than "belongs to a logographic
script". It marks the scripts that participate in the modern CJK grid as
SUEASD models it - Han, Bopomofo and Small Seal, the last because its records
carry modern-CJK equivalences. Jurchen, Nushu and Tangut are identified by
`primary_script` and `SOURCE_DATA`; the validator enforces this boundary
(SUEASD-I7).

### 3.6 The UAX #60 source mask

| Bit | Bit name | UAX #60 property |
|-----|----------|------------------|
| 0 | `SUEASD_SOURCE_JURC_NCREADING` | `kJURC_NCReading` |
| 1 | `SUEASD_SOURCE_JURC_NUMERIC` | `kJURC_Numeric` |
| 2 | `SUEASD_SOURCE_JURC_RSUNICODE` | `kJURC_RSUnicode` |
| 3 | `SUEASD_SOURCE_JURC_SRC` | `kJURC_Src` |
| 4 | `SUEASD_SOURCE_NSHU_DUBENSRC` | `kNSHU_DubenSrc` |
| 5 | `SUEASD_SOURCE_NSHU_READING` | `kNSHU_Reading` |
| 6 | `SUEASD_SOURCE_SEAL_CCZSRC` | `kSEAL_CCZSrc` |
| 7 | `SUEASD_SOURCE_SEAL_DYCSRC` | `kSEAL_DYCSrc` |
| 8 | `SUEASD_SOURCE_SEAL_MCJK` | `kSEAL_MCJK` |
| 9 | `SUEASD_SOURCE_SEAL_QJZSRC` | `kSEAL_QJZSrc` |
| 10 | `SUEASD_SOURCE_SEAL_RAD` | `kSEAL_Rad` |
| 11 | `SUEASD_SOURCE_SEAL_THXSRC` | `kSEAL_THXSrc` |
| 12 | `SUEASD_SOURCE_TGT_MERGEDSRC` | `kTGT_MergedSrc` |
| 13 | `SUEASD_SOURCE_TGT_NUMERIC` | `kTGT_Numeric` |
| 14 | `SUEASD_SOURCE_TGT_RSUNICODE` | `kTGT_RSUnicode` |
| 15 | `SUEASD_SOURCE_HAN` | A Han source declaration (UAX #38) |

### 3.7 The hash

`suas_sueasd_hash` is FNV-1a 32-bit over the little-endian encoding of the
codepoint; it is the index key, not an identity. `suas_sueasd_hash_bytes` is
the same function over arbitrary bytes and is the one the generator uses for
the three metadata hashes. The seeds and prime are the standard FNV-1a
constants (`2166136261` and `16777619`). Published vectors:

| Input | FNV-1a |
|-------|--------|
| codepoint `U+4E00` | `0x94A4F9BB` |
| codepoint `U+18E00` | `0x284F5212` |
| `"nie"` | `0x16A40717` |
| `"NC:002.02"` | `0xAA5125EA` |
| `"1.3"` | `0xEA421A5F` |
| `"i5"` | `0x8838EA63` |
| `"36.01"` | `0x59160879` |

### 3.8 The index

A fixed open-addressing table of 65,536 `uint16_t` slots. A slot holds
`entry_index + 1`; zero means empty. The index is built lazily on the first
query, is idempotent, and never reallocates. Because the curated count
(18,815) is well below the 65,534 slot ceiling, every probe terminates.

---

## 4. UAX #60 / SUCD Field Mapping

| UAX #60 / Scripts.txt | SUEASD field | Transformation |
|-----------------------|--------------|----------------|
| `Scripts.txt` script name | `primary_script` | Mapped to `sueasd_script_id_t` |
| Block position (radical, compat, kana, jamo, syllable) | `script_mask` base bits | Derived per codepoint |
| `kJURC_Src`, `kNSHU_DubenSrc`, `kTGT_MergedSrc`, Seal sources | `source_mask` + `source_hash` | Tag mapped; primary value hashed |
| `kJURC_NCReading`, `kNSHU_Reading` | `source_mask` + `reading_hash` | Tag mapped; primary value hashed |
| `kJURC_RSUnicode`, `kTGT_RSUnicode`, `kSEAL_Rad` | `radical` + `stroke_count` + `aux_hash` | `R.S` split; raw value hashed |
| `kJURC_Numeric`, `kTGT_Numeric` | `numeric_value` | Parsed to an integer |
| `kSEAL_MCJK` | `modern_cjk` | Parsed to a codepoint |
| Number of distinct rows | `source_count` | Counted per codepoint |
| Presence of any covered-script tag | `SUEASD_FLAG_SOURCE_DATA` | Flag |
| Regional script tradition | `source_region` | Script-derived enum |

Where a codepoint carries several source rows of the same kind, the first
value in the file is the primary one; the hash records that value while
`source_count` records how many rows the codepoint had.

---

## 5. The Script Families

### 5.1 Han

The Han ranges carry `IDEOGRAPHIC` and exactly one of `HAN_CORE`,
`HAN_RADICAL` or `HAN_COMPAT`:

- `HAN_CORE` for a unified ideograph;
- `HAN_RADICAL` plus the sketch's `RADICAL` flag for a Kangxi/CJK radical
  (`U+2E80-2EF3`, `U+2F00-2FD5`);
- `HAN_COMPAT` plus `COMPATIBILITY` for a compatibility ideograph
  (`U+F900-FAD9`, `U+2F800-2FA1D`).

### 5.2 Kana

Hiragana and Katakana carry `KANA` and nothing in the Han role bits. The
supplementary kana blocks (`U+1AFF0-1B16A`, `U+1F200`) and the halfwidth
Katakana block (`U+FF66-FF9D`) use the same flag.

### 5.3 Hangul

Hangul splits into two exclusive roles:

- `JAMO` for the Jamo blocks and the compatibility Jamo blocks
  (`U+1100-11FF`, `U+A960-A97C`, `U+D7B0-D7FB`, `U+3131-318E`, `U+FFA0-FFDC`);
- `SYLLABLE` for the precomposed syllables (`U+AC00-D7A3`).

### 5.4 The UAX #60 covered scripts

- **Jurchen** (`U+18E00-191D2`): 914 curated codepoints with readings
  (`kJURC_NCReading`), source identifiers (`kJURC_Src`), radical-stroke
  indexes (`kJURC_RSUnicode`) and occasional numeric values
  (`kJURC_Numeric`).
- **Nushu** (`U+16FE1`, `U+1B170-1B2FB`): 396 curated codepoints with
  Duben source identifiers and readings; no radical-stroke data.
- **Small Seal** (`U+3D000-3FC3F`): 11,328 curated codepoints with up to six
  source rows each (`kSEAL_CCZSrc`, `kSEAL_DYCSrc`, `kSEAL_MCJK`,
  `kSEAL_QJZSrc`, `kSEAL_Rad`, `kSEAL_THXSrc`). `kSEAL_MCJK` yields a
  modern-CJK equivalence and sets `SUEASD_FLAG_MODERN_CJK`.
- **Tangut** (`U+16FE0`, `U+17000-18DF2`): 6,177 curated codepoints with
  merged source identifiers, radical-stroke indexes and numeric values.

### 5.5 Khitan Small Script

`U+16FE4`, `U+18B00-18CFF` are classified as `SUEASD_SCRIPT_KHITAN_SMALL`
with region `KHITAN`. Unicode 18.0.0 encodes the script but UAX #60 does not
yet publish ancillary data for it, so it has no curated rows and its
predicates answer from the classification range alone.

### 5.6 Bopomofo and the white-space ideographs

Bopomofo (`U+02EA-02EB`, `U+3105-312F`, `U+31A0-31BF`) is its own script with
`IDEOGRAPHIC` set. The Han-script white-space and iteration characters
(`U+3005`, `U+3007`, `U+3021-3029`, `U+3038-303B`, `U+16FE2-16FF6`) resolve as
Han core from the classification ranges.

---

## 6. Zone Dispatch and the Algorithmic Default

### 6.1 Zone dispatch

`suas_sueasd_zone_of` classifies a 31-bit value into Bridge, SCP, RNUR, Native,
Trap, Sentinel or ExtSUCS (section 2.3). `suas_sueasd_is_text_space` is true
for Bridge, RNUR and Native, and false for the other four.

`suas_sueasd_get_entry` returns `false`, leaving the output untouched, for a
NULL output or an inadmissible zone (SCP, trap, Sentinel, ExtSUCS).

`suas_sueasd_neography_set` decodes an RNUR coordinate into its set number,
`(cp - 0x00160000) >> 8`, and returns `false` outside the carve-out.

### 6.2 The algorithmic default

For a codepoint in a text space with no classification range and no curated
row, `suas_sueasd_get_entry` returns `true` and synthesizes:

```c
codepoint        = cp;
script_mask      = (is RNUR) ? SUEASD_FLAG_CUSTOM_SCRIPT : 0;
primary_script   = SUEASD_SCRIPT_UNKNOWN;
source_region    = SUEASD_REGION_NONE;
reserved         = 0;
```

`suas_sueasd_get_metadata` returns an all-zero metadata record for the same
codepoint. `suas_sueasd_is_east_asian` and `suas_sueasd_is_curated` remain
`false` for a non-repertoire codepoint, so a caller can always distinguish a
range-resolved or curated record from a synthesized default. This is why a
renderer can answer for a newly declared RNUR coordinate, or for a codepoint
outside the repertoire, without special-casing a missing lookup.

---

## 7. Data Files and the Curated Table

The shipped table is generated from Unicode 18.0.0 (UAX #60 Rev 3,
2026-09-02):

- `Scripts.txt`: the script classification behind the 78 coalesced ranges;
- `JurchenSources.txt`: 914 curated codepoints;
- `NushuSources.txt`: 396 curated codepoints;
- `SealSources.txt`: 11,328 curated codepoints;
- `TangutSources.txt`: 6,177 curated codepoints.

The result is **18,815 curated records**, each with a parallel metadata row,
and **78 classification ranges** that make the property total over the
repertoire. The capacity `SUAS_SUEASD_REGISTRY_CAPACITY` is 32,768 and the
index is 65,536 slots, leaving room for a later UAX #60 revision without
changing the index geometry.

The data is a generated `.inc` file included by `suas_sueasd.c`. The arrays
`sueasd_table[]`, `sueasd_meta_table[]` and `sueasd_range_table[]` are aligned
and their lengths are checked by compile-time assertion. Regenerate through
the SUCD build-time extractor when the Unicode data version changes; do not
edit the catalogue by hand.

---

## 8. Verification

`suas_sueasd_validate_table` checks a compiled table in one left-to-right
pass. It accepts an empty table (`entries == NULL, count == 0`) as
conformant; an implementation with an empty table still answers every
repertoire codepoint by the default of section 6. The outcomes are:

| Error | Condition |
|-------|-----------|
| `SUEASD_REGISTRY_NULL_TABLE` | `count > 0` and `entries == NULL` |
| `SUEASD_REGISTRY_COUNT_OVER` | `count > SUAS_SUEASD_REGISTRY_CAPACITY` |
| `SUEASD_REGISTRY_UNORDERED` | codepoints not ascending |
| `SUEASD_REGISTRY_DUPLICATE_CODEPOINT` | same codepoint twice |
| `SUEASD_REGISTRY_OUT_OF_SPACE` | `codepoint > 0x7FFFFFFF` |
| `SUEASD_REGISTRY_INADMISSIBLE_CODEPOINT` | outside a SUEASD text space or repertoire |
| `SUEASD_REGISTRY_BAD_SCRIPT` | `primary_script >= SUEASD_SCRIPT_COUNT` |
| `SUEASD_REGISTRY_BAD_REGION` | `source_region > SUEASD_REGION_KHITAN` |
| `SUEASD_REGISTRY_BAD_RESERVED` | `reserved != 0` |
| `SUEASD_REGISTRY_IDEOGRAPH_MISMATCH` | `IDEOGRAPHIC` outside Han/Bopomofo/Seal |
| `SUEASD_REGISTRY_KANA_MISMATCH` | `KANA` outside Hiragana/Katakana |
| `SUEASD_REGISTRY_JAMO_MISMATCH` | `JAMO` and `SYLLABLE` both set |
| `SUEASD_REGISTRY_RANGE_UNORDERED` | ranges overlap or are descending |
| `SUEASD_REGISTRY_RANGE_OUT_OF_SPACE` | a range endpoint past `0x7FFFFFFF` |

`suas_sueasd_validate_ranges` checks a compiled classification range table
with the range subset of the same outcomes (`NULL_TABLE`, `COUNT_OVER`,
`RANGE_OUT_OF_SPACE`, `RANGE_UNORDERED`, `BAD_SCRIPT`).

The shipped table and the shipped ranges are validated in the unit suite; the
full 18,815-row roster passes every check.

---

## 9. The SUAS-010 (SSRE) Linkage Gate

SUAS-017 is registered in SSRE as a **mandatory** architecture specification
with module `suas_sueasd`, version `1.0.0` and tier `L0`. The registration
changes no existing dependency: every descriptor that verified before still
verifies. The SUCM claim `sueasd` and engine `suas_sueasd` mirror the SSRE
row, so a conformance image can name the engine and the owning specification
in one lookup.

---

## 10. Stability and Versioning

The record layout, flag meanings, script and region enum values, taxonomy
names and verification outcomes are architectural. The **contents** of the
classification and ancillary tables are version data and may change with a
Unicode release. A table update that only adds, removes or corrects rows is a
patch-level data change; a change to the 12-byte or 24-byte layout, a flag
meaning, a script enum value or a verification outcome is a major change.

The engine is freestanding C99. No entry point reaches for libc, and the
static tables are immutable after the lazy index build.

---

## 11. Conformance Requirements

An implementation conforms to SUAS-017 when it:

1. compiles the 12-byte record with the exact field order and size, and the
   24-byte metadata record with its exact layout;
2. answers `suas_sueasd_get_entry` in O(1) time for every codepoint in a
   SUEASD text space, curated, range-resolved or synthesized;
3. returns `false`, without touching the output, for an inadmissible zone or a
   NULL output;
4. distinguishes a curated record from a range-resolved or synthesized record
   through `suas_sueasd_is_curated`;
5. preserves the sketch's script numbering and appends the covered scripts and
   Khitan Small Script as section 1.4 specifies;
6. reports `IDEOGRAPHIC`, `KANA`, `JAMO` and `SYLLABLE` only where section 5
   allows, and never sets `reserved`;
7. resolves the regional source class from the primary script and decodes the
   UAX #60 source tags of section 3.6;
8. carries the radical-stroke, reading, numeric and modern-CJK data through
   the metadata queries and their hashes;
9. passes `suas_sueasd_validate_table` and `suas_sueasd_validate_ranges` on
   its compiled tables; and
10. registers itself in SSRE as mandatory and publishes the matching SUCM
    claim.

### 11.1 Compliance matrix

| Requirement | Reference implementation | Test surface |
|-------------|--------------------------|--------------|
| 12-byte record | `sueasd_script_entry_t` | `sizeof`, `offsetof` |
| 24-byte metadata | `sueasd_metadata_t` | `sizeof` |
| 12-byte range | `sueasd_range_t` | `sizeof`, `range_count` |
| O(1) lookup | open-addressing index | 18,815-row enumeration and query identity |
| Totality | section 6 default | unassigned Han, RNUR, SCP/trap misses |
| Script taxonomy | `sueasd_script_id_t` | all 11 identifiers |
| Region taxonomy | `sueasd_region_t` | all 9 classes |
| Han roles | core/radical/compat flags | `U+4E00`, `U+2F00`, `U+F900` |
| Kana / Hangul | kana/jamo/syllable flags | `U+3042`, `U+1100`, `U+AC00` |
| Covered scripts | primary script and source mask | Jurchen, Nushu, Seal, Tangut |
| Radical-stroke | `radical`, `stroke_count`, `aux_hash` | `U+18E00`, `U+1736B` |
| Readings | `reading_hash`, `source_count` | `U+18E00`, `U+1B170` |
| Numeric values | `numeric_value` | `U+18E01`, `U+1736B` |
| Modern CJK | `modern_cjk` | `U+3D000` |
| Khitan | classification only | `U+18B00` |
| Verification | all fourteen outcomes | `test_validate_table`, `test_validate_ranges` |
| SSRE/SUCM linkage | registered rows | `SUAS-017` lookup |

---

## 12. Terms and Conventions

- **SUCD** is the SuperUnicode Character Database (SUAS-012).
- **SSRE** is the System Specification Registry and Cross-Reference Engine
  (SUAS-010).
- **SUCM** is the SuperUnicode Conformance Model (SUTR-10).
- **SIP** is the SuperUnicode Ideographic Properties standard (SUAS-009).
- **RNUR** is the Native & Neography Plane carve-out of SUAS-005.
- **UAX #60** is "Data for East Asian Scripts" (Rev 3, Unicode 18.0.0).
- **Jamo** are the Hangul conjoining letters; a **syllable** is a precomposed
  Hangul syllable. They are mutually exclusive roles here.
- A **curated** record is one present in the compiled ancillary table; a
  **range-resolved** record comes from the classification range; a
  **synthesized** record is the default of section 6.
- A **source row** is one line of a UAX #60 manifest; a codepoint may have
  several, and `source_count` says how many.

---

## 13. Testing

`tests/test_suas_sueasd.c` exercises the version and record geometry, zone
dispatch, the RNUR carve-out, FNV-1a, the classification ranges, every script
family (Han, kana, Hangul, Bopomofo, Jurchen, Nushu, Small Seal, Tangut and
Khitan), the ancillary metadata, the algorithmic default, all 18,815 curated
rows, every verification outcome, the taxonomy names and the SSRE
registration. `unified/test_unified.c` includes the SUEASD header alongside
every other SUAS/SUTS/SUTR header and proves the interfaces coexist in one
translation unit.
