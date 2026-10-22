# SUTS-006 — System Internationalized Domain & Identifier Network Architecture (SIDN)

**Standard ID:** SUTS-006
**Status:** Draft — Authoring
**Scope:** Modular Technical Specification (internationalized domain names, label routing, network identifier security)
**Applies To:** Base SUCS (Unicode Compatibility Bridge, SCP, Native), ExtSUCS (unbounded arbitrary-precision plane), the RNUR neography registry, SUCD property tables, the OpenWindows microkernel, the OpenWindows shell, the network routing path, the IPC channel namespace, SUTS-005 (SCSS) character security, the character token stream
**Last Updated:** 2026-10-21

---

## 1. Introduction

A network name is an identifier that leaves the machine. A process that opens a
connection to a host, binds an IPC channel, or registers a service route types a
sequence of symbols, and the operating system must decide two things about it at
once: **is it well-formed**, and **does it name exactly one endpoint**. Both
questions are cheap in ASCII and expensive the moment the name may be
internationalized: the same human-readable name may be written with different
codepoints, a single visual label may be assembled from several scripts, and a
label that a text-level encoder would quietly rewrite into a different label is
exactly the label an attacker wants.

UTS #46, *Unicode IDNA Compatibility Processing*, answers those questions in
prose, and RFC 3492 / RFC 5890 answer them in a text wire format. SIDN is the
runtime counterpart of that idea for OpenWindows: the kernel primitive that maps
a codepoint to a compiled IDNA **Status**, validates a whole label in a single
pass, and produces a **canonical binary routing form** — over 31-bit
SuperUnicode streams, with no heap allocation and no text-based conversion step.

### 1.1 Relationship to UTS #46

SIDN solves the same problem as UTS #46: classifying a codepoint as admissible
in an internationalized label, folding the codepoints that must fold, and
rejecting the ones that must not survive. It is deliberately **not** a UTS #46
analog, because a text IDNA library and a kernel routing primitive differ in
four structural ways:

1. **Text algorithm vs. single pass.** UTS #46 describes a pipeline — *Map*,
   *Normalize to NFC*, *Break*, *Convert* (Punycode / RFC 3492), *Validate* —
   run over dynamically allocated strings, with the Punycode step encoding the
   label as ASCII `xn--…` for the DNS wire. SIDN discards the text pipeline
   entirely: it walks the label **once**, applies the compiled Status of each
   codepoint as it goes, and emits the canonical form in the same pass (§4).
2. **Punycode vs. direct 31-bit routing.** A conforming UTS #46 `ToASCII`
   produces an ASCII-compatible encoding because the DNS itself cannot carry
   non-ASCII. A SuperUnicode network has no such constraint: a SUCS stream is
   the wire form, a `xn--` prefix is a 7-bit artifact of the old DNS, and SIDN
   therefore never performs an ASCII encoding loop at all. The canonical routing
   form is a **SUCS codepoint stream**, not an ACE string.
3. **Heap allocation vs. zero allocation.** A user-space IDNA implementation
   loads the mapping table into hash maps and buffers the intermediate string at
   each pipeline stage, all on the heap. OpenWindows enforces a strict
   zero-allocation mandate for core kernel subsystems. SIDN resolves a
   codepoint's Status through a fixed-size, static, open-addressing index, and
   materializes a label into a bounded stack buffer.
4. **User-space vs. kernel-primitive scope.** UTS #46 is designed for
   user-space browsers, registrars, and library authors who read the mapping
   table from disk. SIDN is a low-level kernel primitive: the microkernel, the
   shell parser, the IPC namespace, and the routing table decide identifier
   admissibility in-band, without a file system, a user-space service, or a
   per-process cache.

Where this document shares vocabulary with UTS #46 (valid, mapped, deviation,
disallowed, label, Bidi), the requirements are normative for SIDN only.

### 1.2 Key architectural shifts

| Dimension | UTS #46 (user-space) | SUTS-006 SIDN (kernel) |
|-----------|----------------------|------------------------|
| Representation | `IdnaMappingTable.txt` parsed at run time | Fixed-size C records in read-only memory |
| Allocation | Heap string at each pipeline stage + heap lookup maps | Zero; one static table + one fixed index, bounded stack buffer |
| Pipeline | Map → NFC → Break → Punycode → Validate | One linear pass: map, fold, validate, route |
| Wire form | ASCII-compatible `xn--` (Punycode / RFC 3492) | Direct 31-bit SUCS codepoint stream |
| Lookup | Parse, tokenize, then a map lookup | codepoint → FNV-1a → fixed open-addressing probe, O(1) |
| Verdict | Status per codepoint, from text | Compiled 3-bit Status per codepoint, computed in O(1) |
| Boundary | Label per `ToASCII`/`ToUnicode` call | Explicit label-separator folding in-band |
| Space coverage | 21-bit Unicode (0x0000–0x10FFFF) | Full 31-bit SUCS plus unbounded ExtSUCS |

### 1.3 The dual-layer architecture

SIDN is a **dual-layer** system, in the same relationship SUAS-009 (SIP),
SUAS-012 (SUCD), and SUTS-005 (SCSS) have with their Unicode counterparts:

* **Offline reference layer — SUCD.** The full text manifests: the IDNA mapping
  table, the label restriction rules, and the human-readable
  domain-profile definitions live in the SuperUnicode Character Database for
  network compilers, developer toolchains, and user-space applications.
* **Runtime kernel layer — SIDN.** Build-time tooling **distils** those SUCD
  datasets into static, immutable, byte-packed binary routing tables embedded
  directly in the kernel memory image. The kernel never reads SUCD at run time.

> The seeding brief for this specification was labelled "SUAS-015" and referred
> to its offline reference layer as "SUCD - SUAS-014". Both numbers are stale:
> in this corpus **SUCD** is already the SuperUnicode Character Database
> (**SUAS-012**), **SUAS-013** is the System Ideographic Source Registry
> (SISR), and the series owner directed that SIDN be authored as a **SUTS**
> technical specification. It is therefore registered as **SUTS-006**, the
> number freed when SISR was moved from SUTS-006 to SUAS-013. The house prefix
> `SIDN_*` replaces the sketched `SIDN_*`/`SUAS_*` mixture, and the sketched
> entry points `suas_sidn_init_engine`, `suas_sidn_validate_label`, and
> `suas_sidn_to_canonical` become `suts_sidn_init_engine`,
> `suts_sidn_validate_label`, and `suts_sidn_to_canonical`, matching the sibling
> SUTS modules. The sketched `SIDN_FLAG_*` names and the
> `sidn_label_descriptor_t` field layout are preserved verbatim.

### 1.4 Terminology and cross-references

SIDN reuses the SuperUnicode architecture documents, numbered as they exist in
this repository:

| SIDN concept | Authoritative document |
|--------------|------------------------|
| Codepoint space, SCP, traps, Sentinel | **SUTR-0 (SUCS Core)** — three-space layout |
| Bridge / Native / ExtSUCS planes | **SUTR-7 (SUCEM)**, **SUTR-6 (Bridge)** |
| RNUR neography registry, property tiers | **SUTR-8 (SUCPM)** — the character property model |
| Character security, homographs, identifier admissibility | **SUTS-005 (SCSS)** — UTS #39 analog |
| Collation of network name ordering | **SUTS-001 (SUCA)** — UCA-equivalent collation |
| Identifier classes and lexical syntax | **SUAS-007 (SIS)** — UAX #31 analog |
| Script identification | **SUAS-005 (SSP)** — System Script Property |
| Locale data and formatting | **SUTS-003 (SLDF)** — binary-compiled locale schema |
| Conformance model and zero-allocation mandate | **SUTR-10 (SUCM)** |
| Offline reference data | **SUCD** — the SuperUnicode Character Database (SUAS-012) |

> The reference implementation ships as `include/suts/suts_sidn.h` +
> `src/suts/suts_sidn.c`. The library and function names (`suts_sidn.h`,
> `suts_sidn_*`) follow the house convention of the sibling SUTS modules. The
> record types are `sidn_mapping_entry_t` and `sidn_label_descriptor_t`.

### 1.5 Conformance

A conforming SIDN implementation MUST:

1. represent the IDNA mapping as immutable, fixed-size records reachable
   directly from a codepoint key, with **no heap allocation** on any query path;
2. resolve a codepoint to its Status and mapping value in **O(1) expected
   time** by a bounded probe of a fixed-size index, not by a linear scan of a
   table;
3. evaluate a label in a **single linear pass** over the input stream, folding
   every label separator to U+002E, and produce the canonical routing form
   without re-entering the string;
4. admit no System Control Plane marker, trap slot, the Sentinel, or an ExtSUCS
   plugin codepoint as label material (§5.4); and
5. treat a codepoint that carries no curated record as **disallowed**, so the
   mapping function is total over the address space (§2.5).

### 1.6 SIDN conformance levels

| Level | Name | Requires |
|-------|------|----------|
| L0 | Kernel Core | the Bridge mapping table; zero-allocation O(1) Status resolution; single-pass validation |
| L1 | Routing | L0 plus the canonical routing form, separator folding, and the validity criteria of §4.2 |
| L2 | Neography & Bidi | L1 plus the RNUR neography zone (§5) and the Bidi / ContextJ criteria of §4.3 |

## 2. The compiled mapping schema

### 2.1 The Status taxonomy

SIDN assigns every codepoint exactly one **Status**, transcribed from the
UTS #46 mapping-table taxonomy:

| Status | Value | Meaning |
|--------|-------|---------|
| `SIDN_STATUS_VALID` | 0 | kept unchanged; admissible in a label |
| `SIDN_STATUS_IGNORED` | 1 | removed from the label (mapped to the empty string) |
| `SIDN_STATUS_MAPPED` | 2 | replaced by the record's mapping value |
| `SIDN_STATUS_DEVIATION` | 3 | kept or mapped, depending on the processing mode (§3) |
| `SIDN_STATUS_DISALLOWED` | 4 | never allowed |

The Status is a compiled field of a fixed-size record, not a token in a
semicolon-delimited data line. `suts_sidn_status_name` renders it.

### 2.2 The mapping record

A compiled per-codepoint mapping record is one `sidn_mapping_entry_t`:

```c
typedef struct {
    uint32_t codepoint;      /* base SUCS codepoint                    */
    uint32_t mapping[2];     /* replacement, up to two codepoints      */
    uint8_t  status;         /* sidn_status_t                          */
    uint8_t  mapping_length; /* 0, 1 or 2                              */
    uint16_t reserved;       /* must be zero                           */
} sidn_mapping_entry_t;
```

The record is exactly **16 bytes** and byte-aligned, so an array of records is
directly indexable in read-only memory. The two-codepoint mapping field is what
lets the record carry the transitional expansion of `ß` → `ss` (`U+0073
U+0073`) as a single self-describing entry, while the overwhelming majority of
records set `mapping_length` to 1. `codepoint` is a 31-bit SUCS value; the
record never appears for an ExtSUCS codepoint above `0x7FFFFFFF`.

### 2.3 The label descriptor

The seeding sketch defines the label descriptor; it is preserved unchanged:

```c
typedef struct {
    uint32_t label_hash;     /* precomputed hash of the normalized stream */
    uint16_t length;         /* codepoint count                           */
    uint16_t flags;          /* SIDN_FLAG_* bitfield                      */
} sidn_label_descriptor_t;
```

The descriptor is exactly **8 bytes**. It is a fixed-size value the caller owns;
`label_hash` is an FNV-1a hash of the *normalized* label stream (the routing
form), `length` is its codepoint count, and `flags` carries the verdict word.

### 2.4 The fixed index and the hash

A mapping query hashes the codepoint and probes a fixed, open-addressing index:

```c
static sidn_index_slot_t sidn_index[SIDN_MAPPING_INDEX_SIZE]; /* 256 slots */
```

The index size (`256`) is a power of two larger than the table capacity
(`128`), so the load factor stays below one-half, the probe sequence stays short
and bounded, and a lookup is **O(1) expected time** with a small constant. The
key hash is FNV-1a 32-bit over the four little-endian bytes of the codepoint
(`SIDN_FNV_OFFSET_BASIS = 2166136261`, `SIDN_FNV_PRIME = 16777619`). The index
is built lazily on first use and is idempotent; no query path allocates.

The label hash is FNV-1a over the whole normalized stream, so two labels that
normalize to the same codepoint sequence hash equally, and two labels that
differ anywhere hash differently with high probability. A zero-length or NULL
label hashes to zero, a value the hash of a non-empty label never produces.

### 2.5 Totality by absence

The reference mapping table is a **curated subset** of the offline SUCD dataset.
A codepoint with no record is therefore *not* automatically admissible: SIDN
treats absence as `SIDN_STATUS_DISALLOWED`, and a codepoint is admitted without
a record only when it lies in a range the engine recognizes as ordinary label
material (§6). This keeps the mapping function **total** over the address space
without the table being dense: `suts_sidn_status_of` always returns a Status,
and it is `DISALLOWED` for an SCP marker, a trap slot, the Sentinel, an ExtSUCS
codepoint, and any uncurated codepoint the engine does not recognize.

## 3. Processing

### 3.1 Label separators

A domain name is a sequence of labels joined by a separator. SIDN recognizes
the four codepoints that UTS #46 folds to the full stop:

| Codepoint | Name | Canonical |
|-----------|------|-----------|
| `U+002E` | FULL STOP | `U+002E` |
| `U+FF0E` | FULLWIDTH FULL STOP | `U+002E` |
| `U+3002` | IDEOGRAPHIC FULL STOP | `U+002E` |
| `U+FF61` | HALFWIDTH IDEOGRAPHIC FULL STOP | `U+002E` |

`suts_sidn_is_label_separator` is the O(1) predicate, and
`SIDN_LABEL_SEPARATOR` is the canonical `U+002E`. The three non-ASCII
separators carry a `SIDN_STATUS_MAPPED` record whose mapping value is `U+002E`,
so the mapping table and the predicate agree by construction.

### 3.2 Deviation characters

Four codepoints are **deviations**: their treatment depends on the processing
mode.

| Codepoint | Name | Nontransitional | Transitional |
|-----------|------|-----------------|--------------|
| `U+00DF` | LATIN SMALL LETTER SHARP S | kept (`ß`) | mapped to `ss` |
| `U+03C2` | GREEK SMALL LETTER FINAL SIGMA | kept (`ς`) | mapped to `σ` |
| `U+200C` | ZERO WIDTH NON-JOINER | kept, subject to ContextJ | ignored |
| `U+200D` | ZERO WIDTH JOINER | kept, subject to ContextJ | ignored |

`SIDN_MODE_NONTRANSITIONAL` (`0`) is the default and the only normative mode;
`SIDN_MODE_TRANSITIONAL` (`1`) is provided for the deprecated UTS #46 C1
behavior and is never selected by the public API. In nontransitional processing
a deviation is **valid and kept**, and the descriptor's `SIDN_FLAG_TRANSITIONAL`
bit records that the label carried one.

### 3.3 The single pass

`sidn_process` is the whole engine. It walks the input once:

1. a label separator **ends the label** — the first separator terminates the
   scan, so a whole domain stream is processed one label at a time by advancing
   `input` past the separator;
2. each codepoint is resolved through the table; `VALID` codepoints are emitted
   as-is, `MAPPED` codepoints emit their mapping, `IGNORED` codepoints emit
   nothing, a kept `DEVIATION` emits itself, and a `DISALLOWED` codepoint fails;
3. when the walk completes, the emitted stream is measured (`length`,
   `label_hash`) and the validity criteria of §4.2 are applied to it.

The criteria are applied to the **mapped** stream, not the raw input: a
non-STD3 ASCII codepoint that survives mapping, or a hyphen that a mapping moved
into an edge position, must be caught. This is why validation materializes the
mapped label into a bounded stack buffer even when the caller asked only for a
verdict.

## 4. Validity and canonical routing

### 4.1 The processing outcomes

`SIDN_OK` is the only success value. Every defect is named:

| Code | Meaning |
|------|---------|
| `SIDN_ERR_NULL_ARGUMENT` | a NULL stream or descriptor |
| `SIDN_ERR_OUTPUT_TOO_SMALL` | the output buffer cannot hold the result |
| `SIDN_ERR_LABEL_EMPTY` | the label is empty |
| `SIDN_ERR_DISALLOWED` | a codepoint with Status disallowed |
| `SIDN_ERR_TRANSITIONAL_ONLY` | a deviation in a transitional mode |
| `SIDN_ERR_LEADING_COMBINING` | the label begins with a mark |
| `SIDN_ERR_INTERNAL_STOP` | a `U+002E` survived into a label |
| `SIDN_ERR_HYPHEN_POSITION` | a hyphen in the third and fourth slot |
| `SIDN_ERR_EDGE_HYPHEN` | the label begins or ends with a hyphen |
| `SIDN_ERR_RESERVED_PREFIX` | the label begins with the reserved prefix |
| `SIDN_ERR_STD3_ASCII` | a non-`[a-z0-9-]` ASCII codepoint |
| `SIDN_ERR_CONTEXTJ` | a joiner in an inadmissible context |
| `SIDN_ERR_BIDI` | a Bidi domain rule is violated |
| `SIDN_ERR_LABEL_TOO_LONG` | longer than the DNS label limit |
| `SIDN_ERR_NO_MAPPING` | a codepoint carries no mapping record |

`suts_sidn_status_code_name` renders any of them.

### 4.2 Validity criteria

A label is **valid** when it satisfies all of the following, each resolved in
O(1) once the normalized stream is known:

1. **Non-empty.** The label carries at least one codepoint.
2. **No internal stop.** No `U+002E` survives into the label (`INTERNAL_STOP`),
   because a separator ended the scan.
3. **Hyphen position** (`suts_sidn_check_hyphens`). The label does not begin or
   end with `U+002D`, and does not have `U+002D` in both the third and fourth
   positions.
4. **Reserved prefix** (`suts_sidn_has_reserved_prefix`). The label does not
   begin with the three codepoints `x n -` (`U+0078 U+006E U+002D`), which are
   reserved for the deprecated ACE encoding.
5. **STD3 ASCII** (`suts_sidn_check_std3`). Every ASCII codepoint in the label
   is a lowercase letter `[a-z]`, a digit `[0-9]`, or a hyphen. Non-ASCII
   codepoints are unconstrained by this criterion.
6. **Leading combining mark.** The label does not begin with a combining mark.
7. **ContextJ** (`suts_sidn_check_joiners`). Every `U+200C` or `U+200D` is in
   an admissible context: `U+200C` between two Arabic letters, or a joiner
   immediately after a Devanagari virama `U+094D`.
8. **Bidi** (`suts_sidn_check_bidi`). If the label contains an RTL letter (a
   **Bidi domain**, `suts_sidn_is_bidi_domain`), the label does not begin with a
   combining mark, and a label that mixes RTL and LTR letters ends in an RTL
   letter.
9. **DNS length** (`SIDN_LABEL_MAX_DNS`, 63). The label is at most 63
   codepoints long. `SIDN_DOMAIN_MAX_DNS` (253) bounds a whole name.

### 4.3 Canonical routing

`suts_sidn_to_canonical` resolves a network label to its **canonical binary
routing form**: the mapped, single-label SUCS stream. Every label separator in
the input is folded to `U+002E` and terminates the label, so a surrounding
`U+002E` is treated as the label boundary. The function returns the number of
codepoints written; on failure it returns the count written before the defect,
with `SIDN_ERR_*` naming the first defect. The output buffer MUST hold at least
`SIDN_LABEL_MAX` codepoints.

The routing form is a lookup key. Two labels are **routing-equivalent** exactly
when their canonical forms are codepoint-identical; the network routing table
keys on the canonical form and the label hash, so a `BÜCHER` typed by a user,
`bücher` typed lowercase, and a fullwidth-capital form all resolve to
`U+0062 U+00FC U+0063 U+0068 U+0065 U+0072` and reach the same endpoint. This is
where SIDN replaces the `xn--` ACE loop: the endpoint is named by 31-bit SUCS
codepoints directly, and there is no text-to-ASCII conversion step at all.

## 5. The address-space model

### 5.1 Zones

SIDN does not invent a second plane taxonomy. `suts_sidn_zone_of` classifies a
codepoint into the SUCD / SSP planes with the RNUR carve-out made explicit:

| Zone | Range | Routable |
|------|-------|----------|
| `SIDN_ZONE_BRIDGE` | `0x00000000–0x0010FFFF` | yes |
| `SIDN_ZONE_SCP` | `0x00110000–0x0011FFFF` | no |
| `SIDN_ZONE_RNUR` | `0x00160000–0x0016FFFF` | yes |
| `SIDN_ZONE_NATIVE` | rest of `0x00120000–0x7FFFFFEF` | yes |
| `SIDN_ZONE_TRAP` | `0x7FFFFFF0–0x7FFFFFFE` | no |
| `SIDN_ZONE_SENTINEL` | `0x7FFFFFFF` | no |
| `SIDN_ZONE_EXTSUCS` | `> 0x7FFFFFFF` | no |

`suts_sidn_is_routable` admits exactly the Bridge, RNUR, and Native zones. A
codepoint in the Control Plane, a trap slot, the Sentinel, or the ExtSUCS plugin
space is never label material, and `sidn_process` refuses it before consulting
the table.

### 5.2 Neography

`suts_sidn_is_neography` and `suts_sidn_neography_set` address the RNUR
carve-out. The set number of an RNUR coordinate is `(cp - 0x00160000) >> 8`, so
`U+00160100` is set 1 and `U+00160200` is set 2, matching SUTR-8 (SUCPM). A label
that contains an RNUR codepoint is flagged `SIDN_FLAG_NEOGRAPHY_DOMAIN` in its
descriptor, which is how a fully internationalized or conlang-based network
architecture on OpenWindows marks a name that lives in the neography registry.

## 6. The curated registry

The reference implementation ships a curated subset of the offline dataset, in
strictly ascending codepoint order (an ordering that is a **verification
invariant**, not a convenience: enumeration and the duplicate check both depend
on it). Extending the registry is a data update — a minor revision — and never
changes the API.

An uncurated but routable codepoint is treated as ordinary label material and
resolved `VALID`, unless it lies in a range the engine recognizes as
inadmissible ASCII (control codepoints, space, and the ASCII symbols outside
`[a-z0-9-]`), in which case it is `DISALLOWED`. This is the mechanism that makes
`s` `e` `r` valid without a per-codepoint record while `<` is rejected.

## 7. Complexity

| Operation | Cost |
|-----------|------|
| `suts_sidn_status_of`, `suts_sidn_mapping_of` | O(1) expected: one hash, one bounded probe |
| `suts_sidn_validate_label`, `suts_sidn_to_canonical` | one linear pass, O(n) in label length |
| Each validity criterion | O(1) after the normalized stream is known |
| `suts_sidn_zone_of`, `is_routable`, `is_neography` | O(1) comparison chain |
| `suts_sidn_hash_codepoint`, `hash_label` | O(1) / O(n), no allocation |

No path allocates, grows a table, or calls into libc.

## 8. Security integration

SIDN and SUTS-005 (SCSS) divide the network-naming problem. SIDN answers *is
this name well-formed and does it name one endpoint*; SCSS answers *does this
name impersonate another*. They compose on the routing path:

1. validate and canonicalize the label with SIDN;
2. screen the canonical form for homographs and script mixing with SCSS;
3. admit the name only when both agree.

Neither step allocates, and both are single-pass, so the composed check stays on
the kernel's O(1)-per-codepoint budget.

## 9. Worked example

Consider the handle `BÜCHER` typed at a service-resolution prompt, where `Ü` is
`U+00DC` and the rest are ASCII. Walking the label in one pass:

| Position | Codepoint | Status | Emitted |
|----------|-----------|--------|---------|
| 0 | `U+0042` `B` | mapped → `b` | `U+0062` |
| 1 | `U+00DC` `Ü` | mapped → `ü` | `U+00FC` |
| 2 | `U+0043` `C` | mapped → `c` | `U+0063` |
| 3 | `U+0048` `H` | mapped → `h` | `U+0068` |
| 4 | `U+0045` `E` | mapped → `e` | `U+0065` |
| 5 | `U+0052` `R` | mapped → `r` | `U+0072` |

The canonical routing form is `U+0062 U+00FC U+0063 U+0068 U+0065 U+0072`, the
label descriptor carries `SIDN_FLAG_VALID_LABEL`, and the routing table keys on
that form. A user who types the fullwidth capital `Ｕ+FF21` for the leading
letter, or writes the separator as `。` (`U+3002`), reaches exactly the same
endpoint — the separator folds to `U+002E` and the letter maps to `a`.

---

## Annex A — Curated registry contents

The reference data holds the records below. Every record is listed by group; the
engine additionally resolves uncurated routable codepoints as `VALID` per §6.

### A.1 Mapped to lowercase

| Group | Codepoints | Mapping |
|-------|-----------|---------|
| ASCII uppercase | `0041`–`005A` | each to its lowercase `0061`–`007A` |
| Fullwidth capital A | `FF21` | `U+0061` `a` |
| Capital sharp S | `1E9E` | `U+00DF` `ß` |

### A.2 Valid (kept)

| Group | Codepoints |
|-------|-----------|
| Hyphen, full stop | `002D 002E` |
| Digits | `0030`–`0039` |
| Underbar (raw symbol; STD3 rejects it alone) | `005F` |
| Lowercase letters | `0061 0065 006F 0073 007A` |
| Latin small u with diaeresis | `00FC` |
| RNUR neography | `160100 160200` |

### A.3 Separators (mapped to `U+002E`)

| Codepoints |
|-----------|
| `3002 FF0E FF61` |

### A.4 Ignored

| Codepoint | Name |
|-----------|------|
| `00AD` | SOFT HYPHEN |

### A.5 Deviations

| Codepoint | Name | Transitional mapping |
|-----------|------|----------------------|
| `00DF` | LATIN SMALL LETTER SHARP S | `ss` |
| `03C2` | GREEK SMALL LETTER FINAL SIGMA | `U+03C3` |
| `200C` | ZERO WIDTH NON-JOINER | ignored |
| `200D` | ZERO WIDTH JOINER | ignored |

### A.6 Disallowed

| Codepoint | Name |
|-----------|------|
| `2488` | DIGIT ONE FULL STOP |
| `2665` | BLACK HEART SUIT |

---

## Annex B — Compliance matrix (reference implementation)

| Requirement | Reference status |
|-------------|------------------|
| Binary, fixed-size mapping records (16 bytes) | Yes |
| Zero heap allocation on any path | Yes |
| O(1) expected Status and mapping resolution | Yes |
| Unique, ascending codepoint key in the table | Yes |
| Single-pass validation and canonical routing | Yes |
| Separator folding to `U+002E` | Yes |
| Deviation handling, nontransitional default | Yes |
| STD3 / hyphen / reserved-prefix criteria | Yes |
| ContextJ and Bidi criteria | Yes |
| Bridge + RNUR coverage | Yes |
| SCP-invariant routing; traps and Sentinel refused | Yes |
| Closed-by-default (total) Status | Yes |
| 31-bit SUCS and unbounded ExtSUCS compatibility | Yes |

---

## References

- **UTS #46** — Unicode IDNA Compatibility Processing (the user-space text
  pipeline this kernel primitive deliberately departs from)
- **RFC 3492** — Punycode (the ACE encoding SIDN replaces with direct 31-bit
  routing)
- **SUTS-005 (SCSS)** — System Character Security Specification (homographs and
  identifier admissibility, the sibling `suts_*` kernel primitive SIDN composes
  with)
- **SUTS-001 (SUCA)** — SuperUnicode Collation Algorithm (network-name ordering)
- **SUTS-003 (SLDF)** — System Locale Data & Formatting (the sibling
  binary-compiled kernel primitive)
- **SUAS-005 (SSP)** — System Script Property (the finer-grained script engine
  the Bidi criterion approximates)
- **SUAS-007 (SIS)** — System Identifiers and Syntax (identifier classes)
- **SUAS-012 (SUCD)** — SuperUnicode Character Database (the offline reference
  layer)
- **SUAS-013 (SISR)** — System Ideographic Source Registry (the standard whose
  move to the SUAS series freed the SUTS-006 number)
- **SUTR-0 (SUCS Core)** — the three-space layout, SCP, traps, and Sentinel
- **SUTR-6 (Unicode Compatibility Bridge)** — the permanent 1:1 guarantee and
  the Bridge boundary
- **SUTR-7 (SUCEM)** — the four-level character encoding model
- **SUTR-8 (SUCPM)** — the RNUR registry space and property tiers
- **SUTR-10 (SUCM)** — the conformance model and the zero-allocation mandate
