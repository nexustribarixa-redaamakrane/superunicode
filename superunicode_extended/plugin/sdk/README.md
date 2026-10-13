# SuperUnicode Plugin SDK

Plugins extend the SuperUnicode codepoint namespace. Each plugin owns one or
more codepoint ranges in the **Native plane** (`0x120000-0x7FFFFFEF`, normal
SuperUnicode) or in **ExtSUCS** strictly above the Sentinel (`0x7FFFFFFF`).
Ranges that overlap the Unicode Bridge, the SCP, or the terminal 16 are
rejected.

## Authoring a plugin

1. Copy `template/` to `<your-plugin-id>/`.
2. Fill in `manifest.txt` and `ranges.txt` (Native-plane or above-Sentinel
   ranges).
3. Edit `src/plugin_entry.c` — the ONE required export is `sucs_plugin_entry()`.
4. Add codepoint tables in `src/plugin_data.c` (names/props), optional
   `plugin_mappings.c` / `plugin_collation.c`.
5. Build the blob with the packer:
   ```bash
   plugin_pack org.openwindows.hellocp 1 0 0 ranges.txt payload.bin hellocp.sucsplugin
   ```
   `payload.bin` is your assembled data (concatenate `names/`, `props/`, ...).
6. Verify offline:
   ```bash
   plugin_verify hellocp.sucsplugin
   ```
7. Stage in the OS: `sucs_plugin_stage_install(blob, size)` returns
   `SUCS_PLUGIN_REBOOT_REQUIRED`.

## Lifecycle (what the kernel does on restart)

`plugin_commit_on_boot()` at early boot:
1. **Checksum gate** — CRC32c + Fletcher-64 must match the packed blob.
2. **Mount** — valid plugins mount as a SuperUnicode **Plugin Partition**,
   **OWFS exclusively**, read-only.
3. **Register** — plugin ranges enter the active codepoint namespace.
4. **Quarantine** — anything failing a gate is never mounted.

Partitions: Base SuperUnicode Partitions may be OWFS or USFS (bugfix + rescue
only). Plugin Partitions MUST be OWFS and mount in Base or Extended mode.

See `examples/hellocp/` for a complete, packable plugin.

The whole SDK ships as a single download:
`superunicode_extended/Public/zipped/Plugin-SDK.zip`.
