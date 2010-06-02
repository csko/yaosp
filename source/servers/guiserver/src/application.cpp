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

#include <guiserver/application.hpp>
#include <guiserver/window.hpp>

Application::Application( GuiServer* guiServer ) : IPCListener("app"), m_clientPort(NULL), m_nextWinId(0),
                                                   m_guiServer(guiServer) {
}

Application::~Application( void ) {
}

bool Application::init( AppCreate* request ) {
    AppCreateReply reply;

    IPCListener::init();

    m_clientPort = new yutilpp::IPCPort();
    m_clientPort->createFromExisting(request->m_clientPort);

    reply.m_serverPort = getPort()->getId();
    yutilpp::IPCPort::sendTo(request->m_replyPort, 0, reinterpret_cast<void*>(&reply), sizeof(reply));

    return true;
}

int Application::ipcDataAvailable( uint32_t code, void* data, size_t size ) {
    switch (code) {
        case Y_WINDOW_CREATE :
            handleWindowCreate( reinterpret_cast<WinCreate*>(data) );
            break;

        case Y_WINDOW_SHOW :
        case Y_WINDOW_HIDE :
        case Y_WINDOW_RENDER : {
            WinHeader* header = reinterpret_cast<WinHeader*>(data);
            WindowMapCIter it = m_windowMap.find(header->m_windowId);

            if ( it != m_windowMap.end() ) {
                it->second->handleMessage(code, data, size);
            }

            break;
        }
    }

    return 0;
}

Application* Application::createFrom( GuiServer* guiServer, AppCreate* request ) {
    Application* app = new Application(guiServer);
    app->init(request);
    return app;
}

int Application::handleWindowCreate( WinCreate* request ) {
    WinCreateReply reply;
    Window* win = Window::createFrom(m_guiServer, request);

    reply.m_windowId = getWindowId();
    m_windowMap[reply.m_windowId] = win;

    yutilpp::IPCPort::sendTo(request->m_replyPort, 0, reinterpret_cast<void*>(&reply), sizeof(reply));

    return 0;
}

int Application::getWindowId( void ) {
    while ( m_windowMap.find(m_nextWinId) != m_windowMap.end() ) {
        m_nextWinId++;

        if ( m_nextWinId < 0 ) {
            m_nextWinId = 0;
        }
    }

    return m_nextWinId;
}
