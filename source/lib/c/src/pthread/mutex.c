/* pthread mutex functions
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

#include <pthread.h>
#include <errno.h>

#include <yaosp/debug.h>
#include <yaosp/syscall.h>
#include <yaosp/syscall_table.h>

int pthread_mutex_init( pthread_mutex_t* mutex, pthread_mutexattr_t* attr ) {
    if ( mutex == NULL ) {
        return EINVAL;
    }

    mutex->mutex_id = syscall2( SYS_mutex_create, ( int )"pthread mutex", 0 );

    if ( mutex->mutex_id < 0 ) {
        return -mutex->mutex_id;
    }

    mutex->init_magic = PTHREAD_MUTEX_MAGIC;

    return 0;
}

int pthread_mutex_destroy( pthread_mutex_t* mutex ) {
    if ( mutex->init_magic != PTHREAD_MUTEX_MAGIC ) {
        return -1;
    }

    syscall1( SYS_mutex_destroy, mutex->mutex_id );

    mutex->mutex_id = -1;
    mutex->init_magic = 0;

    return 0;
}

int pthread_mutex_lock( pthread_mutex_t* mutex ) {
    int error;

    if ( mutex->init_magic != PTHREAD_MUTEX_MAGIC ) {
        error = pthread_mutex_init( mutex, NULL );

        if ( error < 0 ) {
            return -1;
        }
    }

    error = syscall1( SYS_mutex_lock, mutex->mutex_id );

    if ( error < 0 ) {
        errno = error;
        return -1;
    }

    return 0;
}

int pthread_mutex_trylock( pthread_mutex_t* mutex ) {
    int error;

    if ( mutex->init_magic != PTHREAD_MUTEX_MAGIC ) {
        error = pthread_mutex_init( mutex, NULL );

        if ( error < 0 ) {
            return -1;
        }
    }

    error = syscall1( SYS_mutex_trylock, mutex->mutex_id );

    if ( error < 0 ) {
        errno = error;
        return -1;
    }

    return 0;
}

int pthread_mutex_unlock( pthread_mutex_t* mutex ) {
    int error;

    if ( mutex->init_magic != PTHREAD_MUTEX_MAGIC ) {
        error = pthread_mutex_init( mutex, NULL );

        if ( error < 0 ) {
            return -1;
        }
    }

    error = syscall1( SYS_mutex_unlock, mutex->mutex_id );

    if ( error < 0 ) {
        errno = error;
        return -1;
    }

    return 0;
}

