#include <uaccess.h>
#include <core/kernel.h>
#include <core/sched.h>
#include <def/status.h>
#include <def/config.h>
#include <mmu.h>

int copy_from_user(void* kdst, const void* usrc, uint64_t size){
   return NOT_IMPLEMENTED;
}

int copy_to_user(const void* ksrc, void* udst, uint64_t size){
    return NOT_IMPLEMENTED;
}

int copy_string_from_user(char* kdst, const char* usrc, int len){
    return NOT_IMPLEMENTED;
}

int copy_string_to_user(const char* ksrc, char* sdst, int len){
    return NOT_IMPLEMENTED;
}