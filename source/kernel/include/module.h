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
#include <config.h>
#include <loader.h>
#include <lib/hashtable.h>

#define MODULE_DEPENDENCIES(...) \
    const char* __attribute__(( used )) __module_dependencies[] = { \
        __VA_ARGS__, \
        NULL \
    }

#define MODULE_OPTIONAL_DEPENDENCIES(...) \
    const char* __attribute__(( used )) __module_optional_dependencies[] = { \
        __VA_ARGS__, \
        NULL \
    }

/* Module definitions */

typedef int init_module_t( void );
typedef int destroy_module_t( void );

typedef enum module_status {
    MODULE_LOADING,
    MODULE_LOADED
} module_status_t;

typedef struct module {
    hashitem_t hash;

    char* name;
    module_status_t status;

    init_module_t* init;
    destroy_module_t* destroy;

    void* loader_data;
} module_t;

typedef struct module_info {
    uint32_t dependency_count;
    char name[ MAX_MODULE_NAME_LENGTH ];
} module_info_t;

typedef struct module_dependencies {
    size_t dep_count;
    char** dep_table;
    size_t optional_dep_count;
    char** optional_dep_table;
} module_dependencies_t;

/* Module loader definitions */

typedef bool check_module_t( binary_loader_t* loader );
typedef int load_module_t( module_t* module, binary_loader_t* loader );
typedef int get_module_dependencies_t( module_t* module, module_dependencies_t* deps );
typedef int free_module_t( module_t* module );
typedef bool get_module_symbol_t( module_t* module, const char* symbol_name, ptr_t* symbol_addr );
typedef bool get_module_symbol_info_t( module_t* module, ptr_t address, symbol_info_t* symbol_info );

typedef struct module_loader {
    const char* name;
    check_module_t* check_module;
    load_module_t* load_module;
    get_module_dependencies_t* get_dependencies;
    free_module_t* free;
    get_module_symbol_t* get_symbol;
    get_module_symbol_info_t* get_symbol_info;
} module_loader_t;

int load_module( const char* name );

int get_module_symbol_info( ptr_t address, symbol_info_t* info );

int sys_load_module( const char* name );
uint32_t sys_get_module_count( void );
int sys_get_module_info( module_info_t* module_info, uint32_t max_count );

void set_module_loader( module_loader_t* loader );

int init_module_loader( void );

#endif /* _MODULE_H_ */
