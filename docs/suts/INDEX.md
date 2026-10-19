# SUTS — SuperUnicode Technical Specifications

> Modular Drivers & Extensions

This directory contains the technical specifications for individual
SuperUnicode modules, drivers, and extensions. Each SUTS document defines
the public interface, behavioral contract, and implementation requirements
for a discrete subsystem.

## Document Index

| ID | Title | Status |
|----|-------|--------|
| [SUTS-001](SUTS-001-suca.md) | SuperUnicode Collation Algorithm (SUCA) — UCA-equivalent Multilevel Collation | Draft — Ratified |
| [SUTS-002](SUTS-002-sre.md) | SuperUnicode Regular Expressions (SRE) — UTS #18-analog Regex Engine | Draft — Authoring |
| [SUTS-003](SUTS-003-sldf.md) | System Locale Data & Formatting (SLDF) — binary-compiled, zero-allocation locale schema | Draft — Authoring |
| [SUTS-004](SUTS-004-sivd.md) | System Ideographic Variation Database (SIVD) — binary-compiled, zero-allocation variation sequences | Draft — Authoring |
| [SUTS-005](SUTS-005-scss.md) | System Character Security Specification (SCSS) — binary-compiled, zero-allocation homograph detection, identifier admissibility, and script isolation | Draft — Authoring |

## Scope

- Module interface specifications
- Driver behavioral contracts
- Extension point definitions
- API stability and versioning policies
- Collation, normalization, and searching algorithms

## Status

**SUTS-001 (SUCA) is ratified; SUTS-002 (SRE), SUTS-003 (SLDF), SUTS-004
(SIVD) and SUTS-005 (SCSS) are under authoring.** Additional SUTS specifications and algorithms are
pending authoring. All SUTSs are compatible with extSUCS (unbounded 64-bit
codepoint space).
