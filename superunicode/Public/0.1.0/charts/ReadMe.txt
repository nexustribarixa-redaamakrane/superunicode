# charts/ - code charts for Base SUCS.

# 0.1.0 ships the Base block map (../sucd/Blocks.txt) plus a REFERENCE copy
# of the Unicode 18.0.0 code charts:
#
#   Source   https://www.unicode.org/Public/18.0.0/charts/PDF/
#   Files    368 *.pdf, kept flat with Unicode's exact filenames (U0000.pdf,
#            U4E00.pdf, ...). U+XXXX block charts start at the block base.
#   Scope    The Unicode Bridge (0x00000000-0x0010FFFF) is 1:1 with Unicode,
#            so these PDFs are the authoritative printable charts for that
#            part of the Base SUCS space.
#
# They are reference material, not SUCS-native charts: codepoints in the
# control plane, the Native plane and the trap range have no Unicode chart.
# Per-codepoint SUCS charts are generated as the SCP registry and native
# allocations are assigned.
#
# Unicode charts are © Unicode, Inc.; see https://www.unicode.org/copyright.html
# for the Terms of Use. Do not edit the PDFs - re-import from the source above.
