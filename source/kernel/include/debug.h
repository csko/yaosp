/* Debugger related functions, definitions
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

#ifndef _DEBUG_H_
#define _DEBUG_H_

typedef int dbg_cmd_callback_t( const char* params );

typedef struct dbg_command {
    const char* command;
    dbg_cmd_callback_t* callback;
    const char* help;
} dbg_command_t;

struct thread;

char arch_dbg_get_character( void );

int arch_dbg_show_thread_info( struct thread* thread );
int arch_dbg_trace_thread( struct thread* thread );

int dbg_set_scroll_mode( bool scroll_mode );
int dbg_printf( const char* format, ... );

int debug_print_stack_trace( void );

int start_kernel_debugger( void );

#endif /* _DEBUG_H_ */
