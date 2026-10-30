# SuperUnicode Documentation

This directory contains the formal specification and documentation tree
for the SuperUnicode project.

## Document Families

| Prefix | Full Name | Purpose | Binding force |
|--------|-----------|---------|---------------|
| **SUAS** | SuperUnicode Architecture Standards | Core kernel invariants and conformance rules | **Normative - mandatory** for SuperUnicode conformance |
| **SUTS** | SuperUnicode Technical Specifications | Modular driver and extension specifications | **Optional** - not required for SuperUnicode conformance; binding only once an implementation claims that specification |
| **SUTR** | SuperUnicode Technical Reports | Architecture whitepapers and foundational research | **Informative** - no conformance requirements |
| **SUTN** | SuperUnicode Technical Notes | Implementation guidance, checklists, and evidence practice | **Informative and non-normative** - never a conformance condition; following a note does not establish conformance |

### Binding hierarchy

1. **SUAS governs.** Every MUST or MUST NOT in a SUAS document is a condition
   of SuperUnicode conformance.
2. **SUTS binds on claim.** A SUTS specification is optional, but once an
   implementation claims it, its MUSTs become conditions of that claim. A SUTS
   claim never relaxes a SUAS invariant.
3. **SUTR informs.** A report records rationale, findings, and profiles. It
   creates no obligation.
4. **SUTN advises.** A note records an order of work or a practice. It creates
   no obligation and is never a prerequisite for a normative document. A
   missing requirement discovered in a note is proposed as an amendment to the
   owning SUAS or SUTS document, not added to the note.

Where two documents disagree, the higher-ranked family governs: SUAS over SUTS,
SUTS over SUTR, and every normative family over SUTN.

## Navigation

- [SUAS — Architecture Standards](suas/INDEX.md)
- [SUTS — Technical Specifications](suts/INDEX.md)
- [SUTR — Technical Reports](sutr/INDEX.md)
- [SUTN — Technical Notes](sutn/INDEX.md)
- [Document template](TEMPLATE.md)
- [Gap analysis, 2026-10-29](audit/2026-10-29-gap-analysis.md)
- [Pilot implementation audit and roadmap, 2026-10-30](audit/2026-10-30-pilot-implementation-audit.md)

## Status

**The corpus contains 47 documents: 19 SUAS, 12 SUTS, 15 SUTR, and 1 SUTN.**

- **SUAS-001 (SDF), SUAS-002 (SGW), SUAS-003 (SBR), SUAS-004 (SUCF), SUAS-005
  (SSP), SUAS-006 (STS), SUAS-007 (SIS), SUAS-008 (SNCS) and SUAS-009 (SIP) are
  ratified. SUAS-010 (SSRE), SUAS-011 (SCMS), SUAS-012 (SUCD), SUAS-013 (SISR),
  SUAS-014 (SUVTL), SUAS-015 (SUAMR), SUAS-016 (SUKEMET), SUAS-017 (SUEASD),
  SUAS-018 (SKTO) and SUAS-019 (STIM) are under authoring.**
- The SUAS series is contiguous - SUAS-001 through SUAS-019, with no reserved
  gaps.
- **SUTS-001 (SUCA) is ratified. SUTS-002 (SRE), SUTS-003 (SLDF), SUTS-004
  (SIVD), SUTS-005 (SCSS), SUTS-006 (SIDN), SUTS-007 (SUEPS), SUTS-008 (SUSCH),
  SUTS-009 (SULDF), SUTS-011 (SUSN), SUTS-012 (SCTS) and SUTS-013 (SCTE) are
  under authoring.** SUTS-010 remains a deliberate gap.
- **The SUTR series is published at 0.1.0:** SUTR-0 through SUTR-014, covering
  SUCS Core, SUTF, SUCA, storage, ExtSUCS transport, the plugin lifecycle, the
  Unicode compatibility bridge, SUCEM, SUCPM, SUMNLS, SUCM, SUMS, SUCSL, SUEAS,
  and OKTP (the OpenWindows kernel text integration profile).
- **The SUTN series is new:** SUTN-001 (SUIC) is published at 0.1.0 as the first
  informative technical note.

Additional documents are added only when they own a decision that no existing
document owns. The size of the Unicode standard is a useful ceiling, not a
quota. All specifications are compatible with extSUCS (unbounded
arbitrary-precision codepoint space).

## Verification

The documentation validator checks the mechanical rules in
[TEMPLATE.md](TEMPLATE.md): family indexes, file-name and identifier agreement,
required fields, index links, duplicate identifiers, SUTN disclaimers, and
one-to-one coverage between the corpus and the SSRE registry.

```sh
cmake --build build --target verify-docs
```

A passing validator proves shape and cross-reference integrity. It does not
prove that any implementation conforms to a document.

The SUTR series has markdown sources in this tree; the website pages are
generated from them. See [sutr/INDEX.md](sutr/INDEX.md).
