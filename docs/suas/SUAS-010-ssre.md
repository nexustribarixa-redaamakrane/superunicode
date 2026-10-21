# SUAS-010 — System Specification Registry & Cross-Reference Engine (SSRE)

**Status:** Draft — Authoring
**Version:** 1.0.0
**Family:** SUAS — SuperUnicode Architecture Standards (Core Kernel Invariants)
**Applies To:** SUTR-0 (SUCS Core), SUTR-5 (Plugin Lifecycle), SUTR-10 (SUCM)
**Reference Layer:** SUCD (SuperUnicode Character Database)

## 1. Overview

UAX #41 is the Unicode Consortium's attempt to answer a question no other
Unicode technical report asks: *which specifications depend on which?* It is a
bibliography — a table of contents mapping every UAX, UTS, and ICU document to
the documents that reference it, maintained by hand, read by humans, and
enforced by nothing.

That the question matters is beyond dispute. A Unicode implementation assembled
from independently maintained components can silently disagree with itself: a
normalizer built against one version of UAX #15, a bidi engine built against
another revision of UAX #9, a security layer whose confusable data predates the
identifier profile it claims to implement. UAX #41 documents these relationships
so that a careful human might notice. It does not prevent anything.

SUAS-010 (SSRE) is the normative registry and deterministic linkage protocol
that governs how every SUAS standard, SUTS data specification, and SUTR
architecture report cross-references one another inside the OpenWindows
microkernel. Where Unicode relies on document-level human-readable
bibliographies and loose hyperlinks, SSRE mandates an embedded,
binary-encoded dependency metadata schema directly within the kernel image and
the developer toolchain, so that version mismatches, missing prerequisite SUTS
modules, and invalid architectural dependencies are caught instantly — at
compile time, before an image is produced.

SSRE is the one SUAS standard that governs the other SUAS standards. It is the
single source of truth for the dependency tree across all SuperUnicode
subsystems, and a kernel primitive that is not registered in it is, by
definition, not part of the architecture.

### 1.1 Architectural shifts from UAX #41

| Aspect | UAX #41 | SSRE |
|--------|---------|------|
| Representation | Human-readable bibliography | Binary registry in the kernel image |
| Authority | Maintained by hand | Compiled from the SUCD manifest at build time |
| Enforcement | None; advisory reading | Compile-time hard error and run-time rejection |
| Granularity | Document identity only | Identity, version floor, cross-reference kind, conformance tier |
| Direction | Forward references only | Bidirectional: specification ↔ reference unit |
| Failure mode | A careful reader notices | The build halts |
| Scope | Unicode technical documents | Every SUAS, SUTS, and SUTR document |

### 1.2 Goals

1. **Completeness.** Every registered specification resolves in O(1) expected
   time, and the registry covers the whole corpus — SUAS invariants, SUTS data
   specifications, and the SUTR architecture reports they cite.
2. **Enforcement at both surfaces.** A dependency whose provider is a build
   constant is rejected by the preprocessor. A dependency whose provider is
   resolved only at load time is rejected by the kernel. Both surfaces apply
   the same ordering, so they can never disagree.
3. **Zero allocation.** The registry is a static array plus a fixed-size
   open-addressing index. Nothing is copied, interned, or grown.
4. **No libc.** Specifiers are short NUL-terminated ASCII. Hashing and
   equality are bounded single-pass loops; a freestanding image needs no
   `<string.h>`.
5. **Honest classification.** A dependency that claims to be architecturally
   mandatory must actually be a SUAS invariant. Mislabelling an optional data
   specification as a non-negotiable requirement is a linkage error, not a
   style preference.

### 1.3 Non-negotiable invariants

- **SSRE-I1 — Registry primacy.** A specification that is not registered in
  SSRE is not part of the SuperUnicode architecture, and no conforming
  implementation may resolve a dependency against it.
- **SSRE-I2 — Identifier uniqueness.** Every registered `spec_id` is unique
  and immutable for the life of the image.
- **SSRE-I3 — Kind stability.** A specification's cross-reference kind never
  changes within a major version. A SUTS data specification cannot become a
  SUAS invariant without a new major version of both.
- **SSRE-I4 — Ordering agreement.** `SUAS_SSRE_VALIDATE_DEP` and
  `suas_ssre_satisfies` implement the identical partial order. Any divergence
  is a defect in this specification, not a permitted implementation choice.
- **SSRE-I5 — Mandatory means mandatory.** `is_mandatory` is satisfiable only
  against `SUAS_SSRE_KIND_MANDATORY`. SSRE never validates a mandatory claim
  against an OPTIONAL or REFERENCE specification.
- **SSRE-I6 — Bounded probes.** Every identifier consumed by the kernel layer
  is bounded by `SUAS_SSRE_MAX_SPEC_ID`. An unbounded string never reaches a
  probe loop.

### 1.4 Deviations from the reference sketch

The reference sketch accompanying this specification used a bare `SSRE_`
macro prefix. Three deliberate deviations bring the header into line with
house convention without altering any semantics:

1. **Macro prefix.** `SSRE_*` identifiers were standardised to `SUAS_SSRE_*`,
   matching the SUAS family and guaranteeing coexistence with every other
   module in a single translation unit. Structure and function names
   (`ssre_dependency_descriptor_t`, `suas_ssre_init_registry`,
   `suas_ssre_verify_dependency`) are preserved verbatim.
2. **Patch component.** `SUAS_SSRE_VERSION_PATCH` was added so SSRE reports a
   full semantic version like every other kernel primitive.
3. **Registry record.** `ssre_spec_record_t` was added to the sketch. The
   sketch declared a dependency *descriptor* but no registry *record*, leaving
   nothing for `suas_ssre_init_registry` to initialise; the record is the
   binary form of the SUCD manifest and is the engine's only data.

The sketch named the offline reference layer "SUCA". This is a collision:
SUCA is SUTS-001, the SuperUnicode Collation Algorithm. The character
database is **SUCD**, which is the name used throughout — consistent with
SUAS-009 (SIP) and SUTS-005 (SCSS).

## 2. Definitions and Notation

### 2.1 Definitions

**Registered specification.**
: An entry in the SSRE registry: a `(spec_id, short_name, module, version,
  kind, tier)` sextuple compiled at build time from the SUCD manifest.

**Cross-reference kind.**
: The tier of a specification in the standards hierarchy: `MANDATORY`, a SUAS
  architectural invariant; `OPTIONAL`, a SUTS data specification; `REFERENCE`,
  a SUTR architecture report.

**Dependency descriptor.**
: A caller-supplied declaration that a named specification, at or above a
  stated version floor, must be present in the image — optionally asserting
  that the dependency is architecturally mandatory.

**Version floor.**
: A `(major, minor)` pair interpreted as a lower bound under the standard
  major-then-minor ordering. A provider satisfies a floor when its major is
  greater, or equal with a minor that is at least the floor's.

**Reference unit.**
: The C translation unit implementing a specification, e.g. `suts_sldf`. The
  registry maps `spec_id → module`, and
  `suas_ssre_spec_of_module` provides the inverse.

**Build gate.**
: A check that fails the compilation rather than the execution. Implemented
  here by `SUAS_SSRE_REQUIRE_DEP`, whose unsatisfied case produces a negative
  array bound.

### 2.2 Notational conventions

Types carry the suffix `_t`. Boolean predicates are named as questions and
return `bool`. All version components are `uint8_t`. `dep` is a
`const ssre_dependency_descriptor_t*`; `rec` is a `const ssre_spec_record_t*`.
Registry identifiers are quoted ASCII strings; version pairs are written
`major.minor`.

### 2.3 The static record

```c
typedef struct {
    const char *spec_id;          /* canonical key, NUL-terminated, unique */
    const char *short_name;       /* family acronym, e.g. "SLDF"          */
    const char *module;           /* e.g. "suts_sldf"; "" if no unit      */
    uint8_t     major;            /* registered version                    */
    uint8_t     minor;
    uint8_t     kind;             /* suas_ssre_kind_t                      */
    uint8_t     tier;             /* SUCM tier the module claims           */
} ssre_spec_record_t;
```

Three pointers to read-only storage followed by four octets. The pointers are
never dereferenced mutably and are never copied into writable memory; on a
32-bit target the record is 16 bytes, on a 64-bit target 32.

### 2.4 Cross-reference kinds and SUCM tiers

```c
typedef enum {
    SUAS_SSRE_KIND_MANDATORY = 0, /* SUAS — non-negotiable architecture  */
    SUAS_SSRE_KIND_OPTIONAL  = 1, /* SUTS — optional data specification  */
    SUAS_SSRE_KIND_REFERENCE = 2  /* SUTR — architecture report         */
} suas_ssre_kind_t;
```

The **kind** records where a document sits in the standards hierarchy. The
**tier** records the SUCM conformance layer the registered module claims, per
SUTR-10 §3: `SUAS_SSRE_TIER_L0` (Foundation & Encoding), `SUAS_SSRE_TIER_L1`
(Rendering & Layout), `SUAS_SSRE_TIER_L2` (Semantic & Interaction). Every
engine registered at SSRE's authoring is a Level-0 core engine under SUTR-10
§1.3.1, so the tier is currently uniformly `SUAS_SSRE_TIER_L0`; it is carried
in the record so that a Level-1 or Level-2 engine can be registered later
without a schema change.

A miss on `suas_ssre_tier_of` returns `SUAS_SSRE_TIER_LEVELS` — an out-of-range
value, so a caller that forgets to check reads a tier no module can claim.

## 3. The Dual-Layer Architecture

### 3.1 Offline reference layer (SUCD)

The SuperUnicode Character Database (SUCD) holds the human-readable
specification manifest files: one record per specification, in text, carrying
the identifier, title, family, version, cross-reference kind, conformance
tier, reference unit, and the normative reference list of documents it depends
upon. SUCD is what documentation generators, external compilers, compliance
tooling, and human reviewers read. It is the authority from which the kernel
layer is compiled; the kernel layer is never edited by hand.

### 3.2 Runtime kernel layer (SSRE)

At build time the SUCD manifests are compiled into an immutable, byte-packed
binary table of `ssre_spec_record_t` records embedded in the kernel image.
SSRE validates subsystem integrity directly against those static tables — at
boot, at module load, and, through the compile-time gate, before the image
exists at all.

The kernel layer exposes two enforcement surfaces:

- **`SUAS_SSRE_VALIDATE_DEP` / `SUAS_SSRE_REQUIRE_DEP`** — a preprocessor
  gate for dependencies whose provider is a build constant. This is the
  surface that satisfies SSRE's headline guarantee: a schema misalignment is a
  build failure.
- **`suas_ssre_verify_dependency`** — the run-time mirror, for dependencies
  resolved only when a module is actually loaded: plugins, optional SUTS data
  specifications, dynamically linked engines.

Both evaluate the identical partial order (SSRE-I4).

## 4. The Cross-Reference Registry

### 4.1 Registration (`SSRE-D1`)

A specification is registered by exactly one record. The record is immutable
for the life of the image; `suas_ssre_init_registry` builds the lookup index
over it, is idempotent, and is invoked lazily by the first query or eagerly
from kernel startup.

The registry does not grow. A new specification requires a new build. This is
deliberate: a registry that a module could extend at run time could not be
trusted to answer whether that module's dependencies were satisfiable.

### 4.2 Identifier form (`SSRE-D2`)

A `spec_id` is a NUL-terminated ASCII string of at most
`SUAS_SSRE_MAX_SPEC_ID - 1` significant characters, of the form
`SUAS-NNN`, `SUTS-NNN`, or `SUTR-N`. Identifiers are compared exactly and
case-sensitively: `suas-009` is not `SUAS-009`.

Identifiers are not codepoints and carry no SCP invariance obligations. They
never enter a SuperUnicode string stream; they are the names of documents.

### 4.3 The curated corpus

The registry carries 33 entries as of SUAMR's registration:

| Kind | Count | Members |
|------|-------|---------|
| MANDATORY | 15 | SUAS-001 (SDF) … SUAS-009 (SIP), SUAS-010 (SSRE), SUAS-011 (SCMS), SUAS-012 (SUCD), SUAS-013 (SISR), SUAS-014 (SUVTL), SUAS-015 (SUAMR) |
| OPTIONAL | 7 | SUTS-001 (SUCA), SUTS-002 (SRE), SUTS-003 (SLDF), SUTS-004 (SIVD), SUTS-005 (SCSS), SUTS-006 (SIDN), SUTS-007 (SUEPS) |
| REFERENCE | 11 | SUTR-0 through SUTR-10 |

Adding a specification to this table is a patch-level change per §11: no
descriptor that verified before a registration can fail to verify after it.

Every entry is validated by the reference implementation's test suite for
unique identity, reachable index position, in-range kind and tier, and
identifier length — the registry's self-consistency is itself a tested
property, because a registry that lies about itself is worse than no registry.

## 5. Dependency Validation Protocol

### 5.1 The dependency descriptor (`SSRE-D3`)

```c
typedef struct {
    const char *spec_id;          /* e.g., "SUTS-003", "SUAS-009" */
    uint8_t min_version_major;
    uint8_t min_version_minor;
    bool is_mandatory;            /* true if SUAS, false if optional SUTS */
} ssre_dependency_descriptor_t;
```

The descriptor is caller-owned and never copied. `spec_id` points into
read-only storage; SSRE retains no state derived from it beyond the duration
of the call.

### 5.2 The validation rules (`SSRE-D4`)

`suas_ssre_verify_dependency(dep)` returns true if and only if all of:

1. `dep` and `dep->spec_id` are non-NULL;
2. the identifier is registered;
3. the registered version satisfies the declared floor —
   `rec->major > dep->min_version_major`, or equal majors with
   `rec->minor >= dep->min_version_minor`;
4. if `dep->is_mandatory` is set, the registered kind is
   `SUAS_SSRE_KIND_MANDATORY`.

Rule 4 is the rule UAX #41 has no analogue for. A UAX #41 bibliography records
that a document is referenced; it cannot express that the reference is
*binding*. SSRE can, and rejects the mislabel — an application that declares a
mandatory dependency on an optional SUTS data specification has made an
architectural error, and is told so rather than being quietly accommodated.

### 5.3 Whole-set validation

| Function | Semantics |
|----------|-----------|
| `suas_ssre_verify_set(deps, count)` | True iff every descriptor verifies. Vacuously true for an empty set; false for a NULL set with non-zero count. |
| `suas_ssre_first_missing(deps, count)` | The first unverifiable descriptor, or NULL. The diagnostic form — it names the offending entry so a build gate can report it. |
| `suas_ssre_dependency_count(deps, count)` | How many descriptors fail. A NULL set with non-zero count fails entirely, so every member is counted. |

`suas_ssre_first_missing` returns NULL for a NULL set, since there is no
descriptor to point at; `suas_ssre_dependency_count` distinguishes the two
cases.

## 6. Static Packing and O(1) Resolution

### 6.1 The record

The registry is a `static const ssre_spec_record_t[]` of 28 entries under a
declared capacity of `SUAS_SSRE_REGISTRY_CAPACITY` (64). A compile-time
assertion holds the curated count within capacity, so outgrowing the index is
a build failure rather than a silent truncation.

### 6.2 The index

A fixed table of `SUAS_SSRE_INDEX_SIZE` (256) open-addressing slots, each
holding a used flag and a single-byte registry index. The hash is FNV-1a
32-bit over the bytes of the identifier:

```
h ← 2166136261
for each byte b of spec_id up to the NUL:
    h ← (h XOR b) × 16777619
slot ← h AND 255
```

Collisions resolve by linear probing with wraparound. The table is built
lazily on first query and is idempotent under re-initialisation.

### 6.3 Complexity

| Operation | Cost |
|-----------|------|
| `suas_ssre_lookup` | O(1) expected; the probe is bounded by `SUAS_SSRE_MAX_SPEC_ID` bytes per comparison |
| `suas_ssre_verify_dependency` | O(1) expected — one lookup plus constant work |
| `suas_ssre_verify_set`, `_first_missing`, `_dependency_count` | O(count) |
| `suas_ssre_spec_of_module` | O(n) over the registry; the one-directional index, intended for tooling, not the boot path |
| `suas_ssre_spec_at`, `_kind_count` | O(1) |
| Memory | Static only; no allocation at any point |

The registry is small enough that the linear module scan is measured in tens of
comparisons, and it is not on any hot path: it exists so that a tool or a test
can invert the cross-reference without a second table.

## 7. Build-Time Enforcement

### 7.1 The compile-time gate

```c
#define SUAS_SSRE_VALIDATE_DEP(req_major, req_minor, got_major, got_minor) \
    (((got_major) > (req_major)) ||                                        \
     (((got_major) == (req_major)) && ((got_minor) >= (req_minor))))

#define SUAS_SSRE_REQUIRE_DEP(sym, req_major, req_minor, got_major, got_minor) \
    typedef char sym[(SUAS_SSRE_VALIDATE_DEP((req_major), (req_minor),       \
                                             (got_major), (got_minor))) ? 1 : -1]
```

Every operator in `SUAS_SSRE_VALIDATE_DEP` is a preprocessor operator, so the
macro is valid inside `#if` — which is what makes a compile-time gate possible
at all. A module whose provider declares its version as integer literals can
enforce its own prerequisite directly:

```c
#if !SUAS_SSRE_VALIDATE_DEP(1, 0, SUTS_SLDF_VERSION_MAJOR, SUTS_SLDF_VERSION_MINOR)
#error "SSRE: schema misalignment with SUTS-003 (SLDF)"
#endif
```

`SUAS_SSRE_REQUIRE_DEP` applies the same predicate through the classic C99
negative-array-bound idiom, with a caller-chosen symbol so several gates may
coexist in one translation unit. A failure reports as *"size of array is
negative"* naming the dependency being enforced.

### 7.2 Kernel startup verification

Kernel boot verifies the subsystem set it actually links, once, with zero
allocation:

```c
static const ssre_dependency_descriptor_t boot_deps[] = {
    { "SUAS-001", 1u, 0u, true  },
    { "SUAS-010", 1u, 0u, true  },
    { "SUTS-003", 1u, 0u, false },
    { "SUTS-005", 1u, 0u, false }
};

if (!suas_ssre_verify_set(boot_deps, sizeof boot_deps / sizeof boot_deps[0])) {
    const ssre_dependency_descriptor_t* bad =
        suas_ssre_first_missing(boot_deps,
                                sizeof boot_deps / sizeof boot_deps[0]);
    kpanic("SSRE: unsatisfied dependency %s", bad->spec_id);
}
```

### 7.3 Build-gate integration

The dual-layer guarantee is realised by having the two surfaces enforce
different halves of the same question:

| Surface | Question | Provider known | Failure |
|---------|----------|----------------|---------|
| `SUAS_SSRE_REQUIRE_DEP` | "Does the header I am compiling against match?" | At build time | Compilation halts |
| `suas_ssre_verify_dependency` | "Does the image I am loading actually contain it?" | At load time | Module load rejected |

A deployment that only ever consults the run-time surface has already shipped
the mismatch; a deployment that only ever consults the compile-time surface
cannot see a plugin loaded after the fact. SSRE requires both.

## 8. Data Files

| File | Form | Role |
|------|------|------|
| `sucd/registry.d` | Text manifest | The offline reference layer: one record per specification, with its normative reference list |
| Kernel image | `ssre_spec_record_t[]` | The runtime layer, compiled from the manifest at build time |

The manifest is the authority; the binary table is derived. Reconciling the
two is the responsibility of the build, and the reference implementation's
test suite asserts that the binary table is internally consistent, that every
identifier is unique and reachable, and that no entry escapes the declared
capacity.

## 9. Implementation Notes

### 9.1 Querying a specification

```c
const ssre_spec_record_t* rec = suas_ssre_lookup("SUTS-004");
if (rec == NULL) { /* unregistered: not part of the architecture */ }
printf("%s %u.%u %s\n", rec->short_name, rec->major, rec->minor,
       suas_ssre_kind_name(rec->kind));
```

### 9.2 Declaring a module's prerequisites

```c
static const ssre_dependency_descriptor_t my_deps[] = {
    { "SUAS-009", 1u, 0u, true  },  /* an invariant: mandatory           */
    { "SUTS-005", 1u, 0u, false }   /* a data spec: optional by contract */
};

if (!suas_ssre_verify_set(my_deps, 2u))
    return SSRE_ERR_UNLINKED;
```

### 9.3 Inverting the cross-reference

```c
const char* id = suas_ssre_spec_of_module("suts_sivd"); /* "SUTS-004" */
```

This is the operation a documentation generator needs in order to print, for
any engine, the specification that governs it — and the reason the registry
records the reference unit at all.

## 10. Testing

The reference implementation is exercised by `tests/test_suas_ssre.c`,
covering: version and meta constants; the descriptor's fixed layout; the
compile-time gate, exercised both as a constant expression and as a
`#if` directive so the preprocessor path is proven rather than assumed;
registry self-consistency (count, uniqueness, index reachability, in-range
kind and tier, identifier budget); family classification across all three
kinds; O(1) lookup and the miss surface (unallocated identifiers, wrong case,
empty, NULL, overlong); version queries and the out-of-range tier sentinel;
the satisfaction order; the bidirectional module cross-reference; single
descriptor validation including the mandatory-versus-optional rejection; whole-
set validation with first-failure diagnostics; enumeration bounds; hash
determinism and the FNV offset basis for NULL; and idempotence under repeated
initialisation.

## 11. Stability and Versioning

`SUAS_SSRE_SPEC_ID` is `"SUAS-010"`, `SUAS_SSRE_VERSION_MAJOR` is 1, and
`SUAS_SSRE_VERSION_MINOR` is 0. The version identifies the registry schema and
validation protocol, not the registered corpus: adding a specification to the
curated table is a patch-level change, because no descriptor that verified
before can fail to verify afterwards. Removing a registration, changing an
existing entry's kind or identifier, or altering the ordering in
`SUAS_SSRE_VALIDATE_DEP` is a major change.

### 11.1 Numbering note

The SUAS series is contiguous: SUAS-001 through SUAS-015 are all allocated, in
authorship order, and no number is reserved or held back. This standard is
**SUAS-010**.

It was first published as SUAS-011, with SUAS-010 left as a deliberate gap, and
SCMS was first published as SUAS-012 alongside it. Both were shifted down by one
to close the hole, because a skipped number in a small series reads as a
transcription error rather than as a decision. Any citation predating that
shift saying **SUAS-011 (SSRE)** means this standard, and any citation saying
**SUAS-012 (SCMS)** means [SUAS-011 (SCMS)](SUAS-011-scms.md); such citations
are strictly historical. SUAS-012 now names
[SUCD](SUAS-012-sucd.md), SUAS-013 names
[SISR](SUAS-013-sisr.md), SUAS-014 names
[SUVTL](SUAS-014-suvtl.md), SUAS-015 names
[SUAMR](SUAS-015-suamr.md), and SUAS-016 is the first unallocated number.

## 12. Conformance Requirements

An implementation conforms to SUAS-010 when:

1. it registers every specification of the SuperUnicode corpus it implements,
   with the correct kind and version;
2. `suas_ssre_lookup` resolves every registered identifier and no other;
3. `suas_ssre_verify_dependency` implements rules SSRE-D3 and SSRE-D4
   exactly, including the mandatory-implies-MANDATORY rule;
4. `SUAS_SSRE_VALIDATE_DEP` and `suas_ssre_satisfies` agree on every
   `(major, minor)` pair (SSRE-I4);
5. no operation allocates, and every identifier-consuming path is bounded by
   `SUAS_SSRE_MAX_SPEC_ID`;
6. the compile-time gate is available to, and used by, modules whose provider
   is a build constant.

### 12.1 Compliance matrix (reference implementation)

| Requirement | Location | Status |
|-------------|----------|--------|
| R1 — Corpus registration | `src/suas/suas_ssre.c`, `ssre_registry[]` | ✔ |
| R2 — Exact resolution | `suas_ssre_lookup`, `ssre_find` | ✔ |
| R3 — SSRE-D3 / SSRE-D4 | `suas_ssre_verify_dependency` | ✔ |
| R4 — Ordering agreement | `suas_ssre_satisfies` / `SUAS_SSRE_VALIDATE_DEP` | ✔ |
| R5 — Zero allocation, bounded | whole module | ✔ |
| R6 — Compile-time gate | `SUAS_SSRE_REQUIRE_DEP` | ✔ |

## 13. Terms and Conventions

**SSRE** — System Specification Registry & Cross-Reference Engine; this
standard.

**SUCD** — SuperUnicode Character Database; the offline reference layer
holding the human-readable manifests. (Not to be confused with SUCA, SUTS-001,
the SuperUnicode Collation Algorithm.)

**Registry** — The immutable binary table of `ssre_spec_record_t` records
embedded in the kernel image.

**Descriptor** — A `ssre_dependency_descriptor_t`: a dependency declaration
supplied by a requiring module.

**Kind** — A specification's tier in the standards hierarchy: MANDATORY,
OPTIONAL, or REFERENCE.

**Tier** — A SUTR-10 SUCM conformance layer: L0, L1, or L2.

**Reference unit** — The C translation unit implementing a specification.

**Build gate** — A check that fails compilation rather than execution.
