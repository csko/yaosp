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
#include <guiserver/guiserver.hpp>
#include <guiserver/application.hpp>
#include <guiserver/windowmanager.hpp>

Window::Window( GuiServer* guiServer, Application* application ) : m_id(-1), m_visible(false), m_bitmap(NULL),
                                                                   m_mouseOnDecorator(false),
                                                                   m_mousePressedOnDecorator(false), m_moving(false),
                                                                   m_resizing(false), m_guiServer(guiServer),
                                                                   m_application(application),
                                                                   m_rendering(false){
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
    switch (code) {
        case Y_WINDOW_SHOW :
            registerWindow();
            break;

        case Y_WINDOW_HIDE :
            unregisterWindow();
            break;

        case Y_WINDOW_DO_RESIZE :
            resize(reinterpret_cast<WinResize*>(data)->m_size);
            break;

        case Y_WINDOW_DO_MOVETO :
            moveTo(reinterpret_cast<WinMoveTo*>(data)->m_position);
            break;

        case Y_WINDOW_RENDER : {
            if (!m_rendering) {
                m_renderContext.init(m_bitmap);
                m_rendering = true;
            }

            bool renderingFinished = handleRender(
                reinterpret_cast<uint8_t*>(data) + sizeof(WinHeader),
                size - sizeof(WinHeader)
            );

            if (renderingFinished) {
                m_renderContext.finish();
                m_rendering = false;
            }

            break;
        }
    }

    return 0;
}

int Window::moveTo(const yguipp::Point& p) {
    WindowManager* windowManager = m_guiServer->getWindowManager();

    if (m_visible) {
        windowManager->lock();
    }

    doMoveTo(p);

    if (m_visible) {
        windowManager->unLock();
    }

    return 0;
}

int Window::doMoveTo(const yguipp::Point& p) {
    yguipp::Rect oldScreenRect = m_screenRect;

    calculateWindowRects(p, m_clientRect.size(), m_screenRect, m_clientRect);

    if (m_visible) {
        m_guiServer->getWindowManager()->windowRectChanged(this, oldScreenRect, m_screenRect);
    }

    WinMoveTo reply;
    reply.m_header.m_windowId = m_id;
    reply.m_position = p;

    m_application->getClientPort()->send(Y_WINDOW_MOVEDTO, reinterpret_cast<void*>(&reply), sizeof(reply));

    return 0;
}

int Window::resize(const yguipp::Point& p) {
    WindowManager* windowManager = m_guiServer->getWindowManager();

    if (m_visible) {
        windowManager->lock();
    }

    doResize(p);

    if (m_visible) {
        windowManager->unLock();
    }

    return 0;
}

int Window::doResize(const yguipp::Point& p) {
    yguipp::Rect oldScreenRect = m_screenRect;
    calculateWindowRects(m_screenRect.leftTop(), p, m_screenRect, m_clientRect);

    delete m_bitmap;
    m_bitmap = new Bitmap(m_screenRect.width(), m_screenRect.height(), yguipp::CS_RGB32);

    if (m_visible) {
        m_guiServer->getWindowManager()->windowRectChanged(this, oldScreenRect, m_screenRect);
    }

    WinResize reply;
    reply.m_header.m_windowId = m_id;
    reply.m_size = p;

    m_application->getClientPort()->send(Y_WINDOW_RESIZED, reinterpret_cast<void*>(&reply), sizeof(reply));

    return 0;
}

int Window::closeRequest(void) {
    WinHeader cmd;
    cmd.m_windowId = m_id;

    m_application->getClientPort()->send(Y_WINDOW_CLOSE_REQUEST, reinterpret_cast<void*>(&cmd), sizeof(cmd));

    return 0;
}

int Window::close(void) {
    unregisterWindow();
    return 0;
}

int Window::keyPressed(int key) {
    WinKeyPressed cmd;
    cmd.m_header.m_windowId = m_id;
    cmd.m_key = key;

    m_application->getClientPort()->send(Y_WINDOW_KEY_PRESSED, reinterpret_cast<void*>(&cmd), sizeof(cmd));

    return 0;
}

int Window::keyReleased(int key) {
    WinKeyReleased cmd;
    cmd.m_header.m_windowId = m_id;
    cmd.m_key = key;

    m_application->getClientPort()->send(Y_WINDOW_KEY_RELEASED, reinterpret_cast<void*>(&cmd), sizeof(cmd));

    return 0;
}

int Window::mouseEntered(const yguipp::Point& position) {
    m_mouseOnDecorator = !m_clientRect.hasPoint(position);

    if (m_mouseOnDecorator) {
        m_guiServer->getWindowManager()->getDecorator()->mouseEntered(this, position);
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
            m_guiServer->getWindowManager()->getDecorator()->mouseMoved(this, position);
        } else {
            m_guiServer->getWindowManager()->getDecorator()->mouseExited(this);

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

            m_guiServer->getWindowManager()->getDecorator()->mouseEntered(this, position);
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
        m_guiServer->getWindowManager()->getDecorator()->mouseExited(this);
    } else {
        WinMouseExited cmd;
        cmd.m_header.m_windowId = m_id;

        m_application->getClientPort()->send(Y_WINDOW_MOUSE_EXITED, reinterpret_cast<void*>(&cmd), sizeof(cmd));
    }

    return 0;
}

int Window::mousePressed(const yguipp::Point& position, int button) {
    if (m_mouseOnDecorator) {
        m_guiServer->getWindowManager()->getDecorator()->mousePressed(this, position, button);
    } else {
        WinMousePressed cmd;
        cmd.m_header.m_windowId = m_id;
        cmd.m_position = position - m_clientRect.leftTop();
        cmd.m_button = button;

        m_application->getClientPort()->send(Y_WINDOW_MOUSE_PRESSED, reinterpret_cast<void*>(&cmd), sizeof(cmd));
    }

    m_mousePressedOnDecorator = m_mouseOnDecorator;

    return 0;
}

int Window::mouseReleased(int button) {
    if (m_mousePressedOnDecorator) {
        m_guiServer->getWindowManager()->getDecorator()->mouseReleased(this, button);
    } else {
        WinMouseReleased cmd;
        cmd.m_header.m_windowId = m_id;
        cmd.m_button = button;

        m_application->getClientPort()->send(Y_WINDOW_MOUSE_RELEASED, reinterpret_cast<void*>(&cmd), sizeof(cmd));
    }

    return 0;
}

int Window::activated(yguipp::ActivationReason reason) {
    WinActivated cmd;
    cmd.m_header.m_windowId = m_id;
    cmd.m_reason = reason;

    m_application->getClientPort()->send(Y_WINDOW_ACTIVATED, reinterpret_cast<void*>(&cmd), sizeof(cmd));

    return 0;
}

int Window::deactivated(yguipp::DeActivationReason reason) {
    WinDeActivated cmd;
    cmd.m_header.m_windowId = m_id;
    cmd.m_reason = reason;

    m_application->getClientPort()->send(Y_WINDOW_DEACTIVATED, reinterpret_cast<void*>(&cmd), sizeof(cmd));

    return 0;
}

Window* Window::createFrom( GuiServer* guiServer, Application* application, WinCreate* request ) {
    Window* window = new Window(guiServer, application);
    window->init(request);
    return window;
}

void Window::registerWindow(void) {
    if (m_visible) {
        return;
    }

    m_guiServer->getWindowManager()->registerWindow(this);
    m_visible = true;
}

void Window::unregisterWindow(void) {
    if (!m_visible) {
        return;
    }

    m_guiServer->getWindowManager()->unregisterWindow(this);
    m_visible = false;
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
