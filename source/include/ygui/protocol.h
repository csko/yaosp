/* yaosp GUI library
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

/*
 * This file described the protocol between the guiserver and the applications.
 */

#ifndef _YGUI_PROTOCOL_H_
#define _YGUI_PROTOCOL_H_

#include <yaosp/ipc.h>

#include <ygui/point.h>
#include <ygui/rect.h>

enum {
    MSG_CREATE_APPLICATION = 1,
    MSG_CREATE_WINDOW,
    MSG_SHOW_WINDOW,
    MSG_HIDE_WINDOW
};

/* MSG_CREATE_APPLICATION request and reply */

typedef struct msg_create_app {
    ipc_port_id reply_port;
    ipc_port_id client_port;
} msg_create_app_t;

typedef struct msg_create_app_reply {
    ipc_port_id server_port;
} msg_create_app_reply_t;

/* MSG_CREATE_WINDOW request and reply */

/* NOTE: The title of the window is added to the message after this structure */
typedef struct msg_create_win {
    ipc_port_id reply_port;
    ipc_port_id client_port;
    point_t position;
    point_t size;
    int flags;
} msg_create_win_t;

typedef struct msg_create_win_reply {
    ipc_port_id server_port;
} msg_create_win_reply_t;

#endif /* _YGUI_PROTOCOL_H_ */
