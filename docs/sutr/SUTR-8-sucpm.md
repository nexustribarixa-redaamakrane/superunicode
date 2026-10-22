# SUTR-8 — SuperUnicode Character Property Model (SUCPM)

**Standard ID:** SUTR-8
**Status:** 0.1.0 — Published
**Scope:** The formal metadata model governing property assignment, bitmask layout, and evaluation across the SUCS and ExtSUCS address space
**Applies To:** SUCD, SUAS-001 (SDF), SUAS-003 (SBR), SUAS-004 (SUCF), SUAS-012 (SUCD), SUAS-013 (SISR), SUTS-002 (SRE), and every property-consuming subsystem
**Last Updated:** 2026-10-13

---

## 1. Overview

Where UTR #23 defines how properties are classified, inherited, and overridden
across the standard Unicode space, **SUTR-8 (SUCPM)** establishes the formal
metadata model governing property assignment, bitmask layouts, and evaluation
mechanics across all 31 bits of the SuperUnicode address space — including the
Unicode Bridge, the System Control Plane (SCP), the Native plane, and the
RNUR neography registry.

The model answers three questions that every property user must share:

1. **What is a property** in SuperUnicode, and how is it classified?
2. **Where does each property live**, and how is it evaluated at a given
   codepoint?
3. **How is a property set packed** so a freestanding kernel can test it with a
   single bitwise operation and no heap allocation?

The first two are the subject of the UTR #23-style body (§2–§8). The third is
the SuperUnicode-specific architecture specified in §4 and §5.

### 1.1 Relation to UTR #23

SUCPM is the SuperUnicode analog of UTR #23 *Unicode Character Property Model*.
It adopts UTR #23's whole vocabulary — normative vs. informative, simple vs.
derived, stable vs. immutable, the `PD` definition series, properties of
strings, and string functions — and reinterprets it over the larger
SuperUnicode domain. The UTR #23 material is reproduced here so the model is
self-contained; where this report and UTR #23 differ, **this report governs for
SuperUnicode**.

### 1.2 Scope

This report presents a general overview and typology of character properties
and property values, as well as those of properties of enumerated character
sequences and string functions. This description of the SuperUnicode character
property model is not intended to supersede the normative information on
properties in the SuperUnicode Standard, nor the existing body of technical
reports and documentation files in the SuperUnicode Character Database (SUCD)
that provide detailed descriptions for particular character properties.
Instead it focuses on the overall model behind, and common aspects of, all of
these.

This report covers **formal** character properties — those attributes of
characters specified according to the definitions set forth here. Such formal
properties are a subset of character properties in the generic sense, and they
subdivide into properties defined in the SuperUnicode Standard or SUCD, and
those contributed by related registries.

This report does **not** define the value of any individual property, the
contents of any SUCD data file, or the algorithms that consume properties. It
defines the model. The algorithms are SUTR-0 (CCS), SUTR-1 (CEF),
SUTR-4 (CES), SUTR-7 (SUCEM), SUAS-001 (SDF), SUAS-003 (SBR),
SUAS-004 (SUCF), and SUTS-002 (SRE).

### 1.3 Key architectural concepts

**1.3.1 Property categorization hierarchy**

SUCPM fixes three normative tiers, ordered by who may assign them and whether
the kernel may refuse to honor them:

| Tier | Definition | Examples | Overridable |
|------|------------|----------|-------------|
| **Normative** | Invariant traits enforced by the kernel. A conformant process must observe them. | General_Category, Canonical_Combining_Class (CCC), Space_Range, SCP_Invariance | No — a higher-level protocol must not change a kernel-invariant value |
| **Informative** | Layout and shaping metadata. Provided for information; a conformant process may use or change it. | Directional_Type (Bidi), Line_Break_Class, Script_Identifier | Yes |
| **Contributed / Registry** | External codepoint metadata supplied by a registry. Currently the **RNUR** (Reddit Neographical Unicode Registry), a codepoint registry for neographies delivered as a SuperUnicode plugin. | `\p{RNUR}` | Yes — the registry owns admission |

A property's tier is a fact about the property, not about its data source. Both
informative and registry properties are carried in SUCD, but only the normative
tier carries conformance obligations.

**1.3.2 The dual property lookup engine**

Evaluation is a **range-dispatched dual engine**: the codepoint's tier selects
the strategy, and every strategy returns the same packed property mask. There
is no heap allocation and no dynamic string parsing on the hot path.

| Tier | Range | Evaluation | Cost |
|------|-------|------------|------|
| **Bridge** | `0x000000–0x10FFFF` | Indexed from the SUCD binary/flat property tables | `O(log R)` ranges, `O(1)` indexed |
| **SCP Control Plane** | `0x110000–0x11FFFF` | Computed from the SCP instruction ID's directive byte and argument byte | `O(1)` |
| **Native plane** | `0x120000–0x7FFFFFEF` | Register-mapped range property masks | `O(1)` per range register |
| **RNUR neography registry** | `0x160000–0x16FFFF` | Native-space registry, populated by the RNUR plugin; coordinate decode | `O(1)` |
| **Plugin manifest** | Native plane or `> 0x7FFFFFFF` | The mounted plugin's property manifest | `O(1)` after mount |

**1.3.3 Bitmask packing for freestanding kernels**

SUCPM defines how property sets are bit-packed into 32-bit and 64-bit metadata
structures, so core kernel subsystems (SUAS-001 SDF, SUAS-003 SBR, and
SUTS-002 SRE) can execute property tests with fast bitwise operations in `O(1)`
time, with no heap memory and no dynamic string parsing. The layouts are given
in §5.

### 1.4 Property specification matrix

| Property tier | Range target | Evaluation strategy | Primary kernel consumers |
|---------------|--------------|---------------------|--------------------------|
| SUCD table properties | `0x000000–0x10FFFF` | Binary-tree / flat-table lookup in SUCD | `suas_sucd` (CCC, composition), `suas_sdf` (BiDi), `suts_sre` (`\p{L}`) |
| SCP directorial properties | `0x110000–0x11FFFF` | Fixed bitmask decoding from the SCP instruction ID | Kernel IPC pipeline, `suas_sdf` (isolates), `suas_sbr` (hard breaks) |
| Native plane properties | `0x120000–0x7FFFFFEF` | Register-mapped range property masks | `sust` (serialization), the framebuffer/rendering pipeline |
| Neography registry properties | `0x160000–0x16FFFF` | RNUR coordinate decode (plugin-populated Native range) | The RNUR neography plugin, `suts_sre` (`\p{RNUR}`) |

The Native plane range ends at `0x7FFFFFEF`; the 16 terminal values
`0x7FFFFFF0–0x7FFFFFFF` are reserved as the 15 Kernel Security Traps and the
Sentinel (SUTR-0 §6), and are never allocated.

The Neography / registry range is a designated carve-out **inside** the Native
plane. It belongs to the **Reddit Neographical Unicode Registry (RNUR)**, a
*codepoint* registry for neographies, original scripts, and conscripts:

| Registry | Range | Nature |
|----------|-------|--------|
| **RNUR** | `0x160000–0x16FFFF` | Reddit Neographical Unicode Registry — a PUA coordinate system decoded into the Native space |

RNUR data reaches the kernel as a SuperUnicode plugin (SUTR-5); until the RNUR
plugin is mounted, the range contributes no properties.

> **Note.** RNUR is a **PUA coordinate system**: its allocations are
> coordinates in the Unicode Private Use Areas, because the registry is chiefly
> consumed by Unicode text rather than by SuperUnicode. SuperUnicode does not
> mirror those PUA coordinates; it decodes the registry into the Native-plane
> carve-out `0x160000–0x16FFFF` (§4.4), where a mounted RNUR plugin supplies
> the contributed masks. The **RCCR** (Reddit Conlang Code Registry) is
> deliberately **not** part of this model: it is a registry of *codes*
> (BCP 47 language tags), not of codepoints, and therefore contributes no
> character property.

### 1.5 Cross-references and renumbering

Earlier drafts of this and sibling reports used a different cross-reference
numbering. The mapping is:

| Earlier identifier | Current document |
|--------------------|------------------|
| `SUAS-05` (SDF) | **SUAS-001** — Structural Directional Framing |
| `SUAS-06` (SBR) | **SUAS-003** — System Boundary & Line Break Rules |
| `SUAS-07` (SCF) | **SUAS-004** — SuperUnicode Canonical Forms |
| `SUAS-18` (SRE) | **SUTS-002** — SuperUnicode Regular Expressions |

---

## 2. Origin and behavior of character properties

### 2.1 Character semantics

The SuperUnicode Standard views character semantics as inherent to the
definition of a character, and conformant processes are required to take these
into account when interpreting characters.

> **D3-adapted — Character semantics.** The semantics of a character are
> determined by its identity, normative properties, and behavior.

The assignment of character semantics is based on character behavior. Other
character-set standards leave it to the implementer, or to unrelated secondary
standards, to assign character semantics. In contrast, the SuperUnicode
Standard supplies a rich set of attributes, called **properties**, for each
character contained in it. Many properties are specified in relation to
processes or algorithms that interpret them. There are behaviors specific to a
particular text process that are not formally defined; implementations often
provide internal properties to achieve the desired behavior, and the concepts
here apply to those cases too.

### 2.2 Character behavior in context

The interpretation of some properties (such as whether a character is a digit)
is largely independent of context, whereas the interpretation of others (such
as directionality) applies to a character sequence as a whole. Title casing,
script classification of punctuation, the line-breaking rules of SUAS-003
(SBR), and glyph shaping are all contextual. Isolated character properties
typically tell only part of the story.

In some cases the expected behavior depends on external context — the document
type, the language, or the cultural expectations of the user. Where a
reasonably generic set of property values can be assigned, such properties are
defined as part of the standard; such properties and their algorithms define
useful default behavior that may be tailored to more specific requirements.

### 2.3 Relation of character properties to algorithms

When modeling character behavior with computer processes, formal character
properties are assigned to achieve the expected results. Such modeling depends
heavily on the algorithms used. In some cases a property is specified in close
conjunction with an algorithm; in others the algorithms are implied. General
properties may require occasional implementation-specific adjustments, which
are usually achieved by overriding properties for specific algorithms (§6.3).

When assigning properties for a given algorithm it may be tempting to assign
arbitrary values, so long as the algorithm produces the expected results. Doing
so hides the nature of the character and limits re-use. Therefore the standard
pays careful attention to the underlying linguistic identity of the character.
Formal properties alone are not sufficient to describe the complete range of
acceptable character behaviors.

> **Note.** In some cases the relevant algorithm is not defined in the standard.
> The algorithm that converts strings of digits into numerical values, for
> example, is not defined, but implementations nevertheless refer to
> `Numeric_Value`.

### 2.4 Code point and abstract character properties

**Code point properties** are properties of code points per se: in a character
encoding standard these are independent of any assignment of abstract
characters to those code points.

Examples of code point properties include:

- Code point XXX is a Kernel Security Trap.
- Code point XXX is the Sentinel.
- Code point XXX is a reserved code point.
- Code point XXX is reserved for a System Control Plane directive.
- Code point XXX is earmarked for a right-to-left script.
- Code point XXX is a Pattern_Syntax code point.
- Code point XXX is located in the Native plane.

**Character properties** are those an abstract character has independent of any
consideration of its encoding, for example: G is an alphabetic character; G is
in the Latin script; G is an uppercase letter.

General text-processing algorithms and the APIs through which they are accessed
must be prepared to deal with **any** code point, even one unassigned at the
time the implementation was created. They nearly always need a defined value
for every code point, even if that value is `unknown` or `inapplicable`. This
leads to the unifying concept of the **Encoded Character Property**: it combines
the code point property (associating ranges of code points with default values)
with the character property (associating specific values to assigned
characters), so it is no longer necessary to account for the two separately.

### 2.5 Properties applied to strings

Character and code point properties are defined such that all assigned
characters and all code points have a defined property value, even if that
value is "N/A". Assigned characters and code points each form a finite set;
this is generally not true for strings. Some properties for strings can be
described algorithmically, via **String Functions**; others apply only to a
specific set of strings listed explicitly, the **properties of an enumerated
set of strings**. These are elaborated in §3.6 and §3.7.

### 2.6 Normative properties

A **Normative Property** is a character property used in the specification of
the standard. Whenever a normative process depends on a property in a specified
way, that property is designated normative.

Specification that a property is normative means that implementations claiming
conformance to a particular version of the standard, and which make use of that
property, must follow the standard's specifications for that property. For
example, the Directional_Type property is required for conformance whenever
rendering text that requires bidirectional layout.

The fact that a property is normative does not mean its values never change.
Corrections and extensions may require minor changes, though the committee
strives to minimize such changes. Some normative algorithms depend critically on
particular values: canonical ordering cannot be overridden. Other normative
properties, such as case mapping, are overridable by higher-level protocols
because their intent is to provide a common basis for behavior.

By making a property normative and non-overridable, the standard guarantees that
conformant implementations can rely on one another to interpret the character
the same way. This is most useful where precise interpretation rules exist.

### 2.7 Informative properties

An **Informative Property** is a property whose values are provided for
information only. A conformant implementation is free to use or change
informative values as required, while remaining conformant. An implementer may
establish a protocol to convey that particular informative properties are being
used in distinct ways.

Informative properties capture expert implementation experience. When such a
property is explicitly specified in SUCD, its use is strongly recommended.
An informative property may become normative in a later version if its use
acquires conformance implications. Properties may be informative because their
exact nature or applicability is unclear, or because existing implementations
show a range of equally useful behaviors. Tentative properties may instead be
designated **provisional** (§7.4).

### 2.8 Contributed and registry properties

The SuperUnicode model adds a third origin: **Contributed / Registry
properties** are external codepoint metadata supplied by a registry. The
current registry is the **Reddit Neographical Unicode Registry (RNUR)**:

| Registry | Range | Contributes |
|----------|-------|-------------|
| **RNUR** | `0x160000–0x16FFFF` | Neographies, original scripts, and conscripts, allocated in the Unicode Private Use Areas under the coordinate-pair system `(Set_Number, Code_Point)` and decoded into the Native space |

Contributed properties are informative in status: a conformant process may
ignore them, but where it honors them it must honor the registry's assignment.
Admission is owned by the registry, not by the kernel. RNUR data is delivered
as a SuperUnicode plugin (SUTR-5), so the base standard defines the registry's
range but leaves its contents to the plugin.

The **RCCR** (Reddit Conlang Code Registry) is intentionally outside this
model: it is a registry of codes — BCP 47 language tags — not of codepoints, and
a process therefore evaluates no character property from it.

### 2.9 Referring to properties

**Property aliases** and **property value aliases** define names and
abbreviations used to refer to properties and property values. These names can
be used for XML formats of SUCD data, for regular-expression property tests,
and other programmatic textual descriptions. The names themselves are not
normative except where they correspond to normative properties; other standards
may make normative references to both normative and informative aliases.

There is one abbreviated name and one long name for most properties.
Additional aliases may be added at any time. Property value names are **not**
unique across properties. The combination of property name and property value
is, however, unique. The case distinctions, whitespace, and `_` in property
names are not normative; unless a specific form is required, all forms are
equivalent.

To reference a given property and property value, the aliases are used, as in:

> The property value `Uppercase_Letter` from the `General_Category` property,
> as specified in v0.1.0 of the SuperUnicode Standard.

Then cite that version of the standard.

### 2.10 The SuperUnicode Character Database (SUCD)

SUCD is the main repository for machine-readable character properties. It
consists of a number of files containing property data, plus documentation
explaining the organization and format. The current release lives under
`Public/0.1.0/sucd/`:

| File | Contents |
|------|----------|
| `SUCSData.txt` | Base SUCS codepoint inventory (`<codepoint>;<name>;<type>`) |
| `Props/PropList.txt` | Binary property assignments (`<hex-range>;<property>`) |
| `Props/BidiProps.txt` | Directional properties for the Base space |
| `Derived/DerivedCoreProperties.txt` | Inherited property closure over the core set |
| `Blocks.txt` | Named block ranges |

While the committee strives to minimize changes, occasionally the properties of
already-encoded characters must be updated; the relevant data files are then
revised and posted as an update version. A visual documentation of codepoint,
name, and reference glyph, augmented with excerpts from some properties, can be
found in the character charts.

The files themselves are standardized, together with their compiled binary
form, by [SUAS-012](../suas/SUAS-012-sucd.md) (SUCD): the record layout, the
three-kinded page directory, the property mask, the resolution rules for
overlapping ranges, and the O(1) lookup contract. This report describes the
property *model*; SUAS-012 describes the *database* that carries it.

---

## 3. Definitions

The following presents a consistent set of definitions related to character
properties. Where possible these match the formal definitions in the standard.
In those cases the original number is given in square brackets. It is
customary to prefix the term "SuperUnicode" when referring to these definitions
in other contexts: "Character Property" becomes "SuperUnicode Character
Property".

### 3.1 Properties and property values

**PD1. Property.** A named attribute of an entity in the standard, associated
with a defined set of values. [D19]

**PD2. Code Point Property.** A property of code points. [D20] A code point
property defines a set of values and a mapping from each code point to one of
the values.

**PD3. Abstract Character Property.** A property of abstract characters. [D21]

**PD4. Encoded Character Property.** A property of encoded characters in the
standard. [D22] It defines a set of values and a mapping from each code point
to one of the values, and typically maps a default value to any code point not
assigned to a character. In the rest of this report, "character property" or
"property" without qualifier includes code point, abstract character, and
encoded character properties.

**PD5. Property Value.** One of the set of values associated with a property.
[D23] For example, `East_Asian_Width` has the values Narrow, Neutral, Wide,
Ambiguous, and Unassigned.

### 3.2 Types of property values

**PD6. Explicit Property Value.** A value for an encoded character property
explicitly associated with a code point in a SUCD data file. [D24]

**PD7. Implicit Property Value.** A value for an encoded character property
given by a generic rule or an "otherwise" clause in a SUCD data file. [D25]

**PD8. Default Property Value.** The value (or small set of values) associated
with unassigned code points or with characters for which the property is
irrelevant. [D26] There may be more than one default per property, with
different values for different ranges, as in the Bidi property.

### 3.3 Types of properties

**PD9. Enumerated Property.** A property with a small set of named values.
[D27]

**PD10. Closed Enumeration.** An enumerated property whose value set is closed
and will not be extended. [D28] General_Category and Bidi_Class are closed
enumerations, other than Boolean properties.

**PD11. Boolean Property.** A closed enumerated property whose values are
limited to true and false. [D29] The presence or absence of the property is the
essential information.

**PD12. Numeric Property.** A property whose value is a number that can take on
any integer or real value. [D30] `Numeric_Value` is an example.

**PD13. String-Valued Property.** A property whose value is a string. [D31]
`Canonical_Decomposition` is a string-valued property.

**PD13a. Identifier Property.** A string-valued property that represents a
member of a namespace, with additional rules defining identifier
well-formedness, uniqueness, and comparison. A character name is an example.

**PD14. Catalog Property.** An enumerated property, typically unrelated to an
algorithm, that may be extended in each successive version. [D32] Age, Block,
and Script are examples.

**PD15. Miscellaneous Property.** A property not of type Boolean, Enumerated,
Numeric, String-valued, Identifier, or Catalog. `Script_Extensions` is an
example.

### 3.4 Conformance status of properties

**PD16. Normative Property.** A property used in the specification of the
standard. [D33] A normative process that depends on a property in a normative
and testable way is usually sufficient reason to designate it normative.

**PD17. Overridable Property.** A normative property whose values may be
overridden by conformant higher-level protocols. [D34]

**PD18. Informative Property.** A property whose values are provided for
information only. [D35]

**PD19. Provisional Property.** A property whose values are unapproved and
tentative, and which may be incomplete or otherwise not in a usable state.
[D36] Such properties may be removed from future versions without prior notice.

### 3.5 Classification of properties

In these definitions, "code point" is used inclusively to mean code point for a
code point property and character for a character property.

**PD20. Context-dependent Property.** A property that applies to a code point
in the context of a longer sequence. [D37] The lower-case mapping of Greek
sigma is an example.

**PD21. Context-independent Property.** A property that applies to a code point
in isolation. [D38]

**PD22. Stable Transformation.** A transformation `T` on a property `P` is
stable with respect to an algorithm `A` if `A(T(P)) = A(P)` for all code
points. [D39]

**PD23. Stable Property.** A property is stable with respect to a process as
long as possible changes to its values preserve the result of the algorithm for
all previously assigned code points. [D40] CCC is stable with respect to
normalization even though its absolute values are not guaranteed across
versions.

**PD24. Fixed Property.** A property whose values (other than the default),
once associated with a character, are fixed and will not be changed except to
correct obvious errors. [D41] Age is an example.

**PD25. Immutable Property.** A fixed property that is also subject to a
stability guarantee preventing any change to the published listing other than
assignment of new values to formerly unassigned code points. [D42] The
character name is immutable.

**PD26. Stabilized Property.** A property that is neither extended to new
characters nor maintained, but retained in SUCD. [D43] A stabilized property is
also fixed.

**PD27. Deprecated Property.** A property whose use is discouraged. [D44]
Where sufficient legacy support exists, a deprecated property may be extended
to new characters to keep it usable and consistent.

**PD28. Simple Property.** A property whose values are specified directly in
SUCD and cannot be derived from other simple properties. [D45]

**PD29. Derived Property.** A property whose values are algorithmically derived
from a combination of simple properties. [D46]

**PD30. Property Alias.** A unique identifier for a particular property. [D47]
The set of property aliases forms a namespace.

**PD31. Property Value Alias.** A unique identifier for a particular
enumerated value for a particular property. [D48] The value aliases for each
property form a separate namespace; values from different properties may share
names.

### 3.6 Strings

**PD32. String.** An ordered sequence of zero or more code points. All code
points in a string are from the same character encoding.

**PD32a. Empty String.** A string consisting of exactly zero code points.

**PD32b. Code Unit Sequence.** An ordered sequence of one or more code units.
[D78]

**PD32c. SuperUnicode String.** A code unit sequence containing code units of a
particular SUTF encoding form. A single string must not mix forms (SUTR-1).

**PD32d. Coded Character Sequence.** An ordered sequence of one or more code
points. [D12] It may also include noncharacters or reserved code points.

In programming contexts, strings are almost always defined to include the empty
string. In this model the term "SuperUnicode string" is stipulated to also
refer to a coded character sequence and to include the empty string.

### 3.7 Properties of strings

**PD32e. Enumerated Set of Strings.** A set of strings enumerated by an
explicit, finite list of members. An empty string may be listed as a member.

**PD32f. Property of Strings.** A character property whose domain extends to
strings, as opposed to single code points. The same categorizations of types,
values, and statuses apply as for encoded character properties.

**PD32g. Explicit Property of Strings.** A property of strings for which each
value is specified explicitly for each member of an enumerated set of strings.
For example, `RGI_Emoji_Flag_Sequence` is True for a listed sequence and False
otherwise.

**PD32h. Algorithmic Property of Strings.** A property of strings whose values
are determined by a string function applied to the entire string (offsets 0 and
n). `isLowercase` and `isEmojiFlagSequence` are examples.

### 3.8 String functions

**PD33. Offset.** A number from 0 to n, where n is the length of the string in
code points. It indicates a position logically between code points; 0 is before
the first code point and n is after the last. Dealing with offsets at the level
of code units is the concern of lower-level processes (SUTR-1, SUTR-4).

**PD34. [Definition removed.]**

**PD35. String Function.** A function whose input is a string S and two offsets
a and b, with a <= b.

**PD36. Text Boundary Property.** A string function whose value is defined for
a particular offset. Text boundary functions are also called segmentation
functions. A simple one, `IsBreak(S,a,b)`, minimally returns a Boolean; others
may return additional information, such as a break priority for SUAS-003 (SBR).

### 3.9 Classification of string functions

**PD37. Context-independent String Function.** A string function `F` for which
`F(S,a,b)` is independent of the content of S before a and after b.

**PD38. Context-dependent String Function.** A string function that is not
context-independent. Any boundary function of the form `B(S,x,x)` is
context-dependent by definition.

**PD39. String Transform.** A string-valued string function.

**PD40. Idempotent String Function (Folding).** A string transform `F` with
`F(F(S)) = F(S)` for all inputs. A folding establishes an equivalence relation,
where `X` and `Y` are equivalent if and only if `F(X) = F(Y)`. The notation
`toX(s)` denotes the folding, and
`isX(s)` the corresponding test. Case folding and normalization are examples;
normalization is context-dependent.

**PD41. Code Point Count Preserving String Function.** A string function whose
result contains the same number of code points as its input.

**PD42. Buffer Length Preserving String Function.** A string function whose
result contains the same number of code units as its input.

### 3.10 Other definitions

**PD43. Higher-level Protocol.** Any agreement on the interpretation of
characters that extends beyond the scope of the standard. [D16]

---

## 4. The property evaluation engine

Every property test in a freestanding SuperUnicode kernel goes through one
range-dispatched function. It allocates nothing, parses no strings, and returns
a packed mask.

### 4.1 Bridge lookup

For `0x000000–0x10FFFF` the engine indexes the SUCD property tables. The tables
are stored as sorted range lists (`<lo><hi>;<mask>`); lookup is a binary search
over `R` ranges, or `O(1)` where a bit-addressed index is available. The Bridge
is the only tier whose values are inherited from the Unicode repertoire, and
the only tier whose lookup cost depends on table size.

### 4.2 System Control Plane decode

For `0x110000–0x11FFFF` the engine **does not** consult SUCD. The codepoint is
structured as `0x0011_XY` with two property-bearing bytes:

| Byte | Name | Bears |
|------|------|-------|
| `X` (bits 8–15) | Directive class | Cluster identity: B+ `A0–A7`, W+ `A8–AB`, C+ `AC–AD`, S+ `AE` |
| `Y` (bits 0–7) | Argument byte | Directional flag, isolate/embed constraint, break constraint, LTR/RTL marker |

A property such as `SCP_DIR_RTL` or `SCP_BRK_MANDATORY` is therefore a bit test
on `Y`, and a property such as `BANcode` is a range test on `X`. The decode is a
shift and a mask: `O(1)`, no table.

### 4.3 Native plane range masks

For `0x120000–0x7FFFFFEF`, excluding the registry carve-out, properties come
from **range registers**: a plugin or the kernel publishes a small list of
`<lo><hi><mask>` tuples, and the test is a bounded scan of a compile-time
constant number of registers. Because the register count is fixed, the test is
`O(1)`. This is the tier that carries `Printable`, `Trap_Instruction`, and the
framebuffer's glyph-class masks.

### 4.4 RNUR registry decode

For the RNUR Neography Registry Space `0x160000–0x16FFFF`, the engine decodes a
registry coordinate and yields the `RNUR` bit plus the plugin's contributed
mask:

```c
if (cp >= SRE_RNUR_MIN && cp <= SRE_RNUR_MAX)   /* 0x160000-0x16FFFF */
    return SUCPM_F_RNUR | rnur_plugin_mask(cp);
```

The registry is a Native-space carve-out, so the range test is `O(1)`.

### 4.5 Plugin manifest masks

For codepoints strictly above the Sentinel (`> 0x7FFFFFFF`), properties are
supplied by the mounted plugin's property manifest (SUTR-5). The manifest is
loaded once, at mount time, after the checksum gate; a test against a mounted
codepoint is a register read, `O(1)`. A plugin may equally contribute Native
plane ranges (`0x120000–0x7FFFFFEF`), which publish as the range registers of
§4.3 rather than as an above-Sentinel manifest.

### 4.6 Dispatch

```c
static inline uint64_t sucpm_props(sucs_ex_char_t cp) {
    if (cp <= 0x0010FFFFULL) return sucd_lookup(cp);      /* Bridge          */
    if (cp <= 0x0011FFFFULL) return scp_decode(cp);       /* SCP             */
    if (cp <= 0x0015FFFFULL) return native_range(cp);     /* Native low      */
    if (cp <= 0x0016FFFFULL) return rnur_decode(cp);      /* RNUR registry   */
    if (cp <= 0x7FFFFFEFULL) return native_range(cp);     /* Native high     */
    if (cp <= 0x7FFFFFFFULL) return SUCPM_MASK_RESERVED;  /* terminal 16     */
    return plugin_manifest(cp);                            /* plugin (ExtSUCS) */
}
```

The order of the tests is normative: the registry carve-out must be tested
before the general Native range, and the terminal 16 values must be rejected
before ExtSUCS dispatch.

---

## 5. Bitmask packing

SUCPM packs a property set into two widths. Both are plain unsigned integers,
so they can be produced by a lookup and consumed by a bitwise test with no
intermediate structure.

### 5.1 The 32-bit core mask

`sucpm_flags32_t` carries the tier, the registry sector, the Bidi field, and
the common binary properties.

| Bits | Field | Meaning |
|------|-------|---------|
| 0–5 | Bidi field | Mirrors the `SUCD_BIDI_*` layout: LTR, RTL, ARABIC_AL, NEUTRAL, MIRRORED, WHITESPACE |
| 6 | `SUCPM_F_ALPHABETIC` | Alphabetic |
| 7 | `SUCPM_F_UPPERCASE` | Uppercase |
| 8 | `SUCPM_F_LOWERCASE` | Lowercase |
| 9 | `SUCPM_F_WHITE_SPACE` | White_Space |
| 10 | `SUCPM_F_MARK` | General_Category `M*` |
| 11 | `SUCPM_F_NUMBER` | General_Category `N*` / Decimal_Number |
| 12 | `SUCPM_F_PUNCTUATION` | General_Category `P*` |
| 13 | `SUCPM_F_SYMBOL` | General_Category `S*` |
| 14 | `SUCPM_F_JOIN_CONTROL` | Join_Control |
| 15 | `SUCPM_F_DEFAULT_IGNORABLE` | Default_Ignorable_Code_Point |
| 16 | `SUCPM_F_NONCHARACTER` | Noncharacter_Code_Point |
| 17 | `SUCPM_F_ASSIGNED` | Assigned |
| 18 | `SUCPM_F_CANONICAL_DECOMPOSABLE` | Has a canonical decomposition |
| 19 | `SUCPM_F_COMPOSITION_EXCLUSION` | Full_Composition_Exclusion |
| 20 | `SUCPM_F_BIDI_CONTROL` | Bidi_Control |
| 21 | `SUCPM_F_VARIATION_SELECTOR` | Variation_Selector |
| 22 | `SUCPM_F_RNUR` | RNUR neography registry |
| 23 | Reserved | Must be zero |
| 24–27 | Tier | 0 = Bridge, 1 = SCP, 2 = Native, 3 = RNUR registry, 4 = ExtSUCS |
| 28–31 | Reserved | Must be zero |

### 5.2 The 64-bit extended mask

`sucpm_flags64_t` embeds the core mask in its low 32 bits and packs enumerated
property **values** into the high 32 bits, so an enumerated property test is
also a single mask-and-compare.

| Bits | Field | Width |
|------|-------|-------|
| 0–31 | Core mask (as §5.1) | 32 |
| 32–36 | General_Category value | 5 |
| 37–44 | Canonical_Combining_Class value | 8 |
| 45–54 | Script_Identifier value | 10 |
| 55–60 | Line_Break_Class value | 6 |
| 61–63 | Numeric_Type value | 3 |

### 5.3 O(1) tests

```c
typedef uint64_t sucpm_flags64_t;

static inline bool sucpm_has(sucpm_flags64_t m, uint64_t bits) {
    return (m & bits) == bits;                     /* binary property test  */
}

static inline bool sucpm_is(sucpm_flags64_t m, uint64_t field_bits, uint64_t value) {
    return ((m >> field_bits) & 0xFFu) == value;   /* enumerated test       */
}
```

A face that consumes properties (SDF, SBR, SUCF, SRE) receives the packed mask
and performs only shifts, masks, and compares. It never allocates, and it never
re-parses a property name.

---

## 6. Conformance-related considerations

This report does not itself define conformance requirements, but the following
summarizes the requirements related to character properties.

### 6.1 Conformance requirements

A process must interpret a coded character sequence according to the character
semantics established by the standard, if it interprets that coded character
sequence at all. The semantics of a character are established by taking its
coded representation, character name, and representative glyph in context, and
are further defined by its normative properties and behavior. Neither the name
nor the glyph can be relied upon absolutely: a character may have a broader
range of use than the most literal interpretation of its name, and the
representative glyph is only indicative of a range of typical glyphs.

### 6.2 Algorithms and character properties

Algorithms are specified as an idealized series of steps performed on an input
of character codes and their associated properties. An implementation claiming
conformance to an algorithm need only guarantee that it produces the same
results as the logical description; it is not required to follow the described
procedure. Likewise, as long as the same results are achieved, the
implementation is not required to use the actual properties published in SUCD.
Overriding a property value therefore does not necessarily imply an actual
change in property assignments, only that the conformant algorithm now produces
the same result as if the values had changed in the ideal description.

### 6.3 Overriding properties via higher-level protocols

Some normative behavior is default behavior that can be overridden by
higher-level protocols; in the absence of such protocols, the behavior must be
observed. Overrides can take many forms, including providing artificial context
for a context-dependent string function, applying the algorithm to a substring,
emulating format control characters in markup, reassigning a property value
during processing or rendering, or changing the result of a string function for
particular inputs.

Where overrides involve normative properties, specific restrictions apply:

- The character-combination properties and canonical ordering behavior cannot
  be overridden by higher-level protocols.
- Some normative properties are fully overridable — for example,
  General_Category.

Any and all informative properties may be overridden. However, if doing so
changes the result of an algorithm, an implementation wishing to conform to
that algorithm must indicate that overrides have been applied.

---

## 7. Updating properties and extending the standard

### 7.1 Updating properties

Updates to SUCD properties can be required for three reasons: to cover new
characters, to add new properties, and to change assigned values for some
characters. Changing a character's property assignment can impact existing
implementations and is done judiciously, only when there is no better
alternative. As less well-documented scripts are encoded, exact properties and
behavior may not be known at first, and are expected to change as information
becomes available. All updates are subject to the stability guarantees below.

### 7.2 Stability guarantees

The standard guarantees the stability of character assignments: the identity of
a character at a location remains the same. Once a character is encoded, its
properties may still change, but not so as to change the character's
fundamental identity. In addition, one or more of the following aspects may be
guaranteed invariant:

1. stability of assignment;
2. stability of result when applying the property;
3. stability of the set of values for a property;
4. stability of relation to another property;
5. stability of file formats.

Note that the status of a property as normative does not imply a stability
guarantee.

**7.2.1 Stability of assignment**

Stability of assignment is the characteristic of an immutable property. Once a
character is encoded, its code point and name are immutable. An immutable
property lets software and documents refer to its values without tracking
future updates. One side effect is that errors in property values cannot be
fixed: mistakes in naming are annotated with an alias, but the formal name
remains unchanged, even for clear-cut typographical errors. Because Code_Point
is immutable, a character found to be unnecessary is never removed; it is
deprecated instead, and the interpretation of existing documents is unchanged.

**7.2.2 Stability of result when applying the property**

Stability of result is the characteristic of a stable property. Once a
character is encoded, its CCC and decomposition are stable with respect to
normalization: if a string contains only characters from a given version and is
normalized under that version, it remains normalized under any future version.
Unlike code and name, some guaranteed-stable properties may be corrected in
exceptional circumstances that are clearly defined by the stability policy,
provided the mistake is obvious and any alternative would violate the
character's identity.

**7.2.3 Stability of the set of values**

For most properties, additional values may be created and assigned to new and
existing characters. For others the value set is fixed or limited. The set of
`General_Category` or `Bidi_Class` values is fixed, and combining classes are
limited to the values 0–254.

**7.2.4 Stability of relation to another property**

Once a character has a value for one property, it is often likely to have a
particular value for another. These relations are used when assigning
properties to new characters and when checking internal consistency. In some
cases the dependencies are explicitly guaranteed: for example, all characters
other than those of General_Category `M*` have combining class 0.

**7.2.5 Stability of file formats**

In principle the presentation of property information is independent of its
definition. In practice, keeping file formats stable makes updates easier to
track. Canonical and compatibility mappings are always in canonical order, and
the recursive decomposition is also in canonical order; canonical mappings are
limited to a single value or a pair, and the second character of a pair cannot
itself have a canonical mapping. As an alternative to semicolon-separated text
files, SUCD is also available in structured form.

### 7.3 Consistency of properties

In an ideal world all properties would be self-consistent. The standard is the
product of compromises; it balances uniformity of treatment against
compatibility with existing practice, so a certain number of anomalies is
expected. It may be advantageous for an implementation to override anomalous
values to increase uniformity and efficiency, so long as the results do not
conflict with the normative properties. Values assigned to new characters are
generally defined so related characters are consistent, unless deliberate
exceptions are needed.

### 7.4 Provisional properties

Some information in SUCD is provisional data, capturing partial or preliminary
information that may contain errors or omissions or may not be ready for
systematic use. Such data is included partly to encourage review and
improvement. Tags in auxiliary databases are a typical source of provisional
values.

### 7.5 Stabilized properties

Occasionally an existing property may be better represented by other
properties, or it may no longer make sense to extend it. Such a property is
designated stabilized. For backwards compatibility it remains part of SUCD but
is not updated or corrected. `Hyphen` is an example.

---

## 8. Special property values

### 8.1 Not applicable value

Limited properties apply to only a subset of characters. Where such a property
is implemented as a partition of the code space, characters to which the
property does not apply are given a special value denoting that it is not
applicable — the explicit value `NA`, or for some properties another value such
as `XX`.

### 8.2 Default values

Implementations often need specific properties for all code points, including
unassigned ones. The standard therefore assigns default properties to ranges of
code points, and implementations should handle additions gracefully. Three
strategies exist:

1. Rely on the recommended defaults for all code points.
2. Treat the unassigned areas of a block as if they had values common to the
   block, bridging small allocation gaps with the values of the surrounding
   characters.
3. Give an unassigned code point an implementation-defined default that results
   in graceful, if not completely correct, behavior if a character is later
   assigned there.

Each strategy has advantages and drawbacks, and none guarantees that an
implementation conformant to a prior version supports later characters in
precisely the same way. In principle default values are temporary: they are
superseded by final assignments. For noncharacter code points, a property
function returns the same value as the default for unassigned characters.

### 8.3 Preliminary property assignments

Sometimes a determination of property values can be made on incomplete or
preliminary information, and the value may change when better information
arrives. There is currently no machine-readable confidence indicator; the
standard or the defining report may provide general indications of preliminary
status. This is distinct from provisional properties, where the entire property
is preliminary.

---

## 9. Compliance matrix

| Requirement | Mandatory | Verification |
|-------------|-----------|--------------|
| Three property tiers (normative, informative, contributed) are defined | Yes | §1.3.1 |
| Tier determines overridability | Yes | §1.3.1, §6.3 |
| Bridge `0x000000–0x10FFFF` is evaluated from SUCD tables | Yes | §4.1; `Public/0.1.0/sucd/` |
| SCP `0x110000–0x11FFFF` bypasses SUCD and decodes in `O(1)` | Yes | §4.2 |
| Native plane `0x120000–0x7FFFFFEF` uses register range masks | Yes | §4.3 |
| Neography registry is the RNUR range `0x160000–0x16FFFF`, a Native carve-out | Yes | §4.4, §1.4 |
| RNUR is a PUA coordinate system decoded into the Native carve-out | Yes | §1.4, §2.8 |
| RCCR is excluded (a code registry, not codepoints) | Yes | §1.4 |
| Terminal 16 values are never allocated | Yes | §1.4; SUTR-0 §6 |
| Plugin (Native and above-Sentinel) properties come from the mounted plugin | Yes | §4.5; SUTR-5 |
| Dispatch order is normative | Yes | §4.6 |
| 32-bit core mask layout is fixed | Yes | §5.1 |
| 64-bit extended mask packs enumerated values | Yes | §5.2 |
| Property tests are `O(1)` and allocation-free | Yes | §5.3 |
| Bidi bits match the `SUCD_BIDI_*` layout | Yes | §5.1; `include/suas/suas_sucd.h` |
| Normative CCC/ordering cannot be overridden | Yes | §6.3; SUAS-004 (SUCF) |
| SUCD is the machine-readable property repository | Yes | §2.10 |
| This report does not define property values or algorithms | Yes | §1.2 |

---

## 10. Reference implementation

SUCPM is implemented by `include/sutr/sutr_sucpm.h` and `src/sutr/sutr_sucpm.c`
as a freestanding C99 static library (`sutr_static`), with the host unit tests
in `tests/test_sutr_sucpm.c`. It registers with the SuperUnicode Specification
Registry & Cross-Reference Engine (SSRE, SUAS-010) as specification `SUTR-8`
with reference unit `sutr_sucpm`, kind `REFERENCE`, version `0.1.0`.

### 10.1 The range-dispatched engine

`sutr_sucpm_props()` is the §4.6 dispatch, in the normative order: Bridge, SCP,
Native low, RNUR registry, Native high, terminal 16, plugin manifest. The
Registry carve-out is tested *before* the general Native range and the terminal
16 values are rejected *before* ExtSUCS dispatch, because the order is part of
the specification and not an implementation detail.

`sutr_sucpm_tier_of()` reports the tier a codepoint dispatches to, so a
consumer can assert the dispatch itself rather than infer it from a mask.

### 10.2 The Bridge table

The Bridge tier is a static sorted range list, and the lookup is a lower-bound
binary search over it: `O(log R)` for a table of `R` ranges, and `O(1)` in
memory because the table is a compile-time constant. The list is sorted by low
endpoint and no two ranges overlap — the binary search rests on both claims, so
the unit test checks them directly instead of asserting them in a comment.

`sutr_sucpm_bridge_range_count()` and `sutr_sucpm_bridge_range_at()` expose the
table to tooling, so a build gate can verify the ordering without reaching
into the translation unit.

### 10.3 The range registers

The Native and plugin tiers are fixed-capacity publishable registers:
`sutr_sucpm_publish_native_range()` and `sutr_sucpm_publish_plugin_range()`
write into arrays of compile-time capacity, and the lookup is a bounded scan of
that fixed count — which is what makes the tier `O(1)`. The engine stamps the
tier onto a published range and masks off any tier bits the caller supplied,
so a publisher cannot mis-declare the tier of its own range.

### 10.4 The packed mask

The core mask occupies bits 0–31 and the enumerated values pack into the high
32 bits: General_Category at 32–36, Canonical_Combining_Class at 37–44,
Script_Identifier at 45–54, Line_Break_Class at 55–60, and Numeric_Type at
61–63. `sutr_sucpm_has()` is the binary-property test and `sutr_sucpm_is()` is
the enumerated-property test; both are a shift, a mask, and a compare, and
neither parses a property name.

Bits 23 and 28–31 are reserved by this report and assigned by the companion
SUTR-9 revision to the `Math` membership flag and the 4-bit `MathClass` field
(SUTR-9 §4.3). The two modules therefore agree by construction rather than by
convention.

### 10.5 The SCP decode

The System Control Plane decode is a shift and a mask on the `0x0011_XY`
structure. The directional, isolate/embed, and break constraints are bit
positions *within the argument byte Y*, not within the packed mask, so
`sutr_sucpm_scp_is_rtl()`, `_is_ltr()`, `_is_isolate()`, `_is_embed()`,
`_forces_break()`, and `_forbids_break()` are direct bit tests on Y.

### 10.6 Verification

The unit test suite covers the version and metadata, the mask layout and the
field widths, the dispatch order at every tier boundary, the Bridge table
ordering and non-overlap, the range registers and the tier stamping, the
property model vocabulary, the enumerated value catalogs, the math repertoire
extended over the operator and letterlike blocks, the taxonomy names, and the
SSRE registration.

---

## 11. Terms & conventions

- **SUCPM** — SuperUnicode Character Property Model; this report.
- **SUCD** — SuperUnicode Character Database; the machine-readable property
  tables under `Public/0.1.0/sucd/`, standardized with their binary image by
  SUAS-012.
- **Property** — a named attribute of an entity, associated with a defined set
  of values (PD1).
- **Encoded Character Property** — the unified code point + character property
  (PD4).
- **Normative / informative / contributed** — the three property tiers (§1.3.1).
- **Simple / derived** — properties specified directly vs. computed from simple
  properties (PD28, PD29).
- **Stable / fixed / immutable / stabilized / deprecated** — the stability
  classifications (PD23–PD27).
- **Bridge** — the Unicode Compatibility Space `0x000000–0x10FFFF` (SUTR-6).
- **SCP** — System Control Plane `0x110000–0x11FFFF` (SUTR-0).
- **Native plane** — `0x120000–0x7FFFFFEF`, the OpenWindows native allocations.
- **RNUR** — Reddit Neographical Unicode Registry; the contributed codepoint
  registry at `0x160000–0x16FFFF`, delivered as a SuperUnicode plugin (SUTR-5).
- **RCCR** — Reddit Conlang Code Registry; a registry of BCP 47 codes, not
  codepoints, and therefore outside this model.
- **CCC** — Canonical_Combining_Class.
- **Core mask / extended mask** — the 32-bit and 64-bit packed property sets
  (§5).
- **Sentinel** — `0x7FFFFFFF`; the terminal 16 values are never allocated.
- **UTR #23** — Unicode Technical Report #23, the model this report mirrors.
- **SDF / SBR / SUCF / SRE** — SUAS-001, SUAS-003, SUAS-004, and SUTS-002.

---

**END OF SUTR-8**
