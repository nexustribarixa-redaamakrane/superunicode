# SUTS-012 - System Console Text Service (SCTS)

**Standard ID:** SUTS-012
**Status:** Draft - Authoring
**Scope:** Modular Technical Specification (console text plane, cell grid, cursor, wrap and scroll, sink transport, glyph-zone allocation, legacy-sink classification)
**Applies To:** SUAS-002 (SGW), SUAS-003 (SBR), SUAS-006 (STS), SUAS-007 (SIS), SUAS-018 (SKTO), SUTR-0 (SUCS Core), SUTR-1 (SUTF), SUTR-8 (SUCPM), SUTR-10 (SUCM), SUTR-013 (SUEAS), the OpenWindows-Essentials `suct` library, `ttydrv`, `owfont64`, `bootvid`, `owterm`, and any kernel or user-mode component that draws text into a fixed cell grid
**Last Updated:** 2026-10-28

---

## 1. Overview

A console is not a font renderer and not a text engine. It is a **grid of
cells plus a cursor plus a transport**. Its job is to take a stream of text,
decide which cell each glyph occupies, move the cursor, wrap at the right
edge, scroll at the bottom, and hand the resulting rows to something that can
put pixels or bytes on a device.

System Console Text Service (SCTS) specifies that layer. It is the console
counterpart of SKTO (SUAS-018): where SKTO defines how text is carried, SCTS
defines how text is *placed*.

SCTS is an **optional** SuperUnicode Technical Specification. An
implementation is not required to provide a console service to be
SuperUnicode-conforming. However, an implementation that claims SCTS
conformance MUST satisfy every MUST in this document, and that claim is false
if it does not. Claiming SCTS never relaxes a SUAS invariant; in particular,
it does not permit a console to compute its own widths, invent its own
segmentation, or mint its own BANcodes.

The single most important rule in this specification is a rule of
**deference**:

> A console does not decide how wide a character is. SUAS-002 (SGW) decides.
> A console does not decide where a line may break. SUAS-003 (SBR) decides.
> A console does not decide where a character cluster ends. SUAS-006 (STS)
> decides.

The current `suct` library (section 12, informative) does the opposite: it
carries a private range table for zero- and double-width codepoints, a private
notion of wrapping, and an ad-hoc Private Use Area allocation. Those are
evidence of what a console needs, not a licence to fork the architecture.

### 1.1 Goals

- **One authoritative width source.** Every cell decision routes through
  SGW; the console owns no width table.
- **One authoritative break source.** Wrapping is a line-break decision and
  therefore routes through SBR; the console owns no break table.
- **Cluster correctness.** A combining mark is placed with its base
  character, not dropped.
- **Bounded, allocation-free operation.** The plane is fixed, the cells are
  fixed, the sinks are caller-supplied, and no path allocates.
- **Transport independence.** The same plane can drive a byte sink, an
  SUTF-8 sink, a row-replay sink, or a memory sink, without changing the
  placement rules.
- **Honest legacy sinks.** A sink that can only carry ASCII is labelled
  lossy, not conformant.

### 1.2 Non-Negotiable Invariants (on claim)

1. **Width comes from SGW.** No SCTS implementation may maintain a second
   width table. A caching layer is permitted only if it is provably derived
   from SGW and invalidated with it.
2. **Breaks come from SBR.** A wrap point is a break opportunity; the console
   may not choose one that SBR forbids.
3. **Clusters are atomic.** A grapheme cluster, as resolved by STS, occupies
   its cells as a unit. A combining mark is never silently discarded.
4. **No allocation.** The plane, the cell array, and the sink are supplied by
   the caller. SCTS allocates nothing.
5. **No SCP as content.** A System Control Plane value is a directive to the
   console, never a cell.
6. **No invented BANcodes.** Status values come from the registered
   `0x0011Axxx` allocation or from a documented local status space that never
   enters the SCP.
7. **Legacy is declared.** A sink that cannot carry the full SUTF-8 range is
   a *lossy* sink and MUST be documented as such.

---

## 2. Scope

SCTS defines:

- the plane, cell, cursor, and sink model;
- the placement algorithm for zero-, one-, and two-cell clusters;
- wrapping, scrolling, and cursor motion;
- the handling of newline, carriage return, tab, backspace, and SCP
  directives;
- the sink interface and its conformance levels;
- the glyph-zone registry for Private Use Area symbols;
- the status-code contract;
- the classification of legacy byte-oriented sinks.

SCTS does **not** define:

- how text is encoded on the wire, stored, or owned - that is SUTR-1 and
  SUAS-018;
- how a glyph is rasterized - that is the font engine (`owfont64`);
- how text is shaped, reordered, or mirrored - that is SUAS-001 (SDF) and
  the script-specific standards;
- which characters a font contains;
- how a window manager composites the surface.

---

## 3. Relationship to the Authoritative Standards

| Question | Authority | SCTS role |
|----------|-----------|-----------|
| How many cells does this codepoint take? | SUAS-002 (SGW) `suas_sgw_cells` | call it; cache the result per codepoint and per wide-context |
| Is there a break opportunity here? | SUAS-003 (SBR) `suas_sbr_pair` | pick a wrap point only among opportunities SBR allows |
| Where does this cluster end? | SUAS-006 (STS) | treat the cluster as one placement unit |
| Is this a valid identifier? | SUAS-007 (SIS) | not used for cell placement; used only for labelled rows such as a prompt |
| What does this codepoint mean in the property model? | SUTR-8 (SUCPM) | may consult for classification; must not fork its tables |
| What is the East Asian width category? | SUTR-013 (SUEAS) | SGW already resolves it; SCTS reads SGW's answer |
| What is this PUA glyph? | this document, section 7 | the registry is SCTS-owned |

Where two of these disagree, the higher-ranked document wins: SUAS is
mandatory, SUTS is optional, SUTR is informative. SCTS cannot override SGW,
and a SUTR report cannot override SCTS.

### 3.1 The wide-context flag

SGW resolves Ambiguous-width codepoints to one or two cells depending on a
`wide_context` flag. SCTS MUST make that flag a property of the plane, not a
per-call guess, and MUST apply it consistently for the whole write. A plane
that changes `wide_context` in the middle of a row produces a row whose
columns do not sum to the row width, and a replay of that row may land on
different cells than the original.

A conforming implementation SHOULD expose the flag at plane construction and
MUST reject a write that would require a different flag without an explicit
re-layout.

---

## 4. The Model

### 4.1 Cell

A cell is the smallest addressable grid position. It has:

- a **content**: either a codepoint, or a continuation marker, or a reserved
  blank marker;
- an **attribute set**: foreground, background, and emphasis bits.

```c
typedef struct {
    sucs_char_t cp;       /* codepoint, or a reserved marker */
    uint16_t    attrs;    /* attribute bits                  */
} scts_cell_t;
```

The marker codepoints MUST come from the registered reserved zone (section
7.3). A marker MUST NOT be a printable character that a font could also
legitimately receive.

### 4.2 Plane

A plane is a fixed rectangle of cells plus a cursor plus a style:

```c
typedef struct {
    scts_cell_t *cells;      /* rows * cols, caller-owned           */
    uint16_t     rows;       /* 1..65535                            */
    uint16_t     cols;       /* 1..65535                            */
    uint16_t     cursor_row;
    uint16_t     cursor_col;
    uint16_t     fg;
    uint16_t     bg;
    bool         wrap;       /* wrap at the right edge              */
    bool         wide_context;
    bool         initialized;
    uint64_t     writes;
    uint64_t     scrolls;
} scts_plane_t;
```

The plane does not own `cells`. It MUST NOT allocate, reallocate, or free
them. The caller guarantees `rows * cols` cells and guarantees the product
does not overflow. An implementation MUST check the product before using it.

**Row/column limits.** SCTS places no architectural minimum or maximum. The
80x25 VGA baseline is a deployment choice, not a standard requirement, and an
implementation MUST NOT assume it. A conforming implementation MUST accept any
geometry the caller supplies, up to the fixed-width limits of `uint16_t`, and
MUST reject geometry whose cell product overflows `size_t`.

### 4.3 Cursor

The cursor is a (row, column) pair naming the **next** cell to write. It is
not a byte offset and not a codepoint index. It advances by the grid width of
the cluster just placed, in cells.

`cursor_col == cols` is a legal transient state: it means "the row is full and
the next write wraps". It MUST NOT be a legal state for a cell write.

### 4.4 Style

Style is a plane-level default (foreground and background) plus per-write
overrides. An implementation MUST NOT store a style that is outside its
declared range; a 16-colour sink cannot represent a 256-colour index, and
must either quantize with a documented rule or reject the write.

### 4.5 Sink

A sink is a function plus an opaque token:

```c
typedef void (*scts_emit_fn)(uintptr_t token,
                             const uint8_t *sutf8,
                             uint32_t len);

typedef struct {
    scts_emit_fn emit;
    uintptr_t    token;
} scts_sink_t;
```

The sink receives SUTF-8 bytes that already form whole codepoints. It MUST
NOT be handed a partial sequence. It MUST NOT retain the buffer. It MUST NOT
call back into the plane. An implementation that cannot guarantee those three
properties MUST buffer the row before emitting it.

---

## 5. Placement

### 5.1 The placement unit is a cluster

SCTS places **clusters**, not codepoints. The cluster boundary is resolved by
STS (SUAS-006). Within a cluster:

- the **base** codepoint determines the cell span, via SGW;
- combining marks and other zero-width members attach to the base and consume
  no additional cells;
- the whole cluster is written into the cells the base occupies, or into the
  cluster attachment list the implementation uses internally, but it never
  straddles two rows.

A cluster whose base is zero-width is a malformed cluster. It MUST be
rejected with `SCTS_E_CLUSTER` rather than dropped. Dropping it is what the
current `suct` does (section 12.3) and it turns "a combining mark at the
start of a line" into silent data loss.

### 5.2 Zero, one, two

Let `w = SGW.cells(base, plane->wide_context)`.

- **w = 0.** Only legal for a non-base member. A base with `w = 0` is
  `SCTS_E_CLUSTER`.
- **w = 1.** Place the cluster at `(r, c)`; advance `c` by one.
- **w = 2.** The cluster needs two adjacent cells. If `c + 2 > cols`, the
  placement fails at this position and the wrap rule (section 6.1) applies.
  If the row has exactly one cell left and wrapping is disabled, the write is
  `SCTS_E_COL_RANGE`.
- **w > 2.** SGW does not produce values above two. An implementation that
  receives one has a corrupt width source and MUST fail closed.

### 5.3 Wide cluster cells

For `w = 2`, the lead cell holds the base codepoint and is marked as a wide
lead. The second cell is a **continuation cell**: it holds the continuation
marker and is marked as a wide tail. The pair is atomic:

- writing over either half MUST clear both;
- scrolling MUST move both halves together;
- serialization MUST emit the base once and skip the tail;
- a cursor MUST never stop between them.

A tail cell MUST NOT be interpreted as a space. It is a structural marker, not
content.

### 5.4 Overwrite

Writing at an occupied position replaces the content. If the position is a
wide tail, the implementation MUST clear the lead as well. If the position is
a wide lead, it MUST clear the tail. Failing to do so leaves an orphan half
that serializes as a stray marker.

---

## 6. Cursor, Wrap, and Scroll

### 6.1 Wrap

When a cluster does not fit at the current column:

1. If wrapping is disabled, fail with `SCTS_E_COL_RANGE` and leave the plane
   unchanged.
2. If wrapping is enabled, the console MUST move to the start of the next row
   and retry exactly once.
3. If the cluster still does not fit - possible only for a cluster wider than
   the plane - fail with `SCTS_E_CLUSTER`.

The wrap point is a **break opportunity**. The console MUST NOT wrap between
two codepoints that SBR marks `NO_BREAK`, and SHOULD prefer the last break
opportunity on the row when one exists. This is the rule that makes wrapping
differ from "move to column zero": a word that would be split is first moved
whole to the next row if SBR permits a break before it.

An explicit SCP break directive overrides the automatic choice (SUAS-003
section 6.2). A console that does not understand a directive MUST treat it as
`MUST_BREAK` at the directive's position only if the directive is registered
as mandatory; otherwise it MUST reject the stream rather than guess.

### 6.2 Scroll

When the cursor advances below the last row, the plane scrolls by one row:

- row `i` takes the contents of row `i + 1`, for `i` in `[0, rows-2)`;
- the last row is cleared;
- the cursor stays on the last row;
- the scroll counter increments by one.

A wide lead/tail pair is moved intact. A pair that would be split by the
row boundary MUST be replaced by two blanks with a `W+` warning, never left
half-moved.

The implementation MUST track dirty rows so that a replay sink can repaint
only the rows that changed. A full repaint is permitted but MUST be
distinguishable from an incremental one in diagnostics.

### 6.3 Control characters

The following are directives, not cells. They MUST NOT be stored in a cell.

| Codepoint | Meaning | Cursor effect |
|-----------|---------|---------------|
| `U+000A` LF | line feed | next row, column 0; scroll at the bottom |
| `U+000D` CR | carriage return | column 0 of the current row |
| `U+0009` HT | tab | advance to the next tab stop (section 6.4) |
| `U+0008` BS | backspace | column decreases by one if `> 0`; no erase |
| `U+0000` NUL | ignored | no effect |

CRLF is one line break, not two. An implementation MUST treat `CR` followed
immediately by `LF` as a single newline, and MUST NOT scroll twice.

Any other C0 or C1 control is `SCTS_E_CONTROL` unless the implementation
documents a device-specific meaning. Substituting a visible glyph for an
unhandled control is not permitted, because the substitution is
indistinguishable from the user having typed that glyph.

### 6.4 Tab stops

Tab stops are every 8 columns from column 0, by default. The interval is a
property of the plane, not a compile-time constant, and MUST be at least 1.
A tab that would advance past the last column wraps (when wrapping is
enabled) or fails with `SCTS_E_COL_RANGE`.

### 6.5 Erase and clear

Clear fills every cell with the blank marker and resets the cursor to
`(0,0)`. Erase-to-end-of-line and erase-to-end-of-screen are optional; if
provided, they MUST use the same blank marker and MUST clear both halves of
any wide pair they touch.

---

## 7. Sinks and Glyph Zones

### 7.1 Sink conformance levels

| Level | Name | Accepts | Loss |
|-------|------|---------|------|
| L0 | byte sink | `U+0020`-`U+007E` only | lossy by construction; MUST substitute and warn |
| L1 | SUTF-8 sink | any valid SUTF-8 | none |
| L2 | row-replay sink | whole rows of SUTF-8, with a dirty mask | none |
| L3 | surface sink | cells plus a font handle | none, but requires a rasterizer |

A sink declares its level. A console MUST NOT silently degrade a write to
match a lower-level sink without reporting it.

**L0 substitution.** A byte sink receives `'?'` (`U+003F`) for any codepoint
outside its range and the console raises a `W+` truncation warning. It MUST
NOT receive a truncated multi-byte sequence, a raw `uint8_t` cast of a
codepoint above `0xFF`, or a marker codepoint.

### 7.2 Ordering and atomicity

Rows are emitted in increasing row order. Within a row, cells are emitted in
increasing column order. A multi-byte codepoint is emitted in full or not at
all. A sink failure terminates the render; the plane keeps its state so the
caller can retry from the same dirty set.

### 7.3 Reserved markers

The continuation and blank markers MUST be codepoints in a zone that no font
maps to a visible glyph and that no text producer can emit as content. SCTS
reserves two slots inside the OpenWindows glyph zone:

- continuation of a wide glyph;
- blank cell.

An implementation that already reserves different slots MAY keep them, but
MUST register the choice in the glyph-zone table (section 7.4) and MUST NOT
reuse a slot for a second purpose.

### 7.4 Glyph-zone registry

Private Use Area allocation is a namespace and MUST be registered, not
invented at the call site. A zone record states: start, end, owner, purpose,
and whether reuse is permitted after retirement.

Rules:

- **Z1.** Zones are disjoint. Two owners MUST NOT claim overlapping ranges.
- **Z2.** A zone is allocated as a block and does not grow into a neighbour.
- **Z3.** A codepoint that has appeared in a published plane or a stored row
  MUST NOT be reassigned while any stored artifact may still reference it.
- **Z4.** Reserved markers are allocated before any glyph that a font may
  render, so that a marker is never mistaken for content.
- **Z5.** PUA codepoints are Unicode PUA, not RNUR neography. They do not
  acquire script, case, or identifier properties by being in a PUA block, and
  SIS will not accept them as identifier characters.
- **Z6.** Classification of a PUA codepoint for properties follows SUTR-8
  (SUCPM), not a local range check.

The current OpenWindows allocation (section 12.4) is an example of a
conforming *shape* of allocation, not of a conforming *registration*: the
zones are defined in a header with no registry behind them and no rule against
reuse.

### 7.5 Glyph identity

A glyph identifier and a codepoint are not the same thing. A sink that maps a
codepoint to a glyph MUST do so through the font's own table and MUST report
a missing glyph rather than drawing a blank, because a blank is
indistinguishable from a space.

---

## 8. Status Codes

SCTS distinguishes at least:

| Condition | Meaning |
|-----------|---------|
| `SCTS_OK` | success |
| `SCTS_E_NOT_INIT` | plane has not been constructed |
| `SCTS_E_NULL` | absent argument |
| `SCTS_E_ROW_RANGE` | row outside the plane |
| `SCTS_E_COL_RANGE` | column outside the plane, or no fit with wrapping off |
| `SCTS_E_CLUSTER` | cluster cannot be placed as a unit |
| `SCTS_E_CONTROL` | control directive not understood |
| `SCTS_E_ENCODING` | malformed SUTF-8 |
| `SCTS_E_INADMISSIBLE` | SCP, trap, Sentinel, or otherwise barred codepoint |
| `SCTS_E_CAPACITY` | caller buffer too small |
| `SCTS_E_NO_SINK` | no sink bound |
| `SCTS_E_SINK` | sink transport failure |

A condition that must be visible to the BANcode plane maps to the registered
codes in the `0x0011A000`-`0x0011AEFF` range, using the existing assignment.
SCTS MUST NOT allocate a new BANcode. Two consoles reporting the same fault
MUST report the same code.

The status type is a 32-bit unsigned integer. `SCTS_OK` is zero. Every
failure is non-zero and MUST be distinguishable by equality, not by range
guesswork.

---

## 9. Serialization

A row is serialized to SUTF-8 for transport or replay. The rules:

1. cells are visited in column order;
2. a wide lead emits its codepoint and the tail is skipped;
3. a continuation marker never reaches the output;
4. trailing blanks are trimmed by default, because a replay sink that receives
   80 spaces and a replay sink that receives an empty row paint the same
   result, and the empty row is smaller;
5. an interior blank emits a real space (`U+0020`), not a marker;
6. the output buffer's capacity is in bytes, and the operation fails rather
   than truncating mid-sequence.

Trimming is a transport decision. A caller that needs the full row MUST be
able to request it, and the request MUST be explicit in the API.

---

## 10. Conformance Requirements

An implementation that **claims** SCTS conformance MUST:

1. Use SGW for every cell-width decision, with no second width table.
2. Use SBR to choose wrap points and refuse to break where SBR forbids.
3. Use STS to delimit clusters and place a cluster atomically.
4. Reject rather than drop a cluster whose base is zero-width.
5. Maintain wide lead/tail pairs as atoms through write, erase, scroll, and
   serialize.
6. Treat LF, CR, CRLF, HT, BS, and NUL as directives, and reject unhandled
   controls rather than drawing them.
7. Declare a sink level and honour it; a lossy sink substitution MUST be
   reported, not silent.
8. Register every PUA zone in a glyph-zone table and obey Z1-Z6.
9. Use the registered BANcode allocation and mint nothing new.
10. Allocate nothing on any path, and compile as freestanding C99.
11. Expose the plane geometry as caller-supplied, with no hard-coded 80x25
    dependency in the placement logic.
12. Track dirty rows for incremental replay.

Because SCTS is optional, an implementation that does not provide a console
service is still SuperUnicode-conforming. It simply cannot claim SCTS.

---

## 11. Testing

A conforming implementation MUST ship tests that demonstrate:

1. **Width deference.** A codepoint whose SGW width differs from a legacy
   local table is placed by SGW, and a test proves the local table is not
   consulted.
2. **Combining attachment.** `a` + combining acute occupies one cell and the
   acute is not dropped; a combining mark at column 0 is rejected.
3. **Wide pairs.** A wide glyph at the last column wraps whole; a wide tail is
   never emitted by the serializer; overwriting a lead clears the tail.
4. **Wrap.** A word that fits on the next row is moved whole when SBR allows a
   break before it; a forbidden break is not taken.
5. **Scroll.** Scrolling preserves wide pairs and clears the last row; CRLF
   scrolls once, not twice.
6. **Tabs.** Tab stops advance by the configured interval and wrap or fail
   predictably at the edge.
7. **Sink levels.** An L0 sink receives `'?'` and a warning for a non-ASCII
   codepoint and never receives a marker or a partial sequence.
8. **Status.** Every failure condition has a distinct, stable code.
9. **Zero allocation.** The test build links no allocator.

The reference implementation in `src/suts/suts_scts.c` ships with
`tests/test_suts_scts.c`, which covers the nine areas above. The suite also
checks that direct codepoint placement applies the same admissibility rules as
byte input, and that a native codepoint serializes in its canonical five-byte
form rather than the four-byte Unicode bridge form. It is not an
SCTS-conformant console: `suct` remains the only console in the OpenWindows
tree, and its own tests validate `suct`, not this specification.

---

## 12. Implementation Evidence (Informative)

### 12.1 What `suct` already models

`DLL/suct/suct.h` defines a plane of `25 x 80` cells, each holding a
`sucs_char_t` and a 16-bit attribute word; a cursor; a wrap flag; write,
newline, scroll, and row-serialization operations; and a sink of the form
`void (*)(uintptr_t token, const uint8_t *sutf8, uint32_t len)`. That is the
same shape SCTS standardizes in sections 4 through 6 and 9.

### 12.2 What it gets right

- The sink signature is already "whole SUTF-8 units plus a token", which is
  the L1 contract.
- Wide glyphs are written as a lead plus a continuation cell marked with
  `SUCT_ATTR_WIDE_TAIL`.
- Serialization skips the tail and trims trailing blanks.
- Dirty rows are tracked as a bit per row.
- Status codes are distinct and carry BANcode values.

### 12.3 Where it does not conform

| Observation | Section | Why it matters |
|-------------|---------|----------------|
| `suct_cell_width` carries private `zero_width_ranges` and `wide_ranges` tables | 1.2, 10 | A second width authority; it will drift from SGW, and already differs on the Ambiguous class |
| `wide_ranges` includes `0x1F300`-`0x1F64F` as unconditionally wide | 5.2 | Emoji width is context- and font-dependent; SGW owns the answer |
| Combining marks are dropped in `plane_emit_cp` (`w == 0` returns OK) | 5.1 | Silent data loss |
| `SUCT_PLANE_ROWS`/`COLS` are compile-time constants | 4.2 | The placement logic cannot run on any other geometry |
| PUA zones are header constants with no registry | 7.4 | No rule against reuse; no owner recorded |
| No SBR integration at wrap time | 6.1 | A word may be split at a position SBR forbids |

These are recorded as current behaviour. They are not conformance claims.

### 12.4 The OpenWindows glyph allocation

`suct.h` assigns:

| Zone | Range | Slots | Purpose |
|------|-------|-------|---------|
| OWM cell glyphs | `0xEE00`-`0xEE7F` | 128 | window-manager cells |
| boot stage | `0xEF00`-`0xEF3F` | 64 | early boot glyphs |
| telemetry | `0xEF80`-`0xEF9F` | 32 | telemetry markers |
| continuation | `0xEE00` | 1 | wide tail marker |
| blank | `0xEE01` | 1 | blank cell marker |

The continuation and blank markers overlap the OWM cell-glyph block. That
violates the reserved-before-render rule (Z4) as soon as a font is asked to
draw glyph 0 or 1 of that block. It is exactly the kind of collision a
registry prevents.

### 12.5 The legacy sink

`Drivers/ttydrv/ttydrv.h` holds `uint8_t ch` per cell - one byte, ASCII only -
and `tty_write_string` takes a NUL-terminated `char *`. That is an L0 sink.
It may continue to exist, but it MUST be documented as lossy and MUST NOT be
presented as an SCTS-conformant console.

### 12.6 Not specified here

SCTS does not decide whether the console is a kernel object, a user-mode
service, or a driver. The OpenWindows tree currently places the plane in a
user-mode-facing library and the driver in a separate module; both are legal
deployments of this contract.

---

## 13. Open Questions (Informative)

- **Q1. Vertical layout.** SCTS is horizontal-only. Vertical text needs
  SUVTL (SUAS-014) and a separate plane contract; nothing here anticipates
  the column/row transposition.
- **Q2. BiDi.** A plane that mixes directions needs SDF (SUAS-001)
  reordering before placement. SCTS assumes the input stream is already in
  visual order or that the caller has applied SDF.
- **Q3. Wide-context change.** Section 3.1 requires re-layout, but does not
  define what happens to rows already emitted. A full repaint is the safe
  answer; whether it is the required answer is undecided.
- **Q4. Tab stops and alignment.** The 8-column default is a convention, not
  a standard. A future revision may make the interval part of the plane
  contract; it already is in this document, but the default is not justified
  anywhere.
- **Q5. Scrollback.** SCTS defines scroll as destructive. A console with
  scrollback needs a ring of rows outside the plane, and that ring is not
  specified.

---

## 14. Terms and Conventions

- **SCTS** is this specification, SUTS-012.
- **Plane** is the cell grid plus cursor and style.
- **Cluster** is a grapheme cluster as resolved by SUAS-006.
- **Cell** is one grid position.
- **Continuation cell** is the second cell of a two-cell glyph.
- **Sink** is the transport that receives rendered text.
- **PUA** is the Unicode Private Use Area.
- **RNUR** is the Native & Neography Plane carve-out (SUAS-005).
- The key words MUST, MUST NOT, SHOULD, SHOULD NOT, and MAY are used as in
  RFC 2119. Because SCTS is optional, a MUST binds only an implementation
  that claims SCTS conformance.
