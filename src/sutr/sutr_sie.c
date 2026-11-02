#include "sutr/sutr_sie.h"
const char *sutr_sie_version_string(void) { return "SUTR-015 SIE 1.0.0"; }
void sutr_sie_init(sutr_sie_queue_t *q) { if (q != NULL) { q->head = 0u; q->count = 0u; } }
bool sutr_sie_push(sutr_sie_queue_t *q, uint32_t e) { size_t p; if (q == NULL || q->count == SUTR_SIE_CAPACITY) return false; p = (q->head + q->count) % SUTR_SIE_CAPACITY; q->values[p] = e; q->count++; return true; }
bool sutr_sie_pop(sutr_sie_queue_t *q, uint32_t *e) { if (q == NULL || e == NULL || q->count == 0u) return false; *e = q->values[q->head]; q->head = (q->head + 1u) % SUTR_SIE_CAPACITY; q->count--; return true; }
