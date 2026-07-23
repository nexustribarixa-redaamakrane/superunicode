# SuperUnicode Extended (ExtSUCS & extSUTF)

> **OpenWindows System Architecture Core Library**  
> Bare-metal C99 (`-std=c99 -nostdlib -ffreestanding`) implementation of the unbounded 64-bit ExtSUCS character encoding and extended extSUTF text serialization transports (`libsuperunicode_extended.a`).

---

## Architectural Role & Overview

`superunicode_extended` provides high-performance extended character encodings and vector/hypervisor text serialization transports for the OpenWindows system architecture:

* **ExtSUCS (Character Encoding):** Strictly an abstract, unbounded numerical address space (0 -> infinity). Implemented in C99 via the 64-bit `sucs_ex_char_t` container, supporting up to 2^64-1 codepoints.
* **extSUTF (Serialization Transports):** Fixed-width vector alignments (SUTF-32/64/128/256/512/N), variable multi-byte streaming (vSUTF), and hypervisor page-mapped IPC transports (e-SUTF).

> [!NOTE]
> **Out-of-Band Error Model:** In ExtSUCS, `0x7FFFFFFF` is a valid codepoint address (unlike 31-bit Base SUCS where `0x7FFFFFFF` is `SUCS_INVALID_CODEPOINT`). Because ExtSUCS is unbounded, it contains **zero in-band sentinels**. All functions return status flags (`bool` / `size_t`) and pass decoded codepoints via output pointers.

---

## Core Modules & Transport Specifications

### 1. ExtSUCS Character Encoding (`extsucs_types.h`)
* **Address Space:** 0 -> infinity (64-bit container `sucs_ex_char_t`).
* **Hardware Traps:** Inherits the Base SUCS Kernel Security Trap Range (`0x7FFFFFF0`–`0x7FFFFFFE`), which remains reserved across both modes.
* **Casting Utilities:**
  * `sucs_upcast(cp)`: Zero-cost widening conversion from 31-bit Base SUCS to 64-bit ExtSUCS.
  * `sucs_downcast(ex_cp, &out_cp)`: Safe narrowing conversion from 64-bit ExtSUCS to 31-bit Base SUCS. Fails out-of-band if `ex_cp > 0x7FFFFFFF` or equals `0x7FFFFFFF`.

---

### 2. Fixed-Width Vector Transports (`extsutf_fixed.h`)
Designed for SIMD vector register slots (SSE, AVX-256, AVX-512) and AI tensor memory alignment. All vector formats use **big-endian byte order** with zero-padding in upper bytes.

| Transport | Slot Size | Alignment / Target Register | Supported Range |
| :--- | :--- | :--- | :--- |
| **SUTF-32** | 4 Bytes | 32-bit register / Base SUCS fast-path | `0x00000000`–`0xFFFFFFFF` |
| **SUTF-64** | 8 Bytes | 64-bit GPR / AI Tensor Slot | Full 64-bit ExtSUCS range |
| **SUTF-128** | 16 Bytes | 128-bit SSE / NEON register slot | Full 64-bit ExtSUCS range (zero-padded) |
| **SUTF-256** | 32 Bytes | 256-bit AVX-256 register slot | Full 64-bit ExtSUCS range (zero-padded) |
| **SUTF-512** | 64 Bytes | 512-bit AVX-512 register slot | Full 64-bit ExtSUCS range (zero-padded) |
| **SUTF-N** | N Bytes | Arbitrary multi-word block container | Full 64-bit ExtSUCS range (caller-defined N >= 8) |

---

### 3. Variable Streaming Transport (`vsutf.h`)
`vSUTF` provides variable-length multi-byte serialization for the entire 64-bit ExtSUCS space with a fast-path for Base SUCS:

* **Base SUCS Codepoints (`0x0`–`0x7FFFFFFF`):** Serialized using standard SUTF-8 framing (1 to 6 bytes).
* **Extended Codepoints (`> 0x7FFFFFFF`):** Serialized using a 9-byte stream frame (`0xFE` prefix header + 8 bytes big-endian payload).
* **Reserved Prefix:** `0xFF` is reserved for future extensions beyond 64-bit address spaces.

---

### 4. Hypervisor Virtual IPC Transport (`esutf.h`)
`e-SUTF` (Emulated/Virtual SUTF) enables page-mapped coordinate translation between hypervisor host physical codepoints and guest virtual spaces across IPC boundaries:

* **Page Architecture:** Divided into virtual pages of 4,096 codepoints (`ESUTF_PAGE_SIZE`).
* **IPC Frame Structure:** 6-Byte Compact Serialization Frame (4-byte `page_index` + 2-byte `offset`).
* **Translation API:** `esutf_translate_to_guest()`, `esutf_translate_to_host()`, `esutf_encode_ipc()`, `esutf_decode_ipc()`.

---

## Directory Structure

```text
superunicode_extended/
├── CMakeLists.txt              # Build configuration for libsuperunicode_extended.a & tests
├── include/                    # Public C99 headers
│   ├── extsucs_types.h         # ExtSUCS 64-bit type, validation, upcast/downcast
│   ├── extsutf_fixed.h         # Fixed-width vector transports (SUTF-32..512/N)
│   ├── vsutf.h                 # Variable multi-byte streaming transport (vSUTF)
│   └── esutf.h                 # Hypervisor virtual page-mapped transport (e-SUTF)
├── src/                        # Freestanding C99 implementation
│   ├── extsutf_fixed.c         # Fixed-width vector encoders / decoders
│   ├── vsutf.c                 # vSUTF encoder / decoder
│   └── esutf.c                 # e-SUTF page translation & IPC frame handlers
└── tests/
    └── test_extsutf_all.c      # Unit test suite covering all extended transports
```

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

### SIMD Vector Slot Serialization (SUTF-128 & SUTF-64)

```c
#include "extsutf_fixed.h"

sucs_ex_char_t ex_cp = 0x123456789ABCULL;
uint8_t vector_slot[SUTF128_BYTES]; // 16-byte SSE slot

// Encode into 16-byte zero-padded big-endian vector slot
sutf128_encode(ex_cp, vector_slot, sizeof(vector_slot));

// Decode back from SSE slot
sucs_ex_char_t decoded_cp;
sutf128_decode(vector_slot, sizeof(vector_slot), &decoded_cp);
```

### vSUTF Streaming Transport

```c
#include "vsutf.h"

uint8_t stream_buf[VSUTF_MAX_BYTES]; // 9 bytes max
sucs_ex_char_t ex_cp = 0xFF88990011223344ULL;

// Encodes using 0xFE prefix header + 8 bytes payload
size_t bytes_written = vsutf_encode(ex_cp, stream_buf, sizeof(stream_buf));

sucs_ex_char_t decoded_cp;
size_t bytes_read = vsutf_decode(stream_buf, bytes_written, &decoded_cp);
```

### Hypervisor e-SUTF IPC Frame Translation

```c
#include "esutf.h"

sucs_ex_char_t host_cp = (500ULL * ESUTF_PAGE_SIZE) + 128ULL; // Host codepoint on page 500
uint8_t ipc_frame[ESUTF_IPC_FRAME_BYTES]; // 6 bytes

// Serialize host codepoint to 6-byte IPC frame
size_t written = esutf_encode_ipc(host_cp, ipc_frame, sizeof(ipc_frame));

// Deserializes guest IPC frame back to host physical codepoint
sucs_ex_char_t reconstructed_cp;
size_t read = esutf_decode_ipc(ipc_frame, written, &reconstructed_cp);
```

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
