#ifndef _FILE_H
#define _FILE_H

#include <def/config.h>

struct FileDescriptor {};
struct Stat {};

struct Path {
	char* pathTokens[PATH_MAX];
};

void path_parse(struct Path* pathbuf, const char* path);

#endif

