# SUTS-009 — SuperUnicode Link Detection and Formatting (SULDF)

**Standard ID:** SUTS-009
**Status:** Draft — Authoring
**Scope:** Modular Technical Specification (link attribute query, UTS #58 initiation and termination, `Link_Term` / `Link_Bracket` / `Link_Email` compiled tables, email detection and minimal quoting, minimal Part escaping, security screening)
**Applies To:** Base SUCS (Unicode Compatibility Bridge, SCP, Native), the RNUR neography registry, SUCD property tables, the OpenWindows microkernel, SUTS-005 (SCSS), SUTS-006 (SIDN), SUTS-008 (SUSCH), the character token stream
**Last Updated:** 2026-10-22

---

## 1. Introduction

Plain text carries links. A chat message, a log line, a mail body, or a
terminal scrollback buffer is a sequence of characters in which some
substring — `example.com`, `https://例子.测试/路径`, `a.b@例子.com` — is meant
to be read as a *link*, while the text around it is not. A linkifier's job is
to find those substrings and to say where they begin and end.

It is harder than it looks. Where does `example.com/a!` end? Is the trailing
`!` part of the link or sentence punctuation? Does `example.com/a(b)c` keep
its parenthesized tail while `example.com/a)` drops it? Is `foo..bar.com`
a domain? Is `http://user@host/` a URL? These are exactly the questions
UTS #58, *Unicode Link Detection and Formatting*, answers, and a
compatibility-grade linkifier has to answer them the same way.

UTS #58 describes the answers with a text scanner: a candidate finder over a
string, a set of compiled Unicode properties (`Link_Term`, `Link_Bracket`,
`Link_Email`), and a termination algorithm that walks a candidate and decides
where it really stops.

SULDF compiles the same evidence into immutable binary tables. Where UTS #58
loads its property files at run time and interprets them with a regular
expression, SULDF runs a bounded binary search over a static range table and
answers in O(1) per codepoint. Where UTS #58 walks a candidate as text, SULDF
walks a *codepoint stream* — the sequence a terminal emulator, chat client,
or mail reader already holds — in a single linear pass.

### 1.1 Relationship to UTS #58

SULDF is deliberately **not** a UTS #58 implementation, because a user-space
linkifier and a kernel-hosted scanning primitive differ in four structural
ways:

1. **String scanner vs. codepoint-stream scan.** UTS #58 is written against
   the text of a document and its surrounding markup. SULDF is handed the
   codepoint sequence. It never re-lexes, never inspects a markup context,
   and never needs a character-to-byte index.
2. **Runtime property files vs. compiled tables.** UTS #58 reads
   `LinkTerm.txt`, `LinkBracket.txt`, and `LinkEmail.txt` and interprets them
   through a regex or a candidate finder. SULDF embeds the merged, sorted,
   non-overlapping ranges of those files directly in the image and probes them
   with a binary search.
3. **Heap allocation vs. zero allocation.** A user-space linkifier allocates
   candidate buffers, match records, and a report tree. SULDF allocates
   nothing at all: every table is `static const`, every descriptor is a
   fixed-size struct the caller owns, and the scanner holds one fixed
   125-entry open-bracket stack on its own frame. No path reaches for
   `malloc`, `stdio`, or a libc string routine.
4. **Modular optional compliance.** Core operating system text primitives,
   file paths, and kernel IPC channels need strict, predictable link
   boundaries — not a hyperlink renderer. A minimal kernel can omit SULDF
   outright; a terminal emulator or chat client opts in to get it (§1.6).

UTS #58 remains the model. Where this document shares vocabulary with it —
initiation, termination, `Link_Term`, `Link_Bracket`, `Link_Email`, the
Path/Query/Fragment parts, minimal escaping — the requirements are normative
for SULDF only, and they are re-expressed in terms of compiled tables.

### 1.2 Key architectural shifts

| Dimension | UTS #58 (user-space) | SUTS-009 SULDF (kernel) |
|-----------|----------------------|-------------------------|
| Unit of work | A document, scanned as text | A codepoint stream, scanned in place |
| Link properties | Property files loaded into memory | Compiled range tables, probed by binary search |
| Initiation | A candidate finder over a regex | A bounded host/domain parser over codepoints |
| Termination | A textual algorithm with backtracking | The same algorithm with compiled classes, one pass |
| Detection | A report of match offsets | A verdict: a `kind` and a half-open codepoint span |
| Escaping | Regex substitution over a string | A Part walk with a compiled Terminator map |
| Allocation | Per-candidate and per-match buffers | None, on any path |
| Complexity | O(n) in text with per-document setup | O(1) per codepoint, O(n) in stream length, no setup |
| Dependence | A text-scanning toolchain and a properties library | A kernel primitive, no user-space service |
| Compliance | A single conformance statement | Modular optional: omittable outright (§1.6) |

### 1.3 The dual-layer architecture

SULDF is a **dual-layer** system, in the same relationship SUAS-009 (SIP),
SUAS-012 (SUCD), and SUTS-005 (SCSS) have with their Unicode counterparts:

* **Offline reference layer — SUCD.** The human-readable `LinkTerm.txt`,
  `LinkBracket.txt`, and `LinkEmail.txt` inventories live in the SuperUnicode
  Character Database for user-space linkifiers, validators, formatters, and
  developer toolchains. SULDF pins its reference data to **Unicode 18.0.0**.
* **Runtime kernel layer — SULDF.** Build-time tooling compiles those datasets
  into immutable, byte-packed binary lookup tables embedded directly into
  kernel and toolchain images. The kernel never reads a file at run time.

> The seeding sketch for this specification named its offline reference layer
> after the Unicode linkification files and preserved the house prefix
> `SULDF_*`, the entry points `suts_suldf_init_engine`,
> `suts_suldf_get_attributes`, and `suts_suldf_scan_stream`, the four
> sketch flags `SULDF_FLAG_SCHEME_CHAR`, `SULDF_FLAG_DOMAIN_CHAR`,
> `SULDF_FLAG_PATH_CHAR`, and `SULDF_FLAG_EMAIL_DELIM`, and the 12-byte
> attribute layout (`codepoint`, `link_mask`, `term_class`,
> `bracket_class`, `reserved`). All are preserved verbatim, so a consumer
> written against the sketch compiles unchanged. SULDF names `SUCD`
> (SUAS-012) as its reference layer, matching SUAS-009 (SIP), SUAS-010 (SSRE),
> SUAS-011 (SCMS), SUAS-012 (SUCD), SUAS-013 (SISR), SUTS-005 (SCSS),
> SUTS-006 (SIDN), SUTS-007 (SUEPS), and SUTS-008 (SUSCH).

### 1.4 Terminology and cross-references

SULDF reuses the SuperUnicode architecture documents, numbered as they exist
in this repository:

| SULDF concept | Authoritative document |
|---------------|------------------------|
| Codepoint space, SCP, traps, Sentinel | **SUTR-0 (SUCS Core)** — three-space layout |
| The "No Surrogates" rule, atomic scalars, SUTF-8 | **SUTR-1 (SUTF)** — invariants 1 and 2 |
| Bridge / Native / ExtSUCS planes | **SUTR-7 (SUCEM)**, **SUTR-6 (Bridge)** |
| RNUR neography registry, property tiers | **SUTR-8 (SUCPM)** — the character property model |
| Conformance model and zero-allocation mandate | **SUTR-10 (SUCM)** |
| Character security, homographs | **SUTS-005 (SCSS)** — UTS #39 analog |
| Internationalized domain labels, punycode | **SUTS-006 (SIDN)** — UTS #46 analog |
| Source code safety, identifier tokens | **SUTS-008 (SUSCH)** — UTS #55 analog |
| Offline reference data | **SUCD** — the SuperUnicode Character Database (SUAS-012) |

> **Relationship to SUTS-006 (SIDN).** SIDN owns internationalized *domain*
> semantics: label mapping, UTS #46 validation, punycode, and canonical
> routing. SULDF owns link *boundaries*: it uses the `Link_Email` property to
> decide which codepoints may form a domain label and stops there. A
> fully-normalized, registrable host name is a SIDN question; whether
> `例え.テスト` is a link at all is a SULDF question. A toolchain that needs
> both links SULDF for the span and SIDN for the canonical form.

> **Relationship to SUTS-005 (SCSS).** SCSS answers *"is this character
> dangerous for a reader?"* at the level of a single codepoint. SULDF answers
> *"is this substring a link, and where does it stop?"*. SULDF exposes the
> directional-control predicate that a link renderer needs (§8) and leaves the
> full confusable and script-mixing analysis to SCSS.

> The reference implementation ships as `include/suts/suts_suldf.h` +
> `src/suts/suts_suldf.c`. The library and function names (`suts_suldf.h`,
> `suts_suldf.c`, `suts_suldf_get_attributes`, `suts_suldf_scan_stream`) are
> stable API.

### 1.5 What SULDF decides, and what it does not

SULDF answers three questions about a stream:

1. **Is this codepoint link material?** (§3) — an Include character, a Hard
   terminator, a Soft terminator, or a bracket.
2. **Is this substring a URL or an email address, and where does it end?**
   (§4, §5, §6) — the initiation and termination algorithms.
3. **How does a Part serialize with minimal escaping?** (§4.1, §5.3).

It does **not** decide whether a link is *reachable*, whether a domain is
*registered*, whether a URL is *safe to fetch*, or whether a label is a valid
ICANN TLD — the first three are runtime policy, and the fourth is SIDN's
domain (§1.4, §9). Detection is a boundary decision, not an authorization.

### 1.6 Conformance and optionality

SULDF is a **modular optional** specification. A conforming implementation
shall provide, for every codepoint in the 31-bit SuperUnicode address space:

* a total attribute query whose answer is never a lookup failure (§3.4);
* a stream scanner that emits half-open spans in ascending order and
  identifies each as a URL or an email address (§3.5, §6);
* a Part serializer and an email local-part quoter (§4.1, §5.3);
* a zero-allocation path — no `malloc`, no `free`, no libc string routine,
  no static mutable state that grows.

An implementation that omits SULDF is still conforming to the SuperUnicode
architecture; it simply declines to linkify text, and its consumers must then
obtain the equivalent guarantees from the user-space reference layer (SUCD)
directly.

## 2. Scope of link detection

### 2.1 The boundary model

SULDF assumes an adversary who can place arbitrary codepoints around a link,
and a reader who is harmed when the link a renderer draws is not the link the
text said. The class of hazards is smaller than a source-code attack but
sharper than a rendering nicety.

| Vector | What a naive scanner does | What UTS #58 requires | SULDF's behaviour |
|--------|---------------------------|-----------------------|-------------------|
| Sentence period | eats `example.com.` | drops a Soft-final period | drops it (§4.4) |
| Trailing `!` | eats `example.com/a!` | drops a Soft-final `!` | drops it (§4.4) |
| Unmatched `)` | eats `example.com/a)` | drops the unmatched Close | drops it with the bracket stack (§4.5) |
| Matched `(b)` | eats `a(b)c` incorrectly | keeps the matched pair | keeps it (§4.5) |
| Empty label | links `foo..example.com` | rejects the empty label | rejects it (§4.2) |
| Userinfo | links `http://user@host/` | rejects the `@` before the host | rejects it (§4.2, §8) |
| Consecutive dots | links `john..doe@x.com` | rejects the local-part | rejects it (§5.2) |
| Trailing dot local-part | links `john.@x.com` | rejects the local-part | rejects it (§5.2) |

### 2.2 Address-space zones

SULDF classifies every codepoint into one of seven zones, mirroring the
three-space layout of SUTR-0 and the zone taxonomy of SUEPS and SUSCH.

| Zone | Range | Meaning |
|------|-------|---------|
| BRIDGE | `0x00000000`–`0x0010FFFF` | The Unicode Compatibility Bridge: ordinary text and link material. |
| SCP | `0x00110000`–`0x0011FFFF` | The System Control Plane: control markers, never link material. |
| RNUR | `0x00160000`–`0x0016FFFF` | The Native & Neography registry: link material, set-indexed (§5). |
| NATIVE | `0x00120000`–`0x7FFFFFEF` (rest) | The remaining Native plane: link material. |
| TRAP | `0x7FFFFFF0`–`0x7FFFFFFE` | Unallocatable trap slots: never link material. |
| SENTINEL | `0x7FFFFFFF` | The Sentinel: never link material. |
| EXTSUCS | `> 0x7FFFFFFF` | ExtSUCS plugin space: outside the address space, never link material. |

Only the BRIDGE, NATIVE, and RNUR zones can supply link material. A
codepoint in the SCP, a trap slot, the Sentinel, or ExtSUCS is Hard by
construction, so a link can never span one.

## 3. The compiled link schema

### 3.1 Table geometry and the lookup discipline

The compiled data is three range tables:

| Table | Source | Rows (Unicode 18.0.0) | Row | Record |
|-------|--------|----------------------|-----|--------|
| Term | `LinkTerm.txt` | 1117 | `{ first, last, term }` | 12 bytes |
| Email | `LinkEmail.txt` | 825 | `{ first, last }` | 8 bytes |
| Bracket | `LinkBracket.txt` | 65 | `{ close, open }` | 8 bytes |

Rows are sorted by `first`, are non-overlapping, and are separated by at least
one codepoint. This is a verification invariant, not a convenience: the lookup
is a binary search, and an unsorted overlapping table would silently answer
wrongly rather than fail. A comment block precedes each table restating the
invariant, and the capacities of §3.2 are compile-time checked.

The **bridge table** holds the properties that are only meaningful inside the
Unicode Compatibility Bridge. A codepoint above the Bridge is answered by the
zone rules of §5, not by a table row, which is what keeps the tables small.

### 3.2 Table capacities

```c
#define SULDF_TERM_CAPACITY      2048U
#define SULDF_EMAIL_CAPACITY     1536U
#define SULDF_BRACKET_CAPACITY   128U
#define SULDF_BRACKET_STACK_MAX  125U
```

A build-time `typedef char table_fits[...]` assertion fails the *build*, not
the kernel, if a data update overflows a table.

### 3.3 The link attribute

The compiled per-codepoint attribute is exactly 12 bytes, byte-aligned, and is
the struct named by the seeding sketch:

```c
typedef struct {
    uint32_t codepoint;      /* the raw 31-bit SuperUnicode codepoint */
    uint32_t link_mask;      /* the packed SULDF_FLAG_* word          */
    uint16_t term_class;     /* suldf_term_t                          */
    uint8_t  bracket_class;  /* suldf_bracket_t                       */
    uint8_t  reserved;       /* must be zero                          */
} suldf_link_attr_t;
```

### 3.4 The attribute query and the property predicates

`suts_suldf_get_attributes(codepoint, out)` evaluates a codepoint in O(1)
without allocation and without parsing. The query is **total** over the 31-bit
space: every codepoint from `0x00000000` through the Sentinel has an answer.
An unlisted Bridge codepoint is answered by the algorithmic default
`Link_Term = Hard` of §3.4.1 rather than by a lookup failure.

The query returns `false` — leaving `*out` untouched — only for a NULL output
pointer or a codepoint above the Sentinel, which is ExtSUCS plugin space and is
not link material.

The derived predicates are O(1) wrappers over the same tables:

| Predicate | True when |
|-----------|-----------|
| `suts_suldf_term_of` | returns the `Link_Term` category |
| `suts_suldf_is_email_char` | `Link_Email = Yes` |
| `suts_suldf_is_scheme_char` | the codepoint may appear in a URL scheme |
| `suts_suldf_is_domain_char` | the codepoint may appear in a domain label |
| `suts_suldf_is_path_char` | the codepoint may appear in Path / Query / Fragment |
| `suts_suldf_is_email_delim` | the codepoint is `@` |
| `suts_suldf_is_open_bracket` / `suts_suldf_is_close_bracket` | `Link_Term = Open` / `Close` |
| `suts_suldf_is_bidi_control` | the codepoint is a directional formatting control (§8) |

#### 3.4.1 `Link_Term`

`Link_Term` classifies a codepoint for the termination algorithm. The five
categories, in the numeric order of `LinkTerm.txt`, are:

| Category | Meaning |
|----------|---------|
| `Include` | keeps a link going |
| `Hard` | always terminates a link; the algorithmic **default** for unlisted codepoints |
| `Soft` | terminates a link only if the next character does not continue it |
| `Close` | a closing bracket; terminates unless it matches an open bracket on the stack |
| `Open` | an opening bracket; pushed and kept |

Worked example, from `LinkTerm.txt`: `!` and `'` are Soft (ranges
`0021..0022` and `0027`); `#` is Include (`0023..0026`); `(` and `)` are Open
and Close.

#### 3.4.2 `Link_Bracket`

`Link_Bracket` pairs each Close codepoint with the Open codepoint that closes
it. The mapping is exposed two ways:

* `suts_suldf_open_bracket_for(close)` returns the Open codepoint, or `0` when
  the codepoint has no mapping. `0` is never a valid Open bracket, so it is an
  unambiguous sentinel.
* `suts_suldf_brackets_match(close, open)` tests the pair directly.

`(`, `[`, `{`, and `<` are the canonical Open brackets; `)`, `]`, `}`, and `>`
are their Close partners.

### 3.5 The link span

A detection is a half-open codepoint span:

```c
typedef struct {
    size_t  start;      /* first codepoint of the link      */
    size_t  length;     /* number of codepoints in the link */
    uint8_t kind;       /* SULDF_LINK_URL or SULDF_LINK_EMAIL */
    uint8_t reserved0;  /* must be zero */
    uint16_t reserved1; /* must be zero */
} suldf_link_span_t;
```

`suts_suldf_scan_stream(stream, length, out_spans, capacity)` fills up to
`capacity` spans in ascending, non-overlapping order. The **return value is
the total number of links detected, independent of capacity**, so a caller can
size a buffer by calling once with `out_spans == NULL` and `capacity == 0`. A
zero-length stream, a NULL stream, or a NULL output array detects nothing
(and still returns the count when the array is NULL).

### 3.6 The curated dataset

The compiled tables are pinned to **Unicode 18.0.0**
(`https://www.unicode.org/Public/18.0.0/linkification/`), matching UTS #58
Revision 3 (2026-09-14). The merged row counts are:

| Table | Rows |
|-------|------|
| Term | 858 Include, 129 Soft, 65 Close, 65 Open (1117 total) |
| Email | 825 |
| Bracket | 65 |

A future data update is a patch-level change: the tables grow, the capacities
of §3.2 are re-checked, and no descriptor that linked before can fail to link
afterwards.

## 4. URL detection

### 4.1 The Parts and minimal escaping

A URL is a sequence of Parts:

| Part | Initiator | Terminator | ClearStackOpen |
|------|-----------|-----------|----------------|
| Path | `/` | `[?#]` | `[/]` |
| Query | `?` | `[#]` | `[=&]` |
| Fragment | `#` | `{~:~}` (the string `:~:`) | none |
| Fragment Directive | `:~:` (only after a Fragment or Fragment Directive) | none | `[&,{~:~}]` |

`suts_suldf_escape_part(part, length, part_kind, out, capacity)` serializes one
Part's interior (the initiator is not included) with the UTS #58
minimal-escaping algorithm. `part_kind` selects the Part's Terminator and
ClearStackOpen sets. The caller owns the normal interior escaping of literal
syntactic characters described in UTS #58 §4.1.

Codepoints are written to `out` as codepoints. A percent escape is emitted as
the three ASCII codepoints `%`, hex, hex. **Non-ASCII bytes use SUTF-8**
(SUTR-1), so a codepoint above the Unicode Bridge is escaped by its full byte
sequence rather than by a code point.

The return value is the number of codepoints the complete result *needs*,
independent of capacity (the `snprintf` convention). If `out` is NULL or
`capacity` is too small, only the prefix that fits is written.

`suts_suldf_escape_part_ex(..., is_last, ...)` adds the **trailing-soft rule**
of UTS #58 §4.1: the final codepoint of the *last* Part is escaped when it was
skipped as Soft, while a non-last Part copies its trailing Soft characters
because the next Part's initiator follows.

### 4.2 Initiation

A URL is initiated by an explicit scheme or as a bare domain.

* **Explicit scheme.** `http://` and `https://` are matched
  case-insensitively. The scheme must be followed by a host.
* **Bare domain.** A bare URL requires at least two labels and a final label
  of at least two codepoints.

The host parser (`suldf_parse_host`) walks the labels of a host and enforces
the admission rules:

* Each label **starts** with a domain character that is not a dot or a
  hyphen, and **ends** with a character that is not a hyphen.
* A label is a maximal run of `Link_Email` characters, so the same compiled
  table that answers §5 answers the domain question.
* IDNA label **dot equivalents** are accepted: `U+002E` (`FULL STOP`),
  `U+3002` (`IDEOGRAPHIC FULL STOP`), `U+FF0E` (`FULLWIDTH FULL STOP`), and
  `U+FF61` (`HALFWIDTH IDEOGRAPHIC FULL STOP`).
* An **empty label** — a dot immediately followed by a dot — is rejected for
  an explicit-scheme host.
* An optional `:port` of digits is parsed for a URL host. An email domain
  does **not** carry a port.
* A single sentence-final dot is kept only when a new Part initiator
  (`/`, `?`, or `#`) follows it.

**Userinfo is rejected.** A URL with `@` before the first Part initiator is
not a link (UTS #58 removes userinfo for security). SULDF enforces this in
two places: the explicit-scheme parser refuses a host whose scheme is
followed by an `@`, and the email scanner refuses a local-part that sits
inside a `scheme://` (a `://` separator within four codepoints before the
local-part boundary), so `http://john.smith@example.com` yields **no** link
at all.

### 4.3 Termination

`suldf_terminate` applies the UTS #58 termination algorithm to the codepoints
following a host, in one pass with backup:

1. Start in the host. `previous` is the part before the current one, so the
   Fragment-Directive initiator cannot fire before a Fragment.
2. The **open-bracket stack** is a fixed 125-entry array
   (`SULDF_BRACKET_STACK_MAX`). An Open codepoint is pushed; a Close codepoint
   is popped only if it matches the top of the stack; otherwise it terminates
   the link. A push beyond 125 is dropped, so the stack never grows.
3. In a Part, the **ClearStackOpen** set is checked before a Terminator, so a
   bracket opened in the Path is forgotten at the Query initiator.
4. A **Hard** codepoint terminates. A **Soft** codepoint terminates only if
   the next codepoint does not continue the link; a Soft codepoint that is
   *not* followed by Include material is removed from the link (the "backup"
   of the algorithm). A **Close** codepoint terminates unless it matches an
   Open on the stack.
5. A Part **Terminator** transitions to the next Part; the Terminator belongs
   to the old Part, so the new Part's initiator is the following character.

When the algorithm stops, the link ends at the last codepoint that survived.

### 4.4 Worked examples

| Input | Link | Why |
|-------|------|-----|
| `example.com/a b` | `example.com/a` | the space is Hard |
| `example.com/a!b` | `example.com/a!b` | `!` is Soft but Include follows |
| `example.com/a!` | `example.com/a` | trailing Soft is dropped |
| `example.com. on` | `example.com` | sentence-final Soft dot is dropped |
| `http://foo.example.com./x z` | `http://foo.example.com./x` | the dot is kept because a Path follows |
| `example.com/a(b)c` | `example.com/a(b)c` | matched brackets are kept |
| `example.com/a)` | `example.com/a` | unmatched Close ends the link |
| `example.com/a[(b])c` | `example.com/a[(b]` | mismatched Close ends the link |
| `example.com/a?b#c` | `example.com/a?b#c` | Path → Query → Fragment |

### 4.5 The bracket stack

The stack is bounded at 125 by `SULDF_BRACKET_STACK_MAX`. A stream that
contains more than 125 unconsumed Open brackets cannot push further; the
126th Open behaves like a Hard codepoint for the purpose of the stack. This
bound is a security property: the scanner's stack is a fixed array on the
frame, so a hostile stream cannot drive an allocation or an unbounded walk.

## 5. Email detection

### 5.1 `Link_Email`

`Link_Email` marks the codepoints that may occur in an *unquoted* email
local-part, in a domain label, and in the `mailto:` scheme. It is the same
table the host parser of §4.2 consults. Note that the dot is a `Link_Email`
character (`LinkEmail.txt` spans `002D..0039`), so the email algorithm gives
it special semantic handling at the local-part boundaries rather than
excluding it.

### 5.2 Local-part and domain

A detection begins at an `@`. The scanner walks **backward** to find the
local-part start, then **forward** to parse the domain:

* The local-part is a maximal run of `Link_Email` characters.
* A dot immediately before the `@` is rejected (`john.@example.com`).
* A dot immediately after the start, or a pair of consecutive dots, is
  rejected (`john..doe@example.com`, `.john@example.com`).
* The domain is parsed with the host parser in **email mode**: a dot is
  required, at least two labels, no port, and no strict empty-label rule.
* A `mailto:` immediately before the local-part (case-insensitive) is
  included in the span.
* A local-part inside `scheme://` is userinfo and is rejected (§4.2).

The email span therefore covers the local-part, the `@`, the domain, and a
preceding `mailto:` when one is present.

### 5.3 Minimal quoting

`suts_suldf_email_local_needs_quotes(local, length)` returns true when an
unquoted local-part is not valid under `Link_Email` and therefore needs the
wrapped quoted form of UTS #58 §5.3.

`suts_suldf_quote_email_local(local, length, out, capacity)` serializes the
local-part with minimal quoting: if every codepoint is `Link_Email = Yes`, the
local-part is copied unchanged; otherwise it is wrapped in ASCII double quotes
and the two RFC 5322 specials (`"` and `\`) are backslash-escaped. The return
value follows the same `snprintf` convention as `suts_suldf_escape_part`.

## 6. The stream scanner

### 6.1 The single linear pass

`suts_suldf_scan_stream` walks the codepoint stream once. At each index it
tries, in order:

1. an email address, when the codepoint is `@`;
2. an explicit-scheme URL;
3. a bare-domain URL.

On a hit it records the span and resumes **after** the link; on a miss it
advances one codepoint. The cost is O(n) in the stream length and O(1) in the
size of the address space. There is no setup, no lookahead beyond the link
being built, and no allocation on any path.

### 6.2 Ordering and non-overlap

Spans are emitted in ascending `start` order and never overlap, because the
scan resumes past each detected link. A detection always has positive length.

### 6.3 The counting contract

The return value counts **every** detection, even those that did not fit in
`out_spans`. This lets a caller learn the required capacity from a first call
with `out_spans == NULL, capacity == 0`, then size a buffer exactly. A NULL
`out_spans` never writes and never faults.

## 7. Complexity

| Operation | Time | Space |
|-----------|------|-------|
| `suts_suldf_get_attributes` | O(log R) ≈ O(1) | O(1) |
| `suts_suldf_term_of` and the predicates | O(log R) ≈ O(1) | O(1) |
| `suts_suldf_zone_of` | O(1) | O(1) |
| `suts_suldf_scan_stream` | O(n) | O(1) — one fixed 125-entry stack |
| `suts_suldf_escape_part` | O(n) | O(1) |
| `suts_suldf_quote_email_local` | O(n) | O(1) |

`R` is the row count of the table probed. No operation allocates, and no
operation holds state across calls.

## 8. Security considerations

A link renderer is a trust boundary: it decides what a reader clicks. SULDF
makes four commitments to that boundary.

1. **No userinfo.** A `scheme://user@host` is not a link (§4.2), so a
   renderer cannot present an attacker-controlled userinfo string as part of
   an otherwise-trusted host.
2. **No empty or hyphen-edged labels.** A host that cannot be a host is
   rejected rather than linked (§4.2), so `foo..example.com` and
   `-foo.example-.com` never become clickable.
3. **Bounded brackets.** The open-bracket stack is a fixed 125 entries
   (§4.5), so a hostile stream cannot drive unbounded work.
4. **Bidi controls are visible.** `suts_suldf_is_bidi_control` identifies the
   explicit controls (LRE, RLE, PDF, LRO, RLO), the implicit marks (LRM, RLM,
   ALM), and the isolate initiators and terminator (LRI, RLI, FSI, PDI). A
   detected link that contains one is a security finding, because a link whose
   displayed text disagrees with its codepoints is a spoof not a link. The
   consuming renderer is expected to refuse the bidi control or to display it
   visibly.

SULDF asserts nothing about a link's *destination*: it does not resolve,
fetch, or validate a host's existence or registration. That is SIDN's
domain (§1.4) and the runtime's policy.

## 9. Native & Neography integration

Unlike a standards-conformant linkifier restricted to the Unicode
Compatibility Bridge, SULDF admits the **Native & Neography Plane** as link
material. A Native or neography codepoint in a Path, Query, or Fragment is an
Include character and is **retained verbatim by the minimal-escaping
formatter**, so a custom script's URL stays readable instead of being silently
percent-encoded. The System Control Plane, the trap slots, the Sentinel, and
ExtSUCS are never link material.

RNUR coordinates are set-indexed: `suts_suldf_neography_set` returns
`(cp - 0x00160000) >> 8` for a codepoint inside the carve-out and false
outside it, matching the convention SUEPS and SUSCH use.

### 9.1 Specification boundary: ICANN IDN cases

Two UTS #58 conformance cases require full UTS #46 / IDNA data and are
therefore delegated to SUTS-006 (SIDN) rather than resolved here:

* a **Tibetan label** containing `U+0F0B` (`TIBETAN MARK`), which is
  `Link_Term = Include` but not `Link_Email`; UTS #58 admits it in a domain
  label through its domain-name character definition;
* a **punycode TLD** that must be validated as well-formed (`xn--...`).

SULDF's domain acceptance uses `Link_Email`, so those two cases are outside
its boundary. A caller that needs them links SIDN for the label validation
and uses SULDF for the span. This is the same dual-layer split of §1.3: SULDF
decides *where* a link ends, SIDN decides *whether a label is real*.

## 10. Worked example

The seeding brief's motivating case is a bare domain with a path and a
sentence period:

```
See example.com/a. Plus
```

Cleaned to codepoints, the scan finds one link:

```
[4, 17)  "example.com/a"   kind=URL
```

The sentence period at index 17 is a Soft codepoint at the edge of the link,
so it is dropped by the backup step of §4.3. A scan of

```
See http://john.smith@example.com plus
```

finds **no** link: the explicit-scheme parser refuses the userinfo, and the
email scanner refuses a local-part inside `scheme://` (§4.2). A scan of

```
See abcd@example.com on
```

finds one email link, `[4, 20)`, covering the local-part, the `@`, and the
domain.

## Annex A — Compliance matrix (reference implementation)

| Requirement | Reference implementation |
|-------------|--------------------------|
| Total O(1) attribute query (§3.4) | `suts_suldf_get_attributes` |
| `Link_Term` category (§3.4.1) | `suts_suldf_term_of` |
| `Link_Bracket` pairing (§3.4.2) | `suts_suldf_open_bracket_for`, `suts_suldf_brackets_match` |
| `Link_Email` membership (§5.1) | `suts_suldf_is_email_char` |
| Bidi-control predicate (§8) | `suts_suldf_is_bidi_control` |
| Zone classification and neography carve-out (§2.2, §9) | `suts_suldf_zone_of`, `suts_suldf_is_neography`, `suts_suldf_neography_set` |
| URL and email scanning (§4, §5, §6) | `suts_suldf_scan_stream` |
| Minimal Part escaping (§4.1) | `suts_suldf_escape_part`, `suts_suldf_escape_part_ex` |
| Email minimal quoting (§5.3) | `suts_suldf_email_local_needs_quotes`, `suts_suldf_quote_email_local` |
| Taxonomy names | `suts_suldf_term_name`, `suts_suldf_bracket_name`, `suts_suldf_zone_name`, `suts_suldf_kind_name`, `suts_suldf_part_name` |

## Annex B — Flag bit assignments

| Bit | Name | Meaning |
|-----|------|---------|
| 0 | `SULDF_FLAG_SCHEME_CHAR` | may appear in a URL scheme |
| 1 | `SULDF_FLAG_DOMAIN_CHAR` | may appear in a domain label |
| 2 | `SULDF_FLAG_PATH_CHAR` | may appear in Path / Query / Fragment |
| 3 | `SULDF_FLAG_EMAIL_DELIM` | the `@` email local-part marker |
| 4 | `SULDF_FLAG_TERM_INCLUDE` | `Link_Term = Include` |
| 5 | `SULDF_FLAG_TERM_HARD` | `Link_Term = Hard` |
| 6 | `SULDF_FLAG_TERM_SOFT` | `Link_Term = Soft` |
| 7 | `SULDF_FLAG_TERM_CLOSE` | `Link_Term = Close` |
| 8 | `SULDF_FLAG_TERM_OPEN` | `Link_Term = Open` |
| 9 | `SULDF_FLAG_BRACKET_MAPPED` | has a `Link_Bracket` mapping |
| 10 | `SULDF_FLAG_EMAIL_CHAR` | `Link_Email = Yes` |
| 11 | `SULDF_FLAG_NATIVE` | Native / neography plane |
| 12 | `SULDF_FLAG_BIDI_CONTROL` | directional formatting control |

## References

* UTS #58, *Unicode Link Detection and Formatting*, Revision 3 (2026-09-14)
* Unicode 18.0.0 linkification data: `LinkTerm.txt`, `LinkBracket.txt`,
  `LinkEmail.txt`, `LinkDetectionTest.txt`, `LinkFormattingTest.txt`
* SUTR-0 (SUCS Core), SUTR-1 (SUTF), SUTR-6 (Bridge), SUTR-7 (SUCEM),
  SUTR-8 (SUCPM), SUTR-10 (SUCM)
* SUTS-005 (SCSS), SUTS-006 (SIDN), SUTS-008 (SUSCH)
* SUAS-012 (SUCD), SUAS-005 (SSP)
