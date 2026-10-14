# /Public/zipped - Extended release archives (0.1.0)
# All zips pack the corresponding 0.1.0/ sub-tree (Compress-Archive, UTF-8).

ExtSUCD.zip          whole release (extsucd + extdat + plugin-drafts +
                     collation + transport + control + charts + mappings
                     + partitions)
Plugin-SDK.zip       plugin authoring kit (README, template, hellocp
                     example)  [from plugin/sdk/]
transport.zip        vector layout, vsutf, esutf
extcollation.zip     ExtUCA ordering

The extsucd/ registry package has no separate archive: an extsucd.zip would
collide with ExtSUCD.zip on case-insensitive filesystems, so it ships inside
ExtSUCD.zip and uncompressed under 0.1.0/extsucd/.

Base SUCD.zip and its component zips live in the Base tree's zipped/.
