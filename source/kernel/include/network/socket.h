/* Socket handling
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

#ifndef _NETWORK_SOCKET_H_
#define _NETWORK_SOCKET_H_

#include <vfs/inode.h>
#include <lib/hashtable.h>

typedef struct socket {
    hashitem_t hash;

    ino_t inode_number;
} socket_t;

int sys_socket( int family, int type, int protocol );

int init_socket( void );

#endif /* _NETWORK_SOCKET_H_ */
