#include <fs/binfmts.h>
#include <fs/fs.h>
#include <fs/file.h>
#include <memory/kheap.h>
#include <lib/string.h>
#include <def/config.h>
#include <def/status.h>

extern struct binfmt elf_format;
extern struct binfmt script_format;

static struct binfmt* fmts[] = {
    &elf_format, // ELF binary format
    &script_format, // Script binary format
    0x0
};

static struct binprm* alloc_binprm(int fd, const char* filename) {
    struct binprm* fmt = (struct binprm*)kmalloc(sizeof(struct binprm));
    if (!fmt) {
        return 0x0;
    }

    fmt->filename = filename;
    fmt->fd = fd;
    memset(&fmt->mem, 0, sizeof(fmt->mem));

    return fmt;
}

static int count(int max, const char* argv[]) {
    int count = 0;
    while (argv[count] && count < max) {
        count++;
    }
    return count;
}

static int _exec_binprm(struct binprm* bprm) {
    if (!bprm || !bprm->filename || bprm->fd < 0) {
        return INVALID_ARG; // Invalid binary parameters
    }

    struct binfmt* fmt = 0x0;
    for (int i = 0; fmts[i]; i++) {
        if (fmts[i]->load_binary) {
            fmt = fmts[i];
            break;
        }
    }

    if (!fmt) {
        return NOT_IMPLEMENTED; // No suitable binary format found
    }

    return NOT_IMPLEMENTED;
}

int exec(const char* pathname, const char* argv[], const char* envp[]) {
    if(!pathname || !argv || !envp || strlen(pathname) == 0 || strlen(pathname) > PATH_MAX) {
        return INVALID_ARG;
    }

    int fd = open(pathname, O_RDONLY);
    if (fd < 0) {
        return fd;
    }

    struct binprm* bprm = alloc_binprm(fd, pathname);
    if (!bprm) {
        close(fd);
        return NO_MEMORY; // Failed to allocate binprm
    }

    bprm->argc = count(PROC_ARG_MAX, argv);
    bprm->envc = count(PROC_ENV_MAX, envp);

    // TODO: Handle Args

    int8_t status = _exec_binprm(bprm);

    return status;
}
