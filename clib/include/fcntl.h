#ifndef _CLIB_FCNTL_H
#define _CLIB_FCNTL_H

#include <sys/types.h>

#define O_RDONLY    0
#define O_WRONLY    (1 << 0)
#define O_RDWR      (1 << 1)
#define O_CREAT     (1 << 6)
#define O_EXCL      (1 << 7)
#define O_NOCTTY    (1 << 8)
#define O_TRUNC     (1 << 9)
#define O_APPEND    (1 << 10)
#define O_NONBLOCK  (1 << 11)
#define O_DSYNC     (1 << 12)
#define O_ASYNC     (1 << 13)
#define O_DIRECT    (1 << 14)
#define O_LARGEFILE (1 << 15)
#define O_DIRECTORY (1 << 16)
#define O_NOFOLLOW  (1 << 17)
#define O_NOATIME   (1 << 18)
#define O_CLOEXEC   (1 << 19)
#define O_PATH      (1 << 21)
#define O_TMPFILE   (1 << 22)
#define O_ACCMODE (O_RDONLY | O_WRONLY | O_RDWR)

/**
 * @brief Creates a new file or truncates an existing file.
 * 
 * @param pathname The path to the file to create or truncate.
 * @param mode The mode to create the file with.
 * @return The file descriptor of the created file, or -1 if an error occurred.
 */
int creat(const char *pathname, mode_t mode);

/**
 * @brief Opens a file.
 * 
 * @param pathname The path to the file to open.
 * @param flags The flags to open the file with.
 * @param ... The mode to open the file with. (optional)
 * @return The file descriptor of the opened file, or -1 if an error occurred.
 */
int open(const char *pathname, int flags, ... /* umode_t mode */);

#endif