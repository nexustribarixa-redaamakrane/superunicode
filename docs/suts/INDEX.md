# SUTS — SuperUnicode Technical Specifications

> Modular Drivers & Extensions

This directory contains the technical specifications for individual
SuperUnicode modules, drivers, and extensions. Each SUTS document defines
the public interface, behavioral contract, and implementation requirements
for a discrete subsystem.

**Binding force: optional, then binding on claim.** No SUTS document is
required for SuperUnicode conformance; an implementation may omit any or all
of them. However, once an implementation *claims* a SUTS specification, every
MUST in that document becomes a condition of that claim, and the claim is
false if it fails. SUTS conformance is therefore additive: it never relaxes a
SUAS invariant.

## Document Index

| ID | Title | Status |
|----|-------|--------|
| [SUTS-001](SUTS-001-suca.md) | SuperUnicode Collation Algorithm (SUCA) — UCA-equivalent Multilevel Collation | Draft — Ratified |
| [SUTS-002](SUTS-002-sre.md) | SuperUnicode Regular Expressions (SRE) — UTS #18-analog Regex Engine | Draft — Authoring |
| [SUTS-003](SUTS-003-sldf.md) | System Locale Data & Formatting (SLDF) — binary-compiled, zero-allocation locale schema | Draft — Authoring |
| [SUTS-004](SUTS-004-sivd.md) | System Ideographic Variation Database (SIVD) — binary-compiled, zero-allocation variation sequences | Draft — Authoring |
| [SUTS-005](SUTS-005-scss.md) | System Character Security Specification (SCSS) — binary-compiled, zero-allocation homograph detection, identifier admissibility, and script isolation | Draft — Authoring |
| [SUTS-006](SUTS-006-sidn.md) | System Internationalized Domain & Identifier Network Architecture (SIDN) — single-pass, zero-allocation label mapping, validation, and canonical routing (a UTS #46 analog) | Draft — Authoring |
| [SUTS-007](SUTS-007-sueps.md) | SuperUnicode Emoji & Presentation Specification (SUEPS) — zero-allocation, O(1) emoji presentation and flattened ZWJ / modifier sequence resolution (a UTS #51 analog) | Draft — Authoring |
| [SUTS-008](SUTS-008-susch.md) | SuperUnicode Source Code Handling (SUSCH) — zero-allocation, single-pass source code safety attributes, identifier validation, bidi / invisible elimination, homoglyph risk, and identifier chunk analysis (a UTS #55 analog) | Draft — Authoring |
| [SUTS-009](SUTS-009-suldf.md) | SuperUnicode Link Detection & Formatting (SULDF) — zero-allocation, byte-packed, O(1) UTS #58 link detection, `Link_Term` / `Link_Bracket` / `Link_Email` attributes, termination, and minimal escaping | Draft — Authoring |
| [SUTS-011](SUTS-011-susn.md) | SuperUnicode Set Notation (SUSN) - binary-mapped character-set algebra, inversion lists, range-filtered O(1) membership, RNUR neography integration (UTS #61 analog) | Draft - Authoring |
| [SUTS-012](SUTS-012-scts.md) | System Console Text Service (SCTS) - text plane, console sinks, grid/cursor/wrap/scroll, authoritative SUAS width and segmentation, glyph-zone registry | Draft - Authoring |
| [SUTS-013](SUTS-013-scte.md) | System Console Terminal Emulation (SCTE) - SUTF-8 terminal parser, C0/ESC/CSI/OSC subsets, bounded partial-sequence state, SCP directive routing over an SCTS plane | Draft - Authoring |

## Scope

- Module interface specifications
- Driver behavioral contracts
- Extension point definitions
- API stability and versioning policies
- Collation, normalization, and searching algorithms

## Status

**SUTS-001 (SUCA) is ratified; SUTS-002 (SRE), SUTS-003 (SLDF), SUTS-004
(SIVD), SUTS-005 (SCSS), SUTS-006 (SIDN), SUTS-007 (SUEPS), SUTS-008
(SUSCH), SUTS-009 (SULDF), SUTS-011 (SUSN), SUTS-012 (SCTS) and SUTS-013
(SCTE) are under authoring.** SUTS-010 remains a deliberate gap. Additional
SUTS specifications and algorithms are
pending authoring. All SUTSs are compatible with extSUCS (unbounded
arbitrary-precision codepoint space).
