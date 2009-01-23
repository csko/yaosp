/* Multiboot structure definitions and constants
 *
 * Copyright (c) 2008 Zoltan Kovacs
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

#ifndef _MULTIBOOT_H_
#define _MULTIBOOT_H_

#define MB_HEADER_MAGIC     0x1BADB002
#define MB_BOOTLOADER_MAGIC 0x2BADB002

#define MB_FLAG_ALIGN_MODULES 0x0001
#define MB_FLAG_MEMORY_INFO   0x0002

#ifndef __ASSEMBLER__

#include <types.h>

typedef struct multiboot_header {
    uint32_t flags;

    uint32_t memory_lower;
    uint32_t memory_upper;

    uint32_t boot_device;

    const char* kernel_parameters;

    uint32_t module_count;
    void* first_module;

    uint32_t unused1;
    uint32_t unused2;
    uint32_t unused3;
    uint32_t unused4;

    uint32_t memory_map_length;
    uint32_t memory_map_address;
} multiboot_header_t;

typedef struct multiboot_module {
    uint32_t start;
    uint32_t end;
    char* parameters;
    uint32_t reserved;
} multiboot_module_t;

typedef struct multiboot_mmap_entry {
    uint32_t size;
    uint64_t base;
    uint64_t length;
    uint32_t type;
} multiboot_mmap_entry_t;

#endif // __ASSEMBLER__

#endif // _MULTIBOOT_H_
