#include <wey/atomic.h>

int atomic_read(const atomic_t* v){
	return arch_atomic_read(v);
}

void atomic_set(atomic_t* v, int val){
	arch_atomic_set(v, val);
}

int atomic_fetch_add(atomic_t* v, int inc){
	return arch_atomic_fetch_add(v, inc);
}

int atomic_cmpxchg(atomic_t* v, int expected, int new){
	return arch_atomic_cmpxchg(v, expected, new);
}

void atomic_add(int i, atomic_t *v){
	arch_atomic_add(i, v);
}

void atomic_sub(int i, atomic_t *v){
	arch_atomic_sub(i, v);
}

void atomic_inc(atomic_t *v){
	arch_atomic_inc(v);
}

void atomic_dec(atomic_t *v){
	arch_atomic_dec(v);
}

int atomic_dec_and_test(atomic_t *v){
	return arch_atomic_dec_and_test(v);
}
