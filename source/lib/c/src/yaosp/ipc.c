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

#include <yaosp/ipc.h>

#include <yaosp/syscall.h>
#include <yaosp/syscall_table.h>

ipc_port_id create_ipc_port( void ) {
    return syscall0(
        SYS_create_ipc_port
    );
}

int destroy_ipc_port( ipc_port_id port_id ) {
    return syscall1(
        SYS_destroy_ipc_port,
        port_id
    );
}

int send_ipc_message( ipc_port_id port_id, uint32_t code, void* data, size_t size ) {
    return syscall4(
        SYS_send_ipc_message,
        port_id,
        code,
        ( int )data,
        size
    );
}

int recv_ipc_message( ipc_port_id port_id, uint32_t* code, void* buffer, size_t size, uint64_t timeout ) {
    return syscall5(
        SYS_recv_ipc_message,
        port_id,
        ( int )code,
        ( int )buffer,
        size,
        ( int )&timeout
    );
}

int peek_ipc_message( ipc_port_id port_id, uint32_t* code, size_t* size, uint64_t timeout ) {
    return syscall4(
        SYS_peek_ipc_message,
        port_id,
        ( int )code,
        ( int )size,
        ( int )&timeout
    );
}

int register_named_ipc_port( const char* name, ipc_port_id port_id ) {
    return syscall2(
        SYS_register_named_ipc_port,
        ( int )name,
        port_id
    );
}

int get_named_ipc_port( const char* name, ipc_port_id* port_id ) {
    return syscall2(
        SYS_get_named_ipc_port,
        ( int )name,
        ( int )port_id
    );
}
