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

#include <assert.h>
#include <iostream>

#include <ygui++/application.hpp>
#include <ygui++/imageloader.hpp>
#include <ygui++/protocol.hpp>
#include <ygui++/window.hpp>
#include <yutil++/thread.hpp>

namespace yguipp {

Application* Application::m_instance = NULL;

Application::Application( const std::string& name ) : m_guiServerPort(NULL), m_clientPort(NULL),
                                                      m_serverPort(NULL), m_replyPort(NULL) {
}

Application::~Application( void ) {
}

bool Application::init( void ) {
    m_guiServerPort = new yutilpp::IPCPort();

    while (1) {
        if ( m_guiServerPort->createFromNamed("guiserver") ) {
            break;
        }

        yutilpp::Thread::uSleep( 100 * 1000 );
    }

    m_lock = new yutilpp::Mutex("app lock");
    m_serverPort = new yutilpp::IPCPort();
    m_clientPort = new yutilpp::IPCPort();
    m_clientPort->createNew();
    m_replyPort = new yutilpp::IPCPort();
    m_replyPort->createNew();

    return registerApplication();
}

void Application::lock( void ) {
    m_lock->lock();
}

void Application::unLock( void ) {
    m_lock->unLock();
}

Point Application::getDesktopSize(int desktopIndex) {
    Point size;
    uint32_t code;
    DesktopGetSize request;
    request.m_replyPort = m_replyPort->getId();
    request.m_desktopIndex = desktopIndex;

    m_serverPort->send(Y_DESKTOP_GET_SIZE, reinterpret_cast<void*>(&request), sizeof(request));
    m_replyPort->receive(code, reinterpret_cast<void*>(&size), sizeof(size));

    return size;
}

yutilpp::IPCPort* Application::getGuiServerPort( void ) {
    return m_guiServerPort;
}

yutilpp::IPCPort* Application::getClientPort( void ) {
    return m_clientPort;
}

yutilpp::IPCPort* Application::getReplyPort( void ) {
    return m_replyPort;
}

int Application::ipcDataAvailable( uint32_t code, void* buffer, size_t size ) {
    switch (code) {
        case Y_WINDOW_SHOW :
        case Y_WINDOW_HIDE :
        case Y_WINDOW_DO_RESIZE :
        case Y_WINDOW_RESIZED :
        case Y_WINDOW_DO_MOVETO :
        case Y_WINDOW_MOVEDTO :
        case Y_WINDOW_KEY_PRESSED :
        case Y_WINDOW_KEY_RELEASED :
        case Y_WINDOW_MOUSE_ENTERED :
        case Y_WINDOW_MOUSE_MOVED :
        case Y_WINDOW_MOUSE_EXITED :
        case Y_WINDOW_MOUSE_PRESSED :
        case Y_WINDOW_MOUSE_RELEASED :
        case Y_WINDOW_ACTIVATED :
        case Y_WINDOW_DEACTIVATED :
        case Y_WINDOW_WIDGET_INVALIDATED :
        case Y_WINDOW_CLOSE_REQUEST : {
            WinHeader* header = reinterpret_cast<WinHeader*>(buffer);
            WindowMapCIter it = m_windowMap.find(header->m_windowId);

            if ( it != m_windowMap.end() ) {
                Window* window = it->second;
                window->handleMessage(code, buffer, size);
            }

            break;
        }
    }

    return 0;
}

int Application::mainLoop(void) {
    while (!m_windowMap.empty()) {
        int ret;
        uint32_t code;

        ret = m_clientPort->receive(code, m_ipcBuffer, IPC_BUF_SIZE);

        if ( ret >= 0 ) {
            ipcDataAvailable(code, m_ipcBuffer, ret);
        }
    }

    // todo: destroy guiserver part of the application

    return 0;
}

bool Application::createInstance( const std::string& name ) {
    if ( m_instance == NULL ) {
        m_instance = new Application(name);

        if ( !m_instance->init() ) {
            delete m_instance;
            m_instance = NULL;

            return false;
        }

        ImageLoaderManager::createInstance();
    }

    return true;
}

bool Application::registerApplication( void ) {
    uint32_t code;
    AppCreate request;
    AppCreateReply reply;

    request.m_replyPort = m_replyPort->getId();
    request.m_clientPort = m_clientPort->getId();
    request.m_flags = 0;

    if ( m_guiServerPort->send( Y_APPLICATION_CREATE, reinterpret_cast<void*>(&request), sizeof(request) ) < 0 ) {
        return false;
    }

    if ( m_replyPort->receive( code, reinterpret_cast<void*>(&reply), sizeof(reply) ) < 0 ) {
        return false;
    }

    m_serverPort->createFromExisting(reply.m_serverPort);

    return true;
}

int Application::registerWindow(int id, Window* window) {
    assert(m_windowMap.find(id) == m_windowMap.end());
    m_windowMap[id] = window;

    return 0;
}

int Application::unregisterWindow(int id) {
    WindowMapIter it = m_windowMap.find(id);
    assert(it != m_windowMap.end());

    m_windowMap.erase(it);

    return 0;
}

} /* namespace yguipp */
