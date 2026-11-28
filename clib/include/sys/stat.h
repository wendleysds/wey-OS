#ifndef _CLIB_SYS_STAT_H
#define _CLIB_SYS_STAT_H

#include_next <sys/stat.h>

/**
 * @brief Get file attributes
 * @param pathname Path to the file
 * @param statbuf Pointer to the stat structure
 * @return 0 on success, -1 on error
 */
int stat(const char *restrict pathname, struct stat *restrict statbuf);

#endif /* SYS_STAT_H */