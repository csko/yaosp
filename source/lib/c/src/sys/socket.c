/* socket function
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

int socket( int domain, int type, int protocol ) {
#if 0
    int error;

    error = syscall3( SYS_socket, domain, type, protocol );

    if ( error < 0 ) {
        errno = -error;
        return -1;
    }

    return error;
#endif

    return -1;
}
