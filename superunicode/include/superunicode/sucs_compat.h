#ifndef SUPERUNICODE_SUCS_COMPAT_H
#define SUPERUNICODE_SUCS_COMPAT_H

#include "sucs_types.h"

#define SUCS_UNICODE_MAX_COMPAT 0x0010FFFFUL

static inline bool sucs_is_unicode_compat(sucs_char_t cp) {
    return (cp <= SUCS_UNICODE_MAX_COMPAT);
}

static inline bool sucs_is_native_extended(sucs_char_t cp) {
    return (cp > SUCS_UNICODE_MAX_COMPAT && cp <= SUCS_MAX_CODEPOINT);
}

#endif /* SUPERUNICODE_SUCS_COMPAT_H */
