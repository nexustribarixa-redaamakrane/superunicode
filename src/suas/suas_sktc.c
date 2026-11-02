#include "suas/suas_sktc.h"
const char *suas_sktc_version_string(void) { return "SUAS-022 SKTC 1.0.0"; }
void suas_sktc_reset(suas_sktc_report_t *r) { if (r != NULL) { r->passed = 0u; r->failed = 0u; } }
bool suas_sktc_record(suas_sktc_report_t *r, bool ok) { if (r == NULL) return false; if (ok) r->passed++; else r->failed++; return ok; }
bool suas_sktc_complete(const suas_sktc_report_t *r) { return r != NULL && r->failed == 0u && r->passed > 0u && suas_subsystem_registry_valid(); }
