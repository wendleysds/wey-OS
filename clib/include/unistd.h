#ifndef _CLIB_UNISTD_H
#define _CLIB_UNISTD_H

#include <sys/types.h>

#define STDIN_FILENO  (0)
#define STDOUT_FILENO (1)
#define STDERR_FILENO (2)

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


/* man 0p unistd.h
int          access(const char *, int);
unsigned     alarm(unsigned);
int          chdir(const char *);
int          chown(const char *, uid_t, gid_t);
size_t       confstr(int, char *, size_t);
char        *crypt(const char *, const char *);
int          dup(int);
int          dup2(int, int);
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
int          ftruncate(int, off_t);
char        *getcwd(char *, size_t);
gid_t        getegid(void);
uid_t        geteuid(void);
gid_t        getgid(void);
int          getgroups(int, gid_t []);
long         gethostid(void);
int          gethostname(char *, size_t);
char        *getlogin(void);
int          getlogin_r(char *, size_t);
int          getopt(int, char * const [], const char *);
pid_t        getpgid(pid_t);
pid_t        getpgrp(void);
pid_t        getppid(void);
pid_t        getsid(pid_t);
uid_t        getuid(void);
int          isatty(int);
int          lchown(const char *, uid_t, gid_t);
int          link(const char *, const char *);
int          linkat(int, const char *, int, const char *, int);
int          lockf(int, int, off_t);
int          nice(int);
long         pathconf(const char *, int);
int          pause(void);
int          pipe(int [2]);
ssize_t      pread(int, void *, size_t, off_t);
ssize_t      pwrite(int, const void *, size_t, off_t);
ssize_t      readlink(const char *restrict, char *restrict, size_t);
ssize_t      readlinkat(int, const char *restrict, char *restrict, size_t);
int          rmdir(const char *);
int          setegid(gid_t);
int          seteuid(uid_t);
int          setgid(gid_t);
int          setpgid(pid_t, pid_t);
pid_t        setpgrp(void);
int          setregid(gid_t, gid_t);
int          setreuid(uid_t, uid_t);
pid_t        setsid(void);
int          setuid(uid_t);
unsigned     sleep(unsigned);
void         swab(const void *restrict, void *restrict, ssize_t);
int          symlink(const char *, const char *);
int          symlinkat(const char *, int, const char *);
void         sync(void);
long         sysconf(int);
pid_t        tcgetpgrp(int);
int          tcsetpgrp(int, pid_t);
int          truncate(const char *, off_t);
char        *ttyname(int);
int          ttyname_r(int, char *, size_t);
int          unlink(const char *);
int          unlinkat(int, const char *, int);
*/

#endif /* _CLIB_UNISTD_H */
