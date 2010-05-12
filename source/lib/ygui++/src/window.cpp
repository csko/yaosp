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

#include <iostream>

#include <string.h>
#include <ygui/protocol.h>

#include <ygui++/window.hpp>
#include <ygui++/application.hpp>
#include <ygui++/panel.hpp>
#include <ygui++/ipcrendertable.hpp>

namespace yguipp {

Window::Window( const std::string& title, const Point& position, const Point& size ) : IPCListener("window"), m_title(title),
                                                                                       m_position(position), m_size(size),
                                                                                       m_replyPort(NULL) {
    m_container = new Panel();
    m_container->setWindow(this);
    m_container->setPosition( Point(0,0) );
    m_container->setSize(m_size);

    m_renderTable = new IPCRenderTable(this);
    m_graphicsContext = new GraphicsContext(this);
}

Window::~Window( void ) {
    delete m_graphicsContext;
}

bool Window::init( void ) {
    IPCListener::init();

    m_serverPort = new yutilpp::IPCPort();
    m_replyPort = new yutilpp::IPCPort();
    m_replyPort->createNew();

    start();

    return registerWindow();
}

Widget* Window::getContainer( void ) {
    return m_container;
}

RenderTable* Window::getRenderTable( void ) {
    return m_renderTable;
}

yutilpp::IPCPort* Window::getServerPort( void ) {
    return m_serverPort;
}

yutilpp::IPCPort* Window::getReplyPort( void ) {
    return m_replyPort;
}

void Window::show( void ) {
    getPort()->send( MSG_WINDOW_DO_SHOW );
}

int Window::ipcDataAvailable( uint32_t code, void* buffer, size_t size ) {
    switch ( code ) {
        case MSG_WINDOW_DO_SHOW :
            m_graphicsContext->pushRestrictedArea( Rect(m_size) );
            m_container->doPaint( m_graphicsContext );
            m_graphicsContext->flush();

            m_serverPort->send( MSG_WINDOW_SHOW );

            break;
    }

    return 0;
}

bool Window::registerWindow( void ) {
    uint8_t* data;
    uint32_t code;
    size_t dataSize;
    msg_create_win_t* request;
    msg_create_win_reply_t reply;

    dataSize = sizeof(msg_create_win_t) + m_title.size() + 1;
    data = new uint8_t[dataSize];
    request = reinterpret_cast<msg_create_win_t*>(data);

    request->reply_port = m_replyPort->getId();
    request->client_port = getPort()->getId();
    request->position.x = m_position.m_x;
    request->position.y = m_position.m_y;
    request->size.x = m_size.m_x;
    request->size.y = m_size.m_y;
    request->order = W_ORDER_NORMAL;
    request->flags = 0;

    memcpy( reinterpret_cast<void*>(request + 1), m_title.c_str(), m_title.size() + 1 );

    Application::getInstance()->getServerPort()->send( MSG_WINDOW_CREATE, reinterpret_cast<void*>(data), dataSize );
    m_replyPort->receive( code, reinterpret_cast<void*>(&reply), sizeof(msg_create_win_reply_t) );

    m_serverPort->createFromExisting(reply.server_port);

    return true;
}

} /* namespace yguipp */
