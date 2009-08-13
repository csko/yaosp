/* Kernel symbol table
 *
 * Copyright (c) 2008, 2009 Zoltan Kovacs
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

#include <symbols.h>
#include <errno.h>
#include <mm/kmalloc.h>
#include <lib/string.h>

static hashtable_t kernel_symbol_table;

int add_kernel_symbol( const char* name, ptr_t address ) {
    int error;
    size_t name_len;
    kernel_symbol_t* symbol;

    name_len = strlen( name );

    symbol = ( kernel_symbol_t* )kmalloc( sizeof( kernel_symbol_t ) + name_len + 1 );

    if ( symbol == NULL ) {
        error = -ENOMEM;
        goto error1;
    }

    symbol->name = ( char* )( symbol + 1 );
    symbol->address = address;

    memcpy( symbol->name, name, name_len + 1 );

    error = hashtable_add( &kernel_symbol_table, ( hashitem_t* )symbol );

    if ( error < 0 ) {
        goto error2;
    }

    return 0;

error2:
    kfree( symbol );

error1:
    return error;
}

int get_kernel_symbol_address( const char* name, ptr_t* address ) {
    kernel_symbol_t* symbol;

    symbol = ( kernel_symbol_t* )hashtable_get( &kernel_symbol_table, ( void* )name );

    if ( symbol == NULL ) {
        return -ENOENT;
    }

    *address = symbol->address;

    return 0;
}

typedef struct sym_lookup_info {
    ptr_t address;
    symbol_info_t* sym_info;
} sym_lookup_info_t;

static int get_kernel_sym_info_iterator( hashitem_t* item, void* data ) {
    kernel_symbol_t* symbol;
    sym_lookup_info_t* info;

    symbol = ( kernel_symbol_t* )item;
    info = ( sym_lookup_info_t* )data;

    if ( symbol->address > info->address ) {
        return 0;
    }

    if ( info->sym_info->address == 0 ) {
        info->sym_info->name = symbol->name;
        info->sym_info->address = symbol->address;
    } else if ( symbol->address <= info->address ) {
        int last_diff;
        int current_diff;

        last_diff = info->address - info->sym_info->address;
        current_diff = info->address - symbol->address;

        if ( current_diff < last_diff ) {
            info->sym_info->name = symbol->name;
            info->sym_info->address = symbol->address;
        }
    }

    return 0;
}

int get_kernel_symbol_info( ptr_t address, symbol_info_t* info ) {
    int error;
    sym_lookup_info_t lookup_info;

    info->name = NULL;
    info->address = 0;

    lookup_info.sym_info = info;
    lookup_info.address = address;

    error = hashtable_iterate( &kernel_symbol_table, get_kernel_sym_info_iterator, ( void* )&lookup_info );

    if ( info->address == 0 ) {
        return -ENOENT;
    }

    return 0;
}

static void* kernel_sym_key( hashitem_t* item ) {
    kernel_symbol_t* symbol;

    symbol = ( kernel_symbol_t* )item;

    return ( void* )symbol->name;
}

int init_kernel_symbols( void ) {
    int error;

    error = init_hashtable(
        &kernel_symbol_table,
        1024,
        kernel_sym_key,
        hash_str,
        compare_str
    );

    if ( error < 0 ) {
        return error;
    }

    return 0;
}
