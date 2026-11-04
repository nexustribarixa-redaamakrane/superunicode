#include "suts/suts_suicl.h"
const char *suts_suicl_version_string(void) { return "SUTS-013 SUICL 1.0.0"; }
bool suts_suicl_validate(const sucs_ex_char_t *p, size_t n) { size_t i; if (p == NULL || n == 0u || !suas_sis_is_start(p[0])) return false; for (i = 1u; i < n; i++) if (!suas_sis_is_continue(p[i])) return false; return true; }
int suts_suicl_compare(const sucs_ex_char_t *a, size_t an, const sucs_ex_char_t *b, size_t bn) { size_t i, n = an < bn ? an : bn; if (a == NULL || b == NULL) return a == b ? 0 : (a == NULL ? -1 : 1); for (i = 0u; i < n; i++) if (a[i] != b[i]) return a[i] < b[i] ? -1 : 1; return an == bn ? 0 : (an < bn ? -1 : 1); }
