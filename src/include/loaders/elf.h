#ifndef _EXECUTABLE_AND_LINKING_FORMAT_H
#define _EXECUTABLE_AND_LINKING_FORMAT_H

#include <stdint.h>

typedef uint32_t Elf32_Addr;  // Unsigned program address
typedef uint16_t Elf32_Half;  // Unsigned medium integer
typedef uint32_t Elf32_Off;   // Unsigned file offset
typedef int32_t  Elf32_Sword; // Signed large integer
typedef uint32_t Elf32_Word;  // Unsigned large integer

#define EI_NIDENT 16

// e_type

#define ET_NONE   0      // No file type
#define ET_REL    1      // Relocatable file
#define ET_EXEC   2      // Executable file
#define ET_DYN    3      // Shared object file
#define ET_CORE   4      // Core file
#define ET_LOPROC 0xff00 // Processor-specific
#define ET_HIPROC 0xffff // Processor-specific

// e_machine

#define EM_NONE  0 // No machine
#define EM_M32   1 // AT&T WE 32100
#define EM_SPARC 2 // SPARC
#define EM_386   3 // Intel 80386
#define EM_68K   4 // Motorola 68000
#define EM_88K   5 // Motorola 88000
#define EM_860   7 // Intel 80860
#define EM_MIPS  8 // MIPS RS3000

// e_version

#define EV_NONE    0 // Invalid version
#define EV_CURRENT 1 // Current version

// #ELF Identification

#define EI_MAG0    0  // File identification
#define EI_MAG1    1  // File identification
#define EI_MAG2    2  // File identification
#define EI_MAG3    3  // File identification
#define EI_CLASS   4  // File class
#define EI_DATA    5  // Data encoding
#define EI_VERSION 6  // File version
#define EI_PAD     7  // Start of padding bytes
#define EI_NIDENT  16 // Size of e_ident[]

#define ELFCLASSNONE 0 // Invalid class
#define ELFCLASS32   1 // 32-bit objects
#define ELFCLASS64   2 // 64-bit objects

#define ELFDATANONE 0 // Invalid data encoding
#define ELFDATA2LSB 1 // Litte-Endian
#define ELFDATA2MSB 2 // Big-Endian

#define ELF32_R_SYM(i) \
    ((i)>>8)

#define ELF32_R_TYPE(i) \
    ((unsigned char)(i))

#define ELF32_R_INFO(s,t) \
    (((s)<<8)+(unsigned char)(t))

struct Elf32_Ehdr {
    unsigned char e_ident[EI_NIDENT]; 
    Elf32_Half e_type;
    Elf32_Half e_machine;
    Elf32_Word e_version;
    Elf32_Addr e_entry;
    Elf32_Off e_phoff;
    Elf32_Off e_shoff;
    Elf32_Word e_flags;
    Elf32_Half e_ehsize;
    Elf32_Half e_phentsize;
    Elf32_Half e_phnum;
    Elf32_Half e_shentsize;
    Elf32_Half e_shnum;
    Elf32_Half e_shstrndx;
}__attribute__((packed));

struct elf32_shdr{
    Elf32_Word sh_name;
    Elf32_Word sh_type;
    Elf32_Word sh_flags;
    Elf32_Addr sh_addr;
    Elf32_Off sh_offset;
    Elf32_Word sh_size;
    Elf32_Word sh_link;
    Elf32_Word sh_info;
    Elf32_Word sh_addralign;
    Elf32_Word sh_entsize;
}__attribute__((packed));

struct Elf32_Phdr {
    Elf32_Word p_type;
    Elf32_Off p_offset;
    Elf32_Addr p_vaddr;
    Elf32_Addr p_paddr;
    Elf32_Word p_filesz;
    Elf32_Word p_memsz;
    Elf32_Word p_flags;
    Elf32_Word p_align;
}__attribute__((packed));

struct Elf32_Sym{
    Elf32_Word st_name;
    Elf32_Addr st_value;
    Elf32_Word st_size;
    unsigned char st_info;
    unsigned char st_other;
    Elf32_Half st_shndx;
}__attribute__((packed));

struct Elf32_Rel{
    Elf32_Addr r_offset;
    Elf32_Word r_info;
}__attribute__((packed));

struct Elf32_Rela{
    Elf32_Addr r_offset;
    Elf32_Word r_info;
    Elf32_Sword r_addend;
}__attribute__((packed));

#endif