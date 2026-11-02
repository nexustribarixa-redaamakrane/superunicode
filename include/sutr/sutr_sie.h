#ifndef SUTR_SUTR_SIE_H
#define SUTR_SUTR_SIE_H
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif
#define SUTR_SIE_SPEC_ID "SUTR-015"
#define SUTR_SIE_CAPACITY 32u
typedef struct { uint32_t values[SUTR_SIE_CAPACITY]; size_t head, count; } sutr_sie_queue_t;
void sutr_sie_init(sutr_sie_queue_t *q);
bool sutr_sie_push(sutr_sie_queue_t *q, uint32_t event);
bool sutr_sie_pop(sutr_sie_queue_t *q, uint32_t *event);
const char *sutr_sie_version_string(void);
#ifdef __cplusplus
}
#endif
#endif
