/* gettimeofday function
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

#include <sys/time.h>

#include <yaosp/syscall.h>
#include <yaosp/syscall_table.h>

int gettimeofday( struct timeval* tv, struct timezone* tz ) {
    int ret;
    uint64_t time;

    ret = syscall1( SYS_get_system_time, ( int )&time );

    if ( ret >= 0 ) {
        if ( tv != NULL ) {
            tv->tv_sec = time / 1000000;
            tv->tv_usec = time % 1000000;
        }
    }

    return ret;
}
