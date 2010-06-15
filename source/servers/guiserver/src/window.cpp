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

#include <yaosp/debug.h>

#include <guiserver/window.hpp>
#include <guiserver/guiserver.hpp>
#include <guiserver/application.hpp>

Window::Window( GuiServer* guiServer, Application* application ) : m_id(-1), m_visible(false), m_mouseOnDecorator(false),
                                                                   m_drawingMode(yguipp::DM_COPY),
                                                                   m_font(NULL), m_guiServer(guiServer),
                                                                   m_application(application) {
}

Window::~Window( void ) {
}

bool Window::init( WinCreate* request ) {
    Decorator* decorator = m_guiServer->getWindowManager()->getDecorator();

    m_order = request->m_order;
    m_flags = request->m_flags;
    m_title = reinterpret_cast<char*>(request + 1);

    calculateWindowRects(request->m_position, request->m_size, m_screenRect, m_clientRect);

    if ((m_screenRect.width() != 0) &&
        (m_screenRect.height() != 0)) {
        m_bitmap = new Bitmap(m_screenRect.width(), m_screenRect.height(), yguipp::CS_RGB32);
    }

    m_decoratorData = decorator->createWindowData();

    return true;
}

int Window::handleMessage( uint32_t code, void* data, size_t size ) {
    switch ( code ) {
        case Y_WINDOW_SHOW :
            if (!m_visible) {
                m_guiServer->getWindowManager()->registerWindow(this);
                m_visible = true;
            }
            break;

        case Y_WINDOW_HIDE :
            if (m_visible) {
                m_guiServer->getWindowManager()->unregisterWindow(this);
                m_visible = false;
            }
            break;
            
        case Y_WINDOW_DO_RESIZE :
            handleDoResize(reinterpret_cast<WinResize*>(data));
            break;

        case Y_WINDOW_DO_MOVETO :
            handleDoMoveTo(reinterpret_cast<WinMoveTo*>(data));
            break;

        case Y_WINDOW_RENDER :
            handleRender(
                reinterpret_cast<uint8_t*>(data) + sizeof(WinHeader),
                size - sizeof(WinHeader)
            );

            break;
    }

    return 0;
}

int Window::mouseEntered(const yguipp::Point& position) {
    m_mouseOnDecorator = !m_clientRect.hasPoint(position);

    if (m_mouseOnDecorator) {
    } else {
        WinMouseEntered cmd;
        cmd.m_header.m_windowId = m_id;
        cmd.m_position = position - m_clientRect.leftTop();

        m_application->getClientPort()->send(Y_WINDOW_MOUSE_ENTERED, reinterpret_cast<void*>(&cmd), sizeof(cmd));
    }

    return 0;
}

int Window::mouseMoved(const yguipp::Point& position) {
    bool mouseOnDecorator = !m_clientRect.hasPoint(position);

    if (m_mouseOnDecorator) {
        if (mouseOnDecorator) {
            // moved on decorator
        } else {
            // exited from decorator

            WinMouseEntered cmd;
            cmd.m_header.m_windowId = m_id;
            cmd.m_position = position - m_clientRect.leftTop();

            m_application->getClientPort()->send(Y_WINDOW_MOUSE_ENTERED, reinterpret_cast<void*>(&cmd), sizeof(cmd));
        }
    } else {
        if (mouseOnDecorator) {
            WinMouseExited cmd;
            cmd.m_header.m_windowId = m_id;

            m_application->getClientPort()->send(Y_WINDOW_MOUSE_EXITED, reinterpret_cast<void*>(&cmd), sizeof(cmd));

            // entered to decorator
        } else {
            WinMouseMoved cmd;
            cmd.m_header.m_windowId = m_id;
            cmd.m_position = position - m_clientRect.leftTop();

            m_application->getClientPort()->send(Y_WINDOW_MOUSE_MOVED, reinterpret_cast<void*>(&cmd), sizeof(cmd));
        }
    }

    m_mouseOnDecorator = mouseOnDecorator;

    return 0;
}

int Window::mouseExited(void) {
    if (m_mouseOnDecorator) {
    } else {
        WinMouseExited cmd;
        cmd.m_header.m_windowId = m_id;

        m_application->getClientPort()->send(Y_WINDOW_MOUSE_EXITED, reinterpret_cast<void*>(&cmd), sizeof(cmd));
    }

    return 0;
}

int Window::mousePressed(const yguipp::Point& position, int button) {
    if (m_mouseOnDecorator) {
        // todo
    } else {
        WinMousePressed cmd;
        cmd.m_header.m_windowId = m_id;
        cmd.m_position = position - m_clientRect.leftTop();
        cmd.m_button = button;

        m_application->getClientPort()->send(Y_WINDOW_MOUSE_PRESSED, reinterpret_cast<void*>(&cmd), sizeof(cmd));
    }

    return 0;
}

int Window::mouseReleased(int button) {
    if (m_mouseOnDecorator) {
        // todo
    } else {
        WinMouseReleased cmd;
        cmd.m_header.m_windowId = m_id;
        cmd.m_button = button;

        m_application->getClientPort()->send(Y_WINDOW_MOUSE_RELEASED, reinterpret_cast<void*>(&cmd), sizeof(cmd));
    }

    return 0;
}

int Window::activated(void) {
    WinHeader cmd;
    cmd.m_windowId = m_id;

    m_application->getClientPort()->send(Y_WINDOW_ACTIVATED, reinterpret_cast<void*>(&cmd), sizeof(cmd));

    return 0;
}

int Window::deactivated(void) {
    WinHeader cmd;
    cmd.m_windowId = m_id;

    m_application->getClientPort()->send(Y_WINDOW_DEACTIVATED, reinterpret_cast<void*>(&cmd), sizeof(cmd));

    return 0;
}

Window* Window::createFrom( GuiServer* guiServer, Application* application, WinCreate* request ) {
    Window* window = new Window(guiServer, application);
    window->init(request);
    return window;
}

void Window::handleDoResize( WinResize* request ) {
    delete m_bitmap;
    calculateWindowRects(m_screenRect.leftTop(), request->m_size, m_screenRect, m_clientRect);
    m_bitmap = new Bitmap(m_screenRect.width(), m_screenRect.height(), yguipp::CS_RGB32);

    m_application->getClientPort()->send(Y_WINDOW_RESIZED, request, sizeof(WinResize));
}

void Window::handleDoMoveTo( WinMoveTo* request ) {
    calculateWindowRects(request->m_position, m_clientRect.size(), m_screenRect, m_clientRect);
    m_application->getClientPort()->send(Y_WINDOW_MOVEDTO, request, sizeof(WinMoveTo));
}

void Window::calculateWindowRects( const yguipp::Point& position, const yguipp::Point& size,
                                   yguipp::Rect& screenRect, yguipp::Rect& clientRect ) {
    if (m_flags & yguipp::WINDOW_NO_BORDER) {
        m_screenRect = yguipp::Rect(size);
        m_screenRect += position;
        m_clientRect = m_screenRect;
    } else {
        Decorator* decorator = m_guiServer->getWindowManager()->getDecorator();

        m_screenRect = yguipp::Rect(size + decorator->getSize());
        m_screenRect += position;

        m_clientRect = yguipp::Rect(size);
        m_clientRect += position;
        m_clientRect += decorator->leftTop();
    }
}
