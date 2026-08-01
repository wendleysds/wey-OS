#ifndef _UACCESS_H
#define _UACCESS_H

#include <stddef.h>

#define __user

int copy_to_user(void __user *dst, const void *src, size_t len);
int copy_from_user(void *dst, const void __user *src, size_t len);

#endif