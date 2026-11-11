#ifndef _UAPI_SYS_MMAN_H
#define _UAPI_SYS_MMAN_H

#define PROT_NONE   0x0
#define PROT_READ   0x1
#define PROT_WRITE  0x2
#define PROT_EXEC   0x4
#define PROT_VALID  (PROT_READ | PROT_WRITE | PROT_EXEC)

#define MAP_SHARED    0x01
#define MAP_PRIVATE   0x02
#define MAP_FIXED     0x04
#define MAP_ANONYMOUS 0x08
#define MAP_POPULATE  0x10
#define MAP_ANON      MAP_ANONYMOUS
#define MAP_VALID     (MAP_SHARED | MAP_PRIVATE | MAP_FIXED | MAP_ANONYMOUS | MAP_POPULATE)

// alias
#define MAP_ANON MAP_ANONYMOUS

#define MAP_FAILED ((void *)-1)

#endif /* _UAPI_SYS_MMAN_H */