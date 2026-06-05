#ifndef _BARRIER_H
#define _BARRIER_H

void smp_mb(void);   // full barrier
void smp_rmb(void);  // read barrier
void smp_wmb(void);  // write barrier

#endif