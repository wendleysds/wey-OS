#ifndef _RESULT_STATUS_H
#define _RESULT_STATUS_H

#define SUCCESS 0
#define FAILED 1
#define INVALID_VARG -1
#define NO_MEMORY -2

// generic alias

#define OK SUCCESS
#define ERROR FAILED

const char* status_to_strint(int status);

#endif
