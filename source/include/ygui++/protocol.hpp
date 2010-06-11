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
#include <yaosp/region.h>

#include <ygui++/point.hpp>
#include <ygui++/yconstants.hpp>

enum {
    Y_APPLICATION_CREATE = 1000000,
    Y_WINDOW_CREATE,
    Y_WINDOW_SHOW,
    Y_WINDOW_HIDE,
    Y_WINDOW_RENDER,
    Y_FONT_CREATE,
    Y_FONT_STRING_WIDTH,
    Y_BITMAP_CREATE
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

struct FontCreate {
    ipc_port_id m_replyPort;
    yguipp::FontInfo m_fontInfo;
    /* family */
    /* style */
};

struct FontCreateReply {
    int m_fontHandle;
    int m_ascender;
    int m_descender;
    int m_lineGap;
};

struct FontStringWidth {
    ipc_port_id m_replyPort;
    int m_fontHandle;
    int m_length;
    /* text */
};

struct FontStringWidthReply {
    int m_width;
};

struct BitmapCreate {
    ipc_port_id m_replyPort;
    yguipp::Point m_size;
    yguipp::ColorSpace m_colorSpace;
};

struct BitmapCreateReply {
    int m_bitmapHandle;
    region_id m_bitmapRegion;
};

#endif /* _YGUI_PROTOCOL_HPP_ */
