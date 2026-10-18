# SUAS-007 — System Identifiers and Syntax (SIS)

**Standard ID:** SUAS-007
**Status:** Draft — Ratified
**Scope:** Core Kernel Invariant
**Applies To:** SUCD, SUAS-001 (SDF), SUAS-003 (SBR), SUAS-005 (SSP), SUAS-006 (STS), the OpenWindows shell, configuration parsers, the nVAI scripting runtime, the System Control Plane
**Last Updated:** 2026-10-18

---

## 1. Overview

System Identifiers and Syntax (SIS) is the **core architecture governing
identifier parsing and lexical syntax** in SuperUnicode. It defines the two
structural identifier classes — **`ID_Start`** and **`ID_Continue`** — and the
disjoint lexical categories — **whitespace**, **syntax**, and **ignorable
format controls** — over the full SuperUnicode (SUCS) codepoint space, and
binds them to a single **single-pass, zero-allocation** lexical filter.

SIS is the structural successor to the Unicode identifier recommendations
(UAX #31). Where UAX #31 defines identifiers through regular-expression-like
pattern rules and property lookups over the 21-bit Unicode planes, SIS provides
a **deterministic lexical contract**: a caller queries the class of any
codepoint in O(1), or streams codepoints one at a time through a fixed-size
state machine, and receives a definitive yes/no for identifier membership
without any dynamic allocation, regular-expression engine, or multi-pass scan.

SIS is not an application feature. It is a kernel invariant: the OpenWindows
shell's symbol table, the configuration parsers, and the nVAI scripting runtime
all depend on the fact that an identifier — a kernel command, a variable name, a
conlang symbol — is resolved identically, everywhere, in constant time. SIS is
also the lexical front end for anything that must tell identifiers, whitespace,
and operators apart before higher-level parsing.

SIS operates over the SuperUnicode space (`0x000000`–`0x7FFFFFFF`):

| Zone | Range | Identifier treatment |
|------|-------|----------------------|
| Unicode Bridge | `0x00000000`–`0x0010FFFF` | UAX #31 classes via curated SUCD-sourced predicates |
| System Control Plane | `0x00110000`–`0x0011FFFF` | **Strictly invalid** inside identifiers |
| Native plane | `0x00120000`–`0x7FFFFFEF` | Admitted (RNUR / RCCR neography) |
| Trap slots | `0x7FFFFFF0`–`0x7FFFFFFE` | Invalid |
| Sentinel | `0x7FFFFFFF` | Invalid |
| extSUCS plugin plane | above `0x7FFFFFFF` | Admitted (mounted plugin characters) |

### 1.1 Architectural shifts from UAX #31

| Dimension | UAX #31 | SUAS-007 SIS |
|-----------|---------|--------------|
| Execution model | Multi-pass regex classification rules | Single-pass lexical filter, O(1) per codepoint |
| Scope | 21-bit Unicode scalar values | Full SUCS space (`0x000000`–`0x7FFFFFFF`) |
| Neography | Standard script property lookups | Native RNUR / RCCR conlang identifier codepoints |
| Control handling | Whitespace and BOM handling | Strict rejection of System Control Plane tokens |
| Class model | `ID_Start` / `ID_Continue` property sets | `SUAS_SIS_CLASS_START` / `CONTINUE` predicate |
| Syntax split | `Pattern_White_Space` / `Pattern_Syntax` | `suas_sis_get_syntax` category |
| State | Implicit, in the pattern engine | Explicit, in `suas_sis_state_t` |
| Memory | Implementation-defined | Zero allocation; fixed-size stack struct |
| Start predicate | `is_start(c)` | `suas_sis_is_start(c)` / `is_continue(c)` |
| Customization | Profiles and tailoring | Profile parameter on `..._ex` entry points |

SIS is a **lexical filter**, not a parser: it answers membership questions and
never builds a symbol table, interns a string, or performs case folding or
normalization. Those are the caller's responsibility (§5).

### 1.2 Goals

- **Determinism.** The same codepoint always resolves to the same class,
  independent of platform, locale, or threading.
- **Constant time.** A class query or a `suas_sis_feed` step performs a bounded
  number of integer comparisons regardless of position.
- **Zero allocation.** The entire state machine is two scalar fields the caller
  owns on the stack or in a task control block.
- **Freestanding.** No C library, no floating point, no heap; C99 freestanding
  subset only.
- **Total.** Every 64-bit input — including SCP markers, BANcodes, trap slots,
  and unregistered Native codepoints — produces a defined class.

### 1.3 Non-negotiable invariants

1. **Disjointness.** Every codepoint resolves to exactly one identifier class
   (`START`, `CONTINUE`, or `INVALID`) and to exactly one syntax category
   (§4). The identifier, whitespace, syntax, and ignorable sets are pairwise
   disjoint.
2. **SCP invariance.** No codepoint in `0x00110000`–`0x0011FFFF` is ever a
   `START` or `CONTINUE` character, under any profile.
3. **Streaming faithfulness.** The accept/reject sequence produced by feeding a
   codepoint stream equals the sequence produced by classifying each codepoint
   in isolation; the state machine adds only the start/continue position rule.
4. **Idempotent setup.** `suas_sis_init` followed by the same stream always
   yields the same result, regardless of prior use of the struct.
5. **No reinterpretation.** A rejected codepoint is never counted toward an
   identifier (§8.2).

---

## 2. Identifier Classes and Rules

### 2.1 Class value domain

`suas_sis_get_class` maps each codepoint to exactly one `suas_sis_class_t`:

| Class | Value | Meaning |
|-------|-------|---------|
| `SUAS_SIS_CLASS_INVALID` | 0 | Cannot be part of an identifier |
| `SUAS_SIS_CLASS_START` | 1 | Valid as the **initial** codepoint of an identifier |
| `SUAS_SIS_CLASS_CONTINUE` | 2 | Valid only in a **subsequent** position |

`SUAS_SIS_CLASS_START` is a *structural* class, not a promise that the
codepoint is also legal in a continuation position: a profile may admit a start
character that is not a legal continue character (the hashtag `#`, §6). The
authoritative test for a non-initial position is therefore
**`suas_sis_is_continue`**, and the derivation is:

```
is_start(cp)    → SUAS_SIS_CLASS_START
is_continue(cp) → SUAS_SIS_CLASS_CONTINUE
otherwise       → SUAS_SIS_CLASS_INVALID
```

### 2.2 Identifier grammar

SIS identifiers follow the shape of the UAX #31 default identifier syntax, with
an empty `Medial` class:

```
SIS-D1. Default Identifier Syntax:
        <Identifier> := <Start> <Continue>*
```

An identifier is a non-empty run of `START`-then-`CONTINUE` codepoints. A
streaming consumer recognizes an identifier by the state machine of §8.2.

### 2.3 Default identifier classes

Under the `DEFAULT` profile, `suas_sis_get_class` resolves the raw `ID_Start` /
`ID_Continue` classes:

| Class | Coverage |
|-------|----------|
| `START` | `L*` letters (`Lu`, `Ll`, `Lt`, `Lm`, `Lo`), letter numbers (`Nl`), `Other_ID_Start`, plus `U+005F LOW LINE` |
| `CONTINUE` | `START`, plus nonspacing marks (`Mn`), spacing combining marks (`Mc`), decimal numbers (`Nd`), connector punctuation (`Pc`), `Other_ID_Continue`, and the joining controls / variation selectors retained by UAX #31 §2.3 |
| `INVALID` | everything else |

`U+005F LOW LINE` is admitted as a `START` character to match the ubiquitous
shell / configuration convention (UAX #31 Table 3). It is also connector
punctuation and therefore a `CONTINUE` character.

> **Curation note.** The `ID_Start` / `ID_Continue` sets are curated subsets of
> the UCD covering the modern scripts used by the OpenWindows shell, the
> configuration parsers, and the nVAI runtime: Latin, Greek, Cyrillic, Hebrew,
> Arabic, the Indic scripts, Thai and Lao, Georgian, Ethiopic, the CJK
> ideographs, Hiragana, Katakana, Bopomofo, and Hangul. Extending the tables is
> a bridge-data update (§10) and never changes the API.

### 2.4 SCP invariance

The System Control Plane block **`0x00110000`–`0x0011FFFF`** is strictly
rejected inside identifiers. `suas_sis_is_scp` tests membership, and
`suas_sis_get_class` always returns `SUAS_SIS_CLASS_INVALID` for such
codepoints. This prevents in-band control instructions from corrupting symbol
tables or escaping from a literal into the lexer, and keeps the identifier
namespace disjoint from the SBR break markers (`0x00110020`), the math
directives (`0x00110030`), the segmentation directives (`0x00110040`), the SDF
directional block (`0x00110100`), the SSP script directives (`0x00112000`), and
the BANcodes (`0x0011A000`+).

### 2.5 Neography and the Native plane

Codepoints in the Native plane (`0x00120000`–`0x7FFFFFEF`) and in the extSUCS
plugin plane are admitted as identifier characters of class `START`. The primary
consumers are the **RNUR / RCCR conlang** registries in the carve-out
`0x00160000`–`0x0016FFFF`, whose base letters are thereby usable as identifier
start characters without a script-property consult. Trap slots
(`0x7FFFFFF0`–`0x7FFFFFFE`) and the Sentinel (`0x7FFFFFFF`) are invalid.

### 2.6 Combining marks

A base character followed by any number of combining marks is a valid
identifier: nonspacing marks and spacing combining marks are `CONTINUE`
characters. Enclosing combining marks (for example `U+20DD`–`U+20E0`) are
excluded, because the composites they form with letters are not normally valid
constituents of identifiers.

### 2.7 Modifier letters

Modifier letters (`Lm`) are `START` characters; modifier symbols (`Sk`) are not.
Modifier letters are part of many natural-language orthographies and are useful
for word-like identifiers, whereas modifier symbols are seldom part of an
orthography.

### 2.8 Layout and format control characters

Format controls and default-ignorable codepoints do not change the identifier
class of the characters around them, but their presence is a security concern.
SIS keeps the joining controls (`U+200C`, `U+200D`) and the variation selectors
in `CONTINUE` (UAX #31 §2.3), and leaves their exclusion to the
default-ignorable exclusion profile (§7.3). A comparison layer should ignore
them (§1.3 of UAX #31; see also `suas_sis_get_syntax`, which classes `U+200E`
and `U+200F` as ignorable format controls).

### 2.9 Backward compatibility

The `Other_ID_Start` and `Other_ID_Continue` sets are folded into the class
predicates so that any sequence that qualified as an identifier in a previous
version continues to qualify:

| Set | Representative members |
|-----|------------------------|
| `Other_ID_Start` | `U+2118 ℘`, `U+212E ℮`, `U+309B`, `U+309C` |
| `Other_ID_Continue` | `U+00B7 ·`, `U+0387 ·`, `U+1369`–`U+1371`, `U+19DA ᧚` |

---

## 3. Immutable Identifiers

The `IMMUTABLE` profile (`SUAS_SIS_PROFILE_IMMUTABLE`) addresses environments
that cannot update across versions of the SUCS bridge. Instead of enumerating
the allowed characters, it enumerates a small fixed set of **reserved**
codepoints and admits everything else — including unassigned codepoints:

```
SIS-R2. Immutable Identifiers: a non-empty string is an identifier iff it
        contains no codepoint with any of the following:

          Pattern_White_Space = True
          Pattern_Syntax      = True
          General_Category    = Private_Use, Surrogate, or Control
          Noncharacter_Code_Point = True
```

Under this profile every admitted codepoint is a `START` character (there is no
start/continue distinction), so `suas_sis_is_start_ex` and
`suas_sis_is_continue_ex` agree.

This gives absolute codepoint immutability: because unassigned codepoints are
admitted, future bridge assignments never invalidate an existing identifier. The
drawback is that the class carries no notion of human intelligibility, so
immutable identifiers cannot rely on script or normalization properties.

---

## 4. Whitespace and Syntax

Lexical analysis concerns more than identifiers. SIS exposes the disjoint
`Pattern_White_Space` / `Pattern_Syntax` split of UAX #31 §4 through
`suas_sis_get_syntax`:

| Category | Value | Coverage |
|----------|-------|----------|
| `SUAS_SIS_SYNTAX_LITERAL` | 0 | Not whitespace, syntax, or control (identifier chars, string literals) |
| `SUAS_SIS_SYNTAX_WHITESPACE` | 1 | Horizontal space: `U+0009`, `U+0020` |
| `SUAS_SIS_SYNTAX_LINETERM` | 2 | Line terminators: `U+000A`–`U+000D`, `U+0085`, `U+2028`, `U+2029` |
| `SUAS_SIS_SYNTAX_IGNORABLE` | 3 | Ignorable format controls: `U+200E`, `U+200F` |
| `SUAS_SIS_SYNTAX_OPERATOR` | 4 | `Pattern_Syntax`: operators, brackets, quotes, comment marks |
| `SUAS_SIS_SYNTAX_CONTROL` | 5 | SCP markers, trap slots, the Sentinel |

### 4.1 Whitespace

`SIS-R3a. Pattern_White_Space`: a conforming implementation interprets the
`Pattern_White_Space` codepoints as whitespace, as follows:

1. `U+000A`, `U+000B`, `U+000C`, `U+000D`, `U+0085`, `U+2028`, `U+2029` are
   line terminators.
2. `U+200E` and `U+200F` are ignorable format controls.
3. `U+0009` and `U+0020` are horizontal space.

### 4.2 Bidirectional ordering and required spaces

The two implicit directional marks are non-spacing. Where a syntax requires a
run of spaces (for example between two identifiers), at least one of them must
be a real space, never only `U+200E` / `U+200F`; otherwise `else<LRM>if` would
be *seen* as one word but *parsed* as two.

### 4.3 Contexts for ignorable format controls

A conforming implementation should allow an ignorable format control:

- **SIS-I1.** in or adjacent to a run of lexical horizontal space;
- **SIS-I2.** wherever horizontal space could be inserted without changing the
  meaning of the program;
- **SIS-I3.** at the start or end of a lexical line.

### 4.4 Syntax characters

`SIS-R3b. Pattern_Syntax`: the identifier, whitespace, and syntax sets are
pairwise disjoint. `Pattern_Syntax` codepoints carry syntactic use — decimal
marks, arithmetic operators, brackets, comment delimiters, quotation marks,
escape introducers — and are `SUAS_SIS_SYNTAX_OPERATOR`. The reference predicate
is a curated subset of the immutable `Pattern_Syntax` property.

### 4.5 Operator identifiers

`SIS-R3c. Operator Identifiers`: when a language allows user-defined operators,
the operators are formed from `SUAS_SIS_SYNTAX_OPERATOR` codepoints plus
nonspacing marks, and must not overlap identifiers or literals. Operators must
not begin with a character that could begin an identifier; this keeps `+1` and
`−x` from being read as operators.

---

## 5. Normalization and Case

SIS is a *lexical* filter: it does not normalize or case-fold. Implementations
that need canonical or case-insensitive identifier equivalence layer one of the
following over the SIS classes, after parsing has located the identifiers:

| Requirement | Meaning |
|-------------|---------|
| `SIS-R4` Equivalent Normalized Identifiers | identifiers compared under a specified Normalization Form |
| `SIS-R5` Equivalent Case-Insensitive Identifiers | identifiers compared under a specified case folding |
| `SIS-R6` Filtered Normalized Identifiers | allowed identifiers must already be in the Normalization Form |
| `SIS-R7` Filtered Case-Insensitive Identifiers | allowed identifiers must already be in the case-folded form |

The folding of distinctions (via NFKC or case mapping) must be delayed until
after the identifier has been located, so that string literals and comments are
never rewritten by the lexer.

### 5.1 Identifier closure under normalization

The `XID` variants of the classes (the two structural classes SIS exposes) are
constructed so that identifiers are closed under all four Normalization Forms
and under the case operations, with the historical exceptions resolved by the
`Other_ID_*` sets (§2.9). Implementations that compare identifiers should use
the `toNFKC_Casefold` transform (UAX #31 §5) rather than `General_Category` or
`toUppercase`/`toLowercase`, which are not stable.

---

## 6. Hashtag Identifiers

The `HASHTAG` profile (`SUAS_SIS_PROFILE_HASHTAG`) provides the extended
identifier syntax for hashtags:

```
SIS-D2. Default Hashtag Identifier Syntax:
        <Hashtag-Identifier> := <Start> <Continue>*
```

```
SIS-R8. Extended Hashtag Identifiers:
        Start    := [#﹟＃]      (U+0023, U+FE5F, U+FF03)
        Continue := XID_Continue, plus Extended_Pictographic, plus
                    Emoji_Component, plus "_", "-", "+",
                    minus the Start characters.
```

Because the `#` variants are `START`-only, a second `#` terminates the hashtag
(§8.2). When parsing hashtags in flowing text, an extended hashtag should be
recognized only when no `CONTINUE` character immediately precedes the `#`.

---

## 7. Standard Profiles

### 7.1 Immutable profile

Defined in §3; selected with `SUAS_SIS_PROFILE_IMMUTABLE`.

### 7.2 Hashtag profile

Defined in §6; selected with `SUAS_SIS_PROFILE_HASHTAG`.

### 7.3 Default-ignorable exclusion profile

A tailoring may remove all `Default_Ignorable_Code_Point` codepoints from
`CONTINUE`, reducing (but not eliminating) spoofing risk. Because SIS keeps the
joining controls and selectors in `CONTINUE` for legitimate orthographic use
(§2.8), this profile is an explicit opt-out and is not the default.

### 7.4 Mathematical compatibility notation

A tailoring may add `ID_Compat_Math_Start` / `ID_Compat_Math_Continue`
(∂, ∇, ∞, subscript and superscript digits and signs) to the classes, and must
then remove `∂`, `∇`, and `∞` from the syntax set. As with all profiles, the
extension is applied outside the SIS engine by wrapping the class query.

---

## 8. Implementation Notes

### 8.1 Querying

```c
suas_sis_class_t k = suas_sis_get_class(0x4E00); /* SUAS_SIS_CLASS_START */
suas_sis_syntax_t s = suas_sis_get_syntax(0x002B); /* ..._OPERATOR */
```

`suas_sis_get_class` is O(1): it performs a bounded sequence of range tests and
no allocation.

### 8.2 The streaming contract

```c
suas_sis_state_t st;
suas_sis_init(&st);
for (i = 0; i < n; i++) {
    if (!suas_sis_feed(&st, text[i]))
        break;                 /* identifier ended or never began */
}
if (suas_sis_is_started(&st))
    /* text[0..suas_sis_length(&st)) is one identifier */
```

`suas_sis_feed` returns `true` iff the codepoint was validly appended to the
current identifier. When the codepoint cannot be appended and an identifier is
in progress, the identifier is **terminated** and the machine resets, ready for
the next token. The first codepoint must be a `START` character; a `CONTINUE`
character in the initial position is rejected and leaves the machine idle.

### 8.3 The state

`suas_sis_state_t` is a fixed-size, caller-owned struct of two scalar fields:

- `is_started` — whether an identifier is currently in progress;
- `length` — the number of codepoints accepted into that identifier.

There is no unbounded history; the machine is a two-state automaton.

### 8.4 SCP transparency

A conforming stream consumer treats `suas_sis_is_scp(cp) == true` as an
identifier terminator. SCP markers are never appended and never counted toward
`length`; this is the "strict rejection" arm of the SCP contract.

### 8.5 Performance

Each query and each feed step performs a constant number of integer comparisons.
There are no loops proportional to the codepoint value, the stream length, or
any run length. SIS is suitable for interrupt context and for the tokenizer hot
path of the shell and the nVAI runtime.

---

## 9. Testing

Conformance is demonstrated by a host test that exercises the classes and the
state machine. The reference suite covers, at minimum:

- **Classes:** Latin, Greek, Armenian, Hebrew, CJK, Hiragana, and Hangul
  letters as `START`; underscore and `Other_ID_Start` as `START`; digits,
  combining marks, connector punctuation, and joining controls as `CONTINUE`;
  whitespace, syntax, controls, and emoji as `INVALID`.
- **SCP / Native:** SCP markers invalid; Native / RNUR / RCCR codepoints
  admitted; trap slots and the Sentinel invalid.
- **Syntax:** whitespace, line terminator, ignorable, operator, literal, and
  control categories.
- **State machine:** a well-formed identifier accumulates; a `CONTINUE`-only
  initial codepoint is rejected; an invalid codepoint terminates and resets; a
  new identifier may follow.
- **Profiles:** hashtag start/continue and termination; immutable admissibility
  and exclusion of controls, private use, and surrogates.
- **Guards:** NULL state on every entry point; end-of-stream handling.

---

## 10. Stability and Versioning

- The class value domain and the syntax category domain are **stable**; new
  values may only be added, never reordered.
- The streaming contract, `SIS-D1`, and `SIS-D2` are **stable** within a major
  version.
- The curated class tables track the Unicode Bridge; a bridge update is a
  **minor** revision and does not change the API.
- The identifier, whitespace, and syntax sets are guaranteed disjoint and
  immutable in the same sense as the UAX #31 stability policy.

Current version: **1.0.0** (`SUAS_SIS_SPEC_ID` = `"SUAS-007"`).

---

## 11. Conformance Requirements

An implementation claims conformance by identifying the SIS version it
implements (`SIS-C1`) and by declaring which of the following it observes
(`SIS-C2`):

| Requirement | Subject |
|-------------|---------|
| `SIS-R1` | Default Identifiers (`SIS-D1`, `DEFAULT` profile) |
| `SIS-R2` | Immutable Identifiers (`IMMUTABLE` profile) |
| `SIS-R3a` | `Pattern_White_Space` characters |
| `SIS-R3b` | `Pattern_Syntax` characters |
| `SIS-R3c` | Operator Identifiers |
| `SIS-R4`–`R7` | Equivalent / Filtered Normalized and Case-Insensitive Identifiers |
| `SIS-R8` | Extended Hashtag Identifiers (`HASHTAG` profile) |

### 11.1 Compliance matrix (reference implementation)

| Requirement | Reference | Test |
|-------------|-----------|------|
| Version identification (`SIS-C1`) | §10 | `test_version_and_meta` |
| Default identifier classes (`SIS-R1`) | §2.3, §2.6–2.9 | `test_identifier_classes` |
| SCP invariance | §2.4 | `test_scp_and_native` |
| Native / neography admission | §2.5 | `test_scp_and_native` |
| Immutable identifiers (`SIS-R2`) | §3 | `test_profiles` |
| `Pattern_White_Space` (`SIS-R3a`) | §4.1 | `test_syntax` |
| `Pattern_Syntax` (`SIS-R3b`) | §4.4 | `test_syntax` |
| Operator identifiers (`SIS-R3c`) | §4.5 | `test_syntax` |
| Hashtag identifiers (`SIS-R8`) | §6 | `test_profiles` |
| Streaming contract | §8.2 | `test_state_machine` |
| Bounded state / zero allocation | §8.3 | `test_state_machine` |
| Totality over SCP / Native / traps | §2.4–2.5, §8.3 | `test_scp_and_native` |
| Guards | §8.2 | `test_state_machine` |

---

## 12. Terms and Conventions

- **Identifier** — a non-empty run of an initial `START` codepoint followed by
  zero or more `CONTINUE` codepoints.
- **Class** — the structural role of a codepoint: `START`, `CONTINUE`, or
  `INVALID`.
- **Profile** — a named tailoring of the class predicates (`DEFAULT`,
  `IMMUTABLE`, `HASHTAG`).
- **Syntax category** — the lexical role of a codepoint outside identifiers:
  literal, whitespace, line terminator, ignorable, operator, or control.
- **SCP marker** — a codepoint in `0x00110000`–`0x0011FFFF`; always invalid in
  identifiers.
- **Native / neography plane** — codepoints above the Unicode Bridge, admitted
  as identifier characters (RNUR / RCCR conlang letters).
- **Bridge** — the Unicode-compatible codepoint range `0x000000`–`0x0010FFFF`
  served by SUCD tables.

**END OF SUAS-007**
