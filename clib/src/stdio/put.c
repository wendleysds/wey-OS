#include <stdio.h>
#include <string.h>
#include <syscall.h>

long syscall(long no, long arg1, long agr2, long arg3, long arg4);

int putchar(int c){
	char buffer[] = { c, '\0'};
	return syscall(SYS_write, (long)buffer, sizeof(buffer), 0, 0);
}

int puts(const char* str){
	return syscall(SYS_write, (long)str, strlen(str), 0, 0);
}
