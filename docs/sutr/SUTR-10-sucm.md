# SUTR-10 — SuperUnicode Conformance Model (SUCM)

**Standard ID:** SUTR-10
**Status:** 0.1.0 — Published
**Scope:** The strict compliance tiers, the zero-allocation and O(1) execution mandate, the conformance claim rules, and the automated verification gates required of every SuperUnicode subsystem
**Applies To:** SUAS-001 (SDF), SUAS-002 (SGW), SUAS-003 (SBR), SUAS-004 (SUCF), SUAS-005 (SSP), SUAS-006 (STS), SUAS-007 (SIS), SUTS-001 (SUCA), SUTS-002 (SRE), SUTR-0 (SUCS Core), SUTR-5 (Plugin Lifecycle), SUTR-6 (Bridge), SUTR-7 (SUCEM), SUTR-8 (SUCPM), SUTR-9 (SUMNLS), the kernel test harness, the documentation verification gate, and the ExtSUCS plugin system
**Last Updated:** 2026-10-18

---

## 1. Overview

Where UTR #33 *Unicode Conformance Model* defines what it means for a process to
claim conformance to the Unicode Standard — conformance clauses, implementation
levels, and the interoperability guarantees that follow from them —
**SUTR-10 (SUCM)** establishes the equivalent contract for SuperUnicode: the
strict **compliance tiers**, the **zero-allocation and O(1) execution mandate**,
the **conformance claim rules**, and the **automated verification gates** that
every kernel subsystem, driver, or ExtSUCS plugin must satisfy before a system
image may be compiled.

SUCM does not restate the individual algorithm contracts; those are the SUAS and
SUTS documents. SUCM binds them together into a single, checkable compliance
surface: it names the tiers, fixes the resource ceilings, defines how a
conformance claim is written and named, and specifies the pipeline that proves a
claim is true. A subsystem that satisfies its own specification but violates the
SUCM mandate — by allocating, by scanning in variable time, or by leaking an SCP
marker through an identifier — is **not** SuperUnicode-conformant, regardless of
its algorithmic correctness.

The report answers four questions that every claim must share:

1. **Which tier** does the subsystem claim, and what does that tier require?
2. **What resource ceilings** — allocation, stack, and time — must it hold to?
3. **How is a conformance claim** written, named, and published?
4. **How is a claim verified** programmatically before the system image is built?

### 1.1 Relation to UTR #33

SUCM is the SuperUnicode analog of UTR #33 *Unicode Conformance Model*. It
adopts that report's framing: conformance is a *claim*, claims are graded by
*level*, and the meaning of a claim is fixed by numbered clauses. The UTR #33
framing is restated here so the contract is self-contained; where this report and
UTR #33 differ, **this report governs for SuperUnicode**.

The decisive difference is the resource model. UTR #33 grades conformance by
which clauses of the standard a process implements, and is silent on how the
process spends memory or time. SUCM grades conformance by which **tiers** a
subsystem satisfies *and* by whether it holds the **zero-allocation and O(1)
mandate** of §4. On SuperUnicode, a correct algorithm that allocates on the hot
path or scans in variable time does not conform, because the kernel — the shell,
the compositor, the nVAI runtime — runs on a freestanding target with no general
allocator and no bounded-time budget for a backtracking pass.

### 1.2 Scope

This report specifies the three conformance tiers (§3), the zero-allocation and
O(1) execution mandate and its resource ceilings (§4), the conformance claim
clauses and their naming rules (§5), the normative requirements (§6), and the
automated verification pipeline with its gates and data files (§7–§8). It does
**not** define any character property, encoding form, or algorithm; those remain
with SUTR-0 and SUTR-6–9 and with the SUAS and SUTS families. It also does not
define a build system: it defines the *gates* a build system must enforce, and
names the harness commands that carry them.

### 1.3 Key architectural concepts

**1.3.1 Zero-allocation and O(1) execution mandate**

Unlike a conventional conformance model, which permits dynamic heap allocation
and variable-time multi-pass scans, SUCM enforces strict **zero-allocation and
deterministic O(1) stack execution** across all core validation engines. Every
engine named in §3.1 — the SUAS engines for directional framing, glyph width,
line breaking, canonical forms, script, segmentation, and identifiers, together
with the SUTS engine the collation and regular-expression layers build on — must
resolve each input unit with a bounded number of integer operations, in a fixed
amount of stack, with no call to a general allocator (§4).

**1.3.2 The three conformance tiers**

SUCM grades conformance in three cumulative tiers:

| Tier | Name | Kernel surface |
|------|------|----------------|
| Level 0 | Kernel Core Conformance | Freestanding C99, single-pass state machines, SCP invariants |
| Level 1 | Unicode Bridge Conformance | Permanent 1:1 bridge equivalence, zero normalization drift |
| Level 2 | Extended & Plugin Conformance | Native-plane gate, checksum gating, neography carve-out |

Level 1 includes Level 0; Level 2 includes Level 1. A subsystem *claims* a tier;
a higher claim asserts every lower claim (§3.4).

**1.3.3 Automated verification pipeline**

SUCM mandates that **all conformance claims are verified programmatically** —
never asserted in prose alone — by the kernel test harness (`ctest`) and the
documentation verification gate (`verify-docs`) before a system image is
compiled (§7). A claim that is not backed by a passing gate is void.

### 1.4 Conformance specification matrix

| Concern | Graded by | Evaluation | Primary gate |
|---------|-----------|------------|--------------|
| Kernel execution | Level 0 | Freestanding C99, single-pass, bounded stack | `ctest` |
| SCP invariance | Level 0 | No SCP marker admitted to an identifier, break, or property | `ctest` |
| Bridge equivalence | Level 1 | 1:1 round-trip over `0x000000–0x10FFFF`, no drift | `ctest` |
| Normalization drift | Level 1 | Bridge text is everstable under SUCF | `ctest` |
| Native-plane gate | Level 2 | Plugin codepoints inside `0x120000–0x7FFFFFEF` | SUTR-5 gate |
| Integrity gate | Level 2 | CRC32c / Fletcher-64 checksum before mount | SUTR-5 gate |
| Neography mapping | Level 2 | PUA resolves to `0x160000–0x16FFFF` carve-out | `ctest` |
| Specification integrity | All | Every claimed spec is present and cross-linked | `verify-docs` |

The tiers are **cumulative and monotone**: a subsystem that satisfies Level 2
necessarily satisfies Level 1 and Level 0, because the higher tier's clauses
include the lower tier's.

### 1.5 Cross-references

| Topic in this report | Normative elsewhere |
|----------------------|---------------------|
| Tier-0 engines and algorithms | SUAS-001–007, SUTS-001, SUTS-002 |
| SUCS three-space layout, SCP, traps, Sentinel | SUTR-0 |
| Plugin lifecycle, checksum gate, mount, registry | SUTR-5 |
| Permanent 1:1 bridge guarantee | SUTR-6 |
| Four-level character encoding model | SUTR-7 |
| Character property model and bitmask packing | SUTR-8 |
| Native Math ExtSUCS Space (`0x00170000`) | SUTR-9 |
| Neography carve-out (`0x00160000–0x0016FFFF`) | SUTR-5 §5, SUTR-8 §4.3 |

---

## 2. Definitions and notation

### 2.1 Definitions

**SUCM-D1. Conformance unit.** A single deliverable that can make a conformance
claim: a kernel subsystem, a driver, or an ExtSUCS plugin. A conformance unit is
identified by a stable name and a version.

**SUCM-D2. Conformance claim.** An assertion, published in a manifest (§8), that
a conformance unit satisfies a named tier of this report. A claim is a tuple
`(tier, unit, version, clauses, gate-record)`.

**SUCM-D3. Tier.** One of the three cumulative compliance levels of §3:
`L0` (Kernel Core), `L1` (Unicode Bridge), `L2` (Extended & Plugin).

**SUCM-D4. Core validation engine.** A freestanding, single-pass engine that
resolves a property, a class, or a boundary in O(1) per input unit. The core
engines of a Level-0 claim are the SUAS engines — `suas_sdf`, `suas_sgw`,
`suas_sbr`, `suas_sucf`, `suas_ssp`, `suas_sts`, `suas_sis` — and the library
engines the SUTS documents build on.

**SUCM-D5. Resource ceiling.** A hard upper bound on a conformance unit's
consumption of one resource — heap, stack, or time — that the unit must not
exceed for any admitted input. The ceilings are fixed in §4.2.

**SUCM-D6. Verification gate.** A programmatic check that must pass before a
conformance claim is published and before a system image is compiled. SUCM has
two gates: the test gate (`ctest`) and the documentation gate (`verify-docs`)
(§7).

**SUCM-D7. Gate record.** The machine-readable evidence that a gate passed: the
harness name, the harness version, the set of suites or documents exercised, and
the result. A gate record is attached to every claim (§8).

**SUCM-D8. System image.** A linked kernel or firmware artefact produced from
conformance units. A system image may only be compiled from units whose claims
hold at the required tier (§7.4).

### 2.2 Notational conventions

SUCM adopts the SuperUnicode conventions for referring to a sequence of
codepoints treated as a unit, using angle brackets around a comma-delimited list
of codepoints. Both designations below denote the same two-codepoint sequence:

```text
<U+012B, U+0300>
<U+012B LATIN SMALL LETTER I WITH MACRON, U+0300 COMBINING GRAVE ACCENT>
```

A sequence identifier contains **at least two** codepoints. This is a
generalization of the ISO/IEC 10646 UCS Sequence Identifier (USI), which has the
form

```text
<UID1, UID2, ... UIDn>
```

where each `UIDi` is a short codepoint identifier — most commonly `U+012B` or
`012B`. Conformance manifests use exactly this notation when a clause is
expressed over a sequence rather than a single codepoint (§8). In a manifest the
bracketing may be dropped in favour of space delimitation, exactly as in an SUCD
data file.

### 2.3 Claim identifiers

A claim is named by a **claim identifier**: an uppercase ASCII string with the
form

```text
<tier>/<unit>/<version>
```

for example `L1/sucf/0.1.0`. Claim identifiers are unique in the SUCM namespace
for claims (§5.4) and are constructed under the naming rules of §5.4.

---

## 3. Conformance tiers

SUCM defines three cumulative tiers. A conformance unit that claims tier *n*
asserts every clause of tiers `0..n`.

### 3.1 Level 0 — Kernel Core Conformance

**Level 0 (Kernel Core Conformance):** requires complete adherence to
freestanding C99 execution, single-pass state machines, and proper handling of
System Control Plane (SCP) invariants.

A Level-0 unit must:

1. compile and run under freestanding C99 with the freestanding subset only
   (`<stdint.h>`, `<stdbool.h>`, `<stddef.h>`), with no dependency on a hosted C
   library (§4.1);
2. resolve each input unit through a **single pass** over a **bounded state**
   (§4.3);
3. hold every core engine to the resource ceilings of §4.2;
4. treat every System Control Plane marker consistently with its owning
   specification — in particular, no SCP marker may be admitted as an identifier
   character (SUAS-007 §2.4), a segment of user text (SUAS-006), a script value
   (SUAS-005), a canonical-form input (SUAS-004), or a break opportunity
   (SUAS-003);
5. be **total**: for every input in the admitted domain, including unassigned,
   Native, trap, and Sentinel codepoints, it returns a defined result and never
   fails an assertion on well-formed input.

Level 0 is the floor. A subsystem that allocates on the hot path, that scans in
variable time, or that lets an SCP marker through an invariant is not Level-0
conformant even if its output is otherwise correct.

### 3.2 Level 1 — Unicode Bridge Conformance

**Level 1 (Unicode Bridge Conformance):** guarantees permanent, invariant 1:1
round-trip equivalence across the `0x000000–0x10FFFF` bridge range under SUTR-6,
with zero normalization drift.

A Level-1 unit must:

1. satisfy Level 0;
2. preserve the permanent **1:1** correspondence between Bridge codepoints and
   their Unicode scalar values (SUTR-6); no Bridge codepoint may be re-mapped,
   merged, or aliased;
3. round-trip **losslessly**: decoding an encoded Bridge stream and re-encoding it
   reproduces the original codepoint sequence exactly, with no insertion,
   deletion, or reordering;
4. exhibit **zero normalization drift**: text that is in a Normalization Form
   before a transform is in the same form after any Bridge-preserving transform,
   in agreement with SUCF (SUAS-004);
5. report any codepoint the Bridge does not carry — above `0x10FFFF` — as
   **Native**, never as a Bridge character.

### 3.3 Level 2 — Extended & Plugin Conformance

**Level 2 (Extended & Plugin Conformance):** validates that dynamic modules
adhere to the Native-plane plugin range gate (`0x120000–0x7FFFFFEF`), pass
checksum gating (CRC32c / Fletcher-64) under SUTR-5, and correctly resolve PUA
neography mappings to the `0x160000–0x16FFFF` carve-out.

A Level-2 unit must:

1. satisfy Level 1;
2. allocate plugin codepoints **only** within the Native plane,
   `0x00120000–0x7FFFFFEF`, or strictly above `0x7FFFFFFF`; the trap slots
   `0x7FFFFFF0–0x7FFFFFFE` and the Sentinel `0x7FFFFFFF` are unallocatable
   (SUTR-0);
3. pass the **integrity gate** before mount: a CRC32c or Fletcher-64 checksum
   computed over the plugin blob must match its declared digest (SUTR-5);
4. resolve **PUA neography** mappings deterministically to the carve-out
   `0x00160000–0x0016FFFF`; a Private-Use codepoint that carries a neography
   mapping must never be reported as an ordinary Bridge character;
5. fail **closed**: a plugin that fails the range gate or the integrity gate
   contributes no characters and is not registered.

### 3.4 Tier inclusion

The tiers are cumulative. The claim relation is:

```text
L2 ⊃ L1 ⊃ L0
```

A unit that claims `L2` is graded against all clauses of `L0`, `L1`, and `L2`.
The gates of §7 evaluate a claim at its stated tier and assert every lower tier
as a precondition.

---

## 4. Zero-allocation and O(1) execution mandate

### 4.1 Allocation ceiling

SUCM prohibits dynamic allocation on any core engine path.

| Rule | Statement |
|------|-----------|
| `SUCM-4.1a` | A core engine must not call a general allocator (`malloc`, `calloc`, `realloc`, `free`, or a platform equivalent) on any path reachable from its public entry points. |
| `SUCM-4.1b` | A core engine must not depend on a hosted C library facility; it uses the freestanding subset only. |
| `SUCM-4.1c` | All working state is caller-owned and fixed-size: a stack value, a struct returned by value, or a buffer whose length is a compile-time constant. |
| `SUCM-4.1d` | A conforming unit must not grow its state with input length; the state size is the same for a 1-codepoint input and a 10¹²-codepoint input. |

### 4.2 Resource ceilings

Every core engine declares a **memory class** and a **time class**, both fixed at
compile time.

| Resource | Ceiling | Meaning |
|----------|---------|---------|
| Heap | `0 bytes` | No dynamic allocation on any core path |
| Stack | bounded by a compile-time constant | The engine's state is O(1), not O(n) |
| Time per input unit | `O(1)` | A constant number of integer operations per codepoint |
| Time per stream | `O(n)` | Linear in the number of input units, with no super-linear term |
| Backtracking | `none` | No re-scan of an already-consumed unit |

A unit whose per-unit cost grows with the input, or which re-scans a unit it has
already returned, violates the mandate even if its total work is finite.

### 4.3 Single-pass state

A Level-0 unit processes each input unit exactly once, in order, and carries all
of the context a later decision needs in a **bounded** register. The canonical
form is a caller-owned state struct with a fixed number of scalar fields —

```c
/* e.g. SUAS-007 SIS, SUAS-006 STS */
typedef struct {
    bool     is_started;   /* or an equivalent bounded context */
    uint32_t length;
} <engine>_state_t;
```

— and a feed step that returns the decision for the position just resolved:

```text
feed(state, unit) -> decision        /* O(1); state is bounded */
```

No engine may depend on a history buffer whose length is proportional to the
stream. Where a specification needs a short window (for example STS carries a
fixed 4-codepoint ring), the window is a compile-time constant, not a heap buffer.

### 4.4 Determinism

Given the same admitted input and the same build, a conforming unit produces
byte-identical results across runs, across threads, and across platforms. No
core engine reads a locale, an environment variable, the clock, or a random
source.

---

## 5. Conformance claims and clauses

### 5.1 Claim clauses

A conformance claim is expressed with the following clauses.

**SUCM-C1.** A process that purports to implement a SuperUnicode conformance
tier shall claim that tier by name (`L0`, `L1`, or `L2`) in a published manifest
(§8), and shall claim no tier whose clauses it does not satisfy.

**SUCM-C2.** A process that claims a tier shall satisfy every clause of that tier
and of every lower tier (§3.4); the tiers are cumulative.

**SUCM-C3.** A process that claims a core engine shall hold the zero-allocation
and O(1) mandate of §4 for that engine's entire public surface.

**SUCM-C4.** A process that claims Level 1 shall preserve the permanent 1:1
Bridge guarantee of SUTR-6, with zero normalization drift.

**SUCM-C5.** A process that claims Level 2 shall enforce the Native-plane range
gate and the SUTR-5 integrity gate before mount, and shall resolve PUA neography
to the `0x00160000–0x0016FFFF` carve-out.

**SUCM-C6.** A process shall not publish a conformance claim that is not backed
by a passing gate record (§7); an unproven claim is void.

Conformance to these clauses does not constrain a process from offering
*informal* extensions — additional engines, higher-level parsers, host-side
tooling — provided such extensions are not described as part of the standard and
do not weaken a claimed tier.

### 5.2 Requirement identifiers

Each clause of this report carries a requirement identifier of the form
`SUCM-Cn` (conformance), `SUCM-Dn` (definition), or `SUCM-Rn` (requirement). The
identifiers are stable: a clause is never renumbered; a withdrawn clause is
marked `withdrawn` and never reused.

### 5.3 Provisional engines

An engine proposed for a tier but not yet ratified may be listed as
**provisional** in the manifest. A provisional engine's clauses have no
conformance force; it becomes part of a tier only when its owning specification
and its gate record are ratified. The provisional list exists so an in-flight
engine can be tracked without weakening a published claim.

### 5.4 Names and uniqueness

Claim identifiers (§2.3) are unique in the **SUCM namespace for claims**. They
are constructed under the following rules.

**SUCM-R1.** Only Latin capital letters `A`–`Z`, digits `0`–`9` (not first in a
word), SPACE, and HYPHEN-MINUS are used for the unit and version components.

**SUCM-R2.** Only one claim is published per `(tier, unit, version)`; each claim
identifier is unique within the namespace.

**SUCM-R3.** Two claim identifiers are the same if they are equal after SPACE and
medial HYPHEN-MINUS are ignored and the words `LEVEL`, `TIER`, and `CORE` are
ignored. A collision must be resolved by qualifying the unit name, never by
reusing an identifier.

**SUCM-R4.** A claim identifier, where possible, is formed from the unit's module
name and its owning spec, so that the identifier and the specification name agree
(for example `L0/sis/0.1.0` for the SIS engine of SUAS-007 at version 0.1.0).

---

## 6. Requirements

The following requirements are normative for a subsystem that claims a tier.

**SUCM-R5.** **Freestanding.** Core engines build with the freestanding subset
and no general allocator (§4.1).

**SUCM-R6.** **Bounded.** Every core engine declares a compile-time memory class
and time class, and holds them under test (§4.2).

**SUCM-R7.** **Single pass.** Every core engine consumes each input unit once and
carries bounded state (§4.3).

**SUCM-R8.** **SCP invariance.** No System Control Plane marker may be admitted
as identifier text, user text, a property value, a canonical input, or a break
opportunity. An SCP marker is either decoded as a directive by its owner or
rejected, never silently absorbed.

**SUCM-R9.** **Bridge 1:1.** The bridge range `0x000000–0x10FFFF` is permanently
1:1 with Unicode; round-trips are lossless and drift-free (§3.2).

**SUCM-R10.** **Native gate.** Plugin codepoints are allocated only in
`0x00120000–0x7FFFFFEF` or above `0x7FFFFFFF`; trap slots and the Sentinel are
unallocatable (§3.3).

**SUCM-R11.** **Integrity gate.** A plugin mounts only after its CRC32c or
Fletcher-64 checksum matches its declared digest (SUTR-5).

**SUCM-R12.** **Neography mapping.** PUA neography resolves to the
`0x00160000–0x0016FFFF` carve-out, deterministically and without aliasing a
Bridge character (§3.3).

**SUCM-R13.** **Totality.** Every core engine returns a defined result for every
input in its admitted domain and never fails an assertion on well-formed input
(§3.1).

**SUCM-R14.** **Determinism.** Core engines are byte-deterministic across runs,
threads, and platforms (§4.4).

**SUCM-R15.** **Gated claim.** A conformance claim is published only with a
passing gate record (§7), and a system image is compiled only from units whose
claims hold (§7.4).

---

## 7. Automated verification pipeline

SUCM mandates that every conformance claim is verified **programmatically**. Two
gates carry the mandate, and both must pass before a system image is compiled.

### 7.1 The test gate

The **test gate** is the kernel test harness. Every core engine carries a
conformance suite registered with the harness, and the gate passes only when the
entire suite set is green.

```text
ctest --test-dir build            # all suites must pass
```

The harness is the authoritative evidence that the Level-0, Level-1, and
Level-2 clauses hold. A suite that is skipped, disabled, or flaky is a failing
gate. The gate record (§8) names the suites exercised and their result.

### 7.2 The documentation gate

The **documentation gate** verifies that the specification scaffold is intact:
every document family index is present and every claimed specification resolves.

```text
cmake --build build --target verify-docs   # all document indexes present
```

The gate passes only when all family indexes (`suas/INDEX.md`, `suts/INDEX.md`,
`sutr/INDEX.md`) are present and the cross-links a claim depends on resolve. A
claim whose owning specification is missing fails the gate.

### 7.3 The build gate

A system image is built from conformance units under a **build gate**: the build
must not enable a unit whose tier is lower than the image requires, and must not
link a plugin that has not passed the SUTR-5 integrity gate. The build gate is
enforced by the build system, which reads the manifests of §8.

### 7.4 Image compilation

**An image may be compiled only when both gates are green.** The order is fixed:

```text
build → ctest (test gate) → verify-docs (documentation gate) → image compile
```

A red gate aborts the image. There is no override: a subsystem that cannot pass
the gates is not eligible for the tier it claims.

---

## 8. Data files

SUCM adds the following machine-readable data to the SuperUnicode distribution,
under `Public/0.1.0/conformance/`:

| File | Contents |
|------|----------|
| `SUCMTiers.txt` | The three tiers of §3, with their clause ranges |
| `SUCMEngines.txt` | The core engine registry: module name, owning spec, memory class, time class |
| `SUCMClaims.txt` | One row per published claim: `tier;unit;version;clauses;gate-record` |

The file formats follow the SUCD conventions used elsewhere (semicolon-separated
fields, `#` comments, space-delimited codepoint sequences in the notation of
§2.2). `SUCMClaims.txt` is the authoritative input to the build gate of §7.3; a
claim absent from the file does not exist for the purposes of image compilation.

A claim row is, for example:

```text
L0;sis;0.1.0;SUCM-C1,SUCM-C2,SUCM-C3,R5-R8,R13-R14;ctest:green
```

Gate records are stored beside the claims and name the harness, the suites or
documents exercised, and the result.

---

## 9. Implementation notes

- **State is caller-owned.** A conforming engine returns or accepts a fixed-size
  state struct; it never owns heap storage. This is what makes the stack ceiling
  of §4.2 checkable by inspection.
- **Time class is a claim, not a hope.** A memory/time class is declared at
  compile time and asserted by the suite; an engine that cannot state its class
  cannot claim a tier.
- **SCP is decoded once.** A marker is either consumed by its owning directive
  decoder or rejected at the tier boundary; it is never pushed into an invariant.
- **The gates are the contract.** The clauses of §5–§6 are meaningful only
  because §7 turns them into a pass/fail signal. Read together, they are the
  compliance surface.
- **Bridge is forever.** The Level-1 guarantee is permanent and versionless: no
  future release re-maps a Bridge codepoint (SUTR-6).
- **Plugins fail closed.** A plugin that cannot prove its range and its checksum
  contributes nothing; it never degrades to a partial mount.
- **Order is normative.** Build, then test, then docs, then image (§7.4). A gate
  run out of order is not evidence.

---

## 10. Recommendations

1. **Claim the lowest true tier.** Do not claim a tier to look complete; claim
   what the gates prove.
2. **Declare the classes early.** State a memory and time class when the engine
   is designed, not after it ships.
3. **Keep the hot path allocation-free.** Treat an allocation on a core path as a
   conformance bug, not a performance bug.
4. **Gate every claim.** A claim with no gate record is not a claim.
5. **Prefer Bridge codepoints.** Allocate a Native codepoint only when the
   repertoire has no Bridge codepoint (SUTR-8, SUTR-9).
6. **Map PUA deliberately.** Resolve neography into the carve-out rather than
   aliasing a Bridge character.
7. **Never absorb an SCP marker.** Decode it or reject it; silence is a defect.

---

## 11. Stability and versioning

SUCM inherits the SUCS stability guarantees (SUTR-0) and the bridge permanence
(SUTR-6), and adds the following:

- **Tier count is fixed at three.** The tier set `L0`, `L1`, `L2` is a closed
  enumeration; a future tier is appended, never inserted, and never renumbers an
  existing tier.
- **Clause identifiers are stable.** `SUCM-Dn`, `SUCM-Cn`, and `SUCM-Rn` are
  never renumbered; a withdrawn clause is marked and never reused.
- **The mandate is permanent.** The zero-allocation and O(1) ceilings of §4 are
  invariants of the model; a change would require a new report, not an edit.
- **The gates are stable interfaces.** The two gate commands of §7 are the
  stable interface to the pipeline; their implementation may change, their
  meaning may not.
- **Claims are per-version.** A claim is bound to a unit version; a new version
  requires a new claim and a new gate record.

Current version: **0.1.0**.

---

## 12. Compliance matrix

| Requirement | Mandatory | Verification |
|-------------|-----------|--------------|
| Conformance is claimed by tier name | Yes | SUCM-C1; §8 |
| Tiers are cumulative (`L2 ⊃ L1 ⊃ L0`) | Yes | SUCM-C2; §3.4 |
| Core engines hold the zero-allocation mandate | Yes | SUCM-C3, R5; §4.1 |
| Core engines hold the O(1) time ceiling | Yes | SUCM-R6; §4.2 |
| Core engines are single-pass and bounded | Yes | SUCM-R7; §4.3 |
| SCP markers are never absorbed into an invariant | Yes | SUCM-R8; §3.1 |
| Bridge range is permanently 1:1 | Yes | SUCM-C4, R9; §3.2 |
| Bridge round-trips are lossless and drift-free | Yes | SUCM-R9; §3.2 |
| Native-plane range gate (`0x120000–0x7FFFFFEF`) | Yes | SUCM-R10; §3.3 |
| Plugin integrity gate (CRC32c / Fletcher-64) | Yes | SUCM-R11; SUTR-5 |
| PUA neography maps to `0x160000–0x16FFFF` | Yes | SUCM-R12; §3.3 |
| Core engines are total | Yes | SUCM-R13; §3.1 |
| Core engines are deterministic | Yes | SUCM-R14; §4.4 |
| A claim requires a passing gate record | Yes | SUCM-C6, R15; §7 |
| Test gate (`ctest`) passes before image compile | Yes | §7.1, §7.4 |
| Documentation gate (`verify-docs`) passes | Yes | §7.2, §7.4 |
| Claims and engines are published as data files | Yes | §8 |
| This report defines no property or encoding | Yes | §1.2 |

---

## 13. Master SUTR architecture index

SUCM closes the SUTR series. The reports that precede it establish the
architecture that SUCM then grades:

| Report ID | Title | Core architectural focus |
|-----------|-------|--------------------------|
| SUTR-6 | Unicode Compatibility Bridge | Permanent 1:1 codepoint mapping guarantee (`0x000000–0x10FFFF`) |
| SUTR-7 | SUCEM (UTR #17 equivalent) | Character Encoding Model — 4-level abstraction hierarchy |
| SUTR-8 | SUCPM (UTR #23 equivalent) | Character Property Model — SUCD bitmasks and SCP evaluation |
| SUTR-9 | SUMNLS (UTR #25 equivalent) | Mathematical Notation and Layout Support |
| SUTR-10 | SUCM (UTR #33 equivalent) | Conformance Model — implementation levels, zero-allocation rules, and verification gates |

---

## 14. Terms & conventions

- **SUCM** — SuperUnicode Conformance Model; this report (SUTR-10).
- **Conformance unit** — a subsystem, driver, or plugin that can make a claim.
- **Conformance claim** — a published assertion that a unit satisfies a tier.
- **Tier** — one of `L0` (Kernel Core), `L1` (Unicode Bridge), `L2` (Extended &
  Plugin); cumulative.
- **Core validation engine** — a freestanding, single-pass, O(1) engine; the SUAS
  engines and the SUTS library engines.
- **Resource ceiling** — a hard bound on heap, stack, or time (§4.2).
- **Zero-allocation mandate** — no dynamic allocation on any core path (§4.1).
- **O(1) mandate** — a constant number of integer operations per input unit
  (§4.2).
- **Verification gate** — the test gate (`ctest`) or the documentation gate
  (`verify-docs`) (§7).
- **Gate record** — the machine-readable evidence that a gate passed (§8).
- **System image** — a linked kernel or firmware artefact built only from units
  whose claims hold (§7.4).
- **Native plane** — `0x00120000–0x7FFFFFEF`, the plugin allocation range.
- **Neography carve-out** — `0x00160000–0x0016FFFF`, the RNUR / RCCR PUA mapping
  target.
- **USI** — UCS Sequence Identifier; the `<UID1, UID2, …>` notation generalized
  in §2.2.
- **UTR #33** — Unicode Technical Report #33, *Unicode Conformance Model*, the
  report this one mirrors.
- **SDF / SGW / SBR / SUCF / SSP / STS / SIS** — SUAS-001 … SUAS-007.

---

**END OF SUTR-10**
