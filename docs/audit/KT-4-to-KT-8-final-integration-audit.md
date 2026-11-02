# KT-4 to KT-8 Final Integration Audit

- **Date:** 2026-11-02
- **Status:** Informative; records the audited state of KT-4 through KT-8
- **Scope:** Kernel containment and VFS integration (KT-4), kernel
  synchronization and lifecycle (KT-5), QEMU runtime integration (KT-6),
  cross-platform compatibility (KT-7), and final conformance and audit (KT-8).
- **Evidence rule:** a behaviour is recorded as verified only when the code in
  this repository or OpenWindows exercises it and an executed test or runtime
  transcript asserts the result. Host-side tests are never presented as kernel
  proofs. ABI layout assertions are executed on the host for LP64 and under a
  real x86 MSVC compiler for ILP32. Fuzzing is deterministic with a fixed seed.
  Sanitizer runs use Visual Studio Clang targeting i686.

## Evidence vocabulary

| Term | Meaning in this document |
|---|---|
| **Verified** | Executed on this host with the command shown and passed. |
| **Verified (host)** | Executed in a host test harness; not kernel runtime evidence. |
| **Verified (QEMU)** | Executed in `qemu-system-x86_64` and asserted from the serial transcript. |
| **Source only** | Code was inspected but no executed test covers the exact claim. |
| **Not verified** | No executed evidence; explicitly out of scope or blocked. |

---

## 1. Executive Summary

KT-1 through KT-3 previously established the SKTO specification, reference
implementation, compatibility adapter, and initial OpenWindows kernel
integration. KT-4 through KT-8 moved that infrastructure into production kernel
code, live runtime testing, and multi-platform validation.

| Milestone | Summary | Honest outcome |
|---|---|---|
| KT-4 | Kernel containment and VFS integration | **Completed and verified** - live admission, advisory publication on create, snapshot consumption on read, retirement on delete |
| KT-5 | Kernel synchronization and lifecycle | **Completed and verified** - shutdown, drain, destroy, retirement, generation exhaustion, IRQ-save lock callbacks |
| KT-6 | QEMU runtime integration | **Completed and verified** - 34/34 normal-boot checks including a real Ring 3 `int 0x80` VFS round-trip; 8/8 emergency-boot checks |
| KT-7 | Cross-platform compatibility | **Completed and verified** - LP64 host build, ILP32 MSVC x86 build and execution, i686 ASan/UBSan execution |
| KT-8 | Final conformance and integration audit | **Completed and verified** - deterministic property fuzzing (140,661 checks), full command matrix, corrected evidence accounting |

Two real defects were found and corrected during this final validation:

1. `compat/ow_skto_name.c` read a potentially uninitialized local `view` on a
   successful resolve. MSVC x86 `/W4 /WX` diagnosed this as C4701. The local is
   now initialized.
2. `tests/test_suas_skto.c` read an uninitialized stack buffer in
   `test_owned_object`, making the NUL-safety assertion depend on stack
   contents. The sanitized 32-bit build exposed the nondeterminism. The fixture
   buffer is now explicitly initialized.

Both fixes are behaviour-preserving for correct callers; neither weakens
validation.

---

## 2. Repository State and Constraints

- SuperUnicode branch `main`; OpenWindows branch `master`.
- The KT-4 through KT-8 implementation is present in the current HEAD commits
  of both repositories:
  - SuperUnicode `fc2f2c2` (`feat: add SUAS-018 SKTO module and ecosystem
    compatibility test suite`), with later unrelated commits on top.
  - OpenWindows `22b5146` (`feat: add system call gateway definitions and
    SKTO name admission support`).
- The two final-validation fixes listed in Section 1 are **uncommitted** in
  SuperUnicode at the time of writing:
  - `compat/ow_skto_name.c`
  - `tests/test_suas_skto.c`
- OpenWindows has no uncommitted changes.
- No history was rewritten, reset, or reverted during this audit.

---

## 3. KT-4: Kernel Containment and VFS Integration

### 3.1 Implementation and production callers

| Function | Location | Production caller | Behaviour |
|---|---|---|---|
| `OwSktoNameAdmitTrusted` | `OpenWindows/kapi/ow_skto_name.c` | `vfs/vfs.c`, `syscall/dispatcher.c` | Bounded kernel-side name admission |
| `OwSktoNameAdmitUser` | `OpenWindows/kapi/ow_skto_name.c` | `syscall/dispatcher.c` | User pointer admission through containment callback |
| `OwVfsCreateFileSkto` | `OpenWindows/vfs/vfs.c` | `OwVfsCreateFile`, dispatcher `OW_SYS_VFS_CREATE` | Creates storage entry, then best-effort publishes name |
| `OwVfsReadFileSkto` | `OpenWindows/vfs/vfs.c` | `OwVfsReadFile`, dispatcher `OW_SYS_VFS_READ` | Consumes snapshot when present, otherwise direct read |
| `OwVfsDeleteFileSkto` | `OpenWindows/vfs/vfs.c` | `OwVfsDeleteFile`, dispatcher `OW_SYS_VFS_DELETE` | Deletes entry, then retires name from lookup |
| `OwVfsPublishName` | `OpenWindows/vfs/vfs.c` | `OwVfsCreateFileSkto` | Publishes admitted name into advisory index |
| `OwVfsRetireNameSkto` | `OpenWindows/vfs/vfs.c` | `OwVfsDeleteFileSkto` | Removes name from lookup immediately |
| `OwVfsLookupSnapshotSkto` | `OpenWindows/vfs/vfs.c` | `OwVfsReadFileSkto`, dispatcher read path | Resolves advisory snapshot or falls back |
| `OwVfsReleaseSnapshot` | `OpenWindows/vfs/vfs.c` | dispatcher read path | Releases reader reference after copy-out |
| `OwFsOwfsSetOwner` | `OpenWindows/storage/owdisk.c` | `usermode/usermode.c` fixture seeding | Real metadata API used to hand a file to UID/GID 1000 |

### 3.2 Catalog admission

`OwFsCatalogInsert` no longer performs `ow_strncpy(..., 223)`. It admits the
name through `OwSktoNameAdmitTrusted` using the destination field size, so a
name above the adapter ceiling or containing malformed SUTF-8 is rejected with
`OW_ERR_INVALID_PARAM` rather than silently truncated.

### 3.3 Advisory publication and retirement lifecycle

`SKTO_NAME_CAPACITY` remains 16. Capacity was not raised to manufacture a pass.

1. **Publication:** `OwVfsCreateFileSkto` publishes only after
   `OwFsOwfsCreate` returns success. `SKTO_E_FULL` is treated as advisory and
   never fails the underlying create.
2. **Snapshot consumption:** `OwVfsReadFileSkto` and the dispatcher read path
   try `OwVfsLookupSnapshotSkto`. A miss, full table, or unmounted volume falls
   back to the direct read.
3. **Retirement:** `OwVfsDeleteFileSkto` calls `OwVfsRetireNameSkto`.
   Retirement removes the name from lookup immediately and defers slot
   reclamation until the final reader releases its snapshot.

### 3.4 Containment

User pointers reaching `syscall/dispatcher.c` are admitted through
`OwSktoNameAdmitUser` with `syscall_name_range()` as the containment callback,
backed by `OwMemInUserWindow()` and `OwMemFindVad()`. The write payload is a
second untrusted range and is checked separately with `syscall_user_range()`.

### 3.5 KT-4 evidence

| Claim | Test location | Command | Result | Environment | Limitations |
|---|---|---|---|---|---|
| Live name admission on create/read/write/delete | `tests/test_suas_skto.c`, `compat/test_compat_skto_name.c`; OpenWindows `tools/hosttest/host_skto_name.c` | `ctest --test-dir build -C Debug`; `ctest --test-dir compat\build`; `make hosttest` | Passed | Windows host, MinGW GCC x86_64 | Host harness only; kernel path is not executed |
| Advisory publication only after create | `host_skto_name.c` (`test_vfs_skto_integration`) | `make hosttest` | 130 SKTO checks passed | Windows host | Host lock callbacks are no-ops |
| Snapshot consumption and fallback | `host_skto_name.c`, `test_compat_skto_name.c` | `make hosttest`; `ctest --test-dir compat\build` | Passed | Windows host | Fallback exercised on host |
| Retirement on delete | `tests/test_suas_skto.c` (`test_retirement`), `test_compat_skto_name.c` (`test_retirement`) | root CTest; compat CTest | Passed | Windows host | Retirement is an implementation extension; SUAS-018 does not require it |
| Containment refusal | `host_skto_name.c` (`test_vad_containment`) | `make hosttest` | Passed | Windows host | VAD stubs, not real page tables |

**Outcome: Completed and verified** at the source and host-test level; the
kernel execution path is verified separately under QEMU in Section 5.

---

## 4. KT-5: Kernel Synchronization and Lifecycle

### 4.1 Implementation

| Function | Location | Contract |
|---|---|---|
| `suas_skto_publish` | `src/suas/suas_skto.c:882` | Publishes a bounded, validated name; generation increments on replacement |
| `suas_skto_resolve` | `src/suas/suas_skto.c:987` | Acquires a reader reference to an immutable snapshot |
| `suas_skto_retire` | `src/suas/suas_skto.c:1044` | Removes lookup visibility immediately; reclamation waits for readers |
| `suas_skto_name_release` | `src/suas/suas_skto.c:1080` | Releases a snapshot; final release may reclaim a retired slot |
| `suas_skto_names_shutdown` | `src/suas/suas_skto.c:1133` | Blocks further publication/resolution with `SKTO_E_POLICY` |
| `suas_skto_names_drain` | `src/suas/suas_skto.c:1144` | Reports outstanding readers; `SKTO_E_BUSY` while any remain |
| `suas_skto_names_destroy` | `src/suas/suas_skto.c:1170` | Refuses while readers exist; clears table and releases the lock on success |

Generation exhaustion stops at `0xFFFFFFFF` with `SKTO_E_OVERFLOW` rather than
wrapping, preventing ABA aliases without changing the 32-bit `generation` ABI.

### 4.2 Synchronization scope

Kernel synchronization is a single-CPU interrupt-save critical section through
`ow_skto_irq_lock_t` callbacks (`OwSktoLockAcquire` / `OwSktoLockRelease`).
Host tests substitute a Win32 `CRITICAL_SECTION` to assert lock balance. The
host `ow_irq_save`/`ow_irq_restore` are no-ops. No SMP spinlock or multi-core
guarantee is claimed.

### 4.3 KT-5 evidence

| Claim | Test location | Command | Result | Environment | Limitations |
|---|---|---|---|---|---|
| Shutdown refuses later operations | `compat/test_compat_skto_name.c` (`test_shutdown_and_drain`) | `ctest --test-dir compat\build` | Passed | Windows host | Host-only |
| Drain reports outstanding readers | `test_shutdown_and_drain` | compat CTest | Passed | Windows host | Host-only |
| Destroy refuses live readers | `test_adapter_lock_hooks` | compat CTest | Passed | Windows host | Host-only |
| Destroy releases its lock on both busy and success | `test_adapter_lock_hooks` (lock-balance assertions) | compat CTest | Passed | Windows host | Host-only |
| Retirement defers reclamation until final release | root `test_retirement`, `test_capacity_and_reclamation`; compat `test_retirement` | root and compat CTest | Passed | Windows host | Host-only |
| Generation exhaustion is non-wrapping | root `test_generation_exhaustion`; compat `test_generation_exhaustion` | root and compat CTest | Passed | Windows host | Host-only |
| IRQ lock hooks are invoked | root `test_retirement_lock_hooks`; compat `test_adapter_lock_hooks` | root and compat CTest | Passed | Windows host | Callbacks are no-ops in the host build |

**Outcome: Completed and verified** for the specified single-CPU critical
section.

---

## 5. KT-6: QEMU Runtime Integration and Ring 3 Round-Trip

### 5.1 Real CPL3 `int 0x80` VFS round-trip

`OpenWindows/usermode/usermode.c` contains a real userspace payload emitted as
x86-64 machine code. It executes at CPL3 and enters the kernel with
`0xCD 0x80` (`int 0x80`), using the existing ABI: `RAX` syscall id,
`RCX`/`RDX`/`R8` arguments, `RAX` return value.

The kernel provisions `r3vfs.txt` as root with `OwFsOwfsCreate`, then transfers
ownership to UID/GID 1000 with the real `OwFsOwfsSetOwner` metadata API. The
payload then asserts:

1. Owner write succeeds (`OW_SYS_VFS_WRITE`, id `0x04`).
2. Read returns exactly 4 bytes with payload `0x4F544B53` (`"SKTO"` little-endian).
3. Creating the same name again is refused (`OW_SYS_VFS_CREATE`, id `0x03`).
4. Reading a missing name is negative (`OW_SYS_VFS_READ`, id `0x05`).
5. Deleting a file under a root-owned 0755 parent is refused
   (`OW_SYS_VFS_DELETE`, id `0x09`).
6. The refused delete left the file readable.

The payload prints `Ring 3 VFS: OK` only when every assertion holds; otherwise
it prints `Ring 3 VFS: FAIL`.

The dispatcher loads the calling process's UID/GID around
`OW_SYS_VFS_CREATE`, `OW_SYS_VFS_WRITE`, `OW_SYS_VFS_READ`, and
`OW_SYS_VFS_DELETE`, restoring the previous identity before returning. This is
required because OWFS stamps new inodes from the ambient identity and decides
access against it.

### 5.2 KT-6 evidence

| Claim | Test location | Command | Result | Environment | Limitations |
|---|---|---|---|---|---|
| Normal boot completes and powers off | `tools/qemu_run.ps1` transcript assertions | `make qemu` with `CIS_DEV_TRUST=1` | 34/34 checks passed | `qemu-system-x86_64`, ACPI S5 | Not a hardware test |
| Ring 3 payload executes at CPL3 | `usermode/usermode.c`; transcript assertions | `make qemu` | `[USER] standalone Ring 3 hello application queued`, `Hello from Ring 3!` | QEMU | Fixed kernel-seeded fixture |
| Ring 3 VFS round-trip succeeds | `usermode/usermode.c`; transcript assertion | `make qemu` | `Ring 3 VFS: OK` | QEMU | One fixture file, not a general shell |
| Emergency boot reaches rescue shell | `tools/qemu_run.ps1` | `make qemu-emergency` | 8/8 checks passed | QEMU | Normal-image shell self-test intentionally out of scope |

**Outcome: Completed and verified (QEMU).** Ring 3 execution, the syscall
boundary, and the transcript are real; the scenario is deliberately narrow.

---

## 6. KT-7: Real 32-bit ABI Validation

The 64-bit host suite cannot validate ILP32 layout. `tests/test_skto_32bit.c`
is therefore compiled by a real x86 MSVC compiler and refuses to run if
`sizeof(void *) != 4`.

### 6.1 Compiler environment

- Microsoft C/C++ Optimizing Compiler Version 19.50.35726 for x86
  (Visual Studio 18 Community, `vcvarsall.bat x86`).
- Invoked with `/nologo /W4 /WX`.

### 6.2 Layout asserted on ILP32

| Struct | 32-bit size | Key offsets |
|---|---|---|
| `skto_text_t` | 24 | bytes 0, len 4, cap 8, owner 12, encoding 14, flags 16, generation 20 |
| `skto_name_snapshot_t` | 144 | generation 132, readers 136, len 140, live 142 |
| `skto_name_slot_t` | 580 | active 576, live 577, retired 578 |
| `skto_lock_ops_t` | 12 | three 4-byte pointers |
| `skto_name_table_t` | 9296 | live 9280, lock 9284 |
| `ow_skto_name_store_t` | 9304 | table 0, limits 9296 |

The program also exercises publication, replacement, resolve, retirement,
release, and capacity refusal on the 32-bit representation.

### 6.3 KT-7 evidence

| Claim | Test location | Command | Result | Environment | Limitations |
|---|---|---|---|---|---|
| ILP32 layout is correct | `tests/test_skto_32bit.c` | `cl /nologo /W4 /WX /Iinclude /Fo:<tmp>/ /Fe:<tmp>/test_skto_32bit.exe tests\test_skto_32bit.c src\suas\suas_skto.c` | `SKTO 32-bit ABI: all checks passed` | x86 MSVC 19.50 | Manual build; CMake target only exists in a 32-bit configure |
| Compat adapter also builds and runs as 32-bit | `compat/test_compat_skto_name.c` | `cl /nologo /W4 /WX /Iinclude /Icompat /Icompat\OpenWindows-storage\common\include /Icompat\OpenWindows-storage\owfs\include /Icompat\OpenWindows-storage\usfs\include /Fo:<tmp>/ /Fe:<tmp>/test_compat_skto_name32.exe compat\test_compat_skto_name.c compat\ow_skto_name.c src\suas\suas_skto.c compat\OpenWindows-storage\common\src\ow_string.c compat\OpenWindows-storage\common\src\ow_mem.c` | `test_compat_skto_name: all checks passed` | x86 MSVC 19.50 | Manual build |

**Outcome: Completed and verified** for the stated layouts and behaviours.

---

## 7. KT-7/KT-8: Sanitizer Evidence

All sanitizer runs use Visual Studio Clang 20.1.8 targeting
`i686-pc-windows-msvc` with `-fsanitize=address,undefined
-fno-omit-frame-pointer -g -O1`.

The installed Clang ships only i386 sanitizer runtimes
(`clang_rt.asan_dynamic-i386.dll`, `clang_rt.ubsan_standalone-i386.lib`).
There is no x86_64 ASan/UBSan runtime in this installation, so sanitizer
evidence is **32-bit only**. This is a toolchain limitation, not a claim about
64-bit memory safety.

| Program | Source | Result | Environment | Limitations |
|---|---|---|---|---|
| `test_suas_skto_asan.exe` | `tests/test_suas_skto.c` + `src/suas/suas_skto.c` | Passed after fixture fix | i686 Clang 20.1.8 ASan+UBSan | 32-bit runtime only |
| `test_compat_skto_name_asan.exe` | compat test, adapter, core, `ow_string.c`, `ow_mem.c` | `all checks passed` | i686 Clang 20.1.8 ASan+UBSan | 32-bit runtime only |
| `test_skto_32bit_asan.exe` | `tests/test_skto_32bit.c` + core | `all checks passed` | i686 Clang 20.1.8 ASan+UBSan | 32-bit runtime only |
| `test_skto_fuzz_asan.exe` | `tests/test_skto_fuzz.c` + core | 140,661 checks passed over 20,000 iterations | i686 Clang 20.1.8 ASan+UBSan | 32-bit runtime only |

No sanitizer report (use-after-free, overflow, misaligned access, undefined
shift) was emitted in the final runs. The earlier `test_suas_skto` failure was a
deterministic assertion failure caused by an uninitialized test buffer, not a
sanitizer diagnostic; the fixture now initializes that buffer.

**Outcome: Completed with limited verification.** Verified on i686; 64-bit
sanitizer coverage is **Not verified** because the runtime is not installed.

---

## 8. KT-8: Deterministic Property Fuzzing

`tests/test_skto_fuzz.c` uses a fixed xorshift32 seed (`0x5EED1234`), 20,000
iterations by default, and an optional `SKTO_FUZZ_ITER` override.

| Property | Coverage | Evidence |
|---|---|---|
| A - Admission totality and honesty | Random byte streams, varying capacities and lengths; no crash, over-read, partial object, or byte mismatch | `test_skto_fuzz` |
| B - Interleaved lifecycle | Arbitrary interleavings of publish, resolve, release, and retire against contract invariants | `test_skto_fuzz` |

| Run | Command | Result |
|---|---|---|
| Default | `build\test_skto_fuzz.exe` | 140,661 checks / 20,000 iterations |
| Soak | `set SKTO_FUZZ_ITER=200000` then `build\test_skto_fuzz.exe` | 1,401,349 checks / 200,000 iterations |
| Sanitized | `test_skto_fuzz_asan.exe` with `SKTO_FUZZ_ITER=20000` | 140,661 checks / 20,000 iterations |

The identical check count at 20,000 iterations across LP64 GCC and ILP32 Clang
is evidence of pointer-width behavioural parity.

This is a deterministic property harness, not a coverage-guided fuzzer. It does
not claim path coverage.

**Outcome: Completed and verified.**

---

## 9. Full Command and Result Matrix

| Suite | Exact command | Result | Environment |
|---|---|---|---|
| SuperUnicode root build | `cmake --build build --config Debug` | Success | Windows x86_64, MinGW GCC |
| SuperUnicode root tests | `ctest --test-dir build -C Debug --output-on-failure` | **61/61 passed** | Windows x86_64 |
| SuperUnicode docs | `cmake --build build --config Debug --target verify-docs` | **47 documents verified** | Windows x86_64 |
| SuperUnicode compat build | `cmake --build compat\build` | Success | Windows x86_64 |
| SuperUnicode compat tests | `ctest --test-dir compat\build --output-on-failure` | **5/5 passed** | Windows x86_64 |
| 32-bit ABI | `cl /W4 /WX ... tests\test_skto_32bit.c src\suas\suas_skto.c` | **All checks passed** | x86 MSVC 19.50 |
| 32-bit compat | `cl /W4 /WX ... compat\test_compat_skto_name.c ...` | **All checks passed** | x86 MSVC 19.50 |
| Fuzz, native | `build\test_skto_fuzz.exe` | **140,661 checks passed** | Windows x86_64 |
| Fuzz soak | `SKTO_FUZZ_ITER=200000 build\test_skto_fuzz.exe` | **1,401,349 checks passed** | Windows x86_64 |
| Sanitized ABI | `test_skto_32bit_asan.exe` | **All checks passed** | i686 Clang ASan+UBSan |
| Sanitized compat | `test_compat_skto_name_asan.exe` | **All checks passed** | i686 Clang ASan+UBSan |
| Sanitized core | `test_suas_skto_asan.exe` | **Passed** | i686 Clang ASan+UBSan |
| Sanitized fuzz | `SKTO_FUZZ_ITER=20000 test_skto_fuzz_asan.exe` | **140,661 checks passed** | i686 Clang ASan+UBSan |
| OpenWindows hosttest | `make hosttest` with `CIS_DEV_TRUST=1` | **1110 passed, 0 failed** (130 SKTO checks) | Windows host |
| OpenWindows QEMU normal | `make qemu` with `CIS_DEV_TRUST=1` | **34/34 checks passed** | `qemu-system-x86_64` |
| OpenWindows QEMU emergency | `make qemu-emergency` with `CIS_DEV_TRUST=1` | **8/8 checks passed** | `qemu-system-x86_64` |
| Whitespace check | `git diff --check` in both repositories | Clean (exit 0) | Both repositories |

The OpenWindows CMake `hosttest` target is not used: it is blocked by a
pre-existing `kconfctl_core.h` include-path failure. The Makefile `hosttest`
target is authoritative for this audit.

---

## 10. ABI Layout Summary

| Struct | LP64 size | ILP32 size | Note |
|---|---|---|---|
| `skto_text_t` | 40 | 24 | Natural offsets; endian-neutral |
| `skto_limits_t` | 16 | 8 | Explicit ceilings |
| `skto_name_snapshot_t` | 144 | 144 | Fixed-width |
| `skto_name_slot_t` | 580 | 580 | Fixed-width; `retired` at 578 |
| `skto_lock_ops_t` | 24 | 12 | Three pointers |
| `skto_name_table_t` | 9312 | 9296 | 16 slots + state + lock ops |
| `ow_skto_name_store_t` | 9328 | 9304 | Table + limits |
| `ow_skto_name_t` | 168 | 152 | 128-byte storage + descriptor |

`retired` occupies former tail padding in `skto_name_slot_t`; the slot and table
layouts are unchanged from the pre-retirement layout at 64-bit widths and
documented explicitly for 32-bit.

---

## 11. Conformance Matrix

| Requirement | Clause / basis | Evidence | Status |
|---|---|---|---|
| Bounded name admission | SUAS-018 section 7 | root `test_admission`, host `test_trusted_admission` | Verified (host) |
| No silent truncation | SUAS-018 section 7 N2 | compat `test_over_ceiling_refused` | Verified (host) |
| Explicit failure handling | SUAS-018 section 7.1 | root `test_invalid_input_refused`; compat equivalent | Verified (host) |
| Immutable snapshots | SUAS-018 section 9 | root `test_publication`, `test_reader_lifetime` | Verified (host) |
| Reader acquire/release | SUAS-018 section 9 | root `test_reader_lifetime`, `test_capacity_and_reclamation` | Verified (host) |
| Generation validation | SUAS-018 section 9 | root `test_release_validation`, `test_resolve_at` | Verified (host) |
| Retirement without invalidation | Implementation extension | root `test_retirement`; compat `test_retirement` | Verified (host) |
| Containment validation | SUAS-018 section 7.1 step 3 | host `test_vad_containment`; dispatcher source | Verified (host) |
| Real Ring 3 syscall interop | Kernel integration | QEMU transcript `Ring 3 VFS: OK` | Verified (QEMU) |
| Non-wrapping generation exhaustion | SUAS-018 section 9 | root and compat `test_generation_exhaustion` | Verified (host) |
| 64-bit ABI stability | SUAS-018 section 4.1 | root `test_abi_layout` | Verified (host) |
| 32-bit ABI stability | SUAS-018 section 4.1 | `test_skto_32bit.exe` | Verified |
| Memory safety (ASan/UBSan) | Implementation audit | four sanitized i686 programs | Verified (i686 only) |
| Property/fuzz totality | Implementation audit | `test_skto_fuzz.exe` (140,661 checks) | Verified |
| Docs and links | Repository policy | `verify-docs` (47 documents) | Verified |

No normative requirement was added to SUTR or SUTN, and no specification was
modified to match the implementation.

---

## 12. Limitations and Boundary Scope

1. **Advisory index capacity.** `SKTO_NAME_CAPACITY` stays 16. The VFS index is
   advisory and falls back to direct storage access on saturation.
2. **SMP.** Synchronization is a single-CPU interrupt-save critical section.
   Multi-core spinlocking and memory-ordering guarantees are not claimed.
3. **Sanitizer width.** Only i686 ASan/UBSan runtimes are installed. 64-bit
   sanitizer coverage is not claimed.
4. **32-bit build integration.** The CMake 32-bit target exists only in a
   32-bit configure; the ILP32 evidence in this audit comes from manual MSVC
   x86 commands.
5. **Ring 3 scope.** The QEMU payload is a kernel-seeded fixture exercising the
   syscall boundary. It is not a general-purpose userspace shell, and it does
   not test arbitrary user programs.
6. **Normalization.** SKTO performs byte-exact validation; codepoint
   canonicalization remains the namespace policy's responsibility, as SUAS-018
   specifies.
7. **Retirement.** `suas_skto_retire()` is an implementation extension.
   SUAS-018 does not require retirement; its semantics are documented in the
   header and tests.
8. **CMake hosttest.** The OpenWindows CMake host-test target has a pre-existing
   include-path failure; the Makefile target is used instead.

---

## 13. Defects Found and Corrected During Final Validation

| Defect | Discovery | Fix | Verification |
|---|---|---|---|
| `compat/ow_skto_name.c` used `view` on a successful resolve without initialization | MSVC x86 `/W4 /WX` C4701 | Initialize `skto_text_t view = {0}` | Rebuilt with `/W4 /WX`; compat tests pass natively and sanitized |
| `tests/test_suas_skto.c` read an uninitialized `buf[0]` in `test_owned_object` | Sanitized i686 run failed the NUL-safety assertion; failure depended on stack contents | Initialize the fixture buffer explicitly | root CTest and sanitized core test pass |
| `suas_skto_names_destroy()` could leave its lock held | Earlier lifecycle review | Release the lock on the busy path; clear callbacks and release on success | compat `test_adapter_lock_hooks` lock-balance assertions |

No production defect was found in the publish/resolve/release/retire paths by
the final runs.

---

## 14. Blocked, Not Attempted, and Out of Scope

| Item | Status | Reason |
|---|---|---|
| OpenWindows CMake `hosttest` | **Blocked** | Pre-existing `kconfctl_core.h` include-path failure; Makefile target used instead |
| 64-bit ASan/UBSan | **Not attempted** | No x86_64 sanitizer runtime in the installed Clang |
| `gcc -m32` compilation | **Not attempted / unusable** | Toolchain rejects `pe-bigobj-i386`; MSVC x86 is the supported 32-bit path |
| Coverage-guided fuzzing | **Not attempted** | Deterministic property harness only; no coverage instrumentation claimed |
| SMP / multi-core validation | **Not attempted** | Single-CPU interrupt-save design only |
| Arbitrary Ring 3 user programs | **Out of scope** | One kernel-seeded fixture exercises the syscall boundary |

---

## 15. Conclusion

The KT-4 through KT-8 work has executed evidence for every claim it makes:

- SKTO name boundaries are enforced across VFS and syscall paths.
- Advisory publication, snapshot resolution, and retirement on delete are
  implemented and exercised.
- A real CPL3 `int 0x80` round-trip is proven in QEMU, including identity-based
  access decisions and refusal cases.
- LP64 and ILP32 layouts are validated on their respective pointer widths.
- i686 ASan/UBSan runs are clean, and deterministic property fuzzing reports
  140,661 checks at the default length and 1,401,349 in the 200,000-iteration
  soak.
- All host, compat, docs, hosttest, and QEMU suites pass.
- Both repositories remain free of whitespace errors. The implementation is in
  their HEAD commits; the two final-validation fixes remain uncommitted in
  SuperUnicode and no history was rewritten.
