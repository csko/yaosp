/* i386 ELF loader
 *
 * Copyright (c) 2010 Zoltan Kovacs
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

#include <linker/elf32.h>

enum {
    R_386_NONE,
    R_386_32,
    R_386_PC32,
    R_386_GOT32,
    R_386_PLT32,
    R_386_COPY,
    R_386_GLOB_DATA,
    R_386_JMP_SLOT,
    R_386_RELATIVE,
    R_386_GOTOFF,
    R_386_GOTPC
};

int elf32_relocate_i386( elf32_context_t* context, elf32_image_t* image );

#endif /* _ARCH_ELF32_H_ */
