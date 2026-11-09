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

### Architecture & Hardware Discovery
- **Bootloader**: Custom two-stage BIOS bootloader loads the kernel from a FAT-formatted partition without external bootloaders.
- **CPU & Architecture**: Protected mode setup, GDT, IDT, PIC, CPU feature detection (CPUID), and interrupt handling.
- **Exception Tables (`extable`)**: Kernel exception fixup tables with dedicated fault handlers for safe user-space memory access.
- **APIC & HPET**: Local APIC (LAPIC) support, I/O APIC (IOAPIC) driver, and APIC timer calibration utilizing the High Precision Event Timer (HPET).
- **ACPI Subsystem**:
  - Early ACPI detection and table walking.
  - Table parsing for RSDT, MADT, and FADT.
  - DSDT support with an embedded static AML namespace parser.
  - ACPI hardware power control (reboot and shutdown).

### Device Model & Drivers
- **Bus & Driver Abstraction**: Unified Linux-inspired `bus` and `driver` subsystem.
- **PCI Subsystem**: PCI bus enumeration and PCI-to-PCI bridge discovery.
- **Serial (UART)**: Standard 16550A UART serial driver.
- **Input Subsystem**: Generic input event layer and PS/2 keyboard driver with scan code translation.
- **TTY & Terminals**: TTY core, virtual terminal (VT) integration, line discipline (`ldisc`) write handling, and initial `termios` support.
- **Storage**: ATA/IDE PIO driver with device probing, request queues, BIOs, partition scanning, and interrupt handling.

### Memory Management
- **Physical Memory Management**: E820 memory map discovery, `memblock` early allocator, page frame tracking (`struct page`), and buddy page allocator.
- **Virtual Memory & MMU**: Kernel paging, per-process page directories, page tables, and **Huge Pages (4 MiB PSE)** support.
- **Kernel Heap & Slabs**: Slab allocator with caches and `kmalloc`/`kzalloc` memory allocation APIs.
- **Virtual Memory Areas (VMAs)**: Per-task user memory tracking, anonymous mmap handling, and memory layout configuration.

### Virtual Filesystem (VFS) & Storage
- **VFS Core**: Inodes, open file descriptions, superblocks, and filesystem type registration.
- **Directory Cache (`dcache`)**: High-performance dentry cache for path lookups.
- **Path Resolution**: Hierarchical path walking with full support for relative paths and working directory manipulation (`chdir`, `getcwd`).
- **File & Directory Operations**: Syscall and VFS implementations for `open`, `close`, `read`, `write`, `lseek`, `mkdir`, and `rmdir`.
- **Filesystems**: Root `ramfs` in-memory filesystem, embedded CPIO initramfs extraction, and ongoing FAT/VFAT filesystem driver integration.

### Process Management, Scheduler & Signals
- **Tasks & Scheduling**: Preemptive scheduler, kernel and user tasks, idle task, sleep queues (`sleep_on`, `sleep_interruptible`), and context switching.
- **Process Creation & Execution**: `fork` task cloning, PID allocation, and `execve` supporting ELF binaries with full `argv` and `envp` parameter passing.
- **Signal Handling**: POSIX signal support including `sigaction`, `sigreturn`, `kill`, pending signal checking, and signal state inheritance across `fork` and `execve`.
- **Process Termination**: `exit` and `waitpid` handling with child reparenting.

### Clocks & Timekeeping
- **Clocks**: Monotonic and real-time clock infrastructure with `clock_gettime`, `clock_getres`, and `clock_settime` system calls.

### User-space C Library (`clib`) & Userspace
- **First-party Libc**: User-space C standard library under `clib/` offering standard headers, startup runtime (`crt0`), standard I/O, string operations, and syscall wrappers.
- **Math Routines**: Efficient 64-bit integer division and multiplication implemented via Donald Knuth's multi-precision algorithm.
- **Real-world Applications**: Fully capable of compiling and running complex user programs, highlighted by running unmodified **Lua 5.5.1** in Ring 3 as `/init`.

### Build System & Diagnostics
- **Symbol Generation**: Multi-pass kernel symbol generation (`gen_kallsyms.py`) to embed symbol tables for backtraces and debugging.
- **KBuild**: Modular, recursive Makefile build system with initcall-based subsystem initialization.

## Highlights

- **Unmodified Lua 5.5.1 in Ring 3**: Successfully compiles and executes vanilla Lua 5.5.1 without code changes, linked against WeyOS's custom user-space C library (`clib`) and running in Ring 3 user mode as the `/init` process.
- **Modern Hardware & ACPI Platform Support**: ACPI table discovery (RSDT, MADT, FADT, DSDT with a static AML namespace parser), ACPI reboot/shutdown, LAPIC, IOAPIC, and HPET calibration for APIC timers.
- **POSIX Signals & Process Execution**: Full `execve` support with `argv` and `envp` parameter passing, `fork`, `waitpid`, and complete signal delivery infrastructure (`sigaction`, `sigreturn`, `kill`).
- **Device Model & PCI Bus**: Linux-inspired `bus` and `driver` abstractions, PCI enumeration with PCI-to-PCI bridge discovery, and 16550A UART serial driver.
- **TTY & Virtual Terminal Subsystem**: Complete terminal pipeline featuring line discipline (`ldisc`), keyboard event handling, initial `termios` control, and VT integration.
- **Advanced VFS & Path Resolution**: Directory caching (`dcache`), relative path resolution, directory operations (`mkdir`, `rmdir`), and standard POSIX file I/O (`open`, `close`, `read`, `write`, `lseek`).
- **Memory Management & Huge Pages**: 4 MiB Huge Pages (PSE), slab allocator, buddy allocator, and per-process virtual address spaces.
- **Exception Tables (`extable`)**: Linux-style exception table handlers for safe, fault-tolerant memory access across the kernel/user boundary.
- **Custom User-space C Library (`clib`)**: Standalone C standard library with syscall wrappers, startup runtime, and Knuth 64-bit arithmetic routines.

## Repository Layout

```text
.
├── Makefile                 # Top-level kernel build
├── README.md
├── build/                   # Generated objects, binaries and images
├── clib/                    # Standalone user-space C standard library
├── scripts/                 # Build helpers, kallsyms generator and disk scripts
└── src/
    ├── arch/i386/           # x86-specific boot, entry, MMU, APIC and exceptions
    ├── block/               # Block-device core, request queues and partitions
    ├── core/                # Sched, tasks, signals, clock, IRQ subsystem
    ├── drivers/             # ACPI, PCI, serial, input, TTY, VT, ATA, video
    ├── fs/                  # VFS, dcache, path walk, ramfs, execve handlers
    ├── include/             # Kernel and UAPI headers
    ├── init/                # Kernel entry, initcalls and init process launcher
    ├── lib/                 # Kernel utility and string library
    ├── memory/              # PMM, VMM, huge pages, heap, slab and memblock
    └── usr/                 # Embedded initramfs payload
```

## Design Principles

- Keep subsystems small and readable.
- Avoid hidden magic.
- Prefer explicit initialization.
- Build components from scratch whenever practical.
- Favor simplicity over feature count.

## Build Requirements

Required tools:

- `make`
- `nasm`
- `gcc`
- `ld`
- `objcopy`
- `nm`
- `as`
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
4. Architecture setup initializes descriptor tables (GDT, IDT), exception handlers,
   interrupt routing (PIC, LAPIC, IOAPIC), ACPI table discovery, and paging.
5. Core kernel modules initialize physical/virtual memory allocators, device buses
   (PCI, platform drivers), peripherals (TTY, UART 16550A, ATA, HPET), scheduler,
   and registered initcalls.
6. `ramfs` is mounted as `/`, the embedded initramfs archive is unpacked, `/tty` is
   initialized, and the kernel switches to Ring 3 to launch the user-space init
   process (trying `/init`, `/bin/init`, or `/bin/sh`).

## System Calls

WeyOS provides a growing POSIX-like system call interface exposed via `int 0x80`:

| Syscall | Number | Description |
|---|:---:|---|
| `sys_exit` | 1 | Terminates the calling process with status code |
| `sys_read` | 2 | Reads bytes from an open file descriptor |
| `sys_write` | 3 | Writes bytes to an open file descriptor |
| `sys_open` | 4 | Opens or creates a file with pathname and flags |
| `sys_close` | 5 | Closes an open file descriptor |
| `sys_lseek` | 6 | Repositions read/write file offset |
| `sys_execve` | 7 | Executes an ELF binary with `argv` and `envp` |
| `sys_fork` | 8 | Creates a child process by duplicating the calling process |
| `sys_waitpid` | 9 | Waits for child process state changes or termination |
| `sys_getpid` | 10 | Returns the process ID of the calling process |
| `sys_chdir` | 15 | Changes the current working directory |
| `sys_getcwd` | 16 | Gets the current working directory pathname |
| `sys_mkdir` | 18 | Creates a new directory |
| `sys_rmdir` | 19 | Removes an empty directory |
| `sys_clock_gettime` | 27 | Retrieves the time of the specified clock |
| `sys_clock_getres` | 28 | Retrieves the resolution of the specified clock |
| `sys_clock_settime` | 29 | Sets the time of the specified clock |
| `sys_sigaction` | 33 | Examines and changes a signal action |
| `sys_sigreturn` | 34 | Returns from a signal handler and restores task context |
| `sys_kill` | 35 | Sends a signal to a process |
| `sys_tmp_vt_write` | 100 | Diagnostic virtual terminal write |

## In Progress

- Completing dynamic memory allocation syscalls for user space (`brk`, `mmap`, `munmap`).
- Implementing directory listing syscalls (`getdents`) and filesystem unlinking/renaming.
- Hardening user/kernel memory isolation and access validations.
- Wiring FAT/VFAT fully into the default filesystem mount path.
- Adding pipe (`pipe`) and descriptor duplicating (`dup`, `dup2`) support.
- Expanding user-space utilities and the standard C library (`clib`).

## Known Limitations

- The kernel is experimental and designed for educational and research purposes.
- User/kernel memory isolation is actively improving, but page-level protections and fault handling continue to evolve.
- The scheduler is a basic cooperative/preemptive model intended for experimentation.
- Filesystem and block-device code are under active development and not yet battle-tested.
- There is no stable ABI or backward compatibility guarantee.

## License

This project is open source and you are free to use, modify and distribute it.
If you use it as a base, credit is appreciated.

## Author

Developed by Wendley Santos.
