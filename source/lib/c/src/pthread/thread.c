/* pthread thread functions
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

#include <errno.h>
#include <pthread.h>

#include <yaosp/syscall.h>
#include <yaosp/syscall_table.h>

enum {
    PRIORITY_HIGH = 31,
    PRIORITY_DISPLAY = 23,
    PRIORITY_NORMAL = 15,
    PRIORITY_LOW = 7,
    PRIORITY_IDLE = 0
};

int pthread_create( pthread_t* thread, const pthread_attr_t* attr,
                    void *( *start_routine )( void* ), void* arg ) {
    char* name;

    if ( ( thread == NULL ) ||
         ( start_routine == NULL ) ) {
        return EINVAL;
    }

    if ( ( attr != NULL ) && ( attr->name != NULL ) ) {
        name = attr->name;
    } else {
        name = "pthread";
    }

    thread->thread_id = syscall5(
        SYS_create_thread,
        ( int )name,
        PRIORITY_NORMAL,
        ( int )start_routine,
        ( int )arg,
        0
    );

    if ( thread->thread_id < 0 ) {
        return thread->thread_id;
    }

    syscall1(
        SYS_wake_up_thread,
        thread->thread_id
    );


    return 0;
}
