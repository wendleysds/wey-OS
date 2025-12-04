#ifndef _LOADER_H
#define _LOADER_H

int weyos_loader(struct file* file, void** out_entry_point, void* initrd_addr);

#endif