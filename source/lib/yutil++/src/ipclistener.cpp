/* yaosp IPC listener implementation
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

#include <yutil++/ipclistener.hpp>

namespace yutilpp {

IPCListener::IPCListener( const std::string& name ) : Thread(name), m_port(NULL) {
}

IPCListener::~IPCListener( void ) {
}

bool IPCListener::init( void ) {
    if ( m_port != NULL ) {
        return false;
    }

    m_port = new IPCPort();

    if ( !m_port->createNew() ) {
        delete m_port;
        m_port = NULL;

        return false;
    }

    m_buffer = new uint8_t[BUFFER_SIZE];

    return true;
}

IPCPort* IPCListener::getPort( void ) {
    return m_port;
}

int IPCListener::run( void ) {
    while (1) {
        int ret;
        uint32_t code;

        ret = m_port->receive( code, m_buffer, BUFFER_SIZE );

        if ( ret >= 0 ) {
            ipcDataAvailable( code, reinterpret_cast<void*>(m_buffer), ret );
        }
    }

    return 0;
}

} /* namespace yutilpp */
