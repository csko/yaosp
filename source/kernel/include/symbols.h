/* Kernel symbol table
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

#ifndef _SYMBOLS_H_
#define _SYMBOLS_H_

#include <types.h>
#include <loader.h>
#include <lib/hashtable.h>

typedef struct kernel_symbol {
    hashitem_t hash;

    char* name;
    ptr_t address;
} kernel_symbol_t;

int add_kernel_symbol( const char* name, ptr_t address );

int get_kernel_symbol_address( const char* name, ptr_t* address );
int get_kernel_symbol_info( ptr_t address, symbol_info_t* info );

int init_kernel_symbols( void );

#endif /* _SYMBOLS_H_ */
