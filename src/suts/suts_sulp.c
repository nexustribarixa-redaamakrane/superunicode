#include "suts/suts_sulp.h"
const char *suts_sulp_version_string(void) { return "SUTS-014 SULP 1.0.0"; }
const sldf_locale_t *suts_sulp_locale(const char *tag) { return sldf_resolve_locale(tag); }
bool suts_sulp_is_presentable(uint32_t cp) { return suts_sueps_is_renderable(cp); }
