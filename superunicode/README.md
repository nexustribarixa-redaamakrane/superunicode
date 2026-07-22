# superunicode (Super Universal Encoding System - SUES)

`superunicode` is a freestanding, 100% C99-compliant library providing the core character mapping and string serialization engine for the custom `OpenWindows` operating system.

---

## Architecture Overview

### 1. 31-Bit SUCS Address Space
- Code points (`sucs_char_t`) are 32-bit unsigned integers supporting a 31-bit address space (`0x00000000` to `0x7FFFFFFF`).
- Address breakdown: **128 Zones** -> **2,048 Districts** -> **32,768 Planes** -> **256 Block Offsets**.

### 2. Codepoint Classifications
- `0x00000000`–`0x0010FFFF`: **Unicode Bridge Zone** (1:1 standard UTF-8/Unicode parity).
- `0x00110000`–`0x0011FFFF`: **System Control Plane (SCP)** (Inline renderer instructions, style shifts, and layout markers).
- `0x00120000`–`0x7FFFFFFF`: **Native Extended SUCS Allocations** (Custom conlangs, RNUR multi-sets, neographies, technical symbols).

### 3. SUTF Serialization Format (1 to 6 Bytes)
| Byte Count | Codepoint Range | Header Pattern | Payload Bits |
| :--- | :--- | :--- | :--- |
| 1 Byte | `0x00000000`–`0x0000007F` | `0xxxxxxx` | 7 bits |
| 2 Bytes | `0x00000080`–`0x000007FF` | `110xxxxx 10xxxxxx` | 11 bits |
| 3 Bytes | `0x00000800`–`0x0000FFFF` | `1110xxxx 10xxxxxx 10xxxxxx` | 16 bits |
| 4 Bytes | `0x00010000`–`0x0010FFFF` | `11110xxx 10xxxxxx 10xxxxxx 10xxxxxx` | 21 bits |
| 5 Bytes | `0x00110000`–`0x03FFFFFF` | `111110xx 10xxxxxx ...` | 26 bits |
| 6 Bytes | `0x04000000`–`0x7FFFFFFF` | `1111110x 10xxxxxx ...` | 31 bits |

---

## Directory Structure

```text
superunicode/
├── CMakeLists.txt
├── README.md
├── include/
│   └── superunicode/
│       ├── sucs_types.h
│       ├── sucs_plane.h
│       ├── sucs_compat.h
│       ├── sutf.h
│       └── superunicode.h
├── src/
│   ├── sutf_encode.c
│   ├── sutf_decode.c
│   └── sucs_string.c
├── tests/
│   ├── CMakeLists.txt
│   ├── test_sutf.c
│   └── test_sucs_planes.c
└── tools/
    └── sucs_inspector.c
```

---

## Build & Test Instructions

### Building with CMake
```powershell
cmake -B superunicode/build -S superunicode
cmake --build superunicode/build
```

### Running Unit Tests
```powershell
ctest --test-dir superunicode/build --output-on-failure
```

### Using the Inspector CLI Tool
```powershell
./superunicode/build/tools/sucs_inspector 0x110000
./superunicode/build/tools/sucs_inspector 0x123456
```
