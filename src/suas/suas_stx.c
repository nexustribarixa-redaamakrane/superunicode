#include "suas/suas_stx.h"
const char *suas_stx_version_string(void) { return "SUAS-021 STX 1.0.0"; }
void suas_stx_init(suas_stx_state_t *s) { if (s != NULL) { suas_sts_init(&s->segmentation, SUAS_STS_LEVEL_GRAPHEME); s->units = 0u; } }
bool suas_stx_feed(suas_stx_state_t *s, sucs_ex_char_t cp) { suas_sts_boundary_t b; if (s == NULL) return false; b = suas_sts_evaluate(&s->segmentation, cp); s->units++; return b != SUAS_STS_BOUNDARY_NONE; }
suas_sts_boundary_t suas_stx_flush(const suas_stx_state_t *s) { return s == NULL ? SUAS_STS_BOUNDARY_NONE : SUAS_STS_BOUNDARY_MUST; }
