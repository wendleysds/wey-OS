#ifndef _SYS_MMAN_H
#define _SYS_MMAN_H

#include_next <sys/mman.h>

#include <stddef.h>

/**
 * @brief Maps a file or device into memory.
 *
 * @param addr The address to map the file to.
 * @param length The length of the file to map.
 * @param prot The protection flags.
 * @param flags The flags.
 * @param fd The file descriptor.
 * @param offset The offset into the file.
 * @return The address of the mapped file, or NULL if an error occurred.
 */
void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);

/**
 * @brief Unmaps a file or device from memory.
 *
 * @param addr The address of the mapped file.
 * @param length The length of the mapped file.
 * @return 0 if successful, -1 if an error occurred.
 */
int munmap(void *addr, size_t length);

#endif /* _SYS_MMAN_H */