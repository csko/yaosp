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
    Y_APPLICATION_DESTROY,
    Y_WINDOW_CREATE,
    Y_WINDOW_DESTROY,
    Y_WINDOW_SHOW,
    Y_WINDOW_HIDE,
    Y_WINDOW_RENDER,
    Y_WINDOW_DO_RESIZE,
    Y_WINDOW_RESIZED,
    Y_WINDOW_DO_MOVETO,
    Y_WINDOW_MOVEDTO,
    Y_WINDOW_KEY_PRESSED,
    Y_WINDOW_KEY_RELEASED,
    Y_WINDOW_MOUSE_ENTERED,
    Y_WINDOW_MOUSE_MOVED,
    Y_WINDOW_MOUSE_EXITED,
    Y_WINDOW_MOUSE_PRESSED,
    Y_WINDOW_MOUSE_RELEASED,
    Y_WINDOW_ACTIVATED,
    Y_WINDOW_DEACTIVATED,
    Y_WINDOW_WIDGET_INVALIDATED,
    Y_WINDOW_CLOSE_REQUEST,
    Y_FONT_CREATE,
    Y_FONT_STRING_WIDTH,
    Y_BITMAP_CREATE,
    Y_DESKTOP_GET_SIZE,
    Y_SCREEN_MODE_GET_LIST,
    Y_SCREEN_MODE_SET,
    Y_SCREEN_MODE_CHANGED
};

struct AppCreate {
    AppCreate(ipc_port_id replyPort, ipc_port_id clientPort, int flags);

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

struct WinResize {
    WinHeader m_header;
    yguipp::Point m_size;
};

struct WinMoveTo {
    WinHeader m_header;
    yguipp::Point m_position;
};

struct WinActivated {
    WinHeader m_header;
    yguipp::ActivationReason m_reason;
};

struct WinDeActivated {
    WinHeader m_header;
    yguipp::DeActivationReason m_reason;
};

struct WinKeyPressed {
    WinHeader m_header;
    int m_key;
};

struct WinKeyReleased {
    WinHeader m_header;
    int m_key;
};

struct WinMouseEntered {
    WinHeader m_header;
    yguipp::Point m_position;
};

struct WinMouseMoved {
    WinHeader m_header;
    yguipp::Point m_position;
};

struct WinMouseExited {
    WinHeader m_header;
};

struct WinMousePressed {
    WinHeader m_header;
    yguipp::Point m_position;
    int m_button;
};

struct WinMouseReleased {
    WinHeader m_header;
    int m_button;
};

struct FontCreate {
    ipc_port_id m_replyPort;
    int m_pointSize;
    /* family */
    /* style */
};

struct FontCreateReply {
    int m_fontHandle;
    int m_ascender;
    int m_descender;
    int m_height;
};

struct FontStringWidth {
    ipc_port_id m_replyPort;
    int m_fontHandle;
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

struct DesktopGetSize {
    ipc_port_id m_replyPort;
    int m_desktopIndex;
};

struct ScreenModeGetList {
    ipc_port_id m_replyPort;
};

struct ScreenModeSet {
    ipc_port_id m_replyPort;
    yguipp::ScreenModeInfo m_modeInfo;
};

#endif /* _YGUI_PROTOCOL_HPP_ */
