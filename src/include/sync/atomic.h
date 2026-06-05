#ifndef _ATOMIC_H
#define _ATOMIC_H

#include <asm/atomic.h>

int atomic_read(const atomic_t* v);
void atomic_set(atomic_t* v, int val);
int atomic_fetch_add(atomic_t* v, int inc);
int atomic_cmpxchg(atomic_t* v, int expected, int new);
void atomic_add(int i, atomic_t *v);
void atomic_sub(int i, atomic_t *v);
void atomic_inc(atomic_t *v);
void atomic_dec(atomic_t *v);
int atomic_dec_and_test(atomic_t *v);

#endif
