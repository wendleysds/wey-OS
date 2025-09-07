#include <stdio.h>
#include <string.h>
#include <syscall.h>

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
