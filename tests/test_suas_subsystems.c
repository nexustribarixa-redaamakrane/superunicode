#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "suas/suas_subsystems.h"

int main(void)
{
    const suas_subsystem_descriptor_t *s;

    assert(suas_subsystem_count() == 10u);
    assert(suas_subsystem_registry_valid());
    assert(suas_subsystem_at(10u) == NULL);
    assert(suas_subsystem_find(NULL) == NULL);
    assert(suas_subsystem_find("STS") == NULL);
    assert(suas_subsystem_find("SIS") == NULL);
    assert(suas_subsystem_find("SUIC") == NULL);
    assert(suas_subsystem_find("SUCD") == NULL);

    s = suas_subsystem_find("SUCHA");
    assert(s != NULL);
    assert(strcmp(s->document_family, "SUAS") == 0);
    assert(s->reference_status == SUAS_SUBSYSTEM_REFERENCE_AVAILABLE);

    s = suas_subsystem_find("SFS");
    assert(s != NULL);
    assert(s->standards_owner == SUAS_SUBSYSTEM_OWNER_SHARED);
    assert(s->reference_status == SUAS_SUBSYSTEM_REFERENCE_EXTERNAL);

    s = suas_subsystem_find("SKTC");
    assert(s != NULL);
    assert(s->reference_status == SUAS_SUBSYSTEM_REFERENCE_PENDING);

    puts("All subsystem registry tests passed.");
    return 0;
}
