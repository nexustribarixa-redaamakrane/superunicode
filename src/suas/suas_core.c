/* SUAS-CORE - Kernel Initialization and Version Reporting
 *
 * Freestanding C99, zero dynamic allocation.
 */

#include "suas/suas_core.h"

suas_status_t suas_kernel_init(suas_kernel_t* kernel)
{
    if (!kernel) {
        return SUAS_ERR_INVALID_ARGUMENT;
    }

    kernel->magic = SUAS_KERNEL_MAGIC;
    kernel->version_major = SUAS_VERSION_MAJOR;
    kernel->version_minor = SUAS_VERSION_MINOR;
    kernel->version_patch = SUAS_VERSION_PATCH;
    kernel->flags = 0u;
    kernel->state = SUAS_KERNEL_STATE_INITIALIZED;

    return SUAS_OK;
}

suas_status_t suas_kernel_version(uint32_t* out_major,
                                  uint32_t* out_minor,
                                  uint32_t* out_patch)
{
    if (!out_major || !out_minor || !out_patch) {
        return SUAS_ERR_INVALID_ARGUMENT;
    }

    *out_major = SUAS_VERSION_MAJOR;
    *out_minor = SUAS_VERSION_MINOR;
    *out_patch = SUAS_VERSION_PATCH;

    return SUAS_OK;
}

