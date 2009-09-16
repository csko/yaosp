/* pthread mutexattr functions
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

int pthread_mutexattr_init( pthread_mutexattr_t* attr ) {
    attr->flags = 0;

    return 0;
}

int pthread_mutexattr_destroy( pthread_mutexattr_t* attr ) {
    return 0;
}

int pthread_mutexattr_getprioceiling( const pthread_mutexattr_t* attr, int* prioceiling ) {
    return ENOSYS;
}

int pthread_mutexattr_getprotocol( const pthread_mutexattr_t* attr, int* protocol ) {
    return ENOSYS;
}

int pthread_mutexattr_getpshared( const pthread_mutexattr_t* attr, int* shared ) {
    return ENOSYS;
}

int pthread_mutexattr_gettype( const pthread_mutexattr_t* attr, int* type) {
    return ENOSYS;
}

int pthread_mutexattr_setprioceiling( pthread_mutexattr_t* attr, int prioceiling ) {
    return ENOSYS;
}

int pthread_mutexattr_setprotocol( pthread_mutexattr_t* attr, int protocol ) {
    return ENOSYS;
}

int pthread_mutexattr_setpshared( pthread_mutexattr_t* attr, int shared ) {
    return ENOSYS;
}

int pthread_mutexattr_settype( pthread_mutexattr_t* attr, int type ) {
    return ENOSYS;
}

