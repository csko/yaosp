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

#include <ygui/protocol.h>

#include <ygui++/application.hpp>
#include <ygui++/imageloader.hpp>
#include <yutil++/thread.hpp>

namespace yguipp {

Application* Application::m_instance = NULL;

Application::Application( const std::string& name ) : IPCListener("application"), m_guiServerPort(NULL),
                                                      m_serverPort(NULL), m_replyPort(NULL) {
}

Application::~Application( void ) {
}

bool Application::init( void ) {
    IPCListener::init();

    m_guiServerPort = new yutilpp::IPCPort();

    while (1) {
        if ( m_guiServerPort->createFromNamed("guiserver") ) {
            break;
        }

        yutilpp::Thread::uSleep( 100 * 1000 );
    }

    m_lock = new yutilpp::Mutex("app lock");
    m_serverPort = new yutilpp::IPCPort();
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

yutilpp::IPCPort* Application::getGuiServerPort( void ) {
    return m_guiServerPort;
}

yutilpp::IPCPort* Application::getServerPort( void ) {
    return m_serverPort;
}

yutilpp::IPCPort* Application::getReplyPort( void ) {
    return m_replyPort;
}

int Application::ipcDataAvailable( uint32_t code, void* buffer, size_t size ) {
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

Application* Application::getInstance( void ) {
    return m_instance;
}

bool Application::registerApplication( void ) {
    uint32_t code;
    msg_create_app_t request;
    msg_create_app_reply_t reply;

    request.reply_port = m_replyPort->getId();
    request.client_port = getPort()->getId();
    request.flags = 0;

    if ( m_guiServerPort->send( MSG_APPLICATION_CREATE, reinterpret_cast<void*>(&request), sizeof(msg_create_app_t) ) < 0 ) {
        return false;
    }

    if ( m_replyPort->receive( code, reinterpret_cast<void*>(&reply), sizeof(msg_create_app_reply_t) ) < 0 ) {
        return false;
    }

    m_serverPort->createFromExisting(reply.server_port);

    return true;
}

} /* namespace yguipp */
