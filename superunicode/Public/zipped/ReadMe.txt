# /Public/zipped - Base SUCS release archives (0.1.0)
# All zips pack the corresponding 0.1.0/ sub-tree (Compress-Archive, UTF-8).

SUCD.zip          whole release (sucd + sudat + names + sources + collation
                  + sutf + charts + mappings + partitions)  [UCD.zip analog]
sudat.zip         unidata: SCP control plane, Traps, Sentinel, bancodes
                  [UNIDATA.zip analog]
names.zip         NamesList, NameAliases
sources.zip       provenance
collation.zip     SUCA ordering
sutf.zip          SUTF8/16/32 framing tables
mappings.zip      UNICODE, LEGACY, OBSOLETE
charts.zip        code charts
partitions.zip    partition policy (OWFS or USFS)

The sucd/ core inventory has no separate archive: a sucd.zip would collide
with SUCD.zip on case-insensitive filesystems, so it ships inside SUCD.zip
and uncompressed under 0.1.0/sucd/.

ExtSUCD.zip and Plugin-SDK.zip live in the Extended tree's zipped/.
