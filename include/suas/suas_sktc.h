#ifndef SUAS_SUAS_SKTC_H
#define SUAS_SUAS_SKTC_H
#include <stddef.h>
#include <stdbool.h>
#include "suas/suas_subsystems.h"
#ifdef __cplusplus
extern "C" {
#endif
#define SUAS_SKTC_SPEC_ID "SUAS-022"
typedef struct { size_t passed; size_t failed; } suas_sktc_report_t;
void suas_sktc_reset(suas_sktc_report_t *report);
bool suas_sktc_record(suas_sktc_report_t *report, bool passed);
bool suas_sktc_complete(const suas_sktc_report_t *report);
const char *suas_sktc_version_string(void);
#ifdef __cplusplus
}
#endif
#endif
