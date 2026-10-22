# SUTR-9 — SuperUnicode Mathematical Notation and Layout Support (SUMNLS)

**Standard ID:** SUTR-9
**Status:** 0.1.0 — Published
**Scope:** The mathematical character repertoire, the math property set, and the kernel box-model layout engine across the SUCS and ExtSUCS codepoint space
**Applies To:** SUCD, SUTR-0 (SUCS Core), SUTR-6 (Bridge), SUTR-7 (SUCEM), SUTR-8 (SUCPM), SUAS-002 (SGW), SUAS-003 (SBR), SUAS-005 (SSP), the SUF vector font engine, and every math-consuming layout subsystem
**Last Updated:** 2026-10-18

---

## 1. Overview

Where UTR #25 *Unicode Support for Mathematics* describes how mathematics is
represented in the Unicode repertoire and laid out in OpenType MATH fonts,
**SUTR-9 (SUMNLS)** establishes the equivalent contract for SuperUnicode: the
character repertoire, the O(1) property flags, the zero-allocation spatial
layout engine, and the in-band System Control Plane directives that together let
a freestanding SuperUnicode kernel render mathematical notation without a
markup-aware font stack and without a heap.

The report answers four questions that every math consumer must share:

1. **Which characters** make up the mathematical repertoire, and how are the
   alphabetic style variants encoded?
2. **What properties** does a math consumer read to classify an operator, and
   how are they evaluated in `O(1)`?
3. **How is a formula laid out** into stacked fractions, radicals, extensible
   delimiters, and script tiers with a single, allocation-free pass?
4. **How is math context carried in-band** so a plain 64-bit code unit stream is
   self-describing to the layout engine?

The first two are the subject of the UTR #25-style body (§2–§4). The third and
fourth are the SuperUnicode-specific architecture of §5 and §6.

### 1.1 Relation to UTR #25

SUMNLS is the SuperUnicode analog of UTR #25 *Unicode Support for Mathematics*.
It adopts that report's framing: the mathematical repertoire, the `Math`
property, the `MathClass` classification of operators, the mathematical
alphanumeric symbol styles, and the OpenType MATH model of fractions, radicals,
scripts, and extensible delimiters. The UTR #25 material is restated here so the
contract is self-contained; where this report and UTR #25 differ, **this report
governs for SuperUnicode**.

The decisive difference is the domain. UTR #25 works over the 21-bit Unicode
codespace and relies on a font's MATH table for layout metrics. SUMNLS works over
the full 31-bit `SUCS_CP` space and the 64-bit `sucs_ex_char_t` extended space,
reserves a dedicated **Native Math ExtSUCS Space** for symbols Unicode cannot
carry, and folds the layout metrics into the kernel itself so the renderer needs
no external MATH table.

### 1.2 Scope

This report specifies the mathematical character repertoire and its mapping into
the Bridge and the Native Math ExtSUCS Space, the `Math` and `MathClass`
properties and their bitmask packing, the kernel box-model layout algorithm, and
the System Control Plane math directives. It does **not** define a document
markup language, a serialization format, or a font file format. MathML, TeX, and
OpenMath remain higher-level protocols (SUTR-8 §6.3); SUF remains the font
container (SUTR-4); and the property model that carries `Math` and `MathClass` is
SUTR-8 (SUCPM).

### 1.3 Key architectural concepts

**1.3.1 Mathematical alphanumeric and symbol mapping**

The mathematical repertoire is mapped in two tiers. Everything the Unicode
repertoire already carries is preserved as a permanent 1:1 Bridge mapping
(SUTR-6): the Mathematical Alphanumeric Symbols block `0x1D400–0x1D7FF`, the
Mathematical Operators block `0x2200–0x22FF`, and the surrounding symbol blocks.
Everything the repertoire does **not** carry — additional alphabet styles,
native Greek and multi-script math alphabets, large operator assembly parts, and
native fences — is allocated in the **Native Math ExtSUCS Space
`0x00170000–0x0017FFFF`** (§3).

**1.3.2 O(1) property flags in SUCD**

Every mathematical character carries two properties that a layout engine reads
on the hot path: the binary `Math` property and the enumerated `MathClass`
property, whose values mark an operator as unary, binary, relational, a fence
delimiter, a big operator, punctuation, or ordinary. Both are packed into the
SUCPM masks (SUTR-8 §5) so classification is a single mask test with no table
walk and no heap (§4).

**1.3.3 Kernel box-model and layout engine**

SUMNLS defines a **zero-allocation, single-pass spatial layout algorithm**. The
engine builds horizontal and vertical **boxes** for stacked fractions, radicals,
extensible delimiters, and sub/superscript **script tiers**, computing bounding
boxes and baseline offsets as it descends. It integrates directly with the SUF
vector font engine, which supplies glyph bounding boxes, advance widths, and the
vertical math constants; no separate MATH table is consulted (§5).

**1.3.4 System Control Plane math directives**

Mathematical structure that is not expressible as a character is carried
**in-band** by a compact block of SCP markers in the `0x00110030` range: inline
mathematical isolation, directional math-mode switches, alignment tab stops, and
explicit grouping for fractions, radicals, and script tiers (§6).

### 1.4 Math specification matrix

| Concern | Carrier | Evaluation | Primary consumers |
|---------|---------|------------|-------------------|
| Math repertoire (Bridge) | `0x1D400–0x1D7FF`, `0x2200–0x22FF`, symbol blocks | Permanent 1:1 Bridge mapping (SUTR-6) | Repertoire, charts, SUCD |
| Native math repertoire | Native Math ExtSUCS Space `0x00170000–0x0017FFFF` | Native-plane range registers (SUTR-8 §4.3) | Charts, native math plugin |
| Membership | `Math` binary flag | SUCPM core mask bit | Layout engine, SUCD tooling |
| Classification | `MathClass` enumerated value | SUCPM extended mask field | Layout engine, spacing, fences |
| Vertical metrics | SUF glyph metrics + math constants | Per-glyph register read | Box-model engine (§5) |
| Structure and context | SCP math directives `0x00110030` | SCP `O(1)` decode (SUTR-8 §4.2) | Isolation, direction, alignment |

The Native Math ExtSUCS Space is a carve-out **inside** the Native plane
(`0x00120000–0x7FFFFFEF`) and is currently unallocated; a native math plugin
(SUTR-5) supplies its contents after the checksum gate. Until then the range
contributes no characters.

### 1.5 Cross-references

| Topic in this report | Normative elsewhere |
|----------------------|---------------------|
| 31-bit `SUCS_CP` / 64-bit `sucs_ex_char_t` | SUTR-0 |
| Permanent 1:1 Unicode Bridge | SUTR-6 |
| The four-level encoding model | SUTR-7 |
| `Math` and `MathClass` property packing | SUTR-8 §5 |
| Terminal traps and the Sentinel | SUTR-0 §6 |
| Word/glyph metrics and the monospace grid | SUAS-002 (SGW) |
| Line breaking around math | SUAS-003 (SBR) |
| Script resolution for math alphabets | SUAS-005 (SSP) |

---

## 2. Mathematical characters and the repertoire

### 2.1 The mathematical repertoire

SuperUnicode, like Unicode, includes virtually all characters used in
mathematics. Following UTR #25, the repertoire is divided into three broad
groups:

- **arithmetic and algebra symbols** — the four operators, plus/minus, times,
  division, roots, factorial, and the relation set;
- **the mathematical alphanumeric symbols** — styled Latin, Greek, and digit
  variants used as mathematical identifiers;
- **the calculus, logic, and set-theoretic symbols** — n-ary operators, arrows,
  turnstiles, delimiters, and blackboard-bold operators.

The repertoire spans several Bridge blocks, chief among them:

| Bridge block | Range | Role |
|--------------|-------|------|
| Mathematical Operators | `0x2200–0x22FF` | Core operators, relations, fences |
| Miscellaneous Mathematical Symbols-A | `0x27C0–0x27EF` | Brackets, logical symbols |
| Miscellaneous Mathematical Symbols-B | `0x2980–0x29FF` | Extended brackets and fences |
| Supplemental Mathematical Operators | `0x2A00–0x2AFF` | Large n-ary and relation set |
| Mathematical Alphanumeric Symbols | `0x1D400–0x1D7FF` | Styled identifiers |
| Arabic Mathematical Alphabetic Symbols | `0x1EE00–0x1EEFF` | Arabic math identifiers |

### 2.2 Mathematical alphanumeric symbols and style variants

Mathematical identifiers are not styled by a font: the **style is part of the
character identity**. A bold `A`, an italic `A`, and a script `A` are three
distinct characters, so a formula round-trips without font context. UTR #25
formalizes the variant families; SUMNLS adopts them and extends the set in the
Native Math ExtSUCS Space (§3.4).

| Style | Example family | Bridge base |
|-------|----------------|-------------|
| Bold | bold Latin, Greek, digits | `0x1D400`, `0x1D6A8`, `0x1D7CE` |
| Italic | italic Latin | `0x1D434` |
| Bold italic | bold italic Latin, Greek | `0x1D468`, `0x1D71C` |
| Script (calligraphic) | script, bold script | `0x1D49C`, `0x1D4D0` |
| Fraktur | fraktur, bold fraktur | `0x1D504`, `0x1D56C` |
| Double-struck | double-struck Latin, digits | `0x1D538`, `0x1D7D8` |
| Sans-serif | sans, sans bold, sans italic, sans bold italic | `0x1D5A0` |
| Monospace | monospace Latin, digits | `0x1D670`, `0x1D7F6` |

The base codepoints above are the first of each run; the runs are contiguous and
fixed by the Bridge guarantee (SUTR-6), so a consumer addresses any variant by a
constant offset from the family base.

### 2.3 Operators, relations, arrows, and fences

For layout, the important property is not the visual shape of an operator but
its **class**: a relation is spaced differently from a binary operator, and a
fence stretches while an ordinary symbol does not. SUMNLS therefore classifies
every mathematical character with the `MathClass` property (§4.2) rather than by
codepoint range, so a plugin-supplied native symbol is classified exactly like a
Bridge symbol.

### 2.4 Plain-text and encoded math

A higher-level protocol may encode a formula as a linear string of
math-alphanumeric characters and operators. Because the style lives in the
characters and the structure is carried by SCP directives (§6), such a linear
form is a complete, self-describing input to the layout engine. Markup languages
(TeX, MathML, OpenMath) remain supported but are not required; they compile down
to the same characters and directives.

---

## 3. Mathematical alphanumeric and symbol mapping

### 3.1 The Bridge mappings are permanent

The Bridge (SUTR-6) guarantees a permanent 1:1 mapping between
`0x000000–0x10FFFF` and Unicode. SUMNLS allocates **no** character inside the
Bridge; it consumes the existing mappings unchanged. In particular:

- the Mathematical Alphanumeric Symbols block `0x1D400–0x1D7FF` keeps its exact
  Unicode layout, so a style variant is addressed by its Unicode-assigned
  codepoint;
- the operator and symbol blocks keep theirs;
- no Bridge codepoint is re-purposed, renamed, or re-classified in a way that
  changes its identity.

This is what lets a document move between SuperUnicode and Unicode without a
lossy mapping: the repertoire is shared by construction.

### 3.2 The Native Math ExtSUCS Space

Symbols that the Unicode repertoire cannot carry — additional alphabet styles,
non-Latin math alphabets, assembly parts for extensible constructs, and native
operators — are allocated in the **Native Math ExtSUCS Space**
`0x00170000–0x0017FFFF`. The range is a 16-block carve-out inside the Native
plane, disjoint from the RNUR registry (`0x00160000–0x0016FFFF`) and from the
terminal traps (`0x7FFFFFF0–0x7FFFFFFF`).

| Block | Range | Contents |
|-------|-------|----------|
| A — Alphanumerics | `0x00170000–0x00170FFF` | Native styled Latin and digit variants |
| B — Alphabets | `0x00171000–0x00171FFF` | Native Greek, Cyrillic, Hebrew, Arabic math alphabets |
| C — Operators | `0x00172000–0x00172FFF` | Native n-ary, binary, and unary operators |
| D — Relations | `0x00173000–0x00173FFF` | Native relations and turnstiles |
| E — Arrows | `0x00174000–0x00174FFF` | Native arrow set |
| F — Fences | `0x00175000–0x00175FFF` | Extensible delimiter assembly parts |
| G — Radicals | `0x00176000–0x00176FFF` | Radical signs, vinculum, degree marks |
| H — Large operators | `0x00177000–0x00177FFF` | Sum, product, integral, and limit forms |
| I — Accents | `0x00178000–0x00178FFF` | Over/under accents and wide diacritics |
| J — Spacing | `0x00179000–0x00179FFF` | Invisible operators and explicit math spaces |
| K — Big-op parts | `0x0017A000–0x0017AFFF` | Assembly parts for large operators |
| L — Reserved | `0x0017B000–0x0017EFFF` | Future allocation |
| M — Registration | `0x0017F000–0x0017FFFF` | Plugin/registry-defined extensions |

A native math symbol is allocated only through the plugin lifecycle (SUTR-5);
the base standard fixes the block boundaries and leaves the contents to the
native math plugin. Until the plugin is mounted the range contributes no
characters, mirroring the RNUR policy in SUAS-005.

### 3.3 Mapping rules and invariants

- **No duplication.** A symbol that exists in the Bridge MUST NOT be duplicated
  in the Native Math ExtSUCS Space; a native allocation is justified only when
  the repertoire has no Bridge codepoint.
- **Style is identity.** A style variant is a distinct character, never a font
  selection.
- **Stable blocks.** Block boundaries in §3.2 are fixed; contents within a block
  are assigned by the native math plugin and never renumbered.
- **Range-dispatched classification.** A consumer resolves a native math
  character by range dispatch (SUTR-8 §4.3), not by a table of names.

### 3.4 Style variants as distinct characters

The variant families of §2.2 are closed for the Bridge and open for the Native
space. A native plugin MAY define additional families (for example an upright
italic or a bold double-struck) in Block A, provided each family is a contiguous
run with a fixed base and each character carries a correct `MathClass`. The
layout engine treats a native family exactly like a Bridge family, because it
reads properties, not codepoints.

---

## 4. Math properties in SUCD and SUCPM

### 4.1 The `Math` binary property

`Math` is a boolean property. It is true for a character intended for
mathematical use: the operator, relation, arrow, fence, alphanumeric, and
delimiter sets, and false otherwise. It is the membership predicate a consumer
tests before applying any math layout at all.

`Math` is an **informative** property in the SUCPM sense (SUTR-8 §1.3.1): a
conformant process may use it as the default membership test, and a higher-level
protocol may override it, but the layout behavior that depends on it is
normative once math layout is engaged.

### 4.2 The `MathClass` enumerated property

`MathClass` is an enumerated property that classifies a mathematical character
for spacing and layout. SUMNLS adopts the UTR #25 value set:

| Value | Name | Atom equivalent | Examples |
|-------|------|-----------------|----------|
| `N` | Normal | Ord | letters, digits, plain symbols |
| `B` | Binary | Bin | plus, minus, times, union |
| `U` | Unary / Prefix | Op | sine, log, big-operator head |
| `R` | Relation | Rel | equals, less, subset, maps-to |
| `O` | Opening | Open | left parenthesis, left bracket |
| `C` | Closing | Close | right parenthesis, right bracket |
| `F` | Fence | Fence | vertical bar, double bar, angle |
| `P` | Punctuation | Punct | comma, semicolon (in math) |
| `I` | Inner | Inner | a self-contained inner subformula |
| `S` | Special | Special | prime, factorial, ellipsis |

A character that is not mathematical carries no `MathClass` value; the property
is evaluated only where `Math` is true. `MathClass` is informative, but once a
consumer engages the layout engine of §5, the class determines normative
spacing.

### 4.3 O(1) bitmask packing

`Math` and `MathClass` are packed into the SUCPM masks so classification is a
single bitwise operation.

| Mask | Field | Assigned from |
|------|-------|---------------|
| `sucpm_flags32_t` | `SUCPM_F_MATH` — the `Math` membership flag | The reserved core-mask bits, by the companion SUCPM revision |
| `sucpm_flags64_t` | `MathClass` — a 4-bit enumerated field | The reserved high bits of the extended mask |

SUMNLS does not itself choose the bit positions; it requires that the companion
SUCPM revision assign them from the bits SUTR-8 §5 leaves reserved (bit 23 and
bits 28–31 of the core mask, and the unused high bits of the extended mask), so
that `Math` and `MathClass` stay reachable in `O(1)` from the packed mask and no
existing field moves. Because the extended mask is produced by the same dual
lookup engine as every other property (SUTR-8 §4), classification costs the
consumer one shift-and-compare:

```c
/* MathClass occupies a 4-bit field in the extended mask. */
typedef enum {
    SUCPM_MC_N = 0,  /* Normal / Ord   */
    SUCPM_MC_B = 1,  /* Binary         */
    SUCPM_MC_U = 2,  /* Unary / Prefix */
    SUCPM_MC_R = 3,  /* Relation       */
    SUCPM_MC_O = 4,  /* Opening        */
    SUCPM_MC_C = 5,  /* Closing        */
    SUCPM_MC_F = 6,  /* Fence          */
    SUCPM_MC_P = 7,  /* Punctuation    */
    SUCPM_MC_I = 8,  /* Inner          */
    SUCPM_MC_S = 9   /* Special        */
} sucd_math_class_t;

static inline bool sumnls_is_math(sucpm_flags64_t m) {
    return (m & SUCPM_F_MATH) != 0;              /* membership, O(1) */
}

static inline int sumnls_math_class(sucpm_flags64_t m) {
    return (int)((m >> SUCPM_MATHCLASS_SHIFT) & 0xFu);  /* class, O(1) */
}
```

The exact bit position of `SUCPM_F_MATH` and of the `MathClass` field is fixed
by the companion SUCPM revision; the constraint SUMNLS imposes is that both are
readable in `O(1)` from the packed mask with no allocation and no name parsing.

### 4.4 Vertical metrics and stretch properties

Layout needs more than a class; it needs geometry. SUMNLS derives that geometry
from the SUF font engine rather than from a property table:

- **Glyph bounding box** — ascent, descent, and advance, per glyph.
- **Math constants** — axis height, rule thickness, gap above/below display
  style, superscript and subscript shifts and sizes.
- **Stretch data** — the set of parts and their overlap rules for an extensible
  delimiter or a large operator.

These are **informative** at the character level and **normative** once the
engine of §5 consumes them: a conforming renderer must produce the same box
geometry given the same glyph metrics.

### 4.5 Derived properties

Two derived properties assist consumers that inherit math context:

- `Other_Math` — true for characters that behave mathematically without being
  in the operator set, such as styled Greek identifiers. It is derived from the
  `MathClass` classification and the script property (SUAS-005).
- `Other_Alphabetic` — true for the math-alphanumeric identifiers, so
  case-like processing and script resolution treat them as letters.

Both are simple closures computed by SUCD tooling (SUTR-8 §4.1); neither adds a
special case to the hot path.

---

## 5. The kernel box-model and layout engine

### 5.1 The box model

The layout engine composes a formula from **boxes**. A box has a width, a
height above the baseline (ascent), a depth below the baseline (descent), and an
optional list of child boxes. There are two box kinds:

- a **horizontal box** packs children left to right, aligned on a shared
  baseline, and advances by the running width;
- a **vertical box** stacks children top to bottom around a central axis, used
  for fractions, radicals, and limits.

Boxes are allocated on a fixed, bounded stack, never on the heap. The engine's
recursion depth is bounded by the nesting depth of the formula's explicit
groups (§6), so a hostile input cannot exhaust memory.

### 5.2 Single-pass spatial layout

The engine performs one recursive-descent pass:

1. Read the next code unit.
2. If it is an SCP math directive, apply the structural transition (§6).
3. Otherwise classify the character with `MathClass` (§4.3) and read its glyph
   metrics from SUF.
4. Accumulate the glyph into the current horizontal box, applying inter-atom
   spacing from §5.8.
5. When a vertical group opens, descend into a child box, lay it out, and place
   it relative to the parent axis.

No step allocates, and no step re-scans the input, so the pass is linear in the
number of code units and `O(1)` per character.

### 5.3 Stacked fractions

A stacked fraction is a vertical box with three children: the numerator, a rule,
and the denominator.

```text
            numerator
        ----------------   <- rule, thickness = fraction_rule_thickness
           denominator
```

The engine:

1. lays out the numerator and denominator as horizontal boxes;
2. computes the rule thickness from the math constants and the current style
   (display, text, script, or script-script);
3. shifts the numerator up by the numerator gap and the denominator down by the
   denominator gap;
4. sets the fraction's axis height to the midpoint, so a following operator
   aligns to the bar.

A **skewed** (shilling) fraction, as in a built-up `1/2`, uses the same box with
a diagonal rule and a reduced numerator/denominator shift.

### 5.4 Radicals

A radical is a vertical box whose visible height is the height of its radicand.

```text
          _____
        \/  x + 1
```

The engine:

1. lays out the radicand;
2. selects a radical sign glyph whose height covers the radicand plus the
   clearance above it, stretching by assembly (§5.5) when one glyph is not tall
   enough;
3. draws the vinculum across the radicand width;
4. places an optional degree at the upper-left, raised by the degree shift.

### 5.5 Extensible delimiters

A parenthesis, bracket, brace, vertical bar, or arrow can be built from parts.
The engine selects between three strategies:

| Strategy | When | Construction |
|----------|------|--------------|
| Single glyph | A glyph of the needed size exists | One glyph, scaled by line metrics |
| Assembled | Fixed end and extender parts exist | Top, repeated extender, bottom |
| Drawn | No parts exist | A rule-based stroke from the glyph outline |

Assembly reads the stretch data of §4.4: the available parts, their vertical
coverage, and the allowed overlap. The engine repeats the extender part until
the target height is met, then places the end parts. The same mechanism sizes a
large operator's head (§5.7) and a radical sign (§5.4).

### 5.6 Script tiers

Subscripts and superscripts form **script tiers**. The engine maintains a
current math style with four levels, following the OpenType MATH model:

| Style | Role | Typical size |
|-------|------|--------------|
| Display | Whole-line formula | 1.0 |
| Text | Inline formula | 1.0 |
| Script | First-level sub/superscript | superscript/subscript size |
| Script-script | Second-level sub/superscript | script-script size |

A superscript's box is raised by the superscript shift and reduced to the next
smaller size; a subscript's box is dropped by the subscript shift. A sub and a
super on the same base are packed side by side with a minimum gap, and the base
is followed by a prescript or postscript kernel only where the formula supplies
one. Beyond script-script, further nesting stays at script-script size, matching
established practice.

### 5.7 Big operators and limits

A big operator (n-ary summation, product, integral, union) is a character whose
glyph is extended to the surrounding formula's size. Its limits are placed either
as sub/superscripts or, in display style, stacked above and below the operator.
The placement is chosen by an SCP directive (§6.4) or by the operator's own
`MathClass` and the current style.

### 5.8 Spacing and atom classes

Between two adjacent atoms the engine inserts a space determined by their
`MathClass` pairs, following the conventional no-space / thin-space / medium-space
/ thick-space table:

| Left \ Right | Ord | Bin | Rel | Open | Close | Punct |
|--------------|-----|-----|-----|------|-------|-------|
| **Ord** | none | medium | thick | none | none | none |
| **Bin** | medium | none | none | none | none | none |
| **Rel** | thick | medium | none | none | none | none |
| **Open** | none | none | none | none | none | none |
| **Close** | none | medium | thick | none | none | none |
| **Punct** | thin | none | none | none | none | none |

A binary operator that begins or ends a formula, or that follows an opening
fence, is reclassified as ordinary (unary) before spacing is applied. This rule
is normative: it is what makes `-x` and `a + -b` lay out as expected.

### 5.9 Integration with the SUF vector font engine

SUF supplies the geometry the box model consumes:

| SUF provides | The engine uses it for |
|--------------|------------------------|
| Glyph bounding box | Box ascent, descent, advance |
| Math constants | Axis height, rule thickness, shifts, sizes |
| Stretch data | Extensible delimiters, radicals, big operators |
| Outline access | The drawn-delimiter strategy (§5.5) |

Because the metrics live in the font and the algorithm lives in the kernel,
SUMNLS needs no external MATH table and no markup-aware shaping stage. A glyph
with no outline (an invisible operator) contributes advance only.

### 5.10 Worked example

The linear form `(a+b)/2` with an SCP fraction group lays out as:

```text
        a + b
       -------
          2
```

The engine lays out the numerator horizontal box `a + b`, applies binary
spacing around `+`, lays out the denominator box `2`, computes the rule from the
axis height and rule thickness, shifts both by their gaps, and returns a vertical
box whose baseline is the fraction axis. A following relation aligns to that
axis, so the whole expression shares one baseline.

---

## 6. System Control Plane math directives

### 6.1 The `0x00110030` block

Mathematical structure that characters cannot express is carried in-band by a
compact block of SCP markers, `0x00110030–0x0011003F`. The block is disjoint
from the SBR break block (`0x00110020–0x00110022`) and the SDF directional block
(`0x00110100`), and it is decoded in `O(1)` by the SCP tier of the property
engine (SUTR-8 §4.2).

| ID | Name | Meaning |
|----|------|---------|
| `0x00110030` | `SCP_MATH_ISOLATE_START` | Begin an inline math isolation scope |
| `0x00110031` | `SCP_MATH_ISOLATE_END` | End the innermost math isolation scope |
| `0x00110032` | `SCP_MATH_MODE_LTR` | Switch math direction to left-to-right |
| `0x00110033` | `SCP_MATH_MODE_RTL` | Switch math direction to right-to-left |
| `0x00110034` | `SCP_MATH_ALIGN_TAB` | Advance to the next alignment tab stop |
| `0x00110035` | `SCP_MATH_ALIGN_ROW` | End an alignment row and start the next |
| `0x00110036` | `SCP_MATH_STYLE_PUSH` | Push the math style (display/text/script) |
| `0x00110037` | `SCP_MATH_STYLE_POP` | Pop the math style |
| `0x00110038` | `SCP_MATH_FRACTION_START` | Open a stacked fraction group |
| `0x00110039` | `SCP_MATH_FRACTION_MID` | Separate numerator and denominator |
| `0x0011003A` | `SCP_MATH_FRACTION_END` | Close the stacked fraction group |
| `0x0011003B` | `SCP_MATH_RADICAL_START` | Open a radical group |
| `0x0011003C` | `SCP_MATH_RADICAL_END` | Close the radical group |
| `0x0011003D` | `SCP_MATH_LIMITS_OVER` | Place following limits above |
| `0x0011003E` | `SCP_MATH_LIMITS_UNDER` | Place following limits below |
| `0x0011003F` | `SCP_MATH_GROUP_END` | Close the innermost structural group |

The markers are ordinary code points: they flow through SUTF (SUTR-1), SUST
(SUTR-4), and SUCF canonical ordering (SUAS-004) like any other, so a formula is
a plain code unit sequence.

### 6.2 Inline math isolation

`SCP_MATH_ISOLATE_START` and `SCP_MATH_ISOLATE_END` bracket an inline formula
inside surrounding text. Isolation does three things:

- it suspends the outer line-breaking context, so the formula is kept together
  unless it cannot fit (SUAS-003);
- it establishes a math baseline independent of the surrounding text baseline;
- it scopes the style stack, script tiers, and any directional switches, so an
  inner formula cannot leak state into the text around it.

Isolation is nestable; the end marker closes the innermost scope.

### 6.3 Directional math-mode switches

`SCP_MATH_MODE_LTR` and `SCP_MATH_MODE_RTL` select the horizontal progression
order of a formula, so that an Arabic or Hebrew mathematical expression reads in
its natural direction while the Latin identifiers inside it keep their own
direction. The switch interacts with SUAS-001 (SDF): a formula is an isolate in
the surrounding BiDi run, and the math-mode switch sets the direction of that
isolate. Nested switches form nested isolates.

### 6.4 Alignment tab stops

`SCP_MATH_ALIGN_TAB` advances to the next tab stop, and `SCP_MATH_ALIGN_ROW`
starts a new row of an aligned display (for example a multi-line derivation).
Alignment is implemented as a vertical box whose rows share a set of tab-column
widths; the engine measures each row, takes the maximum width per column, and
re-lays the rows on the shared columns. Because the measure is over a bounded
row count, the pass stays linear.

### 6.5 Interaction with the other standards

| Standard | Interaction |
|----------|-------------|
| SUAS-001 (SDF) | A math isolate is a BiDi isolate; directional switches scope within it |
| SUAS-002 (SGW) | Math glyph advances and the monospace grid share one metric source |
| SUAS-003 (SBR) | Isolation suppresses break opportunities inside a formula |
| SUAS-004 (SUCF) | Math alphanumerics participate in canonical equivalence unchanged |
| SUAS-005 (SSP) | A math alphabet resolves to its script; structure does not change script |
| SUTR-8 (SUCPM) | `Math` and `MathClass` are read from the packed masks |

### 6.6 Conformance of the directive block

A conformant process that renders mathematics MUST:

- recognize the `0x00110030–0x0011003F` block and apply the transition named in
  §6.1;
- treat an unclosed group as closing at the end of input, without error;
- keep the directive stream and the character stream in one code unit sequence,
  so no side channel is required.

A process that does not render mathematics MAY treat the block as ordinary SCP
controls, which are invisible by default (SUAS-003).

---

## 7. Data files

SUMNLS adds the following machine-readable data to the SuperUnicode Character
Database (SUCD), under `Public/0.1.0/sucd/`:

| File | Contents |
|------|----------|
| `Props/MathProps.txt` | The `Math` binary property over the Bridge and native math ranges |
| `Props/MathClass.txt` | The `MathClass` value per mathematical character, as `N`, `B`, `U`, `R`, `O`, `C`, `F`, `P`, `I`, or `S` |
| `Derived/MathMetrics.txt` | Reference math constants and per-family stretch metadata (informative) |
| `Math/MathBlocks.txt` | The Native Math ExtSUCS block boundaries of §3.2 |

The file formats follow the SUCD conventions used elsewhere (semicolon-separated
ranges, `#` comments). `Props/MathProps.txt` and `Props/MathClass.txt` are the
authoritative inputs to the packed masks of §4.3; a bridge between them and the
masks is produced by SUCD tooling, exactly as for the base properties.

---

## 8. Implementation notes

- **Freestanding C99.** The engine uses a fixed-depth box stack and no heap; the
  depth limit is a compile-time constant.
- **One pass.** The input is scanned once; a group that opens is laid out on
  descent and resolved on ascent, so the cost is linear in code units.
- **Properties, not tables.** Classification reads the packed SUCPM mask; the
  engine never parses a character name or a markup tag.
- **Metrics from SUF.** Bounding boxes and constants come from the font, so the
  same engine renders any font that supplies the required metrics.
- **Bounded stretch.** Delimiter assembly is bounded by the available parts; a
  request beyond the font's maximum clamps to the largest construct, never
  loops.
- **Invisible operators.** A spacing-only character contributes an advance and
  no ink, so explicit spacing needs no special node type.
- **Deterministic.** Given the same code units and the same font metrics, the
  engine produces byte-identical box geometry, which makes rendering testable.
- **The script tier is a token, not a codepoint.** The §6.1 directive block
  `0x00110030–0x0011003F` is a closed enumeration (§10) and names no
  script-tier directive. Rather than invent a codepoint, the engine takes the
  script tier from its own token stream: a token is a codepoint plus an
  optional control marker (`SUMNLS_CTRL_SUPERSCRIPT`,
  `SUMNLS_CTRL_SUBSCRIPT`). A front end that has already lowered a formula
  emits the markers; a marker is not a character and adds nothing to the
  repertoire. `sutr_sumnls_layout()` is the codepoint-only entry point and
  `sutr_sumnls_layout_tokens()` is the one that accepts markers.
- **The axis height is a constant, not a property.** §5.3 centres the fraction
  rule on the math axis. The axis height is one of the font-supplied constants
  of §4.4, so the numerator is raised by `axis + rule_thickness / 2 +
  numerator_gap` and the denominator dropped by `axis - rule_thickness / 2 -
  denominator_gap`. The engine never derives the axis from a glyph.
- **The radical sign and degree are positional.** The radical group convention
  is: the first atom of the group is the sign, an optional second atom
  `U+00B0` is the degree, and the remainder is the radicand. The degree box is
  created lazily, when the degree is actually seen, so a radical without a
  degree has no empty box between the sign and the radicand. The radicand is
  wrapped in a vertical box that drops it by the rule thickness, and the sign's
  ascent is overridden so that it covers the radicand plus the radical
  clearance.

---

## 9. Recommendations

1. **Encode style in characters, not in fonts.** Use the mathematical
   alphanumeric characters so a formula is portable across fonts and formats.
2. **Carry structure in-band.** Use the SCP math directives rather than an
   out-of-band structure tree, so a formula is a self-describing code unit
   sequence.
3. **Prefer Bridge codepoints.** Allocate a Native Math ExtSUCS codepoint only
   when the repertoire has no Bridge codepoint for the symbol.
4. **Classify by property.** Never classify an operator by codepoint range; read
   `MathClass`, so native and future symbols behave identically.
5. **Keep metrics in the font.** Let SUF supply geometry; do not hard-code
   metrics in the engine.
6. **Isolate math from text.** Always bracket an inline formula with the
   isolation markers so it cannot disturb the surrounding line.
7. **Clamp, never fail.** When a delimiter or radical cannot be stretched to the
   requested size, clamp to the largest available construct.

---

## 10. Stability and versioning

SUMNLS inherits the SUCPM stability guarantees (SUTR-8 §7.2) and adds the
following:

- **Block boundaries are fixed.** The Native Math ExtSUCS block layout of §3.2
  will not be renumbered; new blocks are added only in the reserved range.
- **Directive IDs are fixed.** The `0x00110030–0x0011003F` assignment of §6.1 is
  a closed enumeration; a new structural directive requires a new block.
- **Property values may grow.** The `MathClass` set may gain values in a future
  version; an existing value never changes meaning.
- **Geometry is stable per font.** A font's math constants and stretch data are
  stable within a font version; a font update may change them and the engine
  follows.
- **Bridge is permanent.** No Bridge math character is ever re-mapped or
  re-classified in a way that changes its identity (SUTR-6).

---

## 11. Compliance matrix

| Requirement | Mandatory | Verification |
|-------------|-----------|--------------|
| The Bridge math repertoire is preserved 1:1 | Yes | §3.1; SUTR-6 |
| Math alphanumerics are distinct characters, not font styles | Yes | §2.2, §3.4 |
| The Native Math ExtSUCS Space is `0x00170000–0x0017FFFF` | Yes | §3.2 |
| Native math allocation is plugin-gated | Yes | §3.2; SUTR-5 |
| No duplication of a Bridge symbol in the native space | Yes | §3.3 |
| The `Math` binary property is defined over the repertoire | Yes | §4.1 |
| `MathClass` classifies operators, relations, and fences | Yes | §4.2 |
| `Math` and `MathClass` are readable in `O(1)` from the packed mask | Yes | §4.3 |
| Layout is zero-allocation and bounded | Yes | §5.1, §5.2, §8 |
| Fractions, radicals, delimiters, scripts are defined | Yes | §5.3–§5.6 |
| Big operators and limits are defined | Yes | §5.7 |
| Inter-atom spacing is normative | Yes | §5.8 |
| Metrics are sourced from the SUF font engine | Yes | §5.9 |
| The math directive block is `0x00110030–0x0011003F` | Yes | §6.1 |
| Isolation, direction, and alignment directives are defined | Yes | §6.2–§6.4 |
| An unclosed group closes at end of input | Yes | §6.6 |
| Data files are published in SUCD | Yes | §7 |
| This report defines no markup or font format | Yes | §1.2 |

---

## 12. Reference implementation

SUMNLS is implemented by `include/sutr/sutr_sumnls.h` and
`src/sutr/sutr_sumnls.c` as a freestanding C99 static library (`sutr_static`),
with the host unit tests in `tests/test_sutr_sumnls.c`. It registers with the
SuperUnicode Specification Registry & Cross-Reference Engine (SSRE, SUAS-010) as
specification `SUTR-9` with reference unit `sutr_sumnls`, kind `REFERENCE`,
version `0.1.0`.

### 12.1 The packed classification

SUMNLS owns no character table. `sutr_sumnls_is_math()` and
`sutr_sumnls_math_class()` read the packed SUCPM mask of §4.3: one
shift-and-compare for the `Math` flag, one more for the four-bit `MathClass`
field. A native math plugin and the Bridge repertoire therefore behave
identically, and the two modules agree by construction because the `Math` flag
and the `MathClass` field are the bits SUTR-8 §5.1 reserves for them.

### 12.2 The atom classes and the spacing table

The ten UTR #25 `MathClass` values project onto the six atom classes of §5.8:
`N → Ord`, `B → Bin`, `R → Rel`, `O → Open`, `C → Close`, `P → Punct`, and
`U`, `F`, `I`, `S` all project onto `Ord`. The spacing table is a single
constant matrix with a leading "no previous atom" column, so the first atom
of a run is spaced by the same lookup as every other.

`sutr_sumnls_reclassify()` is the normative §5.8 rule as a predicate: a binary
operator is reclassified as ordinary when it opens a run, closes a run,
follows an opening fence, or follows another binary operator.

### 12.3 The single-pass deferral

The reclassification needs to know whether an atom closes its run, which is a
one-atom look-ahead. The engine therefore holds exactly one pending atom and
commits it when the next atom arrives or at the end of the input. The pass
stays single and linear, with no re-scan and no heap. The space a committed
atom required is carried on the child box as `gap_before`.

### 12.4 The box model

Boxes live in a caller-owned flat array; the engine never allocates. The array
is a storage pool rather than a tree, and the tree is recovered by walking
`first_child` and `next_sibling`, so a renderer traverses it in O(1) per box.
A horizontal box's width is the sum of its children's widths and each child's
`gap_before`; a vertical box's width is the maximum of its children's widths.
Both take their ascent and descent from the children's baseline shifts. A
glyph is a childless horizontal box.

### 12.5 The groups and their geometry

The group kinds are horizontal, fraction, radical, script superscript, script
subscript, limits over, limits under, and isolate. `SCP_MATH_GROUP_END` closes
the innermost structural group, and a group left open at the end of the input
is closed there (§6.6).

A fraction is a vertical box holding the numerator, the rule, and the
denominator, with the rule centred on the constant axis height. A radical is
a vertical box holding the radicand, with the sign as its sibling, under the
positional convention recorded in §8. The limits directives consume the
parent's pending atom as the operator and build a vertical box of the limits
and the operator.

### 12.6 The alignment frame

The alignment frame of §6.4 turns the frame's run box into the vertical
container; the rows are horizontal boxes and the cells are horizontal boxes.
Column widths are measured and the cells are padded when the frame closes, and
the rows are re-finalized after that padding, so a row's width is consistent
with the column it sits in. The bounds are compile-time constants: eight
columns, sixteen rows, sixty-four cells.

### 12.7 Verification

The unit test suite covers the version and metadata, the directive decode, the
classification and the projection, the spacing matrix and the reclassification
rule, the deferral of the pending atom, the box geometry, the fraction, the
radical, the script tiers and the style ladder, the limits, the alignment
frame and its padding, the stretch selection, the taxonomy names, and the SSRE
registration.

---

## 13. Terms & conventions

- **SUMNLS** — SuperUnicode Mathematical Notation and Layout Support; this
  report (SUTR-9).
- **Math repertoire** — the set of characters with the `Math` property true.
- **MathClass** — the enumerated classification of a mathematical character as
  `N`, `B`, `U`, `R`, `O`, `C`, `F`, `P`, `I`, or `S`.
- **Math atom** — a character, with its `MathClass`, considered as a unit of
  layout.
- **Box** — a width, an ascent, a depth, and optional children; the unit of the
  layout engine.
- **Horizontal box** — children packed left to right on a shared baseline.
- **Vertical box** — children stacked around a central axis; used for fractions,
  radicals, and limits.
- **Script tier** — a sub/superscript level at display, text, script, or
  script-script size.
- **Extensible delimiter** — a fence grown from parts (single, assembled, or
  drawn) to a target height.
- **Big operator** — an n-ary operator (sum, product, integral) whose glyph
  grows and whose limits stack.
- **Native Math ExtSUCS Space** — the carve-out `0x00170000–0x0017FFFF` for
  native math characters.
- **Math directive** — an SCP marker in `0x00110030–0x0011003F`.
- **Isolation** — an inline math scope that suppresses outer breaking and scopes
  direction and style.
- **SUF** — the SuperUnicode vector font engine; the source of math metrics.
- **UTR #25** — Unicode Technical Report #25, *Unicode Support for Mathematics*,
  the report this one mirrors.
- **SDF / SBR / SGW / SSP / SUCF** — SUAS-001, SUAS-003, SUAS-002, SUAS-005, and
  SUAS-004.

---

**END OF SUTR-9**
