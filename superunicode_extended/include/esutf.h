#ifndef ESUTF_H
#define ESUTF_H

/**
 * e-SUTF (Emulated/Virtual SUTF) Text Formatting and Serialization Transport
 *
 * e-SUTF is strictly a TEXT FORMATTING TRANSPORT defining page-mapped
 * virtual coordinate translation routines for transmitting ExtSUCS
 * character encoding codepoints between hypervisor host and guest contexts
 * across IPC boundaries.
 *
 * Page Architecture:
 * - The ExtSUCS address space is divided into fixed-size "pages" of
 *   ESUTF_PAGE_SIZE codepoints each.
 * - Guest systems reference codepoints via (page_index, offset) pairs.
 * - The hypervisor translates between guest virtual addresses and host
 *   physical ExtSUCS codepoint addresses.
 *
 * IPC Frame Structure:
 * - 4 bytes: page_index (uint32_t, big-endian)
 * - 2 bytes: offset     (uint16_t, big-endian)
 * - Total: 6 bytes per e-SUTF IPC frame
 */

#include "extsucs_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* e-SUTF Page Architecture Constants */
#define ESUTF_PAGE_SIZE         4096ULL   /* Codepoints per page */
#define ESUTF_PAGE_SHIFT        12        /* log2(ESUTF_PAGE_SIZE) */
#define ESUTF_OFFSET_MASK       0x0FFFULL /* Lower 12 bits = offset within page */
#define ESUTF_IPC_FRAME_BYTES   6         /* 4 (page_index) + 2 (offset) */

/* ============================================================================
 * e-SUTF Page Entry
 *
 * Maps a guest page_index to a host base address in the ExtSUCS encoding.
 * The host_base must be page-aligned (multiple of ESUTF_PAGE_SIZE).
 * ============================================================================ */
typedef struct {
    sucs_ex_char_t  host_base;    /* Host-side ExtSUCS base address (page-aligned) */
    uint32_t        page_index;   /* Guest-side page identifier */
    uint32_t        flags;        /* Page flags (read/write/execute permissions) */
} esutf_page_entry_t;

/* Page Flags */
#define ESUTF_PAGE_READ     0x01U
#define ESUTF_PAGE_WRITE    0x02U
#define ESUTF_PAGE_EXEC     0x04U
#define ESUTF_PAGE_PRESENT  0x08U

/* ============================================================================
 * Host <-> Guest Coordinate Translation
 *
 * These routines translate between flat ExtSUCS codepoint addresses and
 * guest-relative (page_index, offset) coordinate pairs.
 * ============================================================================ */

/**
 * Translates a host ExtSUCS codepoint to a guest (page_index, offset) pair.
 * Returns true on success, false on error (e.g., trap range violation).
 */
bool esutf_translate_to_guest(sucs_ex_char_t host_cp,
                              uint32_t* out_page_index,
                              uint16_t* out_offset);

/**
 * Translates a guest (page_index, offset) pair to a host ExtSUCS codepoint.
 * Returns true on success, false on error (e.g., reconstructed address invalid).
 */
bool esutf_translate_to_host(uint32_t page_index,
                             uint16_t offset,
                             sucs_ex_char_t* out_host_cp);

/* ============================================================================
 * e-SUTF IPC Frame Encoding/Decoding
 *
 * Serializes/deserializes guest-relative coordinates into compact 6-byte
 * IPC frames for transmission between hypervisor host and guest contexts.
 * ============================================================================ */

/**
 * Encodes an ExtSUCS codepoint into a 6-byte e-SUTF IPC frame.
 * Returns bytes written (6), or 0 on error.
 */
size_t esutf_encode_ipc(sucs_ex_char_t host_cp, uint8_t* out_buf, size_t buf_size);

/**
 * Decodes a 6-byte e-SUTF IPC frame into an ExtSUCS codepoint.
 * Returns bytes consumed (6), or 0 on error.
 */
size_t esutf_decode_ipc(const uint8_t* in_buf, size_t buf_size, sucs_ex_char_t* out_cp);

#ifdef __cplusplus
}
#endif

#endif /* ESUTF_H */
