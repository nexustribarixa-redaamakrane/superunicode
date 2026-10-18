# Technical Reports section

$Pages['reports/index'] = @{
    path    = 'reports/index.html'
    sec     = 'reports'
    title   = 'Technical Reports — SUTR'
    desc    = 'The SUTR-0 through SUTR-10 specifications of SuperUnicode.'
    crumbName = 'Technical Reports'
    h1      = 'Technical <span class="grad">Reports</span>'
    subtitle= 'The SUTR series &mdash; normative specifications, tracked per release.'
    body    = @(
        @{ t = 'p'; html = 'Technical Reports formalize each contract of the standard. They play the role of Unicode&rsquo;s UAX/UTR documents and are identified by the <span class="mono">SUTR-<em>n</em></span> scheme with per-version errata.' }
        @{ t = 'table'; head = @('Report', 'Status', 'Scope'); rows = @(
            @('<span class="mono">SUTS-001</span>', 'Draft &mdash; Ratified', '<a href="SUTS-001.html">SuperUnicode Collation Algorithm (SUCA)</a> &mdash; UCA-equivalent multilevel collation, 64-bit extSUCS compatible')
            @('<span class="mono">SUAS-001</span>', 'Draft &mdash; Ratified', '<a href="SUAS-001.html">Structural Directional Framing (SDF)</a> &mdash; core BiDi architecture, scope isolation and glyph mirroring')
            @('<span class="mono">SUAS-002</span>', 'Draft &mdash; Ratified', '<a href="SUAS-002.html">System Glyph Width &amp; Monospace Grid (SGW)</a> &mdash; fixed-cell terminal metrics, EAW-style width over 64-bit SUCS')
            @('<span class="mono">SUAS-003</span>', 'Draft &mdash; Ratified', '<a href="SUAS-003.html">System Boundary &amp; Line Break Rules (SBR)</a> &mdash; single-pass line breaking over 64-bit SUCS')
            @('<span class="mono">SUAS-004</span>', 'Draft &mdash; Ratified', '<a href="SUAS-004.html">SuperUnicode Canonical Forms (SUCF)</a> &mdash; dual-target canonical (de)composition, zero allocation')
            @('<span class="mono">SUAS-005</span>', 'Draft &mdash; Ratified', '<a href="SUAS-005.html">System Script Property (SSP)</a> &mdash; range-dispatched script resolution and font-fallback routing')
            @('<span class="mono">SUAS-006</span>', 'Draft &mdash; Ratified', '<a href="SUAS-006.html">System Text Segmentation (STS)</a> &mdash; single-pass grapheme, word, and sentence boundaries over 64-bit SUCS')
            @('<span class="mono">SUAS-007</span>', 'Draft &mdash; Ratified', '<a href="SUAS-007.html">System Identifiers and Syntax (SIS)</a> &mdash; single-pass identifier classes and lexical syntax over the SUCS space')
            @('<span class="mono">SUAS-008</span>', 'Draft &mdash; Ratified', '<a href="SUAS-008.html">System Named Character Sequences (SNCS)</a> &mdash; static-registry, O(1) resolution of multi-codepoint symbol names')
            @('<span class="mono">SUTR-0</span>', '0.1.0', '<a href="SUTR-0.html">SUCS Core</a> &mdash; hierarchy, three-space layout, SCP, Traps, Sentinel')
            @('<span class="mono">SUTR-1</span>', '0.1.0', '<a href="SUTR-1.html">SUTF</a> &mdash; the Character Encoding Forms: SUTF-8/16/4/2 and vSUTF')
            @('<span class="mono">SUTR-2</span>', '0.1.0', '<a href="SUTR-2.html">SUCA</a> &mdash; SuperUnicode Collation Algorithm')
            @('<span class="mono">SUTR-3</span>', '0.1.0', '<a href="SUTR-3.html">SuperUnicode Storage</a> &mdash; OWFS/USFS partition policy')
            @('<span class="mono">SUTR-4</span>', '0.1.0', '<a href="SUTR-4.html">ExtSUCS Transport</a> &mdash; vector, vsutf, e-SUST')
            @('<span class="mono">SUTR-5</span>', '0.1.0', '<a href="SUTR-5.html">Plugin Lifecycle &amp; Blob Format</a> &mdash; stage, checksum gate, mount, registry')
            @('<span class="mono">SUTR-6</span>', '0.1.0', '<a href="SUTR-6.html">Unicode Compatibility Bridge</a> &mdash; the permanent 1:1 guarantee')
            @('<span class="mono">SUTR-7</span>', '0.1.0', '<a href="SUTR-7.html">SUCEM</a> &mdash; the SuperUnicode Character Encoding Model: ACR &rarr; SUCS &rarr; SUTF &rarr; SUST')
            @('<span class="mono">SUTR-8</span>', '0.1.0', '<a href="SUTR-8.html">SUCPM</a> &mdash; the SuperUnicode Character Property Model: tiers, dual lookup engine, bitmask packing')
            @('<span class="mono">SUTR-9</span>', '0.1.0', '<a href="SUTR-9.html">SUMNLS</a> &mdash; SuperUnicode Mathematical Notation and Layout Support: repertoire, math properties, box-model layout engine')
            @('<span class="mono">SUTR-10</span>', '0.1.0', '<a href="SUTR-10.html">SUCM</a> &mdash; SuperUnicode Conformance Model: compliance tiers, zero-allocation mandate, verification gates')
        ) }
        @{ t = 'h2'; html = 'Core invariants every report honors' }
        @{ t = 'ul'; items = @(
            '<span class="mono">SUCS_CP</span> is 31-bit; <span class="mono">sucs_ex_char_t</span> is 64-bit.'
            '<span class="mono">0x000000&ndash;0x10FFFF</span> is permanently 1:1 with Unicode.'
            'The 16 terminal values <span class="mono">0x7FFFFFF0&ndash;0x7FFFFFFF</span> are unallocatable: 15 trap slots, then the Sentinel at <span class="mono">0x7FFFFFFF</span>. The last allocatable codepoint is <span class="mono">0x7FFFFFEF</span>.'
            'Plugin codepoints live in the Native plane (<span class="mono">0x120000&ndash;0x7FFFFFEF</span>) or strictly above <span class="mono">0x7FFFFFFF</span>, and mount only after the checksum gate.'
        ) }
    )
}

$Pages['reports/SUTS-001'] = @{
    path    = 'reports/SUTS-001.html'
    sec     = 'reports'
    title   = 'SUTS-001 — SuperUnicode Collation Algorithm'
    desc    = 'SUTS-001: the SuperUnicode Collation Algorithm (SUCA) — a full UCA-equivalent multilevel collation over the 64-bit SUCS space.'
    crumbName = 'SUTS-001 — SUCA'
    crumbs  = @( @{ label = 'Technical Reports'; href = 'index.html' } )
    h1      = 'SUTS-001 &mdash; SuperUnicode <span class="grad">Collation Algorithm</span>'
    subtitle= 'The first SuperUnicode Technical Specification: UCA (UTS #10) equivalence over 64-bit SUCS.'
    body    = @(
        @{ t = 'p'; html = 'SUTS-001 (SUCA) is the first SuperUnicode Technical Specification. It defines a full UTS #10 (UCA)-equivalent multilevel collation algorithm for the entire 64-bit SuperUnicode character space (<span class="mono">sucs_ex_char_t</span>), including the extSUCS plugin ranges above <span class="mono">0x7FFFFFFF</span>.' }
        @{ t = 'h2'; html = 'Algorithm' }
        @{ t = 'ol'; items = @(
            '<strong>Normalize (S1)</strong> &mdash; decompose input to NFD; algorithmic Hangul; canonical ordering.'
            '<strong>Map (S2)</strong> &mdash; walk the string, longest-match contractions and expansions, then simple/implicit CEs; apply variable weighting.'
            '<strong>Form sort key (S3)</strong> &mdash; L1&ndash;L4 with level separators, backward secondary support, optional identical level.'
            '<strong>Compare (S4)</strong> &mdash; binary compare of sort keys; identical level resolves by native codepoint order.'
        ) }
        @{ t = 'table'; head = @('Feature', 'Status'); rows = @(
            @('At least 3 collation levels + identical', 'Yes &mdash; L1&ndash;L4 + identical')
            @('Canonical equivalence (NFD)', 'Yes')
            @('Contractions &amp; expansions', 'Yes')
            @('Variable weighting (shifted/blanked/non-ignorable/shift-trimmed)', 'Yes')
            @('Backward secondary (French)', 'Yes')
            @('Implicit weights for unassigned/Han/native/plugin', 'Yes (algorithmic, 64-bit codepoint)')
            @('Tailoring rules (&amp; base &lt; x, &lt;&lt;, &lt;&lt;&lt;, =)', 'Yes (programmatic)')
            @('extSUCS 64-bit compatibility', 'Yes')
        ) }
        @{ t = 'callout'; html = 'Reference implementation: <span class="mono">include/suts/suts_suca.h</span> + <span class="mono">src/suts/suts_suca.c</span> (freestanding C99, no heap), registered as <span class="mono">suts_static</span>. Data: <span class="mono">collation/SUCA.txt</span>, <span class="mono">collation/ExtUCA.txt</span>.' }
        @{ t = 'note'; html = 'Source of truth: <span class="mono">docs/suts/SUTS-001-suca.md</span> in the repository. This Technical Report introduces SUCA; <a href="SUTR-2.html">SUTR-2</a> covers the same ground as the original report.' }
    )
}

$Pages['reports/SUAS-001'] = @{
    path    = 'reports/SUAS-001.html'
    sec     = 'reports'
    title   = 'SUAS-001 — Structural Directional Framing'
    desc    = 'SUAS-001: Structural Directional Framing (SDF) — core BiDi architecture, scope isolation and glyph mirroring over the SUCS space.'
    crumbName = 'SUAS-001 — SDF'
    crumbs  = @( @{ label = 'Technical Reports'; href = 'index.html' } )
    h1      = 'SUAS-001 &mdash; Structural Directional <span class="grad">Framing</span>'
    subtitle= 'The first SuperUnicode Architecture Standard: core BiDi architecture, scope isolation and glyph mirroring.'
    body    = @(
        @{ t = 'p'; html = 'SUAS-001 (SDF) is the first SuperUnicode Architecture Standard and the <strong>core architecture</strong> for bi-directional text layout, scope isolation and glyph mirroring. It is a <strong>non-negotiable</strong> invariant: every conforming renderer MUST resolve bi-directional text through the SDF engine.' }
        @{ t = 'h2'; html = 'Design' }
        @{ t = 'ul'; items = @(
            '<strong>Single-pass, zero-allocation</strong> &mdash; a state-machine pipeline carried in the SCP and the SUTF/SUST decoding streams.'
            '<strong>Dual-Mode Directional Resolver</strong> &mdash; Unicode Bridge classifies from SUCD BiDi; SCP directives are explicit; Native SUCS inherits.'
            '<strong>Fixed isolate stack</strong> &mdash; push/pop are O(1) on an embedded fixed array; overflow/underflow are structural errors.'
            '<strong>Framed output word</strong> &mdash; 64-bit: 31b codepoint, 3b dir_type, 1b mirrored, 29b reserved; renderers mirror instantly.'
        ) }
        @{ t = 'h2'; html = 'SCP directional directives' }
        @{ t = 'table'; head = @('Codepoint', 'Directive'); rows = @(
            @('<span class="mono">0x00110101</span>', '<span class="mono">SCP_DIR_LTR</span> — set resolved direction left-to-right')
            @('<span class="mono">0x00110102</span>', '<span class="mono">SCP_DIR_RTL</span> — set resolved direction right-to-left')
            @('<span class="mono">0x00110104</span>', '<span class="mono">SCP_DIR_ISOLATE_PUSH</span> — open an isolate')
            @('<span class="mono">0x00110108</span>', '<span class="mono">SCP_DIR_ISOLATE_POP</span> — close the active isolate')
        ) }
        @{ t = 'h2'; html = 'Full BiDi processing model (UAX #9)' }
        @{ t = 'p'; html = 'For renderers that require lexical bi-directional layout, SDF also implements the full Unicode Bidirectional Algorithm processing model: paragraph levels (P1&ndash;P3), explicit embedding/override/isolate levels (X1&ndash;X8), X9 removal, weak and neutral resolution (W1&ndash;W7, N0&ndash;N2), implicit levels (I1&ndash;I2), and reordering with mirroring (L1&ndash;L4).' }
        @{ t = 'table'; head = @('Feature', 'Status'); rows = @(
            @('Single-pass deterministic consumption', 'Yes')
            @('Zero heap allocation', 'Yes')
            @('O(1) isolate stack push/pop on fixed array', 'Yes')
            @('dir_type + mirrored metadata on every framed word', 'Yes')
            @('Dual-mode direction resolution', 'Yes')
            @('Full BiDi processing model (UAX #9)', 'Yes')
            @('Explicit-only scope changes (four SCP directives)', 'Yes')
        ) }
        @{ t = 'callout'; html = 'Reference implementation: <span class="mono">include/suas/suas_sdf.h</span> + <span class="mono">src/suas/suas_sdf.c</span> (freestanding C99, no heap), registered in <span class="mono">suas_static</span>.' }
        @{ t = 'note'; html = 'Source of truth: <span class="mono">docs/suas/SUAS-001-sdf.md</span> in the repository.' }
    )
}

$Pages['reports/SUAS-002'] = @{
    path    = 'reports/SUAS-002.html'
    sec     = 'reports'
    title   = 'SUAS-002 — System Glyph Width & Monospace Grid'
    desc    = 'SUAS-002: System Glyph Width & Monospace Grid (SGW) — East_Asian_Width-style width classification and O(1) fixed-cell grid over the 64-bit SUCS space.'
    crumbName = 'SUAS-002 — SGW'
    crumbs  = @( @{ label = 'Technical Reports'; href = 'index.html' } )
    h1      = 'SUAS-002 &mdash; System Glyph Width <span class="grad">&amp; Monospace Grid</span>'
    subtitle= 'The second SuperUnicode Architecture Standard: East_Asian_Width-style width classification and O(1) fixed-cell grid metric.'
    body    = @(
        @{ t = 'p'; html = 'SUAS-002 (SGW) is the second SuperUnicode Architecture Standard. It defines an East_Asian_Width-style (UAX #11) width classification over the full 64-bit SUCS space (<span class="mono">sucs_ex_char_t</span>), folded into an O(1) monospace grid contract (0/1/2 cells + column cursor) for terminal emulators and CJK/ideographic framebuffer consoles.' }
        @{ t = 'h2'; html = 'Width classes' }
        @{ t = 'p'; html = 'Six width classes are defined, mirroring the UAX #11 categories:' }
        @{ t = 'ul'; items = @(
            '<strong>F</strong> (Fullwidth) &mdash; characters that are always wide.'
            '<strong>H</strong> (Halfwidth) &mdash; characters that are always narrow (e.g. <span class="mono">U+20A9 &#8361;</span>).'
            '<strong>W</strong> (Wide) &mdash; characters that are wide in East Asian context (e.g. Han ideographs, Emoji_Presentation).'
            '<strong>Na</strong> (Narrow) &mdash; characters that are always narrow (e.g. <span class="mono">U+00A5 &yen;</span>).'
            '<strong>A</strong> (Ambiguous) &mdash; characters whose width depends on context (e.g. <span class="mono">U+212B &#8491;</span>, <span class="mono">U+01D4</span>).'
            '<strong>N</strong> (Neutral) &mdash; characters that do not occur in East Asian text (e.g. <span class="mono">U+00C5 &Aring;</span>, <span class="mono">U+01D3</span>).'
        ) }
        @{ t = 'h2'; html = 'Zone dispatch' }
        @{ t = 'ol'; items = @(
            '<strong>Unicode Bridge</strong> <span class="mono">0x00000000&ndash;0x0010FFFF</span> &mdash; curated EAW table.'
            '<strong>SCP</strong> <span class="mono">0x00110000&ndash;0x0011FFFF</span> &mdash; non-advancing control, grid 0.'
            '<strong>Native SUCS</strong> <span class="mono">0x00120000&ndash;0x7FFFFFFE</span> &mdash; single cell.'
            '<strong>ExtSUCS</strong> <span class="mono">&gt;0x7FFFFFFF</span> &mdash; single cell.'
            '<strong>Trap</strong> <span class="mono">0x7FFFFFF0&ndash;0x7FFFFFFE</span> / <strong>Sentinel</strong> <span class="mono">0x7FFFFFFF</span> &mdash; grid 0.'
        ) }
        @{ t = 'h2'; html = 'Key rules' }
        @{ t = 'ul'; items = @(
            '<span class="mono">U+20A9 &#8361;</span> = H, <span class="mono">U+00A5 &yen;</span> = Na, <span class="mono">U+00C5 &Aring;</span> = N, <span class="mono">U+212B &#8491;</span> = A.'
            '<span class="mono">U+01D4</span> = A, <span class="mono">U+01D3</span> = N.'
            'Han ranges = Wide (including unassigned).'
            'Emoji_Presentation = Wide except Regional_Indicator (<span class="mono">U+1F1E6..1F1FF</span>).'
        ) }
        @{ t = 'table'; head = @('Feature', 'Status'); rows = @(
            @('Six width classes F/H/W/Na/A/N', 'Yes')
            @('O(1) grid cell + column advance', 'Yes')
            @('Zero heap allocation', 'Yes')
            @('64-bit extSUCS zone dispatch', 'Yes')
            @('Ambiguous contextual resolution', 'Yes')
            @('Emoji_Presentation&rarr;Wide except Regional_Indicator', 'Yes')
            @('Tailoring override hook', 'Yes')
        ) }
        @{ t = 'callout'; html = 'Reference implementation: <span class="mono">include/suas/suas_sgw.h</span> + <span class="mono">src/suas/suas_sgw.c</span> (freestanding C99, no heap), registered in <span class="mono">suas_static</span>.' }
        @{ t = 'note'; html = 'Source of truth: <span class="mono">docs/suas/SUAS-002-sgw.md</span> in the repository.' }
    )
}

$Pages['reports/SUAS-003'] = @{
    path    = 'reports/SUAS-003.html'
    sec     = 'reports'
    title   = 'SUAS-003 — System Boundary & Line Break Rules'
    desc    = 'SUAS-003: System Boundary & Line Break Rules (SBR) — single-pass deterministic line breaking over the 64-bit SUCS space.'
    crumbName = 'SUAS-003 — SBR'
    crumbs  = @( @{ label = 'Technical Reports'; href = 'index.html' } )
    h1      = 'SUAS-003 &mdash; System Boundary <span class="grad">&amp; Line Break Rules</span>'
    subtitle= 'The third SuperUnicode Architecture Standard: single-pass deterministic line breaking over 64-bit SUCS.'
    body    = @(
        @{ t = 'p'; html = 'SUAS-003 (SBR) is the third SuperUnicode Architecture Standard. It defines where a line may, must, or must not be broken over the full 64-bit SUCS space (<span class="mono">sucs_ex_char_t</span>) via a <strong>single-pass deterministic state transition table</strong> (a UAX #14 analog) &mdash; no multi-pass buffer algorithm, zero allocation, O(1) pair resolution.' }
        @{ t = 'h2'; html = 'Break statuses' }
        @{ t = 'ul'; items = @(
            '<strong>MUST_BREAK</strong> &mdash; a line boundary is mandatory (e.g. after a control).'
            '<strong>CAN_BREAK</strong> &mdash; an allowed / opportunistic boundary.'
            '<strong>NO_BREAK</strong> &mdash; a prohibited boundary.'
            '<strong>ALPHANUM_BREAK</strong> &mdash; mandatory CJK/numeric alphabetic break.'
        ) }
        @{ t = 'h2'; html = 'Explicit SCP Break Markers' }
        @{ t = 'table'; head = @('Codepoint', 'Directive'); rows = @(
            @('<span class="mono">0x00110020</span>', '<span class="mono">SCP_BRK_MANDATORY</span> — force a line break here')
            @('<span class="mono">0x00110021</span>', '<span class="mono">SCP_BRK_PROHIBITED</span> — force no line break here')
            @('<span class="mono">0x00110022</span>', '<span class="mono">SCP_BRK_OPPORTUNISTIC</span> — allow a break here')
        ) }
        @{ t = 'h2'; html = 'Zone dispatch' }
        @{ t = 'ol'; items = @(
            '<strong>Unicode Bridge</strong> <span class="mono">0x00000000&ndash;0x0010FFFF</span> &mdash; curated UAX #14 break-class table + transition matrix.'
            '<strong>SCP</strong> <span class="mono">0x00110000&ndash;0x0011FFFF</span> &mdash; Explicit Break Markers override; other controls are mandatory.'
            '<strong>Native SUCS</strong> <span class="mono">0x00120000&ndash;0x7FFFFFFE</span> &mdash; O(1) high-bit range bitmask word test.'
            '<strong>ExtSUCS</strong> <span class="mono">&gt;0x7FFFFFFF</span> &mdash; neutral gap default.'
        ) }
        @{ t = 'table'; head = @('Feature', 'Status'); rows = @(
            @('Four break statuses (MUST/CAN/NO/ALPHANUM)', 'Yes')
            @('Single-pass O(1) transition-matrix pair resolution', 'Yes')
            @('Zero heap allocation', 'Yes')
            @('64-bit extSUCS zone dispatch', 'Yes')
            @('Explicit SCP Break Markers override', 'Yes')
            @('Invisible formatting (WJ, ZWSP)', 'Yes')
            @('Native SUCS high-bit bitmask word test', 'Yes')
            @('Alphanumeric (CJK numeric) break', 'Yes')
            @('Tailoring override hook', 'Yes')
        ) }
        @{ t = 'callout'; html = 'Reference implementation: <span class="mono">include/suas/suas_sbr.h</span> + <span class="mono">src/suas/suas_sbr.c</span> (freestanding C99, no heap), registered in <span class="mono">suas_static</span>.' }
        @{ t = 'note'; html = 'Source of truth: <span class="mono">docs/suas/SUAS-003-sbr.md</span> in the repository.' }
    )
}

$Pages['reports/SUAS-004'] = @{
    path    = 'reports/SUAS-004.html'
    sec     = 'reports'
    title   = 'SUAS-004 — SuperUnicode Canonical Forms (SUCF)'
    desc    = 'SUAS-004: SuperUnicode Canonical Forms (SUCF) — dual-target canonical decomposition/composition over the 64-bit SUCS space.'
    crumbName = 'SUAS-004 — SUCF'
    crumbs  = @( @{ label = 'Technical Reports'; href = 'index.html' } )
    h1      = 'SUAS-004 &mdash; SuperUnicode <span class="grad">Canonical Forms</span>'
    subtitle= 'The fourth SuperUnicode Architecture Standard: dual-target, zero-allocation canonical (de)composition over 64-bit SUCS.'
    body    = @(
        @{ t = 'p'; html = 'SUAS-004 (SUCF) is the fourth SuperUnicode Architecture Standard. It defines canonical equivalence over the full 64-bit SUCS space (<span class="mono">sucs_ex_char_t</span>) via a <strong>single-pass sliding-window engine</strong> (a UAX #15 analog) with two targets: <strong>SUCF-C</strong> (canonical composition &mdash; the compact storage / equality form) and <strong>SUCF-D</strong> (canonical decomposition &mdash; the analysis / stripping / sorting form). Zero allocation: all reordering lives in a small stack-allocated window.' }
        @{ t = 'h2'; html = 'Dual targets' }
        @{ t = 'table'; head = @('Target', 'Purpose', 'Behavior'); rows = @(
            @('<span class="mono">SUCF-C</span>', 'Compact storage &amp; equality', 'decompose, reorder by CCC, then recompose greedily (Hangul algorithmic)')
            @('<span class="mono">SUCF-D</span>', 'Analysis, stripping &amp; sorting', 'decompose and reorder by CCC; never recompose')
        ) }
        @{ t = 'h2'; html = 'CCC reordering' }
        @{ t = 'p'; html = 'Every combining mark carries a Canonical Combining Class (CCC). Marks sort ascending by CCC; a pair swaps iff <span class="mono">CCC_A &gt; CCC_B</span> and <span class="mono">CCC_B &ne; 0</span> &mdash; stable for equal classes, and a starter (CCC 0) never moves. Stream-safe bound: 30 non-starters per run.' }
        @{ t = 'h2'; html = 'Hangul' }
        @{ t = 'p'; html = 'No table rows: syllables <span class="mono">0xAC00&ndash;0xD7A3</span> decompose algorithmically to L+V(+T); SUCF-C composes explicit L+V and LV+T sequences arithmetically.' }
        @{ t = 'h2'; html = 'SCP immunity' }
        @{ t = 'p'; html = 'SCP <span class="mono">0x00110000&ndash;0x0011FFFF</span>, BANcodes, Trap and Sentinel are canonically invariant: they pass through untouched and never disturb combining state, while preserved in their original stream position.' }
        @{ t = 'table'; head = @('Feature', 'Status'); rows = @(
            @('Dual targets (SUCF-C composition / SUCF-D decomposition)', 'Yes')
            @('Single-pass sliding-window engine', 'Yes')
            @('Zero heap allocation', 'Yes')
            @('Curated CCC + decomposition tables (Unicode Bridge)', 'Yes')
            @('Algorithmic Hangul (de)composition', 'Yes')
            @('Composition exclusions (singletons, non-starters)', 'Yes')
            @('30 non-starter stream-safe bound', 'Yes')
            @('SCP / Trap / Sentinel invariance', 'Yes')
            @('Quick check YES/NO/MAYBE', 'Yes')
            @('Streaming &equiv; bulk transform', 'Yes')
        ) }
        @{ t = 'callout'; html = 'Reference implementation: <span class="mono">include/suas/suas_sucf.h</span> + <span class="mono">src/suas/suas_sucf.c</span> (freestanding C99, no heap), registered in <span class="mono">suas_static</span>.' }
        @{ t = 'note'; html = 'Source of truth: <span class="mono">docs/suas/SUAS-004-sucf.md</span> in the repository.' }
    )
}

$Pages['reports/SUAS-005'] = @{
    path    = 'reports/SUAS-005.html'
    sec     = 'reports'
    title   = 'SUAS-005 — System Script Property (SSP)'
    desc    = 'SUAS-005: System Script Property (SSP) — range-dispatched Script and Script_Extensions resolution over the 64-bit SUCS space.'
    crumbName = 'SUAS-005 — SSP'
    crumbs  = @( @{ label = 'Technical Reports'; href = 'index.html' } )
    h1      = 'SUAS-005 &mdash; System <span class="grad">Script Property</span>'
    subtitle= 'The fifth SuperUnicode Architecture Standard: O(1) range-dispatched script resolution and font-fallback routing over 64-bit SUCS.'
    body    = @(
        @{ t = 'p'; html = 'SUAS-005 (SSP) is the fifth SuperUnicode Architecture Standard. It defines the <strong>Script</strong> and <strong>Script_Extensions</strong> properties over the full 64-bit SUCS space (<span class="mono">sucs_ex_char_t</span>) via a single <strong>range-dispatched, zero-allocation engine</strong> (a UAX #24 analog). Every codepoint resolves to a compact 16-bit script id that packs directly into a character token, letting the <span class="mono">suf</span> font engine jump straight to the right typeface block with no fallback search.' }
        @{ t = 'h2'; html = 'Two properties' }
        @{ t = 'table'; head = @('Property', 'Value', 'Behavior'); rows = @(
            @('<span class="mono">Script</span>', 'Enumerated 16-bit id', 'One writing system per codepoint; two implicit values (<span class="mono">UNKNOWN</span>, <span class="mono">COMMON</span>)')
            @('<span class="mono">Script_Extensions</span>', 'Set of ids (max 4)', 'Scripts in which a codepoint is commonly used; defaults to the singleton <span class="mono">{ Script(cp) }</span>')
        ) }
        @{ t = 'h2'; html = 'Zone dispatch' }
        @{ t = 'ol'; items = @(
            '<strong>Unicode Bridge</strong> <span class="mono">0x00000000&ndash;0x0010FFFF</span> &mdash; curated Script + merged Common range tables.'
            '<strong>SCP</strong> <span class="mono">0x00110000&ndash;0x0011FFFF</span> &mdash; a script-context directive decodes its selected id; other controls carry no script.'
            '<strong>RNUR carve-out</strong> <span class="mono">0x00160000&ndash;0x0016FFFF</span> &mdash; registry coordinate decode (unregistered &rarr; Unknown).'
            '<strong>Native plane</strong> <span class="mono">0x00120000&ndash;0x7FFFFFFE</span> / <strong>ExtSUCS</strong> &mdash; Unknown until a plugin registers a script.'
            '<strong>Trap</strong> <span class="mono">0x7FFFFFF0&ndash;0x7FFFFFFE</span> / <strong>Sentinel</strong> <span class="mono">0x7FFFFFFF</span> &mdash; Unknown.'
        ) }
        @{ t = 'h2'; html = 'SCP script-context directives' }
        @{ t = 'p'; html = 'An explicit script context is in-band, at <span class="mono">0x00112000 + (id &amp; 0x7FFF)</span> (sub-range <span class="mono">0x00112000&ndash;0x00119FFF</span>) &mdash; disjoint from the BANcodes (<span class="mono">0x0011A000</span>+) and the SDF/SBR directives.' }
        @{ t = 'h2'; html = 'Common / Inherited handling' }
        @{ t = 'p'; html = 'Unicode&rsquo;s <strong>Common</strong> and <strong>Inherited</strong> scripts are merged into the single implicit value <span class="mono">COMMON</span>. <span class="mono">suas_ssp_resolve()</span> gives a mark or shared punctuation the script of the preceding base when that context is explicit.' }
        @{ t = 'h2'; html = 'Font-engine token' }
        @{ t = 'p'; html = 'Bits 0&ndash;15 carry the 16-bit script id, bits 16&ndash;46 the 31-bit codepoint, bits 47&ndash;63 reserved zero &mdash; a direct index into the registered typeface-block table.' }
        @{ t = 'table'; head = @('Feature', 'Status'); rows = @(
            @('Enumerated Script value for every codepoint', 'Yes')
            @('Merged Common + Inherited with inheritance', 'Yes')
            @('Script_Extensions set (non-empty, max 4)', 'Yes')
            @('Explicit-only SCP script context', 'Yes')
            @('RNUR carve-out decode (unregistered &rarr; Unknown)', 'Yes')
            @('16-bit id packs into a 64-bit token', 'Yes')
            @('SUCPM Script_Identifier reconciliation (10-bit Bridge window)', 'Yes')
            @('Zero heap allocation, bounded O(log R)', 'Yes')
            @('Freestanding C99', 'Yes')
        ) }
        @{ t = 'callout'; html = 'Reference implementation: <span class="mono">include/suas/suas_ssp.h</span> + <span class="mono">src/suas/suas_ssp.c</span> (freestanding C99, no heap), registered in <span class="mono">suas_static</span>.' }
        @{ t = 'note'; html = 'Source of truth: <span class="mono">docs/suas/SUAS-005-ssp.md</span> in the repository.' }
    )
}

$Pages['reports/SUAS-006'] = @{
    path    = 'reports/SUAS-006.html'
    sec     = 'reports'
    title   = 'SUAS-006 — System Text Segmentation (STS)'
    desc    = 'SUAS-006: System Text Segmentation (STS) — single-pass grapheme, word, and sentence boundaries over the 64-bit SUCS space.'
    crumbName = 'SUAS-006 — STS'
    crumbs  = @( @{ label = 'Technical Reports'; href = 'index.html' } )
    h1      = 'SUAS-006 &mdash; System <span class="grad">Text Segmentation</span>'
    subtitle= 'The sixth SuperUnicode Architecture Standard: a zero-allocation, single-pass state machine for grapheme, word, and sentence boundaries (a UAX #29 analog).'
    body    = @(
        @{ t = 'p'; html = 'SUAS-006 (STS) is the sixth SuperUnicode Architecture Standard. It defines the <strong>grapheme cluster</strong>, <strong>word</strong>, and <strong>sentence</strong> tiers over the full 64-bit SUCS space (<span class="mono">sucs_ex_char_t</span>) via a single <strong>single-pass, zero-allocation state machine</strong> (a UAX #29 analog). Where UAX #29 relies on multi-scan rulesets with unbounded look-ahead, STS feeds one codepoint at a time and returns the boundary status of the position just resolved in O(1), carrying all necessary context in a bounded register. It powers cursor movement, selection, line wrapping, and paragraph parsing in the kernel compositor and terminal shell.' }
        @{ t = 'h2'; html = 'Three tiers, one engine' }
        @{ t = 'table'; head = @('Tier', 'Unit', 'Consumers'); rows = @(
            @('<span class="mono">GRAPHEME</span>', 'User-perceived character', 'Cursor movement, backspace, selection extent')
            @('<span class="mono">WORD</span>', 'Word / token', 'Double-click selection, spell-check, search')
            @('<span class="mono">SENTENCE</span>', 'Sentence / paragraph', 'Paragraph parsing, TTS, diff granularity')
        ) }
        @{ t = 'h2'; html = 'Boundary status' }
        @{ t = 'table'; head = @('Status', 'Value', 'Meaning'); rows = @(
            @('<span class="mono">BOUNDARY_NONE</span>', '0', 'Do not break; the two sides form one unit')
            @('<span class="mono">BOUNDARY_ALLOW</span>', '1', 'A unit boundary exists here (opportunity)')
            @('<span class="mono">BOUNDARY_MUST</span>', '2', 'A hard boundary is required')
        ) }
        @{ t = 'h2'; html = 'SCP segmentation directives' }
        @{ t = 'p'; html = 'Explicit in-band control lives in the block <span class="mono">0x00110040&ndash;0x0011004F</span>, disjoint from the SBR break block (<span class="mono">0x00110020</span>) and the math directives (<span class="mono">0x00110030</span>). Directives are transparent: they are never pushed onto the window and never mutate the carried context.' }
        @{ t = 'table'; head = @('Directive', 'Codepoint', 'Effect'); rows = @(
            @('<span class="mono">SCP_STS_BREAK_FORCE</span>', '0x00110040', 'Force a boundary (<span class="mono">MUST</span>)')
            @('<span class="mono">SCP_STS_BREAK_BLOCK</span>', '0x00110041', 'Prohibit a boundary (<span class="mono">NONE</span>)')
            @('<span class="mono">SCP_STS_BREAK_ALLOW</span>', '0x00110042', 'Permit a boundary (<span class="mono">ALLOW</span>)')
        ) }
        @{ t = 'h2'; html = 'Bounded carried context' }
        @{ t = 'p'; html = 'Every piece of state UAX #29 would recover by rescanning is held in a fixed-size register inside <span class="mono">suas_sts_state_t</span>: regional-indicator parity, the emoji ZWJ sequence, the Indic linker run, the effective word class for the WB4 &ldquo;ignore&rdquo; rule, and the sentence terminator run. Together with the <span class="mono">SUAS_STS_WINDOW_SIZE</span> (4) codepoint ring, the engine is total and allocation-free over arbitrarily long runs.' }
        @{ t = 'table'; head = @('Feature', 'Status'); rows = @(
            @('Single-pass, O(1) per codepoint', 'Yes')
            @('Grapheme, word, and sentence tiers', 'Yes')
            @('Grapheme rules GB1&ndash;GB999 (incl. GB9c, GB11, GB12/13)', 'Yes')
            @('Word rules WB1&ndash;WB999 (incl. WB4 ignore)', 'Yes')
            @('Sentence rules SB1&ndash;SB998 (incl. SB5 ignore)', 'Yes')
            @('Explicit SCP segmentation directives', 'Yes')
            @('Algorithmic Hangul determination', 'Yes')
            @('Native / plugin codepoints are opaque (total)', 'Yes')
            @('Zero heap allocation, fixed-size state', 'Yes')
            @('Freestanding C99', 'Yes')
        ) }
        @{ t = 'callout'; html = 'Reference implementation: <span class="mono">include/suas/suas_sts.h</span> + <span class="mono">src/suas/suas_sts.c</span> (freestanding C99, no heap), registered in <span class="mono">suas_static</span>.' }
        @{ t = 'note'; html = 'Source of truth: <span class="mono">docs/suas/SUAS-006-sts.md</span> in the repository.' }
    )
}

$Pages['reports/SUAS-007'] = @{
    path    = 'reports/SUAS-007.html'
    sec     = 'reports'
    title   = 'SUAS-007 — System Identifiers and Syntax (SIS)'
    desc    = 'SUAS-007: System Identifiers and Syntax (SIS) — single-pass identifier classes and lexical syntax over the SUCS space.'
    crumbName = 'SUAS-007 — SIS'
    crumbs  = @( @{ label = 'Technical Reports'; href = 'index.html' } )
    h1      = 'SUAS-007 &mdash; System <span class="grad">Identifiers and Syntax</span>'
    subtitle= 'The seventh SuperUnicode Architecture Standard: a zero-allocation, single-pass identifier lexical filter (a UAX #31 analog).'
    body    = @(
        @{ t = 'p'; html = 'SUAS-007 (SIS) is the seventh SuperUnicode Architecture Standard. It defines the two structural identifier classes &mdash; <span class="mono">START</span> and <span class="mono">CONTINUE</span> (the <span class="mono">ID_Start</span> / <span class="mono">ID_Continue</span> analogs) &mdash; plus the disjoint lexical categories of whitespace, syntax, and ignorable format controls, over the full SuperUnicode (SUCS) codepoint space. SIS is a <strong>single-pass, zero-allocation lexical filter</strong> (a UAX #31 analog): a caller resolves the class of any codepoint in O(1), or streams codepoints through a fixed-size state machine, with no heap, no regex engine, and no multi-pass scan. It governs symbol naming in the OpenWindows shell, the configuration parsers, and the nVAI scripting runtime.' }
        @{ t = 'h2'; html = 'Identifier classes' }
        @{ t = 'table'; head = @('Class', 'Value', 'Meaning'); rows = @(
            @('<span class="mono">SUAS_SIS_CLASS_INVALID</span>', '0', 'Cannot be part of an identifier')
            @('<span class="mono">SUAS_SIS_CLASS_START</span>', '1', 'Valid as the initial codepoint of an identifier')
            @('<span class="mono">SUAS_SIS_CLASS_CONTINUE</span>', '2', 'Valid only in a subsequent position')
        ) }
        @{ t = 'h2'; html = 'Identifier syntax' }
        @{ t = 'p'; html = '<span class="mono">&lt;Identifier&gt; := &lt;Start&gt; &lt;Continue&gt;*</span> &mdash; a non-empty run of an initial <span class="mono">START</span> codepoint followed by zero or more <span class="mono">CONTINUE</span> codepoints. Under the default profile, <span class="mono">START</span> covers letters, letter numbers, and <span class="mono">Other_ID_Start</span> (plus <span class="mono">U+005F LOW LINE</span>); <span class="mono">CONTINUE</span> adds marks, decimal digits, connector punctuation, and <span class="mono">Other_ID_Continue</span>.' }
        @{ t = 'h2'; html = 'Address-space treatment' }
        @{ t = 'table'; head = @('Zone', 'Range', 'Identifier treatment'); rows = @(
            @('Unicode Bridge', '<span class="mono">0x00000000&ndash;0x0010FFFF</span>', 'UAX #31 classes via curated predicates')
            @('System Control Plane', '<span class="mono">0x00110000&ndash;0x0011FFFF</span>', '<strong>Strictly invalid</strong> inside identifiers')
            @('Native plane', '<span class="mono">0x00120000&ndash;0x7FFFFFEF</span>', 'Admitted (RNUR / RCCR neography)')
            @('Trap slots', '<span class="mono">0x7FFFFFF0&ndash;0x7FFFFFFE</span>', 'Invalid')
            @('Sentinel', '<span class="mono">0x7FFFFFFF</span>', 'Invalid')
            @('extSUCS plugin plane', 'above <span class="mono">0x7FFFFFFF</span>', 'Admitted (mounted plugin characters)')
        ) }
        @{ t = 'h2'; html = 'Lexical syntax categories' }
        @{ t = 'table'; head = @('Category', 'Value', 'Coverage'); rows = @(
            @('<span class="mono">SUAS_SIS_SYNTAX_LITERAL</span>', '0', 'Not whitespace, syntax, or control')
            @('<span class="mono">SUAS_SIS_SYNTAX_WHITESPACE</span>', '1', 'Horizontal space (<span class="mono">U+0009</span>, <span class="mono">U+0020</span>)')
            @('<span class="mono">SUAS_SIS_SYNTAX_LINETERM</span>', '2', 'Line terminators (LF&ndash;CR, NEL, LS, PS)')
            @('<span class="mono">SUAS_SIS_SYNTAX_IGNORABLE</span>', '3', 'Ignorable format controls (<span class="mono">U+200E</span>, <span class="mono">U+200F</span>)')
            @('<span class="mono">SUAS_SIS_SYNTAX_OPERATOR</span>', '4', '<span class="mono">Pattern_Syntax</span>')
            @('<span class="mono">SUAS_SIS_SYNTAX_CONTROL</span>', '5', 'SCP markers, trap slots, the Sentinel')
        ) }
        @{ t = 'h2'; html = 'Profiles' }
        @{ t = 'table'; head = @('Profile', 'Rule', 'Behaviour'); rows = @(
            @('<span class="mono">SUAS_SIS_PROFILE_DEFAULT</span>', 'R1', 'XID_Start / XID_Continue classes')
            @('<span class="mono">SUAS_SIS_PROFILE_IMMUTABLE</span>', 'R2', 'Admit everything except PWS / PS / PUA / surrogate / control / noncharacter')
            @('<span class="mono">SUAS_SIS_PROFILE_HASHTAG</span>', 'R8', '<span class="mono">#</span> start; emoji and <span class="mono">- + _</span> continue')
        ) }
        @{ t = 'h2'; html = 'Streaming state machine' }
        @{ t = 'p'; html = '<span class="mono">suas_sis_state_t</span> is a fixed-size, caller-owned struct of two scalar fields: <span class="mono">is_started</span> and <span class="mono">length</span>. <span class="mono">suas_sis_feed</span> returns true iff the codepoint was validly appended; a codepoint that cannot be appended while an identifier is in progress terminates it and resets the machine. The first codepoint must be a <span class="mono">START</span> character. There is no unbounded history.' }
        @{ t = 'table'; head = @('Feature', 'Status'); rows = @(
            @('Single-pass, O(1) per codepoint', 'Yes')
            @('START / CONTINUE identifier classes', 'Yes')
            @('SCP invariance (strict rejection)', 'Yes')
            @('Native / RNUR / RCCR neography admission', 'Yes')
            @('Pattern_White_Space / Pattern_Syntax split', 'Yes')
            @('Default / Immutable / Hashtag profiles', 'Yes')
            @('Zero heap allocation, fixed-size state', 'Yes')
            @('Freestanding C99', 'Yes')
        ) }
        @{ t = 'callout'; html = 'Reference implementation: <span class="mono">include/suas/suas_sis.h</span> + <span class="mono">src/suas/suas_sis.c</span> (freestanding C99, no heap), registered in <span class="mono">suas_static</span>.' }
        @{ t = 'note'; html = 'Source of truth: <span class="mono">docs/suas/SUAS-007-sis.md</span> in the repository.' }
    )
}

$Pages['reports/SUAS-008'] = @{
    path    = 'reports/SUAS-008.html'
    sec     = 'reports'
    title   = 'SUAS-008 — System Named Character Sequences (SNCS)'
    desc    = 'SUAS-008: System Named Character Sequences (SNCS) — static-registry, O(1) resolution of multi-codepoint symbol names.'
    crumbName = 'SUAS-008 — SNCS'
    crumbs  = @( @{ label = 'Technical Reports'; href = 'index.html' } )
    h1      = 'SUAS-008 &mdash; System <span class="grad">Named Character Sequences</span>'
    subtitle= 'The eighth SuperUnicode Architecture Standard: a zero-allocation, static-registry, O(1) resolver for multi-codepoint symbol names (a UAX #34 analog).'
    body    = @(
        @{ t = 'p'; html = 'SUAS-008 (SNCS) is the eighth SuperUnicode Architecture Standard. It gives a <strong>name</strong> to a <strong>sequence</strong> of codepoints, so that a symbol which is not a single codepoint &mdash; a combining cluster, a ligature, a flag emoji, or a neography logograph &mdash; can be referred to, looked up, and rendered by one stable token. Where UAX #34 defines named character sequences through dynamic string tables, SNCS binds the concept to a <strong>zero-allocation, static lookup table</strong> optimized for the OpenWindows shell and the SUF font engine. A name resolves to its packed codepoint token in O(1), with no heap, no dynamic table growth, and no string parsing.' }
        @{ t = 'h2'; html = 'Dual-registry index' }
        @{ t = 'table'; head = @('Registry', 'Range', 'Purpose'); rows = @(
            @('<span class="mono">SUAS_SNCS_REGISTRY_BRIDGE</span>', '<span class="mono">0x000000&ndash;0x10FFFF</span>', 'Standard compatibility sequences mapped from the Unicode named sequence registry')
            @('<span class="mono">SUAS_SNCS_REGISTRY_NATIVE</span>', '<span class="mono">0x160000&ndash;0x16FFFF</span>', 'RNUR neography compound glyphs, ligatures, and multi-codepoint logographs')
        ) }
        @{ t = 'p'; html = 'RCCR functions separately as an out-of-band metadata/subtag registry for language identification and <strong>allocates no codepoints</strong>; it therefore contributes no SNCS sequences. A sequence is classified as Native iff any of its codepoints lies in the Native plane.' }
        @{ t = 'h2'; html = 'The static token' }
        @{ t = 'p'; html = 'A resolved sequence is a fixed-size, stack-allocatable token. Codepoints are 64-bit (<span class="mono">sucs_ex_char_t</span>), so one token spans the Unicode Bridge, the System Control Plane, the Native plane, and the unbounded extSUCS plugin space uniformly. A named character sequence is always two or more codepoints. The name limit was raised from the seeding sketch&rsquo;s 32 bytes to 64, because 32 truncates most Unicode named-sequence names.' }
        @{ t = 'spec'; html = '#define SUAS_SNCS_MAX_SEQUENCE_LEN 8U<br>#define SUAS_SNCS_MAX_NAME_LEN     64U<br><br>typedef struct {<br>    char           name[SUAS_SNCS_MAX_NAME_LEN];<br>    sucs_ex_char_t codepoints[SUAS_SNCS_MAX_SEQUENCE_LEN];<br>    uint8_t        length;<br>} suas_named_sequence_t;' }
        @{ t = 'h2'; html = 'Name rules' }
        @{ t = 'table'; head = @('Rule', 'Subject'); rows = @(
            @('<span class="mono">SNCS-R1</span>', 'Only A&ndash;Z, digits 0&ndash;9 (not first in a word), SPACE, and HYPHEN-MINUS')
            @('<span class="mono">SNCS-R2</span>', 'One name per sequence; unique within the SUCS namespace for names')
            @('<span class="mono">SNCS-R3</span>', 'Unique even ignoring SPACE, medial HYPHEN-MINUS, and <span class="mono">LETTER</span> / <span class="mono">CHARACTER</span> / <span class="mono">DIGIT</span>')
        ) }
        @{ t = 'h2'; html = 'Resolution' }
        @{ t = 'p'; html = 'The registry is a static array of at most <span class="mono">SUAS_SNCS_REGISTRY_CAPACITY</span> (64) tokens, indexed once into a fixed array of <span class="mono">SUAS_SNCS_INDEX_SIZE</span> (128) open-addressing slots (FNV-1a hash, linear probing). Resolution hashes the query name and probes a bounded number of slots. The index is built lazily on first use and is idempotent; it never grows at run time.' }
        @{ t = 'table'; head = @('Operation', 'Cost'); rows = @(
            @('<span class="mono">suas_sncs_resolve_sequence</span>', 'O(1) expected (bounded probes), no allocation')
            @('<span class="mono">suas_sncs_init_registry</span>', 'O(n) once, n &le; 64, no allocation')
            @('<span class="mono">suas_sncs_sequence_at</span> / <span class="mono">registry_count</span>', 'O(1)')
            @('<span class="mono">suas_sncs_registry_of</span>', 'O(length), length &le; 8')
        ) }
        @{ t = 'table'; head = @('Feature', 'Status'); rows = @(
            @('Static multi-codepoint packing', 'Yes')
            @('O(1) name resolution, no heap', 'Yes')
            @('Dual registry: Bridge + RNUR neography', 'Yes')
            @('SCP / trap / Sentinel invariance', 'Yes')
            @('SNCS-R1 validity and SNCS-R3 uniqueness', 'Yes')
            @('Lazy, idempotent index build', 'Yes')
            @('Freestanding C99', 'Yes')
        ) }
        @{ t = 'callout'; html = 'Reference implementation: <span class="mono">include/suas/suas_sncs.h</span> + <span class="mono">src/suas/suas_sncs.c</span> (freestanding C99, no heap), registered in <span class="mono">suas_static</span>.' }
        @{ t = 'note'; html = 'Source of truth: <span class="mono">docs/suas/SUAS-008-sncs.md</span> in the repository.' }
    )
}
