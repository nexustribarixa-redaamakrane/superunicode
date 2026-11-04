#ifndef SUTS_SUTS_STRS_H
#define SUTS_SUTS_STRS_H
#include <stdbool.h>
#include <stdint.h>
#include "suas/suas_sgw.h"
#include "suas/suas_suvtl.h"
#ifdef __cplusplus
extern "C" {
#endif
#define SUTS_STRS_SPEC_ID "SUTS-015"
uint8_t suts_strs_width(uint32_t codepoint, bool wide_context);
uint16_t suts_strs_orientation(uint32_t codepoint);
const char *suts_strs_version_string(void);
#ifdef __cplusplus
}
#endif
#endif
