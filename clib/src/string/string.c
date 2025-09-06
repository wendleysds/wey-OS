#include <string.h>

int strlen(const char *s){
	const char *p;
	for (p = s; *p; ++p);
	return (p - s);
}