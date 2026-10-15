# SUAS-005 — System Script Property (SSP)

**Standard ID:** SUAS-005
**Status:** Draft — Ratified
**Scope:** Core Kernel Invariant
**Applies To:** SUCD, SUAS-001 (SDF), SUAS-003 (SBR), SUAS-004 (SUCF), SUTS-001 (SUCA), SUTS-002 (SRE), the `suf` font subsystem, the System Control Plane
**Last Updated:** 2026-10-14

---

## 1. Overview

System Script Property (SSP) is the **core architecture governing writing-system
identification** in SuperUnicode. It defines the **Script property** and the
**Script_Extensions property** over the full 64-bit ExtSUCS codepoint space
(Base SUCS + SCP + Native SUCS + RNUR registry + ExtSUCS plugins), and binds
them to a single **range-dispatched, zero-allocation** resolution engine.

SSP is the structural successor to the Unicode Script property (UAX #24,
`Scripts.txt` / `ScriptExtensions.txt`). Where UAX #24 maps characters to
writing systems through flat property tables, SSP provides a **unified
script-tagging mechanism**: a compact 16-bit `suas_script_t` id is resolved for
every codepoint by one engine spanning four districts, and that id is small
enough to pack directly into a character token header. A freestanding font
engine (`suf`) can therefore jump **straight to the correct registered typeface
block** with no multi-font fallback search.

SSP is the *value* half of the script metadata model. Its sibling **SUTR-8
(SUCPM)** is the *model* half: SUCPM defines how enumerated property values —
including the **`Script_Identifier`** field — are packed into the 64-bit
extended property mask and tested in `O(1)`. SSP defines which value goes into
that field, and reconciles the 16-bit SSP namespace with SUCPM's 10-bit field
(§5.6).

### 1.1 Goals

- **Unified resolution.** One engine resolves the Script property for any SUCS
  or ExtSUCS codepoint: `O(1)` range dispatch between districts, `O(log R)`
  binary search inside the Bridge and registry tables.
- **Compact tag.** A 16-bit script id, packable into the low half of a 64-bit
  character token, so the `suf` font engine selects a typeface block by direct
  index rather than a fallback search.
- **Explicit script context.** A script context is declared in-band via
  dedicated SCP directive codepoints — never inferred heuristically, never by
  mutating user data.
- **Zero allocation.** All tables are immutable static data; every lookup is a
  bounded binary search or an `O(1)` range test. Nothing is allocated.
- **Unicode Bridge compatibility.** For `0x000000`–`0x0010FFFF`, SSP
  reproduces the Unicode Script / Script_Extensions assignments exactly, via
  curated tables derived from SUCD.
- **Merged implicit value.** Unicode's `Common` and `Inherited` scripts share a
  single merged implicit value (`COMMON`); inheritance from a base character is
  applied by `suas_ssp_resolve()`, not by a distinct stored value.

### 1.2 Non-Negotiable Invariants

1. **No heap.** The SSP engine performs no dynamic allocation; all tables are
   immutable static data.
2. **Total partition.** Every codepoint resolves to exactly one
   `suas_script_t` id, and its Script_Extensions set is never empty.
3. **Bounded time.** District dispatch is `O(1)`; in-district lookup is a
   bounded binary search, `O(log R)` in the number of curated ranges.
4. **Bridge compatibility.** `0x000000`–`0x0010FFFF` agrees with the SUCD
   `Scripts.txt` / `ScriptExtensions.txt` assignments (curated subset).
5. **Explicit-only context.** A script context changes only through an SCP
   script-context directive in `0x00112000`–`0x00119FFF`; no other codepoint
   silently changes the surrounding script.
6. **Stable ids.** A `suas_script_t` value, once assigned, is never renumbered
   or reused (§9).
7. **Closed extensions.** The Script_Extensions set always contains at least
   the Script value of the codepoint; the default is the singleton
   `{ Script(cp) }`.

---

## 2. Scope

SSP defines the **identity and assignment** of the Script and Script_Extensions
properties and the engine that resolves them. Specifically it defines:

- The **`suas_script_t` value namespace** and its two implicit values.
- **Zone policy** across the Unicode Bridge, the System Control Plane, the
  RNUR registry carve-out, the Native plane, ExtSUCS, and the terminal traps.
- The **Script_Extensions** set and its membership test.
- **SCP script-context directives** and their disjoint sub-range.
- The **`suf` font-engine token** packing of a script id with a codepoint.
- The **reconciliation** with the SUCPM `Script_Identifier` field (SUTR-8 §5.2).

SSP does **not** define the SUCD data files themselves (SUCD is the source of
truth, §6), nor glyph shaping, bidirectional layout (SUAS-001 SDF), line
breaking (SUAS-003 SBR), canonical equivalence (SUAS-004 SUCF), or collation
(SUTS-001 SUCA). SSP is consumed by the `suf` font subsystem, the regular-
expression engine (`\p{Script}` tests, SUTS-002 SRE), and identifier and
fallback analysis layers.

---

## 3. The Script Property

The Script property assigns exactly one `suas_script_t` value to every
codepoint. The property name is **`Script`**; the value domain is enumerated.

### 3.1 Property values

The value namespace is a full partition of the codespace. Each value carries a
stable 16-bit id:

| Value | Id | Kind |
|-------|----|------|
| `UNKNOWN` | `0x0000` | Implicit — no script |
| `COMMON` | `0x0001` | Implicit — merged Common + Inherited |
| `LATIN` | `0x0002` | Bridge script |
| `ARABIC` | `0x0003` | Bridge script |
| `CYRILLIC` | `0x0004` | Bridge script |
| `GREEK` | `0x0005` | Bridge script |
| `HAN` | `0x0006` | Bridge script |
| `HEBREW` | `0x0007` | Bridge script |
| `DEVANAGARI` | `0x0008` | Bridge script |
| `HIRAGANA` | `0x0009` | Bridge script |
| `KATAKANA` | `0x000A` | Bridge script |
| `HANGUL` | `0x000B` | Bridge script |
| `THAI` | `0x000C` | Bridge script |
| `LAO` | `0x000D` | Bridge script |
| `TIBETAN` | `0x000E` | Bridge script |
| `ARMENIAN` | `0x000F` | Bridge script |
| `GEORGIAN` | `0x0010` | Bridge script |
| `BRAILLE` | `0x0011` | Bridge script |
| `SYRIAC` | `0x0012` | Bridge script |
| `COPTIC` | `0x0013` | Bridge script |
| `CUSTOM_MAX` | `0xFFFF` | Upper bound of the 16-bit space |

The **Bridge range** is `0x0000`–`SUAS_SCRIPT_BRIDGE_MAX` (`0x03FF`); the
**Native range** begins at `SUAS_SCRIPT_NATIVE_MIN` (`0x1000`). The gap between
them is reserved for future Bridge scripts so that the packed form (§5.6)
remains a contiguous window. Native / registry script values are assigned from
`0x1000` upward, but **none are registered yet**: a mounted registry (the RNUR
plugin, §5.4) defines them when it mounts.

### 3.2 Implicit values

Two values are **implicit** — that is, they carry no writing-system identity on
their own and are subject to inheritance:

- **`UNKNOWN` (`0x0000`)** — unassigned codepoints, Private Use Area
  codepoints, noncharacters, the unregistered remainder of the Native plane,
  ExtSUCS plugin codepoints that register no script, and the Trap /
  Sentinel range.
- **`COMMON` (`0x0001`)** — the merged Unicode `Common` and `Inherited`
  scripts: shared punctuation, decimal digits, mathematical and currency
  symbols, emoji, variation selectors, and the combining marks that attach to
  an arbitrary base.

`suas_ssp_is_implicit()` reports whether a value is one of these two. An
implicit value is replaced by the surrounding explicit context when one exists
(§5.2); otherwise it stands as the property value.

### 3.3 Relation to ISO 15924

For Bridge scripts, the SSP **short name** is the four-letter **ISO 15924**
script code (`Latn`, `Arab`, `Cyrl`, `Grek`, `Hani`, `Hebr`, `Deva`, `Hira`,
`Kana`, `Hang`, `Thai`, `Laoo`, `Tibt`, `Armn`, `Geor`, `Brai`, `Syrc`, `Copt`).
The special value `UNKNOWN` carries the ISO 15924 code `Zzzz`, and the merged
`COMMON` value carries `Zyyy`.

Native / registry scripts (assigned at `0x1000` and above) have no ISO 15924
assignment of their own. A mounted registry supplies their names and codes,
which occupy the ISO 15924 **private-use** space and are meaningful only while
that plugin is mounted (§5.4). None are registered yet.

### 3.4 Script codes and designators

Each value has two canonical textual forms:

- the **long name** (the designator used in data files and diagnostics), e.g.
  `Latin`, `Han`, `Coptic`;
- the **short name** (the ISO 15924-style code), e.g. `Latn`, `Hani`, `Copt`.

`suas_ssp_script_long_name()` and `suas_ssp_script_short_name()` return these
forms for any value; both never return `NULL`, falling back to the `UNKNOWN`
forms for an out-of-range id.

### 3.5 Names and aliases

`suas_ssp_script_from_tag()` resolves a loose script tag against both the long
and short names. Matching is **loose** in the UAX #44 §5.9 sense: ASCII
case-insensitive, with SPACE, HYPHEN-MINUS and UNDERSCORE ignored. Thus
`Latn`, `latin`, and `  LATN ` all resolve to `LATIN`.

Property-value names are **not** unique across the whole SUCD property model,
but the combination of property name and value aliases is. SSP value aliases
are never used as bare identifiers outside a `Script` context.

### 3.6 Anomalies

- **Common and Inherited are merged.** Unicode keeps `Common` and `Inherited`
  distinct. SSP merges them into one implicit value so a font-fallback key is a
  single tag; inheritance is reconstructed at resolution time (§5.2) rather
  than being stored. This is a deliberate, documented deviation.
- **Noncharacters are `UNKNOWN`.** The noncharacters `0xFDD0`–`0xFDEF` and the
  `xFFFE`/`xFFFF` pairs are `UNKNOWN` (as in Unicode). Because the Arabic
  presentation-form block surrounds a noncharacter gap, Arabic is curated as
  `0xFB50`–`0xFDCF` **and** `0xFDF0`–`0xFDFF`, explicitly excluding
  `0xFDD0`–`0xFDEF`.
- **Private Use is `UNKNOWN`, not `COMMON`.** PUA codepoints (`0xE000`–`0xF8FF`,
  supplementary PUA, and the SSP Native plane outside a registry carve-out)
  map to `UNKNOWN`, matching Unicode rather than `Common`.
- **Combining marks are `COMMON`.** `0x0300`–`0x036F` (Combining Diacritical
  Marks) resolve to the merged implicit value and inherit their base's script
  through `suas_ssp_resolve()`.
- **Cyrillic is contiguous.** `0x0400`–`0x052F` is curated as one Cyrillic
  range (basic block plus Cyrillic Supplement), so a Cyrillic codepoint is
  never split across a table boundary.
- **Curated, not exhaustive.** The reference tables are a representative
  subset of SUCD (§6). An unlisted Bridge codepoint falls through to the Common
  table and then to `UNKNOWN`; SUCD remains the source of truth.

### 3.7 Initial assignment

The SSP value ids are frozen at the version of this standard (v1.0.0, §9).
Bridge ids are assigned in the order shown in §3.1; native ids begin at
`0x1000`. The initial curated assignment is a subset of SUCD sufficient to
exercise every consuming face; new assignments extend the tables without
renumbering any existing value.

---

## 4. The Script_Extensions Property

The **Script_Extensions** (`scx`) property generalizes Script: instead of a
single script, a codepoint may expose a small set of scripts in which it is
commonly used. The property name is **`Script_Extensions`**; the value domain is
a set of `suas_script_t` values.

An `scx` set is **small, unordered, and deduplicated**:

- `SUAS_SSP_SCX_MAX` is **4** — the largest curated extension set.
- The empty set is never legal. A codepoint with no curated extension maps to
  the **singleton** `{ Script(cp) }`.
- `suas_ssp_get_script_extensions()` resolves the set;
  `suas_ssp_scx_contains()` tests membership.
- `suas_ssp_matches_script(cp, script)` is true when the codepoint's Script
  **equals** the target **or** its Script_Extensions set **contains** it — the
  single correct predicate for shaped-text script matching.

Representative curated sets:

| Codepoint | Script | Script_Extensions |
|-----------|--------|-------------------|
| `U+30FC` KATAKANA-HIRAGANA PROLONGED SOUND MARK | `COMMON` | `{ HIRAGANA, KATAKANA }` |
| `U+3099` / `U+309A` combining kana voiced marks | `COMMON` | `{ HIRAGANA, KATAKANA }` |
| `U+0640` ARABIC TATWEEL | `COMMON` | `{ ARABIC, SYRIAC }` |
| `U+060C` ARABIC COMMA | `COMMON` | `{ ARABIC, SYRIAC }` |
| `U+3001` / `U+3002` CJK ideographic punctuation | `COMMON` | `{ HAN, HIRAGANA, KATAKANA }` |

Because these codepoints have Script `COMMON`, a plain equality test against
`HIRAGANA` would fail; the `scx` membership test is what makes them match. This
is exactly the role of `Script_Extensions` in UAX #24, preserved verbatim.

---

## 5. The Three-Tier Resolution Engine

Resolution is a single forward pass over three tiers: **district dispatch**,
**implicit-context inheritance**, and **explicit directives**. The reference
entry point is `suas_ssp_get_script()`.

### 5.1 Zone dispatch

The 64-bit codespace is partitioned into districts, each with its own
resolution strategy:

| District | Range | Policy |
|----------|-------|--------|
| **Unicode Bridge** | `0x000000`–`0x0010FFFF` | Explicit script range table first, then the merged Common table; otherwise `UNKNOWN`. |
| **System Control Plane** | `0x00110000`–`0x0011FFFF` | A script-context directive decodes to its selected script (`O(1)`); any other control codepoint is `UNKNOWN`. |
| **RNUR carve-out** | `0x00160000`–`0x0016FFFF` | Reserved for the RNUR registry; resolves to `UNKNOWN` until the registry mounts and registers scripts. |
| **Native plane** | `0x00120000`–`0x7FFFFFEF` | `UNKNOWN` unless a mounted plugin/registry registers a script. |
| **ExtSUCS plugin** | `> 0x7FFFFFFF` | `UNKNOWN` unless a plugin registers a script. |
| **Trap / Sentinel** | `0x7FFFFFF0`–`0x7FFFFFFF` | `UNKNOWN`; never text. |

Inside the Bridge, the explicit table is consulted before the Common table, so
an explicit range always wins an overlap. The Common table is a curated set of
shared/symbol/mark ranges; a codepoint in neither table is `UNKNOWN`.

### 5.2 Common and Inherited handling

The merged implicit value is resolved to a concrete script by inheritance.
`suas_ssp_resolve(cp, context_script)` applies the rule:

> If `Script(cp)` is implicit (`UNKNOWN` or `COMMON`) **and** the context
> script is explicit, the result is the context script; otherwise the result
> is `Script(cp)`.

The context script is normally the resolved script of the preceding base
character in the run. This reproduces Unicode's `Inherited` behavior (a
combining mark takes its base's script) and extends it to shared punctuation
without adding a second implicit value. With no explicit context
(`COMMON` or `UNKNOWN`), the implicit value stands unchanged.

### 5.3 SCP script-context directives

The System Control Plane can carry an **explicit script context** as a
directive codepoint:

```
codepoint = SUAS_SSP_SCP_SCRIPT_BASE + (script_id & 0x7FFF)
          = 0x00112000 + (script_id & 0x7FFF)
```

The directive sub-range is **`0x00112000`–`0x00119FFF`**, reserved exclusively
for script context. It is deliberately disjoint from:

- the **BANcodes** (`0x0011A000` and above),
- the **SDF** directional directives (`0x00110101`, `0x00110102`,
  `0x00110104`, `0x00110108`), and
- the **SBR** break markers (`0x00110020`–`0x00110022`).

`suas_ssp_is_script_directive()` reports whether a codepoint is a directive and
writes the selected script id to an out-parameter when one is supplied. The id
is recovered with `cp − SUAS_SSP_SCP_SCRIPT_BASE` (not a bitwise OR), masking
the low 15 bits. A directive resolves to its selected script directly, so it
can force a context for the run that follows without emitting base text.

### 5.4 RNUR and RCCR

The **RNUR** (Reddit Neographical Unicode Registry) is a codepoint registry for
neographies, original scripts, and conscripts. It is a **PUA coordinate
system**: allocations are `(Set_Number, Code_Point)` coordinates in the Unicode
Private Use Areas, because the registry is chiefly consumed by Unicode text.
SuperUnicode does not mirror those PUA coordinates; it **decodes** the registry
into the Native-plane carve-out **`0x00160000`–`0x0016FFFF`**.

Within the carve-out, the high byte of the coordinate offset is the **set
number**: `set = (cp − 0x00160000) >> 8`, an `O(1)` operation.

No registry script is mounted yet, so every coordinate currently resolves to
`UNKNOWN`. SSP defines the carve-out and the coordinate decode; the RNUR plugin
(via the SUTR-5 mount lifecycle) supplies the set → script mapping and the
script names when it is mounted. Until then, `suas_ssp_get_script()` returns
`UNKNOWN` for the entire range.

The **RCCR** (Reddit Conlang Code Registry) is deliberately **not** part of
this model. It is a registry of *codes* — BCP 47 language tags — not of
codepoints, and therefore has no set layer and contributes **no** Script
property. A process evaluates no character property from the RCCR.

### 5.5 `suf` font-engine token integration

The 16-bit script id is packed into a 64-bit character token together with the
31-bit codepoint:

| Bits | Field |
|------|-------|
| `0–15` | 16-bit `suas_script_t` id |
| `16–46` | 31-bit SUCS codepoint |
| `47–63` | Reserved — MUST be zero |

```
token = ((cp & 0x7FFFFFFF) << 16) | (script & 0xFFFF)
```

`suas_ssp_tag_codepoint()` packs a token; `suas_ssp_token_script()` and
`suas_ssp_token_codepoint()` unpack it. The `suf` font engine uses the low
script id as a **direct index** into its registered typeface-block table, so a
shaped run routes to the correct script face in `O(1)` with no multi-font
fallback search. Reserved bits are masked on unpack and MUST be zero on pack.

### 5.6 Reconciliation with SUCPM `Script_Identifier`

SUTR-8 (SUCPM) §5.2 packs the `Script_Identifier` into **bits 45–54** of the
64-bit extended property mask — a **10-bit** field (`0x000`–`0x3FF`). SSP uses
a **16-bit** id. The two are reconciled as follows:

- **Bridge scripts pack losslessly.** Every Bridge id (`0x0000`–`0x03FF`)
  fits the 10-bit field exactly. `SUAS_SCRIPT_BRIDGE_MAX` is `0x03FF`
  precisely so the Bridge namespace *is* the SUCPM field: a SUCPM consumer
  reads the field and hands it to `suas_ssp_script_long_name()` unchanged.
- **Native ids are range-dispatched, not packed.** Registry script values are
  assigned at `0x1000` and above and therefore exceed 10 bits; they MUST NOT be
  truncated into the field. For a codepoint in the RNUR carve-out, the
  authoritative script comes from the tier-3 coordinate decode of §5.4 (the
  codepoint itself encodes the set). A SUCPM mask for such a codepoint carries
  the RNUR tier flag and a reserve value in the `Script_Identifier` field; the
  consumer recovers the registry script from the codepoint, not from the packed
  bits. Until a registry is mounted the field is unused and the codepoint
  resolves to `UNKNOWN`.
- **SSP is the source of truth for the value.** SUCPM defines the *packing*
  of the value; SSP defines the *value*. Where the two disagree on a Bridge
  codepoint, the SUCD-derived SSP table governs.

This keeps the fast path fast (10-bit mask test for all Bridge text) while
letting the full 16-bit Native namespace remain addressable.

---

## 6. Data Files

The **source of truth** for the Script and Script_Extensions properties is
**SUCD** (`sucd/Scripts.txt` and `sucd/ScriptExtensions.txt`, derived from the
Unicode Character Database and versioned with SuperUnicode). The reference
implementation curates a representative subset of those files into three
immutable, sorted, non-overlapping tables:

| Table | Content |
|-------|---------|
| `SSP_SCRIPT_TABLE` | Explicit Bridge ranges → script id (sorted by `lo`). |
| `SSP_COMMON_TABLE` | Merged Common / Inherited ranges (sorted by `lo`), consulted after the explicit table. |
| `SSP_SCX_TABLE` | Codepoints whose Script_Extensions set is not the singleton, each with up to four ids. |

Lookups are binary searches; the tables are `static const` and never mutated. A
codepoint absent from all Bridge tables resolves to `UNKNOWN`. Long and short
names live in a fourth static table (`SSP_NAMES`). Where a curated table and
SUCD differ, **SUCD governs**; the curation is a performance and
freestanding-size choice, not a redefinition.

---

## 7. Implementation Notes

- **Dispatch first.** Test the district with range comparisons before any
  table search; the Bridge test is a single `<= 0x0010FFFF` comparison and the
  SCP / RNUR / terminal districts are `O(1)` tests.
- **Explicit before Common.** Always consult the explicit Bridge table before
  the Common table so an explicit script wins an overlap.
- **Binary search, bounded.** Both the range tables and the `scx` table are
  searched with a half-open `[lo, hi)` binary search; the iteration count is
  bounded by `log2(R)`, no recursion.
- **Freestanding C99.** The implementation compiles under `-std=c99
  -ffreestanding` with no libc dependency. No `malloc`, no string library:
  tag comparison walks bytes directly.
- **Immutable static data.** All tables are `static const`; the engine holds no
  mutable global state.
- **Reserved bits.** On pack, reserved token bits (`47–63`) MUST be zero; on
  unpack, mask the script field and shift the codepoint field. Never trust
  stale high bits.
- **Never return `NULL`.** Name accessors fall back to the `UNKNOWN` forms so
  callers can print unconditionally.

---

## 8. Recommendations

- **Matching text.** Use `suas_ssp_matches_script()` for shaped-text tests
  (it honors Script_Extensions); use `suas_ssp_get_script()` only when the
  base Script value itself is required.
- **Combining marks and shared punctuation.** Resolve them with
  `suas_ssp_resolve(cp, context)` carrying the preceding base's script; do not
  assume `COMMON` is a terminal answer.
- **Directives sparingly.** Use an SCP script-context directive only to force a
  context that cannot be inferred, and keep it disjoint from BANcodes and the
  SDF/SBR directives.
- **Names, not literals.** Obtain script names through the accessors rather
  than hard-coding strings, so a future assignment extends cleanly.
- **Registry scripts.** Treat the Native conlang scripts as available only
  after the RNUR plugin is mounted, and never publish their private-use codes
  outside the SuperUnicode context.

---

## 9. Stability and Versioning

- **Versioned.** SSP is at **1.0.0**. The property values are pinned to the
  SUCD version shipped with a SuperUnicode release; a later release may add
  Bridge or Native values but never renumbers an existing one.
- **Bridge compatibility is permanent.** `0x000000`–`0x0010FFFF` tracks
  Unicode; an assignment fixed in one SUCD version is never moved.
- **Extensions append.** New Bridge scripts occupy the reserved gap below
  `SUAS_SCRIPT_BRIDGE_MAX`; new Native scripts are appended at `0x1000` and
  above, in registry order.
- **Property-name stability.** Long names, short names, and the loose-matching
  contract are frozen; new aliases may be added, but an existing tag never
  changes meaning.
- **Source of truth.** Where a curated table and a newer SUCD disagree, SUCD
  governs; a conforming process MAY rebase its curation on SUCD without
  changing the property model.

---

## 10. Compliance Matrix

| Requirement | Mandatory | Verification |
|-------------|-----------|--------------|
| Enumerated Script value for every codepoint | Yes | `suas_ssp_get_script` over all districts. |
| Two implicit values (`UNKNOWN`, `COMMON`) | Yes | `suas_ssp_is_implicit`. |
| Merged Common + Inherited with inheritance | Yes | `suas_ssp_resolve` inheritance tests. |
| Script_Extensions set, non-empty, bounded at 4 | Yes | `suas_ssp_get_script_extensions`; singleton default. |
| `scx` membership predicate | Yes | `suas_ssp_scx_contains`, `suas_ssp_matches_script`. |
| Explicit-only SCP script context | Yes | Directive sub-range `0x00112000`–`0x00119FFF`; disjoint from BANcodes / SDF / SBR. |
| RNUR carve-out defined; unregistered → `UNKNOWN` | Yes | `0x00160000`–`0x0016FFFF` resolves to `UNKNOWN` until a registry mounts. |
| RCCR excluded (codes, no set layer) | Yes | No codepoint mapping for BCP 47 tags. |
| 16-bit id packs into a 64-bit token | Yes | `suas_ssp_tag_codepoint` round-trip tests. |
| Bridge compat with SUCD Script(s)_Extensions | Yes | Curated tables; SUCD is source of truth. |
| SUCPM `Script_Identifier` reconciliation | Yes | Bridge ids ≤ `0x03FF`; natives range-dispatched (§5.6). |
| Bounded time, zero allocation, freestanding C99 | Yes | Binary search + `O(1)` dispatch; no `malloc`; `-ffreestanding`. |
| Stable, never-renumbered ids | Yes | Value table (§3.1) frozen at v1.0.0. |

---

## 11. Terms & Conventions

- **SSP** — System Script Property; the SUAS-005 script architecture.
- **Script** — the enumerated property assigning one writing system (or
  implicit value) to a codepoint.
- **Script_Extensions (`scx`)** — the set-valued generalization of Script; the
  scripts in which a codepoint is commonly used.
- **`suas_script_t`** — the 16-bit SSP script id (Bridge range
  `0x0000`–`0x03FF`; Native range `0x1000`+).
- **Common** — the merged implicit value for Unicode's `Common` and
  `Inherited` scripts.
- **Inherited** — Unicode's implicit script for marks; merged into `COMMON`
  here and reconstructed by `suas_ssp_resolve()`.
- **Implicit value** — `UNKNOWN` or `COMMON`; carries no standalone script and
  yields to an explicit context.
- **RNUR** — Reddit Neographical Unicode Registry; a PUA coordinate registry
  decoded into the Native carve-out `0x00160000`–`0x0016FFFF`.
- **RCCR** — Reddit Conlang Code Registry; a registry of BCP 47 language codes,
  not codepoints, contributing no Script property.
- **SCP** — System Control Plane (`0x00110000`–`0x0011FFFF`); carries the
  script-context directives at `0x00112000`–`0x00119FFF`.
- **Bridge** — the Unicode Bridge, `0x000000`–`0x0010FFFF`, permanently 1:1
  with Unicode.
- **Native plane** — `0x00120000`–`0x7FFFFFEF`; addressable by mounted
  plugins/registries; terminal traps and Sentinel above it are unallocatable.
- **SUCD** — SuperUnicode Character Database; source of truth for Script and
  Script_Extensions data.
- **SUCS / ExtSUCS** — the 31-bit SuperUnicode codepoint space and its 64-bit
  extension (`sucs_ex_char_t`).
- **SUCPM** — the SuperUnicode Character Property Model (SUTR-8); defines the
  bitmask packing, including the 10-bit `Script_Identifier` field.
- **Script token** — a 64-bit pack of a 16-bit script id (bits 0–15) and a
  31-bit codepoint (bits 16–46) for the `suf` font engine.
- **ISO 15924** — the international standard for script codes; the source of
  SSP short names for Bridge scripts.
- **Loose matching** — case-insensitive matching ignoring SPACE, HYPHEN-MINUS,
  and UNDERSCORE (UAX #44 §5.9).

---

**END OF SUAS-005**
