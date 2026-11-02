#include "suts/suts_strs.h"
const char *suts_strs_version_string(void) { return "SUTS-016 STRS 1.0.0"; }
uint8_t suts_strs_width(uint32_t cp, bool wide) { suas_sgw_options_t o; suas_sgw_options_default(&o); return (uint8_t)suas_sgw_resolve_ambiguous(cp, wide, &o); }
uint16_t suts_strs_orientation(uint32_t cp) { return suas_suvtl_orientation_of(cp); }
