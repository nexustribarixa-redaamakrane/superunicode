# SUTN-001 - SuperUnicode Implementation Checklist (SUIC)

**Standard ID:** SUTN-001
**Status:** 0.1.0 - Published
**Scope:** An informative, ordered checklist for adopting the SuperUnicode corpus in a kernel or application platform, with an evidence table and an OpenWindows-labelled annex
**Applies To:** implementers, integrators, reviewers, and release engineers adopting SUAS, SUTS, or SUTR documents; the OpenWindows annex applies only to OpenWindows
**Last Updated:** 2026-10-29

---

## 1. Purpose and Non-Normative Status

This note is non-normative. Following it does not establish conformance to
SuperUnicode, any SUAS standard, or any SUTS specification.

Its purpose is to turn the corpus into an order of work. The corpus names many
decisions; it does not say which one to land first, what evidence to collect, or
when an integrator may honestly write "adopted" in a status report. This note
offers an order and an evidence format. It creates no obligation, grants no
conformance, and never substitutes for the document it cites.

If this note and a normative document disagree, the normative document governs
without qualification. If this note describes a requirement that a normative
document does not contain, the requirement does not exist. The correct response
is to propose an amendment to the owning SUAS or SUTS document, not to treat
the note as the source.

---

## 2. Audience and Prerequisites

The checklist is written for four readers:

- **Platform integrators** deciding which documents a kernel, a console, or an
  application runtime should adopt.
- **Module authors** implementing one document and needing to know what it
  depends on.
- **Reviewers** checking whether a claimed adoption has evidence.
- **Release engineers** deciding whether a release note may mention a document.

Prerequisites:

1. The reader has the four family indexes open: `docs/suas/INDEX.md`,
   `docs/suts/INDEX.md`, `docs/sutr/INDEX.md`, and `docs/sutn/INDEX.md`.
2. The reader accepts the binding hierarchy in `docs/INDEX.md`: SUAS is
   mandatory, SUTS is optional and binding on claim, SUTR and SUTN are
   informative.
3. The reader can build and run the repository's tests. A claim without an
   executing test is a plan, not evidence.

---

## 3. How to Use This Note

Work top to bottom. A stage may be skipped only when the platform genuinely
lacks the capability the stage governs, and the skip is recorded in the
evidence table with that reason.

Each stage has four parts:

- **Adopt** — the documents the stage depends on.
- **Prove** — the evidence that the stage landed.
- **Do not** — the shortcuts that produce a false claim.
- **Exit** — the condition for moving on.

Use the evidence table in section 9. Copy it into the platform's own repository
and fill one row per document.

---

## 4. Stage 0 - Inventory

**Adopt:** none.

**Do:**

1. List every place the platform turns bytes into text or text into bytes.
2. Record, for each place, whether it allocates, whether it depends on a NUL
   terminator, and whether it crosses a trust boundary.
3. Record the byte encodings actually in use, including any legacy ASCII path.

**Prove:** a table of call sites with file paths and a one-line description.

**Do not:** count a header that exists but is not on a build path as adopted.
A document's presence is not evidence.

**Exit:** every text path is named, including the ones that will be replaced.

---

## 5. Stage 1 - Encoding and repertoire

**Adopt:** SUTR-0 (SUCS Core), SUTR-1 (SUTF), SUTR-7 (SUCEM), and the SUCD /
SCMS documents that supply character data (SUAS-011, SUAS-012).

**Do:**

1. Link one SUTF form and prove encode, decode, and validate.
2. Prove that the SCP range is rejected as content and accepted only as a
   directive.
3. Prove that surrogates, traps, and the Sentinel are rejected.
4. Record which form is the interior form and which forms are boundary-only.

**Prove:** a round-trip test over the admissible range and a rejection test over
each forbidden range.

**Do not:** treat "the codec exists in the tree" as adoption. The build must
name the translation unit and a test must execute it.

**Exit:** one interior form, one boundary form, and a rejection test for every
forbidden range.

---

## 6. Stage 2 - Bounded text

**Adopt:** SUAS-018 (SKTO).

**Do:**

1. Replace length-free kernel string entry points with text objects that carry
   length, capacity, ownership, and flags.
2. Route every trust-boundary copy through the admission procedure.
3. Replace in-place name mutation with publication and generation.
4. Prove that no path computes length by scanning for a terminator.

**Prove:** tests for terminator independence, capacity refusal, admission
failure, publication immutability, and resolve-then-use stability.

**Do not:** add a text object type but keep the old `char *` path beside it and
call the old path "temporary". A second path becomes the permanent path.

**Exit:** every kernel text entry point takes an explicit length, and names are
published rather than edited.

---

## 7. Stage 3 - Names and identity

**Adopt:** SUAS-004 (SUCF), SUAS-007 (SIS), and SUTS-005 (SCSS), in that order.

**Do:**

1. Apply normalization before comparison.
2. Classify identifiers with the SIS classes rather than a local `isalnum`.
3. Apply the security skeleton before admitting a name into a shared namespace.

**Prove:** canonical-equivalence tests, identifier-class tests, and a confusable
pair that the security layer rejects.

**Do not:** normalize at display time only. Comparison is the operation that
decides identity.

**Exit:** two canonically equivalent names cannot both exist in one namespace,
and a confusable pair is either rejected or reported.

---

## 8. Stage 4 - Presentation and input

**Adopt:** SUAS-002 (SGW), SUAS-003 (SBR), SUAS-006 (STS), SUTS-012 (SCTS),
SUAS-019 (STIM), and SUTS-013 (SCTE), as the platform's surfaces require.

**Do:**

1. Replace every private width table with SGW.
2. Replace every private wrap heuristic with SBR.
3. Route cluster boundaries through STS.
4. Build the console on SCTS and declare its sink level honestly.
5. Route key events through STIM; do not let an application own a dead key.
6. If the platform ships a terminal, parse controls with SCTE and keep SCP
   directives out of cells.

**Prove:** a width test that differs from the old table, a break test at a
forbidden point, a combining-mark placement test, a dead-key composition test,
and a truncated-control-sequence test.

**Do not:** let a console own a second width table "for speed". A cache is
permitted only when it is derived from SGW and invalidated with it.

**Exit:** the console and the input path each have one authority, and both
authorities are named in the build.

---

## 9. Evidence Table

Copy this table into the adopting repository. One row per document. A blank
evidence cell means the row is a plan, not an adoption.

| Document | Version | Status | Build entry | Test | Evidence owner | Date |
|----------|---------|--------|-------------|------|----------------|------|
| SUTR-0 | 0.1.0 | | | | | |
| SUTR-1 | 0.1.0 | | | | | |
| SUAS-018 | unversioned | | | | | |
| SUAS-019 | unversioned | | | | | |
| SUTS-012 | unversioned | | | | | |
| SUTS-013 | unversioned | | | | | |
| … | | | | | | |

Status vocabulary for the table:

| Status | Evidence required |
|--------|-------------------|
| **Absent** | no file and no build entry |
| **Present** | file exists, not on the build path |
| **Linked** | build entry plus a public header on the include path |
| **Tested** | a named test executes and passes |
| **Claimed** | the platform publishes the claim, with Linked and Tested rows |

A document may be **Claimed** only when its own conformance section is
satisfied. This table does not define conformance; it records evidence.

---

## 10. OpenWindows Annex (Platform-Specific, Informative)

This annex is labelled because it is about one platform. It is not a
SuperUnicode requirement, and a different platform should not inherit it.

### 10.1 Recommended order for OpenWindows

1. **KT-0 → KT-1.** Adopt SUTR-0, SUTR-1, and SUAS-018 in the kernel. The
   observable defect to remove first is terminator-dependent length in
   `lib/kstring.c`, `lib/kprintf.c`, `vfs/vfs.c`, and `core/alpc.c`.
2. **Name safety.** Adopt SUAS-004, SUAS-007, and SUTS-005 at the VFS and ALPC
   boundaries.
3. **Console authority.** Adopt SUTS-012 in `DLL/suct` and `owterm`, replacing
   the private width table and wrap heuristic.
4. **Input authority.** Adopt SUAS-019 in the keyboard path before two
   applications disagree about dead keys.
5. **Terminal control.** Adopt SUTS-013 in `owterm` if it continues to accept
   escape sequences.
6. **Optional algorithms.** Adopt SUCA, SRE, SLDF, SIDN, and SUEPS only when a
   consumer exists.

### 10.2 OpenWindows evidence shortcuts that do not count

- A vendored corpus directory without a build entry.
- A passing host test that exercises a different repository.
- A console that is "close enough" while keeping a private width table.
- A keyboard path that maps scan codes directly to ASCII and calls the result
  Unicode input.
- A terminal that implements a different CSI subset and calls it compatible.

### 10.3 What this annex does not do

It does not assert that OpenWindows has adopted any document. It does not
override SUTR-014 (OKTP); where the two differ, OKTP carries the adoption
matrix and this annex carries the checklist. Neither is normative.

---

## 11. Relationship to Normative Documents

| Family | Force | How this note treats it |
|--------|-------|-------------------------|
| SUAS | mandatory | cites the clause; never softens or extends it |
| SUTS | optional, binding on claim | cites the claim condition; never treats a claim as automatic |
| SUTR | informative | cites the report; never promotes its advice to a requirement |
| SUTN | informative, non-normative | this family; no conformance force at all |

Rules for this note:

1. Every quoted requirement carries its source identifier and section.
2. A paraphrase is labelled as a paraphrase.
3. A missing requirement is proposed to the owning document as an amendment.
   It is not added here.
4. A normative document change is reflected by re-deriving the checklist, not
   by editing the note to preserve an old conclusion.
5. This note never becomes a prerequisite for a normative document.

---

## 12. Lifecycle and Review

- **Review interval:** every twelve months, or sooner when a cited document
  changes a claim condition or a conformance clause.
- **Review owner:** the maintainer named in section 13, with one reviewer from
  the platform that last adopted a cited document.
- **Change rule:** editorial changes increment the patch version. A change to
  the checklist order or the evidence vocabulary increments the minor version.
- **Withdrawal:** the note is withdrawn when the corpus publishes a normative
  conformance procedure that makes the checklist redundant. Withdrawal keeps
  the file and changes the status to `Withdrawn`; the identifier is never
  reused.
- **Stale-note rule:** if a cited document is superseded, the note is updated in
  the same release that publishes the replacement. A note that cites a
  withdrawn document is itself marked stale until reviewed.

---

## 13. Owner and Maintainer

- **Owner:** SuperUnicode documentation maintainers.
- **Maintainer role:** the documentation lead for the SUTN family.
- **Reviewers:** one maintainer from each family touched by a change.
- **Contact:** the repository issue tracker, labelled `docs` and `sutn`.

The owner is a role, not a person, so the note survives a change of personnel.
A named individual may be recorded in the revision table, but the role is the
durable owner.

---

## 14. Revision History

| Version | Date | Change |
|---------|------|--------|
| 0.1.0 | 2026-10-29 | Initial publication: stages 0–4, evidence table, OpenWindows annex, lifecycle rules. |

---

## 15. Terms and Conventions

- **SUIC** is this note, SUTN-001.
- **SUTN** is the SuperUnicode Technical Notes family: informative,
  non-normative, and never a conformance condition.
- **SKTO**, **STIM**, **SCTS**, **SCTE**, **SGW**, **SBR**, **STS**, **SIS**,
  **SUCF**, **SCSS**, and **OKTP** are the documents named in their citations.
- **KT-0**, **KT-1**, and **KT-2** are the informative profile levels of
  SUTR-014 (OKTP).
- **Linked**, **Tested**, and **Claimed** are evidence labels from section 9,
  not conformance levels.
- The words **must**, **should**, and **may** in this note are advisory. Where a
  normative sentence is quoted, its source document supplies the force.
