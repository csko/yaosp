/* yaosp C library
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

#ifndef _YAOSP_THREAD_H_
#define _YAOSP_THREAD_H_

#include <sys/types.h>

enum {
    PRIORITY_HIGH = 31,
    PRIORITY_DISPLAY = 23,
    PRIORITY_NORMAL = 15,
    PRIORITY_LOW = 7,
    PRIORITY_IDLE = 0
};

typedef int thread_id;
typedef int thread_entry_t( void* arg );

thread_id create_thread( const char* name, int priority, thread_entry_t* entry, void* arg, uint32_t user_stack_size );
int wake_up_thread( thread_id id );

int kill_thread( thread_id id, int signal );

#endif /* _YAOSP_THREAD_H_ */
