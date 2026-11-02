#ifndef SUTR_SUTR_SIM_H
#define SUTR_SUTR_SIM_H
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif
#define SUTR_SIM_SPEC_ID "SUTR-017"
#define SUTR_SIM_CAPACITY 16u
#define SUTR_SIM_PAYLOAD 256u
typedef struct { uint8_t bytes[SUTR_SIM_PAYLOAD]; size_t length; } sutr_sim_message_t;
typedef struct { sutr_sim_message_t messages[SUTR_SIM_CAPACITY]; size_t head, count; } sutr_sim_channel_t;
void sutr_sim_init(sutr_sim_channel_t *c);
bool sutr_sim_send(sutr_sim_channel_t *c, const void *data, size_t length);
bool sutr_sim_receive(sutr_sim_channel_t *c, void *data, size_t capacity, size_t *length);
const char *sutr_sim_version_string(void);
#ifdef __cplusplus
}
#endif
#endif
