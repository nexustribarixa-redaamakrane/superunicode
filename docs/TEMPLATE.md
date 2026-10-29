# SuperUnicode Document Template

**Standard ID:** n/a (process document)
**Status:** 1.0.0 - Published
**Scope:** The required shape of every SUAS, SUTS, SUTR, and SUTN document, and the checks `verify-docs` performs
**Applies To:** every author, reviewer, and maintainer of the SuperUnicode corpus
**Last Updated:** 2026-10-29

---

## 1. Purpose

This template is the contract between a document and the repository's validator.
`cmake --build build --target verify-docs` reads every family document and
checks the shape described here. A document that does not match the shape
fails the build target, not a human review.

The template is itself a process document. It does not belong to any of the
four families and is not registered in SSRE.

---

## 2. File and identifier rules

1. **File name:** `<ID>-<slug>.md`, lower-case slug, hyphen-separated.
   - `SUAS-019-stim.md`
   - `SUTS-013-scte.md`
   - `SUTR-015-stl.md`
   - `SUTN-001-superunicode-implementation-checklist.md`
2. **Identifier:** the file-name prefix before the first hyphen, in the form
   `<FAMILY>-<number>` with a zero-padded number of at least three digits.
   SUTR keeps its historical one- and two-digit identifiers (`SUTR-0`,
   `SUTR-10`); new documents use three digits.
3. **One identifier, one document.** An identifier is never reused after a
   document is withdrawn. A withdrawn document keeps its file and gains a
   `Withdrawn` status.
4. **No gaps by accident.** A missing number is either a deliberate gap
   (`SUTS-010`) or an error. Deliberate gaps are recorded in the family index.
5. **In-family sequence:** the first heading is
   `# <ID> - <Title> (<ACRONYM>)`, with an ASCII hyphen and a single space on
   each side.

---

## 3. Required field block

Every family document begins with the heading, a blank line, and this block.
Fields are bold, use a colon, and appear in this order.

```markdown
# SUAS-019 - System Text Input Model (STIM)

**Standard ID:** SUAS-019
**Status:** Draft - Authoring
**Scope:** Core Kernel Invariant - event-to-text resolution
**Applies To:** ... comma-separated identifiers, subsystems, and repositories ...
**Last Updated:** 2026-10-29
```

Optional fields may follow `Last Updated`, never precede it:

- `**Version:**` — semantic version when it differs from the status.
- `**Family:**` — long family name, for documents that predate the index.
- `**Supersedes:**` / `**Superseded By:**` — identifier links.
- `**Reference Layer:**` — data roots such as `/Public/<version>/sucd/`.

### 3.1 Status vocabulary

The status must contain exactly one of these words:

| Status | Meaning | Allowed family |
|--------|---------|----------------|
| `Draft - Ratified` | reviewed and stable; changes need a new revision | SUAS, SUTS |
| `Draft - Authoring` | published for use, still open to editorial change | SUAS, SUTS |
| `0.1.0 - Published` | released informative text | SUTR, SUTN |
| `Withdrawn` | no longer maintained; retained for the record | any |

Do not invent intermediate words. `Proposed`, `Draft`, `Final`, and `RFC` are
not statuses in this corpus.

---

## 4. Body skeleton by family

All documents share sections 1–3 (Overview, Scope, Definitions or Terms). The
rest depends on the family. Section numbers may shift; the headings may not.

### 4.1 SUAS — mandatory architecture standard

```text
1. Overview                  what decision the document owns, and why
2. Scope                     what it defines and what it does not
3. Definitions               numbered terms (TD1, TD2, ...)
4. ... normative model ...   the decision, in enforceable language
...
N. Resource Constraints      freestanding C99, zero allocation, bounds
N. Implementation Evidence   informative; files and tests that exist today
N. Conformance Requirements  numbered MUST/MUST NOT list, one per testable rule
N. Open Questions            informative; explicitly unresolved
N. Terms and Conventions     acronyms and key-word usage
N. Testing                   what a conforming test must exercise
```

Every normative sentence uses **MUST**, **MUST NOT**, **SHOULD**, or **MAY** in
upper case. A sentence that merely explains uses lower case and is marked
informative where ambiguity is possible.

### 4.2 SUTS — optional specification, binding on claim

```text
1. Overview
2. Scope
3. Relationship to the Authoritative Standards   a deference table
4. ... model ...
N. Conformance Requirements   starts with the claim condition
N. Testing
N. Implementation Evidence    informative
N. Open Questions             informative
N. Terms and Conventions
```

The first paragraph after the heading states the claim condition in the form:

> An implementation that claims SUTS-013 conformance MUST satisfy every MUST
> in this document, and the claim is false if it does not.

### 4.3 SUTR — informative report

```text
1. Purpose
2. Method and Evidence Rules
3. ... findings ...
N. Non-Goals
N. Change Control
N. Terms and Conventions
Annex A. References
```

A SUTR never uses upper-case MUST for its own advice. It may quote a normative
sentence, in which case the source is cited in the same paragraph.

### 4.4 SUTN — informative technical note

A SUTN is a short, non-normative note. It is **not** a specification and it is
never a condition of conformance. The following sections are mandatory:

```text
1. Purpose and Non-Normative Status
2. Audience and Prerequisites
3. ... note content ...
N. Relationship to Normative Documents
N. Lifecycle and Review
N. Owner and Maintainer
N. Revision History
N. Terms and Conventions
```

The disclaimer in section 1 must contain, verbatim, both of these sentences:

> This note is non-normative. Following it does not establish conformance to
> SuperUnicode, any SUAS standard, or any SUTS specification.

A SUTN must also satisfy all of the following:

1. **Stable identifier.** The identifier is allocated once and never reused.
2. **Distinct purpose.** The note does not restate a normative rule as if it
   were the source of that rule.
3. **Versioned references.** Every cited SUAS/SUTS/SUTR document carries its
   identifier and, where a version matters, its version.
4. **No silent redefinition.** Where the note and a normative document differ,
   the normative document governs. A missing requirement is proposed as an
   amendment to the owning normative document, not added to the note.
5. **Owner and maintainer.** A named role, not an anonymous "the project".
6. **Revision history.** A table with date, version, and change.
7. **Lifecycle and review.** A review interval and a withdrawal rule.
8. **Platform labels.** OpenWindows-specific advice is labelled as such and is
   never generalized into a platform requirement.

---

## 5. Conformance section rules

A normative document ends its requirements with a numbered list. Each item:

- states one testable obligation;
- names the clause it comes from;
- uses MUST or MUST NOT;
- has a corresponding test in `tests/` or an explicit note that the test is
  pending.

Example:

```markdown
1. **[C1]** A conforming implementation MUST resolve a pending dead key before
   emitting any other text. (Section 6.2)
2. **[C2]** A conforming implementation MUST NOT emit a System Control Plane
   value as text. (Section 7.4)
```

The clause identifiers `[C1]`, `[C2]`, … are stable. Removing a clause retires
its identifier; it is never reassigned.

---

## 6. Revision history

Every document added after 2026-10-29 carries a revision table immediately
before the terms section:

```markdown
## Revision History

| Version | Date | Change |
|---------|------|--------|
| 0.1.0 | 2026-10-29 | Initial publication. |
```

The table is required for SUTN and recommended for every other family.

---

## 7. Writing rules

- **One decision per document.** If the scope sentence needs an "and" that
  joins two independent decisions, the document is probably two documents.
- **Cite, do not copy.** Reference the owning document by identifier and
  section. Do not reproduce a table that another document owns.
- **No forward requirements.** A document may describe a future capability but
  may not make it a condition of a present claim.
- **No implementation evidence in normative text.** Evidence goes in an
  informative section and names files and commands.
- **No conformance claim from existence.** A document never asserts that an
  implementation conforms because the document exists.
- **ASCII punctuation in identifiers and headings.** Em dashes are allowed in
  prose; identifiers, file names, and status words are ASCII.

---

## 8. Validator checklist

`cmake/verify_docs.cmake` enforces the mechanical rules:

| Check | Rule |
|-------|------|
| V1 | Each family directory has an `INDEX.md`. |
| V2 | The root index links to each family index. |
| V3 | Every family document file name matches `<ID>-<slug>.md`. |
| V4 | The first heading is `<ID> - <Title>`. |
| V5 | The `Standard ID` field equals the file-name identifier. |
| V6 | `Status`, `Scope`, `Applies To`, and `Last Updated` are present. |
| V7 | `Last Updated` is `YYYY-MM-DD`. |
| V8 | The family index contains a link to every document. |
| V9 | Every relative markdown link in an index resolves to a file. |
| V10 | No identifier appears twice in the corpus. |
| V11 | Every family document identifier appears in the SSRE registry. |
| V12 | Every registry identifier has a family document. |
| V13 | Every SUTN contains the non-normative disclaimer. |
| V14 | Every SUTN contains the lifecycle, owner, and revision sections. |

For older documents, V4 and V6 also accept an en dash or em dash where the
template shows an ASCII hyphen, and V14 accepts a numeric section prefix (for
example `## 11. Relationship to Normative Documents`). New documents should
still use the ASCII forms shown in section 2 and section 4.

A change to this table is a change to the template and requires the validator
to be updated in the same commit.
