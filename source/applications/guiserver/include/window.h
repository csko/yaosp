/* GUI server
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

#ifndef _WINDOW_H_
#define _WINDOW_H_

#include <ygui/protocol.h>

typedef struct window {
    char* title;
    point_t position;
    point_t size;
    int flags;

    ipc_port_id client_port;
    ipc_port_id server_port;
} window_t;

int handle_create_window( msg_create_win_t* request );

#endif /* _WINDOW_H_ */
