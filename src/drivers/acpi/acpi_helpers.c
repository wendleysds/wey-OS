#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

bool acpi_checksum_ok(const void *addr, size_t len){
    u8 sum = 0;

    for (size_t i = 0; i < len; i++){
        sum += ((u8*)addr)[i];
    }

    return sum == 0;
}