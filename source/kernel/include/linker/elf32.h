/* 32bit ELF format handling
 *
 * Copyright (c) 2008, 2009, 2010 Zoltan Kovacs
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

#ifndef _ELF32_H_
#define _ELF32_H_

#include <types.h>
#include <loader.h>
#include <macros.h>
#include <mm/region.h>
#include <linker/elf.h>
#include <lock/mutex.h>
#include <lib/array.h>

#define ELF32_R_SYM(i)  ((i)>>8)
#define ELF32_R_TYPE(i) ((unsigned char)(i))

#define ELF32_ST_BIND(i) ((i)>>4)
#define ELF32_ST_TYPE(i) ((i)&0xF)

enum {
    SHT_NULL = 0,
    SHT_PROGBITS,
    SHT_SYMTAB,
    SHT_STRTAB,
    SHT_RELA,
    SHT_HASH,
    SHT_DYNAMIC,
    SHT_NOTE,
    SHT_NOBITS,
    SHT_REL,
    SHT_SHLIB,
    SHT_DYNSYM
};

enum {
    SF_WRITE = 1,
    SF_ALLOC = 2
};

enum {
    DT_NEEDED = 1,
    DT_PLTRELSZ = 2,
    DT_REL = 17,
    DT_RELSZ = 18,
    DT_JMPREL = 23
};

enum {
    STT_OBJECT = 1,
    STT_FUNC = 2
};

enum {
    SHN_UNDEF = 0
};

enum {
    STN_UNDEF = 0
};

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
} __PACKED elf_section_header_t;

typedef struct elf_symbol {
    uint32_t name;
    uint32_t value;
    uint32_t size;
    uint8_t info;
    uint8_t other;
    uint16_t shndx;
} __PACKED elf_symbol_t;

typedef struct elf_dynamic {
    int32_t tag;
    uint32_t value;
} elf_dynamic_t;

typedef struct elf_reloc {
    uint32_t offset;
    uint32_t info;
} elf_reloc_t;

typedef struct elf_hashtable {
    uint32_t bucket_cnt;
    uint32_t chain_cnt;
} elf_hashtable_t;

typedef struct my_elf_symbol {
    char* name;
    uint32_t address;
    uint32_t size;
    uint8_t info;
    uint16_t section;
} my_elf_symbol_t;

typedef struct elf32_image_info {
    elf_header_t header;
    ptr_t virtual_address;
    elf_section_header_t* section_headers;

    char* string_table;
    char* sh_string_table;
    char* dyn_string_table;

    uint32_t symbol_count;
    elf_symbol_t* symbol_table;

    uint32_t dyn_symbol_count;
    my_elf_symbol_t* dyn_symbol_table;

    uint32_t reloc_count;
    elf_reloc_t* reloc_table;

    char** needed_table;
    uint32_t needed_count;

    elf_hashtable_t* symhash;
    uint32_t* sym_bucket;
    uint32_t* sym_chain;
} elf32_image_info_t;

typedef struct elf32_image {
    elf32_image_info_t info;
    memory_region_t* text_region;
    memory_region_t* data_region;
    struct elf32_image* subimages;
} elf32_image_t;

struct elf32_context;

typedef int elf32_relocate_t( struct elf32_context* context, elf32_image_t* image );

typedef struct elf32_context {
    elf32_image_t main;
    elf32_relocate_t* relocate;
    lock_id lock;
    array_t libraries;
} elf32_context_t;

typedef struct elf_module {
    elf32_image_info_t image_info;

    uint32_t text_address;
    size_t text_size;

    memory_region_t* text_region;
    memory_region_t* data_region;
} elf_module_t;

int elf32_load_and_validate_header( elf32_image_info_t* info, binary_loader_t* loader, uint16_t type );
int elf32_load_section_headers( elf32_image_info_t* info, binary_loader_t* loader );
int elf32_parse_section_headers( elf32_image_info_t* info, binary_loader_t* loader );

int elf32_free_section_headers( elf32_image_info_t* info );
int elf32_free_reloc_table( elf32_image_info_t* info );

my_elf_symbol_t* elf32_get_symbol( elf32_image_info_t* info, const char* name );
elf_symbol_t* elf32_get_symbol2( elf32_image_info_t* info, const char* name );
int elf32_get_symbol_info( elf32_image_info_t* info, ptr_t address, symbol_info_t* symbol_info );

int elf32_init_image_info( elf32_image_info_t* info, ptr_t virtual_address );
int elf32_destroy_image_info( elf32_image_info_t* info );

int elf32_image_load( elf32_image_t* image, binary_loader_t* loader, ptr_t virtual_address, elf_binary_type_t type );

int elf32_context_init( elf32_context_t* context, elf32_relocate_t* relocate );
int elf32_context_destroy( elf32_context_t* context );
int elf32_context_get_symbol( elf32_context_t* context, const char* name, int skip_main, int dynsym_only,
                              elf32_image_t** image, void** symbol );

void* sys_dlopen( const char* filename, int flag );
int sys_dlclose( void* handle );
void* sys_dlsym( void* handle, const char* symbol );
int sys_dlgetglobalinit(void* handle, uint32_t* table, int size);

int init_elf32_kernel_symbols( void );
int init_elf32_module_loader( void );
int init_elf32_application_loader( void );

#endif /* _ELF32_H_ */
