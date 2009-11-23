/* Inter Process Communication
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

#ifndef _IPC_H_
#define _IPC_H_

#include <types.h>
#include <lock/condition.h>

typedef int ipc_port_id;

typedef struct ipc_message {
    uint32_t code;
    size_t size;
    struct ipc_message* next;
} ipc_message_t;

typedef struct ipc_port {
    hashitem_t hash;

    ipc_port_id id;
    lock_id queue_semaphore;
    ipc_message_t* message_queue;
    ipc_message_t* message_queue_tail;
} ipc_port_t;

typedef struct named_ipc_port_t {
    hashitem_t hash;

    const char* name;
    ipc_port_id port_id;
} named_ipc_port_t;

ipc_port_id sys_create_ipc_port( void );
int sys_destroy_ipc_port( ipc_port_id port_id );

int sys_send_ipc_message( ipc_port_id port_id, uint32_t code, void* data, size_t size );
int sys_recv_ipc_message( ipc_port_id port_id, uint32_t* code, void* buffer, size_t size, uint64_t* timeout );
int sys_peek_ipc_message( ipc_port_id port_id, uint32_t* code, size_t* size, uint64_t* timeout );

int sys_register_named_ipc_port( const char* name, ipc_port_id port_id );
int sys_get_named_ipc_port( const char* name, ipc_port_id* port_id );

int init_ipc( void );

#endif /* _IPC_H_ */
