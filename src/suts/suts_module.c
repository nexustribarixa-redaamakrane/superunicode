/* SUTS-MODULE - System Technical Specification Module Registry
 *
 * Freestanding C99 reference implementation. Zero dynamic heap allocation.
 */

#include "suts/suts_module.h"

static suts_module_t s_module_registry[SUTS_MODULE_MAX_REGISTRY];

static size_t suts_mod_strlen(const char *s)
{
    size_t len = 0;
    if (!s) return 0;
    while (s[len] != '\0') {
        len++;
    }
    return len;
}

static int suts_mod_strcmp(const char *a, const char *b)
{
    if (!a || !b) return (a == b) ? 0 : (a ? 1 : -1);
    while (*a && (*a == *b)) {
        a++;
        b++;
    }
    return (int)(unsigned char)*a - (int)(unsigned char)*b;
}

suts_status_t suts_register(const suts_module_ops_t* ops, suts_module_t** out_handle)
{
    suts_module_info_t info;
    suts_status_t st;
    size_t i;
    int free_slot = -1;

    if (!ops || !ops->get_info || !out_handle) {
        return SUTS_ERR_INVALID_ARGUMENT;
    }

    /* Query module info */
    info.name[0] = '\0';
    info.version_major = 0;
    info.version_minor = 0;
    info.version_patch = 0;
    info.capabilities = SUTS_CAP_NONE;

    st = ops->get_info(NULL, &info);
    if (st != SUTS_OK) {
        return st;
    }

    if (info.name[0] == '\0' || suts_mod_strlen(info.name) >= SUTS_MODULE_NAME_MAX) {
        return SUTS_ERR_INVALID_ARGUMENT;
    }

    /* Check for duplicate name and locate a free slot */
    for (i = 0; i < (size_t)SUTS_MODULE_MAX_REGISTRY; i++) {
        if (s_module_registry[i].active) {
            if (suts_mod_strcmp(s_module_registry[i].info.name, info.name) == 0) {
                return SUTS_ERR_INVALID_ARGUMENT; /* Duplicate module name */
            }
        } else if (free_slot < 0) {
            free_slot = (int)i;
        }
    }

    if (free_slot < 0) {
        return SUTS_ERR_UNSUPPORTED; /* Registry capacity exhausted */
    }

    suts_module_t *slot = &s_module_registry[free_slot];
    slot->active = false;
    slot->ops = *ops;
    slot->info = info;

    if (ops->init) {
        st = ops->init(slot);
        if (st != SUTS_OK) {
            slot->info.name[0] = '\0';
            return SUTS_ERR_MODULE_INIT_FAILED;
        }
    }

    slot->active = true;
    *out_handle = slot;
    return SUTS_OK;
}

suts_status_t suts_find(const char* name, suts_module_t** out_handle)
{
    size_t i;

    if (!name || !out_handle) {
        return SUTS_ERR_INVALID_ARGUMENT;
    }

    *out_handle = NULL;

    for (i = 0; i < (size_t)SUTS_MODULE_MAX_REGISTRY; i++) {
        if (s_module_registry[i].active && suts_mod_strcmp(s_module_registry[i].info.name, name) == 0) {
            *out_handle = &s_module_registry[i];
            return SUTS_OK;
        }
    }

    return SUTS_ERR_MODULE_NOT_FOUND;
}

suts_status_t suts_unregister(suts_module_t* handle)
{
    size_t i;
    bool found = false;

    if (!handle) {
        return SUTS_ERR_INVALID_ARGUMENT;
    }

    for (i = 0; i < (size_t)SUTS_MODULE_MAX_REGISTRY; i++) {
        if (&s_module_registry[i] == handle) {
            found = true;
            break;
        }
    }

    if (!found || !handle->active) {
        return SUTS_ERR_INVALID_ARGUMENT;
    }

    if (handle->ops.shutdown) {
        handle->ops.shutdown(handle);
    }

    handle->active = false;
    handle->info.name[0] = '\0';
    handle->ops.init = NULL;
    handle->ops.shutdown = NULL;
    handle->ops.get_info = NULL;

    return SUTS_OK;
}

suts_status_t suts_registry_reset(void)
{
    size_t i;

    for (i = 0; i < (size_t)SUTS_MODULE_MAX_REGISTRY; i++) {
        if (s_module_registry[i].active) {
            if (s_module_registry[i].ops.shutdown) {
                s_module_registry[i].ops.shutdown(&s_module_registry[i]);
            }
            s_module_registry[i].active = false;
            s_module_registry[i].info.name[0] = '\0';
            s_module_registry[i].ops.init = NULL;
            s_module_registry[i].ops.shutdown = NULL;
            s_module_registry[i].ops.get_info = NULL;
        }
    }

    return SUTS_OK;
}

