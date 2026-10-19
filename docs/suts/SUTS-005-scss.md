# SUTS-005 — System Character Security Specification (SCSS)

**Standard ID:** SUTS-005
**Status:** Draft — Authoring
**Scope:** Modular Technical Specification (visual homographs, identifier security, script isolation)
**Applies To:** Base SUCS (Unicode Compatibility Bridge, SCP, Native), ExtSUCS (unbounded 64-bit plane), the RNUR neography registry, the RCCR language and script subtags, SUCD property tables, the OpenWindows microkernel, the OpenWindows shell, the authentication and capability path, the `suf` font engine, the character token stream
**Last Updated:** 2026-10-19

---

## 1. Introduction

A string is not a string is not a string. A human reading the word
`раypal` cannot tell, at a glance, which of its letters are Latin and which
are Cyrillic; a program that compares codepoints can, but a program that
*displays* the string to a human usually does not. The attack that exploits
this gap — the **homograph** or *confusable* attack — is the single most
enduring character-level threat in a connected system: a login page whose host
name is `аpple.com` (Cyrillic `а`), a shell alias that resolves to a different
binary than the one an auditor read, an IPC channel whose name is
indistinguishable from a privileged one.

The Unicode Standard addresses this in **UTS #39, Unicode Security
Mechanisms**, which ships two flat text files — `Confusables.txt` (a
script-neutral skeleton mapping) and `IdentifierStatus.txt` (an
Allowed/Restricted verdict per codepoint) — that a user-space application
reads, tokenizes, and interprets at run time.

**SUTS-005 (SCSS) is the System Character Security Specification.** It is a core
kernel primitive of the OpenWindows microkernel. SCSS establishes a
**binary-compiled, zero-allocation schema** for detecting visual homographs,
evaluating identifier permissibility, and enforcing script-boundary isolation
across SuperUnicode string streams — bypassing traditional text-based security
databases in favour of fixed-size, byte-aligned memory layouts optimized for
O(1) kernel execution.

A subsystem asks three questions of the kernel and nothing else:

1. *May this codepoint appear in a secure identifier?*
2. *Is this codepoint a visual homograph of that one?*
3. *May these two codepoints share one identifier without crossing a script
   boundary?*

SCSS answers each with a bounded probe of a fixed-size index over an immutable
static table.

### 1.1 OpenWindows kernel principles

In keeping with OpenWindows kernel principles, SCSS ditches text-based security
databases, heap-resident lookup trees, and any per-process confusable cache.
Instead, it defines a **compiled binary schema**: the security metadata is
compiled to immutable, statically-linked tables of fixed-size C structures
(`scss_security_attr_t`, `scss_skeleton_entry_t`). The microkernel reads the
flags byte directly from a fixed memory offset during single-pass identifier
validation. A query is a single probe of a fixed-size open-addressing index
built once over a static array, so it performs a bounded number of integer
operations with no call to a general allocator.

### 1.2 Relationship to UTS #39

SCSS solves the same problem as UTS #39 *Unicode Security Mechanisms*:
classifying a codepoint as admissible in a secure identifier, and mapping a
confusable variant onto a canonical prototype so that two strings that look
alike can be told apart. It is deliberately **not** a UTS #39 analog, because
the two systems differ in three structural ways:

1. **Text databases vs. compiled binary structures.** `Confusables.txt` and
   `IdentifierStatus.txt` are UTF-8 flat files that a conforming
   implementation reads, tokenizes on `;`, and interprets at run time.
   Interpreting one line requires file I/O, a text parser, and dynamic string
   manipulation. A kernel primitive cannot afford a flat-file parser on the
   authentication path. SCSS discards the text databases entirely in favour of
   byte-aligned, statically-compiled C structures that sit directly in
   read-only memory.
2. **Heap allocation vs. zero allocation.** A user-space UTS #39 implementation
   loads the two data files into dynamic structures — hash maps from codepoint
   to skeleton, per-script identifier tables, fallback chains — built on the
   heap at start-up. OpenWindows enforces a strict zero-allocation mandate for
   core kernel subsystems. SCSS achieves O(1) lookup through fixed-size, static
   tables without ever touching a heap.
3. **User-space vs. kernel-primitive scope.** UTS #39 is designed for
   user-space text processors, browsers, and identity libraries that read the
   files from disk. SCSS is a low-level kernel primitive: it gives the
   OpenWindows microkernel, the shell parser, and the capability path
   deterministic, in-band decisions without depending on an external file
   system, a user-space service, or a per-process cache.

Where this document shares vocabulary with UTS #39 (confusable, skeleton,
identifier status, script), the requirements are normative for SCSS only.

### 1.3 Key architectural shifts

| Dimension | UTS #39 (user-space) | SUTS-005 SCSS (kernel) |
|-----------|----------------------|------------------------|
| Representation | `Confusables.txt` + `IdentifierStatus.txt` parsed at run time | Fixed-size C structs in read-only memory |
| Allocation | Dynamic maps and per-script tables on the heap | Zero; two static tables + two fixed indexes, no heap |
| Scope | Browsers, identity libraries, text processors | Kernel primitive: microkernel, shell, capability path |
| Lookup | Parse, tokenize, then a map lookup | codepoint → FNV-1a → fixed open-addressing probe, O(1) |
| Verdict | Allowed / Restricted per codepoint, from text | 32-bit compiled flag word, computed in O(1) |
| Boundary | Script set per identifier, user-space policy | Explicit Bridge/Native isolation test |
| Space coverage | 21-bit Unicode (0x0000–0x10FFFF) | Full 31-bit SUCS plus 64-bit ExtSUCS |

### 1.4 The dual-layer architecture

SCSS is a **dual-layer** system:

* **Offline reference layer — SUCD.** The full text manifests: the confusable
  mappings, the identifier profiles, and the human-readable security tables
  live in the SuperUnicode Character Database for compilers, developer
  toolchains, and user-space web applications.
* **Runtime kernel layer — SCSS.** Build-time tooling **distils** those SUCD
  security datasets into static, immutable, byte-packed binary tables
  embedded directly in the kernel memory image. The kernel never reads SUCD at
  run time.

> The seeding brief named the offline reference layer "SUCA". In this corpus
> **SUCA** is already the SuperUnicode Collation Algorithm (SUTS-001), so SCSS
> names **SUCD** as its reference layer — the same resolution SUAS-009 (SIP)
> adopts.

### 1.5 Script boundary flags

SCSS encodes explicit **script-isolation bitfield flags** to protect system
paths from cross-script injection, specifically controlling interactions
between the **Unicode Compatibility Bridge** space (`0x000000–0x10FFFF`) and
the **Native & Neography Space** (`0x160000–0x16FFFF`, and the Native plane
generally). A Native-plane codepoint always carries
`SUTS_SCSS_FLAG_NEOGRAPHY_MIXED`, meaning it *requires an isolated script
context*: it may appear in a neography or conlang identifier, but it may never
be silently mixed into a Bridge identifier. `scss_crosses_bridge_boundary`
tests the boundary directly.

### 1.6 Terminology and cross-references

SCSS reuses the SuperUnicode architecture documents, numbered as they exist in
this repository:

| SCSS concept | Authoritative document |
|--------------|------------------------|
| Codepoint space, SCP, traps, Sentinel | **SUTR-0 (SUCS Core)** — three-space layout |
| Bridge / Native / ExtSUCS planes | **SUTR-7 (SUCEM)**, **SUTR-6 (Bridge)** |
| RNUR neography registry, property tiers | **SUTR-8 (SUCPM)** — the character property model |
| Identifier classes and lexical syntax | **SUAS-007 (SIS)** — UAX #31 analog |
| Script identification | **SUAS-005 (SSP)** — System Script Property |
| Ideographic properties | **SUAS-009 (SIP)** — UAX #38 analog |
| Named character sequences | **SUAS-008 (SNCS)** — UAX #34 analog |
| Conformance model and zero-allocation mandate | **SUTR-10 (SUCM)** |
| Offline reference data | **SUCD** — the SuperUnicode Character Database |

> The reference implementation ships as `include/suts/suts_scss.h` +
> `src/suts/suts_scss.c`. The sketched identifiers `SCSS_SPEC_ID`,
> `SCSS_SPEC_TITLE`, `SCSS_VERSION_MAJOR`, and `SCSS_VERSION_MINOR` become
> `SUTS_SCSS_SPEC_ID`, `SUTS_SCSS_SPEC_TITLE`, `SUTS_SCSS_VERSION_MAJOR`, and
> `SUTS_SCSS_VERSION_MINOR` (with `SUTS_SCSS_VERSION_PATCH` added), matching the
> house convention of the sibling SUTS modules. The sketched flags
> `SCSS_FLAG_*` become `SUTS_SCSS_FLAG_*`. The library and function names
> (`suts_scss.h`, `scss_*`) and the struct names (`scss_skeleton_entry_t`,
> `scss_security_attr_t`) are unchanged. The sketch typed the codepoint fields
> as `uint32_t`; the reference types them as `sucs_ex_char_t` (64-bit) so an
> RNUR neography codepoint in `0x00160000–0x0016FFFF` and an ExtSUCS plugin
> codepoint above `0x7FFFFFFF` survive intact.

### 1.7 Conformance

A conforming SCSS implementation MUST:

1. represent security metadata as immutable, fixed-size structures reachable
   directly from a codepoint key, with **no heap allocation** on any query
   path;
2. resolve a codepoint to its flag word in **O(1) expected time** by a bounded
   probe of a fixed-size index, not by a linear scan of a table;
3. resolve a confusable codepoint to its skeleton in **O(1) expected time**, and
   return the identity mapping when no skeleton exists;
4. admit no System Control Plane marker, trap slot, or the Sentinel as an
   admissible identifier codepoint (§4.4); and
5. classify every Native-plane codepoint as requiring an isolated script
   context, so a Bridge/Native mix is always detected (§2.3).

### 1.8 SCSS conformance levels

| Level | Name | Requires |
|-------|------|----------|
| L0 | Kernel Core | the Bridge security table; zero-allocation O(1) flag and skeleton resolution |
| L1 | Script Isolation | L0 plus the script groups of §2.3 and the Bridge/Native boundary test |
| L2 | Plugin & ExtSUCS | L1 plus attribute records sourced from a mounted plugin over the ExtSUCS space |

## 2. The compiled security schema

### 2.1 The security attribute

A compiled per-codepoint security record is one `scss_security_attr_t`:

```c
typedef struct {
    sucs_ex_char_t codepoint; /* the codepoint, e.g. U+200B             */
    uint32_t       flags;     /* SUTS_SCSS_FLAG_*                       */
} scss_security_attr_t;
```

The codepoint field is `sucs_ex_char_t` (64-bit), never `uint32_t` or `char`.
This is the central SCSS deviation from the sketched structure: an RNUR
neography codepoint in `0x00160000–0x0016FFFF` and an ExtSUCS plugin
codepoint above `0x7FFFFFFF` survive intact, whereas a narrower field would
truncate or misinterpret them.

The `flags` word carries the compiled security metadata:

| Flag | Bit | Meaning |
|------|-----|---------|
| `SUTS_SCSS_FLAG_ALLOWED_IDENTIFIER` | 0 | Safe for kernel/system paths and secure identifiers |
| `SUTS_SCSS_FLAG_RESTRICTED` | 1 | Invisible, unassigned, or control: never part of an identifier |
| `SUTS_SCSS_FLAG_CONFUSABLE` | 2 | A known visual homograph; carries a skeleton mapping (§2.2) |
| `SUTS_SCSS_FLAG_NEOGRAPHY_MIXED` | 3 | Requires an isolated script context (Native & Neography) |

The four flags are orthogonal bit positions, so one record can express
"confusable **and** requires isolation" — the case for an RNUR neography
lookalike of a Latin letter.

### 2.2 The skeleton mapping

A compiled confusable mapping is one `scss_skeleton_entry_t`:

```c
typedef struct {
    sucs_ex_char_t codepoint;          /* the lookalike, e.g. U+0430      */
    sucs_ex_char_t skeleton_codepoint; /* its prototype, e.g. U+0061 'a'  */
} scss_skeleton_entry_t;
```

`skeleton_codepoint` is the **prototype canonical skeleton** a confusable
variant maps onto. Two codepoints are **visual homographs** exactly when their
skeletons agree and their codepoints differ. A codepoint with no skeleton
record maps to **itself** (the identity mapping), which is why an ordinary
Latin `a` is recognized as confusable with Cyrillic `а` without requiring the
Latin `a` to carry its own record.

The skeleton is a single codepoint, not a string: SCSS deliberately does not
reproduce UTS #39's full multi-codepoint skeleton expansion. A kernel path
check needs the *equivalence class*, not the rendering string, and a
single-codepoint prototype keeps the record fixed-size, byte-aligned, and
O(1).

### 2.3 Script groups and script-boundary isolation

SCSS classifies a codepoint into a **script group**:

```c
typedef enum {
    SUTS_SCSS_SCRIPT_COMMON   = 0, /* script-neutral digits, punctuation */
    SUTS_SCSS_SCRIPT_LATIN    = 1,
    SUTS_SCSS_SCRIPT_GREEK    = 2,
    SUTS_SCSS_SCRIPT_CYRILLIC = 3,
    SUTS_SCSS_SCRIPT_HAN      = 4,
    SUTS_SCSS_SCRIPT_ARABIC   = 5,
    SUTS_SCSS_SCRIPT_HEBREW   = 6,
    SUTS_SCSS_SCRIPT_NATIVE   = 7, /* Native / RNUR neography plane      */
    SUTS_SCSS_SCRIPT_SCP      = 8, /* SCP markers, traps, the Sentinel   */
    SUTS_SCSS_SCRIPT_UNKNOWN  = 9  /* unclassified (ExtSUCS plugins)     */
} suts_scss_script_t;
```

The rules are normative:

* `COMMON` covers script-neutral codepoints — ASCII digits, punctuation, and
  symbols — that may combine with any script without being a violation.
* `NATIVE` is the **Native & Neography plane**, `0x00120000–0x7FFFFFEF`
  (the RNUR carve-out `0x00160000–0x0016FFFF` included). A Native codepoint
  requires an isolated script context.
* `SCP` covers SCP markers, trap slots, and the Sentinel.
* `UNKNOWN` covers the ExtSUCS plugin space above `0x7FFFFFFF`, whose scripts
  are declared by a mounted plugin that the base image does not know.
* Everything else in `0x000000–0x10FFFF` is dispatched to the curated
  Bridge script ranges (Latin, Greek, Cyrillic, Han, Arabic, Hebrew).

`scss_script_compatible(a, b)` is true when the two codepoints may appear
together in one identifier: they share a script group, or either is
`COMMON`, or either is `UNKNOWN` (an unclassified plugin codepoint is not a
boundary violation). `scss_is_mixed_script` is its negation.

`scss_crosses_bridge_boundary(a, b)` is the specific test the standard
protects: true exactly when one codepoint is in the Unicode Compatibility
Bridge (`0x000000–0x10FFFF`) and the other in the Native & Neography plane.
This is the primary cross-script-injection vector — a system path that accepts
a Native logograph beside a Latin letter can be spoofed in both directions.

### 2.4 Identifier admissibility

`scss_is_allowed_identifier` is true when a codepoint carries
`SUTS_SCSS_FLAG_ALLOWED_IDENTIFIER` and **none** of `RESTRICTED`,
`CONFUSABLE`, or `NEOGRAPHY_MIXED`. Three defaults make the predicate total
over the whole codepoint space, not merely over curated records:

* the ASCII identifier class (`A`–`Z`, `a`–`z`, `0`–`9`, `_`, `-`) is allowed
  by default;
* an SCP marker, trap slot, or the Sentinel is `RESTRICTED` by default;
* any Native-plane codepoint is `NEOGRAPHY_MIXED` by default.

A codepoint that matches none of these and is not curated carries no flags and
is therefore **not** admissible — a secure identifier is closed by default.

## 3. Homographs and identifier security

### 3.1 The canonical skeleton

A codepoint resolves to its skeleton by one probe. `scss_resolve_skeleton`
returns the mapped prototype, or the original codepoint when no mapping exists.
The mapping is **not** required to be idempotent across scripts: mapping
Cyrillic `а` (`U+0430`) to Latin `a` (`U+0061`) and leaving Latin `a` as
itself produces one equivalence class `{a, а, α, ⲁ…}` — exactly the class a
confusable check needs.

### 3.2 Confusable pairs

`scss_confusable_with(a, b)` is true exactly when `a != b` and their skeletons
agree. Because the skeleton of an unmapped codepoint is the codepoint itself,
a plain Latin letter is recognized as confusable with any curated lookalike of
it without a record for the Latin letter.

### 3.3 Restricted codepoints

A **restricted** codepoint is one that carries `SUTS_SCSS_FLAG_RESTRICTED`:
invisible formatting (zero-width spaces and joiners, the soft hyphen, the
byte-order mark), bidi attack controls (the right-to-left override), isolate
controls, plus the SCP markers, trap slots, and the Sentinel. None may appear
in an identifier; SCSS detects them in O(1) with no scan for a range.

### 3.4 Script mixing

Script mixing is the generalization of the homograph attack from a single
character to a whole identifier: `apple` (Latin) and `аррle` (Cyrillic `а`,
`р`, `р`) are visually near-identical, but only a *mixing* check catches the
case where a single identifier contains both scripts. `scss_is_mixed_script`
catches it in O(1) per adjacent pair, and the layout stream can therefore
reject a mixed identifier in a single pass.

## 4. The registry and O(1) resolution

### 4.1 The static tables

The registry is two static arrays, immutable and read-only: a table of
`scss_security_attr_t` records with capacity `SUTS_SCSS_ATTR_CAPACITY` (128),
and a table of `scss_skeleton_entry_t` records with capacity
`SUTS_SCSS_SKELETON_CAPACITY` (128). The curated reference data holds 31
attribute records and 22 skeleton records (Annex A). Extending the tables is a
bridge-data update — a minor revision — and never changes the API or the lookup
complexity.

### 4.2 The codepoint hash and the open-addressing indexes

Each table is indexed once into a fixed array of `SUTS_SCSS_INDEX_SIZE` (256)
slots. Each slot records whether it is occupied and, if so, the table index of
its record. An index is built by hashing each record's codepoint and placing it
at `hash & (256 - 1)` with linear probing.

The key is the **FNV-1a 32-bit** hash over the little-endian encoding of the
codepoint, with offset basis `2166136261` and prime `16777619`:

```c
h = 2166136261u;
for i in 0..7: h ^= (codepoint >> (8 * i)) & 0xFF; h *= 16777619u;
```

A query hashes the codepoint and probes until it finds a matching record or an
empty slot. Both indexes are built lazily on the first lookup if
`scss_init_registry` has not been called, and the build is **idempotent**. It
never grows at run time. Because both capacities (128) are half of
`SUTS_SCSS_INDEX_SIZE` (256), the load factor stays below 1/2 and a probe
terminates.

### 4.3 Complexity

| Operation | Cost |
|-----------|------|
| `scss_init_registry` | O(n) once, n ≤ 128 per table; no allocation |
| `scss_get_flags` | O(1) expected (bounded probes); no allocation |
| `scss_resolve_skeleton` | O(1) expected (bounded probes); no allocation |
| `scss_confusable_with` | O(1) expected (two probes) |
| `scss_is_allowed_identifier` / `scss_is_restricted` / `scss_is_confusable` / `scss_is_neo_mixed` | O(1) expected |
| `scss_lookup_attr` / `scss_lookup_skeleton` | O(1) expected |
| `scss_attr_count` / `scss_attr_at` | O(1) |
| `scss_skeleton_count` / `scss_skeleton_at` | O(1) |
| `scss_script_of` / `scss_script_compatible` / `scss_is_mixed_script` / `scss_crosses_bridge_boundary` | O(1) |
| `scss_is_bridge_codepoint` / `scss_is_native_codepoint` / `scss_plane_of` / `scss_hash` | O(1) |

### 4.4 SCP invariance and trap exclusion

A codepoint admitted as a secure identifier MUST NOT be:

* a System Control Plane marker in `0x00110000–0x0011FFFF`, which is out-of-band
  control data and never part of rendered text;
* a trap slot in `0x7FFFFFF0–0x7FFFFFFE`; or
* the Sentinel `0x7FFFFFFF`.

This is the SCSS form of the SuperUnicode SCP invariance: the control plane
carries directives, not glyphs, so it can never be admissible.
`scss_get_flags` ORs `SUTS_SCSS_FLAG_RESTRICTED` for all three, and the
script group of all three is `SUTS_SCSS_SCRIPT_SCP`. No attribute or skeleton
record in the reference table falls in any of the three ranges; the test suite
asserts this (§9).

## 5. Zone and plane mapping

`scss_plane_of` classifies a codepoint into its address-space plane in O(1),
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
it. The Bridge plane and the RNUR plane are the two spaces of §1.5; all other
planes are classified for completeness.

## 6. Kernel integration

SCSS sits on the identifier-validation path of the OpenWindows microkernel, the
shell, and the capability system:

1. The parser emits one codepoint at a time. For each codepoint it calls
   `scss_get_flags`, which returns the compiled word in O(1).
2. A word carrying `RESTRICTED`, `CONFUSABLE`, or `NEOGRAPHY_MIXED` fails the
   identifier check; the parser reports a security fault at the exact offending
   position without rescanning the string.
3. When two identifiers must be compared for spoofing rather than one
   identifier validated, `scss_confusable_with` and `scss_script_compatible`
   give the pairwise verdicts in O(1) each.

The whole path is single-pass and allocation-free: one probe per codepoint, a
fixed amount of stack, and no reconstruction of any security database.

## 7. The compile-time registration model

SCSS replaces the run-time read of the UTS #39 flat files with a **compile-time
registration model**. The registration authority still accepts submissions in
the usual interchange forms and still reviews them; the difference is that the
approved data is *compiled* into the binary tables of §4.1, and the kernel never
reads a file at run time.

### 7.1 Confusable submissions

A confusable submission names a codepoint and a prototype skeleton codepoint.
At compile time the pair becomes one `scss_skeleton_entry_t`, and the source
codepoint is flagged `SUTS_SCSS_FLAG_CONFUSABLE` in its attribute record.

### 7.2 Identifier-status submissions

An identifier-status submission names a codepoint and an
Allowed/Restricted/Confusable verdict. At compile time the verdict becomes the
`flags` word of one `scss_security_attr_t`.

### 7.3 Review

A submission is reviewed against the SUCD policy: a confusable mapping must not
collapse two codepoints that are distinguishable in a secure context, and an
"allowed" verdict must not admit an invisible or control codepoint. Approved
submissions are compiled into the tables as a minor revision.

## 8. Reference implementation

The reference implementation ships as a freestanding, zero-allocation C99
module, `include/suts/suts_scss.h` + `src/suts/suts_scss.c`, registered in the
`suts_static` library.

### 8.1 Header `suts_scss.h`

```c
#define SUTS_SCSS_SPEC_ID           "SUTS-005"
#define SUTS_SCSS_SPEC_TITLE        "System Character Security Specification"
#define SUTS_SCSS_VERSION_MAJOR     1
#define SUTS_SCSS_VERSION_MINOR     0
#define SUTS_SCSS_VERSION_PATCH     0

#define SUTS_SCSS_ATTR_CAPACITY     128U
#define SUTS_SCSS_SKELETON_CAPACITY 128U
#define SUTS_SCSS_INDEX_SIZE        256U

#define SUTS_SCSS_FLAG_ALLOWED_IDENTIFIER (1u << 0)
#define SUTS_SCSS_FLAG_RESTRICTED         (1u << 1)
#define SUTS_SCSS_FLAG_CONFUSABLE         (1u << 2)
#define SUTS_SCSS_FLAG_NEOGRAPHY_MIXED    (1u << 3)
```

### 8.2 API

| Function | Purpose |
|----------|---------|
| `scss_version_string()` | version string |
| `scss_init_registry()` | build both indexes once; idempotent, allocation-free |
| `scss_get_flags(cp)` | compiled flag word of a codepoint, in O(1) |
| `scss_resolve_skeleton(cp)` | prototype skeleton, or identity |
| `scss_is_allowed_identifier(cp)` | secure-identifier admissibility |
| `scss_is_restricted(cp)` | `RESTRICTED` predicate |
| `scss_is_confusable(cp)` | `CONFUSABLE` predicate |
| `scss_is_neo_mixed(cp)` | `NEOGRAPHY_MIXED` predicate |
| `scss_confusable_with(a, b)` | whether two codepoints are homographs |
| `scss_lookup_attr(cp)` | stable pointer to the attribute record, or `NULL` |
| `scss_lookup_skeleton(cp)` | stable pointer to the skeleton record, or `NULL` |
| `scss_attr_count()` / `scss_attr_at(i)` | enumerate attribute records |
| `scss_skeleton_count()` / `scss_skeleton_at(i)` | enumerate skeleton records |
| `scss_script_of(cp)` | coarse script group |
| `scss_script_compatible(a, b)` | whether two codepoints share an identifier |
| `scss_is_mixed_script(a, b)` | the boundary violation |
| `scss_crosses_bridge_boundary(a, b)` | Bridge vs. Native crossing |
| `scss_is_bridge_codepoint(cp)` | Bridge membership |
| `scss_is_native_codepoint(cp)` | Native-plane membership (RNUR included) |
| `scss_plane_of(cp)` | address-space plane of a codepoint |
| `scss_hash(cp)` | FNV-1a hash of the codepoint |

### 8.3 Worked examples

| Subtask | Result |
|---------|--------|
| `scss_get_flags(0x0041)` | `ALLOWED_IDENTIFIER` (ASCII default) |
| `scss_get_flags(0x0430)` | `CONFUSABLE` (curated Cyrillic `а`) |
| `scss_get_flags(0x160100)` | `CONFUSABLE \| NEOGRAPHY_MIXED` (RNUR lookalike) |
| `scss_get_flags(0x110000)` | `RESTRICTED` (SCP marker) |
| `scss_is_allowed_identifier(0x0430)` | `false` (a homograph is never admissible) |
| `scss_is_allowed_identifier(0x4E00)` | `true` (curated ideograph) |
| `scss_resolve_skeleton(0x0430)` | `0x0061` |
| `scss_resolve_skeleton(0x0061)` | `0x0061` (identity) |
| `scss_confusable_with(0x0430, 0x03B1)` | `true` (Cyrillic `а` ≈ Greek `α`) |
| `scss_confusable_with(0x0430, 0x0440)` | `false` (`а` ≠ `р`) |
| `scss_crosses_bridge_boundary(0x0041, 0x160100)` | `true` |
| `scss_script_of(0x80000000)` | `UNKNOWN` (ExtSUCS plugin space) |
| `scss_plane_of(0x160000)` | `SUTS_SCSS_PLANE_RNUR` |

## 9. Testing

The reference module is covered by `tests/test_suts_scss.c`, which exercises the
version, the flag word and every predicate, the skeleton mapping and
confusability, the two registries and their enumeration (including uniqueness of
each codepoint key and SCP invariance of the records), script-group
classification and boundary isolation, zone classification, and the codepoint
hash. The test is also included in the unified header-coexistence harness
(`unified/test_unified.c`), which compiles every module header in one
translation unit.

## 10. Stability and versioning

* The `SCSS` identifiers, the `scss_security_attr_t` and `scss_skeleton_entry_t`
  layouts, and the API are stable.
* Additive fields are appended before the reserved tail or replace it in a
  minor revision; a reader ignores a field it does not know.
* The curated tables may grow in a minor revision; the order of existing
  records is stable, and **no codepoint is ever re-keyed to a different
  verdict**.
* Removing a record or re-pointing a skeleton is a major revision and is
  avoided.

## 11. Conformance requirements

An implementation claims SCSS L0 when it provides the Bridge security table with
zero-allocation O(1) flag and skeleton resolution. L1 adds the script groups of
§2.3 and the Bridge/Native boundary test. L2 adds plugin-sourced attribute
records over ExtSUCS. In every level, `scss_lookup_attr` and
`scss_lookup_skeleton` MUST return a stable pointer to read-only memory, and no
query path may allocate.

### 11.1 Compliance matrix (reference implementation)

| Requirement | Reference status |
|-------------|------------------|
| Binary, fixed-size attribute and skeleton records | Yes |
| Zero heap allocation on any path | Yes |
| O(1) expected flag and skeleton resolution | Yes |
| Unique codepoint key in each table | Yes |
| Bridge + RNUR security coverage | Yes |
| Explicit Bridge/Native script-isolation test | Yes |
| SCP-invariant identifiers; traps and Sentinel restricted | Yes |
| Closed-by-default identifier verdict | Yes |
| 64-bit extSUCS compatibility | Yes |

---

## Annex A — Curated registry contents

The reference data holds **31** attribute records and **22** skeleton records.
Every attribute record's flag word, and every skeleton mapping, is listed below.

### A.1 Attribute records (`scss_security_attr_t`)

| Group | Codepoints | Flags |
|-------|-----------|-------|
| Cyrillic lowercase homoglyphs | `0430 0435 043E 0440 0441 0445 0443 043A 043C 0442` | `CONFUSABLE` |
| Cyrillic uppercase homoglyphs | `0410 0415 041E 0421 0420 0422` | `CONFUSABLE` |
| Greek homoglyphs | `03BF 03B1 03BD 039F` | `CONFUSABLE` |
| RNUR neography lookalikes | `160100 160200` | `CONFUSABLE \| NEOGRAPHY_MIXED` |
| Invisible / control | `00AD 200B 200C 200D 202E 2066 FEFF` | `RESTRICTED` |
| Curated allowed ideographs | `4E00 4E8C` | `ALLOWED_IDENTIFIER` |

### A.2 Skeleton records (`scss_skeleton_entry_t`)

| Prototype | Confusable variants |
|-----------|---------------------|
| `U+0061` `a` | `0430` (Cyrillic а), `03B1` (Greek α), `160100` (RNUR) |
| `U+0065` `e` | `0435` (Cyrillic е) |
| `U+006F` `o` | `043E` (Cyrillic о), `03BF` (Greek ο), `160200` (RNUR) |
| `U+0070` `p` | `0440` (Cyrillic р) |
| `U+0063` `c` | `0441` (Cyrillic с) |
| `U+0078` `x` | `0445` (Cyrillic х) |
| `U+0079` `y` | `0443` (Cyrillic у) |
| `U+006B` `k` | `043A` (Cyrillic к) |
| `U+006D` `m` | `043C` (Cyrillic м) |
| `U+0074` `t` | `0442` (Cyrillic т) |
| `U+0041` `A` | `0410` (Cyrillic А) |
| `U+0045` `E` | `0415` (Cyrillic Е) |
| `U+004F` `O` | `041E` (Cyrillic О), `039F` (Greek Ο) |
| `U+0043` `C` | `0421` (Cyrillic С) |
| `U+0050` `P` | `0420` (Cyrillic Р) |
| `U+0054` `T` | `0422` (Cyrillic Т) |
| `U+0076` `v` | `03BD` (Greek ν) |

---

## Annex B — Worked example

Consider a login path that validates a user handle before resolving it against
the capability table. The attacker supplies the handle `раypal` — three
Cyrillic letters (`U+0440 U+0430 U+0443`), one Latin letter, and one digit,
spelled to look like `paypal`.

Walking the handle one codepoint at a time, in a single pass with no
allocation:

| Position | Codepoint | `scss_get_flags` | Verdict |
|----------|-----------|------------------|---------|
| 0 | `U+0440` Cyrillic `р` | `CONFUSABLE` | reject — homograph of `p` |
| 1 | `U+0430` Cyrillic `а` | `CONFUSABLE` | reject — homograph of `a` |
| 2 | `U+0443` Cyrillic `у` | `CONFUSABLE` | reject — homograph of `y` |
| 3 | `U+0070` Latin `p` | `ALLOWED_IDENTIFIER` | admit |
| 4 | `U+0061` Latin `a` | `ALLOWED_IDENTIFIER` | admit |
| 5 | `U+0079` Latin `y` | `ALLOWED_IDENTIFIER` | admit |
| 6 | `U+0061` Latin `a` | `ALLOWED_IDENTIFIER` | admit |
| 7 | `U+006C` Latin `l` | `ALLOWED_IDENTIFIER` | admit |

The first codepoint already fails, at position 0, in one probe. Had the
handle instead been a *mixed* identifier — `payрal`, one Cyrillic `р` among
Latin letters — a homograph check on the offending codepoint alone still
catches it, and `scss_script_compatible` additionally reports the Latin/Cyrillic
boundary violation for the pair `(U+0070, U+0440)`.

A Bridge/Native mix is caught by the same machinery: a system path that
concatenates a Latin prefix with a Native logograph is rejected by
`scss_crosses_bridge_boundary` in O(1) per adjacent pair.

---

## References

- **UTS #39** — Unicode Security Mechanisms (the user-space flat-file system
  this kernel primitive deliberately departs from)
- **SUAS-007 (SIS)** — System Identifiers and Syntax (identifier classes, the
  lexical filter SCSS sits in front of)
- **SUAS-005 (SSP)** — System Script Property (the finer-grained script engine
  SCSS's coarse groups compress)
- **SUAS-008 (SNCS)** — System Named Character Sequences (multi-codepoint
  symbols, which SCSS also admits)
- **SUAS-009 (SIP)** — System Ideographic Properties (the sibling SUCD-backed
  binary kernel primitive)
- **SUTS-001 (SUCA)** — SuperUnicode Collation Algorithm (the collation engine)
- **SUTS-003 (SLDF)** — System Locale Data & Formatting (the sibling
  binary-compiled kernel primitive)
- **SUTS-004 (SIVD)** — System Ideographic Variation Database (the sibling
  binary-compiled kernel primitive)
- **SUTR-0 (SUCS Core)** — the three-space layout, SCP, traps, and Sentinel
- **SUTR-6 (Unicode Compatibility Bridge)** — the permanent 1:1 guarantee and
  the Bridge boundary SCSS protects
- **SUTR-7 (SUCEM)** — the four-level character encoding model
- **SUTR-8 (SUCPM)** — the RNUR registry space and property tiers
- **SUTR-10 (SUCM)** — the conformance model and the zero-allocation mandate
- **suf** — the SuperUnicode Font format and vector font engine
