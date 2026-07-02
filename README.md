# WeyOS

WeyOS is a small educational operating system kernel for the x86 32-bit
architecture. The project is written in C and Assembly and is built from
scratch to explore how a kernel boots, initializes hardware, manages memory,
loads programs and exposes basic operating-system services.

The codebase is experimental and intentionally low-level. It is a study kernel,
not a production system.

## Goals

- Understand and document the x86 boot path from real mode to protected mode.
- Keep a custom boot flow capable of loading the kernel from a FAT-based disk
  image.
- Build kernel subsystems directly, without depending on a hosted runtime.
- Develop a usable memory-management stack with paging, physical page
  allocation, kernel heap allocation, slab caches and per-task address spaces.
- Provide a small process model with tasks, PIDs, context switching, `fork`,
  `waitpid`, `getpid` and `exit`.
- Grow a syscall interface that can support real user-space programs.
- Support executable loading through binary-format handlers, starting with ELF
  and script execution.
- Maintain a VFS layer with mount points, path walking, inodes, files,
  superblocks and basic filesystem operations.
- Use `ramfs` and embedded CPIO initramfs as the first root filesystem path.
- Integrate persistent storage through block devices, ATA/IDE PIO and partition
  scanning.
- Complete FAT/VFAT support so the kernel can mount and use FAT filesystems,
  not only build FAT boot images.
- Provide basic interactive I/O through VGA text output, terminal/VT support
  and keyboard input.
- Keep a small user-space C library for startup code, syscall wrappers and
  simple programs.
- Keep the project small enough to be readable while still resembling real
  kernel architecture.

## Implemented Features

- A custom two-stage BIOS bootloader loads the kernel from a FAT filesystem without
  relying on GRUB or other third-party bootloaders.
- Protected mode setup, GDT, IDT, PIC, exception handlers and interrupt entry.
- Early architecture setup with E820 memory discovery and paging.
- Kernel virtual memory support with paging helpers and page tables.
- Physical memory management with `memblock`, `struct page` metadata and page
  allocation.
- Kernel heap allocators, including page allocation, slab allocation and
  `kmalloc`/`kzalloc` style APIs.
- Virtual memory areas and per-task MMU context switching for loaded programs.
- Basic scheduler with kernel tasks, context switching, idle task and timer
  driven reschedule requests.
- PID management, `fork`, `waitpid`, `getpid` and `exit` system-call support.
- Initcall levels inspired by Linux-style subsystem initialization.
- Virtual filesystem layer with inodes, files, superblocks, mount points and
  path walking.
- Root `ramfs` mounted at `/`.
- Initramfs support using embedded CPIO archives.
- Executable loading through binary-format handlers.
- ELF executable loader and script format loader.
- Basic syscall entry path for i386.
- Character-device registration layer.
- Block-device layer with request queues, BIOs, simple elevator and partition
  scanning.
- ATA/IDE PIO driver with device probing, read/write requests and IRQ support.
- VGA text output, terminal/VT support and keyboard driver.
- Kernel logging, panic/assert helpers and internal C utility library.
- Small user-space C library under `clib/` with startup code and syscall
  wrappers.
- FAT/VFAT filesystem implementation and FAT image tooling are present in the
  tree, with integration still in progress.

## Highlights

- Custom BIOS bootloader
- ELF executable loader
- Linux-inspired VFS architecture
- Linux-inspired KBuild
- Slab allocator
- Per-process virtual address spaces
- Initcall-based subsystem initialization
- ATA PIO driver with interrupt support
- Embedded initramfs support

## Repository Layout

```text
.
├── Makefile                 # Top-level kernel build
├── README.md
├── build/                   # Generated objects, binaries and images
├── clib/                    # Small user-space C library
├── scripts/                 # Build helpers and image generation scripts
├── tools/
|   ├── boot/embed/          # Bootloader/image installer tooling
|   └── fs/fat/              # FAT image manipulation tools
└── src/
    ├── arch/i386/           # x86-specific boot, entry, MMU and kernel code
    ├── block/               # Block-device core, queues and partitions
    ├── core/                # Kernel init, scheduler, syscalls, fork, clock
    ├── drivers/             # VGA, terminal, ATA, device core
    ├── fs/                  # VFS, ramfs, exec and filesystem code
    ├── include/             # Kernel headers
    ├── lib/                 # Kernel utility library
    ├── memory/              # PMM, VMM, heap, slab and memblock
    └── usr/                 # Embedded initramfs object
```

## Design Principles

- Keep subsystems small and readable.
- Avoid hidden magic.
- Prefer explicit initialization.
- Build components from scratch whenever practical.
- Favor simplicity over feature count.

## Build Requirements

The default toolchain prefix is `i686-elf-`.

Required tools:

- `make`
- `nasm`
- `i686-elf-gcc`
- `i686-elf-ld`
- `i686-elf-objcopy`
- `i686-elf-nm`
- standard Unix shell utilities used by the build scripts (`sh`, `sed`, `dd`,
  `cat`, `find`)
- `python3` for the image tooling

Optional:

- `qemu-system-i386` or another x86 emulator for manual testing.

You can override the cross-compiler prefix:

```sh
make CROSS_COMPILE=i686-elf-
```

## Building

Build the kernel:

```sh
make
```

The main generated files are:

- `build/bin/kernel.elf`
- `build/bin/kernel.bin`
- `build/bzImage`

Build with debug information:

```sh
make DEBUG_KERNEL=1
```

Embed an initramfs archive:

```sh
make INITRAM=path/to/initramfs.cpio
```

Clean generated files:

```sh
make clean
```

## Disk Image

After building `build/bzImage`, a bootable FAT-based hard-disk image can be
generated with:

```sh
sh scripts/genhdimage.sh build/bzImage build/hdimage
```

The script creates a 32 MiB image, installs the embedded bootloader, copies the
kernel to `/boot/kernel` and writes a minimal `/boot/boot.cfg`.

There is no maintained top-level `make run` target at the moment. Run the image
with your emulator of choice, for example QEMU, after generating `build/hdimage`.

## Boot Flow

1. The boot/setup code starts in real mode.
2. BIOS helpers collect early platform information such as memory and video
   data.
3. The kernel switches to protected mode and enters the 32-bit startup path.
4. Architecture setup initializes descriptor tables, interrupts, paging and
   memory discovery.
5. Core kernel modules initialize memory, terminal, scheduler and registered
   initcalls.
6. `ramfs` is mounted as `/`, the optional initramfs is unpacked, and the kernel
   attempts to execute `/bash`.

## System Calls

The syscall table currently enables:

- `exit`
- `fork`
- `waitpid`
- `getpid`
- temporary terminal write syscall used by the current user-space library

Several common syscall slots are already reserved in the table but are not
enabled yet, including `read`, `write`, `open`, `close`, `execve`, `brk`,
`mmap`, directory operations and reboot.

## In Progress

- Completing and hardening the user/kernel boundary and user-mode execution
  model.
- Expanding syscall coverage for normal file and process operations.
- Wiring FAT/VFAT fully into the default filesystem build and mount path.
- Hardening the VFS and block layers.
- Improving process address spaces and memory isolation.
- Adding a maintained emulator run target.
- Growing user-space programs and the small C library.

## Known Limitations

- The kernel is experimental and all core code still runs with high privilege.
- Ring 3 execution exists, but the user/kernel boundary is not complete yet.
- Process address spaces exist, but isolation and validation are still limited.
- The scheduler is basic and intended for experimentation.
- Many drivers are synchronous or minimal.
- Filesystem and block-device code are young and should be treated as unsafe.
- FAT/VFAT code exists, but it is not fully integrated into the default build.
- The syscall surface is intentionally incomplete.
- There is no stable ABI or API compatibility guarantee.

## License

This project is open source and you are free to use, modify and distribute it.
If you use it as a base, credit is appreciated.

## Author

Developed by Wendley Santos.
