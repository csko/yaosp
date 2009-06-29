/* IPC functions
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

#ifndef _YAOSP_IPC_H_
#define _YAOSP_IPC_H_

#include <sys/types.h>

typedef int ipc_port_id;

ipc_port_id create_ipc_port( void );
int destroy_ipc_port( ipc_port_id port_id );

int send_ipc_message( ipc_port_id port_id, uint32_t code, void* data, size_t size );
int recv_ipc_message( ipc_port_id port_id, uint32_t* code, void* buffer, size_t size, uint64_t timeout );
int peek_ipc_message( ipc_port_id port_id, uint32_t* code, size_t* size, uint64_t timeout );

int register_named_ipc_port( const char* name, ipc_port_id port_id );
int get_named_ipc_port( const char* name, ipc_port_id* port_id );

#endif /* _YAOSP_IPC_H_ */
