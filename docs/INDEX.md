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
(SSP), SUAS-006 (STS), SUAS-007 (SIS), SUAS-008 (SNCS), SUAS-009 (SIP) and
SUTS-001 (SUCA) are ratified, and SUAS-010 (SSRE), SUAS-011 (SCMS), SUAS-012
(SUCD), SUAS-013 (SISR), SUAS-014 (SUVTL), SUAS-015 (SUAMR), SUAS-016 (SUKEMET),
SUTS-002 (SRE), SUTS-003 (SLDF), SUTS-004 (SIVD), SUTS-005 (SCSS), SUTS-006
(SIDN), SUTS-007 (SUEPS) and SUTS-008 (SUSCH) are under authoring.**
The SUAS series is contiguous — SUAS-001 through SUAS-016, with no reserved
gaps.
The SUTR series is
published — SUTR-012 at **1.0.0** and the earlier reports at **0.1.0**:
SUTR-0 through SUTR-012, covering SUCS Core, SUTF, SUCA, storage, ExtSUCS
transport, the plugin lifecycle, the Unicode compatibility bridge, SUCEM
(the four-level character encoding model), SUCPM (the character property
model), SUMNLS (mathematical notation and layout support), SUCM (the
conformance model), SUMS (Mongolian shaping and vertical direction), and
SUCSL (Cuneiform sign lists). Additional documents across all families are pending authoring. All
specifications are compatible with extSUCS (unbounded arbitrary-precision codepoint space).

The SUTR series has markdown sources in this tree; the website pages are
generated from them. See [sutr/INDEX.md](sutr/INDEX.md).
