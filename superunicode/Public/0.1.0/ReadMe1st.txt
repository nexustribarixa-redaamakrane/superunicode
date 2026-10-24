SUPERUNICODE /Public/0.1.0 - READ ME FIRST
==========================================

This is the first versioned release of the Base SuperUnicode machine-
instruction database (SUCD) for the Base SuperUnicode Character System
(SUCS). Read ReadMe.txt for the formal release statement; this file is a
guided tour of what is actually in the folder and how the pieces fit.

The Base SUCS codepoint space is 0x00000000-0x7FFFFFFF (31-bit). Anything
above it is plugin territory and lives in the Extended tree
(superunicode_extended/Public/0.1.0/).


1. TOP-LEVEL MAP
----------------

  0.1.0/
    ReadMe.txt      formal release readme (what the release IS)
    ReadMe1st.txt   this orientation file (what the release CONTAINS)
    sucd/           character database: inventory, blocks, properties
    sudat/          machine-instruction data: SCP slots, diagnostics
    names/          codepoint and block names, aliases
    sources/        provenance (author + date per range)
    collation/      SUCA collation ordering
    sutf/           SUTF-8/16/32 framing rules
    mappings/       UNICODE bridge, legacy/obsolete placeholders
    partitions/     partition policy (OWFS / USFS)
    charts/         printable charts: Unicode 18.0.0 reference PDFs


2. sucd/ - THE CHARACTER DATABASE
---------------------------------

  sucd/SUCSData.txt
      The codepoint inventory. Semicolon columns:
      <codepoint-or-range>;<name>;<type>. Type 0 = printable native
      allocation (cursor advances); type 1 = control / machine
      instruction (cursor does NOT advance). This is the source of truth
      for the zone layout below.

  sucd/Blocks.txt
      The same space as named blocks (Unicode Bridge, SuperUnicode
      Control Plane, BANcode, WARNcode, COMcode, SOFTcode, Native,
      Traps, Sentinel).

  sucd/Props/PropList.txt
      Explicit per-range properties: Unicode_Bridged, Control_Plane,
      Diagnostic_BAN/WARN/COM/SOFT, Printable, Trap_Instruction,
      Sentinel_Value.

  sucd/Props/BidiProps.txt
      BiDi masks for the Unicode Bridge (LTR, RTL, ARABIC_AL, NEUTRAL,
      MIRRORED, WHITESPACE), sourced from SUAS-001 Structural
      Directional Framing. Unlisted codepoints default to NEUTRAL +
      WHITESPACE. This is the only substantial property table in 0.1.0.

  sucd/Derived/DerivedCoreProperties.txt
      Inherited closure: Alphabetic over Bridge + Native, Control over
      the control plane, diagnostic blocks, and traps.

  sucd/auxiliary/
      Empty in 0.1.0. Its readme reserves the space for future
      GraphemeBreakProperty, WordBreakProperty, SentenceBreakProperty,
      CaseFolding and NormalizationTest data (this release has no case
      or break constraints yet).


3. sudat/ - MACHINE-INSTRUCTION DATA
------------------------------------

  sudat/assignments.txt
      SCP slot-allocation convention: one machine instruction per SCP
      slot; slots below 0x0011A000 are reserved for future blocks.

  sudat/control/SCP.txt
      The control plane layout, 0x00110000-0x0011FFFF:
        00110000-0011A000  reserved
        0011A000-0011A7FF  BANcode   unrecoverable critical faults
        0011A800-0011ABFF  WARNcode  non-fatal telemetry / alerts
        0011AC00-0011ADFF  COMcode   IPC, bus, handshakes, driver attach
        0011AE00-0011AEFF  SOFTcode  soft recovery / self-healing
        0011AF00-0011FFFF  reserved

  sudat/control/Traps.txt
      Traps 0x7FFFFFF0-0x7FFFFFFE. A trap enters the diagnostic
      subsystem and does not advance the cursor; every trap slot is
      still unassigned.

  sudat/control/Sentinel.txt
      SUCS_CP_SENTINEL = 0x7FFFFFFF. End-of-stream marker, not a
      printable allocation. No plugin may allocate at it or anywhere in
      the terminal 16 codepoints.

  sudat/bancodes/
      Four slot registries, one per diagnostic block. Every slot in all
      four files is currently UNASSIGNED, so the blocks are reserved
      shape, not content:
        BANcode.txt   2048 slots (0x0011A000-0x0011A7FF), subcategorised
                      into kernel traps, plugin quarantine, transport,
                      OWFS/USFS integrity, HTL hardware, MBL bootloader,
                      UEFI firmware; 0x0011A780-0x0011A7FF has no trap
                      handler.
        WARNcode.txt  1024 slots, split mode/config, OWFS state, USFS
                      state, boot & protocol.
        COMcode.txt    512 slots, split kernel/plugins, IPC, bus,
                      driver/power, stream handshakes, storage, FS
                      control plane, reserved.
        SOFTcode.txt   256 slots, split encoding errors, soft recovery,
                      OWFS/USFS soft errors and recovery, HTL hardware,
                      boot & protocol.
      Diagnostic codepoints "may be printable": they can render an error
      glyph rather than only a codename.


4. names/, sources/
-------------------

  names/NamesList.txt   starter names: BLOCK BANCODE, BLOCK WARNCODE,
                        BLOCK COMCODE, BLOCK SOFTCODE, BLOCK TRAPS,
                        SENTINEL.
  names/NameAliases.txt empty in 0.1.0 (no formal aliases).
  sources/Sources.txt   provenance per range: the Unicode Bridge comes
                        from unicode.org; control plane and native space
                        from the superunicode project; all dated
                        2026-08-14.


5. collation/ - SUCA
--------------------

  collation/SUCA.txt defines the SuperUnicode Collation Algorithm
  (SUCET) for the Base space, formalised by SUTS-001. Multilevel L1-L4
  plus identical level; defaults tertiary strength, shifted variables,
  NFD on. Key contracts:
    - all SCP control codepoints are variable (ignorable L1-L3).
    - the Sentinel collates highest at every level.
    - unassigned codepoints get an algorithmic implicit primary
      (0x1000000 + cp).
    - the identical level tie-breaks by native codepoint order.
  The reference implementation lives in include/suts/suts_suca.h and
  src/suts/suts_suca.c and embeds a compiled subset of this table.


6. sutf/ - SERIALISATION
------------------------

  SUTF8.txt   standard UTF-8 leading-byte framing over 31 bits; Base
              SUCS never emits a sequence above 0x7FFFFFFF.
  SUTF16.txt  NO SURROGATES anywhere: 0xD800-0xDFFF are ordinary PUA
              codepoints. One word for 0x0000-0x7FFF, two words
              (marker lead word with bit 15 set) up to 0x7FFFFFFF.
              Byte order is an explicit per-transport choice (SUST-16),
              never a BOM.
  SUTF32.txt  fixed 4-byte little-endian units, 31 significant bits,
              the only lossless framing for the full native space.

  Note the deliberate asymmetry: SUTF16 covers the entire native space,
  SUTF8 does not, and SUTF32 always does.


7. mappings/ AND partitions/
----------------------------

  mappings/UNICODE.txt   the 1:1 Unicode Bridge over
                         0x00000000-0x0010FFFF: same numeric value,
                         bi-directional and lossless; sucs_downcast()
                         of a bridged codepoint returns that Unicode
                         value.
  mappings/LEGACY.txt    reserved for pre-Bridge encodings; empty.
  mappings/OBSOLETE.txt  reserved for superseded mappings; empty.

  partitions/spec.txt    A SuperUnicode Partition is a removable
                         filesystem declared by machine instructions.
                         Base SUCS uses partitions only for bugfix and
                         rescue payloads, and MAY use OWFS or USFS.
                         Mounts are always read-only; plugin partitions
                         must be OWFS and mount after the checksum gate.


8. ZONE LAYOUT AT A GLANCE
--------------------------

  00000000-0010FFFF  Unicode Bridge        type 0, 1:1 with Unicode
  00110000-0011FFFF  Control Plane (SCP)   type 1
    0011A000-0011A7FF  BANcode
    0011A800-0011ABFF  WARNcode
    0011AC00-0011ADFF  COMcode
    0011AE00-0011AEFF  SOFTcode
  00120000-7FFFFFFF  Native allocations    type 0
  7FFFFFF0-7FFFFFFE  Traps                 type 1
  7FFFFFFF           Sentinel              not an allocation


9. STATUS OF THIS RELEASE
-------------------------

  0.1.0 is a STARTER release: the zone layout, control-plane design,
  collation contract, serialisation rules and partition policy are
  fixed, while the actual codepoint payload is mostly not yet assigned.
  Expect UNASSIGNED slots and empty optional tables (sucd/auxiliary/).
  charts/ is populated with the 368 Unicode 18.0.0 code-chart PDFs, which
  cover the 1:1 Unicode Bridge; SUCS-native charts are generated later.
  Later releases fill the rest in without changing the architecture.

  Codepoints outside 0x00000000-0x7FFFFFFF - including plugin claims in
  the Native plane and everything above the Sentinel - are NOT here.
  See superunicode_extended/Public/0.1.0/ and the plugin SDK at
  superunicode_extended/plugin/sdk/README.md.
