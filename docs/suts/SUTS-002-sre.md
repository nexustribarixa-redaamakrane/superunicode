# SUTS-002 — SuperUnicode Regular Expressions (SRE)

**Standard ID:** SUTS-002
**Status:** Draft — Authoring
**Scope:** Modular Technical Specification (pattern matching / regular expressions)
**Applies To:** Base SUCS (Unicode Bridge, SCP, Native), ExtSUCS (unbounded arbitrary-precision plane), SUCD property tables, SDF directional scopes, SBR line boundaries, SUCF canonical forms, search engines, lexers, kernel diagnostics
**Last Updated:** 2026-10-13

---

## 1. Introduction

Regular expressions are a powerful tool for using patterns to search and
modify text. They are a key component of many programming languages,
databases, and spreadsheets. SRE specifies how to support the SuperUnicode
codepoint space in regular expression engines.

### 1.1 OpenWindows kernel principles

In keeping with OpenWindows kernel principles, SRE ditches multi-pass
backtracking algorithms, heavy heap allocations, and dynamic NFA/DFA state
table growth. Instead, it defines a **zero-allocation, single-pass automaton
pipeline** operating directly over 31-bit SUCS codepoints, SUTF streams, and
System Control Plane (SCP) flags in `O(N)` linear time.

SRE is therefore not "UTS #18 with new syntax": it is a *same-shape, stricter-
engine* specification. It keeps UTS #18's conformance vocabulary (properties,
character classes, loose matches, boundaries, canonical equivalence) while
replacing the execution model with a bounded, allocation-free automaton.

### 1.2 Relationship to UTS #18

SRE is the SuperUnicode analog of UTS #18 *Unicode Regular Expressions*. It
adopts the same problem statement and the same Level 1 / Level 2 feature
inventory, but reinterprets them over the larger SuperUnicode domain and adds a
third, kernel-specific level for the System Control Plane. Where this document
mirrors UTS #18 wording, the requirements are normative for SRE only.

### 1.3 Key architectural shifts

| Dimension | Standard Regex (UTS #18) | SuperUnicode Regex (SUTS-002 SRE) |
|-----------|--------------------------|-----------------------------------|
| Execution engine | Backtracking NFA / dynamic heap allocation | Zero-allocation linear automaton (`O(N)` guarantees) |
| Space coverage | 21-bit Unicode plane range (`0x0000–0x10FFFF`) | Full 31-bit SUCS space (`0x000000–0x7FFFFFFF`) plus unbounded ExtSUCS |
| Control handling | Out-of-band regex flags or anchors | In-band SCP Directives & BANcode trap awareness |
| Bridge vs Native | Standard property sets (`\p{L}`) | SUCD lookup for the Bridge + direct bitmask testing for Native / ExtSUCS |

### 1.4 Terminology and cross-references

SRE reuses the SuperUnicode architecture documents but numbered as they exist
in this repository:

| SRE concept | Authoritative document |
|-------------|------------------------|
| Directional isolation scopes | **SUAS-001 (SDF)** — Structural Directional Framing |
| Word and line boundary rules | **SUAS-003 (SBR)** — System Boundary & Line Break Rules |
| Canonical equivalence (`SUCF-C`) | **SUAS-004 (SUCF)** — SuperUnicode Canonical Forms |
| Character properties | **SUAS-001 SUCD** — property tables |

> Earlier drafts of this text referred to these as `SUAS-05` (SDF), `SUAS-06`
> (SBR), `SUAS-07` (SCF), and `SUAS-18` (SRE). Those identifiers have been
> renumbered to **SUAS-001/003/004** and this specification is **SUTS-002**.

### 1.5 Notation

In order to describe regular expression syntax, an extended BNF form is used:

| Syntax | Meaning |
|--------|---------|
| `x y` | the sequence consisting of `x` then `y` |
| `x*` | zero or more occurrences of `x` |
| `x?` | zero or one occurrence of `x` |
| `x \| y` | either `x` or `y` |
| `( x )` | for grouping |
| `"XYZ"` | terminal character(s) |

The text also uses the following notation for sets in describing the behavior
of character classes.

| Symbol | Description | Example | Equivalent |
|--------|-------------|---------|------------|
| α, β, γ, … | A code point or multi-code-point string | `a`, `ab` | n/a |
| A, B, C, … | A set of code points and/or strings | `A` | n/a |
| {…} | A set of literal items, comma delimited | `{α, β}` | n/a |
| ℙ | The set of all code points | `ℙ ∩ {a, ab}` | `{a}` |
| 𝕊 | The set of all strings | `𝕊 ∩ {a, ab}` | `{a, ab}` |
| A ∪ B | Union | `{α, β} ∪ {β, γ}` | `{α, β, γ}` |
| A ∩ B | Intersection | `{α, β} ∩ {β, γ}` | `{β}` |
| A ∖ B | Set difference | `{α, β} ∖ {β, γ}` | `{α}` |
| A ⊖ B | Symmetric difference | `{α, β} ⊖ {β, γ}` | `{α, γ}` |
| ∁ℙA | Code point complement | `∁ℙA` | `ℙ ∖ A` |

The code point range for SRE is `0x000000–0x7FFFFFFF` for Base SUCS, with the
Native SUCS zone at `0x00120000–0x7FFFFFFE`, the System Control Plane at
`0x00110000–0x0011FFFF`, and the unbounded ExtSUCS space above `0x7FFFFFFF`.

#### 1.5.1 Character classes

A Character Class represents a set of characters.

| Nonterminal | Production Rule | Comments & Constraints |
|-------------|-----------------|------------------------|
| `CHARACTER_CLASS` | `:= '[' COMPLEMENT? SEQUENCE ']'` | if complement is present, it is `∁ℙA` |
| `SEQUENCE` | `:= ITEM (SEQ_EXTEND)*` | union of items, then set operators |
| `SEQ_EXTEND` | `:= OPERATOR CHARACTER_CLASS \| ITEM` | see RL1.3 |
| `ITEM` | `:= LITERAL ('-' LITERAL)?` `:= CHARACTER_CLASS` | parse error if range start > end |
| `LITERAL` | `:= ESCAPE (SYNTAX_CHAR \| SPECIAL_CHAR)` `:= NON_SYNTAX_CHAR` | |
| `COMPLEMENT` | `:= '^'` | |
| `ESCAPE` | `:= '\'` | |
| `OPERATOR` | `:= '\|\|' \| '&&' \| '--' \| '~~'` | union, intersection, difference, symmetric difference |

The following table gives examples of character classes:

| Character Class | Matches |
|-----------------|---------|
| `[a-z \|\| A-Z \|\| 0-9]` | ASCII alphanumerics |
| `[^a-z A-Z 0-9]` | all code points except ASCII alphanumerics |
| `[\] \- \ ]` | the literal characters `]`, `-`, space |

Where string offsets are used in examples, they are from zero to n (the length
of the string), and indicate positions between characters.

The following additional notation is defined:

| Syntax | Meaning |
|--------|---------|
| `\n` | expands to the text matching the nth parenthesized group (à la Perl) |
| `$n` | expands to the text matching the nth group in a replacement string |

#### 1.5.2 Property examples

All examples of properties being equivalent to certain literal character
classes are illustrative. They are generated at a point in time and are not
updated with each release.

### 1.6 Conformance

All syntax and API presented in this document is only for the purpose of
illustration; there is absolutely no requirement to follow such syntax or API.
Regular expression syntax varies widely: the features discussed here would need
to be adapted to the syntax of the particular implementation.

**C0.** An implementation claiming conformance to this specification at any
Level shall identify the version of this specification and the version of the
SuperUnicode property tables (SUCD).

**C1.** An implementation claiming conformance to Level 1 shall satisfy the
requirements in §2, namely RL1.1 through RL1.7.

**C2.** An implementation claiming conformance to Level 2 shall satisfy C1, and
meet the requirements in §3, namely RL2.1 through RL2.6.

**C3.** An implementation claiming conformance to Level 3 shall satisfy C2, and
meet the requirements in §4, the System Control Plane and kernel diagnostics
requirements.

**C4.** An implementation claiming partial conformance shall clearly indicate
which levels are completely supported (C1–C3), plus any additional supported
features.

A regular expression engine may be operating in the context of a larger system.
In that case some of the requirements may be met by the overall system. For
example, the requirements of canonical equivalence might be met by making
normalization available as part of the larger system. Such usage is conformant,
as long as the situation is clearly documented. For backwards compatibility,
some functionality may only be available if some special setting is turned on;
none of the conformance requirements require the functionality to be available
by default.

### 1.7 SRE Conformance Levels

SUTS-002 defines three core kernel compliance levels.

#### Level 1 — Basic SuperUnicode Support (Core Kernel & Shell)

- **Full 31-bit addressing:** correct handling of all SUCS codepoints up to
  `0x7FFFFFFF`.
- **SUCD script & category properties:** standard character classes using binary
  SUCD property flags (`\p{L}`, `\p{N}`, `\p{Arabic}`, `\p{Han}`).
- **Canonical equivalence match (SUCF-C):** matches evaluate under SUAS-004
  (SUCF) canonical normalization rules.
- **SCP invariance:** System Control Plane markers (`0x110000–0x11FFFF`) are
  ignored by default unless explicitly matched using `\p{SCP}` or exact SCP
  codepoint escapes.

#### Level 2 — Extended System & Neography Support

- **Private registry matcher:** direct support for the RNUR neography registry
  (`\p{RNUR}`), a Native-plane codepoint registry delivered as a plugin.
- **SDF & directional scope anchors:** boundary anchors for the directional
  isolation scopes defined in SUAS-001 (SDF) (`\b{LTR}`, `\b{RTL}`).
- **Line & word boundary rules:** boundary matching driven deterministically by
  SUAS-003 (SBR).

#### Level 3 — System Control Plane & Kernel Diagnostics

- **BANcode trap matching:** diagnostic patterns matching system-mode or
  app-mode BANcode crash sequences (`\p{BANcode}`).
- **SCP directorial in-band anchors:** matching explicit SCP stream directives
  (`\p{SCP_DIR_RTL}`, `\p{SCP_BRK_MANDATORY}`).

---

## 2. Level 1 — Basic SuperUnicode Support

Regular expression syntax usually allows for an expression to denote a set of
single characters, such as `[a-z A-Z 0-9]`. Because there are a very large
number of characters in the SuperUnicode space, simple list expressions do not
suffice.

### 2.1 Hex notation (RL1.1)

The character set used by the regular expression writer may not be SuperUnicode,
or may not have the ability to input all codepoints from a keyboard.

**RL1.1 — Hex notation.** To meet this requirement, an implementation shall
supply a mechanism for specifying any codepoint, using the hexadecimal code
point representation.

The syntax must use the code point in its hexadecimal representation. Syntax
such as `\uD834\uDD1E` does not meet this requirement for expressing U+1D11E
because `1D11E` does not appear in the syntax. In contrast, `\U0001D11E`,
`\x{1D11E}`, or `\u{1D11E}` do satisfy the requirement.

A sample notation within strings uses `\u` followed by four hex digits, or
`\u{` followed by any number of hex digits and terminated by `}`:

| Nonterminal | Production Rule | Comments |
|-------------|-----------------|----------|
| `LITERAL` | `:= HEX` | adds to previous LITERAL rules |
| `HEX` | `:= '\u' HEX_CHAR{4}` `:= '\u{' CODEPOINT (SP CODEPOINT)* '}'` | `\u{3b1 3b3 3b5 3b9}` |
| `HEX_CHAR` | `:= [0-9A-Fa-f]` | |
| `CODEPOINT` | `:= HEX_CHAR{1,}` | any non-negative ExtSUCS codepoint; no fixed width |

> **Note.** `\u{3b1 3b3 3b5 3b9}` is semantic sugar — useful for readability and
> concision but not a requirement. It can be used anywhere the equivalent
> individual hex escapes could be.

Examples:

| Syntax | Matches |
|--------|---------|
| `[\u{3040}-\u{309F} \u{30FC}]` | the Hiragana block, plus the prolonged sound sign |
| `[\u{B2} \u{2082}]` | superscript ² and subscript ₂ |
| `[a \u{10450}]` | "a" and SHAVIAN LETTER PEEP |
| `ab\u{63 64}` | "abcd" |

#### 2.1.1 Hex notation and normalization

Literal text in regular expressions may be normalized in transmission, out of
the control of the authors of that text. Hex notation can be used to avoid this
problem. A regular expression engine may also enforce a single, uniform
interpretation of regular expressions by always normalizing input text to
Normalization Form C (SUCF-C) before interpreting that text. See SUAS-004
(SUCF).

### 2.2 Properties (RL1.2)

Because SuperUnicode is a large character set that is regularly extended, a
regular expression engine needs to provide for the recognition of whole
categories of characters as well as simply literal sets of characters and
strings. This is done by providing syntax for sets of characters based on the
SUCD character properties. Examples include `\p{Script=Greek}` and
`[:Script=Greek:]`. Properties can also be combined with literal sets, e.g.
`[\p{Script=Greek}--\p{General_Category=Letter}]`.

#### 2.2.1 Domain of properties

For regular expressions, it is helpful to divide properties by the treatment of
their domain (what they are properties of) and their codomain (the values of
the properties). Most properties are properties of codepoints; thus their
domains are simply the full set of codepoints. In addition to properties of
characters, there are also properties of strings (sequences of characters).

#### 2.2.2 Codomain of properties

The values (codomain) of properties of characters (or strings) have the
following simple types: Binary, Enumerated, Numeric, Code Point, and String.
Properties can also have multivalued types: a Set or List of other types.

#### 2.2.3 Property syntax

The recommended names for properties and property values follow the SUCD
aliases. It is strongly recommended that both abbreviated and long names be
recognized, and that loose matching of property names and values be implemented.

| Nonterminal | Production Rule | Comments |
|-------------|-----------------|----------|
| `CHARACTER_CLASS` | `:= '\' [pP] '{' PROP_SPEC '}'` `:= '[:' COMPLEMENT? PROP_SPEC ':]'` | adds to previous rules |
| `PROP_SPEC` | `:= PROP_NAME (RELATION PROP_VALUE)?` | |
| `RELATION` | `:= '=' \| '≠' \| '!='` | |

`\P{X}` and `[:^X:]` are defined to be identical to `[^\p{X}]`, that is, the
code point complement of `\p{X}`.

Examples:

| Syntax | Matches |
|--------|---------|
| `[\p{L} \p{Nd}]` | all letters and decimal digits |
| `\P{script=greek}` | all codepoints except those with the Greek script |
| `\p{East Asian Width:Narrow}` | enumerated property value match |
| `\p{Whitespace}` | binary property value `Whitespace = True` |
| `\p{scx=Kana}` | characters whose Script_Extensions includes Kana |

**RL1.2 — Properties.** To meet this requirement, an implementation shall
provide at least a minimal list of properties, consisting of:

- General_Category and core properties;
- Script and Script_Extensions;
- Alphabetic, Uppercase, Lowercase, White_Space;
- Noncharacter_Code_Point, Default_Ignorable_Code_Point;
- `ANY`, `ASCII`, `ASSIGNED`.

The values for these properties must follow the SuperUnicode definitions,
including the property and property value aliases. Matching of Binary,
Enumerated, Catalog, and Name values must follow the SUCD matching rules.

**RL1.2a — Compatibility properties.** To meet this requirement, an
implementation shall provide the compatibility properties (for example `alpha`,
`lower`, `upper`, `punct`, `digit`/`\d`, `space`/`\s`, `word`/`\w`). Such an
implementation shall document whether it is using the standard recommendation
or POSIX-compatible properties.

Implementers may add aliases beyond those recognized in SUCD. However, they
must be aware that such additional aliases may cause problems if they collide
with future SUCD aliases for different values.

#### 2.2.4 General category property

The most basic overall character property is `General_Category`, which is a
basic categorization of characters into Letters, Punctuation, Symbols, Marks,
Numbers, Separators, and Other.

| Abb. | Long form | Abb. | Long form | Abb. | Long form |
|------|-----------|------|-----------|------|-----------|
| L | Letter | S | Symbol | Z | Separator |
| Lu | Uppercase Letter | Sm | Math Symbol | Zs | Space Separator |
| Ll | Lowercase Letter | Sc | Currency Symbol | Zl | Line Separator |
| Lt | Titlecase Letter | Sk | Modifier Symbol | Zp | Paragraph Separator |
| Lm | Modifier Letter | So | Other Symbol | C | Other |
| Lo | Other Letter | P | Punctuation | Cc | Control |
| M | Mark | Pc | Connector Punctuation | Cf | Format |
| Mn | Non-Spacing Mark | Pd | Dash Punctuation | Cs | Surrogate |
| Mc | Spacing Combining Mark | Ps | Open Punctuation | Co | Private Use |
| Me | Enclosing Mark | Pe | Close Punctuation | Cn | Unassigned |
| N | Number | Pi | Initial Punctuation | | |
| Nd | Decimal Digit Number | Pf | Final Punctuation | | |
| Nl | Letter Number | Po | Other Punctuation | | |
| No | Other Number | | | | |

**Core properties.** There are three additional binary properties associated
with the General_Category property:

| Value | Matches | Equivalent to | Notes |
|-------|---------|---------------|-------|
| Any | all code points | `\p{any}` | may be expressed as `.` |
| Assigned | all assigned characters | `\P{Cn}` | also includes private use |
| ASCII | all ASCII characters | `[\u{0}-\u{7F}]` | |

#### 2.2.5 Script and Script Extensions

A regular-expression mechanism may choose to offer the ability to identify
characters on the basis of other properties besides the General Category. In
particular, characters are also divided into scripts. Some characters are
regularly used with multiple scripts; for such characters the Script_Extensions
property (abbreviated `scx`) identifies the set of associated scripts. The
expression `\p{sc=Hira}` includes those characters whose Script value is Hira,
while `\p{scx=Hira}` includes all the characters whose Script_Extensions value
contains Hira.

#### 2.2.6 Age and blocks

In regex expressions, the Age property is used to indicate the characters that
were in a particular version of the standard. Blocks have an associated
enumerated property, the Block property. If blocks are used, some of the names
can collide with Script names, so they should be distinguished, with syntax such
as `\p{Greek Block}` or `\p{Block=Greek}`.

### 2.3 Subtraction and intersection (RL1.3)

**RL1.3 — Subtraction and intersection.** To meet this requirement, an
implementation shall supply mechanisms for union, intersection and set-
difference of sets of characters within regular expression character class
expressions.

| Nonterminal | Production Rule | Comments |
|-------------|-----------------|----------|
| `SEQUENCE` | `:= ITEM (SEQ_EXTEND)*` | replaces the earlier SEQUENCE definition |
| `OPERATOR` | `:= '\|\|' \| '&&' \| '--' \| '~~'` | union / intersection / difference / symmetric difference |

The symmetric difference of two sets is defined as the union minus the
intersection, or equivalently the union of the asymmetric differences.

Code point complement can also be expressed using the property `\p{any}` or the
equivalent literal `[\u{0}-\u{10FFFF}]`. Thus `[^A]` is equivalent to
`[\p{any}--A]`.

Examples:

| Expression | Matches |
|------------|---------|
| `[\p{L}--[QW]]` | all letters but Q and W |
| `[\p{N}--[\p{Nd}--[0-9]]]` | all non-decimal numbers, plus 0-9 |
| `[\u{0}-\u{7F}--\P{letter}]` | all letters in the ASCII range |
| `[\p{Greek}--\N{GREEK SMALL LETTER ALPHA}]` | Greek letters except alpha |
| `[\p{letter}~~\p{ascii}]` | either letter or ASCII, but not both |

### 2.4 Simple word boundaries (RL1.4)

**RL1.4 — Simple word boundaries.** To meet this requirement, an implementation
shall extend the word boundary mechanism so that:

- The class of `<word_character>` includes all the Alphabetic values from the
  property tables, plus the decimals, and U+200C and U+200D
  (`Join_Control=True`).
- Nonspacing marks are never divided from their base characters, and otherwise
  ignored in locating boundaries.

Level 2 provides more general support for word boundaries between arbitrary
characters, which may override this behavior.

### 2.5 Simple loose matches (RL1.5)

**RL1.5 — Simple loose matches.** To meet this requirement, if an
implementation provides for case-insensitive matching, then it shall provide at
least the simple, default SuperUnicode case-insensitive matching, and specify
which properties are closed and which are not.

Some caseless matches may match one character against two: for example, U+00DF
"ß" matches the two characters "SS". Because many implementations are not set up
to handle this, at Level 1 only simple case matches are required.

Matching case-insensitively is one example of matching under an equivalence
relation: a regular expression R matches under an equivalence relation E
whenever, for all strings S and T, if S is equivalent to T under E, then R
matches S if and only if R matches T. In the Unicode Standard, the relevant
equivalence relation for case-insensitivity is established according to whether
two strings case fold to the same value.

For case-insensitive matching:

1. Each string literal is matched case-insensitively. For example, `/Dåb/`
   matches as if it were `/(?:d|D)(?:å|Å|\u{212B})(?:b|B)/`.
2. (Optional) Each character class is closed under case. For example,
   `[\p{Block=Phonetic_Extensions} [A-E]]` matches 133 codepoints under a given
   version, and its case-closure adds 7 more.

### 2.6 Line boundaries (RL1.6)

**RL1.6 — Line boundaries.** To meet this requirement, if an implementation
provides for line-boundary testing, it shall recognize not only CRLF, LF, CR,
but also NEL (U+0085), PARAGRAPH SEPARATOR (U+2029) and LINE SEPARATOR
(U+2028). Formfeed (U+000C) also normally indicates an end-of-line.

A newline sequence is defined to be any of:

```
\u{A} | \u{B} | \u{C} | \u{D} | \u{85} | \u{2028} | \u{2029} | \u{D A}
```

It is strongly recommended that there be a regular expression meta-character,
such as `\R`, for matching all line ending characters and sequences listed
above. For more information on line breaking, see SUAS-003 (SBR).

### 2.7 Code points (RL1.7)

**RL1.7 — Supplementary code points.** To meet this requirement, an
implementation shall handle the full range of codepoints. SRE extends this to
the full 31-bit SUCS space and the unbounded ExtSUCS space; matching is
defined over codepoints, not code units, at every layer from the Unicode Bridge
through Native SUCS to ExtSUCS.

---

## 3. Level 2 — Extended System & Neography Support

Level 1 support works well in many circumstances. However, it does not handle
more complex languages or extensions to the SuperUnicode Standard very well.
Particularly important cases are canonical equivalence, word boundaries, and
loose matches.

### 3.1 Canonical equivalents (RL2.1)

The equivalence relation for canonical equivalence is established by whether two
strings are identical when normalized to the canonical decomposition form.

For most full-featured regular expression engines, it is quite difficult to
match under canonical equivalence, which may involve reordering, splitting, or
merging of characters. It is feasible, however, to construct patterns that will
match against a defined normalization form, by:

1. Putting the text to be matched into a defined normalization form.
2. Having the user design the pattern to match against that form.
3. Applying the matching algorithm on a code point basis.

SRE instead provides **SUCF-C enforcement** as an engine flag: when set, the
incoming codepoint stream is canonically composed (SUAS-004 SUCF) before the
automaton consumes it, so a precomposed literal matches its decomposed input.

### 3.2 Extended grapheme clusters and character classes with strings (RL2.2)

One or more characters may make up what the user thinks of as a character, a
*grapheme cluster*. The SuperUnicode Standard defines extended grapheme
clusters that treat certain sequences as units, including Hangul syllables and
base characters with combining marks. The precise definition is in the
superunicode text segmentation rules; the CLDR definitions are strongly
recommended where they are more comprehensive.

**RL2.2.** To meet this requirement, an implementation shall provide a mechanism
for matching against an arbitrary extended grapheme cluster, character classes
with strings, and extended grapheme cluster boundaries. For example, `\X` can
match any extended grapheme cluster, while `\b{g}` is a zero-width match against
a grapheme cluster boundary.

Useful zero-width boundary detections:

| Syntax | Zero-width match at |
|--------|---------------------|
| `\b{g}` | a grapheme cluster boundary |
| `\b{w}` | a word boundary (different from `\b` alone) |
| `\b{l}` | a line break boundary |
| `\b{s}` | a sentence boundary |

#### 3.2.1 Character classes with strings

Regular expression engines should also provide a mechanism for matching against
character classes with strings. One mechanism is explicit syntax for strings in
character classes, as in the following addition:

| Nonterminal | Production Rule | Comments |
|-------------|-----------------|----------|
| `ITEM` | `:= '\q{' LITERAL* ('\|' LITERAL*)* '}'` | one or more literal strings |

The following table shows examples:

| Expression | Matches |
|------------|---------|
| `[a-z\q{x\u{323}}]` | characters a-z, and the string x with under-dot |
| `[a-z\q{aa}]` | the characters a-z, and the string aa |
| `[a-z ñ \q{ch\|ll\|rr}]` | some lowercase characters in traditional Spanish |
| `[a-z \q{🧐\|🇫🇷}]` | characters a-z and two emoji |

String literals in character classes are especially useful in combination with
a property of strings, because they can remove exceptions that cannot be
expressed by other means.

### 3.3 Default word boundaries (RL2.3)

**RL2.3.** To meet this requirement, an implementation shall provide a mechanism
for matching default word boundaries. The simple Level 1 support using simple
`<word_character>` classes is only a rough approximation of user word
boundaries. A much better method takes into account more context than just a
single pair of letters. For more information, see SUAS-003 (SBR).

### 3.4 Default case conversion (RL2.4)

**RL2.4.** To meet this requirement, if an implementation provides for case
conversions, then it shall provide at least the full, default case folding. It
is recommended that implementations provide full case conversion, allowing users
to provide both patterns and target text that has been fully case folded.

### 3.5 Name properties (RL2.5)

**RL2.5.** To meet this requirement, an implementation shall support
individually named characters. The `\p{name=...}` syntax matches a set of
characters; the `\N{...}` syntax specifies a single codepoint by exact name:

| Expression | Equivalent to |
|------------|---------------|
| `\N{WHITE SMILING FACE}` | `\u{263A}` |
| `\N{GREEK SMALL LETTER ALPHA}` | `\u{3B1}` |
| `[\N{GREEK SMALL LETTER ALPHA}-\N{GREEK SMALL LETTER BETA}]` | `[\u{3B1}-\u{3B2}]` |

### 3.6 Wildcards in property values (RL2.6)

**RL2.6.** To meet this requirement, an implementation shall support wildcards
in property values:

| Nonterminal | Production Rule | Comments |
|-------------|-----------------|----------|
| `PROP_VALUE` | `:= '/' <regex expression> '/'` | property value matches a regex |
| | `:= '@' PROP_NAME '@'` | property value identity comparison |

For example, `\p{name=/(SMILING|GRINNING) FACE/}` matches all characters whose
names contain "SMILING FACE" or "GRINNING FACE".

### 3.7 Private registry matcher (RNUR)

Level 2 additionally requires direct matching of the contributed registry
allocations. Their placement is defined by SUTR-8 (SUCPM): the **RNUR** (Reddit
Neographical Unicode Registry) is a *codepoint* registry that owns the
**Neography Registry Space** `0x160000–0x16FFFF`, a carve-out of the Native
plane.

| Property | Matches | Range |
|----------|---------|-------|
| `\p{RNUR}` | neographies, original scripts, and conscripts | `0x160000–0x16FFFF` |

RNUR is defined by SUTR-7 (SUCEM) and located by SUTR-8 (SUCPM). Its contents
are delivered as a SuperUnicode plugin (SUTR-5), so SRE evaluates `\p{RNUR}`
against the registry range and defers the plugin's per-codepoint masks to the
plugin. The registry range is tested before the general Native range, so
`\p{RNUR}` and `\p{Native}` never overlap.

The **RCCR** (Reddit Conlang Code Registry) is a registry of *codes* — BCP 47
language tags — **not** of codepoints, so SRE exposes no `\p{RCCR}` property: a
regular expression is evaluated over codepoints, and RCCR contributes none.

### 3.8 Directional scope anchors (SDF)

Boundary anchors for the directional isolation scopes defined in SUAS-001 (SDF):

| Syntax | Zero-width match at |
|--------|---------------------|
| `\b{LTR}` | a boundary into a left-to-right directional run |
| `\b{RTL}` | a boundary into a right-to-left directional run |

---

## 4. Level 3 — System Control Plane & Kernel Diagnostics

Level 3 adds matchers that only make sense inside the SuperUnicode kernel.

### 4.1 BANcode trap matching

`\p{BANcode}` matches kernel BANcode trap sequences — the system-mode and
app-mode crash sequences carried in the System Control Plane. This enables
diagnostic patterns that scan a stream for fault signatures.

### 4.2 SCP directorial in-band anchors

SCP directives are in-band codepoints. SRE can match them directly:
`\p{SCP_DIR_RTL}`, `\p{SCP_BRK_MANDATORY}`, and so on. By default (Level 1
invariance) SCP markers are transparent and are skipped; mentioning an SCP
directive — or setting the `MATCH_SCP` flag — re-enables them.

---

## 5. Reference implementation

The reference implementation ships as a freestanding, zero-allocation C99
module.

### 5.1 Header `suas_sre.h` / `suts_sre.h`

The kernel-facing header is `include/suts/suts_sre.h` (SUAS integration may
re-export it as `suas/suas_sre.h`). It defines the specification identifiers,
flags, property enumerations, the bounded automaton structures, and the API.

```c
#define SUTS_SRE_VERSION_MAJOR   0
#define SUTS_SRE_VERSION_MINOR   1
#define SUTS_SRE_VERSION_PATCH   0

#define SUTS_SRE_MAX_STATES       64u   /* automaton states (stack frame)   */
#define SUTS_SRE_MAX_TRANSITIONS  128u  /* automaton edges                  */
#define SUTS_SRE_MAX_CLASSES      8u    /* character-class descriptors       */
#define SUTS_SRE_MAX_CLASS_ITEMS  12u   /* items per character class         */
#define SUTS_SRE_MAX_INPUT        512u  /* effect buffer for match helpers   */

typedef uint32_t suts_sre_flags_t;

#define SUTS_SRE_FLAG_NONE           0x00000000u
#define SUTS_SRE_FLAG_IGNORE_CASE    0x00000001u /* case-fold via SUCD lookup */
#define SUTS_SRE_FLAG_MATCH_SCP      0x00000002u /* match SCP in-band         */
#define SUTS_SRE_FLAG_SCF_NORMALIZED 0x00000004u /* enforced SUCF-C input     */
#define SUTS_SRE_FLAG_DIR_AWARE      0x00000008u /* SDF directional anchors   */
#define SUTS_SRE_FLAG_MULTILINE      0x00000010u /* ^/$ at line boundaries    */
```

### 5.2 API

| Function | Purpose |
|----------|---------|
| `suts_sre_version_string()` | version string |
| `suts_sre_compile(pattern, len, flags, dfa)` | compile an ASCII-meta pattern into a bounded automaton |
| `suts_sre_prop_match(prop, cp)` | SUCD-style property test |
| `suts_sre_init_match(match, dfa)` | seed a streaming match context |
| `suts_sre_step(dfa, match, cp, offset)` | feed one codepoint in `O(1)` |
| `suts_sre_match_at(dfa, cps, n, start, &s, &e)` | anchored match at `start` |
| `suts_sre_search(dfa, cps, n, start, &s, &e)` | leftmost-then-longest search |

### 5.3 Execution model

Patterns compile into a bounded automaton of `SUTS_SRE_MAX_STATES` states and
`SUTS_SRE_MAX_TRANSITIONS` edges, held entirely in the caller-owned
`suts_sre_compiled_dfa_t`. Matching simulates the automaton over a fixed-size
state subset (`uint64_t` bitmask plus per-state start offsets), performs no
backtracking, allocates no heap, and is linear in the input length: `O(N)` time,
`O(1)` extra space. Edge predicates are evaluated against lots:

1. an exact codepoint or inclusive range;
2. `\p{...}` properties, tested through SUCD lookup for the Bridge and direct
   bitmask tests for Native SUCS / ExtSUCS / SCP;
3. compiled character-class descriptors with union, intersection, difference,
   and symmetric difference.

Zero-width assertions (`^`, `$`, `\b`, `\B`, `\b{LTR}`, `\b{RTL}`) are evaluated
during the epsilon closure against the codepoints adjacent to the current
offset, so no separate assertion pass is required.

### 5.4 Supported reference grammar

The reference engine implements the following subset of the illustrative
syntax:

- concatenation, alternation `|`, groups `(...)` and `(?:...)`;
- quantifiers `*`, `+`, `?` (greedy);
- atoms: literal codepoint, `.`, `[...]` classes (with `^`, ranges, escapes,
  properties, and `&&`/`--`/`||`/`~~`), `\p{...}`/`\P{...}`, `[:...:]`;
- hex escapes `\uXXXX`, `\u{...}`, `\UXXXXXXXX`;
- shorthands `\d \D \w \W \s \S`;
- assertions `^`, `$`, `\b`, `\B`, `\b{w}`, `\b{g}`, `\b{l}`, `\b{s}`,
  `\b{LTR}`, `\b{RTL}`;
- flags `IGNORE_CASE`, `MATCH_SCP`, `SCF_NORMALIZED`, `DIR_AWARE`, `MULTILINE`.

Bounded repetition `{m,n}`, named groups, and back-references are outside the
reference subset; an implementation may add them without changing the Level 1–3
requirements.

### 5.5 Worked examples

| Pattern | Input | Result |
|---------|-------|--------|
| `[a-z]+` | `12abc34` | match `[2,5)` = `abc` |
| `\p{L}+` (Bridge) | `\u{3B1}\u{3B2}` | match |
| `\p{ExtSUCS}+` | `\u{80000005}` | match |
| `ab` (default SCP invariance) | `a <SCP> b` | match (SCP skipped) |
| `\p{SCP}` | `<SCP_DIR_RTL>` | match |
| `\u{E9}` + `SCF_NORMALIZED` | `e` + `\u{0301}` | match |
| `[a-z--[aeiou]]+` | `aeiouxyz` | match `[5,8)` = `xyz` |
| `[[a-f]&&[d-z]]+` | `abcdef` | match `[3,6)` = `def` |

---

## Annex A — Character blocks

The Block property can be a useful property for quickly describing a set of
characters. However, block names need to be used with discretion: blocks are not
exclusive, may include unassigned characters, and writing systems may use
characters from multiple blocks. For these reasons, Script values are generally
preferred to Block values.

## Annex B — Compatibility properties

The following table shows recommended assignments for compatibility property
names for use in regular expressions.

| Property | Standard | Comments |
|----------|----------|----------|
| `alpha` | `\p{Alphabetic}` | more than `gc = Letter` |
| `lower` | `\p{Lowercase}` | more than `gc = Ll` |
| `upper` | `\p{Uppercase}` | more than `gc = Lu` |
| `punct` | `\p{gc=Punctuation}` | |
| `digit` (`\d`) | `\p{gc=Decimal_Number}` | non-decimal numbers excluded |
| `xdigit` | `\p{gc=Decimal_Number}`, `\p{Hex_Digit}` | |
| `alnum` | `\p{alpha}` and `\p{digit}` | |
| `space` (`\s`) | `\p{Whitespace}` | |
| `blank` | `\p{gc=Space_Separator}`, `\N{CHARACTER TABULATION}` | |
| `cntrl` | `\p{gc=Control}` | |
| `word` (`\w`) | `\p{alpha}`, `\p{gc=Mark}`, `\p{digit}`, `\p{gc=Connector_Punctuation}`, `\p{Join_Control}` | approximation to word boundaries |
| `\X` | extended grapheme clusters | |
| `\b` | default word boundaries | may be aligned with `\w` |

## Annex C — Resolving character classes with strings and complement

The operators and contents of a character class correspond to a set of strings.
With full complement, the normal set-theoretic equivalences are maintained.
However, the full complement turns a finite set into an infinite set, which is
a problem for regular expressions. For this reason `[^...]` remains a *code
point* complement even when other syntax is extended to allow strings. The other
set operations (`∪`, `∩`, `∖`, `⊖`) cannot produce an infinite set from finite
sets, and are therefore safe.

## Annex D — Parsing character classes

Building a parser for character classes is reasonably straightforward: a
recursive-descent parser recognizes `\p{`, `\P{`, `[`, and `[^`, and combines
items with the set operators on the fly. The reference implementation builds
each class into a `suts_sre_class_t` descriptor with an ordered item list and a
per-item operator, and evaluates it at match time. Implementations may use
optimized structures, such as a DFA, for processing, provided the observable
results are the same.

## Annex E — Notation for string properties

Properties of strings apply to sequences of two or more characters in addition
to single characters. The preferred notation is `\p{Property_Name}`, the same as
for properties of characters. Implementations constrained to not support strings
in character classes may use `\m{Property_Name}` as an alternate notation outside
of character classes.

---

## References

- **UTS #18** — Unicode Regular Expressions (problem statement and levels)
- **SUAS-001 (SDF)** — Structural Directional Framing and SUCD property tables
- **SUAS-003 (SBR)** — System Boundary & Line Break Rules
- **SUAS-004 (SUCF)** — SuperUnicode Canonical Forms
- **SUTR-7 (SUCEM)** — the RNUR neography registry and the encoding model
- **SUTR-8 (SUCPM)** — property tiers, the dual lookup engine, and the
  registry-space location used by `\p{RNUR}`
- **SUTS-001 (SUCA)** — SuperUnicode Collation Algorithm
