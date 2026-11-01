# KT-4 to KT-8 Final Integration Audit

- **Date:** 2026-11-01
- **Status:** Informative; records integration work from KT-4 through KT-8
- **Scope:** Kernel containment & VFS integration (KT-4), kernel synchronization
  & lifecycle (KT-5), QEMU runtime integration (KT-6), cross-platform
  compatibility (KT-7), and final conformance & audit (KT-8).
- **Evidence rule:** A behaviour is recorded as implemented only when source in
  the superunicode or OpenWindows repository exercises it and a test asserts it.
  QEMU boot validation is real output from `qemu-system-x86_64` with ACPI S5
  power-off. Host-side tests are not claimed as kernel proofs. ABI layout
  assertions are compiled and executed on the host for 64-bit and under MSVC
  x86 for 32-bit. Fuzzing is deterministic with fixed seed. Sanitizers (ASan and
  UBSan) are compiled and executed via Visual Studio Clang.

---

## 1. Executive Summary

KT-1 through KT-3 (completed previously) established the SKTO specification,
reference implementation, compatibility adapter, and initial OpenWindows kernel
integration. Milestones KT-4 through KT-8 have moved this infrastructure into
production kernel code, live runtime testing, and multi-platform validation:

| Milestone | Summary | Status |
|---|---|---|
| KT-4 | Kernel containment & VFS integration | **Completed and verified** (live name admission, advisory publication on create, snapshot consumption on read, retirement on delete) |
| KT-5 | Kernel synchronization & lifecycle | **Completed and verified** (shutdown, drain, destroy, retirement, generation exhaustion, IRQ-save lock callbacks) |
| KT-6 | QEMU runtime integration | **Completed and verified** (34/34 checks in normal boot including real Ring 3 `int 0x80` VFS round-trip; 8/8 checks in emergency boot) |
| KT-7 | Cross-platform compatibility | **Completed and verified** (64-bit MinGW GCC, 32-bit MSVC x86 compiled & executed, Clang ASan/UBSan validated) |
| KT-8 | Final conformance & integration audit | **Completed and verified** (deterministic property fuzzing: 140,661 checks, 0 defects; complete evidence audit) |

---

## 2. KT-4: Kernel Containment & VFS Integration

### 2.1 VFS Facade Boundaries

The VFS facade (`OpenWindows/vfs/vfs.c`) is the kernel's file-operation entry
point. Every public file operation now admits names through SKTO before passing
them to storage:

| Function | Integration |
|---|---|
| `OwVfsCreateFile` | Calls `OwSktoNameAdmitTrusted`; refuses on any SKTO error |
| `OwVfsWriteFile` | Calls `OwSktoNameAdmitTrusted`; refuses on any SKTO error |
| `OwVfsReadFile` | Calls `OwSktoNameAdmitTrusted`; refuses on any SKTO error |
| `OwVfsDeleteFile` | Calls `OwSktoNameAdmitTrusted`; refuses on any SKTO error |
| `OwVfsCreateFileSkto` | Takes pre-admitted `ow_skto_name_t`; publishes best-effort on create |
| `OwVfsWriteFileSkto` | Takes pre-admitted `ow_skto_name_t`; validates bounds |
| `OwVfsReadFileSkto` | Takes pre-admitted `ow_skto_name_t`; consumes snapshot when present, falls back to direct read |
| `OwVfsDeleteFileSkto` | Takes pre-admitted `ow_skto_name_t`; retires name on delete |
| `OwVfsPublishName` | Explicitly publishes admitted name into the VFS namespace |
| `OwVfsRetireName` / `OwVfsRetireNameSkto` | Withdraws name from lookup index |
| `OwVfsLookupSnapshot` / `OwVfsLookupSnapshotSkto` | Resolves snapshot or verifies on disk |
| `OwVfsReleaseSnapshot` | Releases reader reference on snapshot view |

### 2.2 Catalog Insertion

The legacy helper `OwFsCatalogInsert` previously used `ow_strncpy(e->Name, Name, 223)`,
silently truncating names longer than 223 bytes. It now calls `OwSktoNameAdmitTrusted`,
enforcing that names above the adapter ceiling (127 content bytes + NUL) or with
malformed SUTF-8 are rejected with `OW_ERR_INVALID_PARAM`.

### 2.3 Advisory Publication & Retirement Lifecycle

The 16-slot table capacity ceiling (`SKTO_NAME_CAPACITY = 16`) was addressed
architecturally without inventing artificial capacity increases:
1. **Advisory VFS index:** `OwVfsCreateFileSkto` publishes names on successful
   creation. If the table is full, publication returns `SKTO_E_FULL`, which the
   VFS facade treats as advisory (`OW_ERR_INSUFFICIENT` internally), allowing
   the underlying file creation to succeed.
2. **Snapshot consumption on read:** `OwVfsReadFileSkto` and the dispatcher's
   `OW_SYS_VFS_READ` look up the snapshot via `OwVfsLookupSnapshotSkto`. If
   present, the read is performed using the immutable snapshot bytes; if absent
   or full, it falls back to the direct path.
3. **Retirement on delete:** `OwVfsDeleteFileSkto` calls `OwVfsRetireNameSkto`,
   withdrawing the name from lookup immediately while allowing existing readers
   holding snapshots to release them safely.

### 2.4 Containment

User-space pointers reaching `syscall/dispatcher.c` are validated by `OwSktoNameAdmitUser`
via the containment callback `syscall_name_range()`, backed by `OwMemInUserWindow()`
and `OwMemFindVad()`. The exact memory page is validated before any byte is read,
preventing kernel faults from invalid user pointers.

---

## 3. KT-5: Kernel Synchronization & Lifecycle

### 3.1 Shutdown, Drain, Destroy, and Retirement

`skto_name_table_t` provides comprehensive lifecycle management:
- `suas_skto_names_shutdown()`: Marks the namespace as shutting down. Further
  publications and resolves return `SKTO_E_POLICY`.
- `suas_skto_names_drain()`: Reports outstanding reader references across all slots.
  Returns `SKTO_OK` when 0 remain, or `SKTO_E_BUSY` when references are held.
- `suas_skto_names_destroy()`: Verifies reader count is 0 before zeroing memory.
  Saves lock release callbacks, clears table, and releases the lock safely.
- `suas_skto_retire()`: Withdraws an equal name from lookup immediately. Defers
  zeroing/reclamation until all readers holding snapshots have released.

### 3.2 Generation Exhaustion

A safe non-wrapping generation policy was implemented: reaching `0xFFFFFFFFu`
halts publication with `SKTO_E_OVERFLOW`, preventing ABA alias hazards without
breaking the 32-bit `generation` ABI.

### 3.3 Synchronization

Kernel table synchronization is provided by `ow_skto_irq_lock_t` callbacks
(`OwSktoLockAcquire` / `OwSktoLockRelease`) wrapping `ow_irq_save()` and
`ow_irq_restore()`. In the host test harness, these are no-ops; concurrency
guarantees on the host are verified using Win32 `CRITICAL_SECTION` tests.

---

## 4. KT-6: QEMU Runtime Integration & Ring 3 Round-Trip

### 4.1 Real CPL3 Ring 3 `int 0x80` VFS Round-Trip

To establish genuine runtime evidence of user-to-kernel name boundary
containment, `OpenWindows/usermode/usermode.c` was extended to perform a live
CPL3 round-trip over `int 0x80`:
1. The kernel provisions `r3vfs.txt` and assigns ownership to the unprivileged
   process `user-hello` (UID 1000, GID 1000).
2. `dispatcher.c` was updated for `OW_SYS_VFS_CREATE`, `OW_SYS_VFS_WRITE`, and
   `OW_SYS_VFS_DELETE` to swap ambient identity to the calling process's
   credentials (`proc->Uid` / `proc->Gid`), ensuring real permission checks.
3. The CPL3 payload executes in userspace:
   - Writes `"SKTO"` to `r3vfs.txt` via syscall `0x04` (`OW_SYS_VFS_WRITE`).
   - Reads back the file via syscall `0x05` (`OW_SYS_VFS_READ`), verifying the
     byte count (4) and little-endian payload (`0x4F544B53`).
   - Attempts to create an existing file (`OW_SYS_VFS_CREATE`), expecting refusal.
   - Attempts to read a nonexistent file (`r3missing.txt`), expecting negative error.
   - Attempts to delete a root-owned file, expecting refusal under UID 1000.
   - Emits `"Ring 3 VFS: OK\r\n"` via UART on complete round-trip success.

### 4.2 QEMU Test Results

```
make CIS_DEV_TRUST=1 qemu
```
- **34/34 checks PASSED** (including `[USER] standalone Ring 3 hello application queued`,
  `Hello from Ring 3!`, and `Ring 3 VFS: OK`).
- ACPI S5 clean shutdown achieved.

```
make CIS_DEV_TRUST=1 qemu-emergency
```
- **8/8 checks PASSED** (emergency State 2 boot with `owinitv.owx` spawning rescue `owrs.owx`).

---

## 5. KT-7: Cross-Platform Compatibility & Sanitizers

### 5.1 Struct Layout Verification

Layout assertions are validated in three suites:

| Struct | 64-bit Size | 32-bit Size | Invariant |
|---|---|---|---|
| `skto_text_t` | 40 bytes | 24 bytes | Offsets natural, endian-neutral |
| `skto_limits_t` | 16 bytes | 8 bytes | Explicit ceilings |
| `skto_name_snapshot_t` | 144 bytes | 144 bytes | Fixed-width (independent of ptr width) |
| `skto_name_slot_t` | 580 bytes | 580 bytes | Fixed-width, `retired` at offset 578 |
| `skto_lock_ops_t` | 24 bytes | 12 bytes | 3 pointers |
| `skto_name_table_t` | 9312 bytes | 9296 bytes | 16 slots + state + lock ops |
| `ow_skto_name_store_t` | 9328 bytes | 9304 bytes | Table + limits |
| `ow_skto_name_t` | 168 bytes | 152 bytes | 128 bytes storage + `skto_text_t` |

### 5.2 Real 32-bit Compilation & Execution

`tests/test_skto_32bit.c` was created and built directly using Visual Studio 2022 /
v141 x86 MSVC (`cl.exe /nologo /W4 /WX /Iinclude`):
- Result: **SKTO 32-bit ABI: all checks passed** (exit code 0).
- Verifies ILP32 widths, struct offsets, snapshot preservation across retirement,
  and capacity ceiling enforcement.

### 5.3 AddressSanitizer & UndefinedBehaviorSanitizer

Both `tests/test_suas_skto.c`, `tests/test_skto_fuzz.c`, and `compat/test_compat_skto_name.c`
were compiled and executed under Clang 20.1.8 with `-fsanitize=address,undefined`:
- **Defect detected and fixed:** ASan identified a `stack-use-after-scope` in
  `test_suas_skto.c` where a temporary buffer `nbuf` was published after its block
  scope exited. The test scope was corrected.
- **ASan & UBSan outcome:**
  - `test_suas_skto_asan.exe`: Passed, 0 errors.
  - `test_fuzz_asan.exe`: Passed (140,661 checks over 20,000 iterations), 0 errors.
  - `test_compat_asan.exe`: Passed, 0 errors.

---

## 6. KT-8: Conformance & Property Fuzzing

### 6.1 Deterministic Property Fuzz Harness

`tests/test_skto_fuzz.c` executes deterministic randomized testing using xorshift32:
- **Property A (Admission totality & honesty):** Random byte streams across varying
  capacities and lengths. Validates that `suas_skto_admit` never crashes, over-reads,
  or leaves partial objects; output objects always match input byte-for-byte.
- **Property B (Interleaved lifecycle modeling):** Arbitrary interleavings of
  `publish`, `resolve`, `release`, and `retire` modeled against contract invariants.
- **Result:** **140,661 checks passed over 20,000 iterations** (seed `0x5EED1234`).

### 6.2 Full Conformance Matrix

| Requirement | Clause | Evidence | Status |
|---|---|---|---|
| Bounded name admission | SUAS-018 §7 | `test_suas_skto::test_admission` | Verified |
| No silent truncation | SUAS-018 §7 N2 | `test_compat_skto_name::test_ceiling_enforced` | Verified |
| Explicit failure handling | SUAS-018 §7.1 | `test_invalid_input_refused` | Verified |
| Immutable snapshots | SUAS-018 §9 | `test_publication`, `test_reader_lifetime` | Verified |
| Reader acquire/release | SUAS-018 §9 | `test_reader_lifetime`, `test_capacity_and_reclamation` | Verified |
| Generation validation | SUAS-018 §9 | `test_release_validation`, `test_resolve_at` | Verified |
| Retirement without invalidation | SUAS-018 ext | `test_retirement`, `host_skto_name` | Verified |
| Containment validation | SUAS-018 §7.1 step 3 | `test_vad_containment`, `dispatcher.c` | Verified |
| Real Ring 3 syscall interop | Kernel integration | QEMU `Ring 3 VFS: OK` | Verified (QEMU) |
| Non-wrapping generation exhaustion | SUAS-018 §9 | `test_generation_exhaustion` | Verified |
| 64-bit ABI stability | SUAS-018 §4.1 | `test_abi_layout` | Verified |
| 32-bit ABI stability | SUAS-018 §4.1 | `test_skto_32bit.exe` (MSVC x86) | Verified |
| Memory safety (ASan/UBSan) | Implementation audit | Clang `-fsanitize=address,undefined` | Verified |
| Property / Fuzz totality | Implementation audit | `test_skto_fuzz` (140k checks) | Verified |

---

## 7. Full Test Results Summary

| Suite / Target | Command | Result |
|---|---|---|
| `superunicode` root CTest | `ctest --test-dir build` | **59/59 passed** (100%) |
| `superunicode` verify-docs | `cmake --build build --target verify-docs` | **47/47 docs verified** |
| `superunicode` compat CTest | `ctest --test-dir compat/build` | **5/5 passed** (100%) |
| `superunicode` 32-bit MSVC | `cl /W4 /WX test_skto_32bit.c ...` | **All checks passed** |
| `superunicode` Clang ASan/UBSan | `clang -fsanitize=address,undefined ...` | **Clean, 0 errors** |
| `superunicode` Property Fuzz | `test_skto_fuzz.exe` | **140,661 checks passed** |
| OpenWindows hosttest | `make hosttest` | **1110 passed, 0 failed** (130 SKTO checks) |
| OpenWindows QEMU boot | `make CIS_DEV_TRUST=1 qemu` | **34/34 checks PASSED** |
| OpenWindows QEMU emergency | `make CIS_DEV_TRUST=1 qemu-emergency` | **8/8 checks PASSED** |
| Git whitespace check | `git diff --check` | **Clean across all repos** |

---

## 8. Limitations & Boundary Scope

1. **Advisory index capacity:** `SKTO_NAME_CAPACITY` remains 16. The VFS facade
   uses this as an advisory lookup cache and falls back to disk verification on
   saturation.
2. **SMP synchronization:** Kernel synchronization uses single-CPU interrupt
   masking (`ow_irq_save`/`restore`). Multi-core SMP spinlocks are not claimed.
3. **Normalization:** As specified by SUAS-018, SKTO performs byte-exact
   validation; codepoint canonicalization remains the responsibility of the
   individual namespace policy.

---

## 9. Conclusion

The KT-4 through KT-8 roadmap is complete with source-level and runtime evidence:
- SKTO name boundaries are enforced across all VFS and syscall paths.
- Advisory publication, snapshot resolution, and retirement on delete are implemented and active.
- Real userspace CPL3 `int 0x80` round-trips are proven in QEMU.
- 32-bit and 64-bit ABIs, ASan/UBSan memory safety, and property fuzzing are verified.
- All changes remain uncommitted across both repositories.
