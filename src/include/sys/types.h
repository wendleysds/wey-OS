#ifndef _TYPES_H
#define _TYPES_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	volatile int value;
} atomic_t;

typedef uint64_t paddr_t;
typedef uintptr_t vaddr_t;

/*1 sector = 1 LBA*/
typedef uint64_t sector_t;
typedef uint64_t pfn_t;

typedef uint32_t dev_t;
typedef int64_t off_t;

typedef int32_t pid_t;
typedef int32_t tid_t;
typedef uint32_t uid_t;
typedef uint32_t gid_t;

typedef uint64_t time_ns_t;
typedef uint64_t time_us_t;
typedef uint64_t time_ms_t;
typedef uint64_t tick_t;

typedef uint32_t cpu_id_t;

typedef uintptr_t flags_t;
typedef uintptr_t reg_t;

#ifdef __cplusplus
}
#endif

#endif
