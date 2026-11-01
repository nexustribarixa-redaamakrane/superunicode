#ifndef OW_STRING_H
#define OW_STRING_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Returned instead of a copied length when a name cannot fit. */
#define OW_STRING_COPY_REFUSED ((size_t)-1)

bool ow_sutf8_validate(const uint8_t *s, size_t len);
int ow_sutf8_name_cmp(const uint8_t *a, size_t a_len, const uint8_t *b, size_t b_len);

/* Copy a name and its terminator. Returns src_len on success, or
 * OW_STRING_COPY_REFUSED when dest is NULL, dest_cap is zero, src is NULL with
 * a non-zero length, or src_len + 1 would exceed dest_cap. A refusal leaves
 * dest untouched: the function never silently truncates a name. */
size_t ow_sutf8_name_copy(uint8_t *dest, size_t dest_cap, const uint8_t *src, size_t src_len);

#endif /* OW_STRING_H */
