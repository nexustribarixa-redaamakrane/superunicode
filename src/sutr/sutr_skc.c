#include "sutr/sutr_skc.h"
const char *sutr_skc_version_string(void) { return "SUTR-018 SKC 1.0.0"; }
void sutr_skc_init(sutr_skc_buffer_t *b) { if (b != NULL) b->length = 0u; }
bool sutr_skc_write(sutr_skc_buffer_t *b, const char *t, size_t n) { size_t i; if (b == NULL || t == NULL || n > SUTR_SKC_CAPACITY - b->length) return false; for (i = 0u; i < n; i++) b->bytes[b->length + i] = t[i]; b->length += n; return true; }
