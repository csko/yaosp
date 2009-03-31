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
#include <semaphore.h>

typedef int ipc_port_id;

typedef struct ipc_message {
    size_t size;
    struct ipc_message* next;
} ipc_message_t;

typedef struct ipc_port {
    hashitem_t hash;

    ipc_port_id id;

    semaphore_id queue_sync;
    size_t queue_size;
    ipc_message_t* message_queue;
    ipc_message_t* message_queue_tail;
} ipc_port_t;

int init_ipc( void );

#endif /* _IPC_H_ */
