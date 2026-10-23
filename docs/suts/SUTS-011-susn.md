# SUTS-011 — SuperUnicode Set Notation (SUSN)

**Standard ID:** SUTS-011  
**Status:** Draft — Authoring  
**Scope:** Modular Technical Specification (binary-mapped character-set algebra, inversion lists, range-filtering masks, O(1) membership, RNUR neography/conscript set integration)  
**Applies To:** Base SUCS (Unicode Compatibility Bridge, SCP, Native), the RNUR neography registry, SUAS-001 (SDF), SUTR-10 (SUCM), the OpenWindows microkernel, identifier validators, font fallback, policy engines, and any consumer that tests set membership without a heap  
**Last Updated:** 2026-10-23

---

## 1. Introduction

Plain text processing asks a simple question: is this codepoint a member of some character set? Traditional Unicode implementations answer it by parsing bracketed set notation at run time, building mutable set objects, or dispatching through a tree of properties. A kernel that must decide identifier admissibility, homograph membership, or presentation class for every codepoint in a stream cannot afford those costs on every query.

SUSN takes the opposite approach. Following the compiled-table discipline of the SUTS family (and modeling UTS #61, "Unicode Set Notation"), SUSN treats a set as data: an immutable descriptor and a sorted, non-overlapping array of inclusive codepoint ranges. Every algebraic operation - union, intersection, difference, symmetric difference, complement - is resolved at build time or image-construction time into canonical positive ranges. The result is a data structure that can answer membership in O(1) time with a bounded binary search and zero heap allocations.

Three commitments define SUSN:

1. **Binary-mapped set algebra.** A set is `susn_set_descriptor_t` plus a flat `susn_codepoint_range_t` array. There is no expression tree, no token stream, and no run-time string parser on any code path.
2. **Inversion lists.** The `SUSN_FLAG_NEGATED` bit turns the same range array into an inversion list. The ranges name the members of the set, and the flag means membership is the complement of that list over the 31-bit universe. All algebra operators accept negated operands directly.
3. **Zero allocation, O(1) membership.** Every table is static, the fixed-capacity builder is caller-owned, and every query is a bounded probe. No path reaches for `malloc`, `stdio`, or a libc string routine.

SUSN is modular and optional. A minimal kernel may omit it; consumers that opt in obtain predictable set membership with deterministic cost.

### 1.1 Relationship to UTS #61

UTS #61 defines a textual set notation and a grammar for constructing sets. SUSN does not implement that parser; it defines the compiled destination: the range arrays, inversion lists, and the algebra that produces them. An implementation may feed a UTS #61 parser's output into `suts_susn_builder_*`, resolve sets at build time with `suts_susn_combine` / `suts_susn_complement`, and ship the resulting descriptors as static tables.

### 1.2 Conformance and optionality

SUSN is a **SUTS** (modular technical specification), registered with SSRE as kind OPTIONAL. It carries no dependency on a particular SUAS beyond the architecture's address-space invariants and the 31-bit SUCS universe. It is freestanding C99.

SUTS-010 does not exist; the sequence includes a deliberate gap. SUTS registrations remain independent and gaps are permitted.

---

## 2. Address space and zones

The universe is the full 31-bit SuperUnicode address space, `0x00000000` through `0x7FFFFFFF` (`SUSN_UNIVERSE_MAX`). Codepoints above the Sentinel are not members of any 31-bit set.

The engine classifies codepoints into zones:

| Zone | Range | Meaning |
|------|-------|---------|
| `SUSN_ZONE_BRIDGE` | `0x00000000-0x0010FFFF` | Unicode Compatibility Bridge |
| `SUSN_ZONE_SCP` | `0x00110000-0x0011FFFF` | System Control Plane |
| `SUSN_ZONE_RNUR` | `0x00160000-0x0016FFFF` | Native & Neography Plane (RNUR carve-out) |
| `SUSN_ZONE_NATIVE` | `0x00120000-0x7FFFFFEF` | Native plane (excludes RNUR when used as a disjoint slice, includes RNUR inside the native extent per the catalog) |
| `SUSN_ZONE_TRAP` | `0x7FFFFFF0-0x7FFFFFFE` | Trap slots |
| `SUSN_ZONE_SENTINEL` | `0x7FFFFFFF` | Sentinel |
| `SUSN_ZONE_EXTSUCS` | `> 0x7FFFFFFF` | ExtSUCS / plugin space (31-bit sets never contain these) |

The catalog includes zone sets (`SUSN_SETID_BRIDGE`, `SUSN_SETID_SCP`, `SUSN_SETID_RNUR`, `SUSN_SETID_NATIVE`, `SUSN_SETID_TRAP`, `SUSN_SETID_SENTINEL`).

### 2.1 Range filtering

`SUSN_FILTER_*` masks (`BRIDGE`, `SCP`, `RNUR`, `NATIVE`, `TRAP`, `SENTINEL`, `ALL`) restrict membership queries. `suts_susn_contains_in(set, cp, mask)` returns true only if the zone bit of `cp` is selected and `suts_susn_contains` would return true. `suts_susn_filter(set, zone_mask, out, capacity)` resolves any negated operand and produces a canonical positive range array containing only those members of `set` that fall in the selected zones. When both `NATIVE` and `RNUR` are selected, `RNUR` is not duplicated (the implementation merges zones without double-emission).

`SUSN_FILTER_EXTSUCS` has no bit; codepoints above the Sentinel are never admitted.

### 2.2 Probe masks

`suts_susn_probe_mask(set, first, stride, count)` builds a 32-bit occupancy mask over `count` samples (capped at 32): bit `i` is set if `suts_susn_contains(set, first + i*stride)` is true and the sample does not pass the Sentinel. `stride == 0` probes `first` alone. This is a block-rejection primitive for tokenizers and scanners.

---

## 3. Data model

### 3.1 The range

```c
typedef struct {
    uint32_t first;
    uint32_t last;    /* inclusive */
} susn_codepoint_range_t;
```

A canonical range array is sorted ascending by `first`, has no overlapping ranges, and no adjacent ranges (ranges separated by >= 1 non-member are separate). `suts_susn_validate` enforces canonicality for ranges inside the 31-bit universe; `suts_susn_normalize` sorts, merges overlaps, and merges adjacency into `out`.

### 3.2 The descriptor

```c
typedef struct {
    uint32_t set_id;
    uint32_t set_flags;
    uint32_t range_count;
    const susn_codepoint_range_t *ranges;
} susn_set_descriptor_t;
```

The descriptor borrows its range array - it never owns or frees it. Flags: `SUSN_FLAG_NEGATED` (inversion list / complement), `SUSN_FLAG_PROPERTY_BASED`, `SUSN_FLAG_RANGE_ARRAY` (always set for range arrays), `SUSN_FLAG_NEOSCRIPT`, `SUSN_FLAG_COMPILED`. A descriptor with `range_count == 0` and no ranges is the empty set. A `NULL` descriptor is also treated as the empty set by membership and algebra (with defined guard behavior).

### 3.3 Membership and cardinality

`suts_susn_contains(set, codepoint)` returns `false` if `codepoint > SUSN_UNIVERSE_MAX`, if `set == NULL`, or if `set` is malformed in the range-pointer sense. Otherwise it performs a bounded binary search for membership in the raw range list and XORs with `SUSN_FLAG_NEGATED`. Membership is total over the 31-bit space.

`suts_susn_cardinality(set)` sums range lengths for positive sets, or returns `SUSN_UNIVERSE_SIZE - sum` for negated sets (`SUSN_UNIVERSE_SIZE` is `0x80000000`). A NULL set has cardinality 0.

### 3.4 Set algebra

Algebra is a single event sweep over the boundaries of the two operand range arrays. For each constant segment between consecutive boundaries, the effective membership of each operand is `(raw_inside XOR negated)`, and `suts_susn_combine(op, a, b, out, capacity)` applies the operator:

| Operator | Result |
|----------|--------|
| `SUSN_OP_UNION` | `a ∪ b` |
| `SUSN_OP_INTERSECTION` | `a ∩ b` |
| `SUSN_OP_DIFFERENCE` | `a \ b` |
| `SUSN_OP_SYMMETRIC_DIFFERENCE` | `a △ b` |

Wrappers `suts_susn_union`, `suts_susn_intersection`, `suts_susn_difference`, `suts_susn_symmetric_difference` exist. The output is always canonical and positive (the `NEGATED` flag is not implied). A `NULL` operand is the empty set.

`suts_susn_complement(set, out, capacity)` returns the complement over the 31-bit universe as a canonical positive array. It is equivalent to flipping the operand's negation and taking union over empty (or complement of a negated set). `suts_susn_complement(NULL, ...)` produces the full universe `[0, SUSN_UNIVERSE_MAX]`.

The return convention follows the `snprintf` style: the function returns the number of canonical ranges the complete result needs, independent of `capacity`. Up to `capacity` ranges are written. `capacity == 0` and `out == NULL` is allowed to query the needed size.

### 3.5 Relations

`suts_susn_is_empty`, `suts_susn_is_universe`, `suts_susn_is_subset`, `suts_susn_is_disjoint`, `suts_susn_equals` are computed via the sweep (subset/difference-empty and intersection-empty).

### 3.6 The fixed-capacity builder

```c
typedef struct {
    uint32_t flags;
    uint32_t count;
    susn_codepoint_range_t ranges[SUSN_BUILDER_CAPACITY];
} susn_builder_t;
```

`SUSN_BUILDER_CAPACITY` is 256. The builder accumulates positive ranges with merging on every insertion. `suts_susn_builder_add_codepoint`, `suts_susn_builder_add_range` merge overlaps and adjacency. `suts_susn_builder_add_set` adds members of a positive set directly, or adds the complement gaps of a negated set (honoring inversion-list semantics). On overflow the insertion returns false. `suts_susn_builder_finish` publishes the builder's array as a descriptor (the descriptor aliases `builder->ranges`, so the builder must outlive it).

---

## 4. RNUR neography and conscript sets

The Native & Neography Plane provides a carve-out `SUSN_RNUR_MIN`–`SUSN_RNUR_MAX` (`0x00160000`–`0x0016FFFF`) divided into `SUSN_RNUR_SET_COUNT` blocks of `SUSN_RNUR_SET_SIZE` (256) codepoints each.

Helpers:
- `suts_susn_is_neography(cp)` tests membership in the carve-out.
- `suts_susn_neography_set_of(cp, &out_set)` returns the block number `(cp - 0x00160000) >> 8`.
- `suts_susn_neography_bounds(set_number, &first, &last)` returns block bounds.
- `suts_susn_neography_contains(set_number, cp)` tests a specific block.
- `suts_susn_neography_set(set_number, storage, out)` materializes block `set_number` as a one-range descriptor in caller-owned `storage`. The descriptor carries `SUSN_FLAG_RANGE_ARRAY | SUSN_FLAG_NEOSCRIPT` and identifier `SUSN_SETID_NEOSCRIPT_BASE + set_number`.

A neography block is a regular set and composes with all algebra, filtering, and catalog operations.

---

## 5. The curated catalog

`suts_susn_catalog_count()` is 25. Entries (by `SUSN_SETID_*`): `ASCII`, `PRINTABLE_ASCII`, `DIGIT`, `UPPER`, `LOWER`, `ASCII_PUNCT`, `CONTROL`, `WHITESPACE`, `LATIN1`, `GREEK`, `CYRILLIC`, `HEBREW`, `ARABIC`, `HIRAGANA`, `KATAKANA`, `HAN`, `HANGUL`, `EMOJI`, `BIDI_CONTROL`, `BRIDGE`, `SCP`, `RNUR`, `NATIVE`, `TRAP`, `SENTINEL`. Each is a canonical range array validated by the engine's invariants. Lookups: `suts_susn_catalog_at`, `suts_susn_catalog_find(set_id)`, `suts_susn_catalog_find_name(name)`. `suts_susn_set_name`, `suts_susn_zone_name`, `suts_susn_op_name` provide taxonomy strings.

The catalog is compiled data included from `src/suts/suts_susn_data.inc`. It is reference data, not a complete UCD dump; a consumer that needs a different cut compiles its own table and wraps it in a descriptor.

---

## 6. API summary

### Version and setup
- `const char* suts_susn_version_string(void)` → `"SUTS-011/1.0"`
- `void suts_susn_init_engine(void)` — idempotent, allocation-free setup hook

### Membership
- `bool suts_susn_contains(const susn_set_descriptor_t *set, uint32_t codepoint)`
- `bool suts_susn_contains_in(const susn_set_descriptor_t *set, uint32_t codepoint, uint32_t filter_mask)`
- `uint64_t suts_susn_cardinality(const susn_set_descriptor_t *set)`

### Validation and canonicalization
- `bool suts_susn_validate(const susn_set_descriptor_t *set)`
- `uint32_t suts_susn_normalize(const susn_codepoint_range_t *in, uint32_t in_count, susn_codepoint_range_t *out, uint32_t capacity)`
- `bool suts_susn_descriptor_init(susn_set_descriptor_t *out, uint32_t set_id, uint32_t flags, uint32_t range_count, const susn_codepoint_range_t *ranges)`

### Algebra
- `uint32_t suts_susn_combine(susn_set_op_t op, const susn_set_descriptor_t *a, const susn_set_descriptor_t *b, susn_codepoint_range_t *out, uint32_t capacity)`
- `uint32_t suts_susn_union/intersection/difference/symmetric_difference(...)`
- `uint32_t suts_susn_complement(const susn_set_descriptor_t *set, susn_codepoint_range_t *out, uint32_t capacity)`

### Filtering and probes
- `uint32_t suts_susn_filter(const susn_set_descriptor_t *set, uint32_t zone_mask, susn_codepoint_range_t *out, uint32_t capacity)`
- `uint32_t suts_susn_probe_mask(const susn_set_descriptor_t *set, uint32_t first, uint32_t stride, uint32_t count)`

### Relations
- `bool suts_susn_is_empty/is_universe/is_subset/is_disjoint/equals(...)`

### Zones and neography
- `susn_zone_t suts_susn_zone_of(uint32_t codepoint)`
- `uint32_t suts_susn_zone_filter(susn_zone_t zone)`
- `bool suts_susn_is_neography(uint32_t codepoint)`
- `bool suts_susn_neography_set_of/bounds/contains(...)`
- `bool suts_susn_neography_set(uint32_t set_number, susn_codepoint_range_t *storage, susn_set_descriptor_t *out)`

### Builder and catalog
- `void suts_susn_builder_init(...)`
- `bool suts_susn_builder_add_codepoint/range/set(...)`
- `bool suts_susn_builder_finish(...)`
- `uint32_t suts_susn_catalog_count(void)`
- `const susn_set_descriptor_t *suts_susn_catalog_at/find/find_name(...)`
- `const char* suts_susn_op_name/zone_name/set_name(...)`

All functions are defined in `include/suts/suts_susn.h` and implemented in `src/suts/suts_susn.c` (data in `src/suts/suts_susn_data.inc`), compiled into `suts_static`. Freestanding C99, no heap, no libc string/stdlib usage on the engine paths.

---

## 7. Implementation notes

- Bounded binary search membership; depth is a compile-time constant of the image.
- Event sweep uses explicit boundaries and a half-open universe terminator; the writer merges adjacent segments into canonical ranges.
- Negated operands are handled in the sweep (XOR effective membership) without materializing complements.
- `suts_susn_filter` builds zone spans, intersects via sweep with `UNION` over empty set, and relies on canonical merging to avoid duplicates.
- `suts_susn_builder_add_set` for a negated operand enumerates the complement gaps and inserts them with merging.
- RNUR block numbering is `set_number = (cp - 0x00160000) >> 8`, `0 <= set_number < 256`.
- Static asserts guard layout (`susn_codepoint_range_t` 8 bytes, descriptor offsets, builder inline storage) and catalog capacity.

---

## 8. Testing

The host test suite `tests/test_suts_susn.c` validates identifiers/flags/layout, the full curated catalog (all 25 sets canonical and addressable), membership and inversion-list negation including ExtSUCS rejection, normalization/validation, all algebra operators including NULL operands and negated operands, relations, zone filtering/probe masks, builder merging/overflow/capacity/complement-via-negation, RNUR block APIs and composition, zone/taxonomy names, and argument guards. All tests pass under `ctest` (47/47).

---

**END OF SUTS-011**
