/* sendto function
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

ssize_t sendto( int s, const void* buf, size_t len, int flags, const struct sockaddr* to, socklen_t tolen ) {
    int error;
    struct msghdr msg;
    struct iovec iov;

    iov.iov_base = ( void* )buf;
    iov.iov_len = len;

    msg.msg_name = ( void* )to;
    msg.msg_namelen = tolen;
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_control = NULL;
    msg.msg_controllen = 0;
    msg.msg_flags = 0;

    error = sendmsg( s, &msg, flags );

    if ( error < 0 ) {
        errno = -error;
        return -1;
    }

    return error;
}
