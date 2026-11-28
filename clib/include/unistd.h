#ifndef _CLIB_UNISTD_H
#define _CLIB_UNISTD_H

#include <sys/types.h>

#define STDIN_FILENO  (0)
#define STDOUT_FILENO (1)
#define STDERR_FILENO (2)

#define F_OK 0
#define X_OK 1
#define W_OK 2
#define R_OK 4

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

/**
 * @brief Creates a new process.
 * 
 * @return The process ID of the new process, or -1 on error.
 */
pid_t fork(void);

/**
 * @brief Replaces the current process image with a new process image.
 * 
 * @param filename The name of the file to execute.
 * @param argv Array of pointers to strings that are the arguments to the new process image.
 * @param envp Array of pointers to strings that are the environment variables for the new process image.
 * @return -1 on error.
 */
int execve(const char *filename, char *const argv[], char *const envp[]);

/**
 * @brief Terminates the calling process without flushing stdio buffers or calling registered exit handlers.
 * 
 * @param status The exit status of the process.
 */     
void _exit(int status) __attribute__((noreturn));

/**
 * @brief Get the Process Group ID (PGID) of a process.
 * @param pid The process ID.
 * @return The process group ID of the process, or -1 on error.
 */
pid_t getpgid(pid_t);

/**
 * @brief Set the Process Group ID (PGID) of a process.
 * 
 * @param pid The process ID.
 * @param pgid The process group ID to set.
 * @return The process group ID of the process, or -1 on error.
 */
int setpgid(pid_t, pid_t);

/**
 * @brief Get the process group ID of the current process.
 * @return The process group ID of the current process, or -1 on error.
 */
pid_t getpgrp(void);

/**
 * @brief Create a new process group.
 * @return The process group ID of the new process group, or -1 on error.
 */
pid_t setpgrp(void);

/**
 * @brief Get the Session ID (SID) of a process.
 * 
 * @param pid The process ID.
 * @return The session ID of the process, or -1 on error.
 */
pid_t getsid(pid_t);

/**
 * @brief Create a new session.
 * 
 * @return The session ID of the new session, or -1 on error.
 */
pid_t setsid(void);

/**
 * @brief Truncate a file to a specified length.
 * 
 * @param fd The file descriptor to truncate.
 * @param length The new length of the file.
 * @return 0 on success, -1 on error.
 */
int ftruncate(int fd, off_t length);

/**
 * @brief Truncate a file to a specified length.
 * 
 * @param filename The name of the file to truncate.
 * @param length The new length of the file.
 * @return 0 on success, -1 on error.
 */
int truncate(const char* filename, off_t length);

/**
 * @brief Get the controlling terminal process group ID (PGID) of a terminal.
 * 
 * @param fd The file descriptor of the terminal.
 * @return The process group ID of the controlling terminal, or -1 on error.
 */
pid_t tcgetpgrp(int fd);

/**
 * @brief Set the controlling terminal process group ID (PGID) of a terminal.
 * 
 * @param fd The file descriptor of the terminal.
 * @param pgrp The process group ID to set.
 * @return 0 on success, -1 on error.
 */
int tcsetpgrp(int fd, pid_t pgrp);

/**
 * @brief Duplicate a file descriptor.
 * 
 * @param oldfd The file descriptor to duplicate.
 * @return The new file descriptor, or -1 on error.
 */
int dup(int oldfd);

/**
 * @brief Duplicate a file descriptor.
 * 
 * @param oldfd The file descriptor to duplicate.
 * @param newfd The new file descriptor to use.
 * @return The new file descriptor, or -1 on error.
 */
int dup2(int oldfd, int newfd);

/**
 * @brief Create a pipe.
 * 
 * @param pipefd Array of two integers to store the file descriptors of the pipe.
 * @return 0 on success, -1 on error.
 */
int pipe(int pipefd[2]);

/**
 * @brief Create a pipe.
 * 
 * @param pipefd Array of two integers to store the file descriptors of the pipe.
 * @param flags Flags to set on the pipe.
 * @return 0 on success, -1 on error.
 */
int pipe2(int pipefd[2], int flags);

/**
 * @brief Change the current working directory.
 * 
 * @param path The path to the new current working directory.
 * @return 0 on success, -1 on error.
 */
int chdir(const char *path);

/**
 * @brief Get the current working directory.
 * 
 * @param buf Buffer to store the current working directory.
 * @param size Size of the buffer.
 * @return Pointer to the current working directory, or NULL on error.
 */
char* getcwd(char *buf, size_t size);

/**
 * @brief Check if a file descriptor is a terminal.
 * 
 * @param fd The file descriptor to check.
 * @return 1 if the file descriptor is a terminal, 0 otherwise.
 */
int isatty(int fd);

/**
 * @brief Check if the calling process has the specified access mode to the file.
 * 
 * @param path Path to the file.
 * @param mode Access mode to check (F_OK, X_OK, W_OK, R_OK).
 * @return 0 on success, -1 on error.
 */
int access(const char *path, int mode);

/* man 0p unistd.h
unsigned     alarm(unsigned);
int          chown(const char *, uid_t, gid_t);
size_t       confstr(int, char *, size_t);
char        *crypt(const char *, const char *);
void         encrypt(char [64], int);
int          execl(const char *, const char *, ...);
int          execle(const char *, const char *, ...);
int          execlp(const char *, const char *, ...);
int          execv(const char *, char *const []);
int          execvp(const char *, char *const []);
int          faccessat(int, const char *, int, int);
int          fchdir(int);
int          fchown(int, uid_t, gid_t);
int          fchownat(int, const char *, uid_t, gid_t, int);
int          fdatasync(int);
int          fexecve(int, char *const [], char *const []);
long         fpathconf(int, int);
int          fsync(int);
gid_t        getegid(void);
uid_t        geteuid(void);
gid_t        getgid(void);
int          getgroups(int, gid_t []);
long         gethostid(void);
int          gethostname(char *, size_t);
char        *getlogin(void);
int          getlogin_r(char *, size_t);
int          getopt(int, char * const [], const char *);
pid_t        getppid(void);
uid_t        getuid(void);
int          lchown(const char *, uid_t, gid_t);
int          link(const char *, const char *);
int          linkat(int, const char *, int, const char *, int);
int          lockf(int, int, off_t);
int          nice(int);
long         pathconf(const char *, int);
int          pause(void);
ssize_t      pread(int, void *, size_t, off_t);
ssize_t      pwrite(int, const void *, size_t, off_t);
ssize_t      readlink(const char *restrict, char *restrict, size_t);
ssize_t      readlinkat(int, const char *restrict, char *restrict, size_t);
int          rmdir(const char *);
int          setegid(gid_t);
int          seteuid(uid_t);
int          setgid(gid_t);
int          setregid(gid_t, gid_t);
int          setreuid(uid_t, uid_t);
int          setuid(uid_t);
unsigned     sleep(unsigned);
void         swab(const void *restrict, void *restrict, ssize_t);
int          symlink(const char *, const char *);
int          symlinkat(const char *, int, const char *);
void         sync(void);
long         sysconf(int);
char        *ttyname(int);
int          ttyname_r(int, char *, size_t);
int          unlink(const char *);
int          unlinkat(int, const char *, int);
*/

#endif /* _CLIB_UNISTD_H */
