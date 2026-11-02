#include "sutr/sutr_sim.h"
const char *sutr_sim_version_string(void) { return "SUTR-017 SIM 1.0.0"; }
void sutr_sim_init(sutr_sim_channel_t *c) { if (c != NULL) { c->head = 0u; c->count = 0u; } }
bool sutr_sim_send(sutr_sim_channel_t *c, const void *d, size_t n) { size_t p, i; const uint8_t *b = (const uint8_t *)d; if (c == NULL || d == NULL || n > SUTR_SIM_PAYLOAD || c->count == SUTR_SIM_CAPACITY) return false; p = (c->head + c->count) % SUTR_SIM_CAPACITY; for (i = 0u; i < n; i++) c->messages[p].bytes[i] = b[i]; c->messages[p].length = n; c->count++; return true; }
bool sutr_sim_receive(sutr_sim_channel_t *c, void *d, size_t cap, size_t *out) { size_t i, n; uint8_t *b = (uint8_t *)d; if (c == NULL || d == NULL || out == NULL || c->count == 0u || cap < c->messages[c->head].length) return false; n = c->messages[c->head].length; for (i = 0u; i < n; i++) b[i] = c->messages[c->head].bytes[i]; *out = n; c->head = (c->head + 1u) % SUTR_SIM_CAPACITY; c->count--; return true; }
