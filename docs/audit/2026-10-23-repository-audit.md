# Repository Audit — SuperUnicode and OpenWindows

- **Date:** 2026-10-23
- **Auditor:** OpenCode (automated source audit)
- **Repositories**
  - `C:\Users\KARIMABENDA\Documents\superunicode` — `main`, HEAD `89daf6f`
  - `C:\Users\KARIMABENDA\Documents\OpenWindows` — `master`, HEAD `10b7a61`
- **Scope:** Verify the claimed SUAS/SUTS/SUTR inventory against actual files and
  build targets; inventory implemented vs. declared-only surface; map OpenWindows
  integration points; propose a dependency-aware plan. No source rewrites in this
  pass.
- **Evidence rule:** A component is recorded as *implemented* only when a real
  translation unit, a public header, a build entry, and an executable test exist.
  A document's existence does not make its requirements normative. Proposed
  components are not implemented until files and tests exist.

---

## 1. Executive summary

1. **The documented module inventory is real.** Every SUAS-001…017 and every
   SUTS document except the deliberate SUTS-010 gap has a public header, a
   freestanding C translation unit, a build entry, and a host test. Seven SUTR
   reports (SUTR-7…013) also have reference implementations and tests. No module
   is documentation-only except the two stub headers described below.
2. **Two headers are declared but unimplemented:** `include/suas/suas_core.h`
   (`suas_kernel_init`, `suas_kernel_version`) and
   `include/suts/suts_module.h` (`suts_register`, `suts_find`,
   `suts_unregister`). They have no `.c`, no build entry, no test, and no
   callers. Their "pending SUAS-001 / SUTS-001" comments are wrong: SUAS-001 is
   SDF and SUTS-001 is SUCA, neither owns a kernel/registry lifecycle.
3. **The two CTest failures are test-harness defects, not library defects.**
   `test_suts_susn` and `test_extsucs_big` use `assert(...)` for calls with
   side effects. The build uses `-O3 -DNDEBUG`, which compiles the calls out and
   leaves objects uninitialized. Both pass when assertions are enabled
   (verified by recompiling with `-UNDEBUG`). Fix the tests, not the libraries.
4. **All other tests pass, and OpenWindows host tests pass.** SuperUnicode:
   50/52 CTest entries pass; the two failures are explained above.
   OpenWindows: `build-cmake2` reports 3/3 (`hosttest`,
   `crypto_reference_model`, `crypto_field_reference`).
5. **OpenWindows integrates almost none of SuperUnicode.** The kernel links only
   `sutf/src/sutf8.c` and `sutf/src/sucs_mode.c`. SUAS, SUTS, SUTR, `sust`,
   `suf`, ExtSUCS/vSUTF, and SUCD are not linked into the kernel.
6. **OpenWindows has a real syscall-boundary security gap.** `VFS_READ` validates
   and copies user pointers, but `VFS_CREATE`, `VFS_WRITE`, `VFS_DELETE`, and
   `ALPC_CREATE/SEND/RECV` pass raw user pointers into kernel routines.
7. **Governance is not yet codified.** No document states that SUAS compliance is
   mandatory for SuperUnicode compliance, that SUTS conformance is optional, or
   that SUTR is informative. No document defines the `S-` component naming rule.
8. **Documentation drift exists but is limited:** stale status paragraphs, a
   `SUTS (SIDN)` typo, an off-by-one count in the SUTR index, and inconsistent
   SUTR number padding.

No permanent document numbers were assigned by this audit.

---

## 2. Repository and build state

### 2.1 SuperUnicode

| Item | Observed |
|---|---|
| Branch / HEAD | `main` / `89daf6f` |
| Preexisting untracked files | `superunicode/Public/0.1.0/ReadMe1st.txt`, `superunicode_extended/Public/0.1.0/ReadMe1st.txt`, chart `.part` downloads |
| Build directory | `build/` (MinGW Makefiles, `C:\w64devkit\bin\cc.exe`) |
| Effective test flags | `-O3 -DNDEBUG -std=c99` |
| Root targets | `suas_static`, `suts_static`, `sutr_static`, subprojects |
| CTest entries | 52 |
| Result | 50 passed, 2 failed (`test_suts_susn`, `test_extsucs_big`) — both harness defects |

The audit made no permanent source changes. Diagnostic edits to
`tests/test_suts_susn.c` were reverted with `git checkout`.

### 2.2 OpenWindows

| Item | Observed |
|---|---|
| Branch / HEAD | `master` / `10b7a61`, clean |
| Working host build | `build-cmake2/` (GCC/MinGW-style host harness) |
| Host tests | `hosttest`, `crypto_reference_model`, `crypto_field_reference` — 3/3 pass |
| SuperUnicode sources linked | `${SUTF_DIR}/src/sutf8.c`, `${SUTF_DIR}/src/sucs_mode.c` only |
| Sibling checkouts | `superunicode`, `vip`, `OpenWindows-Essentials`, `OpenWindows-Storage` present |
| Stale artefact | `hosttest_build.log` records an old Makefile failure (`ow_hal.h`, missing `../BANcode`) that no longer reflects the CMake build |

---

## 3. Verified document inventory

### 3.1 SUAS — Architecture Standards (17 documents)

All 17 are present in `docs/suas/`. Status is taken from `docs/suas/INDEX.md`.

| ID | Module | Status | Header | Source | Test |
|---|---|---|---|---|---|
| SUAS-001 SDF | Structural Directional Framing | Ratified | `suas_sdf.h` | `suas_sdf.c` (995) | `test_suas_sdf.c` |
| SUAS-002 SGW | Glyph Width & Monospace Grid | Ratified | `suas_sgw.h` | `suas_sgw.c` (284) | `test_suas_sgw.c` |
| SUAS-003 SBR | Boundary & Line Break Rules | Ratified | `suas_sbr.h` | `suas_sbr.c` (363) | `test_suas_sbr.c` |
| SUAS-004 SUCF | Canonical Forms | Ratified | `suas_sucf.h` | `suas_sucf.c` (788) | `test_suas_sucf.c` |
| SUAS-005 SSP | Script Property | Ratified | `suas_ssp.h` | `suas_ssp.c` (457) | `test_suas_ssp.c` |
| SUAS-006 STS | Text Segmentation | Ratified | `suas_sts.h` | `suas_sts.c` (1117) | `test_suas_sts.c` |
| SUAS-007 SIS | Identifiers and Syntax | Ratified | `suas_sis.h` | `suas_sis.c` (718) | `test_suas_sis.c` |
| SUAS-008 SNCS | Named Character Sequences | Ratified | `suas_sncs.h` | `suas_sncs.c` (349) | `test_suas_sncs.c` |
| SUAS-009 SIP | Ideographic Properties | Ratified | `suas_sip.h` | `suas_sip.c` (327) | `test_suas_sip.c` |
| SUAS-010 SSRE | Specification Registry & X-Ref | Authoring | `suas_ssre.h` | `suas_ssre.c` (370) | `test_suas_ssre.c` |
| SUAS-011 SCMS | Character Manifest & Schema | Authoring | `suas_scms.h` | `suas_scms.c` (794) | `test_suas_scms.c` |
| SUAS-012 SUCD | Character Database | Authoring | `suas_sucd.h` | `suas_sucd.c` (723) | `test_suas_sucd.c` |
| SUAS-013 SISR | Ideographic Source Registry | Authoring | `suas_sisr.h` | `suas_sisr.c` (709) | `test_suas_sisr.c` |
| SUAS-014 SUVTL | Vertical Text Layout | Authoring | `suas_suvtl.h` | `suas_suvtl.c` (736) | `test_suas_suvtl.c` |
| SUAS-015 SUAMR | Arabic Mark Rendering | Authoring | `suas_suamr.h` | `suas_suamr.c` (801) | `test_suas_suamr.c` |
| SUAS-016 SUKEMET | Egyptian Hieroglyph Database | Authoring | `suas_sukemet.h` | `suas_sukemet.c` (835 + data) | `test_suas_sukemet.c` |
| SUAS-017 SUEASD | East Asian Script Data | Authoring | `suas_sueasd.h` | `suas_sueasd.c` (775 + data) | `test_suas_sueasd.c` |

Line counts are source lines. SUAS implementation total: **11,141** lines across
17 translation units (plus two generated `.inc` data tables).

**Finding:** there is no SUAS document for the kernel/version/registry stubs in
`suas_core.h`. The header is included by most SUAS headers for common constants
and types, but its two functions are unowned and untested.

### 3.2 SUTS — Technical Specifications (10 documents, gap at 010)

| ID | Module | Status | Header | Source | Test |
|---|---|---|---|---|---|
| SUTS-001 SUCA | Collation | Ratified | `suts_suca.h` | `suts_suca.c` (735) | `test_suts_suca.c` |
| SUTS-002 SRE | Regular Expressions | Authoring | `suts_sre.h` | `suts_sre.c` (1319) | `test_suts_sre.c` |
| SUTS-003 SLDF | Locale Data & Formatting | Authoring | `suts_sldf.h` | `suts_sldf.c` (455) | `test_suts_sldf.c` |
| SUTS-004 SIVD | Ideographic Variation DB | Authoring | `suts_sivd.h` | `suts_sivd.c` (326) | `test_suts_sivd.c` |
| SUTS-005 SCSS | Character Security | Authoring | `suts_scss.h` | `suts_scss.c` (440) | `test_suts_scss.c` |
| SUTS-006 SIDN | Domain & Identifier Network | Authoring | `suts_sidn.h` | `suts_sidn.c` (777) | `test_suts_sidn.c` |
| SUTS-007 SUEPS | Emoji & Presentation | Authoring | `suts_sueps.h` | `suts_sueps.c` (627) | `test_suts_sueps.c` |
| SUTS-008 SUSCH | Source Code Handling | Authoring | `suts_susch.h` | `suts_susch.c` (1447) | `test_suts_susch.c` |
| SUTS-009 SULDF | Link Detection & Formatting | Authoring | `suts_suldf.h` | `suts_suldf.c` (3179) | `test_suts_suldf.c` |
| SUTS-010 | **Deliberate gap** | — | — | — | — |
| SUTS-011 SUSN | Set Notation | Authoring | `suts_susn.h` | `suts_susn.c` (1064 + data) | `test_suts_susn.c` |

SUTS implementation total: **10,369** lines. All ten are in `suts_static` and
have a CTest entry. SUTS-010's absence is intentional and documented in the root
`CMakeLists.txt`.

### 3.3 SUTR — Technical Reports (14 documents, 7 reference implementations)

Documents SUTR-0 through SUTR-013 are present at `0.1.0`. Seven carry reference
implementations in `sutr_static`:

| Report | Module | Source | Test |
|---|---|---|---|
| SUTR-7 SUCEM | `sutr_sucem` | `sutr_sucem.c` (490) | `test_sutr_sucem.c` |
| SUTR-8 SUCPM | `sutr_sucpm` | `sutr_sucpm.c` (1428) | `test_sutr_sucpm.c` |
| SUTR-9 SUMNLS | `sutr_sumnls` | `sutr_sumnls.c` (1452) | `test_sutr_sumnls.c` |
| SUTR-10 SUCM | `sutr_sucm` | `sutr_sucm.c` (1604) | `test_sutr_sucm.c` |
| SUTR-11 SUMS | `sutr_sums` | `sutr_sums.c` (817) | `test_sutr_sums.c` |
| SUTR-012 SUCSL | `sutr_sucsl` | `sutr_sucsl.c` (718) | `test_sutr_sucsl.c` |
| SUTR-013 SUEAS | `sutr_sueas` | `sutr_sueas.c` (3025) | `test_sutr_sueas.c` |

SUTR implementation total: **9,534** lines. SUTR-0…6 are document-only in this
inventory (they describe core format/model behavior implemented by the
`superunicode`, `sutf`, `sust`, and `superunicode_extended` subprojects).

### 3.4 Core encoding libraries (outside the three doc families)

| Area | Sources | Notes |
|---|---|---|
| `superunicode/` | `sucs_conv.c`, `sucs_string.c`, `sucs_trap.c`, `sucs_ucd_names.c`, `sutf_encode.c`, `sutf_decode.c` | SUCS/UCD/SUTF base; tests under `superunicode/tests/` |
| `sutf/` | `sutf8.c`, `sutf16.c`, `sutf2.c`, `sutf4.c`, `sucs_mode.c` | SUTF encoding forms; only `sutf8.c` + `sucs_mode.c` used by OpenWindows |
| `sust/` | `esust.c`, `sust16.c`, `sustfixed.c` | Encoding schemes |
| `superunicode_extended/` | `extsucs_big.c`, `extsucs_conv.c`, `extsucs_ucd_names.c`, `vsutf.c` | ExtSUCS/vSUTF; arbitrary-precision path |
| `suf/` | `suf_builder.c`, `suf_conv.c`, `suf_parser.c` | Font format tooling |
| `sutf_convert/`, `unicode_port/` | converters and harnesses | Build conversions |
| `unified/` | `test_unified.c` | Header-coexistence test |

The unified header test includes every public header **except**
`suts/suts_module.h`. That omission is consistent with the registry being a stub,
but it also means the header is not compile-checked in the coexistence suite.

### 3.5 Test corpus

| Family | Files | Total lines |
|---|---|---|
| `tests/test_suas_*.c` | 17 | 7,266 |
| `tests/test_suts_*.c` | 10 | 4,675 |
| `tests/test_sutr_*.c` | 7 | 4,224 |
| Subproject tests / unified | remainder of the 52 CTest entries | — |

All tests use the host CRT for assertions only; the libraries remain
freestanding (`-nostdlib -ffreestanding`).

---

## 4. Declared-but-unimplemented surface

| Symbol | Header | Definition | Build entry | Test | Callers |
|---|---|---|---|---|---|
| `suas_kernel_init` | `suas_core.h` | none | none | none | none |
| `suas_kernel_version` | `suas_core.h` | none | none | none | none |
| `suts_register` | `suts_module.h` | none | none | none | none |
| `suts_find` | `suts_module.h` | none | none | none | none |
| `suts_unregister` | `suts_module.h` | none | none | none | none |

Both headers are otherwise useful: `suas_core.h` defines `suas_status_t`, the
31-bit constants, and zone bounds; `suts_module.h` defines status codes,
capability flags, and the module ops vtable. The functions, however, are
aspirational.

**Action:** either (a) implement and assign ownership to a real SUAS/SUTS
document with tests, or (b) remove the function declarations and retain only the
types/constants that are actually consumed. Do not leave them as apparently
callable API.

---

## 5. Build and test findings

### 5.1 The two CTest failures

**Observed:** `ctest` reports `test_suts_susn` and `test_extsucs_big` as
`***Exception: SegFault`. Direct execution reproduces exit `-1073741819`
(`0xC0000005`).

**Root cause:** Both tests perform setup inside `assert(...)`:

```c
/* test_extsucs_big.c */
assert(extsucs_big_init(&v, storage, BIG_LIMBS));
...
assert(extsucs_big_assign_u64(&v, 0x41));

/* test_suts_susn.c */
CHECK(suts_susn_descriptor_init(&comp, 0u, SUSN_FLAG_RANGE_ARRAY, n1, out));
```

The target is compiled with `-O3 -DNDEBUG` (see
`build/CMakeFiles/test_suts_susn.dir/flags.make` and the corresponding
`test_extsucs_big.dir/flags.make`). Under `NDEBUG`, `assert(expr)` expands to
`((void)0)` and **does not evaluate `expr`**. The objects are used uninitialized
and the process faults.

**Verification:**

```text
cc -O3 -std=c99 -UNDEBUG -Iinclude tests/test_suts_susn.c build/libsuts_static.a
  → all 11 test groups pass

cc -O3 -std=c99 -UNDEBUG -Iinclude -Isuperunicode_extended/include \
   -Isuperunicode/include superunicode_extended/tests/test_extsucs_big.c \
   superunicode_extended/bin/libsuperunicode_extended.a \
   superunicode/bin/libsuperunicode_static.a
  → all 5 test groups pass
```

**Conclusion:** the libraries are not at fault; the test harness is built in a
mode that disables its own setup. This is a release-blocking test-infrastructure
defect and should be fixed before any conformance claim.

**Recommended fix (choose one, consistently):**

1. Replace `assert`/`CHECK` with a macro that always evaluates its argument and
   records failure without relying on `NDEBUG`, e.g.
   `#define CHECK(c) do { if (!(c)) { fprintf(stderr, ...); exit(1); } } while (0)`.
2. Compile every test target with `-UNDEBUG` (and `/UNDEBUG` on MSVC), leaving
   `NDEBUG` for product code only.
3. Build tests in a dedicated Debug configuration.

Option 1 is preferred because it also removes the dependency on `<assert.h>` and
makes failures visible in every configuration.

### 5.2 Pass/fail matrix

| Suite | Entries | Result |
|---|---|---|
| SUAS host tests | 17 | pass |
| SUTS host tests | 10 | 8 pass, 2 fail under `NDEBUG` |
| SUTR host tests | 7 | pass |
| Unified header test | 1 | pass |
| Subproject tests (superunicode, sutf, sust, suf, sutf_convert, unicode_port, extended) | 17 | pass |
| **Total** | **52** | **50 pass, 2 harness defects** |

### 5.3 OpenWindows host tests

`build-cmake2/` contains a working host harness:

```text
1/3 hosttest ................. Passed
2/3 crypto_reference_model ... Passed
3/3 crypto_field_reference ... Passed
100% tests passed, 0 tests failed out of 3
```

The stale `hosttest_build.log` in the repository root is not evidence of the
current state; it records an older Makefile invocation against a removed
`../BANcode` layout.

---

## 6. OpenWindows integration audit

### 6.1 What is actually integrated

| Subsystem | SuperUnicode dependency |
|---|---|
| `lib/sucs.c` | SUTF-8 + SUCS mode via `sutf/src/sutf8.c`, `sutf/src/sucs_mode.c` |
| `lib/kprintf.c` | none (own formatting) |
| `vfs/vfs.c`, OWFS/USFS | none (own string/security/storage) |
| `core/alpc.c` | none |
| `hal/kbd.c` | none |
| `shell/shell.c` | none |
| BANcode | none |

The kernel does not link `suas_static`, `suts_static`, `sutr_static`, `sust`,
`suf`, ExtSUCS/vSUTF, SUCD, or the generated data tables. Any claim that
OpenWindows "runs on SuperUnicode" is currently limited to SUTF-8/SUCS mode.

### 6.2 Syscall boundary findings

`syscall/dispatcher.c` validates user memory only on the read path.

| Syscall | Validation before use | Risk |
|---|---|---|
| `OW_SYS_VFS_READ` | `syscall_user_range` + `syscall_copy_name` | OK |
| `OW_SYS_VFS_CREATE` | raw `(const char *)P1` into `OwVfsCreateFile` | Unvalidated user pointer |
| `OW_SYS_VFS_WRITE` | raw `P1` name and raw `P2` buffer | Unvalidated user pointer / read of arbitrary kernel memory |
| `OW_SYS_VFS_DELETE` | raw `P1` | Unvalidated user pointer |
| `OW_SYS_ALPC_CREATE` | raw `P1` | Unvalidated user pointer |
| `OW_SYS_ALPC_SEND` | `ow_memcpy` from raw `P2` | Kernel read from user address |
| `OW_SYS_ALPC_RECV` | `ow_memcpy` to raw `P2` | Kernel write to user address |

Additional concerns:

- Port handles (`P1` in ALPC calls) are cast directly to `OW_ALPC_PORT *` with no
  handle-table validation.
- `VFS_WRITE` trusts `P3` as a length while the source pointer is unvalidated, so
  a call can read past the caller's mapping.
- `VFS_CREATE`/`WRITE`/`DELETE` do not load the caller identity the way
  `VFS_READ` does; authorization is therefore ambient rather than caller-scoped.

**Action:** add one shared `syscall_user_range` check per pointer/length pair,
copy names through `syscall_copy_name`, validate ALPC handles against the
process handle table, and make identity scoping uniform across all VFS calls.

### 6.3 Integration candidates

| Candidate | Existing substrate | Gap |
|---|---|---|
| Character data | `suas_sucd`, SDF/SSP/SIP tables | not linked |
| Segmentation / break / identifiers | `suas_sts`, `suas_sbr`, `suas_sis` | not linked |
| Collation / security | `suts_suca`, `suts_scss`, `suts_susch` | not linked |
| Locale / emoji / links | `suts_sldf`, `suts_sueps`, `suts_suldf` | not linked |
| Vertical / Arabic / CJK / hieroglyph | `suas_suvtl`, `suas_suamr`, `suas_sueasd`, `suas_sukemet` | not linked |
| Set algebra | `suts_susn` | not linked |
| Conformance reporting | `sutr_sucm` | not linked |

---

## 7. Proposed `S-` components — evidence and status

The `S-` names below are architectural proposals only. **None of them appears in
either repository**, so there is no name collision and no implementation to
preserve. They must not be described as implemented until source and tests exist.

| Proposed | Intended role | Existing substrate | Status | First verification |
|---|---|---|---|---|
| **S-CD** | Character data authority | SUCD (`suas_sucd`), SUAS-012 | substrate exists, component not formed | `test_suas_sucd`, `unified` |
| **S-TS** | Text services (segmentation, breaks, identifiers) | STS, SBR, SIS, SDF, SUCF | substrate exists | `test_suas_sts/sbr/sis/sdf/sucf` |
| **S-IC** | Identifier/collation/security | SUCA, SIS, SCSS, SUSCH, SIDN | substrate exists | `test_suts_suca/scss/susch/sidn` |
| **S-LO** | Locale and presentation | SLDF, SUEPS | substrate exists | `test_suts_sldf/sueps` |
| **S-TR** | Text rendering/layout | SGW, SUVTL, SUAMR, SUKEMET, SUEASD, SIP, SUF | substrate exists | `test_suas_sgw/suvtl/suamr/sukemet/sueasd/sip`, `test_suf` |
| **S-IN** | Input (keyboard, UART) | OpenWindows `hal/kbd.c`, UART driver | OS code exists; no SuperUnicode binding | OpenWindows hosttest |
| **S-IM** | Inter-process messaging | OpenWindows `core/alpc.c`, syscall ALPC | OS code exists; unvalidated boundary | OpenWindows hosttest |
| **S-FS** | Filesystem / path authority | OpenWindows `vfs/vfs.c`, OWFS/USFS | OS code exists; no SuperUnicode binding | OWFS/USFS host tests |
| **S-KC** | Kernel console | OpenWindows `lib/kprintf.c`, `shell/shell.c` | OS code exists | OpenWindows hosttest |
| **S-KT** | Kernel test/conformance | CTest suites, OpenWindows `hosttest` | substrate exists | full `ctest` + OpenWindows CTest |

### 7.1 Placement rule

`S-*` is a component name, not a document number. The portable library
components (`S-CD`, `S-TS`, `S-IC`, `S-LO`, `S-TR`) belong in SuperUnicode.
The OS-bound components (`S-FS`, `S-IM`, `S-IN`, `S-KC`, `S-KT`) belong in the
OpenWindows integration layer unless a later SUAS document explicitly makes them
portable requirements. This keeps SuperUnicode a portable C library ecosystem,
not an OS.

---

## 8. Dependency map

### 8.1 Existing implementation dependencies (verified by build graph)

Compile-time coupling inside the libraries is deliberately small. The only
cross-module `#include` edges in `src/` are:

- `suas_scms.c` → `suas_ssre.h`
- `suas_sucd.c` → `suas_ssp.h`, `suas_ssre.h`

Everything else includes only its own public header (plus generated `.inc`
tables). The relationships below are therefore **conceptual/documentation-level
ordering**, not linker dependencies. This keeps the modules independently
testable but means agreement between them is not enforced by the compiler.

```text
SUCS core (superunicode)
  ├─ SUTF (sutf)                 → SUST (sust)
  ├─ ExtSUCS / vSUTF             → SUTF base
  ├─ SUAS modules                → SUCS constants + SUCD data
  │    ├─ SDF / SSP / SIP        → SUCD properties
  │    ├─ STS / SBR / SIS        → SDF / SUCF / SSP
  │    └─ SGW / SUVTL / SUAMR / SUKEMET / SUEASD
  ├─ SUTS modules                → SUAS text/identifier layers
  │    ├─ SUCA / SRE / SLDF
  │    ├─ SIVD / SCSS / SIDN
  │    ├─ SUEPS / SUSCH / SULDF
  │    └─ SUSN
  └─ SUTR reference impls        → SUAS (tests link sutr_static + suas_static)

OpenWindows kernel
  └─ sutf8.c + sucs_mode.c only
```

### 8.2 Proposed `S-*` dependency order

```text
S-CD  ─┬─> S-TS ─┬─> S-IC
       │         ├─> S-LO
       │         └─> S-TR
       ├─> S-FS
       └─> S-KC

S-IM  ─────> S-FS (path/authorization), S-IN (event delivery)
S-IN  ─────> S-IM, S-TS
S-KT  ─────> all of the above (verification only)
```

Rules:

- `S-CD` is the only character-data authority; derived components read its
  tables and do not fork them.
- `S-TS` is the shared text front door; `S-IC`, `S-LO`, and `S-TR` may depend on
  it but not on one another.
- `S-FS`, `S-IM`, `S-IN`, and `S-KC` are OS integration components and must not
  be pulled into `suas_static`/`suts_static`.
- `S-KT` depends on everything and owns no runtime behavior.

---

## 9. Prioritized implementation plan

Priorities are ordered by correctness and security risk, then architectural
value. Each item lists the dependency and the verification method.

### P0 — Correctness and truthfulness of the current tree

| # | Action | Depends on | Verification |
|---|---|---|---|
| P0-1 | Fix the `assert`-side-effect defect in all test targets (macro that always evaluates, or `-UNDEBUG` for tests) | none | 52/52 CTest pass in the release configuration |
| P0-2 | Resolve the `suas_core.h` and `suts_module.h` stubs: implement or delete the five functions | decision in §11 Q1 | new tests for any retained function; unified header test still passes |
| P0-3 | Correct stale comments ("pending SUAS-001", "pending SUTS-001") | P0-2 | header review |
| P0-4 | Codify governance in a new SUAS document: SUAS mandatory, SUTS optional, SUTR informative, explicit normative-dependency rule | none | document review; index updated without renumbering existing docs |
| P0-5 | Codify the `S-` naming rule and the portable-vs-OS placement rule | P0-4 | document review; grep confirms no legacy `SY*` names |
| P0-6 | Fix documentation drift: `SUTS (SIDN)` typo, SUTR "six of thirteen"/"all fourteen" count, stale "SUAS-001…004" reference, SUTR number padding | none | `verify-docs` target + link check |

### P1 — Security of the OpenWindows boundary

| # | Action | Depends on | Verification |
|---|---|---|---|
| P1-1 | Validate and copy every user pointer in `VFS_CREATE/WRITE/DELETE` | P0-4 (ownership) | new hosttest cases with bad pointers |
| P1-2 | Validate/copy `ALPC_CREATE/SEND/RECV` payloads and check port handles | P1-1 | hosttest ALPC fault-injection |
| P1-3 | Apply caller identity scoping uniformly across VFS calls | P1-1 | permission-denial hosttest |
| P1-4 | Add negative tests for `syscall_user_range` (null, wrap, kernel range, cross-page) | P1-1 | hosttest |

### P2 — Form the `S-*` components on real substrate

| # | Action | Depends on | Verification |
|---|---|---|---|
| P2-1 | Define `S-CD` as a thin, documented facade over `suas_sucd`; do not duplicate tables | P0-5 | single-source check + `test_suas_sucd` |
| P2-2 | Define `S-TS` over STS/SBR/SIS/SDF/SUCF | P2-1 | existing SUAS tests + new facade test |
| P2-3 | Define `S-IC` over SUCA/SIS/SCSS/SUSCH/SIDN | P2-1 | existing SUTS tests + facade test |
| P2-4 | Define `S-LO` over SLDF/SUEPS | P2-1 | existing SUTS tests + facade test |
| P2-5 | Define `S-TR` over SGW/SUVTL/SUAMR/SUKEMET/SUEASD/SIP/SUF | P2-1 | existing tests + facade test |
| P2-6 | Define OS-layer `S-FS`, `S-IM`, `S-IN`, `S-KC` in OpenWindows, not in the portable libraries | P1 | OpenWindows hosttest |
| P2-7 | Define `S-KT` as the aggregate CTest/hosttest gate | all above | one command runs both repositories' suites |

### P3 — Deepen integration only where justified

| # | Action | Depends on | Verification |
|---|---|---|---|
| P3-1 | Link `suas_sucd` and `suas_sis` into OpenWindows where identifiers/paths are validated | P2-1, P2-2 | hosttest + new path tests |
| P3-2 | Link `suts_scss`/`suts_susch` for identifier admissibility in the shell | P2-3 | shell input tests |
| P3-3 | Link `suas_sts`/`suas_sbr` for console line handling, if the console needs Unicode segmentation | P2-2, P2-6 | console regression tests |
| P3-4 | Evaluate `suts_susn` for kernel set filtering | P2-1 | benchmark + test |
| P3-5 | Wire `sutr_sucm` into a conformance report target | P0-4 | generated report reviewed |

Do **not** reimplement SUAS behavior inside OpenWindows to avoid the dependency.
Do **not** move OpenWindows-only services into `suas_static`.

---

## 10. Documentation consistency findings

| Location | Finding | Suggested correction |
|---|---|---|
| `docs/INDEX.md` line 27 | `SUTS (SIDN)` missing the number; SIDN is SUTS-006 | `SUTS-006 (SIDN)` |
| `docs/suts/INDEX.md` status | Status paragraph omits SUTS-011 | add SUTS-011 to the authoring list |
| `docs/sutr/INDEX.md` line 59 | "Six of the thirteen reports ship" but the table lists seven and the series has fourteen | "Seven of the fourteen reports ship…" |
| `docs/sutr/INDEX.md` line 80 | "SUAS-001…004, ratified" is stale | "SUAS-001…009, ratified" |
| `docs/sutr/INDEX.md` | IDs mix `SUTR-0…9` and `SUTR-012/013` | choose zero-padded or unpadded form and apply consistently |
| `docs/suas/INDEX.md` | 010…017 are "authoring" but already have source and tests | clarify what "authoring" means when implementation exists |
| `suas_core.h` comments | "pending SUAS-001" (SUAS-001 is SDF) | point at the owning document once decided |
| `suts_module.h` comments | "pending SUTS-001" (SUTS-001 is SUCA) | point at the owning document once decided |
| `hosttest_build.log` | stale failure against removed `../BANcode` | delete or move under `artifacts/` with a date |

`verify-docs` only checks file presence; it does not catch these semantic
inconsistencies.

---

## 11. Unresolved architectural questions

1. **Ownership of the core/registry stubs.** Which document owns kernel
   initialization, version reporting, and module registration? Neither SUAS-001
   nor SUTS-001 is the right owner today. Options: a new SUAS document, an
   existing document, or deletion.
2. **Meaning of "authoring" when code exists.** SUAS-010…017 and SUTS-002…011
   are marked authoring while carrying complete headers, sources, and tests.
   Does implementation precede ratification, or should those documents be
   promoted? The conformance model needs a single answer.
3. **Normative relationship between families.** The requirement that SUAS is
   mandatory, SUTS conformance is optional, and SUTR is informative — and that a
   SUTS/SUTR becomes normative only when SUAS says so — is not written down.
   Which document states it, and does it amend SUTR-10?
4. **`S-*` naming authority.** Is the `S-` rule a new SUAS document, an appendix
   to an existing one, or a separate registry? How are names reserved before
   implementation?
5. **Portable vs. OS boundary.** Are `S-FS`, `S-IM`, `S-IN`, and `S-KC` allowed
   to have SUAS contracts even though their implementations live in OpenWindows?
   If yes, they need portable interface headers without OS dependencies.
6. **SUCD as single source of truth.** Several modules describe SUCD as
   authoritative while carrying their own curated tables (SDF, SSP, SIP). Which
   table wins when they disagree, and is there a build-time reconciliation test?
7. **ExtSUCS in SUAS.** SUAS is defined over 31-bit `SUCS_CP`, while ExtSUCS
   allows values above `0x7FFFFFFF`. Which SUAS APIs must accept
   `sucs_ex_char_t`, and which must reject it? Today this is per-module.
8. **OpenWindows conformance target.** Is OpenWindows required to link any SUAS
   library for a compliance claim, or is SUTF-8/SUCS mode sufficient? The answer
   determines whether P3 items are requirements or optimizations.
9. **Test configuration policy.** Should product code and test code share
   `NDEBUG`? The current failures argue for a split, but that is a build-policy
   decision, not a per-file fix.
10. **Document numbering.** No permanent number should be assigned to new
    governance/`S-*` documents until the indexes are reconciled (P0-6).

---

## 12. What was verified vs. not verified

**Verified:**

- File presence, headers, sources, build entries, and tests for all 17 SUAS,
  10 SUTS, and 7 SUTR reference modules.
- That the five stub functions are undefined, unbuilt, untested, and uncalled.
- The `S-*` names do not occur in either repository.
- 50/52 SuperUnicode CTest entries pass; the two failures reproduce and are
  explained by `NDEBUG`.
- Both failing suites pass with assertions enabled.
- OpenWindows `build-cmake2` host tests pass 3/3.
- Only `sutf8.c` and `sucs_mode.c` are linked from SuperUnicode into OpenWindows.
- The unvalidated-pointer paths in `syscall/dispatcher.c`.

**Not verified (out of scope or blocked):**

- Runtime behavior on real OpenWindows hardware/QEMU.
- Full MSVC kernel build and PE/OWX packaging.
- Semantic correctness of every normative clause in SUAS/SUTS/SUTR beyond what
  the host tests exercise.
- Whether the curated per-module tables agree with SUCD on every codepoint.
- Performance claims (O(1), zero-allocation) beyond the tests' assertions.

---

## 13. Reproduction commands

```bat
:: SuperUnicode
cd C:\Users\KARIMABENDA\Documents\superunicode
cmake --build build
cd build && ctest --output-on-failure

:: The two failures, with assertions restored
C:\w64devkit\bin\cc.exe -O3 -std=c99 -UNDEBUG -Iinclude ^
  tests\test_suts_susn.c build\libsuts_static.a -o %TEMP%\susn_dbg.exe
%TEMP%\susn_dbg.exe

:: OpenWindows host tests
cd C:\Users\KARIMABENDA\Documents\OpenWindows\build-cmake2
ctest --output-on-failure
```

---

## 14. Bottom line

The SuperUnicode tree is substantially implemented and tested — far more than a
documentation scaffold. Its weaknesses are concentrated in three places: test
harness configuration, two orphan stub headers, and unstated governance. The
OpenWindows integration is real but narrow, and its syscall boundary needs
hardening before it can safely expose filesystem or ALPC services to untrusted
callers. The `S-*` components are a sensible layering proposal, but they must be
built as thin facades over existing, tested substrate rather than as parallel
reimplementations, and the portable/OS boundary must be written down first.
