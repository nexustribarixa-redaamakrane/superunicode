#ifndef SUAS_SUAS_STX_H
#define SUAS_SUAS_STX_H
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "suas/suas_sts.h"
#include "suas/suas_sbr.h"
#ifdef __cplusplus
extern "C" {
#endif
#define SUAS_STX_SPEC_ID "SUAS-021"
typedef struct { suas_sts_state_t segmentation; size_t units; } suas_stx_state_t;
void suas_stx_init(suas_stx_state_t *state);
bool suas_stx_feed(suas_stx_state_t *state, sucs_ex_char_t cp);
suas_sts_boundary_t suas_stx_flush(const suas_stx_state_t *state);
const char *suas_stx_version_string(void);
#ifdef __cplusplus
}
#endif
#endif
