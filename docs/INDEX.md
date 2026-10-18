# SuperUnicode Documentation

This directory contains the formal specification and documentation tree
for the SuperUnicode project.

## Document Families

| Prefix | Full Name | Purpose |
|--------|-----------|---------|
| **SUAS** | SuperUnicode Architecture Standards | Core kernel invariants and conformance rules |
| **SUTS** | SuperUnicode Technical Specifications | Modular driver and extension specifications |
| **SUTR** | SuperUnicode Technical Reports | Architecture whitepapers and foundational research |

## Navigation

- [SUAS — Architecture Standards](suas/INDEX.md)
- [SUTS — Technical Specifications](suts/INDEX.md)
- [SUTR — Technical Reports](sutr/INDEX.md)

## Status

**SUAS-001 (SDF), SUAS-002 (SGW), SUAS-003 (SBR), SUAS-004 (SUCF), SUAS-005
(SSP), SUTS-001 (SUCA) are ratified, and SUTS-002 (SRE) is under authoring.**
The SUTR series is
published at **0.1.0**:
SUTR-0 through SUTR-9, covering SUCS Core, SUTF, SUCA, storage, ExtSUCS
transport, the plugin lifecycle, the Unicode compatibility bridge, SUCEM
(the four-level character encoding model), SUCPM (the character property
model), and SUMNLS (mathematical notation and layout support). Additional documents across all families are pending authoring. All
specifications are compatible with extSUCS (unbounded 64-bit codepoint space).

The SUTR series has markdown sources in this tree; the website pages are
generated from them. See [sutr/INDEX.md](sutr/INDEX.md).
