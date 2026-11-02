# SUTN-003 - Document Selection Guide

**Standard ID:** SUTN-003
**Status:** 0.1.0 - Published
**Scope:** Informative guidance for choosing SUAS, SUTS, SUTR, and SUTN documents
**Applies To:** implementers, integrators, reviewers, and architecture planners
**Last Updated:** 2026-11-02

## 1. Purpose and Non-Normative Status

This note is non-normative. Following it does not establish conformance to
SuperUnicode, any SUAS standard, or any SUTS specification.

Use this note to decide which family owns a proposed piece of work. It is a
triage aid, not a substitute for reading the family documents.

## 2. Audience and Prerequisites

Readers should know the [family hierarchy](../INDEX.md) and identify the
decision they need to document before applying this guide.

## 3. Decision Tree

1. If the decision is a mandatory kernel invariant, start with **SUAS**.
2. If it is an optional interface or algorithm with a claimable contract, use
   **SUTS**.
3. If it records rationale, comparison, or implementation research, use
   **SUTR**.
4. If it records workflow, review practice, or adoption advice, use **SUTN**.
5. If no document owns the decision, propose an amendment or a new document;
   do not hide the requirement in a note.

## 4. Examples

Character database authority belongs with SUAS-012 and the SUCHA architecture
surface. Terminal parsing belongs with SUTS-013. OpenWindows adoption analysis
belongs with SUTR-014. A release checklist belongs with SUTN.

## 5. Relationship to Normative Documents

The [SUAS](../suas/INDEX.md), [SUTS](../suts/INDEX.md), and
[SUTR](../sutr/INDEX.md) indexes govern classification and status.

## 6. Lifecycle and Review

Review every 12 months and when a new family or governance rule is introduced.
Withdraw the note if its decision tree duplicates the family indexes.

## 7. Owner and Maintainer

Owner: Standards Architecture Maintainer. Maintainer: SuperUnicode
documentation maintainers.

## 8. Revision History

| Version | Date | Change |
|---|---|---|
| 0.1.0 | 2026-11-02 | Initial publication. |

## 9. Terms and Conventions

“Owns” means defines the decision; it does not necessarily mean that the
SuperUnicode repository supplies the only implementation.
