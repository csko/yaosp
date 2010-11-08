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
#include <ygui++/protocol.hpp>

namespace yguipp {

Window::Window(const std::string& title, const Point& position, const Point& size, int flags, WindowOrder order)
    : m_title(title), m_position(position), m_size(size), m_flags(flags), m_order(order),
      m_visible(false), m_mouseWidget(NULL), m_mouseDownWidget(NULL), m_focusedWidget(NULL) {
    m_container = new Panel();
    m_container->setWindow(this);
    m_container->setPosition( Point(0,0) );
    m_container->setSize(m_size);

    m_renderTable = new IPCRenderTable(this);
    m_graphicsContext = new GraphicsContext(this);
}

Window::~Window( void ) {
    // todo: release m_container
    delete m_renderTable;
    delete m_graphicsContext;
}

bool Window::init( void ) {
    return registerWindow();
}

void Window::addWindowListener(WindowListener* windowListener) {
    for (WindowListener::ListCIter it = m_windowListeners.begin();
         it != m_windowListeners.end();
         ++it) {
        if (*it == windowListener) {
            return;
        }
    }

    m_windowListeners.push_back(windowListener);
}

void Window::removeWindowListener(WindowListener* windowListener) {
    for (WindowListener::ListIter it = m_windowListeners.begin();
         it != m_windowListeners.end();
         ++it) {
        if (*it == windowListener) {
            m_windowListeners.erase(it);
            return;
        }
    }
}

const Point& Window::getSize( void ) {
    return m_size;
}

const Point& Window::getPosition( void ) {
    return m_position;
}

Widget* Window::getContainer( void ) {
    return m_container;
}

RenderTable* Window::getRenderTable( void ) {
    return m_renderTable;
}

void Window::show( void ) {
    WinHeader request;
    request.m_windowId = m_id;

    Application::getInstance()->getClientPort()->send(Y_WINDOW_SHOW, reinterpret_cast<void*>(&request), sizeof(request));
}

void Window::hide( void ) {
    WinHeader request;
    request.m_windowId = m_id;

    Application::getInstance()->getClientPort()->send(Y_WINDOW_HIDE, reinterpret_cast<void*>(&request), sizeof(request));
}

void Window::resize( const Point& size ) {
    WinResize request;
    request.m_header.m_windowId = m_id;
    request.m_size = size;

    Application* app = Application::getInstance();
    app->getClientPort()->send(Y_WINDOW_DO_RESIZE, reinterpret_cast<void*>(&request), sizeof(request));

    /* If this is the event dispatcher thread then wait until we receive the acknowledgement message to
       our resize request, so the internal structures of the Window class will be updated properly. */
    if (app->isEventDispatchThread()) {
        Application::Message msg;

        do {
            if (app->receiveMessage(msg)) {
                app->handleMessage(msg);
            }
        } while (msg.m_code != Y_WINDOW_RESIZED);
    }
}

void Window::moveTo( const Point& position ) {
    WinMoveTo request;
    request.m_header.m_windowId = m_id;
    request.m_position = position;

    Application* app = Application::getInstance();
    app->getClientPort()->send(Y_WINDOW_DO_MOVETO, reinterpret_cast<void*>(&request), sizeof(request));

    /* If this is the event dispatcher thread then wait until we receive the acknowledgement message to
       our moveTo request, so the internal structures of the Window class will be updated properly. */
    if (app->isEventDispatchThread()) {
        Application::Message msg;

        do {
            if (app->receiveMessage(msg)) {
                app->handleMessage(msg);
            }
        } while (msg.m_code != Y_WINDOW_MOVEDTO);
    }
}

int Window::handleMessage(const Application::Message& msg) {
    switch (msg.m_code) {
        case Y_WINDOW_SHOW :
            if (m_visible) {
                break;
            }

            m_mouseWidget = NULL;
            m_mouseDownWidget = NULL;
            doRepaint(true);
            Application::getInstance()->getServerPort()->send(Y_WINDOW_SHOW, msg.m_buffer, msg.m_size);

            m_visible = true;

            break;

        case Y_WINDOW_HIDE :
            if (!m_visible) {
                break;
            }

            Application::getInstance()->getServerPort()->send(Y_WINDOW_HIDE, msg.m_buffer, msg.m_size);
            m_mouseWidget = NULL;
            m_mouseDownWidget = NULL;

            m_visible = false;

            break;

        case Y_WINDOW_WIDGET_INVALIDATED :
            doRepaint();
            break;

        case Y_WINDOW_CLOSE_REQUEST : {
            Application* app;

            app = Application::getInstance();
            app->getServerPort()->send(Y_WINDOW_DESTROY, msg.m_buffer, msg.m_size);
            app->unregisterWindow(m_id);
            decRef();

            break;
        }

        case Y_WINDOW_DO_RESIZE :
            Application::getInstance()->getServerPort()->send(Y_WINDOW_DO_RESIZE, msg.m_buffer, msg.m_size);
            break;

        case Y_WINDOW_RESIZED : {
            const WinResize* reply = reinterpret_cast<const WinResize*>(msg.m_buffer);
            m_size = reply->m_size;
            m_container->setSize(m_size);

            if (m_visible) {
                doRepaint(true);
            }

            break;
        }

        case Y_WINDOW_DO_MOVETO :
            Application::getInstance()->getServerPort()->send(Y_WINDOW_DO_MOVETO, msg.m_buffer, msg.m_size);
            break;

        case Y_WINDOW_MOVEDTO :
            m_position = reinterpret_cast<const WinMoveTo*>(msg.m_buffer)->m_position;
            break;

        case Y_WINDOW_KEY_PRESSED :
            handleKeyPressed(reinterpret_cast<const WinKeyPressed*>(msg.m_buffer)->m_key);
            break;

        case Y_WINDOW_KEY_RELEASED :
            handleKeyReleased(reinterpret_cast<const WinKeyReleased*>(msg.m_buffer)->m_key);
            break;

        case Y_WINDOW_MOUSE_ENTERED :
            handleMouseEntered(reinterpret_cast<const WinMouseEntered*>(msg.m_buffer)->m_position);
            break;

        case Y_WINDOW_MOUSE_MOVED :
            handleMouseMoved(reinterpret_cast<const WinMouseEntered*>(msg.m_buffer)->m_position);
            break;

        case Y_WINDOW_MOUSE_EXITED :
            handleMouseExited();
            break;

        case Y_WINDOW_MOUSE_PRESSED : {
            const WinMousePressed* cmd = reinterpret_cast<const WinMousePressed*>(msg.m_buffer);
            handleMousePressed(cmd->m_position, cmd->m_button);
            break;
        }

        case Y_WINDOW_MOUSE_RELEASED :
            handleMouseReleased(reinterpret_cast<const WinMouseReleased*>(msg.m_buffer)->m_button);
            break;

        case Y_WINDOW_ACTIVATED :
            handleWindowActivated(reinterpret_cast<const WinActivated*>(msg.m_buffer)->m_reason);
            break;

        case Y_WINDOW_DEACTIVATED :
            handleWindowDeActivated(reinterpret_cast<const WinDeActivated*>(msg.m_buffer)->m_reason);
            break;
    }

    return 0;
}

bool Window::registerWindow( void ) {
    uint8_t* data;
    uint32_t code;
    size_t dataSize;
    Application* app;
    WinCreate* request;
    WinCreateReply reply;

    app = Application::getInstance();

    dataSize = sizeof(WinCreate) + m_title.size() + 1;
    data = new uint8_t[dataSize];
    request = reinterpret_cast<WinCreate*>(data);

    request->m_replyPort = app->getReplyPort()->getId();
    request->m_position = m_position;
    request->m_size = m_size;
    request->m_order = m_order;
    request->m_flags = m_flags;
    memcpy( reinterpret_cast<void*>(request + 1), m_title.c_str(), m_title.size() + 1 );

    app->lock();
    app->getServerPort()->send( Y_WINDOW_CREATE, reinterpret_cast<void*>(data), dataSize );
    app->getReplyPort()->receive( code, reinterpret_cast<void*>(&reply), sizeof(reply) );
    app->unLock();

    m_id = reply.m_windowId;

    app->registerWindow(m_id, this);

    return true;
}

void Window::doRepaint( bool forced ) {
    m_graphicsContext->pushRestrictedArea( Rect(m_size) );
    m_container->doPaint( m_graphicsContext, forced );

    if (m_graphicsContext->needToFlush()) {
        m_graphicsContext->finish();
        m_renderTable->flush();
        m_graphicsContext->cleanUp();
        m_renderTable->waitForFlush();
    } else {
        m_renderTable->reset();
    }
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

Point Window::getWidgetPosition( Widget* widget, Point p ) {
    while (widget != NULL) {
        p -= widget->getPosition();
        widget = widget->getParent();
    }

    return p;
}

int Window::handleKeyPressed(int key) {
    if (m_focusedWidget != NULL) {
         m_focusedWidget->keyPressed(key);
    }

    return 0;
}

int Window::handleKeyReleased(int key) {
    if (m_focusedWidget != NULL) {
        m_focusedWidget->keyReleased(key);
    }

    return 0;
}

int Window::handleMouseEntered(const yguipp::Point& position) {
    assert(m_mouseWidget == NULL);
    m_mouseWidget = findWidgetAt(position);
    assert(m_mouseWidget != NULL);

    m_mouseWidget->mouseEntered(getWidgetPosition(m_mouseWidget, position));

    return 0;
}

int Window::handleMouseMoved(const yguipp::Point& position) {
    Widget* mouseWidget;

    assert(m_mouseWidget != NULL);
    mouseWidget = findWidgetAt(position);
    assert(mouseWidget != NULL);

    if (mouseWidget == m_mouseWidget) {
        m_mouseWidget->mouseMoved(getWidgetPosition(m_mouseWidget, position));
    } else {
        m_mouseWidget->mouseExited();
        m_mouseWidget = mouseWidget;
        m_mouseWidget->mouseEntered(getWidgetPosition(m_mouseWidget, position));
    }

    return 0;
}

int Window::handleMouseExited(void) {
    assert(m_mouseWidget != NULL);
    m_mouseWidget->mouseExited();
    m_mouseWidget = NULL;

    return 0;
}

int Window::handleMousePressed(const yguipp::Point& position, int button) {
    assert(m_mouseWidget != NULL);
    assert(m_mouseDownWidget == NULL);

    if (m_focusedWidget != m_mouseWidget) {
        // todo: m_focusedWidget lost focus
        m_focusedWidget = m_mouseWidget;
        // todo: m_focusedWidget received focus
    }

    m_mouseDownWidget = m_mouseWidget;
    m_mouseDownWidget->mousePressed(getWidgetPosition(m_mouseDownWidget, position), button);

    return 0;
}

int Window::handleMouseReleased(int button) {
    if (m_mouseDownWidget != NULL) {
        m_mouseDownWidget->mouseReleased(button);
        m_mouseDownWidget = NULL;
    }

    return 0;
}

int Window::handleWindowActivated(yguipp::ActivationReason reason) {
    WindowListener::List tmpList = m_windowListeners;

    for (WindowListener::ListCIter it = tmpList.begin();
         it != tmpList.end();
         ++it) {
        (*it)->windowActivated(this, reason);
    }

    return 0;
}

int Window::handleWindowDeActivated(yguipp::DeActivationReason reason) {
    WindowListener::List tmpList = m_windowListeners;

    for (WindowListener::ListCIter it = tmpList.begin();
         it != tmpList.end();
         ++it) {
        (*it)->windowDeActivated(this, reason);
    }

    return 0;
}

} /* namespace yguipp */
