# SUAS-008 — System Named Character Sequences (SNCS)

**Standard ID:** SUAS-008
**Status:** Draft — Ratified
**Scope:** Core Kernel Invariant
**Applies To:** SUCD, SUTR-5 (RNUR / RCCR codepoint registry), SUAS-004 (SUCF), SUAS-007 (SIS), the OpenWindows shell, the SUF font engine, the System Control Plane
**Last Updated:** 2026-10-18

---

## 1. Overview

System Named Character Sequences (SNCS) is the core kernel architecture governing
the **deterministic parsing, static packing, and O(1) resolution of
multi-codepoint symbol names** across the SuperUnicode (SUCS) codepoint space. It
gives a *name* to a *sequence* of codepoints, so that a symbol which is not a
single codepoint — a combining cluster, a ligature, a flag emoji, or a neography
logograph — can be referred to, looked up, and rendered by one stable token.

SNCS is the multi-codepoint successor to the Unicode named character sequences
specification (UAX #34). Where UAX #34 defines named sequences for standard
multi-character symbols and emoji flag sequences through dynamic string tables,
SNCS binds the same concept to a **zero-allocation, static lookup table**
optimized for the OpenWindows shell and the font rendering engine (`suf`).

SNCS is not an application feature. It is a kernel invariant: the shell's symbol
expansion, the diagnostic scripts, and the font glyph resolver all depend on the
fact that a name — a compound glyph handle, a cluster token, a ligature name —
resolves to the same codepoint sequence, everywhere, in constant time.

SNCS operates over the SuperUnicode space (`0x000000`–`0x7FFFFFFF`):

| Zone | Range | Sequence treatment |
|------|-------|--------------------|
| Unicode Bridge | `0x00000000`–`0x0010FFFF` | Bridge sequences mapped from the Unicode registry |
| System Control Plane | `0x00110000`–`0x0011FFFF` | **Strictly inadmissible** inside a sequence |
| Native plane | `0x00120000`–`0x7FFFFFEF` | Admitted (RNUR / RCCR neography) |
| Trap slots | `0x7FFFFFF0`–`0x7FFFFFFE` | Inadmissible |
| Sentinel | `0x7FFFFFFF` | Inadmissible |
| extSUCS plugin plane | above `0x7FFFFFFF` | Admitted (mounted plugin codepoints) |

### 1.1 Architectural shifts from UAX #34

| Dimension | UAX #34 | SUAS-008 SNCS |
|-----------|---------|----------------|
| Store | Dynamic string tables (`NamedSequences.txt`) | Static, compile-time token array |
| Resolution | Table scan / string matching | O(1) hash-indexed lookup |
| Memory | Implementation-defined | Zero allocation; fixed-size stack token |
| Scope | 21-bit Unicode scalar values | Full SUCS space (`0x000000`–`0x7FFFFFFF`) |
| Neography | Not represented | Native RNUR compound glyphs / logographs |
| Length | Arbitrary | Bounded by `SUAS_SNCS_MAX_SEQUENCE_LEN` (8) |
| Name namespace | Shared with Unicode character names | SUCS namespace for names (`SNCS-D3`) |
| Consumers | Cross-mapping to other standards | OpenWindows shell, SUF font engine |
| RCCR role | n/a | Out-of-band metadata/subtag registry; allocates no codepoints |

SNCS is a **resolver**, not a rendering engine. It answers "what sequence does
this name denote?" and nothing more; it never rasterizes, shapes, or normalizes.
Normalization, if any, is the caller's responsibility (see SUAS-004, SUCF).

### 1.2 Goals

- **Determinism.** The same name always resolves to the same sequence,
  independent of platform, locale, or threading.
- **Constant time.** A resolution performs a bounded number of hash and
  comparison steps regardless of the registry size.
- **Zero allocation.** The registry is a static table and the index is a
  fixed-size array; a resolved token is a stack struct.
- **Freestanding.** No C library, no floating point, no heap; C99 freestanding
  subset only.
- **Total.** Every name and every codepoint produces a defined answer.

### 1.3 Non-negotiable invariants

1. **Name uniqueness.** No two distinct sequences in the registry share a name
   in the SUCS namespace for names (`SNCS-R2`, `SNCS-R3`).
2. **SCP invariance.** No codepoint in `0x00110000`–`0x0011FFFF` is ever part of
   a named sequence, and no trap slot or the Sentinel is either (§2.4).
3. **Minimum length.** A named character sequence is two or more codepoints
   (`SNCS-D1`).
4. **Idempotent setup.** `suas_sncs_init_registry` may be called any number of
   times; the index is always the same afterwards.
5. **Static capacity.** The number of registered sequences never exceeds
   `SUAS_SNCS_REGISTRY_CAPACITY`; the index never grows at run time.

---

## 2. Definitions and Notation

### 2.1 Definitions

```
SNCS-D1. Named character sequence: a specific sequence of two or more
         codepoints, together with a formal name designating that sequence.

SNCS-D2. Namespace: a set of strings used to label entities in some context,
         together with the rules that specify how the strings are constructed
         and a uniqueness rule that determines when candidate strings are
         unique within the set.

SNCS-D3. SUCS namespace for names: the namespace consisting of sequence names
         over the full SuperUnicode space. Its Bridge part is the analog of the
         Unicode namespace for character names; its Native part is the RNUR
         neography name registry.
```

> **Curation note.** As with UAX #34, it is neither possible nor desirable to
> name every conceivable sequence. SNCS admits a deliberately selective, curated
> set: registered Bridge sequences plus RNUR-registered neography compounds.

### 2.2 Notational conventions

A named character sequence is written as a **USI** (Unicode Sequence Identifier
analog) — angle brackets around a comma-delimited list of codepoints — followed
directly by the name:

```
<U+012B, U+0300> LATIN SMALL LETTER I WITH MACRON AND GRAVE
<U+0061, U+0302, U+0301> LATIN SMALL LETTER A WITH CIRCUMFLEX AND ACUTE
<U+17D2, U+1780> KHMER CONSONANT SIGN COENG KA
```

In data tables the brackets and comma delimiters may be dropped, using
space-delimited codepoints and a separate name field:

```
TAMIL CONSONANT K; 0B95 0BCD
```

A USI must contain at least two codepoints. The UID forms may be `U+0061` or
`0061`; SNCS tables use the bare hexadecimal form.

### 2.3 The static token

A resolved sequence is a fixed, stack-allocatable token (`suas_named_sequence_t`,
§6.1). It carries the name, a packed codepoint array, and a length. There is no
pointer into shared mutable storage and no heap.

### 2.4 SCP invariance

The System Control Plane block `0x00110000`–`0x0011FFFF` is strictly
inadmissible inside a named sequence, as are the trap slots
(`0x7FFFFFF0`–`0x7FFFFFFE`) and the Sentinel (`0x7FFFFFFF`). This keeps the name
namespace disjoint from the in-band control instructions of SBR, the math and
segmentation directives, the SDF directional block, the SSP script directives,
and the BANcodes.

### 2.5 Bridge and Native sequences

- **Bridge sequences** draw their codepoints from `0x00000000`–`0x0010FFFF` and
  are mapped from the Unicode named sequence registry (for example the Khmer
  vowel and consonant signs, the Latin diacritic clusters, and the Ainu katakana
  ligature).
- **Native & neography sequences** draw their codepoints from the RNUR carve-out
  `0x00160000`–`0x0016FFFF` (inside the Native plane), letting a conlang expose
  compound glyphs, ligatures, and multi-codepoint logographs under unique string
  handles. RCCR functions separately as an out-of-band metadata/subtag registry
  for language identification and **allocates no codepoints**; it therefore
  contributes no SNCS sequences.

A sequence's registry is derived from its codepoints: if any codepoint lies in
the Native plane the sequence is Native, otherwise it is Bridge
(`suas_sncs_registry_of`).

---

## 3. Names

Names of named character sequences are unique. They are part of the SUCS
namespace for names (`SNCS-D3`). Bridge names are constructed according to the
following rules, adapted from UAX #34:

```
SNCS-R1. Only LATIN CAPITAL LETTER A-Z, digits 0-9 (provided that a digit is
         not the first character in a word), SPACE, and HYPHEN-MINUS are used
         for writing the names.

SNCS-R2. Only one name is given to each named character sequence, and each
         named character sequence is unique within the SUCS namespace for names.

SNCS-R3. Names are unique in the namespace if they are different even when
         SPACE and medial HYPHEN-MINUS are ignored, and when the strings
         "LETTER", "CHARACTER", and "DIGIT" are ignored in the comparison.
```

### 3.1 Medial hyphens

`SNCS-R3` ignores only **medial** HYPHEN-MINUS: a hyphen that occurs directly
after an alphanumeric character and directly before one. An initial or final
hyphen is significant, so the following two names remain distinct in the
namespace:

```
TIBETAN LETTER A
TIBETAN LETTER -A
```

### 3.2 Non-uniqueness example

The following two hypothetical names would not be unique by `SNCS-R3`:

```
SARATI LETTER AA
SARATI CHARACTER AA
```

because they differ only by the ignorable substrings `LETTER` and `CHARACTER`.

### 3.3 Construction guidance

```
SNCS-R4. Where possible, names are constructed by appending the names of the
         constituent elements together while eliding duplicate elements, and
         possibly introducing the words "WITH" or "AND" between elements for
         clarity. Should this produce a name that already exists, the name is
         modified suitably to guarantee uniqueness.

SNCS-R5. Where applicable, the character naming guidelines of Annex L of
         ISO/IEC 10646 apply.
```

Names for Native sequences follow the same rules, conventionally prefixed with
the owning registry token (`RNUR …`), so that the neography name space remains
disjoint from the Bridge name space.

> **Informal names.** Conformance to `SNCS-C1` does not prevent a process from
> providing informal names of its own to arbitrary sequences. Such informal
> names are not specified by this standard for interchange.

---

## 4. Conformance

```
SNCS-C1. A process that purports to implement SUCS named character sequences
         shall resolve only those sequences defined in the normative SNCS
         registry (§8), using the names exactly as registered.
```

Only the sequences in the normative registry are named by this standard. No
other SUCS sequences are given names in this version, although sequences may be
added in the future. As with Unicode, only sequences that are in Normalization
Form NFC are given Bridge names.

### 4.1 Provisional process

When a new named sequence is first suggested, it may be accepted **provisionally**
into a pending list, mirroring `NamedSequencesProv.txt`. Provisional entries have
no normative status; they become part of the standard only when promoted into the
normative registry. The provisional stage allows review before the normative
registry — which is the stable, compile-time table — is changed.

### 4.2 Stability of approved named character sequences

Once a named character sequence is approved, it is stable: neither the name nor
the associated codepoint sequence changes in a future version. This mirrors the
Unicode Consortium stability policies for named character sequences.

---

## 5. The Dual-Registry Index

SNCS keeps a single logical index with two logical registries.

### 5.1 Bridge registry

The Bridge registry mirrors the Unicode named sequence list. It covers the
Latin diacritic clusters, the modifier-letter contour tone bars, the Ainu
katakana ligature, the Khmer signs, the Indic nukta forms, and the emoji flag
and ZWJ sequences.

### 5.2 Native / neography registry

The Native registry carries RNUR neography compound sequences from the carve-out
`0x00160000`–`0x0016FFFF`: ligatures, compound signs, diphthongs, trigraphs, and
logographs. RCCR supplies language-identification subtags out of band and is not
queried for codepoints.

### 5.3 Disjointness

The Bridge and Native codepoint ranges are disjoint, and the name spaces are
disjoint by construction (`RNUR …` vs. the Bridge conventions). A sequence is
classified as Native iff any of its codepoints is a Native codepoint.

---

## 6. Static Packing and O(1) Resolution

### 6.1 The token

```c
#define SUAS_SNCS_MAX_SEQUENCE_LEN 8U
#define SUAS_SNCS_MAX_NAME_LEN     64U

typedef struct {
    char           name[SUAS_SNCS_MAX_NAME_LEN];
    sucs_ex_char_t codepoints[SUAS_SNCS_MAX_SEQUENCE_LEN];
    uint8_t        length;
} suas_named_sequence_t;
```

The token is fixed-size and stack-allocatable. Codepoints are 64-bit
(`sucs_ex_char_t`), so one token spans the Bridge, the SCP, the Native plane, and
the unbounded extSUCS plugin space uniformly. The name limit (64 bytes,
including the terminating NUL) accommodates the longest Unicode named sequence
name; the sketch that seeded this standard proposed 32 bytes, which truncates
most Bridge names and is therefore raised here. A sequence is always two or more
codepoints (`SNCS-D1`).

### 6.2 The index

The registry is a static array `sncs_table[]` of at most
`SUAS_SNCS_REGISTRY_CAPACITY` (64) tokens. A second fixed array of
`SUAS_SNCS_INDEX_SIZE` (128) slots is an open-addressing hash index:

1. `suas_sncs_init_registry` hashes each registered name (FNV-1a, 32-bit) and
   places its table offset in the first free slot, resolving collisions by
   linear probing. The build is O(n) once and touches no heap.
2. `suas_sncs_resolve_sequence` hashes the query name and probes the same
   sequence of slots until it matches or hits an empty slot.

Because the load factor is at most one-half over a bounded, static table, the
number of probes is bounded by a small constant. Resolution is O(1) in the
registry size. Resolution builds the index lazily on first use, so it is correct
even if `suas_sncs_init_registry` was not called.

### 6.3 Complexity

| Operation | Cost |
|-----------|------|
| `suas_sncs_init_registry` | O(n) once, n ≤ 64, no allocation |
| `suas_sncs_resolve_sequence` | O(1) expected (bounded probes) |
| `suas_sncs_registry_count` | O(1) |
| `suas_sncs_sequence_at` | O(1) |
| `suas_sncs_registry_of` | O(length), length ≤ 8 |
| `suas_sncs_names_equal` | O(name length), bounded by 2 × 64 bytes |

---

## 7. Kernel Shell and Font Integration

SNCS provides fast symbol resolution for the OpenWindows terminal shell and for
the `suf` font engine:

- **Shell expansion.** A user or a diagnostic script references a complex symbol
  or script cluster by its exact name token (`<USI>` plus name, or the bare
  name). The shell calls `suas_sncs_resolve_sequence` and receives the codepoint
  sequence in O(1), without string parsing overhead beyond the lookup itself.
- **Font glyph lookup.** Given a cluster the renderer has assembled, the font
  engine can classify it (`suas_sncs_registry_of`) and reach the sequence by the
  same static index, so Bridge compatibility clusters and Native neography
  logographs resolve through one path.

The shell is expected to delimit a name token before calling the resolver; the
resolver matches the name exactly, byte for byte, against the canonical
(upper-case) registered name.

---

## 8. Data Files

The normative registry is the compiled static table
`sncs_table[]` in `src/suas/suas_sncs.c`. It is the single source of truth for
resolvable names and their codepoint sequences. The analogous on-disk form,
should one be materialized, uses space-delimited codepoints and a
semicolon-delimited name field:

```
TAMIL CONSONANT K; 0B95 0BCD
KHMER VOWEL SIGN OM; 17BB 17C6
```

A provisional list may be maintained separately (§4.1) and carries no normative
status until promoted. Implementations may use other formats for this data, as
appropriate; the key information is the name and its associated codepoint
sequence.

---

## 9. Implementation Notes

### 9.1 Resolving

```c
suas_named_sequence_t seq;
if (suas_sncs_resolve_sequence("KHMER VOWEL SIGN OM", &seq)) {
    /* seq.codepoints[0] == 0x17BB, seq.codepoints[1] == 0x17C6 */
}
```

`suas_sncs_resolve_sequence` is O(1): it performs a bounded hash and probe
sequence and no allocation.

### 9.2 Enumerating

```c
uint32_t i, n = suas_sncs_registry_count();
suas_named_sequence_t seq;
for (i = 0; i < n; i++) {
    suas_sncs_sequence_at(i, &seq);
    /* seq.name, seq.codepoints, seq.length */
}
```

### 9.3 Validating

`suas_sncs_is_valid_name` checks `SNCS-R1`; `suas_sncs_names_equal` implements
the `SNCS-R3` uniqueness-ignoring comparison; `suas_sncs_is_well_formed` checks a
token's name, length, and the admissibility of its codepoints.

---

## 10. Testing

Conformance is demonstrated by a host test that exercises the registry and the
name rules. The reference suite covers, at minimum:

- **Resolution:** known Bridge sequences of length 2, 3, 5, and 7 (including an
  emoji tag sequence); a Native RNUR sequence; an exact-miss; NULL guards.
- **Enumeration:** the registry count, per-entry well-formedness, and the
  presence of both Bridge and Native entries; out-of-range and NULL guards.
- **Zone predicates:** Bridge, Native-plane, RNUR carve-out, and plugin-plane
  codepoints; rejection of the SCP, trap slots, and the Sentinel.
- **Names:** valid names (including a hyphen-led word); invalid lower-case,
  underscore, tab, empty, NULL, and digit-first names; `SNCS-R3` equality across
  SPACE, medial hyphen, and the ignorable words `LETTER` / `CHARACTER` / `DIGIT`.
- **Well-formedness:** minimum length two; rejection of SCP, trap, and Sentinel
  codepoints; NULL guard.
- **Comparison:** codepoint-list equality ignoring the name.

---

## 11. Stability and Versioning

- The token layout, the registry/index sizes, and the registry, and the
  resolution contract are **stable** within a major version.
- New named sequences may be **added** to the registry; existing names and their
  codepoint sequences never change (`SNCS-C1`, §4.2).
- The curated Bridge table tracks the Unicode Bridge; a bridge-data update is a
  **minor** revision and does not change the API.

Current version: **1.0.0** (`SUAS_SNCS_SPEC_ID` = `"SUAS-008"`).

### 11.1 Version-domain introduction

At the time of writing, `SUAS-008` (SNCS) is introduced alongside the ratified
SUAS family (SUAS-001 … SUAS-007) as a **1.0** core invariant. The registry it
ships is the initial Bridge + RNUR set; additions are minor revisions.

---

## 12. Conformance Requirements

An implementation claims conformance by identifying the SNCS version it
implements (`SNCS-C1`) and by declaring which of the following it observes:

| Requirement | Subject |
|-------------|---------|
| `SNCS-C1` | Use only the normative registry and its exact names |
| `SNCS-D1` | Named character sequences are two or more codepoints |
| `SNCS-R1` | Name character set and digit placement |
| `SNCS-R2` | One name per sequence; unique within the namespace |
| `SNCS-R3` | Uniqueness under SPACE / medial hyphen / ignorable-word folding |
| `SNCS-R4` | Name construction guidance |
| `SNCS-R5` | ISO/IEC 10646 Annex L naming guidelines |
| §2.4 | SCP / trap / Sentinel invariance |
| §6 | Static packing and O(1) resolution |

### 12.1 Compliance matrix (reference implementation)

| Requirement | Reference | Test |
|-------------|-----------|------|
| Version identification (`SNCS-C1`) | §11 | `test_version_and_meta` |
| Bridge resolution | §5.1 | `test_resolve_bridge` |
| Native resolution | §5.2 | `test_resolve_native` |
| Lazy / idempotent index build | §6.2 | `test_init_idempotent` |
| Miss and guard behaviour | §6.2 | `test_resolve_miss_and_guards` |
| Enumeration and dual registry | §5, §6 | `test_registry_enumeration` |
| Zone and SCP invariance | §2.4, §2.5 | `test_zone_predicates` |
| Name rules (`SNCS-R1`, `R3`) | §3 | `test_names` |
| Minimum length and admissibility (`SNCS-D1`, §2.4) | §2.4, §6.1 | `test_wellformed_and_compare` |
| Sequence comparison | §6.1 | `test_wellformed_and_compare` |

---

## 13. Terms and Conventions

- **Named character sequence** — a name plus a sequence of two or more
  codepoints (`SNCS-D1`).
- **USI** — a comma-delimited, angle-bracketed sequence of codepoints
  (`<U+0061, U+0302, U+0301>`); the Unicode Sequence Identifier analog.
- **Bridge sequence** — a sequence whose codepoints lie in
  `0x00000000`–`0x0010FFFF`, mapped from the Unicode registry.
- **Native / neography sequence** — a sequence with at least one codepoint in
  the Native plane, supplied by the RNUR registry.
- **RCCR** — the out-of-band metadata/subtag registry for language
  identification; allocates no codepoints and contributes no SNCS sequences.
- **Token** — the fixed-size, stack-allocatable `suas_named_sequence_t`.
- **Registry index** — the fixed-size open-addressing array mapping names to
  tokens in O(1).
- **SCP marker** — a codepoint in `0x00110000`–`0x0011FFFF`; always inadmissible
  inside a sequence.

**END OF SUAS-008**
