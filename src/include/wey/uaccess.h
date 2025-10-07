#ifndef _USER_ACCESS_H
#define _USER_ACCESS_H

#include <def/compile.h>
#include <stdint.h>

int __must_check copy_from_user(void* kdst, const void* usrc, uint64_t size);
int __must_check copy_to_user(const void* ksrc, void* udst, uint64_t size);
int __must_check copy_string_from_user(char* kdst, const char* usrc, int len);
int __must_check copy_string_to_user(const char* ksrc, char* udst, int len);

#endif