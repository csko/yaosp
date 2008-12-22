/* Module loader and manager
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

#ifndef _MODULE_H_
#define _MODULE_H_

#include <types.h>
#include <bootmodule.h>
#include <lib/hashtable.h>

typedef int module_id;

typedef int init_module_t( void );
typedef int destroy_module_t( void );

typedef struct module {
    hashitem_t hash;

    module_id id;

    init_module_t* init;
    destroy_module_t* destroy;

    void* loader_data;
} module_t;

typedef bool check_module_t( void* data, size_t size );
typedef module_t* load_module_t( void* data, size_t size );
typedef int free_module_t( module_t* module );
typedef bool get_module_symbol_t( module_t* module, const char* symbol_name, ptr_t* symbol_addr );

typedef struct module_loader {
    const char* name;
    check_module_t* check;
    load_module_t* load;
    free_module_t* free;
    get_module_symbol_t* get_symbol;
} module_loader_t;

module_t* create_module( void );

module_id load_module_from_bootmodule( bootmodule_t* bootmodule );
int initialize_module( module_id id );

void set_module_loader( module_loader_t* loader );

int init_module_loader( void );

#endif // _MODULE_H_
