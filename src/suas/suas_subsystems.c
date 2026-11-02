#include "suas/suas_subsystems.h"

static const suas_subsystem_descriptor_t g_subsystems[] = {
    { "SUCHA", "SuperUnicode Character Authority", "SUAS",
      "suas_sucd", SUAS_SUBSYSTEM_OWNER_SUPERUNICODE,
      SUAS_SUBSYSTEM_REFERENCE_AVAILABLE },
    { "STX", "System Text Services", "SUAS",
      "suas_sts, suas_sbr, suas_sis, suas_sdf, suas_sucf",
      SUAS_SUBSYSTEM_OWNER_SUPERUNICODE, SUAS_SUBSYSTEM_REFERENCE_AVAILABLE },
    { "SUICL", "SuperUnicode Identifier and Collation Layer", "SUTS",
      "suts_suca, suas_sis, suts_scss, suts_susch, suts_sidn",
      SUAS_SUBSYSTEM_OWNER_SUPERUNICODE, SUAS_SUBSYSTEM_REFERENCE_AVAILABLE },
    { "SULP", "SuperUnicode Locale and Presentation", "SUTS",
      "suts_sldf, suts_sueps", SUAS_SUBSYSTEM_OWNER_SUPERUNICODE,
      SUAS_SUBSYSTEM_REFERENCE_AVAILABLE },
    { "STRS", "System Text Rendering Subsystem", "SUTS",
      "suas_sgw, suas_suvtl, suas_suamr, suas_sukemet, suas_sueasd, suas_sip, suf",
      SUAS_SUBSYSTEM_OWNER_SUPERUNICODE, SUAS_SUBSYSTEM_REFERENCE_AVAILABLE },
    { "SFS", "System Filesystem", "SUTR",
      "OpenWindows VFS and OWFS/USFS", SUAS_SUBSYSTEM_OWNER_SHARED,
      SUAS_SUBSYSTEM_REFERENCE_EXTERNAL },
    { "SIM", "System IPC and Messaging", "SUTR",
      "OpenWindows ALPC and syscall boundary", SUAS_SUBSYSTEM_OWNER_SHARED,
      SUAS_SUBSYSTEM_REFERENCE_EXTERNAL },
    { "SIE", "System Input Events", "SUTR",
      "OpenWindows keyboard and UART input", SUAS_SUBSYSTEM_OWNER_SHARED,
      SUAS_SUBSYSTEM_REFERENCE_EXTERNAL },
    { "SKC", "System Kernel Console", "SUTR",
      "OpenWindows kprintf and shell console", SUAS_SUBSYSTEM_OWNER_SHARED,
      SUAS_SUBSYSTEM_REFERENCE_EXTERNAL },
    { "SKTC", "System Kernel Test and Conformance", "SUAS",
      "SuperUnicode CTest and OS host-test integration",
      SUAS_SUBSYSTEM_OWNER_SHARED, SUAS_SUBSYSTEM_REFERENCE_PENDING }
};

#define SUAS_SUBSYSTEM_COUNT \
    (sizeof g_subsystems / sizeof g_subsystems[0])

size_t suas_subsystem_count(void)
{
    return SUAS_SUBSYSTEM_COUNT;
}

const suas_subsystem_descriptor_t *suas_subsystem_at(size_t index)
{
    if (index >= SUAS_SUBSYSTEM_COUNT) {
        return NULL;
    }
    return &g_subsystems[index];
}

static int subsystem_equal(const char *a, const char *b)
{
    size_t i;

    if (a == NULL || b == NULL) {
        return a == b;
    }
    for (i = 0u; a[i] != '\0' || b[i] != '\0'; i++) {
        if (a[i] != b[i]) {
            return 0;
        }
    }
    return 1;
}

const suas_subsystem_descriptor_t *suas_subsystem_find(const char *identifier)
{
    size_t i;

    if (identifier == NULL) {
        return NULL;
    }
    for (i = 0u; i < SUAS_SUBSYSTEM_COUNT; i++) {
        if (subsystem_equal(identifier, g_subsystems[i].identifier)) {
            return &g_subsystems[i];
        }
    }
    return NULL;
}

int suas_subsystem_registry_valid(void)
{
    static const char *const established[] = {
        "SUAS", "SUTS", "SUTR", "SUTN", "SUCD", "STS", "SIS", "SUIC"
    };
    size_t i;
    size_t j;

    for (i = 0u; i < SUAS_SUBSYSTEM_COUNT; i++) {
        if (g_subsystems[i].identifier == NULL ||
            g_subsystems[i].identifier[0] == '\0' ||
            g_subsystems[i].title == NULL ||
            g_subsystems[i].document_family == NULL ||
            g_subsystems[i].substrate == NULL) {
            return 0;
        }
        for (j = 0u; j < sizeof established / sizeof established[0]; j++) {
            if (subsystem_equal(g_subsystems[i].identifier, established[j])) {
                return 0;
            }
        }
        for (j = i + 1u; j < SUAS_SUBSYSTEM_COUNT; j++) {
            if (subsystem_equal(g_subsystems[i].identifier,
                                g_subsystems[j].identifier)) {
                return 0;
            }
        }
    }
    return 1;
}
