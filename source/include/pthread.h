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

#ifndef _PTHREAD_H_
#define _PTHREAD_H_

#include <inttypes.h>

#define PTHREAD_MUTEX_MAGIC 0xC001C0DE
#define PTHREAD_COND_MAGIC  0xDEADBEEF

#define PTHREAD_MUTEX_INITIALIZER { 0, -1 }

enum {
    PTHREAD_MUTEX_DEFAULT,
    PTHREAD_MUTEX_ERRORCHECK,
    PTHREAD_MUTEX_NORMAL,
    PTHREAD_MUTEX_RECURSIVE
};

typedef struct pthread_attr {
    char* name;
} pthread_attr_t;

typedef struct pthread {
    int thread_id;
} pthread_t;

typedef struct pthread_mutexattr {
    int flags;
} pthread_mutexattr_t;

typedef struct pthread_mutex {
    uint32_t init_magic;
    int mutex_id;
} pthread_mutex_t;

typedef struct pthread_condattr {
    int flags;
} pthread_condattr_t;

typedef struct pthread_cond {
    uint32_t init_magic;
    int cond_id;
} pthread_cond_t;

int pthread_attr_init( pthread_attr_t* attr );
int pthread_attr_destroy( pthread_attr_t* attr );
int pthread_attr_getname( pthread_attr_t* attr, char** name );
int pthread_attr_setname( pthread_attr_t* attr, char* name );

int pthread_create( pthread_t* thread, const pthread_attr_t* attr,
                    void *( *start_routine )( void* ), void* arg );

int pthread_mutexattr_init( pthread_mutexattr_t* attr );
int pthread_mutexattr_destroy( pthread_mutexattr_t* attr );
int pthread_mutexattr_getprioceiling( const pthread_mutexattr_t* attr, int* prioceiling );
int pthread_mutexattr_getprotocol( const pthread_mutexattr_t* attr, int* protocol );
int pthread_mutexattr_getpshared( const pthread_mutexattr_t* attr, int* shared );
int pthread_mutexattr_gettype( const pthread_mutexattr_t* attr, int* type );
int pthread_mutexattr_setprioceiling( pthread_mutexattr_t* attr, int prioceiling );
int pthread_mutexattr_setprotocol( pthread_mutexattr_t* attr, int protocol );
int pthread_mutexattr_setpshared( pthread_mutexattr_t* attr, int shared );
int pthread_mutexattr_settype( pthread_mutexattr_t* attr, int type );

int pthread_mutex_init( pthread_mutex_t* mutex, pthread_mutexattr_t* attr );
int pthread_mutex_destroy( pthread_mutex_t* mutex );
int pthread_mutex_lock( pthread_mutex_t* mutex );
int pthread_mutex_trylock( pthread_mutex_t* mutex );
int pthread_mutex_unlock( pthread_mutex_t* mutex );

int pthread_cond_init( pthread_cond_t* cond, const pthread_condattr_t* attr );
int pthread_cond_destroy( pthread_cond_t* cond );
int pthread_cond_wait( pthread_cond_t* cond, pthread_mutex_t* mutex );
int pthread_cond_signal( pthread_cond_t* cond );
int pthread_cond_broadcast( pthread_cond_t* cond );

#endif /* _PTHREAD_H_ */
