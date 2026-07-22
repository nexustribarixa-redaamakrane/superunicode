#ifndef SUPERUNICODE_SUCS_PLANE_H
#define SUPERUNICODE_SUCS_PLANE_H

#include "sucs_types.h"

/* Coordinate Extraction Macros */
#define SUCS_GET_ZONE(cp)     (((cp) >> 24) & 0x7FUL)
#define SUCS_GET_DISTRICT(cp) (((cp) >> 15) & 0xFFFFUL)
#define SUCS_GET_PLANE(cp)    (((cp) >> 8)  & 0x7FFFFFUL)
#define SUCS_GET_OFFSET(cp)   ((cp) & 0xFFUL)

/* Inline Coordinate & Property Helpers */

static inline bool sucs_is_fixed_plane(sucs_char_t cp) {
    uint32_t plane = SUCS_GET_PLANE(cp);
    return (plane == 0UL || plane == 1UL);
}

static inline sucs_codepoint_type_t sucs_classify_codepoint(sucs_char_t cp) {
    if (cp <= 0x0010FFFFUL) {
        return SUCS_TYPE_UNICODE_COMPAT;
    } else if (cp >= SUCS_SCP_MIN && cp <= SUCS_SCP_MAX) {
        return SUCS_TYPE_SYS_FUNCTION;
    } else {
        return SUCS_TYPE_NATIVE_ALLOC;
    }
}

static inline bool sucs_is_unicode_compatible(sucs_char_t cp) {
    return (cp <= 0x0010FFFFUL);
}

static inline bool sucs_is_scp_plane(sucs_char_t cp) {
    return (cp >= SUCS_SCP_MIN && cp <= SUCS_SCP_MAX);
}

static inline bool sucs_is_formatting_char(sucs_char_t cp) {
    return sucs_is_scp_plane(cp);
}

#endif /* SUPERUNICODE_SUCS_PLANE_H */
