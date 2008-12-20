/* Interrupt descriptor table definitions
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

#ifndef _ARCH_IDT_H_
#define _ARCH_IDT_H_

#include <types.h>

#define IDT_ENTRIES 256

typedef struct idt_descriptor {
    uint16_t base_low;
    uint16_t selector;
    uint8_t always0;
    uint8_t flags;
    uint16_t base_high;
} __attribute__(( packed )) idt_descriptor_t;

typedef struct idt {
    uint16_t limit;
    uint32_t base;
} __attribute__(( packed )) idt_t;

extern idt_descriptor_t idt[ IDT_ENTRIES ];

#endif // _ARCH_IDT_H_
