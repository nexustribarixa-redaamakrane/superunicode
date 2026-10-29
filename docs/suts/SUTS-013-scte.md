# SUTS-013 - System Console Terminal Emulation (SCTE)

**Standard ID:** SUTS-013
**Status:** Draft - Authoring
**Scope:** Modular Technical Specification (terminal control parsing over an SCTS plane: C0 controls, ESC, CSI, OSC, SCP directives, partial-sequence state)
**Applies To:** SUTS-012 (SCTS), SUAS-002 (SGW), SUAS-003 (SBR), SUAS-006 (STS), SUAS-018 (SKTO), SUTR-0 (SUCS Core), SUTR-1 (SUTF), SUTR-10 (SUCM), the OpenWindows-Essentials terminal (`Software/owterm`, `ttydrv`), and any kernel or user-mode component that interprets a terminal byte stream
**Last Updated:** 2026-10-29

---

## 1. Overview

A terminal emulator is a byte-stream parser that drives a console plane. It is
not a second console. The plane decides where a glyph goes; the terminal decides
which control the bytes request. SUTS-012 (SCTS) deliberately stops at
placement, so it leaves a real gap: two terminals can agree on cells and still
disagree about what `ESC [ 2 J` means.

SCTE closes that gap for the control vocabulary an OpenWindows application is
expected to receive. It defines:

- the parser states and the partial-sequence rules;
- the C0 controls it honours and the ones it ignores;
- the ESC sequences it recognises;
- the CSI subset it executes, including the default-parameter rules;
- what happens to an unsupported, malformed, or oversized sequence;
- how a System Control Plane directive is delivered without becoming a cell;
- the bounds that keep the parser allocation-free and terminating.

SCTE is an **optional** SuperUnicode Technical Specification. An implementation
is not required to provide a terminal to be SuperUnicode-conforming. An
implementation that **claims SCTE conformance MUST satisfy every MUST in this
document**, and the claim is false if it does not. Claiming SCTE never relaxes a
SUAS invariant and never overrides SCTS.

The rule of deference is the same as SCTS:

> The terminal does not decide how wide a character is. SUAS-002 (SGW) decides.
> The terminal does not decide where a line may break. SUAS-003 (SBR) decides.
> The terminal does not decide where a cluster ends. SUAS-006 (STS) decides.
> The terminal does not decide where a cell is. SUTS-012 (SCTS) decides.

### 1.1 Goals

- **One parser shape.** Applications do not each invent a different CSI state
  machine.
- **No partial effect.** A sequence is executed only after its final byte
  arrives. A truncated sequence never moves the cursor.
- **Bounded state.** Parameters, intermediates, and OSC payloads have fixed
  ceilings.
- **Directives stay directives.** An SCP value is delivered to a callback or
  counted as dropped; it never becomes a cell.
- **Transport independence.** The same parser drives any SCTS sink and any
  caller-supplied plane.
- **Zero allocation.** The plane, the parser state, and the callbacks are
  caller-supplied.

### 1.2 Non-Negotiable Invariants (on claim)

1. **Placement is SCTS.** Text is placed only through the SCTS plane API. SCTE
   does not maintain a second cell array.
2. **Width, breaks, and clusters are not SCTE's.** A line feed and a carriage
   return are cursor operations; they do not redefine SBR or STS.
3. **No partial sequence.** A parser MUST NOT apply any part of a control
   sequence before it has seen the final byte.
4. **No allocation.** SCTE allocates nothing on any path.
5. **No SCP as content.** A System Control Plane codepoint is a directive. It is
   never placed in a cell and never serialized as text.
6. **Unsupported is not fatal.** An unknown sequence is consumed as a whole and
   counted. It never corrupts the plane and never aborts the stream.
7. **Bounded payload.** An OSC payload that exceeds the fixed ceiling is
   discarded, not buffered without limit.
8. **Encoding is SUTF-8.** The text path decodes SUTF-8; control bytes are the
   ASCII subset and never appear inside a multibyte sequence.

---

## 2. Scope

SCTE defines:

- the terminal state, parser states, and carry rules;
- the C0 control vocabulary;
- the ESC, CSI, and OSC subsets;
- parameter parsing, defaults, and bounds;
- execution of cursor, erase, insert/delete, scroll, and style sequences;
- unsupported and malformed sequence behaviour;
- SCP directive delivery;
- the interaction with the SCTS plane and sink;
- conformance and testing requirements.

SCTE does **not** define:

- the plane, cell, cursor, wrap, or scroll model — SUTS-012 (SCTS);
- width, break opportunities, or cluster boundaries — SUAS-002/003/006;
- a font, a glyph, or a rasterizer;
- a terminal window, a scrollback UI, or a selection model;
- a device protocol such as a serial line, a pseudo-terminal, or a network
  transport;
- an operating-system shell, a job-control model, or a session manager;
- colour palettes beyond the SCTS foreground and background indices.

---

## 3. Relationship to the Authoritative Standards

| Question | Authority | SCTE role |
|----------|-----------|-----------|
| Where is the next cell? | SUTS-012 (SCTS) | set the plane cursor and call plane operations |
| How wide is this character? | SUAS-002 (SGW) | never consulted directly; SCTS calls SGW |
| May the line break here? | SUAS-003 (SBR) | never chosen by SCTE; a newline is a cursor move |
| Where does this cluster end? | SUAS-006 (STS) | never chosen by SCTE; text is passed to SCTS |
| Is this a valid text object? | SUAS-018 (SKTO) | parameters and payloads are bounded integers, not text objects |
| What does this codepoint mean? | SUTR-0 (SUCS Core) | the SCP range is a directive, not a cell |
| What is the byte encoding? | SUTR-1 (SUTF) | the parser consumes SUTF-8 and ASCII controls |

Where this specification and an SCTS rule disagree, SCTS governs, because SCTE
claims conformance to SCTS and cannot weaken it.

### 3.1 Why the parser is separate from the plane

SCTS has no notion of an escape sequence, and it should not gain one. A console
library is useful to a boot loader, a kernel log, and a full-screen application
alike; only the last of those needs ANSI parsing. Keeping the parser separate
lets a minimal consumer link SCTS alone and a terminal consumer link both.

### 3.2 Accepted vocabulary

The vocabulary is a **subset**, chosen for what OpenWindows applications already
emit. A sequence outside the subset is consumed and counted, not executed. This
is a deliberate compatibility position: the parser is predictable, and a future
extension is a new clause rather than an accident of a permissive parser.

---

## 4. The Terminal

### 4.1 Terminal state

```c
typedef struct {
    scts_plane_t     *plane;        /* borrowed; never owned            */
    scte_state_t      state;        /* parser state                     */
    uint8_t           carry[8];     /* incomplete SUTF-8 bytes          */
    uint8_t           carry_len;
    uint32_t          params[SCTE_MAX_PARAMS];
    uint8_t           param_count;
    uint8_t           prefix;       /* '?' / '>' / '!' or 0             */
    uint8_t           intermediate; /* one 0x20..0x2F byte or 0         */
    bool              have_param;   /* current parameter has digits     */
    uint16_t          saved_row;
    uint16_t          saved_col;
    bool              cursor_visible;
    bool              origin_mode;
    uint64_t          controls;     /* C0 and ESC executions            */
    uint64_t          sequences;    /* executed CSI sequences           */
    uint64_t          unsupported;  /* consumed but not executed        */
    uint64_t          directives;   /* SCP codepoints delivered/dropped */
    uint64_t          encoding_errors;
    scte_scp_fn       scp;
    scte_bell_fn      bell;
    uintptr_t         token;
} scte_term_t;
```

The terminal borrows the plane. It never frees it, never reallocates it, and
never replaces its cell array. A terminal initialised over a plane that is not
itself initialised MUST fail with `SCTE_E_PLANE`.

### 4.2 Parser states

| State | Meaning | Final transition |
|-------|---------|------------------|
| `SCTE_GROUND` | text and C0 | text to SCTS, C0 executes |
| `SCTE_ESC` | after `0x1B` | `[` → CSI; `]` → OSC; others execute or ignore |
| `SCTE_CSI_ENTRY` | after `ESC [` | parameter, intermediate, or final byte |
| `SCTE_CSI_PARAM` | parameter digits | digit, `;`, intermediate, or final |
| `SCTE_CSI_INTERMEDIATE` | one intermediate byte | final byte |
| `SCTE_OSC` | after `ESC ]` | `BEL` or `ESC \` terminates |
| `SCTE_OSC_ESC` | after `ESC` inside OSC | `\` terminates; other bytes continue OSC |

A state transition that is not listed is a malformed sequence. The parser
MUST consume it, reset to `GROUND`, increment `unsupported`, and apply no
effect.

### 4.3 Ground text

In `GROUND`, a decoded codepoint is:

- a C0 control (`0x00`–`0x1F`, `0x7F`) — executed or ignored by section 5;
- an SCP codepoint (`0x110000`–`0x11FFFF`) — delivered as a directive;
- anything else — passed to `scts_plane_put_cp` as one cluster.

A codepoint that is a surrogate, a trap, or the Sentinel is inadmissible. The
parser consumes it, increments `unsupported`, and does not pass it to the plane.

### 4.4 SUTF-8 carry

The parser accepts bytes, not codepoints. An incomplete trailing sequence is
stored in `carry` and consumed. A later `suts_scte_feed` continues it. The carry
ceiling is eight bytes, which is more than the six-byte SUTF-8 maximum.

If the carry already holds a complete but invalid sequence, or a continuation
byte arrives in the wrong position, the parser MUST:

1. increment `encoding_errors`;
2. discard the carry;
3. return `SCTE_E_ENCODING`; and
4. leave the plane unchanged for that byte.

`SCTE_E_ENCODING` is recoverable: the caller may continue with the next byte.
`suts_scte_finish` reports `SCTE_E_INCOMPLETE` when the carry or parser state is
not empty at end of stream.

---

## 5. C0 Controls

| Byte | Name | Effect |
|------|------|--------|
| `0x00` | NUL | ignored |
| `0x07` | BEL | call `bell` if bound; no cell change |
| `0x08` | BS | `scts_plane_backspace` |
| `0x09` | HT | `scts_plane_tab` |
| `0x0A` | LF | `scts_plane_newline` |
| `0x0B` | VT | same as LF |
| `0x0C` | FF | same as LF |
| `0x0D` | CR | `scts_plane_carriage_return` |
| `0x0E` | SO | ignored, counted |
| `0x0F` | SI | ignored, counted |
| `0x1B` | ESC | enter `SCTE_ESC` |
| `0x7F` | DEL | ignored |

Every other C0 control is ignored and counted in `controls`. Ignoring is not a
failure; the sequence is not a text cell.

A C0 control inside `ESC`, CSI, or OSC is handled by section 6, not by this
table. In particular, `ESC` inside a CSI sequence restarts the parser at
`SCTE_ESC`; it does not execute a text control.

---

## 6. Escape Sequences

### 6.1 ESC

| Sequence | Effect |
|----------|--------|
| `ESC [` | enter CSI |
| `ESC ]` | enter OSC |
| `ESC 7` | save cursor row and column |
| `ESC 8` | restore cursor row and column, clamped to the plane |
| `ESC D` | index: newline |
| `ESC E` | next line: carriage return then newline |
| `ESC M` | reverse index: move up one row, or scroll down at the top |
| `ESC c` | full reset: clear the plane and reset parser state |
| `ESC \` | string terminator outside OSC: ignored |
| anything else | consume and count as unsupported |

### 6.2 OSC

`ESC ]` enters OSC. The parser consumes bytes until `BEL` (`0x07`) or the
two-byte `ESC \` terminator. The payload is not interpreted in this revision.
It is bounded by `SCTE_MAX_OSC` bytes; once the ceiling is exceeded, the parser
continues consuming but discards the payload and marks the sequence
unsupported.

An OSC sequence never writes to the plane. A title, a hyperlink, or a palette
request is a presentation concern outside this specification.

### 6.3 CSI

A CSI sequence has the form:

```text
ESC [ <parameter bytes> <intermediate bytes> <final byte>
```

- parameter bytes are `0x30`–`0x3F` (digits, `;`, `:`, and a private prefix);
- intermediate bytes are `0x20`–`0x2F`;
- the final byte is `0x40`–`0x7E`.

The parser accepts at most `SCTE_MAX_PARAMS` parameters and one intermediate
byte. A sequence that exceeds either limit is consumed to its final byte and
reported as unsupported. It is never partially executed.

#### 6.3.1 Parameter rules

- An empty parameter is the default for the sequence.
- A missing parameter at the end of the list is the default.
- `0` is a valid explicit value and MUST NOT be rewritten to the default unless
  the individual sequence says so.
- A parameter above `SCTE_MAX_PARAM_VALUE` is clamped to that value.
- A private prefix (`?`, `>`, `!`) is recorded in `prefix`. A sequence with a
  prefix other than the ones listed in section 6.3.3 is unsupported.

#### 6.3.2 Supported CSI sequences

| Final | Name | Parameters (default) | Effect |
|-------|------|----------------------|--------|
| `A` | CUU | n (1) | move cursor up n, clamped to row 0 |
| `B` | CUD | n (1) | move cursor down n, clamped to last row |
| `C` | CUF | n (1) | move cursor right n, clamped to last column |
| `D` | CUB | n (1) | move cursor left n, clamped to column 0 |
| `E` | CNL | n (1) | cursor down n and column 0 |
| `F` | CPL | n (1) | cursor up n and column 0 |
| `G` | CHA | n (1) | cursor column n-1, clamped |
| `H` | CUP | row (1), col (1) | cursor to row-1, col-1, clamped |
| `f` | HVP | row (1), col (1) | same as CUP |
| `J` | ED | mode (0) | erase display: 0 cursor→end, 1 start→cursor, 2 all |
| `K` | EL | mode (0) | erase line: 0 cursor→end, 1 start→cursor, 2 all |
| `L` | IL | n (1) | insert n blank lines at the cursor row |
| `M` | DL | n (1) | delete n lines at the cursor row |
| `P` | DCH | n (1) | delete n cells, shifting the rest left |
| `@` | ICH | n (1) | insert n blank cells, shifting the rest right |
| `S` | SU | n (1) | scroll up n |
| `T` | SD | n (1) | scroll down n |
| `m` | SGR | list (0) | select graphic rendition, section 6.3.4 |
| `h` | SM | `?7`, `?25` | set autowrap or cursor visibility |
| `l` | RM | `?7`, `?25` | reset autowrap or cursor visibility |

An erase fills a cell with `SCTS_MARK_BLANK`. An insert or delete moves whole
cells and does not re-cluster text. A cursor move that lands on the tail of a
wide glyph lands there literally; the next write overwrites from that cell.

#### 6.3.3 Private sequences

Only `?7` and `?25` are recognised, and only with `h` and `l`. Every other
private sequence is consumed and counted as unsupported. In particular, this
revision does not implement alternate screens (`?1049`), bracketed paste
(`?2004`), mouse reporting, or keyboard protocol modes.

#### 6.3.4 SGR subset

| Parameter | Effect |
|-----------|--------|
| `0` | reset foreground and background to default |
| `30`–`37` | foreground index 0–7 |
| `39` | default foreground |
| `40`–`47` | background index 0–7 |
| `49` | default background |
| `90`–`97` | bright foreground index 8–15 |
| `100`–`107` | bright background index 8–15 |
| anything else | ignored within SGR |

An SGR sequence with several parameters applies them left to right. An unknown
parameter does not invalidate the known parameters beside it; SGR is the one
sequence for which partial semantic application is defined, because each
parameter is an independent attribute.

### 6.4 SCP directives

A decoded codepoint in `0x110000`–`0x11FFFF` is a System Control Plane value.
It is **never** a cell. The parser calls `scp(token, cp)` when the callback is
bound. When it is not bound, the parser increments `directives` and otherwise
ignores the value.

The callback MUST NOT call back into the same terminal. It receives the plane
through `token` only if the caller chose to put it there.

---

## 7. Status and Reporting

```c
typedef enum {
    SCTE_OK             = 0,
    SCTE_E_NULL         = 1,
    SCTE_E_PLANE        = 2,
    SCTE_E_ENCODING     = 3,
    SCTE_E_INCOMPLETE   = 4,
    SCTE_E_INTERNAL     = 5
} scte_status_t;
```

- `SCTE_OK` means the bytes were consumed and every complete sequence was
  executed or deliberately ignored.
- `SCTE_E_ENCODING` means a malformed SUTF-8 byte was seen. The parser is
  recoverable and the carry is cleared.
- `SCTE_E_INCOMPLETE` is returned only by `suts_scte_finish` when the carry or
  parser state is not empty.
- `SCTE_E_PLANE` means the borrowed plane is not initialised.
- `SCTE_E_NULL` means a required argument is absent.

An unsupported but well-formed sequence is **not** an error. It increments
`unsupported` and leaves `SCTE_OK`.

---

## 8. Resource Constraints

| Resource | Ceiling | Owner |
|----------|---------|-------|
| Parameters per CSI | `SCTE_MAX_PARAMS` (16) | header |
| Intermediate bytes | 1 | model |
| OSC payload | `SCTE_MAX_OSC` (128) | header |
| SUTF-8 carry | 8 bytes | model |
| Parameter value | `SCTE_MAX_PARAM_VALUE` (65535) | header |
| Allocation | 0 bytes | model |
| Recursion | none | model |

The terminal state is a value type. It contains no pointer other than the
borrowed plane. Copying a terminal copies its parser state but not the plane;
the copy points at the same plane, which is a caller error and is documented
as such.

---

## 9. Conformance Requirements

A conforming SCTE implementation MUST satisfy each clause. Clause identifiers
are stable.

1. **[C1]** It MUST place text only through the SCTS plane API and MUST NOT
   maintain a second cell array. (Section 3)
2. **[C2]** It MUST consume a control sequence completely before applying any
   effect. (Section 1.2, 6.3)
3. **[C3]** It MUST restart at `SCTE_ESC` when `ESC` appears inside a CSI
   sequence, without executing the abandoned sequence. (Section 5)
4. **[C4]** It MUST implement the C0 effects of section 5 and ignore unknown C0
   controls without error. (Section 5)
5. **[C5]** It MUST implement the ESC effects of section 6.1 and count other
   ESC sequences as unsupported. (Section 6.1)
6. **[C6]** It MUST implement the CSI subset of section 6.3.2 with the default
   and clamping rules of section 6.3.1. (Sections 6.3.1, 6.3.2)
7. **[C7]** It MUST consume an unsupported or malformed sequence as a whole,
   increment `unsupported`, and apply no effect. (Sections 4.2, 6.3)
8. **[C8]** It MUST NOT execute a sequence whose parameter or intermediate
   count exceeds the fixed ceilings. (Section 8)
9. **[C9]** It MUST deliver an SCP codepoint to the callback or count it as
   dropped, and MUST NOT place it in a cell. (Section 6.4)
10. **[C10]** It MUST buffer an incomplete SUTF-8 sequence across calls and
    MUST report `SCTE_E_INCOMPLETE` from `suts_scte_finish`. (Section 4.4)
11. **[C11]** It MUST treat malformed SUTF-8 as recoverable: clear the carry,
    increment `encoding_errors`, and continue from the next byte.
    (Section 4.4)
12. **[C12]** It MUST allocate nothing and use no recursion. (Section 8)
13. **[C13]** It MUST leave the plane unchanged when a control sequence cannot
    be executed in full. (Sections 1.2, 6.3)
14. **[C14]** It MUST expose the counters of section 4.1 and keep them
    monotonic. (Section 4.1)

---

## 10. Testing

A conforming test suite MUST cover at least:

1. Plain ASCII and multibyte SUTF-8 text placed through the plane.
2. LF, CR, BS, HT, and VT/FF.
3. BEL with and without a callback.
4. Every supported CSI sequence with an explicit and a default parameter.
5. Cursor clamping at row 0, the last row, column 0, and the last column.
6. ED and EL in all three modes.
7. IL, DL, ICH, and DCH at the plane edges.
8. SU and SD, including the reverse-index scroll-down path.
9. SGR reset, foreground, background, bright, and unknown parameters.
10. `?7` and `?25` set and reset; an unsupported private sequence.
11. An OSC terminated by BEL and by `ESC \`, including the ceiling.
12. A CSI split across two `feed` calls, including the final byte.
13. An incomplete UTF-8 sequence split across two calls.
14. A malformed UTF-8 byte, checking recovery.
15. `finish` on clean, partial-escape, and partial-UTF-8 states.
16. An SCP codepoint with a callback and without one.
17. An unsupported sequence, checking `unsupported` and an unchanged plane.
18. Repeated identical streams, checking identical cell contents.

---

## 11. Implementation Evidence (Informative)

The reference implementation lives in `src/suts/suts_scte.c` with the public
header `include/suts/suts_scte.h`. The unit suite `tests/test_suts_scte.c`
exercises the parser states, the supported CSI subset, partial sequences, SCP
delivery, encoding recovery, and determinism.

The suite proves the cases it asserts. It does not prove that `owterm`,
`ttydrv`, or any other consumer has adopted SCTE. No such claim is made.

The current OpenWindows terminal paths interpret a smaller and different subset
locally. SCTE is the contract they would need to share before two applications
could rely on the same escape behaviour.

---

## 12. Open Questions (Informative)

1. **Alternate screen.** `?1049` is common and deliberately omitted. Adding it
   needs a second plane or a saved cell snapshot, which is a memory-policy
   decision, not a parser decision.
2. **Bracketed paste.** The parser can consume it, but the security model for
   pasted text belongs to SKTO admission and the application.
3. **Mouse reporting.** This is an input protocol, not a terminal control
   protocol. It belongs with STIM (SUAS-019), not SCTE.
4. **OSC interpretation.** Titles, hyperlinks, and palettes need a policy
   document. This revision consumes and discards them.
5. **Wide-character erase.** Erasing half of a wide glyph currently leaves the
   other half as a blank marker. A future revision may define a repair rule.

---

## 13. Terms and Conventions

- **SCTE** is this document, SUTS-013.
- **SCTS** is SUTS-012, System Console Text Service.
- **SKTO** is SUAS-018, System Kernel Text Objects.
- **STIM** is SUAS-019, System Text Input Model.
- **CSI** is Control Sequence Introducer, `ESC [`.
- **OSC** is Operating System Command, `ESC ]`.
- **C0** is the control range `0x00`–`0x1F` plus `0x7F`.
- **SCP** is the System Control Plane defined by SUTR-0.
- **SUTF-8** is the eight-bit transformation format of SUTR-1.
- The key words **MUST**, **MUST NOT**, **SHOULD**, and **MAY** carry their
  usual normative force. Lower-case uses are explanatory.
