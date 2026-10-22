# SUAS-006 — System Text Segmentation (STS)

**Standard ID:** SUAS-006
**Status:** Draft — Ratified
**Scope:** Core Kernel Invariant
**Applies To:** SUCD, SUAS-003 (SBR), SUAS-004 (SUCF), SUAS-005 (SSP), the kernel text compositor, the terminal shell, SUTS-002 (SRE), the System Control Plane
**Last Updated:** 2026-10-18

---

## 1. Overview

System Text Segmentation (STS) is the **core architecture governing text
segmentation** in SuperUnicode. It defines the three canonical segmentation
tiers — **grapheme cluster**, **word**, and **sentence** — over the full ExtSUCS
codepoint space (Base SUCS + SCP + Native SUCS + RNUR registry + ExtSUCS
plugins), and binds them to a single **single-pass, zero-allocation** state
machine.

STS is the structural successor to the Unicode text-segmentation algorithm
(UAX #29). Where UAX #29 describes boundaries through a set of regular
expressions with unbounded look-ahead and look-behind, STS provides a
**deterministic streaming contract**: the caller feeds one codepoint at a time
and receives, in O(1), the boundary status of the position just resolved. Every
piece of context that UAX #29 would recover by rescanning — the "ignore" runs,
the regional-indicator parity, the emoji ZWJ sequence, the terminator run — is
carried in a bounded register, so **no unbounded history is ever required and
no allocation ever occurs**.

STS is not an application feature. It is a kernel invariant: cursor movement,
text selection, double-click word picking, line wrapping, spell-check token
boundaries, and paragraph detection in the compositor and terminal shell all
depend on the fact that grapheme, word, and sentence boundaries are resolved
identically, everywhere, in constant time.

### 1.1 Architectural shifts from UAX #29

| Dimension | UAX #29 | SUAS-006 STS |
|-----------|---------|--------------|
| Model | Regular-expression rule set with look-ahead/look-behind | Single-pass deterministic state machine |
| Evaluation | Multi-scan over a materialized string | One codepoint in, one boundary out, O(1) |
| State | Implicit, in the rule engine | Explicit, in `suas_sts_state_t` (sliding window + carried register) |
| Memory | Implementation-defined, may allocate | Zero allocation; fixed-size stack struct |
| Codepoint space | 21-bit Unicode scalar values | Full ExtSUCS (`sucs_ex_char_t` 64-bit fast path) |
| Control | None (out-of-band tailoring only) | In-band SCP segmentation directives |
| Bridge data | `GraphemeBreakProperty.txt` and friends | Curated SUCD-sourced class tables |
| Tiers | Grapheme, word, sentence | Grapheme, word, sentence (identical three) |
| Out-of-range | Undefined | Native/plugin planes resolve to an opaque "other" class |

The three tiers are **not** three separate algorithms. They are three
projections of one engine over one codepoint stream; switching tiers changes the
class predicate and the rule table, never the streaming contract.

### 1.2 Goals

- **Determinism.** The same codepoint stream, in the same tier, always yields
  the same boundary sequence, independent of platform, locale, or threading.
- **Constant time.** Each `suas_sts_evaluate` call performs a bounded number of
  comparisons regardless of the position in the stream.
- **Zero allocation.** The entire engine state is a fixed-size struct the caller
  owns on the stack or in a task control block.
- **Freestanding.** No C library, no floating point, no heap; C99 freestanding
  subset only.
- **Total.** Every ExtSUCS codepoint — including trap markers, BANcodes, and
  unregistered Native codepoints — produces a defined boundary.

### 1.3 Non-negotiable invariants

1. **Streaming faithfulness.** A boundary yielded by the streaming engine is
   identical to the boundary in a fully materialized string evaluated at the
   same offset.
2. **Bounded context.** The engine shall not retain more than
   `SUAS_STS_WINDOW_SIZE` codepoints plus a fixed-size carried register.
3. **Order independence of setup.** `suas_sts_init` followed by the same stream
   always yields the same result, regardless of prior use of the struct.
4. **Directive transparency.** An SCP segmentation directive never enters the
   codepoint window and never mutates the carried context.
5. **Start/end totality.** Start of text and end of text are always
   `SUAS_STS_BOUNDARY_MUST`.

---

## 2. Scope

STS defines:

- the three segmentation tiers and their boundary-status domain;
- the class value domains for each tier (the `Grapheme_Cluster_Break`,
  `Word_Break`, and `Sentence_Break` analogs);
- the boundary rules for each tier, in normative order;
- the SCP in-band segmentation directives;
- the streaming contract of `suas_sts_evaluate` and `suas_sts_flush`;
- the Hangul syllable boundary determination;
- the conformance testing model.

STS does **not** define:

- line-breaking opportunities — that is SUAS-003 (SBR);
- canonical equivalence or normalization — that is SUAS-004 (SUCF);
- glyph width or cell metrics — that is SUAS-002 (SGW);
- script identification — that is SUAS-005 (SSP);
- the mapping from a binary encoding to a codepoint stream — that is SUTF /
  SUST.

The relationship to SBR is deliberate: STS resolves the *word and grapheme
units*, while SBR resolves the *line-break opportunities between them*. A
compositor runs STS to identify units and SBR to lay them out.

---

## 3. Boundaries and the Three Tiers

### 3.1 The three tiers

STS defines exactly three tiers, selected at initialization:

| Tier | Constant | Unit | Primary consumers |
|------|----------|------|-------------------|
| Grapheme | `SUAS_STS_LEVEL_GRAPHEME` | User-perceived character | Cursor movement, backspace, selection extent |
| Word | `SUAS_STS_LEVEL_WORD` | Word / token | Double-click selection, spell-check, search |
| Sentence | `SUAS_STS_LEVEL_SENTENCE` | Sentence / paragraph | Paragraph parsing, TTS, diff granularity |

### 3.2 Boundary status

Every offset is classified into exactly one status:

| Status | Value | Meaning |
|--------|-------|---------|
| `SUAS_STS_BOUNDARY_NONE` | 0 | Do not break here; the two sides form one unit |
| `SUAS_STS_BOUNDARY_ALLOW` | 1 | A unit boundary exists here (opportunity) |
| `SUAS_STS_BOUNDARY_MUST` | 2 | A hard boundary is required here |

`MUST` is reserved for boundaries that cannot be overridden by tailoring:
start/end of text, the control and newline boundaries, paragraph separators,
and the `SCP_STS_BREAK_FORCE` directive. `ALLOW` denotes a genuine unit edge
(grapheme edge, word opportunity, sentence terminal). `NONE` denotes a
prohibited break.

> A consumer that only needs grapheme extents treats both `ALLOW` and `MUST` as
> "break and advance"; a consumer that distinguishes hard from soft breaks (for
> example, a wrapping compositor) treats them differently.

### 3.3 SCP segmentation directives

The System Control Plane block **`0x00110040`–`0x0011004F`** is reserved for
explicit in-band segmentation control. It is disjoint from the SBR break markers
(`0x00110020`–`0x00110022`), the math directive block (`0x00110030`–`0x0011003F`),
the SDF directional block (`0x00110100`), the SSP script directives
(`0x00112000`–`0x00119FFF`), and the BANcodes (`0x0011A000`+).

| Codepoint | Macro | Effect |
|-----------|-------|--------|
| `0x00110040` | `SCP_STS_BREAK_FORCE` | Force a boundary at this offset (`MUST`) |
| `0x00110041` | `SCP_STS_BREAK_BLOCK` | Prohibit a boundary at this offset (`NONE`) |
| `0x00110042` | `SCP_STS_BREAK_ALLOW` | Permit a boundary at this offset (`ALLOW`) |

Directives are **transparent**: they are recognised by `suas_sts_evaluate`,
return their imposed status immediately, and are neither pushed onto the window
nor allowed to mutate the carried context. They therefore do not perturb the
segmentation of the surrounding text. A directive takes precedence over every
rule in §4–§6.

---

## 4. Grapheme Cluster Boundaries

A **grapheme cluster** is the smallest unit a user perceives as a single
character: a base plus its combining marks, a Hangul syllable, an emoji ZWJ
sequence, a flag, or a Devanagari conjunct.

### 4.1 Property values

`suas_sts_classify_grapheme` maps each codepoint to exactly one
`suas_sts_gcb_t` value:

| Class | Notes |
|-------|-------|
| `SUAS_STS_GCB_OTHER` | Default; includes emoji pictographs and unlisted letters |
| `SUAS_STS_GCB_CR` | U+000D |
| `SUAS_STS_GCB_LF` | U+000A |
| `SUAS_STS_GCB_CONTROL` | C0/C1 controls, format and separators (not CR/LF/ZWNJ/ZWJ) |
| `SUAS_STS_GCB_EXTEND` | Grapheme_Extend and emoji modifiers (not ZWJ) |
| `SUAS_STS_GCB_ZWJ` | U+200D |
| `SUAS_STS_GCB_RI` | Regional indicators U+1F1E6–U+1F1FF |
| `SUAS_STS_GCB_PREPEND` | Arabic number signs and other prepend marks |
| `SUAS_STS_GCB_SPACINGMARK` | Indic spacing vowel signs |
| `SUAS_STS_GCB_L` | Hangul leading jamo |
| `SUAS_STS_GCB_V` | Hangul vowel jamo |
| `SUAS_STS_GCB_T` | Hangul trailing jamo |
| `SUAS_STS_GCB_LV` | Hangul LV syllable |
| `SUAS_STS_GCB_LVT` | Hangul LVT syllable |

The L/V/T/LV/LVT values are resolved algorithmically (§9), not from a table.

### 4.2 Rules

Rules are applied in order; the first match decides the boundary. `÷` denotes
`ALLOW` (or `MUST` where noted), `×` denotes `NONE`.

| Rule | Pattern | Result |
|------|---------|--------|
| GB1 | `sot ÷` | `MUST` |
| GB2 | `÷ eot` | `MUST` |
| GB3 | `CR × LF` | `NONE` |
| GB4 | `(Control \| CR \| LF) ÷` | `MUST` |
| GB5 | `÷ (Control \| CR \| LF)` | `MUST` |
| GB6 | `L × (L \| V \| LV \| LVT)` | `NONE` |
| GB7 | `(LV \| V) × (V \| T)` | `NONE` |
| GB8 | `(LVT \| T) × T` | `NONE` |
| GB9 | `× (Extend \| ZWJ)` | `NONE` |
| GB9a | `× SpacingMark` | `NONE` |
| GB9b | `Prepend ×` | `NONE` |
| GB9c | `Linker Extend* × Consonant` | `NONE` |
| GB11 | `ExtPict Extend* ZWJ × ExtPict` | `NONE` |
| GB12/13 | `RI RI` pair parity | `NONE` on the first of a pair |
| GB999 | otherwise | `ALLOW` |

GB12/13 is encoded as a one-bit parity register: a regional indicator joins the
previous one only when the run length so far is odd. The extended-context rules
GB9c and GB11 are encoded as the `gcb_linker` and `gcb_emoji` registers, so the
`Extend*` runs between the pivot codepoints never need to be re-scanned.

---

## 5. Word Boundaries

A **word boundary** separates tokens for selection, search, and
spell-checking. It is the analog of the UAX #29 word-break rules.

### 5.1 Property values

`suas_sts_classify_word` maps each codepoint to exactly one `suas_sts_wb_t`:

| Class | Notes |
|-------|-------|
| `SUAS_STS_WB_OTHER` | Default (ideographs, symbols, unlisted) |
| `SUAS_STS_WB_CR` / `SUAS_STS_WB_LF` / `SUAS_STS_WB_NEWLINE` | Line/paragraph terminators |
| `SUAS_STS_WB_EXTEND` | Grapheme_Extend, SpacingMark, emoji modifiers |
| `SUAS_STS_WB_ZWJ` | U+200D |
| `SUAS_STS_WB_FORMAT` | Format controls (not ZWNJ/ZWJ) |
| `SUAS_STS_WB_RI` | Regional indicators |
| `SUAS_STS_WB_KATAKANA` | Katakana (and Katakana-like) |
| `SUAS_STS_WB_HEBREW` | Hebrew_Letter |
| `SUAS_STS_WB_ALETTER` | Alphabetic letters |
| `SUAS_STS_WB_SINGLE_QUOTE` | U+0027 |
| `SUAS_STS_WB_DOUBLE_QUOTE` | U+0022 |
| `SUAS_STS_WB_MIDNUMLET` / `MIDLETTER` / `MIDNUM` | In-word punctuation |
| `SUAS_STS_WB_NUMERIC` | Decimal digits |
| `SUAS_STS_WB_EXTENDNUMLET` | Connector punctuation |
| `SUAS_STS_WB_WSEGSPACE` | Space separators |

### 5.2 Rules

`Extend`, `Format`, and `ZWJ` are **ignored** (WB4): their boundary follows the
rule for the preceding significant codepoint, except after start of text, CR,
LF, or Newline, where they break. The engine records the last *effective*
class, so the ignore run is O(1).

| Rule | Pattern | Result |
|------|---------|--------|
| WB1 | `sot ÷` | `MUST` |
| WB2 | `÷ eot` | `MUST` |
| WB3 | `CR × LF` | `NONE` |
| WB3a | `(Newline \| CR \| LF) ÷` | `MUST` |
| WB3b | `÷ (Newline \| CR \| LF)` | `MUST` |
| WB3c | `ZWJ × ExtPict` | `NONE` |
| WB3d | `WSegSpace × WSegSpace` | `NONE` |
| WB4 | `X (Extend \| Format \| ZWJ)* → X` | ignore |
| WB5 | `AHLetter × AHLetter` | `NONE` |
| WB6/7/11/12 | `(AHLetter \| Numeric) (Mid…) (AHLetter \| Numeric)` | `NONE` |
| WB7a | `Hebrew × Single_Quote` | `NONE` |
| WB7b/c | `Hebrew × Double_Quote Hebrew` | `NONE` |
| WB8 | `Numeric × Numeric` | `NONE` |
| WB9 | `AHLetter × Numeric` | `NONE` |
| WB10 | `Numeric × AHLetter` | `NONE` |
| WB13 | `Katakana × Katakana` | `NONE` |
| WB13a | `(AHLetter \| Numeric \| Katakana \| ExtendNumLet) × ExtendNumLet` | `NONE` |
| WB13b | `ExtendNumLet × (AHLetter \| Numeric \| Katakana)` | `NONE` |
| WB15/16 | `RI RI` pair parity | `NONE` on the first of a pair |
| WB999 | otherwise | `ALLOW` |

> **Curation note.** WB6/WB7/WB11/WB12 require look-ahead (the punctuation must
> be *followed* by the compatible class). In a single-pass engine without
> retraction, STS resolves these **conservatively**: a `MidLetter`, `MidNum`, or
> `MidNumLet` flanked by word characters never breaks. This joins `3.4`,
> `can't`, and `U.S.` correctly; a mis-flanked `3.x` is likewise joined. Strict
> look-ahead tailoring may refine this without changing the streaming contract.

---

## 6. Sentence Boundaries

A **sentence boundary** separates sentences and paragraphs. It is the analog of
the UAX #29 sentence-break rules.

### 6.1 Property values

`suas_sts_classify_sentence` maps each codepoint to exactly one `suas_sts_sb_t`:

| Class | Notes |
|-------|-------|
| `SUAS_STS_SB_OTHER` | Default |
| `SUAS_STS_SB_CR` / `SUAS_STS_SB_LF` | Line terminators |
| `SUAS_STS_SB_EXTEND` | Grapheme_Extend, SpacingMark, ZWJ |
| `SUAS_STS_SB_SEP` | Paragraph separators (U+0085, U+2028, U+2029) |
| `SUAS_STS_SB_FORMAT` | Format controls (not ZWNJ/ZWJ) |
| `SUAS_STS_SB_SP` | Whitespace (not Sep/CR/LF) |
| `SUAS_STS_SB_LOWER` / `UPPER` / `OLETTER` | Cased and other letters |
| `SUAS_STS_SB_NUMERIC` | Decimal digits |
| `SUAS_STS_SB_ATERM` | Ambiguous terminator (`.`, fullwidth) |
| `SUAS_STS_SB_STERM` | Strong terminator (`!`, `?`, `。`, …) |
| `SUAS_STS_SB_SCONTINUE` | Continue punctuation (`,`, `-`, `:`, …) |
| `SUAS_STS_SB_CLOSE` | Closing punctuation |

### 6.2 Rules

`Extend` and `Format` are ignored (SB5), as in WB4. The terminator run state is
carried in `sb_term` ∈ {none, after-term, after-close, after-space}, which
encodes the `SATerm Close* Sp* ParaSep?` pattern of UAX #29 without rescanning.

| Rule | Pattern | Result |
|------|---------|--------|
| SB1 | `sot ÷` | `MUST` |
| SB2 | `÷ eot` | `MUST` |
| SB3 | `CR × LF` | `NONE` |
| SB4 | `ParaSep ÷` | `MUST` |
| SB5 | `X (Extend \| Format)* → X` | ignore |
| SB6 | `ATerm × Numeric` | `NONE` |
| SB7 | `(Upper \| Lower) ATerm × Upper` | `NONE` |
| SB8 | `ATerm Close* Sp* (¬(…))* × Lower` | `NONE` |
| SB8a | `SATerm Close* Sp* × (SContinue \| SATerm)` | `NONE` |
| SB9 | `SATerm Close* × (Close \| Sp \| ParaSep)` | `NONE` |
| SB10 | `SATerm Close* Sp* × (Sp \| ParaSep)` | `NONE` |
| SB11 | `SATerm Close* Sp* ParaSep? ÷` | `ALLOW` |
| SB998 | otherwise | `NONE` |

SB11 is the only rule that produces a sentence boundary; it fires when a
terminator run is followed by a codepoint that is neither a continuation, a
close, a space, nor a lower-case letter — that is, when the ambiguous
lower-case continuation of SB8 can no longer apply.

> **Curation note.** SB8's `(¬(OLetter|Upper|Lower|ParaSep|SATerm))*` scan is
> carried as the `sb_aterm` register: a terminator run that began with an
> ambiguous `.` remains "pending" across neutral codepoints and is only resolved
> when a lower-case letter (SB8, no break) or an excluded class (SB11, break)
> arrives.

---

## 7. Rule Constraints and Notation

The notation in §4–§6 follows UAX #29 and denotes **positions between
codepoints**, never codepoints themselves:

- `÷` marks a position where a boundary exists; in STS it maps to
  `SUAS_STS_BOUNDARY_ALLOW`, except for the hard boundaries (GB1/2, GB4/5,
  WB1/2, WB3a/3b, SB1/2, SB4) which map to `SUAS_STS_BOUNDARY_MUST`.
- `×` marks a position where no boundary exists; it maps to
  `SUAS_STS_BOUNDARY_NONE`.
- Class names (`CR`, `Extend`, `ATerm`, …) refer to the tier's class domain,
  not to literal codepoints.
- `X*` denotes zero or more codepoints of class `X`. In STS these runs are
  collapsed into the carried register, never enumerated.
- `(A | B)` denotes class union; `sot` and `eot` denote start and end of text.

A rule may only be violated by an explicit SCP directive (§3.3). Rule order is
normative: a later rule is never consulted once an earlier rule matches.

---

## 8. Implementation Notes

### 8.1 The streaming contract

```c
suas_sts_state_t st;
suas_sts_init(&st, SUAS_STS_LEVEL_WORD);
for (i = 0; i < n; i++) {
    suas_sts_boundary_t b = suas_sts_evaluate(&st, text[i]);
    /* b is the boundary BEFORE text[i] */
}
suas_sts_boundary_t end = suas_sts_flush(&st);   /* MUST */
```

`suas_sts_evaluate` returns the boundary at the offset **between the previously
consumed codepoint and `cp`**. The first codepoint returns
`SUAS_STS_BOUNDARY_MUST` (start of text). `suas_sts_flush` returns the
end-of-text boundary (always `MUST`) and does not clear the state.

### 8.2 The state

`suas_sts_state_t` is a fixed-size, caller-owned struct containing:

- a `SUAS_STS_WINDOW_SIZE` (4) codepoint ring buffer, for introspection and
  bounded look-back;
- the level;
- the **carried register**: grapheme parity/linker/emoji bits, the effective
  word class and mid-punctuation context, and the sentence terminator-run state.

The ring buffer is a convenience for callers that wish to inspect the recent
units; the boundary decision itself depends only on the carried register, which
is why the engine remains correct for arbitrarily long ignore-runs and RI runs.

### 8.3 Random access and rewind

For a random-access consumer, a fresh `suas_sts_init` plus a forward scan from
a known synchronization point reconstructs the state. Because the carried
register is bounded, a synchronization point is reached as soon as a class that
resets every register (any `OTHER` codepoint for grapheme, any `ALETTER`/`OTHER`
for word, any `SP`/`OLETTER` for sentence) is observed.

### 8.4 Native and plugin planes

Codepoints above the Unicode Bridge (`> 0x0010FFFF`) classify as the tier's
default (`OTHER`). They therefore form opaque units: a grapheme edge at every
offset, a word/selection opportunity at every offset, and no sentence
terminator. This guarantees total, defined behaviour for trap markers, BANcodes,
and unregistered Native codepoints without any registry consultation.

### 8.5 Performance

Each call performs a constant number of integer comparisons and one bounded
class lookup; there are no loops proportional to the codepoint value, the stream
length, or the run length. The engine is suitable for interrupt context and for
the hot path of a terminal line editor.

---

## 9. Hangul Syllable Boundary Determination

Hangul syllable boundaries are determined **algorithmically** from the codepoint
value, not from a property table. This is the one place where classification is
computed rather than looked up.

Given a codepoint `c`:

1. If `U+1100 ≤ c ≤ U+115F` or `U+A960 ≤ c ≤ U+A97C`, the class is **L**.
2. If `U+1160 ≤ c ≤ U+11A7` or `U+D7B0 ≤ c ≤ U+D7C6`, the class is **V**.
3. If `U+11A8 ≤ c ≤ U+11FF` or `U+D7CB ≤ c ≤ U+D7FB`, the class is **T**.
4. If `U+AC00 ≤ c ≤ U+D7A3`, compute `S = c − U+AC00`; the class is **LV** when
   `S mod 28 = 0`, otherwise **LVT**.
5. Otherwise the codepoint is not Hangul.

Rules GB6–GB8 then combine the jamo into exactly one cluster per syllable. This
is equivalent to the Hangul syllable boundary determination of UAX #29 §9, but
expressed as a constant-time branch rather than a table walk.

---

## 10. Testing

Conformance is demonstrated by a host test that exercises each tier over
representative streams and asserts the exact boundary sequence. The reference
suite covers, at minimum:

- **Grapheme:** base + combining mark (`a◌̈`); `CR LF` versus `CR` + letter;
  two ordinary letters; Hangul `L V T`; emoji ZWJ (`ExtPict ZWJ ExtPict`);
  regional-indicator pairing; a Devanagari conjunct.
- **Word:** letter runs; space separation; letter↔digit joins; a decimal point
  inside a number; an apostrophe inside a contraction; mandatory newline
  breaks; regional-indicator pairing.
- **Sentence:** `Hi. Bye` (boundary before the capital); `Hi. bye` (no
  boundary); `Hi! Bye` (strong terminator); `3.4` (no boundary);
  paragraph-separator hard breaks.
- **Directives:** `SCP_STS_BREAK_FORCE`, `SCP_STS_BREAK_BLOCK`,
  `SCP_STS_BREAK_ALLOW` precedence over the rules.
- **Edges:** start/end of text; NULL and empty guards; Native-codepoint
  opacity.

A conforming implementation need not use the same internal representation, but
it shall produce identical boundary sequences for identical input streams and
tiers.

---

## 11. Recommendations for Tailoring

STS is designed to be tailored **out of band**: an application that needs a
different word notion (for example, treating ideographs as words, or
suppressing the `Hi. bye` continuation) should not fork the engine. Instead:

- use the SCP directives (§3.3) to force, block, or allow individual offsets;
- layer a higher-level dictionary or script-specific rule over the STS units;
- reserve tailoring to the word tier, where the definition is most
  application-dependent.

The grapheme and sentence tiers should be treated as stable, since their
consumers (cursor movement, paragraph parsing) depend on cross-application
consistency.

---

## 12. Stability and Versioning

- The tier set, the boundary-status domain, and the streaming contract are
  **stable**: they will not change within a major version.
- The class value domains are **stable**; new values may only be added, never
  reordered.
- The curated class tables track the Unicode Bridge; a bridge update is a
  **minor** revision and does not change the API.
- The SCP directive block `0x00110040`–`0x0011004F` is **frozen**; sub-allocations
  within it require a new minor revision.

Current version: **1.0.0** (`SUAS_STS_SPEC_ID` = `"SUAS-006"`).

---

## 13. Compliance Matrix

| Requirement | Reference | Test |
|-------------|-----------|------|
| Three tiers selectable at init | §3.1 | `test_version_and_meta` |
| Boundary status domain | §3.2 | `test_grapheme`, `test_word`, `test_sentence` |
| SCP directives override rules | §3.3 | `test_directives_and_edges` |
| Grapheme rules GB1–GB999 | §4.2 | `test_grapheme` |
| Word rules WB1–WB999 | §5.2 | `test_word` |
| Sentence rules SB1–SB998 | §6.2 | `test_sentence` |
| Rule order is normative | §7 | all tier tests |
| Streaming contract | §8.1 | `feed` in all tier tests |
| Bounded state / zero allocation | §8.2 | struct is fixed-size |
| Native codepoint totality | §8.4 | `test_classification`, `test_directives_and_edges` |
| Hangul determination | §9 | `test_classification`, `test_grapheme` |
| Start/end totality | §1.3 | `test_grapheme`, `test_directives_and_edges` |

---

## 14. Terms and Conventions

- **Grapheme cluster** — a user-perceived character; the smallest selection
  unit.
- **Word boundary** — a boundary suitable for selection, search, and
  tokenization.
- **Sentence boundary** — a boundary at or after a sentence terminator run.
- **Carried register** — the fixed-size context in `suas_sts_state_t` that
  encodes everything UAX #29 would otherwise rescan.
- **Ignore rule** — WB4/SB5: `Extend`/`Format`/`ZWJ` inherit the boundary of
  the preceding significant codepoint.
- **Directive** — an SCP codepoint in `0x00110040`–`0x0011004F` that overrides a
  boundary decision.
- **Bridge** — the Unicode-compatible codepoint range `0x000000`–`0x0010FFFF`
  served by SUCD tables.
- **Native / plugin plane** — codepoints above the Bridge; opaque to STS.

**END OF SUAS-006**


