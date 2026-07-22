# SUTF (SuperUnicode Transformation Format) Base Transports & Kernel Controller

> **OpenWindows System Architecture Core Library**  
> Bare-metal C99 (`-std=c99 -nostdlib -ffreestanding`) implementation of Base SUTF text formatting serialization transports and the Kernel Mode-Switching Controller (`libsutf.a`).

---

## Architectural Role & Overview

In the OpenWindows system architecture, a strict distinction is maintained between **Character Encodings** and **Serialization Transports**:

* **Base SUCS (Character Encoding):** Defines the abstract 31-bit numerical address space (`0x00000000` to `0x7FFFFFFF`).
* **SUTF (Text Formatting & Serialization Transport):** Defines the physical byte-packing, bit-alignment, memory layouts, and stream framing rules for storing and transmitting SUCS codepoints across system buses, CPU caches, and IPC channels.

`sutf` provides the fundamental 8-bit, 16-bit, 4-bit (nibble), and 2-bit (symbol frame) transport encoders and decoders, alongside the kernel mode-switching controller that manages transitions between Base SUCS and ExtSUCS operating modes.

---

## Transport Specifications

| Transport | Frame / Unit Size | Range Covered | Description & Primary Use Case |
| :--- | :--- | :--- | :--- |
| **SUTF-8** | 1 to 6 Bytes | `0x00000000`–`0x7FFFFFFF` | Variable multi-byte stream transport for Base SUCS. Standard UTF-8 parity up to `0x10FFFF`, extending up to 6 bytes for native extended planes. |
| **SUTF-16** | 1 to 2 16-Bit Words | `0x00000000`–`0x7FFFFFFF` | Word-aligned stream transport. 1 word for `0x0000`–`0xFFFF` (`0xD800`–`0xDFFF` valid PUA), 2 words for higher planes. |
| **SUTF-4** | 4-Bit Hex Nibbles | `0x00000000`–`0x7FFFFFFF` | 8 nibbles (4 bytes) fixed per codepoint. Used for console dumps, terminal logging, and low-level debugging. |
| **SUTF-2** | 2-Bit Symbol Frames | `0x00000000`–`0x7FFFFFFF` | 16 frames (4 bytes) fixed per codepoint. Optimized compressed bitstream framing for inter-thread IPC channels. |

---

## Kernel Mode-Switching Subsystem (`sucs_mode.h`)

The OpenWindows kernel mode controller regulates active system encoding and transport capabilities:

* **`SUCS_MODE_BASE` (0):** 31-bit Base SUCS encoding & Base SUTF transports (SUTF-8, SUTF-16, SUTF-4, SUTF-2).
* **`SUCS_MODE_EXTENDED` (1):** Unbounded ExtSUCS 64-bit encoding & extSUTF transports (SUTF-32..512, vSUTF, e-SUTF).

> [!IMPORTANT]
> **Reboot Safety:** Switching operating modes requires a mandatory kernel restart. Mode alterations are staged as `pending_mode` via `sucs_request_mode_switch()` and committed during early boot execution via `sucs_commit_mode_on_boot()`.

```c
// Staging a mode alteration during runtime
sucs_switch_status_t status = sucs_request_mode_switch(SUCS_MODE_EXTENDED);
if (status == SUCS_SWITCH_REBOOT_REQUIRED) {
    // Stage success; trigger kernel restart sequence
}

// Early kernel boot entry point (in boot loader / kernel main)
sucs_kernel_boot_config_t boot_cfg;
sucs_init_boot_config(&boot_cfg, SUCS_MODE_BASE);
bool mode_changed = sucs_commit_mode_on_boot(&boot_cfg);
```

---

## Directory Structure

```text
sutf/
├── CMakeLists.txt              # Build configuration for libsutf.a & unit tests
├── include/                    # Public C99 headers
│   ├── sutf.h                  # Master aggregation header
│   ├── sucs_mode.h             # Kernel mode-switching controller API
│   ├── sucs_types.h            # Base SUCS types, validation, and sentinel macros
│   ├── sutf8.h                 # SUTF-8 1-6 byte transport
│   ├── sutf16.h                # SUTF-16 1-2 word transport
│   ├── sutf4.h                 # SUTF-4 4-bit nibble transport
│   └── sutf2.h                 # SUTF-2 2-bit symbol frame transport
├── src/                        # Freestanding C99 implementation
│   ├── sucs_mode.c             # Mode switching logic
│   ├── sutf8.c                 # SUTF-8 encoder / decoder
│   ├── sutf16.c                # SUTF-16 encoder / decoder
│   ├── sutf4.c                 # SUTF-4 nibble pack / unpack
│   └── sutf2.c                 # SUTF-2 symbol frame pack / unpack
└── tests/
    └── test_sutf_all.c         # Unit test suite for base transports & mode switching
```

---

## Usage Examples

### SUTF-8 Encoding & Decoding

```c
#include "sutf.h"

uint8_t buffer[6];
sucs_char_t cp = 0x110000; // System Control Plane codepoint

// Encode SUCS codepoint to SUTF-8 stream
size_t bytes_written = sutf8_encode_char(cp, buffer, sizeof(buffer));

// Decode SUTF-8 stream back to SUCS codepoint
sucs_char_t decoded_cp;
size_t bytes_read = sutf8_decode_char(buffer, bytes_written, &decoded_cp);
```

### SUTF-4 Nibble Stream (Debugging & Console Dumps)

```c
#include "sutf.h"

uint8_t nibble_buf[SUTF4_BYTES_PER_CODEPOINT]; // 4 bytes (8 nibbles)
sucs_char_t cp = 0x123456;

size_t written = sutf4_encode_char(cp, nibble_buf, sizeof(nibble_buf));

sucs_char_t decoded_cp;
size_t read = sutf4_decode_char(nibble_buf, written, &decoded_cp);
```

---

## Building and Testing

### Build `libsutf.a` Static Library
```bash
cmake -B sutf/build -S sutf -G "MinGW Makefiles"
cmake --build sutf/build
```

### Run Unit Tests
```bash
ctest --test-dir sutf/build --output-on-failure
```
