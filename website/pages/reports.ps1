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
            @('<span class="mono">SUTS-002</span>', 'Draft &mdash; Authoring', '<a href="SUTS-002.html">SuperUnicode Regular Expressions (SRE)</a> &mdash; zero-allocation, single-pass regex engine over 64-bit extSUCS')
            @('<span class="mono">SUTS-003</span>', 'Draft &mdash; Authoring', '<a href="SUTS-003.html">System Locale Data &amp; Formatting (SLDF)</a> &mdash; binary-compiled, zero-allocation locale schema; O(1) RCCR-subtag resolution')
            @('<span class="mono">SUTS-004</span>', 'Draft &mdash; Authoring', '<a href="SUTS-004.html">System Ideographic Variation Database (SIVD)</a> &mdash; binary-compiled variation sequences; O(1) (base, selector) &rarr; glyph')
            @('<span class="mono">SUTS-005</span>', 'Draft &mdash; Authoring', '<a href="SUTS-005.html">System Character Security Specification (SCSS)</a> &mdash; binary-compiled homograph detection, identifier admissibility, and script isolation')
            @('<span class="mono">SUAS-001</span>', 'Draft &mdash; Ratified', '<a href="SUAS-001.html">Structural Directional Framing (SDF)</a> &mdash; core BiDi architecture, scope isolation and glyph mirroring')
            @('<span class="mono">SUAS-002</span>', 'Draft &mdash; Ratified', '<a href="SUAS-002.html">System Glyph Width &amp; Monospace Grid (SGW)</a> &mdash; fixed-cell terminal metrics, EAW-style width over 64-bit SUCS')
            @('<span class="mono">SUAS-003</span>', 'Draft &mdash; Ratified', '<a href="SUAS-003.html">System Boundary &amp; Line Break Rules (SBR)</a> &mdash; single-pass line breaking over 64-bit SUCS')
            @('<span class="mono">SUAS-004</span>', 'Draft &mdash; Ratified', '<a href="SUAS-004.html">SuperUnicode Canonical Forms (SUCF)</a> &mdash; dual-target canonical (de)composition, zero allocation')
            @('<span class="mono">SUAS-005</span>', 'Draft &mdash; Ratified', '<a href="SUAS-005.html">System Script Property (SSP)</a> &mdash; range-dispatched script resolution and font-fallback routing')
            @('<span class="mono">SUAS-006</span>', 'Draft &mdash; Ratified', '<a href="SUAS-006.html">System Text Segmentation (STS)</a> &mdash; single-pass grapheme, word, and sentence boundaries over 64-bit SUCS')
            @('<span class="mono">SUAS-007</span>', 'Draft &mdash; Ratified', '<a href="SUAS-007.html">System Identifiers and Syntax (SIS)</a> &mdash; single-pass identifier classes and lexical syntax over the SUCS space')
            @('<span class="mono">SUAS-008</span>', 'Draft &mdash; Ratified', '<a href="SUAS-008.html">System Named Character Sequences (SNCS)</a> &mdash; static-registry, O(1) resolution of multi-codepoint symbol names')
            @('<span class="mono">SUAS-009</span>', 'Draft &mdash; Ratified', '<a href="SUAS-009.html">System Ideographic Properties (SIP)</a> &mdash; zero-allocation, O(1) radical-stroke, flags, and variant-linkage extraction')
            @('<span class="mono">SUAS-010</span>', 'Draft &mdash; Authoring', '<a href="SUAS-010.html">System Specification Registry &amp; Cross-Reference Engine (SSRE)</a> &mdash; embedded dependency metadata and build-time linkage validation (a UAX #41 analog)')
            @('<span class="mono">SUAS-011</span>', 'Draft &mdash; Authoring', '<a href="SUAS-011.html">System Character Manifest &amp; Schema Standard (SCMS)</a> &mdash; the line-oriented, binary-mappable character-manifest grammar (a UAX #42 analog)')
            @('<span class="mono">SUAS-012</span>', 'Draft &mdash; Authoring', '<a href="SUAS-012.html">SuperUnicode Character Database (SUCD)</a> &mdash; the verified binary character database, O(1) range lookup and partition proof (a UAX #44 analog)')
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

$Pages['reports/SUTS-002'] = @{
    path    = 'reports/SUTS-002.html'
    sec     = 'reports'
    title   = 'SUTS-002 — SuperUnicode Regular Expressions (SRE)'
    desc    = 'SUTS-002: SuperUnicode Regular Expressions (SRE) — a zero-allocation, single-pass regex engine over the 64-bit extSUCS space.'
    crumbName = 'SUTS-002 — SRE'
    crumbs  = @( @{ label = 'Technical Reports'; href = 'index.html' } )
    h1      = 'SUTS-002 &mdash; SuperUnicode <span class="grad">Regular Expressions</span>'
    subtitle= 'The second SuperUnicode Technical Specification: a UTS #18-analog regex engine with a bounded, allocation-free automaton.'
    body    = @(
        @{ t = 'p'; html = 'SUTS-002 (SRE) is the second SuperUnicode Technical Specification. It is the SuperUnicode analog of <strong>UTS #18</strong> (Unicode Regular Expressions): it adopts the same problem statement and the same Level 1 / Level 2 feature inventory, but reinterprets them over the full 64-bit SuperUnicode codepoint space and adds a third, kernel-specific level for the System Control Plane.' }
        @{ t = 'h2'; html = 'Zero-allocation, single-pass engine' }
        @{ t = 'p'; html = 'Where a conventional engine backtracks over a dynamically grown NFA/DFA, SRE defines a <strong>bounded automaton held entirely in caller-provided storage</strong>. A pattern compiles once into a fixed set of states and edges; matching then simulates that automaton over a fixed-size state subset in linear time &mdash; <span class="mono">O(N)</span> time, <span class="mono">O(1)</span> extra space, with no heap and no backtracking.' }
        @{ t = 'h2'; html = 'Conformance levels' }
        @{ t = 'table'; head = @('Level', 'Name', 'Coverage'); rows = @(
            @('<strong>Level 1</strong>', 'Basic SuperUnicode Support', 'Full 31-bit addressing, SUCD script and category properties, canonical equivalence (SUCF-C), SCP invariance by default')
            @('<strong>Level 2</strong>', 'Extended System &amp; Neography', 'RNUR registry matcher (<span class="mono">\p{RNUR}</span>), SDF directional scope anchors (<span class="mono">\b{LTR}</span>, <span class="mono">\b{RTL}</span>), SBR line and word boundaries')
            @('<strong>Level 3</strong>', 'SCP &amp; Kernel Diagnostics', 'BANcode trap matching (<span class="mono">\p{BANcode}</span>) and explicit SCP directorial in-band anchors (<span class="mono">\p{SCP_DIR_RTL}</span>)')
        ) }
        @{ t = 'h2'; html = 'Relationship to UTS #18' }
        @{ t = 'table'; head = @('Dimension', 'UTS #18', 'SUTS-002 SRE'); rows = @(
            @('Execution engine', 'Backtracking NFA / dynamic heap allocation', 'Zero-allocation linear automaton (O(N))')
            @('Space coverage', '21-bit Unicode plane range (0x0000&ndash;0x10FFFF)', 'Full 31-bit SUCS plus 64-bit ExtSUCS')
            @('Control handling', 'Out-of-band flags or anchors', 'In-band SCP directives and BANcode traps')
            @('Bridge vs Native', 'Standard property sets (\p{L})', 'SUCD lookup for the Bridge + bitmask tests for Native / ExtSUCS')
        ) }
        @{ t = 'h2'; html = 'Property classes' }
        @{ t = 'ul'; items = @(
            '<strong>Bridge</strong> &mdash; SUCD lookup for standard properties: <span class="mono">\p{L}</span>, <span class="mono">\p{N}</span>, <span class="mono">\p{Arabic}</span>, <span class="mono">\p{Han}</span>.'
            '<strong>Native / ExtSUCS / SCP</strong> &mdash; direct bitmask tests: <span class="mono">\p{Native}</span>, <span class="mono">\p{ExtSUCS}</span>, <span class="mono">\p{SCP}</span>, <span class="mono">\p{BANcode}</span>.'
            '<strong>Neography</strong> &mdash; <span class="mono">\p{RNUR}</span>, the SUTR-8 Native-plane neography registry (RCCR is a code registry, not a codepoint registry, and exposes no property).'
        ) }
        @{ t = 'spec'; html = '#define SUTS_SRE_MAX_STATES       64U<br>#define SUTS_SRE_MAX_TRANSITIONS  128U<br>#define SUTS_SRE_MAX_CLASSES      8U<br>#define SUTS_SRE_MAX_CLASS_ITEMS  12U<br><br>typedef uint32_t suts_sre_flags_t;<br><br>#define SUTS_SRE_FLAG_NONE           0x00000000u<br>#define SUTS_SRE_FLAG_IGNORE_CASE    0x00000001u<br>#define SUTS_SRE_FLAG_MATCH_SCP      0x00000002u<br>#define SUTS_SRE_FLAG_SCF_NORMALIZED 0x00000004u<br>#define SUTS_SRE_FLAG_DIR_AWARE      0x00000008u<br>#define SUTS_SRE_FLAG_MULTILINE      0x00000010u' }
        @{ t = 'table'; head = @('API', 'Purpose'); rows = @(
            @('<span class="mono">suts_sre_compile</span>', 'Compile an ASCII-meta pattern into a bounded automaton')
            @('<span class="mono">suts_sre_prop_match</span>', 'SUCD-style property test for a codepoint')
            @('<span class="mono">suts_sre_init_match</span> / <span class="mono">suts_sre_step</span>', 'Streaming match context; feed one codepoint in O(1)')
            @('<span class="mono">suts_sre_match_at</span> / <span class="mono">suts_sre_search</span>', 'Anchored match and leftmost-then-longest search')
        ) }
        @{ t = 'table'; head = @('Feature', 'Status'); rows = @(
            @('Zero heap allocation, no state-table growth', 'Yes')
            @('Linear O(N) matching, no backtracking', 'Yes')
            @('64-bit extSUCS codepoint domain', 'Yes')
            @('Character classes with union / intersection / difference', 'Yes')
            @('Canonical equivalence (SUCF-C) under a flag', 'Yes')
            @('SCP invariance by default; explicit <span class="mono">\p{SCP}</span>', 'Yes')
            @('SDF directional anchors and multiline <span class="mono">^</span> / <span class="mono">$</span>', 'Yes')
        ) }
        @{ t = 'callout'; html = 'Reference implementation: <span class="mono">include/suts/suts_sre.h</span> + <span class="mono">src/suts/suts_sre.c</span> (freestanding C99, no heap), registered in <span class="mono">suts_static</span>.' }
        @{ t = 'note'; html = 'Source of truth: <span class="mono">docs/suts/SUTS-002-sre.md</span> in the repository.' }
    )
}

$Pages['reports/SUTS-003'] = @{
    path    = 'reports/SUTS-003.html'
    sec     = 'reports'
    title   = 'SUTS-003 — System Locale Data & Formatting (SLDF)'
    desc    = 'SUTS-003: System Locale Data & Formatting (SLDF) — a binary-compiled, zero-allocation locale schema with O(1) RCCR-subtag resolution.'
    crumbName = 'SUTS-003 — SLDF'
    crumbs  = @( @{ label = 'Technical Reports'; href = 'index.html' } )
    h1      = 'SUTS-003 &mdash; System Locale Data &amp; <span class="grad">Formatting</span>'
    subtitle= 'The third SuperUnicode Technical Specification: a binary-compiled locale schema for the OpenWindows microkernel.'
    body    = @(
        @{ t = 'p'; html = 'SUTS-003 (SLDF) is the third SuperUnicode Technical Specification and a <strong>core kernel primitive</strong> of the OpenWindows microkernel. It establishes a <strong>binary-compiled, zero-allocation schema</strong> for locale-specific formatting &mdash; number systems, date/time patterns, and collation seeds &mdash; that bypasses text-based markup (such as UTS #35 LDML) in favour of fixed-size, byte-aligned layouts (<span class="mono">sldf_locale_t</span>) optimized for O(1) kernel execution.' }
        @{ t = 'h2'; html = 'Why not LDML' }
        @{ t = 'ul'; items = @(
            '<strong>Compiled binary vs. markup</strong> &mdash; LDML is XML parsed into heap object trees; SLDF is fixed-size C structs in read-only memory, at known offsets, with no parser.'
            '<strong>Zero allocation vs. heap</strong> &mdash; LDML builds patterns, fallback chains, and caches on the fly; SLDF resolves from a static table and a fixed index, never touching a heap.'
            '<strong>Kernel primitive vs. user-space</strong> &mdash; LDML targets application runtimes and browsers; SLDF gives the microkernel, system loggers, and terminal drivers deterministic rules in-band.'
        ) }
        @{ t = 'h2'; html = 'RCCR subtag integration' }
        @{ t = 'p'; html = 'A subsystem requests rules by its <strong>RCCR subtag</strong> (a BCP 47-style language code; RCCR is a code registry and allocates no codepoints). The kernel normalizes the subtag (case-fold and hyphen normalization), hashes it with FNV-1a, and probes a fixed 128-slot open-addressing index built once over the static table. The <span class="mono">lumer</span> and <span class="mono">kupon</span> conlangs resolve exactly like <span class="mono">en-US</span>.' }
        @{ t = 'h2'; html = 'Formatting domains' }
        @{ t = 'table'; head = @('Domain', 'Contents'); rows = @(
            @('<strong>Numeric</strong>', 'Decimal and grouping separators, grouping sizes (3,3 or Indian 3,2), currency symbol and position, negative-sign placement')
            @('<strong>Temporal</strong>', 'Date order vector (YMD / MDY / DMY), separators, 12/24-hour flag, week bounds, long month form')
            @('<strong>Collation</strong>', 'A base weight offset mapped onto the 31-bit SUCS space or the RNUR neography plane, seeding SUCA')
        ) }
        @{ t = 'h2'; html = 'Zero-allocation resolution' }
        @{ t = 'spec'; html = '#define SUTS_SLDF_MAX_TAG_LEN       16U<br>#define SUTS_SLDF_MAX_SYMBOL_LEN    8U<br>#define SUTS_SLDF_REGISTRY_CAPACITY 64U<br>#define SUTS_SLDF_INDEX_SIZE        128U<br><br>typedef struct {<br>    char                   tag_key[SUTS_SLDF_MAX_TAG_LEN];<br>    sldf_numeric_format_t  numeric;<br>    sldf_temporal_format_t temporal;<br>    uint32_t               collation_base_offset;<br>    uint8_t                collation_plane;<br>    uint8_t                is_rtl;<br>    uint8_t                script;<br>    uint8_t                reserved;<br>} sldf_locale_t;' }
        @{ t = 'table'; head = @('Operation', 'Cost'); rows = @(
            @('<span class="mono">sldf_resolve_locale</span>', 'O(1) expected (bounded probes), no allocation')
            @('<span class="mono">sldf_init_registry</span>', 'O(n) once, n &le; 64, no allocation')
            @('<span class="mono">sldf_locale_count</span> / <span class="mono">locale_at</span>', 'O(1)')
            @('<span class="mono">sldf_plane_of</span> / <span class="mono">collation_base</span>', 'O(1)')
        ) }
        @{ t = 'table'; head = @('Feature', 'Status'); rows = @(
            @('Binary, fixed-size locale blocks', 'Yes')
            @('Zero heap allocation on any path', 'Yes')
            @('O(1) expected RCCR-subtag resolution', 'Yes')
            @('Case-insensitive, hyphen-normalized keys', 'Yes')
            @('Bridge + RNUR conlang locales', 'Yes')
            @('SCP-invariant separators/symbols', 'Yes')
            @('64-bit extSUCS compatibility', 'Yes')
        ) }
        @{ t = 'callout'; html = 'Reference implementation: <span class="mono">include/suts/suts_sldf.h</span> + <span class="mono">src/suts/suts_sldf.c</span> (freestanding C99, no heap), registered in <span class="mono">suts_static</span>.' }
        @{ t = 'note'; html = 'Source of truth: <span class="mono">docs/suts/SUTS-003-sldf.md</span> in the repository.' }
    )
}

$Pages['reports/SUTS-004'] = @{
    path    = 'reports/SUTS-004.html'
    sec     = 'reports'
    title   = 'SUTS-004 — System Ideographic Variation Database (SIVD)'
    desc    = 'SUTS-004: System Ideographic Variation Database (SIVD) — a binary-compiled, zero-allocation variation schema with O(1) (base, selector) resolution.'
    crumbName = 'SUTS-004 — SIVD'
    crumbs  = @( @{ label = 'Technical Reports'; href = 'index.html' } )
    h1      = 'SUTS-004 &mdash; System Ideographic <span class="grad">Variation Database</span>'
    subtitle= 'The fourth SuperUnicode Technical Specification: binary-compiled variant sequences for the OpenWindows microkernel and its suf font pipeline.'
    body    = @(
        @{ t = 'p'; html = 'SUTS-004 (SIVD) is the fourth SuperUnicode Technical Specification and a <strong>core kernel primitive</strong> of the OpenWindows microkernel. It establishes a <strong>binary-compiled, zero-allocation schema</strong> for ideographic and stylistic character variant sequences &mdash; mapping a registered variation sequence (a base ideograph followed by a variation selector) to a specific glyph &mdash; that bypasses the text-heavy, multi-megabyte flat files of Unicode&rsquo;s <strong>UTS #37</strong> Ideographic Variation Database in favour of fixed-size, byte-aligned layouts optimized for O(1) kernel execution and font-pipeline lookup.' }
        @{ t = 'h2'; html = 'Why not a flat-file IVD' }
        @{ t = 'ul'; items = @(
            '<strong>Compiled binary vs. text registries</strong> &mdash; UTS #37 is two UTF-8 files (<span class="mono">IVD_Collections.txt</span>, <span class="mono">IVD_Sequences.txt</span>) tokenized at run time; SIVD is fixed-size C structs in read-only memory, at known offsets, with no parser.'
            '<strong>Zero allocation vs. heap</strong> &mdash; a user-space IVD loads the registries into dynamic maps and tables; SIVD resolves from a static table and a fixed index, never touching a heap.'
            '<strong>Kernel primitive vs. user-space</strong> &mdash; the IVD targets layout engines reading files from disk; SIVD gives the microkernel, the <span class="mono">suf</span> font pipeline, and the terminal drivers deterministic variant resolution in-band.'
        ) }
        @{ t = 'h2'; html = 'The dual-plane variation index' }
        @{ t = 'table'; head = @('Plane', 'Range', 'Contents'); rows = @(
            @('<strong>Bridge Variation Space</strong>', '<span class="mono">0x000000&ndash;0x10FFFF</span>', 'Standard Ideographic Variation Sequences over the supplementary selectors <span class="mono">U+E0100&ndash;U+E01EF</span>')
            @('<strong>Native &amp; Neography</strong>', '<span class="mono">0x160000&ndash;0x16FFFF</span>', 'RNUR neography scripts and conlangs: contextual alternates, regional forms, stylistic flourishes')
        ) }
        @{ t = 'h2'; html = 'Binary variation schema' }
        @{ t = 'spec'; html = '#define SUTS_SIVD_MAX_COLLECTION_ID   32U<br>#define SUTS_SIVD_MAX_ENTRY_ID        16U<br>#define SUTS_SIVD_MAX_ID_PATTERN      32U<br>#define SUTS_SIVD_REGISTRY_CAPACITY   128U<br>#define SUTS_SIVD_COLLECTION_CAPACITY 16U<br>#define SUTS_SIVD_INDEX_SIZE          256U<br><br>typedef struct {<br>    sucs_ex_char_t base_codepoint;<br>    sucs_ex_char_t variation_selector;<br>    uint32_t       resolved_glyph_index;<br>    uint16_t       collection_index;<br>    uint8_t        plane;<br>    uint8_t        flags;<br>} sivd_variation_entry_t;' }
        @{ t = 'p'; html = 'Codepoints are <span class="mono">sucs_ex_char_t</span> (64-bit), not <span class="mono">uint32_t</span>, so an RNUR neography base in <span class="mono">0x00160000&ndash;0x0016FFFF</span> and an ExtSUCS plugin base above <span class="mono">0x7FFFFFFF</span> survive intact. <span class="mono">resolved_glyph_index</span> is the font-bundle-relative <span class="mono">uint32_t glyph_id</span> the <span class="mono">suf</span> vector font engine renders.' }
        @{ t = 'h2'; html = 'Collections' }
        @{ t = 'p'; html = 'Registered sequences are grouped into <strong>collections</strong>, each owning a contiguous run of entries so enumeration is an O(1) range walk. The identifier pattern is stored as an <strong>opaque descriptor</strong>: SIVD never evaluates it, because there is no regex engine on the lookup path. The reference table holds 20 entries in 6 collections, including the Annex B example <span class="mono">&lt;82A6 E0134&gt;</span>.' }
        @{ t = 'h2'; html = 'Zero-allocation resolution' }
        @{ t = 'table'; head = @('Operation', 'Cost'); rows = @(
            @('<span class="mono">sivd_resolve_variant</span>', 'O(1) expected (bounded probes), no allocation')
            @('<span class="mono">sivd_init_registry</span>', 'O(n) once, n &le; 128, no allocation')
            @('<span class="mono">sivd_lookup</span> / <span class="mono">is_registered</span> / <span class="mono">collection_of</span>', 'O(1) expected')
            @('<span class="mono">sivd_entry_at</span> / <span class="mono">collection_at</span> / <span class="mono">plane_of</span>', 'O(1)')
        ) }
        @{ t = 'h2'; html = 'Font pipeline integration (suf)' }
        @{ t = 'p'; html = 'The layout pass emits codepoints one at a time. When it reads a valid base followed by a variation selector it forms the pair, calls <span class="mono">sivd_resolve_variant</span> for the glyph index in O(1), and feeds that index to the <span class="mono">suf</span> outline and metric lookup. On a miss the selector is default-ignorable and the base renders its default glyph. The path is single-pass and allocation-free: one probe per selector, a fixed stack, no character-map rebuild.' }
        @{ t = 'table'; head = @('Feature', 'Status'); rows = @(
            @('Binary, fixed-size variation entries', 'Yes')
            @('Zero heap allocation on any path', 'Yes')
            @('O(1) expected (base, selector) resolution', 'Yes')
            @('Unique (base, selector) key; entries never reassigned', 'Yes')
            @('Bridge IVS + supplementary selectors', 'Yes')
            @('RNUR neography variation space', 'Yes')
            @('SCP-invariant bases', 'Yes')
            @('64-bit extSUCS compatibility', 'Yes')
        ) }
        @{ t = 'callout'; html = 'Reference implementation: <span class="mono">include/suts/suts_sivd.h</span> + <span class="mono">src/suts/suts_sivd.c</span> (freestanding C99, no heap), registered in <span class="mono">suts_static</span>.' }
        @{ t = 'note'; html = 'Source of truth: <span class="mono">docs/suts/SUTS-004-sivd.md</span> in the repository.' }
    )
}

$Pages['reports/SUTS-005'] = @{
    path    = 'reports/SUTS-005.html'
    sec     = 'reports'
    title   = 'SUTS-005 — System Character Security Specification (SCSS)'
    desc    = 'SUTS-005: System Character Security Specification (SCSS) — a binary-compiled, zero-allocation homograph, identifier, and script-isolation security schema.'
    crumbName = 'SUTS-005 — SCSS'
    crumbs  = @( @{ label = 'Technical Reports'; href = 'index.html' } )
    h1      = 'SUTS-005 &mdash; System Character <span class="grad">Security</span>'
    subtitle= 'The fifth SuperUnicode Technical Specification: a binary-compiled, zero-allocation schema for homograph detection and identifier security in the OpenWindows microkernel.'
    body    = @(
        @{ t = 'p'; html = 'SUTS-005 (SCSS) is the fifth SuperUnicode Technical Specification and a <strong>core kernel primitive</strong> of the OpenWindows microkernel. It establishes a <strong>binary-compiled, zero-allocation schema</strong> for character-level security &mdash; visual homograph (confusable) detection, identifier permissibility, and script-boundary isolation &mdash; bypassing traditional text-based security databases such as Unicode&rsquo;s <strong>UTS #39</strong> (<span class="mono">Confusables.txt</span>, <span class="mono">IdentifierStatus.txt</span>) in favour of fixed-size, byte-aligned memory layouts optimized for O(1) kernel execution.' }
        @{ t = 'h2'; html = 'Why not a text security database' }
        @{ t = 'ul'; items = @(
            '<strong>Compiled binary vs. text databases</strong> &mdash; UTS #39 is two UTF-8 flat files tokenized at run time; SCSS is fixed-size C structs in read-only memory, at known offsets, with no parser.'
            '<strong>Zero allocation vs. heap</strong> &mdash; a user-space UTS #39 layer loads the data files into dynamic maps and per-script tables; SCSS resolves from static tables and two fixed indexes, never touching a heap.'
            '<strong>Kernel primitive vs. user-space</strong> &mdash; UTS #39 targets browsers and identity libraries reading files from disk; SCSS gives the microkernel, the shell parser, and the capability path deterministic, in-band verdicts.'
        ) }
        @{ t = 'h2'; html = 'Dual-layer architecture' }
        @{ t = 'table'; head = @('Layer', 'Role'); rows = @(
            @('<span class="mono">SUCD</span> &mdash; offline reference', 'Full text manifests: confusable mappings, identifier profiles, and human-readable security tables for developer tooling and user-space applications')
            @('<span class="mono">SCSS</span> &mdash; runtime kernel', 'Immutable, byte-packed binary tables (<span class="mono">scss_security_attr_t</span>, <span class="mono">scss_skeleton_entry_t</span>) queried by the kernel in O(1)')
        ) }
        @{ t = 'p'; html = 'Build-time tooling distils the SUCD security datasets into the tables the kernel embeds. The seeding brief named the offline layer &ldquo;SUCA&rdquo;; in this corpus <strong>SUCA</strong> is already the SuperUnicode Collation Algorithm (SUTS-001), so SCSS names <span class="mono">SUCD</span>, consistent with <a href="SUAS-009.html">SUAS-009 (SIP)</a>.' }
        @{ t = 'h2'; html = 'Script boundary flags' }
        @{ t = 'p'; html = 'SCSS encodes explicit <strong>script-isolation bitfield flags</strong> protecting system paths from cross-script injection, specifically controlling interactions between the <strong>Unicode Compatibility Bridge</strong> (<span class="mono">0x000000&ndash;0x10FFFF</span>) and the <strong>Native &amp; Neography Space</strong> (<span class="mono">0x160000&ndash;0x16FFFF</span>). Every Native-plane codepoint carries <span class="mono">NEOGRAPHY_MIXED</span> and requires an isolated script context.' }
        @{ t = 'h2'; html = 'Compiled security metadata' }
        @{ t = 'p'; html = 'A security attribute word is a compiled 32-bit bitmask; a skeleton record binds a confusable variant to a single-codepoint prototype. Codepoints are <span class="mono">sucs_ex_char_t</span> (64-bit), not <span class="mono">uint32_t</span>, so an RNUR neography lookalike in <span class="mono">0x00160000&ndash;0x0016FFFF</span> and an ExtSUCS plugin codepoint above <span class="mono">0x7FFFFFFF</span> survive intact.' }
        @{ t = 'spec'; html = '#define SUTS_SCSS_ATTR_CAPACITY     128U<br>#define SUTS_SCSS_SKELETON_CAPACITY 128U<br>#define SUTS_SCSS_INDEX_SIZE        256U<br><br>#define SUTS_SCSS_FLAG_ALLOWED_IDENTIFIER (1u &lt;&lt; 0)<br>#define SUTS_SCSS_FLAG_RESTRICTED         (1u &lt;&lt; 1)<br>#define SUTS_SCSS_FLAG_CONFUSABLE         (1u &lt;&lt; 2)<br>#define SUTS_SCSS_FLAG_NEOGRAPHY_MIXED    (1u &lt;&lt; 3)<br><br>typedef struct {<br>    sucs_ex_char_t codepoint;          /* the lookalike      */<br>    sucs_ex_char_t skeleton_codepoint; /* its prototype      */<br>} scss_skeleton_entry_t;<br><br>typedef struct {<br>    sucs_ex_char_t codepoint; /* the codepoint              */<br>    uint32_t       flags;     /* SUTS_SCSS_FLAG_*           */<br>} scss_security_attr_t;' }
        @{ t = 'h2'; html = 'Identifier admissibility' }
        @{ t = 'p'; html = '<span class="mono">scss_is_allowed_identifier</span> is true when a codepoint carries <span class="mono">ALLOWED_IDENTIFIER</span> and none of <span class="mono">RESTRICTED</span>, <span class="mono">CONFUSABLE</span>, or <span class="mono">NEOGRAPHY_MIXED</span>. Three defaults make the predicate <strong>total</strong> over the whole space: the ASCII identifier class is allowed, SCP markers / traps / the Sentinel are restricted, and any Native-plane codepoint requires isolation. A codepoint matching none of these is <strong>not</strong> admissible &mdash; a secure identifier is closed by default.' }
        @{ t = 'h2'; html = 'Homographs and script mixing' }
        @{ t = 'p'; html = 'A codepoint with no skeleton record maps to <strong>itself</strong>, so a plain Latin <span class="mono">a</span> is recognized as confusable with Cyrillic <span class="mono">&#1072;</span> without the Latin letter carrying a record of its own. <span class="mono">scss_script_compatible</span> generalizes the attack from a single character to a whole identifier, and <span class="mono">scss_crosses_bridge_boundary</span> tests the Bridge/Native crossing directly.' }
        @{ t = 'table'; head = @('Operation', 'Cost'); rows = @(
            @('<span class="mono">scss_get_flags</span> / <span class="mono">scss_resolve_skeleton</span>', 'O(1) expected (bounded probes), no allocation')
            @('<span class="mono">scss_init_registry</span>', 'O(n) once, n &le; 128, no allocation')
            @('<span class="mono">scss_confusable_with</span> / <span class="mono">scss_is_allowed_identifier</span>', 'O(1) expected')
            @('<span class="mono">scss_script_of</span> / <span class="mono">scss_plane_of</span> / <span class="mono">scss_hash</span>', 'O(1)')
        ) }
        @{ t = 'h2'; html = 'The curated registry' }
        @{ t = 'p'; html = 'The reference data holds <strong>31</strong> attribute records and <strong>22</strong> skeleton records: Cyrillic, Greek, and RNUR neography homoglyphs of Latin; invisible and control codepoints such as <span class="mono">U+200B ZWSP</span>, <span class="mono">U+202E RLO</span>, and <span class="mono">U+FEFF BOM</span>; and curated allowed ideographs. Skeleton prototypes collapse each script&rsquo;s lookalikes onto one Latin class &mdash; Cyrillic <span class="mono">&#1072;</span>, Greek <span class="mono">&alpha;</span>, and RNUR <span class="mono">U+160100</span> all map to <span class="mono">U+0061</span>.' }
        @{ t = 'table'; head = @('Feature', 'Status'); rows = @(
            @('Binary, fixed-size attribute and skeleton records', 'Yes')
            @('Zero heap allocation on any path', 'Yes')
            @('O(1) expected flag and skeleton resolution', 'Yes')
            @('Bridge + RNUR homograph coverage', 'Yes')
            @('Explicit Bridge / Native script-isolation test', 'Yes')
            @('SCP invariance; traps and Sentinel restricted', 'Yes')
            @('Closed-by-default identifier verdict', 'Yes')
            @('64-bit extSUCS compatibility', 'Yes')
        ) }
        @{ t = 'callout'; html = 'Reference implementation: <span class="mono">include/suts/suts_scss.h</span> + <span class="mono">src/suts/suts_scss.c</span> (freestanding C99, no heap), registered in <span class="mono">suts_static</span>.' }
        @{ t = 'note'; html = 'Source of truth: <span class="mono">docs/suts/SUTS-005-scss.md</span> in the repository.' }
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

$Pages['reports/SUAS-009'] = @{
    path    = 'reports/SUAS-009.html'
    sec     = 'reports'
    title   = 'SUAS-009 — System Ideographic Properties (SIP)'
    desc    = 'SUAS-009: System Ideographic Properties (SIP) — zero-allocation, O(1) radical-stroke, flags, and variant-linkage extraction.'
    crumbName = 'SUAS-009 — SIP'
    crumbs  = @( @{ label = 'Technical Reports'; href = 'index.html' } )
    h1      = 'SUAS-009 &mdash; System <span class="grad">Ideographic Properties</span>'
    subtitle= 'The ninth SuperUnicode Architecture Standard: a zero-allocation, O(1) engine for ideographic radical-stroke, classification, and variant data (a UAX #38 analog).'
    body    = @(
        @{ t = 'p'; html = 'SUAS-009 (SIP) is the ninth SuperUnicode Architecture Standard. It governs the <strong>zero-allocation, O(1) querying and bitfield extraction of ideographic character attributes</strong>: the Kangxi radical, the stroke count, a classification bitfield, a collation seed, and the linkage to a regional or historical variant. Where a traditional Unicode environment consults the multi-megabyte Unihan Database (UAX #38) &mdash; full of historical notes and string properties parsed at run time &mdash; SIP compiles the same concept into immutable, byte-packed binary tables that the kernel reads directly, with no text parsing and no heap.' }
        @{ t = 'h2'; html = 'Dual-layer architecture' }
        @{ t = 'table'; head = @('Layer', 'Role'); rows = @(
            @('<span class="mono">SUCD</span> &mdash; offline reference', 'Full text manifests and flat-file datasets for developer tooling, user-space applications, and offline documentation')
            @('<span class="mono">SIP</span> &mdash; runtime kernel', 'Immutable, byte-packed binary tables (<span class="mono">suas_glyph_prop_t</span>) queried by the kernel in O(1)')
        ) }
        @{ t = 'p'; html = 'The seeding brief named the offline layer &ldquo;SUCA&rdquo;; in this corpus <strong>SUCA</strong> is already the SuperUnicode Collation Algorithm (SUTS-001), so SIP names <span class="mono">SUCD</span> (the SuperUnicode Character Database) as its reference layer.' }
        @{ t = 'h2'; html = 'The static record' }
        @{ t = 'p'; html = 'A queried property set is a fixed-size, stack-allocatable record. The seeding sketch&rsquo;s five core fields are kept verbatim and in order; <span class="mono">codepoint</span> is widened to 64-bit <span class="mono">sucs_ex_char_t</span> so an RNUR neography logograph survives, and one additive field, <span class="mono">native_variant</span>, carries the variant linkage. The record is 24 bytes on both 32- and 64-bit ABIs.' }
        @{ t = 'spec'; html = 'typedef struct {<br>    sucs_ex_char_t codepoint;      /* e.g. U+4E00 */<br>    uint16_t       radical;        /* Kangxi radical (0 = none) */<br>    uint8_t        strokes;        /* total stroke count */<br>    uint8_t        flags;          /* SUAS_SIP_FLAG_* */<br>    uint32_t       sort_weight;    /* SUCA collation seed */<br>    sucs_ex_char_t native_variant; /* Native/RNUR target, or 0 */<br>} suas_glyph_prop_t;' }
        @{ t = 'h2'; html = 'Property extraction' }
        @{ t = 'p'; html = 'SIP discards the human-readable text keys in favour of direct bitfield masking and a hash-indexed static array. The text-only properties stay in the offline SUCD layer and never reach the kernel path.' }
        @{ t = 'table'; head = @('UAX #38 key', 'SIP representation'); rows = @(
            @('<span class="mono">kRSUnicode</span> / <span class="mono">kTotalStrokes</span>', '<span class="mono">radical</span> + <span class="mono">strokes</span>')
            @('<span class="mono">kPrimaryNumeric</span>', '<span class="mono">SUAS_SIP_FLAG_NUMERIC</span>')
            @('<span class="mono">kCompatibilityVariant</span>', '<span class="mono">SUAS_SIP_FLAG_COMPAT</span> + sort-weight fold')
            @('<span class="mono">kSimplifiedVariant</span> / <span class="mono">kTraditionalVariant</span>', '<span class="mono">SUAS_SIP_FLAG_TRADITIONAL</span> / <span class="mono">native_variant</span>')
            @('<span class="mono">kMandarin</span>, <span class="mono">kDefinition</span>, <span class="mono">kIRG_*</span>', 'Not carried &mdash; offline SUCD only')
        ) }
        @{ t = 'h2'; html = 'Classification flags' }
        @{ t = 'table'; head = @('Flag', 'Meaning'); rows = @(
            @('<span class="mono">SUAS_SIP_FLAG_UNIFIED</span>', 'Unified_Ideograph')
            @('<span class="mono">SUAS_SIP_FLAG_RADICAL</span>', 'Radical-stroke data present')
            @('<span class="mono">SUAS_SIP_FLAG_VARIANT</span>', 'Carries a variant linkage')
            @('<span class="mono">SUAS_SIP_FLAG_NUMERIC</span>', 'Carries a numeric value')
            @('<span class="mono">SUAS_SIP_FLAG_NATIVE</span>', 'Native / RNUR-plane logograph')
            @('<span class="mono">SUAS_SIP_FLAG_COMPAT</span>', 'Compatibility ideograph')
            @('<span class="mono">SUAS_SIP_FLAG_TRADITIONAL</span>', 'Traditional (vs. simplified) form')
        ) }
        @{ t = 'h2'; html = 'Variant routing and the repertoire' }
        @{ t = 'p'; html = 'For an ideograph that needs a distinct regional or historical representation beyond Unicode Han unification, <span class="mono">suas_sip_variant_target</span> routes the query <strong>directly into the Native &amp; Neography Plane</strong> (<span class="mono">0x160000&ndash;0x16FFFF</span>) and the <a href="SUTS-004.html">SIVD</a> variation database (SUTS-004). <span class="mono">suas_sip_is_ideograph</span> decides repertoire membership by O(1) range dispatch, so an unregistered but valid ideograph is still recognized; compatibility forms fold their sort weight onto their canonical unified counterpart.' }
        @{ t = 'h2'; html = 'Resolution' }
        @{ t = 'p'; html = 'The table is a static array of at most <span class="mono">SUAS_SIP_REGISTRY_CAPACITY</span> (128) records, indexed once into a fixed array of <span class="mono">SUAS_SIP_INDEX_SIZE</span> (256) open-addressing slots (FNV-1a hash, linear probing). Every query hashes the codepoint and probes a bounded number of slots. The index is built lazily on first use and is idempotent; it never grows at run time.' }
        @{ t = 'table'; head = @('Operation', 'Cost'); rows = @(
            @('<span class="mono">suas_sip_get_properties</span>', 'O(1) expected (bounded probes), no allocation')
            @('<span class="mono">suas_sip_init_engine</span>', 'O(n) once, n &le; 128, no allocation')
            @('<span class="mono">suas_sip_is_ideograph</span>', 'O(1) range dispatch')
            @('<span class="mono">suas_sip_get_sort_weight</span>', 'O(1) stored, or algorithmic native-order fallback')
        ) }
        @{ t = 'table'; head = @('Feature', 'Status'); rows = @(
            @('Static, byte-packed property record', 'Yes')
            @('O(1) attribute extraction, no heap', 'Yes')
            @('Radical-stroke, flags, and collation seed', 'Yes')
            @('Variant linkage into the Native/RNUR plane', 'Yes')
            @('SCP / trap / Sentinel invariance', 'Yes')
            @('Lazy, idempotent index build', 'Yes')
            @('Freestanding C99', 'Yes')
        ) }
        @{ t = 'callout'; html = 'Reference implementation: <span class="mono">include/suas/suas_sip.h</span> + <span class="mono">src/suas/suas_sip.c</span> (freestanding C99, no heap), registered in <span class="mono">suas_static</span>.' }
        @{ t = 'note'; html = 'Source of truth: <span class="mono">docs/suas/SUAS-009-sip.md</span> in the repository.' }
    )
}

$Pages['reports/SUAS-010'] = @{
    path    = 'reports/SUAS-010.html'
    sec     = 'reports'
    title   = 'SUAS-010 — System Specification Registry & Cross-Reference Engine (SSRE)'
    desc    = 'SUAS-010: System Specification Registry & Cross-Reference Engine (SSRE) — embedded dependency metadata and build-time linkage validation for the SuperUnicode corpus (a UAX #41 analog).'
    crumbName = 'SUAS-010 — SSRE'
    crumbs  = @( @{ label = 'Technical Reports'; href = 'index.html' } )
    h1      = 'SUAS-010 &mdash; System Specification <span class="grad">Registry &amp; Cross-Reference</span>'
    subtitle= 'The SuperUnicode meta-standard: an embedded, binary-encoded dependency registry that makes the specification tree a machine-verifiable artefact of the kernel image (a UAX #41 analog).'
    body    = @(
        @{ t = 'p'; html = 'SUAS-010 (SSRE) is the tenth SuperUnicode Architecture Standard and the <strong>meta-standard of the set</strong> &mdash; the normative registry and deterministic linkage protocol that governs how every SUAS standard, SUTS data specification, and SUTR architecture report cross-references one another inside the OpenWindows microkernel. It is the one architecture standard that governs the other architecture standards.' }
        @{ t = 'p'; html = 'UAX #41 is the Unicode Consortium&rsquo;s attempt to answer a question no other Unicode technical report asks: <em>which specifications depend on which?</em> Its answer is a bibliography &mdash; a human-readable table of contents, maintained by hand, read by humans, and enforced by nothing. SSRE keeps the question and replaces the answer with an <strong>embedded binary registry</strong>, so that version mismatches, missing prerequisite SUTS modules, and invalid architectural dependencies are caught instantly, at compile time, before an image is produced.' }
        @{ t = 'h2'; html = 'Dual-layer architecture' }
        @{ t = 'table'; head = @('Layer', 'Role'); rows = @(
            @('<span class="mono">SUCD</span> &mdash; offline reference', 'Human-readable specification manifests and normative reference lists, for documentation, external compilers, and compliance tooling')
            @('<span class="mono">SSRE</span> &mdash; runtime kernel', 'Immutable, byte-packed binary tables (<span class="mono">ssre_spec_record_t</span>) embedded in the kernel image and queried in O(1)')
        ) }
        @{ t = 'p'; html = 'As with <a href="SUAS-009.html">SIP</a> and <a href="SUTS-005.html">SCSS</a>, the seeding brief named the offline layer &ldquo;SUCA&rdquo;; in this corpus <strong>SUCA</strong> is already the SuperUnicode Collation Algorithm (SUTS-001), so SSRE names <span class="mono">SUCD</span> as its reference layer.' }
        @{ t = 'h2'; html = 'The cross-reference kinds' }
        @{ t = 'p'; html = 'Where a UAX #41 bibliography records only that one document references another, SSRE classifies <em>what kind of thing</em> is being referenced &mdash; and this is the distinction that makes the registry enforceable rather than merely informative.' }
        @{ t = 'table'; head = @('Kind', 'Family', 'Meaning'); rows = @(
            @('<span class="mono">SUAS_SSRE_KIND_MANDATORY</span>', 'SUAS', 'A non-negotiable architectural invariant')
            @('<span class="mono">SUAS_SSRE_KIND_OPTIONAL</span>', 'SUTS', 'An optional data specification')
            @('<span class="mono">SUAS_SSRE_KIND_REFERENCE</span>', 'SUTR', 'An architecture report')
        ) }
        @{ t = 'note'; html = 'A dependency asserting <span class="mono">is_mandatory</span> is satisfiable <strong>only</strong> against a MANDATORY specification. An application that declares a mandatory dependency on an optional SUTS data specification has made an architectural error, and SSRE rejects it rather than quietly accommodating it.' }
        @{ t = 'h2'; html = 'The dependency descriptor' }
        @{ t = 'spec'; html = 'typedef struct {<br>    const char *spec_id;          /* e.g. "SUTS-003", "SUAS-009" */<br>    uint8_t min_version_major;<br>    uint8_t min_version_minor;<br>    bool is_mandatory;            /* true if SUAS, false if optional SUTS */<br>} ssre_dependency_descriptor_t;' }
        @{ t = 'h2'; html = 'Two enforcement surfaces' }
        @{ t = 'table'; head = @('Surface', 'Question it answers', 'Provider known', 'Failure'); rows = @(
            @('<span class="mono">SUAS_SSRE_REQUIRE_DEP</span>', 'Does the header I am compiling against match?', 'At build time', 'Compilation halts')
            @('<span class="mono">suas_ssre_verify_dependency</span>', 'Does the image I am loading actually contain it?', 'At load time', 'Module load rejected')
        ) }
        @{ t = 'p'; html = 'Every operator in <span class="mono">SUAS_SSRE_VALIDATE_DEP</span> is a <strong>preprocessor operator</strong>, so the predicate is valid inside <span class="mono">#if</span> &mdash; which is what makes a compile-time gate possible at all. A module whose provider declares its version as integer literals enforces its own prerequisite directly:' }
        @{ t = 'spec'; html = '#if !SUAS_SSRE_VALIDATE_DEP(1, 0, SUTS_SLDF_VERSION_MAJOR, SUTS_SLDF_VERSION_MINOR)<br>#error "SSRE: schema misalignment with SUTS-003 (SLDF)"<br>#endif' }
        @{ t = 'p'; html = '<span class="mono">SUAS_SSRE_REQUIRE_DEP</span> applies the same predicate through the classic C99 negative-array-bound idiom, with a caller-chosen symbol so several gates may coexist in one translation unit. Both surfaces implement the identical partial order, so they can never disagree.' }
        @{ t = 'h2'; html = 'The curated registry' }
        @{ t = 'table'; head = @('Kind', 'Count', 'Members'); rows = @(
            @('MANDATORY', '12', 'SUAS-001 (SDF) &hellip; SUAS-009 (SIP), SUAS-010 (SSRE), SUAS-011 (SCMS), SUAS-012 (SUCD)')
            @('OPTIONAL', '5', 'SUTS-001 (SUCA), SUTS-002 (SRE), SUTS-003 (SLDF), SUTS-004 (SIVD), SUTS-005 (SCSS)')
            @('REFERENCE', '11', 'SUTR-0 through SUTR-10')
        ) }
        @{ t = 'p'; html = 'Each record carries the specification identifier, its family acronym, the <strong>reference unit</strong> implementing it (e.g. <span class="mono">suts_sldf</span>), its registered version, its kind, and the SUCM conformance tier it claims. The registry maps specification &rarr; unit, and <span class="mono">suas_ssre_spec_of_module</span> provides the inverse &mdash; the operation a documentation generator needs in order to print, for any engine, the specification that governs it.' }
        @{ t = 'h2'; html = 'Resolution' }
        @{ t = 'p'; html = 'The registry is a static array of at most <span class="mono">SUAS_SSRE_REGISTRY_CAPACITY</span> (64) records, indexed once into a fixed array of <span class="mono">SUAS_SSRE_INDEX_SIZE</span> (256) open-addressing slots (FNV-1a hash over the identifier bytes, linear probing). Identifiers are short NUL-terminated ASCII, so hashing and equality are bounded single-pass loops and a freestanding image needs no <span class="mono">&lt;string.h&gt;</span> at all.' }
        @{ t = 'table'; head = @('Operation', 'Cost'); rows = @(
            @('<span class="mono">suas_ssre_lookup</span>', 'O(1) expected (bounded probes), no allocation')
            @('<span class="mono">suas_ssre_verify_dependency</span>', 'O(1) expected &mdash; one lookup plus constant work')
            @('<span class="mono">suas_ssre_verify_set</span> / <span class="mono">_first_missing</span>', 'O(count)')
            @('<span class="mono">suas_ssre_spec_of_module</span>', 'O(n) over the registry; tooling path, not the boot path')
        ) }
        @{ t = 'table'; head = @('Feature', 'Status'); rows = @(
            @('Embedded binary registry in the kernel image', 'Yes')
            @('Compile-time gate usable inside <span class="mono">#if</span>', 'Yes')
            @('Mandatory-implies-MANDATORY rejection', 'Yes')
            @('Bidirectional specification &harr; unit cross-reference', 'Yes')
            @('Zero allocation, freestanding C99', 'Yes')
            @('Registry self-consistency under test', 'Yes')
        ) }
        @{ t = 'h2'; html = 'Numbering note' }
        @{ t = 'p'; html = 'The SUAS series is <strong>contiguous</strong>: SUAS-001 through SUAS-012 are all allocated, in authorship order, and no number is reserved or held back. This standard is <strong>SUAS-010</strong>.' }
        @{ t = 'p'; html = 'It was first published as SUAS-011, with SUAS-010 left as a deliberate gap, because a skipped number in a small series reads as a transcription error rather than as a decision &mdash; so both it and SCMS were shifted down by one. Citations predating that shift that say <strong>SUAS-011 (SSRE)</strong> mean this standard; those that say <strong>SUAS-012 (SCMS)</strong> mean <a href="SUAS-011.html">SUAS-011 (SCMS)</a>. Those citations are strictly historical: SUAS-012 now names <a href="SUAS-012.html">SUCD</a>.' }
        @{ t = 'callout'; html = 'Reference implementation: <span class="mono">include/suas/suas_ssre.h</span> + <span class="mono">src/suas/suas_ssre.c</span> (freestanding C99, no heap), registered in <span class="mono">suas_static</span>.' }
        @{ t = 'note'; html = 'Source of truth: <span class="mono">docs/suas/SUAS-010-ssre.md</span> in the repository.' }
    )
}

$Pages['reports/SUAS-011'] = @{
    path    = 'reports/SUAS-011.html'
    sec     = 'reports'
    title   = 'SUAS-011 — System Character Manifest & Schema Standard (SCMS)'
    desc    = 'SUAS-011: System Character Manifest & Schema Standard (SCMS) — the line-oriented, binary-mappable character-manifest grammar, compiled in a single bounded pass with zero allocation (a UAX #42 analog).'
    crumbName = 'SUAS-011 — SCMS'
    crumbs  = @( @{ label = 'Technical Reports'; href = 'index.html' } )
    h1      = 'SUAS-011 &mdash; System Character <span class="grad">Manifest &amp; Schema</span>'
    subtitle= 'The structural counterpart of UAX #42: a flat, tab-delimited manifest grammar that compiles straight into immutable kernel tables &mdash; no DOM, no heap, no second pass (a UAX #42 analog).'
    body    = @(
        @{ t = 'p'; html = 'SUAS-011 (SCMS) is the eleventh SuperUnicode Architecture Standard and the <strong>structural counterpart of UAX #42</strong>, the Unicode Character Database in XML. It defines the exact grammar and field-mapping rules by which an offline character manifest is written, validated, and compiled into the immutable binary lookup tables the OpenWindows kernel embeds.' }
        @{ t = 'p'; html = 'UAX #42 describes the character database as an XML schema, and that choice is expensive in a way that is structural rather than incidental. XML is hierarchical, so a record has no fixed size and no fixed offset; a parser must build a tree, and building a tree means allocating; a schema validator is a general-purpose engine, so its cost is unbounded by anything the database knows; and nothing in the result is binary-mappable, so a kernel must re-encode it a second time before the first lookup can happen. <strong>A database compiled through UAX #42 costs a DOM, a heap, and two full passes before a single query returns.</strong>' }
        @{ t = 'h2'; html = 'Why line-oriented' }
        @{ t = 'p'; html = 'SCMS rejects hierarchical markup precisely <em>because</em> it wants binary mappability. A manifest is a stream of lines; a record is exactly six tab-delimited fields in a fixed order, each of a known and bounded type; a directive is a name and a value. One record line corresponds to one kernel table entry, in order, with no re-encoding step and no intermediate tree. Build tooling parses a manifest in a single linear pass over zero-allocation streaming buffers, and a manifest that does not validate is never compiled.' }
        @{ t = 'table'; head = @('Aspect', 'UAX #42', 'SCMS'); rows = @(
            @('Format', 'XML, hierarchical', 'Line-oriented, flat')
            @('Record shape', 'Unbounded, nested elements', 'Exactly 6 tab-delimited fields')
            @('Parser', 'DOM or SAX tree builder', 'Single bounded linear pass')
            @('Memory', 'Heap-allocated node tree', 'Caller-owned fixed buffers')
            @('Validation', 'General-purpose schema engine', 'Bounded decoders, one outcome code each')
            @('Output', 'A tree to be re-encoded', 'Binary-mappable, 1:1 with kernel tables')
            @('Plane safety', 'None', 'Endpoints <em>and</em> interior checked')
            @('Linkage', 'None', 'SUAS-010 (SSRE) gate on the target schema')
        ) }
        @{ t = 'h2'; html = 'The record grammar' }
        @{ t = 'spec'; html = 'record := start TAB end TAB radical TAB strokes TAB flags TAB weight' }
        @{ t = 'table'; head = @('#', 'Column', 'Form', 'Bounds'); rows = @(
            @('0', 'start', '<span class="mono">U+</span> + 1&ndash;8 hex', 'admissible')
            @('1', 'end', '<span class="mono">U+</span> + 1&ndash;8 hex', 'admissible, &ge; start')
            @('2', 'radical', 'decimal', '0 &hellip; 214 (Kangxi)')
            @('3', 'strokes', 'decimal', '0 &hellip; 255')
            @('4', 'flags', '<span class="mono">0x</span> + 1&ndash;4 hex', '0 &hellip; <span class="mono">0xFFFF</span>')
            @('5', 'weight', 'decimal', '0 &hellip; <span class="mono">0xFFFFFFFF</span>')
        ) }
        @{ t = 'p'; html = 'A single codepoint is expressed as <span class="mono">end == start</span>, and such a record is <strong>binary-aligned</strong>: it maps to exactly one kernel table slot rather than a run the tool must expand. <span class="mono">is_binary_aligned</span> is the compilation decision the parse descriptor exists to carry.' }
        @{ t = 'h2'; html = 'The record descriptor' }
        @{ t = 'p'; html = 'The descriptor reports <em>structure</em>, not payload: the range resolved, how many columns were found, and whether the record maps to a single table slot. A compiler walks the payload columns itself through <span class="mono">suas_scms_field_at</span>, which decodes column <em>n</em> on demand. This keeps the descriptor fixed-size and keeps one code path responsible for each field&rsquo;s bounds.' }
        @{ t = 'spec'; html = 'typedef struct {<br>    sucs_ex_char_t start_codepoint;<br>    sucs_ex_char_t end_codepoint;      /* equal to start for one codepoint */<br>    uint16_t       field_count;        /* fields actually found, not expected */<br>    bool           is_binary_aligned;  /* true iff the record is one codepoint */<br>} scms_record_descriptor_t;' }
        @{ t = 'note'; html = 'Codepoints are widened to the 64-bit ExtSUCS type <span class="mono">sucs_ex_char_t</span> so an RNUR neography logograph survives the round trip, even though the grammar currently caps <span class="mono">U+</span> at eight hex digits.' }
        @{ t = 'h2'; html = 'Admissibility: the interior is the point' }
        @{ t = 'p'; html = 'Range validation runs in a fixed order so the reported outcome names the first thing that is wrong &mdash; ordering, then the endpoints, then the <strong>interior</strong>, then the span ceiling. The interior check is the step a naive implementation omits, and it is the one that matters:' }
        @{ t = 'note'; html = 'A range from <span class="mono">U+10FFFF</span> to <span class="mono">U+00120000</span> has two perfectly admissible endpoints and swallows the <strong>entire System Control Plane</strong> in between. A manifest range is a promise that <em>every</em> codepoint it covers is data; a range is admissible only when that promise holds throughout.' }
        @{ t = 'p'; html = 'This is the SCP invariance that <a href="SUAS-009.html">SIP</a> and <a href="SUTS-005.html">SCSS</a> enforce on live codepoint queries, applied to the data <em>at rest</em> &mdash; so a malformed manifest cannot smuggle a Control Plane codepoint, a trap slot, or the Sentinel into the image in the first place. RNUR neography (<span class="mono">0x160000&ndash;0x16FFFF</span>) is fully admissible and is reported by the plane mask as both NATIVE and RNUR.' }
        @{ t = 'h2'; html = 'The streaming reader' }
        @{ t = 'p'; html = 'A manifest is read through a caller-allocated <span class="mono">scms_manifest_state_t</span> &mdash; typically on the stack &mdash; so an entire database can be validated with no allocation at all. The reader accumulates counts and the observed plane mask, attributes the first error to its 1&ndash;based line number, and on close requires a <span class="mono">%manifest</span> directive, at least one record, no failed line, and agreement with a declared <span class="mono">%records</span> total. A separate cross-check requires every plane the records touched to appear in the declared <span class="mono">%planes</span> mask.' }
        @{ t = 'h2'; html = 'The SSRE linkage gate' }
        @{ t = 'p'; html = 'A manifest is meaningless in isolation: it exists to feed a target schema, and that schema is itself a registered specification. SCMS validates the manifest&rsquo;s structure; <a href="SUAS-010.html">SUAS-010 (SSRE)</a> validates that the destination exists and is satisfiable. Both must hold before a blob is embedded.' }
        @{ t = 'table'; head = @('Case', 'Result'); rows = @(
            @('Schema unregistered', 'Refused &mdash; a blob nobody can resolve must not be embedded')
            @('<span class="mono">mandatory</span> asserted against an optional SUTS data spec', 'Refused &mdash; a mislabelled architectural claim')
            @('Version floor above the registered version', 'Refused &mdash; schema misalignment')
        ) }
        @{ t = 'h2'; html = 'Named failures' }
        @{ t = 'p'; html = 'Every rejection carries its own outcome code, because a compiler that rejects a database must be able to say <em>exactly</em> why &mdash; a silently truncated radical reaching a kernel table is the failure this standard exists to prevent. The reference suite pins each violation to its own code.' }
        @{ t = 'table'; head = @('Operation', 'Cost'); rows = @(
            @('<span class="mono">suas_scms_line_kind</span>', 'O(1) &mdash; one byte test after the budget check')
            @('<span class="mono">suas_scms_validate_record</span>', 'O(len), len &le; 128 &mdash; bounded, hence O(1)')
            @('<span class="mono">suas_scms_feed_line</span>', 'O(len)')
            @('<span class="mono">suas_scms_end</span> / <span class="mono">_planes_declared</span>', 'O(1)')
            @('<span class="mono">suas_scms_verify_schema_link</span>', 'O(1) expected &mdash; one SSRE lookup')
        ) }
        @{ t = 'table'; head = @('Feature', 'Status'); rows = @(
            @('Fixed six-column record arity', 'Yes')
            @('Interior range checking (SCP, traps, Sentinel)', 'Yes')
            @('Zero allocation, freestanding C99', 'Yes')
            @('Bounded single-pass parsing', 'Yes')
            @('Caller-owned streaming manifest state', 'Yes')
            @('Outcome code per violation', 'Yes')
            @('SUAS-010 (SSRE) linkage gate before emission', 'Yes')
        ) }
        @{ t = 'h2'; html = 'Numbering note' }
        @{ t = 'p'; html = 'The seeding sketch was labelled <strong>SUAS-013</strong>; it is registered as <strong>SUAS-011</strong>, the number immediately after <a href="SUAS-010.html">SUAS-010 (SSRE)</a>, which keeps the SUAS series <strong>contiguous with no reserved gaps</strong>. SUAS-013 is not used by this specification.' }
        @{ t = 'p'; html = 'As with <a href="SUAS-009.html">SIP</a>, <a href="SUAS-010.html">SSRE</a>, and <a href="SUTS-005.html">SCSS</a>, the brief named the offline reference layer &ldquo;SUCA&rdquo;; in this corpus <strong>SUCA</strong> is already the SuperUnicode Collation Algorithm (SUTS-001), so SCMS names <span class="mono">SUCD</span> as the character database it holds.' }
        @{ t = 'callout'; html = 'Reference implementation: <span class="mono">include/suas/suas_scms.h</span> + <span class="mono">src/suas/suas_scms.c</span> (freestanding C99, no heap), registered in <span class="mono">suas_static</span> and in the SSRE registry as a MANDATORY specification.' }
        @{ t = 'note'; html = 'Source of truth: <span class="mono">docs/suas/SUAS-011-scms.md</span> in the repository.' }
    )
}

$Pages['reports/SUAS-012'] = @{
    path    = 'reports/SUAS-012.html'
    sec     = 'reports'
    title   = 'SUAS-012 — SuperUnicode Character Database (SUCD)'
    desc    = 'SUAS-012: SuperUnicode Character Database (SUCD) — the verified binary character database with a three-kinded page directory, O(1) range lookup and a proven partition (a UAX #44 analog).'
    crumbName = 'SUAS-012 — SUCD'
    crumbs  = @( @{ label = 'Technical Reports'; href = 'index.html' } )
    h1      = 'SUAS-012 &mdash; SuperUnicode <span class="grad">Character Database</span>'
    subtitle= 'The structural counterpart of UAX #44: one verified binary image, one 16-byte range record, and a lookup that is one directory load, at most one index load, and one compare.'
    body    = @(
        @{ t = 'p'; html = 'SUAS-012 (SUCD) is the twelfth SuperUnicode Architecture Standard and the <strong>structural counterpart of UAX #44</strong>, the Unicode Character Database. UAX #44 defines the database as a collection of text files, each with its own columns, its own range syntax, and its own defaults. SUCD keeps the data and closes the architecture around it: one binary image, a bounded lookup, and a partition the compiler must prove before it emits a byte.' }
        @{ t = 'p'; html = 'A database defined as a folder of files has no architecture. It states no single definition of a record, offers no guarantee that two files agree about the same codepoint, declares no relationship between the human-readable form and the machine form, and bounds no query. Every implementation that wants a runtime table writes its own compiler, its own reconciliation rules, and its own bugs. <strong>SUCD makes the reconciliation a specified step instead of a tradition.</strong>' }
        @{ t = 'h2'; html = 'Why a binary image' }
        @{ t = 'table'; head = @('Aspect', 'UAX #44', 'SUCD'); rows = @(
            @('Definition', 'A folder of text files', 'One verified binary image, compiled from the files')
            @('Record shape', 'Per-file columns', 'One 16-byte range record, uniform across properties')
            @('Address space', '17 planes, 21 bits', '31 bits, <span class="mono">0x00000000&ndash;0x7FFFFFFF</span>')
            @('Lookup', 'Read the file, or build your own map', 'Two-level directory, O(1)')
            @('Consistency', 'Left to the implementer', 'A partition, proven by the compiler')
            @('Defaults', 'Per-file <span class="mono">@missing</span> lines', 'One default record, <span class="mono">Cn</span> and no properties')
            @('Overlap', 'Allowed and common', 'Rejected by the verifier')
            @('Failure mode', 'A file disagrees with another', 'The verifier names the first defect')
        ) }
        @{ t = 'h2'; html = 'The character record' }
        @{ t = 'p'; html = 'Every property, block and classification compiles into the same fixed record. A point is a range whose endpoints are equal, so there is no separate single-codepoint form for a caller to special-case.' }
        @{ t = 'spec'; html = 'typedef struct {<br>    uint32_t first;            /* inclusive range start */<br>    uint32_t last;             /* inclusive range end   */<br>    uint32_t property_mask;    /* SUCD_PROP_* bits      */<br>    uint16_t general_category; /* sucd_general_category_t */<br>    uint8_t  script_id;        /* SUAS-005 (SSP)        */<br>    uint8_t  flags;            /* SUCD_RECORD_*         */<br>} sucd_record_t;' }
        @{ t = 'p'; html = 'Seventeen property bits are assigned, one per property the published <span class="mono">0.1.0</span> files actually name, and the upper fifteen are reserved so that an unmasked bit is a detectable defect rather than an accident. The spellings are the files&rsquo; own: a compiler matches <span class="mono">Unicode_Bridged</span>, and the API returns <span class="mono">Unicode_Bridged</span>.' }
        @{ t = 'table'; head = @('Source', 'Properties'); rows = @(
            @('<span class="mono">Props/PropList.txt</span>', '<span class="mono">Unicode_Bridged</span>, <span class="mono">Control_Plane</span>, <span class="mono">Printable</span>, the four <span class="mono">Diagnostic_*</span>, <span class="mono">Trap_Instruction</span>, <span class="mono">Sentinel_Value</span>')
            @('<span class="mono">Derived/DerivedCoreProperties.txt</span>', '<span class="mono">Alphabetic</span>, <span class="mono">Control</span>')
            @('<span class="mono">Props/BidiProps.txt</span>', '<span class="mono">LTR</span>, <span class="mono">RTL</span>, <span class="mono">ARABIC_AL</span>, <span class="mono">NEUTRAL</span>, <span class="mono">MIRRORED</span>, <span class="mono">WHITESPACE</span>')
        ) }
        @{ t = 'h2'; html = 'The three-kinded page directory' }
        @{ t = 'p'; html = 'The 31-bit space splits into 32&nbsp;768 pages of 65&nbsp;536 codepoints. Each directory slot is exactly one of three things, because the space is simultaneously enormous and empty in the middle and dense at the bottom.' }
        @{ t = 'table'; head = @('Slot', 'Kind', 'Meaning'); rows = @(
            @('<span class="mono">SUAS_SUCD_NO_PAGE</span>', 'Empty', 'Every codepoint in the page is unassigned')
            @('<span class="mono">SUAS_SUCD_UNIFORM_TAG | i</span>', 'Uniform', 'Record <em>i</em> covers the whole page; no index load')
            @('anything else', 'Dense', 'Offset into the page&rsquo;s per-codepoint <span class="mono">uint16</span> index')
        ) }
        @{ t = 'p'; html = 'The Native allocation is <em>one range</em> spanning more than two billion codepoints, so a dense table for it would cost 4&nbsp;GiB; the ASCII page carries a dozen distinct bidirectional classifications, so a uniform slot cannot describe it. Three kinds let the Native plane pay one slot per page while ASCII pays one index, and the empty middle pays nothing at all.' }
        @{ t = 'h2'; html = 'Lookup and its cost' }
        @{ t = 'spec'; html = 'slot = directory[cp &gt;&gt; 16]<br>if slot == NO_PAGE            &rarr; default record<br>if slot &amp; UNIFORM_TAG         &rarr; i = slot &amp; 0x7FFFFFFF<br>else                          &rarr; i = dense[slot + (cp &amp; 0xFFFF)]<br>r = records[i]<br>return (cp &lt; r.first || cp &gt; r.last) ? default : r' }
        @{ t = 'table'; head = @('Operation', 'Cost'); rows = @(
            @('<span class="mono">suas_sucd_get_record</span>', 'O(1) &mdash; one directory load, &le; one index load, one record load')
            @('<span class="mono">suas_sucd_property_mask</span> / <span class="mono">_plane_of</span>', 'O(1)')
            @('<span class="mono">suas_sucd_verify_image</span>', 'O(image extent) &mdash; one pass, no allocation')
            @('<span class="mono">suas_sucd_range_form</span> / <span class="mono">_parse_range</span>', 'O(len) over a fixed line')
            @('<span class="mono">suas_sucd_verify_schema_link</span>', 'O(1) expected &mdash; one SSRE lookup')
        ) }
        @{ t = 'p'; html = 'The image stores no dense-index length. Its size, the fixed directory size and <span class="mono">total_records</span> determine the dense extent exactly, and a size that admits no exact layout is rejected &mdash; so there is no length field for a corrupt image to lie about.' }
        @{ t = 'h2'; html = 'Verification names the defect' }
        @{ t = 'p'; html = 'A loader that rejects an image must be able to say <em>which</em> invariant failed. <span class="mono">suas_sucd_verify_image</span> returns the first of seventeen named outcomes: a bad magic, a wrong directory count, a record count over capacity, a size that is not a layout, a bad cursor flag, a reversed range, a codepoint past the space, a general category out of range, a reserved property bit, unordered records, overlapping records, a directory slot that names no record, a dense offset past the index, or a page that its uniform record does not actually cover.' }
        @{ t = 'p'; html = 'Unordered and overlapping records are rejected outright, so the partition is an image property rather than a hope. A codepoint that is merely unassigned is not a failure at all: it receives the default record &mdash; <span class="mono">Cn</span>, no properties, the UNKNOWN script &mdash; because a database that answers &ldquo;not assigned&rdquo; is answering the question.' }
        @{ t = 'h2'; html = 'The published files are not a partition' }
        @{ t = 'p'; html = 'The standard says so plainly. <span class="mono">SUCSData.txt</span> and <span class="mono">Blocks.txt</span> nest &mdash; the Native row swallows the traps and the Sentinel, and the control-plane block swallows the four diagnostic blocks. <span class="mono">PropList.txt</span> overlays <span class="mono">Control_Plane</span> on the <span class="mono">Diagnostic_*</span> ranges. <span class="mono">BidiProps.txt</span> overlaps and duplicates itself. And <span class="mono">Traps.txt</span> marks trap codepoints unassigned against the enclosing Native allocation.' }
        @{ t = 'p'; html = 'Four range syntaxes coexist across the tree, so a compiler that reads only one convention silently drops most of the database; <span class="mono">suas_sucd_range_form</span> and <span class="mono">suas_sucd_parse_range</span> decode all four. The over<i>laps</i> are resolved by the compiler, deterministically &mdash; narrower wins for nested ranges, and an explicit Bidirectional override wins over its enclosing range &mdash; and the result must still satisfy the partition before an image is written.' }
        @{ t = 'h2'; html = 'Numbering note' }
        @{ t = 'p'; html = 'The seeding sketch was labelled <strong>SUAS-014</strong>; it is registered as <strong>SUAS-012</strong>, the number immediately after <a href="SUAS-011.html">SUAS-011 (SCMS)</a>, which keeps the SUAS series <strong>contiguous with no reserved gaps</strong>. Neither SUAS-013 nor SUAS-014 is used by this specification.' }
        @{ t = 'p'; html = 'As with <a href="SUAS-009.html">SIP</a>, <a href="SUAS-010.html">SSRE</a>, <a href="SUAS-011.html">SCMS</a>, and <a href="SUTS-005.html">SCSS</a>, the brief named the reference layer &ldquo;SUCA&rdquo;; in this corpus <strong>SUCA</strong> is already the SuperUnicode Collation Algorithm (SUTS-001), so this standard names <span class="mono">SUCD</span> as the character database.' }
        @{ t = 'callout'; html = 'Reference implementation: <span class="mono">include/suas/suas_sucd.h</span> + <span class="mono">src/suas/suas_sucd.c</span> (freestanding C99, no heap), registered in <span class="mono">suas_static</span> and in the SSRE registry as a MANDATORY specification.' }
        @{ t = 'note'; html = 'Source of truth: <span class="mono">docs/suas/SUAS-012-sucd.md</span> in the repository.' }
    )
}
