/* yaosp GUI library
 *
 * Copyright (c) 2010 Zoltan Kovacs
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

#ifndef _YGUI_PROTOCOL_HPP_
#define _YGUI_PROTOCOL_HPP_

#include <yaosp/ipc.h>

#include <ygui++/point.hpp>

enum {
    Y_APPLICATION_CREATE = 1000000,
    Y_WINDOW_CREATE,
    Y_WINDOW_SHOW,
    Y_WINDOW_HIDE,
    Y_WINDOW_RENDER
};

struct AppCreate {
    ipc_port_id m_replyPort;
    ipc_port_id m_clientPort;
    int m_flags;
};

struct AppCreateReply {
    ipc_port_id m_serverPort;
};

struct WinCreate {
    ipc_port_id m_replyPort;
    yguipp::Point m_position;
    yguipp::Point m_size;
    int m_order;
    int m_flags;
    /* title */
};

struct WinCreateReply {
    int m_windowId;
};

struct WinHeader {
    int m_windowId;
};

struct WinShow {
    WinHeader m_header;
};

#endif /* _YGUI_PROTOCOL_HPP_ */
