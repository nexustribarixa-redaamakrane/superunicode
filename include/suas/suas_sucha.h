#ifndef SUAS_SUAS_SUCHA_H
#define SUAS_SUAS_SUCHA_H
#include <stdint.h>
#include <stdbool.h>
#include "suas/suas_sucd.h"
#ifdef __cplusplus
extern "C" {
#endif
#define SUAS_SUCHA_SPEC_ID "SUAS-020"
const char *suas_sucha_version_string(void);
bool suas_sucha_ready(void);
bool suas_sucha_query(uint32_t codepoint, sucd_record_t *out_record);
uint32_t suas_sucha_property_mask(uint32_t codepoint);
uint8_t suas_sucha_plane_of(uint32_t codepoint);
#ifdef __cplusplus
}
#endif
#endif
