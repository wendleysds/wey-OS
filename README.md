# Mini x86 Operating System

This project is a simple operating system developed from scratch for the x86 (32-bit) architecture.
It implements fundamental low-level concepts such as memory management, drivers, basic multitasking, etc.  

### Why?

For studies and also to understand how an operating system works "under the hood"

## Implemented Features
> **So far**

- Boot via bootloader in assembly
- Transition from Real Mode (16-bit) to Protected Mode (32-bit)
- GDT (Global Descriptor Table) setup
- IDT (Interrupt Descriptor Table) setup
- Handlers for hardware and software interrupts
- Paging (memory management via paging)
- Heap allocator (hmalloc, hcalloc, hfree)
- Basic video driver (text mode via VGA)
- Initial support for VESA (graphics mode)
- Keyboard driver
- Basic terminal interface
- Modular organization with support for future extensions

## Directory Structure

```
.
├── build/         # Compiled files (binaries, objects, etc.)
├── linker16.ld    # Linker script for 16-bit code (/src/boot)
├── linker32.ld    # Linker script for 32-bit code
├── makefile       # Build script
├── README.md      # Project documentation
└── src/           # Operating system source code
    ├── arch/      # Architecture-specific code
    ├── boot/      # Bootloader and initialization (16-bit code)
    ├── core/      # System core (scheduling, syscalls, etc.)
    ├── drivers/   # Device drivers (keyboard, video, etc.)
    ├── include/   # Header files (.h)
    ├── lib/       # Helper functions and internal libraries
    └── memory/    # Memory management (paging, heap, etc.)
```

## How to Build

Requirements:

- `nasm`
- `gcc` (cross-compiler recommended)
- `ld`
- `qemu` (for testing)

Build:

```bash
make all
```

Run via QEMU:

```bash
make run
```

Disassemble image for assembly debugging:

```bash
make disassembly-img
```

## In Development

- Multitasking via TSS + context switching
- Graphics mode support (via VESA)
- Basic file system

## Skills Demonstrated

- Low-level programming in C and Assembly
- Bare-metal kernel development
- Memory management using heap and paging
- Hardware structure setup (GDT, IDT, TSS)
- Creation of simple drivers
- Use of toolchains and emulators

## Known Limitations

### Memory Management
- Unsafe heap allocator
  - The heap lacks protections against memory overwrites (e.g., double free, overflow).
- No alignment checks
  - Some structures may be misaligned, causing access issues.
- No shrinking or reallocation
- No heap compaction
  - Fragmentation may occur, reducing long-term allocation efficiency.
- No per-process virtual memory
  - The entire system operates within a single global address space.

### Protection and Security
- No user mode (Ring 3)
  - All code runs at the highest privilege level (Ring 0), posing stability risks.
- No process isolation
  - A process (in the future) could access all system memory without restrictions.
- No pointer validation
  - Lack of checks on passed pointers may lead to memory corruption.

### I/O and Drivers
- Simple, synchronous drivers
  - All drivers are blocking and straightforward, without buffers or complex interrupts.
- No nested interrupt handling or priorities.

### Multitasking
- No multitasking or process support (yet)
- The system is single-tasked, executing only a continuous kernel loop.
- TSS partially implemented. No context switching yet.

### File System
- Non-existent

### Modularity and Extensibility
- No loadable modules: The kernel is monolithic and does not support dynamic loading of drivers or extensions.

### Graphics and Interface
- Experimental graphics support: VESA is under development, but the system still relies mainly on VGA text mode.

## Licence

This project is open-source and you are free to use, modify, and distribute it as you wish. 
If you use it as a base, I’d appreciate a credit :D

## Author

Developed by Wendley Santos.
Feel free to contact me or collaborate!
