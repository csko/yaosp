/* Common ELF format definitions
 *
 * Copyright (c) 2009 Zoltan Kovacs
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

#ifndef _LINKER_ELF_H_
#define _LINKER_ELF_H_

#include <types.h>

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
    ELF_MAGIC0 = 0x7F,
    ELF_MAGIC1 = 'E',
    ELF_MAGIC2 = 'L',
    ELF_MAGIC3 = 'F'
};

enum {
    ELF_CLASS_32 = 1,
    ELF_CLASS_64 = 2
};

enum {
    ELF_TYPE_EXEC = 2,
    ELF_TYPE_DYN = 3
};

enum {
    ELF_DATA_2_LSB = 1
};

enum {
    ELF_MACHINE_386 = 3
};

typedef struct elf_header {
    uint8_t ident[ ID_SIZE ];
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

bool elf_check_header( elf_header_t* header, uint8_t class, uint8_t data, uint16_t type, uint16_t machine );

#endif /* _LINKER_ELF_H_ */
