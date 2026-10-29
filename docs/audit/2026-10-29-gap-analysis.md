# Gap Analysis — OpenWindows Kernel and Application Text Platform

- **Date:** 2026-10-29
- **Status:** Informative analysis; selects the three documents in the six-document pilot
- **Scope:** The existing SUAS, SUTS, and SUTR corpus, the OpenWindows kernel and
  OpenWindows-Essentials text paths, and the missing platform capabilities that
  applications built for OpenWindows would otherwise have to invent.
- **Evidence rule:** A capability is recorded as *covered* only when a document
  defines it **and** a normative chain reaches it. A gap is recorded as
  *document-worthy* only when no existing document owns the decision and the
  decision cannot be delegated to an existing module. A document's existence is
  never treated as implementation evidence.

---

## 1. Why this analysis exists

The six-document pilot began with three documents that were already selected:
SUAS-018 (SKTO), SUTS-012 (SCTS), and SUTR-014 (OKTP). Those three close the
most visible defects in the kernel text stack — unbounded strings, a console
that owns a private width table, and no adoption map.

They do not close the whole platform. An OpenWindows application consumes text
through four paths:

1. **Events in.** A keyboard, a compose key, a dead key, or an input method
   produces text.
2. **Text held.** The kernel carries bounded text objects (SKTO).
3. **Text placed.** A console or terminal puts glyphs into cells (SCTS).
4. **Text stored and compared.** Names, identifiers, files, and protocols.

The first path had no document at all. The third path had a placement model but
no terminal control model. And the corpus had no non-normative implementation
guidance family, so every integrator had to reconstruct the same adoption order
from fifteen reports.

This analysis screens candidate gaps, ranks them, and selects three documents —
two normative/optional technical documents and one informative technical note —
to complete the pilot without inflating the corpus.

The long-term target of roughly sixty documents is a **ceiling suggested by the
size of the Unicode standard**, not a quota. A document that does not own a
decision should not exist.

---

## 2. Current capability map

| Layer | Capability | Owner today | State |
|-------|------------|-------------|-------|
| Encoding | SUCS repertoire, SUTF forms, vSUTF, SUST | SUTR-0, SUTR-1, SUTR-4 | Covered |
| Character data | SUCD, SCMS, SUCEM, SUCPM, ideographic data | SUAS-012/011, SUTR-7/8, SUAS-009/013/016/017 | Covered |
| Text objects | length, capacity, ownership, admission, publication | SUAS-018 (SKTO) | Covered, pilot |
| Width and breaks | cell width, line-break opportunities | SUAS-002 (SGW), SUAS-003 (SBR) | Covered |
| Segmentation | grapheme, word, sentence boundaries | SUAS-006 (STS) | Covered |
| Direction | bidi resolution and reordering | SUAS-001 (SDF) | Covered |
| Normalization | canonical and compatibility forms | SUAS-004 (SUCF) | Covered |
| Names and identifiers | identifier classes, security skeletons | SUAS-007 (SIS), SUTS-005 (SCSS) | Covered |
| Console placement | cells, cursor, wrap, scroll, sinks | SUTS-012 (SCTS) | Covered, pilot |
| **Input** | **key event → text, dead keys, composition** | **none** | **Gap** |
| **Terminal control** | **C0/C1/CSI parsing, terminal state** | **none** | **Gap** |
| Application text layout | proportional rows, caret, hit testing | none | Gap, but large |
| Font selection | fallback, coverage, glyph choice | none | Gap, but not text semantics |
| Shaping | Arabic, Mongolian, Indic | SUAS-015, SUTR-11 (script-specific) | Covered per script |
| Storage | on-disk names and records | SUTR-3 | Covered |
| Collation, regex, locale, IDN, emoji | optional algorithms | SUTS-001/002/003/006/007 | Covered |
| Adoption guidance | which document binds which subsystem | SUTR-014 (OKTP) | Covered, pilot |
| **Implementation guidance** | **ordered evidence-producing checklist** | **none** | **Gap** |

---

## 3. Candidate gaps and screening

Each candidate is judged against three questions:

- **Does an existing document already own the decision?** If yes, the candidate
  is a duplicate and is rejected.
- **Can the decision be delegated to an existing module?** If yes, the candidate
  is an integration task, not a document.
- **Does the gap block an OpenWindows kernel or application path?** If no, it is
  deferred.

| # | Candidate gap | Owner today | Verdict |
|---|---------------|-------------|---------|
| C-1 | Key events become text; dead keys and composition | none | **Select** |
| C-2 | Terminal control sequences over an SCTS plane | SCTS defines placement only | **Select** |
| C-3 | Ordered implementation checklist with evidence | OKTP is a profile, not a procedure | **Select as SUTN** |
| C-4 | Proportional application text layout and caret | none | Defer; large, and no consumer ships it yet |
| C-5 | Font fallback and coverage selection | none | Defer; graphics contract, not character semantics |
| C-6 | Clipboard and drag-and-drop text negotiation | none | Defer; SUTS-012 sinks are the transport primitive |
| C-7 | Scrollback buffer format | SCTS §6 already fixes scrolling | Reject; storage policy belongs to SUTR-3 |
| C-8 | Keyboard layout data file format | C-1 owns the model, not the file | Fold into C-1 as an informative annex |
| C-9 | Window manager text drawing | SCTS + SDF own the decisions | Reject; compositor is out of scope |
| C-10 | Text search and replace | SUTS-001/002 already own comparison and matching | Reject; duplicate |
| C-11 | Input method framework | C-1 defines the composition contract | Fold into C-1; a full IME framework is future work |
| C-12 | Terminal colour and attribute model | SCTS already carries fg/bg attribute bits | Reject; no new decision |
| C-13 | Kernel logging text policy | SKTO already forbids SCP and unbounded text | Reject; policy is local |
| C-14 | Document template and validation workflow | no document owns the process | **Select as process deliverable** |
| C-15 | A note family for non-normative guidance | no family exists | **Select as SUTN-001’s family charter** |

### 3.1 Why C-1 is document-worthy

Input is not a rendering problem and not an encoding problem. It is a **state
machine that turns an event stream into text**. The decisions it makes are
observable and testable:

- whether a dead key is pending or committed;
- whether an unmatched dead key emits its spacing form or is discarded;
- whether a compose sequence is bounded;
- whether a key event can inject a System Control Plane value;
- whether the same event stream produces the same text on every run.

Those are architecture decisions. If each application answers them locally,
then the same physical key produces different text in the shell, the editor,
and the terminal. That is precisely the defect SKTO removes for text that is
already text; STIM removes it for text that is being produced.

**Decision:** create `SUAS-019 — System Text Input Model (STIM)` as a mandatory
architecture standard, conditional on the implementation accepting text input.
An implementation with no input device satisfies it trivially.

### 3.2 Why C-2 is document-worthy

SCTS deliberately stops at placement. It says a cell is a cell, a cursor is a
cursor, and a sink is a sink. It does **not** define how a byte stream becomes
cursor motion. That is correct separation: a console library should not carry a
terminal parser.

But OpenWindows ships `owterm`, and every application that talks to a serial
device or a pseudo-terminal needs the same parser. Without a specification,
each one invents a different subset of C0, CSI, and SCP handling, and each one
disagrees about what happens when a sequence is truncated or unsupported.

The decisions are testable: parameter bounds, partial-sequence buffering,
unsupported-sequence behavior, and the rule that an SCP directive never becomes
a cell. Those belong in a specification, and the implementation is small
because SCTS already owns the plane.

**Decision:** create `SUTS-013 — System Console Terminal Emulation (SCTE)` as
an optional technical specification. It is binding only on an implementation
that claims it.

### 3.3 Why C-3 belongs in a new family, not in SUAS or SUTS

The corpus has three families with three different forces. It has no place for
material that is **useful, ordered, and explicitly non-binding**. OKTP is close,
but it is a SUTR report about *adoption*; it is not a reusable implementation
procedure, and it is tied to one platform snapshot.

If that material were placed in SUAS, it would become mandatory. If it were
placed in SUTS, it would become binding on claim. Both outcomes would make
recommendations normative by accident. The correct response is a fourth family
with a clear charter: **SUTN — SuperUnicode Technical Notes**, informative,
non-normative, never a conformance condition.

**Decision:** establish SUTN and create `SUTN-001 — SuperUnicode Implementation
Checklist (SUIC)` as its first note. The note provides an ordered checklist and
an evidence table. It disclaims conformance explicitly and points every
requirement back to its normative source.

### 3.4 Why the deferred candidates are deferred

C-4 (application text layout) is the largest remaining gap, but it depends on a
consumer that does not exist. Writing it now would produce a specification with
no implementation evidence, which the corpus has so far avoided. It is the
first candidate for the next expansion.

C-5 (font fallback) is a graphics and asset policy. It needs a font format and a
coverage database contract before it can be specified without guessing.

C-6 (clipboard) is a transport negotiation. SCTS sinks already define the
transport primitive; a clipboard document can be written when an application
actually shares text.

C-7, C-9, C-10, and C-12 are rejected because an existing document already owns
the decision. Re-specifying them would create two authorities, which is the
failure SCTS §1.2 exists to prevent.

---

## 4. Dependency placement

The selected documents fit the existing dependency graph without a cycle:

```text
SUAS-019 (STIM)
  ├── SUAS-018 (SKTO)          text objects produced by input
  ├── SUAS-004 (SUCF)          composition output is normalized text
  ├── SUAS-007 (SIS)           identifier-producing input paths
  └── SUAS-010 (SSRE)          registration and version gates

SUTS-013 (SCTE)
  ├── SUTS-012 (SCTS)          the plane, cursor, and scroll model
  ├── SUAS-018 (SKTO)          bounded parameter and payload text
  ├── SUAS-003 (SBR)           line-feed placement is a break decision
  └── SUAS-010 (SSRE)          registration and version gates

SUTN-001 (SUIC)
  └── cites SUAS/SUTS/SUTR; creates no dependency edge in the other direction
```

A SUTN note never becomes a prerequisite for a normative document. If the
checklist discovers a missing requirement, the requirement is proposed as an
amendment to the owning SUAS or SUTS document; it is not added to the note.

---

## 5. Ranking

| Rank | Document | Blocking impact | Implementation size | Risk if omitted |
|------|----------|-----------------|---------------------|-----------------|
| 1 | SUAS-019 (STIM) | Applications disagree on what a key produces | Small state machine | Input text becomes platform-specific and untestable |
| 2 | SUTS-013 (SCTE) | Every terminal re-invents escape parsing | Small parser over SCTS | Incompatible terminals and partial-sequence defects |
| 3 | SUTN-001 (SUIC) | Integrators cannot order the work | Documentation only | Adoption remains guesswork; no evidence trail |

STIM ranks first because it is the only gap that changes *what text exists*.
SCTE ranks second because it changes *how existing text is presented*. SUIC
ranks third because it changes *how adopters prove the other two*.

---

## 6. Selected pilot slate

| Document | Family | Force | Purpose |
|----------|--------|-------|---------|
| SUAS-018 (SKTO) | SUAS | mandatory | bounded kernel text objects (already authored) |
| SUTS-012 (SCTS) | SUTS | optional, binding on claim | console placement (already authored) |
| SUTR-014 (OKTP) | SUTR | informative | OpenWindows adoption profile (already authored) |
| **SUAS-019 (STIM)** | SUAS | mandatory, conditional on input | event → text model |
| **SUTS-013 (SCTE)** | SUTS | optional, binding on claim | terminal control over SCTS |
| **SUTN-001 (SUIC)** | SUTN | informative, non-normative | implementation checklist |

After this slate the corpus contains **47 documents**: 19 SUAS, 12 SUTS, 15
SUTR, and 1 SUTN. The SUTS series retains its deliberate `SUTS-010` gap.

---

## 7. What this analysis does not claim

- It does not claim that OpenWindows implements any selected document. Only a
  linked translation unit and an executing test support that claim.
- It does not claim that 47 documents are complete. C-4 and C-5 remain open and
  are likely the next two documents.
- It does not create requirements. The two normative selections create
  requirements only through their own published text, and the SUTN note creates
  none at all.
- It does not assign identifiers. The identifiers `SUAS-019`, `SUTS-013`, and
  `SUTN-001` were verified free in the corpus before authoring and are recorded
  in the SSRE registry when the documents land.
