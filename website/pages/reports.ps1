# Technical Reports section

$Pages['reports/index'] = @{
    path    = 'reports/index.html'
    sec     = 'reports'
    title   = 'Technical Reports — SUTR'
    desc    = 'The SUTR-0 through SUTR-013 specifications of SuperUnicode.'
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
            @('<span class="mono">SUTS-006</span>', 'Draft &mdash; Authoring', '<a href="SUTS-006.html">System Internationalized Domain &amp; Identifier Network Architecture (SIDN)</a> &mdash; single-pass, zero-allocation label mapping, validation, and canonical routing (a UTS #46 analog)')
            @('<span class="mono">SUTS-007</span>', 'Draft &mdash; Authoring', '<a href="SUTS-007.html">SuperUnicode Emoji &amp; Presentation Specification (SUEPS)</a> &mdash; zero-allocation, O(1) emoji presentation and flattened ZWJ / modifier sequence resolution (a UTS #51 analog)')
            @('<span class="mono">SUTS-008</span>', 'Draft &mdash; Authoring', '<a href="SUTS-008.html">SuperUnicode Source Code Handling (SUSCH)</a> &mdash; zero-allocation, single-pass source code safety attributes, identifier validation, bidi / invisible elimination, homoglyph risk, and identifier chunk analysis (a UTS #55 analog)')
            @('<span class="mono">SUTS-009</span>', 'Draft &mdash; Authoring', '<a href="SUTS-009.html">SuperUnicode Link Detection &amp; Formatting (SULDF)</a> &mdash; zero-allocation, byte-packed, O(1) link detection, termination, and minimal escaping (a UTS #58 analog)'),
            @('<span class="mono">SUTS-011</span>', 'Draft &mdash; Authoring', '<a href="SUTS-011.html">SuperUnicode Set Notation (SUSN)</a> &mdash; binary-mapped character-set algebra, inversion lists, range-filtered O(1) membership, RNUR neography integration (a UTS #61 analog)'),
            @('<span class="mono">SUTS-011</span>', 'Draft &mdash; Authoring', '<a href="SUTS-011.html">SuperUnicode Set Notation (SUSN)</a> &mdash; binary-mapped character-set algebra, inversion lists, range-filtered O(1) membership, RNUR neography integration (a UTS #61 analog)')
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
            @('<span class="mono">SUAS-013</span>', 'Draft &mdash; Authoring', '<a href="SUAS-013.html">System Ideographic Source Registry (SISR)</a> &mdash; byte-packed source identities, registry classes and O(1) canonical fallback (a UAX #45 analog)')
            @('<span class="mono">SUAS-014</span>', 'Draft &mdash; Authoring', '<a href="SUAS-014.html">SuperUnicode Vertical Text Layout (SUVTL)</a> &mdash; byte-packed vertical orientation, rotation and baseline metrics, with a total algorithmic default (a UAX #50 analog)')
            @('<span class="mono">SUAS-015</span>', 'Draft &mdash; Authoring', '<a href="SUAS-015.html">SuperUnicode Arabic Mark Rendering (SUAMR)</a> &mdash; byte-packed Arabic mark stacking and a total AMTRA transient reordering (a UAX #53 analog)')
            @('<span class="mono">SUAS-016</span>', 'Draft &mdash; Authoring', '<a href="SUAS-016.html">SuperUnicode Egyptian Hieroglyph Database (SUKEMET)</a> &mdash; byte-packed Gardiner sign records, format-control taxonomy and O(1) Unikemet classification (a UAX #57 analog)')
            @('<span class="mono">SUAS-017</span>', 'Draft &mdash; Authoring', '<a href="SUAS-017.html">SuperUnicode East Asian Script Data (SUEASD)</a> &mdash; byte-packed script records, classification ranges and O(1) UAX #60 ancillary resolution (a UAX #60 analog)')
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
            @('<span class="mono">SUTR-11</span>', '0.1.0', '<a href="SUTR-11.html">SUMS</a> &mdash; SuperUnicode Mongolian Shaping &amp; Vertical Direction: binary shaping schema, flattened FVS sequences, O(1) contextual forms, native vertical flow')
            @('<span class="mono">SUTR-012</span>', '0.1.0', '<a href="SUTR-012.html">SUCSL</a> &mdash; SuperUnicode Cuneiform Sign Lists: packed sign records, compound composition, O(1) sign lookup, RNUR extension')
            @('<span class="mono">SUTR-013</span>', '0.1.0', '<a href="SUTR-013.html">SUEAS</a> &mdash; SuperUnicode East Asian Spacing: compiled East_Asian_Spacing/Width tables, O(1) width categories, contextual auto-spacing, zero-allocation grid engine')
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

$Pages['reports/SUTS-006'] = @{
    path    = 'reports/SUTS-006.html'
    sec     = 'reports'
    title   = 'SUTS-006 — System Internationalized Domain & Identifier Network Architecture (SIDN)'
    desc    = 'SUTS-006: System Internationalized Domain & Identifier Network Architecture (SIDN) — single-pass, zero-allocation label mapping, validation, and canonical routing over 31-bit SUCS (a UTS #46 analog).'
    crumbName = 'SUTS-006 — SIDN'
    crumbs  = @( @{ label = 'Technical Reports'; href = 'index.html' } )
    h1      = 'SUTS-006 &mdash; Internationalized Domain &amp; <span class="grad">Identifier Network</span>'
    subtitle= 'The sixth SuperUnicode Technical Specification: a single-pass, zero-allocation engine for mapping, validating, and canonically routing internationalized labels in the OpenWindows microkernel.'
    body    = @(
        @{ t = 'p'; html = 'SUTS-006 (SIDN) is the sixth SuperUnicode Technical Specification and a <strong>core kernel primitive</strong> of the OpenWindows microkernel. It establishes a <strong>binary-compiled, zero-allocation architecture</strong> for internationalized domain and identifier routing &mdash; mapping a codepoint to a compiled IDNA Status, validating a whole label in a single pass, and producing a <strong>canonical binary routing form</strong> &mdash; replacing the text-based pipeline of Unicode&rsquo;s <strong>UTS #46</strong> (<span class="mono">IdnaMappingTable.txt</span>, Map &rarr; NFC &rarr; Punycode &rarr; Validate) with fixed-size, byte-aligned memory layouts optimized for O(1) kernel execution.' }
        @{ t = 'h2'; html = 'Why not a text IDNA pipeline' }
        @{ t = 'ul'; items = @(
            '<strong>Single pass vs. text pipeline</strong> &mdash; UTS #46 runs Map &rarr; Normalize (NFC) &rarr; Break &rarr; Convert &rarr; Validate over dynamically allocated strings; SIDN walks the label once, applies each codepoint&rsquo;s compiled Status as it goes, and emits the canonical form in the same pass.'
            '<strong>Direct 31-bit routing vs. Punycode</strong> &mdash; UTS #46 <span class="mono">ToASCII</span> produces an ASCII <span class="mono">xn--</span> encoding for the DNS wire; a SuperUnicode network carries SUCS codepoints directly, so SIDN never performs an ASCII encoding loop at all.'
            '<strong>Zero allocation vs. heap</strong> &mdash; a user-space IDNA layer buffers every pipeline stage and loads the mapping table into heap-resident maps; SIDN resolves from one static table and one fixed index, materializing a label into a bounded stack buffer.'
            '<strong>Kernel primitive vs. user-space</strong> &mdash; UTS #46 targets browsers, registrars, and library authors reading files from disk; SIDN gives the microkernel, the shell parser, the IPC namespace, and the routing table deterministic, in-band verdicts.'
        ) }
        @{ t = 'h2'; html = 'Dual-layer architecture' }
        @{ t = 'table'; head = @('Layer', 'Role'); rows = @(
            @('<span class="mono">SUCD</span> &mdash; offline reference', 'Full text manifests: the IDNA mapping table, label restriction rules, and domain-profile definitions for network compilers and user-space applications')
            @('<span class="mono">SIDN</span> &mdash; runtime kernel', 'Immutable, byte-packed binary routing tables (<span class="mono">sidn_mapping_entry_t</span>, <span class="mono">sidn_label_descriptor_t</span>) queried by the kernel in O(1)')
        ) }
        @{ t = 'p'; html = 'Build-time tooling distils the SUCD IDNA datasets into the tables the kernel embeds. The seeding sketch labelled SIDN &ldquo;SUAS-015&rdquo; and named its offline layer &ldquo;SUCD &mdash; SUAS-014&rdquo;; both numbers were stale. In this corpus <strong>SUCD</strong> is the SuperUnicode Character Database (<a href="SUAS-012.html">SUAS-012</a>), SISR is <a href="SUAS-013.html">SUAS-013</a>, and SIDN is registered as <strong>SUTS-006</strong> &mdash; the number freed when SISR moved from SUTS-006 to the SUAS series.' }
        @{ t = 'h2'; html = 'Address-space model' }
        @{ t = 'p'; html = 'SIDN does not invent a second plane taxonomy: <span class="mono">suts_sidn_zone_of</span> classifies a codepoint into the SUCD / SSP planes with the RNUR carve-out made explicit. Only the <strong>Bridge</strong> (<span class="mono">0x00000000&ndash;0x0010FFFF</span>), the <strong>RNUR</strong> neography zone (<span class="mono">0x00160000&ndash;0x0016FFFF</span>), and the <strong>Native</strong> plane are routable; the Control Plane, the trap slots, the Sentinel, and ExtSUCS are never label material.' }
        @{ t = 'h2'; html = 'Compiled mapping metadata' }
        @{ t = 'p'; html = 'A mapping record is a compiled 16-byte entry binding a codepoint to its Status and mapping value; the sketch&rsquo;s label descriptor is preserved unchanged at 8 bytes. Codepoints are 31-bit SUCS values, so an RNUR neography codepoint in <span class="mono">0x00160000&ndash;0x0016FFFF</span> and an ExtSUCS plugin codepoint above <span class="mono">0x7FFFFFFF</span> are classified correctly.' }
        @{ t = 'spec'; html = '#define SIDN_MAPPING_CAPACITY    128U<br>#define SIDN_MAPPING_INDEX_SIZE  256U<br>#define SIDN_LABEL_MAX_DNS        63U<br>#define SIDN_DOMAIN_MAX_DNS      253U<br><br>#define SIDN_FLAG_VALID_LABEL      (1U &lt;&lt; 0)<br>#define SIDN_FLAG_TRANSITIONAL     (1U &lt;&lt; 1)<br>#define SIDN_FLAG_DISALLOWED       (1U &lt;&lt; 2)<br>#define SIDN_FLAG_NEOGRAPHY_DOMAIN (1U &lt;&lt; 3)<br><br>typedef struct {<br>    uint32_t codepoint;      /* base SUCS codepoint       */<br>    uint32_t mapping[2];     /* replacement, up to two    */<br>    uint8_t  status;         /* sidn_status_t             */<br>    uint8_t  mapping_length; /* 0, 1 or 2                 */<br>    uint16_t reserved;       /* must be zero              */<br>} sidn_mapping_entry_t;<br><br>typedef struct {<br>    uint32_t label_hash; /* hash of the normalized stream */<br>    uint16_t length;     /* codepoint count               */<br>    uint16_t flags;      /* SIDN_FLAG_*                   */<br>} sidn_label_descriptor_t;' }
        @{ t = 'h2'; html = 'Mapping status and deviations' }
        @{ t = 'p'; html = 'Every codepoint resolves to one of five compiled Status values &mdash; <span class="mono">valid</span>, <span class="mono">ignored</span>, <span class="mono">mapped</span>, <span class="mono">deviation</span>, <span class="mono">disallowed</span> &mdash; as a fixed-size record, not by tokenizing a semicolon-delimited data line. Four codepoints are <strong>deviations</strong>: <span class="mono">U+00DF</span> (&szlig;), <span class="mono">U+03C2</span> (&sigmaf;), <span class="mono">U+200C</span> (ZWNJ), and <span class="mono">U+200D</span> (ZWJ). Nontransitional processing is the default: a deviation is kept, and the transitional UTS #46 C1 behavior is deprecated.' }
        @{ t = 'h2'; html = 'Label separators and canonical routing' }
        @{ t = 'p'; html = 'The four label separators &mdash; <span class="mono">U+002E</span>, <span class="mono">U+FF0E</span>, <span class="mono">U+3002</span>, and <span class="mono">U+FF61</span> &mdash; all fold to <span class="mono">U+002E</span>, ending the label. <span class="mono">suts_sidn_to_canonical</span> resolves a network label to its canonical binary routing form: the mapped, single-label SUCS stream. Two labels are <strong>routing-equivalent</strong> exactly when their canonical forms are codepoint-identical, so <span class="mono">B&Uuml;CHER</span>, <span class="mono">b&uuml;cher</span>, and a fullwidth-capital form all resolve to <span class="mono">U+0062 U+00FC U+0063 U+0068 U+0065 U+0072</span> and reach the same endpoint.' }
        @{ t = 'h2'; html = 'Validity criteria' }
        @{ t = 'ul'; items = @(
            '<strong>Hyphen position</strong> &mdash; no leading or trailing hyphen, and no hyphen in both the third and fourth positions.'
            '<strong>Reserved prefix</strong> &mdash; the label does not begin with the three codepoints <span class="mono">x n -</span>, reserved for the deprecated ACE encoding.'
            '<strong>STD3 ASCII</strong> &mdash; every ASCII codepoint is <span class="mono">[a-z0-9-]</span>; non-ASCII codepoints are unconstrained.'
            '<strong>ContextJ</strong> &mdash; every <span class="mono">U+200C</span> or <span class="mono">U+200D</span> is in an admissible context (Arabic or Devanagari virama).'
            '<strong>Bidi</strong> &mdash; a label containing an RTL letter does not begin with a combining mark, and a mixed RTL/LTR label ends in an RTL letter.'
            '<strong>DNS length</strong> &mdash; at most 63 codepoints per label; 253 per name.'
        ) }
        @{ t = 'h2'; html = 'Complexity' }
        @{ t = 'table'; head = @('Operation', 'Cost'); rows = @(
            @('<span class="mono">suts_sidn_status_of</span> / <span class="mono">suts_sidn_mapping_of</span>', 'O(1) expected (one hash, one bounded probe), no allocation')
            @('<span class="mono">suts_sidn_validate_label</span> / <span class="mono">suts_sidn_to_canonical</span>', 'one linear pass, O(n) in label length')
            @('Each validity criterion', 'O(1) after the normalized stream is known')
            @('<span class="mono">suts_sidn_zone_of</span> / <span class="mono">suts_sidn_hash_codepoint</span>', 'O(1)')
        ) }
        @{ t = 'h2'; html = 'Security integration with SCSS' }
        @{ t = 'p'; html = 'SIDN and <a href="SUTS-005.html">SUTS-005 (SCSS)</a> divide the network-naming problem. SIDN answers <em>is this name well-formed and does it name one endpoint</em>; SCSS answers <em>does this name impersonate another</em>. They compose on the routing path: validate and canonicalize with SIDN, screen the canonical form for homographs and script mixing with SCSS, and admit the name only when both agree.' }
        @{ t = 'table'; head = @('Feature', 'Status'); rows = @(
            @('Binary, fixed-size mapping records (16 bytes)', 'Yes')
            @('Zero heap allocation on any path', 'Yes')
            @('O(1) expected Status and mapping resolution', 'Yes')
            @('Single-pass validation and canonical routing', 'Yes')
            @('Separator folding to U+002E', 'Yes')
            @('Deviation handling, nontransitional default', 'Yes')
            @('STD3 / hyphen / reserved-prefix criteria', 'Yes')
            @('ContextJ and Bidi criteria', 'Yes')
            @('Bridge + RNUR coverage', 'Yes')
            @('SCP invariance; traps and Sentinel refused', 'Yes')
            @('Closed-by-default (total) Status', 'Yes')
            @('31-bit SUCS and 64-bit ExtSUCS compatibility', 'Yes')
        ) }
        @{ t = 'callout'; html = 'Reference implementation: <span class="mono">include/suts/suts_sidn.h</span> + <span class="mono">src/suts/suts_sidn.c</span> (freestanding C99, no heap), registered in <span class="mono">suts_static</span>.' }
        @{ t = 'note'; html = 'Source of truth: <span class="mono">docs/suts/SUTS-006-sidn.md</span> in the repository.' }
    )
}

$Pages['reports/SUTS-007'] = @{
    path    = 'reports/SUTS-007.html'
    sec     = 'reports'
    title   = 'SUTS-007 — SuperUnicode Emoji & Presentation Specification (SUEPS)'
    desc    = 'SUTS-007: SuperUnicode Emoji & Presentation Specification (SUEPS) — zero-allocation, O(1) emoji presentation and flattened ZWJ / modifier sequence resolution over 31-bit SUCS (a UTS #51 analog).'
    crumbName = 'SUTS-007 — SUEPS'
    crumbs  = @( @{ label = 'Technical Reports'; href = 'index.html' } )
    h1      = 'SUTS-007 &mdash; SuperUnicode <span class="grad">Emoji &amp; Presentation</span>'
    subtitle= 'The seventh SuperUnicode Technical Specification: a zero-allocation, O(1) engine for emoji presentation and flattened ZWJ / modifier sequence resolution in the OpenWindows microkernel.'
    body    = @(
        @{ t = 'p'; html = 'SUTS-007 (SUEPS) is the seventh SuperUnicode Technical Specification and a <strong>core kernel primitive</strong> of the OpenWindows microkernel. It establishes a <strong>binary-compiled, zero-allocation architecture</strong> for emoji presentation &mdash; resolving a codepoint to its presentation flags and a base + modifier pair to a precompiled sequence token &mdash; replacing the text-based pipeline of Unicode&rsquo;s <strong>UTS #51</strong> (<span class="mono">emoji-data.txt</span>, <span class="mono">emoji-test.txt</span>, a ZWJ sequence scanner) with fixed-size, byte-aligned memory layouts optimized for O(1) kernel execution.' }
        @{ t = 'h2'; html = 'Why not a text emoji library' }
        @{ t = 'ul'; items = @(
            '<strong>Atomic scalars vs. surrogate pairs</strong> &mdash; in UTF-16 an emoji above the BMP is a surrogate pair (&#128512; is <span class="mono">U+1F600</span>, encoded <span class="mono">\uD83D\uDE00</span>) that every consumer must recombine; SuperUnicode has no surrogate mechanism at all, so <span class="mono">U+1F600</span> is one atomic 31-bit scalar and there is nothing to recombine.'
            '<strong>Flattened table vs. string parser</strong> &mdash; UTS #51 leaves a modifier sequence, a ZWJ sequence, a flag pair, a keycap sequence, and a tag sequence to a sequence-aware scanner; SUEPS flattens every admitted chain into static binary transition tables resolved by one hash and one bounded probe.'
            '<strong>Zero allocation vs. heap maps</strong> &mdash; a user-space emoji layer loads the annotation set into heap-resident hash maps and buffers each sequence stage; SUEPS resolves every property from one static table and one fixed index, holding no per-query state.'
            '<strong>Kernel primitive vs. user-space</strong> &mdash; UTS #51 targets chat and email applications reading data files from disk; SUEPS gives the graphical subsystem, the text-rendering path, and the emoji palette deterministic, in-band verdicts.'
        ) }
        @{ t = 'h2'; html = 'The &ldquo;No Surrogates&rdquo; mandate' }
        @{ t = 'p'; html = 'SUEPS inherits the second non-negotiable invariant of the SuperUnicode transformation formats, <a href="SUTR-1.html">SUTR-1 (SUTF)</a> &sect;1.2: <em>SuperUnicode has no surrogate mechanism. The range <span class="mono">U+D800&ndash;U+DFFF</span> is an ordinary block of valid PUA codepoints and MUST be encoded as such &mdash; never paired, never reserved, never rejected.</em> A Unicode emoji above the BMP is one surrogate pair in UTF-16 and every consumer must recombine it; SUEPS has no such step, because there is no such pair. <span class="mono">U+D83D</span> alone is not a broken half &mdash; it is an ordinary valid PUA codepoint, and <span class="mono">suts_sueps_is_pua_half</span> classifies that block rather than rejecting it.' }
        @{ t = 'table'; head = @('', 'UTF-16 (UTS #51)', 'SuperUnicode (SUEPS)'); rows = @(
            @('&#128512; <span class="mono">U+1F600</span>', 'two code units <span class="mono">D83D DE00</span>, recombined', 'one atomic 31-bit scalar <span class="mono">0x0001F600</span>')
            @('<span class="mono">U+D83D</span> alone', 'an unpaired surrogate: ill-formed', 'an ordinary valid PUA codepoint')
            @('<span class="mono">U+DC00</span> alone', 'an unpaired surrogate: ill-formed', 'an ordinary valid PUA codepoint')
        ) }
        @{ t = 'h2'; html = 'Dual-layer architecture' }
        @{ t = 'table'; head = @('Layer', 'Role'); rows = @(
            @('<span class="mono">SUCD</span> &mdash; offline reference', 'Full human-readable emoji annotation files, sequence charts, and presentation property manifests for user-space applications, font designers, and toolchains')
            @('<span class="mono">SUEPS</span> &mdash; runtime kernel', 'Immutable, byte-packed binary lookup tables (<span class="mono">sueps_emoji_entry_t</span>, the flattened modifier transition table) queried by the kernel in O(1)')
        ) }
        @{ t = 'p'; html = 'As with <a href="SUAS-009.html">SIP</a>, <a href="SUAS-010.html">SSRE</a>, <a href="SUAS-012.html">SUCD</a>, and <a href="SUTS-006.html">SIDN</a>, the brief named the offline layer &ldquo;SUCA&rdquo;; in this corpus <strong>SUCA</strong> is already the SuperUnicode Collation Algorithm (<a href="SUTS-001.html">SUTS-001</a>), so SUEPS names <span class="mono">SUCD</span> (<a href="SUAS-012.html">SUAS-012</a>) as its reference layer. The house prefix <span class="mono">SUEPS_*</span> and the entry points <span class="mono">suts_sueps_init_registry</span>, <span class="mono">suts_sueps_get_entry</span>, and <span class="mono">suts_sueps_resolve_sequence</span> are preserved verbatim.' }
        @{ t = 'h2'; html = 'Address-space model' }
        @{ t = 'p'; html = 'SUEPS does not invent a second plane taxonomy: <span class="mono">suts_sueps_zone_of</span> classifies a codepoint into the SUCD / SSP planes with the RNUR carve-out made explicit. Only the <strong>Bridge</strong> (<span class="mono">0x00000000&ndash;0x0010FFFF</span>), the <strong>RNUR</strong> neography zone (<span class="mono">0x00160000&ndash;0x0016FFFF</span>), and the <strong>Native</strong> plane are emoji material; the Control Plane, the trap slots, the Sentinel, and ExtSUCS are never emoji material. The former-surrogate block sits squarely inside the Bridge zone: it is renderable, because SUTR-1 makes it ordinary PUA.' }
        @{ t = 'h2'; html = 'Compiled emoji metadata' }
        @{ t = 'p'; html = 'A compiled emoji record is a fixed 12-byte entry binding a 31-bit codepoint to its packed presentation flags, a precomputed sequence-token hash, and a modifier-base reference; the sketch&rsquo;s field layout is preserved unchanged. Codepoints are 31-bit SUCS values, so an RNUR neography emoji in <span class="mono">0x00160000&ndash;0x0016FFFF</span> is classified correctly and an ExtSUCS plugin codepoint above <span class="mono">0x7FFFFFFF</span> is refused.' }
        @{ t = 'spec'; html = '#define SUEPS_ENTRY_CAPACITY    128U<br>#define SUEPS_ENTRY_INDEX_SIZE  256U<br><br>#define SUEPS_FLAG_EMOJI          (1U &lt;&lt; 0)<br>#define SUEPS_FLAG_TEXT_DEFAULT   (1U &lt;&lt; 1)<br>#define SUEPS_FLAG_MODIFIER       (1U &lt;&lt; 2)<br>#define SUEPS_FLAG_ZWJ_SEQUENCE   (1U &lt;&lt; 3)<br><br>typedef struct {<br>    uint32_t codepoint;          /* Direct 31-bit codepoint (No Surrogates) */<br>    uint32_t sequence_hash;      /* Precomputed token hash for chains        */<br>    uint16_t presentation_flags; /* Packed attribute flags                   */<br>    uint8_t  modifier_base;      /* Associated base character reference      */<br>    uint8_t  reserved;           /* must be zero                             */<br>} sueps_emoji_entry_t;   /* exactly 12 bytes */' }
        @{ t = 'h2'; html = 'Presentation class and sequence resolution' }
        @{ t = 'p'; html = 'A curated emoji records whether it is default emoji presentation (ED-6) or default text presentation (ED-7); <span class="mono">U+FE0E</span> (VS15) requests a text presentation and <span class="mono">U+FE0F</span> (VS16) an emoji presentation, and both are components, never emoji alone. A base + modifier pair resolves through the flattened transition table: <span class="mono">suts_sueps_resolve_sequence(0x1F44D, 0x1F3FD)</span> returns the base record with <span class="mono">SUEPS_FLAG_MODIFIER</span> and <span class="mono">SUEPS_FLAG_ZWJ_SEQUENCE</span> set and <span class="mono">sequence_hash</span> set to the precomputed pair token &mdash; no re-scan and no dynamic buffer.' }
        @{ t = 'h2'; html = 'Resolution codes' }
        @{ t = 'table'; head = @('Code', 'Meaning'); rows = @(
            @('<span class="mono">SUEPS_ERR_NULL_ARGUMENT</span>', 'a NULL output descriptor')
            @('<span class="mono">SUEPS_ERR_NOT_EMOJI</span>', 'the base carries no emoji record')
            @('<span class="mono">SUEPS_ERR_BAD_BASE</span>', 'the base is not an emoji modifier base (ED-12)')
            @('<span class="mono">SUEPS_ERR_BAD_MODIFIER</span>', 'the second codepoint is not an emoji modifier (ED-11)')
            @('<span class="mono">SUEPS_ERR_ILLEGAL_ZONE</span>', 'a Control Plane / trap / Sentinel / ExtSUCS argument')
            @('<span class="mono">SUEPS_ERR_UNQUALIFIED</span>', 'the pair is not a qualified sequence')
        ) }
        @{ t = 'h2'; html = 'Complexity' }
        @{ t = 'table'; head = @('Operation', 'Cost'); rows = @(
            @('<span class="mono">suts_sueps_get_entry</span> / <span class="mono">is_emoji</span> and the other predicates', 'O(1) expected (one hash, one bounded probe), no allocation')
            @('<span class="mono">suts_sueps_resolve_sequence</span>', 'O(1) expected: one pair hash, one probe')
            @('<span class="mono">suts_sueps_zone_of</span> / <span class="mono">is_renderable</span> / <span class="mono">is_neography</span>', 'O(1) comparison chain')
            @('<span class="mono">suts_sueps_is_modifier</span> / <span class="mono">is_regional_indicator</span>', 'O(1) range test')
        ) }
        @{ t = 'h2'; html = 'Modular optional compliance' }
        @{ t = 'p'; html = 'SUEPS is a <strong>modular</strong> specification. The architectural contract &mdash; the record layout, the lookup, the flattened transition table, the &ldquo;No Surrogates&rdquo; rule &mdash; is mandatory for an implementation that claims SUEPS support; but a lightweight server kernel or a headless environment MAY omit the module entirely, because core primitives, file paths, and kernel IPC channels require strict text safety rather than graphical emoticons.' }
        @{ t = 'table'; head = @('Feature', 'Status'); rows = @(
            @('Binary, fixed-size emoji records (12 bytes)', 'Yes')
            @('Zero heap allocation on any path', 'Yes')
            @('O(1) expected presentation resolution', 'Yes')
            @('O(1) expected modifier-sequence resolution', 'Yes')
            @('Flattened ZWJ / modifier transition table', 'Yes')
            @('&ldquo;No Surrogates&rdquo;: <span class="mono">U+D800&ndash;U+DFFF</span> as ordinary PUA', 'Yes')
            @('Unique, ascending codepoint key in the table', 'Yes')
            @('Total property functions (algorithmic default)', 'Yes')
            @('Bridge + RNUR coverage', 'Yes')
            @('SCP invariance; traps and Sentinel refused', 'Yes')
            @('Modular optionality (module omittable)', 'Yes')
            @('31-bit SUCS and 64-bit ExtSUCS compatibility', 'Yes')
        ) }
        @{ t = 'callout'; html = 'Reference implementation: <span class="mono">include/suts/suts_sueps.h</span> + <span class="mono">src/suts/suts_sueps.c</span> (freestanding C99, no heap), registered in <span class="mono">suts_static</span>.' }
        @{ t = 'note'; html = 'Source of truth: <span class="mono">docs/suts/SUTS-007-sueps.md</span> in the repository.' }
    )
}

$Pages['reports/SUTS-008'] = @{
    path    = 'reports/SUTS-008.html'
    sec     = 'reports'
    title   = 'SUTS-008 — SuperUnicode Source Code Handling (SUSCH)'
    desc    = 'SUTS-008: SuperUnicode Source Code Handling (SUSCH) — zero-allocation, single-pass source code safety attributes, identifier validation, bidi / invisible elimination, homoglyph risk, and identifier chunk analysis over 31-bit SUCS (a UTS #55 analog).'
    crumbName = 'SUTS-008 — SUSCH'
    crumbs  = @( @{ label = 'Technical Reports'; href = 'index.html' } )
    h1      = 'SUTS-008 &mdash; SuperUnicode <span class="grad">Source Code Handling</span>'
    subtitle= 'The eighth SuperUnicode Technical Specification: a zero-allocation, single-pass engine for source code safety screening in the OpenWindows microkernel.'
    body    = @(
        @{ t = 'p'; html = 'SUTS-008 (SUSCH) is the eighth SuperUnicode Technical Specification and a <strong>core kernel primitive</strong> of the OpenWindows microkernel. It establishes a <strong>binary-compiled, zero-allocation architecture</strong> for source code handling &mdash; compiling a codepoint to a fixed-size safety attribute record, validating a whole identifier token in a single pass, and reporting homoglyph, mixed-script, and cross-zone findings as advisory bitmaps &mdash; replacing the text-scanning pipeline of Unicode&rsquo;s <strong>UTS #55</strong> (a maintainer-mode lexer, a set of regular-expression-style filters over identifier characters, and a per-file diagnostic report) with fixed-size, byte-aligned memory layouts optimized for O(1) kernel execution.' }
        @{ t = 'h2'; html = 'Why not a text scanner' }
        @{ t = 'ul'; items = @(
            '<strong>Token validator vs. file scanner</strong> &mdash; UTS #55 must decide where an identifier begins and ends inside arbitrary text, so it carries a maintainer-mode lexer and needs a source line for every diagnostic; SUSCH is handed the codepoint stream of one identifier, already delimited, and never re-lexes it.'
            '<strong>Verdict and bitmap vs. diagnostic report</strong> &mdash; a linter emits prose with line numbers and ranges for a human reader; SUSCH emits a boolean verdict, a closed outcome code, and a packed advisory word, because the consumer is a compiler front end, a kernel script engine, or a CI gate.'
            '<strong>Compiled tables vs. runtime filters</strong> &mdash; UTS #55 loads properties and confusables data into memory and runs a filter over the text; SUSCH answers every per-codepoint question by a bounded binary search over an immutable static table, with no file reads and no per-file setup.'
            '<strong>Zero allocation vs. per-line buffers</strong> &mdash; a user-space linter allocates per-line buffers, per-match records, and a report tree; SUSCH allocates nothing at all, and holds a fixed number of registers whatever the token&rsquo;s length.'
        ) }
        @{ t = 'h2'; html = 'The threat model' }
        @{ t = 'table'; head = @('Vector', 'What the reader sees', 'What the compiler reads', 'SUSCH&rsquo;s finding'); rows = @(
            @('Right-to-left override (RLO)', '<span class="mono">txt.exe</span>', '<span class="mono">exe.txt</span>', 'Bidi control &mdash; <strong>defect</strong>')
            @('Zero-width space', '<span class="mono">pay ment</span>', '<span class="mono">pay ment</span>', 'Invisible &mdash; <strong>defect</strong>')
            @('Cyrillic homoglyph', '<span class="mono">pаyment</span>', 'a different identifier', 'Homoglyph risk &mdash; <strong>advisory</strong>')
            @('Mixed-script chunk', '<span class="mono">pаyment</span>', 'one identifier, two scripts', 'Mixed script &mdash; <strong>advisory</strong>')
            @('Neography substitution', 'an unknown glyph', 'a native-plane identifier', 'Cross-zone &mdash; <strong>advisory</strong>')
            @('Tag or format character', 'nothing at all', 'a hidden codepoint', 'Invisible &mdash; <strong>defect</strong>')
        ) }
        @{ t = 'p'; html = 'The division of labour follows UTS #55: a directional control or an invisible character makes a token <strong>invalid</strong>, while a confusable, a mixed-script chunk, and a chunk that crosses the Bridge boundary are <strong>warnings</strong> that never change the verdict. A legitimate Greek or Cyrillic identifier must still compile &mdash; the finding is what a linter or a CI gate acts on, not a rejection.' }
        @{ t = 'h2'; html = 'Dual-layer architecture' }
        @{ t = 'table'; head = @('Layer', 'Role'); rows = @(
            @('<span class="mono">SUCD</span> &mdash; offline reference', 'Full human-readable confusables manifests, identifier-status tables, Default_Ignorable_Code_Point inventories, and directional-formatting-character inventories for user-space compilers, linters, IDEs, and developer toolchains')
            @('<span class="mono">SUSCH</span> &mdash; runtime kernel', 'Immutable, byte-packed binary lookup tables (the class, skeleton, identifier, script, and case tables) queried by the kernel in O(1), with a 12-byte attribute record per codepoint')
        ) }
        @{ t = 'p'; html = 'The seeding brief for this specification named its offline reference layer &ldquo;SUCD &mdash; SUAS-014&rdquo;. That is stale: in this corpus <strong>SUCD</strong> is the SuperUnicode Character Database (<a href="SUAS-012.html">SUAS-012</a>), while SUAS-014 is <strong>SUVTL</strong>, the SuperUnicode Version and Licensing Table, which has nothing to do with character data. SUSCH therefore names <span class="mono">SUCD</span> (<a href="SUAS-012.html">SUAS-012</a>) as its reference layer. The house prefix <span class="mono">SUSCH_*</span> and the entry points <span class="mono">suts_susch_init_engine</span>, <span class="mono">suts_susch_get_attributes</span>, and <span class="mono">suts_susch_validate_token_stream</span> are preserved verbatim.' }
        @{ t = 'h2'; html = 'The compiled attribute' }
        @{ t = 'p'; html = 'A compiled source attribute is a fixed 12-byte entry binding a 31-bit codepoint to its packed security mask, its coarse script classification, and its three-state identifier verdict; the sketch&rsquo;s field layout is preserved unchanged. The four sketched flags keep bits 0&ndash;3, the derived flags occupy bits 4&ndash;12, and the query is <strong>total</strong> over the whole 31-bit address space: an unclassified codepoint is answered by the algorithmic default rather than by a lookup failure.' }
        @{ t = 'spec'; html = '#define SUSCH_FLAG_VALID_IDENTIFIER (1U &lt;&lt; 0)<br>#define SUSCH_FLAG_BIDI_CONTROL      (1U &lt;&lt; 1)<br>#define SUSCH_FLAG_INVISIBLE         (1U &lt;&lt; 2)<br>#define SUSCH_FLAG_HOMOGLYPH_RISK    (1U &lt;&lt; 3)<br><br>typedef struct {<br>    uint32_t codepoint;         /* Raw 31-bit SuperUnicode codepoint */<br>    uint32_t security_mask;     /* Packed source code safety attributes */<br>    uint16_t script_category;   /* Associated script classification */<br>    uint8_t  identifier_status; /* Allowed, Restricted, or Disallowed */<br>    uint8_t  reserved;          /* must be zero */<br>} susch_source_attr_t;   /* exactly 12 bytes */' }
        @{ t = 'h2'; html = 'Identifier admission' }
        @{ t = 'p'; html = 'Every codepoint carries a three-state admission verdict: <strong>Allowed</strong> (admissible with no caveat), <strong>Restricted</strong> (admissible, but carrying a caveat the caller should see &mdash; a confusable prototype, a joining control, a neography character), and <strong>Disallowed</strong> (never identifier material). The positional rule is separate: <span class="mono">suts_susch_is_identifier_start</span> and <span class="mono">suts_susch_is_identifier_continue</span> decide whether a codepoint may open or extend an identifier, so a decimal digit continues happily and never starts.' }
        @{ t = 'h2'; html = 'Identifier chunks' }
        @{ t = 'p'; html = 'Chunk boundaries are the camel, hat, and snake boundaries of UTS #55 &sect;5.1.2.1, expressed in terms of compiled case classes rather than runtime property lookups. A combining mark never forms a chunk of its own: it belongs to the letter it follows.' }
        @{ t = 'table'; head = @('Token', 'Chunks', 'Rule'); rows = @(
            @('<span class="mono">HTTPRequest</span>', '<span class="mono">HTTP</span> | <span class="mono">Request</span>', 'HAT before <span class="mono">R</span>')
            @('<span class="mono">TypeII</span>', '<span class="mono">Type</span> | <span class="mono">II</span>', 'CAMEL before <span class="mono">I</span>')
            @('<span class="mono">OCaml</span>', '<span class="mono">O</span> | <span class="mono">Caml</span>', 'HAT before <span class="mono">C</span>')
            @('<span class="mono">my_variable</span>', '<span class="mono">my</span> | <span class="mono">_</span> | <span class="mono">variable</span>', 'SNAKE on both sides')
            @('<span class="mono">abc</span>', '<span class="mono">abc</span>', 'no rule fires')
        ) }
        @{ t = 'p'; html = 'The hat rule needs to see the codepoint after the one it applies to, so the boundary decision is taken with a one-codepoint lookahead. The engine holds two chunk-significant codepoints in registers and never re-reads the stream: the pass stays linear.' }
        @{ t = 'h2'; html = 'The RNUR neography carve-out' }
        @{ t = 'p'; html = 'SUSCH does not invent a second plane taxonomy: <span class="mono">suts_susch_zone_of</span> classifies a codepoint into the SUCD / SSP planes with the RNUR carve-out made explicit. Only the <strong>Bridge</strong> (<span class="mono">0x00000000&ndash;0x0010FFFF</span>) and the <strong>RNUR</strong> neography zone (<span class="mono">0x00160000&ndash;0x0016FFFF</span>) carry source text; the Control Plane, the trap slots, the Sentinel, and ExtSUCS are never source material. Within the RNUR, every <span class="mono">0x100</span>-aligned block is one neography set: offsets <span class="mono">0x00&ndash;0x5F</span> are letter positions, <span class="mono">0x60&ndash;0x7F</span> are continuation-only, and <span class="mono">0x80&ndash;0xFF</span> are symbol positions that may never appear in an identifier. A neography character is <strong>Restricted</strong> rather than Allowed, because the shipped table cannot vouch for a profile it does not carry.' }
        @{ t = 'h2'; html = 'Outcome codes' }
        @{ t = 'table'; head = @('Code', 'Meaning'); rows = @(
            @('<span class="mono">SUSCH_TOKEN_OK</span>', 'the stream is a valid identifier')
            @('<span class="mono">SUSCH_TOKEN_ERR_NULL</span>', 'a NULL stream with a non-zero length')
            @('<span class="mono">SUSCH_TOKEN_ERR_EMPTY</span>', 'a zero-length identifier')
            @('<span class="mono">SUSCH_TOKEN_ERR_ILLEGAL_ZONE</span>', 'a trap slot, the Sentinel, the SCP, or ExtSUCS')
            @('<span class="mono">SUSCH_TOKEN_ERR_BIDI_CONTROL</span>', 'a directional formatting character')
            @('<span class="mono">SUSCH_TOKEN_ERR_INVISIBLE</span>', 'a zero-width or invisible character')
            @('<span class="mono">SUSCH_TOKEN_ERR_LINE_BREAK</span>', 'a line terminator inside the token')
            @('<span class="mono">SUSCH_TOKEN_ERR_INVALID_START</span>', 'the first codepoint may not start an identifier')
            @('<span class="mono">SUSCH_TOKEN_ERR_INVALID_CONTINUE</span>', 'a later codepoint may not continue one')
        ) }
        @{ t = 'p'; html = 'The four advisory bits &mdash; <span class="mono">SUSCH_WARN_HOMOGLYPH</span>, <span class="mono">SUSCH_WARN_MIXED_SCRIPT</span>, <span class="mono">SUSCH_WARN_CROSS_ZONE</span>, and <span class="mono">SUSCH_WARN_RESTRICTED</span> &mdash; never change the verdict. The two joining controls U+200C and U+200D are the one exception to the invisible rule: UTS #55 &sect;5.1.3 permits them under contextual restriction, so they stay admissible and are reported through the advisory channel instead.' }
        @{ t = 'h2'; html = 'Complexity' }
        @{ t = 'table'; head = @('Operation', 'Cost'); rows = @(
            @('<span class="mono">suts_susch_get_attributes</span>', 'O(1): a bounded binary search per table, no allocation')
            @('<span class="mono">suts_susch_skeleton_of</span> / <span class="mono">confusable_with</span>', 'O(1): one or two skeleton folds')
            @('<span class="mono">suts_susch_zone_of</span> / <span class="mono">is_neography</span>', 'O(1) comparison chain')
            @('<span class="mono">suts_susch_validate_token_stream</span>', 'O(n) in the token length, single pass, no allocation')
            @('<span class="mono">suts_susch_chunk_count</span> / <span class="mono">is_mixed_script</span>', 'O(n) in the token length, single pass, no allocation')
        ) }
        @{ t = 'h2'; html = 'Modular optional compliance' }
        @{ t = 'p'; html = 'SUSCH is a <strong>modular</strong> specification. The architectural contract &mdash; the attribute layout, the compiled tables, the chunk grammar, the defect / advisory split &mdash; is mandatory for an implementation that claims SUSCH support; but a minimal kernel MAY omit the module entirely, because core operating system primitives, file paths, and kernel IPC channels need strict text safety rather than a source code linter. An implementation that omits SUSCH obtains the equivalent guarantees from the user-space reference layer (SUCD) directly.' }
        @{ t = 'table'; head = @('Feature', 'Status'); rows = @(
            @('Binary, fixed-size source attribute (12 bytes)', 'Yes')
            @('Zero heap allocation on any path', 'Yes')
            @('O(1) attribute query per codepoint', 'Yes')
            @('Total attribute query over the 31-bit space', 'Yes')
            @('Sorted, non-overlapping compiled tables', 'Yes')
            @('Bidi control detection (explicit, implicit, isolate)', 'Yes')
            @('Invisible / zero-width detection', 'Yes')
            @('Homoglyph risk and confusable pairing', 'Yes')
            @('UTS #55 &sect;5.1.2.1 chunk boundaries', 'Yes')
            @('Mixed-script chunk detection', 'Yes')
            @('Bridge / Native cross-zone detection', 'Yes')
            @('RNUR neography carve-out with a positional convention', 'Yes')
            @('Single linear pass over the token stream', 'Yes')
            @('Defects distinguished from advisories', 'Yes')
            @('Modular optionality (module omittable)', 'Yes')
            @('31-bit SUCS and 64-bit ExtSUCS compatibility', 'Yes')
        ) }
        @{ t = 'callout'; html = 'Reference implementation: <span class="mono">include/suts/suts_susch.h</span> + <span class="mono">src/suts/suts_susch.c</span> (freestanding C99, no heap), registered in <span class="mono">suts_static</span>.' }
        @{ t = 'note'; html = 'Source of truth: <span class="mono">docs/suts/SUTS-008-susch.md</span> in the repository.' }
    )
}

$Pages['reports/SUTS-009'] = @{
    path    = 'reports/SUTS-009.html'
    sec     = 'reports'
    title   = 'SUTS-009 — SuperUnicode Link Detection & Formatting (SULDF)'
    desc    = 'SUTS-009: SuperUnicode Link Detection & Formatting (SULDF) — zero-allocation, byte-packed, O(1) link detection, termination, and minimal escaping over 31-bit SUCS (a UTS #58 analog).'
    crumbName = 'SUTS-009 — SULDF'
    crumbs  = @( @{ label = 'Technical Reports'; href = 'index.html' } )
    h1      = 'SUTS-009 &mdash; SuperUnicode <span class="grad">Link Detection &amp; Formatting</span>'
    subtitle= 'The ninth SuperUnicode Technical Specification: a zero-allocation, byte-packed engine for URL and email detection in the OpenWindows microkernel.'
    body    = @(
        @{ t = 'p'; html = 'SUTS-009 (SULDF) is the ninth SuperUnicode Technical Specification and a <strong>core kernel primitive</strong> of the OpenWindows microkernel. It establishes a <strong>binary-compiled, zero-allocation architecture</strong> for link detection &mdash; compiling a codepoint to its <span class="mono">Link_Term</span>, <span class="mono">Link_Bracket</span>, and <span class="mono">Link_Email</span> attributes, scanning a codepoint stream for URLs and email addresses in a single pass, and serializing a URL Part with minimal percent escaping &mdash; replacing the text-scanning pipeline of Unicode&rsquo;s <strong>UTS #58</strong> (property files, a candidate finder, a regex-driven termination algorithm) with fixed-size, byte-aligned memory layouts optimized for O(1) kernel execution.' }
        @{ t = 'h2'; html = 'Why not a text scanner' }
        @{ t = 'ul'; items = @(
            '<strong>Codepoint stream vs. document</strong> &mdash; UTS #58 is written against the text of a document and its surrounding markup; SULDF is handed the codepoint sequence a terminal emulator, chat client, or mail reader already holds, and never re-lexes a byte index.'
            '<strong>Compiled tables vs. runtime property files</strong> &mdash; UTS #58 loads <span class="mono">LinkTerm.txt</span>, <span class="mono">LinkBracket.txt</span>, and <span class="mono">LinkEmail.txt</span> and interprets them with a regex; SULDF embeds the merged, sorted, non-overlapping ranges directly in the image and probes them with a bounded binary search.'
            '<strong>Zero allocation vs. candidate buffers</strong> &mdash; a user-space linkifier allocates candidate buffers, per-match records, and a report tree; SULDF allocates nothing at all, and holds one fixed 125-entry bracket stack on the scanner&rsquo;s own frame.'
            '<strong>A verdict vs. a report</strong> &mdash; UTS #58 reports match offsets for a human or a renderer; SULDF returns a half-open codepoint span and a <span class="mono">kind</span>, because the consumer is a kernel text primitive.'
        ) }
        @{ t = 'h2'; html = 'The boundary model' }
        @{ t = 'table'; head = @('Vector', 'Naive scanner', 'UTS #58 / SULDF'); rows = @(
            @('Sentence period', 'eats <span class="mono">example.com.</span>', 'drops the Soft-final period')
            @('Trailing <span class="mono">!</span>', 'eats <span class="mono">example.com/a!</span>', 'drops the Soft-final <span class="mono">!</span>')
            @('Unmatched <span class="mono">)</span>', 'eats <span class="mono">example.com/a)</span>', 'drops the unmatched Close')
            @('Matched <span class="mono">(b)</span>', 'misplaces the tail', 'keeps the matched pair')
            @('Empty label', 'links <span class="mono">foo..example.com</span>', 'rejects the empty label')
            @('Userinfo', 'links <span class="mono">http://user@host/</span>', 'rejects the <span class="mono">@</span> before the host')
            @('Consecutive dots', 'links <span class="mono">john..doe@x.com</span>', 'rejects the local-part')
            @('Trailing-dot local-part', 'links <span class="mono">john.@x.com</span>', 'rejects the local-part')
        ) }
        @{ t = 'h2'; html = 'Dual-layer architecture' }
        @{ t = 'table'; head = @('Layer', 'Role'); rows = @(
            @('<span class="mono">SUCD</span> &mdash; offline reference', 'The human-readable <span class="mono">LinkTerm.txt</span>, <span class="mono">LinkBracket.txt</span>, and <span class="mono">LinkEmail.txt</span> inventories (Unicode 18.0.0) for user-space linkifiers, validators, and toolchains')
            @('<span class="mono">SULDF</span> &mdash; runtime kernel', 'Immutable, byte-packed binary range tables (term, email, bracket) queried by the kernel in O(1), with a 12-byte link attribute record per codepoint')
        ) }
        @{ t = 'p'; html = 'The seeding sketch preserved the house prefix <span class="mono">SULDF_*</span>, the entry points <span class="mono">suts_suldf_init_engine</span>, <span class="mono">suts_suldf_get_attributes</span>, and <span class="mono">suts_suldf_scan_stream</span>, the four sketch flags (<span class="mono">SCHEME_CHAR</span>, <span class="mono">DOMAIN_CHAR</span>, <span class="mono">PATH_CHAR</span>, <span class="mono">EMAIL_DELIM</span>), and the 12-byte attribute layout. All are preserved verbatim. Like SIP, SSRE, SUCD, SIDN, SUEPS, and SUSCH, SULDF names <span class="mono">SUCD</span> (<a href="SUAS-012.html">SUAS-012</a>) as its reference layer.' }
        @{ t = 'h2'; html = 'The compiled attribute' }
        @{ t = 'p'; html = 'A compiled link attribute is a fixed 12-byte entry binding a 31-bit codepoint to its packed attribute word, its <span class="mono">Link_Term</span> category, and its <span class="mono">Link_Bracket</span> role; the sketch&rsquo;s field layout is preserved unchanged. The four sketched flags keep bits 0&ndash;3, the derived flags occupy bits 4&ndash;12, and the query is <strong>total</strong> over the whole 31-bit address space: an unlisted codepoint is answered by the algorithmic default <span class="mono">Link_Term = Hard</span> rather than by a lookup failure.' }
        @{ t = 'spec'; html = '#define SULDF_FLAG_SCHEME_CHAR   (1U &lt;&lt; 0)<br>#define SULDF_FLAG_DOMAIN_CHAR   (1U &lt;&lt; 1)<br>#define SULDF_FLAG_PATH_CHAR     (1U &lt;&lt; 2)<br>#define SULDF_FLAG_EMAIL_DELIM   (1U &lt;&lt; 3)<br><br>typedef struct {<br>    uint32_t codepoint;      /* Raw 31-bit SuperUnicode codepoint */<br>    uint32_t link_mask;      /* Packed link-detection attributes */<br>    uint16_t term_class;     /* Link_Term: Include / Hard / Soft / Close / Open */<br>    uint8_t  bracket_class;  /* Link_Bracket: None / Open / Close */<br>    uint8_t  reserved;       /* must be zero */<br>} suldf_link_attr_t;   /* exactly 12 bytes */' }
        @{ t = 'h2'; html = 'Link_Term' }
        @{ t = 'p'; html = 'A compiled attribute binds a codepoint to its Link_Term category and its Link_Bracket role; a compiled span carries its <span class="mono">kind</span> and half-open codepoint offsets. The sketch&rsquo;s field layout is preserved unchanged, and the query is <strong>total</strong> over the whole 31-bit address space: an unlisted codepoint is answered by the algorithmic default <span class="mono">Hard</span> rather than by a lookup failure.' }
        @{ t = 'table'; head = @('Category', 'Meaning'); rows = @(
            @('<span class="mono">Include</span>', 'keeps a link going')
            @('<span class="mono">Hard</span>', 'always terminates; the algorithmic default for unlisted codepoints')
            @('<span class="mono">Soft</span>', 'terminates only if the next character does not continue the link')
            @('<span class="mono">Close</span>', 'a closing bracket; terminates unless it matches an open bracket on the stack')
            @('<span class="mono">Open</span>', 'an opening bracket; pushed and kept')
        ) }
        @{ t = 'h2'; html = 'Termination and the bracket stack' }
        @{ t = 'p'; html = 'The termination algorithm walks the codepoints after a host in one pass with backup. An Open bracket is pushed on a fixed <strong>125-entry</strong> stack; a Close bracket is popped only if it matches the top of the stack, and terminates the link otherwise. A Part&rsquo;s <em>ClearStackOpen</em> set is checked before its Terminator, so a bracket opened in the Path is forgotten at the Query initiator. A Hard codepoint terminates, and a Soft codepoint that is not followed by Include material is removed from the link. The bounded stack is a security property: a hostile stream cannot drive an allocation or an unbounded walk.' }
        @{ t = 'table'; head = @('Input', 'Link', 'Why'); rows = @(
            @('<span class="mono">example.com/a b</span>', '<span class="mono">example.com/a</span>', 'the space is Hard')
            @('<span class="mono">example.com/a!b</span>', '<span class="mono">example.com/a!b</span>', '<span class="mono">!</span> is Soft but Include follows')
            @('<span class="mono">example.com/a!</span>', '<span class="mono">example.com/a</span>', 'trailing Soft is dropped')
            @('<span class="mono">example.com. on</span>', '<span class="mono">example.com</span>', 'sentence-final Soft dot is dropped')
            @('<span class="mono">example.com/a(b)c</span>', '<span class="mono">example.com/a(b)c</span>', 'matched brackets are kept')
            @('<span class="mono">example.com/a)</span>', '<span class="mono">example.com/a</span>', 'unmatched Close ends the link')
            @('<span class="mono">example.com/a?b#c</span>', '<span class="mono">example.com/a?b#c</span>', 'Path &rarr; Query &rarr; Fragment')
        ) }
        @{ t = 'h2'; html = 'URL initiation and email detection' }
        @{ t = 'p'; html = 'A URL is initiated by an explicit <span class="mono">http://</span> or <span class="mono">https://</span> scheme or as a bare domain of at least two labels. The host parser admits labels whose ends are not hyphen edges, accepts the IDNA dot equivalents (<span class="mono">U+002E</span>, <span class="mono">U+3002</span>, <span class="mono">U+FF0E</span>, <span class="mono">U+FF61</span>), parses an optional <span class="mono">:port</span>, and rejects an empty label. <strong>Userinfo is rejected</strong>: a <span class="mono">scheme://user@host</span> is not a link.' }
        @{ t = 'p'; html = 'An email detection begins at an <span class="mono">@</span>, walks backward over the <span class="mono">Link_Email</span> local-part (rejecting a leading, trailing, or consecutive dot), and parses the domain with the same host parser. A preceding <span class="mono">mailto:</span> is included. The <span class="mono">Link_Email</span> table is also what answers the domain-label question, so a link can never span a codepoint that may not appear in a host.' }
        @{ t = 'h2'; html = 'Minimal escaping and quoting' }
        @{ t = 'p'; html = 'The formatter serializes one URL Part&rsquo;s interior with the UTS #58 minimal-escaping algorithm: the Part&rsquo;s <em>Terminator</em> and <em>ClearStackOpen</em> sets drive which literal characters must be percent-escaped. Non-ASCII bytes use <strong>SUTF-8</strong> (SUTR-1), so a codepoint above the Bridge is escaped by its full byte sequence. The return value follows the <span class="mono">snprintf</span> convention. Email local-parts that are not valid under <span class="mono">Link_Email</span> are wrapped in ASCII double quotes with the two RFC 5322 specials backslash-escaped.' }
        @{ t = 'h2'; html = 'Security considerations' }
        @{ t = 'ul'; items = @(
            '<strong>No userinfo</strong> &mdash; a <span class="mono">scheme://user@host</span> is not a link, so a renderer cannot present an attacker-controlled userinfo string as part of a trusted host.'
            '<strong>No empty or hyphen-edged labels</strong> &mdash; a host that cannot be a host is rejected rather than linked.'
            '<strong>Bounded brackets</strong> &mdash; the open-bracket stack is a fixed 125 entries, so a hostile stream cannot drive unbounded work.'
            '<strong>Bidi controls are visible</strong> &mdash; <span class="mono">suts_suldf_is_bidi_control</span> identifies the explicit controls, implicit marks, and isolate initiators, because a link whose displayed text disagrees with its codepoints is a spoof, not a link.'
        ) }
        @{ t = 'h2'; html = 'The RNUR neography carve-out' }
        @{ t = 'p'; html = 'Unlike a standards-conformant linkifier restricted to the Unicode Compatibility Bridge, SULDF admits the <strong>Native &amp; Neography Plane</strong> as link material. A Native or neography codepoint in a Path, Query, or Fragment is an Include character and is retained verbatim by the minimal-escaping formatter, so a custom script&rsquo;s URL stays readable instead of being silently percent-encoded. Only the <strong>Bridge</strong> (<span class="mono">0x00000000&ndash;0x0010FFFF</span>), the <strong>Native</strong> plane, and the <strong>RNUR</strong> neography carve-out (<span class="mono">0x00160000&ndash;0x0016FFFF</span>) carry link material; the Control Plane, the trap slots, the Sentinel, and ExtSUCS never do.' }
        @{ t = 'h2'; html = 'Complexity' }
        @{ t = 'table'; head = @('Operation', 'Cost'); rows = @(
            @('<span class="mono">suts_suldf_get_attributes</span>', 'O(1): a bounded binary search per table, no allocation')
            @('<span class="mono">suts_suldf_term_of</span> / the predicates', 'O(1): a bounded binary search, no allocation')
            @('<span class="mono">suts_suldf_zone_of</span> / <span class="mono">is_neography</span>', 'O(1) comparison chain')
            @('<span class="mono">suts_suldf_scan_stream</span>', 'O(n) in the stream length, single pass, one fixed 125-entry stack')
            @('<span class="mono">suts_suldf_escape_part</span>', 'O(n) in the Part length, no allocation')
            @('<span class="mono">suts_suldf_quote_email_local</span>', 'O(n) in the local-part length, no allocation')
        ) }
        @{ t = 'h2'; html = 'Modular optional compliance' }
        @{ t = 'p'; html = 'SULDF is a <strong>modular</strong> specification. The architectural contract &mdash; the attribute layout, the compiled tables, the termination algorithm, the escape and quote forms &mdash; is mandatory for an implementation that claims SULDF support; but a minimal kernel MAY omit the module entirely, because core operating system text primitives need strict link boundaries, not a hyperlink renderer. An implementation that omits SULDF obtains the equivalent guarantees from the user-space reference layer (SUCD) directly.' }
        @{ t = 'table'; head = @('Feature', 'Status'); rows = @(
            @('Binary, fixed-size link attribute (12 bytes)', 'Yes')
            @('Zero heap allocation on any path', 'Yes')
            @('O(1) attribute query per codepoint', 'Yes')
            @('Total attribute query over the 31-bit space', 'Yes')
            @('Sorted, non-overlapping compiled tables', 'Yes')
            @('Explicit-scheme and bare-domain URL initiation', 'Yes')
            @('UTS #58 termination with a 125-entry bracket stack', 'Yes')
            @('Path / Query / Fragment / Fragment-Directive parts', 'Yes')
            @('Email detection and minimal quoting', 'Yes')
            @('Userinfo rejection', 'Yes')
            @('Minimal percent escaping (SUTF-8 for non-ASCII)', 'Yes')
            @('Bidi-control predicate', 'Yes')
            @('RNUR neography carve-out with a set convention', 'Yes')
            @('Single linear pass over the stream', 'Yes')
            @('Modular optionality (module omittable)', 'Yes')
            @('31-bit SUCS and 64-bit ExtSUCS compatibility', 'Yes')
        ) }
        @{ t = 'callout'; html = 'Reference implementation: <span class="mono">include/suts/suts_suldf.h</span> + <span class="mono">src/suts/suts_suldf.c</span> (freestanding C99, no heap), registered in <span class="mono">suts_static</span>.' }
        @{ t = 'note'; html = 'Source of truth: <span class="mono">docs/suts/SUTS-009-suldf.md</span> in the repository.' }
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
            @('MANDATORY', '17', 'SUAS-001 (SDF) &hellip; SUAS-009 (SIP), SUAS-010 (SSRE), SUAS-011 (SCMS), SUAS-012 (SUCD), SUAS-013 (SISR), SUAS-014 (SUVTL), SUAS-015 (SUAMR), SUAS-016 (SUKEMET), SUAS-017 (SUEASD)')
            @('OPTIONAL', '10', 'SUTS-001 (SUCA), SUTS-002 (SRE), SUTS-003 (SLDF), SUTS-004 (SIVD), SUTS-005 (SCSS), SUTS-006 (SIDN), SUTS-007 (SUEPS), SUTS-008 (SUSCH), SUTS-009 (SULDF), SUTS-011 (SUSN)')
            @('REFERENCE', '14', 'SUTR-0 through SUTR-013')
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
        @{ t = 'p'; html = 'The SUAS series is <strong>contiguous</strong>: SUAS-001 through SUAS-013 are all allocated, in authorship order, and no number is reserved or held back. This standard is <strong>SUAS-010</strong>.' }
        @{ t = 'p'; html = 'It was first published as SUAS-011, with SUAS-010 left as a deliberate gap, because a skipped number in a small series reads as a transcription error rather than as a decision &mdash; so both it and SCMS were shifted down by one. Citations predating that shift that say <strong>SUAS-011 (SSRE)</strong> mean this standard; those that say <strong>SUAS-012 (SCMS)</strong> mean <a href="SUAS-011.html">SUAS-011 (SCMS)</a>. Those citations are strictly historical: SUAS-012 now names <a href="SUAS-012.html">SUCD</a>, and SUAS-013 names <a href="SUAS-013.html">SISR</a>.' }
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
        @{ t = 'p'; html = 'The seeding sketch was labelled <strong>SUAS-013</strong>; it is registered as <strong>SUAS-011</strong>, the number immediately after <a href="SUAS-010.html">SUAS-010 (SSRE)</a>, which keeps the SUAS series <strong>contiguous with no reserved gaps</strong>. SUAS-013 was unallocated when this standard was written; it now names <a href="SUAS-013.html">SISR</a>.' }
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
        @{ t = 'p'; html = 'The seeding sketch was labelled <strong>SUAS-014</strong>; it is registered as <strong>SUAS-012</strong>, the number immediately after <a href="SUAS-011.html">SUAS-011 (SCMS)</a>, which keeps the SUAS series <strong>contiguous with no reserved gaps</strong>. SUAS-014 is not used by this specification; SUAS-013, allocated afterwards, is <a href="SUAS-013.html">SISR</a>.' }
        @{ t = 'p'; html = 'As with <a href="SUAS-009.html">SIP</a>, <a href="SUAS-010.html">SSRE</a>, <a href="SUAS-011.html">SCMS</a>, and <a href="SUTS-005.html">SCSS</a>, the brief named the reference layer &ldquo;SUCA&rdquo;; in this corpus <strong>SUCA</strong> is already the SuperUnicode Collation Algorithm (SUTS-001), so this standard names <span class="mono">SUCD</span> as the character database.' }
        @{ t = 'callout'; html = 'Reference implementation: <span class="mono">include/suas/suas_sucd.h</span> + <span class="mono">src/suas/suas_sucd.c</span> (freestanding C99, no heap), registered in <span class="mono">suas_static</span> and in the SSRE registry as a MANDATORY specification.' }
        @{ t = 'note'; html = 'Source of truth: <span class="mono">docs/suas/SUAS-012-sucd.md</span> in the repository.' }
    )
}

$Pages['reports/SUAS-013'] = @{
    path    = 'reports/SUAS-013.html'
    sec     = 'reports'
    title   = 'SUAS-013 — System Ideographic Source Registry (SISR)'
    desc    = 'SUAS-013: System Ideographic Source Registry (SISR) — byte-packed source identities, registry classes, canonical variant fallback with O(1) resolution, and a proven table (a UAX #45 analog).'
    crumbName = 'SUAS-013 — SISR'
    crumbs  = @( @{ label = 'Technical Reports'; href = 'index.html' } )
    h1      = 'SUAS-013 &mdash; System Ideographic <span class="grad">Source Registry</span>'
    subtitle= 'The structural counterpart of UAX #45: one 16-byte source record, one 12-byte variant link, and a lookup that is a hash and a bounded probe.'
    body    = @(
        @{ t = 'p'; html = 'SUAS-013 (SISR) is the thirteenth SuperUnicode Architecture Standard and the <strong>structural counterpart of UAX #45</strong>, the U-source ideograph annex. UAX #45 records an ideograph&rsquo;s provenance as a string such as <span class="mono">UTC-00932</span>, and its status, IDS decomposition, radical-stroke data and first residual stroke count as text interleaved with that string. SISR keeps the identity and closes the architecture around it: two byte-packed tables, a bounded lookup, and a verification surface the compiler must satisfy before it emits a record.' }
        @{ t = 'p'; html = 'SISR does not re-implement <a href="SUAS-009.html">SUAS-009 (SIP)</a>. SIP answers what an ideograph <em>is</em> &mdash; radical, stroke count, classification, collation seed. SISR answers where it <em>came from</em> and what replaces it. Where both name a fallback, the two tables must agree; the reference tests enforce that invariant.' }
        @{ t = 'h2'; html = 'Why a binary registry' }
        @{ t = 'table'; head = @('Aspect', 'UAX #45', 'SISR'); rows = @(
            @('Definition', 'A text data file, parsed at run time', 'Two static binary tables, compiled at build time')
            @('Identity', 'A string, <span class="mono">UTC-00932</span>', 'A 24-bit authority-local serial; the string is a derived view')
            @('Status', 'Narrative text', 'A proven six-bit classification field')
            @('Lookup', 'Search the file', 'A hash and a bounded probe &mdash; O(1) expected')
            @('Fallback', 'A description', 'One canonical target per source form, resolved in O(1)')
            @('Failure mode', 'An implementer&rsquo;s own reconciliation', 'The verifier names the first of fourteen defects')
        ) }
        @{ t = 'h2'; html = 'The source record' }
        @{ t = 'p'; html = 'Six fields, 16 bytes, no padding on any supported ABI. The seeding sketch supplied <span class="mono">codepoint</span>, <span class="mono">source_id_hash</span>, <span class="mono">registry_flags</span>, <span class="mono">source_type</span> and <span class="mono">reserved</span>; SISR adds <span class="mono">source_serial</span>, because a 32-bit hash is an integrity check, not an identity.' }
        @{ t = 'spec'; html = 'typedef struct {<br>    uint32_t codepoint;      /* Base SUCS codepoint                   */<br>    uint32_t source_serial;  /* authority-local serial, 1..0xFFFFFF   */<br>    uint32_t source_id_hash; /* FNV-1a of the canonical token         */<br>    uint16_t registry_flags; /* SISR_SOURCE_* bitfield                */<br>    uint8_t  source_type;    /* suas_sisr_source_type_t               */<br>    uint8_t  reserved;       /* must be zero                           */<br>} sisr_source_entry_t;' }
        @{ t = 'p'; html = 'The identity is the pair <span class="mono">(source_type, source_serial)</span>. The token is derived: <span class="mono">PREFIX-</span> followed by at least five zero-padded decimal digits, widening only above 99999 &mdash; so <span class="mono">UTC-00932</span> round-trips exactly and <span class="mono">RNUR-00256</span> needs no special case. <span class="mono">source_id_hash</span> is the FNV-1a of that token.' }
        @{ t = 'table'; head = @('Source','Registry class','Variant fallback'); rows = @(
            @('<span class="mono">0x00160100</span> &mdash; <span class="mono">RNUR-00256</span>', 'RNUR set 1, verified', '&mdash;')
            @('<span class="mono">0x00160101</span> &mdash; <span class="mono">RNUR-00257</span>', 'RNUR set 1, verified', '<span class="mono">0x00160102</span>')
            @('<span class="mono">0x00009F8D</span> &mdash; <span class="mono">UK-00001</span>', 'historical (UK)', '<span class="mono">0x00160200</span> &mdash; traditional')
            @('<span class="mono">0x00009F99</span> &mdash; <span class="mono">UTC-00002</span>', 'historical (UTC)', '<span class="mono">0x00160201</span> &mdash; simplified')
            @('<span class="mono">0x0000FA26</span> &mdash; <span class="mono">UTC-00932</span>', 'historical, verified', '<span class="mono">0x000090FD</span> &mdash; compatibility')
            @('<span class="mono">0x00160300</span> &mdash; <span class="mono">CONL-00001</span>', 'conlang, verified', '<span class="mono">0x00160301</span>')
        ) }
        @{ t = 'h2'; html = 'The variant link' }
        @{ t = 'p'; html = 'Source identity and glyph preference are different questions, so links live in their own 12-byte table. A codepoint may have a source identity and no fallback, a fallback that is itself a source form, or share a fallback with others.' }
        @{ t = 'spec'; html = 'typedef struct {<br>    uint32_t codepoint;          /* the source form              */<br>    uint32_t fallback_codepoint; /* the canonical glyph to render */<br>    uint16_t link_flags;         /* SISR_LINK_* bitfield         */<br>    uint16_t reserved;           /* must be zero                  */<br>} sisr_variant_link_t;' }
        @{ t = 'p'; html = 'Eight link bits are assigned: <span class="mono">SIMPLIFIED</span>, <span class="mono">TRADITIONAL</span>, <span class="mono">Z_VARIANT</span>, <span class="mono">COMPATIBILITY</span>, <span class="mono">REORDERED</span>, <span class="mono">APPROXIMATE</span>, <span class="mono">REGISTRY</span> and <span class="mono">CUSTOM</span>. A link whose flags are zero is rejected, and a self-link is rejected, because an unclassified or reflexive fallback cannot be audited.' }
        @{ t = 'h2'; html = 'Zones and the RNUR decode' }
        @{ t = 'p'; html = 'A source identity may name only the Bridge plane, the RNUR neography carve-out, or the custom-conscript Native plane. The Control Plane, the trap slots, the Sentinel and ExtSUCS are never source identities. The RNUR set number is the same decode <a href="SUAS-005.html">SSP</a> publishes: <span class="mono">set = (cp &minus; 0x00160000) &gt;&gt; 8</span>.' }
        @{ t = 'table'; head = @('Zone', 'Range', 'Source identity?'); rows = @(
            @('<span class="mono">BRIDGE</span>', '<span class="mono">0x00000000&ndash;0x0010FFFF</span>', 'Yes')
            @('<span class="mono">SCP</span>', '<span class="mono">0x00110000&ndash;0x0011FFFF</span>', 'No &mdash; Control Plane')
            @('<span class="mono">NATIVE</span>', 'rest of <span class="mono">0x00120000&ndash;0x7FFFFFEF</span>', 'Yes &mdash; custom conscript space')
            @('<span class="mono">RNUR</span>', '<span class="mono">0x00160000&ndash;0x0016FFFF</span>', 'Yes &mdash; neography')
            @('<span class="mono">TRAP</span> / <span class="mono">SENTINEL</span> / <span class="mono">EXTSUCS</span>', 'terminals and plugin space', 'No')
        ) }
        @{ t = 'h2'; html = 'Lookup and its cost' }
        @{ t = 'table'; head = @('Operation', 'Cost'); rows = @(
            @('<span class="mono">suas_sisr_init_registry</span>', 'O(n) once, n &le; 256, no allocation')
            @('<span class="mono">suas_sisr_get_source_entry</span>', 'O(1) expected &mdash; one hash and a bounded probe')
            @('<span class="mono">suas_sisr_fallback_target</span> / <span class="mono">_get_variant_link</span>', 'O(1) expected')
            @('<span class="mono">suas_sisr_is_registered</span>', 'O(1) expected')
            @('<span class="mono">suas_sisr_format_source_id</span>', 'O(1) &mdash; bounded token writer')
            @('<span class="mono">suas_sisr_validate_table</span> / <span class="mono">_links</span>', 'O(n&sup2;) worst case &mdash; a compiler-time verifier')
        ) }
        @{ t = 'h2'; html = 'Verification names the defect' }
        @{ t = 'p'; html = '<span class="mono">suas_sisr_validate_table</span> returns the first of fourteen named outcomes: a NULL table, a count over capacity, an unordered pair, a duplicate codepoint, a codepoint past the space, an inadmissible codepoint, a bad serial, a bad authority, a reserved or contradictory flag bit, a nonzero reserved byte, a duplicate identity, a stale hash, or a broken link. An <strong>empty registry is valid</strong>: an implementation with no ideographic sources ships zero records and conforms.' }
        @{ t = 'h2'; html = 'Classification note' }
        @{ t = 'p'; html = 'The seeding sketch labelled SISR <strong>SUTS-006</strong>, on the argument that a registry is modular optional compliance. The series owner directed that it be authored as a <strong>SUAS</strong> standard, so it is registered as <strong>SUAS-013</strong>, the number immediately after <a href="SUAS-012.html">SUAS-012 (SUCD)</a>. One element of the SUTS framing survives deliberately: the registry <em>contents</em> are optional version data, so an implementation with no ideographic sources ships an empty registry and remains conformant &mdash; while the engine, the layout, the lookup contract and the verification rules are mandatory architecture.' }
        @{ t = 'p'; html = 'The sketch&rsquo;s own body cited SUCD as &ldquo;SUAS-014&rdquo;. That is stale: SUCD is <strong>SUAS-012</strong>. SISR is SUAS-013, and SUAS-014 is now <a href="SUAS-014.html">SUVTL</a>.' }
        @{ t = 'callout'; html = 'Reference implementation: <span class="mono">include/suas/suas_sisr.h</span> + <span class="mono">src/suas/suas_sisr.c</span> (freestanding C99, no heap), registered in <span class="mono">suas_static</span> and in the SSRE registry as a MANDATORY specification.' }
        @{ t = 'note'; html = 'Source of truth: <span class="mono">docs/suas/SUAS-013-sisr.md</span> in the repository.' }
    )
}

$Pages['reports/SUAS-014'] = @{
    path    = 'reports/SUAS-014.html'
    sec     = 'reports'
    title   = 'SUAS-014 — SuperUnicode Vertical Text Layout (SUVTL)'
    desc    = 'SUAS-014: SuperUnicode Vertical Text Layout (SUVTL) — byte-packed vertical orientation, rotation and baseline metrics, with a total algorithmic default so every codepoint has exactly one answer (a UAX #50 analog).'
    crumbName = 'SUAS-014 — SUVTL'
    crumbs  = @( @{ label = 'Technical Reports'; href = 'index.html' } )
    h1      = 'SUAS-014 &mdash; SuperUnicode <span class="grad">Vertical Text Layout</span>'
    subtitle= 'The runtime counterpart of UAX #50: one 8-byte orientation record, a total answer for every codepoint, and a lookup that is a hash and a bounded probe.'
    body    = @(
        @{ t = 'p'; html = 'SUAS-014 (SUVTL) is the fourteenth SuperUnicode Architecture Standard and the <strong>runtime counterpart of UAX #50</strong>, the Vertical Text Layout annex. UAX #50 defines the <span class="mono">Vertical_Orientation</span> (<span class="mono">vo</span>) property in prose and ships it as <span class="mono">VerticalOrientation.txt</span> &mdash; semicolon-delimited ranges tagged <span class="mono">U</span>, <span class="mono">R</span>, <span class="mono">Tu</span> or <span class="mono">Tr</span>, with narrative glyph-change notes every implementation reads at run time. SUVTL keeps the property and closes the architecture around it: a byte-packed table, a bounded lookup, and a verification surface the compiler must satisfy before it emits a record.' }
        @{ t = 'p'; html = 'SUVTL does not re-implement <a href="SUAS-006.html">SUAS-006 (STS)</a>. STS decides where a grapheme cluster begins and ends; SUVTL takes the <em>first</em> character of a cluster as the cluster&rsquo;s orientation, with the enclosing-mark exception of UAX #50. The two compose; neither re-derives the other.' }
        @{ t = 'h2'; html = 'Why a binary property' }
        @{ t = 'table'; head = @('Aspect', 'UAX #50', 'SUVTL'); rows = @(
            @('Definition', 'A text data file, parsed at run time', 'A static binary table, compiled at build time')
            @('Orientation', 'A four-letter tag on a range line', 'Three value bits; the tag is a derived view')
            @('Glyph changes', 'Narrative tables', 'A single <span class="mono">TRANSFORM</span> flag; the prose stays in the reference layer')
            @('Lookup', 'Search the file', 'A hash and a bounded probe &mdash; O(1) expected')
            @('Unlisted codepoint', 'Undefined', 'A total algorithmic default &mdash; every codepoint answers')
            @('Failure mode', 'An implementer&rsquo;s own reconciliation', 'The verifier names the first of eleven defects')
        ) }
        @{ t = 'h2'; html = 'The orientation record' }
        @{ t = 'p'; html = 'Four fields, 8 bytes, no padding on any supported ABI. They are the seeding sketch&rsquo;s fields, verbatim: <span class="mono">codepoint</span>, <span class="mono">orientation_flags</span>, <span class="mono">baseline_offset</span> and <span class="mono">reserved</span>.' }
        @{ t = 'spec'; html = 'typedef struct {<br>    uint32_t codepoint;         /* Base SUCS codepoint                */<br>    uint16_t orientation_flags; /* SUVTL_ORIENT_* + SUVTL_FLAG_*      */<br>    uint8_t  baseline_offset;   /* signed 1/16-em vertical adjustment */<br>    uint8_t  reserved;          /* must be zero                       */<br>} suvtl_glyph_layout_t;' }
        @{ t = 'h2'; html = 'The four values, read off the bits' }
        @{ t = 'p'; html = 'UAX #50&rsquo;s four values are encoded by the three bits the seeding sketch exposed, rather than a separate enum: <span class="mono">UPRIGHT</span>, <span class="mono">ROTATED</span> and <span class="mono">TRANSFORM</span>. There is no second, redundant value field to keep in sync, and a caller that only wants &ldquo;does this rotate?&rdquo; tests one bit.' }
        @{ t = 'table'; head = @('Value', 'Encoding', 'Meaning', 'Fallback'); rows = @(
            @('<span class="mono">U</span>', '<span class="mono">0x0001</span> = UPRIGHT', 'Upright', '&mdash;')
            @('<span class="mono">R</span>', '<span class="mono">0x0002</span> = ROTATED', 'Rotated 90&deg; clockwise', '&mdash;')
            @('<span class="mono">Tu</span>', '<span class="mono">0x0005</span> = UPRIGHT | TRANSFORM', 'Transformed upright', '<span class="mono">U</span>')
            @('<span class="mono">Tr</span>', '<span class="mono">0x0006</span> = ROTATED | TRANSFORM', 'Transformed rotated', '<span class="mono">R</span>')
        ) }
        @{ t = 'p'; html = 'Three orthogonal flags sit above the value: <span class="mono">CUSTOM_FLOW</span> (a Native/neography vertical-flow declaration), <span class="mono">SQUARED</span> (a squared katakana or ideographic word) and <span class="mono">QUOTATION</span> (a font-substituted quotation mark). <span class="mono">baseline_offset</span> is a signed 1/16-em vertical adjustment, zero by default.' }
        @{ t = 'h2'; html = 'The grapheme-cluster rule' }
        @{ t = 'p'; html = 'UAX #50 defines a cluster&rsquo;s orientation as its <em>first</em> character&rsquo;s, unless the cluster contains an enclosing combining mark (general category Me), in which case the whole cluster is <span class="mono">U</span>. <span class="mono">suas_suvtl_cluster_orientation</span> applies that rule to one cluster the caller supplies; <span class="mono">suas_suvtl_is_enclosing_mark</span> answers the Me question from a small curated set of Me ranges, so a caller need not have loaded SUCD. The engine never computes cluster boundaries &mdash; that is STS.' }
        @{ t = 'h2'; html = 'Zones and the total default' }
        @{ t = 'p'; html = 'A layout query may name only the Bridge plane, the RNUR neography carve-out, or the custom-conscript Native plane. A codepoint with no curated record is <em>not</em> unknown: <span class="mono">suas_suvtl_get_layout_properties</span> synthesizes an algorithmic default from the codepoint&rsquo;s vertical-behaviour group &mdash; CJK and the remainder upright, Latin/Greek/Cyrillic rotated &mdash; and returns <span class="mono">true</span>. It returns <span class="mono">false</span> only for an inadmissible zone or a NULL output. The property is total.' }
        @{ t = 'table'; head = @('Zone', 'Range', 'Text space?'); rows = @(
            @('<span class="mono">BRIDGE</span>', '<span class="mono">0x00000000&ndash;0x0010FFFF</span>', 'Yes')
            @('<span class="mono">SCP</span>', '<span class="mono">0x00110000&ndash;0x0011FFFF</span>', 'No &mdash; Control Plane')
            @('<span class="mono">NATIVE</span>', 'rest of <span class="mono">0x00120000&ndash;0x7FFFFFEF</span>', 'Yes &mdash; custom conscript space')
            @('<span class="mono">RNUR</span>', '<span class="mono">0x00160000&ndash;0x0016FFFF</span>', 'Yes &mdash; neography')
            @('<span class="mono">TRAP</span> / <span class="mono">SENTINEL</span> / <span class="mono">EXTSUCS</span>', 'terminals and plugin space', 'No')
        ) }
        @{ t = 'h2'; html = 'The curated roster' }
        @{ t = 'p'; html = 'The compiled table carries the full UAX #50 <strong>Table 3</strong> Tu/Tr roster &mdash; the characters whose default glyph is not acceptable in vertical text and for which a font must supply a separate vertical form &mdash; plus the Native/neography custom-flow declarations. It is a curated subset of the SUCD reference layer; extending it is a data update, never an API change.' }
        @{ t = 'table'; head = @('Block', 'Value', 'Flag'); rows = @(
            @('Quotation marks', '<span class="mono">Tr</span>', '<span class="mono">QUOTATION</span>')
            @('Brackets, punctuation, small kana, Bopomofo, fullwidth forms', '<span class="mono">Tr</span>', '&mdash;')
            @('Squared katakana / ideographic words', '<span class="mono">Tu</span>', '<span class="mono">SQUARED</span>')
            @('Native / RNUR declarations', '<span class="mono">U</span>', '<span class="mono">CUSTOM_FLOW</span>')
        ) }
        @{ t = 'h2'; html = 'Lookup and its cost' }
        @{ t = 'table'; head = @('Operation', 'Cost'); rows = @(
            @('<span class="mono">suas_suvtl_init_engine</span>', 'O(n) once, n &le; 512, no allocation')
            @('<span class="mono">suas_suvtl_get_layout_properties</span>', 'O(1) expected &mdash; one hash and a bounded probe')
            @('<span class="mono">suas_suvtl_orientation_of</span> / <span class="mono">_is_upright</span> / <span class="mono">_is_rotated</span> / <span class="mono">_is_transformed</span>', 'O(1)')
            @('<span class="mono">suas_suvtl_cluster_orientation</span>', 'O(cluster) &mdash; the enclosing-mark scan plus one lookup')
            @('<span class="mono">suas_suvtl_validate_table</span>', 'O(n&sup2;) worst case &mdash; a compiler-time verifier')
        ) }
        @{ t = 'h2'; html = 'Verification names the defect' }
        @{ t = 'p'; html = '<span class="mono">suas_suvtl_validate_table</span> returns the first of eleven named outcomes: a NULL table, a count over capacity, an unordered pair, a duplicate codepoint, a codepoint past the space, an inadmissible codepoint, a value that names no orientation, a reserved or contradictory flag bit, a nonzero reserved byte, or a custom-flow claim outside the RNUR set. An <strong>empty registry is valid</strong>: an implementation with no curated records ships zero rows and conforms.' }
        @{ t = 'h2'; html = 'Classification note' }
        @{ t = 'p'; html = 'SUVTL is a <strong>SUAS</strong> core invariant, registered as <strong>SUAS-014</strong>, the number immediately after <a href="SUAS-013.html">SUAS-013 (SISR)</a>. One element of the SUTS framing survives deliberately: the registry <em>contents</em> are optional version data, so an implementation with no curated records ships an empty table and remains conformant &mdash; while the engine, the record layout, the lookup contract and the verification rules are mandatory architecture.' }
        @{ t = 'p'; html = 'The seeding sketch named its offline layer &ldquo;SUCA&rdquo;. In this corpus SUCA is already the <strong>SuperUnicode Collation Algorithm</strong> (<a href="SUTS-001.html">SUTS-001</a>), so SUVTL names <a href="SUAS-012.html">SUCD (SUAS-012)</a> as its reference layer &mdash; exactly as SIP, SCSS and SISR already do.' }
        @{ t = 'callout'; html = 'Reference implementation: <span class="mono">include/suas/suas_suvtl.h</span> + <span class="mono">src/suas/suas_suvtl.c</span> (freestanding C99, no heap), registered in <span class="mono">suas_static</span> and in the SSRE registry as a MANDATORY specification.' }
        @{ t = 'note'; html = 'Source of truth: <span class="mono">docs/suas/SUAS-014-suvtl.md</span> in the repository.' }
    )
}

$Pages['reports/SUAS-015'] = @{
    path    = 'reports/SUAS-015.html'
    sec     = 'reports'
    title   = 'SUAS-015 — SuperUnicode Arabic Mark Rendering (SUAMR)'
    desc    = 'SUAS-015: SuperUnicode Arabic Mark Rendering (SUAMR) — byte-packed Arabic mark stacking and a total AMTRA transient reordering, in O(1) with zero allocation (a UAX #53 analog).'
    crumbName = 'SUAS-015 — SUAMR'
    crumbs  = @( @{ label = 'Technical Reports'; href = 'index.html' } )
    h1      = 'SUAS-015 &mdash; SuperUnicode <span class="grad">Arabic Mark Rendering</span>'
    subtitle= 'The runtime counterpart of UAX #53: one 12-byte mark-layout record, a parallel combining-class table, and an in-place AMTRA pass the caller can override with a combining grapheme joiner.'
    body    = @(
        @{ t = 'p'; html = 'SUAS-015 (SUAMR) is the fifteenth SuperUnicode Architecture Standard and the <strong>runtime counterpart of UAX #53</strong>, Unicode Arabic Mark Rendering. UAX #53 defines the Arabic Mark Transient Reordering Algorithm (AMTRA) in prose and leaves the actual placement of each diacritic to an OpenType shaping engine with dynamically parsed font metrics and runtime heuristics. SUAMR keeps the algorithm and closes the architecture around it: a byte-packed table, a bounded lookup, and a verification surface the compiler must satisfy before it emits a record.' }
        @{ t = 'p'; html = 'SUAMR does not re-implement <a href="SUAS-006.html">SUAS-006 (STS)</a>. STS decides where a grapheme cluster begins and ends; SUAMR takes the cluster the caller supplies and reorders its marks. The two compose; neither re-derives the other.' }
        @{ t = 'h2'; html = 'Why a binary property' }
        @{ t = 'table'; head = @('Aspect', 'UAX #53 / a shaping engine', 'SUAMR'); rows = @(
            @('Positioning', 'Glyph placement resolved by GSUB/GPOS at run time', 'A 12-byte static record read from a fixed table')
            @('Metrics', 'Font tables parsed at run time', 'Fixed-point offsets compiled at build time')
            @('Memory', 'Heap-allocated shaping buffers', 'Zero allocation; the caller owns every buffer')
            @('Ordering', 'Left to the shaper', 'One deterministic AMTRA pass, byte-identical everywhere')
            @('Override', 'A shaper-specific escape hatch', 'A combining grapheme joiner the algorithm never crosses')
            @('Unlisted mark', 'Undefined', 'A total algorithmic default &mdash; every mark answers')
            @('Failure mode', 'An implementer&rsquo;s own reconciliation', 'The verifier names the first of twelve defects')
        ) }
        @{ t = 'h2'; html = 'The mark-layout record' }
        @{ t = 'p'; html = 'Six fields, 12 bytes, no padding on any supported ABI. They are the seeding sketch&rsquo;s fields, verbatim: <span class="mono">codepoint</span>, <span class="mono">x_offset</span>, <span class="mono">y_offset</span>, <span class="mono">positioning_flags</span>, <span class="mono">stacking_level</span> and <span class="mono">reserved</span>. The offsets are signed fixed-point displacements in 1/64 em.' }
        @{ t = 'spec'; html = 'typedef struct {<br>    uint32_t codepoint;         /* Combining mark codepoint                 */<br>    int16_t  x_offset;          /* Fixed-point horizontal displacement       */<br>    int16_t  y_offset;          /* Fixed-point vertical displacement         */<br>    uint16_t positioning_flags; /* Packed stacking and rendering flags       */<br>    uint8_t  stacking_level;    /* Stacking depth index for multi-mark runs  */<br>    uint8_t  reserved;          /* must be zero                              */<br>} suamr_mark_layout_t;' }
        @{ t = 'p'; html = 'The record carries no combining-class field: the class lives in a <strong>parallel table</strong> aligned with the layout table at build time. This preserves the sketch&rsquo;s exact 12 bytes while still carrying the 220/230 generalized classes AMTRA needs.' }
        @{ t = 'h2'; html = 'The positioning flags' }
        @{ t = 'table'; head = @('Flag', 'Meaning'); rows = @(
            @('<span class="mono">STACK_ABOVE</span>', 'Stack the mark above the base')
            @('<span class="mono">STACK_BELOW</span>', 'Stack the mark below the base')
            @('<span class="mono">STAGGER</span>', 'Apply a stagger adjustment to avoid collision')
            @('<span class="mono">CUSTOM_CLASS</span>', 'Uses extended neography positioning (RNUR only)')
            @('<span class="mono">MCM</span>', 'A Modifier Combining Mark &mdash; binds tightly to the base')
            @('<span class="mono">SHADDA</span>', 'The shadda (ccc=33)')
            @('<span class="mono">KASRA_LIKE</span>', 'A kasra or kasra-like ligating mark')
            @('<span class="mono">TRANSPARENT</span>', 'Renders directly over the base')
        ) }
        @{ t = 'h2'; html = 'The canonical combining classes' }
        @{ t = 'p'; html = 'UAX #53 <strong>Table 1</strong> assigns eleven non-zero <span class="mono">ccc</span> values to Arabic script combining marks, plus two generalized classes. SUAMR carries each as a closed constant and, for an uncurated mark, derives the class from its block.' }
        @{ t = 'table'; head = @('ccc', 'Marks'); rows = @(
            @('<span class="mono">0</span>', 'Combining grapheme joiner, combining alef overlay')
            @('<span class="mono">27&ndash;29</span>', 'Fathatan / dammatan / kasratan and their open forms')
            @('<span class="mono">30&ndash;32</span>', 'Fatha / damma / kasra and their small forms')
            @('<span class="mono">33</span>', 'Shadda')
            @('<span class="mono">34</span>', 'Sukun')
            @('<span class="mono">35</span>', 'Superscript alef')
            @('<span class="mono">220</span>', 'All other below marks except small low noon with kasra')
            @('<span class="mono">230</span>', 'All other above marks, small low noon with kasra')
        ) }
        @{ t = 'h2'; html = 'The AMTRA transient reordering' }
        @{ t = 'p'; html = 'AMTRA normalizes the input to <strong>NFD</strong>, then, within each run of non-starters, reorders the marks into four tiers, front to back: the leading <span class="mono">ccc=220</span> MCM run, the leading <span class="mono">ccc=230</span> MCM run, the shadda (ccc=33), and everything else in its encoded order. <span class="mono">suas_suamr_reorder_cluster</span> applies that in place, allocating nothing, and is stable &mdash; canonically equivalent sequences render identically.' }
        @{ t = 'table'; head = @('Input (marks after the base)', 'Output'); rows = @(
            @('damma, shadda', 'shadda, damma')
            @('fatha, hamza above (MCM)', 'hamza above, fatha')
            @('kasra, hamza below (MCM)', 'hamza below, kasra')
            @('shadda, fatha, hamza below (MCM)', 'hamza below, shadda, fatha')
        ) }
        @{ t = 'callout'; html = 'The <strong>combining grapheme joiner</strong> (<span class="mono">U+034F</span>) pins the mark that follows it: AMTRA never moves a mark across a CGJ. Encode <span class="mono">&lt;damma, CGJ, hamza above&gt;</span> to place the hamza above over the damma &mdash; the exceptional order, exactly as the orthography demands.' }
        @{ t = 'h2'; html = 'The mistaken U+08D9 assignment' }
        @{ t = 'p'; html = '<span class="mono">U+08D9 ARABIC SMALL LOW NOON WITH KASRA</span> appears <em>below</em> the text but was mistakenly given <span class="mono">ccc=230</span> in Unicode 9.0, and that cannot now be changed. SUAMR records it as it is &mdash; ccc=230, but stacking <strong>below</strong> &mdash; and a conforming implementation gives it no effect on the marks above.' }
        @{ t = 'h2'; html = 'Zones and the total default' }
        @{ t = 'p'; html = 'A layout query may name only the Bridge plane, the RNUR neography carve-out, or the custom-conscript Native plane. A mark with no curated record is <em>not</em> unknown: <span class="mono">suas_suamr_get_mark_layout</span> synthesizes an algorithmic default from the mark&rsquo;s block and returns <span class="mono">true</span>. It returns <span class="mono">false</span> only for an inadmissible zone or a NULL output. The property is total.' }
        @{ t = 'table'; head = @('Zone', 'Range', 'Text space?'); rows = @(
            @('<span class="mono">BRIDGE</span>', '<span class="mono">0x00000000&ndash;0x0010FFFF</span>', 'Yes')
            @('<span class="mono">SCP</span>', '<span class="mono">0x00110000&ndash;0x0011FFFF</span>', 'No &mdash; Control Plane')
            @('<span class="mono">NATIVE</span>', 'rest of <span class="mono">0x00120000&ndash;0x7FFFFFEF</span>', 'Yes &mdash; custom conscript space')
            @('<span class="mono">RNUR</span>', '<span class="mono">0x00160000&ndash;0x0016FFFF</span>', 'Yes &mdash; neography')
            @('<span class="mono">TRAP</span> / <span class="mono">SENTINEL</span> / <span class="mono">EXTSUCS</span>', 'terminals and plugin space', 'No')
        ) }
        @{ t = 'h2'; html = 'Extension to complex conscript abugidas' }
        @{ t = 'p'; html = 'Because the stacking engine is fully generalized, the same layout tables extend to complex diacritic stacking in the Native &amp; Neography Plane (<span class="mono">0x160000&ndash;0x16FFFF</span>) &mdash; a conlang writing system such as the K&#333;pu abvukjelits&#257; abugida, or a script declared in the Reddit Neographical Unicode Registry (RNUR), carries <span class="mono">CUSTOM_CLASS</span> so a renderer knows the positioning came from a registry declaration rather than a historical default.' }
        @{ t = 'h2'; html = 'Lookup and its cost' }
        @{ t = 'table'; head = @('Operation', 'Cost'); rows = @(
            @('<span class="mono">suas_suamr_init_engine</span>', 'O(n) once, n &le; 256, no allocation')
            @('<span class="mono">suas_suamr_get_mark_layout</span>', 'O(1) expected &mdash; one hash and a bounded probe')
            @('<span class="mono">suas_suamr_is_above</span> / <span class="mono">_is_below</span> / <span class="mono">_is_mcm</span> / <span class="mono">_is_shadda</span>', 'O(1)')
            @('<span class="mono">suas_suamr_ccc</span> / <span class="mono">_stacking_level</span> / <span class="mono">_x_offset</span> / <span class="mono">_y_offset</span>', 'O(1)')
            @('<span class="mono">suas_suamr_reorder_cluster</span>', 'O(cluster) &mdash; one pass, ccc &le; 32')
            @('<span class="mono">suas_suamr_validate_table</span>', 'O(n) &mdash; a compiler-time verifier')
        ) }
        @{ t = 'h2'; html = 'Verification names the defect' }
        @{ t = 'p'; html = '<span class="mono">suas_suamr_validate_table</span> returns the first of twelve named outcomes: a NULL table, a count over capacity, an unordered pair, a duplicate codepoint, a codepoint past the space, an inadmissible codepoint, an invalid combining class, a reserved or contradictory flag bit, both stacking sides set at once, a nonzero reserved byte, or a custom-class claim outside the RNUR set. An <strong>empty registry is valid</strong>: an implementation with no curated records ships zero rows and conforms.' }
        @{ t = 'h2'; html = 'Classification note' }
        @{ t = 'p'; html = 'SUAMR is a <strong>SUAS</strong> core invariant, registered as <strong>SUAS-015</strong>, the number immediately after <a href="SUAS-014.html">SUAS-014 (SUVTL)</a>. One element of the SUTS framing survives deliberately: the registry <em>contents</em> are optional version data, so an implementation with no curated records ships an empty table and remains conformant &mdash; while the engine, the record layout, the reordering rules and the verification rules are mandatory architecture.' }
        @{ t = 'p'; html = 'The seeding sketch named its offline layer &ldquo;SUCA&rdquo;. In this corpus SUCA is already the <strong>SuperUnicode Collation Algorithm</strong> (<a href="SUTS-001.html">SUTS-001</a>), so SUAMR names <a href="SUAS-012.html">SUCD (SUAS-012)</a> as its reference layer &mdash; exactly as SIP, SCSS, SISR and SUVTL already do.' }
        @{ t = 'callout'; html = 'Reference implementation: <span class="mono">include/suas/suas_suamr.h</span> + <span class="mono">src/suas/suas_suamr.c</span> (freestanding C99, no heap), registered in <span class="mono">suas_static</span> and in the SSRE registry as a MANDATORY specification.' }
        @{ t = 'note'; html = 'Source of truth: <span class="mono">docs/suas/SUAS-015-suamr.md</span> in the repository.' }
    )
}

$Pages['reports/SUAS-016'] = @{
    path    = 'reports/SUAS-016.html'
    sec     = 'reports'
    title   = 'SUAS-016 - SuperUnicode Egyptian Hieroglyph Database (SUKEMET)'
    desc    = 'SUAS-016: SuperUnicode Egyptian Hieroglyph Database (SUKEMET) - byte-packed Gardiner sign records, format-control taxonomy and O(1) Unikemet classification (a UAX #57 analog).'
    crumbName = 'SUAS-016 - SUKEMET'
    crumbs  = @( @{ label = 'Technical Reports'; href = 'index.html' } )
    h1      = 'SUAS-016 &mdash; SuperUnicode <span class="grad">Egyptian Hieroglyph Database</span>'
    subtitle= 'The runtime counterpart of UAX #57: one 12-byte sign record, a parallel Unikemet metadata table, and a total O(1) answer for every Egyptian codepoint.'
    body    = @(
        @{ t = 'p'; html = 'SUAS-016 (SUKEMET) is the sixteenth SuperUnicode Architecture Standard and the <strong>runtime counterpart of UAX #57</strong>, the Unicode Egyptian Hieroglyph Database. UAX #57 publishes Unikemet as a three-field text manifest that an implementation must read, join and interpret at run time. SUKEMET keeps the catalogue and closes the architecture around it: a byte-packed table, a bounded lookup, and a verification surface the compiler must satisfy before it emits a record.' }
        @{ t = 'p'; html = 'SUKEMET does not re-implement <a href="SUAS-006.html">SUAS-006 (STS)</a>. STS decides where a grapheme cluster begins and ends; SUKEMET classifies the signs and controls inside the cluster. The two compose; neither re-derives the other.' }
        @{ t = 'h2'; html = 'Why a binary property' }
        @{ t = 'table'; head = @('Aspect', 'UAX #57 / a text manifest', 'SUKEMET'); rows = @(
            @('Catalogue', 'Unikemet.txt parsed and joined per query', 'A 12-byte static record read from a fixed table')
            @('Gardiner group', 'Inferred from a catalogue string', 'A byte-indexed SUKEMET_GARDINER_* value')
            @('Core class', 'A kEH_Core text field', 'CORE / LEGACY flags plus a one-byte class')
            @('Functions', 'A kEH_Func string per row', 'A packed function bitmask')
            @('Alternate sequence', 'Re-parsed as text', 'A flattened uint32_t store with offset and length')
            @('Format controls', 'Ad-hoc renderer rules', 'Named flags plus a one-byte modifier role')
            @('Unlisted codepoint', 'Undefined', 'A total algorithmic default &mdash; every Egyptian codepoint answers')
            @('Failure mode', 'An implementer&rsquo;s own reconciliation', 'The verifier names the first of twelve defects')
        ) }
        @{ t = 'h2'; html = 'The hieroglyph record' }
        @{ t = 'p'; html = 'Five fields, 12 bytes, no padding on any supported ABI. They are the seeding sketch&rsquo;s fields, verbatim: <span class="mono">codepoint</span>, <span class="mono">phonetic_hash</span>, <span class="mono">sukemet_flags</span>, <span class="mono">gardiner_category</span> and <span class="mono">sub_index</span>.' }
        @{ t = 'spec'; html = 'typedef struct {<br>    uint32_t codepoint;         /* 31-bit SUCS codepoint                   */<br>    uint32_t phonetic_hash;     /* FNV-1a of the primary kEH_FVal reading  */<br>    uint16_t sukemet_flags;     /* Packed classification/control flags     */<br>    uint8_t  gardiner_category; /* A-Z, AA, or SUKEMET_GARDINER_NONE       */<br>    uint8_t  sub_index;         /* Sequential index within the group       */<br>} sukemet_hieroglyph_entry_t;' }
        @{ t = 'p'; html = 'The record carries no catalogue index, source mask, Core class, function mask or alternate-sequence length: those live in a <strong>parallel metadata table</strong> aligned one-to-one with the main table at build time. This preserves the sketch&rsquo;s exact 12 bytes while carrying the full UAX #57 metadata surface.' }
        @{ t = 'h2'; html = 'The Gardiner taxonomy' }
        @{ t = 'p'; html = 'Every UAX #57 group A-Z plus AA has a closed constant. Gardiner has no group J, and SUKEMET preserves that gap; AA is the unclassified catch-all. The packed catalogue index is <span class="mono">(category &lt;&lt; 24) | (subgroup &lt;&lt; 16) | item</span>, so <span class="mono">A-01-001</span> is <span class="mono">0x00010001</span>.' }
        @{ t = 'table'; head = @('Group', 'Domain'); rows = @(
            @('<span class="mono">A-I, K-Z</span>', 'Man, woman, deities, body parts, mammals, birds, reptiles, fish, invertebrates, plants, sky, buildings, ships, furniture, emblems, dress, warfare, agriculture, rope, vessels, food, writing, strokes')
            @('<span class="mono">AA</span>', 'Unclassified and extended signs')
            @('<span class="mono">NONE</span>', 'A format control, blank sign or neography declaration')
        ) }
        @{ t = 'h2'; html = 'Core, function and source' }
        @{ t = 'p'; html = 'The Core class is exactly one of <span class="mono">N</span>, <span class="mono">C</span> or <span class="mono">L</span>; CORE and LEGACY are mutually exclusive flag bits. The function mask records one or more of pictogram, logogram, phonogram, classifier, phono-repeater, radicogram, interpretant and control. The source mask records which Unikemet fields were present, so an absent <span class="mono">kEH_FVal</span> is distinguishable from a hash that happens to be zero.' }
        @{ t = 'table'; head = @('Unikemet field', 'SUKEMET representation'); rows = @(
            @('<span class="mono">kEH_Cat</span>', 'Gardiner category, sub-index and packed catalogue index')
            @('<span class="mono">kEH_Core</span>', 'CORE / LEGACY flags and core_class')
            @('<span class="mono">kEH_Desc</span>', 'description_hash')
            @('<span class="mono">kEH_Func</span>', 'function_types bitmask')
            @('<span class="mono">kEH_FVal</span>', 'phonetic_hash of the primary alternative')
            @('<span class="mono">kEH_AltSeq</span>', 'alternate_length and suas_sukemet_alt_seq_at')
            @('<span class="mono">kEH_NoMirror</span> / <span class="mono">kEH_NoRotate</span>', 'NO_MIRROR / NO_ROTATE flags')
        ) }
        @{ t = 'h2'; html = 'Format controls and enclosures' }
        @{ t = 'p'; html = 'The Egyptian Hieroglyph Format Controls block is curated from Unicode 18.0.0: vertical and horizontal joiners, insert-at positions, the overlay middle, begin and end segment, the rounded cartouche, the rectangular serekh, the mirror control, the blank and lost signs, and the damage modifiers. A cartouche is <span class="mono">SUKEMET_ENCLOSURE_CARTOUCHE</span>; a serekh-style frame is <span class="mono">SUKEMET_ENCLOSURE_SEREKH</span>; a neography may declare <span class="mono">SUKEMET_ENCLOSURE_CUSTOM</span>.' }
        @{ t = 'p'; html = 'Unicode 18.0.0 defines no standard Egyptian shading control. SUKEMET still names <span class="mono">SUKEMET_MODIFIER_SHADING</span>, but a shading modifier is always an RNUR declaration and is never a mislabelled standard codepoint.' }
        @{ t = 'h2'; html = 'Totality and neography' }
        @{ t = 'p'; html = 'A codepoint inside the Egyptian blocks or the Native &amp; Neography Plane carve-out (<span class="mono">0x00160000&ndash;0x0016FFFF</span>) always resolves. If no curated row exists, SUKEMET synthesizes the record of section 6 &mdash; zero hash, no Gardiner group, and the NEOGRAPHY flag in RNUR &mdash; while <span class="mono">suas_sukemet_is_registered</span> remains false. A renderer can therefore answer for an unassigned Extended-A slot or a newly declared RNUR coordinate without special-casing a missing lookup.' }
        @{ t = 'p'; html = 'The shipped table carries 5,109 curated records: 5,067 Unikemet signs, 38 format-control, blank/lost and damage codepoints, and four RNUR declarations. It also carries 98 alternate sequences covering 265 codepoints.' }
        @{ t = 'h2'; html = 'Verification names the defect' }
        @{ t = 'p'; html = '<span class="mono">suas_sukemet_validate_table</span> returns the first of twelve named outcomes: a NULL table, a count over capacity, an unordered pair, a duplicate codepoint, a codepoint past the space, an inadmissible codepoint, a reserved or contradictory flag bit, an invalid Gardiner category, a flag/category disagreement, a format control outside its block, a neography flag outside RNUR, or Core and Legacy set together. An <strong>empty registry is valid</strong>: an implementation with no curated records ships zero rows and conforms.' }
        @{ t = 'h2'; html = 'Classification note' }
        @{ t = 'p'; html = 'SUKEMET is a <strong>SUAS</strong> core invariant, registered as <strong>SUAS-016</strong>, the number immediately after <a href="SUAS-015.html">SUAS-015 (SUAMR)</a>. One element of the SUTS framing survives deliberately: the registry <em>contents</em> are version data, so an implementation with no curated records ships an empty table and remains conformant &mdash; while the engine, the record layout, the taxonomy and the verification rules are mandatory architecture.' }
        @{ t = 'callout'; html = 'Reference implementation: <span class="mono">include/suas/suas_sukemet.h</span> + <span class="mono">src/suas/suas_sukemet.c</span> (freestanding C99, no heap), registered in <span class="mono">suas_static</span> and in the SSRE registry as a MANDATORY specification.' }
        @{ t = 'note'; html = 'Source of truth: <span class="mono">docs/suas/SUAS-016-sukemet.md</span> in the repository.' }
    )
}

$Pages['reports/SUAS-017'] = @{
    path    = 'reports/SUAS-017.html'
    sec     = 'reports'
    title   = 'SUAS-017 - SuperUnicode East Asian Script Data (SUEASD)'
    desc    = 'SUAS-017: SuperUnicode East Asian Script Data (SUEASD) - byte-packed script records, classification ranges and O(1) UAX #60 ancillary resolution (a UAX #60 analog).'
    crumbName = 'SUAS-017 - SUEASD'
    crumbs  = @( @{ label = 'Technical Reports'; href = 'index.html' } )
    h1      = 'SUAS-017 &mdash; SuperUnicode <span class="grad">East Asian Script Data</span>'
    subtitle= 'The runtime counterpart of UAX #60: one 12-byte script record, a compact classification range table, a parallel UAX #60 metadata table, and a total O(1) answer for every East Asian codepoint.'
    body    = @(
        @{ t = 'p'; html = 'SUAS-017 (SUEASD) is the seventeenth SuperUnicode Architecture Standard and the <strong>runtime counterpart of UAX #60</strong>, "Data for East Asian Scripts". Four text manifests publish the ancillary data for Jurchen, Nushu, Small Seal and Tangut; Scripts.txt carries the primary script assignment. SUEASD compiles both layers into a byte-packed table with a bounded lookup, so an input method, font engine or terminal can classify a codepoint without parsing a manifest.' }
        @{ t = 'p'; html = 'SUEASD does not re-implement <a href="SUAS-009.html">SUAS-009 (SIP)</a>. SIP carries the Han radical-stroke and ideographic attributes; SUEASD routes the East Asian scripts and carries the UAX #60 ancillary data for the scripts outside the Han core. The two compose; neither re-derives the other.' }
        @{ t = 'h2'; html = 'Why a binary property' }
        @{ t = 'table'; head = @('Aspect', 'UAX #60 / text manifests', 'SUEASD'); rows = @(
            @('Script', 'Scripts.txt scanned per query', 'A 12-byte static record read from a fixed range table')
            @('Ancillary data', 'Four source files parsed and joined per query', 'One curated record plus a parallel metadata row')
            @('Radical-stroke', 'A "12.4" text field', 'A uint16_t radical and a uint8_t stroke count')
            @('Readings and sources', 'Free-form text values', 'FNV-1a hashes of the primary fields')
            @('Numeric value', 'Parsed from text', 'A uint16_t integer')
            @('Modern CJK', 'A string join', 'A uint32_t codepoint equivalence')
            @('Unlisted codepoint', 'Undefined', 'A total algorithmic default &mdash; every East Asian codepoint answers')
            @('Failure mode', 'An implementer&rsquo;s own reconciliation', 'The verifier names the first of fourteen defects')
        ) }
        @{ t = 'h2'; html = 'The script record' }
        @{ t = 'p'; html = 'Five fields, 12 bytes, no padding on any supported ABI. They are the seeding sketch&rsquo;s fields, verbatim: <span class="mono">codepoint</span>, <span class="mono">script_mask</span>, <span class="mono">primary_script</span>, <span class="mono">source_region</span> and <span class="mono">reserved</span>.' }
        @{ t = 'spec'; html = 'typedef struct {<br>    uint32_t codepoint;      /* 31-bit SUCS codepoint                   */<br>    uint32_t script_mask;    /* Packed script property flags            */<br>    uint16_t primary_script; /* SUEASD_SCRIPT_* identifier              */<br>    uint8_t  source_region;  /* SUEASD_REGION_* class                   */<br>    uint8_t  reserved;       /* must be zero                            */<br>} sueasd_script_entry_t;' }
        @{ t = 'p'; html = 'The record carries no source mask, radical-stroke counts, numeric value, raw-field hashes or modern-CJK equivalence: those live in a <strong>parallel 24-byte metadata table</strong> aligned one-to-one with the curated table. The radical is 16 bits because the Small Seal and Tangut radical sets are numbered past 255.' }
        @{ t = 'h2'; html = 'Classification ranges' }
        @{ t = 'p'; html = 'Enumerating every East Asian codepoint would exceed the 65,534 index-slot ceiling, so SUEASD resolves a base record from <strong>78 disjoint, coalesced ranges</strong> derived from Scripts.txt and consults the curated table only for the UAX #60 ancillary layer. A range is 12 bytes: <span class="mono">first</span>, <span class="mono">last</span>, <span class="mono">primary_script</span> and <span class="mono">base_flags</span>.' }
        @{ t = 'table'; head = @('Script', 'Repertoire', 'Ancillary'); rows = @(
            @('<span class="mono">Han</span>', 'U+2E80&ndash;33479, U+2F800&ndash;2FA1D', 'Classification ranges only (SIP carries the Han details)')
            @('<span class="mono">Hangul</span>', 'U+1100&ndash;11FF, U+AC00&ndash;D7A3, compatibility Jamo', 'Classification ranges only')
            @('<span class="mono">Hiragana / Katakana</span>', 'U+3041&ndash;309F, U+30A1&ndash;30FF, supplementary kana', 'Classification ranges only')
            @('<span class="mono">Bopomofo</span>', 'U+02EA&ndash;02EB, U+3105&ndash;312F, U+31A0&ndash;31BF', 'Classification ranges only')
            @('<span class="mono">Jurchen</span>', 'U+18E00&ndash;191D2', '914 curated codepoints')
            @('<span class="mono">Nushu</span>', 'U+16FE1, U+1B170&ndash;1B2FB', '396 curated codepoints')
            @('<span class="mono">Small Seal</span>', 'U+3D000&ndash;3FC3F', '11,328 curated codepoints')
            @('<span class="mono">Tangut</span>', 'U+16FE0, U+17000&ndash;18DF2', '6,177 curated codepoints')
            @('<span class="mono">Khitan Small Script</span>', 'U+16FE4, U+18B00&ndash;18CFF', 'Classification only &mdash; UAX #60 has no source data yet')
        ) }
        @{ t = 'h2'; html = 'Flags and roles' }
        @{ t = 'p'; html = 'The sketch&rsquo;s four flags are preserved, including the two macros it printed without an <span class="mono">=</span>. SUEASD adds the orthogonal roles it derives from position in the repertoire: HAN_CORE / HAN_RADICAL / HAN_COMPAT, KANA, JAMO / SYLLABLE, and the UAX #60 presence flags SOURCE_DATA / READING / RS / NUMERIC / MODERN_CJK. IDEOGRAPHIC marks the modern CJK grid role (Han, Bopomofo, Small Seal); the covered scripts are identified by their script id and source mask.' }
        @{ t = 'table'; head = @('UAX #60 field', 'SUEASD representation'); rows = @(
            @('<span class="mono">kJURC_Src</span> / <span class="mono">kNSHU_DubenSrc</span> / <span class="mono">kTGT_MergedSrc</span>', 'source_mask bit plus source_hash of the primary value')
            @('<span class="mono">kJURC_NCReading</span> / <span class="mono">kNSHU_Reading</span>', 'source_mask bit plus reading_hash')
            @('<span class="mono">kJURC_RSUnicode</span> / <span class="mono">kTGT_RSUnicode</span> / <span class="mono">kSEAL_Rad</span>', 'radical, stroke_count and aux_hash')
            @('<span class="mono">kJURC_Numeric</span> / <span class="mono">kTGT_Numeric</span>', 'numeric_value')
            @('<span class="mono">kSEAL_MCJK</span>', 'modern_cjk codepoint equivalence')
            @('Distinct rows', 'source_count')
        ) }
        @{ t = 'h2'; html = 'Totality and neography' }
        @{ t = 'p'; html = 'A codepoint inside an East Asian range or the Native &amp; Neography Plane carve-out (<span class="mono">0x00160000&ndash;0x0016FFFF</span>) always resolves. If no range and no curated row exist, SUEASD synthesizes the default of section 6 &mdash; UNKNOWN script, NONE region, and the CUSTOM_SCRIPT flag in RNUR &mdash; while <span class="mono">suas_sueasd_is_east_asian</span> and <span class="mono">suas_sueasd_is_curated</span> remain false. A renderer can therefore answer for a newly declared RNUR coordinate or a non-East-Asian codepoint without special-casing a missing lookup.' }
        @{ t = 'p'; html = 'The shipped table carries 18,815 curated records drawn from the four UAX #60 manifests, and 78 classification ranges that make the property total over the East Asian repertoire.' }
        @{ t = 'h2'; html = 'Verification names the defect' }
        @{ t = 'p'; html = '<span class="mono">suas_sueasd_validate_table</span> returns the first of fourteen named outcomes: a NULL table, a count over capacity, an unordered pair, a duplicate codepoint, a codepoint past the space, an inadmissible codepoint, a bad script or region, a non-zero reserved byte, an ideographic or kana disagreement, jamo and syllable together, or a malformed range table. An <strong>empty registry is valid</strong>: an implementation with no curated records ships zero rows and conforms.' }
        @{ t = 'h2'; html = 'Classification note' }
        @{ t = 'p'; html = 'SUEASD is a <strong>SUAS</strong> core invariant, registered as <strong>SUAS-017</strong>, the number immediately after <a href="SUAS-016.html">SUAS-016 (SUKEMET)</a>. One element of the SUTS framing survives deliberately: the table <em>contents</em> are version data, so an implementation with no curated records ships an empty table and remains conformant &mdash; while the engine, the record layout, the taxonomy and the verification rules are mandatory architecture.' }
        @{ t = 'callout'; html = 'Reference implementation: <span class="mono">include/suas/suas_sueasd.h</span> + <span class="mono">src/suas/suas_sueasd.c</span> (freestanding C99, no heap), registered in <span class="mono">suas_static</span> and in the SSRE registry as a MANDATORY specification.' }
        @{ t = 'note'; html = 'Source of truth: <span class="mono">docs/suas/SUAS-017-sueasd.md</span> in the repository.' }
    )
}
