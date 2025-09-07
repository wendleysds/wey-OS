#include <stdio.h>
#include <string.h>
#include <syscall.h>

long syscall(long no, long arg1, long agr2, long arg3, long arg4);

int putchar(int c){
	char ch = (char)c;
    return syscall(SYS_write, (long)&ch, 1, 0, 0);
}

int puts(const char* str){
	int len = strlen(str);
	
	syscall(SYS_write, (long)str, len, 0, 0);
	putchar('\n');

	return len + 1;
}
