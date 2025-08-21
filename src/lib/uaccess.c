#include <uaccess.h>
#include <core/kernel.h>
#include <core/sched.h>
#include <def/status.h>
#include <def/config.h>
#include <mmu.h>

int copy_from_user(void* kdst, const void* usrc, uint64_t size){
    if(!kdst || !usrc || size <= 0){
        return INVALID_ARG;
    }

    pcb_page_current();

    if (!mmu_user_pointer_valid_range(usrc, size)){
        kernel_page();
        return INVALID_PTR;
    }

    memcpy(kdst, usrc, size);

    kernel_page();
    return SUCCESS;
}

int copy_to_user(const void* ksrc, void* udst, uint64_t size){
    if(!ksrc || !udst || size <= 0){
        return INVALID_ARG;
    }

    pcb_page_current();

    if (!mmu_user_pointer_valid_range(udst, size)){
        kernel_page();
        return INVALID_PTR;
    }

    memcpy(udst, ksrc, size);

    kernel_page();
    return SUCCESS;
}

int copy_string_from_user(char* kdst, const char* usrc, int len){
    if (!kdst || !usrc || len <= 0){
        return INVALID_ARG;
    }

    pcb_page_current();

    if (!mmu_user_pointer_valid_range(usrc, len)){
        kernel_page();
        return INVALID_PTR;
    }

    int i;
    for (i = 0; i < len - 1; i++){
        char c = usrc[i];
        kdst[i] = c;

        if (c == '\0') {
            kernel_page();
            return SUCCESS;
        }
    }

    kernel_page();
    kdst[i] = '\0';
    return OVERFLOW;
}

int copy_string_to_user(const char* ksrc, char* sdst, int len){
    if (!ksrc || !sdst || len <= 0){
        return INVALID_ARG;
    }

    pcb_page_current();

    if (!mmu_user_pointer_valid_range(sdst, len)){
        sdst[0] = '\0';
        kernel_page();
        return INVALID_PTR;
    }

    int i;
    for (i = 0; i < len - 1; i++) {
        char c = ksrc[i];
        sdst[i] = c;

        if (c == '\0') {
            kernel_page();
            return SUCCESS;
        }
    }

    sdst[i] = '\0';
    kernel_page();
    return OVERFLOW;
}