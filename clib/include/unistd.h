#ifndef _CLIB_UNISTD_H
#define _CLIB_UNISTD_H

#include <sys/types.h>

#define STDIN_FILENO  (0)
#define STDOUT_FILENO (1)
#define STDERR_FILENO (2)

// read, write, close, lseek, getpid, access, isatty, brk

/**
 * @brief Reads up to count bytes from the file descriptor fd.
 * 
 * @param fd The file descriptor to read from.
 * @param buf Pointer to the buffer to read into.
 * @param count Number of bytes to read.
 * @return The number of bytes read, or -1 on error.
 */
ssize_t read(int fd, void *buf, size_t count);

/**
 * @brief Writes up to count bytes from the buffer buf to the file descriptor fd.
 * 
 * @param fd The file descriptor to write to.
 * @param buf Pointer to the buffer to write from.
 * @param count Number of bytes to write.
 * @return The number of bytes written, or -1 on error.
 */
ssize_t write(int fd, const void *buf, size_t count);

/**
 * @brief Closes the file descriptor fd.
 * 
 * @param fd The file descriptor to close.
 * @return 0 on success, -1 on error.
 */
int close(int fd);

/**
 * @brief Moves the file offset of the file descriptor fd.
 * 
 * @param fd The file descriptor to move the offset of.
 * @param offset The offset to move the file offset by.
 * @param whence The position to move the file offset from (SEEK_SET, SEEK_CUR, SEEK_END).
 * @return The new file offset, or -1 on error.
 */
off_t lseek(int fd, off_t offset, int whence);

/**
 * @brief Gets the process ID of the current process.
 * 
 * @return The process ID of the current process.
 */
pid_t getpid(void);

#endif /* _CLIB_UNISTD_H */
