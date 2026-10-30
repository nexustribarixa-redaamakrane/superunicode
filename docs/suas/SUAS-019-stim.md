# SUAS-019 - System Text Input Model (STIM)

**Standard ID:** SUAS-019
**Status:** Draft - Authoring
**Scope:** Core Kernel Invariant - deterministic resolution of key events into bounded SUTF-8 text
**Applies To:** SUAS-004 (SUCF), SUAS-007 (SIS), SUAS-010 (SSRE), SUAS-018 (SKTO), SUTR-0 (SUCS Core), SUTR-1 (SUTF), SUTR-10 (SUCM), the OpenWindows kernel input path (`Drivers/kbdlayout/`, `core/`), OpenWindows-Essentials, `owterm`, the shell, and any application or input method that turns a key event into text
**Last Updated:** 2026-10-29

---

## 1. Overview

A key event is not text. It is a physical fact — a key was pressed, with a set
of modifiers, in a particular state. Text is the result of resolving that fact
against a layout, a composition state, and the admissibility rules of the
character set. The resolution is a small state machine, and every application
that needs input currently builds its own.

That is the gap STIM closes. It defines:

- the event that enters the model;
- the layout that maps an event to a codepoint sequence;
- the pending state that makes a dead key or a compose sequence work;
- the rules that decide when the pending state is committed or flushed;
- the bounds that keep the machine allocation-free and terminating; and
- the admissibility rules that keep a key from injecting a directive or a trap.

STIM is a **mandatory** member of the SuperUnicode Architecture Standards. A
conforming implementation MUST satisfy sections 3 through 10 **if it accepts
text input**. An implementation with no input device satisfies STIM trivially
and MUST still avoid claiming that it implements an input model.

The reference implementation is deliberately smaller than a production
keyboard stack. It does not ship a layout database, a locale, or an input
method protocol. It ships the state machine those things must agree on.

### 1.1 Goals

- **One event, one meaning.** The same event stream and the same layout produce
  the same text on every run and every machine.
- **Composition is explicit.** A dead key is pending state, not a hidden buffer
  inside an application.
- **Bounded state.** A pending composition, a layout, and an output sequence
  have fixed ceilings.
- **No directive injection.** A key cannot produce a System Control Plane value,
  a surrogate, a trap, or the Sentinel.
- **Zero allocation.** Events, layouts, and output buffers are caller-supplied.
- **Deterministic failure.** When output does not fit, nothing is emitted and
  the caller learns why.

### 1.2 Non-Negotiable Invariants

1. **Events are not text.** No component may place a keycode into a text object
   or a console cell. Resolution through STIM is mandatory.
2. **Pending state is owned.** A pending dead key belongs to one input context
   and MUST NOT be shared between contexts.
3. **Pending state is resolved before new text.** A conforming implementation
   MUST flush or compose a pending dead key before emitting text from the next
   binding.
4. **No allocation.** STIM performs no dynamic allocation on any path.
5. **No inadmissible output.** SCP values (`0x110000`–`0x11FFFF`), surrogates
   (`0xD800`–`0xDFFF`), traps (`0x7FFFFFF0`–`0x7FFFFFFE`), and the Sentinel
   (`0x7FFFFFFF`) MUST NOT appear in emitted text.
6. **No partial emission.** A call that cannot fit its entire result in the
   destination buffer MUST emit nothing and report `STIM_E_CAPACITY`.
7. **Release is silent.** A key release produces no text and does not change
   pending composition state.
8. **Layout is data.** The state machine is fixed; the mapping from key to
   codepoint is a caller-supplied table. A layout cannot introduce a new state.

---

## 2. Scope

STIM defines:

- the event descriptor, modifiers, and action classes;
- the layout binding table and its validation rules;
- the dead-key and composition state machine;
- the exact matching and precedence rules for bindings;
- the emission, flush, and reset rules;
- the bound on a single output sequence and on layout capacity;
- the admissibility test for emitted codepoints;
- the contract for committed text that did not come from a key event.

STIM does **not** define:

- a keyboard layout database, a locale, or a keycap legend;
- an input method protocol, a candidate window, or a network IME;
- how text is normalized — SUAS-004 (SUCF) owns canonical forms, and a caller
  that requires normalized input applies SUCF after STIM;
- how text is segmented or identified — SUAS-006 (STS) and SUAS-007 (SIS);
- how text is stored, compared, or published — SUAS-018 (SKTO);
- how text is placed in a console — SUTS-012 (SCTS);
- how a terminal parses control sequences — SUTS-013 (SCTE).

Committed text from an input method, a paste, or a synthetic event does not
enter through the key path. It is admitted as text by SUAS-018 §7 and then
consumed like any other text object.

---

## 3. Definitions

- **TD1 - Key event.** A tuple of physical keycode, modifier mask, and action.
- **TD2 - Binding.** A layout entry mapping a keycode and modifier mask to a
  sequence of codepoints, or to a dead-key codepoint.
- **TD3 - Text binding.** A binding that emits one or more codepoints directly.
- **TD4 - Dead binding.** A binding that makes its codepoint pending instead of
  emitting it.
- **TD5 - Pending codepoint.** The spacing form of the most recent dead binding,
  held until it is composed, flushed, or discarded.
- **TD6 - Composition.** A table entry mapping a pending codepoint and the first
  codepoint of the next text binding to one result codepoint.
- **TD7 - Flush.** Emitting the pending codepoint unchanged because the next
  event did not compose with it.
- **TD8 - Input context.** The state owned by one editor, shell, or text field.
  Pending state never crosses a context boundary.
- **TD9 - Committed text.** Text that arrives already resolved, such as a paste
  or an input-method commit. It is admitted by SKTO, not by STIM.
- **TD10 - Inadmissible codepoint.** A codepoint listed in invariant 5.
- **TD11 - Exact modifier match.** A binding matches when every required
  modifier is present and every forbidden modifier is absent. Unlisted
  modifiers are ignored.
- **TD12 - Layout order.** The order in which bindings were added. The first
  exact match wins.

---

## 4. Events

### 4.1 Descriptor

The normative fields are:

```c
typedef enum {
    STIM_ACT_PRESS   = 0,
    STIM_ACT_RELEASE = 1,
    STIM_ACT_REPEAT  = 2
} stim_action_t;

typedef struct {
    uint32_t      keycode;    /* physical key identity, layout-defined */
    uint16_t      modifiers;  /* STIM_MOD_*                            */
    uint8_t       action;     /* stim_action_t                         */
    uint8_t       reserved;   /* MUST be zero                          */
} stim_event_t;
```

The modifier mask is:

| Bit | Name | Meaning |
|-----|------|---------|
| `0x0001` | `STIM_MOD_SHIFT` | shift |
| `0x0002` | `STIM_MOD_CTRL` | control |
| `0x0004` | `STIM_MOD_ALT` | alt / option |
| `0x0008` | `STIM_MOD_META` | meta / command |
| `0x0010` | `STIM_MOD_CAPS` | caps lock active |
| `0x0020` | `STIM_MOD_NUM` | num lock active |
| `0x0040` | `STIM_MOD_ALTGR` | alternate graphic |

Bits above `0x0040` are reserved and MUST be rejected by a binding or ignored
by an event. A layout that depends on a reserved bit is not STIM-conforming.

### 4.2 Actions

- **PRESS** resolves a binding, subject to the composition state.
- **REPEAT** resolves exactly like PRESS. Auto-repeat is a property of the input
  device, not of the text model.
- **RELEASE** emits nothing, changes nothing, and MUST return `STIM_OK`. It is
  recorded only if the implementation counts events.

An event with an unknown action returns `STIM_E_ACTION`.

### 4.3 Event order

The model is a pure function of the event sequence. It never consults wall-clock
time, a repeat delay, a locale, or the environment. Two runs that receive the
same events in the same order with the same layout MUST produce the same bytes.

---

## 5. Layouts

### 5.1 Binding descriptor

```c
typedef enum {
    STIM_BIND_TEXT = 0,
    STIM_BIND_DEAD = 1
} stim_bind_kind_t;

typedef struct {
    uint32_t keycode;
    uint16_t required;       /* modifiers that must be present */
    uint16_t forbidden;      /* modifiers that must be absent  */
    uint8_t  kind;           /* stim_bind_kind_t               */
    uint8_t  count;          /* codepoints in cps; 1..8        */
    uint32_t cps[STIM_MAX_SEQUENCE];
} stim_binding_t;
```

`required` and `forbidden` MUST be disjoint. A binding with an overlap is
invalid because no event could ever satisfy it.

### 5.2 Composition descriptor

```c
typedef struct {
    uint32_t pending;   /* spacing form of the dead key */
    uint32_t next;      /* first codepoint of the text binding */
    uint32_t result;    /* composed codepoint */
} stim_composition_t;
```

### 5.3 Validation rules

A layout is valid only when:

- **L1** the binding and composition arrays are caller-owned and non-NULL when
  their capacity is non-zero;
- **L2** every keycode is distinct within the same `(required, forbidden)`
  mask; adding a duplicate MUST fail with `STIM_E_BINDING`;
- **L3** every binding has `1 <= count <= STIM_MAX_SEQUENCE`;
- **L4** every codepoint in a binding or composition is admissible;
- **L5** `required & forbidden == 0`;
- **L6** a dead binding carries exactly one codepoint;
- **L7** every composition pair `(pending, next)` is unique;
- **L8** capacity is never exceeded; adding beyond capacity fails with
  `STIM_E_FULL` and leaves the layout unchanged.

### 5.4 Precedence

The layout is ordered. For one event, the resolver scans from the first binding
to the last and selects the first binding whose keycode matches and whose
modifier condition is exact. Layout authors control precedence by insertion
order. A resolver MUST NOT use a hash order, a pointer order, or a
platform-specific lookup that changes the result.

The reference implementation documents the scan as O(n) in the number of
bindings. A conforming implementation may index the table, provided the
observable result is the same as the ordered scan.

---

## 6. The State Machine

### 6.1 States

An input context is in exactly one of two states:

- **IDLE** — no pending codepoint.
- **PENDING** — one dead codepoint is held.

A context also carries monotonic counters for events seen and text emitted.
Counters are diagnostic; they never affect text.

### 6.2 Transitions

For a **text binding** `b` with codepoints `b.cps[0..b.count-1]`:

1. If the context is IDLE, emit all of `b.cps`.
2. If the context is PENDING with codepoint `p`:
   - if a composition `(p, b.cps[0]) -> r` exists, emit `r`, then emit
     `b.cps[1..count-1]`, and clear the pending state;
   - otherwise flush `p`, emit all of `b.cps`, and clear the pending state.

For a **dead binding** `b` carrying `d`:

1. If the context is PENDING with `p`, flush `p` first.
2. Set the pending state to `d`.

For a **release**, do nothing.

A **press or repeat with no matching binding** emits nothing, changes no pending
state, and returns `STIM_OK`. An unmapped key is an ordinary event, not an
error: function keys, media keys, and platform-reserved keys are expected to
reach the model and produce no text.

For **flush** requested explicitly, emit the pending codepoint if one exists and
clear the state. Flush on an idle context emits nothing and returns `STIM_OK`.

For **reset**, discard the pending codepoint without emitting it. Reset is for
context destruction and window changes, not for ordinary editing. A caller that
wants the spacing form MUST call flush before reset.

### 6.3 Composition scope

Composition matches only the **first** codepoint of the next text binding. A
binding that emits several codepoints composes its head and appends its tail
unchanged. This rule keeps composition deterministic when a layout emits a
base plus a combining mark.

### 6.4 Double dead keys

Pressing two dead keys in a row flushes the first spacing form, then makes the
second pending. The model does not attempt to compose dead-key sequences beyond
one step. A layout that needs multi-step input uses an input method and commits
text through SKTO admission.

---

## 7. Emission

### 7.1 Encoding

Emitted text is SUTF-8 (SUTR-1). The reference implementation encodes directly
into the caller's buffer. A conforming implementation MAY use another interior
form only if it can prove the bytes are a valid SUTF-8 encoding of the same
codepoint sequence.

### 7.2 All-or-nothing

A call computes the complete encoded length before writing. If the destination
cannot hold the result, the call:

- writes nothing;
- leaves pending state unchanged;
- sets `*out_len` and `*out_cp_count` to `0`; and
- returns `STIM_E_CAPACITY`.

This rule is what makes the model safe in a kernel: a failed input event cannot
leave half a codepoint in a caller buffer or consume a dead key silently.

### 7.3 Output descriptor

```c
typedef struct {
    const uint8_t *bytes;   /* caller buffer, or NULL when len == 0 */
    size_t         len;     /* bytes written                        */
    size_t         cap;     /* caller capacity                      */
    size_t         cp_count;/* codepoints represented               */
    bool           flushed; /* true when a dead key was flushed     */
} stim_output_t;
```

`flushed` is diagnostic. It is true when the call emitted a pending spacing
form because no composition matched.

### 7.4 Normalization boundary

STIM does not normalize. If a consumer needs canonical equivalence, it applies
SUAS-004 (SUCF) to the emitted text before comparison or publication. A layout
author SHOULD build composed forms directly when the target is an identifier,
so that the common path does not need a second pass.

---

## 8. Admissibility

A codepoint is admissible when it is not:

- in the System Control Plane, `0x00110000`–`0x0011FFFF`;
- a surrogate, `0x0000D800`–`0x0000DFFF`;
- a trap, `0x7FFFFFF0`–`0x7FFFFFFE`;
- the Sentinel, `0x7FFFFFFF`; or
- above the last allocatable codepoint, `0x7FFFFFEF`.

The check is applied at binding registration and again at emission. Checking
only at registration is insufficient because a caller could mutate a layout
after validation; checking only at emission would let a bad layout exist. Both
checks are required.

A key event cannot carry a codepoint at all. Only a layout can, which is why a
layout is trusted data and must be validated like any other trusted table.

---

## 9. Resource Constraints

| Resource | Ceiling | Owner |
|----------|---------|-------|
| Codepoints per binding | `STIM_MAX_SEQUENCE` (8) | header |
| Bindings per layout | `STIM_MAX_BINDINGS` (128) | header |
| Compositions per layout | `STIM_MAX_COMPOSITIONS` (128) | header |
| Pending codepoints | 1 | model |
| Allocation | 0 bytes | model |
| Recursion | none | model |

No operation scans an unbounded input. `stim_feed` reads one event, not a
stream. A caller that has a stream of events calls it once per event.

The implementation is freestanding C99. It includes only `<stdint.h>`,
`<stddef.h>`, and `<stdbool.h>`, and it calls no library function.

---

## 10. Conformance Requirements

A conforming STIM implementation MUST satisfy each clause. Clause identifiers
are stable.

1. **[C1]** It MUST resolve a `PRESS` or `REPEAT` event through the layout and
   MUST emit only the codepoints selected by section 6. (Section 6.2)
2. **[C2]** It MUST emit nothing and change nothing for a `RELEASE` event.
   (Section 4.2)
3. **[C3]** It MUST compose a pending dead key with the first codepoint of the
   next text binding when a composition entry exists. (Section 6.2)
4. **[C4]** It MUST flush a pending dead key as its spacing form when the next
   text binding does not compose with it. (Section 6.2)
5. **[C5]** It MUST flush an earlier dead key before making a later one pending.
   (Section 6.4)
6. **[C6]** It MUST use the first exact binding match in insertion order and
   MUST reject duplicate binding keys. (Sections 5.3, 5.4)
7. **[C7]** It MUST reject a layout containing an inadmissible codepoint, a
   modifier overlap, a zero-length binding, or an oversized sequence.
   (Section 5.3)
8. **[C8]** It MUST apply the admissibility test again at emission.
   (Section 8)
9. **[C9]** It MUST encode emitted text as valid SUTF-8. (Section 7.1)
10. **[C10]** It MUST emit nothing and report `STIM_E_CAPACITY` when the result
    does not fit, leaving pending state unchanged. (Section 7.2)
11. **[C11]** It MUST NOT allocate, recurse, or read past the layout capacity.
    (Section 9)
12. **[C12]** It MUST produce identical bytes for identical event sequences,
    layouts, and initial states. (Section 4.3)
13. **[C13]** It MUST keep pending state per input context. (Section 6.1)
14. **[C14]** It MUST provide an explicit flush and an explicit reset with the
    behaviour of section 6.2. (Section 6.2)

---

## 11. Implementation Evidence (Informative)

The reference implementation lives in `src/suas/suas_stim.c` with the public
header `include/suas/suas_stim.h`. The unit suite `tests/test_suas_stim.c`
exercises binding validation, dead-key composition, unmatched flush, capacity
failure, inadmissibility, repeat and release handling, UTF-8 encoding, and
determinism.

The suite also checks the canonical SUTF-8 framing boundaries: four bytes
through `0x0010FFFF`, five bytes through `0x03FFFFFF`, and six bytes above
that, with each emitted sequence validated through the SKTO reader. The suite
proves the cases it asserts. It does not prove that the OpenWindows kernel,
`kbdlayout`, the shell, or any application has adopted STIM. No such claim is
made here.

At the time of writing, the OpenWindows keyboard path resolves scan codes to
ASCII-oriented characters locally. STIM is the contract that path would need to
adopt before the same physical key could be guaranteed to produce the same text
in two applications.

---

## 12. Open Questions (Informative)

1. **Layout file format.** The model treats a layout as a caller-supplied table.
   A portable on-disk layout format is not defined. It would be a separate
   SUTS document, not an extension of STIM.
2. **Input method protocol.** Committed text enters through SKTO admission.
   A candidate window, a pre-edit buffer, and a remote IME protocol are out of
   scope and would need their own specification.
3. **Level-3 and level-5 shift states.** `ALTGR` covers the common case.
   A future revision may add explicit level bits if a layout needs more than
   two levels per modifier class.
4. **Repeat coalescing.** The model treats every repeat as a press. A device
   driver may coalesce repeats before the model sees them; the model does not.

---

## 13. Terms and Conventions

- **STIM** is this document, SUAS-019.
- **SKTO** is SUAS-018, System Kernel Text Objects.
- **SCTS** is SUTS-012, System Console Text Service.
- **SCTE** is SUTS-013, System Console Terminal Emulation.
- **SUCF**, **SIS**, **STS**, **SGW**, **SBR** are the SUAS standards named in
  their citations.
- **SUTF-8** is the eight-bit transformation format of SUTR-1.
- **SCP** is the System Control Plane defined by SUTR-0.
- The key words **MUST**, **MUST NOT**, **SHOULD**, and **MAY** carry their
  usual normative force. Lower-case uses are explanatory.

---

## 14. Testing

A conforming test suite MUST cover at least:

1. A text binding with one codepoint and with several codepoints.
2. A dead key followed by a composing key.
3. A dead key followed by a non-composing key, checking the flush order.
4. Two dead keys in a row.
5. An explicit flush on a pending and on an idle context.
6. An explicit reset, checking that the pending form is discarded.
7. A release event, checking that it emits nothing and preserves pending state.
8. A repeat event, checking that it behaves like a press.
9. Duplicate, overlapping, empty, oversized, and inadmissible bindings.
10. A destination buffer one byte too small, checking all-or-nothing failure.
11. A destination buffer exactly large enough.
12. Codepoints at the SCP, surrogate, trap, and Sentinel boundaries.
13. Two identical runs, checking byte-for-byte determinism.
