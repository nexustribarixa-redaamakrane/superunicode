# SUAS — SuperUnicode Architecture Standards

> Core Kernel Invariants

This directory contains the formal architecture standards governing the
SuperUnicode kernel internals. SUAS documents define the invariant contracts,
memory models, and encoding guarantees that all conforming implementations
must uphold.

**Binding force: normative and mandatory.** Every SUAS requirement marked
MUST or MUST NOT is a condition of SuperUnicode conformance. An implementation
that omits a SUAS invariant is not SuperUnicode-conforming, regardless of which
optional SUTS modules it ships.

## Document Index

| ID | Title | Status |
|----|-------|--------|
| [SUAS-001](SUAS-001-sdf.md) | Structural Directional Framing (SDF) — Core BiDi Architecture | Draft — Ratified |
| [SUAS-002](SUAS-002-sgw.md) | System Glyph Width & Monospace Grid (SGW) — Fixed-Cell Terminal Metrics | Draft — Ratified |
| [SUAS-003](SUAS-003-sbr.md) | System Boundary & Line Break Rules (SBR) — Single-Pass Line Breaking | Draft — Ratified |
| [SUAS-004](SUAS-004-sucf.md) | SuperUnicode Canonical Forms (SUCF) — Dual-Target Canonical (De)composition | Draft — Ratified |
| [SUAS-005](SUAS-005-ssp.md) | System Script Property (SSP) — Range-Dispatched Script Resolution | Draft — Ratified |
| [SUAS-006](SUAS-006-sts.md) | System Text Segmentation (STS) — Single-Pass Grapheme/Word/Sentence Boundaries | Draft — Ratified |
| [SUAS-007](SUAS-007-sis.md) | System Identifiers and Syntax (SIS) — Single-Pass Identifier Classes & Lexical Syntax | Draft — Ratified |
| [SUAS-008](SUAS-008-sncs.md) | System Named Character Sequences (SNCS) — Static-Registry, O(1) Named Sequence Resolution | Draft — Ratified |
| [SUAS-009](SUAS-009-sip.md) | System Ideographic Properties (SIP) — Zero-Allocation, O(1) Ideographic Attribute Extraction | Draft — Ratified |
| [SUAS-010](SUAS-010-ssre.md) | System Specification Registry & Cross-Reference Engine (SSRE) — Embedded Dependency Metadata & Build-Time Linkage Validation | Draft — Authoring |
| [SUAS-011](SUAS-011-scms.md) | System Character Manifest & Schema Standard (SCMS) — Line-Oriented, Binary-Mappable Manifest Grammar & Build-Time Compilation | Draft — Authoring |
| [SUAS-012](SUAS-012-sucd.md) | SuperUnicode Character Database (SUCD) — Verified Binary Character Database, O(1) Range Lookup & Partition Proof | Draft — Authoring |
| [SUAS-013](SUAS-013-sisr.md) | System Ideographic Source Registry (SISR) — Byte-Packed Source Identities, Registry Classes & O(1) Canonical Fallback | Draft — Authoring |
| [SUAS-014](SUAS-014-suvtl.md) | SuperUnicode Vertical Text Layout (SUVTL) — Byte-Packed Orientation Records & O(1) Upright/Rotated/Transformed Resolution | Draft — Authoring |
| [SUAS-015](SUAS-015-suamr.md) | SuperUnicode Arabic Mark Rendering (SUAMR) — Byte-Packed Mark-Layout Records & O(1) Arabic Mark Stacking and AMTRA Reordering | Draft — Authoring |
| [SUAS-016](SUAS-016-sukemet.md) | SuperUnicode Egyptian Hieroglyph Database (SUKEMET) — Byte-Packed Hieroglyph Records & O(1) Gardiner/Unikemet Classification | Draft — Authoring |
| [SUAS-017](SUAS-017-sueasd.md) | SuperUnicode East Asian Script Data (SUEASD) - Byte-Packed Script Records, Classification Ranges & O(1) UAX #60 Ancillary Resolution | Draft - Authoring |
| [SUAS-018](SUAS-018-skto.md) | System Kernel Text Objects (SKTO) - Bounded Text Descriptors, Ownership Lifetime & Untrusted-Text Admission | Draft - Authoring |
| [SUAS-019](SUAS-019-stim.md) | System Text Input Model (STIM) - Deterministic Key-Event Resolution, Dead-Key Composition & Bounded SUTF-8 Emission | Draft - Authoring |

## Scope

- Kernel invariant definitions
- Memory layout and alignment contracts
- Codepoint representation guarantees
- Conformance requirements for implementations
- Bi-directional layout, scope isolation, and glyph mirroring
- Single-byte / fixed-cell width metrics and monospace grid layout
- Line-boundary resolution and system-controlled break directives
- Canonical equivalence: dual-target canonical (de)composition over ExtSUCS (64-bit `sucs_ex_char_t` fast path)
- Script identification, Script_Extensions boundaries, and font-fallback routing
- Grapheme, word, and sentence segmentation over the ExtSUCS space (64-bit `sucs_ex_char_t` fast path)
- Identifier classes (ID_Start / ID_Continue), lexical syntax, and SCP invariance
- Named character sequences: static multi-codepoint packing and O(1) name resolution
- Ideographic properties: static radical-stroke tables, classification bitfields, and O(1) attribute extraction
- Specification registry: embedded dependency metadata, cross-reference kinds, and build-time linkage validation
- Character manifests: line-oriented, binary-mappable grammar, bounded single-pass parsing, and range/plane validation
- Character database: verified binary image, three-kind page directory, O(1) range lookup, and partition proof
- Ideographic source identities: byte-packed source records, registry classes, and O(1) canonical variant fallback
- Vertical text layout: byte-packed orientation records, total algorithmic defaulting, and O(1) upright/rotated/transformed resolution
- Arabic mark rendering: byte-packed mark-layout records, a parallel canonical-combining-class table, total algorithmic defaulting, and O(1) mark stacking with the AMTRA transient reordering
- Egyptian hieroglyph classification: byte-packed sign records, a parallel Unikemet metadata table, Gardiner/Core/function taxonomy, and O(1) hieroglyph and format-control resolution
- East Asian script data: byte-packed script records, a classification range table, a parallel UAX #60 metadata table, script/region/source taxonomy, and O(1) script and ancillary-property resolution
- Kernel text objects: bounded descriptors with explicit length, ownership and lifetime classes, borrowed versus copied buffers, untrusted-text admission, SUTF framing, and immutable published names
- Text input: deterministic key-event resolution, exact modifier matching, bounded dead-key composition, all-or-nothing SUTF-8 emission, and finite input context state

## Status

**SUAS-001 (SDF), SUAS-002 (SGW), SUAS-003 (SBR), SUAS-004 (SUCF), SUAS-005
(SSP), SUAS-006 (STS), SUAS-007 (SIS), SUAS-008 (SNCS) and SUAS-009 (SIP) are
ratified, and SUAS-010 (SSRE), SUAS-011 (SCMS), SUAS-012 (SUCD), SUAS-013
(SISR), SUAS-014 (SUVTL), SUAS-015 (SUAMR), SUAS-016 (SUKEMET), SUAS-017
(SUEASD), SUAS-018 (SKTO) and SUAS-019 (STIM) are under authoring.**
The SUAS series is contiguous - SUAS-001 through SUAS-019, with no reserved
gaps.
Additional SUAS rules and invariants are pending authoring.
