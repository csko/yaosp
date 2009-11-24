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

typedef struct binary_loader {
    void* private;
    int ( *read )( void* private, void* buffer, off_t offset, int size );
    char* ( *get_name )( void* private );
    int ( *get_fd )( void* private );
} binary_loader_t;

binary_loader_t* get_app_binary_loader( int fd );
void put_app_binary_loader( binary_loader_t* loader );

/* Application loader calls */

typedef struct symbol_info {
    const char* name;
    ptr_t address;
} symbol_info_t;

struct thread;

typedef bool check_application_t( binary_loader_t* loader );
typedef int load_application_t( binary_loader_t* loader );
typedef int execute_application_t( void );
typedef int get_symbol_info_t( struct thread* thread, ptr_t address, symbol_info_t* info );
typedef int destroy_application_t( void* data );

typedef struct application_loader {
    const char* name;

    check_application_t* check;
    load_application_t* load;
    execute_application_t* execute;
    get_symbol_info_t* get_symbol_info;
    destroy_application_t* destroy;

    struct application_loader* next;
} application_loader_t;

/* Interpreter loader calls */

typedef bool check_interpreter_t( binary_loader_t* loader );
typedef int execute_interpreter_t( binary_loader_t* loader, const char* filename, char** argv, char** envp );

typedef struct interpreter_loader {
    const char* name;

    check_interpreter_t* check;
    execute_interpreter_t* execute;

    struct interpreter_loader* next;
} interpreter_loader_t;

struct thread;

int get_application_symbol_info( struct thread* thread, ptr_t address, symbol_info_t* info );

int do_execve( char* path, char** argv, char** envp, bool free_argv );
int sys_execve( char* path, char** argv, char** envp );

int register_application_loader( application_loader_t* loader );
int register_interpreter_loader( interpreter_loader_t* loader );

int init_interpreter_loader( void );
int init_application_loader( void );

#endif /* _LOADER_H_ */
