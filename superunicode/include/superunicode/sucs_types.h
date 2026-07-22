#ifndef SUPERUNICODE_SUCS_TYPES_H
#define SUPERUNICODE_SUCS_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* 31-bit code point representation */
typedef uint32_t sucs_char_t;

/* Maximum valid SUCS code point boundary (31-bit maximum) */
#define SUCS_MAX_CODEPOINT 0x7FFFFFFFUL

/* System Control Plane (SCP) boundaries: Plane 17, District 1 */
#define SUCS_SCP_MIN 0x00110000UL
#define SUCS_SCP_MAX 0x0011FFFFUL

/* Native SUCS Formatting & System Control Points (SCP) */
#define SUCS_FMT_BOLD_ON    0x00110000UL
#define SUCS_FMT_BOLD_OFF   0x00110001UL
#define SUCS_FMT_ITALIC_ON  0x00110002UL
#define SUCS_FMT_ITALIC_OFF 0x00110003UL
#define SUCS_FMT_COLOR_RGB  0x00110010UL
#define SUCS_FMT_RESET      0x001100FFUL

/* SUES Status Return Codes */
typedef enum {
    SUES_SUCCESS               = 0,
    SUES_ERR_INVALID_BYTE      = -1,
    SUES_ERR_BUFFER_TOO_SMALL  = -2,
    SUES_ERR_OUT_OF_BOUNDS     = -3
} sues_status_t;

/* Code Point Classification Types */
typedef enum {
    SUCS_TYPE_UNICODE_COMPAT = 0, /* 0x00000000 - 0x0010FFFF: Unicode Parity Zone */
    SUCS_TYPE_SYS_FUNCTION   = 1, /* 0x00110000 - 0x0011FFFF: System Control Plane (SCP) */
    SUCS_TYPE_NATIVE_ALLOC    = 2  /* 0x00120000 - 0x7FFFFFFF: Native Extended Allocations */
} sucs_codepoint_type_t;

/* Kernel-Safe Descriptor */
typedef struct {
    uint32_t length_bytes;
    uint32_t capacity_bytes;
    char*    buffer;
} SUCS_STRING;

#endif /* SUPERUNICODE_SUCS_TYPES_H */
