/* yaosp GUI library
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

#ifndef _YGUI_APPLICATION_H_
#define _YGUI_APPLICATION_H_

#include <inttypes.h>
#include <yaosp/ipc.h>

typedef int msg_handler_t( uint32_t code, void* buffer );

enum {
    APP_NONE = 0,
    APP_NOTIFY_RESOLUTION_CHANGE = ( 1 << 0 )
};

typedef struct ymsg {
    uint32_t code;
    uint8_t* buffer;
    size_t size;
} ymsg_t;

int ymsg_init( ymsg_t* msg, size_t max_size );

ipc_port_id application_get_guiserver_port( void );
ipc_port_id application_get_client_port( void );

int application_set_message_handler( msg_handler_t* handler );
int application_register_window_listener( int get_window_list );

int application_init( uint32_t flags );

int application_receive_msg( ymsg_t* msg, uint64_t timeout );
int application_process_msg( ymsg_t* msg );

int application_run( void );

#endif /* _YGUI_APPLICATION_H_ */
