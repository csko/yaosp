/* Global Descriptor Table handling
 *
 * Copyright (c) 2008, 2009 Zoltan Kovacs
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

#ifndef _I386_GDT_H_
#define _I386_GDT_H_

#define KERNEL_CS 0x08
#define KERNEL_DS 0x10
#define USER_CS   0x18
#define USER_DS   0x20

#ifndef __ASSEMBLER__

#include <types.h>
#include <config.h>

#define GDT_ENTRIES 5

typedef struct gdt_descriptor {
    unsigned limit_low : 16;
    unsigned base_low : 16;
    unsigned base_mid : 8;
    unsigned access : 8;
    unsigned limit_high : 4;
    unsigned available : 1;
    unsigned unused : 1;
    unsigned special : 1;
    unsigned granularity : 1;
    unsigned base_high : 8;
} __attribute__(( packed )) gdt_descriptor_t;

typedef struct gdt {
    uint16_t size;
    uint32_t base;
} __attribute__(( packed )) gdt_t;

extern gdt_descriptor_t gdt[ GDT_ENTRIES + MAX_CPU_COUNT ];

void gdt_set_descriptor_base( uint16_t desc, uint32_t base );
void gdt_set_descriptor_limit( uint16_t desc, uint32_t limit );
void gdt_set_descriptor_access( uint16_t desc, uint8_t access );

void reload_segment_descriptors( void );

int init_gdt( void );

#endif // __ASSEMBLER__

#endif // _I386_GDT_H_
