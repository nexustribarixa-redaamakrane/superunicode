#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <stddef.h>

#include "suas/suas_core.h"

static void test_kernel_version(void)
{
    uint32_t maj = 99, min = 99, pat = 99;

    assert(suas_kernel_version(NULL, &min, &pat) == SUAS_ERR_INVALID_ARGUMENT);
    assert(suas_kernel_version(&maj, NULL, &pat) == SUAS_ERR_INVALID_ARGUMENT);
    assert(suas_kernel_version(&maj, &min, NULL) == SUAS_ERR_INVALID_ARGUMENT);

    assert(suas_kernel_version(&maj, &min, &pat) == SUAS_OK);
    assert(maj == SUAS_VERSION_MAJOR);
    assert(min == SUAS_VERSION_MINOR);
    assert(pat == SUAS_VERSION_PATCH);
    assert(maj == 0);
    assert(min == 1);
    assert(pat == 0);

    printf("[PASS] test_kernel_version\n");
}

static void test_kernel_init(void)
{
    suas_kernel_t kernel;

    assert(suas_kernel_init(NULL) == SUAS_ERR_INVALID_ARGUMENT);

    assert(suas_kernel_init(&kernel) == SUAS_OK);
    assert(kernel.magic == SUAS_KERNEL_MAGIC);
    assert(kernel.version_major == SUAS_VERSION_MAJOR);
    assert(kernel.version_minor == SUAS_VERSION_MINOR);
    assert(kernel.version_patch == SUAS_VERSION_PATCH);
    assert(kernel.state == SUAS_KERNEL_STATE_INITIALIZED);

    printf("[PASS] test_kernel_init\n");
}

static void test_constants_and_types(void)
{
    assert(SUAS_CODEPOINT_BITS == 31);
    assert(SUAS_CODEPOINT_MAX == 0x7FFFFFFFUL);
    assert(SUAS_ZONE_UNICODE_MIN == 0x00000000UL);
    assert(SUAS_ZONE_UNICODE_MAX == 0x0010FFFFUL);
    assert(SUAS_ZONE_SCP_MIN == 0x00110000UL);
    assert(SUAS_ZONE_SCP_MAX == 0x0011FFFFUL);
    assert(SUAS_ZONE_NATIVE_MIN == 0x00120000UL);
    assert(SUAS_ZONE_NATIVE_MAX == 0x7FFFFFFEUL);

    assert(SUAS_OK == 0);
    assert(SUAS_ERR_NOT_INITIALIZED < 0);
    assert(SUAS_ERR_INVALID_ARGUMENT < 0);

    printf("[PASS] test_constants_and_types\n");
}

int main(void)
{
    test_constants_and_types();
    test_kernel_version();
    test_kernel_init();
    printf("All SUAS Core tests passed.\n");
    return 0;
}

