#ifndef SUTS_SUTS_MODULE_H
#define SUTS_SUTS_MODULE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── SUTS Version & Identification ─────────────────────────────── */

#ifndef SUTS_VERSION_MAJOR
#define SUTS_VERSION_MAJOR 0
#endif
#ifndef SUTS_VERSION_MINOR
#define SUTS_VERSION_MINOR 1
#endif
#ifndef SUTS_VERSION_PATCH
#define SUTS_VERSION_PATCH 0
#endif

/* ── Freestanding Compatibility ─────────────────────────────────── */

#ifndef SUTS_FREESTANDING
#if defined(__STDC_HOSTED__) && __STDC_HOSTED__ == 0
#define SUTS_FREESTANDING 1
#else
#define SUTS_FREESTANDING 0
#endif
#endif

/* ── Module Metadata ────────────────────────────────────────────── */

#define SUTS_MODULE_NAME_MAX 64
#define SUTS_MODULE_MAX_REGISTRY 32

/* ── Status Codes ───────────────────────────────────────────────── */

typedef enum {
    SUTS_OK                      =  0,
    SUTS_ERR_NOT_INITIALIZED     = -1,
    SUTS_ERR_INVALID_ARGUMENT    = -2,
    SUTS_ERR_MODULE_NOT_FOUND    = -3,
    SUTS_ERR_MODULE_INIT_FAILED  = -4,
    SUTS_ERR_UNSUPPORTED         = -5
} suts_status_t;

/* ── Module Interface Types ─────────────────────────────────────── */

/**
 * Module capability flags.
 * Individual modules advertise which capabilities they support.
 */
typedef enum {
    SUTS_CAP_NONE           = 0,
    SUTS_CAP_ENCODER        = (1 << 0),
    SUTS_CAP_DECODER        = (1 << 1),
    SUTS_CAP_VALIDATOR      = (1 << 2),
    SUTS_CAP_TRANSFORM      = (1 << 3)
} suts_module_caps_t;

/**
 * Module descriptor — identifies a registered module.
 */
typedef struct {
    char     name[SUTS_MODULE_NAME_MAX];
    uint32_t version_major;
    uint32_t version_minor;
    uint32_t version_patch;
    uint32_t capabilities;
} suts_module_info_t;

/**
 * Module handle type.
 */
typedef struct suts_module suts_module_t;

/**
 * Module lifecycle vtable — each module implements these callbacks.
 */
typedef struct {
    suts_status_t (*init)(suts_module_t* module);
    suts_status_t (*shutdown)(suts_module_t* module);
    suts_status_t (*get_info)(const suts_module_t* module, suts_module_info_t* out_info);
} suts_module_ops_t;

/**
 * Module instance descriptor.
 */
struct suts_module {
    bool active;
    suts_module_ops_t ops;
    suts_module_info_t info;
};

/* ── Registry Interface ─────────────────────────────────────────── */

/**
 * Registers a module with the given operations table.
 */
suts_status_t suts_register(const suts_module_ops_t* ops, suts_module_t** out_handle);

/**
 * Looks up a module by name.
 */
suts_status_t suts_find(const char* name, suts_module_t** out_handle);

/**
 * Unregisters a previously registered module.
 */
suts_status_t suts_unregister(suts_module_t* handle);

/**
 * Resets the module registry, unregistering all active modules.
 */
suts_status_t suts_registry_reset(void);

#ifdef __cplusplus
}
#endif

#endif /* SUTS_SUTS_MODULE_H */
