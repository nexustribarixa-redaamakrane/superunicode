#include "sutr/sutr_sfs.h"
const char *sutr_sfs_version_string(void) { return "SUTR-016 SFS 1.0.0"; }
bool sutr_sfs_valid_name(const char *p, size_t n) { size_t i; if (p == NULL || n == 0u || n > 127u) return false; for (i = 0u; i < n; i++) if (p[i] == '/' || p[i] == '\\' || p[i] == '\0') return false; return true; }
