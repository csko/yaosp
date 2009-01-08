/* 32bit ELF format handling
 *
 * Copyright (c) 2008 Zoltan Kovacs
 * Copyright (c) 2009 Kornel Csernai
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef _ARCH_ELF32_H_
#define _ARCH_ELF32_H_

#include <types.h>
#include <mm/region.h>

#define ELF32_R_SYM(i)  ((i)>>8)
#define ELF32_R_TYPE(i) ((unsigned char)(i))

#define ELF32_ST_TYPE(i) ((i)&0xF)

enum {
    ID_MAGIC0 = 0,
    ID_MAGIC1,
    ID_MAGIC2,
    ID_MAGIC3,
    ID_CLASS,
    ID_DATA,
    ID_VERSION,
    ID_SIZE = 16
};

enum {
    ELF32_MAGIC0 = 0x7F,
    ELF32_MAGIC1 = 'E',
    ELF32_MAGIC2 = 'L',
    ELF32_MAGIC3 = 'F'
};

enum {
    ELF_CLASS_32 = 1
};

enum {
    ELF_DATA_2_LSB = 1
};

enum {
    ELF_EM_386 = 3
};

enum {
    SECTION_NULL = 0,
    SECTION_PROGBITS,
    SECTION_SYMTAB,
    SECTION_STRTAB,
    SECTION_RELA,
    SECTION_HASH,
    SECTION_DYNAMIC,
    SECTION_NOTE,
    SECTION_NOBITS,
    SECTION_REL,
    SECTION_SHLIB,
    SECTION_DYNSYM
};

enum {
    SF_WRITE = 1,
    SF_ALLOC = 2
};

enum {
    DYN_PLTRELSZ = 2,
    DYN_REL = 17,
    DYN_RELSZ = 18,
    DYN_JMPREL = 23
};

enum {
    R_386_32 = 1,
    R_386_GLOB_DATA = 6,
    R_386_JMP_SLOT = 7,
    R_386_RELATIVE = 8
};

enum {
    STT_OBJECT = 1,
    STT_FUNC = 2
};

typedef struct elf_header {
    unsigned char ident[ ID_SIZE ];
    uint16_t type;
    uint16_t machine;
    uint32_t version;
    uint32_t entry;
    uint32_t phoff;
    uint32_t shoff;
    uint32_t flags;
    uint16_t ehsize;
    uint16_t phentsize;
    uint16_t phnum;
    uint16_t shentsize;
    uint16_t shnum;
    uint16_t shstrndx;
} __attribute__(( packed )) elf_header_t;

typedef struct elf_section_header {
    uint32_t name;
    uint32_t type;
    uint32_t flags;
    uint32_t address;
    uint32_t offset;
    uint32_t size;
    uint32_t link;
    uint32_t info;
    uint32_t addralign;
    uint32_t entsize;
} __attribute__(( packed )) elf_section_header_t;

typedef struct elf_symbol {
    uint32_t name;
    uint32_t value;
    uint32_t size;
    uint8_t info;
    uint8_t other;
    uint16_t shndx;
} __attribute__(( packed )) elf_symbol_t;

typedef struct elf_dynamic {
    int32_t tag;
    uint32_t value;
} elf_dynamic_t;

typedef struct elf_reloc {
    uint32_t offset;
    uint32_t info;
} elf_reloc_t;

typedef struct my_elf_symbol {
    char* name;
    uint32_t address;
    uint8_t info;
} my_elf_symbol_t;

typedef struct elf_module {
    uint32_t section_count;
    elf_section_header_t* sections;

    char* strings;

    uint32_t symbol_count;
    my_elf_symbol_t* symbols;

    uint32_t reloc_count;
    elf_reloc_t* relocs;

    uint32_t text_address;
    region_id text_region;
    region_id data_region;
} elf_module_t;

typedef struct elf_application {
    uint32_t section_count;
    elf_section_header_t* sections;

    char* strings;

    uint32_t symbol_count;
    my_elf_symbol_t* symbols;

    uint32_t reloc_count;
    elf_reloc_t* relocs;

    register_t entry_address;

    region_id text_region;
    region_id data_region;
} elf_application_t;

int init_elf32_module_loader( void );
int init_elf32_application_loader( void );

bool elf32_check( elf_header_t* header );

#endif // _ARCH_ELF32_H_
