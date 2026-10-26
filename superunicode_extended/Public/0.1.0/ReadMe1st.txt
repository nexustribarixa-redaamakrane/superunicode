SUPERUNICODE EXTENDED /Public/0.1.0 - READ ME FIRST
====================================================

This is the first versioned release of the Extended SuperUnicode machine-
instruction database (ExtSUCD) for the Extended SuperUnicode Character
System (ExtSUCS). Read ReadMe.txt for the formal release statement; this
file is a guided tour of what is actually in the folder and how it relates
to the Base tree.

ExtSUCS is UNBOUNDED. sucs_ex_char_t is the 64-bit fast-path view;
sucs_ex_big_t carries arbitrary-precision codepoints. The Base
codepoint space 0x00000000-0x7FFFFFFF stays authoritative in the Base tree
(superunicode/Public/0.1.0/). This release only ADDS plugin-claimed
codepoints: in the Native plane, or above the Base limit.


1. TOP-LEVEL MAP
----------------

  0.1.0/
    ReadMe.txt       formal release readme
    ReadMe1st.txt    this orientation file
    extsucd/         plugin registry, Base snapshot, recorded plugin packages
    extdat/          cross-plugin extended data (readme only)
    plugin-drafts/   draft plugin packages (readme only)
    collation/       ExtUCA ordering for the unbounded space
    transport/       vector layout, vSUTF, eSUTF
    control/         inherited Base control plane (readme only)
    charts/          Unicode 18.0.0 reference PDFs (mirrored)
    mappings/        Unicode-bridge pointer for plugins
    partitions/      plugin partition policy (OWFS only)


2. extsucd/ - REGISTRY AND PLUGINS
----------------------------------

  extsucd/registry.txt
      One row per recorded plugin:
      plugin_id | version | ranges | partition_fs | state
      Current contents: exactly one entry, recorded with state "approved":
        org.openwindows.hellocp | 1.0.0 | 1 @ 0x80000000-0x80000FFF |
        OWFS | approved
      The state is recorded as "approved" when the package is in
      plugins/<id>/ AND a row exists here. This is the project's mechanical
      registry rule; it does not imply review, audit, or endorsement by a
      formal body.

  extsucd/base/
      Frozen snapshot of the Base SUCS data plugins build against. Do
      not edit; refresh only when a new Base release lands. (Readme only
      in 0.1.0 - the live data is in the Base tree.)

  extsucd/extracted/
      Per-codepoint data extracted from plugin packages recorded in the
      registry at release time (mirrors UCD/extracted). Empty until plugins
      ship derived tables.

  extsucd/auxiliary/
      Reserved for break/case/normalization tables over ExtSUCS. Empty
      in 0.1.0.

  extsucd/plugins/org.openwindows.hellocp/
      The reference "Hello Codepoints" package, mirroring
      sdk/examples/hellocp; blob name hellocp.sucsplugin.
        manifest.txt   plugin_id, 1.0.0, maintainer, base_limit
                       0x7FFFFFFF, partition_fs OWFS, description.
        ranges.txt     claims 0x80000000-0x80000FFF, i.e. strictly
                       above the Base limit.
        data.txt       packed payload marker HELLOCP:DATA:1.0.0.
        names/         HELLOCP.GREETING at 0x80000000,
                       HELLOCP.PARTING at 0x80000001.
        props/         both codepoints Printable.
        sources/       provenance: openwindows-dev@example.com,
                       2026-08-14.
        mappings/      no Base counterparts; reserved for future
                       sucs_downcast() tables.


3. extdat/ AND plugin-drafts/
-----------------------------

  extdat/          Assignments shared across plugins. The Native plane
                   and everything above 0x7FFFFFFF are plugin-claimable;
                   this directory is only for cross-plugin registry data
                   and is a readme-only placeholder in 0.1.0.
  plugin-drafts/   Draft packages under consideration; explicitly NOT
                   normative. A plugin is promoted by completing the SDK
                   template, validating with plugin_verify, then landing
                   it in extsucd/plugins/ plus a registry row at the next
                   release. Readme only in 0.1.0.


4. collation/ - ExtUCA
----------------------

  collation/ExtUCA.txt applies the SAME algorithm and weight scheme as
  Base SUCA to the unbounded space. Key contracts:
    - the whole Base region (0..0x7FFFFFFF) collates before all plugin
      regions;
    - plugin codepoints collate by implicit primary derived from the
      codepoint, monotonic within a plugin;
    - SCP controls are variable everywhere;
    - the Base Sentinel collates highest of the Base region at every
      level;
    - implicit primary is 0x1000000 + cp for cp < 2^32, and clamps to
      0x1000000 + 0xFFFFFF above that because weights are 32-bit;
    - the identical level resolves ties by numeric codepoint magnitude,
      including beyond 0xFFFFFFFFFFFFFFFF, so the total order is stable
      and deterministic.
  Per-plugin tailoring goes through suts_suca_apply_rules() or
  plugin_collation.c.


5. transport/ - CARRYING EXTENDED CODEPOINTS
--------------------------------------------

  transport/vector/VectorLayout.txt
      A vector is an in-memory sequence of sucs_ex_char_t: uint64,
      little-endian, one codepoint per unit. Length comes from the
      stream framing, not from the data (the Sentinel terminates Base
      streams; vectors are length-prefixed).

  transport/vsutf.txt
      vSUTF, the Variable SUTF transformation format, with three
      regimes:
        0x00000000-0x7FFFFFFF   standard SUTF-8 framing (1-6 bytes)
        0x80000000-0xFFFFFFFFFFFFFFFF
                                0xFE prefix + 8 bytes big-endian (9 bytes)
        > 0xFFFFFFFFFFFFFFFF    0xFF prefix + canonical LEB128 length L
                                (L >= 9) + L bytes big-endian
      0xFE/0xFF cannot collide with SUTF-8 lead bytes (0xC0-0xFD), which
      is why this design works where a generic LEB128 variable integer
      would be ambiguous. There is no maximum ExtSUCS codepoint; the 0xFF
      regime is limited only by caller storage and addressable memory.
      Base streams are already valid vSUTF streams byte-for-byte.
      Codepoints in the inherited trap range are not encodable (length
      0).

  transport/esutf.txt
      Extended SUTF: a family of fixed-base encodings over the 64-bit
      fast path, complementing Base SUTF8/16/32 which stop at
      0x7FFFFFFF. See esutf.h for the reference implementation.


6. control/, charts/, mappings/, partitions/
--------------------------------------------

  control/ReadMe.txt
      ExtSUCS inherits SCP, Traps and Sentinel VERBATIM from the Base
      tree (sudat/control/). There is no Extended redefinition; traps
      dispatch plugin diagnostics via sucs_bancode_to_trap().

  charts/ReadMe.txt
      Extended charts are generated from each recorded plugin's names/
      and props/ tables at release time. For convenience this folder also
      mirrors the 368 Unicode 18.0.0 code-chart PDFs; they cover only the
      Unicode Bridge and make no claim about plugin codepoints.

  mappings/UNICODE.txt
      Points at the Base 1:1 Unicode Bridge for 0x00000000-0x0010FFFF.
      Plugin regions have NO implicit Unicode mapping - per-plugin
      mappings live in extsucd/plugins/<id>/mappings/.

  partitions/spec.txt
      A plugin partition is created on FIRST VALID BOOT of a staged
      plugin, after the checksum gate:
        stage install -> reboot required -> boot commit (CRC32c +
        Fletcher-64 verify) -> mount -> register ranges.
      Plugin partitions are OWFS ONLY; USFS is never accepted. Always
      read-only. A quarantined (gate-failed) plugin is never mounted and
      never registers ranges. Base bugfix/rescue partitions MAY be OWFS
      or USFS but live in the Base tree.


7. BOUNDARY BETWEEN BASE AND EXTENDED
-------------------------------------

  00000000-7FFFFFFF  Base SUCS        authoritative in the Base tree
    ...Native plane   plugin-claimable (plugins may register here)
  7FFFFFFF           Base Sentinel   never allocated
  80000000+          plugin codepoints (up to arbitrary precision)

  So: read the Base tree for the codepoint space itself, and this tree
  for which plugins claim which ranges and how extended values are
  collated, transported and mounted.


8. STATUS OF THIS RELEASE
-------------------------

  0.1.0 is a STARTER release. The registry format, recording rule,
  partition policy, collation contract and transport formats are fixed,
  but there is exactly one registry entry (state "approved") and several
  readme-only directories (extdat/, plugin-drafts/, control/, extsucd/base/,
  extsucd/extracted/, extsucd/auxiliary/); charts/ mirrors the Unicode
  18.0.0 reference PDFs. Everything here is additive;
  the Base tree remains the authority for 0x00000000-0x7FFFFFFF.

  Start with the SDK: superunicode_extended/plugin/sdk/README.md.
