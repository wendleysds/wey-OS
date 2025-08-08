#ifndef _RESULT_STATUS_H
#define _RESULT_STATUS_H

// Status codes for various operations in the system

// Generic
#define OK               0
#define SUCCESS          0
#define FAILED          -1
#define ERROR           -2

// Arguments
#define INVALID_ARG     -10
#define NULL_PTR        -11
#define OUT_OF_BOUNDS   -12
#define BAD_ALIGNMENT   -13
#define INVALID_FORMAT  -14
#define NOT_EXISTS      -15
#define NOT_FOUND       -16

// Memory
#define NO_MEMORY       -20
#define HEAP_CORRUPTED  -21
#define PAGE_FAULT      -22
#define STACK_OVERFLOW  -23
#define OUT_OF_VMEM     -24
#define ALREADY_MAPD    -25
#define ALREADY_UMAPD   -26
#define OVERFLOW        -27

// I/O
#define ERROR_IO        -30
#define DISK_NOT_READY  -31
#define READ_FAIL       -32
#define WRITE_FAIL      -33
#define FS_CORRUPTED    -34
#define INVALID_FS      -35

// Time Or State
#define TIMEOUT         -40
#define BUSY            -41
#define NOT_READY       -42
#define OP_ABORTED      -43

// Acess/Security
#define ACCESS_DENIED   -50
#define NOT_IMPLEMENTED -51
#define NOT_SUPPORTED   -52
#define INVALID_STATE   -53
#define INVALID_PTR     -54

// Mult-task/Processes
#define TASK_NOT_FOUND  -60
#define TASK_BLOCKED    -61
#define TASK_DEAD       -62
#define INVALID_PID     -63
#define NO_TASKS        -64

// File System
#define FILE_NOT_FOUND  -70
#define FILE_EXISTS     -71
#define END_OF_FILE     -72
#define DIR_NOT_EMPTY   -73
#define INVALID_FILE    -74
#define FILE_TOO_LARGE  -75
#define INVALID_NAME    -76

#endif
