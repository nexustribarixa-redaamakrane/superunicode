#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "suts/suts_module.h"

static int g_init_called = 0;
static int g_shutdown_called = 0;

static suts_status_t dummy_init(suts_module_t* mod)
{
    (void)mod;
    g_init_called++;
    return SUTS_OK;
}

static suts_status_t dummy_shutdown(suts_module_t* mod)
{
    (void)mod;
    g_shutdown_called++;
    return SUTS_OK;
}

static suts_status_t dummy_get_info(const suts_module_t* mod, suts_module_info_t* out_info)
{
    (void)mod;
    if (!out_info) return SUTS_ERR_INVALID_ARGUMENT;
    strncpy(out_info->name, "TEST_MOD", SUTS_MODULE_NAME_MAX - 1);
    out_info->name[SUTS_MODULE_NAME_MAX - 1] = '\0';
    out_info->version_major = 1;
    out_info->version_minor = 2;
    out_info->version_patch = 3;
    out_info->capabilities = SUTS_CAP_ENCODER | SUTS_CAP_DECODER;
    return SUTS_OK;
}

static suts_status_t failing_init(suts_module_t* mod)
{
    (void)mod;
    return SUTS_ERR_MODULE_INIT_FAILED;
}

static suts_status_t dummy2_get_info(const suts_module_t* mod, suts_module_info_t* out_info)
{
    (void)mod;
    if (!out_info) return SUTS_ERR_INVALID_ARGUMENT;
    strncpy(out_info->name, "FAIL_MOD", SUTS_MODULE_NAME_MAX - 1);
    out_info->name[SUTS_MODULE_NAME_MAX - 1] = '\0';
    out_info->version_major = 1;
    out_info->version_minor = 0;
    out_info->version_patch = 0;
    out_info->capabilities = SUTS_CAP_VALIDATOR;
    return SUTS_OK;
}

static void test_module_lifecycle(void)
{
    suts_module_ops_t ops = {
        .init = dummy_init,
        .shutdown = dummy_shutdown,
        .get_info = dummy_get_info
    };
    suts_module_t *handle = NULL;
    suts_module_t *found = NULL;

    suts_registry_reset();
    g_init_called = 0;
    g_shutdown_called = 0;

    /* Invalid arguments */
    assert(suts_register(NULL, &handle) == SUTS_ERR_INVALID_ARGUMENT);
    assert(suts_register(&ops, NULL) == SUTS_ERR_INVALID_ARGUMENT);

    /* Registration */
    assert(suts_register(&ops, &handle) == SUTS_OK);
    assert(handle != NULL);
    assert(g_init_called == 1);
    assert(strcmp(handle->info.name, "TEST_MOD") == 0);
    assert(handle->info.version_major == 1);
    assert(handle->info.version_minor == 2);
    assert(handle->info.version_patch == 3);
    assert(handle->info.capabilities == (SUTS_CAP_ENCODER | SUTS_CAP_DECODER));

    /* Duplicate registration rejected */
    assert(suts_register(&ops, &handle) == SUTS_ERR_INVALID_ARGUMENT);

    /* Lookup */
    assert(suts_find(NULL, &found) == SUTS_ERR_INVALID_ARGUMENT);
    assert(suts_find("TEST_MOD", NULL) == SUTS_ERR_INVALID_ARGUMENT);
    assert(suts_find("NONEXISTENT", &found) == SUTS_ERR_MODULE_NOT_FOUND);
    assert(suts_find("TEST_MOD", &found) == SUTS_OK);
    assert(found == handle);

    /* Unregistration */
    assert(suts_unregister(NULL) == SUTS_ERR_INVALID_ARGUMENT);
    assert(suts_unregister(handle) == SUTS_OK);
    assert(g_shutdown_called == 1);

    /* Lookup after unregister fails */
    found = NULL;
    assert(suts_find("TEST_MOD", &found) == SUTS_ERR_MODULE_NOT_FOUND);

    /* Double unregister fails */
    assert(suts_unregister(handle) == SUTS_ERR_INVALID_ARGUMENT);

    printf("[PASS] test_module_lifecycle\n");
}

static void test_failing_init(void)
{
    suts_module_ops_t ops = {
        .init = failing_init,
        .shutdown = NULL,
        .get_info = dummy2_get_info
    };
    suts_module_t *handle = NULL;
    suts_module_t *found = NULL;

    suts_registry_reset();
    assert(suts_register(&ops, &handle) == SUTS_ERR_MODULE_INIT_FAILED);
    assert(suts_find("FAIL_MOD", &found) == SUTS_ERR_MODULE_NOT_FOUND);

    printf("[PASS] test_failing_init\n");
}

int main(void)
{
    test_module_lifecycle();
    test_failing_init();
    printf("All SUTS Module tests passed.\n");
    return 0;
}

