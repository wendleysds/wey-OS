#include <stdlib.h>
#include <string.h>
#include <errno.h>

extern char **environ;

static char **__alloc_environ = NULL;
static size_t __environ_cap = 0;

static int __ensure_heap_environ(size_t needed_len) {
    if (environ != __alloc_environ) {
        size_t current_len = 0;
        while (environ[current_len] != NULL) {
            current_len++;
        }

        size_t new_cap = (current_len + needed_len + 16);
        char **new_env = malloc(sizeof(char *) * (new_cap + 1));
        if (!new_env) {
            return -1;
        }

        for (size_t i = 0; i < current_len; i++) {
            new_env[i] = environ[i];
        }
        new_env[current_len] = NULL;

        if (__alloc_environ != NULL) {
            free(__alloc_environ);
        }

        environ = __alloc_environ = new_env;
        __environ_cap = new_cap;
        return 0;
    }

    size_t current_len = 0;
    while (environ[current_len] != NULL) {
        current_len++;
    }

    if (current_len + needed_len > __environ_cap) {
        size_t new_cap = (__environ_cap == 0) ? 16 : (__environ_cap * 2);
        while (new_cap < current_len + needed_len) {
            new_cap *= 2;
        }

        char **new_env = realloc(__alloc_environ, sizeof(char *) * (new_cap + 1));
        if (!new_env) {
            return -1;
        }

        environ = __alloc_environ = new_env;
        __environ_cap = new_cap;
    }

    return 0;
}

int setenv(const char *name, const char *value, int overwrite) {
    if (!name || name[0] == '\0' || strchr(name, '=') != NULL || !value) {
        errno = EINVAL;
        return -1;
    }

    size_t name_len = strlen(name);
    size_t value_len = strlen(value);

    size_t i = 0;
    if (environ != NULL) {
        for (; environ[i] != NULL; i++) {
            if (strncmp(environ[i], name, name_len) == 0 && environ[i][name_len] == '=') {
                if (!overwrite) {
                    return 0;
                }
                break;
            }
        }
    }

    int is_new = (environ == NULL || environ[i] == NULL);
    if (__ensure_heap_environ(is_new ? 1 : 0) < 0) {
        return -1;
    }

    char *new_var = malloc(name_len + value_len + 2);
    if (!new_var) {
        return -1;
    }

    memcpy(new_var, name, name_len);
    new_var[name_len] = '=';
    memcpy(new_var + name_len + 1, value, value_len + 1); 
    environ[i] = new_var;
    if (is_new) {
        environ[i + 1] = NULL;
    }

    return 0;
}

int unsetenv(const char *name) {
    if (!name || name[0] == '\0' || strchr(name, '=') != NULL) {
        errno = EINVAL;
        return -1;
    }

    if (environ == NULL) {
        return 0;
    }

    size_t name_len = strlen(name);
    size_t i = 0;

    while (environ[i] != NULL) {
        if (strncmp(environ[i], name, name_len) == 0 && environ[i][name_len] == '=') {
            size_t j = i;
            while (environ[j] != NULL) {
                environ[j] = environ[j + 1];
                j++;
            }
        } else {
            i++;
        }
    }

    return 0;
}

char *getenv(const char *name) {
    if (!name || name[0] == '\0' || strchr(name, '=') != NULL || environ == NULL) {
        return NULL;
    }

    size_t name_len = strlen(name);

    for (size_t i = 0; environ[i] != NULL; i++) {
        if (strncmp(environ[i], name, name_len) == 0 && environ[i][name_len] == '=') {
            return environ[i] + name_len + 1;
        }
    }

    return NULL;
}

