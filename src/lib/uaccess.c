#include <wey/uaccess.h>
#include <wey/panic.h>
#include <wey/sched.h>
#include <wey/mmu.h>
#include <def/status.h>
#include <def/config.h>

int copy_from_user(void* kdst, const void* usrc, uint64_t size){
   return NOT_IMPLEMENTED;
}

int copy_to_user(const void* ksrc, void* udst, uint64_t size){
    return NOT_IMPLEMENTED;
}

int copy_string_from_user(char* kdst, const char* usrc, int len){
	int i;
	for (i = 0; i < len - 1; i++){
		char c = usrc[i];
		kdst[i] = c;

		if (c == '\0') {
			return SUCCESS;
		}
	}

	kdst[i] = '\0';
	return OVERFLOW;
}

int copy_string_to_user(const char* ksrc, char* sdst, int len){
    return NOT_IMPLEMENTED;
}