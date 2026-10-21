# SUAS-012 — SuperUnicode Character Database (SUCD)

**Status:** Draft — Authoring
**Version:** 1.0.0
**Family:** SUAS — SuperUnicode Architecture Standards (Core Kernel Invariants)
**Applies To:** SUTR-0 (SUCS Core), SUTR-8 (SUCPM), SUTR-10 (SUCM), SUAS-001 (SDF), SUAS-005 (SSP), SUAS-010 (SSRE), SUAS-011 (SCMS), SUTS-001 (SUCA), SUTS-002 (SRE), SUTS-005 (SCSS)
**Reference Layer:** `/Public/<version>/sucd/`, `/Public/<version>/sudat/`

## 1. Overview

UAX #44 describes the Unicode Character Database. It does so, deliberately, as a
*collection of data files*: `UnicodeData.txt` for one set of fields, `Blocks.txt`
for another, `PropList.txt` for a third, `DerivedCoreProperties.txt` for a
fourth, and so on, each with its own column layout, its own range syntax, and its
own defaults. That framing is accurate. It is also the problem.

A database defined as a folder of text files has no architecture. It has no
single statement of what a record is. It offers no guarantee that two files agree
about the same codepoint. It declares no relationship between the human-readable
form and the machine form, so every implementation that wants a runtime table
writes its own compiler, its own reconciliation rules, and its own bugs. And it
bounds no query: nothing in a folder of text says what a lookup may cost. An
implementer reconciles `UnicodeData.txt` against `Blocks.txt` against
`DerivedCoreProperties.txt` by hand, and every reconciliation is a chance to
diverge from every other implementation.

SUCD keeps the data and closes the architecture with three commitments UAX #44
never makes:

1. **One binary image.** The offline text manifests and the runtime table are two
   forms of one artefact, related by a specified compiler step — not two things a
   developer keeps in sync by discipline.
2. **A bounded lookup.** A query is one directory load, at most one index load,
   one record load, and one bounds compare. O(1), no allocation, no search, no
   libc.
3. **A partition.** The compiled records are ordered and disjoint. The published
   files are free to overlap and nest; an image may not. The compiler resolves
   them, proves the result, and sets a flag that says it did.

SUCD is the structural counterpart of UAX #44 in the same sense that SUAS-009
(SIP) is the structural counterpart of UAX #38, SUAS-010 (SSRE) of UAX #41, and
SUAS-011 (SCMS) of UAX #42: the concept is retained, the document format is not.

### 1.1 Architectural shifts from UAX #44

| Aspect | UAX #44 | SUCD |
|--------|---------|------|
| Definition | A folder of text files | One verified binary image, compiled from the files |
| Record shape | Per-file columns | One 16-byte range record, uniform across properties |
| Address space | 17 planes of 65 536, 21 bits | 31 bits, `0x00000000–0x7FFFFFFF` |
| Lookup | Read the file, or build your own map | Two-level directory, O(1) |
| Consistency | Left to the implementer | A partition, proven by the compiler |
| Property encoding | Property names and `@missing` lines | A 17-bit mask per record |
| Defaults | Per-file `@missing` conventions | One default record, `Cn`/no properties |
| Overlap | Allowed and common | Rejected by the verifier |
| Machine form | None normative | The image *is* the machine form |
| Failure mode | A file disagrees with another | The verifier names the first defect |
| Control plane | Not a Unicode concept | A first-class plane the image models |

### 1.2 Goals

1. **One record type.** Every property, block and classification compiles into
   the same range record, so there is one thing for a kernel to understand.
2. **A bounded lookup.** The cost of a query must not depend on the size of the
   database, the number of ranges, or the codepoint.
3. **A proven partition.** The compiler resolves the files' overlaps and proves
   the records are ordered and disjoint before it writes an image.
4. **Zero allocation.** The image is caller-mapped and read-only; the engine
   holds four pointers into it and nothing else.
5. **Faithful names.** The property spellings are the published files' own. A
   compiler matches `Unicode_Bridged`; an API returns `Unicode_Bridged`.
6. **Permissive where the architecture is descriptive, strict where it
   allocates.** SUCD models the control plane, the traps and the Sentinel.
   SCMS, which *allocates*, refuses to name them. Both are correct.
7. **A named failure for every defect.** A loader that rejects an image must be
   able to say which invariant failed, because a verifier that says only "bad
   image" cannot be acted on.

### 1.3 Non-negotiable invariants

- **SUCD-I1 — Verify before read.** No field of an image may be read before the
  geometry and magic have been verified. Verification is a precondition of
  binding, not a courtesy.
- **SUCD-I2 — Bounded lookup.** A query performs at most two indexed loads into
  the image beyond the header, and one bounds compare. No loop runs over
  records, pages, or codepoints.
- **SUCD-I3 — Ordered, disjoint records.** Records are sorted by `first` and
  satisfy `record[i].first > record[i-1].last`. An image that violates this is
  rejected (`SUCD_IMAGE_UNORDERED_RECORDS` / `SUCD_IMAGE_OVERLAPPING_RECORDS`).
- **SUCD-I4 — Size implies layout.** The header's `total_records`, the fixed
  directory size, and the caller-supplied image size together determine the
  dense-index extent exactly. An image whose size admits no exact layout is
  rejected (`SUCD_IMAGE_SIZE_MISMATCH`).
- **SUCD-I5 — No allocation.** No operation in this standard allocates, and no
  operation retains a pointer beyond the call or the caller's mapping lifetime.
- **SUCD-I6 — Assigned bits only.** A `0.1.0` image sets no property bit above
  the seventeen this standard assigns; the upper fifteen are reserved
  (`SUCD_IMAGE_RESERVED_PROPERTY_BIT`).
- **SUCD-I7 — One cursor contract.** Every record sets exactly one of
  `SUCD_RECORD_PRINTABLE` and `SUCD_RECORD_CONTROL` and no other flag bit
  (`SUCD_IMAGE_BAD_RECORD_FLAG`).
- **SUCD-I8 — Linkage before compilation.** An image may not be compiled until
  the schema that produced it — normally SUAS-011 (SCMS) — is confirmed
  registered in the SSRE cross-reference registry (§7).

### 1.4 Deviations from the reference sketch

1. **Numbering.** The seeding sketch was labelled **SUAS-014**. It is registered
   as **SUAS-012**, the number immediately after SUAS-011 (SCMS), which keeps the
   SUAS series contiguous with no reserved gaps (§11.1 of SUAS-010). SUAS-014 is
   not used by this specification, and neither is SUAS-013.
2. **Macro prefix.** `SUCD_PROP_*`, `SUCD_PLANE_*`, `SUCD_GC_*`, `SUCD_IMAGE_*`
   and `SUCD_RANGE_FORM_*` are used for the enumerations and masks. Spec-level
   constants (`SUAS_SUCD_SPEC_ID`, `SUAS_SUCD_PAGE_SHIFT`,
   `SUAS_SUCD_DIRECTORY_SIZE`, `SUAS_SUCD_MAGIC`, …) carry the `SUAS_SUCD_*`
   prefix, matching the family. Function names use `suas_sucd_*`.
3. **Page geometry.** The sketch proposed a dense per-codepoint table behind a
   single directory load. That cannot describe the published data: the Native
   allocation is *one range* spanning `0x00120000–0x7FFFFFEF` — some 2.1 billion
   codepoints, which a dense table would cost 4 GiB to store — while the ASCII
   page carries a dozen distinct bidirectional classifications that a uniform
   page cannot distinguish. The directory is therefore **three-kinded**: a whole
   page may be unassigned, covered by one record, or indexed per codepoint
   (§5.3). This preserves the O(1) bound and makes both the Native plane and
   ASCII representable.
4. **Record shape.** The sketch stored `codepoint` per record, one row per
   codepoint. SUCD stores `first`/`last` and range-compresses, because every file
   in the published tree is range-compressed and a point record would cost one
   row per codepoint for data that describes a few dozen spans.
5. **Property taxonomy.** The sketch listed a broader mask. SUCD assigns exactly
   the seventeen properties the published `0.1.0` files actually name, and
   reserves the rest, so an unmasked bit is a detectable defect rather than an
   accident.
6. **Additive surface.** `sucd_image_error_t` and its seventeen outcomes, the
   range decoder (`suas_sucd_range_form`, `suas_sucd_parse_range`), the partition
   proof (`suas_sucd_is_partition`, `suas_sucd_records_overlap`), the taxonomy
   lookups, and the SSRE linkage gate were added. The sketch declared a lookup
   API but no way to verify an image, decode a file, or prove a partition.
7. **Legacy BiDi surface.** SUCD was, before this standard, an eleven-line header
   serving SUAS-001 (SDF) with a six-bit BiDi mask. That surface
   (`SUCD_BIDI_*`, `suas_sucd_bidi`, `suas_sucd_is_unicode_bridge`,
   `suas_sucd_is_scp`, `suas_sucd_is_native`) is retained verbatim in §14,
   because SUAS-001 §6 and SUTR-8 both name its bits and SDF compares them for
   equality.

The sketch named the reference layer "SUCA". SUCA is SUTS-001, the SuperUnicode
Collation Algorithm. The character database is **SUCD** — the fifth time this
collision has been resolved in favour of SUCD, consistently with SUAS-009,
SUAS-010, SUAS-011, and SUTS-005.

## 2. Definitions and Notation

**Character database.**
: The property and classification data for the Base SUCS space, published as the
  `/Public/<version>/sucd/` tree and, when compiled, as one binary image.

**Image.**
: A caller-owned, read-only block of bytes laid out exactly as §5.2 specifies.

**Page.**
: A 65 536-codepoint window, selected by `codepoint >> 16`. There are exactly
  32 768 pages, exhausting the 31-bit space.

**Directory.**
: The 32 768-entry array, one `uint32_t` per page, that begins every image.

**Dense index.**
: A page-sized array of `uint16_t` record indices, present only for pages whose
  codepoints do not share one classification.

**Record.**
: A 16-byte `sucd_record_t`: an inclusive `first`/`last` range, a property mask,
  a general category, a script identifier, and cursor flags.

**Default record.**
: The record a codepoint outside every assigned range receives: general category
  `Cn`, no properties, the UNKNOWN script, and the `PRINTABLE` cursor contract.

**Partition.**
: A record set that is ordered and pairwise disjoint. Gaps are permitted, because
  an unassigned codepoint inside a covered span is still unassigned; overlap is
  not.

**Simple property.**
: A property whose assignment is stated directly by a file, one range at a time —
  here, everything in `sucd/Props/PropList.txt`.

**Derived property.**
: A property computed as a closure or union over other properties, published in
  `sucd/Derived/DerivedCoreProperties.txt`.

**Contributory property.**
: A property that contributes to a derived property but is not itself exposed for
  query. The `0.1.0` tree publishes none; the taxonomy is defined so one can be
  added without changing the record.

**`@missing`.**
: UAX #44's per-file default declaration. SUCD replaces the mechanism with the
  default record: a codepoint with no covering range needs no per-file
  declaration, because the absence is represented once, in the directory.

**Property status.**
: UAX #44's five lifecycle values — *stabilized*, *deprecated*, *obsolete*,
  *provisional*, and *deprecated in favour of* another property. SUCD carries no
  status field: a property that is removed from the files stops being assigned a
  bit, and a bit that is retired is not reused, exactly as a registry identifier
  is not reused (§11).

### 2.1 Property classification in SUCD

UAX #44's three-tier taxonomy is retained as the compiler's contract:

| Tier | Source file | SUCD treatment |
|------|-------------|----------------|
| Simple | `sucd/Props/PropList.txt` | Assigned a mask bit directly |
| Derived | `sucd/Derived/DerivedCoreProperties.txt` | Assigned a mask bit after closure |
| Contributory | *(none in 0.1.0)* | Reserved; no bit assigned |
| Bidirectional | `sucd/Props/BidiProps.txt` | Assigned a mask bit, override-resolved |

General category, block membership and the cursor contract are not "properties"
in the UAX #44 sense: they get dedicated record fields rather than mask bits,
because they are single-valued and a caller expects to read them without masking.

## 3. The Published Data Files

### 3.1 The document tree

| Path | Columns | Role |
|------|---------|------|
| `sucd/SUCSData.txt` | `codepoint;name;type` | Hierarchical codepoint inventory; `type` is the cursor contract |
| `sucd/Blocks.txt` | `AAAA..BBBB;Block Name` | Named blocks, including the control plane and traps |
| `sucd/Props/PropList.txt` | `AAAA..BBBB;Property` | Simple properties |
| `sucd/Props/BidiProps.txt` | `AAAA[-BBBB];Mask` | Bidirectional masks, with `#` comments |
| `sucd/Derived/DerivedCoreProperties.txt` | `AAAA..BBBB;Property` | Derived properties |
| `sucd/auxiliary/ReadMe.txt` | prose | Provenance of the auxiliary files |
| `sudat/assignments.txt` | `codepoint;…` | Named-assignment index |
| `sudat/control/SCP.txt` | `AAAA-AAAA;…` | System Control Plane sub-blocks |
| `sudat/control/Traps.txt` | `AAAA-AAAA;…` | Trap slots, each marked unassigned |
| `sudat/control/Sentinel.txt` | `AAAA;…` | The Sentinel and its allocation prohibition |
| `sudat/bancodes/{BAN,WARN,COM,SOFT}code.txt` | `…` | Per-severity diagnostic payloads |

The sibling `names/`, `mappings/`, `collation/`, `partitions/`, `sources/`,
`charts/` and `sutf/` trees are cited by the database but are not themselves SUCD
records; they are named here so a compiler knows what it may ignore.

### 3.2 Four range syntaxes coexist

The published tree does not agree with itself about how a range is spelled. A
compiler that reads only one convention silently drops most of the database.

| Syntax | Example | Files |
|--------|---------|-------|
| UCD double-dot, no padding | `0000..0010FFFF` | `Blocks.txt`, `PropList.txt`, `DerivedCoreProperties.txt` |
| Eight-hex hyphen | `00000000-0010FFFF` | `SUCSData.txt`, `sudat/**` |
| Four-hex hyphen | `0021-0027` | `BidiProps.txt` |
| Bare singleton | `0028` | `BidiProps.txt` |

`suas_sucd_range_form` reports which convention a line uses and
`suas_sucd_parse_range` decodes all four into an inclusive `(first, last)` pair,
accepting one to eight significant hex digits. The parser rejects a reversed
pair, a ninth hex digit, a value past `0x7FFFFFFF`, and a missing right operand.

### 3.3 Overlap and nesting are real defects in the data

The published `0.1.0` files do **not** form a partition, and the standard says so
plainly rather than pretending otherwise. Five distinct patterns occur:

1. **`SUCSData.txt` nests.** `00120000-7FFFFFFF;NATIVE ALLOCATION SPACE`
   swallows `7FFFFFF0-7FFFFFFE;TRAP MACHINE INSTRUCTIONS` and the Sentinel at
   `7FFFFFFF`. It is a hierarchical inventory, not a partition.
2. **`Blocks.txt` nests the same way.** `00120000..7FFFFFFF;Native` contains the
   `Traps` and `Sentinel` blocks, and `00110000..0011FFFF;SuperUnicode Control
   Plane` contains `BANcode`, `WARNcode`, `COMcode` and `SOFTcode`.
3. **`PropList.txt` overlays.** `00110000..0011FFFF;Control_Plane` contains the
   four `Diagnostic_*` ranges — by design, since a diagnostic block *is* in the
   control plane.
4. **`BidiProps.txt` overlaps and duplicates.** `0660-0669;NEUTRAL` sits inside
   `0600-06FF;ARABIC_AL`; `2018` sits inside `2000-206F;NEUTRAL`; and
   `0041-005A;LTR` appears twice. It is an override list, not a partition.
5. **`sudat/control/Traps.txt` contradicts `SUCSData.txt`.** The traps file marks
   every trap codepoint `UNASSIGNED`, while the enclosing Native row calls the
   same codepoints printable allocations.

None of this prevents SUCD from producing a partition; it means the **compiler**
must resolve it. SUCD requires the resolution to be deterministic and the result
to be ordered and disjoint (I3). The reference compiler applies two rules:

- **Narrower wins.** When one range contains another, the contained range
  determines the field it describes. This resolves (1), (2), (3) and (5).
- **Explicit override wins.** For `BidiProps.txt`, a line's mask overrides the
  enclosing mask for the codepoints it names, and a later line overrides an
  earlier one. This resolves (4).

A compiler MAY choose other deterministic rules; it MUST NOT emit an overlapping
image, and a verifier rejects one that is emitted anyway.

## 4. The Character Record

### 4.1 Layout

| Offset | Size | Field | Meaning |
|--------|------|-------|---------|
| 0 | 4 | `first` | Inclusive range start |
| 4 | 4 | `last` | Inclusive range end |
| 8 | 4 | `property_mask` | `SUCD_PROP_*` bits |
| 12 | 2 | `general_category` | `sucd_general_category_t` |
| 14 | 1 | `script_id` | SUAS-005 (SSP) identifier |
| 15 | 1 | `flags` | `SUCD_RECORD_*` |

The struct is exactly 16 bytes and 4-aligned. There is no `codepoint` field: a
point is a range whose `first` equals its `last`.

### 4.2 The property mask

Seventeen bits are assigned, one per property the published files name. The
spelling in the right-hand column is the file's own and is what
`suas_sucd_property_by_name` matches and `suas_sucd_property_name` returns.

| Bit | Macro | Published spelling | Source |
|-----|-------|--------------------|--------|
| `0x00000001` | `SUCD_PROP_UNICODE_BRIDGED` | `Unicode_Bridged` | PropList |
| `0x00000002` | `SUCD_PROP_CONTROL_PLANE` | `Control_Plane` | PropList |
| `0x00000004` | `SUCD_PROP_PRINTABLE` | `Printable` | PropList |
| `0x00000008` | `SUCD_PROP_DIAGNOSTIC_BAN` | `Diagnostic_BAN` | PropList |
| `0x00000010` | `SUCD_PROP_DIAGNOSTIC_WARN` | `Diagnostic_WARN` | PropList |
| `0x00000020` | `SUCD_PROP_DIAGNOSTIC_COM` | `Diagnostic_COM` | PropList |
| `0x00000040` | `SUCD_PROP_DIAGNOSTIC_SOFT` | `Diagnostic_SOFT` | PropList |
| `0x00000080` | `SUCD_PROP_TRAP_INSTRUCTION` | `Trap_Instruction` | PropList |
| `0x00000100` | `SUCD_PROP_SENTINEL_VALUE` | `Sentinel_Value` | PropList |
| `0x00000200` | `SUCD_PROP_ALPHABETIC` | `Alphabetic` | DerivedCoreProperties |
| `0x00000400` | `SUCD_PROP_CONTROL` | `Control` | DerivedCoreProperties |
| `0x00000800` | `SUCD_PROP_BIDI_LTR` | `LTR` | BidiProps |
| `0x00001000` | `SUCD_PROP_BIDI_RTL` | `RTL` | BidiProps |
| `0x00002000` | `SUCD_PROP_BIDI_ARABIC_AL` | `ARABIC_AL` | BidiProps |
| `0x00004000` | `SUCD_PROP_BIDI_NEUTRAL` | `NEUTRAL` | BidiProps |
| `0x00008000` | `SUCD_PROP_BIDI_MIRRORED` | `MIRRORED` | BidiProps |
| `0x00010000` | `SUCD_PROP_BIDI_WHITESPACE` | `WHITESPACE` | BidiProps |

`SUCD_PROP_ASSIGNED_MASK` is `0x0001FFFF`. Bits `0x00020000–0x80000000` are
reserved and MUST be zero in a `0.1.0` image.

`Control_Plane` and `Control` are distinct bits because they are distinct
properties: the former is the PropList statement that a codepoint is in the SCP,
the latter the DerivedCoreProperties closure that includes the SCP, the diagnostic
blocks and the traps.

### 4.3 General category

Thirty values, densely numbered so that `Cn` — the unassigned default — is zero.
The eight UAX #44 group values (`L`, `M`, `N`, `P`, `S`, `Z`, `C`, `LC`) are
unions and are deliberately absent: a record stores the finest classification it
has, and a caller wanting "a letter" tests four values.

`suas_sucd_gc_name` returns the canonical two-letter alias and `??` for a value
outside the enum. `suas_sucd_gc_by_name` matches exactly and case-sensitively,
returning `Cn` for anything that is not an alias.

### 4.4 Cursor contract

`SUCSData.txt`'s `type` column says whether the cursor advances over a
codepoint. SUCD records that as flags:

| Flag | `type` | Meaning |
|------|--------|---------|
| `SUCD_RECORD_PRINTABLE` | 0 | The cursor advances |
| `SUCD_RECORD_CONTROL` | 1 | A machine instruction; the cursor does not advance |

Exactly one is set. The default record is `PRINTABLE`, because an unassigned
codepoint consumes a cell rather than executing.

### 4.5 Plane taxonomy

`suas_sucd_plane_of` returns one `SUCD_PLANE_*` bit, most specific first:

| Plane | Range | Bit |
|-------|-------|-----|
| Bridge | `0x00000000–0x0010FFFF` | `SUCD_PLANE_BRIDGE` |
| SCP | `0x00110000–0x0011FFFF` | `SUCD_PLANE_SCP` |
| RNUR | `0x00160000–0x0016FFFF` | `SUCD_PLANE_RNUR` |
| Native | `0x00120000–0x7FFFFFEF` | `SUCD_PLANE_NATIVE` |
| Trap | `0x7FFFFFF0–0x7FFFFFFE` | `SUCD_PLANE_TRAP` |
| Sentinel | `0x7FFFFFFF` | `SUCD_PLANE_SENTINEL` |

RNUR is a carve-out *inside* Native and reports itself rather than Native, so it
stays addressable; a caller wanting "is it native" tests both bits. A codepoint
past `0x7FFFFFFF` returns `0`.

### 4.6 The default record

A codepoint is unassigned when its page is `NO_PAGE`, when its dense entry is
`NO_RECORD`, or when the named record's `[first, last]` does not contain it. All
three yield the default record rather than a failure:

```
first = last = 0        property_mask = 0
general_category = Cn   script_id = SUAS_SCRIPT_UNKNOWN
flags = SUCD_RECORD_PRINTABLE
```

A database that answers "not assigned" is answering the question.

## 5. The Binary Image

### 5.1 Geometry

The Base SUCS space is 31 bits. SUCD splits it `16 / 15`:

```
SUAS_SUCD_PAGE_SHIFT      16
SUAS_SUCD_PAGE_SIZE       65 536
SUAS_SUCD_PAGE_MASK       0x0000FFFF
SUAS_SUCD_DIRECTORY_SIZE  32 768       (= (0x7FFFFFFF >> 16) + 1)
SUAS_SUCD_DIRECTORY_BYTES 131 072
```

The split is exhaustive: the highest page index is `0x7FFFFFFF >> 16 == 0x7FFF`,
the last of 32 768. Exhaustive is what lets the directory carry no terminator and
the lookup need no page bound check.

The page is the unit of *resolution*, not of storage. A page whose codepoints
share one classification costs one directory slot; only a page whose
classification varies pays for a per-codepoint index.

### 5.2 Layout

```
offset 0                             sucd_header_t          24 bytes
offset 24                            uint32_t directory[32768]   131 072 bytes
offset 131 096                       uint16_t dense[...]    (variable)
                                     sucd_record_t records[n]    16n bytes
```

The dense extent is not stored: it is derived.

```
dense_units = (image_size − 24 − 131 072 − 16 · total_records) / 2
```

The subtraction must be non-negative and the division exact, or the image is
rejected with `SUCD_IMAGE_SIZE_MISMATCH`. This is SUCD-I4: an image's size fully
determines its layout, so there is no length field to lie about.

Header:

| Field | Meaning |
|-------|---------|
| `magic` | `SUAS_SUCD_MAGIC` (`'SUCD'`, `0x53554344`) |
| `version` | Packed `MAJOR<<16 | MINOR<<8 | PATCH` |
| `total_records` | Record count, `≤ 65 535` |
| `plane_mask` | `SUCD_PLANE_*` bits the image covers |
| `directory_count` | Must equal `SUAS_SUCD_DIRECTORY_SIZE` |
| `flags` | `SUCD_IMAGE_PARTITION` when the compiler proved §4's partition |

### 5.3 The three slot kinds

Each `directory[p]` is exactly one of:

| Value | Kind | Meaning |
|-------|------|---------|
| `SUAS_SUCD_NO_PAGE` (`0xFFFFFFFF`) | empty | Every codepoint in the page is unassigned |
| `SUAS_SUCD_UNIFORM_TAG \| i` | uniform | Record `i` covers the entire page; no index load |
| anything else | dense | Offset, in `uint16` units, into the page's dense index |

A dense index holds 65 536 entries, one per codepoint, each a record index or
`SUAS_SUCD_NO_RECORD` (`0xFFFF`). A record index therefore never collides with
the marker, which caps the record count at 65 535.

Three kinds, not two, because the space is simultaneously enormous and empty in
the middle and dense at the bottom. A two-way encoding forces one of those truths
to be paid for everywhere:

- **Native** is one range of 2.1 billion codepoints → one uniform slot per page,
  no index at all.
- **ASCII** carries a dozen distinct classifications in one 65 536-codepoint page
  → one 128 KiB dense index, nothing else.
- The middle of the Native plane is unassigned → `NO_PAGE`, no storage.

Dense-everything costs 4 GiB. Dense-nothing cannot describe ASCII.

### 5.4 Lookup

```
slot = directory[cp >> 16]
if slot == NO_PAGE                  → default
if slot & UNIFORM_TAG               → i = slot & 0x7FFFFFFF
else                                → i = dense[slot + (cp & 0xFFFF)]
                                      (NO_RECORD → default)
r = records[i]
return (cp < r.first || cp > r.last) ? default : r
```

One directory load, at most one index load, one record load, one compare. The
final compare is what makes the dense index safe even if it were stale; the
verifier makes it redundant, and it is kept because redundancy in the query path
is cheaper than a wrong answer.

### 5.5 Verification outcomes

`suas_sucd_verify_image` is O(image extent) and returns the *first* defect:

| Outcome | Trigger |
|---------|---------|
| `SUCD_IMAGE_NULL_IMAGE` | Base pointer is NULL |
| `SUCD_IMAGE_TRUNCATED` | Size below the 24-byte header |
| `SUCD_IMAGE_BAD_MAGIC` | `magic != SUAS_SUCD_MAGIC` |
| `SUCD_IMAGE_BAD_DIRECTORY_COUNT` | `directory_count != 32 768` |
| `SUCD_IMAGE_RECORD_COUNT_OVER` | `total_records > 65 535` |
| `SUCD_IMAGE_SIZE_MISMATCH` | No exact layout for the size |
| `SUCD_IMAGE_BAD_RECORD_FLAG` | Flags are neither, or both, cursor kinds |
| `SUCD_IMAGE_RANGE_ORDER` | `first > last` |
| `SUCD_IMAGE_OUT_OF_SPACE` | `last > 0x7FFFFFFF` |
| `SUCD_IMAGE_GC_OUT_OF_RANGE` | General category ≥ 30 |
| `SUCD_IMAGE_RESERVED_PROPERTY_BIT` | Mask outside `SUCD_PROP_ASSIGNED_MASK` |
| `SUCD_IMAGE_UNORDERED_RECORDS` | `first` not strictly ascending |
| `SUCD_IMAGE_OVERLAPPING_RECORDS` | `first ≤ previous.last` |
| `SUCD_IMAGE_DIRECTORY_OUT_OF_RANGE` | Slot or dense entry names no record |
| `SUCD_IMAGE_DENSE_OFFSET_OVER` | Dense slot runs past the index |
| `SUCD_IMAGE_PAGE_NOT_COVERED` | Uniform record misses its page, or a dense entry names a record that does not cover its codepoint |

`SUCD_IMAGE_OK` is zero, so a zero-initialised reason reads as success.
`suas_sucd_image_error_name` returns the macro spelling or
`"SUCD_IMAGE_UNKNOWN"`.

### 5.6 What the base image costs

A full `0.1.0` image populates 21 pages: pages 0–17 (Bridge and SCP), 18 (Native
start), 22 (RNUR) and 32 767 (traps and Sentinel). Two of them — the ASCII page
and the top page — carry dense indices. The image is therefore roughly
128 KiB + 21 · 128 KiB + a few hundred bytes of records ≈ **2.8 MiB**, for data
that is over 150 MiB as text. Fewer dense pages is the right trade for a
kernel-mapped immutable database; it is not an optimisation that may change the
lookup's shape.

## 6. Data-File Decoding

### 6.1 Range forms

`suas_sucd_range_form` returns `SUCD_RANGE_FORM_UCD` for `A..B`,
`SUCD_RANGE_FORM_HYPHEN` for `A-B`, `SUCD_RANGE_FORM_POINT` for a lone `A`, and
`SUCD_RANGE_FORM_NONE` for a comment, a blank line, or anything that is not a
range. Leading spaces and tabs are skipped; a `#` after the range is a comment
and is ignored by `suas_sucd_parse_range`.

### 6.2 Bounds

`suas_sucd_parse_range` accepts 1–8 significant hex digits and rejects:

- a reversed pair (`005A-0041`);
- a ninth hex digit (`123456789`);
- a value past the space (`80000000`);
- a missing right operand (`0021-`);
- a NULL line.

`NULL` out-parameters are permitted, so a caller may take only the verdict.

## 7. The SUAS-010 (SSRE) Linkage Gate

An image records what it was compiled *from*. `suas_sucd_verify_schema_link`
confirms that schema is registered in SSRE and meets a declared floor:

```c
static const sucd_schema_link_t scms_link = {
    "SUAS-011", 1u, 0u, true       /* SCMS ≥ 1.0, mandatory */
};
```

The gate fails for a NULL link, a NULL identifier, an unregistered schema, a
mandatory claim against an optional data specification, and a floor above the
registered version. A failed gate means no image is emitted (SUCD-I8).

## 8. Complexity and Memory

| Operation | Cost |
|-----------|------|
| `suas_sucd_verify_image` | O(image extent) — one pass, no allocation |
| `suas_sucd_init` | O(image extent) — verification plus pointer placement |
| `suas_sucd_get_record` | O(1) — one directory load, ≤ one index load, one record load, one compare |
| `suas_sucd_property_mask`, `_plane_of` | O(1) |
| `suas_sucd_gc_name`, `_plane_name`, `_image_error_name` | O(1) |
| `suas_sucd_gc_by_name`, `_property_by_name` | O(len) over a fixed table |
| `suas_sucd_range_form`, `_parse_range` | O(len) over a fixed line |
| `suas_sucd_records_overlap` | O(1) |
| `suas_sucd_is_partition` | O(count) |
| `suas_sucd_verify_schema_link` | O(1) expected — one SSRE lookup |

The engine's entire mutable state is four pointers, two lengths and a flag.
Nothing is copied out of the image, and nothing is retained past the mapping.

## 9. Implementation Notes

### 9.1 Compiling an image

```c
/* 1. Resolve every published file into candidate ranges. */
/* 2. Apply the resolution rules of §3.3 to obtain a partition. */
if (!suas_sucd_is_partition(records, n))
    return SUCD_COMPILE_ABORT;              /* SUCD-I3 */

/* 3. Place each record: uniform page, dense page, or empty page. */
/* 4. Emit header, directory, dense index, records. */

if (!suas_sucd_verify_image(buf, len, &why)) {
    fprintf(stderr, "image: %s\n", suas_sucd_image_error_name(why));
    return SUCD_COMPILE_ABORT;
}
```

### 9.2 Binding and querying

```c
sucd_record_t rec;

if (!suas_sucd_verify_schema_link(&scms_link))
    return SUCD_ABORT;                       /* SUCD-I8 */
if (!suas_sucd_init(image, image_len))
    return SUCD_ABORT;                       /* verification failed */

if (suas_sucd_get_record(cp, &rec)) {
    if (rec.property_mask & SUCD_PROP_ALPHABETIC)
        ...
}
```

`suas_sucd_get_record` returns `true` whenever it wrote a record — including the
default — and `false` only for a NULL output pointer. An unbound engine answers
with the default record rather than failing, so a caller may query before
mounting and learn "nothing is assigned" instead of crashing.

### 9.3 Decoding the published files

```c
uint32_t first, last;

for (each_line(file, &line, &len)) {
    if (suas_sucd_range_form(line, len) == SUCD_RANGE_FORM_NONE)
        continue;                            /* comment or blank */
    if (!suas_sucd_parse_range(line, len, &first, &last))
        return SUCD_COMPILE_ABORT;            /* malformed range */
    add_candidate(first, last, property_of(line));
}
```

## 10. Testing

The reference implementation is exercised by `tests/test_suas_sucd.c` against a
constructed 393 KB image that deliberately contains all three slot kinds — dense
pages at both ends of the space, uniform pages across the Native plane, a dense
gap inside ASCII, and a second 128 KiB image with no dense index at all. Coverage:

- version, geometry constants, and the exhaustiveness of the `16/15` split;
- the general-category and property taxonomies, including exact and
  case-sensitive name matching, multi-bit rejection, and reserved bits;
- plane resolution for every plane, the RNUR carve-out, the Sentinel, and
  out-of-space;
- all four range syntaxes, and every rejection the parser must make;
- overlap and partition predicates, including adjacency, nesting, gaps,
  disorder and out-of-space;
- every `sucd_image_error_t`, each reached by corrupting exactly one thing in a
  fresh copy of a valid image;
- lookup through a uniform page, a dense page, a dense gap, the default record,
  and out-of-space; binding idempotence and shutdown;
- the SSRE linkage gate across its four failure modes;
- confirmation that SUCD is itself registered in SSRE.

The unified test (`unified/test_unified.c`) additionally binds a minimal image in
the same translation unit as every other public header, which is the real check
that the legacy BiDi surface and the new record surface coexist.

## 11. Stability and Versioning

`SUAS_SUCD_SPEC_ID` is `"SUAS-012"`, and the version is `1.0.0`. The version
identifies the record layout, the property mask and the image geometry.

- **Patch.** New verification outcome that cannot change the verdict of a
  previously valid image; new informative text.
- **Minor.** Adding a property bit within the reserved range; adding a range-form
  alias; adding a taxonomy name. A previously valid image still verifies and
  every previously correct lookup still returns the same record.
- **Major.** Changing `SUAS_SUCD_PAGE_SHIFT`, the directory size, the record
  size, the meaning of an existing bit, or the partition invariant. Any of these
  changes what an existing image means.

Property bits are retired, never reused, exactly as SSRE identifiers are retired
(§11.1 of SUAS-010). A bit that is removed stays reserved so that a stale image
fails verification instead of silently reinterpreting a field.

## 12. Conformance Requirements

An implementation conforms to SUAS-012 when:

1. it verifies an image before reading it, and rejects one whose geometry, magic,
   record count or size is inconsistent (§5.5, SUCD-I1, I4);
2. it resolves a codepoint in O(1) with at most one directory load, one index
   load and one record load (SUCD-I2);
3. it rejects unordered or overlapping records and proves a partition before
   emitting an image (SUCD-I3);
4. it decodes all four published range syntaxes and rejects the malformed forms
   of §6.2;
5. it assigns exactly the seventeen properties of §4.2 and treats every other bit
   as reserved (SUCD-I6);
6. it sets exactly one cursor flag per record (SUCD-I7);
7. it returns the default record for an unassigned page, an unassigned dense
   entry, an out-of-space codepoint and an unbound engine;
8. it exposes the SSRE linkage gate and refuses to compile an image whose schema
   link does not verify (SUCD-I8);
9. it allocates nothing in any operation and retains no pointer past the call.

### 12.1 Compliance matrix (reference implementation)

| Requirement | Location | Status |
|-------------|----------|--------|
| R1 — Verify before read; named defects | `suas_sucd_verify_image`, `sucd_image_error_t` | ✔ |
| R2 — O(1) lookup | `suas_sucd_get_record` | ✔ |
| R3 — Partition proof | `suas_sucd_is_partition`, `suas_sucd_records_overlap` | ✔ |
| R4 — Four range syntaxes | `suas_sucd_range_form`, `suas_sucd_parse_range` | ✔ |
| R5 — Seventeen assigned properties | `SUCD_PROP_*`, `suas_sucd_property_by_name` | ✔ |
| R6 — One cursor contract | `SUCD_RECORD_PRINTABLE` / `SUCD_RECORD_CONTROL` | ✔ |
| R7 — Default record everywhere | `suas_sucd_default_record`, `suas_sucd_get_record` | ✔ |
| R8 — SSRE linkage gate | `suas_sucd_verify_schema_link` | ✔ |
| R9 — Zero allocation | whole module | ✔ |

## 13. Terms and Conventions

**SUCD** — SuperUnicode Character Database; this standard. Also the offline
reference layer at `/Public/<version>/sucd/`. (Not SUCA, SUTS-001, the
SuperUnicode Collation Algorithm.)

**Image** — The binary form of the database: header, directory, optional dense
index, records.

**Page** — A 65 536-codepoint window, `codepoint >> 16`.

**Uniform page** — A page wholly covered by one record; its directory slot names
that record directly.

**Dense page** — A page whose codepoints do not share one classification; its
directory slot names a page-sized index.

**Dense index** — The `uint16_t[65 536]` belonging to a dense page.

**Record** — A 16-byte inclusive range with a property mask, general category,
script and cursor flags.

**Default record** — `Cn`, no properties, UNKNOWN script, `PRINTABLE`.

**Partition** — An ordered, pairwise-disjoint record set; gaps allowed, overlaps
not.

**Simple / derived / contributory property** — UAX #44's three-tier taxonomy,
retained as the compiler's contract (§2.1).

**`@missing`** — UAX #44's per-file default. Replaced in SUCD by the default
record.

**Compiled subset** — The alias for the legacy SUAS-001 BiDi projection in §14.

## 14. The Legacy BiDi Surface (SUAS-001)

Before this standard, SUCD existed only to answer SUAS-001's question — "what is
this codepoint's bidirectional role?" — through a compiled subset of
`sucd/Props/BidiProps.txt`. That surface is retained verbatim because SUAS-001
§6 and SUTR-8 both name its bits, and SDF compares them for equality:

| Mask | Value | Meaning |
|------|-------|---------|
| `SUCD_BIDI_LTR` | `1u << 0` | Strong left-to-right |
| `SUCD_BIDI_RTL` | `1u << 1` | Strong right-to-left |
| `SUCD_BIDI_ARABIC_AL` | `1u << 2` | Arabic letter, resolves right-to-left |
| `SUCD_BIDI_NEUTRAL` | `1u << 3` | Direction-neutral |
| `SUCD_BIDI_MIRRORED` | `1u << 4` | Paired-format glyph, auto-mirror |
| `SUCD_BIDI_WHITESPACE` | `1u << 5` | Separator / whitespace, neutral |

`SUCD_BIDI_ALL` is their union. `suas_sucd_bidi(cp)` returns the mask for a
codepoint inside the Unicode Bridge and `SUCD_BIDI_NEUTRAL` otherwise. The zone
predicates `suas_sucd_is_unicode_bridge`, `suas_sucd_is_scp` and
`suas_sucd_is_native` are likewise retained.

The legacy mask is **not** an alias for `SUCD_PROP_BIDI_*`. They answer the same
taxonomy in two encodings: the legacy mask numbers from bit 0 because SDF's
comparisons depend on it, while the record mask numbers within the larger
seventeen-property space. Conflating them would silently change the value SDF
asserts.
