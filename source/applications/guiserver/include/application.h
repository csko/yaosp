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

#ifndef _APPLICATION_H_
#define _APPLICATION_H_

#include <pthread.h>

#include <ygui/protocol.h>
#include <yutil/array.h>

typedef struct application {
    ipc_port_id server_port;
    ipc_port_id client_port;

    array_t window_list;
    array_t bitmap_list;
    array_t font_list;

    pthread_mutex_t lock;
} application_t;

struct window;
struct bitmap;

int application_insert_bitmap( application_t* application, struct bitmap* bitmap );
int application_remove_bitmap( application_t* application, struct bitmap* bitmap );

int application_insert_window( application_t* application, struct window* window );
int application_remove_window( application_t* application, struct window* window );

int handle_create_application( msg_create_app_t* request );

#endif /* _APPLICATION_H_ */
