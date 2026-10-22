# SuperUnicode Extended (ExtSUCS & vSUTF)

> **OpenWindows System Architecture Core Library**  
> Bare-metal C99 (`-std=c99 -nostdlib -ffreestanding`) implementation of the **arbitrary-precision** ExtSUCS character encoding and the vSUTF transformation format (`libsuperunicode_extended.a`).

---

## Architectural Role & Overview

`superunicode_extended` provides the high-performance extended character encoding for the OpenWindows system architecture:

* **ExtSUCS (Character Encoding):** Strictly an abstract, unbounded numerical address space (0 -> infinity). The full domain is represented by `sucs_ex_big_t` (`extsucs_big.h`), a caller-backed base-2^32 integer with **no limb limit and no maximum codepoint**. `sucs_ex_char_t` is retained as the 64-bit **fast-path** view of the low 2^64 addresses, not as the encoding boundary. Inherits the System Control Plane (SCP) boundaries (`0x00110000`–`0x0011FFFF`) and, inside the SCP, the **BANcode Registry Plugin Range** (`0x0011A000`–`0x0011AEFF`) exactly as defined in Base SUCS.
* **vSUTF (Transformation Format):** Variable multi-byte transformation covering the full ExtSUCS range, with a fast-path for Base SUCS, a fixed 9-byte 64-bit escape (`0xFE`), and an arbitrary-length `0xFF` regime for values above 2^64-1.

> [!IMPORTANT]
> **Serialization transports live in SUST:** The fixed-width vector transports (SUST-32/64/128/256/512/N) and the hypervisor page-mapped IPC transport (e-SUST) were relocated to the SUST library. See `sust/` in the workspace root and `<sust.h>` — do not reference `extsutf_fixed.h` or `esutf.h`, they no longer exist.

> [!NOTE]
> **Out-of-Band Error Model:** In ExtSUCS, `0x7FFFFFFF` is a valid codepoint address (unlike 31-bit Base SUCS where `0x7FFFFFFF` is `SUCS_INVALID_CODEPOINT`). Because ExtSUCS is unbounded, it contains **zero in-band sentinels**. All functions return status flags (`bool` / `size_t`) and pass decoded codepoints via output pointers.

---

## Core Modules

### 1. ExtSUCS Character Encoding (`extsucs_types.h`, `extsucs_big.h`)
* **Address Space:** 0 -> infinity. No library-defined maximum. `sucs_ex_big_t` holds as many base-2^32 limbs as the caller provides; `sucs_ex_char_t` (uint64_t) is the fast path for the low 2^64 addresses only.
* **Hardware Traps:** Inherits the Base SUCS Kernel Security Trap Range (`0x7FFFFFF0`–`0x7FFFFFFE`), which remains reserved across both modes. Every value above `0x7FFFFFFE`, including arbitrary-precision values, is valid.
* **System Control Plane & BANcode Registry:** Inherits the SCP (`0x00110000`–`0x0011FFFF`) and the BANcode Registry (`0x0011A000`–`0x0011AEFF`; B+ BANcode `0x0011A000`–`0x0011A7FF`, W+ WARNcode `0x0011A800`–`0x0011ABFF`, C+ COMcode `0x0011AC00`–`0x0011ADFF`, S+ SOFTcode `0x0011AE00`–`0x0011AEFF`), plus classification helpers (`extsucs_is_scp_plane`, `extsucs_is_bancode_registry`, `extsucs_classify_bancode`, ...).
* **Casting Utilities:**
  * `sucs_upcast(cp)`: Zero-cost widening conversion from 31-bit Base SUCS to the 64-bit ExtSUCS fast path.
  * `sucs_downcast(ex_cp, &out_cp)`: Safe narrowing conversion from the 64-bit ExtSUCS fast path to 31-bit Base SUCS. Fails out-of-band if `ex_cp > 0x7FFFFFFF` or equals `0x7FFFFFFF`.
  * `extsucs_big_*`: Arbitrary-precision assignment, comparison, byte import/export, validation, and safe Base downcast.

---

### 2. Variable Streaming Transformation (`vsutf.h`)
`vSUTF` provides variable-length multi-byte transformation for the entire ExtSUCS space with a fast-path for Base SUCS:

* **Base SUCS Codepoints (`0x0`–`0x7FFFFFFF`):** Transformed using standard SUTF-8 framing (1 to 6 bytes).
* **64-bit Extended Codepoints (`0x80000000`–`0xFFFFFFFFFFFFFFFF`):** Transformed using a 9-byte frame (`0xFE` prefix header + 8 bytes big-endian payload).
* **Arbitrary-Precision Codepoints (`> 0xFFFFFFFFFFFFFFFF`):** `0xFF` prefix + canonical unsigned LEB128 payload length `L` (where `L >= 9`) + `L` big-endian payload bytes with no leading zero. There is **no maximum `L`**.
* **Streaming:** `vsutf_encode_big_stream` / `vsutf_decode_big_stream` emit and consume the payload in bounded chunks, so a codepoint larger than any single buffer can be transported.


---

## Directory Structure

```text
superunicode_extended/
├── CMakeLists.txt              # Build configuration for libsuperunicode_extended.a & tests
├── include/                    # Public C99 headers
│   ├── extsucs_types.h         # ExtSUCS fast-path type, validation, upcast/downcast
│   ├── extsucs_big.h           # Arbitrary-precision codepoints (full address space)
│   └── vsutf.h                 # Variable multi-byte streaming transformation (vSUTF)
├── src/                        # Freestanding C99 implementation
│   ├── extsucs_big.c           # Unbounded base-2^32 codepoint arithmetic
│   ├── vsutf.c                 # vSUTF encoder / decoder (base, 64-bit, 0xFF)
│   ├── extsucs_ucd_names.c     # Extended UCD name lookup
│   └── extsucs_conv.c          # UTF-8 <-> vSUTF / SUCS <-> ExtSUCS conversion
└── tests/
    ├── test_extsutf_all.c      # ExtSUCS validators, SCP/BANcode, upcast/downcast, vSUTF tests
    ├── test_extsucs_big.c      # Arbitrary-precision + 0xFF round-trip/streaming tests
    └── test_extsucs_ucd.c      # Pins the shared UCS/UCB data to the same Unicode release as Base SUCS
```

> [!NOTE]
> **UCS/UCB data is shared, not duplicated:** ExtSUCS has no UCD tables of its own. `extsucs_compat.h` re-exports the base block table and `extsucs_ucd_names.c` delegates name lookups to the base library, so both modules always track the *same* Unicode release (currently **18.0**). Query it with `EXTSUCS_UNICODE_VERSION_MAJOR`/`_MINOR`, `EXTSUCS_UCD_UNICODE_VERSION`, `EXTSUCS_UCD_BLOCK_COUNT` (353) and `EXTSUCS_UCD_NAME_COUNT` (41232). `tests/test_extsucs_ucd.c` asserts these match `superunicode/tests/test_sucs_ucd.c`, so a one-sided Unicode bump fails the build. The 64-bit wrappers (`extsucs_ucd_block_lookup`, `extsucs_ucd_get_name`, ...) reject codepoints above `0x0010FFFF` rather than truncating them into the bridge.

> **Fixed-width (SUST-32/64/128/256/512/N) and e-SUST IPC transports:** see the `sust/` sub-project (headers `sustfixed.h`, `esust.h`, aggregate `sust.h`).

---

## Code Examples

### ExtSUCS Upcasting & Safe Downcasting

```c
#include "extsucs_types.h"

// Zero-cost widening cast
sucs_char_t base_cp = 0x10000;
sucs_ex_char_t ex_cp = sucs_upcast(base_cp);

// Out-of-band safe narrowing cast
sucs_char_t out_base;
if (sucs_downcast(ex_cp, &out_base)) {
    // Successfully downcasted to Base SUCS
} else {
    // Codepoint exceeds 31-bit limit or falls in trap/sentinel range
}
```

### vSUTF Streaming Transformation

```c
#include "vsutf.h"

uint8_t stream_buf[VSUTF_MAX_BYTES]; // 9 bytes for the 64-bit fast path
sucs_ex_char_t ex_cp = 0xFF88990011223344ULL;

// Transforms using 0xFE prefix header + 8 bytes payload
size_t bytes_written = vsutf_encode(ex_cp, stream_buf, sizeof(stream_buf));

sucs_ex_char_t decoded_cp;
size_t bytes_read = vsutf_decode(stream_buf, bytes_written, &decoded_cp);
```

### Arbitrary-Precision Codepoints (no upper bound)

```c
#include "extsucs_big.h"
#include "vsutf.h"

// 300-byte magnitude (2400 bits) — far beyond 2^64-1.
uint32_t limbs[80];
sucs_ex_big_t cp;
extsucs_big_init(&cp, limbs, 80);

uint8_t magnitude[300] = { 0xA5, /* ... 299 more bytes ... */ };
extsucs_big_assign_bytes_be(&cp, magnitude, sizeof(magnitude));

// 0xFF + LEB128(300) + 300 bytes = 303 bytes. No format-level maximum.
uint8_t frame[400];
size_t written = vsutf_encode_big(&cp, frame, sizeof(frame));

uint32_t decoded_limbs[80];
sucs_ex_big_t decoded;
extsucs_big_init(&decoded, decoded_limbs, 80);
size_t used = vsutf_decode_big(frame, written, &decoded);
// used == written, and extsucs_big_compare(&cp, &decoded) == 0
```

For values too large for one output buffer, use
`vsutf_encode_big_stream(&cp, write_fn, ctx, &n)` and
`vsutf_decode_big_stream(read_fn, ctx, &decoded, &consumed)`.

> For SUST serialization transports (fixed-width SUST-32/64/128/256/512/N, e-SUST page-mapped IPC), see `sust/README.md`.

---

## Building and Testing

### Build `libsuperunicode_extended.a` Static Library
```bash
cmake -B superunicode_extended/build -S superunicode_extended -G "MinGW Makefiles"
cmake --build superunicode_extended/build
```

### Run Unit Tests
```bash
ctest --test-dir superunicode_extended/build --output-on-failure
```
