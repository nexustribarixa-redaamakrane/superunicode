# Technical Reports section

$Pages['reports/index'] = @{
    path    = 'reports/index.html'
    sec     = 'reports'
    title   = 'Technical Reports — SUTR'
    desc    = 'The SUTR-0 through SUTR-6 specifications of SuperUnicode.'
    crumbName = 'Technical Reports'
    h1      = 'Technical <span class="grad">Reports</span>'
    subtitle= 'The SUTR series &mdash; normative specifications, tracked per release.'
    body    = @(
        @{ t = 'p'; html = 'Technical Reports formalize each contract of the standard. They play the role of Unicode&rsquo;s UAX/UTR documents and are identified by the <span class="mono">SUTR-<em>n</em></span> scheme with per-version errata.' }
        @{ t = 'table'; head = @('Report', 'Status', 'Scope'); rows = @(
            @('<span class="mono">SUTR-0</span>', '0.1.0', '<a href="SUTR-0.html">SUCS Core</a> &mdash; hierarchy, three-space layout, SCP, Traps, Sentinel')
            @('<span class="mono">SUTR-1</span>', '0.1.0', '<a href="SUTR-1.html">SUTF</a> &mdash; SUCS UTF-8/16/32 framing')
            @('<span class="mono">SUTR-2</span>', '0.1.0', '<a href="SUTR-2.html">SUCA</a> &mdash; SuperUnicode Collation Algorithm')
            @('<span class="mono">SUTR-3</span>', '0.1.0', '<a href="SUTR-3.html">SuperUnicode Storage</a> &mdash; OWFS/USFS partition policy')
            @('<span class="mono">SUTR-4</span>', '0.1.0', '<a href="SUTR-4.html">ExtSUCS Transport</a> &mdash; vector, vsutf, esutf')
            @('<span class="mono">SUTR-5</span>', '0.1.0', '<a href="SUTR-5.html">Plugin Lifecycle &amp; Blob Format</a> &mdash; stage, checksum gate, mount, registry')
            @('<span class="mono">SUTR-6</span>', '0.1.0', '<a href="SUTR-6.html">Unicode Compatibility Bridge</a> &mdash; the permanent 1:1 guarantee')
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
    subtitle= 'The foundational layout of the Base SuperUnicode Character Set.'
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
    desc    = 'SUTF: SUCS UTF-8/16/32 framing.'
    crumbName = 'SUTR-1 — SUTF'
    crumbs  = @( @{ label = 'Technical Reports'; href = 'index.html' } )
    h1      = 'SUTR-1 &mdash; <span class="grad">SUTF</span>'
    subtitle= 'SUCS UTF-8, UTF-16 and UTF-32 framing.'
    body    = @(
        @{ t = 'p'; html = 'The SUTF family frames 31-bit <span class="mono">SUCS_CP</span> values into byte sequences. All three are lossless over the full Base space.' }
        @{ t = 'table'; head = @('Encoding', 'Unit', 'Covers', 'Notes'); rows = @(
            @('<span class="mono">SUTF-8</span>', '1&ndash;4 bytes', 'Full 31-bit space', 'Standard UTF-8 leading-byte framing; never emits a 4-byte value above <span class="mono">0x7FFFFFFF</span>')
            @('<span class="mono">SUTF-16</span>', '2 bytes + pairs', '<span class="mono">0x000000&ndash;0x10FFFF</span>', 'Unicode Bridge only; native allocations have no 16-bit form')
            @('<span class="mono">SUTF-32</span>', '4 bytes (LE)', 'Full 31-bit space', 'One codepoint per unit; the only lossless framing for the native space')
        ) }
        @{ t = 'callout'; html = 'Reference implementation: <a href="../modules/sutf/index.html">modules/sutf</a>. Data: <span class="mono">Public/0.1.0/sutf/</span>.' }
    )
}

$Pages['reports/SUTR-2'] = @{
    path    = 'reports/SUTR-2.html'
    sec     = 'reports'
    title   = 'SUTR-2 — SUCA'
    desc    = 'SUCA: the SuperUnicode Collation Algorithm.'
    crumbName = 'SUTR-2 — SUCA'
    crumbs  = @( @{ label = 'Technical Reports'; href = 'index.html' } )
    h1      = 'SUTR-2 &mdash; <span class="grad">SUCA</span>'
    subtitle= 'The SuperUnicode Collation Algorithm.'
    body    = @(
        @{ t = 'p'; html = 'Collation is the ordering contract for SuperUnicode strings. The default is simple, stable and total:' }
        @{ t = 'ul'; items = @(
            '<strong>Default binary order</strong> &mdash; codepoint-ascending, Sentinel last. Cheap, deterministic, universal.'
            '<strong>Unicode Bridge ordering</strong> &mdash; the compatibility space sorts by its bridged Unicode codepoint.'
            '<strong>Control plane ordering</strong> &mdash; all SCP instructions sort after every printable allocation.'
        ) }
        @{ t = 'p'; html = 'Per-script tailoring arrives with native allocation data in later releases. Extended collation (ExtUCA) extends the ordering over plugin ranges: Base region first, then plugin regions by registration sequence.' }
        @{ t = 'note'; html = 'Data: <span class="mono">Public/0.1.0/collation/SUCA.txt</span> (Base) and <span class="mono">collation/ExtUCA.txt</span> (Extended).' }
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
    desc    = 'ExtSUCS Transport: vector, vsutf and esutf framing.'
    crumbName = 'SUTR-4 — ExtSUCS Transport'
    crumbs  = @( @{ label = 'Technical Reports'; href = 'index.html' } )
    h1      = 'SUTR-4 &mdash; ExtSUCS <span class="grad">Transport</span>'
    subtitle= 'Framing for 64-bit codepoints.'
    body    = @(
        @{ t = 'table'; head = @('Transport', 'Description'); rows = @(
            @('<span class="mono">Vector</span>', 'Length-prefixed sequences of 64-bit <span class="mono">sucs_ex_char_t</span>; one codepoint per little-endian unit.')
            @('<span class="mono">vsutf</span>', 'Variable-length integer framing (LEB128-style) covering the full 64-bit space.')
            @('<span class="mono">esutf</span>', 'The fixed-base Extended SUTF family.')
        ) }
        @{ t = 'h2'; html = 'Requirements' }
        @{ t = 'ul'; items = @(
            'Every transport must represent any value from <span class="mono">0</span> to <span class="mono">0xFFFFFFFFFFFFFFFF</span>.'
            'Base SUTF8/16/32 values must round-trip unchanged into the Extended transports.'
            'Sentinel semantics carry over: plugin streams terminate with the inherited Sentinel value.'
        ) }
        @{ t = 'note'; html = 'Reference: <a href="../modules/extsutf/index.html">modules/extsutf</a>. Data: <span class="mono">Public/0.1.0/transport/</span>.' }
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
        @{ t = 'callout'; html = 'The bridge stops where the SCP begins: above <span class="mono">0x10FFFF</span>, SuperUnicode is fully its own system. <a href="../standard/spaces.html">The three spaces &raquo;</a>' }
    )
}
