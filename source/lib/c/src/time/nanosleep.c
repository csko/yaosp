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

#include <time.h>
#include <errno.h>
#include <sys/types.h>

#include <yaosp/syscall.h>
#include <yaosp/syscall_table.h>

int nanosleep( const struct timespec* req, struct timespec* rem ) {
    int error;
    uint64_t microsecs;
    uint64_t remaining;

    microsecs = ( uint64_t )req->tv_sec * 1000000 + ( uint64_t )req->tv_nsec / 1000;

    if ( microsecs == 0 ) {
        microsecs = 1;
    }

    error = syscall2( SYS_sleep_thread, ( int )&microsecs, ( int )&remaining );

    if ( error < 0 ) {
        errno = -error;

        if ( rem != NULL ) {
            rem->tv_sec = remaining / 1000000;
            rem->tv_nsec = ( remaining % 1000000 ) * 1000;
        }

        return -1;
    }

    return 0;
}
