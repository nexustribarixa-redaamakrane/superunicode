#ifndef SUTS_SUTS_SUICL_H
#define SUTS_SUTS_SUICL_H
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "suas/suas_sis.h"
#include "suts/suts_suca.h"
#ifdef __cplusplus
extern "C" {
#endif
#define SUTS_SUICL_SPEC_ID "SUTS-013"
bool suts_suicl_validate(const sucs_ex_char_t *text, size_t length);
int suts_suicl_compare(const sucs_ex_char_t *a, size_t an, const sucs_ex_char_t *b, size_t bn);
const char *suts_suicl_version_string(void);
#ifdef __cplusplus
}
#endif
#endif
