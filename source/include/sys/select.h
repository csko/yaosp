/* yaosp C library
 *
 * Copyright (c) 2009, 2010 Zoltan Kovacs
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

#ifndef _SYS_SELECT_H_
#define _SYS_SELECT_H_

#include <string.h>
#include <sys/time.h>
#include <sys/types.h>

#define FD_SETSIZE 1024
#define FD_ZERO(set) \
    memset((set)->fds, 0, FD_SETSIZE / 32);
#define FD_CLR(fd,set) \
    (set)->fds[fd/32] &= ~(1<<(fd%32));
#define FD_SET(fd,set) \
    (set)->fds[fd/32] |= (1<<(fd%32));
#define FD_ISSET(fd,set) \
    ((set)->fds[fd/32] & (1<<(fd%32)))

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t fds[ FD_SETSIZE / 32 ];
} fd_set;

int select(int fds, fd_set* readfds, fd_set* writefds, fd_set* exceptfds, struct timeval* timeout);

#ifdef __cplusplus
}
#endif

#endif /* _SYS_SELECT_H_ */
