# SUAS-013 — System Ideographic Source Registry (SISR)

**Standard ID:** SUAS-013
**Status:** Draft — Ratified
**Scope:** Core Kernel Invariant
**Applies To:** SUCD, UAX #45 (U-source analog), SUAS-005 (SSP), SUAS-009 (SIP), SUAS-011 (SCMS), SUAS-012 (SUCD), SUTS-004 (SIVD), SUTR-8 (SUCPM), the SUF font engine, the OpenWindows shell, the System Control Plane
**Last Updated:** 2026-10-21

---

## 1. Overview

System Ideographic Source Registry (SISR) is the core algorithmic subsystem
governing the **zero-allocation, O(1) resolution of ideographic source
identity**: which external source record a codepoint came from, which registry
class it belongs to, and which canonical glyph it falls back to when the
preferred form is unavailable.

Where a traditional Unicode environment consults the U-source data of
UAX #45 — a text file in which an identifier such as `UTC-00932` carries a
two-or-three-letter authority prefix, a zero-padded serial, a status, an
optional source field, an IDS decomposition, radical-stroke data and a first
residual stroke count, spread across lines and comments that every
implementation renders into its own shape — SISR is a **dual-layer**
architecture:

- the **offline reference layer** is the SuperUnicode Character Database
  (SUCD) and the published `/Public` tree: the `sources/Sources.txt` provenance
  manifests, the RNUR allocation coordinates defined by SUAS-005 (SSP) §5.4,
  and the name manifests that carry external aliases;
- the **runtime kernel layer** is SISR: at build time those records are compiled
  into two immutable, byte-packed binary tables — `sisr_source_entry_t` and
  `sisr_variant_link_t` — and the kernel executes the SISR algorithms directly
  against the static arrays.

### 1.1 Architectural shifts from UAX #45

| UAX #45 (U-source data) | SISR (SUAS-013) |
|-------------------------|-----------------|
| A text data file, read and parsed at run time | Two static, byte-packed binary tables compiled at build time |
| Identity is a string (`UTC-00932`) | Identity is a 16-byte record: a 24-bit authority-local serial plus a classification bitfield; the string is a derived view |
| Status, evidence, IDS, comments are narrative text interleaved with the data | Status is a proven bitfield; IDS, radical-stroke and comments remain in the reference layer (SUCD) and are never parsed by the kernel |
| Source order is file order; a lookup is a search | Records are ordered and indexed; a lookup is a hash probe — O(1) expected |
| A registry is a submission archive | A registry is a **verified** artifact: every defect the compiler can produce has a name (§9) |

### 1.2 Goals

1. **One record, one identity.** A codepoint's source identity is a fixed
   16-byte descriptor whose every field is byte-aligned and whose every reserved
   bit is proven zero before the table is admitted.
2. **A bounded fallback.** Every variant link resolves to its canonical target
   in O(1) expected time, with no search, no allocation and no libc.
3. **A proven table.** A compiled registry is verified before it is queried; the
   verification surface names each defect rather than returning a bare failure.
4. **No duplication of SUAS-009.** SIP answers what an ideograph *is*; SISR
   answers where it *came from* and what replaces it. Where the two overlap, they
   must agree (§6.3).

### 1.3 Non-negotiable invariants

- **SISR-I1 — Zero allocation.** No SISR operation allocates. The tables are
  static; every descriptor is a fixed-size struct the caller owns.
- **SISR-I2 — Bounded lookup.** A lookup hashes the codepoint, probes a fixed
  index, and returns. The number of probes is bounded by a small constant over a
  table whose load factor is below one-half.
- **SISR-I3 — Source spaces only.** A record may name only a Bridge, RNUR or
  Native codepoint. The Control Plane, the trap slots, the Sentinel and ExtSUCS
  are never source identities (`SISR_REGISTRY_INADMISSIBLE_CODEPOINT`,
  `SISR_REGISTRY_OUT_OF_SPACE`).
- **SISR-I4 — Assigned bits only.** A `1.0` record sets no flag bit above the
  six this standard assigns (`SISR_REGISTRY_BAD_FLAGS`); a `1.0` link sets no
  bit above the eight it assigns.
- **SISR-I5 — Reserved bytes are zero.** The reserved byte of a source record
  and the reserved halfword of a link are zero (`SISR_REGISTRY_BAD_RESERVED`).
- **SISR-I6 — Ordered and unique.** Records are ascending by codepoint, unique
  by codepoint, and unique by `(authority, serial)`.
  (`SISR_REGISTRY_UNORDERED`, `SISR_REGISTRY_DUPLICATE_CODEPOINT`,
  `SISR_REGISTRY_DUPLICATE_SOURCE_ID`).
- **SISR-I7 — Hashes are exact.** A record's `source_id_hash` equals the FNV-1a
  hash of its canonical token (`SISR_REGISTRY_HASH_MISMATCH`).
- **SISR-I8 — Links are classified and non-reflexive.** Every link sets at least
  one flag bit and does not point at its own source codepoint
  (`SISR_REGISTRY_BAD_LINKAGE`).
- **SISR-I9 — Agreement with SIP.** Where SIP and SISR both name a fallback for
  a codepoint, the targets are identical (§6.3).

### 1.4 Deviations from the reference sketch

1. **Classification.** The seeding sketch labelled SISR **SUTS-006**, on the
   argument that the registry is modular optional compliance. The series owner
   directed that it be authored as a **SUAS** standard, so it is registered as
   **SUAS-013**, the number immediately after SUAS-012 (SUCD), which keeps the
   SUAS series contiguous with no reserved gaps (§11.1 of SUAS-010). The one
   element of the SUTS framing that survives is deliberate: the **registry
   contents are optional version data**. The engine, the record layout, the
   lookup contract and the verification rules are mandatory architecture; an
   implementation with no ideographic sources ships an **empty registry** and
   remains conformant (§11). An empty table is valid, not an error.
2. **Numbering correction.** The sketch's own body cited SUCD as "SUAS-014".
   That is stale: SUCD is **SUAS-012**. SISR is SUAS-013. SUAS-014 remains
   unallocated.
3. **Macro prefix.** `SISR_SOURCE_*`, `SISR_LINK_*` and the `SUAS_SISR_ZONE_*`
   / `SUAS_SISR_LIMIT_*` constants are used for the enumerations and masks;
   spec-level constants (`SUAS_SISR_SPEC_ID`, `SUAS_SISR_REGISTRY_CAPACITY`,
   `SUAS_SISR_INDEX_SIZE`, …) carry the `SUAS_SISR_*` prefix. Functions use
   `suas_sisr_*`, and the public record types keep the sketch's own names.
4. **An added field.** The sketch supplied `codepoint`, `source_id_hash`,
   `registry_flags`, `source_type` and `reserved`. SISR **adds
   `source_serial`**, because a 32-bit hash is an integrity check, not an
   identity. The serial is the normative half of the identity; the hash lets a
   caller compare a full external token without carrying it.
5. **A hash is not the identity.** `source_id_hash` alone cannot distinguish two
   colliding tokens, so it is never the sole key: the `(authority, serial)` pair
   is the key, and uniqueness is verified over that pair.

---

## 2. Definitions and Notation

### 2.1 Definitions

- **Source identity.** The external record that a SuperUnicode codepoint was
  adopted from, named by an authority (the identifier prefix) and an
  authority-local serial.
- **Authority.** Who issued the identity. The closed enumeration
  `suas_sisr_source_type_t`: `UTC`, `UCI`, `UK`, `RNUR`, `CONLANG`, `CUSTOM`.
- **Canonical token.** The derived textual form of an identity:
  `PREFIX-`, then at least five zero-padded decimal digits, widening only for
  serials above 99999 (`UTC-00932`, `RNUR-00256`, `RNUR-100000`). The token is a
  *view*, never a stored key.
- **Registry class.** The `registry_flags` bitfield: which RNUR set, whether the
  record is conlang-origin or historical, whether its evidence is verified, and
  whether it is a superseded duplicate.
- **Fallback.** The canonical glyph a source form renders as when the preferred
  form is unavailable.
- **RNUR set number.** For a neography coordinate,
  `set = (cp − 0x00160000) >> 8` (SUAS-005 §5.4) — an O(1) decode.

### 2.2 Notational conventions

`U+` names a Bridge-plane Unicode codepoint; a bare hexadecimal value names a
Base SUCS codepoint. `cp` is a 31-bit Base SUCS value
(`0x00000000–0x7FFFFFFF`). A value above `SUAS_CODEPOINT_MAX` is ExtSUCS and is
outside the registry.

### 2.3 The address-space zones

| Zone | Range | Source identity? |
|------|-------|------------------|
| `SUAS_SISR_ZONE_BRIDGE` | `0x00000000–0x0010FFFF` | yes |
| `SUAS_SISR_ZONE_SCP` | `0x00110000–0x0011FFFF` | no — Control Plane |
| `SUAS_SISR_ZONE_NATIVE` | rest of `0x00120000–0x7FFFFFEF` | yes — custom conscript space |
| `SUAS_SISR_ZONE_RNUR` | `0x00160000–0x0016FFFF` | yes — neography |
| `SUAS_SISR_ZONE_TRAP` | `0x7FFFFFF0–0x7FFFFFFE` | no |
| `SUAS_SISR_ZONE_SENTINEL` | `0x7FFFFFFF` | no |
| `SUAS_SISR_ZONE_EXTSUCS` | `> 0x7FFFFFFF` | no — plugin space |

`suas_sisr_is_source_space` is true for Bridge, RNUR and Native only.

---

## 3. The Record and the Index

### 3.1 The source record

```c
typedef struct {
    uint32_t codepoint;      /* Base SUCS codepoint                   */
    uint32_t source_serial;  /* authority-local serial, 1..0xFFFFFF   */
    uint32_t source_id_hash; /* FNV-1a of the canonical token (§3.2)  */
    uint16_t registry_flags; /* SISR_SOURCE_* bitfield                */
    uint8_t  source_type;    /* suas_sisr_source_type_t               */
    uint8_t  reserved;       /* must be zero                           */
} sisr_source_entry_t;
```

The record is 16 bytes on every supported ABI, with no padding. Its field
offsets are `0, 4, 8, 12, 14, 15`.

### 3.2 Canonical tokens and the hash

The identity is `(source_type, source_serial)`. The token is derived:

```
PREFIX = prefix(source_type)         /* "UTC", "UCI", "UK", "RNUR", "CONL", "CUST" */
WIDTH  = max(5, decimal_digits(source_serial))
TOKEN  = PREFIX + "-" + zero_padded(source_serial, WIDTH)
```

`suas_sisr_format_source_id` writes the token into a caller buffer (never a
static, because the kernel writes no mutable global string state), and
`suas_sisr_source_id_matches` compares a caller's token against the derived
form. `source_id_hash = FNV-1a(TOKEN)` and `suas_sisr_hash_source_token`
reproduces it. A NULL token or a zero length yields `0`, which no canonical
token produces.

### 3.3 The variant link

```c
typedef struct {
    uint32_t codepoint;          /* the source form                    */
    uint32_t fallback_codepoint; /* the canonical glyph to render      */
    uint16_t link_flags;         /* SISR_LINK_* bitfield               */
    uint16_t reserved;           /* must be zero                       */
} sisr_variant_link_t;
```

The link is 12 bytes, offsets `0, 4, 8, 10`. A link table is one-to-one on
`codepoint` but many-to-one on `fallback_codepoint`.

### 3.4 The registry flags

| Bit | Name | Meaning |
|-----|------|---------|
| `0x0001` | `SISR_SOURCE_RNUR_SET1` | RNUR ideographic set 1 |
| `0x0002` | `SISR_SOURCE_RNUR_SET2` | RNUR ideographic set 2 |
| `0x0004` | `SISR_SOURCE_CONLANG` | conlang-origin record |
| `0x0008` | `SISR_SOURCE_HISTORICAL` | historical / obsolete source |
| `0x0010` | `SISR_SOURCE_VERIFIED` | evidence meets the current bar |
| `0x0020` | `SISR_SOURCE_DEPRECATED` | superseded duplicate record |

`SISR_SOURCE_SET_MASK` is `0x0003`; the two set bits are mutually exclusive.
`SISR_SOURCE_ASSIGNED_MASK` is `0x003F`; the upper ten bits are reserved.

### 3.5 The link flags

| Bit | Name | Meaning |
|-----|------|---------|
| `0x0001` | `SISR_LINK_SIMPLIFIED` | simplified form |
| `0x0002` | `SISR_LINK_TRADITIONAL` | traditional form |
| `0x0004` | `SISR_LINK_Z_VARIANT` | z-variant |
| `0x0008` | `SISR_LINK_COMPATIBILITY` | compatibility ideograph |
| `0x0010` | `SISR_LINK_REORDERED` | component-reordered variant |
| `0x0020` | `SISR_LINK_APPROXIMATE` | approximate IDS match |
| `0x0040` | `SISR_LINK_REGISTRY` | registry fallback to the Native plane |
| `0x0080` | `SISR_LINK_CUSTOM` | implementation-defined link |

`SISR_LINK_ASSIGNED_MASK` is `0x00FF`; the upper eight bits are reserved. A link
whose flags are zero is rejected.

### 3.6 The index

The static source table holds at most `SUAS_SISR_REGISTRY_CAPACITY` (256)
records; the static link table holds at most `SUAS_SISR_LINK_CAPACITY` (64)
links. Two fixed open-addressing arrays — `SUAS_SISR_INDEX_SIZE` (512) and
`SUAS_SISR_LINK_INDEX_SIZE` (128) slots — are the indexes:

1. `suas_sisr_init_registry` hashes each record's codepoint (FNV-1a over the
   four little-endian bytes) and places its offset in the first free slot,
   resolving collisions by linear probing. The build is O(n) once and touches no
   heap.
2. `suas_sisr_get_source_entry` (and every other query) hashes the query
   codepoint and probes the same sequence until it matches or hits an empty
   slot.

Because the load factor is below one-half over a bounded, static table, the
probe count is bounded by a small constant. Queries build the indexes lazily on
first use, so they are correct even if `suas_sisr_init_registry` was not called.

---

## 4. The Query Surface

| Function | Cost | Behaviour |
|----------|------|-----------|
| `suas_sisr_init_registry` | O(n) once | Idempotent, allocation-free |
| `suas_sisr_get_source_entry` | O(1) expected | Copies the record; false on a miss or an inadmissible zone |
| `suas_sisr_is_registered` | O(1) expected | Presence test |
| `suas_sisr_entry_count` / `entry_at` | O(1) | Enumeration in ascending codepoint order |
| `suas_sisr_get_variant_link` | O(1) expected | Copies the link |
| `suas_sisr_fallback_target` | O(1) expected | The target only |
| `suas_sisr_link_count` / `link_at` | O(1) | Link enumeration |
| `suas_sisr_hash_codepoint` | O(1) | The index key |
| `suas_sisr_hash_source_token` | O(len) | Token hash, `len ≤ 24` |
| `suas_sisr_format_source_id` | O(1) | Bounded token writer |
| `suas_sisr_zone_of` / `is_source_space` / `is_neography` | O(1) | Range dispatch |
| `suas_sisr_neography_set` | O(1) | RNUR set decode |
| `suas_sisr_validate_table` / `validate_links` | O(n²) worst case | Compiler-time verifier, `n ≤ 256` |

A miss returns `false` and leaves the output untouched. A NULL output returns
`false`.

---

## 5. Range Dispatch and RNUR

`suas_sisr_zone_of` tests the ExtSUCS bound first, then the Bridge ceiling, then
the SCP, trap and Sentinel ranges, then the RNUR carve-out, and defaults to
Native. `suas_sisr_neography_set` returns
`(cp − 0x00160000) >> 8` and fails outside the carve-out. This is the same
decode SUAS-005 §5.4 publishes, so a SISR source identity in RNUR set 1 is
exactly an SSP coordinate in set 1.

---

## 6. Variant and Fallback Linkage

### 6.1 Why a separate table

Source identity and glyph preference are different questions. A codepoint can
have a source identity and no fallback (it is canonical); it can have a fallback
that is itself a source form (a chain); and several codepoints can share one
fallback. Keeping links in their own 12-byte table keeps the 16-byte source
record free of a field most records would leave zero.

### 6.2 Link kinds

`SISR_LINK_REGISTRY` marks a fallback into the Native & Neography Plane;
`SISR_LINK_COMPATIBILITY` marks a compatibility-decomposition-like fold;
`SISR_LINK_SIMPLIFIED` / `SISR_LINK_TRADITIONAL` / `SISR_LINK_Z_VARIANT` /
`SISR_LINK_REORDERED` / `SISR_LINK_APPROXIMATE` mirror the U-source variant
vocabulary; `SISR_LINK_CUSTOM` is the escape hatch. `suas_sisr_link_name`
returns the name of the lowest assigned bit, so a display layer has a stable
label without a switch of its own.

### 6.3 Agreement with SUAS-009 (SIP)

SIP publishes a single `native_variant` per curated ideograph. SISR is the
registry view of the same fact. Where a SISR link's source codepoint is one SIP
also models, `suas_sip_variant_target` and `suas_sisr_fallback_target` must
return the same target. `tests/test_suas_sisr.c` walks the whole link table and
asserts the overlap; the invariant is **SISR-I9**.

---

## 7. UAX #45 Field Mapping

The U-source fields do not all belong to one layer. SISR carries identity and
status; SIP carries the ideographic metrics; SUCD carries the text.

| UAX #45 field | Owner | Form |
|---------------|-------|------|
| Identifier prefix (`UTC`, `UCI`, `UK`) | **SISR** | `source_type` |
| Serial | **SISR** | `source_serial` (24-bit); the token widens for large values |
| Status (compressed, compatibility, etc.) | **SISR** | `registry_flags` |
| Evidence / verification | **SISR** | `SISR_SOURCE_VERIFIED` |
| Deprecated duplicate | **SISR** | `SISR_SOURCE_DEPRECATED` |
| Variant of / fallback | **SISR** | `sisr_variant_link_t` |
| Radical-stroke | SIP | `suas_sip_get_radical_stroke` |
| Classification (unified, compat, …) | SIP | `suas_sip_flags` |
| Collation seed | SIP | `suas_sip_get_sort_weight` |
| IDS decomposition | SUCD | text manifest; not parsed by the kernel |
| First residual stroke count | SUCD | text manifest; not parsed by the kernel |
| Source field, comments | SUCD | text manifest; not parsed by the kernel |

The kernel therefore never parses a decomposition or a comment; a caller that
needs one reads the reference layer.

---

## 8. Data Files and the Curated Table

The normative runtime table is the compiled static array `sisr_table[]` in
`src/suas/suas_sisr.c`; the link table is `sisr_links[]`. They are the single
source of truth for runtime identity. The offline forms are the published
`/Public/<version>/sources/Sources.txt` provenance manifests
(`<range>;<author>;<date>`), the RNUR coordinates of SUAS-005 §5.4, and the name
manifests that carry external aliases. The static table is a **curated subset**
of that dataset; extending it is a data update (a minor revision) and never
changes the API.

The curated rows are illustrative registry allocations (`UTC`, `UCI`, `UK`,
`RNUR`, `CONLANG`, `CUSTOM`) chosen to exercise every zone, every authority and
every link kind. The `UTC-00932 → U+FA26` compatibility example is the one
normative UAX #45 citation the standard keeps; the `RNUR` rows are the
SuperUnicode allocations the RNUR carve-out decodes.

---

## 9. Verification

`suas_sisr_validate_table` is O(n²) worst case and returns the **first** defect.
It is a compiler-time check, not a query path. An **empty table is valid**: an
implementation with no ideographic sources ships zero records and conforms.

| Outcome | Trigger |
|---------|---------|
| `SISR_REGISTRY_OK` | The table is well-formed (or empty) |
| `SISR_REGISTRY_NULL_TABLE` | `count > 0` with a NULL pointer |
| `SISR_REGISTRY_COUNT_OVER` | `count` above the static capacity |
| `SISR_REGISTRY_UNORDERED` | A codepoint below its predecessor |
| `SISR_REGISTRY_DUPLICATE_CODEPOINT` | The same codepoint twice |
| `SISR_REGISTRY_OUT_OF_SPACE` | A codepoint above `0x7FFFFFFF` |
| `SISR_REGISTRY_INADMISSIBLE_CODEPOINT` | SCP, trap or Sentinel |
| `SISR_REGISTRY_BAD_SOURCE_ID` | Serial zero or wider than 24 bits |
| `SISR_REGISTRY_BAD_SOURCE_TYPE` | Authority outside the enumeration |
| `SISR_REGISTRY_BAD_FLAGS` | A reserved bit, or both RNUR set bits |
| `SISR_REGISTRY_BAD_RESERVED` | A reserved byte not zero |
| `SISR_REGISTRY_DUPLICATE_SOURCE_ID` | The same `(authority, serial)` |
| `SISR_REGISTRY_HASH_MISMATCH` | `source_id_hash` ≠ canonical token hash |
| `SISR_REGISTRY_BAD_LINKAGE` | A self-link or an unflagged link |

`suas_sisr_validate_links` applies the same contract to the link table:
NULL/empty, over-count, out-of-space, an inadmissible source or target, a
reserved byte, a reserved or empty flag set, and a self-link.

---

## 10. The SUAS-010 (SSRE) Linkage Gate

SISR is registered in the SSRE cross-reference registry as
`{ "SUAS-013", "SISR", "suas_sisr", 1u, 0u, MANDATORY, L0 }`. A build that
compiles a SISR table confirms the registration through SUAS-010 before it
emits an image, exactly as SUCD confirms its schema (SUAS-011) — the linkage gate
is the same one described in SUAS-012 §7.

---

## 11. Stability and Versioning

- **API stability.** The record layouts, the enumerations, the masks and the
  function signatures are frozen at `1.0`. Adding a registry or link row is a
  **patch** change (no descriptor that verified before can fail afterwards);
  adding a flag bit, an authority or an error outcome is a **minor** change;
  changing a layout or a mask is a **major** change.
- **Empty registry.** An implementation with no ideographic sources registers
  zero records and zero links and remains conformant. This is the one respect in
  which the seeding sketch's "modular optional compliance" framing survives
  (§1.4).
- **Data updates.** The curated rows track the published reference layer; a data
  refresh is a minor revision of the table alone.

---

## 12. Conformance Requirements

An implementation conforms to SUAS-013 when:

1. it defines `sisr_source_entry_t` and `sisr_variant_link_t` as the 16-byte and
   12-byte layouts of §3.1 and §3.3;
2. `suas_sisr_get_source_entry` and `suas_sisr_get_variant_link` resolve in
   O(1) expected time with no allocation (SISR-I1, SISR-I2);
3. it rejects inadmissible and out-of-space codepoints (SISR-I3);
4. it sets no reserved flag bit and no nonzero reserved byte (SISR-I4, SISR-I5);
5. it keeps its records ordered and unique by codepoint and by
   `(authority, serial)` (SISR-I6);
6. it derives `source_id_hash` from the canonical token (SISR-I7);
7. it rejects a self-link and an unflagged link (SISR-I8);
8. its fallbacks agree with SIP where both are defined (SISR-I9);
9. it registers `SUAS-013` in SSRE (§10);
10. it treats an empty registry as valid (§11).

### 12.1 Compliance matrix (reference implementation)

| Requirement | Evidence |
|-------------|----------|
| Layouts | `SISR_STATIC_ASSERT` in `suas_sisr.c`; runtime `sizeof`/`offsetof` checks |
| O(1) lookup | Open-addressing index, `sisr_find` |
| Source spaces | `suas_sisr_zone_of`, `suas_sisr_is_source_space` |
| Assigned bits | `sisr_check_flags` |
| Ordering / uniqueness | `suas_sisr_validate_table` |
| Hash exactness | `suas_sisr_validate_table` recomputes each token hash |
| Link rules | `suas_sisr_validate_links` |
| SIP agreement | `test_sip_agreement` |
| SSRE registration | `test_ssre_registration` |

---

## 13. Terms and Conventions

- **SISR** — System Ideographic Source Registry (this standard).
- **SIP** — System Ideographic Properties (SUAS-009).
- **SUCD** — SuperUnicode Character Database (SUAS-012).
- **SSP** — System Script Property (SUAS-005).
- **SSRE** — System Specification Registry & Cross-Reference Engine (SUAS-010).
- **RNUR** — Reddit Neographical Unicode Registry (SUAS-005 §5.4).
- **Authority** — the issuing prefix of a source identity.
- **Canonical token** — the derived textual form `PREFIX-NNNNN` of an identity.
- **Fallback** — the canonical glyph a source form renders as.

---

## 14. Testing

`tests/test_suas_sisr.c` covers, in order: version and meta; record geometry;
zone dispatch and the RNUR set decode; the codepoint and token hashes; canonical
token formatting (including the widened serial and the too-small buffer);
taxonomy names; the O(1) source query on the lazy-build path; idempotent init;
variant linkage; the SIP agreement invariant; enumeration and table
self-consistency; every `suas_sisr_validate_table` outcome; every
`suas_sisr_validate_links` outcome plus verification of the shipped tables; and
the SSRE registration. The unified test `unified/test_unified.c` asserts the
SISR header coexists with the rest of the family and that `SUAS-013` is
registered and mandatory.
