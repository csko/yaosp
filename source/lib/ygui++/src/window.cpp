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
#include <assert.h>
#include <iostream>

#include <ygui++/window.hpp>
#include <ygui++/application.hpp>
#include <ygui++/panel.hpp>
#include <ygui++/ipcrendertable.hpp>

namespace yguipp {

Window::Window( const std::string& title, const Point& position,
                const Point& size ) : IPCListener("window"), m_title(title), m_position(position), m_size(size),
                                      m_replyPort(NULL), m_mouseWidget(NULL) {
    m_container = new Panel();
    m_container->setWindow(this);
    m_container->setPosition( Point(0,0) );
    m_container->setSize(m_size);

    m_renderTable = new IPCRenderTable(this);
    m_graphicsContext = new GraphicsContext(this);
}

Window::~Window( void ) {
    delete m_renderTable;
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
        case MSG_WIDGET_INVALIDATED :
            doRepaint();
            break;

        case MSG_WINDOW_DO_SHOW :
            m_mouseWidget = NULL;
            m_mouseDownWidget = NULL;
            doRepaint();
            m_serverPort->send( MSG_WINDOW_SHOW );
            break;

        case MSG_WINDOW_RESIZED :
            handleResized( reinterpret_cast<msg_win_resized_t*>(buffer) );
            break;

        case MSG_KEY_PRESSED :
            handleKeyPressed( reinterpret_cast<msg_key_pressed_t*>(buffer) );
            break;

        case MSG_KEY_RELEASED :
            handleKeyReleased( reinterpret_cast<msg_key_released_t*>(buffer) );
            break;

        case MSG_MOUSE_ENTERED :
            handleMouseEntered( reinterpret_cast<msg_mouse_entered_t*>(buffer) );
            break;

        case MSG_MOUSE_MOVED :
            handleMouseMoved( reinterpret_cast<msg_mouse_moved_t*>(buffer) );
            break;

        case MSG_MOUSE_EXITED :
            handleMouseExited();
            break;

        case MSG_MOUSE_PRESSED :
            handleMousePressed( reinterpret_cast<msg_mouse_pressed_t*>(buffer) );
            break;

        case MSG_MOUSE_RELEASED :
            handleMouseReleased( reinterpret_cast<msg_mouse_released_t*>(buffer) );
            break;

        case MSG_MOUSE_SCROLLED :
            handleMouseScrolled( reinterpret_cast<msg_mouse_scrolled_t*>(buffer) );
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

void Window::doRepaint( void ) {
    m_graphicsContext->pushRestrictedArea( Rect(m_size) );
    m_container->doPaint( m_graphicsContext );

    if ( m_graphicsContext->needToFlush() ) {
        m_graphicsContext->finish();
        m_renderTable->flush();
        m_graphicsContext->cleanUp();
        m_renderTable->waitForFlush();
    } else {
        m_renderTable->reset();
    }
}

void Window::handleResized( msg_win_resized_t* cmd ) {
    m_size = Point(&cmd->size);

    m_container->setSize(m_size);
    m_container->doInvalidate(false);
    doRepaint();
}

void Window::handleKeyPressed( msg_key_pressed_t* cmd ) {
}

void Window::handleKeyReleased( msg_key_released_t* cmd ) {
}

void Window::handleMouseEntered( msg_mouse_entered_t* cmd ) {
    assert( m_mouseWidget == NULL );
    m_mouseWidget = findWidgetAt( Point(&cmd->mouse_position) );

    assert( m_mouseWidget != NULL );
    m_mouseWidget->mouseEntered( Point() );
}

void Window::handleMouseMoved( msg_mouse_moved_t* cmd ) {
    Widget* newMouseWidget;

    assert( m_mouseWidget != NULL );
    newMouseWidget = findWidgetAt( Point(&cmd->mouse_position) );
    assert( newMouseWidget != NULL );

    if ( m_mouseWidget == newMouseWidget ) {
        m_mouseWidget->mouseMoved( Point() );
    } else {
        m_mouseWidget->mouseExited();
        m_mouseWidget = newMouseWidget;
        m_mouseWidget->mouseEntered( Point() );
    }
}

void Window::handleMouseExited( void ) {
    assert( m_mouseWidget != NULL );
    m_mouseWidget->mouseExited();
    m_mouseWidget = NULL;
}

void Window::handleMousePressed( msg_mouse_pressed_t* cmd ) {
    assert( m_mouseWidget != NULL );
    assert( m_mouseDownWidget == NULL );

    m_mouseDownWidget = m_mouseWidget;
    m_mouseDownWidget->mousePressed( Point() );
}

void Window::handleMouseReleased( msg_mouse_released_t* cmd ) {
    assert( m_mouseDownWidget != NULL );

    m_mouseDownWidget->mouseReleased();
    m_mouseDownWidget = NULL;
}

void Window::handleMouseScrolled( msg_mouse_scrolled_t* cmd ) {
}

Widget* Window::findWidgetAt( const Point& p ) {
    return findWidgetAtHelper( m_container, p, Point(0,0), Rect(m_size) );
}

Widget* Window::findWidgetAtHelper( Widget* widget, const Point& position,
                                    Point leftTop, const Rect& visibleRect ) {
    Rect widgetRect;

    widgetRect = widget->getBounds();
    widgetRect += leftTop;
    widgetRect &= visibleRect;

    if ( ( !widgetRect.isValid() ) ||
         ( !widgetRect.hasPoint(position) ) ) {
        return NULL;
    }

    const Widget::ChildVector& children = widget->getChildren();

    for ( Widget::ChildVectorCIter it = children.begin();
          it != children.end();
          ++it ) {
        Rect newVisibleRect;
        Widget* result;
        Widget* child = it->first;
        const Point& childPosition = child->getPosition();
        const Point& childVisibleSize = child->getVisibleSize();
        const Point& childScrollOffset = child->getScrollOffset();

        leftTop += childPosition;
        leftTop += childScrollOffset;

        newVisibleRect.m_left = visibleRect.m_left + childPosition.m_x;
        newVisibleRect.m_top = visibleRect.m_top + childPosition.m_y;
        newVisibleRect.m_right = std::min( visibleRect.m_right, newVisibleRect.m_left + childVisibleSize.m_x - 1 );
        newVisibleRect.m_bottom = std::min( visibleRect.m_bottom, newVisibleRect.m_top + childVisibleSize.m_y - 1 );

        result = findWidgetAtHelper( child, position, leftTop, newVisibleRect );

        if ( result != NULL ) {
            return result;
        }

        leftTop -= childScrollOffset;
        leftTop -= childPosition;
    }

    return widget;
}

} /* namespace yguipp */
