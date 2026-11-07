#ifndef _TYPES_H
#define _TYPES_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	volatile int value;
} atomic_t;

// Memory
typedef uint64_t paddr_t;
typedef uintptr_t vaddr_t;
typedef unsigned long pfn_t;

// CPU
typedef uint32_t cpu_id_t;
typedef unsigned long reg_t;

// Devices
typedef uint64_t sector_t;
typedef uint32_t dev_t;

// Process
typedef int32_t pid_t;
typedef int32_t tid_t;
typedef uint32_t uid_t;
typedef uint32_t gid_t;

typedef uint32_t sigset_t;

// Files
typedef uint16_t umode_t;
typedef uint16_t mode_t ;
typedef signed long off_t;

// Time
typedef int64_t time_t;
typedef uint64_t time_ns_t;
typedef uint64_t time_us_t;
typedef uint64_t time_ms_t;
typedef uint64_t timespan; // in seconds
typedef uint64_t tick_t;
typedef uint32_t clockid_t;

// Misc
typedef uintptr_t flags_t;
typedef intptr_t ssize_t;


#ifdef __cplusplus
}
#endif

#endif
