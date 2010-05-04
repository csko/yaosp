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

#include <ygui++/application.hpp>
#include <yutil++/thread.hpp>

namespace yguipp {

Application* Application::m_instance = NULL;

Application::Application( const std::string& name ) : m_guiServerPort(NULL) {
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

    return true;
}

yutilpp::IPCPort* Application::getGuiServerPort( void ) {
    return m_guiServerPort;
}

bool Application::createInstance( const std::string& name ) {
    if ( m_instance == NULL ) {
        m_instance = new Application(name);

        if ( !m_instance->init() ) {
            delete m_instance;
            m_instance = NULL;

            return false;
        }
    }

    return true;
}

Application* Application::getInstance( void ) {
    return m_instance;
}

} /* namespace yguipp */
