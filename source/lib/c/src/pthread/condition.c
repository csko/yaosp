/* pthread condition functions
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

int pthread_cond_init( pthread_cond_t* cond, const pthread_condattr_t* attr ) {
    if ( cond == NULL ) {
        return EINVAL;
    }

    cond->cond_id = syscall1( SYS_condition_create, ( int )"pthread condition" );

    if ( cond->cond_id < 0 ) {
        return -cond->cond_id;
    }

    cond->init_magic = PTHREAD_COND_MAGIC;

    return 0;
}

int pthread_cond_destroy( pthread_cond_t* cond ) {
    /* todo */
    return 0;
}

int pthread_cond_wait( pthread_cond_t* cond, pthread_mutex_t* mutex ) {
    int error;

    /* todo */

    error = syscall2( SYS_condition_wait, cond->cond_id, mutex->mutex_id );

    if ( error < 0 ) {
        return -error;
    }

    return 0;
}

int pthread_cond_signal( pthread_cond_t* cond ) {
    int error;

    /* todo */

    error = syscall1( SYS_condition_signal, cond->cond_id );

    if ( error < 0 ) {
        return -error;
    }

    return 0;
}

int pthread_cond_broadcast( pthread_cond_t* cond ) {
    int error;

    /* todo */

    error = syscall1( SYS_condition_broadcast, cond->cond_id );

    if ( error < 0 ) {
        return -error;
    }

    return 0;
}
