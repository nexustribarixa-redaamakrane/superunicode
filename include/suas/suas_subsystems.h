#ifndef SUAS_SUAS_SUBSYSTEMS_H
#define SUAS_SUAS_SUBSYSTEMS_H

/*
 * SuperUnicode subsystem authority registry.
 *
 * This registry names architectural subsystems without changing the
 * established SUAS, SUTS, SUTR, or SUTN document identifiers. It records
 * standards ownership separately from reference and operating-system
 * implementation ownership.
 */

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SUAS_SUBSYSTEMS_VERSION_MAJOR 1u
#define SUAS_SUBSYSTEMS_VERSION_MINOR 0u
#define SUAS_SUBSYSTEMS_VERSION_PATCH 0u

typedef enum {
    SUAS_SUBSYSTEM_OWNER_SUPERUNICODE = 0,
    SUAS_SUBSYSTEM_OWNER_SHARED       = 1
} suas_subsystem_owner_t;

typedef enum {
    SUAS_SUBSYSTEM_REFERENCE_AVAILABLE = 0,
    SUAS_SUBSYSTEM_REFERENCE_EXTERNAL  = 1,
    SUAS_SUBSYSTEM_REFERENCE_PENDING   = 2
} suas_subsystem_reference_t;

typedef struct {
    const char *identifier;
    const char *title;
    const char *document_family;
    const char *substrate;
    suas_subsystem_owner_t standards_owner;
    suas_subsystem_reference_t reference_status;
} suas_subsystem_descriptor_t;

size_t suas_subsystem_count(void);
const suas_subsystem_descriptor_t *suas_subsystem_at(size_t index);
const suas_subsystem_descriptor_t *suas_subsystem_find(const char *identifier);
int suas_subsystem_registry_valid(void);

#ifdef __cplusplus
}
#endif

#endif
