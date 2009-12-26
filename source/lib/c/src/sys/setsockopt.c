/* setsockopt function
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
#include <sys/socket.h>

#include <yaosp/syscall.h>
#include <yaosp/syscall_table.h>

int setsockopt( int s, int level, int optname,
                const void* optval, socklen_t optlen ) {
    int error;

    error = syscall5( SYS_setsockopt, s, level, optname, ( int )optval, ( int )optlen );

    if ( error < 0 ) {
        errno = -error;
        return -1;
    }

    return error;
}
