# SUTN — SuperUnicode Technical Notes

> Informative Implementation Guidance

This directory contains short technical notes for people implementing,
integrating, and reviewing SuperUnicode. A note explains an order of work,
records a practice, or collects evidence. It does not define a requirement.

**Binding force: informative and non-normative.** No SUTN note is required for
SuperUnicode conformance. Following a note does not establish conformance to
SuperUnicode, any SUAS standard, or any SUTS specification. Where a note and a
normative document disagree, the normative document governs. A note never
relaxes a SUAS invariant, never adds a condition to a SUTS claim, and never
silently redefines a SUAS, SUTS, or SUTR document.

**Missing requirements are amendments.** If a note discovers a requirement that
no normative document states, the correct action is to propose an amendment to
the owning SUAS or SUTS document. The requirement is not added to the note, and
the note does not become its source.

## Document Index

| ID | Title | Status |
|----|-------|--------|
| [SUTN-001](SUTN-001-superunicode-implementation-checklist.md) | SuperUnicode Implementation Checklist (SUIC) — ordered adoption stages, evidence table, OpenWindows annex | 0.1.0 - Published |

## Rules for a SUTN

1. **Stable identifier.** A note number is allocated once and never reused. A
   withdrawn note keeps its file and its number.
2. **Distinct purpose.** A note must not restate a normative rule as though the
   note were the source of the rule.
3. **Versioned references.** Every cited SUAS, SUTS, or SUTR document carries
   its identifier and, where the version matters, its version.
4. **Explicit non-normative status.** Every note contains the disclaimer in its
   first section, verbatim.
5. **Owner and maintainer.** Every note names a role, not an anonymous project.
6. **Revision history.** Every note carries a dated revision table.
7. **Lifecycle and review.** Every note states its review interval, change
   rule, and withdrawal rule.
8. **Platform labels.** Advice about one platform is labelled as such and is
   never generalized into a platform requirement.

## Relationship to the Other Families

| Family | Force | Relationship to a SUTN |
|--------|-------|------------------------|
| SUAS | normative, mandatory | a note may cite it; it may not soften or extend it |
| SUTS | optional, binding on claim | a note may cite the claim condition; it may not create a claim |
| SUTR | informative | a note may cite a report; it may not promote its advice |
| SUTN | informative, non-normative | no conformance force at all |

A SUTN note is not a prerequisite for a normative document. A normative
document may cite a note as informative background, but its requirements must
remain readable and testable without the note.

## How Notes Are Reviewed

A new note is accepted when it has a stable identifier, a distinct purpose, a
named owner, a revision table, and a lifecycle section. The documentation
validator (`cmake --build build --target verify-docs`) checks the mechanical
parts of that list. A human reviewer checks that the note does not restate or
reinterpret a normative document.

## Status

**SUTN-001 (SUIC) is published at 0.1.0.** Additional notes are added only when
they have a distinct purpose; the family has no target count.
