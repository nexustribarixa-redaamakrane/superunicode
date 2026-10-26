SUPERUNICODE EXTENDED /Public
=============================

Machine-instruction database for the Extended SuperUnicode Character System
(ExtSUCS). ExtSUCS is UNBOUNDED: `sucs_ex_char_t` is the 64-bit fast-path
view and `sucs_ex_big_t` carries arbitrary-precision codepoints. The default
runtime ceiling is the Base SUCS limit 0x7FFFFFFF; codepoints above it, and
plugin-claimed codepoints in the Native plane, are provided by PLUGINS, each
mounting as a read-only OWFS partition.

Unlike the Base tree, this tree has NO fixed layout overlap with it:

  0.1.0/extsucd/    registry.txt, base/ (inherited snapshot), plugins/
  0.1.0/extdat/     extended per-codepoint data
  0.1.0/plugin-drafts/  plugin proposals for the NEXT release
  0.1.0/collation/  ExtUCA ordering
  0.1.0/transport/  vector layout, vsutf, esutf
  0.1.0/control/    inherited SCP/Traps/Sentinel (Base tree)
  0.1.0/charts/, mappings/, partitions/

A plugin is recorded in the registry with state `approved` when its package
is in extsucd/plugins/<plugin-id>/ AND a row exists in extsucd/registry.txt.
That state records mechanical inclusion under the project's documented
registry rule; it is not a finding of technical review, security audit, or
endorsement.

See the SDK: superunicode_extended/plugin/sdk/README.md.
