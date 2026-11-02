#include "suas/suas_sucha.h"
const char *suas_sucha_version_string(void) { return "SUAS-020 SUCHA 1.0.0"; }
bool suas_sucha_ready(void) { return suas_sucd_is_loaded(); }
bool suas_sucha_query(uint32_t cp, sucd_record_t *out) { return out != NULL && suas_sucd_get_record(cp, out); }
uint32_t suas_sucha_property_mask(uint32_t cp) { return suas_sucd_property_mask(cp); }
uint8_t suas_sucha_plane_of(uint32_t cp) { return suas_sucd_plane_of(cp); }
