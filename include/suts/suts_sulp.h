#ifndef SUTS_SUTS_SULP_H
#define SUTS_SUTS_SULP_H
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "suts/suts_sldf.h"
#include "suts/suts_sueps.h"
#ifdef __cplusplus
extern "C" {
#endif
#define SUTS_SULP_SPEC_ID "SUTS-015"
const sldf_locale_t *suts_sulp_locale(const char *tag);
bool suts_sulp_is_presentable(uint32_t codepoint);
const char *suts_sulp_version_string(void);
#ifdef __cplusplus
}
#endif
#endif
