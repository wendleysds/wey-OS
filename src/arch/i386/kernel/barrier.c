#include <asm/barrier.h>
#include <wey/barrier.h>

void smp_mb(void){
	arch_mb();
}
void smp_rmb(void){
	arch_rmb();
}
void smp_wmb(void){
	arch_wmb();
}
