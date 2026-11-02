#ifndef SUTR_SUTR_SFS_H
#define SUTR_SUTR_SFS_H
#include <stddef.h>
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif
#define SUTR_SFS_SPEC_ID "SUTR-016"
bool sutr_sfs_valid_name(const char *name, size_t length);
const char *sutr_sfs_version_string(void);
#ifdef __cplusplus
}
#endif
#endif
