#ifndef _RESULT_STATUS_H
#define _RESULT_STATUS_H

// Generic
#define OK               0
#define SUCCESS          0
#define FAILED           1
#define ERROR           -1

// Arguments
#define INVALID_ARG     -10
#define NULL_PTR        -11
#define OUT_OF_BOUNDS   -12
#define BAD_ALIGNMENT   -13

// Memory
#define NO_MEMORY       -20
#define HEAP_CORRUPTED  -21
#define PAGE_FAULT      -22
#define STACK_OVERFLOW  -23
#define OUT_OF_VMEM     -24

// I/O
#define ERROR_IO        -30
#define DISK_NOT_READY  -31
#define READ_FAIL       -32
#define WRITE_FAIL      -33
#define FILE_NOT_FOUND  -34
#define FILE_EXISTS     -35
#define FS_CORRUPTED    -36
#define DIR_NOT_FOUND   -37
#define INVALID_FS      -38

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

// Mult-task/Processes
#define TASK_NOT_FOUND  -60
#define TASK_BLOCKED    -61
#define TASK_DEAD       -62
#define INVALID_PID     -63

#endif
