# Pilot Implementation Audit and Roadmap

- **Date:** 2026-10-30
- **Status:** Informative analysis; records implemented work and remaining gaps
- **Scope:** The completed six-document pilot (SUAS-018, SUTS-012, SUTR-014,
  SUAS-019, SUTS-013, SUTN-001), the reference implementations and unit suites
  in this repository, the vendored `compat/` material, and the OpenWindows
  integration points named by SUTR-014.
- **Evidence rule:** A behaviour is recorded as implemented only when source in
  this repository exercises it and a test asserts it. A document's existence is
  never implementation evidence. No OpenWindows, Windows, QEMU, or platform
  conformance claim is made.

---

## 1. Why this audit exists

The six-document pilot is complete. The corpus contains 47 documents: 19 SUAS,
12 SUTS, 15 SUTR, and 1 SUTN. The next question is not "which document is
missing" but "which implemented behaviour can be trusted, and what blocks
adoption".

This audit answers that question in four steps:

1. Compare the four pilot implementations (SKTO, SCTS, STIM, SCTE) against
   their normative clauses.
2. Fix the highest-value defects that are implementable and testable inside
   this repository.
3. Separate unit evidence from integration evidence, host-side kernel evidence,
   QEMU evidence, and platform conformance.
4. Recommend one next milestone and defer document authoring that does not own a
   distinct decision.

---

## 2. Baseline and method

The audit began from a green baseline:

- `cmake --build build` succeeded.
- `ctest --test-dir build` passed 57 of 57.
- `cmake --build build --target verify-docs` verified 47 documents.
- `git diff --check` was clean apart from CRLF notices.

Each pilot implementation, public header, unit suite, and the OKTP adoption
matrix were read against the normative text. Candidate work was ranked by
whether it:

- corrected a stated invariant rather than a preference;
- could be proven by a test in this repository;
- avoided inventing platform behaviour or weakening a normative clause;
- preserved freestanding C99 and zero allocation;
- did not require absent OpenWindows source trees.

Four defects survived that screen. They are recorded in section 3 and were
implemented in this change.

---

## 3. Findings and priority

### P1 - Canonical SUTF-8 framing was inconsistent between encoder and decoder

**Observed.** STIM encoded native codepoints in `0x00110000`-`0x001FFFFF` as
four-byte sequences. SUTS-013's decoder accepted non-canonical four-byte values
above `0x0010FFFF` and rejected canonical five-byte values below `0x00200000`.
The two ends therefore disagreed about the same codepoint.

**Normative anchor.** SUTR-1 defines four-byte framing through `0x0010FFFF`,
five-byte framing from `0x00110000` through `0x03FFFFFF`, and six-byte framing
from `0x04000000` through `0x7FFFFFEF`. STIM clause C9 requires valid SUTF-8;
SCTE clauses C9 and C12 require correct delivery and encoding-error recovery.

**Impact.** Text produced by an input model and consumed by a terminal could be
rejected, or accepted under a non-canonical spelling. This is a direct
interoperability defect.

**Action.** Corrected STIM's length and emit thresholds, and corrected SCTE's
canonical bounds for four-, five-, and six-byte sequences. SCTE still returns
`SCTE_E_ENCODING` for malformed input and remains usable, as required by
SUTS-013 section 4.4.

### P2 - SCTS direct codepoint placement skipped admissibility checks

**Observed.** `scts_plane_write` validated decoded input, but
`scts_plane_put_cp` admitted surrogates and values above `0x7FFFFFEF`. A caller
using the direct API could inject codepoints that the byte path rejected.

**Normative anchor.** SUTS-012 section 1.2 invariant 5 forbids SCP as content;
the status table defines `SCTS_E_INADMISSIBLE` for SCP, trap, Sentinel, or
otherwise barred codepoints. SUTS-013 section 4.3 is explicit that a surrogate
is inadmissible.

**Impact.** Direct callers, including terminal control or host adapters, had a
second, weaker admission path. That is the kind of split the standard exists to
remove.

**Action.** Applied the same surrogate and upper-bound rejection used by the
byte path to `scts_plane_put_cp`. SCTE's existing surrogate-to-`unsupported`
handling is unchanged: it is a terminal policy decision, not an encoding error.

### P3 - SKTO lacked mandatory owned, legacy, and bounded-terminator surfaces

**Observed.** SKTO documented owned storage, a bounded legacy constructor, and a
NUL-terminated admission step, but the public header exposed only borrowed,
static, and empty construction. Admission required a pre-measured length, and
there was no way to fill an owned object.

**Normative anchor.** SUAS-018 sections 4.4, 5.3, and 7.1. Section 5.3 requires
owned writes to be bounded by `cap` and to leave the object unchanged on
failure. Section 7.1 step 4 requires the terminator scan to be bounded by the
admission ceiling, never by the buffer's apparent contents. Section 7.3 requires
failure reporting that distinguishes absence, overflow, containment, encoding,
inadmissibility, capacity, and policy.

**Impact.** A kernel could not use SKTO for the two paths it most needs: filling
caller-owned storage and admitting a legacy C string without an unbounded
`strlen`.

**Action.**

- Added `suas_skto_assign` for owned objects. It validates before copying,
  refuses capacity excess without partial writes, supports overlapping source
  and destination with `memmove` ordering, sets `SKTO_F_ADMITTED`, and sets
  `SKTO_F_NUL_SAFE` only when `cap > len` and the bytes contain no interior
  `0x00`.
- Added `suas_skto_init_legacy_cstr`, the only constructor that scans for a
  terminator, with the scan bounded by the caller's `cap`.
- Added `suas_skto_admit_cstr`, which validates containment over the whole scan
  window before reading, bounds the scan by
  `min(scan_cap, admission ceiling)`, and returns `SKTO_E_TERMINATOR` when no
  terminator is found.
- Added `SKTO_E_TERMINATOR` to the status space.

### P4 - Published-name replacement rewrote storage in place

**Observed.** Republishing an equal name copied the new bytes over the slot
before assigning the new generation. A reader holding the old generation could
observe a partially rewritten name.

**Normative anchor.** SUAS-018 sections 9.2 and 9.3: publication is a
transition; a reader sees either the old object or the new one, never a partial
write; published bytes must not change while a reader can reach them.

**Impact.** The defect undermines resolve-then-use. It is exactly the race OKTP
gap G-3/G-4 records for `vfs/vfs.c`.

**Action.** Gave each name slot two immutable buffers. Publication writes the
inactive buffer, advances the generation, and switches the active index, so a
single-threaded reader that resolved before the replacement still sees the
bytes it resolved. The header documents the resulting lifetime honestly: a
snapshot remains readable until that name is published twice more or the table
is re-initialised. This change removes the in-place rewrite; it does not add a
cross-thread memory-ordering protocol, which is recorded as a remaining gap.

---

## 4. Implemented work by specification

### 4.1 SUAS-018 (SKTO)

| Clause | Change | Evidence |
|--------|--------|----------|
| 4.4 Construction | Bounded legacy C-string constructor | `test_legacy_cstr_and_assign` |
| 5.3 Owned | Bounded assignment into caller storage, failure atomicity | `test_legacy_cstr_and_assign` |
| 7.1 step 4 | NUL-terminated admission with bounded scan | `test_admission` |
| 7.3 Failure reporting | `SKTO_E_TERMINATOR` | `test_legacy_cstr_and_assign`, `test_admission` |
| 9.2/9.3 Publication | Two-buffer generation switch; old snapshot survives one replacement | `test_publication` |
| 11 Resource constraints | No allocation added; freestanding C99 build unchanged | CMake freestanding targets |

The owned-assignment test covers capacity refusal, encoding refusal,
inadmissibility refusal, an unchanged object after each failure, overlap, and
the terminator byte. The admission test covers containment over the scan
window, a ceiling shorter than the buffer, a missing terminator, and bytes
behind the first terminator.

### 4.2 SUAS-019 (STIM)

| Clause | Change | Evidence |
|--------|--------|----------|
| 7.1 / C9 | Canonical four/five/six-byte framing | `test_sutf8_framing` |

The new test emits a four-byte BMP boundary value, the first native five-byte
value, and the last allocatable six-byte value. Each result is read back through
SKTO, so the test checks agreement between two independent components rather
than repeating the encoder's arithmetic.

### 4.3 SUTS-012 (SCTS)

| Clause | Change | Evidence |
|--------|--------|----------|
| 1.2 invariant 5, 8 status table | Direct placement rejects surrogates and values above `0x7FFFFFEF` | `test_controls_and_encoding` |
| 9 Serialization | Native values serialize in canonical five-byte form | `test_controls_and_encoding` |

### 4.4 SUTS-013 (SCTE)

| Clause | Change | Evidence |
|--------|--------|----------|
| C9, C12 | Canonical four/five/six-byte decoder bounds | `test_native_framing` |

The test proves that a four-byte native value is a recoverable encoding error
with a counted `encoding_errors` increment, and that the canonical five-byte
form places the same codepoint.

---

## 5. Files changed

Implementation and tests:

- `include/suas/suas_skto.h`
- `src/suas/suas_skto.c`
- `src/suas/suas_stim.c`
- `src/suts/suts_scte.c`
- `src/suts/suts_scts.c`
- `tests/test_suas_skto.c`
- `tests/test_suas_stim.c`
- `tests/test_suts_scte.c`
- `tests/test_suts_scts.c`

Informative documentation:

- `docs/suas/SUAS-018-skto.md`
- `docs/suas/SUAS-019-stim.md`
- `docs/suts/SUTS-012-scts.md`
- `docs/suts/SUTS-013-scte.md`
- `docs/audit/2026-10-30-pilot-implementation-audit.md` (this file)
- `docs/INDEX.md` (navigation link)

No normative clause was weakened and no new identifier was minted. The status
addition is local to the documented SKTO status space, and no BANcode was
allocated.

---

## 6. Test levels and results

| Level | What ran | Result |
|-------|----------|--------|
| Spec validation | `cmake --build build --target verify-docs` | 47 documents verified across SUAS, SUTS, SUTR, and SUTN; indexes, fields, links, SUTN disclaimers, and SSRE coverage pass |
| Unit | `ctest --test-dir build` | 57 of 57 pass |
| Cross-module integration | STIM output validated through SKTO; SCTE drives SCTS; SCTS uses SGW/SBR/STS | Pass in the unit binaries that link those modules |
| Host-side kernel | Freestanding C99 static libraries and test executables built on the host | Build and unit execution pass |
| QEMU | OpenWindows boot and commit assertions (OKTP K-2) | Not run; the kernel tree is not present in this repository |
| Platform conformance | OpenWindows or Windows consumer conformance | Not claimed; no consumer was built or executed |

Additional checks:

- `git diff --check` reports no whitespace errors; Git only warns that LF will
  be normalised to CRLF on the next touch.
- The work remains uncommitted.

During bring-up, three tests failed and were corrected before the final run: an
inverted overlap-copy direction in `suas_skto_assign`, a test that expected the
wrong six-byte lead byte (`0xFC` instead of the canonical `0xFD` for
`0x7FFFFFEF`), and a serialization test whose buffer was too small for a
non-trimmed row. The implementation fix was limited to the copy direction; the
other two were test-expectation and test-fixture corrections.

---

## 7. OpenWindows integration dependencies

The OpenWindows kernel and Essentials source trees are absent. `compat/`
contains vendored compatibility material, not the kernel. Consequently no
integration was performed, and none is claimed.

Adopting the pilot in OpenWindows requires, at minimum:

### 7.1 Kernel text (KT-1)

- `lib/kstring.c`, `inc/ow_string.h`, and `lib/kprintf.c` to consume SKTO
  objects instead of NUL-terminated `char *`.
- `syscall/dispatcher.c` to replace the one-call-site range check with
  `suas_skto_admit` and a real page-mapping callback over the caller's VAD
  tree. The callback is the missing dependency: the reference API is ready, but
  the kernel must supply the containment answer.
- `vfs/vfs.c`, `core/alpc.c`, `ps/owx_loader.c`, and `shell/shell.c` to publish
  names through SKTO and act on a generation-stamped handle.
- A kernel-side retirement policy for old generations. The two-buffer slot
  prevents torn reads across one replacement but does not by itself satisfy the
  section 9.3 requirement to wait for readers or make use-after-retire
  impossible.

### 7.2 Console (KT-2)

- `DLL/suct` to defer width to SGW, breaks to SBR, and clusters to STS, and to
  replace its private range tables.
- `Software/owterm` to claim SCTS before SCTE, and to route key events through
  STIM instead of resolving them locally.
- `Drivers/ttydrv` to remain an explicitly declared L0 sink, never presented as
  SCTS-conformant.
- A registered PUA/glyph-zone table before a second consumer uses the zones.

### 7.3 Evidence gates

OKTP gates K-1 (`hosttest`) and K-2 (`make qemu`) run in the OpenWindows
repository. They were not run here. Until they pass with the adopted translation
units linked, the honest status remains: reference implementations and unit
suites present, no OpenWindows integration evidence, no conformance claim.

---

## 8. Windows boundary guidance

Windows is a separate boundary, not an OpenWindows kernel requirement. The
following are constraints for a future Windows host adapter; none was
implemented or tested in this change.

- Win32 text APIs (`CreateFileW`, `FindFirstFileW`, console `*W` calls) use
  UTF-16 code units, not SuperUnicode codepoints. A value above the BMP is one
  SUCS codepoint and two UTF-16 code units. Conversion must happen at the
  boundary and must not treat a surrogate pair as two characters.
- Length units differ. A Win32 buffer capacity is usually a count of `WCHAR`,
  while SKTO capacity is a byte count. Conversion must check multiplication
  overflow before allocating or admitting.
- `MultiByteToWideChar` replaces invalid UTF-8 unless `MB_ERR_INVALID_CHARS` is
  used. A conforming adapter must validate rather than silently substitute, or
  use the SuperUnicode decoder directly.
- Win32 errors are `GetLastError`/`HRESULT` values. They must be mapped to the
  local status space at the boundary; they are not BANcodes and must not enter
  the SCP.
- Windows has no `suct`, `owterm`, or `ttydrv`. SCTS/SCTE/STIM claims require a
  Windows console or window adapter that supplies the plane, sink, and input
  event source. The existence of a Win32 console is not evidence of SCTS
  conformance.

---

## 9. Remaining gaps

1. **Published-name retirement is bounded.** The double buffer guarantees that
   the next publication cannot tear a snapshot already returned. It does not
   guarantee that an arbitrarily old snapshot remains valid. A reader-tracking
   or reference-counted retirement scheme is still required for full section
   9.3 behaviour.
2. **No concurrency model.** The publication flip is described as one
   descriptor update, but the implementation uses ordinary C stores. A
   multi-core kernel needs an agreed memory order or a lock at the namespace
   boundary.
3. **Public struct layout changed.** `skto_name_slot_t` grew a second buffer.
   `skto_name_table_t` is caller-allocated and therefore changed size. This is
   an ABI break for any external consumer and needs a versioning decision
   before the header is frozen.
4. **The compat name path still truncates.** `compat/OpenWindows-storage`
   exposes `ow_sutf8_name_copy`, which truncates rather than refusing or
   publishing. It was not migrated.
5. **No real containment callback.** `suas_skto_admit` is tested with a stub
   range function. A kernel VAD or Windows virtual-memory implementation is
   still needed.
6. **No fuzzing or property tests.** The decoders and admission paths have
   example-based tests only. A differential or property test over the full
   SUTF-8 space would find framing regressions faster.
7. **Documented open questions remain open.** STIM has no portable layout file
   format or IME protocol; SCTE omits alternate screen, mouse reporting, and
   wide-character erase repair; SCTS does not define a process-wide glyph-zone
   registry beyond the table contract.
8. **No OpenWindows, QEMU, or Windows execution evidence.** Every claim in this
   report stops at the host unit boundary.

---

## 10. Future documents

No new document is recommended for this milestone. The remaining high-value
work is integration and evidence, not authorship.

- **C-4 (proportional application text layout)** and **C-5 (font fallback)**
  remain deferred for the reasons in the 2026-10-29 gap analysis: both are
  large, neither has a shipping consumer in this repository, and C-5 is a
  graphics asset policy rather than character semantics.
- **Clipboard, scrollback, window-manager drawing, search, terminal colour, and
  kernel logging policy** still have owners in SCTS, SUTR-3, SUAS-018, SUTS-001,
  or local policy. They should not become new documents.
- A **Windows host profile** would own a distinct decision only if a Windows
  adapter is actually built. Until then it would duplicate the OKTP evidence
  rules and the boundary notes in section 8.

If a future document is proposed, it should name the decision no existing
document owns, the consumer that needs it, and the executing test that would
prove adoption.

---

## 11. Recommended next milestone

**Milestone: KT-1 host integration evidence at the vendored name boundary.**

Goal: replace the truncating `ow_sutf8_name_copy` path with a SKTO-based
admission-and-publication adapter and prove it with a host test.

Why this milestone:

- It is the smallest step that turns SKTO from a tested library into adoption
  evidence.
- It exercises the three SKTO surfaces added here: bounded admission, owned
  assignment, and generation-stamped publication.
- It does not require the absent OpenWindows kernel tree.
- It produces a reusable adapter for the eventual `vfs/vfs.c` and
  `syscall/dispatcher.c` changes.

Exit criteria:

1. The adapter refuses an over-capacity name instead of truncating it.
2. It copies admitted bytes into namespace-owned storage and publishes a
   generation.
3. A resolve-then-use test shows that a replacement does not tear the snapshot
   already held by the reader.
4. The adapter builds under the freestanding flags where possible, or documents
   the host-only dependency.
5. `ctest --test-dir build` and `verify-docs` stay green.

After that, the next console milestone is KT-2: bind SCTS to SGW/SBR/STS in a
host harness, then place SCTE and STIM on top. Kernel QEMU evidence remains
gated on the OpenWindows tree and should not be claimed before K-1 and K-2 run.
