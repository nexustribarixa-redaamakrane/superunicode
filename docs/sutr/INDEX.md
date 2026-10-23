# SUTR — SuperUnicode Technical Reports

> Architecture Whitepapers & Foundations

This directory contains technical reports documenting architectural
rationale, design explorations, comparative analyses, and foundational
research supporting the SuperUnicode standard.

The SUTR series plays the role of Unicode's UAX/UTR documents: each report
formalizes one contract of the standard, and each is tracked per release.

## Document Index

| ID | Title | Status | Source |
|----|-------|--------|--------|
| [SUTR-0](SUTR-0-sucs-core.md) ([web](https://nexustribarixa-redaamakrane.github.io/superunicode/reports/SUTR-0.html)) | SUCS Core — hierarchy, three-space layout, SCP, Traps, Sentinel | 0.1.0 | Repo |
| [SUTR-1](SUTR-1-sutf.md) ([web](https://nexustribarixa-redaamakrane.github.io/superunicode/reports/SUTR-1.html)) | SUTF — the Character Encoding Forms: SUTF-8/16/4/2 and vSUTF | 0.1.0 | Repo |
| [SUTR-2](SUTR-2-suca.md) ([web](https://nexustribarixa-redaamakrane.github.io/superunicode/reports/SUTR-2.html)) | SUCA — SuperUnicode Collation Algorithm | 0.1.0 | Repo |
| [SUTR-3](SUTR-3-superunicode-storage.md) ([web](https://nexustribarixa-redaamakrane.github.io/superunicode/reports/SUTR-3.html)) | SuperUnicode Storage — the OWFS/USFS partition policy | 0.1.0 | Repo |
| [SUTR-4](SUTR-4-extsucs-transport.md) ([web](https://nexustribarixa-redaamakrane.github.io/superunicode/reports/SUTR-4.html)) | ExtSUCS Transport — vector, vSUTF and e-SUST framing | 0.1.0 | Repo |
| [SUTR-5](SUTR-5-plugin-lifecycle.md) ([web](https://nexustribarixa-redaamakrane.github.io/superunicode/reports/SUTR-5.html)) | Plugin Lifecycle & Blob Format — stage, checksum gate, mount, registry | 0.1.0 | Repo |
| [SUTR-6](SUTR-6-unicode-bridge.md) ([web](https://nexustribarixa-redaamakrane.github.io/superunicode/reports/SUTR-6.html)) | Unicode Compatibility Bridge — the permanent 1:1 guarantee | 0.1.0 | Repo |
| [SUTR-7](SUTR-7-sucem.md) ([web](https://nexustribarixa-redaamakrane.github.io/superunicode/reports/SUTR-7.html)) | SUCEM — the SuperUnicode Character Encoding Model: ACR → SUCS → SUTF → SUST | 0.1.0 | Repo |
| [SUTR-8](SUTR-8-sucpm.md) ([web](https://nexustribarixa-redaamakrane.github.io/superunicode/reports/SUTR-8.html)) | SUCPM — the SuperUnicode Character Property Model: tiers, dual lookup engine, bitmask packing | 0.1.0 | Repo |
| [SUTR-9](SUTR-9-sumnls.md) ([web](https://nexustribarixa-redaamakrane.github.io/superunicode/reports/SUTR-9.html)) | SUMNLS — SuperUnicode Mathematical Notation and Layout Support: math repertoire, O(1) math properties, kernel box-model layout engine | 0.1.0 | Repo |
| [SUTR-10](SUTR-10-sucm.md) ([web](https://nexustribarixa-redaamakrane.github.io/superunicode/reports/SUTR-10.html)) | SUCM — SuperUnicode Conformance Model: compliance tiers, zero-allocation mandate, automated verification gates | 0.1.0 | Repo |
| [SUTR-11](SUTR-11-sums.md) ([web](https://nexustribarixa-redaamakrane.github.io/superunicode/reports/SUTR-11.html)) | SUMS — SuperUnicode Mongolian Shaping & Vertical Direction: binary shaping schema, flattened FVS sequences, O(1) contextual forms, native vertical flow | 0.1.0 | Repo |
| [SUTR-012](SUTR-012-sucsl.md) ([web](https://nexustribarixa-redaamakrane.github.io/superunicode/reports/SUTR-012.html)) | SUCSL — SuperUnicode Cuneiform Sign Lists: packed sign records, compound composition, O(1) sign lookup, RNUR extension | 0.1.0 | Repo |
| [SUTR-013](SUTR-013-sueas.md) ([web](https://nexustribarixa-redaamakrane.github.io/superunicode/reports/SUTR-013.html)) | SUEAS — SuperUnicode East Asian Spacing: compiled East_Asian_Spacing/Width tables, O(1) width categories, contextual auto-spacing, zero-allocation grid engine | 0.1.0 | Repo |

## Core invariants every report honors

- `SUCS_CP` is 31-bit; `sucs_ex_char_t` is 64-bit.
- `0x000000–0x10FFFF` is permanently 1:1 with Unicode.
- The 16 terminal values `0x7FFFFFF0`–`0x7FFFFFFF` are unallocatable: 15
  trap slots, then the Sentinel at `0x7FFFFFFF`. The last allocatable
  codepoint is `0x7FFFFFEF`.
- Plugin codepoints live in the Native plane (`0x120000`–`0x7FFFFFEF`) or
  strictly above `0x7FFFFFFF`, and mount only after the checksum gate.

## SUCEM model

SUTR-7 defines SUCEM, the four-level model that the rest of the series
specializes:

```text
Level 1   ACR   Abstract Character Repertoire   — Unicode, RNUR, SCC directives
Level 2   CCS   Coded Character Set (SUCS)      — 31-bit, 0x00000000–0x7FFFFFFF
Level 3   CEF   Character Encoding Form (SUTF)  — endian-neutral code-unit sequences
Level 4   CES   Character Encoding Scheme (SUST) — byte order, alignment, framing
```

SUTF and SUST are separate modules because they answer different questions:
*if the processor byte order changed, would this format change?* No → CEF
(SUTF). Yes → CES (SUST). A fixed 32-bit form is `SUST-32`, not an SUTF form.

## Reference implementations

Six of the thirteen reports ship a freestanding C99 reference implementation.
Each is a translation unit in the `sutr_static` library, each is O(1) per
input unit, and each allocates nothing on any path:

| Report | Module | Header | Unit tests |
|--------|--------|--------|------------|
| SUTR-7 | `sutr_sucem` | `include/sutr/sutr_sucem.h` | `tests/test_sutr_sucem.c` |
| SUTR-8 | `sutr_sucpm` | `include/sutr/sutr_sucpm.h` | `tests/test_sutr_sucpm.c` |
| SUTR-9 | `sutr_sumnls` | `include/sutr/sutr_sumnls.h` | `tests/test_sutr_sumnls.c` |
| SUTR-10 | `sutr_sucm` | `include/sutr/sutr_sucm.h` | `tests/test_sutr_sucm.c` |
| SUTR-11 | `sutr_sums` | `include/sutr/sutr_sums.h` | `tests/test_sutr_sums.c` |
| SUTR-012 | `sutr_sucsl` | `include/sutr/sutr_sucsl.h` | `tests/test_sutr_sucsl.c` |
| SUTR-013 | `sutr_sueas` | `include/sutr/sutr_sueas.h` | `tests/test_sutr_sueas.c` |

The two that depend on a sibling read that sibling's packed mask rather than a
second copy of the data: SUMNLS classifies math atoms through the SUCPM mask,
so the two modules agree by construction.

## Related families

- [SUAS — Architecture Standards](../suas/INDEX.md) — core kernel invariants
  and conformance rules (SUAS-001…004, ratified).
- [SUTS — Technical Specifications](../suts/INDEX.md) — modular driver and
  extension specifications (SUTS-001, ratified).

## Status

**SUTR-0 through SUTR-013 are published, and all fourteen have markdown
sources in this directory**, in parity with SUAS and SUTS. Every report is
published at 0.1.0. The *web* column links
to the published page, which is now **generated from the document beside it** by
`website/pages/sutr.ps1`.

**The repository is authoritative, and the site is a build artefact of it.** The
generator reads each specification and emits the page, so the two cannot drift:
a change to a file here appears on the site at the next `website/dispatch.ps1`
run. Every generated page carries a provenance note naming its source file and
recording that the repository copy governs.

The previously hand-authored SUTR pages in `website/pages/reports.ps1` have been
deleted, so this directory is the only place the SUTR page content is written.
`docs/` itself is not deployed, so a relative `.md` link between specifications
is rewritten by the generator onto the published page that carries the same
content; an unmapped one degrades to plain text rather than shipping a 404.

The two website pages that used to disagree with the headers now agree, because
they were corrected and the SUTR page is generated. `standard/hierarchy.html`
previously stated a seven-level chain (`block`/`range` as address fields) that
`sucs_plane.h` does not define — it defines four bit-fields, Zone 7 / District 8
/ Plane 8 / Offset 8, with Block and Range as variable-length *semantic*
groupings. The SUTR-4 page previously described vSUTF as LEB128-style, where
`vsutf.h` specifies SUTF-8 for the Base range plus a fixed `0xFE` + 8-byte
escape above it. Both are recorded in SUTR-0 and SUTR-4 here, and both now reach
the site.
