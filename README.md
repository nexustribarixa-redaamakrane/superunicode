# SuperUnicode (SUCS / ExtSUCS) & SUTF / extSUTF Serialization Transports

> **OpenWindows System Architecture Core Library**  
> Bare-Metal C99 (`-std=c99 -nostdlib -ffreestanding`) implementation of SuperUnicode character encodings and SUTF text formatting serialization transports.

---

## Architecture Overview

### 1. Character Encodings vs. Serialization Transports

- **SuperUnicode (SUCS) and SuperUnicode Extended (ExtSUCS)**:  
  Strictly **CHARACTER ENCODINGS** defining numerical address spaces.
  - **Base SUCS**: 31-bit address space (`0x00000000` to `0x7FFFFFFF`). Reserved Kernel Security Trap Range: `0x7FFFFFF0`–`0x7FFFFFFE`. Sentinel: `0x7FFFFFFF` (`SUCS_INVALID_CODEPOINT`).
  - **ExtSUCS**: Unbounded address space (0 -> infinity, currently implemented via 64-bit `sucs_ex_char_t` container). Out-of-band error handling with zero in-band sentinels. Inherits Base SUCS trap range.

- **SUTF and extSUTF**:  
  Strictly **TEXT FORMATTING AND SERIALIZATION TRANSPORTS** defining physical byte-packing, bit-alignment, memory layouts, and stream framing rules.
  - **SUTF-8**: 1 to 6 Byte Variable Stream Transport
  - **SUTF-16**: 1 to 2 16-Bit Word Stream Transport (with `0xD800`–`0xDFFF` valid PUA)
  - **SUTF-4**: 4-Bit Hex Nibble Stream Transport for console & bus debugging
  - **SUTF-2**: 2-Bit Symbol Frame Stream Transport for IPC thread channels
  - **SUTF-32 / SUTF-64**: 4B / 8B Fixed Vector Slot Transports (SIMD & AI Alignment)
  - **SUTF-128 / 256 / 512 / N**: 16B / 32B / 64B / N-byte Aligned Vector Transports (SSE/AVX)
  - **vSUTF**: Variable Multi-Byte Streaming Transport (`0xFE` 9-byte prefix for 64-bit range)
  - **e-SUTF**: Hypervisor Page-Mapped Virtual IPC Transport (4096 codepoint pages, 6-byte IPC frame)

---

## Workspace Structure

```text
SuperUnicode/
├── CMakeLists.txt                      # Top-level unified CMake workspace configuration
├── README.md                           # Master project documentation
├── superunicode/                       # Base SUCS character encoding & inspector tools
│   ├── CMakeLists.txt
│   ├── include/superunicode/           # sucs_types.h, sucs_plane.h, sucs_compat.h, sutf.h, superunicode.h
│   ├── src/                            # sutf_encode.c, sutf_decode.c, sucs_string.c
│   ├── tests/                          # test_sutf.c, test_sucs_planes.c
│   └── tools/                          # sucs_inspector.c
├── sutf/                               # Bare-metal Base SUTF transports & Kernel Mode Controller (libsutf.a)
│   ├── CMakeLists.txt
│   ├── include/                        # sutf.h, sucs_mode.h, sucs_types.h, sutf8/16/4/2.h
│   ├── src/                            # sucs_mode.c, sutf8.c, sutf16.c, sutf4.c, sutf2.c
│   └── tests/                          # test_sutf_all.c
└── superunicode_extended/              # ExtSUCS 64-bit encoding & extSUTF transports (libsuperunicode_extended.a)
    ├── CMakeLists.txt
    ├── include/                        # extsucs_types.h, extsutf_fixed.h, vsutf.h, esutf.h
    ├── src/                            # extsutf_fixed.c, vsutf.c, esutf.c
    └── tests/                          # test_extsutf_all.c
```

---

## Kernel Mode-Switching Subsystem (`sucs_mode.h`)

The OpenWindows Kernel mode controller allows switching between operating modes:
- **`SUCS_MODE_BASE` (0)**: 31-bit Base SUCS & Base SUTF (SUTF-8/16/4/2).
- **`SUCS_MODE_EXTENDED` (1)**: Unbounded ExtSUCS & extSUTF (SUTF-32/64/128/256/512/N, vSUTF, e-SUTF).

Mode alterations require a **mandatory system restart**:
1. `sucs_request_mode_switch(new_mode)` stages `pending_mode` and sets `reboot_required = true`.
2. `sucs_commit_mode_on_boot(&cfg)` commits the alteration during early kernel boot initialization.

---

## Building and Testing with CMake

### Prerequisites
- GCC / MinGW C99 compiler (`-std=c99 -nostdlib -ffreestanding`)
- CMake 3.10+

### Build Instructions
```bash
# Configure build tree
cmake -B build -G "MinGW Makefiles" -DCMAKE_C_COMPILER=gcc

# Build static libraries (libsutf.a, libsuperunicode_extended.a, libsuperunicode_static.a)
cmake --build build

# Execute full test suite
ctest --test-dir build --output-on-failure
```

---

## License & Integration

Designed for core integration into the OpenWindows Operating System Kernel (`OpenWindows-Kernel`), bare-metal platforms, compilers, and system utilities.

Dual-licensed under either of:
- **MIT License** ([`LICENSE-MIT`](file:///c:/Users/KARIMABENDA/Documents/superunicode/LICENSE-MIT))
- **Apache License, Version 2.0** ([`LICENSE-APACHE`](file:///c:/Users/KARIMABENDA/Documents/superunicode/LICENSE-APACHE))

at your option.

