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

#include <ygui++/application.hpp>
#include <ygui++/imageloader.hpp>
#include <ygui++/protocol.hpp>
#include <ygui++/window.hpp>

namespace yguipp {

Application* Application::m_instance = NULL;

Application::Application( const std::string& name ) : m_guiServerPort(NULL), m_clientPort(NULL),
                                                      m_serverPort(NULL), m_replyPort(NULL),
                                                      m_mainLoopThread(-1) {
}

Application::~Application( void ) {
}

bool Application::init( void ) {
    m_guiServerPort = new yutilpp::IPCPort();

    while (1) {
        if ( m_guiServerPort->createFromNamed("guiserver") ) {
            break;
        }

        yutilpp::thread::Thread::uSleep( 100 * 1000 );
    }

    m_lock = new yutilpp::thread::Mutex("app lock");
    m_serverPort = new yutilpp::IPCPort();
    m_clientPort = new yutilpp::IPCPort();
    m_clientPort->createNew();
    m_replyPort = new yutilpp::IPCPort();
    m_replyPort->createNew();

    return registerApplication();
}

void Application::addListener(ApplicationListener* listener) {
    for (std::vector<ApplicationListener*>::const_iterator it = m_listeners.begin();
         it != m_listeners.end();
         ++it) {
        if (*it == listener) {
            return;
        }
    }

    m_listeners.push_back(listener);
}

void Application::lock(void) {
    m_lock->lock();
}

void Application::unLock(void) {
    m_lock->unLock();
}

ScreenModeInfo Application::getCurrentScreenMode(int desktopIndex) {
    uint32_t code;
    ScreenModeInfo info;
    DesktopGetSize request;

    request.m_replyPort = m_replyPort->getId();
    request.m_desktopIndex = desktopIndex;

    m_serverPort->send(Y_DESKTOP_GET_SIZE, reinterpret_cast<void*>(&request), sizeof(request));
    m_replyPort->receive(code, reinterpret_cast<void*>(&info), sizeof(info));

    return info;
}

int Application::getScreenModeList(std::vector<ScreenModeInfo>& modeList) {
    uint32_t code;
    size_t size;
    uint8_t* data;
    ScreenModeInfo* modeInfo;
    ScreenModeGetList request;

    request.m_replyPort = m_replyPort->getId();

    m_serverPort->send(Y_SCREEN_MODE_GET_LIST, reinterpret_cast<void*>(&request), sizeof(request));
    m_replyPort->peek(code, size);

    data = new uint8_t[size];
    modeInfo = reinterpret_cast<ScreenModeInfo*>(data);

    m_replyPort->receive(code, data, size);

    for (size_t i = 0; i < (size / sizeof(ScreenModeInfo)); i++, modeInfo++) {
        modeList.push_back(*modeInfo);
    }

    return 0;
}

bool Application::setScreenMode(const ScreenModeInfo& modeInfo) {
    int result;
    uint32_t code;
    ScreenModeSet request;

    request.m_replyPort = m_replyPort->getId();
    request.m_modeInfo = modeInfo;

    m_serverPort->send(Y_SCREEN_MODE_SET, reinterpret_cast<void*>(&request), sizeof(request));
    m_replyPort->receive(code, &result, sizeof(result));

    return (result == 0);
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

int Application::mainLoop(void) {
    Message msg;

    m_mainLoopThread = yutilpp::thread::Thread::currentThread();

    while (!m_windowMap.empty()) {
        if (receiveMessage(msg)) {
            handleMessage(msg);
        }
    }

    m_serverPort->send(Y_APPLICATION_DESTROY, NULL, 0);

    return 0;
}

bool Application::isEventDispatchThread(void) {
    if (m_mainLoopThread == -1) {
        dbprintf("Application: mainLoop not yet started.\n");
        return false;
    }

    return (yutilpp::thread::Thread::currentThread() == m_mainLoopThread);
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

bool Application::receiveMessage(Message& msg) {
    int ret = m_clientPort->receive(msg.m_code, msg.m_buffer, sizeof(msg.m_buffer));

    if (ret < 0) {
        msg.m_size = -1;
        return false;
    }

    msg.m_size = ret;

    return true;
}

bool Application::handleMessage(const Message& msg) {
    switch (msg.m_code) {
        case Y_SCREEN_MODE_CHANGED :
            handleScreenModeChanged(msg.m_buffer);
            break;

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
            const WinHeader* header = reinterpret_cast<const WinHeader*>(msg.m_buffer);
            WindowMapCIter it = m_windowMap.find(header->m_windowId);

            if ( it != m_windowMap.end() ) {
                Window* window = it->second;
                window->handleMessage(msg);
            }

            break;
        }
    }

    return true;
}

bool Application::registerApplication( void ) {
    AppCreate request(m_replyPort->getId(), m_clientPort->getId(), 0);

    if (m_guiServerPort->send(Y_APPLICATION_CREATE, reinterpret_cast<void*>(&request), sizeof(request)) < 0) {
        return false;
    }

    uint32_t code;
    AppCreateReply reply;

    if (m_replyPort->receive(code, reinterpret_cast<void*>(&reply), sizeof(reply)) < 0) {
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

void Application::handleScreenModeChanged(const void* buffer) {
    const ScreenModeInfo* modeInfo = reinterpret_cast<const ScreenModeInfo*>(buffer);

    for (std::vector<ApplicationListener*>::const_iterator it = m_listeners.begin();
         it != m_listeners.end();
         ++it) {
        ApplicationListener* listener = *it;
        listener->onScreenModeChanged(*modeInfo);
    }
}

} /* namespace yguipp */
