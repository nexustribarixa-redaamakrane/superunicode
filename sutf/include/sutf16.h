#ifndef SUTF_SUTF16_H
#define SUTF_SUTF16_H

#include "sucs_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * SUTF-16 Text Formatting and Serialization Transport
 *
 * SUTF-16 defines 16-bit word-packing and memory layout transport rules
 * for storing and transmitting SUCS codepoints in 1 to 2 16-bit words.
 */

/* Inline helper for SUTF-16 transport stream word length calculation */
static inline size_t sutf16_codepoint_length(sucs_char_t cp) {
    if (!sucs_is_valid(cp)) {
        return 0;
    }
    if (cp <= 0xFFFFUL) {
        return 1;
    } else {
        return 2;
    }
}

/**
 * Encodes a SUCS codepoint into the SUTF-16 text formatting and transport stream.
 * Returns 16-bit words written, or 0 on error.
 */
size_t sutf16_encode_char(sucs_char_t cp, uint16_t* out_words, size_t buf_words);

/**
 * Decodes a SUTF-16 transport stream into a SUCS character encoding codepoint.
 * Returns 16-bit words read, or 0 on error (yielding SUCS_INVALID_CODEPOINT).
 */
size_t sutf16_decode_char(const uint16_t* in_words, size_t buf_words, sucs_char_t* out_cp);

#ifdef __cplusplus
}
#endif

#endif /* SUTF_SUTF16_H */
