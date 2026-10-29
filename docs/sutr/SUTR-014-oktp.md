# SUTR-014 - OpenWindows Kernel Text Integration Profile (OKTP)

**Standard ID:** SUTR-014
**Status:** 0.1.0 - Published
**Scope:** An informative profile of the SuperUnicode documents that the OpenWindows kernel and OpenWindows-Essentials text paths should adopt, the order in which they should adopt them, and the evidence that shows what is actually implemented
**Applies To:** the OpenWindows microkernel, OpenWindows-Essentials, and any reader trying to determine which SuperUnicode requirements bind a given subsystem
**Last Updated:** 2026-10-28

---

## 1. Purpose

This report answers one question:

> Given the SuperUnicode corpus and the OpenWindows tree as they exist today,
> which documents should each text-handling subsystem adopt, and how would
> anyone know whether it did?

It is a **profile**, not a standard. It creates no requirement. Every
obligation it discusses comes from a SUAS or SUTS document, and where this
report and a normative document disagree, the normative document governs.

The profile exists because the corpus had grown to nineteen architecture
standards, twelve technical specifications, fifteen reports, and one
implementation note, while the kernel linked exactly two SuperUnicode
translation units. "Adopt SuperUnicode" had stopped being an actionable
instruction. This report turns it into a matrix and a set of gates.

### 1.1 Binding force

| Family | Force | Effect here |
|--------|-------|-------------|
| SUAS | normative, mandatory | every MUST binds any SuperUnicode-conforming implementation |
| SUTS | optional, binding on claim | binds only an implementation that claims that specification |
| SUTR | informative | this report; binds nothing |
| SUTN | informative, non-normative | implementation guidance; binds nothing and establishes no conformance |

The report uses **should** and **recommended** for its own advice, and
**must** only when quoting a normative document, in which case the citation is
given.

---

## 2. Method and Evidence Rules

The profile classifies each subsystem on two axes.

**Adoption status**

| Status | Meaning |
|--------|---------|
| **Linked** | the subsystem's build actually compiles and links the SuperUnicode translation unit |
| **Present** | the translation unit or header exists in the tree but is not on the subsystem's build path |
| **Specified** | the governing document exists but no implementation exists |
| **Absent** | neither document nor implementation exists |

**Adoption tier**

| Tier | Meaning |
|------|---------|
| **T0 - Adopt now** | the requirement is already violated by observable code, or the subsystem cannot be made safe without it |
| **T1 - Adopt with the feature** | the requirement becomes binding when a specific feature is enabled (BiDi UI, vertical text, IDN, …) |
| **T2 - Deferred** | useful, not blocking, and no consumer exists |
| **N/A** | out of scope for this kernel |

A component is recorded as **Linked** only when a build file names the
translation unit, the public header is on the include path, and a test
executes. A document's existence never upgrades a status. A passing test
proves the behaviour that test checks, nothing more.

SUTN notes are not cited in the matrix. They create no obligation to adopt, so
a row that pointed at one would blur the line between guidance and a claim.
Where a note describes an implementation of a SUAS or SUTS requirement, the
matrix cites the normative document instead.

---

## 3. The Current Text Stack

The table records what the two repositories contain at the time of writing.
It is evidence, not a conformance claim.

| Layer | Repository | File | Current state |
|-------|------------|------|---------------|
| SUCS mode | OpenWindows | `lib/sucs.c`, `sucs/sucs_mode.o` | Linked; Base/Extended switch with a deferred-commit path |
| SUTF-8 codec | OpenWindows | `sutf/src/sutf8.c`, `sucs/sutf8.o` | Linked |
| SUTF-16/4/2, vSUTF | superunicode | `sutf/` | Present; not linked into the kernel |
| SUAS/SUTS/SUTR engines | superunicode | `src/suas/`, `src/suts/`, `src/sutr/` | Present; none linked into the kernel |
| Kernel strings | OpenWindows | `lib/kstring.c` | NUL-terminated `char *`; no length, no capacity |
| Kernel formatting | OpenWindows | `lib/kprintf.c` | Byte sink; `%s` walks to `'\0'` |
| Shell | OpenWindows | `shell/shell.c` | Fixed 512/256/128-byte buffers |
| VFS names | OpenWindows | `vfs/vfs.c` | `ow_strcmp`, fixed-width copy at 223 bytes |
| ALPC names | OpenWindows | `core/alpc.c` | `char *` port names |
| Image names | OpenWindows | `ps/owx_loader.c` | Scans to `'.'` or `'\0'` |
| Syscall admission | OpenWindows | `syscall/dispatcher.c` | `syscall_user_range` + `syscall_copy_name`; page-wise, rejects overlong names |
| Console plane | Essentials | `DLL/suct/suct.c` | 25x80 plane, private width table, PUA zones |
| Terminal controls | Essentials | `Software/owterm` | escape sequences parsed inline; no separate terminal model |
| TTY driver | Essentials | `Drivers/ttydrv/ttydrv.c` | `uint8_t` ASCII cells |
| Font engine | Essentials | `DLL/owfont64/owfont64.c` | 1-bpp bitmap blit |
| Locale | Essentials | `DLL/owlocale64/` | Present; no SUAS/SUTS binding |
| Keyboard | Essentials | `Drivers/kbdlayout/` | scan-code to character; ASCII-oriented, no composition state or dead-key model |

Two facts dominate the profile:

1. **The kernel links encoding and nothing else.** SUCS mode and SUTF-8 are
   compiled in; the string, name, console, and segmentation layers are not.
2. **The two text stacks do not share a contract.** `suct` computes widths
   locally, `kstring` measures by terminator, and neither has an ownership
   model, so text that crosses between them is copied by convention only.

---

## 4. Adoption Matrix

The matrix is the core of this report. It has 40 rows: 9 kernel-core paths,
10 console and presentation paths, and 21 text algorithms. The 21 algorithm
rows are not a backlog by themselves; the tier decides whether a row is work
now, work with a feature, or no work at all.

### 4.1 Kernel core

| Subsystem | Documents | Tier | Why |
|-----------|-----------|------|-----|
| `lib/sucs.c` mode switching | SUTR-0, SUTR-1, SUTR-10 | **Linked** | already the only integration point; SUTR-10 zero-allocation gate applies |
| `lib/kstring.c` | SUAS-018 (SKTO) | **T0** | NUL dependence and no capacity is the root defect behind most others |
| `lib/kprintf.c` | SUAS-018, SUAS-002 | **T0** | `%s` is an unbounded read; width is needed for column arithmetic |
| `syscall/dispatcher.c` | SUAS-018 §7 | **T0** | admission exists for one call site; the standard generalizes it |
| `vfs/vfs.c` names | SUAS-018, SUAS-007, SUAS-004, SUTS-005 | **T0** | names are resolved and re-resolved; publication immutability closes the race |
| `core/alpc.c` port names | SUAS-018, SUAS-007 | **T0** | same defect as VFS, same fix |
| `ps/owx_loader.c` image names | SUAS-018, SUTS-005 | **T1** | bounded names matter at the trust boundary; no immutability need yet |
| `shell/shell.c` lexing | SUAS-018, SUAS-007, SUAS-006, SUAS-003 | **T1** | line editing and token boundaries need STS/SBR once the shell accepts non-ASCII |
| Diagnostics and `bancode/` | SUTR-0, SUAS-018 | **T0** | BANcodes are SCP values and must not enter text objects |

### 4.2 Console and presentation

| Subsystem | Documents | Tier | Why |
|-----------|-----------|------|-----|
| `DLL/suct` plane | SUTS-012, SUAS-002 | **T0** | private width table is a direct conflict with the mandatory standard |
| `DLL/suct` wrapping | SUTS-012, SUAS-003 | **T0** | wrap points are break decisions |
| `DLL/suct` clustering | SUTS-012, SUAS-006 | **T0** | combining marks are currently dropped |
| `DLL/suct` PUA zones | SUTS-012 §7.4, SUTR-8 | **T1** | registry needed before any second consumer of the zones appears |
| `Drivers/ttydrv` | SUTS-012 §7.1 (L0) | **T1** | legal, but must be declared lossy and never presented as conformant |
| `DLL/owfont64` | SUTR-8, SUAS-014 | **T2** | rasterizer does not need properties until vertical or complex scripts arrive |
| `DLL/bootvid` | SUTS-012 §7 | **T1** | boot glyphs are a PUA consumer; needs zone registration |
| `Software/owterm` | SUTS-012, SUAS-002/003/006 | **T1** | the user-facing console should claim SCTS before it claims anything else |
| `Software/owterm` input | SUAS-019 (STIM), SUAS-018 | **T1** | key events reach a text sink with no declared event-to-text model |
| `Software/owterm` emulation | SUTS-013 (SCTE), SUTS-012 | **T1** | control sequences are interpreted inline instead of through SCTE |

### 4.3 Text algorithms

| Subsystem | Documents | Tier | Why |
|-----------|-----------|------|-----|
| Shell identifier checking | SUAS-007 (SIS) | **T1** | only when the shell accepts non-ASCII identifiers |
| Confusable and homograph checks | SUTS-005 (SCSS) | **T1** | needed once names can contain non-ASCII |
| Canonical equivalence in names | SUAS-004 (SUCF) | **T1** | needed once names can be canonically equivalent |
| Line breaking | SUAS-003 (SBR) | **T0** for any wrapping surface; **T2** otherwise | no wrapping surface outside the console yet |
| Segmentation | SUAS-006 (STS) | **T1** | line editor, word motion, selection |
| Collation | SUTS-001 (SUCA) | **T2** | no sorted user-visible list yet |
| Regex | SUTS-002 (SRE) | **T2** | no in-kernel regex consumer |
| Locale formatting | SUTS-003 (SLDF) | **T2** | `owlocale64` exists but does not format through a schema |
| Variation sequences | SUTS-004 (SIVD) | **T2** | needs a font that carries them |
| Emoji sequences | SUTS-007 (SUEPS) | **T2** | no colour glyph path |
| Source-code handling | SUTS-008 (SUSCH) | **N/A** | no kernel source pipeline |
| Link detection | SUTS-009 (SULDF) | **N/A** | no user-facing link surface |
| Set notation | SUTS-011 (SUSN) | **T1** | useful for compiled allow/deny sets in policy checks |
| IDN | SUTS-006 (SIDN) | **T2** | network exists; no internationalized labels yet |
| BiDi | SUAS-001 (SDF) | **T2** | no right-to-left UI |
| Vertical text | SUAS-014 (SUVTL) | **T2** | no vertical surface |
| Arabic marks | SUAS-015 (SUAMR) | **T2** | no shaping pipeline |
| Mongolian | SUTR-11 (SUMS) | **T2** | no shaping pipeline |
| Cuneiform, hieroglyphs, East Asian script data | SUAS-016, SUAS-017, SUTR-012 | **T2** | data-only; adopt with the fonts that need them |
| East Asian spacing | SUTR-013 (SUEAS) | **T2** | SGW already answers the grid question |
| Mathematical layout | SUTR-9 (SUMNLS) | **T2** | no math surface |

### 4.4 What the matrix is not

The tier is a judgement about **this tree**, not a claim that a document is
unimportant. SUCA and SRE are fully implemented and tested; they are T2 here
only because no kernel subsystem consumes collation or regular expressions
yet.

---

## 5. Profile Levels

Three cumulative levels describe how far a kernel has adopted the text
architecture. They are informative labels for planning, not claims a document
confers.

### 5.1 KT-0 - Encoding only

The implementation links SUCS and one SUTF form, and can encode, decode, and
validate a byte stream.

Current OpenWindows status: **met**. It links SUTF-8 and SUCS mode.

What KT-0 does not give: any guarantee about text ownership, name stability,
or console correctness. A kernel at KT-0 can still walk off a buffer, split a
name between check and use, and drop combining marks.

### 5.2 KT-1 - Bounded text

KT-0 plus SUAS-018: text objects with explicit length and capacity, the four
ownership classes, admission for untrusted text, and immutable published
names. This is the level at which the kernel's own interfaces stop being the
weakest part of the chain.

Current OpenWindows status: **not met**. `syscall_copy_name` implements part
of the admission pattern for one call site; there is no text object anywhere.

### 5.3 KT-2 - Conformant console

KT-1 plus SUTS-012: a plane that takes width from SGW, breaks from SBR,
clusters from STS, and declares its sink level. An input surface additionally
follows SUAS-019 (STIM), and a console that interprets control sequences
follows SUTS-013 (SCTE) on top of SCTS.

Current OpenWindows status: **not met**. `suct` is close in shape and
conflicts in authority.

Levels are cumulative in the sense that a KT-2 implementation that is not
KT-1 is not really KT-2: a console built on unbounded strings inherits every
one of their defects.

---

## 6. Gaps

Each gap names the normative document that closes it and the observable
evidence that it is open. The profile records 11 gaps.

| # | Gap | Closed by | Evidence |
|---|-----|-----------|----------|
| G-1 | Kernel text has no length independent of a terminator | SUAS-018 | `lib/kstring.c` `ow_strlen` |
| G-2 | No ownership or lifetime classes | SUAS-018 | `inc/ow_string.h` is all `char *` |
| G-3 | Name publication is mutable | SUAS-018 §9 | `vfs/vfs.c` writes `e->Name` in place |
| G-4 | Resolve-then-use races on names | SUAS-018 §9.4 | `vfs/vfs.c` compares, then acts |
| G-5 | Console owns a second width table | SUTS-012 §1.2 / SUAS-002 | `suct.c` `zero_width_ranges`, `wide_ranges` |
| G-6 | Console drops combining marks | SUTS-012 §5.1 / SUAS-006 | `plane_emit_cp` returns OK for `w == 0` |
| G-7 | PUA zones are unregistered and overlap markers | SUTS-012 §7.4 | `suct.h` `0xEE00`/`0xEE01` inside the OWM block |
| G-8 | Legacy ASCII sink is indistinguishable from a conformant one | SUTS-012 §7.1 | `ttydrv.h` `uint8_t ch` |
| G-9 | No SCP filtering in text | SUAS-018 §4.3 | no kernel path rejects `0x110000`-`0x11FFFF` |
| G-10 | Key events reach a text sink without a declared model | SUAS-019 | `owterm` input path has no composition or repeat contract |
| G-11 | Console control sequences are interpreted without a terminal model | SUTS-013 | `owterm` parses escapes inline; no SCTS/SCTE separation |

The registry-drift gap recorded during the 2026-10-23 audit is closed: the
curated counts and the corpus now agree at 47 specifications (19 mandatory, 12
optional, 15 reference, 1 note), and D-4 asserts that agreement. It is not
carried as an open gap here.

---

## 7. Gates

A gate is a check that can be run and that produces a pass or fail. The gates
below are recommendations for this repository; they do not create obligations
beyond the documents they test.

### 7.1 What each gate proves

| Gate | Command | Proves |
|------|---------|--------|
| D-1 | `cmake --build build --target verify-docs` | the four family indexes exist |
| D-2 | `ctest --test-dir build` | the unit suites pass, including registry self-consistency and header coexistence |
| D-3 | `ctest --test-dir build -R test_unified` | every public header compiles in one translation unit |
| D-4 | `ctest --test-dir build -R test_suas_ssre` | the curated registry agrees with its own tests |
| K-1 | OpenWindows `hosttest` | the kernel boots and its integration assertions pass |
| K-2 | OpenWindows `make qemu` | the boot path and SUCS commit assertions pass |

### 7.2 What the gates do not prove

- D-1 checks **document shape and cross-reference integrity**: family
  directories and indexes, required fields, index links, duplicate identifiers,
  SUTN disclaimers, and one-to-one coverage between the corpus and the SSRE
  registry. It does not check prose quality, the truth of a status or version
  claim, or that every conformance clause has an executing test. A document with
  a correct shape and an overstated status still passes D-1.
- D-2 proves the behaviour each test asserts. The SUAS-018, SUTS-012,
  SUAS-019, and SUTS-013 reference implementations now ship with unit suites in
  this repository, and D-2 runs them. Those suites prove the cases they assert;
  none proves that the OpenWindows kernel, `suct`, `owterm`, or any other
  consumer has adopted the standard.
- K-1 and K-2 run in a different repository and were not run for this report.

The honest status of the corpus is therefore: **documents present, indexes
verified for presence, reference implementations and their unit suites present
for SUAS-018, SUTS-012, SUAS-019, and SUTS-013, no OpenWindows integration
evidence for any of them, and no claim of conformance.**

### 7.3 Sequencing

The recommended order follows the dependency graph rather than the document
number:

1. **KT-1 foundation.** SUAS-018 first, because VFS, ALPC, the shell, and the
   console all consume text and all currently consume it unsafely. Landing a
   console standard before a text object standard builds a console on the
   defect it is meant to remove.
2. **Name safety.** SUAS-007 and SUTS-005 follow immediately, since admission
   without policy still admits homographs.
3. **Console authority.** SUTS-012, replacing `suct_cell_width` with SGW and
   the wrap heuristic with SBR.
4. **Identity and search.** SUAS-004, then SUCA and SRE when a consumer
   exists.
5. **Everything gated on a feature.** BiDi, vertical, shaping, IDN, emoji.

---

## 8. Answers to the Open Questions

The repository audit of 2026-10-23 left several questions open. This report
records a position on the ones that concern text. A position here is advice.

- **Q5 - Which documents are mandatory?** The family hierarchy answers it:
  SUAS is mandatory, SUTS is optional but binding on claim, SUTR is
  informative. There is no separate list of "important" documents; the prefix
  is the list. See `docs/INDEX.md`.
- **Q8 - What is the OpenWindows conformance target?** KT-1 is the realistic
  near target and KT-2 the next. KT-0 is met today and is not a meaningful
  claim on its own.
- **Q4 - Governance for new identifiers?** Not answered here. The SSRE
  registry (SUAS-010) provides the mechanism, but who may allocate a number,
  and what review a new SUAS requires, is still unwritten. This report flags
  it rather than deciding it.

---

## 9. Non-Goals

This profile explicitly does not do the following.

- **No BiDi.** OpenWindows has no right-to-left UI. SUAS-001 stays T2.
- **No vertical text.** SUAS-014 stays T2 until a vertical surface exists.
- **No shaping.** Arabic, Mongolian, and Indic shaping need a shaping engine
  that does not exist in either repository.
- **No emoji rendering.** A colour glyph path is a graphics problem; SUAS-007
  (SUTS-007) can wait for it.
- **No font work.** This profile does not ask `owfont64` to gain features.
- **No storage changes.** OWFS/USFS formats are SUTR-3's business.
- **No collation or regex adoption.** Both are implemented; neither has a
  consumer.
- **No renumbering.** The profile adopts the existing identifiers and does
  not propose renames.
- **No conformance claim.** Nothing in this report asserts that OpenWindows
  conforms to any SUAS or SUTS document beyond KT-0.

---

## 10. Change Control

This report is informative and may be revised whenever the evidence changes.
Its own rules:

1. The matrix must cite a file or a command for every status it asserts.
2. A status may be downgraded without notice; an upgrade requires the
   evidence rule of section 2.
3. New documents are added to the matrix when they are registered in SUAS-010,
   not when they are drafted.
4. When a normative document changes, the affected rows are re-derived rather
   than edited to match the old conclusion.

---

## 11. Terms and Conventions

- **KT-0, KT-1, KT-2** are the informative profile levels of section 5.
- **T0, T1, T2** are the adoption tiers of section 2.
- **SKTO** is SUAS-018, System Kernel Text Objects.
- **STIM** is SUAS-019, System Text Input Model.
- **SCTS** is SUTS-012, System Console Text Service.
- **SCTE** is SUTS-013, System Console Terminal Emulation.
- **SGW**, **SBR**, **STS**, **SIS**, **SDF**, **SUCF**, **SUCD** are the SUAS
  standards named in their citations.
- **SSRE** is the specification registry (SUAS-010).
- **SUCM** is the conformance model (SUTR-10).
- The key words must, should, and may carry no normative force in this
  report; where a normative sentence is quoted, its source document is cited.

---

## Annex A. References

- `docs/INDEX.md` - family hierarchy and binding force.
- `docs/suas/SUAS-018-skto.md` - System Kernel Text Objects.
- `docs/suas/SUAS-019-stim.md` - System Text Input Model.
- `docs/suts/SUTS-012-scts.md` - System Console Text Service.
- `docs/suts/SUTS-013-scte.md` - System Console Terminal Emulation.
- `docs/sutn/SUTN-001-superunicode-implementation-checklist.md` - the
  non-normative implementation checklist.
- `docs/audit/2026-10-29-gap-analysis.md` - the corpus gap analysis that
  selected SUAS-019, SUTS-013, and SUTN-001.
- `docs/suas/SUAS-002-sgw.md`, `SUAS-003-sbr.md`, `SUAS-006-sts.md`,
  `SUAS-007-sis.md` - width, breaks, clusters, identifiers.
- `docs/suas/SUAS-010-ssre.md` - the registry this profile uses as a gate.
- `docs/sutr/SUTR-10-sucm.md` - zero-allocation and verification gates.
- `docs/audit/2026-10-23-repository-audit.md` - the audit this profile
  answers.
- `OpenWindows/krnl_spec.md` - the kernel's own inventory.
- `OpenWindows-Essentials/SUPERUNICODE_INTEGRATION.md` - how Essentials
  consumes the vendored corpus.
