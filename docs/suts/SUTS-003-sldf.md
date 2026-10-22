# SUTS-003 — System Locale Data & Formatting (SLDF)

**Standard ID:** SUTS-003
**Status:** Draft — Authoring
**Scope:** Modular Technical Specification (locale data & formatting)
**Applies To:** Base SUCS (Unicode Bridge, SCP, Native), ExtSUCS (unbounded arbitrary-precision plane), RCCR language and script subtags, the RNUR neography registry, SUCD property tables, SDF directional scopes, SUCA collation, the Native Math ExtSUCS space, system loggers, terminal drivers, the OpenWindows microkernel and shell
**Last Updated:** 2026-10-19

---

## 1. Introduction

Locale-specific formatting is the process of presenting numbers, dates, times,
and currency according to the conventions of a language and region. It is not a
property of a value: the integer `1234` is presented as `1,234.56` in one locale
and `1.234,56` in another, dates are ordered month-first in some locales and
day-first in others, and the sorting weight of a string depends on the locale's
collation rules. Every kernel subsystem that emits human-readable text — the
system logger, the terminal driver, the shell, the installer — consumes these
rules.

**SUTS-003 (SLDF) is the System Locale Data & Formatting specification.** It is
a core kernel primitive of the OpenWindows microkernel. SLDF establishes a
**binary-compiled, zero-allocation schema** for locale formatting data — number
systems, date/time patterns, and collation seeds — that bypasses text-based
markup in favour of fixed-size, byte-aligned memory layouts optimized for O(1)
kernel execution. It is also styled the *System Locale Data Specification*; both
names denote this document.

A subsystem requests formatting rules for a context by its **RCCR subtag**. The
Reddit Conlang Code Registry (RCCR) is an out-of-band code registry for language
and script identification that allocates no codepoints (SUTR-8). The kernel
normalizes the subtag, hashes it, and indexes the matching `sldf_locale_t` block
in O(1) expected time.

### 1.1 OpenWindows kernel principles

In keeping with OpenWindows kernel principles, SLDF ditches XML-style markup
parsers, dynamic resource bundles, and heap-resident object trees. Instead, it
defines a **compiled binary schema**: locale definitions are compiled to
immutable, statically-linked tables of fixed-size C structures. The kernel reads
a decimal separator, a negative-sign placement, or a date order directly from a
fixed memory offset. Lookup is a single probe of a fixed-size open-addressing
index built once over a static array, so a resolve performs a bounded number of
integer operations with no call to a general allocator.

### 1.2 Relationship to UTS #35 LDML

SLDF solves the same problem as UTS #35 *Unicode Locale Data Markup Language*
(LDML): describing locale-specific formatting. It is deliberately **not** an
LDML analog, because the two systems differ in three structural ways:

1. **Text-based markup vs. compiled binary structures.** LDML is an XML markup
   language. Interpreting a rule requires a text parser, a document or tree
   traversal, and dynamic string manipulation. A kernel primitive cannot afford
   a markup parser at run time. SLDF discards markup entirely in favour of
   byte-aligned, statically-compiled C structures (`sldf_numeric_format_t`,
   `sldf_temporal_format_t`, `sldf_locale_t`) that sit directly in read-only
   memory.
2. **Heap allocation vs. zero allocation.** LDML relies on dynamic memory
   allocation to build formatting patterns, fallback chains, and localized
   string caches on the fly. OpenWindows enforces a strict zero-allocation
   mandate for core kernel subsystems. SLDF achieves O(1) lookup through
   fixed-size, static tables without ever touching a heap.
3. **User-space vs. kernel-primitive scope.** LDML is designed for high-level
   user-space application runtimes, text editors, and web browsers to format
   user-facing text. SLDF is a low-level kernel primitive: it gives the
   OpenWindows microkernel, system loggers, and terminal drivers immediate
   access to deterministic number, date, and collation rules without depending
   on external libraries or user-space services.

Where this document shares vocabulary with LDML (decimal separator, currency
position, date order), the requirements are normative for SLDF only.

### 1.3 Key architectural shifts

| Dimension | UTS #35 LDML (user-space) | SUTS-003 SLDF (kernel) |
|-----------|---------------------------|------------------------|
| Representation | XML markup parsed into heap object trees | Fixed-size C structs in read-only memory |
| Allocation | Dynamic patterns, fallback chains, caches built on the fly | Zero; static table + fixed index, no heap |
| Scope | Application runtimes, editors, browsers | Kernel primitive: microkernel, loggers, terminal drivers |
| Lookup | Resource-bundle resolution, often dynamic | RCCR subtag → FNV-1a → fixed open-addressing probe, O(1) |
| Selection | BCP 47 language-range matching (multi-pass) | Exact normalized RCCR subtag key (O(1)) |
| Fallback | Chains of locales/templates merged at run time | Explicit single block; an absent tag resolves to nothing |

### 1.4 Terminology and cross-references

SLDF reuses the SuperUnicode architecture documents, numbered as they exist in
this repository:

| SLDF concept | Authoritative document |
|--------------|------------------------|
| Directional scope / RTL layout | **SUAS-001 (SDF)** — Structural Directional Framing |
| Script identification | **SUAS-005 (SSP)** — System Script Property |
| Search-space separators and symbols (SUCS) | **SUTR-0 (SUCS Core)** — three-space layout |
| Collation and implicit weights | **SUTS-001 (SUCA)** — SuperUnicode Collation Algorithm |
| RNUR neography registry, property tiers | **SUTR-8 (SUCPM)** — the character property model |
| Neography carve-out and plugins | **SUTR-5 (Plugin Lifecycle)** |
| Native Math ExtSUCS space | **SUTR-9 (SUMNLS)** |
| Conformance model and zero-allocation mandate | **SUTR-10 (SUCM)** |

> Earlier drafts of this text numbered the document **SUTS-004**. It is
> renumbered **SUTS-003**, the third SUTS after SUCA and SRE, and titled *System
> Locale Data & Formatting*. The sketch identifier `SLDF_SPEC_ID` becomes
> `SUTS_SLDF_SPEC_ID`; the library and function names (`suts_sldf.h`, `sldf_*`)
> are unchanged.

### 1.5 Conformance

A conforming SLDF implementation MUST:

1. represent locale data as immutable, fixed-size structures reachable directly
   from identifiers, with **no heap allocation** on any lookup path;
2. resolve an RCCR subtag to its locale block in **O(1) expected time** by a
   bounded probe of a fixed-size index, not by a linear scan of the table;
3. normalize the query subtag before comparison (case folding and hyphen
   normalization), so lookup is case-insensitive;
4. keep the locale identifiers unique under the normalization of §3.2; and
5. confine all codepoints it stores as separators or symbols to the SUCS space,
   excluding SCP markers, trap slots, and the Sentinel (§4.4).

### 1.6 SLDF conformance levels

| Level | Name | Requires |
|-------|------|----------|
| L0 | Kernel Core | numeric, temporal, and collation domains for the Bridge plane; zero-allocation O(1) resolution |
| L1 | Bridge & Neography | L0 plus RNUR-plane conlang locales and the plane mapping of §5 |
| L2 | Plugin & ExtSUCS | L1 plus locale blocks sourced from a mounted plugin over the ExtSUCS space |

## 2. The binary locale schema

### 2.1 The locale block

A locale is one `sldf_locale_t` block. It carries its RCCR subtag, the three
formatting domains, and a small set of hints (direction, script) drawn from
sibling architecture standards:

```c
typedef struct {
    char                   tag_key[SUTS_SLDF_MAX_TAG_LEN]; /* RCCR subtag, NUL-terminated */
    sldf_numeric_format_t  numeric;
    sldf_temporal_format_t temporal;
    uint32_t               collation_base_offset;
    uint8_t                collation_plane;   /* suts_sldf_plane_t */
    uint8_t                is_rtl;            /* 1 = RTL script (SDF hint) */
    uint8_t                script;            /* SSP script-id hint (opaque) */
    uint8_t                reserved;
} sldf_locale_t;
```

`tag_key` is at most `SUTS_SLDF_MAX_TAG_LEN` (16) bytes including the
terminator. The hint fields are advisory: `is_rtl` seeds SDF (§References,
SUAS-001) and `script` seeds SSP (SUAS-005); neither changes the formatting
data. `reserved` is zero and MUST be ignored by readers.

All separators and symbols are `sucs_ex_char_t` (64-bit) codepoints, never
`char`. This is the central SLDF deviation from a byte-oriented LDML profile: a
decimal separator of U+066B (Arabic decimal separator), a grouping separator of
U+202F (narrow no-break space), or an RNUR neography glyph survives intact,
whereas a `char` field would truncate or misinterpret it.

### 2.2 Numeric formatting domain

The numeric domain is `sldf_numeric_format_t`. It fixes the decimal point, the
grouping separator and sizes, the currency symbol and its placement, and the
negative-sign placement.

| Field | Meaning |
|-------|---------|
| `decimal_separator` | Decimal point, e.g. U+002E or U+002C |
| `grouping_separator` | Thousands separator, e.g. U+002C, U+202F, or U+066C |
| `grouping_primary` | Digits in the first (rightmost) group; usually 3 |
| `grouping_secondary` | Digits in later groups; `0` means "same as primary" |
| `currency_symbol[]` | Fixed codepoint array, terminated by 0 |
| `currency_symbol_length` | Meaningful codepoints, excluding the terminator |
| `currency_position` | `SUTS_SLDF_AFFIX_PREFIX` or `SUTS_SLDF_AFFIX_SUFFIX` |
| `currency_space` | 1 when a significant space sits between symbol and number |
| `minus_sign` | Negative sign, e.g. U+002D or U+2212 (minus sign) |
| `minus_position` | `SUTS_SLDF_AFFIX_PREFIX` or `SUTS_SLDF_AFFIX_SUFFIX` |
| `negative_parenthesized` | 1 for accounting style, e.g. `(1,234)` |

`grouping_secondary` expresses the group sizes that differ from the default of
three. A value of `2` yields Indian lakh/crore grouping (`12,34,567`); a value of
`0` disables the change (every group is `grouping_primary`).

### 2.3 Temporal formatting domain

The temporal domain is `sldf_temporal_format_t`. It stores an **order vector**
rather than a formatted pattern string: the order vector names which component
leads, and the separators and the 24-hour flag complete the pattern without
storing a pattern string at all.

| Field | Meaning |
|-------|---------|
| `date_order` | `SUTS_SLDF_DATE_YMD`, `..._MDY`, or `..._DMY` |
| `date_separator` | e.g. U+002F, U+002D, U+002E |
| `time_separator` | e.g. U+003A |
| `use_24_hour` | 1 = 24-hour cycle, 0 = 12-hour cycle |
| `first_day_of_week` | 0 = Sunday … 6 = Saturday |
| `weekend_start`, `weekend_end` | The two weekend days, same index |
| `long_month_name` | 1 = long month form, 0 = numeric |

### 2.4 Collation indexing domain

SLDF does **not** define a collation algorithm; SUCA (SUTS-001) does. SLDF
supplies each locale's **collation base**: the offset at which the locale's
primary weights begin, mapped onto the 31-bit SuperUnicode address space or the
RNUR neography plane. A collator seeds its implicit weights from this base, so
two locales sort unassigned and native codepoints consistently without a
per-locale weight table.

The base is computed in O(1) as `seed(collation_plane) + collation_base_offset`:

| Plane | Seed |
|-------|------|
| Bridge | `0x00000000` |
| SCP | `0x00110000` |
| Native | `0x00120000` |
| RNUR | `0x00160000` |

A Bridge-plane locale with offset `0x0100` has base `0x00000100`; an RNUR-plane
locale with the same offset has base `0x00160100`.

## 3. RCCR subtag integration

### 3.1 The RCCR subtag key

SLDF selects a locale by its **RCCR subtag**: a BCP 47-style language tag such
as `en-US`, `zh-Hans-CN`, or a registered conlang code such as `lumer` or
`kupon`. RCCR is a *code* registry for language and script identification; it
allocates no codepoints and contributes no entries to the character namespace.
The subtag is purely the key that selects a block.

Selection is **exact**: SLDF does not perform BCP 47 language-range matching,
because range matching is a multi-pass, potentially allocating procedure. A
query either names a block that exists or resolves to nothing; the caller
chooses any desired fallback explicitly, one exact lookup at a time.

### 3.2 Normalization and the FNV-1a hash

Before hashing or comparison, a subtag is normalized:

1. ASCII letters A–Z are folded to a–z;
2. only `[a-z0-9-]` bytes are retained; any other byte makes the tag malformed;
3. the key is at most `SUTS_SLDF_MAX_TAG_LEN - 1` (15) bytes.

A well-formed RCCR subtag additionally begins with a letter, contains no
leading, trailing, or doubled HYPHEN-MINUS, and has no subtag longer than eight
bytes. Region subtags MAY begin with a digit (for example `es-419`).

The index key is the **FNV-1a 32-bit** hash of the normalized bytes, with offset
basis `2166136261` and prime `16777619`:

```c
h = 2166136261u;
for each normalized byte b:
    h ^= (uint8_t)b;
    h *= 16777619u;
```

Normalization makes lookup case-insensitive: `en-US` and `EN-us` normalize to
the same key and hash. Two locale tags are equal (SLDF-R2) when their normalized
forms are byte-for-byte equal.

### 3.3 Natural languages and conlang locales

The registry holds two kinds of block:

* **Natural-language locales** (Bridge plane) — `en-US`, `de-DE`, `ja-JP`, and
  so on — whose separators and symbols are Unicode Bridge codepoints.
* **Conlang locales** (RNUR plane) — registered conlang codes such as `lumer`
  and `kupon` — whose separators, symbols, and signs are RNUR neography
  codepoints in `0x00160000–0x0016FFFF`. These are the same codes that the RNUR
  registry (SUTR-8) uses for naming neography compound glyphs.

## 4. The registry and O(1) resolution

### 4.1 The static table

The registry is a static array of `sldf_locale_t` blocks, immutable and
read-only, with capacity `SUTS_SLDF_REGISTRY_CAPACITY` (64). The curated
reference table holds 15 blocks (Annex A). Extending the table is a
bridge-data update — a minor revision — and never changes the API or the
lookup complexity.

### 4.2 The open-addressing index

The table is indexed once into a fixed array of `SUTS_SLDF_INDEX_SIZE` (128)
slots. Each slot records whether it is occupied and, if so, the table index of
its block. The index is built by hashing each block's normalized `tag_key` and
placing it at `hash & (128 - 1)` with linear probing. A resolve hashes the
normalized query and probes until it finds a matching block or an empty slot.

The index is built lazily on the first resolve if `sldf_init_registry` has not
been called, and it is **idempotent**. It never grows at run time. Because
`SUTS_SLDF_REGISTRY_CAPACITY` (64) is half of `SUTS_SLDF_INDEX_SIZE` (128), the
load factor stays below 1 and a probe terminates.

### 4.3 Complexity

| Operation | Cost |
|-----------|------|
| `sldf_init_registry` | O(n) once, n ≤ 64; no allocation |
| `sldf_resolve_locale` | O(1) expected (bounded probes); no allocation |
| `sldf_locale_count` / `sldf_locale_at` | O(1) |
| `sldf_tag_hash` / `sldf_is_valid_tag` / `sldf_tags_equal` | O(len), len ≤ 15 |
| `sldf_plane_of` / `sldf_collation_base` | O(1) |

### 4.4 SCP invariance and trap exclusion

A codepoint stored as a separator, symbol, or sign MUST lie inside the SUCS
space and MUST NOT be:

* a System Control Plane marker in `0x00110000–0x0011FFFF`, which is out-of-band
  control data and never part of formatted output;
* a trap slot in `0x7FFFFFF0–0x7FFFFFFE`; or
* the Sentinel `0x7FFFFFFF`.

This is the SLDF form of the SuperUnicode SCP invariance: the control plane
carries directives, not glyphs, so it can never appear in a formatting symbol.

## 5. Zone and plane mapping

`sldf_plane_of` classifies a codepoint into its address-space plane in O(1),
using the boundaries of SUTR-0:

| Plane | Range | Notes |
|-------|-------|-------|
| Bridge | `0x00000000–0x0010FFFF` | permanently 1:1 with Unicode |
| SCP | `0x00110000–0x0011FFFF` | system directives |
| Native | `0x00120000–0x7FFFFFEF` | native allocations |
| RNUR | `0x00160000–0x0016FFFF` | neography carve-out (inside Native) |
| Trap | `0x7FFFFFF0–0x7FFFFFFE` | 15 unallocatable slots |
| Sentinel | `0x7FFFFFFF` | end-of-space marker |
| ExtSUCS | `> 0x7FFFFFFF` | unbounded plugin space |

The RNUR range is tested before the Native range, because it is carved out of
it.

## 6. Reference implementation

The reference implementation ships as a freestanding, zero-allocation C99
module, `include/suts/suts_sldf.h` + `src/suts/suts_sldf.c`, registered in the
`suts_static` library.

### 6.1 Header `suts_sldf.h`

```c
#define SUTS_SLDF_SPEC_ID        "SUTS-003"
#define SUTS_SLDF_SPEC_TITLE     "System Locale Data & Formatting Specification"
#define SUTS_SLDF_VERSION_MAJOR  1
#define SUTS_SLDF_VERSION_MINOR  0
#define SUTS_SLDF_VERSION_PATCH  0

#define SUTS_SLDF_MAX_TAG_LEN       16u
#define SUTS_SLDF_MAX_SYMBOL_LEN    8u
#define SUTS_SLDF_REGISTRY_CAPACITY 64u
#define SUTS_SLDF_INDEX_SIZE        128u
```

### 6.2 API

| Function | Purpose |
|----------|---------|
| `sldf_version_string()` | version string |
| `sldf_init_registry()` | build the index once; idempotent, allocation-free |
| `sldf_resolve_locale(tag)` | resolve an RCCR subtag to its block in O(1), or NULL |
| `sldf_locale_count()` | number of registered blocks |
| `sldf_locale_at(i)` | stable pointer to block `i`, or NULL |
| `sldf_tag_hash(tag)` | FNV-1a 32-bit hash of the normalized subtag |
| `sldf_is_valid_tag(tag)` | RCCR subtag well-formedness |
| `sldf_tags_equal(a, b)` | case-insensitive, hyphen-normalized equality |
| `sldf_plane_of(cp)` | address-space plane of a codepoint |
| `sldf_collation_base(locale)` | the locale's collation seed |

### 6.3 Worked examples

| Subtask | Result |
|---------|--------|
| `sldf_resolve_locale("en-US")` | block with decimal `.`, grouping `,`, `$` prefix |
| `sldf_resolve_locale("DE-de")` | the `de-DE` block (case-insensitive) |
| `sldf_resolve_locale("de-DE")` decimal | `,` (U+002C), grouping `.` (U+002E) |
| `sldf_resolve_locale("fr-FR")` grouping | U+202F narrow no-break space |
| `sldf_resolve_locale("hi-IN")` grouping | primary 3, secondary 2 (lakh/crore) |
| `sldf_resolve_locale("ar-EG")` decimal | U+066B, RTL hint set |
| `sldf_resolve_locale("lumer")` plane | RNUR; separators in `0x001600xx` |
| `sldf_resolve_locale("xx-YY")` | NULL (no such block) |
| `sldf_collation_base(sldf_resolve_locale("kupon"))` | `0x00160200` |

## 7. Testing

The reference module is covered by `tests/test_suts_sldf.c`, which exercises the
version, the tag helpers, the registry and enumeration, O(1) resolution
(including the case-insensitive and miss paths), all three formatting domains,
the RTL hints, the plane mapping, and the collation seeds. The test is also
included in the unified header-coexistence harness (`unified/test_unified.c`),
which compiles every module header in one translation unit.

## 8. Stability and versioning

* The `SLDF` identifiers, the `sldf_locale_t` layout, and the API are stable.
* Additive fields are appended before `reserved` or replace it in a minor
  revision; a reader ignores a field it does not know.
* The curated table may grow in a minor revision; the order of existing blocks
  is stable, and no tag is reused for a different locale.
* Removing or re-pointing a tag is a major revision and is avoided.

## 9. Conformance requirements

An implementation claims SLDF L0 when it provides the Bridge-plane numeric,
temporal, and collation domains with zero-allocation O(1) resolution. L1 adds
the RNUR plane and the plane mapping of §5. L2 adds plugin-sourced blocks over
ExtSUCS. In every level, `sldf_resolve_locale` MUST return a stable pointer to
read-only memory, and no lookup path may allocate.

### 9.1 Compliance matrix (reference implementation)

| Requirement | Reference status |
|-------------|------------------|
| Binary, fixed-size locale blocks | Yes |
| Zero heap allocation on any path | Yes |
| O(1) expected RCCR-subtag resolution | Yes |
| Case-insensitive, hyphen-normalized keys | Yes |
| Numeric, temporal, and collation domains | Yes |
| RNUR-plane conlang locales and plane mapping | Yes |
| SCP-invariant separators/symbols | Yes |
| ExtSUCS compatibility (64-bit fast path) | Yes |

---

## Annex A — Curated registry contents

The reference table holds these blocks. Conlang locales are marked (RNUR).

| Tag | Plane | Decimal | Grouping | Date order | Currency |
|-----|-------|---------|----------|------------|----------|
| `en-US` | Bridge | `.` | `,` (3,3) | MDY, 12h | `$` prefix |
| `en-GB` | Bridge | `.` | `,` (3,3) | DMY, 24h | `£` prefix |
| `de-DE` | Bridge | `,` | `.` (3,3) | DMY, 24h | `€` suffix |
| `fr-FR` | Bridge | `,` | U+202F (3,3) | DMY, 24h | `€` suffix |
| `es-ES` | Bridge | `,` | `.` (3,3) | DMY, 24h | `€` suffix |
| `it-IT` | Bridge | `,` | `.` (3,3) | DMY, 24h | `€` suffix |
| `ru-RU` | Bridge | `,` | U+00A0 (3,3) | DMY, 24h | `₽` suffix |
| `zh-CN` | Bridge | `.` | `,` (3,3) | YMD, 24h | `¥` prefix |
| `ja-JP` | Bridge | `.` | `,` (3,3) | YMD, 24h | `¥` prefix |
| `hi-IN` | Bridge | `.` | `,` (3,2) | DMY, 12h | `₹` prefix |
| `ar-EG` | Bridge | U+066B | U+066C (3,3) | DMY, 12h | `ج.م` suffix |
| `he-IL` | Bridge | `.` | `,` (3,3) | DMY, 24h | `₪` prefix |
| `th-TH` | Bridge | `.` | `,` (3,3) | DMY, 24h | `฿` prefix |
| `lumer` (RNUR) | RNUR | `0x160010` | `0x160011` (3,0) | YMD, 24h | `0x160020` prefix |
| `kupon` (RNUR) | RNUR | `0x160012` | `0x160013` (3,2) | DMY, 12h | `0x160021 0x160022` suffix |

---

## References

- **UTS #35** — Unicode Locale Data Markup Language (LDML; the user-space
  system this kernel primitive deliberately departs from)
- **RCCR** — Reddit Conlang Code Registry (language and script subtag keys,
  including `lumer` and `kupon`)
- **SUAS-001 (SDF)** — Structural Directional Framing (RTL scope hint)
- **SUAS-005 (SSP)** — System Script Property (script-id hint)
- **SUTS-001 (SUCA)** — SuperUnicode Collation Algorithm (the collation domain
  SLDF seeds)
- **SUTR-0 (SUCS Core)** — the three-space layout, SCP, traps, and Sentinel
- **SUTR-5 (Plugin Lifecycle)** — the neography carve-out and plugin mounting
- **SUTR-8 (SUCPM)** — the RNUR registry space and property tiers
- **SUTR-9 (SUMNLS)** — the Native Math ExtSUCS space
- **SUTR-10 (SUCM)** — the conformance model and the zero-allocation mandate
