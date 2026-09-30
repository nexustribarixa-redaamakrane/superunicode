# Technical Reports section

$Pages['reports/index'] = @{
    path    = 'reports/index.html'
    sec     = 'reports'
    title   = 'Technical Reports — SUTR'
    desc    = 'The SUTR-0 through SUTR-7 specifications of SuperUnicode.'
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
            @('<span class="mono">SUTR-0</span>', '0.1.0', '<a href="SUTR-0.html">SUCS Core</a> &mdash; hierarchy, three-space layout, SCP, Traps, Sentinel')
            @('<span class="mono">SUTR-1</span>', '0.1.0', '<a href="SUTR-1.html">SUTF</a> &mdash; SUCS UTF-8/16/32 framing')
            @('<span class="mono">SUTR-2</span>', '0.1.0', '<a href="SUTR-2.html">SUCA</a> &mdash; SuperUnicode Collation Algorithm')
            @('<span class="mono">SUTR-3</span>', '0.1.0', '<a href="SUTR-3.html">SuperUnicode Storage</a> &mdash; OWFS/USFS partition policy')
            @('<span class="mono">SUTR-4</span>', '0.1.0', '<a href="SUTR-4.html">ExtSUCS Transport</a> &mdash; vector, vsutf, e-SUST')
            @('<span class="mono">SUTR-5</span>', '0.1.0', '<a href="SUTR-5.html">Plugin Lifecycle &amp; Blob Format</a> &mdash; stage, checksum gate, mount, registry')
            @('<span class="mono">SUTR-6</span>', '0.1.0', '<a href="SUTR-6.html">Unicode Compatibility Bridge</a> &mdash; the permanent 1:1 guarantee')
            @('<span class="mono">SUTR-7</span>', '0.1.0', '<a href="SUTR-7.html">SUCEM</a> &mdash; the SuperUnicode Character Encoding Model: ACR &rarr; SUCS &rarr; SUTF &rarr; SUST')
        ) }
        @{ t = 'h2'; html = 'Core invariants every report honors' }
        @{ t = 'ul'; items = @(
            '<span class="mono">SUCS_CP</span> is 31-bit; <span class="mono">sucs_ex_char_t</span> is 64-bit.'
            '<span class="mono">0x000000&ndash;0x10FFFF</span> is permanently 1:1 with Unicode.'
            '<span class="mono">0x7FFFFFFF</span> is the Sentinel; no allocation exists at or below it.'
            'Plugin codepoints start strictly above <span class="mono">0x7FFFFFFF</span> and mount only after the checksum gate.'
        ) }
    )
}

$Pages['reports/SUTR-0'] = @{
    path    = 'reports/SUTR-0.html'
    sec     = 'reports'
    title   = 'SUTR-0 — SUCS Core'
    desc    = 'SUCS Core: hierarchy, three-space layout, SCP, Traps, Sentinel.'
    crumbName = 'SUTR-0 — SUCS Core'
    crumbs  = @( @{ label = 'Technical Reports'; href = 'index.html' } )
    h1      = 'SUTR-0 &mdash; SUCS <span class="grad">Core</span>'
    subtitle= 'The foundational layout of the Base SuperUnicode Character System.'
    body    = @(
        @{ t = 'p'; html = 'This report defines the Base SUCS codepoint space, its hierarchy, its three spaces, and the reserved terminal region.' }
        @{ t = 'h2'; html = 'Space' }
        @{ t = 'spec'; html = 'SUCS_CP        uint32_t, 31 significant bits<br>range          0x00000000 &ndash; 0x7FFFFFFF<br>hierarchy      codepoint &lt; block &lt; range &lt; plane &lt; district &lt; zone &lt; territory' }
        @{ t = 'h2'; html = 'Spaces' }
        @{ t = 'table'; head = @('Space', 'Range', 'Owner'); rows = @(
            @('Unicode Compatibility Space', '<span class="mono">0x000000&ndash;0x10FFFF</span>', '1:1 with Unicode, permanent')
            @('System Control Plane', '<span class="mono">0x110000&ndash;0x11FFFF</span>', 'BANcode Registry (B+, W+, C+, S+)')
            @('Native SuperUnicode Space', '<span class="mono">0x120000&ndash;0x7FFFFFFF</span>', 'OpenWindows allocations')
        ) }
        @{ t = 'h2'; html = 'Terminal region' }
        @{ t = 'ul'; items = @(
            'Traps: <span class="mono">0x7FFFFFF0&ndash;0x7FFFFFFE</span>'
            'Sentinel: <span class="mono">0x7FFFFFFF</span>'
            'No allocation exists at or below the Sentinel.'
        ) }
    )
}

$Pages['reports/SUTR-1'] = @{
    path    = 'reports/SUTR-1.html'
    sec     = 'reports'
    title   = 'SUTR-1 — SUTF'
    desc    = 'SUTF: the SuperUnicode Character Encoding Forms, SUTF-8/16/4/2 and vSUTF.'
    crumbName = 'SUTR-1 — SUTF'
    crumbs  = @( @{ label = 'Technical Reports'; href = 'index.html' } )
    h1      = 'SUTR-1 &mdash; <span class="grad">SUTF</span>'
    subtitle= 'The SuperUnicode Character Encoding Forms: SUTF-8, SUTF-16, SUTF-4, SUTF-2 and vSUTF.'
    body    = @(
        @{ t = 'p'; html = 'SUTF is <strong>Level 3</strong> of the <a href="SUTR-7.html">SUCEM</a> model: a character encoding form maps the 31-bit <span class="mono">SUCS_CP</span> values of the coded character set to sequences of code units. A <em>code unit</em> is an integer of a specified binary width. All SUTF forms are lossless over the full Base space, and every one of them is <strong>endian-neutral</strong> &mdash; none of them states a byte order.' }
        @{ t = 'table'; head = @('Form', 'Code unit', 'Covers', 'Shape'); rows = @(
            @('<span class="mono">SUTF-8</span>', 'byte', 'Full 31-bit space', 'Variable, <strong>1&ndash;6 bytes</strong>. Standard UTF-8 parity to <span class="mono">0x10FFFF</span>, extending to 6 bytes for the native extended planes, per <span class="mono">sutf8.h</span>.')
            @('<span class="mono">SUTF-16</span>', '16-bit word', 'Full 31-bit space', 'Variable, 1&ndash;2 words: one to <span class="mono">0x7FFF</span>, two above. <strong>No surrogates</strong> &mdash; <span class="mono">0xD800</span>&ndash;<span class="mono">0xDFFF</span> are valid PUA codepoints.')
            @('<span class="mono">SUTF-4</span>', '4-bit nibble', 'Full 31-bit space', 'A fixed count of hex nibbles; for console and bus debugging.')
            @('<span class="mono">SUTF-2</span>', '2-bit frame', 'Full 31-bit space', 'Symbol-frame transformation for narrow IPC channels.')
            @('<span class="mono">vSUTF</span>', 'byte', 'Full 64-bit ExtSUCS space', 'Variable-length streaming form with a Base-SUCS fast path, per <span class="mono">vsutf.h</span>.')
        ) }
        @{ t = 'h2'; html = 'What SUTF deliberately does not decide' }
        @{ t = 'p'; html = 'Because a CEF is a transformation and not a serialization, SUTF says nothing about byte order, memory layout, register alignment or frame structure. That is <strong>Level 4</strong>, and it lives in the SUST family.' }
        @{ t = 'ul'; items = @(
            'SUTF-16 needs an explicit order decision to become a byte stream. <span class="mono">SUST-16</span> makes it: the canonical <strong>big-endian</strong> serialization of the SUTF-16 word stream. There is no &ldquo;SUTF-16-LE&rdquo; &mdash; little-endian is a different CES, not a different CEF.'
            'Fixed-width 32-bit, 64-bit, 128-bit, 256-bit, 512-bit and arbitrary <em>N</em>-word framings are likewise <span class="mono">SUST-32/64/128/256/512/N</span>, defined in <span class="mono">sustfixed.h</span>. A fixed 32-bit form is therefore <strong>not</strong> an SUTF form, and neither is it little-endian by default.'
            'SUTF-8 is byte-oriented, so its CES is the identity and no <span class="mono">SUST-8</span> exists.'
        ) }
        @{ t = 'note'; html = 'The operative test: <em>if the processor byte order changed, would this format change?</em> No &rarr; SUTF. Yes &rarr; SUST. See <a href="SUTR-7.html">SUTR-7</a> for why the two are separate modules.' }
        @{ t = 'callout'; html = 'Reference implementation: <a href="../modules/sutf/index.html">modules/sutf</a>. Data: <span class="mono">Public/0.1.0/sutf/</span>. Conformance: <span class="mono">test_sutf_all</span>.' }
    )
}

$Pages['reports/SUTR-2'] = @{
    path    = 'reports/SUTR-2.html'
    sec     = 'reports'
    title   = 'SUTR-2 — SUCA'
    desc    = 'SUCA: the SuperUnicode Collation Algorithm (formalized by SUTS-001).'
    crumbName = 'SUTR-2 — SUCA'
    crumbs  = @( @{ label = 'Technical Reports'; href = 'index.html' } )
    h1      = 'SUTR-2 &mdash; <span class="grad">SUCA</span>'
    subtitle= 'The SuperUnicode Collation Algorithm.'
    body    = @(
        @{ t = 'p'; html = 'Collation is the ordering contract for SuperUnicode strings. SUCA is now a full UCA(UTS #10)-equivalent multilevel algorithm, formally specified by <a href="SUTS-001.html">SUTS-001</a> and implemented by the freestanding <span class="mono">suts_suca</span> reference.' }
        @{ t = 'ul'; items = @(
            '<strong>Multilevel</strong> &mdash; L1 (base), L2 (accent), L3 (case), L4 (variable), identical (NFD codepoint tie-break).'
            '<strong>Canonical equivalence</strong> &mdash; input is NFD-normalized before mapping (e.g. <span class="mono">role &lt; r&ocirc;le &lt; roles</span>, and precomposed &asymp; decomposed).'
            '<strong>Contractions &amp; expansions</strong> &mdash; e.g. <span class="mono">c h</span> as one letter; <span class="mono">&#339; &asymp; oe</span>.'
            '<strong>Variable weighting</strong> &mdash; shifted (default), blanked, non-ignorable, shift-trimmed; SCP controls ignorable at L1&ndash;L3.'
            '<strong>Backward secondary</strong> &mdash; French dictionary order.'
            '<strong>Implicit weights</strong> &mdash; Unassigned/Han/Native/plugin codepoints get algorithmic primaries from the 64-bit codepoint (monotonic; plugin space above Base).'
        ) }
        @{ t = 'callout'; html = 'The complete normative algorithm, weight scheme, data files and conformance matrix are in <a href="SUTS-001.html">SUTS-001 — SuperUnicode Collation Algorithm</a>.' }
        @{ t = 'note'; html = 'Data: <span class="mono">Public/0.1.0/collation/SUCA.txt</span> (Base) and <span class="mono">collation/ExtUCA.txt</span> (Extended) &mdash; both extSUCS-compatible.' }
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

$Pages['reports/SUTR-3'] = @{
    path    = 'reports/SUTR-3.html'
    sec     = 'reports'
    title   = 'SUTR-3 — SuperUnicode Storage'
    desc    = 'SuperUnicode Storage: the OWFS/USFS partition policy.'
    crumbName = 'SUTR-3 — SuperUnicode Storage'
    crumbs  = @( @{ label = 'Technical Reports'; href = 'index.html' } )
    h1      = 'SUTR-3 &mdash; SuperUnicode <span class="grad">Storage</span>'
    subtitle= 'Partitions, formats and the mount policy.'
    body    = @(
        @{ t = 'p'; html = 'Storage is governed by the OpenWindows Storage specification. A <strong>SuperUnicode Partition</strong> is a removable filesystem whose layout is declared by SuperUnicode machine instructions.' }
        @{ t = 'table'; head = @('Format', 'Description'); rows = @(
            @('<span class="mono">OWFS</span>', 'Native drive filesystem; data starts at offset <span class="mono">0x10000</span>; CRC32c + Fletcher-64 integrity; <span class="mono">ow_sec</span> identity; optional ChaCha20 at-rest encryption.')
            @('<span class="mono">USFS</span>', 'Portable external-media filesystem.')
        ) }
        @{ t = 'h2'; html = 'Policy summary' }
        @{ t = 'ul'; items = @(
            'Base SuperUnicode Partitions may be OWFS or USFS, and are used for bugfix and rescue payloads only.'
            'Plugin Partitions are Extended-only and MUST be OWFS &mdash; never USFS.'
            'All partitions mount read-only; plugin partitions mount only after the boot checksum gate.'
        ) }
        @{ t = 'note'; html = 'Full policy text: <span class="mono">partitions/spec.txt</span> in both /Public trees.' }
    )
}

$Pages['reports/SUTR-4'] = @{
    path    = 'reports/SUTR-4.html'
    sec     = 'reports'
    title   = 'SUTR-4 — ExtSUCS Transport'
    desc    = 'ExtSUCS Transport: vector, vsutf and e-SUST framing.'
    crumbName = 'SUTR-4 — ExtSUCS Transport'
    crumbs  = @( @{ label = 'Technical Reports'; href = 'index.html' } )
    h1      = 'SUTR-4 &mdash; ExtSUCS <span class="grad">Transport</span>'
    subtitle= 'Framing for 64-bit codepoints.'
    body    = @(
        @{ t = 'table'; head = @('Transport', 'Description'); rows = @(
            @('<span class="mono">Vector</span>', 'Length-prefixed sequences of 64-bit <span class="mono">sucs_ex_char_t</span>; one codepoint per little-endian unit.')
            @('<span class="mono">vsutf</span>', 'Variable-length integer framing (LEB128-style) covering the full 64-bit space.')
            @('<span class="mono">e-SUST</span>', 'The fixed-base Extended family, complementing Base SUTF8/16/32, with page-mapped IPC framing (<span class="mono">esust.h</span>).')
        ) }
        @{ t = 'h2'; html = 'Requirements' }
        @{ t = 'ul'; items = @(
            'Every transport must represent any value from <span class="mono">0</span> to <span class="mono">0xFFFFFFFFFFFFFFFF</span>.'
            'Base SUTF8/16/32 values must round-trip unchanged into the Extended transports.'
            'Sentinel semantics carry over: plugin streams terminate with the inherited Sentinel value.'
        ) }
        @{ t = 'note'; html = 'Reference: <a href="../modules/sust/index.html">modules/sust</a>. Data: <span class="mono">Public/0.1.0/transport/</span>.' }
    )
}

$Pages['reports/SUTR-5'] = @{
    path    = 'reports/SUTR-5.html'
    sec     = 'reports'
    title   = 'SUTR-5 — Plugin Lifecycle &amp; Blob Format'
    desc    = 'Plugin Lifecycle and Blob Format: stage, checksum gate, mount, registry, quarantine.'
    crumbName = 'SUTR-5 — Plugin Lifecycle'
    crumbs  = @( @{ label = 'Technical Reports'; href = 'index.html' } )
    h1      = 'SUTR-5 &mdash; Plugin <span class="grad">Lifecycle</span>'
    subtitle= 'How a plugin goes from blob to registered codepoints.'
    body    = @(
        @{ t = 'h2'; html = 'Blob format' }
        @{ t = 'p'; html = 'A plugin blob is one packed file: a 94-byte header (magic <span class="mono">SUCS</span>, blob version, plugin version, 64-byte id, range count, blob size, CRC32c, Fletcher-64), followed by 16-byte little-endian range pairs and the payload.' }
        @{ t = 'h2'; html = 'Gates' }
        @{ t = 'ol'; items = @(
            '<strong>Checksum gate</strong> &mdash; CRC32c and Fletcher-64 over the whole blob (with the checksum fields zeroed).'
            '<strong>Range gate</strong> &mdash; ranges sorted, non-overlapping, and strictly above the base limit.'
            '<strong>Collision gate</strong> &mdash; no overlap with any mounted plugin&rsquo;s ranges.'
            '<strong>Mount gate</strong> &mdash; OWFS only, read-only.'
            '<strong>Register</strong> &mdash; ranges enter ExtSUCS and lookups resolve.'
        ) }
        @{ t = 'h2'; html = 'Status codes' }
        @{ t = 'p'; html = 'The plugin ABI exposes a 14-value status enum. <span class="mono">SUCS_PLUGIN_REBOOT_REQUIRED</span> (13) is returned by staging and signals the mandatory restart. Gate failures return the specific diagnostic state.' }
        @{ t = 'callout'; html = 'Tooling: <a href="../extended/sdk.html">plugin_pack and plugin_verify</a> reproduce the checksum gate offline.' }
    )
}

$Pages['reports/SUTR-6'] = @{
    path    = 'reports/SUTR-6.html'
    sec     = 'reports'
    title   = 'SUTR-6 — Unicode Compatibility Bridge'
    desc    = 'The Unicode Compatibility Bridge: the permanent 1:1 guarantee across 0x000000-0x10FFFF.'
    crumbName = 'SUTR-6 — Unicode Compatibility Bridge'
    crumbs  = @( @{ label = 'Technical Reports'; href = 'index.html' } )
    h1      = 'SUTR-6 &mdash; Unicode <span class="grad">Compatibility Bridge</span>'
    subtitle= 'The permanent 1:1 guarantee.'
    body    = @(
        @{ t = 'p'; html = 'Codepoints <span class="mono">0x000000&ndash;0x10FFFF</span> correspond 1:1 with Unicode codepoints of the same numeric value. This is the <em>Unicode Compatibility Space</em>.' }
        @{ t = 'h2'; html = 'The guarantee' }
        @{ t = 'ul'; items = @(
            'Bidirectional and lossless: <span class="mono">sucs_downcast()</span> of a bridged codepoint yields the identical Unicode value, and every Unicode codepoint has a SuperUnicode identity.'
            'Permanent by position, not by version: the mapping is fixed at release 0.1.0 and does not change when Unicode publishes new versions or new codepoints.'
            'New Unicode codepoints appear at the same numeric values &mdash; the bridge tracks Unicode growth without any remapping.'
        ) }
        @{ t = 'spec'; html = 'SUCS 0x000000 &harr; U+000000   <span class="hl">// bi-directional, lossless</span><br>SUCS 0x10FFFF &harr; U+10FFFF   <span class="hl">// the ceiling of the bridge</span>' }
        @{ t = 'h2'; html = 'Tracked Unicode release' }
        @{ t = 'p'; html = 'The <em>mapping</em> is frozen by position, but the <em>data</em> published about the bridge (block definitions and character names) is regenerated from a concrete Unicode release whenever one is adopted. The current release is queryable at compile time:' }
        @{ t = 'table'; head = @('Macro', 'Value', 'Meaning'); rows = @(
            @('<span class="mono">SUCS_UNICODE_VERSION_MAJOR</span> / <span class="mono">_MINOR</span>', '<span class="mono">18</span> / <span class="mono">0</span>', 'Unicode release the UCS/UCB data is synchronized with')
            @('<span class="mono">SUCS_UCD_UNICODE_VERSION</span>', '<span class="mono">1800</span>', 'Same, packed as <span class="mono">major * 100 + minor</span>')
            @('<span class="mono">SUCS_UCD_BLOCK_COUNT</span>', '<span class="mono">353</span>', 'Unicode blocks in <span class="mono">sucs_compat.h</span>')
            @('<span class="mono">SUCS_UCD_NAME_COUNT</span>', '<span class="mono">41232</span>', 'Named codepoints in <span class="mono">sucs_ucd_names.h</span>')
        ) }
        @{ t = 'note'; html = 'Adopting a new Unicode release regenerates the tables from <span class="mono">unicode.org/Public/&lt;version&gt;/ucd/</span> via <span class="mono">superunicode/tools/gen_ucd_names.py</span>. <span class="mono">test_sucs_ucd</span> pins these values so a partial bump fails the build.' }
        @{ t = 'p'; html = '<b>ExtSUCS shares this data set.</b> <span class="mono">superunicode_extended</span> keeps no UCD tables of its own &mdash; <span class="mono">extsucs_compat.h</span> re-exports the base block table and <span class="mono">extsucs_ucd_names.c</span> delegates name lookups &mdash; so both modules always report the same release via <span class="mono">EXTSUCS_UNICODE_VERSION_MAJOR</span>/<span class="mono">_MINOR</span> and <span class="mono">EXTSUCS_UCD_*_COUNT</span>. <span class="mono">test_extsucs_ucd</span> asserts those match the base values, so a one-sided bump fails the build.' }
        @{ t = 'callout'; html = 'The bridge stops where the SCP begins: above <span class="mono">0x10FFFF</span>, SuperUnicode is fully its own system. <a href="../standard/spaces.html">The three spaces &raquo;</a>' }
    )
}

$Pages['reports/SUTR-7'] = @{
    path    = 'reports/SUTR-7.html'
    sec     = 'reports'
    title   = 'SUTR-7 ' + [char]0x2014 + ' SUCEM'
    desc    = 'SUCEM: the four-level SuperUnicode Character Encoding Model, from abstract character repertoire to serialized transport.'
    crumbName = 'SUTR-7 ' + [char]0x2014 + ' SUCEM'
    crumbs  = @( @{ label = 'Technical Reports'; href = 'index.html' } )
    h1      = 'SUTR-7 &mdash; SuperUnicode <span class="grad">Character Encoding Model</span>'
    subtitle= 'The four levels, and why SUTF and SUST are separate things.'
    body    = @(
        @{ t = 'p'; html = 'This report describes a model for the structure of character encodings. The SuperUnicode Character Encoding Model (SUCEM) places SuperUnicode in the context of other character encodings of all types, and of other encoding models such as the character architecture promoted by the Internet Architecture Board for use on the internet (RFC 2130), or the Character Data Representation Architecture (CDRA) defined by IBM for organizing and cataloging its own proprietary array of character encodings.' }
        @{ t = 'p'; html = 'Like the Unicode Character Encoding Model, SUCEM places the OpenWindows Standard in the context of other character encodings, and it separates abstract characters from physical bits with the same four levels. The SuperUnicode implementation of each level is named below; the level names are the standard <span class="mono">ACR</span>, <span class="mono">CCS</span>, <span class="mono">CEF</span> and <span class="mono">CES</span>, so that SuperUnicode encodings can be described with the same vocabulary as every other encoding on the web.' }
        @{ t = 'spec'; html = 'Level 1   ACR   Abstract Character Repertoire<br>            &nbsp;&nbsp;&nbsp;&nbsp;the set of characters to be encoded &mdash; Unicode, RNUR, SCC, directives<br><br>Level 2   CCS   Coded Character Set (SUCS)<br>            &nbsp;&nbsp;&nbsp;&nbsp;abstract character &rarr; 31-bit codepoint, <span class="mono">0x00000000</span>-<span class="mono">0x7FFFFFFF</span><br><br>Level 3   CEF   Character Encoding Form (SUTF)<br>            &nbsp;&nbsp;&nbsp;&nbsp;codepoint &rarr; sequence of code units, endian-neutral<br><br>Level 4   CES   Character Encoding Scheme &amp; Transport (SUST)<br>            &nbsp;&nbsp;&nbsp;&nbsp;code units &rarr; serialized bytes: order, alignment, framing' }
        @{ t = 'p'; html = 'The critical property of this model is that a <em>character</em> is never confused with a <em>glyph</em>, a <em>code point</em> is never confused with a <em>code unit</em>, and a code unit is never confused with a <em>byte</em>. Each level is a distinct object with distinct invariants, and the SuperUnicode Standard defines each one exactly once.' }

        @{ t = 'h2'; html = 'Level 1 &mdash; Abstract Character Repertoire (ACR)' }
        @{ t = 'p'; html = 'A character repertoire is an unordered set of abstract characters to be encoded, defined by convention rather than by shape. The SuperUnicode repertoire is <strong>open</strong>: it is not fixed at a catalogue number, because it is deliberately designed to grow without ever renumbering what already exists. It has three sources.' }
        @{ t = 'h3'; html = 'The three sources of the repertoire' }
        @{ t = 'table'; head = @('Source', 'What it contributes', 'Registry'); rows = @(
            @('<strong>Unicode</strong>', 'The <span class="mono">0x000000&ndash;0x10FFFF</span> repertoire: every assigned Unicode abstract character, at its Unicode numeric value.', '<a href="SUTR-6.html">SUTR-6</a>')
            @('<strong>RNUR</strong>', 'Original scripts, conlangs and neographies, allocated in the Unicode Private Use Areas under a coordinate-pair system <span class="mono">(Set_Number, Code_Point)</span>.', 'RNUR')
            @('<strong>SCC</strong>', 'The System Control Plane: non-printable, in-band kernel directives, style shifts, layout markers and BANcode trap points.', '<a href="SUTR-0.html">SUTR-0</a>')
        ) }
        @{ t = 'p'; html = 'The <strong>RCCR</strong> (Reddit Conlang Code Registry) assigns BCP 47 language tags to those RNUR-encoded conlangs and conscripts, so that an abstract character can be referred to by name across repertoires and mappings. A conlang receives an <span class="mono">art-x-</span> tag only when no existing BCP 47 or ISO 639 code names its language; a conscript receives an <span class="mono">art-x-sx-</span> tag wherever the RNUR holds the allocation. Because the <span class="mono">art-x-</span> space is unbounded, RCCR has no set layer and resolves every coordinate at runtime &mdash; so a tag survives an RNUR eviction unchanged.' }

        @{ t = 'h3'; html = 'Characters are not glyphs' }
        @{ t = 'p'; html = 'Glyphs are the images that render a character, and the correspondence is not one-to-one. A single <span class="mono">fi</span> sequence may render as one ligature glyph or as two; an accented character may be one glyph or several positioned components; a context-sensitive script may substitute shape, position and width per surrounding glyph. What is encoded is the <em>character sequence</em>. Rendering it is the host&rsquo;s business, and SUCEM deliberately places rendering outside the model, at or above Level 1.' }

        @{ t = 'h3'; html = 'Versioning' }
        @{ t = 'p'; html = 'The Unicode-sourced portion of the repertoire is versioned by the Unicode Standard: the UCD snapshot is queryable at compile time as <span class="mono">SUCS_UNICODE_VERSION_MAJOR</span>/<span class="mono">_MINOR</span> (currently 18.0), with <span class="mono">SUCS_UCD_BLOCK_COUNT</span> blocks and <span class="mono">SUCS_UCD_NAME_COUNT</span> named codepoints. The RNUR-sourced portion is versioned by its own set layers and the Graduation Rule. The repertoire only ever grows; nothing is ever removed, and a new abstract character never displaces an existing one.' }

        @{ t = 'h2'; html = 'Level 2 &mdash; Coded Character Set (SUCS)' }
        @{ t = 'p'; html = 'A coded character set is a mapping from a set of abstract characters to a set of non-negative integers. SUCS is that mapping: it assigns each abstract character a 31-bit <span class="mono">sucs_char_t</span>. The <strong>codespace</strong> is the numerical space it spans.' }
        @{ t = 'spec'; html = 'SUCS_CP      uint32_t carrying 31 significant bits<br>codespace    0x00000000 &ndash; 0x7FFFFFFF   (2^31 = 2,147,483,648 codepoints)<br>hierarchy    codepoint &lt; block &lt; range &lt; plane &lt; district &lt; zone &lt; territory<br>partitioning 128 zones &times; 256 districts &times; 256 planes &times; 256 block offsets' }
        @{ t = 'h3'; html = 'The three spaces' }
        @{ t = 'p'; html = 'The codespace is partitioned into three spaces with different owners and different rules. This is the one place where the SuperUnicode Standard departs from a flat, uniform codespace, and the partition is what makes the rest of the model tractable.' }
        @{ t = 'table'; head = @('Space', 'Range', 'Owner and rule'); rows = @(
            @('<strong>Unicode Compatibility Space</strong>', '<span class="mono">0x000000&ndash;0x10FFFF</span>', '1:1 with Unicode, permanently. New Unicode codepoints appear at the same numeric values; no remapping ever occurs.')
            @('<strong>System Control Plane</strong>', '<span class="mono">0x00110000&ndash;0x0011FFFF</span>', 'In-band, non-printable kernel directives. Contains the BANcode Registry at <span class="mono">0x0011A000&ndash;0x0011AEFF</span>.')
            @('<strong>Native SuperUnicode Space</strong>', '<span class="mono">0x00120000&ndash;0x7FFFFFFF</span>', 'OpenWindows allocations, reached through RNUR coordinates and RCCR tags.')
        ) }
        @{ t = 'h3'; html = 'Non-contiguity is intentional' }
        @{ t = 'p'; html = 'The codespace is deliberately non-contiguous in its <em>use</em>, not in its span. The UTF-16 surrogate range <span class="mono">0xD800&ndash;0xDFFF</span> is <strong>not</strong> excluded: SuperUnicode has no surrogates, so those 2,048 codepoints are ordinary valid codepoints and are allocated to PUA content like any other. The Sentinel and the trap range sit at the very top of the codespace, where no allocation may reach.' }
        @{ t = 'table'; head = @('Reserved region', 'Range', 'Meaning'); rows = @(
            @('Kernel Security Traps', '<span class="mono">0x7FFFFFF0&ndash;0x7FFFFFFE</span>', 'The 15 damage-control trap slots dispatched by a B+ BANcode.')
            @('<strong>Sentinel</strong>', '<span class="mono">0x7FFFFFFF</span>', 'The one in-band invalid value. No allocation exists at or below it.')
        ) }
        @{ t = 'h3'; html = 'ExtSUCS' }
        @{ t = 'p'; html = 'ExtSUCS is a second, unbounded CCS in the same family, carrying codepoints in a 64-bit <span class="mono">sucs_ex_char_t</span> with <strong>zero in-band sentinels</strong> &mdash; every function reports status out of band instead. It inherits the same three-space partition and the same trap range, and adds the plugin space above the Sentinel, which mounts only after the SUTR-5 checksum gate.' }

        @{ t = 'h2'; html = 'Level 3 &mdash; Character Encoding Form (SUTF)' }
        @{ t = 'p'; html = 'A character encoding form maps the integers of a CCS to sequences of code units. A code unit is an integer of a specified binary width. SUTF defines this level and stops there: <strong>an SUTF is endian-neutral</strong>. It says how many code units a codepoint becomes and what they contain, and nothing about how those units are laid out in memory or on a wire.' }
        @{ t = 'table'; head = @('Form', 'Unit', 'Covers', 'Shape'); rows = @(
            @('<span class="mono">SUTF-8</span>', 'byte', 'Full 31-bit space', 'Variable, 1&ndash;6 bytes per codepoint.')
            @('<span class="mono">SUTF-16</span>', '16-bit word', 'Full 31-bit space', 'Variable, 1&ndash;2 words. <strong>No surrogates</strong>: <span class="mono">0xD800&ndash;0xDFFF</span> are ordinary codepoints. One word to <span class="mono">0x7FFF</span>, two above.')
            @('<span class="mono">SUTF-4</span>', '4-bit nibble', 'Full 31-bit space', 'Fixed count of nibbles; for console and bus debugging.')
            @('<span class="mono">SUTF-2</span>', '2-bit frame', 'Full 31-bit space', 'Symbol-frame transformation for narrow IPC channels.')
            @('<span class="mono">vSUTF</span>', 'byte', 'Full 64-bit ExtSUCS space', 'Variable-length streaming form, with a Base-SUCS fast path.')
        ) }
        @{ t = 'p'; html = 'Fixed-width and variable-width forms coexist deliberately. SUTF-16 carries the whole 31-bit space in at most two words, so no fixed 32-bit form is required for Base SUCS; but for the Native space, where a codepoint is a full 31 bits, a fixed-width 32-bit CEF is the only form with a constant cost, which is what the SUST vector transports consume.' }

        @{ t = 'h2'; html = 'Level 4 &mdash; Character Encoding Scheme &amp; Transport (SUST)' }
        @{ t = 'p'; html = 'A character encoding scheme is a reversible transformation from sequences of code units to serialized sequences of bytes. SUST is that level, and it is the only level in SuperUnicode that knows about byte order, memory layout, register alignment and frame structure. The distinction is not a distinction of degree: it is the difference between a transformation and a serialization.' }
        @{ t = 'table'; head = @('Transport', 'Bytes', 'Role'); rows = @(
            @('<span class="mono">SUST-16</span>', '2 or 4', 'Canonical <strong>big-endian</strong> serialization of the SUTF-16 word stream. The reference SUST.')
            @('<span class="mono">SUST-32</span>', '4', 'Fixed-width Base-SUCS fast-path container.')
            @('<span class="mono">SUST-64</span>', '8', 'SIMD and AI tensor slot; full 64-bit ExtSUCS range.')
            @('<span class="mono">SUST-128</span>', '16', 'SSE / NEON vector register slot.')
            @('<span class="mono">SUST-256</span>', '32', 'AVX-256 vector register slot, zero-padded big-endian.')
            @('<span class="mono">SUST-512</span>', '64', 'AVX-512 vector register slot.')
            @('<span class="mono">SUST-N</span>', 'N', 'The general fixed-width rule for any further alignment.')
            @('<span class="mono">e-SUST</span>', '6', 'Page-mapped hypervisor IPC: a 4-byte big-endian page index plus a 2-byte offset, over 4,096-codepoint pages.')
        ) }
        @{ t = 'p'; html = 'SUST transports are <strong>simple</strong> CESs: each code unit maps to a unique byte sequence, in order, with no shift mechanism and no escaping. There is no byte order mark anywhere in the SUST family, because SuperUnicode fixes the order per transport instead of marking it in the data &mdash; the transport <em>is</em> the declaration of order.' }

        @{ t = 'h2'; html = 'Why the SUTF and SUST split is normative' }
        @{ t = 'p'; html = 'The repository gives SUTF and SUST separate top-level directories. That separation is not an organizational convenience; it is the direct consequence of the Level 3 / Level 4 boundary, and it follows from one question.' }
        @{ t = 'spec'; html = 'If the processor byte order changed, would this format change?<br><br>&nbsp;&nbsp;NO  &rarr;  Level 3, a CEF.   SUTF-8, SUTF-16, SUTF-4, SUTF-2, vSUTF.<br>&nbsp;&nbsp;YES &rarr;  Level 4, a CES.   SUST-16, SUST-32/64/128/256/512/N, e-SUST.' }
        @{ t = 'p'; html = 'SUTF-8 is byte-oriented and so its CES is trivial, which is why <span class="mono">SUST</span> has no &ldquo;SUST-8&rdquo;: a byte-oriented CEF has an identity serialization. SUTF-16 is <em>not</em> byte-oriented, so it requires an explicit order decision, and that decision &mdash; not the transformation &mdash; is what SUST-16 makes. The two levels compose as a product: one CEF, many CESs; one CES, many CEFs. Keeping them in one library would force every consumer to re-derive which half it was using, and would make &ldquo;is this string portable?&rdquo; answerable only by reading the implementation.' }
        @{ t = 'ul'; items = @(
            '<strong>Different invariants.</strong> A CEF must be lossless and endian-neutral. A CES must be byte-exact and order-explicit. A test that passes for one says nothing about the other.'
            '<strong>Different portability.</strong> SUTF artifacts are portable between machines of any endianness. SUST artifacts are portable only between machines that agree on the transport.'
            '<strong>Different hardware consumers.</strong> A CEF feeds a decoder loop. A CES feeds a memory-mapped vector register, a page table, or a hypervisor frame.'
        ) }
        @{ t = 'callout'; html = 'Concretely: a SUCS codepoint in memory, cast to a byte array, is not yet a SUST stream &mdash; casting a wider type to bytes is a serialization, and serialization is Level 4. This is the same trap the Unicode model sets with the UTF-16 form / UTF-16LE scheme distinction.' }

        @{ t = 'h2'; html = 'Anchoring the System Control Plane' }
        @{ t = 'p'; html = 'The SCP is where the four levels are easiest to conflate, because SCP codepoints are simultaneously addresses, directives, and protocol elements. SUCEM separates the three roles precisely, and that separation is what makes the kernel damage-control workflow well-defined.' }
        @{ t = 'table'; head = @('Level', 'What the SCP is there'); rows = @(
            @('<strong>Level 1 &mdash; ACR</strong>', 'A set of <em>non-printable abstract directives</em>: fatal kernel error, warning, command, soft signal. They are abstract, and they are not glyphs; nothing renders them.')
            @('<strong>Level 2 &mdash; SUCS</strong>', 'A set of <em>addresses</em>. The BANcode Registry occupies <span class="mono">0x0011A000&ndash;0x0011AEFF</span>, partitioned into B+, W+, C+ and S+ clusters of 2,048, 1,024, 512 and 256 codepoints. A BANcode is purely a number here.')
            @('<strong>Level 3 &mdash; SUTF</strong>', 'A set of <em>transformed integers</em>, identical in kind to every other codepoint. An SCP directive is framed by the same SUTF-8 or SUTF-16 rules as a letter.')
            @('<strong>Level 4 &mdash; SUST</strong>', 'A set of <em>framed packets</em>. Under e-SUST a BANcode becomes 6 bytes: a 4-byte big-endian page index and a 2-byte offset. Under SUST-16 it becomes a big-endian word.')
        ) }
        @{ t = 'h3'; html = 'The dispatch workflow, level by level' }
        @{ t = 'ol'; items = @(
            '<strong>Crash.</strong> The kernel faults and raises a fatal B+ BANcode. At Level 2 this is a single address in the B+ cluster; at Level 1 it is the abstract directive <em>fatal kernel error</em>.'
            '<strong>Resolve.</strong> The handler calls <span class="mono">sucs_bancode_to_trap()</span>, mapping the BANcode to its Kernel Security Trap address in <span class="mono">0x7FFFFFF0&ndash;0x7FFFFFFE</span>. Still Level 2 &mdash; a number becomes a different number.'
            '<strong>Address.</strong> The trap slot identifies a Damage Control Handler governing a cluster of 128 BANcodes. <span class="mono">sucs_trap_to_bancode_range()</span> inverts the mapping.'
            '<strong>Serialize.</strong> Only now does Level 4 enter: the chosen transport frames the directive and its payload for the dump destination.'
        ) }
        @{ t = 'note'; html = 'A load-bearing consequence: because SCP codepoints are in-band <em>addresses</em> at Level 2, they must survive canonical transformation. SUCF guarantees the SCP, the trap range and the Sentinel pass through normalization untouched and in their original stream position, so a kernel directive is never decomposed, reordered, or dropped by a text pipeline.' }
        @{ t = 'p'; html = 'The same discipline applies to the Sentinel. Because <span class="mono">0x7FFFFFFF</span> is an in-band invalid value, it is a Level 2 fact that constrains Levels 3 and 4: no SUTF form may be defined to emit it as data, and every SUST transport must be able to represent it as a terminator. In ExtSUCS this constraint is dissolved &mdash; the space is unbounded, so <span class="mono">0x7FFFFFFF</span> becomes an ordinary codepoint and errors move entirely out of band.' }

        @{ t = 'h2'; html = 'Character Maps and Transfer Encoding Syntax' }
        @{ t = 'p'; html = 'Two related concepts sit outside the four levels proper, and SuperUnicode has a direct analogue of each.' }
        @{ t = 'ul'; items = @(
            '<strong>Character Map (CM)</strong> &mdash; a mapping from abstract characters straight to serialized bytes, collapsing all four levels into one operation. SuperUnicode ships the Unicode bridge as an identity CM over <span class="mono">0x000000&ndash;0x10FFFF</span>, published as <span class="mono">Public/0.1.0/mappings/UNICODE.txt</span>. The <span class="mono">unicode2superunicode</span> and <span class="mono">superunicode2unicode</span> tools are CM implementations bridging SUCS and UTF-8 end to end.'
            '<strong>Transfer Encoding Syntax (TES)</strong> &mdash; a reversible transform of encoded data that may not contain text at all. e-SUST is SuperUnicode&rsquo;s TES: it is a framing over already-encoded data whose purpose is transport across an isolation boundary, not the representation of characters. No compression scheme is defined at any level; compression is deliberately left to the storage layer in <a href="SUTR-3.html">SUTR-3</a>.'
        ) }

        @{ t = 'h2'; html = 'Definitions and Acronyms' }
        @{ t = 'table'; head = @('Term', 'Meaning'); rows = @(
            @('<span class="mono">ACR</span>', 'Abstract Character Repertoire &mdash; Level 1')
            @('<span class="mono">CCS</span>', 'Coded Character Set &mdash; Level 2')
            @('<span class="mono">CEF</span>', 'Character Encoding Form &mdash; Level 3')
            @('<span class="mono">CES</span>', 'Character Encoding Scheme &mdash; Level 4')
            @('<span class="mono">CM</span>', 'Character Map &mdash; all four levels in one operation')
            @('<span class="mono">TES</span>', 'Transfer Encoding Syntax')
            @('<span class="mono">Codespace</span>', 'The numerical space spanned by the integers of a CCS')
            @('<span class="mono">Code unit</span>', 'The minimal bit combination representing one unit of encoded text, of a specified width')
            @('<span class="mono">Sentinel</span>', '<span class="mono">0x7FFFFFFF</span> &mdash; the in-band invalid SUCS codepoint')
        ) }
        @{ t = 'callout'; html = 'Related reports: <a href="SUTR-0.html">SUTR-0 SUCS Core</a> &middot; <a href="SUTR-1.html">SUTR-1 SUTF</a> &middot; <a href="SUTR-3.html">SUTR-3 SuperUnicode Storage</a> &middot; <a href="SUTR-4.html">SUTR-4 ExtSUCS Transport</a> &middot; <a href="SUTR-6.html">SUTR-6 Unicode Compatibility Bridge</a>' }
        @{ t = 'note'; html = 'Reference implementations: <a href="../modules/unicode/index.html">superunicode</a> (ACR + CCS), <a href="../modules/sutf/index.html">sutf</a> (CEF), <a href="../modules/sust/index.html">sust</a> (CES). Conformance is exercised by the <span class="mono">test_sucs_ucd</span>, <span class="mono">test_extsucs_ucd</span>, <span class="mono">test_sutf_all</span> and <span class="mono">test_sust_all</span> suites.' }
    )
}
