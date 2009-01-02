/* System calls
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

#ifndef _SYSCALL_H_
#define _SYSCALL_H_

#include <types.h>

#define SYSCALL_SAVE_STACK 0x01

typedef struct system_call_entry {
    char* name;
    void* function;
    uint32_t flags;
} system_call_entry_t;

typedef int system_call_t( uint32_t param1, uint32_t param2, uint32_t param3, uint32_t param4, uint32_t param5 );

int handle_system_call( uint32_t number, uint32_t* parameters, void* stack );

#endif // _SYSCALL_H_
