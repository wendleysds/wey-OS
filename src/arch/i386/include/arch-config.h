#ifndef _X86_CONFIG_H
#define _X86_CONFIG_H

#define X86_32 1

/*GDT*/
#define TOTAL_GDT_SEGMENTS 6

#define GDT_NULL_INDEX        0 
#define GDT_KERNEL_CODE_INDEX 1
#define GDT_KERNEL_DATA_INDEX 2
#define GDT_USER_CODE_INDEX   3
#define GDT_USER_DATA_INDEX   4
#define GDT_TSS_BASE_INDEX    5

#define GDT_KERNEL_CODE  (GDT_KERNEL_CODE_INDEX << 3)
#define GDT_KERNEL_DATA  (GDT_KERNEL_DATA_INDEX << 3)
#define GDT_USER_CODE    ((GDT_USER_CODE_INDEX << 3) | 3)
#define GDT_USER_DATA    ((GDT_USER_DATA_INDEX << 3) | 3)
#define GDT_TSS(index)   ((index) << 3)

/* PIC */
#define TIMER_FREQUENCY 20

/* Memory */
#define PAGE_SIZE 0x1000
#define PAGE_SHIFT 12
#define MAX_ARCH_PFN (1 << (32 - PAGE_SHIFT))

#endif
