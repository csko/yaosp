/* Module loader and manager
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

#ifndef _MODULE_H_
#define _MODULE_H_

#include <types.h>
#include <lib/hashtable.h>

/* Module definitions */

typedef int module_id;

typedef int init_module_t( void );
typedef int destroy_module_t( void );

typedef struct module {
    hashitem_t hash;

    module_id id;
    char* name;

    init_module_t* init;
    destroy_module_t* destroy;

    void* loader_data;
} module_t;

/* Module reader definitions */

typedef int read_module_t( void* private, void* data, off_t offset, int size );
typedef size_t get_module_size_t( void* private );
typedef char* get_module_name_t( void* private );

typedef struct module_reader {
    void* private;
    read_module_t* read;
    get_module_size_t* get_size;
    get_module_name_t* get_name;
} module_reader_t;

/* Module loader definitions */

typedef bool check_module_t( module_reader_t* reader );
typedef module_t* load_module_t( module_reader_t* reader );
typedef int free_module_t( module_t* module );
typedef bool get_module_symbol_t( module_t* module, const char* symbol_name, ptr_t* symbol_addr );

typedef struct module_loader {
    const char* name;
    check_module_t* check;
    load_module_t* load;
    free_module_t* free;
    get_module_symbol_t* get_symbol;
} module_loader_t;

module_t* create_module( const char* name );

int read_module_data( module_reader_t* reader, void* buffer, off_t offset, int size );
size_t get_module_size( module_reader_t* reader );
char* get_module_name( module_reader_t* reader );

module_id load_module( module_reader_t* reader );
int initialize_module( module_id id );

void set_module_loader( module_loader_t* loader );

int init_module_loader( void );

#endif // _MODULE_H_
