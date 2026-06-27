global __initramfs_size

#ifdef CONFIG_INITRAM
section .init.ramfs

__start:
incbin CONFIG_INITRAM
__end:

section .init.ramfs.info
__initramfs_size: dd __end - __start

#else
section .init.ramfs.info
__initramfs_size: dd 0

#endif