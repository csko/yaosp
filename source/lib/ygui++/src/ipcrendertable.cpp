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
#include <ygui/protocol.h>

#include <ygui++/ipcrendertable.hpp>
#include <ygui++/window.hpp>

namespace yguipp {

IPCRenderTable::IPCRenderTable( Window* window ) : RenderTable(window), m_position(sizeof(ipc_port_id)) {
    m_buffer = new uint8_t[BUFFER_SIZE];
}

IPCRenderTable::~IPCRenderTable( void ) {
    delete[] m_buffer;
}

void* IPCRenderTable::allocate( size_t size ) {
    void* p;

    if ( ( m_position + size ) > BUFFER_SIZE ) {
        flush();
        waitForFlush();
    }

    assert( m_position + size < BUFFER_SIZE );

    p = reinterpret_cast<void*>( m_buffer + m_position );
    m_position += size;

    return p;
}

int IPCRenderTable::reset( void ) {
    m_position = sizeof(ipc_port_id);

    return 0;
}

int IPCRenderTable::flush( void ) {
    ipc_port_id* id;

    id = reinterpret_cast<ipc_port_id*>(m_buffer);
    //*id = m_window->getReplyPort()->getId();

    //m_window->getServerPort()->send( MSG_RENDER_COMMANDS, m_buffer, m_position );

    return 0;
}

int IPCRenderTable::waitForFlush( void ) {
    uint32_t code;

    //m_window->getReplyPort()->receive( code );
    m_position = sizeof(ipc_port_id);

    return 0;
}

} /* namespace yguipp */
