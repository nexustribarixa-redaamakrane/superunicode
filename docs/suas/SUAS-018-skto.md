# SUAS-018 - System Kernel Text Objects (SKTO)

**Standard ID:** SUAS-018
**Status:** Draft - Authoring
**Scope:** Core Kernel Invariant
**Applies To:** SUTR-0 (SUCS Core), SUTR-1 (SUTF), SUTR-10 (SUCM), SUAS-001 (SDF), SUAS-002 (SGW), SUAS-003 (SBR), SUAS-004 (SUCF), SUAS-006 (STS), SUAS-007 (SIS), SUAS-010 (SSRE), SUAS-012 (SUCD), SUTS-005 (SCSS), the OpenWindows microkernel (`lib/kstring.c`, `lib/kprintf.c`, `syscall/`, `vfs/`, `core/`, `ps/`), and any kernel, driver, or user-mode component that passes text across a trust boundary
**Last Updated:** 2026-10-28

---

## 1. Overview

Every SuperUnicode implementation eventually has to carry text from one
execution context to another: from a user process into the kernel, from a
filesystem into a name comparison, from a console driver into a frame buffer,
from a loader into a process table. SUTF-1 (SUTF) defines how a codepoint is
written down, and SUTR-0 (SUCS Core) defines which codepoints may exist. Neither
defines **who owns the bytes, how long they are, or how long the answer stays
true**.

System Kernel Text Objects (SKTO) define that missing layer. A *text object*
is a bounded, explicitly measured view over SUTF-8 bytes. It carries its length
in bytes, its available capacity, its ownership class, and a small set of
capability flags. It does not rely on a terminator, it does not imply a heap
allocation, and it cannot be lengthened in place.

SKTO is a **mandatory** member of the SuperUnicode Architecture Standards.
An implementation that claims SuperUnicode conformance MUST satisfy the
requirements in sections 3 through 11. It may satisfy them with the reference
descriptor in section 4 or with any representation that provably enforces the
same invariants; the descriptor is normative in its *fields and meanings*, not
in its C spelling.

Three failures motivate the standard, all of them observable in the current
OpenWindows tree (section 12, informative):

1. **Terminator dependence.** `ow_strlen` walks until `'\0'`. A buffer that
   loses its terminator turns a length query into an unbounded read, and a
   length query is the first operation most callers perform.
2. **Length-free interfaces.** The kernel's text entry points take `char *`
   and infer the length themselves. The caller's real bound - the size of the
   buffer it passed - is discarded at the boundary and cannot be recovered.
3. **Publication without freezing.** A name resolved once, then compared again
   later, may have changed between the two reads. That is a check-to-use race,
   and no amount of pointer validation closes it if the bytes are mutable.

SKTO closes all three by making length explicit, ownership a property of the
object rather than a convention, and publication an irreversible transition.

### 1.1 Goals

- **Explicit length.** Every text object states how many bytes are text. The
  answer never depends on inspecting the contents.
- **Explicit ownership.** Every text object states whether the caller retains
  the buffer, whether the object owns it, or whether it is immutable image
  data.
- **Explicit capacity.** Every writable text object states how much room it
  has, so a write cannot silently overrun a caller buffer.
- **Trust-boundary separation.** Text that originates outside the current
  privilege domain is a distinct class with a distinct admission procedure.
- **Zero allocation.** No SKTO operation allocates. Growth is a caller
  decision made with a caller buffer.
- **Publication immutability.** A text object that has been published cannot
  change; replacement is a new object with a new generation.

### 1.2 Non-Negotiable Invariants

1. **No terminator semantics.** No SKTO operation may determine length by
   scanning for `U+0000`. `len` is authoritative for every read.
2. **No read past `len`.** A consumer MUST NOT inspect `bytes[len]` unless
   the object carries `SKTO_F_NUL_SAFE`.
3. **No write past `cap`.** A producer MUST refuse an operation that would
   exceed `cap` rather than truncate it silently.
4. **No borrowing across a lifetime.** A borrowed view MUST NOT outlive the
   call that produced it.
5. **No allocation.** SKTO performs no dynamic allocation on any path.
6. **No SCP in text.** Codepoints in the System Control Plane
   (`0x110000`-`0x11FFFF`) are directives, not text, and MUST NOT appear in a
   text object.
7. **No unallocatable codepoints.** `0x7FFFFFF0`-`0x7FFFFFFF` MUST NOT appear
   in a text object; the Sentinel is in-band-invalid, not data.
8. **Deterministic comparison.** Two text objects compare equal iff their
   lengths are equal and their byte sequences are identical. Comparison never
   consults locale, normalization form, or the environment.

---

## 2. Scope

SKTO defines:

- the text-object descriptor, its fields, and its invariants;
- the ownership and lifetime classes and the rules that attach to each;
- when a callee may borrow a caller buffer and when it must copy;
- the admission procedure for text that crosses a trust boundary;
- the SUTF framing rules for text objects and the conversion boundary;
- the publication and immutability rules for names;
- the comparison, hashing, and lookup primitives a name consumer needs;
- resource constraints under freestanding C99 and zero allocation.

SKTO does **not** define:

- how text is rendered, measured in cells, or broken into lines - those are
  SUAS-002 (SGW) and SUAS-003 (SBR);
- how text is segmented into graphemes, words, or sentences - SUAS-006 (STS);
- how names are classified as identifiers - SUAS-007 (SIS);
- how text is normalized - SUAS-004 (SUCF);
- how text is stored on disk or transmitted - SUTR-3 and SUTR-4;
- which codepoints are admissible in a particular namespace - that is a
  policy decision made by the namespace owner using SUTS-005 (SCSS) and
  SUAS-007 (SIS).

---

## 3. Definitions

- **TD1 - Text object.** A descriptor that names a byte sequence together
  with its length, capacity, ownership class, and flags.
- **TD2 - Text bytes.** The `len` bytes at `bytes[0]` through `bytes[len-1]`
  of a text object. Only these bytes are text.
- **TD3 - Capacity.** The number of bytes the storage can hold, including the
  bytes already occupied. `cap >= len` for every valid object.
- **TD4 - Interior form.** The encoding used for text bytes inside a text
  object. In SKTO v1.0 the interior form is always SUTF-8.
- **TD5 - Borrowed view.** A text object whose `bytes` remain owned by the
  caller. The callee may read them and MUST NOT retain the object.
- **TD6 - Owned text.** A text object whose `bytes` are a caller-provided
  buffer that the object has filled. The buffer's owner remains the caller;
  the object owns the *contents* only for the duration of the write.
- **TD7 - Static text.** A text object whose `bytes` point into read-only
  image data and whose lifetime is the lifetime of the image.
- **TD8 - Published name.** A text object registered in a namespace and
  thereafter immutable until explicitly unpublished.
- **TD9 - Admission.** The procedure that converts an untrusted candidate
  (a user pointer plus a user length, or a device-provided buffer) into a
  text object the kernel may use.
- **TD10 - Generation.** A monotonically increasing counter attached to a
  published name; a new generation means the name was replaced, not edited.
- **TD11 - NUL-safe view.** A text object that additionally guarantees one
  `0x00` byte at `bytes[len]` and no `0x00` byte inside the text. This is the
  only form that may be passed to a legacy C-string interface.
- **TD12 - Overlong form.** A SUTF-8 sequence that encodes a value in more
  bytes than its shortest encoding. Overlong forms are invalid text.

---

## 4. The Text Object

### 4.1 Descriptor

The normative fields are:

```c
typedef enum {
    SKTO_OWN_STATIC    = 0,  /* immutable image data, no release      */
    SKTO_OWN_BORROWED  = 1,  /* caller-owned, valid for this call only */
    SKTO_OWN_OWNED     = 2,  /* caller storage filled by this object   */
    SKTO_OWN_PUBLISHED = 3   /* registered, immutable, generation-tagged */
} skto_owner_t;

typedef enum {
    SKTO_ENC_SUTF8 = 0       /* the only interior form in SKTO 1.0 */
} skto_encoding_t;

typedef struct {
    const uint8_t *bytes;    /* text bytes; NULL only when len == 0 */
    size_t         len;      /* text length in BYTES, not codepoints */
    size_t         cap;      /* storage capacity in bytes; >= len */
    uint16_t       owner;    /* skto_owner_t */
    uint16_t       encoding; /* skto_encoding_t */
    uint32_t       flags;    /* SKTO_F_* */
    uint32_t       generation; /* meaningful only for PUBLISHED */
} skto_text_t;
```

A conforming implementation MAY substitute a different C layout. It MUST
preserve the six observable quantities: bytes, byte length, capacity,
ownership class, encoding, and generation.

### 4.2 Flags

| Flag | Meaning | Legal owners |
|------|---------|--------------|
| `SKTO_F_NUL_SAFE` | `bytes[len] == 0` and no interior `0x00` | all |
| `SKTO_F_CONST` | the text bytes must not be written | STATIC, BORROWED, PUBLISHED |
| `SKTO_F_NAME` | the object is a namespace name, not free text | PUBLISHED |
| `SKTO_F_ADMITTED` | the object passed the admission procedure | all except STATIC |

`SKTO_F_CONST` and `SKTO_F_NAME` are assertions by the producer. A consumer
MUST NOT clear them. `SKTO_F_ADMITTED` MUST be cleared when the bytes are
modified, because the admission verdict applies to a byte sequence, not to a
buffer address.

### 4.3 Validity rules

An object is **valid** when all of the following hold:

- **V1.** `bytes != NULL` whenever `len > 0`.
- **V2.** `len <= cap`.
- **V3.** `owner` is one of the four defined classes.
- **V4.** `encoding == SKTO_ENC_SUTF8`.
- **V5.** The `len` bytes are well-formed SUTF-8: no truncated sequence, no
  overlong encoding, no code unit `0x80`-`0xBF` outside a sequence.
- **V6.** No decoded value lies in `0x110000`-`0x11FFFF`.
- **V7.** No decoded value lies in `0x7FFFFFF0`-`0x7FFFFFFF`.
- **V8.** If `SKTO_F_NUL_SAFE` is set, `bytes[len] == 0` and no byte in
  `[0, len)` is `0x00`.

V1 through V4 are structural and MUST be checked in O(1). V5 through V7
require a scan and are the responsibility of the producer; a consumer that
did not produce the object MUST either trust `SKTO_F_ADMITTED` or scan before
use. A consumer MUST NOT assume V5 through V8 from `len` alone.

> **Why V7 matters.** SUTR-0 reserves `0x7FFFFFF0`-`0x7FFFFFFE` for traps and
> `0x7FFFFFFF` for the Sentinel. They are unallocatable. A text object that
> contains one is not "text with a strange character"; it is a malformed
> object, and treating it as data would let a trap identifier masquerade as
> content.

### 4.4 Construction

The standard defines no constructor function by name. A conforming
implementation MUST provide equivalents of:

- construct a static object from an image-data pointer and a compile-time
  length;
- construct a borrowed object from a validated pointer and length;
- construct an owned object over a caller buffer with its capacity;
- construct an empty object, which is `len == 0`, `cap == 0` or non-zero,
  `bytes` possibly `NULL`, and every flag clear.

`len` MUST be supplied, never computed. There is no "measure this C string"
constructor in the mandatory surface, because that constructor is exactly the
unbounded read SKTO exists to remove. A compatibility constructor MAY exist
for legacy callers; it MUST be named as such, MUST stop at `cap`, and MUST
report failure when no terminator was found.

### 4.5 Empty and absent

An empty text object and an absent text object are different:

- **empty** - `len == 0`; the object exists and denotes the empty string;
- **absent** - the pointer to the descriptor is `NULL`.

SKTO does not permit a `NULL` descriptor to stand for empty. Callers that
return "no value" MUST return a status code, not a zeroed descriptor,
otherwise "the user sent nothing" and "the call failed before reading
anything" become indistinguishable.

---

## 5. Ownership and Lifetime

### 5.1 Static

`SKTO_OWN_STATIC` names bytes in the image or in a read-only data section.

- The bytes MUST remain valid for the lifetime of the program.
- The object MUST NOT be released, reallocated, or modified.
- `cap` MUST equal `len`; there is no writable capacity.

Typical use: a literal error message, a specification identifier such as
`"SUAS-018"`, a compiled table key.

### 5.2 Borrowed

`SKTO_OWN_BORROWED` names caller storage that the callee may read.

- The borrower MUST NOT retain the object beyond the call.
- The borrower MUST NOT write through `bytes`.
- The caller MUST guarantee that the storage remains valid and unmodified
  for the whole call, including any callback the callee invokes.
- A borrowed object MUST NOT be stored in any structure whose lifetime
  exceeds the call.

A callee that needs the bytes later MUST copy them into storage it owns, or
record a published name (section 9).

### 5.3 Owned

`SKTO_OWN_OWNED` describes storage the caller supplied for the callee to fill.

- The callee writes at most `cap` bytes and sets `len` to the number written.
- The callee MUST NOT reallocate; a larger result is
  `SKTO_E_CAPACITY`.
- On failure the callee MUST leave `len` unchanged or set it to `0`; a
  partially written buffer MUST NOT be reported as a shorter valid string
  unless the operation explicitly defines truncation and reports it.
- The caller's buffer remains the caller's memory. "Owned" never means "freed
  by the callee".

### 5.4 Published

`SKTO_OWN_PUBLISHED` names an entry in a namespace registry (section 9).

- The bytes are immutable from publication until unpublication.
- The object carries a `generation`. A reader that observes a generation and
  later needs to act on the same name MUST confirm the generation is
  unchanged, or act on a handle rather than a pointer.
- Publication is not a mutation of an existing object. Replacing a name
  publishes a new object with `generation + 1` and retires the old one.

### 5.5 Lifetime table

| Owner | Valid until | Writable | Retainable | Release |
|-------|-------------|----------|------------|---------|
| STATIC | image unload | no | yes | none |
| BORROWED | return from the call | no | no | none |
| OWNED | caller's buffer is reused or freed | during the call | no | none |
| PUBLISHED | unpublication | no | yes, by generation | unpublish |

---

## 6. Borrow or Copy

The decision is not stylistic. A callee MUST copy when **any** of the
following is true, and MAY borrow otherwise:

- **C1.** The callee may block, yield, or be preempted before it finishes
  reading the text, and the text came from a domain that can run
  concurrently with it.
- **C2.** The callee retains the text past the call.
- **C3.** The text crosses a privilege boundary in the direction
  user-to-kernel, and the operation is not guaranteed to complete within the
  same admission step.
- **C4.** The callee publishes the text.
- **C5.** The text is modified by a writer the callee does not control.

Borrowing is permitted for a same-domain, non-blocking, read-only operation
whose entire use of the text happens inside the call. It is an optimization,
never a right.

A callee that borrows MUST treat the object as read-only regardless of the
`const` qualifier's absence on the caller side, and MUST re-validate
structural invariants at entry (section 4.3, V1-V4).

**Copying is bounded.** A copy into caller-provided storage MUST stop at
`cap` and report `SKTO_E_CAPACITY`; it MUST NOT return a truncated object
that claims success. This matches the existing kernel behaviour of
`syscall_copy_name`, which rejects rather than silently truncating (section
12.2). Truncation for *display* is a different operation and MUST be reported
as a warning, never as a successful name admission.

---

## 7. Admission of Untrusted Text

Admission converts a candidate supplied by another privilege domain into a
text object. It is the only place where SKTO reads memory it does not already
trust.

### 7.1 Admission procedure

An implementation MUST perform these steps in order, and MUST stop at the
first failure:

1. **Presence.** Reject a `NULL` pointer with a non-zero length.
2. **Arithmetic.** Reject if `base + len` overflows the address type, or if
   `len` exceeds the implementation's admission ceiling.
3. **Containment.** Reject unless every page touched by `[base, base+len)`
   is present in the calling domain's own mapping. Checking only the first
   and last byte is insufficient; a range that starts inside a mapping and
   runs off its end is a forged range.
4. **Termination, when required.** If the caller supplied a NUL-terminated
   interface, scan at most the admission ceiling for the terminator and
   reject if absent. The scan bound is the ceiling, never the buffer's
   apparent contents.
5. **Encoding.** Validate SUTF-8 well-formedness in one pass (V5).
6. **Admissibility.** Reject SCP values (V6), trap values, and the Sentinel
   (V7).
7. **Policy.** If the object is a name, apply the namespace's identifier and
   security policy (SUAS-007, SUTS-005). Policy is applied *after*
   structural validity, so that a malformed object can never be reported as
   merely "not permitted".
8. **Materialization.** Either copy into kernel storage with a known
   capacity, or mark the object borrowed with a lifetime that ends before the
   call returns.

### 7.2 What admission must not do

- It MUST NOT read past `base + len`.
- It MUST NOT trust a length supplied by the same domain that supplied the
  pointer without the containment check.
- It MUST NOT normalize. Normalization changes the identity of the text and
  is a separate decision by the namespace owner (SUAS-004).
- It MUST NOT repair. An overlong sequence, a truncated sequence, or a
  surrogate-form value is a rejection, not a character to substitute.
- It MUST NOT admit SCP directives as content. SCP is carried in band by
  design (SUTR-0), but a directive is handled by the plane that understands
  it, not stored in a text object.

### 7.3 Failure reporting

Admission failure MUST be distinguishable from admission success with empty
text. The status values are implementation-defined but MUST distinguish at
least: absent argument, arithmetic overflow, containment failure, encoding
failure, inadmissible codepoint, capacity exhaustion, and policy rejection.

A failure that must reach the BANcode space maps to an already-registered
`0x0011Axxx` code. SKTO MUST NOT mint BANcodes; the registry is owned by
SUTR-0 and the SUCT allocation described in SUTS-012.

---

## 8. Encoding and Framing

### 8.1 Interior form

The interior form of a text object is SUTF-8, and only SUTF-8. SUTF-16,
SUTF-4, SUTF-2, and the vSUTF escape form are **transport** forms: they are
converted at the boundary by SUTR-1 and never stored in a text object.

This is a deliberate narrowing. A kernel that accepts five interior forms
must validate five, compare across five, and answer "how long is this
character" five ways. One interior form makes every downstream contract
smaller and makes the length in bytes well-defined.

### 8.2 Byte length versus codepoint count

`len` is a byte count. A consumer that needs a codepoint count MUST derive it
by decoding, and MUST cache the result if it needs it more than once in a
loop. No SKTO operation silently converts between the two.

Slicing MUST occur at a codepoint boundary. A slice that splits a multi-byte
sequence produces an invalid object, and an implementation MUST reject it
rather than emit it.

### 8.3 Codepoint range

Text objects cover the Base SUCS space `0x00000000`-`0x7FFFFFEF`, excluding
the SCP and the sixteen terminal values. The Unicode Bridge range
`0x00000000`-`0x0010FFFF` is permanently 1:1 with Unicode (SUTR-6), so a
text object may hold any Unicode text without conversion. Native-plane
codepoints encode as five- or six-byte SUTF-8 sequences.

### 8.4 Boundary conversion

Conversion is the caller's explicit step:

- decode a text object into a codepoint array when an algorithm needs random
  access (SUAS-001, SUAS-006, SUAS-007 all take codepoint views);
- encode a codepoint array back into a text object when the result is
  stored or compared;
- never convert by casting, and never assume the byte and codepoint counts
  match.

The decode buffer is caller-provided. Its capacity is in elements, not bytes,
and the conversion MUST report both the number of elements written and the
number of bytes consumed, so a caller can resume at the correct boundary.

### 8.5 NUL

`U+0000` is valid SUTF-8 (`0x00`). It is legal inside a text object, because
the object carries its length. It is **not** legal in a NUL-safe view, and
therefore not legal in a published name. A consumer that hands text to a
legacy C interface MUST convert to a NUL-safe view first and MUST reject text
that contains an interior NUL instead of truncating at it.

---

## 9. Published Names

A published name is a text object with `SKTO_OWN_PUBLISHED` and
`SKTO_F_NAME`. It is the representation used for VFS entries, ALPC port
names, device names, process image names, and any other identifier that is
resolved once and acted on later.

### 9.1 Admission requirements for a name

A name MUST additionally satisfy:

- **N1.** It is a valid text object (section 4.3).
- **N2.** It contains no `U+0000`.
- **N3.** It is NUL-safe, so a legacy consumer can read it without
  conversion.
- **N4.** It is a valid identifier or path element under SUAS-007 (SIS) for
  the namespace that owns it.
- **N5.** It does not exceed the namespace's byte ceiling. The ceiling is a
  property of the namespace, not of SKTO; a VFS that stores 223 bytes and a
  process table that stores 31 bytes each publish against their own limit.
- **N6.** It is already in the namespace's chosen equivalence form. If the
  namespace treats canonically equivalent sequences as the same name, the
  producer MUST apply SUAS-004 before publication. Comparison after
  publication is byte-exact and MUST NOT normalize.

### 9.2 Publication

Publication is a transition, not a write:

1. the candidate is admitted and validated;
2. its bytes are copied into storage that the namespace owns and no other
   writer can reach;
3. its generation is assigned (zero for the first publication of a fresh
   entry, else the previous generation plus one);
4. the entry becomes visible atomically: a reader sees either the old object
   or the new one, never a partial write.

An implementation MUST NOT publish a pointer into caller storage, into a
borrowed view, or into a buffer that any code path can still write.

### 9.3 Immutability

Once published:

- the bytes MUST NOT change;
- `len`, `cap`, and the flag set MUST NOT change;
- the object MUST NOT be freed while any reader can still reach it.

Updating a published name means publishing a new object with a higher
generation and retiring the old one. Retiring MUST wait for readers that
already hold the old generation, or use a handle scheme that makes
use-after-retire impossible.

### 9.4 Resolution and use

A resolver returns a *handle* (or a generation-stamped object), not a bare
pointer. A consumer that resolves a name and then performs a privileged
action MUST act on the resolved handle:

- resolving a path and then re-opening it by name is a check-to-use race, even
  if both operations individually validate;
- authorizing a name and then passing the string to a driver that resolves it
  again is the same race with an extra step.

SKTO does not prescribe the handle representation; it requires that the
verdict and the target be bound to the same immutable object.

---

## 10. Comparison, Hashing, and Lookup

### 10.1 Equality

Two text objects are equal iff `len` is equal and the `len` bytes are
identical. The comparison is byte-exact and order-independent of locale,
normalization, and case. It MUST be O(min(len_a, len_b)).

An implementation SHOULD compare length first, so that a mismatch of length
costs one comparison rather than a full scan.

### 10.2 Hash

A namespace that needs a hash table MUST derive its hash from the bytes and
the length and nothing else. The hash MUST be stable across runs and across
processes, and MUST be computed over exactly `len` bytes.

The corpus already standardizes FNV-1a for specification identifiers
(SUAS-010). A namespace MAY use a different function, but if it participates
in the same registry as SSRE, it SHOULD use the same one so that identifiers
hash identically in both places.

A hash is not an equality test. A consumer MUST confirm equality after a hash
match.

### 10.3 Ordering

No SKTO operation imposes a collation order. Sorting is SUCA (SUTS-001).
A binary search over a sorted name table orders by unsigned byte value, and
that order is a storage detail, not a textual order.

---

## 11. Resource Constraints

- **Freestanding C99.** SKTO uses only `<stdint.h>`, `<stddef.h>`, and
  `<stdbool.h>`. No `string.h`, no `stdlib.h`, no `stdio.h`.
- **Zero allocation.** No SKTO operation calls an allocator. Every operation
  either reads existing storage or writes caller-provided storage.
- **No unbounded stack.** Admission and validation are single-pass and use
  O(1) state. No operation copies a whole object to the stack.
- **Bounded work.** Validation is O(len) in bytes. Comparison is
  O(min(len_a, len_b)). Structural checks are O(1).
- **No reentrancy hazards.** A borrowed view MUST NOT be handed to a callback
  that can outlive the call, and a published object MUST NOT be mutated by a
  callback at all.
- **Integer discipline.** Length arithmetic uses `size_t`; every addition
  that could overflow is checked before use. `len` and `cap` are never signed.

These constraints are the same ones SUTR-10 (SUCM) imposes on any conforming
module. SKTO is a Level-0 component under that model: it has no optional heap
tier and no fallback path.

---

## 12. Implementation Evidence (Informative)

This section records what the OpenWindows tree does today. It is evidence of
current behaviour, not proof of conformance, and it creates no requirement.

### 12.1 Current kernel text handling

| Location | Current behaviour | SKTO gap |
|----------|-------------------|----------|
| `lib/kstring.c` | `ow_strlen` walks to `'\0'`; `ow_strcmp`/`ow_strncmp` are byte compares; `ow_strncpy` pads with NUL | No descriptor, no explicit length, no capacity |
| `inc/ow_string.h` | The whole string surface is `char *` | Nothing to attach `len`/`cap`/owner to |
| `lib/kprintf.c` | `%s` walks to `'\0'`; output is a byte sink | Cannot print a non-NUL-safe object without a copy |
| `shell/shell.c` | `g_CmdLine[512]`, `g_CwdPath[256]`, 128-byte path components | Fixed buffers with implicit NUL; no capacity is passed to parsers |
| `vfs/vfs.c` | Name compare via `ow_strcmp`; `ow_strncpy(e->Name, Name, 223)` | Names are stored fixed-width and compared by terminator; no publication generation |
| `core/alpc.c` | Port names are `char *` | Same |
| `ps/owx_loader.c` | Image names derived by scanning to `'.'` or `'\0'` | Same |

### 12.2 What the tree already gets right

`syscall/dispatcher.c` contains two behaviours that SKTO generalizes:

- `syscall_user_range` checks every page of a user range against the
  caller's VAD tree, not just the endpoints, and rejects arithmetic overflow.
- `syscall_copy_name` copies at most `cap - 1` bytes, validates each page as
  the copy crosses it, and returns failure when the name does not fit rather
  than truncating it into a name that happens to resolve.

Those are exactly steps 2, 3, and 4 of section 7.1, implemented for one call
site. SKTO makes the procedure the rule rather than the exception.

### 12.3 What is missing

- No kernel type states a byte length independently of the bytes.
- No kernel type distinguishes borrowed from owned text.
- No kernel name is immutable by construction, so resolve-then-use is a race
  wherever the name can change.
- No kernel path rejects SCP or Sentinel values inside a string; they are
  simply bytes.

### 12.4 What is not being specified here

`WHAT-TO-DO.md` sketches inline formatting codepoints. SUTR-0 assigns the
System Control Plane to machine directives and states that it is not text.
SKTO therefore does not define formatting, and it rejects SCP values in text
objects. Where formatting lives - an SCP directive stream, a separate
presentation layer, or a future standard - is an open question recorded in
section 14 and is not decided by this document.

---

## 13. Conformance Requirements

An implementation conforms to SUAS-018 when all of the following hold:

1. It exposes a text object with the observable quantities of section 4.1.
2. Every read is bounded by `len`, and every write is bounded by `cap`.
3. No operation derives length by scanning for `U+0000`.
4. The four ownership classes exist and their lifetime rules are enforced at
   every interface that accepts a text object.
5. The borrow-or-copy rules of section 6 are applied, and no borrowed view is
   retained.
6. Untrusted text passes through the admission procedure of section 7.1.
7. Failed admission is distinguishable from empty text, and no failure path
   returns a truncated object as success.
8. The interior form is SUTF-8, and no other SUTF form is stored in a text
   object.
9. SCP values and the sixteen terminal codepoints are rejected.
10. Published names satisfy section 9 and cannot be mutated after
    publication.
11. Comparison is byte-exact and does not normalize, case-fold, or consult a
    locale.
12. The implementation allocates nothing on any SKTO path and compiles as
    freestanding C99.

An implementation that satisfies 1-12 is conformant whether or not it also
implements SUTS-012 (SCTS) or any other optional specification. Conversely,
implementing SUTS-012 does not excuse a failure of any requirement above.

---

## 14. Open Questions (Informative)

These are recorded because the architecture does not yet decide them. None is
resolved by this document.

- **Q1. Formatting.** `WHAT-TO-DO.md` proposes inline formatting codepoints.
  SUTR-0 gives the SCP to machine directives and says it is not text. If
  formatting is to be in-band, it needs an owner and a rule for whether a
  text object may contain it. SKTO currently says no.
- **Q2. Interior NUL policy.** SKTO permits `U+0000` in free text and forbids
  it in names. A kernel that never needs interior NULs could forbid it
  everywhere; that is a tightening, not a change, and is left to a future
  revision.
- **Q3. Normalization point.** SKTO forbids normalizing during comparison and
  requires the producer to normalize before publication when the namespace
  wants canonical equivalence. Whether the kernel should normalize at
  admission instead is a performance and identity question that belongs to
  the namespace owners.
- **Q4. Handle representation.** Section 9.4 requires a handle but does not
  define one. The VFS object model and the ALPC port model may need different
  answers.
- **Q5. Generation width.** A 32-bit generation wraps. Whether that is
  acceptable depends on the publication rate of each namespace; no namespace
  in the current tree publishes often enough to matter, but the reasoning is
  not recorded anywhere.

---

## 15. Terms and Conventions

- **SKTO** is this standard, SUAS-018.
- **SUTF** is the SuperUnicode Transformation Format family (SUTR-1).
- **SCP** is the System Control Plane, `0x110000`-`0x11FFFF` (SUTR-0).
- **SUCM** is the SuperUnicode Conformance Model (SUTR-10).
- **SSRE** is the specification registry (SUAS-010).
- **SIS** is the identifier standard (SUAS-007).
- **SCSS** is the character security specification (SUTS-005).
- **SUCF** is the canonical-form standard (SUAS-004).
- The key words MUST, MUST NOT, SHOULD, SHOULD NOT, and MAY are used as in
  RFC 2119, and a MUST is a condition of SuperUnicode conformance.

---

## 16. Testing

A conforming implementation MUST ship tests that demonstrate, at minimum:

1. **Structural rejection.** A descriptor with `len > cap`, a `NULL` buffer
   with non-zero `len`, or an unknown owner is rejected in O(1).
2. **No terminator dependence.** A valid object whose `bytes[0]` is not `0x00`
   and whose `bytes[len]` is not `0x00` is read correctly, and a byte beyond
   `len` is never inspected (guard-page or canary test).
3. **Capacity refusal.** A write of more than `cap` returns
   `SKTO_E_CAPACITY` and leaves no truncated success.
4. **Lifetime.** A borrowed object is not retained: a test that frees the
   caller buffer after the call and runs a leak/use detector observes no
   access.
5. **Admission.** Each of the eight steps in section 7.1 has a test that
   fails that step and no other, including an overflow range, a range that
   crosses out of the caller mapping, an overlong encoding, a truncated
   sequence, an SCP value, and a Sentinel value.
6. **Publication.** A published name cannot be modified; a replacement
   increments the generation; a reader of the old generation cannot observe a
   partially written entry.
7. **Comparison and hash.** Equal objects hash equally, unequal objects of
   equal length do not compare equal, and a hash collision does not produce
   equality.
8. **Zero allocation.** The test build links no allocator into the SKTO
   translation unit, and the suite runs under that constraint.

The reference implementation in `src/suas/suas_skto.c` ships with
`tests/test_suas_skto.c`, which exercises the eight areas above, including
guard-byte reads, all admission failure modes, publication generations, and a
table-full refusal. A passing suite is evidence for those cases only; it does
not make SKTO conformance a property of the test binary, and the OpenWindows
kernel does not yet link the translation unit.
