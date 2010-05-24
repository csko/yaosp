/* yaosp IPC port implementation
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

#include <yutil++/ipcport.hpp>

namespace yutilpp {

IPCPort::IPCPort( void ) : m_canSend(false), m_canReceive(false), m_id(-1) {
}

IPCPort::~IPCPort( void ) {
}

bool IPCPort::createNew( void ) {
    if ( m_id >= 0 ) {
        return false;
    }

    m_id = create_ipc_port();

    if ( m_id < 0 ) {
        return false;
    }

    m_canSend = true;
    m_canReceive = true;

    return true;
}

bool IPCPort::createFromExisting( ipc_port_id id ) {
    if ( id < 0 ) {
        return false;
    }

    m_id = id;
    m_canSend = true;

    return true;
}

bool IPCPort::createFromNamed( const std::string& name ) {
    if ( get_named_ipc_port( name.c_str(), &m_id ) != 0 ) {
        return false;
    }

    m_canSend = true;

    return true;
}

ipc_port_id IPCPort::getId( void ) {
    return m_id;
}

int IPCPort::send( uint32_t code, void* data, size_t size ) {
    if ( ( m_id < 0 ) ||
         ( !m_canSend ) ) {
        return -1;
    }

    return send_ipc_message( m_id, code, data, size );
}

int IPCPort::receive( uint32_t& code, void* data, size_t maxSize, uint64_t timeOut ) {
    if ( ( m_id < 0 ) ||
         ( !m_canReceive ) ) {
        return -1;
    }

    return recv_ipc_message( m_id, &code, data, maxSize, timeOut );
}

int IPCPort::sendTo( ipc_port_id id, uint32_t code, void* data, size_t size ) {
    return send_ipc_message( id, code, data, size );
}

} /* namespace yutilpp */
