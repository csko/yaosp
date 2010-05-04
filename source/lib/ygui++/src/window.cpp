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

#include <string.h>
#include <ygui/protocol.h>

#include <ygui++/window.hpp>
#include <ygui++/application.hpp>

namespace yguipp {

Window::Window( const std::string& title ) : IPCListener("window"), m_title(title), m_replyPort(NULL) {
}

Window::~Window( void ) {
}

bool Window::init( void ) {
    IPCListener::init();

    m_replyPort = new yutilpp::IPCPort();
    m_replyPort->createNew();

    start();

    return registerWindow();
}

void Window::show( void ) {
}

int Window::ipcDataAvailable( uint32_t code, void* buffer, size_t size ) {
    return 0;
}

bool Window::registerWindow( void ) {
    uint8_t* data;
    size_t dataSize;
    msg_create_win_t* request;

    dataSize = sizeof(msg_create_win_t) + m_title.size() + 1;
    data = new uint8_t[dataSize];
    request = reinterpret_cast<msg_create_win_t*>(data);

    request->reply_port = m_replyPort->getId();
    request->client_port = getPort()->getId();
    request->position.x = 0;
    request->position.y = 0;
    request->size.x = 100;
    request->size.y = 100;
    request->order = W_ORDER_NORMAL;
    request->flags = 0;

    memcpy( reinterpret_cast<void*>(request + 1), m_title.c_str(), m_title.size() + 1 );

    Application::getInstance()->getGuiServerPort()->send( MSG_WINDOW_CREATE, reinterpret_cast<void*>(data), dataSize );

    return true;
}

} /* namespace yguipp */
