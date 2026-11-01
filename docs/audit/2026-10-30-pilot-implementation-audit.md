# Pilot Implementation Audit and Roadmap

- **Date:** 2026-10-30
- **Status:** Informative analysis; records implemented work and remaining gaps
- **Scope:** The completed six-document pilot (SUAS-018, SUTS-012, SUTR-014,
  SUAS-019, SUTS-013, SUTN-001), the reference implementations and unit suites
  in this repository, the vendored `compat/` material, and the OpenWindows
  integration points named by SUTR-014.
- **Evidence rule:** A behaviour is recorded as implemented only when source in
  this repository or in the audited OpenWindows tree exercises it and a test
  asserts it. A document's existence is never implementation evidence. KT-3
  records a real kernel build and a kernel-side host test; no QEMU boot or
  platform conformance claim is made.

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
- did not depend on a consumer source tree being unavailable at the screening
  stage;

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

**Action.** Replaced the in-place rewrite with a per-name pool of immutable
snapshot records plus explicit reader references. Publication writes only an
inactive record whose reader count is zero, then switches the visible index.
`suas_skto_resolve` acquires a reference and `suas_skto_name_release` releases
it, so a held generation survives any number of later publications and is
reclaimed only after its final release. When every reusable record is held,
publication returns `SKTO_E_BUSY` without mutation. The earlier two-buffer step
is superseded by the KT-2 reader-lifetime record in section 12. The lock model
is described there as well: optional caller callbacks bracket each operation,
and the implementation does not claim lock-free behaviour.

---

## 4. Implemented work by specification

### 4.1 SUAS-018 (SKTO)

| Clause | Change | Evidence |
|--------|--------|----------|
| 4.4 Construction | Bounded legacy C-string constructor | `test_legacy_cstr_and_assign` |
| 5.3 Owned | Bounded assignment into caller storage, failure atomicity | `test_legacy_cstr_and_assign` |
| 7.1 step 4 | NUL-terminated admission with bounded scan | `test_admission` |
| 7.3 Failure reporting | `SKTO_E_TERMINATOR` | `test_legacy_cstr_and_assign`, `test_admission` |
| 9.2/9.3 Publication | Snapshot pool with explicit reader references; a referenced record is never rewritten; final release enables lazy reuse | `test_publication`, `test_reader_lifetime`, `test_capacity_and_reclamation` |
| 9.1 N5 Name storage | Storage ceiling raised from 64 to 128 bytes so a complete OWFS/USFS name field fits; namespace limits can still lower it | `test_store_geometry`, `test_over_ceiling_refused` |
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

### 4.5 KT-1 compatibility adapter (host-side)

`compat/ow_skto_name.h` and `compat/ow_skto_name.c` add a small adapter at the
vendored OWFS/USFS name-field boundary. It is host compatibility glue, not a
kernel component, and it is kept outside the vendored storage sources.

Architecture:

1. `ow_skto_name_store_t` owns a `skto_name_table_t` plus the namespace limits.
   It is caller-allocated; the adapter performs no allocation.
2. `ow_skto_name_publish` rejects a length above `OW_SKTO_NAME_MAX_BYTES`
   (127 bytes, the 128-byte OWFS/USFS field minus its terminator) with
   `SKTO_E_POLICY` before touching the candidate.
3. It calls `suas_skto_admit` with a 128-byte stack stage and the caller's
   optional containment callback. Admission validates encoding and
   admissibility, and refuses an interior NUL at the name-policy step.
4. It calls `suas_skto_publish`, which copies the complete admitted text into
   namespace-owned storage and returns the generation. No caller pointer is
   retained, and no prefix is ever copied.
5. `ow_skto_name_resolve` builds a local borrowed descriptor and, when the
   caller asks for a view, acquires one reader reference. The caller must pass
   that view to `ow_skto_name_release` exactly once. `ow_skto_name_resolve_at`
   additionally requires the expected generation; on a stale generation it
   releases the reference it acquired and returns `SKTO_E_NOTFOUND` rather than
   authorising a stale use.

The adapter exposes the KT-2 lifetime model directly. A held view stays
byte-exact across later publications, not merely one. Publication that would
overwrite a referenced record returns `SKTO_E_BUSY` without changing the store.
A new locked initialiser, `ow_skto_name_store_init_locked`, installs caller lock
callbacks so publish, resolve, and release are mutually excluded. The adapter
performs no atomic operations and makes no lock-free claim; without callbacks
the caller must serialize the table. `ow_sutf8_name_copy` still refuses a name
that cannot fit together with its terminator, returning
`OW_STRING_COPY_REFUSED` and leaving the destination untouched. Successful
in-range calls keep the old signature and result.

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
- `CMakeLists.txt` (registers the host adapter test)

Compatibility boundary:

- `compat/ow_skto_name.h` (new adapter API and lifetime contract)
- `compat/ow_skto_name.c` (new bounded admission/publication implementation)
- `compat/test_compat_skto_name.c` (new host test)
- `compat/test_compat_storage.c` (truncation-refusal assertion)
- `compat/OpenWindows-storage/common/include/ow_string.h`
- `compat/OpenWindows-storage/common/src/ow_string.c`
- `compat/CMakeLists.txt` (adapter test in the separate compat suite)

KT-2 reader lifetime changed the same SKTO and adapter files, plus their tests:
`include/suas/suas_skto.h`, `src/suas/suas_skto.c`, `tests/test_suas_skto.c`,
`compat/ow_skto_name.h`, `compat/ow_skto_name.c`, and
`compat/test_compat_skto_name.c`. No new build target was required; the
existing root and compat tests were extended.

Informative documentation:

- `docs/suas/SUAS-018-skto.md`
- `docs/suas/SUAS-019-stim.md`
- `docs/suts/SUTS-012-scts.md`
- `docs/suts/SUTS-013-scte.md`
- `docs/audit/2026-10-30-pilot-implementation-audit.md` (this file)
- `docs/INDEX.md` (navigation link)

API and compatibility implications:

- `SKTO_NAME_MAX_BYTES` changed from 64 to 128 in KT-1. KT-2 replaced the
  two-buffer slot with a pool of four `skto_name_snapshot_t` records. On a
  64-bit host the 16-name table grows from roughly 4.3 KiB to about 9.1 KiB.
  `skto_name_slot_t`, `skto_name_table_t`, and the embedded
  `ow_skto_name_store_t` are caller-allocated public structs, so this is a
  further ABI break for an already-compiled external consumer. It does not
  change any normative clause: N5 makes the ceiling a namespace policy, and the
  pool size is an implementation storage bound under section 11.
- KT-2 changed several public signatures. `suas_skto_resolve` now takes a
  mutable `skto_name_table_t *` because acquisition updates a reader count, and
  `ow_skto_name_resolve`/`ow_skto_name_resolve_at` likewise take mutable
  stores. `suas_skto_name_release`, `suas_skto_names_init_locked`,
  `ow_skto_name_release`, and `ow_skto_name_store_init_locked` are new.
  `SKTO_E_BUSY` is new in the SKTO status space. Existing publication
  signatures are unchanged.
- Reader references are mandatory for every non-NULL resolve view. A caller
  that previously treated the view as valid until two later publications must
  now release it; failing to release pins capacity until the table is
  re-initialised. With no lock callbacks installed, callers must serialize all
  table operations.
- `ow_sutf8_name_copy` keeps its signature. Its contract changed from
  truncating to refusing, using `OW_STRING_COPY_REFUSED` (`(size_t)-1`). A
  caller that relied on silent truncation must now handle the sentinel; callers
  that already checked `copied == len` behave as before.
- The compat adapter API is new and host-only. No vendored storage caller was
  rewired. KT-3 later added a separate kernel adapter in the real OpenWindows
  tree at `kapi/ow_skto_name.{h,c}` and wired it into the syscall name
  boundaries; see section 13. The compat and kernel adapters are independent
  implementations over the same SUAS-018 API, not one shared translation unit.

No normative clause was weakened and no new identifier was minted. The status
addition is local to the documented SKTO status space, and no BANcode was
allocated.

---

## 6. Test levels and results

| Level | What ran | Result |
|-------|----------|--------|
| Spec validation | `cmake --build build --target verify-docs` | 47 documents verified across SUAS, SUTS, SUTR, and SUTN; indexes, fields, links, SUTN disclaimers, and SSRE coverage pass |
| Unit | `ctest --test-dir build` | 58 of 58 pass |
| Compatibility boundary | `ctest --test-dir compat/build` | 5 of 5 pass, including `compat_skto_name` |
| Cross-module integration | STIM output validated through SKTO; SCTE drives SCTS; SCTS uses SGW/SBR/STS | Pass in the unit binaries that link those modules |
| Host-side kernel | Freestanding C99 static libraries and test executables built on the host | Build and unit execution pass |
| OpenWindows kernel boundary | `openwinkrnl_pe` target in `../OpenWindows/build-kt3` (MinGW GCC 15.2.0) | Builds and links with the SKTO adapter and the dispatcher admission paths; see section 13 |
| OpenWindows hosttest | `make hosttest` in `../OpenWindows` | 1045 passed, 0 failed, including 63 SKTO name-admission checks |
| QEMU | OpenWindows boot and commit assertions (OKTP K-2) | Not run; only a kernel PE link was exercised |
| Platform conformance | OpenWindows or Windows consumer conformance | Not claimed; no consumer was built or executed |

The KT-1 host test covers the required categories directly:

| Requirement | Test |
|-------------|------|
| Invalid input refusal | `test_invalid_input_refused` (absent argument, overlong, truncated, SCP, interior NUL) |
| Out-of-range refusal | `test_over_ceiling_refused` (127 accepted, 128 refused as policy) |
| Bounded copying | `test_bounded_copy_and_containment`, `test_legacy_copy_refuses_truncation` |
| Successful publication | `test_publication_and_generation` (ASCII and 3/4/5/6-byte names) |
| Generation stamping | `test_publication_and_generation` |
| Resolve-then-use stability | `test_resolve_then_use_stability` (one later publication survives; retired generations rejected) |
| Failure atomicity | `test_failure_atomicity` (failed publish leaves the live generation intact) |

The KT-2 reader-lifetime tests cover the required categories directly:

| Requirement | Test |
|-------------|------|
| Acquisition and release | `test_reader_lifetime`, `test_release_validation` |
| Older generation retained after newer publication | `test_reader_lifetime` (generation 0 held across six publications), `test_resolve_then_use_stability` (adapter) |
| Stale-generation rejection | `test_resolve_then_use_stability` (`ow_skto_name_resolve_at` rejects retired generations), `test_publication` |
| No reuse while referenced | `test_capacity_and_reclamation`, `test_adapter_reader_lifetime` |
| Reclamation after final release | `test_capacity_and_reclamation`, `test_adapter_reader_lifetime` |
| All snapshot records occupied | `test_capacity_and_reclamation`, `test_adapter_reader_lifetime` (`SKTO_E_BUSY`) |
| Failure atomicity at capacity | Same tests: generation, active record, and held bytes unchanged after `SKTO_E_BUSY` |
| Multiple readers on one generation | `test_capacity_and_reclamation` (three references per generation), `test_release_validation` |
| Multiple successive publications | `test_reader_lifetime` (six), `test_concurrent_readers_and_writer` (sixteen) |
| Concurrent readers and writer | `test_concurrent_readers_and_writer` (Windows host; two readers and one writer synchronized so every publication occurs while both readers hold a view) |
| Lock-hook bracketing | `test_lock_hooks` (core), `test_adapter_lock_hooks` (adapter) |

The concurrency test is host-side evidence for the installed critical-section
callbacks only. It does not claim a kernel memory-order implementation or
lock-free publication, and it is compiled only on Windows.

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

The OpenWindows kernel and Essentials source trees are present as sibling
repositories (`../OpenWindows`, `../OpenWindows-Storage`,
`../OpenWindows-Essentials`). KT-3 integrated SKTO into the real syscall name
boundary and is recorded in section 13. `compat/` remains vendored
compatibility material, not the kernel. Where this section says a consumer is
not wired, that statement describes the state after KT-3: the gateway is wired,
the object and VFS namespaces are not.

Adopting the pilot in OpenWindows still requires, at minimum:

### 7.1 Kernel text (KT-1)

- `lib/kstring.c`, `inc/ow_string.h`, and `lib/kprintf.c` to consume SKTO
  objects instead of NUL-terminated `char *`. Not done: they still take
  `const char *`. KT-3 admits a name before those calls, so the pointer they
  receive is bounded and complete, but the functions themselves do not carry an
  explicit length.
- `syscall/dispatcher.c`: done in KT-3. The gateway calls
  `syscall_admit_name()`, which calls `OwSktoNameAdmitUser()` with
  `syscall_user_range()` as the containment callback, and the in-kernel
  `OwApi*` wrappers take a trusted path. See section 13.
- `vfs/vfs.c`, `core/alpc.c`, `ps/owx_loader.c`, and `shell/shell.c` to publish
  names through SKTO and act on a generation-stamped handle. Not done: these
  receive an already-admitted `const char *` but do not hold a published name or
  a generation. The publication table is not wired into the live namespace.
- A kernel-side retirement policy for old generations. Not done: the reference
  implementation provides a snapshot pool with explicit acquire/release and
  `SKTO_E_BUSY`, and KT-3 supplies real interrupt-save lock callbacks, but no
  live OpenWindows namespace owns a table to retire from. SKTO's 16-name table
  capacity is the blocker recorded in section 13.7.

The host adapter at `compat/ow_skto_name.h` remains the host-side analogue of
the publication step for those consumers. The kernel adapter is the separate
`kapi/ow_skto_name.{h,c}` file added by KT-3. Neither is linked into
`vfs/vfs.c`, and no OpenWindows namespace publishes through either one.

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
repository. KT-3 ran the kernel compile/link and the Makefile `hosttest` gate;
K-2 (`make qemu`) was not run. CMake `hosttest` remains unbuildable for a
pre-existing reason recorded in section 13.4. Until K-2 passes with the adopted
translation units linked, the honest status remains: reference implementations,
unit suites, and a real kernel boundary integration are present; no OpenWindows
boot evidence and no conformance claim.

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

1. **Reader lifetime is bounded by explicit capacity.** A reader that holds a
   view pins its record until release; four records exist per name. If four
   generations are all referenced, publication of a fifth returns
   `SKTO_E_BUSY`. This is deterministic but not unbounded. Callers must release
   views and must not re-initialise or destroy a table while a view is
   outstanding.
2. **Synchronization is caller-supplied, not lock-free.** The table embeds
   optional lock callbacks, and the host test installs a Windows critical
   section. SKTO uses ordinary C stores and defines no atomic memory order, so a
   kernel consumer must supply its own namespace lock or a separately proven
   ordering protocol. The `SKTO_E_BUSY` and reclamation logic assumes every
   table operation is serialized by that lock.
3. **Public struct layout changed twice.** KT-1 raised `SKTO_NAME_MAX_BYTES`
   from 64 to 128; KT-2 replaced the two-buffer slot with a four-record
   snapshot pool. `skto_name_slot_t`, `skto_name_table_t`, and
   `ow_skto_name_store_t` are caller-allocated, so both changes are ABI breaks
   for an external consumer and need a versioning decision before the header is
   frozen.
4. **The compat name path no longer truncates but is not wired into a live
   namespace.** `ow_sutf8_name_copy` refuses overflow, and the compat adapter
   admits and publishes names correctly. The compat adapter is consumed only by
   `compat/test_compat_skto_name.c`. KT-3 added an independent kernel adapter
   and wired its admission step into every untrusted syscall name handoff, but
   neither adapter is used by the VFS or object namespace to publish names.
5. **Containment is real at the gateway but not yet exercised by a Ring 3
   execution.** The kernel adapter's user path validates every byte against
   `OwMemInUserWindow()` and the caller's `OwMemFindVad()` tree, and the kernel
   host test drives those predicates against a real VAD tree. No CPL3 transition
   occurs in the host harness, so this is source plus predicate evidence, not a
   syscall round-trip.
6. **No fuzzing or property tests.** The decoders and admission paths have
   example-based tests only. A differential or property test over the full
   SUTF-8 space would find framing regressions faster.
7. **Documented open questions remain open.** STIM has no portable layout file
   format or IME protocol; SCTE omits alternate screen, mouse reporting, and
   wide-character erase repair; SCTS does not define a process-wide glyph-zone
   registry beyond the table contract.
8. **No QEMU or Windows execution evidence.** KT-3 adds a real OpenWindows
   kernel compile/link and kernel-side host tests, but no OpenWindows image was
   booted, no commit assertions ran, and no Windows consumer was built. KT-1 and
   KT-2 remain host-side evidence; KT-3 is build/source evidence plus a
   kernel-side host test, not a booted-kernel claim.
9. **The adapter ceiling follows the storage field.** `OW_SKTO_NAME_MAX_BYTES`
   is derived from `OWFS_NAME_MAX_BYTES`, and the header refuses to compile if
   SKTO cannot store that field. A future namespace with a different field
   size needs its own ceiling rather than a change to SKTO's storage constant.

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

## 11. KT-1 completion record

**Status:** complete as host-side evidence on 2026-10-30.

Exit criteria:

1. The adapter refuses an over-capacity name instead of truncating it. The
   policy check runs before admission, and `ow_sutf8_name_copy` refuses with
   `OW_STRING_COPY_REFUSED`.
2. It copies admitted bytes into namespace-owned SKTO storage and publishes a
   generation. The caller's buffer is not retained.
3. `test_resolve_then_use_stability` holds a generation-0 snapshot across one
   replacement, then shows that `ow_skto_name_resolve_at` rejects that retired
   generation after the next replacement.
4. The adapter is ordinary host C99 with fixed stack staging and no allocation.
   It is compatibility glue, not part of the freestanding `suas_static` library,
   and that host-only placement is explicit.
5. `ctest --test-dir build` passes 58 of 58, the separate compat suite passes 5
   of 5, and `verify-docs` still verifies 47 documents.

No normative specification clause was changed. The `SKTO_NAME_MAX_BYTES`
increase is an implementation storage bound under N5, not a new namespace rule.

**Evidence boundary.** This milestone proves the adapter on the host: admission
failure, ceiling refusal, bounded copy, publication, generation stamping,
snapshot survival across one publication, generation-checked resolution, and
failure atomicity. At the time of KT-1 the audit did not use the OpenWindows
source tree, so it did not prove kernel integration, real VAD containment, a
kernel retirement protocol, or QEMU/Windows execution. KT-3 later established
the kernel build, the real VAD containment callback, and the interrupt-save lock
callbacks at the syscall boundary (section 13); the retirement protocol and
booted-kernel evidence remain gated on K-2.

The next console integration milestone is to bind SCTS to SGW/SBR/STS in a host
harness, then place SCTE and STIM on top. Kernel QEMU evidence remains gated on
the OpenWindows tree and should not be claimed before K-1 and K-2 run.

---

## 12. KT-2 reader-lifetime completion record (SKTO)

**Status:** complete as host-side evidence on 2026-10-31. This KT-2 is the SKTO
reader-lifetime milestone. It is separate from the console workstream label in
section 7.2, which is retained as written.

### 12.1 Lifetime model

- A live name owns four fixed, caller-owned `skto_name_snapshot_t` records.
  One is visible; the rest can retain older generations.
- `suas_skto_resolve()` with a non-NULL `out_view` acquires one reference and
  returns an immutable published view. `suas_skto_name_release()` releases it.
  With `out_view == NULL`, resolve is a point-in-time observation and acquires
  nothing.
- A view is valid until release, table re-initialisation, or destruction.
  Publication cannot overwrite it while the reference is held.
- `suas_skto_resolve_at()` / `ow_skto_name_resolve_at()` acquire first, compare
  the generation, and release on mismatch. A generation check is never treated
  as memory protection by itself.
- Release matches buffer address, generation, and length. A stale, duplicate,
  or tampered view returns `SKTO_E_NOTFOUND` without decrementing another
  reader.

### 12.2 Publication, retirement, and reclamation

1. Validate the candidate before taking the table lock or writing anything.
2. Under the table lock, find an equal live name or a free name slot.
3. For a replacement, choose only an inactive record whose `readers == 0`.
4. Copy into that record, set the new generation, mark it live, and switch the
   visible index.
5. Retire the previous visible record by leaving it immutable. It remains
   readable by its holders and becomes reusable only after its final release.
6. If every inactive record is referenced, return `SKTO_E_BUSY` before any
   mutation. If all name slots are occupied for a fresh name, return
   `SKTO_E_FULL`.
7. Reclamation is lazy, in a later publication. There is no background worker
   and no allocation.

### 12.3 Synchronization and ordering

- The table carries optional `skto_lock_fn` acquire/release callbacks and a
  context pointer. When both are installed, every publish, resolve, and release
  is bracketed by them.
- The callbacks must provide mutual exclusion and at least acquire/release
  ordering. They must not call back into SKTO. The context must outlive the
  table.
- With no callbacks, callers must serialize every table operation externally.
- SKTO performs no atomic operations and makes no lock-free claim. The host
  concurrency test installs a Windows `CRITICAL_SECTION`; it is evidence for the
  callback contract, not for lock-free publication.
- Initialisation and re-initialisation are outside the lock. Re-initialising or
  destroying a table while an operation or view is outstanding is a lifecycle
  error.

### 12.4 Capacity and failure behaviour

`SKTO_NAME_SNAPSHOTS` is four. One record is visible, so up to three older
generations can stay pinned while replacements continue; holding all four
forces `SKTO_E_BUSY`. Reader count saturation at `UINT32_MAX` also returns
`SKTO_E_BUSY`. Failed publication clears `*out_generation`, leaves the active
generation, the visible name, and every snapshot unchanged, and preserves all
held views.

### 12.5 ABI and API changes

- `skto_name_slot_t` now contains `snapshots[SKTO_NAME_SNAPSHOTS]`, `active`,
  and `live`. `skto_name_table_t` embeds the same slots plus `live` and
  `skto_lock_ops_t lock`.
- On a 64-bit host the 16-name table grows from roughly 4.3 KiB to about
  9.1 KiB. `ow_skto_name_store_t` embeds the table and therefore grows too.
- `suas_skto_resolve` and the adapter resolve functions take a mutable table or
  store pointer because acquisition updates a reader count.
- New public names: `SKTO_NAME_SNAPSHOTS`, `SKTO_E_BUSY`, `skto_lock_fn`,
  `skto_lock_ops_t`, `suas_skto_names_init_locked`,
  `suas_skto_name_release`, `ow_skto_name_store_init_locked`, and
  `ow_skto_name_release`.
- `ow_skto_name_publish` keeps its signature. `ow_sutf8_name_copy` keeps its
  KT-1 signature and refusal contract.
- This is an ABI break for an external consumer compiled against the earlier
  header. No versioning decision has been made.

### 12.6 Tests and results

The root suite gained `test_reader_lifetime`, `test_capacity_and_reclamation`,
`test_release_validation`, `test_lock_hooks`, and
`test_concurrent_readers_and_writer` (Windows only). The compat suite gained
`test_adapter_reader_lifetime` and `test_adapter_lock_hooks`, and existing
adapter tests now release every acquired view.

| Check | Result |
|-------|--------|
| `cmake --build build` | Pass |
| `ctest --test-dir build --output-on-failure` | 58 of 58 pass |
| `cmake --build build --target verify-docs` | 47 documents verified |
| `cmake --build compat/build` | Pass |
| `ctest --test-dir compat/build --output-on-failure` | 5 of 5 pass |
| `git diff --check` | No whitespace errors; Git reports expected LF-to-CRLF notices only |

The concurrency test uses two reader threads and one writer thread with
auto-reset events. In every cycle both readers acquire and hold a view, the
writer publishes while both references are held, and only then are the readers
allowed to verify their bytes and release. The test therefore exercises overlap
deterministically rather than relying on favourable scheduling.

### 12.7 Remaining limitations

- Capacity is bounded at four generations per name; a caller pinning all four
  cannot publish a replacement until one is released.
- The generation is 32-bit and wraps. Release matching inherits the residual
  alias hazard after `2^32` publications of one name.
- No atomics, no lock-free publication, and no kernel memory-order guarantee.
  A kernel must provide the lock and lifecycle policy.
- No kernel integration was added by KT-2 itself. KT-3 later added a kernel
  adapter and wired its admission step into the syscall name boundaries; the
  publication table is still not owned by a live namespace, and no QEMU run or
  platform conformance evidence exists. The KT-2 tests run on the host only.
- Re-initialisation while views are outstanding remains unsafe and is documented
  as a lifecycle rule rather than detected at runtime.

No normative clause was changed. The protocol is evidence for SUAS-018 section
9.3, which already required retiring to wait for readers or use a handle scheme.

---

## 13. KT-3 OpenWindows integration record (SKTO)

**Status:** the syscall name-admission boundary is integrated and builds; the
publication table is not wired into a live namespace and is reported as
partially blocked (section 13.7). This milestone uses the real OpenWindows
kernel tree as a sibling repository. It does not claim a booted kernel, a Ring 3
syscall round-trip, SMP safety, or platform conformance.

### 13.1 Source discovery and baseline

- The audited source trees are `../OpenWindows`, `../OpenWindows-Storage`, and
  `../OpenWindows-Essentials`. OpenWindows already consumes `../superunicode/sutf`
  and `../vip`, so extending its build to `../superunicode/include` and the SUAS
  translation unit is a sibling-repository dependency of the same kind, not a
  new build model.
- Before KT-3 the OpenWindows tree contained no SKTO reference.
- The kernel target `openwinkrnl_pe` built in `../OpenWindows/build-kt3` with
  MinGW GCC 15.2.0 before any change and after the integration.
- `vfs/vfs.c` contains shell-facing legacy helpers that are not the live path
  for user files. The live path is `storage/owdisk.c` into the vendored OWFS and
  USFS code, reached from `syscall/dispatcher.c`. The syscall gateway is the
  genuine untrusted name boundary; the VFS helper layer is not.
- `hal/idt.c` installs vector `0x80` as a DPL3 64-bit interrupt gate
  (`OW_IDT_SYSCALL_ATTR 0xEE`), so handlers run with interrupts disabled on the
  single CPU this kernel supports. `inc/ow_io.h` supplies
  `ow_irq_save()`/`ow_irq_restore()`.

### 13.2 Name boundary audit

Every boundary below is in `syscall/dispatcher.c`. "Admit" means
`syscall_admit_name()`, which selects the untrusted or trusted adapter path and
enforces the same SUTF-8 and namespace-ceiling rules. Storage is the handler's
`ow_skto_name_t` unless stated otherwise; it is complete, NUL-terminated, and
never retained past the call.

| Boundary | Input | Before KT-3 | Validation and storage after KT-3 | Lifetime | Residual risk | Integration |
|---|---|---|---|---|---|---|
| `OW_SYS_VFS_READ` (0x005) | P1 name, P2 buffer, P3 cap | Bounded copy into static `s_read_name`; destination range checked | Per-byte VAD/window containment, then exact-length admission. Destination range checked before access. Name is a stack object; OWFS copies what it persists | Call only | Ring 3 names are capped at 63 content bytes by `OW_USERSPACE_NAME_MAX`, below the 127-byte storage ceiling; a longer name is refused, not truncated | `OwFsOwfsReadOffset(name, ...)` under the caller's Uid/Gid |
| `OW_SYS_VFS_CREATE` (0x003) | P1 name, P2 attrs | Raw pointer passed to `OwVfsCreateFile` | Admit, then the NUL-terminated complete name is passed on | Call only; OWFS stores the directory entry | Legacy helper still calls `ow_strlen`, safe only because the adapter output is bounded and terminated | `OwVfsCreateFile` |
| `OW_SYS_VFS_WRITE` (0x004) | P1 name, P2 payload, P3 size | Raw name and raw payload pointer | Admit; payload range validated against the same VAD/window predicate for Ring 3 origin | Call only | A zero-size payload is accepted without a range check, matching "no bytes read" | `OwVfsWriteFile` |
| `OW_SYS_VFS_DELETE` (0x009) | P1 name | Raw pointer | Admit | Call only | Same 63-byte Ring 3 ceiling | `OwVfsDeleteFile` |
| `OW_SYS_ALPC_CREATE` (0x006) | P1 name, P2 server flag | Raw pointer | Admit | Object creation copies into `OW_OBJECT_HEADER.Name` (`OW_MAX_NAME` = 256), so no second truncation at 127 | The object namespace still has no generation | `OwAlpcCreatePort` |
| `OW_SYS_PS_CREATE_PROCESS` (0x010) | P1 optional name, P2 parent PID | Raw pointer, including NULL | NULL remains "unnamed"; a present name is admitted before the process slot is committed | Process copies the name into its header | Parent PID remains caller-supplied here, as before; this boundary is a name boundary only | `OwPsCreateProcess` |
| `OW_SYS_PS_SPAWN_OWX` (0x020) | P1 image name | Static `s_spawn_name` staging | Admit, then the complete name is used for the OWFS read; the derived process name stays bounded | Image read and process entry | The image bytes are a separate untrusted concern validated by `OwOwxImageIsLoadable` | `spawn_owx_image` |
| `OW_SYS_SYNC_CREATE_EVENT/SEMAPHORE/MUTEX` (0x012/0x015/0x019) | P1 optional name | Raw pointer | NULL remains unnamed; a present name is admitted before the object is allocated | `KeCreate*` copies into the object header | No generation; names are diagnostic, not lookup keys | `KeCreateEvent`, `KeCreateSemaphore`, `KeCreateMutex` |

`OW_SYS_OBJ_OPEN` was audited and left alone: it carries a handle, not a name,
and no untrusted path string reaches it.

### 13.3 Kernel adapter

`kapi/ow_skto_name.{h,c}` is a freestanding C99 adapter over SUAS-018:

- `OwSktoNameAdmitUser()` requires a containment callback. It scans at most
  `ScanCap` bytes, asks the callback about each byte before reading it, then
  calls `suas_skto_admit()` with the explicit length and the same callback.
- `OwSktoNameAdmitTrusted()` skips containment for kernel-owned pointers but
  keeps the bound, SUTF-8 validation, and namespace ceiling.
- `suas_skto_name_check()` enforces NUL-safety, no interior NUL, and the name
  ceiling. An empty name is `SKTO_E_POLICY`, matching OWFS's own rejection of
  `name_len == 0`.
- Storage is `ow_skto_name_t`: 128 bytes plus a `skto_text_t`, content at most
  127 bytes. No allocation, no pointer retention, no truncation.
- A failed call clears `text.bytes` to NULL, so a caller cannot accidentally
  consume a stale name.

The adapter does not call `suas_skto_admit_cstr()`. With
`max_admit_bytes == limit`, a NUL exactly at `limit - 1` is reported as no
terminator, so the adapter uses the explicit-length scan plus `suas_skto_admit`
instead. This is a deliberate workaround, not a second policy.

### 13.4 Build and test integration

- CMake: added `SUAS_DIR`, `${SUAS_DIR}/include`, `kapi/ow_skto_name.c`,
  `${SUAS_DIR}/src/suas/suas_skto.c`, and `tools/hosttest/host_skto_name.c`.
- Makefile: added `SUAS_DIR`, the include path, `SUAS_OBJS`, a
  `suas/%.o: $(SUAS_DIR)/src/suas/%.c` rule, the adapter object, and the test
  source.
- `tools/hosttest/host_boot.c` calls `test_skto_name_run()` and accumulates its
  counts separately.

Results on 2026-10-31:

| Check | Result |
|-------|--------|
| `cmake --build build-kt3 --target openwinkrnl_pe` | Pass; links the adapter and dispatcher |
| `make hosttest` in `../OpenWindows` | 1045 passed, 0 failed; 63 of those are SKTO name-admission checks |
| SKTO suite coverage | Trusted/user admission, per-byte containment, terminator and ceiling refusal, byte-exact multi-byte names, malformed SUTF-8, SCP-plane inadmissibility, real VAD predicates, generation/reference lifecycle, `SKTO_E_BUSY`, and 16-name capacity |
| `make hosttest-asan` | Not run to completion: this w64devkit has no `-lasan` or `-lubsan`; the link fails before execution |
| `cmake --build build-kt3 --target hosttest` | Blocked before KT-3 by a missing Essentials include: `host_boot.c:77` cannot find `kconfctl_core.h` because CMake's include list omits `Software/kconfctl`. The new `host_skto_name.c` compiled successfully before that blocker. The Makefile harness includes the directory and is the harness used here. |
| `make qemu` | Not run |
| Root `ctest --test-dir build` | 58 of 58 pass |
| Root `verify-docs` | 47 documents verified |
| Compat `ctest --test-dir compat/build` | 5 of 5 pass |
| `git diff --check` (both repositories) | Exit 0; Git reports only expected LF-to-CRLF notices |

During bring-up the SKTO suite had two wrong expectations: a surrogate-form
sequence is reported by the current decoder as `SKTO_E_ENCODING` (it is
structurally invalid UTF-8), not `SKTO_E_INADMISSIBLE`, and the containment
callback is invoked once per scanned byte plus once by `suas_skto_admit`. The
tests now assert the linked behaviour, and an SCP-plane five-byte sequence
(`U+110000`) covers the `SKTO_E_INADMISSIBLE` branch. This is an observation
about the existing decoder, not a normative change.

### 13.5 Containment status

`syscall_user_range()` is the same predicate the gateway already used: both
ends of the range must be inside `OW_USER_WINDOW_BASE..OW_USER_WINDOW_END`, and
every page the range touches must be found by `OwMemFindVad()` in the calling
process's own `VadRoot`. The adapter calls it through `syscall_name_range()`,
so containment is decided before a byte is read.

What is claimed:

- the production adapter, the production dispatcher predicate, and the real VAD
  tree code are linked and execute on the host harness;
- an address outside the user window is refused even when a VAD exists for it;
- a range that starts inside a VAD but crosses its end is refused;
- the adapter reads no byte in a page the callback refused.

What is not claimed:

- that a CPL3 process actually made a syscall in these tests. The harness runs
  in Ring 0 on the host and never executes `int 0x80`.
- that the page tables map the range. `syscall_user_range()` validates the VAD
  and the user window; `OwMemVerifyUserPath()`/page-table checks are a separate
  layer and are not added by KT-3.
- that this is SMP-safe or lock-free.

`g_kernel_call_depth` is a plain counter set only by `syscall_call_kernel()`.
It exists so the in-kernel `OwApi*` wrappers can pass kernel-owned pointers
without being rejected by a user-window predicate. It is single-CPU
bookkeeping, not an atomic, and it is not reachable from the interrupt gate. A
future SMP port cannot keep it as a plain global.

### 13.6 Synchronization and publication lifecycle

- Vector `0x80` is an interrupt gate, so `IF=0` for the whole handler on this
  kernel. `OwSktoLockAcquire()`/`OwSktoLockRelease()` wrap the real
  `ow_irq_save()`/`ow_irq_restore()` pair and preserve the caller's IF rather
  than forcing interrupts on.
- Admission itself uses only handler-local storage, so it needs no lock. That is
  a statement about the absence of shared state, not a lock-free protocol claim.
- The SKTO publication table is exercised in the kernel-side host test with
  those lock callbacks. The test covers generation advancement, immutable held
  views, byte-exact older generations, duplicate release rejection, all four
  snapshot records being pinned (`SKTO_E_BUSY`), release followed by successful
  publication, and the 16-name capacity (`SKTO_E_FULL`).
- On host builds `ow_irq_save()`/`ow_irq_restore()` are no-ops. The test proves
  callback wiring and lifecycle, not interrupt masking.

### 13.7 Publication table blocker

SKTO's published namespace holds `SKTO_NAME_CAPACITY` = 16 live names. The live
OpenWindows namespaces are not bounded that way:

- OWFS/USFS store a name in a 128-byte field per directory entry, with no
  16-entry published-name registry.
- The object namespace is a directory tree with `OW_OBJECT_MAX_ENTRIES` per
  directory and objects whose names are copied into their headers.
- There is no existing completion or destruction callback at which a published
  SKTO slot could be retired, and no bounded registry that could own a table
  without changing namespace semantics.

Wiring `suas_skto_publish()` into the VFS or object boundary would therefore
either fail from the 17th distinct name onward or invent a lifecycle the
namespace does not have. KT-3 does not do that. The publication table is
implemented, lockable, and tested, but it is **not** connected to a live
namespace. This is the genuine partially blocked part of KT-3.

A later milestone needs a bounded, long-lived published-name registry with a
defined retirement point, or a larger capacity with a documented eviction and
reference policy, before publication can be wired in without lying about
lifetime.

### 13.8 ABI and error propagation

- `OW_SYS_VFS_ENAMETOOLONG` (`-7`) is added to `inc/ow_syscall.h`. It reports a
  name that did not fit the gateway's scan bound, overflowed length arithmetic,
  or exhausted adapter capacity. The existing negative codes keep their
  meanings.
- `syscall_name_error()` maps: NULL/encoding/inadmissible/empty-policy to
  `OW_SYS_VFS_EINVAL`; containment failure to `OW_SYS_VFS_EFAULT`;
  terminator/overflow/capacity to `OW_SYS_VFS_ENAMETOOLONG`.
- `VFS_READ`, `VFS_CREATE`, `VFS_WRITE`, and `VFS_DELETE` return the mapped
  negative code on admission failure. Their backend success/failure values stay
  `1`/`0`; the in-kernel `OwApi*` wrappers still translate anything other than
  `1` to `OW_ERR_IO`. No caller in OpenWindows or Essentials compared these
  returns to `0` for success.
- `ALPC_CREATE`, `PS_CREATE_PROCESS`, and the three `SYNC_CREATE_*` calls return
  `0` for a refused name, preserving their "no object" contract.
  `PS_SPAWN_OWX` keeps `-1`.
- The in-kernel wrappers now enter through `syscall_call_kernel()`, so trusted
  names take the trusted adapter path instead of failing user-window
  containment.

### 13.9 Limitations and unclaimed behaviour

1. No QEMU run, no booted-kernel assertion, and no platform conformance.
2. No Ring 3 `int 0x80` round-trip test; the dispatcher path is source evidence
   plus adapter/VAD predicate tests.
3. No SMP or memory-order claim. The kernel is single-CPU and the handler runs
   behind an interrupt gate.
4. No publication into a live VFS/object namespace; section 13.7 is the blocker.
5. The Ring 3 name ceiling remains 63 content bytes because
   `OW_USERSPACE_NAME_MAX` is 64 including the terminator. Kernel-origin names
   can use up to 127. Neither path truncates.
6. The adapter does not normalize names. Normalization remains a namespace
   decision, as SUAS-018 requires.
7. No fuzzing or property tests were added.
8. All KT-3 changes remain uncommitted in both repositories.

### 13.10 Files changed by KT-3

OpenWindows:

- `kapi/ow_skto_name.h`, `kapi/ow_skto_name.c` (new)
- `syscall/dispatcher.c` (admission, trusted origin flag, wrapper conversion)
- `inc/ow_syscall.h` (`OW_SYS_VFS_ENAMETOOLONG`, contract note)
- `CMakeLists.txt`, `Makefile` (SUAS source/include/object/test integration)
- `tools/hosttest/host_skto_name.c` (new)
- `tools/hosttest/host_boot.c` (suite registration)

superunicode:

- `docs/audit/2026-10-30-pilot-implementation-audit.md` (this section and the
  corrected source-availability statements)

No normative specification was changed by KT-3.
