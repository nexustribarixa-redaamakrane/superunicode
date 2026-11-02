#ifndef SUTR_SUTR_SKC_H
#define SUTR_SUTR_SKC_H
#include <stddef.h>
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif
#define SUTR_SKC_SPEC_ID "SUTR-018"
#define SUTR_SKC_CAPACITY 512u
typedef struct { char bytes[SUTR_SKC_CAPACITY]; size_t length; } sutr_skc_buffer_t;
void sutr_skc_init(sutr_skc_buffer_t *b);
bool sutr_skc_write(sutr_skc_buffer_t *b, const char *text, size_t length);
const char *sutr_skc_version_string(void);
#ifdef __cplusplus
}
#endif
#endif
