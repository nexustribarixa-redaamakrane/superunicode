# SUAS-014 — SuperUnicode Vertical Text Layout (SUVTL)

**Standard ID:** SUAS-014
**Status:** Draft — Ratified
**Scope:** Core Kernel Invariant
**Applies To:** SUCD, UAX #50 (Vertical Text Layout analog), SUAS-005 (SSP), SUAS-006 (STS), SUAS-009 (SIP), SUAS-011 (SCMS), SUAS-012 (SUCD), SUTS-001 (SUCA), SUTS-003 (SRE), SUTS-005 (SCSS), the SUF font engine, the OpenWindows shell, the System Control Plane
**Last Updated:** 2026-10-21

---

## 1. Overview

SuperUnicode Vertical Text Layout (SUVTL) is the core algorithmic subsystem
governing the **zero-allocation, O(1) resolution of a character's orientation
in vertical text**: whether a glyph stands upright, rotates ninety degrees
clockwise, or requires a distinct vertical form, together with a fine-grained
vertical baseline adjustment.

Where a traditional Unicode environment consults the Vertical_Orientation
(`vo`) property of UAX #50 — a text data file (`VerticalOrientation.txt`) of
semicolon-delimited ranges tagged `U`, `R`, `Tu` or `Tr`, accompanied by
narrative notes and a glyph-change table that every implementation reads and
interprets at run time — SUVTL is a **dual-layer** architecture:

- the **offline reference layer** is the SuperUnicode Character Database
  (SUCD, SUAS-012) and the published `/Public` tree: the
  `VerticalOrientation.txt` manifests, the per-script vertical-flow
  definitions, and the human-readable glyph-change tables meant for font
  designers, user-space layout engines and developer toolchains;
- the **runtime kernel layer** is SUVTL: at build time those records are
  compiled into an immutable, byte-packed binary table — `suvtl_glyph_layout_t`
  — and the kernel executes the SUVTL algorithms directly against the static
  array.

This is the same dual-layer relationship SUAS-009 (SIP) has with UAX #38 and
SUAS-013 (SISR) has with UAX #45.

### 1.1 Architectural shifts from UAX #50

| UAX #50 (`VerticalOrientation.txt`) | SUVTL (SUAS-014) |
|-------------------------------------|------------------|
| A text data file, read and parsed at run time | A static, byte-packed binary table compiled at build time |
| Orientation is a four-letter tag on a range line | Orientation is an 8-byte record: three value bits plus a baseline adjustment; the tag is a derived view |
| Glyph-change tables are narrative text interleaved with the data | The Tu/Tr roster is compiled into the record's TRANSFORM flag; the prose stays in the reference layer (SUCD) and is never parsed by the kernel |
| Range order is file order; a lookup is a search | Records are ordered and indexed; a lookup is a hash probe — O(1) expected |
| A codepoint absent from the file has no defined orientation | The property is **total**: an uncurated codepoint resolves by the algorithmic default of §5 |
| UAX #50 is limited to historical Unicode blocks | SUVTL extends the model into the Native & Neography Plane, where a custom script declares its own vertical flow |

### 1.2 Goals

1. **One record, one orientation.** A codepoint's vertical layout is a fixed
   8-byte descriptor whose every field is byte-aligned and whose every reserved
   bit is proven zero before the table is admitted.
2. **A bounded lookup.** A query hashes the codepoint, probes a fixed index, and
   returns in O(1) expected time, with no search, no allocation and no libc.
3. **A total answer.** Every codepoint in the 31-bit space has exactly one
   orientation. The engine never says "unknown": it falls back to the
   algorithmic default derived from the codepoint's script group and plane.
4. **A proven table.** A compiled registry is verified before it is queried; the
   verification surface names each defect rather than returning a bare failure.
5. **No duplication of SUAS-006.** STS decides where a grapheme cluster begins
   and ends; SUVTL takes the *first* character of a cluster as the cluster's
   orientation (§4). The two compose; neither re-derives the other.

### 1.3 Non-negotiable invariants

- **SUVTL-I1 — Zero allocation.** No SUVTL operation allocates. The table is
  static; every descriptor is a fixed-size struct the caller owns.
- **SUVTL-I2 — Bounded lookup.** A lookup hashes the codepoint, probes a fixed
  index, and returns. The number of probes is bounded by a small constant over a
  table whose load factor is below one-half.
- **SUVTL-I3 — Text spaces only.** A record may name only a Bridge, RNUR or
  Native codepoint. The Control Plane, the trap slots, the Sentinel and ExtSUCS
  are never orientations (`SUVTL_REGISTRY_INADMISSIBLE_CODEPOINT`,
  `SUVTL_REGISTRY_OUT_OF_SPACE`).
- **SUVTL-I4 — Assigned bits only.** A `1.0` record sets no flag bit above the
  six this standard assigns (`SUVTL_REGISTRY_BAD_FLAGS`).
- **SUVTL-I5 — Reserved bytes are zero.** The reserved byte of a layout record
  is zero (`SUVTL_REGISTRY_BAD_RESERVED`).
- **SUVTL-I6 — Ordered and unique.** Records are ascending by codepoint and
  unique by codepoint (`SUVTL_REGISTRY_UNORDERED`,
  `SUVTL_REGISTRY_DUPLICATE_CODEPOINT`).
- **SUVTL-I7 — A well-formed value.** A record's three orientation bits name
  exactly one of U/R/Tu/Tr, so exactly one of UPRIGHT and ROTATED is set
  (`SUVTL_REGISTRY_BAD_ORIENTATION`).
- **SUVTL-I8 — Custom flow is a neography right.** `SUVTL_FLAG_CUSTOM_FLOW`
  appears only on an RNUR record (`SUVTL_REGISTRY_CUSTOM_FLOW_MISMATCH`).
- **SUVTL-I9 — Totality.** `suas_suvtl_get_layout_properties` returns `true`
  for every codepoint in a text space, curated or not (§5.3).

### 1.4 Deviations from the reference sketch

1. **Classification.** The seeding sketch labelled SUVTL **SUAS-014**, and the
   series owner ratified it as a **SUAS** core invariant, not a SUTS
   specification. It is registered as **SUAS-014**, the number immediately after
   SUAS-013 (SISR), which keeps the SUAS series contiguous with no reserved gaps
   (§11.1 of SUAS-010). The one element of the SUTS framing that survives is
   deliberate: the **registry contents are optional version data**. The engine,
   the record layout, the lookup contract and the verification rules are
   mandatory architecture; an implementation with no curated records ships an
   **empty registry** and remains conformant (§11), resolving every codepoint by
   the algorithmic default.
2. **A stale numbering reference.** The sketch named the offline reference layer
   "SUCA". In this corpus SUCA is already the **SuperUnicode Collation
   Algorithm** (SUTS-001), so SUVTL names **SUCD (SUAS-012)** as its offline
   layer — exactly as SUAS-009 (SIP), SUTS-005 (SCSS) and SUAS-013 (SISR)
   already do. The reference is corrected throughout (§1, §7).
3. **Macro prefix.** The orientation value masks use `SUVTL_ORIENT_*`;
   spec-level constants (`SUAS_SUVTL_SPEC_ID`, `SUAS_SUVTL_REGISTRY_CAPACITY`,
   `SUAS_SUVTL_INDEX_SIZE`, `SUAS_SUVTL_ZONE_*`, …) carry the `SUAS_SUVTL_*`
   prefix. Functions use `suas_suvtl_*`, and the public record type keeps the
   sketch's own name.
4. **The value is the bits.** The sketch supplied `UPRIGHT = 1<<0`,
   `ROTATED = 1<<1` and `TRANSFORM = 1<<2` as independent flags. SUVTL keeps
   those three bits verbatim as the *normative encoding* of UAX #50's four
   values rather than a separate enum: `U = UPRIGHT`, `R = ROTATED`,
   `Tu = UPRIGHT | TRANSFORM`, `Tr = ROTATED | TRANSFORM`. There is no
   second, redundant value field to keep in sync, and a caller that only wants
   "does this rotate?" tests one bit.
5. **A total property.** UAX #50 leaves an absent codepoint undefined. SUVTL
   requires a **total** answer and supplies the algorithmic default of §5 so
   that a layout engine never has to invent one.

---

## 2. Definitions and Notation

### 2.1 Definitions

- **Vertical orientation.** How a character is rendered when the text stream
  flows in vertical columns: standing upright (`U`), rotated ninety degrees
  clockwise (`R`), or requiring a distinct vertical glyph whose fallback is
  upright (`Tu`) or rotated (`Tr`).
- **Transformed form.** A `Tu` or `Tr` character: its default (non-vertical)
  rendering is not acceptable in vertical text, so a font supplies a distinct
  vertical glyph. The TRANSFORM bit records this.
- **Grapheme cluster.** A user-perceived character, bounded by SUAS-006 (STS).
  SUVTL does not compute the boundary; it consumes one cluster.
- **Enclosing mark.** A combining mark of general category **Me**, the sole
  exception to the first-character rule of §4.
- **Squared word.** A katakana or ideographic word enclosed in a square
  (`U+3300`-range, `U+1F200`-range), tagged `Tu` and carrying
  `SUVTL_FLAG_SQUARED`.
- **Custom flow.** A vertical-flow direction declared for a Native/Neography
  codepoint rather than inherited from a historical default; recorded with
  `SUVTL_FLAG_CUSTOM_FLOW`.
- **Algorithmic default.** The orientation synthesized for an uncurated
  codepoint from its script group and plane (§5.3).
- **RNUR set number.** For a neography coordinate,
  `set = (cp − 0x00160000) >> 8` (SUAS-005 §5.4) — an O(1) decode.

### 2.2 Notational conventions

`U+` names a Bridge-plane Unicode codepoint; a bare hexadecimal value names a
Base SUCS codepoint. `cp` is a 31-bit Base SUCS value
(`0x00000000–0x7FFFFFFF`). A value above `SUAS_CODEPOINT_MAX` is ExtSUCS and is
outside the registry. The strings `U`, `R`, `Tu`, `Tr` are UAX #50's own value
tags; the macros `SUVTL_ORIENT_U`, `SUVTL_ORIENT_R`, `SUVTL_ORIENT_TU` and
`SUVTL_ORIENT_TR` are their binary encodings.

### 2.3 The address-space zones

| Zone | Range | Text space? |
|------|-------|-------------|
| `SUAS_SUVTL_ZONE_BRIDGE` | `0x00000000–0x0010FFFF` | yes |
| `SUAS_SUVTL_ZONE_SCP` | `0x00110000–0x0011FFFF` | no — Control Plane |
| `SUAS_SUVTL_ZONE_RNUR` | `0x00160000–0x0016FFFF` | yes — neography |
| `SUAS_SUVTL_ZONE_NATIVE` | rest of `0x00120000–0x7FFFFFEF` | yes — custom conscript space |
| `SUAS_SUVTL_ZONE_TRAP` | `0x7FFFFFF0–0x7FFFFFFE` | no |
| `SUAS_SUVTL_ZONE_SENTINEL` | `0x7FFFFFFF` | no |
| `SUAS_SUVTL_ZONE_EXTSUCS` | `> 0x7FFFFFFF` | no — plugin space |

`suas_suvtl_is_text_space` is true for Bridge, RNUR and Native only.

---

## 3. The Record and the Index

### 3.1 The orientation record

```c
typedef struct {
    uint32_t codepoint;         /* Base SUCS codepoint                */
    uint16_t orientation_flags; /* SUVTL_ORIENT_* + SUVTL_FLAG_*      */
    uint8_t  baseline_offset;   /* signed 1/16-em vertical adjustment */
    uint8_t  reserved;          /* must be zero                       */
} suvtl_glyph_layout_t;
```

The record is 8 bytes on every supported ABI, with no padding. Its field
offsets are `0, 4, 6, 7`.

### 3.2 The orientation value and the flags

UAX #50's four values are encoded by three bits, exactly as the seeding sketch
exposed them:

| Value | Encoding | Meaning | Vertical fallback |
|-------|----------|---------|-------------------|
| `U`  | `SUVTL_ORIENT_U  = 0x0001` = UPRIGHT | upright | — |
| `R`  | `SUVTL_ORIENT_R  = 0x0002` = ROTATED | rotated 90° clockwise | — |
| `Tu` | `SUVTL_ORIENT_TU = 0x0005` = UPRIGHT \| TRANSFORM | transformed upright | `U` |
| `Tr` | `SUVTL_ORIENT_TR = 0x0006` = ROTATED \| TRANSFORM | transformed rotated | `R` |

`SUVTL_ORIENT_MASK` is `0x0007`. A well-formed record sets exactly one of
UPRIGHT and ROTATED (SUVTL-I7).

The orthogonal flags sit above the value:

| Bit | Name | Meaning |
|-----|------|---------|
| `0x0001` | `SUVTL_ORIENT_UPRIGHT` | render upright in vertical flows |
| `0x0002` | `SUVTL_ORIENT_ROTATED` | rotate 90° clockwise |
| `0x0004` | `SUVTL_ORIENT_TRANSFORM` | requires a distinct vertical glyph |
| `0x0008` | `SUVTL_FLAG_CUSTOM_FLOW` | Native/Neography baseline rules |
| `0x0010` | `SUVTL_FLAG_SQUARED` | squared kata/ideographic word |
| `0x0020` | `SUVTL_FLAG_QUOTATION` | quotation mark; font-substituted |

`SUVTL_FLAG_ASSIGNED_MASK` is `0x003F`; the upper ten bits are reserved and must
be zero.

### 3.3 The baseline adjustment

`baseline_offset` is a **signed** fine-tuning value in **1/16 em**
(`SUVTL_BASELINE_SCALE` = 16), added to the character's vertical position within
its column cell. Zero is the default; a positive value shifts the glyph toward
the end of the column. A curated record may carry a nonzero value for a
neography logograph whose declared baseline differs from the ideographic
default. `suas_suvtl_baseline_offset` returns it sign-extended as an `int`.

### 3.4 The hash

`suas_suvtl_hash` is FNV-1a 32-bit (offset `2166136261`, prime `16777619`) over
the four little-endian bytes of the codepoint. It is the index key, not an
identity.

### 3.5 The index

The static table holds at most `SUAS_SUVTL_REGISTRY_CAPACITY` (512) records; a
fixed open-addressing array of `SUAS_SUVTL_INDEX_SIZE` (1024) slots is the
index:

1. The first query hashes each record's codepoint and places its offset in the
   first free slot, resolving collisions by linear probing. The build is O(n)
   once and touches no heap.
2. Every query then hashes its codepoint and probes the same sequence until it
   matches or hits an empty slot.

Because the load factor is below one-half over a bounded, static table, the
probe count is bounded by a small constant. Queries build the index lazily on
first use, so they are correct even if `suas_suvtl_init_engine` was not called.
`suas_suvtl_init_engine` is idempotent and allocation-free.

---

## 4. The Grapheme-Cluster Rule

UAX #50 defines the orientation of a grapheme cluster as the orientation of its
first character, with one exception: if the cluster contains an **enclosing
combining mark** (general category Me), the whole cluster is `U`.

`suas_suvtl_cluster_orientation(codepoints, count)` applies that rule to one
cluster the caller supplies. It scans the cluster for an Me mark; if one is
present it returns `SUVTL_ORIENT_U`, otherwise it returns
`suas_suvtl_orientation_of(codepoints[0])`. A NULL stream or a zero count is
`Upright`.

The engine does **not** compute cluster boundaries — that is SUAS-006 (STS).
`suas_suvtl_is_enclosing_mark` answers the Me question from a small curated set
of Me ranges, so a caller need not have loaded SUCD:

| Range | Block |
|-------|-------|
| `U+0488–U+0489` | Cyrillic combining enclosing |
| `U+1ABE–U+1AFF` | combining diacritical marks extended |
| `U+20DD–U+20E0`, `U+20E2–U+20E4` | combining enclosing marks for symbols |
| `U+A670–U+A672` | Cyrillic combining enclosing (extended) |
| `U+FE20–U+FE2F` | combining half marks |

---

## 5. Range Dispatch, Groups and the Algorithmic Default

### 5.1 Zone dispatch

`suas_suvtl_zone_of` tests the ExtSUCS bound first, then the Bridge ceiling, then
the SCP, trap and Sentinel ranges, then the RNUR carve-out, and defaults to
Native. `suas_suvtl_neography_set` returns `(cp − 0x00160000) >> 8` and fails
outside the carve-out — the same decode SUAS-005 §5.4 publishes.

### 5.2 Vertical-behaviour groups

The algorithmic default needs only the codepoint's *vertical behaviour*, not its
script identity; a caller that needs the script itself asks SUAS-005 (SSP). The
closed enumeration `suas_suvtl_group_t` is:

| Group | Members | Default orientation |
|-------|---------|---------------------|
| `SUAS_SUVTL_GROUP_CJK` | Han, Kana, Hangul, Bopomofo, CJK compatibility/fullwidth forms, Ext-B+ | `U` |
| `SUAS_SUVTL_GROUP_ROTATING` | Latin, Greek, Cyrillic | `R` |
| `SUAS_SUVTL_GROUP_OTHER` | punctuation, symbols, the remainder | `U` |
| `SUAS_SUVTL_GROUP_NATIVE` | the RNUR carve-out | `U` |

### 5.3 The algorithmic default

A codepoint with no curated record is not unknown. Its orientation follows from
its group: a rotating-script codepoint defaults to `R`; every other group
defaults to `U`, which is UAX #50's own default for unassigned and common
characters. `suas_suvtl_get_layout_properties` synthesizes a record carrying the
defaulted value and returns **true**; it returns **false** only for an
inadmissible zone (SCP, trap, Sentinel, ExtSUCS) or a NULL output
(SUVTL-I9). The property is total.

---

## 6. The Query Surface

| Function | Cost | Behaviour |
|----------|------|-----------|
| `suas_suvtl_init_engine` | O(n) once | Idempotent, allocation-free |
| `suas_suvtl_get_layout_properties` | O(1) expected | Copies the record, or the algorithmic default; false only on an inadmissible zone or NULL output |
| `suas_suvtl_orientation_of` | O(1) | The value only; `U` on an inadmissible zone |
| `suas_suvtl_is_upright` / `is_rotated` / `is_transformed` | O(1) | Derived predicates |
| `suas_suvtl_baseline_offset` | O(1) | The signed 1/16-em adjustment; 0 on an inadmissible zone |
| `suas_suvtl_cluster_orientation` | O(cluster) | The §4 grapheme rule |
| `suas_suvtl_is_enclosing_mark` | O(1) | The curated Me test |
| `suas_suvtl_is_registered` | O(1) expected | Curated-record presence test |
| `suas_suvtl_entry_count` / `entry_at` | O(1) | Enumeration in ascending codepoint order |
| `suas_suvtl_zone_of` / `is_text_space` / `is_neography` | O(1) | Range dispatch |
| `suas_suvtl_neography_set` | O(1) | RNUR set decode |
| `suas_suvtl_group_of` | O(1) | Vertical-behaviour group |
| `suas_suvtl_hash` | O(1) | The index key |
| `suas_suvtl_validate_table` | O(n²) worst case | Compiler-time verifier, `n ≤ 512` |

A miss on an inadmissible zone returns `false` and leaves the output untouched.

---

## 7. UAX #50 Field Mapping

UAX #50's fields do not all belong to one layer. SUVTL carries the orientation
and the baseline; SUCD carries the prose and the font-design tables.

| UAX #50 field | Owner | Form |
|---------------|-------|------|
| Vertical_Orientation value (`U`/`R`/`Tu`/`Tr`) | **SUVTL** | `orientation_flags` value bits |
| Glyph-change table membership (Tu/Tr roster) | **SUVTL** | `SUVTL_ORIENT_TRANSFORM` |
| Squared-word classification | **SUVTL** | `SUVTL_FLAG_SQUARED` |
| Quotation-mark classification | **SUVTL** | `SUVTL_FLAG_QUOTATION` |
| Per-script vertical-flow direction (Native/RNUR) | **SUVTL** | `SUVTL_FLAG_CUSTOM_FLOW` |
| Vertical baseline adjustment | **SUVTL** | `baseline_offset` |
| Glyph-change illustrations, prose notes | SUCD | text manifest; not parsed by the kernel |
| Script / block identity | SSP | `suas_ssp_*` (SUAS-005) |
| Grapheme cluster boundaries | STS | `suas_sts_*` (SUAS-006) |
| The `VerticalOrientation.txt` file itself | SUCD | the offline reference layer |

The kernel therefore never parses the glyph-change prose; a caller that needs it
reads the reference layer.

---

## 8. Data Files and the Curated Table

The normative runtime table is the compiled static array `suvtl_table[]` in
`src/suas/suas_suvtl.c`. It is the single source of truth for runtime
orientation. The offline forms are the published `VerticalOrientation.txt`
manifests in the `/Public` tree and the per-script vertical-flow definitions of
SUCD. The static table is a **curated subset** of that dataset; extending it is a
data update (a minor revision) and never changes the API.

The curated table carries the full UAX #50 **Table 3** Tu/Tr set — the characters
whose default glyph is not acceptable in vertical text and for which a font must
supply a separate vertical form — together with the Native/neography
custom-flow declarations. The Tu/Tr roster (217 curated rows) comprises:

| Block | Rows | Value | Flag |
|-------|------|-------|------|
| Quotation marks `U+2018–U+201D` | 4 | `Tr` | `QUOTATION` |
| Brackets & punctuation `U+2329–U+3030` | 27 | `Tr` | — |
| Hiragana small letters/marks `U+3041–U+309C` | 14 | `Tr` | — |
| Katakana small letters/marks `U+30A0–U+30FC` | 14 | `Tr` | — |
| Bopomofo `U+3127–U+31BB` | 6 | `Tr` | — |
| Katakana phonetic extensions `U+31F0–U+31FF` | 16 | `Tr` | — |
| Squared katakana/ideographic words `U+32FF–U+3357`, `U+337B–U+337F` | 94 | `Tu` | `SQUARED` |
| Small forms `U+FE50–U+FE5E` | 9 | `Tr` | — |
| Fullwidth forms `U+FF01–U+FFE3` | 19 | `Tr` | — |
| Kana supplement/extended `U+1B132–U+1B168` | 10 | `Tr` | — |
| Enclosed ideographic supplement `U+1F200–U+1F201` | 2 | `Tu` | `SQUARED` |
| Native/RNUR custom-flow declarations `0x00160000`, `0x00160100`, `0x00160200` | 3 | `U` | `CUSTOM_FLOW` |

`U+2018`, `U+2019`, `U+201C`, `U+201D` are tagged `QUOTATION` as well as `Tr`.
The three RNUR rows exercise the `CUSTOM_FLOW` flag, the signed baseline field
(`0x00160100` carries `+4`; `0x00160200` carries `-1`), and the zone dispatch.

---

## 9. Verification

`suas_suvtl_validate_table` is O(n²) worst case and returns the **first** defect.
It is a compiler-time check, not a query path. An **empty table is valid**: an
implementation with no curated records ships zero rows and conforms.

The check order is fixed and observable: inadmissible/out-of-space codepoint,
then reserved bits (`BAD_FLAGS`), then value well-formedness
(`BAD_ORIENTATION`), then the reserved byte (`BAD_RESERVED`), then the custom-flow
zone rule (`CUSTOM_FLOW_MISMATCH`), then ordering and uniqueness.

| Outcome | Trigger |
|---------|---------|
| `SUVTL_REGISTRY_OK` | The table is well-formed (or empty) |
| `SUVTL_REGISTRY_NULL_TABLE` | `count > 0` with a NULL pointer |
| `SUVTL_REGISTRY_COUNT_OVER` | `count` above the static capacity |
| `SUVTL_REGISTRY_UNORDERED` | A codepoint below its predecessor |
| `SUVTL_REGISTRY_DUPLICATE_CODEPOINT` | The same codepoint twice |
| `SUVTL_REGISTRY_OUT_OF_SPACE` | A codepoint above `0x7FFFFFFF` |
| `SUVTL_REGISTRY_INADMISSIBLE_CODEPOINT` | SCP, trap or Sentinel |
| `SUVTL_REGISTRY_BAD_ORIENTATION` | Bits that name no U/R/Tu/Tr value |
| `SUVTL_REGISTRY_BAD_FLAGS` | A reserved bit set |
| `SUVTL_REGISTRY_BAD_RESERVED` | A reserved byte not zero |
| `SUVTL_REGISTRY_CUSTOM_FLOW_MISMATCH` | `CUSTOM_FLOW` outside the RNUR set |

---

## 10. The SUAS-010 (SSRE) Linkage Gate

SUVTL is registered in the SSRE cross-reference registry as
`{ "SUAS-014", "SUVTL", "suas_suvtl", 1u, 0u, MANDATORY, L0 }`. A build that
compiles an SUVTL table confirms the registration through SUAS-010 before it
emits an image, exactly as SUCD confirms its schema (SUAS-011) and SISR confirms
its registry (SUAS-013) — the linkage gate is the same one described in
SUAS-012 §7.

---

## 11. Stability and Versioning

- **API stability.** The record layout, the value encodings, the masks, the
  enumerations and the function signatures are frozen at `1.0`. Adding a curated
  row is a **patch** change (no descriptor that verified before can fail
  afterwards); adding a flag bit, a zone, a group or an error outcome is a
  **minor** change; changing a layout or a mask is a **major** change.
- **Empty registry.** An implementation with no curated records registers zero
  rows and remains conformant, resolving every codepoint by the algorithmic
  default. This is the one respect in which the seeding sketch's "modular
  optional compliance" framing survives (§1.4).
- **Data updates.** The curated rows track the published reference layer; a data
  refresh is a minor revision of the table alone.

---

## 12. Conformance Requirements

An implementation conforms to SUAS-014 when:

1. it defines `suvtl_glyph_layout_t` as the 8-byte layout of §3.1;
2. it encodes U/R/Tu/Tr by the three bits of §3.2 and resolves
   `suas_suvtl_get_layout_properties` in O(1) expected time with no allocation
   (SUVTL-I1, SUVTL-I2);
3. it rejects inadmissible and out-of-space codepoints (SUVTL-I3);
4. it sets no reserved flag bit and no nonzero reserved byte (SUVTL-I4,
   SUVTL-I5);
5. it keeps its records ordered and unique by codepoint (SUVTL-I6);
6. it admits only a well-formed value, so exactly one of UPRIGHT/ROTATED is set
   (SUVTL-I7);
7. it confines `CUSTOM_FLOW` to the RNUR set (SUVTL-I8);
8. it answers every text-space codepoint, curated or not (SUVTL-I9);
9. it applies the §4 grapheme rule per cluster and does not re-derive the
   boundaries owned by SUAS-006;
10. it registers `SUAS-014` in SSRE (§10);
11. it treats an empty registry as valid (§11).

### 12.1 Compliance matrix (reference implementation)

| Requirement | Evidence |
|-------------|----------|
| Layout | `SUVTL_STATIC_ASSERT` in `suas_suvtl.c`; runtime `sizeof`/`offsetof` checks |
| Value encoding | `SUVTL_ORIENT_*` macros; `test_orientation_values` |
| O(1) lookup | Open-addressing index, `suvtl_find` |
| Text spaces | `suas_suvtl_zone_of`, `suas_suvtl_is_text_space` |
| Assigned bits | `suas_suvtl_validate_table` reserved-bit check |
| Ordering / uniqueness | `suas_suvtl_validate_table` |
| Well-formed value | `suvtl_value_well_formed` |
| Custom-flow rule | `suas_suvtl_validate_table` `CUSTOM_FLOW_MISMATCH` |
| Totality | `suas_suvtl_get_layout_properties`; `test_groups_and_default` |
| Grapheme rule | `suas_suvtl_cluster_orientation`, `suas_suvtl_is_enclosing_mark` |
| SSRE registration | `test_ssre_registration` |

---

## 13. Annex — Curated Tu/Tr Roster

The full UAX #50 Table 3 roster transcribed by §8, by codepoint. `Tu` rows carry
`SUVTL_FLAG_SQUARED`; the quotation-mark rows carry `SUVTL_FLAG_QUOTATION`.

**Quotation marks (Tr, QUOTATION).** `U+2018 U+2019 U+201C U+201D`.

**Brackets & punctuation (Tr).** `U+2329 U+232A U+3001 U+3002 U+3008 U+3009
U+300A U+300B U+300C U+300D U+300E U+300F U+3010 U+3011 U+3014 U+3015 U+3016
U+3017 U+3018 U+3019 U+301A U+301B U+301C U+301D U+301E U+301F U+3030`.

**Hiragana (Tr).** `U+3041 U+3043 U+3045 U+3047 U+3049 U+3063 U+3083 U+3085
U+3087 U+308E U+3095 U+3096 U+309B U+309C`.

**Katakana (Tr).** `U+30A0 U+30A1 U+30A3 U+30A5 U+30A7 U+30A9 U+30C3 U+30E3
U+30E5 U+30E7 U+30EE U+30F5 U+30F6 U+30FC`.

**Bopomofo (Tr).** `U+3127 U+31B4 U+31B5 U+31B6 U+31B7 U+31BB`.

**Katakana phonetic extensions (Tr).** `U+31F0–U+31FF`.

**Squared words (Tu, SQUARED).** `U+32FF–U+3357`, `U+337B–U+337F`,
`U+1F200–U+1F201`.

**Small forms (Tr).** `U+FE50 U+FE51 U+FE52 U+FE59 U+FE5A U+FE5B U+FE5C U+FE5D
U+FE5E`.

**Fullwidth forms (Tr).** `U+FF01 U+FF08 U+FF09 U+FF0C U+FF0E U+FF1A U+FF1B
U+FF1F U+FF3B U+FF3D U+FF3F U+FF5B U+FF5C U+FF5D U+FF5E U+FF5F U+FF60 U+FFE3`.

**Kana supplement / extended (Tr).** `U+1B132 U+1B150 U+1B151 U+1B152 U+1B155
U+1B164 U+1B165 U+1B166 U+1B167 U+1B168`.

**Native / RNUR custom flow (U, CUSTOM_FLOW).** `0x00160000`, `0x00160100`
(baseline `+4`), `0x00160200` (baseline `−1`).

---

## 14. Terms and Conventions

- **SUVTL** — SuperUnicode Vertical Text Layout (this standard).
- **SUCD** — SuperUnicode Character Database (SUAS-012).
- **SIP** — System Ideographic Properties (SUAS-009).
- **SSP** — System Script Property (SUAS-005).
- **STS** — System Text Segmentation (SUAS-006).
- **SSRE** — System Specification Registry & Cross-Reference Engine (SUAS-010).
- **SUCA** — SuperUnicode Collation Algorithm (SUTS-001); *not* the offline
  layer of this standard.
- **RNUR** — Reddit Neographical Unicode Registry (SUAS-005 §5.4).
- **`vo`** — UAX #50's Vertical_Orientation property; the property SUVTL
  serves at run time.

---

## 15. Testing

`tests/test_suas_suvtl.c` covers, in order: version and meta; record geometry;
zone dispatch and the RNUR carve-out; the codepoint hash; the four values and
their derived predicates; the derived-bit consistency of every curated record;
the vertical-behaviour groups and the algorithmic default; baseline
adjustments; the grapheme-cluster rule including the enclosing-mark exception;
enumeration and table self-consistency; idempotent init; every
`suas_suvtl_validate_table` outcome plus verification of the shipped table;
taxonomy names; and the SSRE registration. The unified test
`unified/test_unified.c` asserts the SUVTL header coexists with the rest of the
family and that `SUAS-014` is registered and mandatory.
