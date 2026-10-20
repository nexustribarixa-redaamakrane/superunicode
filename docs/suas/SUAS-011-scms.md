# SUAS-011 — System Character Manifest & Schema Standard (SCMS)

**Status:** Draft — Authoring
**Version:** 1.0.0
**Family:** SUAS — SuperUnicode Architecture Standards (Core Kernel Invariants)
**Applies To:** SUTR-0 (SUCS Core), SUTR-7 (SUCEM), SUTR-8 (SUCPM), SUTR-10 (SUCM), SUAS-009 (SIP), SUTS-005 (SCSS), SUAS-010 (SSRE)
**Reference Layer:** SUCD (SuperUnicode Character Database)

## 1. Overview

UAX #42 describes the Unicode Character Database as an XML schema. It is the
mechanism by which a conforming implementation is supposed to obtain the data
that Unicode defines, and it is the most expensive thing in the Consortium's
entire catalogue.

The cost is not incidental to XML; it is structural. XML is hierarchical, so a
record has no fixed size and no fixed offset. A parser must therefore build a
tree, and building a tree means allocating. A schema validator is a
general-purpose engine, so its running cost is not bounded by anything the
character database itself knows. And nothing in the result is binary-mappable,
so a kernel that wants the data must re-encode it a second time before a single
lookup can happen. A character database compiled through UAX #42 costs a DOM, a
heap, and two full passes before the first query returns.

SCMS replaces all of it with a grammar that is line-oriented *because* it is
binary-mappable. A manifest is a stream of lines. A record is exactly six
tab-delimited fields in a fixed order, each of a known and bounded type. A
directive is a name and a value. One record line corresponds to one kernel
table entry, in order, with no re-encoding step and no intermediate tree. Build
tooling parses a manifest in a single linear pass over zero-allocation
streaming buffers, and a manifest that does not validate is never compiled.

SCMS is the structural counterpart of UAX #42 in the same sense that SUAS-009
(SIP) is the structural counterpart of UAX #38 and SUAS-010 (SSRE) is the
structural counterpart of UAX #41: the concept is retained, the document
format is not.

### 1.1 Architectural shifts from UAX #42

| Aspect | UAX #42 | SCMS |
|--------|---------|------|
| Format | XML, hierarchical | Line-oriented, flat |
| Record shape | Unbounded, nested elements | Exactly 6 tab-delimited fields |
| Parser | DOM or SAX tree builder | Single bounded linear pass |
| Memory | Heap-allocated node tree | Caller-owned fixed buffers |
| Validation | General-purpose schema engine | Bounded decoders, one outcome code each |
| Output | A tree to be re-encoded | Binary-mappable, 1:1 with kernel tables |
| Range handling | Element attributes | `start` and `end` columns |
| Plane safety | None | Endpoints *and* interior checked |
| Linkage | None | SSRE gate on the target schema |
| Failure mode | A validator reports | The build refuses to emit a blob |

### 1.2 Goals

1. **One line, one table entry.** The mapping from manifest record to kernel
   record must be direct and ordered, with no intermediate representation.
2. **A single linear pass.** No backtracking, no lookahead beyond the current
   field, no second scan.
3. **Zero allocation.** Every decoder is bounded; the streaming state is
   caller-allocated; nothing is copied into writable storage.
4. **Bounded everything.** A line has a maximum length, a field has a maximum
   digit count, a range has a maximum span. Bounded is what makes O(1) honest.
5. **A named failure for every violation.** A manifest compiler that rejects a
   database must be able to say exactly why, because the alternative — a
   silently truncated radical reaching a kernel table — is the failure this
   standard exists to prevent.
6. **Data at rest, held to the same invariants as data in flight.** The SCP
   invariance that SIP and SCSS enforce on live codepoint queries is enforced
   here on the manifest itself.

### 1.3 Non-negotiable invariants

- **SCMS-I1 — Fixed record arity.** A record is exactly
  `SUAS_SCMS_RECORD_FIELD_COUNT` (6) fields. Not "at least", not "at most".
- **SCMS-I2 — Bounded lines.** No line exceeds `SUAS_SCMS_MAX_LINE` (128)
  bytes including its terminator. Every decoder is bounded by this.
- **SCMS-I3 — SCP invariance.** No manifest line may place a codepoint in the
  System Control Plane (`0x00110000–0x0011FFFF`), a trap slot
  (`0x7FFFFFF0–0x7FFFFFFE`), or the Sentinel (`0x7FFFFFFF`) — at either
  endpoint **or in the interior of a range**.
- **SCMS-I4 — No leading whitespace.** A record line begins at byte 0. The
   grammar has no indentation and no continuation.
- **SCMS-I5 — No allocation.** No operation in this standard allocates, and
   no operation retains a pointer beyond the call or the caller's own buffer
   lifetime.
- **SCMS-I6 — Linkage before emission.** A manifest may not be compiled into
   a kernel blob until the schema it feeds is confirmed registered in the
   SSRE cross-reference registry (§6).

### 1.4 Deviations from the reference sketch

1. **Numbering.** The seeding sketch was labelled SUAS-013. It is registered
   as **SUAS-011**, the number immediately after SUAS-010 (SSRE), which keeps
   the SUAS series contiguous with no reserved gaps (§11.1 of SUAS-010).
   SUAS-013 is not used by this specification.
2. **Macro prefix.** `SCMS_*` identifiers were standardised to `SUAS_SCMS_*`,
   matching the SUAS family. Structure names (`scms_manifest_header_t`,
   `scms_record_descriptor_t`, `scms_field_type_t`) and function names
   (`suas_scms_init_engine`, `suas_scms_validate_record`) are preserved
   verbatim.
3. **Patch component.** `SUAS_SCMS_VERSION_PATCH` was added.
4. **Codepoint width.** `start_codepoint` and `end_codepoint` are widened from
   `uint32_t` to `sucs_ex_char_t` (64-bit), consistent with SUAS-009. The
   grammar currently caps `U+` at 8 hex digits, so no value today exceeds 32
   bits; the type is widened so a future ExtSUCS manifest form cannot silently
   truncate.
5. **Additive types.** `scms_line_kind_t`, `scms_error_t`,
   `scms_manifest_state_t`, and `scms_schema_link_t` were added, along with the
   streaming reader, the plane mask, and the SSRE linkage gate. The sketch
   declared a record validator but no way to validate a *manifest*, and no way
   to validate one twice.

The sketch named the offline reference layer "SUCA". SUCA is SUTS-001, the
SuperUnicode Collation Algorithm. The character database is **SUCD** — the
fourth time this collision has been resolved in favour of SUCD, consistently
with SUAS-009, SUAS-010, and SUTS-005.

## 2. Definitions and Notation

**Manifest.**
: A sequence of lines describing character data for compilation into kernel
tables.

**Record.**
: A manifest line describing one codepoint or one contiguous range, in exactly
six tab-delimited columns.

**Directive.**
: A manifest line of the form `%name value`, carrying manifest-level metadata.

**Column.**
: A single tab-delimited field. Columns are numbered from 0.

**Binary alignment.**
: The property of a record whose `start` equals its `end`, so that the record
maps to exactly one kernel table slot rather than a run that must be expanded.

**Streaming state.**
: The caller-allocated `scms_manifest_state_t` that accumulates a manifest
read without allocation.

**Schema link.**
: The `scms_schema_link_t` naming the registered specification a manifest's
output is destined for.

**WSP.**
: A space or a horizontal tab. Permitted between a directive's name and value;
never permitted in a record.

## 3. The Manifest Grammar

### 3.1 Line dispatch

A manifest is a stream of lines. Dispatch is by first byte, and is total:

| First byte | Kind | Meaning |
|------------|------|---------|
| *(length 0)* | `SCMS_LINE_BLANK` | Ignored |
| `#` | `SCMS_LINE_COMMENT` | Ignored through end of line |
| `%` | `SCMS_LINE_DIRECTIVE` | Manifest metadata |
| anything else | `SCMS_LINE_RECORD` | A character record |
| *(over budget / NULL)* | `SCMS_LINE_INVALID` | Rejected |

Because dispatch is a single byte test and a record begins at byte 0, the
grammar has no indentation and no continuation lines (SCMS-I4).

### 3.2 Directives

```
directive := '%' name WSP+ value
name      := 1*16 ALPHA
value     := 1*(%x21-7E)          ; printable, non-space, to end of line
```

Three directive names are normative and are folded into the manifest header:

| Name | Value | Header field |
|------|-------|--------------|
| `manifest` | a version token | `manifest_version` |
| `records` | a decimal count | `total_records` |
| `planes` | `0x` + 1–2 hex digits | `plane_mask` |

An unknown but well-formed directive is **accepted and ignored**, so a manifest
may carry toolchain-private metadata without becoming invalid. A malformed
directive — no name, an empty value, whitespace inside a value, or a name
longer than 16 letters — is `SCMS_ERR_BAD_DIRECTIVE`.

`manifest_version` borrows the caller's line buffer and is never copied. A
build tool keeps the manifest alive for the duration of the compile, which it
does in any case in order to parse it.

### 3.3 The streaming reader

```
manifest := directive* record+
```

A manifest is read through a caller-allocated `scms_manifest_state_t`:

```c
suas_scms_begin(&st);
while (read_line(&buf, &len))
    if (!suas_scms_feed_line(&st, buf, len))
        report(st.first_error_line, st.last_error);
if (!suas_scms_end(&st))
    report(st.last_error);
```

`suas_scms_begin` resets the state. `suas_scms_feed_line` dispatches the line,
validates it, and accumulates counters and the observed plane mask.
`suas_scms_end` closes the manifest and requires that:

1. a `%manifest` directive was seen;
2. at least one record was read;
3. no line failed validation; and
4. where a `%records` directive declared a total, the counted total matches.

`suas_scms_planes_declared` is the separate integrity cross-check: every plane
the records touched must appear in the declared `plane_mask`. A manifest that
omits `%planes` declares nothing, and so can never satisfy it.

## 4. The Record Grammar

### 4.1 Column layout

```
record := start TAB end TAB radical TAB strokes TAB flags TAB weight
```

| # | Name | Form | Bounds | `scms_field_type_t` |
|---|------|------|--------|---------------------|
| 0 | start | `U+` + 1–8 hex | admissible (§4.3) | `SCMS_FIELD_CODEPOINT` |
| 1 | end | `U+` + 1–8 hex | admissible, ≥ start | `SCMS_FIELD_CODEPOINT` |
| 2 | radical | decimal | 0 … `SUAS_SCMS_RADICAL_MAX` (214) | `SCMS_FIELD_RADICAL` |
| 3 | strokes | decimal | 0 … `SUAS_SCMS_STROKES_MAX` (255) | `SCMS_FIELD_STROKES` |
| 4 | flags | `0x` + 1–4 hex | 0 … `SUAS_SCMS_FLAGS_MAX` (0xFFFF) | `SCMS_FIELD_FLAGS` |
| 5 | weight | decimal | 0 … `SUAS_SCMS_WEIGHT_MAX` (0xFFFFFFFF) | `SCMS_FIELD_WEIGHT` |

`scms_field_type_t` names the five *kinds of value*, not the six columns:
columns 0 and 1 are both `SCMS_FIELD_CODEPOINT`, the start and end of the
described range.

Kangxi radicals run 1–214; 0 means the record carries no radical. A single
codepoint is expressed as `end == start`.

### 4.2 Field forms

Every field is one of exactly three forms, and each is decoded by a bounded
loop that stops at the field's own length:

- **Codepoint** — `'U'` or `'u'`, `'+'`, then 1 to `SUAS_SCMS_CODEPOINT_HEX_MAX`
  (8) hex digits, then the tab or the end of line. A ninth digit is a
  violation, not a truncation.
- **Hex** — `'0'`, `'x'` or `'X'`, then 1 to `SUAS_SCMS_FLAGS_HEX_MAX` (4) hex
  digits.
- **Decimal** — 1 to `SUAS_SCMS_DECIMAL_MAX` (10) decimal digits, accumulated
  in 64 bits and rejected on overflow before it can wrap.

An empty field is `SCMS_ERR_EMPTY_FIELD`. A field with trailing content after
its final digit is a violation of its own form. There is no quoting, no
escaping, and no default: a field is present and well-formed, or the line is
rejected.

### 4.3 Range validation

Range checks run in a fixed order, so the reported outcome names the first
thing that is wrong:

1. **Ordering.** `start ≤ end`, else `SCMS_ERR_RANGE_ORDER`.
2. **Endpoint admissibility.** Both endpoints must be admissible, else
   `SCMS_ERR_INADMISSIBLE_CODEPOINT`.
3. **Interior admissibility.** No inadmissible zone may lie within the range,
   else `SCMS_ERR_INADMISSIBLE_CODEPOINT`.
4. **Span.** `end − start ≤ SUAS_SCMS_RANGE_MAX_SPAN` (0x10FFFF), else
   `SCMS_ERR_RANGE_TOO_WIDE`.

Step 3 is the step a naive implementation omits, and it is the one that
matters. A range from `U+10FFFF` to `U+00120000` has two perfectly admissible
endpoints and swallows the entire System Control Plane in between. A manifest
range is a promise that *every* codepoint it covers is data; a range is
admissible only when that promise holds throughout.

A codepoint is admissible when it is at or below the last allocatable Native
codepoint (`0x7FFFFFEF`) and outside the Control Plane. Everything above the
Native plane is a trap slot or the Sentinel, and none of it is data.

### 4.4 The record descriptor

```c
typedef struct {
    sucs_ex_char_t start_codepoint;
    sucs_ex_char_t end_codepoint;      /* equal to start for one codepoint */
    uint16_t       field_count;        /* fields actually found, not expected */
    bool           is_binary_aligned;  /* true iff the record is one codepoint */
} scms_record_descriptor_t;
```

The descriptor reports *structure*, not payload: the range resolved, how many
columns were found, and whether the record maps to a single table slot. A
compiler walks the payload columns itself through `suas_scms_field_at`, which
decodes column *n* on demand. This keeps the descriptor fixed-size and keeps
one code path responsible for each field's bounds.

`is_binary_aligned` is the compilation decision the descriptor exists to carry.
A single codepoint compiles to one entry; a range compiles to a run that the
tool must expand.

## 5. Validation Outcomes

Every rejection carries its own outcome code. The reference implementation
reports the most recent one through `suas_scms_last_error()`, and
`suas_scms_error_name` renders it for a diagnostic.

| Outcome | Cause |
|---------|-------|
| `SCMS_ERR_NULL_INPUT` | A NULL line, state, or out-parameter |
| `SCMS_ERR_LINE_TOO_LONG` | The line reaches `SUAS_SCMS_MAX_LINE` |
| `SCMS_ERR_EMPTY_FIELD` | A zero-length column |
| `SCMS_ERR_BAD_FIELD_COUNT` | Not exactly six columns |
| `SCMS_ERR_BAD_CODEPOINT` | A `U+` column is malformed |
| `SCMS_ERR_INADMISSIBLE_CODEPOINT` | SCP, trap slot, or Sentinel |
| `SCMS_ERR_RANGE_ORDER` | `start > end` |
| `SCMS_ERR_RANGE_TOO_WIDE` | Span exceeds the ceiling |
| `SCMS_ERR_BAD_RADICAL` | Radical out of bounds or not decimal |
| `SCMS_ERR_BAD_STROKES` | Stroke count out of bounds or not decimal |
| `SCMS_ERR_BAD_FLAGS` | Flag word malformed or out of bounds |
| `SCMS_ERR_BAD_WEIGHT` | Weight malformed, out of bounds, or overflowing |
| `SCMS_ERR_BAD_DIRECTIVE` | A directive is malformed |
| `SCMS_ERR_MISSING_HEADER` | No `%manifest` directive |
| `SCMS_ERR_RECORD_COUNT_MISMATCH` | Count disagrees with `%records` |
| `SCMS_ERR_EMPTY_MANIFEST` | A header and no records |

The outcome code is part of the standard, not an implementation detail: a
build system that reports `SCMS_ERR_BAD_RADICAL` for a line whose radical is
in fact correct is defective, and the reference test suite pins each
violation to its own code.

## 6. The SUAS-010 (SSRE) Linkage Gate

A manifest is meaningless in isolation. It exists to feed a target schema —
SIP's property table, SCSS's security table — and that schema is itself a
registered specification with a version and a cross-reference kind. SCMS
validates the manifest's structure; SUAS-010 (SSRE) validates that the
destination exists and is satisfiable.

```c
typedef struct {
    const char *schema_spec_id;  /* e.g. "SUAS-009" for a SIP table */
    uint8_t     schema_major;
    uint8_t     schema_minor;
    bool        schema_mandatory;
} scms_schema_link_t;
```

`suas_scms_verify_schema_link` forwards the link to
`suas_ssre_verify_dependency` and reports the result in SCMS's own vocabulary.
Three cases are worth stating explicitly:

- A link naming an **unregistered** specification fails. A blob whose schema
  nobody can resolve must not be embedded.
- A link asserting `schema_mandatory` against an **optional SUTS data
  specification** fails, per SSRE's mandatory-implies-MANDATORY rule. A
  manifest that treats an optional data spec as architecturally binding has
  mislabelled the architecture.
- A link whose version floor is **above the registered version** fails.

The gate is the mechanism by which SCMS and SSRE divide the work: structure
here, linkage there, both required before emission (SCMS-I6).

## 7. Complexity and Memory

| Operation | Cost |
|-----------|------|
| `suas_scms_line_kind` | O(1) — one byte test after the budget check |
| `suas_scms_validate_record` | O(len), len ≤ 128 — bounded, hence O(1) |
| `suas_scms_field_at` | O(len) |
| `suas_scms_validate_directive` | O(len) |
| `suas_scms_feed_line` | O(len) |
| `suas_scms_end`, `_planes_declared` | O(1) |
| `suas_scms_codepoint_admissible`, `_range_admissible`, `_plane_mask` | O(1) |
| `suas_scms_verify_schema_link` | O(1) expected — one SSRE lookup |

Memory is caller-owned throughout: a line buffer of at most 128 bytes, a
descriptor, and a streaming state. Nothing in this standard allocates, and
nothing retains a pointer past the call except `manifest_version`, which
borrows a buffer the caller already owns.

## 8. Data Files

| File | Form | Role |
|------|------|------|
| `sucd/*.scms` | Text manifest | The offline reference layer: the character database itself |
| Kernel image | `suas_glyph_prop_t[]`, `scss_security_attr_t[]`, … | The runtime layer, compiled from the manifest |

The manifest is the authority and the kernel table is derived from it. A change
to a character database is a change to a manifest, and a manifest that does not
validate produces no table at all.

## 9. Implementation Notes

### 9.1 Compiling a manifest

```c
static const scms_schema_link_t sip_link = {
    "SUAS-009", 1u, 0u, true
};

scms_manifest_state_t st;
scms_record_descriptor_t desc;

if (!suas_scms_verify_schema_link(&sip_link))
    return SCMS_ABORT;               /* the schema is not registered  */

suas_scms_begin(&st);
while (read_line(&buf, &len)) {
    if (!suas_scms_feed_line(&st, buf, len)) {
        fprintf(stderr, "line %u: %s\n", st.first_error_line,
                suas_scms_error_name((scms_error_t)st.last_error));
        return SCMS_ABORT;
    }
    if (suas_scms_line_kind(buf, len) == SCMS_LINE_RECORD &&
        suas_scms_validate_record(buf, len, &desc)) {
        if (suas_scms_is_binary_aligned(&desc))
            emit_one_entry(&desc);
        else
            emit_expanded_run(&desc);
    }
}
```

### 9.2 Reading a payload column

```c
uint32_t radical;
if (suas_scms_field_at(buf, len, 2u, &radical))
    entry.radical = (uint16_t)radical;
```

### 9.3 Enforcing the plane budget

```c
if (!suas_scms_planes_declared(&st))
    return SCMS_ABORT;   /* records outside the declared address space */
```

## 10. Testing

The reference implementation is exercised by `tests/test_suas_scms.c`,
covering: version and meta constants and the descriptor's fixed layout; line
classification including the invalid and over-budget cases; the record grammar
on its happy path (single codepoints, ranges, lowercase forms, RNUR
neography); **every documented outcome code**, each pinned to the specific
violation that produces it; column extraction across all six columns and its
own failure surface; the directive form including toolchain-private names; the
streaming reader's completeness checks, error line attribution, and the plane
cross-check; plane admissibility, the plane mask, and interior range
crossing; outcome naming; the SSRE linkage gate across its four failure modes;
and confirmation that SCMS is itself registered in the SSRE registry.

## 11. Stability and Versioning

`SUAS_SCMS_SPEC_ID` is `"SUAS-011"`, `SUAS_SCMS_VERSION_MAJOR` is 1, and
`SUAS_SCMS_VERSION_MINOR` is 0. The version identifies the grammar and the
validation protocol. Adding an outcome code or a directive name is a
minor-level change. Removing a column, changing the record arity, relaxing a
bound, or removing an admissibility rule is a major change, because a manifest
that compiled before may compile to a different table afterwards.

## 12. Conformance Requirements

An implementation conforms to SUAS-011 when:

1. it implements the line dispatch of §3.1 and the record grammar of §4
   exactly, including the six-column arity and the three field forms;
2. every rejection maps to the outcome code §5 assigns to it;
3. range validation checks the interior of a range and not merely its
   endpoints (SCMS-I3);
4. every operation is bounded by `SUAS_SCMS_MAX_LINE` and none allocates;
5. it exposes the SUAS-010 linkage gate and refuses to compile a manifest whose
   schema link does not verify (SCMS-I6);
6. the streaming reader attributes the first error to its line number.

### 12.1 Compliance matrix (reference implementation)

| Requirement | Location | Status |
|-------------|----------|--------|
| R1 — Line dispatch and record grammar | `suas_scms_line_kind`, `suas_scms_validate_record` | ✔ |
| R2 — One outcome per violation | `scms_error_t`, `suas_scms_error_name` | ✔ |
| R3 — Interior range checking | `suas_scms_range_admissible` | ✔ |
| R4 — Bounded, zero-allocation | whole module | ✔ |
| R5 — SSRE linkage gate | `suas_scms_verify_schema_link` | ✔ |
| R6 — Streaming reader | `suas_scms_begin` / `_feed_line` / `_end` | ✔ |

## 13. Terms and Conventions

**SCMS** — System Character Manifest & Schema Standard; this standard.

**SUCD** — SuperUnicode Character Database; the offline reference layer
holding the manifests. (Not SUCA, SUTS-001, the SuperUnicode Collation
Algorithm.)

**Manifest** — A stream of lines describing character data.

**Record** — A line describing one codepoint or one range, in six
tab-delimited columns.

**Directive** — A `%name value` line carrying manifest metadata.

**Column** — One tab-delimited field, numbered from 0.

**Binary alignment** — The property of a record covering exactly one
codepoint.

**Streaming state** — The caller-allocated accumulator for a manifest read.

**Schema link** — The registered specification a manifest's output feeds.
