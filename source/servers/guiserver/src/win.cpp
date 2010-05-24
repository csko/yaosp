/* GUI server
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

#include <guiserver/window.hpp>

Window::Window( void ) {
}

Window::~Window( void ) {
}

bool Window::init( WinCreate* request ) {
    return true;
}

int Window::handleMessage( uint32_t code, void* data, size_t size ) {
    switch ( code ) {
        case Y_WINDOW_SHOW :
            break;
    }

    return 0;
}

Window* Window::createFrom( WinCreate* request ) {
    Window* win = new Window();
    win->init(request);
    return win;
}
