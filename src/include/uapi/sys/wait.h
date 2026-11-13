#ifndef _UAPI_SYS_WAIT_H
#define _UAPI_SYS_WAIT_H

#define WIFEXITED(s)    (((s) & 0x7F) == 0)
#define WEXITSTATUS(s)  (((s) >> 8) & 0xFF)
#define WIFSIGNALED(s)  ((((s) & 0x7F) != 0) && (((s) & 0x7F) != 0x7F))
#define WTERMSIG(s)     ((s) & 0x7F)
#define WIFSTOPPED(s)   (((s) & 0xFF) == 0x7F)
#define WSTOPSIG(s)     (((s) >> 8) & 0xFF)
#define WCOREDUMP(s)    (((s) & 0x80) != 0)

#define WNOHANG 0x1

#endif /*_UAPI_SYS_WAIT_H*/