/* Application loader
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

#ifndef _LOADER_H_
#define _LOADER_H_

#include <types.h>

/* Application loader calls */

typedef struct symbol_info {
    const char* name;
    ptr_t address;
} symbol_info_t;

typedef bool check_application_t( int fd );
typedef int load_application_t( int fd );
typedef int execute_application_t( void );
typedef int get_symbol_info_t( ptr_t address, symbol_info_t* info );

typedef struct application_loader {
    const char* name;

    check_application_t* check;
    load_application_t* load;
    execute_application_t* execute;
    get_symbol_info_t* get_symbol_info;

    struct application_loader* next;
} application_loader_t;

/* Interpreter loader calls */

typedef bool check_interpreter_t( int fd );
typedef int execute_interpreter_t( int fd, const char* filename, char** argv, char** envp );

typedef struct interpreter_loader {
    const char* name;

    check_interpreter_t* check;
    execute_interpreter_t* execute;

    struct interpreter_loader* next;
} interpreter_loader_t;

int get_symbol_info( ptr_t address, symbol_info_t* info );

int do_execve( char* path, char** argv, char** envp, bool free_argv );
int sys_execve( char* path, char** argv, char** envp );

int register_application_loader( application_loader_t* loader );
int register_interpreter_loader( interpreter_loader_t* loader );

int init_interpreter_loader( void );
int init_application_loader( void );

#endif // _LOADER_H_
