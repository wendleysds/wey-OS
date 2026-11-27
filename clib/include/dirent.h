#ifndef _CLIB_DIRENT_H
#define _CLIB_DIRENT_H

#include_next <dirent.h>

#define DT_UNKNOWN WEY_DT_UNKNOWN
#define DT_REG WEY_DT_REG
#define DT_DIR WEY_DT_DIR
#define DT_LNK WEY_DT_LNK
#define DT_FIFO WEY_DT_FIFO
#define DT_SOCK WEY_DT_SOCK
#define DT_CHR WEY_DT_CHR
#define DT_BLK WEY_DT_BLK

#ifndef NAME_MAX
#define NAME_MAX 255
#endif

struct dirent {
    ino_t d_ino;
    off_t d_off;
    unsigned short d_reclen;
    unsigned char d_type;
    char d_name[NAME_MAX + 1];
};

typedef struct __dirstream DIR;

/**
 * @brief Close a directory stream.
 *
 * @param dir The directory stream to close.
 * @return int 0 on success, -1 on error.
 */
int closedir(DIR *dir);

/**
 * @brief Get the file descriptor associated with a directory stream.
 *
 * @param dir The directory stream.
 * @return int The file descriptor associated with the directory stream.
 */
int dirfd(DIR *dir);

/**
 * @brief Open a directory stream from a file descriptor.
 *
 * @param fd The file descriptor.
 * @return DIR * A pointer to the directory stream, or NULL on error.
 */
DIR *fdopendir(int fd);

/**
 * @brief Open a directory stream.
 *
 * @param path The path to the directory.
 * @return DIR * A pointer to the directory stream, or NULL on error.
 */
DIR *opendir(const char *path);

/**
 * @brief Read a directory entry.
 *
 * @param dir The directory stream.
 * @return struct dirent * A pointer to the directory entry, or NULL on error.
 */
struct dirent *readdir(DIR *dir);

/**
 * @brief Rewind a directory stream.
 *
 * @param dir The directory stream.
 */
void rewinddir(DIR *dir);

/**
 * @brief Seek to a specific position in a directory stream.
 *
 * @param dir The directory stream.
 * @param offset The offset to seek to.
 */
void seekdir(DIR *dir, long offset);

/**
 * @brief Get the current position in a directory stream.
 *
 * @param dir The directory stream.
 * @return long The current position.
 */
long telldir(DIR *dir);

#endif /* _CLIB_DIRENT_H */